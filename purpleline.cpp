#include <iostream>

#include <time.h>

#include <conversation.h>
#include <request.h>
#include <debug.h>
#include <sslconn.h>
#include <util.h>
#include <eventloop.h>

#include "purpleline.hpp"
#include "wrapper.hpp"

const char *LINE_CERTIFICATE = "line-certificate";

static std::string markup_escape(std::string const &text) {
    gchar *escaped = purple_markup_escape_text(text.c_str(), text.size());
    std::string result(escaped);
    g_free(escaped);

    return result;
}

static std::string markup_unescape(std::string const &markup) {
    gchar *unescaped = purple_unescape_html(markup.c_str());
    std::string result(unescaped);
    g_free(unescaped);

    return result;
}

std::map<PurpleLine::ChatType, std::string> PurpleLine::chat_type_to_string {
    { PurpleLine::ChatType::GROUP, "group" },
    { PurpleLine::ChatType::ROOM, "room" },
};

PurpleLine::PurpleLine(PurpleConnection *conn, PurpleAccount *acct) :
    conn(conn),
    acct(acct),
    next_purple_id(1)
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
        // This breaks?
        //purple_notify_user_info_add_section_break(user_info);

        purple_notify_user_info_add_pair_plaintext(user_info,
            "Temporary",
            "You are currently in a conversation with this contact, "
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

GList *PurpleLine::chat_info() {
    struct proto_chat_entry *entry = g_new0(struct proto_chat_entry, 1);

    entry->label = "Group _Name";
    entry->identifier = "name",
    entry->required = TRUE;

    return g_list_append(nullptr, entry);
}

void PurpleLine::start_login() {
    purple_connection_set_state(conn, PURPLE_CONNECTING);
    purple_connection_update_progress(conn, "Logging in", 0, 3);

    std::string certificate(purple_account_get_string(acct, LINE_CERTIFICATE, ""));

    c_out->send_loginWithIdentityCredentialForCertificate(
        line::IdentityProvider::LINE,
        purple_account_get_username(acct),
        purple_account_get_password(acct),
        true,
        "127.0.0.1",
        "purple-line (Pidgin)",
        certificate);
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

        if (result.type == line::LoginResultType::SUCCESS && result.authToken != "")
        {
            got_auth_token(result.authToken);
        }
        else if (result.type == line::LoginResultType::REQUIRE_DEVICE_CONFIRM)
        {
            pin_verification(result);
        }
        else
        {
            std::stringstream ss("Could not log in. Bad LoginResult type: ");
            ss << result.type;
            std::string msg = ss.str();

            purple_connection_error(
                conn,
                msg.c_str());
        }
    });
}

void PurpleLine::pin_verification(line::LoginResult result) {
    const int minutes = 3;

    const std::string verifier = result.verifier;

    std::stringstream ss;
    ss
        << result.pinCode
        << "\n\nThe number has to be entered into the LINE mobile application within "
        << minutes
        << " minutes. If the time runs out, reconnect to try again."
        << "\n\nYou will only have to verify your account once per computer.";
    std::string pin_msg = ss.str();

    pin_ui_handle = purple_request_action(
        (void *)conn,
        "LINE account verification",
        "Enter this number on your mobile device",
        pin_msg.c_str(),
        0,
        acct,
        nullptr,
        nullptr,
        (gpointer)this,
        1,
        "Cancel",
        (PurpleRequestActionCb)WRAPPER(PurpleLine::pin_verification_cancel));

    pin_timeout = purple_timeout_add_seconds(
        minutes * 60,
        WRAPPER(PurpleLine::pin_verification_timeout),
        (gpointer)this);

    http_pin = boost::make_shared<LineHttpTransport>(acct, conn, "gd2.line.naver.jp", 443, false);
    http_pin->set_auth_token(verifier);

    http_pin->request("GET", "/Q", [this, verifier]() {
        if (http_pin->status_code() != 200) {
            std::stringstream ss;
            ss << "Account verification failed: invalid status code: " << http_pin->status_code();

            pin_verification_error(ss.str());
            return;
        }

        std::string json((size_t)http_pin->content_length(), '\0');
        http_pin->read((uint8_t *)&json[0], json.size());

        // Don't feel like invoking an entire JSON parser for this, this should be good enough.
        if (json.find("\"QRCODE_VERIFIED\"") == std::string::npos) {
            pin_verification_error("Account verification failed: server would not verify.");
            return;
        }

        c_out->send_loginWithVerifierForCertificate(verifier);
        c_out->send([this, verifier]() {
            line::LoginResult result;

            try {
                c_out->recv_loginWithVerifierForCertificate(result);
            } catch (line::TalkException &err) {
                pin_verification_error("Account verification failed: Exception: " + err.reason);
                return;
            }

            if (result.authToken == "") {
                pin_verification_error("Account verification failed: no auth token.");
                return;
            }

            if (result.certificate != "")
                purple_account_set_string(acct, LINE_CERTIFICATE, result.certificate.c_str());

            pin_verification_end();

            got_auth_token(result.authToken);
        });
    });
}

