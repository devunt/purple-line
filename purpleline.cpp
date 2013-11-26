#include <iostream>

#include <time.h>

#include <conversation.h>
#include <debug.h>
#include <sslconn.h>

#include "purpleline.hpp"

static const char *LINE_GROUP = "LINE";
static const char *LINE_TEMP_GROUP = "LINE Temporary";

PurpleLine::PurpleLine(PurpleConnection *conn, PurpleAccount *acct) :
    conn(conn),
    acct(acct),
    next_id(1)
{
    c_out = boost::make_shared<ThriftClient>(acct, conn, "/S4");
    c_in = boost::make_shared<ThriftClient>(acct, conn, "/P4");
    http_os = boost::make_shared<LineHttpTransport>(acct, conn, "os.line.naver.jp", 443, false);
}

PurpleLine::~PurpleLine() {
    c_out->close();
    c_in->close();
    http_os->close();
}

const char *PurpleLine::list_icon(PurpleAccount *, PurpleBuddy *) {
    return "line";
}

GList *PurpleLine::status_types(PurpleAccount *) {
    const char *attr_id = "message", *attr_name = "What's Up?";

    GList *types = nullptr;
    PurpleStatusType *t;

    // Friends
    t = purple_status_type_new_with_attrs(
        PURPLE_STATUS_AVAILABLE, nullptr, nullptr,
        TRUE, TRUE, FALSE,
        attr_id, attr_name, purple_value_new(PURPLE_TYPE_STRING),
        nullptr);
    types = g_list_append(types, t);

    // Temporary friends
    t = purple_status_type_new_with_attrs(
        PURPLE_STATUS_UNAVAILABLE, "temporary", "Temporary",
        TRUE, TRUE, FALSE,
        attr_id, attr_name, purple_value_new(PURPLE_TYPE_STRING),
        nullptr);
    types = g_list_append(types, t);

    // Not actually used because LINE doesn't report online status, but this is apparently required.
    t = purple_status_type_new_with_attrs(
        PURPLE_STATUS_OFFLINE, nullptr, nullptr,
        TRUE, TRUE, FALSE,
        attr_id, attr_name, purple_value_new(PURPLE_TYPE_STRING),
        nullptr);
    types = g_list_append(types, t);

    return types;
}

char *PurpleLine::get_chat_name(GHashTable *components) {
    return g_strdup((char *)g_hash_table_lookup(components, (gconstpointer)"id"));
}

char *PurpleLine::status_text(PurpleBuddy *buddy) {
    PurplePresence *presence = purple_buddy_get_presence(buddy);
    PurpleStatus *status = purple_presence_get_active_status(presence);

    const char *msg = purple_status_get_attr_string(status, "message");
    if (msg && msg[0])
        return g_markup_escape_text(msg, -1);

    return nullptr;
}

void PurpleLine::tooltip_text(PurpleBuddy *buddy, PurpleNotifyUserInfo *user_info, gboolean full) {
    purple_notify_user_info_add_pair_plaintext(user_info,
        "Name", purple_buddy_get_alias(buddy));

    //purple_notify_user_info_add_pair_plaintext(user_info,
    //    "User ID", purple_buddy_get_name(buddy));

    if (purple_blist_node_get_bool(PURPLE_BLIST_NODE(buddy), "official_account"))
        purple_notify_user_info_add_pair_plaintext(user_info, "Official account", "Yes");

    if (PURPLE_BLIST_NODE_HAS_FLAG(buddy, PURPLE_BLIST_NODE_FLAG_NO_SAVE)) {
        purple_notify_user_info_add_section_break(user_info);
        purple_notify_user_info_add_pair_plaintext(user_info,
            "Temporary",
            "You are currently in a conversation with this user, "
                "but they are not on your friend list.");
    }
}

void PurpleLine::login(PurpleAccount *acct) {
    PurpleConnection *conn = purple_account_get_connection(acct);

    PurpleLine *plugin = new PurpleLine(conn, acct);
    conn->proto_data = (void *)plugin;

    plugin->start_login();
}

void PurpleLine::close() {
    delete this;
}

static struct proto_chat_entry id_chat_entry {
    .label = "Group _Name",
    .identifier = "name",
    .required = TRUE,
};

GList *PurpleLine::chat_info() {
    return g_list_append(nullptr, g_memdup(&id_chat_entry, sizeof(struct proto_chat_entry)));
}

