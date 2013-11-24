#pragma once

#include <string>
#include <deque>

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include <debug.h>
#include <plugin.h>
#include <prpl.h>

#include <protocol/TCompactProtocol.h>

#include "thrift_line/Line.h"
#include "thrift_line/line_types.h"
#include "thrift_line/line_constants.h"

#include "linehttptransport.hpp"

#define LINEPRPL_ID "prpl-mvirkkunen-line"

class ThriftClient;

class PurpleLine {

    PurpleConnection *conn;
    PurpleAccount *acct;

    boost::shared_ptr<ThriftClient> c_out, c_in;
    boost::shared_ptr<LineHttpTransport> http_os;

    line::Profile profile;
    int64_t local_rev;

    int next_id;
    std::map<int, std::string> chat_purple_id_to_id;
    std::map<std::string, int> chat_id_to_purple_id;

    std::deque<std::string> recent_messages;

    // Latest information about each seen group
    std::map<std::string, line::Group> group_map;

public:

    PurpleLine(PurpleConnection *conn, PurpleAccount *acct);
    ~PurpleLine();

    static const char *list_icon(PurpleAccount *, PurpleBuddy *buddy);
    static GList *status_types(PurpleAccount *);
    static char *get_chat_name(GHashTable *components);
    static char *status_text(PurpleBuddy *buddy);
    static void tooltip_text(PurpleBuddy *buddy, PurpleNotifyUserInfo *user_info, gboolean full);
    static void login(PurpleAccount *acct);

    void close();
    GList *chat_info();
    int send_im(const char *who, const char *message, PurpleMessageFlags flags);
    void join_chat(GHashTable *components);
    int chat_send(int id, const char *message, PurpleMessageFlags flags);

private:

    // Login process methods, executed in this this order
    void start_login();
    void get_last_op_revision();
    void get_profile();
    void get_contacts();
    void get_groups();

    // Long poll return channel
    void fetch_operations();
    void handle_message(line::Message &msg, bool sent, bool replay);

    PurpleChat *blist_find_chat_by_id(std::string id);
    PurpleGroup *blist_ensure_group(std::string group_name, bool temporary=false);
    PurpleBuddy *blist_ensure_buddy(std::string uid, bool temporary=false);
    void blist_update_buddy(std::string uid, bool temporary=false);
    void blist_update_buddy(line::Contact contact, bool temporary=false);
    bool blist_is_buddy_in_any_conversation(std::string uid);
    void blist_remove_buddy(std::string uid);

    void set_chat_participants(PurpleConvChat *chat, line::Group &group);

    int send_message(std::string to, int chat_purple_id, std::string text);
    void push_recent_message(std::string id);

};

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