int PurpleLine::pin_verification_timeout() {
    pin_verification_error("Account verification timed out.");

    return FALSE;
}

void PurpleLine::pin_verification_cancel(int) {
    pin_verification_error("Account verification cancelled.");
}

void PurpleLine::pin_verification_end() {
    http_pin.reset();

    if (pin_timeout) {
        purple_timeout_remove(pin_timeout);
        pin_timeout = 0;
    }

    if (pin_ui_handle) {
        purple_request_close(PURPLE_REQUEST_ACTION, pin_ui_handle);
        pin_ui_handle = nullptr;
    }
}

void PurpleLine::pin_verification_error(std::string error) {
    pin_verification_end();

    purple_connection_error(
        conn,
        error.c_str());
}

void PurpleLine::got_auth_token(std::string auth_token) {
    // Re-open output client to update persistent headers
    c_out->close();

    c_out->set_auth_token(auth_token);
    c_in->set_auth_token(auth_token);
    http_os->set_auth_token(auth_token);

    get_last_op_revision();
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

            std::set<PurpleBuddy *> buddies_to_delete = blist_find<PurpleBuddy>();

            for (line::Contact &contact: contacts) {
                if (contact.status == line::ContactStatus::FRIEND)
                    buddies_to_delete.erase(blist_update_buddy(contact));
            }

            for (PurpleBuddy *buddy: buddies_to_delete)
                blist_remove_buddy(purple_buddy_get_name(buddy));
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

            std::set<PurpleChat *> chats_to_delete = blist_find_chats_by_type(ChatType::GROUP);

            for (line::Group &group: groups)
                chats_to_delete.erase(blist_update_chat(group));

            for (PurpleChat *chat: chats_to_delete)
                purple_blist_remove_chat(chat);

            get_rooms();
        });
    });
}

void PurpleLine::get_rooms() {
    c_out->send_getMessageBoxCompactWrapUpList(1, 65535);
    c_out->send([this]() {
        line::TMessageBoxWrapUpResponse wrap_up_list;
        c_out->recv_getMessageBoxCompactWrapUpList(wrap_up_list);

        std::set<std::string> uids;

        for (line::TMessageBoxWrapUp &ent: wrap_up_list.messageBoxWrapUpList) {
            if (ent.messageBox.midType != line::MIDType::ROOM)
                continue;

            for (line::Contact &c: ent.contacts)
                uids.insert(c.mid);
        }

        if (!uids.empty()) {
            // Room contacts don't contain full contact information, so pull separately to get names

            c_out->send_getContacts(std::vector<std::string>(uids.begin(), uids.end()));
            c_out->send([this, wrap_up_list]{
                std::vector<line::Contact> contacts;
                c_out->recv_getContacts(contacts);

                for (line::Contact &c: contacts)
                    this->contacts[c.mid] = c;

                update_rooms(wrap_up_list);
            });
        } else {
            update_rooms(wrap_up_list);
        }
    });
}

