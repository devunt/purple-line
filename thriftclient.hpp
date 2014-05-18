#pragma once

#include <string>
#include <deque>

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include <debug.h>
#include <plugin.h>
#include <prpl.h>

#include "thrift_line/TalkService.h"

#include "linehttptransport.hpp"

class ThriftClient : public line::TalkServiceClient {

    std::string path;
    boost::shared_ptr<LineHttpTransport> http;

public:

    ThriftClient(PurpleAccount *acct, PurpleConnection *conn, std::string path);

    void set_path(std::string path);
    void set_auth_token(std::string token);
    void send(std::function<void()> callback);

    int status_code();
    void close();

};