void PurpleLine::start_login() {
    purple_connection_set_state(conn, PURPLE_CONNECTING);
    purple_connection_update_progress(conn, "Connecting", 0, 3);

    c_out->send_loginWithIdentityCredentialForCertificate(
        purple_account_get_username(acct),
        purple_account_get_password(acct),
        true,
        "127.0.0.1",
        "libpurple",
        line::Provider::LINE,
        "");
    c_out->send([this]() {
        line::LoginResult result;

        try {
            c_out->recv_loginWithIdentityCredentialForCertificate(result);
        } catch (line::TalkException &err) {
            std::string msg = "Could not log in. " + err.reason;

            purple_connection_error(
                conn,
                msg.c_str());

            return;
        }

        // Re-open output client to update persistent headers
        c_out->close();

        c_out->set_auth_token(result.authToken);
        c_in->set_auth_token(result.authToken);
        http_os->set_auth_token(result.authToken);

        get_last_op_revision();
    });
}

void PurpleLine::get_last_op_revision() {
    c_out->send_getLastOpRevision();
    c_out->send([this]() {
        local_rev = c_out->recv_getLastOpRevision();

        get_profile();
    });
}

void PurpleLine::get_profile() {
    c_out->send_getProfile();
    c_out->send([this]() {
        c_out->recv_getProfile(profile);

        // Update display name
        purple_account_set_alias(acct, profile.displayName.c_str());

        // Set account as connected. Buddy list synchronization happens next, but otherwise
        // everything is usable
        purple_connection_set_state(conn, PURPLE_CONNECTED);
        purple_connection_update_progress(conn, "Synchronizing buddy list", 1, 3);

        // Update account icon (not sure if there's a way to tell whether it has changed, maybe
        // pictureStatus?)
        if (profile.picturePath != "") {
            std::string pic_path = profile.picturePath + "/preview";
            //if (icon_path != purple_account_get_string(acct, "icon_path", "")) {
                http_os->request("GET", pic_path, [this, pic_path]{
                    guchar *buffer = (guchar *)malloc(http_os->content_length() * sizeof(guchar));
                    http_os->read(buffer, http_os->content_length());

                    purple_buddy_icons_set_account_icon(
                        acct,
                        (guchar *)buffer,
                        (size_t)http_os->content_length());

                    //purple_account_set_string(acct, "icon_path", icon_path.c_str());
                });
            //}
        } else {
            // TODO: Delete icon
        }

        get_contacts();
    });
}

void PurpleLine::get_contacts() {
    c_out->send_getAllContactIds();
    c_out->send([this]() {
        std::vector<std::string> uids;
        c_out->recv_getAllContactIds(uids);

        c_out->send_getContacts(uids);
        c_out->send([this]() {
            std::vector<line::Contact> contacts;
            c_out->recv_getContacts(contacts);

            for (line::Contact &contact: contacts) {
                if (contact.status == line::ContactStatus::FRIEND)
                    blist_update_buddy(contact);
            }

            // TODO: Remove buddies that are no longer contacts

            {
                // Add self as buddy for those lonely debugging conversations
                // TODO: Remove

                line::Contact self;
                self.mid = profile.mid;
                self.displayName = profile.displayName + " [Profile]";
                self.statusMessage = profile.statusMessage;
                self.picturePath = profile.picturePath;

                blist_update_buddy(self);
            }

            get_groups();
        });
    });
}

void PurpleLine::get_groups() {
    c_out->send_getGroupIdsJoined();
    c_out->send([this]() {
        std::vector<std::string> gids;
        c_out->recv_getGroupIdsJoined(gids);

        c_out->send_getGroups(gids);
        c_out->send([this]() {
            std::vector<line::Group> groups;
            c_out->recv_getGroups(groups);

            for (line::Group &g: groups) {
                group_map[g.id] = g;

                // Add chat to buddy list if it's not already there

                PurpleChat *chat = blist_find_chat_by_id(g.id);
                if (!chat) {
                    GHashTable *components = g_hash_table_new_full(
                        g_str_hash, g_str_equal, g_free, g_free);

                    g_hash_table_insert(components, g_strdup("name"), g_strdup(g.name.c_str()));
                    g_hash_table_insert(components, g_strdup("id"), g_strdup(g.id.c_str()));

                    chat = purple_chat_new(acct, g.name.c_str(), components);

                    purple_blist_add_chat(chat, blist_ensure_group(LINE_GROUP), nullptr);
                }

                // If chat is somehow already open, set its members

                PurpleConvChat *conv_chat = PURPLE_CONV_CHAT(
                    purple_find_chat(conn, chat_id_to_purple_id[g.id]));

                if (conv_chat)
                    set_chat_participants(conv_chat, g);
            }

            // Start up return channel
            fetch_operations();

            purple_connection_update_progress(conn, "Connected", 2, 3);
        });
    });
}