void PurpleLine::update_rooms(line::TMessageBoxWrapUpResponse wrap_up_list) {
    std::set<PurpleChat *> chats_to_delete = blist_find_chats_by_type(ChatType::ROOM);

    for (line::TMessageBoxWrapUp &ent: wrap_up_list.messageBoxWrapUpList) {
        if (ent.messageBox.midType != line::MIDType::ROOM)
            continue;

        line::Room room;
        room.mid = ent.messageBox.id;
        room.contacts = ent.contacts;

        chats_to_delete.erase(blist_update_chat(room));
    }

    for (PurpleChat *chat: chats_to_delete)
        purple_blist_remove_chat(chat);

    // Start up return channel
    fetch_operations();

    purple_connection_set_state(conn, PURPLE_CONNECTED);
    purple_connection_update_progress(conn, "Connected", 2, 3);
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
                case line::OpType::END_OF_OPERATION:
                    break;

                case line::OpType::ADD_CONTACT:
                    blist_update_buddy(op.param1);
                    break;

                case line::OpType::BLOCK_CONTACT:
                    blist_remove_buddy(op.param1);
                    break;

                case line::OpType::LEAVE_GROUP:
                    blist_remove_chat(op.param1, ChatType::GROUP);
                    break;

                case line::OpType::LEAVE_ROOM:
                    blist_remove_chat(op.param1, ChatType::ROOM);
                    break;

                case line::OpType::SEND_MESSAGE:
                    handle_message(op.message, true, false);
                    break;

                case line::OpType::RECEIVE_MESSAGE:
                    handle_message(op.message, false, false);
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
            text = markup_escape(msg.text);
            break;

        // TODO: other content types

        default:
            text = "<em>[Not implemented: ";
            text += line::_ContentType_VALUES_TO_NAMES.at(msg.contentType);
            text += " message]</em>";
            break;
    }

    if (sent) {
        // Messages sent by user (sync from other devices)

        flags |= PURPLE_MESSAGE_SEND;

        if (msg.toType == line::MIDType::USER) {
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
        } else if (msg.toType == line::MIDType::GROUP || msg.toType == line::MIDType::ROOM) {
            PurpleConversation *conv = purple_find_conversation_with_account(
                PURPLE_CONV_TYPE_CHAT,
                msg.to.c_str(),
                acct);

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

        if (msg.toType == line::MIDType::USER) {
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
        } else if (msg.toType == line::MIDType::GROUP || msg.toType == line::MIDType::ROOM) {
            PurpleConversation *conv = purple_find_conversation_with_account(
                PURPLE_CONV_TYPE_CHAT,
                msg.to.c_str(),
                acct);

            if (!conv)
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
                    purple_conv_chat_get_id(PURPLE_CONV_CHAT(conv)),
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

std::string PurpleLine::get_room_display_name(line::Room &room) {
    std::vector<line::Contact *> rcontacts;

    for (line::Contact &rc: room.contacts) {
        if (contacts.count(rc.mid) == 1)
            rcontacts.push_back(&contacts[rc.mid]);
    }

    std::stringstream ss;
    ss << "Chat with ";

    switch (rcontacts.size()) {
        case 0:
            ss << "nobody...?";
            break;

        case 1:
            ss << rcontacts[0]->displayName;
            break;

        case 2:
            ss << rcontacts[0]->displayName << " and " << rcontacts[1]->displayName;
            break;

        default:
            ss << rcontacts[0]->displayName << " and " << (rcontacts.size() - 1) << " other people";
            break;
    }

    return ss.str();
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

void PurpleLine::set_chat_participants(PurpleConvChat *chat, line::Room &room) {
    purple_conv_chat_clear_users(chat);

    GList *users = NULL, *flags = NULL;

    for (line::Contact &rc: room.contacts) {
        // Room contacts don't have full contact information.
        if (contacts.count(rc.mid) == 1)
            blist_update_buddy(contacts[rc.mid], true);

        users = g_list_prepend(users, (gpointer)rc.mid.c_str());
        flags = g_list_prepend(flags, GINT_TO_POINTER(0));
    }

    // Room contact lists don't contain self, so add for consistency
    users = g_list_prepend(users, (gpointer)profile.mid.c_str());
    flags = g_list_prepend(flags, GINT_TO_POINTER(0));

    purple_conv_chat_add_users(chat, users, NULL, flags, FALSE);

    g_list_free(users);
    g_list_free(flags);
}

void PurpleLine::join_chat(GHashTable *components) {
    char *type_ptr = (char *)g_hash_table_lookup(components, "type");
    if (!type_ptr) {
        purple_debug_warning("line", "Tried to join a chat with no type.");
        return;
    }

    ChatType type;

    // >_>
    std::string type_str(type_ptr);
    if (type_str == chat_type_to_string[ChatType::GROUP]) {
        type = ChatType::GROUP;
    } else if (type_str == chat_type_to_string[ChatType::ROOM]) {
        type = ChatType::ROOM;
    } else {
        purple_debug_warning("line", "Tried to join a chat with weird type.");
        return;
    }

    char *id_ptr = (char *)g_hash_table_lookup(components, "id");
    if (!id_ptr) {
        purple_debug_warning("line", "Tried to join a chat with no id.");
        return;
    }

    std::string id(id_ptr);

    // Assume chat is on buddy list for now.

    int purple_id = next_purple_id++;

    PurpleConversation *conv = serv_got_joined_chat(
        conn,
        purple_id,
        (char *)g_hash_table_lookup(components, "id"));

    if (type == ChatType::GROUP) {
        line::Group &group = groups[id];

        set_chat_participants(PURPLE_CONV_CHAT(conv), group);
    } else if (type == ChatType::ROOM) {
        line::Room &room = rooms[id];

        set_chat_participants(PURPLE_CONV_CHAT(conv), room);
    }

    c_out->send_getRecentMessages(id, 20);
    c_out->send([this, purple_id]() {
        std::vector<line::Message> recent_msgs;
        c_out->recv_getRecentMessages(recent_msgs);

        PurpleConversation *conv = purple_find_chat(conn, purple_id);
        if (!conv)
            return; // Chat went away already?

        for (auto i = recent_msgs.rbegin(); i != recent_msgs.rend(); i++) {
            line::Message &msg = *i;

            handle_message(msg, (msg.from == profile.mid), true);

            //push_recent_message(msg.id);
        }
    });
}

int PurpleLine::send_message(std::string to, std::string text) {
    line::Message msg;

    msg.from = profile.mid;
    msg.to = to;
    msg.text = text;

    c_out->send_sendMessage(0, msg);
    c_out->send([this, to]() {
        line::Message msg_back;

        try {
            c_out->recv_sendMessage(msg_back);
        } catch (line::TalkException &err) {
            PurpleConversation *conv = nullptr;

            conv = purple_find_conversation_with_account(
                PURPLE_CONV_TYPE_ANY,
                to.c_str(),
                acct);

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

        // Kludge >_>
        if (to[0] == 'u')
            push_recent_message(msg_back.id);
    });

    return 1;
}

int PurpleLine::send_im(const char *who, const char *message, PurpleMessageFlags flags) {
    return send_message(who, markup_unescape(message));
}

void PurpleLine::chat_leave(int id) {
    PurpleConversation *conv = purple_find_chat(conn, id);
    if (!conv)
        return;

    PurpleConvChat *chat = PURPLE_CONV_CHAT(conv);

    for (GList *buddies = purple_conv_chat_get_users(chat);
        buddies;
        buddies = g_list_next(buddies))
    {
        PurpleConvChatBuddy *buddy = (PurpleConvChatBuddy *)buddies->data;

        blist_remove_buddy(purple_conv_chat_cb_get_name(buddy), true, chat);
    }
}

int PurpleLine::chat_send(int id, const char *message, PurpleMessageFlags flags) {
    PurpleConversation *conv = purple_find_chat(conn, id);
    if (!conv) {
        purple_debug_warning("line", "Tried to send to a nonexistent chat.");
        return 0;
    }

    return send_message(purple_conversation_get_name(conv), markup_unescape(message));
}

void PurpleLine::push_recent_message(std::string id) {
    recent_messages.push_back(id);
    if (recent_messages.size() > 50)
        recent_messages.pop_front();
}

PurpleChat *PurpleLine::find_blist_chat(const char *name) {
    return blist_find_chat(name, ChatType::ANY);
}
