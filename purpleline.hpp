#pragma once

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <transport/TSSLSocket.h>
#include <transport/THttpClient.h>
#include <protocol/TCompactProtocol.h>
#include "thrift_line/Line.h"
#include "thrift_line/line_types.h"
#include "thrift_line/line_constants.h"
#include "debug.h"
#include "plugin.h"
#include "prpl.h"

#define LINEPRPL_ID "prpl-mvirkkunen-line"

class PurpleLine {

    PurpleConnection *conn;
    PurpleAccount *acct;

    boost::shared_ptr<apache::thrift::transport::TSSLSocket> thrift_socket;
    boost::shared_ptr<apache::thrift::transport::THttpClient> thrift_transport;
    boost::shared_ptr<apache::thrift::protocol::TCompactProtocol> thrift_protocol;

    boost::shared_ptr<line::LineClient> client;

public:
    PurpleLine(PurpleConnection *conn, PurpleAccount *acct);

    static const char *list_icon(PurpleBuddy *buddy);
    static GList *status_types();

    void login();
    void close();

};
