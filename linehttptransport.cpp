#include <sstream>
#include <limits>

#include <debug.h>

#include <thrift/transport/TTransportException.h>

#include "thrift_line/TalkService.h"

#include "constants.hpp"
#include "linehttptransport.hpp"

LineHttpTransport::LineHttpTransport(
        PurpleAccount *acct,
        PurpleConnection *conn,
        std::string host,
        uint16_t port,
        bool ls_mode) :
    acct(acct),
    conn(conn),
    host(host),
    port(port),
    ls_mode(ls_mode),
    auth_token(""),
    ssl(NULL),
    connection_id(0),
    connection_close(false),
    status_code_(0),
    content_length_(0)
{
}

LineHttpTransport::~LineHttpTransport() {
    close();
}

void LineHttpTransport::set_auth_token(std::string token) {
    this->auth_token = token;
}

int LineHttpTransport::status_code() {
    return status_code_;
}

int LineHttpTransport::content_length() {
    return content_length_;
}

void LineHttpTransport::open() {
    if (ssl)
        return;

    in_progress = false;

    connection_id++;
    ssl = purple_ssl_connect(
        acct,
        host.c_str(),
        port,
        WRAPPER(LineHttpTransport::ssl_connect),
        WRAPPER_TYPE(LineHttpTransport::ssl_error, end),
        (gpointer)this);
}

void LineHttpTransport::close() {
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

uint32_t LineHttpTransport::read_virt(uint8_t *buf, uint32_t len) {
    return (uint32_t )response_buf.sgetn((char *)buf, len);
}

void LineHttpTransport::write_virt(const uint8_t *buf, uint32_t len) {
    request_buf.sputn((const char *)buf, len);
}

void LineHttpTransport::request(std::string method, std::string path, std::function<void()> callback) {
    Request req;
    req.method = method;
    req.path = path;
    req.data = request_buf.str();
    req.callback = callback;
    request_queue.push(req);

    request_buf.str("");

    send_next();
}

/*const uin8_t* LineHttpTransport::borrow_virt(uint8_t *buf, uint32_t *len) {
}

void LineHttpTransport::consume_virt(uint32_t len) {
}*/

void LineHttpTransport::send_next() {
    if (!ssl) {
        open();
        return;
    }

    if (in_progress || request_queue.empty())
        return;

    connection_close = false;
    status_code_ = -1;
    content_length_ = -1;

    Request &next_req = request_queue.front();

    std::ostringstream data;

    data
        << next_req.method << " " << next_req.path << " HTTP/1.1" "\r\n";

    if (next_req.method == "POST")
        data << "Content-Length: " << next_req.data.size() << "\r\n";

    if (ls_mode) {
        if (x_ls != "")
        {
            data << "X-LS: " << x_ls << "\r\n";
        }
        else
        {
            data
                << "Connection: Keep-Alive" "\r\n"
                << "Content-Type: application/x-thrift" "\r\n"
                << "Host: " << host << ":" << port << "\r\n"
                << "Accept: application/x-thrift" "\r\n"
                << "User-Agent: " LINE_USER_AGENT "\r\n"
                << "X-Line-Application: " LINE_APPLICATION "\r\n";

            if (auth_token != "")
                data << "X-Line-Access: " << auth_token << "\r\n";
        }
    } else {
        data
            << "Connection: Keep-Alive" "\r\n"
            << "Host: " << host << ":" << port << "\r\n"
            << "User-Agent: " LINE_USER_AGENT "\r\n"
            << "X-Line-Application: " LINE_APPLICATION "\r\n";

        if (auth_token != "")
            data << "X-Line-Access: " << auth_token << "\r\n";
    }

    data
        << "\r\n"
        << next_req.data;

    std::string data_str = data.str();

    in_progress = true;
    purple_ssl_write(ssl, data_str.c_str(), data_str.size());
}

int LineHttpTransport::reconnect() {
    close();

    in_progress = false;

    open();

    // Don't repeat when using as timeout callback
    return FALSE;
}

void LineHttpTransport::ssl_connect(PurpleSslConnection *, PurpleInputCondition) {
    purple_ssl_input_add(ssl, WRAPPER(LineHttpTransport::ssl_input), (gpointer)this);

    send_next();
}

void LineHttpTransport::ssl_input(PurpleSslConnection *, PurpleInputCondition cond) {
    if (!ssl || cond != PURPLE_INPUT_READ)
        return;

    bool any = false;

    while (true) {
        size_t count = purple_ssl_read(ssl, (void *)buf, BUFFER_SIZE);

        if (count == 0) {
            if (any)
                break;

            // Disconnected from server.

            close();

            // If there was a request in progress, re-open immediately to try again.
            if (in_progress) {
                purple_debug_info("line", "Reconnecting immediately to re-send.\n");

                open();
            }

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

            try {
                request_queue.front().callback();
            } catch (line::TalkException &err) {
                std::string msg = "LINE: TalkException: ";
                msg += err.reason;

                if (err.code == line::ErrorCode::NOT_AUTHORIZED_DEVICE) {
                    if (err.reason == "DEVICE_LOSE" || err.reason == "EXPIRED") {
                        purple_account_set_string(acct, LINE_ACCOUNT_CERTIFICATE, "");
                        msg = "LINE: Your certificate had expired. Please re-enable the LINE account.";
                    } else if (err.reason == "AUTHENTICATION_DIVESTED_BY_OTHER_DEVICE") {
                        msg = "LINE: You have been logged out because "
                            "you logged in from another device.";
                    } else if (err.reason == "REVOKE") {
                        msg = "LINE: This device was logged out via the mobile app.";
                    }
                }

                conn->wants_to_die = TRUE;
                purple_connection_error(conn, msg.c_str());
                return;
            } catch (apache::thrift::TApplicationException &err) {
                std::string msg = "LINE: Application error: ";
                msg += err.what();

                conn->wants_to_die = TRUE;
                purple_connection_error(conn, msg.c_str());
                return;
            } catch (apache::thrift::transport::TTransportException &err) {
                std::string msg = "LINE: Transport error: ";
                msg += err.what();

                conn->wants_to_die = TRUE;
                purple_connection_error(conn, msg.c_str());
                return;
            }

            request_queue.pop();

            in_progress = false;

            if (connection_id != connection_id_before)
                break; // Callback closed connection, don't try to continue reading

            if (connection_close) {
                close();
                send_next();
                break;
            }

            send_next();
        }
    }
}

void LineHttpTransport::ssl_error(PurpleSslConnection *, PurpleSslErrorType err) {
    purple_debug_warning("line", "SSL error: %s\n", purple_ssl_strerror(err));

    ssl = nullptr;

    purple_connection_ssl_error(conn, err);
}

void LineHttpTransport::try_parse_response_header() {
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

        if (name == "Connection") {
            std::string value;
            std::getline(stream, value, '\r');

            if (value == "Close" || value == "close")
                connection_close = true;
        }

        stream.ignore(256, '\n');
    }

    response_str.erase(0, header_end + 4);
}