void PurpleLine::fetch_operations() {
    c_in->send_fetchOperations(local_rev, 50);
    c_in->send([this]() {
        int status = c_in->status_code();

        if (status == -1) {
            // Plugin closing
            return;
        } else if (status == 410) {
            // Long poll timeout, resend
            fetch_operations();
            return;
        } else if (status != 200) {
            purple_debug_warning("line", "fetchOperations error %d. TODO: Retry after a timeout", status);
            return;
        }

        std::vector<line::Operation> operations;
        c_in->recv_fetchOperations(operations);

        for (line::Operation &op: operations) {
            switch (op.type) {
                case line::OperationType::END_OF_OPERATION:
                    break;

                case line::OperationType::RECEIVE_MESSAGE:
                    handle_message(op.message, false, false);
                    break;

                case line::OperationType::SEND_MESSAGE:
                    handle_message(op.message, true, false);
                    break;

                case line::OperationType::ADD_CONTACT:
                    blist_update_buddy(op.param1);
                    break;

                case line::OperationType::BLOCK_CONTACT:
                    blist_remove_buddy(op.param1);
                    break;

                default:
                    purple_debug_warning("line", "Unhandled operation type: %d\n", op.type);
                    break;
            }

            if (op.revision > local_rev)
                local_rev = op.revision;
        }

        fetch_operations();
    });
}

// TODO: Refactor this, it's about to become a mess
void PurpleLine::handle_message(line::Message &msg, bool sent, bool replay) {
    std::string text;
    int flags = 0;
    time_t mtime = (time_t)(msg.createdTime / 1000);

    if (std::find(recent_messages.cbegin(), recent_messages.cend(), msg.id) != recent_messages.cend()) {
        // We already processed this message. User is probably talking with himself.
        return;
    }

    // Replaying messages from history
    //if (replay)
    //    flags |= PURPLE_MESSAGE_NO_LOG;

    switch (msg.contentType) {
        case line::ContentType::NONE: // actually text
            text = msg.text;
            break;

        // TODO: other content types

        default:
            text = "[";
            text += line::_ContentType_VALUES_TO_NAMES.at(msg.contentType);
            text += " message]";
            break;
    }

    if (sent) {
        // Messages sent by user (sync from other devices)

        flags |= PURPLE_MESSAGE_SEND;

        if (msg.toType == line::ToType::USER) {
            PurpleConversation *conv = purple_find_conversation_with_account(
                PURPLE_CONV_TYPE_IM,
                msg.to.c_str(),
                acct);

            if (conv) {
                purple_conv_im_write(
                    PURPLE_CONV_IM(conv),
                    msg.from.c_str(),
                    text.c_str(),
                    (PurpleMessageFlags)flags,
                    mtime);
            }
        } else if (msg.toType == line::ToType::GROUP) {
            PurpleConversation *conv = purple_find_chat(conn, chat_id_to_purple_id[msg.to]);

            if (conv) {
                purple_conv_chat_write(
                    PURPLE_CONV_CHAT(conv),
                    msg.from.c_str(),
                    text.c_str(),
                    (PurpleMessageFlags)flags,
                    mtime);
            }
        }
    } else {
        // Messages received from other users

        flags |= PURPLE_MESSAGE_RECV;

        if (msg.toType == line::ToType::USER) {
            if (msg.to != profile.mid) {
                purple_debug_warning("line", "Got message meant for some other user...?");
                return;
            }

            serv_got_im(
                conn,
                msg.from.c_str(),
                text.c_str(),
                (PurpleMessageFlags)flags,
                mtime);
        } else if (msg.toType == line::ToType::GROUP) {
            int purple_id = chat_id_to_purple_id[msg.to];

            if (!purple_id)
                return; // Chat isn't open TODO perhaps notify

            //if (replay) {
            //    PurpleConversation *conv = purple_find_chat(conn, chat_id_to_purple_id[msg.to]);

            //    if (!conv)
            //        return; // Chat isn't open

            //    purple_conv_chat_write(
            //        PURPLE_CONV_CHAT(conv),
            //        msg.from.c_str(),
            //        text.c_str(),
            //        (PurpleMessageFlags)flags,
            //        mtime);
            //} else {
                serv_got_chat_in(
                    conn,
                    purple_id,
                    msg.from.c_str(),
                    (PurpleMessageFlags)flags,
                    text.c_str(),
                    mtime);
            //}
        }
    }

    // Hack
    if (msg.from == msg.to)
        push_recent_message(msg.id);
}

