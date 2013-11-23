#include <iostream>

#include <conversation.h>
#include <debug.h>
#include <sslconn.h>

#include "purpleline.hpp"

static const char *LINE_GROUP = "LINE";
static const char *LINE_TEMP_GROUP = "LINE Temporary Contacts";

//static void line_timeout_add(guint interval, std::function<void> callback) {
//
//}

PurpleLine::PurpleLine(PurpleConnection *conn, PurpleAccount *acct) :
    conn(conn),
    acct(acct),
    next_id(1)
{
    c_out = boost::make_shared<ThriftClient>(acct, conn, "/S4");
    c_in = boost::make_shared<ThriftClient>(acct, conn, "/P4");
}

PurpleLine::~PurpleLine() {
    c_out->close();
    c_in->close();
}

const char *PurpleLine::list_icon(PurpleAccount *, PurpleBuddy *) {
    return "line";
}

GList *PurpleLine::status_types(PurpleAccount *) {
    GList *types = nullptr;
    PurpleStatusType *t;

    t = purple_status_type_new_full(PURPLE_STATUS_AVAILABLE, nullptr, nullptr, TRUE, TRUE, FALSE);
    types = g_list_append(types, t);

    t = purple_status_type_new_full(PURPLE_STATUS_OFFLINE, nullptr, nullptr, TRUE, TRUE, FALSE);
    types = g_list_append(types, t);

    return types;
}

char *PurpleLine::get_chat_name(GHashTable *components) {
    return (char *)g_hash_table_lookup(components, (gconstpointer)"id");
}

void PurpleLine::login(PurpleAccount *acct) {
    PurpleConnection *conn = purple_account_get_connection(acct);

    PurpleLine *plugin = new PurpleLine(conn, acct);
    conn->proto_data = (void *)plugin;

    plugin->start_login();
}

