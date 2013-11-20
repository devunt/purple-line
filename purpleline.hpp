#pragma once

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include "debug.h"
#include "plugin.h"
#include "prpl.h"

#include "thrift_line/Line.h"
#include "thrift_line/line_types.h"
#include "thrift_line/line_constants.h"

#include "purplehttpclient.hpp"

#define LINEPRPL_ID "prpl-mvirkkunen-line"

class PurpleLine {

    PurpleConnection *conn;
    PurpleAccount *acct;

    boost::shared_ptr<PurpleHttpClient> http_in, http_out;

    boost::shared_ptr<line::LineClient> client_in, client_out;

public:

    PurpleLine(PurpleConnection *conn, PurpleAccount *acct);

    static const char *list_icon(PurpleBuddy *buddy);
    static GList *status_types();

    void login();
    void close();

private:

    void login_complete(int status);
    void get_profile_complete(int status);

};