PurpleChat *PurpleLine::blist_find_chat_by_id(std::string gid) {
    for (PurpleBlistNode *node = purple_blist_get_root();
        node;
        node = purple_blist_node_next(node, FALSE))
    {
        if (!PURPLE_BLIST_NODE_IS_CHAT(node))
            continue;

        PurpleChat *chat = PURPLE_CHAT(node);

        if (purple_chat_get_account(chat) == acct
            && gid == (char *)g_hash_table_lookup(purple_chat_get_components(chat), "id"))
        {
            return chat;
        }
    }

    return nullptr;
}

PurpleGroup *PurpleLine::blist_ensure_group(std::string group_name, bool temporary) {
    PurpleGroup *group = purple_find_group(group_name.c_str());
    if (!group) {
        group = purple_group_new(group_name.c_str());
        purple_blist_add_group(group, nullptr);

        if (temporary) {
            purple_blist_node_set_flags(PURPLE_BLIST_NODE(group), PURPLE_BLIST_NODE_FLAG_NO_SAVE);
            purple_blist_node_set_bool(PURPLE_BLIST_NODE(group), "collapsed", TRUE);
        }
    }

    return group;
}

// Ensures buddy exists on buddy list so their name shows up correctly. Will create temporary
// buddies if needed.
PurpleBuddy *PurpleLine::blist_ensure_buddy(std::string uid, bool temporary) {
    PurpleBuddy *buddy = purple_find_buddy(acct, uid.c_str());
    if (buddy) {
        int flags = purple_blist_node_get_flags(&buddy->node);

        // If buddy is not temporary anymore, make them permanent.
        if ((flags & PURPLE_BLIST_NODE_FLAG_NO_SAVE) && !temporary)
        {
            purple_blist_node_set_flags(
                &buddy->node,
                (PurpleBlistNodeFlags)(flags & ~PURPLE_BLIST_NODE_FLAG_NO_SAVE));

            if (purple_buddy_get_group(buddy) == blist_ensure_group(LINE_TEMP_GROUP))
                purple_blist_add_buddy(buddy, nullptr, blist_ensure_group(LINE_GROUP), nullptr);
        }
    } else {
        buddy = purple_buddy_new(acct, uid.c_str(), uid.c_str());

        purple_blist_add_buddy(
            buddy,
            nullptr,
            blist_ensure_group(temporary ? LINE_TEMP_GROUP : LINE_GROUP, temporary),
            nullptr);

        if (temporary)
            purple_blist_node_set_flags(&buddy->node, PURPLE_BLIST_NODE_FLAG_NO_SAVE);
    }
    // Mark temporary friends as temporary. blist_update_buddy will mark actual friends as
    // available.
    if (temporary) {
        purple_prpl_got_user_status(
            acct,
            uid.c_str(),
            "temporary",
            nullptr);
    }

    return buddy;
}

// Fetch information for contact and call the other update_buddy
void PurpleLine::blist_update_buddy(std::string uid, bool temporary) {
    // Put buddy on list already so it shows up as loading
    blist_ensure_buddy(uid.c_str(), temporary);

    c_out->send_getContacts(std::vector<std::string> { uid });
    c_out->send([this, temporary]{
        std::vector<line::Contact> contacts;
        c_out->recv_getContacts(contacts);

        if (contacts.size() == 1)
            blist_update_buddy(contacts[0], temporary);
    });
}

