#pragma once

#include <string>
#include <deque>

#include <cmds.h>
#include <debug.h>
#include <plugin.h>
#include <prpl.h>

#include "constants.hpp"
#include "thriftclient.hpp"
#include "httpclient.hpp"
#include "poller.hpp"
#include "pinverifier.hpp"

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

enum class ChatType {
    ANY = 0,
    GROUP = 1,
    ROOM = 2,
    GROUP_INVITE = 3,
};

class PurpleLine {

    static std::map<ChatType, std::string> chat_type_to_string;

    PurpleConnection *conn;
    PurpleAccount *acct;

    boost::shared_ptr<ThriftClient> c_out;

    HTTPClient http;

    friend class Poller;
    Poller poller;

    friend class PINVerifier;
    PINVerifier pin_verifier;

    int next_purple_id;

    std::deque<std::string> recent_messages;

    line::Profile profile;
    line::Contact profile_contact; // contains some fields from profile
    line::Contact no_contact; // empty object
    std::map<std::string, line::Group> groups;
    std::map<std::string, line::Room> rooms;
    std::map<std::string, line::Contact> contacts;

    void *pin_ui_handle;
    guint pin_timeout;

public:

    PurpleLine(PurpleConnection *conn, PurpleAccount *acct);
    ~PurpleLine();

    static void register_commands();
    static const char *list_icon(PurpleAccount *, PurpleBuddy *buddy);
    static GList *status_types(PurpleAccount *);
    static char *get_chat_name(GHashTable *components);
    static char *status_text(PurpleBuddy *buddy);
    static void tooltip_text(PurpleBuddy *buddy, PurpleNotifyUserInfo *user_info, gboolean full);
    static void login(PurpleAccount *acct);

    void close();
    GList *chat_info();
    int send_im(const char *who, const char *message, PurpleMessageFlags flags);
    void remove_buddy(PurpleBuddy *buddy, PurpleGroup *);
    void join_chat(GHashTable *components);
    void reject_chat(GHashTable *components);
    void chat_leave(int id);
    int chat_send(int id, const char *message, PurpleMessageFlags flags);
    PurpleChat *find_blist_chat(const char *name);

    PurpleCmdRet cmd_sticker(PurpleConversation *conv,
        const gchar *cmd, gchar **args, gchar **error, void *data);

    PurpleCmdRet cmd_history(PurpleConversation *conv,
        const gchar *cmd, gchar **args, gchar **error, void *data);

private:

    static ChatType get_chat_type(const char *type_ptr);
    static std::string get_sticker_id(line::Message &msg);
    static std::string get_sticker_url(line::Message &msg, bool thumb = false);

    void connect_signals();
    void disconnect_signals();

    // Login process methods, executed in this this order
    void start_login();

    void pin_verification(line::LoginResult result); // optional
    int pin_verification_timeout();
    void pin_verification_cancel(int);
    void pin_verification_end();
    void pin_verification_error(std::string error);

    void got_auth_token(std::string auth_token);
    void get_last_op_revision();
    void get_profile();
    void get_contacts();
    void get_groups();
    void get_rooms();
    void update_rooms(line::TMessageBoxWrapUpResponse wrap_up_list);
    void get_group_invites();
    void sync_done();

    void join_chat_success(ChatType type, std::string id);

    void handle_message(line::Message &msg, bool replay);
    void write_message(PurpleConversation *conv, line::Message &msg,
        time_t mtime, int flags, std::string text);
    void handle_group_invite(line::Group &group, line::Contact &invitee, line::Contact &inviter);

    template <typename T>
    std::set<T *> blist_find(std::function<bool(T *)> predicate);

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

    std::string get_room_display_name(line::Room &room);
    void set_chat_participants(PurpleConvChat *chat, line::Room &room);
    void set_chat_participants(PurpleConvChat *chat, line::Group &group);

    line::Contact &get_up_to_date_contact(line::Contact &c);

    int send_message(std::string to, std::string text);
    void send_message(line::Message &msg);
    void push_recent_message(std::string id);

    void signal_blist_node_removed(PurpleBlistNode *node);
    void signal_conversation_created(PurpleConversation *conv);
    void signal_deleting_conversation(PurpleConversation *conv);

    void fetch_conversation_history(PurpleConversation *conv, int count);

    void notify_error(std::string msg);
};

template <typename T>
std::set<T *> PurpleLine::blist_find(std::function<bool(T *)> predicate) {
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
