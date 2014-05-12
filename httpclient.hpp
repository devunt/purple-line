#pragma once

#include <string>
#include <functional>
#include <list>

#include <account.h>
#include <util.h>

class HTTPClient {
    const int MAX_IN_FLIGHT = 4;

    using CompleteFunc = std::function<void(int, const guchar *, gsize)>;

    struct Request {
        HTTPClient *client;
        std::string url;
        bool auth;
        CompleteFunc callback;
        PurpleUtilFetchUrlData *handle;
    };

    PurpleAccount *acct;

    std::string auth_token;

    std::list<Request *> request_queue;
    int in_flight;

    void request_core(std::string url, CompleteFunc callback, bool auth);

    void execute_next();
    void parse_response(const char *res, int &status, const guchar *&body);
    void complete(Request *req, const gchar *url_text, gsize len, const gchar *error_message);

    static void purple_cb(PurpleUtilFetchUrlData *url_data, gpointer user_data,
        const gchar *url_text, gsize len, const gchar *error_message);

public:

    HTTPClient(PurpleAccount *acct);
    ~HTTPClient();

    void set_auth_token(std::string token);

    void request(std::string url, CompleteFunc callback);
    void request_auth(std::string url, CompleteFunc callback);

};