// Updates buddy details such as alias, icon, status message
void PurpleLine::blist_update_buddy(line::Contact contact, bool temporary) {
    PurpleBuddy *buddy = blist_ensure_buddy(contact.mid.c_str(), temporary);
    if (!buddy) {
        purple_debug_warning("line", "Tried to update a non-existent buddy %s\n",
            contact.mid.c_str());
        return;
    }

    // Update display name
    purple_blist_alias_buddy(buddy, contact.displayName.c_str());

    // Update buddy icon if necessary
    if (contact.picturePath != "") {
        std::string pic_path = contact.picturePath + "/preview";
        const char *current_pic_path = purple_buddy_icons_get_checksum_for_user(buddy);
        if (!current_pic_path || std::string(current_pic_path) != pic_path) {
            std::string uid = contact.mid;

            http_os->request("GET", pic_path, [this, uid, pic_path]{
                uint8_t *buffer = (uint8_t *)malloc(http_os->content_length() * sizeof(uint8_t));
                http_os->read(buffer, http_os->content_length());

                purple_buddy_icons_set_for_user(
                    acct,
                    uid.c_str(),
                    (void *)buffer,
                    (size_t)http_os->content_length(),
                    pic_path.c_str());
            });
        }
    } else {
        // TODO: delete icon if any
    }

    // Mark actual friends as available.
    if (!temporary) {
        purple_prpl_got_user_status(
            acct,
            contact.mid.c_str(),
            purple_primitive_get_id_from_type(PURPLE_STATUS_AVAILABLE),
            "message", contact.statusMessage.c_str(),
            nullptr);
    }

    if (contact.attributes & 32)
        purple_blist_node_set_bool(PURPLE_BLIST_NODE(buddy), "official_account", TRUE);
    else
        purple_blist_node_remove_setting(PURPLE_BLIST_NODE(buddy), "official_account");
}

bool PurpleLine::blist_is_buddy_in_any_conversation(std::string uid) {
    for (GList * convs = purple_get_conversations();
        convs;
        convs = g_list_next(convs))
    {
        PurpleConversation *conv = (PurpleConversation *)convs->data;

        if (purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_IM) {
            if (uid == purple_conversation_get_name(conv))
                return true;
        } else if (purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_CHAT) {

            for (GList *buddies = purple_conv_chat_get_users(PURPLE_CONV_CHAT(conv));
                buddies;
                buddies = g_list_next(buddies))
            {
                PurpleConvChatBuddy *buddy = (PurpleConvChatBuddy *)buddies->data;

                if (uid == purple_conv_chat_cb_get_name(buddy))
                    return true;
            }
        }
    }

    return false;
}

void PurpleLine::blist_remove_buddy(std::string uid) {
    PurpleBuddy *buddy = purple_find_buddy(acct, uid.c_str());
    if (!buddy)
        return;

    if (blist_is_buddy_in_any_conversation(uid)) {
        // If the buddy exists in a conversation, demote them to a temporary buddy instead of
        // deleting them completely.

        purple_blist_node_set_flags(&buddy->node,
            (PurpleBlistNodeFlags)
                (purple_blist_node_get_flags(&buddy->node) | PURPLE_BLIST_NODE_FLAG_NO_SAVE));

        purple_blist_add_buddy(buddy, nullptr, blist_ensure_group(LINE_TEMP_GROUP), nullptr);

        // Get current status text to preserve it
        PurplePresence *presence = purple_buddy_get_presence(buddy);
        PurpleStatus *status = purple_presence_get_active_status(presence);

        purple_prpl_got_user_status(
            acct,
            uid.c_str(),
            "temporary",
            "message", purple_status_get_attr_string(status, "message"),
            nullptr);
    } else {
        // Otherwise, delete them.

        purple_blist_remove_buddy(buddy);
    }
}

void PurpleLine::set_chat_participants(PurpleConvChat *chat, line::Group &group) {
    purple_conv_chat_clear_users(chat);

    GList *users = NULL, *flags = NULL;

    for (line::Contact &c: group.members) {
        blist_update_buddy(c, true);

        int cbflags = 0;

        if (c.mid == group.creator.mid)
            cbflags |= PURPLE_CBFLAGS_FOUNDER;

        users = g_list_prepend(users, (gpointer)c.mid.c_str());
        flags = g_list_prepend(flags, GINT_TO_POINTER(cbflags));
    }

    for (line::Contact &c: group.invitee) {
        blist_update_buddy(c, true);

        users = g_list_prepend(users, (gpointer)c.mid.c_str());
        flags = g_list_prepend(flags, GINT_TO_POINTER(PURPLE_CBFLAGS_AWAY));
    }

    purple_conv_chat_add_users(chat, users, NULL, flags, FALSE);

    g_list_free(users);
    g_list_free(flags);
}