void PurpleLine::close() {
    std::cout << "I'm closing" << std::endl;

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

PurpleChat *PurpleLine::find_chat_by_id(std::string gid) {
    PurpleBlistNode *node = purple_blist_get_root();

    while (node) {
        if (PURPLE_BLIST_NODE_IS_CHAT(node)) {
            PurpleChat *chat = PURPLE_CHAT(node);

            if (purple_chat_get_account(chat) == acct
                && gid == (char *)g_hash_table_lookup(purple_chat_get_components(chat), "id"))
            {
                return chat;
            }
        }

        node = purple_blist_node_next(node, FALSE);
    }

    return nullptr;
}

void PurpleLine::start_login() {
    purple_connection_set_state(conn, PURPLE_CONNECTING);
    purple_connection_update_progress(conn, "Connecting", 0, 2);

    c_out->send_loginWithIdentityCredentialForCertificate(
        purple_account_get_username(acct),
        purple_account_get_password(acct),
        true,
        "127.0.0.1",
        "libpurple",
        line::IdentityProvider::LINE,
        "");
    c_out->send([this]() {
        line::LoginResult result;

        try {
            c_out->recv_loginWithIdentityCredentialForCertificate(result);
        } catch (line::Error &err) {
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

        purple_account_set_alias(acct, profile.displayName.c_str());

        //std::cout << "Your profile: " << std::endl
        //    << "  ID: " << profile.mid << std::endl
        //    << "  Display name: " << profile.displayName << std::endl
        //    << "  Picture status: " << profile.pictureStatus << std::endl
        //    << "  Status message: " << profile.statusMessage << std::endl;

        purple_connection_set_state(conn, PURPLE_CONNECTED);
        purple_connection_update_progress(conn, "Connected", 1, 2);

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

            PurpleGroup *group = purple_find_group(LINE_GROUP);

            for (line::Contact &c: contacts) {
                //std::cout << "Contact " << c.mid << std::endl
                //    //<< "  Type: " << line::_ContactType_VALUES_TO_NAMES.at(c.type) << std::endl
                //    << "  Status: " << line::_ContactStatus_VALUES_TO_NAMES.at(c.status) << std::endl
                //    //<< "  Relation: " << line::_ContactRelation_VALUES_TO_NAMES.at(c.relation) << std::endl
                //    << "  Display name: " << c.displayName << std::endl
                //    << "  Picture status: " << c.pictureStatus << std::endl
                //    << "  Status message: " << c.statusMessage << std::endl;

                if (c.status == line::ContactStatus::FRIEND) {
                    if (!group)
                        group = purple_group_new(LINE_GROUP);

                    PurpleBuddy *buddy = purple_find_buddy(acct, c.mid.c_str());
                    if (!buddy) {
                        buddy = purple_buddy_new(acct, c.mid.c_str(), c.displayName.c_str());
                        purple_blist_add_buddy(buddy, nullptr, group, nullptr);
                    }

                    purple_prpl_got_user_status(
                        acct,
                        c.mid.c_str(),
                        purple_primitive_get_id_from_type(PURPLE_STATUS_AVAILABLE),
                        nullptr);

                    //purple_blist_alias_buddy(buddy, c.displayName.c_str());
                }
            }

            {
                // Add self as buddy for those longely debugging conversations

                PurpleBuddy *buddy = purple_find_buddy(acct, profile.mid.c_str());
                if (!buddy) {
                    buddy = purple_buddy_new(acct, profile.mid.c_str(), profile.displayName.c_str());
                    purple_blist_add_buddy(buddy, nullptr, group, nullptr);
                }

                purple_prpl_got_user_status(
                    acct,
                    profile.mid.c_str(),
                    purple_primitive_get_id_from_type(PURPLE_STATUS_AVAILABLE),
                    nullptr);
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

            PurpleGroup *group = purple_find_group(LINE_GROUP);

            for (line::Group &g: groups) {
                group_map[g.id] = g;

                if (!group)
                    group = purple_group_new(LINE_GROUP);

                // Add chat to buddy list if it's not already there

                PurpleChat *chat = find_chat_by_id(g.id);
                if (!chat) {
                    GHashTable *components = g_hash_table_new_full(
                        g_str_hash, g_str_equal, g_free, g_free);

                    g_hash_table_insert(components, g_strdup("name"), g_strdup(g.name.c_str()));
                    g_hash_table_insert(components, g_strdup("id"), g_strdup(g.id.c_str()));

                    chat = purple_chat_new(acct, g.name.c_str(), components);

                    purple_blist_add_chat(chat, group, nullptr);
                }

                // If chat is somehow already open, set its members

                PurpleConvChat *conv_chat = PURPLE_CONV_CHAT(
                    purple_find_chat(conn, chat_id_to_purple_id[g.id]));

                if (conv_chat)
                    set_chat_participants(conv_chat, g);
            }

            // Start up return channel
            fetch_operations();
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

                default:
                    purple_debug_warning("line", "Unhandled operation type: %s\n",
                        line::_OperationType_VALUES_TO_NAMES.at(op.type));
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

        if (msg.toType == line::MessageToType::USER) {
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
        } else if (msg.toType == line::MessageToType::GROUP) {
            PurpleConversation *conv = purple_find_chat(conn, chat_id_to_purple_id[msg.to]);
            if (!conv)
                return; // Chat isn't open

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

        if (msg.toType == line::MessageToType::USER) {
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
        } else if (msg.toType == line::MessageToType::GROUP) {
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

// Ensures buddy exists on buddy list so their name shows up correctly. Will create temporary
// buddies if needed.
void PurpleLine::ensure_buddy_exists(std::string uid, std::string displayName) {
    PurpleBuddy *buddy = purple_find_buddy(acct, uid.c_str());
    if (buddy)
        return;

    buddy = purple_buddy_new(acct, uid.c_str(), displayName.c_str());

    PurpleGroup *group = purple_find_group(LINE_TEMP_GROUP);
    if (!group)
    {
        group = purple_group_new(LINE_TEMP_GROUP);
        purple_blist_add_group(group, nullptr);
        purple_blist_node_set_flags(&group->node, PURPLE_BLIST_NODE_FLAG_NO_SAVE);
    }

    purple_blist_add_buddy(buddy, nullptr, group, nullptr);
    purple_blist_node_set_flags(&buddy->node, PURPLE_BLIST_NODE_FLAG_NO_SAVE);
}

void PurpleLine::set_chat_participants(PurpleConvChat *chat, line::Group &group) {
    purple_conv_chat_clear_users(chat);

    for (line::Contact &c: group.members) {
        ensure_buddy_exists(c.mid, c.displayName);

        int flags = 0;

        if (c.mid == group.creator.mid)
            flags |= PURPLE_CBFLAGS_FOUNDER;

        purple_conv_chat_add_user(
            chat,
            c.mid.c_str(),
            "Extra!",
            (PurpleConvChatBuddyFlags)flags,
            FALSE);
    }

    for (line::Contact &c: group.invitee) {
        ensure_buddy_exists(c.mid, c.displayName);

        purple_conv_chat_add_user(
            chat,
            c.mid.c_str(),
            "[Invited]",
            (PurpleConvChatBuddyFlags)PURPLE_CBFLAGS_AWAY,
            FALSE);
    }
}

int PurpleLine::send_im(const char *who, const char *message, PurpleMessageFlags flags) {
    line::Message msg;

    msg.from = profile.mid;
    msg.to = who;
    msg.text = message;

    c_out->send_sendMessage(0, msg);
    c_out->send([this]() {
        line::Message msg_back;
        c_out->recv_sendMessage(msg_back);

        push_recent_message(msg_back.id);
    });

    return 1;
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

int PurpleLine::chat_send(int id, const char *message, PurpleMessageFlags flags) {
    std::string gid = chat_purple_id_to_id[id];
    if (gid == "") {
        purple_debug_warning("line", "Tried to send to a nonexistent chat.");
        return 0;
    }

    line::Message msg;

    msg.from = profile.mid;
    msg.to = gid;
    msg.text = message;

    c_out->send_sendMessage(0, msg);
    c_out->send([this]() {
        line::Message msg_back;
        c_out->recv_sendMessage(msg_back);
    });

    return 1;
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
            boost::make_shared<LineHttpTransport>(acct, conn, "gd2.line.naver.jp"))),
    path(path)
{
    http = piprot_->getTransport();
}

void ThriftClient::set_auth_token(std::string token) {
    http->set_auth_token(token);
}

void ThriftClient::send(std::function<void()> callback) {
    http->send(path, callback);
}

int ThriftClient::status_code() {
    return http->status_code();
}

void ThriftClient::close() {
    http->close();
}
