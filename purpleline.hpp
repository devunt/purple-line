#pragma once

#include <string>
#include <deque>

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

    line::Profile profile;
    int64_t local_rev;

    std::deque<std::string> recent_messages;

public:

    PurpleLine(PurpleConnection *conn, PurpleAccount *acct);

    static const char *list_icon(PurpleBuddy *buddy);
    static GList *status_types();
    static GList *chat_info();
    static char *get_chat_name(GHashTable *components);

    void login();
    void close();

    int send_im(const char *who, const char *message, PurpleMessageFlags flags);

private:

    // Login process methods, executed in this order.
    void get_last_op_revision();
    void get_profile();
    void get_contacts();
    void get_groups();

    // Long poll return channel
    void fetch_operations();
    void handle_message(line::Message &msg, bool sent);

    void push_recent_message(std::string id);

};
