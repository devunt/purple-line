#include <algorithm>
#include <iostream>
#include <functional>

#include <time.h>

#include <cmds.h>
#include <connection.h>
#include <conversation.h>
#include <debug.h>
#include <eventloop.h>
#include <notify.h>
#include <request.h>
#include <sslconn.h>
#include <util.h>

#include "purpleline.hpp"
#include "wrapper.hpp"

#define LINE_ACCOUNT_CERTIFICATE "line-certificate"

std::string PurpleLine::markup_escape(std::string const &text) {
    gchar *escaped = purple_markup_escape_text(text.c_str(), text.size());
    std::string result(escaped);
    g_free(escaped);

    return result;
}

std::string PurpleLine::markup_unescape(std::string const &markup) {
    gchar *unescaped = purple_unescape_html(markup.c_str());
    std::string result(unescaped);
    g_free(unescaped);

    return result;
}

std::string PurpleLine::url_encode(std::string const &str) {
    return purple_url_encode(str.c_str());
}

std::string PurpleLine::get_sticker_id(line::Message &msg) {
    std::map<std::string, std::string> &meta = msg.contentMetadata;

    if (meta.count("STKID") == 0 || meta.count("STKVER") == 0 || meta.count("STKPKGID") == 0)
        return "";

    std::stringstream id;

    id << "[LINE sticker "
        << meta["STKVER"] << "/"
        << meta["STKPKGID"] << "/"
        << meta["STKID"];

    if (meta.count("STKTXT") == 1)
        id << " " << meta["STKTXT"];

    id << "]";

    return id.str();
}

std::string PurpleLine::get_sticker_url(line::Message &msg, bool thumb) {
    std::map<std::string, std::string> &meta = msg.contentMetadata;

    int ver;
    std::stringstream ss(meta["STKVER"]);
    ss >> ver;

    std::stringstream url;

    url << LINE_STICKER_URL
        << (ver / 1000000) << "/" << (ver / 1000) << "/" << (ver % 1000) << "/"
        << meta["STKPKGID"] << "/"
        << "PC/stickers/"
        << meta["STKID"];

    if (thumb)
        url << "_key";

    url << ".png";

    return url.str();
}

PurpleLine::PurpleLine(PurpleConnection *conn, PurpleAccount *acct) :
    conn(conn),
    acct(acct),
    http(acct),
    poller(*this),
    pin_verifier(*this),
    next_purple_id(1)
{
    c_out = boost::make_shared<ThriftClient>(acct, conn, LINE_LOGIN_PATH);
}

PurpleLine::~PurpleLine() {
    c_out->close();
}

void PurpleLine::register_commands() {
    purple_cmd_register(
        "sticker",
        "w",
        PURPLE_CMD_P_PRPL,
        (PurpleCmdFlag)(PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT),
        LINE_PRPL_ID,
        WRAPPER(PurpleLine::cmd_sticker),
        "Sends a sticker. The argument should be of the format VER/PKGID/ID.",
        nullptr);

    purple_cmd_register(
        "history",
        "w",
        PURPLE_CMD_P_PRPL,
        (PurpleCmdFlag)
            (PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_ALLOW_WRONG_ARGS),
        LINE_PRPL_ID,
        WRAPPER(PurpleLine::cmd_history),
        "Shows more chat history. Optional argument specifies number of messages to show.",
        nullptr);
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

    plugin->connect_signals();

    plugin->start_login();
}

void PurpleLine::close() {
    disconnect_signals();

    delete this;
}

void PurpleLine::connect_signals() {
    purple_signal_connect(
        purple_blist_get_handle(),
        "blist-node-removed",
        (void *)this,
        PURPLE_CALLBACK(WRAPPER_TYPE(PurpleLine::signal_blist_node_removed, signal)),
        (void *)this);

    purple_signal_connect(
        purple_conversations_get_handle(),
        "conversation-created",
        (void *)this,
        PURPLE_CALLBACK(WRAPPER_TYPE(PurpleLine::signal_conversation_created, signal)),
        (void *)this);

    purple_signal_connect(
        purple_conversations_get_handle(),
        "deleting-conversation",
        (void *)this,
        PURPLE_CALLBACK(WRAPPER_TYPE(PurpleLine::signal_deleting_conversation, signal)),
        (void *)this);
}

