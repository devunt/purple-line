#pragma once

#include <string>
#include <deque>

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include <debug.h>
#include <plugin.h>
#include <prpl.h>

#include <thrift/protocol/TCompactProtocol.h>

#include "thrift_line/Line.h"
#include "thrift_line/line_types.h"
#include "thrift_line/line_constants.h"

#include "linehttptransport.hpp"

class ThriftProtocol : public apache::thrift::protocol::TCompactProtocolT<LineHttpTransport> {

public:

    ThriftProtocol(boost::shared_ptr<LineHttpTransport> trans);

    LineHttpTransport *getTransport();

};

class ThriftClient : public line::LineClientT<ThriftProtocol> {

    std::string path;
    LineHttpTransport *http;

public:

    ThriftClient(PurpleAccount *acct, PurpleConnection *conn, std::string path);

    void set_auth_token(std::string token);
    void send(std::function<void()> callback);

    int status_code();
    void close();

};
