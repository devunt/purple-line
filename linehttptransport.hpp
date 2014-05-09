#pragma once

#include <functional>
#include <string>
#include <sstream>
#include <queue>

#include <stdint.h>

#include <account.h>
#include <sslconn.h>

#include <thrift/transport/TTransport.h>

#include "wrapper.hpp"

class LineHttpTransport : public apache::thrift::transport::TTransport {

    class Request {
    public:
        std::string method;
        std::string path;
        std::string data;
        std::function<void()> callback;
    };

    static const size_t BUFFER_SIZE = 4096;

    PurpleAccount *acct;
    PurpleConnection *conn;

    std::string host;
    uint16_t port;
    bool ls_mode;
    std::string auth_token;
    std::string x_ls;

    PurpleSslConnection *ssl;
    int connection_id;

    uint8_t buf[BUFFER_SIZE];

    std::stringbuf request_buf;

    bool in_progress;
    std::string response_str;
    std::stringbuf response_buf;

    std::queue<Request> request_queue;

    bool connection_close;
    int status_code_;
    int content_length_;

public:

    LineHttpTransport(PurpleAccount *acct, PurpleConnection *conn,
        std::string host, uint16_t port,
        bool plain_http);
    ~LineHttpTransport();

    void set_auth_token(std::string token);

    virtual void open();
    virtual void close();

    virtual uint32_t read_virt(uint8_t *buf, uint32_t len);
    void write_virt(const uint8_t *buf, uint32_t len);

    void request(std::string method, std::string path, std::function<void()> callback);
    int status_code();
    int content_length();

    //virtual const uin8_t* borrow_virt(uint8_t *buf, uint32_t *len);
    //virtual void consume_virt(uint32_t len);

//private:

    void ssl_connect(PurpleSslConnection *, PurpleInputCondition);
    void ssl_input(PurpleSslConnection *, PurpleInputCondition cond);
    void ssl_error(PurpleSslConnection *, PurpleSslErrorType err);

private:

    void send_next();
    int reconnect();

    void try_parse_response_header();
};
