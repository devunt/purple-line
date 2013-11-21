#pragma once

#include <functional>
#include <string>
#include <sstream>
#include <deque>

#include <stdint.h>

#include <account.h>
#include <sslconn.h>

#include <transport/TTransport.h>

class PurpleHttpClient : public apache::thrift::transport::TTransport {

    class Request {
    public:
        std::string data;
        std::function<void(int)> callback;
    };

    static const size_t BUFFER_SIZE = 4096;

    PurpleAccount *acct;

    std::string host;
    uint16_t port;
    std::string path;
    std::string auth_token;
    std::string x_ls;

    PurpleSslConnection *ssl;
    int connection_id;
    bool first_request;

    uint8_t buf[BUFFER_SIZE];

    std::stringbuf request_buf;

    bool in_progress;
    int status_code;
    int content_length;
    std::string response_str;
    std::stringbuf response_buf;

    std::deque<Request> request_queue;

public:

    PurpleHttpClient(PurpleAccount *acct, std::string host, uint16_t port, std::string path);
    ~PurpleHttpClient();

    void set_path(std::string path);
    void set_auth_token(std::string token);

    virtual void open();
    virtual void close();

    virtual uint32_t read_virt(uint8_t *buf, uint32_t len);
    void write_virt(const uint8_t *buf, uint32_t len);

    virtual void send(std::function<void(int)> callback);

    //virtual const uin8_t* borrow_virt(uint8_t *buf, uint32_t *len);
    //virtual void consume_virt(uint32_t len);

//private:

    void ssl_connect();
    void ssl_input(PurpleInputCondition cond);
    void ssl_error(PurpleSslErrorType err);

private:

    void send_next();

    void try_parse_response_header();
};