void PurpleLine::disconnect_signals() {
    purple_signal_disconnect(
        purple_blist_get_handle(),
        "blist-node-removed",
        (void *)this,
        PURPLE_CALLBACK(WRAPPER_TYPE(PurpleLine::signal_blist_node_removed, signal)));

    purple_signal_disconnect(
        purple_conversations_get_handle(),
        "conversation-created",
        (void *)this,
        PURPLE_CALLBACK(WRAPPER_TYPE(PurpleLine::signal_conversation_created, signal)));


    purple_signal_disconnect(
        purple_conversations_get_handle(),
        "deleting-conversation",
        (void *)this,
        PURPLE_CALLBACK(WRAPPER_TYPE(PurpleLine::signal_deleting_conversation, signal)));
}

void PurpleLine::start_login() {
    purple_connection_set_state(conn, PURPLE_CONNECTING);
    purple_connection_update_progress(conn, "Logging in", 0, 3);

    std::string certificate(purple_account_get_string(acct, LINE_ACCOUNT_CERTIFICATE, ""));

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
            pin_verifier.verify(result, [this](std::string auth_token, std::string certificate) {
                if (certificate != "") {
                    purple_account_set_string(
                        acct,
                        LINE_ACCOUNT_CERTIFICATE,
                        certificate.c_str());
                }

                got_auth_token(auth_token);
            });
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

void PurpleLine::got_auth_token(std::string auth_token) {
    // Re-open output client to update persistent headers
    c_out->close();
    c_out->set_path(LINE_COMMAND_PATH);

    c_out->set_auth_token(auth_token);
    poller.set_auth_token(auth_token);
    http.set_auth_token(auth_token);

    get_last_op_revision();
}

void PurpleLine::get_last_op_revision() {
    c_out->send_getLastOpRevision();
    c_out->send([this]() {
        poller.set_local_rev(c_out->recv_getLastOpRevision());

        get_profile();
    });
}

void PurpleLine::get_profile() {
    c_out->send_getProfile();
    c_out->send([this]() {
        c_out->recv_getProfile(profile);

        profile_contact.mid = profile.mid;
        profile_contact.displayName = profile.displayName;

        // Update display name
        purple_account_set_alias(acct, profile.displayName.c_str());

        purple_connection_set_state(conn, PURPLE_CONNECTED);
        purple_connection_update_progress(conn, "Synchronizing buddy list", 1, 3);

        // Update account icon (not sure if there's a way to tell whether it has changed, maybe
        // pictureStatus?)
        if (profile.picturePath != "") {
            std::string pic_path = profile.picturePath.substr(1) + "/preview";
            //if (icon_path != purple_account_get_string(acct, "icon_path", "")) {
                http.request_auth(LINE_OS_URL + pic_path,
                    [this](int status, const guchar *data, gsize len)
                {
                    if (status != 200 || !data)
                        return;

                    purple_buddy_icons_set_account_icon(
                        acct,
                        (guchar *)g_memdup(data, len),
                        len);

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

    get_group_invites();
}

void PurpleLine::get_group_invites() {
    c_out->send_getGroupIdsInvited();
    c_out->send([this]() {
        std::vector<std::string> gids;
        c_out->recv_getGroupIdsInvited(gids);

        if (gids.size() == 0) {
            sync_done();
            return;
        }

        c_out->send_getGroups(gids);
        c_out->send([this]() {
            std::vector<line::Group> groups;
            c_out->recv_getGroups(groups);

            for (line::Group &g: groups)
                handle_group_invite(g, profile_contact, no_contact);

            sync_done();
        });
    });
}

void PurpleLine::sync_done() {
    poller.start();

    purple_connection_update_progress(conn, "Connected", 2, 3);
}

void PurpleLine::handle_message(line::Message &msg, bool replay) {
    std::string text;
    int flags = 0;
    time_t mtime = (time_t)(msg.createdTime / 1000);

    bool sent = (msg.from == profile.mid);

    if (std::find(recent_messages.cbegin(), recent_messages.cend(), msg.id)
        != recent_messages.cend())
    {
        // We already processed this message. User is probably talking with himself.
        return;
    }

    // Hack
    if (msg.from == msg.to)
        push_recent_message(msg.id);

    PurpleConversation *conv = purple_find_conversation_with_account(
        (msg.toType == line::MIDType::USER ? PURPLE_CONV_TYPE_IM : PURPLE_CONV_TYPE_CHAT),
        ((!sent && msg.toType == line::MIDType::USER) ? msg.from.c_str() : msg.to.c_str()),
        acct);

    // If this is a new received IM, create the conversation if it doesn't exist
    if (!conv && !sent && msg.toType == line::MIDType::USER)
        conv = purple_conversation_new(PURPLE_CONV_TYPE_IM, acct, msg.from.c_str());

    // If this is a new conversation, we're not replaying history and history hasn't been fetched
    // yet, queue the message instead of showing it.
    if (conv && !replay) {
        auto *queue = (std::vector<line::Message> *)
            purple_conversation_get_data(conv, "line-message-queue");

        if (queue) {
            queue->push_back(msg);
            return;
        }
    }

    // Replaying messages from history
    // Unfortunately Pidgin displays messages with this flag with odd formatting and no username.
    // Disable for now.
    //if (replay)
    //    flags |= PURPLE_MESSAGE_NO_LOG;

    switch (msg.contentType) {
        case line::ContentType::NONE: // actually text
        case line::ContentType::LOCATION:
            if (msg.__isset.location) {
                line::Location &loc = msg.location;

                text = markup_escape(loc.title)
                    + " | <a href=\"https://maps.google.com/?q=" + url_encode(loc.address)
                    + "&ll=" + std::to_string(loc.latitude)
                    + "," + std::to_string(loc.longitude)
                    + "\">"
                    + (loc.address.size()
                        ? markup_escape(loc.address)
                        : "(no address)")
                    + "</a>";
            } else {
                text = markup_escape(msg.text);
            }
            break;

        case line::ContentType::STICKER:
            {
                std::string id = get_sticker_id(msg);

                if (id == "")  {
                    text = "<em>[Broken sticker]</em>";

                    purple_debug_warning("line", "Got a broken sticker.\n");
                } else {
                    text = id;

                    if (conv
                        && purple_conv_custom_smiley_add(conv, id.c_str(), "id", id.c_str(), TRUE))
                    {
                        http.request(get_sticker_url(msg),
                            [this, id, conv](int status, const guchar *data, gsize len)
                            {
                                if (status == 200 && data && len > 0) {
                                    purple_conv_custom_smiley_write(
                                        conv,
                                        id.c_str(),
                                        data,
                                        len);
                                } else {
                                    purple_debug_warning(
                                        "line",
                                        "Couldn't download sticker. Status: %d\n",
                                        status);
                                }

                                purple_conv_custom_smiley_close(conv, id.c_str());
                            });
                    }
                }
            }
            break;

        case line::ContentType::IMAGE:
            {
                std::string id = "[LINE image " + msg.id + "]";

                text = id;

                if (!conv
                    || !purple_conv_custom_smiley_add(conv, id.c_str(), "id", id.c_str(), TRUE))
                {
                    break;
                }

                if (msg.contentPreview.size() > 0) {
                    purple_conv_custom_smiley_write(
                        conv,
                        id.c_str(),
                        (const guchar *)msg.contentPreview.c_str(),
                        msg.contentPreview.size());

                    purple_conv_custom_smiley_close(conv, id.c_str());
                } else {
                    std::string preview_url = msg.contentMetadata.count("PREVIEW_URL")
                        ? msg.contentMetadata["PREVIEW_URL"]
                        : std::string(LINE_OS_URL) + "os/m/" + msg.id + "/preview";

                    http.request_auth(preview_url,
                        [this, id, conv](int status, const guchar *data, gsize len)
                        {
                            if (status == 200 && data && len > 0) {
                                purple_conv_custom_smiley_write(
                                    conv,
                                    id.c_str(),
                                    data,
                                    len);
                            } else {
                                purple_debug_warning(
                                    "line",
                                    "Couldn't download image message. Status: %d",
                                    status);
                            }

                            purple_conv_custom_smiley_close(conv, id.c_str());
                        });
                }
            }
            break;

        case line::ContentType::AUDIO:
            {
                text = "[Audio message";

                if (msg.contentMetadata.count("AUDLEN")) {
                    int len = 0;

                    try {
                        len = std::stoi(msg.contentMetadata["AUDLEN"]);
                    } catch(...) { /* ignore */ }

                    if (len > 0) {
                        text += " "
                            + std::to_string(len / 1000)
                            + "."
                            + std::to_string((len % 1000) / 100)
                            + "s";
                    }
                }

                text += "]";
            }
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

        write_message(conv, msg, mtime, flags | PURPLE_MESSAGE_SEND, text);
    } else {
        // Messages received from other users

        flags |= PURPLE_MESSAGE_RECV;

        if (replay) {
            // Write replayed messages instead of serv_got_* to avoid Pidgin's IM sound

            write_message(conv, msg, mtime, flags, text);
        } else {
            if (msg.toType == line::MIDType::USER) {
                serv_got_im(
                    conn,
                    msg.from.c_str(),
                    text.c_str(),
                    (PurpleMessageFlags)flags,
                    mtime);
            } else if (msg.toType == line::MIDType::GROUP || msg.toType == line::MIDType::ROOM) {
                serv_got_chat_in(
                    conn,
                    purple_conv_chat_get_id(PURPLE_CONV_CHAT(conv)),
                    msg.from.c_str(),
                    (PurpleMessageFlags)flags,
                    text.c_str(),
                    mtime);
            }
        }
    }
}

void PurpleLine::write_message(PurpleConversation *conv, line::Message &msg,
    time_t mtime, int flags, std::string text)
{
    if (!conv)
        return;

    if (msg.toType == line::MIDType::USER) {
        purple_conv_im_write(
            PURPLE_CONV_IM(conv),
            msg.from.c_str(),
            text.c_str(),
            (PurpleMessageFlags)flags,
            mtime);
    } else if (msg.toType == line::MIDType::GROUP || msg.toType == line::MIDType::ROOM) {
        purple_conv_chat_write(
            PURPLE_CONV_CHAT(conv),
            msg.from.c_str(),
            text.c_str(),
            (PurpleMessageFlags)flags,
            mtime);
    }
}

line::Contact &PurpleLine::get_up_to_date_contact(line::Contact &c) {
    return (contacts.count(c.mid) != 0) ? contacts[c.mid] : c;
}

int PurpleLine::send_message(std::string to, std::string text) {
    line::Message msg;

    msg.contentType = line::ContentType::NONE;
    msg.from = profile.mid;
    msg.to = to;
    msg.text = text;

    send_message(msg);

    return 1;
}

void PurpleLine::send_message(line::Message &msg) {
    std::string to(msg.to);

    c_out->send_sendMessage(0, msg);
    c_out->send([this, to]() {
        line::Message msg_back;

        try {
            c_out->recv_sendMessage(msg_back);
        } catch (line::TalkException &err) {
            std::string msg = "Failed to send message: " + err.reason;

            PurpleConversation *conv = purple_find_conversation_with_account(
                PURPLE_CONV_TYPE_ANY,
                to.c_str(),
                acct);

            if (conv) {
                purple_conversation_write(
                    conv,
                    "",
                    msg.c_str(),
                    (PurpleMessageFlags)PURPLE_MESSAGE_ERROR,
                    time(NULL));
            } else {
                notify_error(msg);
            }

            return;
        }

        // Kludge >_>
        if (to[0] == 'u')
            push_recent_message(msg_back.id);
    });
}

int PurpleLine::send_im(const char *who, const char *message, PurpleMessageFlags flags) {
    return send_message(who, markup_unescape(message));
}

void PurpleLine::remove_buddy(PurpleBuddy *buddy, PurpleGroup *) {
    c_out->send_updateContactSetting(
        0,
        purple_buddy_get_name(buddy),
        line::ContactSetting::CONTACT_SETTING_DELETE,
        "true");
    c_out->send([this]{
        try {
            c_out->recv_updateContactSetting();
        } catch (line::TalkException &err) {
            notify_error(std::string("Couldn't delete buddy: ") + err.reason);
        }
    });
}

void PurpleLine::signal_blist_node_removed(PurpleBlistNode *node) {
    if (!(PURPLE_BLIST_NODE_IS_CHAT(node)
        && purple_chat_get_account(PURPLE_CHAT(node)) == acct))
    {
        return;
    }

    GHashTable *components = purple_chat_get_components(PURPLE_CHAT(node));

    char *id_ptr = (char *)g_hash_table_lookup(components, "id");
    if (!id_ptr) {
        purple_debug_warning("line", "Tried to remove a chat with no id.");
        return;
    }

    std::string id(id_ptr);

    ChatType type = get_chat_type((char *)g_hash_table_lookup(components, "type"));

    if (type == ChatType::ROOM) {
        c_out->send_leaveRoom(0, id);
        c_out->send([this]{
            try {
                c_out->recv_leaveRoom();
            } catch (line::TalkException &err) {
                notify_error(std::string("Couldn't leave from chat: ") + err.reason);
            }
        });
    } else if (type == ChatType::GROUP) {
        c_out->send_leaveGroup(0, id);
        c_out->send([this]{
            try {
                c_out->recv_leaveGroup();
            } catch (line::TalkException &err) {
                notify_error(std::string("Couldn't leave from group: ") + err.reason);
            }
        });
    } else {
        purple_debug_warning("line", "Tried to remove a chat with no type.");
        return;
    }
}

void PurpleLine::signal_conversation_created(PurpleConversation *conv) {
    if (purple_conversation_get_account(conv) != acct)
        return;

    // Start queuing messages while the history is fetched
    purple_conversation_set_data(conv, "line-message-queue", new std::vector<line::Message>());

    fetch_conversation_history(conv, 10, false);
}

void PurpleLine::fetch_conversation_history(PurpleConversation *conv, int count, bool requested) {
    PurpleConversationType type = conv->type;
    std::string name(purple_conversation_get_name(conv));

    int64_t end_seq = -1;

    int64_t *end_seq_p = (int64_t *)purple_conversation_get_data(conv, "line-end-seq");
    if (end_seq_p)
        end_seq = *end_seq_p;

    if (end_seq != -1)
        c_out->send_getPreviousMessages(name, end_seq - 1, count);
    else
        c_out->send_getRecentMessages(name, count);

    c_out->send([this, requested, type, name, end_seq]() {
        int64_t new_end_seq = end_seq;

        std::vector<line::Message> recent_msgs;

        if (end_seq != -1)
            c_out->recv_getPreviousMessages(recent_msgs);
        else
            c_out->recv_getRecentMessages(recent_msgs);

        PurpleConversation *conv = purple_find_conversation_with_account(type, name.c_str(), acct);
        if (!conv)
            return; // Conversation died while fetching messages

        auto *queue = (std::vector<line::Message> *)
            purple_conversation_get_data(conv, "line-message-queue");

        purple_conversation_set_data(conv, "line-message-queue", nullptr);

        // Find least seq value from messages for future history queries
        for (line::Message &msg: recent_msgs) {
            if (msg.contentMetadata.count("seq")) {
                try {
                    int64_t seq = std::stoll(msg.contentMetadata["seq"]);

                    if (new_end_seq == -1 || seq < new_end_seq)
                        new_end_seq = seq;
                } catch (...) { /* ignore parse error */ }
            }
        }

        if (queue) {
            // If there's a message queue, remove any already-queued messages in the recent message
            // list to prevent them showing up twice.

            recent_msgs.erase(
                std::remove_if(
                    recent_msgs.begin(),
                    recent_msgs.end(),
                    [&queue](line::Message &rm) {
                        auto r = find_if(
                            queue->begin(),
                            queue->end(),
                            [&rm](line::Message &qm) { return qm.id == rm.id; });

                        return (r != queue->end());
                    }),
                recent_msgs.end());
        }

        if (recent_msgs.size()) {
            purple_conversation_write(
                conv,
                "",
                "<strong>Message history</strong>",
                (PurpleMessageFlags)PURPLE_MESSAGE_RAW,
                time(NULL));

            for (auto msgi = recent_msgs.rbegin(); msgi != recent_msgs.rend(); msgi++)
                handle_message(*msgi, true);

            purple_conversation_write(
                conv,
                "",
                "<hr>",
                (PurpleMessageFlags)PURPLE_MESSAGE_RAW,
                time(NULL));
        } else {
            if (requested) {
                // If history was requested by the user and there is none, let the user know

                purple_conversation_write(
                    conv,
                    "",
                    "<strong>No more history</strong>",
                    (PurpleMessageFlags)PURPLE_MESSAGE_RAW,
                    time(NULL));
            }
        }

        // If there's a message queue, play it back now
        if (queue) {
            for (line::Message &msg: *queue)
                handle_message(msg, false);

            delete queue;
        }

        int64_t *end_seq_p = (int64_t *)purple_conversation_get_data(conv, "line-end-seq");
        if (end_seq_p)
            delete end_seq_p;

        purple_conversation_set_data(conv, "line-end-seq", new int64_t(new_end_seq));
    });
}

void PurpleLine::signal_deleting_conversation(PurpleConversation *conv) {
    if (purple_conversation_get_account(conv) != acct)
        return;

    auto *queue = (std::vector<line::Message> *)
        purple_conversation_get_data(conv, "line-message-queue");

    if (queue) {
        purple_conversation_set_data(conv, "line-message-queue", nullptr);
        delete queue;
    }

    int64_t *end_seq_p = (int64_t *)purple_conversation_get_data(conv, "line-end-seq");
    if (end_seq_p) {
        purple_conversation_set_data(conv, "line-end-seq", nullptr);
        delete end_seq_p;
    }
}

void PurpleLine::push_recent_message(std::string id) {
    recent_messages.push_back(id);
    if (recent_messages.size() > 50)
        recent_messages.pop_front();
}

static const char *sticker_fields[] = { "STKVER", "STKPKGID", "STKID" };

PurpleCmdRet PurpleLine::cmd_sticker(PurpleConversation *conv,
    const gchar *cmd, gchar **args, gchar **error, void *data)
{
    (void)cmd;
    (void)data;

    line::Message msg;

    std::stringstream ss(args[0]);
    std::string item;

    int part = 0;
    while (std::getline(ss, item, '/')) {
        if (part == 3) {
            *error = g_strdup("Invalid sticker.");
            return PURPLE_CMD_RET_FAILED;
        }

        msg.contentMetadata[sticker_fields[part]] = item;

        part++;
    }

    if (part != 3) {
        *error = g_strdup("Invalid sticker.");
        return PURPLE_CMD_RET_FAILED;
    }

    msg.contentType = line::ContentType::STICKER;
    msg.from = profile.mid;
    msg.to = purple_conversation_get_name(conv);

    handle_message(msg, false);

    send_message(msg);

    return PURPLE_CMD_RET_OK;
}

PurpleCmdRet PurpleLine::cmd_history(PurpleConversation *conv,
    const gchar *cmd, gchar **args, gchar **error, void *data)
{
    (void)cmd;
    (void)data;

    int count = 10;

    if (args[0]) {
        try {
            count = std::stoi(args[0]);
        } catch (...) {
            *error = g_strdup("Invalid message count.");
            return PURPLE_CMD_RET_FAILED;
        }
    }

    fetch_conversation_history(conv, count, true);

    return PURPLE_CMD_RET_OK;
}

void PurpleLine::notify_error(std::string msg) {
    purple_notify_error(
        (void *)conn,
        "LINE error",
        msg.c_str(),
        nullptr);
}
