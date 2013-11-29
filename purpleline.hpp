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

template <typename T>
struct _blist_node_type { };

template <>
struct _blist_node_type<PurpleBuddy> {
    static const PurpleBlistNodeType type = PURPLE_BLIST_BUDDY_NODE;
    static PurpleAccount *get_account(PurpleBuddy *buddy) {
        return purple_buddy_get_account(buddy);
    }
};

template <>
struct _blist_node_type<PurpleChat> {
    static const PurpleBlistNodeType type = PURPLE_BLIST_CHAT_NODE;
    static PurpleAccount *get_account(PurpleChat *chat) {
        return purple_chat_get_account(chat);
    }
};

class PurpleLine {

    enum class ChatType {
        GROUP = 1,
        ROOM = 2,
    };

    static std::map<ChatType, std::string> chat_type_to_string;

    PurpleConnection *conn;
    PurpleAccount *acct;

    boost::shared_ptr<ThriftClient> c_out, c_in;
    boost::shared_ptr<LineHttpTransport> http_os;

    int64_t local_rev;

    int next_purple_id;

    std::deque<std::string> recent_messages;

    line::Profile profile;
    std::map<std::string, line::Group> groups;
    std::map<std::string, line::Room> rooms;
    std::map<std::string, line::Contact> contacts;

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
    void chat_leave(int id);
    int chat_send(int id, const char *message, PurpleMessageFlags flags);

private:

    // Login process methods, executed in this this order
    void start_login();
    void get_last_op_revision();
    void get_profile();
    void get_contacts();
    void get_groups();
    void get_rooms();

    // Long poll return channel
    void fetch_operations();
    void handle_message(line::Message &msg, bool sent, bool replay);

    template <typename T>
    std::set<T *> blist_find(std::function<bool(T *)> predicate) {
        std::set<T *> results;

        for (PurpleBlistNode *node = purple_blist_get_root();
            node;
            node = purple_blist_node_next(node, FALSE))
        {
            if (_blist_node_type<T>::get_account((T *)node) == acct
                && purple_blist_node_get_type(node) == _blist_node_type<T>::type
                && predicate((T *)node))
            {
                results.insert((T *)node);
            }
        }

        return results;
    }

    template <typename T>
    std::set<T *> blist_find() {
        return blist_find<T>([](T *){ return true; });
    }

    PurpleGroup *blist_ensure_group(std::string group_name, bool temporary=false);
    PurpleBuddy *blist_ensure_buddy(std::string uid, bool temporary=false);
    void blist_update_buddy(std::string uid, bool temporary=false);
    PurpleBuddy *blist_update_buddy(line::Contact &contact, bool temporary=false);
    bool blist_is_buddy_in_any_conversation(std::string uid, PurpleConvChat *ignore_chat);
    void blist_remove_buddy(std::string uid,
        bool temporary_only=false, PurpleConvChat *ignore_chat=nullptr);

    std::set<PurpleChat *> blist_find_chats_by_type(ChatType type);
    PurpleChat *blist_find_chat(std::string id, ChatType type);
    PurpleChat *blist_ensure_chat(std::string id, ChatType type);
    void blist_update_chat(std::string id, ChatType type);
    PurpleChat *blist_update_chat(line::Group &group);
    PurpleChat *blist_update_chat(line::Room &room);
    void blist_remove_chat(std::string id, ChatType type);

    void set_chat_participants(PurpleConvChat *chat, line::Room &room);
    void set_chat_participants(PurpleConvChat *chat, line::Group &group);


    int send_message(std::string to, std::string text);
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
