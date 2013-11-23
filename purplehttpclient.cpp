#include "purplehttpclient.hpp"

#include <sstream>
#include <limits>

#include <debug.h>

#include <thrift/transport/TTransportException.h>

PurpleHttpClient::PurpleHttpClient(
        PurpleAccount *acct,
        PurpleConnection *conn,
        std::string host,
        uint16_t port,
        std::string default_path) :
    acct(acct),
    conn(conn),
    host(host),
    port(port),
    default_path(default_path),
    auth_token("x"),
    ssl(NULL),
    connection_id(0),
    status_code_(0),
    content_length_(0)
{
}

PurpleHttpClient::~PurpleHttpClient() { }

void PurpleHttpClient::set_auth_token(std::string token) {
    this->auth_token = token;
}

int PurpleHttpClient::status_code() {
    return status_code_;
}

int PurpleHttpClient::content_length() {
    return content_length_;
}

void PurpleHttpClient::open() {
    if (ssl)
        return;

    in_progress = false;
    first_request = true;

    connection_id++;
    ssl = purple_ssl_connect(
        acct,
        host.c_str(),
        port,
        WRAPPER(PurpleHttpClient::ssl_connect),
        WRAPPER_AT(PurpleHttpClient::ssl_error, end),
        (gpointer)this);
}

void PurpleHttpClient::close() {
    if (!ssl)
        return;

    purple_ssl_close(ssl);
    ssl = NULL;
    connection_id++;

    x_ls = "";

    request_buf.str("");

    response_str = "";
    response_buf.str("");
}

uint32_t PurpleHttpClient::read_virt(uint8_t *buf, uint32_t len) {
    return (uint32_t )response_buf.sgetn((char *)buf, len);
}

void PurpleHttpClient::write_virt(const uint8_t *buf, uint32_t len) {
    request_buf.sputn((const char *)buf, len);
}

void PurpleHttpClient::send(std::function<void()> callback) {
    send("", callback);
}

void PurpleHttpClient::send(std::string path, std::function<void()> callback) {
    Request req;
    req.path = (path == "" ? default_path : path);
    req.data = request_buf.str();
    req.callback = callback;
    request_queue.push(req);

    request_buf.str("");

    send_next();
}

/*const uin8_t* PurpleHttpClient::borrow_virt(uint8_t *buf, uint32_t *len) {
}

void PurpleHttpClient::consume_virt(uint32_t len) {
}*/

void PurpleHttpClient::send_next() {
    if (!ssl) {
        open();
        return;
    }

    if (in_progress || request_queue.empty())
        return;

    status_code_ = -1;
    content_length_ = -1;

    Request &next_req = request_queue.front();

    std::ostringstream data;

    data
        << "POST " << next_req.path << " HTTP/1.1" "\r\n"
        << "Content-Length: " << next_req.data.size() << "\r\n";

    if (x_ls.size() > 0)
        data << "X-LS: " << x_ls << "\r\n";

    if (first_request) {
        data
            << "Connection: Keep-Alive" "\r\n"
            << "Content-Type: application/x-thrift" "\r\n"
            << "Host: " << host << ":" << port << "\r\n"
            << "Accept: application/x-thrift" "\r\n"
            << "User-Agent: purple-line (LINE for libpurple)" "\r\n"
            << "X-Line-Application: DESKTOPWIN\t3.2.1.83\tWINDOWS\t5.1.2600-XP-x64" "\r\n"
            << "X-Line-Access: " << auth_token << "\r\n";

        first_request = false;
    }

    data
        << "\r\n"
        << next_req.data;

    std::string data_str = data.str();

    in_progress = true;
    purple_ssl_write(ssl, data_str.c_str(), data_str.size());
}

int PurpleHttpClient::reconnect() {
    close();

    in_progress = false;

    open();

    // Don't repeat when using as timeout callback
    return FALSE;
}

void PurpleHttpClient::ssl_connect(PurpleSslConnection *, PurpleInputCondition) {
    purple_ssl_input_add(ssl, WRAPPER(PurpleHttpClient::ssl_input), (gpointer)this);

    send_next();
}

void PurpleHttpClient::ssl_input(PurpleSslConnection *, PurpleInputCondition cond) {
    if (cond != PURPLE_INPUT_READ)
        return;

    bool any = false;

    while (true) {
        size_t count = purple_ssl_read(ssl, (void *)buf, BUFFER_SIZE);

        if (count == 0) {
            if (any)
                break;

            // Disconnected from server.

            purple_debug_info("line", "A connection died.\n");

            close();

            // If there was a request in progress, re-open immediately to try again.
            if (in_progress) {
                purple_debug_info("line", "Reconnecting immediately to re-send.\n");

                open();
            }

            return;


                /*conn->wants_to_die = TRUE;
                purple_connection_error(conn, "Lost connection to server.");
            }

            // Probably just the keep-alive connection timing out, so mark client as closed.

            //purple_connection_error(conn, "Keep-alive connection died.");

            purple_debug_info("line", "Connection died.");

            close();*/

            /*reconnect_count++;

            // First disconnection could be just the keep-alive connection timing out. Try to
            // reconnect immediately.

            if (reconnect_count == 1) {
                purple_debug_info("line", "Reconnecting immediately.");

                reconnect();
                return;
            }

            // Otherwise try to connect again after a timeout

            if (reconnect_count == 2) {
                purple_debug_info("line", "Reconnecting after a timeout.");

                purple_timeout_add_seconds(
                    5,
                    WRAPPER(PurpleHttpClient::reconnect),
                    (gpointer)this);

                return;
            }

            purple_debug_info("line", "Connection broken :(");

            // Connection seems to be broken.

            conn->wants_to_die = TRUE;
            purple_connection_error(conn, "Could not connect to server.");*/

            return;
        }

        if (count == (size_t)-1)
            break;

        any = true;

        response_str.append((const char *)buf, count);

        try_parse_response_header();

        if (content_length_ >= 0 && response_str.size() >= (size_t)content_length_) {
            if (status_code_ == 403) {
                // Don't try to reconnect because this usually means the user has logged in from
                // elsewhere.
                // TODO: Check actual reason

                conn->wants_to_die = TRUE;
                purple_connection_error(conn, "Session died.");
                return;
            }

            response_buf.str(response_str.substr(0, content_length_));
            response_str.erase(0, content_length_);

            int connection_id_before = connection_id;

            request_queue.front().callback();
            request_queue.pop();

            in_progress = false;

            if (connection_id != connection_id_before)
                break; // Callback closed connection, don't try to continue reading

            send_next();
        }
    }
}

void PurpleHttpClient::ssl_error(PurpleSslConnection *, PurpleSslErrorType err) {
    purple_debug_warning("line", "SSL error: %s\n", purple_ssl_strerror(err));

    ssl = nullptr;

    purple_connection_ssl_error(conn, err);
}

void PurpleHttpClient::try_parse_response_header() {
    size_t header_end = response_str.find("\r\n\r\n");
    if (header_end == std::string::npos)
        return;

    std::istringstream stream(response_str.substr(0, header_end));

    stream.ignore(256, ' ');
    stream >> status_code_;
    stream.ignore(256, '\n');

    while (stream) {
        std::string name, value;

        std::getline(stream, name, ':');
        stream.ignore(256, ' ');

        if (name == "Content-Length")
            stream >> content_length_;

        if (name == "X-LS")
            std::getline(stream, x_ls, '\r');

        stream.ignore(256, '\n');
    }

    response_str.erase(0, header_end + 4);
}