void PurpleLine::join_chat(GHashTable *components) {
    char *id_ptr = (char *)g_hash_table_lookup(components, "id");
    if (!id_ptr) {
        purple_debug_warning("line", "Tried to join a chat with no id.");
        return;
    }

    std::string gid(id_ptr);

    // Assume chat is on buddy list for now.

    int purple_id = next_id++;

    // TODO: clean up maps when chat is closed
    chat_purple_id_to_id[purple_id] = gid;
    chat_id_to_purple_id[gid] = purple_id;

    PurpleConversation *conv = serv_got_joined_chat(
        conn,
        purple_id,
        (char *)g_hash_table_lookup(components, "name"));

    set_chat_participants(PURPLE_CONV_CHAT(conv), group_map[gid]);

    c_out->send_getRecentMessages(gid, 20);
    c_out->send([this, purple_id]() {
        std::vector<line::Message> recent_msgs;
        c_out->recv_getRecentMessages(recent_msgs);

        PurpleConversation *conv = purple_find_chat(conn, purple_id);
        if (!conv) {
            // Chat went away already?
            return;
        }

        for (auto i = recent_msgs.rbegin(); i != recent_msgs.rend(); i++) {
            line::Message &msg = *i;

            handle_message(msg, (msg.from == profile.mid), true);

            //push_recent_message(msg.id);
        }
    });
}


int PurpleLine::send_message(std::string to, int chat_purple_id, std::string text) {
    if (chat_purple_id) {
        to = chat_purple_id_to_id[chat_purple_id];
        if (to == "") {
            purple_debug_warning("line", "Tried to send to a nonexistent chat.");
            return 0;
        }
    }

    line::Message msg;

    msg.from = profile.mid;
    msg.to = to;
    msg.text = text;

    c_out->send_sendMessage(0, msg);
    c_out->send([this, to, chat_purple_id]() {
        line::Message msg_back;

        try {
            c_out->recv_sendMessage(msg_back);
        } catch (line::TalkException &err) {
            PurpleConversation *conv = nullptr;

            if (chat_purple_id) {
                conv = purple_find_chat(conn, chat_purple_id);
            } else {
                conv = purple_find_conversation_with_account(
                    PURPLE_CONV_TYPE_IM,
                    to.c_str(),
                    acct);
            }

            if (conv) {
                purple_conversation_write(
                    conv,
                    "",
                    "Failed to send message.",
                    (PurpleMessageFlags)PURPLE_MESSAGE_ERROR,
                    time(NULL));
            }

            throw;
        }

        if (!chat_purple_id)
            push_recent_message(msg_back.id);
    });

    return 1;
}

int PurpleLine::send_im(const char *who, const char *message, PurpleMessageFlags flags) {
    return send_message(who, 0, message);
}

int PurpleLine::chat_send(int id, const char *message, PurpleMessageFlags flags) {
    return send_message("", id, message);
}

void PurpleLine::push_recent_message(std::string id) {
    recent_messages.push_back(id);
    if (recent_messages.size() > 50)
        recent_messages.pop_front();
}

ThriftProtocol::ThriftProtocol(boost::shared_ptr<LineHttpTransport> trans)
    : apache::thrift::protocol::TCompactProtocolT<LineHttpTransport>(trans)
{
}

LineHttpTransport *ThriftProtocol::getTransport() {
    return trans_;
}

ThriftClient::ThriftClient(PurpleAccount *acct, PurpleConnection *conn, std::string path)
    : line::LineClientT<ThriftProtocol>(
        boost::make_shared<ThriftProtocol>(
            boost::make_shared<LineHttpTransport>(acct, conn, "gd2.line.naver.jp", 443, true))),
    path(path)
{
    http = piprot_->getTransport();
}

void ThriftClient::set_auth_token(std::string token) {
    http->set_auth_token(token);
}

void ThriftClient::send(std::function<void()> callback) {
    http->request("POST", path, callback);
}

int ThriftClient::status_code() {
    return http->status_code();
}

void ThriftClient::close() {
    http->close();
}
