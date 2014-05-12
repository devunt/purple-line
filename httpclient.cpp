#include <sstream>
#include <string.h>

#include <debug.h>

#include "constants.hpp"
#include "httpclient.hpp"

HTTPClient::HTTPClient(PurpleAccount *acct) :
    acct(acct),
    in_flight(0)
{
}

HTTPClient::~HTTPClient() {
    for (Request *r: request_queue) {
        if (r->handle)
            purple_util_fetch_url_cancel(r->handle);
    }
}

void HTTPClient::set_auth_token(std::string token) {
    this->auth_token = token;
}

void HTTPClient::request(std::string url, HTTPClient::CompleteFunc callback) {
    request_core(url, callback, false);
}

void HTTPClient::request_auth(std::string url, HTTPClient::CompleteFunc callback) {
    request_core(url, callback, true);
}

void HTTPClient::request_core(std::string url, HTTPClient::CompleteFunc callback, bool auth) {
    Request *req = new Request();
    req->client = this;
    req->url = url;
    req->auth = auth;
    req->callback = callback;
    req->handle = nullptr;

    request_queue.push_back(req);

    execute_next();
}

void HTTPClient::execute_next() {
    while (in_flight < MAX_IN_FLIGHT && request_queue.size() > 0) {
        Request *req = request_queue.front();
        request_queue.pop_front();

        std::stringstream ss;

        char *host, *path;
        int port;

        purple_url_parse(req->url.c_str(), &host, &port, &path, nullptr, nullptr);

        ss
            << "GET /" << path << " HTTP/1.1" "\r\n"
            << "Connection: close\r\n"
            << "Host: " << host << ":" << port << "\r\n"
            << "User-Agent: " << LINE_USER_AGENT << "\r\n";

        free(host);
        free(path);

        if (req->auth) {
            ss
                << "X-Line-Application: " << LINE_APPLICATION << "\r\n"
                << "X-Line-Access: " << auth_token << "\r\n";
        }

        ss << "\r\n";

        in_flight++;

        req->handle = purple_util_fetch_url_request_len_with_account(
            acct,
            req->url.c_str(),
            TRUE,
            LINE_USER_AGENT,
            TRUE,
            ss.str().c_str(),
            TRUE,
            -1,
            purple_cb,
            (gpointer)req);
    }
}

void HTTPClient::parse_response(const char *res, int &status, const guchar *&body) {
    // libpurple guarantees that responses are null terminated even if they're binary, so string
    // functions are safe to use.

    const char *status_end = strstr(res, "\r\n");
    if (!status_end)
        return;

    const char *header_end = strstr(res, "\r\n\r\n");
    if (!header_end)
        return;

    std::stringstream ss(std::string(res, status_end - res));
    ss.ignore(255, ' ');

    ss >> status;

    body = (const guchar *)(header_end + 4);
}

void HTTPClient::complete(HTTPClient::Request *req,
    const gchar *url_text, gsize len, const gchar *error_message)
{
    if (!url_text || error_message) {
        purple_debug_error("util", "HTTP error: %s\n", error_message);
        req->callback(-1, nullptr, 0);
    } else {
        // HTTP/1.1 200 OK

        int status;
        const guchar *body = nullptr;

        parse_response(url_text, status, body);

        req->callback(status, body, len);
    }

    request_queue.remove(req);
    delete req;

    in_flight--;

    execute_next();
}

void HTTPClient::purple_cb(PurpleUtilFetchUrlData *url_data, gpointer user_data,
    const gchar *url_text, gsize len, const gchar *error_message)
{
    Request *req = (Request *)user_data;

    req->client->complete(req, url_text, len, error_message);
}
