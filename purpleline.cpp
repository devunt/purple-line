#include <algorithm>
#include <iostream>
#include <functional>

#include <time.h>

#include <glib.h>
#include <glib/gstdio.h>

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

    purple_cmd_register(
        "open",
        "w",
        PURPLE_CMD_P_PRPL,
        (PurpleCmdFlag)(PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT),
        LINE_PRPL_ID,
        WRAPPER(PurpleLine::cmd_open),
        "Opens an attachment (image, audio) by number.",
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

std::string PurpleLine::get_tmp_dir(bool create) {
    std::string name = "line-" + profile.mid;

    for (char c: name) {
        if (!(c == '-' || std::isalpha(c) || std::isdigit(c))) {
            name = "line";
            break;
        }
    }

    gchar *dir_p = g_build_filename(
        g_get_tmp_dir(),
        name.c_str(),
        nullptr);

    if (create)
        g_mkdir_with_parents(dir_p, 0700);

    std::string dir(dir_p);

    g_free(dir_p);

    return dir;
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

    plugin->login_start();
}

void PurpleLine::close() {
    disconnect_signals();

    if (temp_files.size()) {
        for (std::string &path: temp_files)
            g_unlink(path.c_str());

        g_rmdir(get_tmp_dir().c_str());
    }

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

std::string PurpleLine::conv_attachment_add(PurpleConversation *conv,
    line::ContentType::type type, std::string id)
{
    auto atts = (std::vector<Attachment> *)purple_conversation_get_data(conv, "line-attachments");
    if (!atts) {
        atts = new std::vector<Attachment>();
        purple_conversation_set_data(conv, "line-attachments", atts);
    }

    atts->emplace_back(type, id);

    return std::to_string(atts->size());
}

PurpleLine::Attachment *PurpleLine::conv_attachment_get(PurpleConversation *conv, std::string token)
{
    int index;

    try {
        index = std::stoi(token);
    } catch (...) {
        return nullptr;
    }

    auto atts = (std::vector<Attachment> *)purple_conversation_get_data(conv, "line-attachments");

    return (atts && index <= (int)atts->size()) ? &(*atts)[index - 1] : nullptr;
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

                if (conv) {
                    text += " <font color=\"#888888\">/open "
                        + conv_attachment_add(conv, msg.contentType, msg.id)
                        + "</font>";
                }

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

                if (conv) {
                    text += " <font color=\"#888888\">/open "
                        + conv_attachment_add(conv, msg.contentType, msg.id)
                        + "</font>";
                }
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

    auto queue = (std::vector<line::Message> *)
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

    auto atts = (std::vector<Attachment> *)purple_conversation_get_data(conv, "line-attachments");
    if (atts) {
        purple_conversation_set_data(conv, "line-attachments", nullptr);
        delete atts;
    }
}

void PurpleLine::push_recent_message(std::string id) {
    recent_messages.push_back(id);
    if (recent_messages.size() > 50)
        recent_messages.pop_front();
}

static const char *sticker_fields[] = { "STKVER", "STKPKGID", "STKID" };

PurpleCmdRet PurpleLine::cmd_sticker(PurpleConversation *conv,
    const gchar *, gchar **args, gchar **error, void *)
{
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
    const gchar *, gchar **args, gchar **error, void *)
{
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

static std::map<line::ContentType::type, std::string> attachment_extensions = {
    { line::ContentType::IMAGE, ".jpg" },
    { line::ContentType::AUDIO, ".mp3" },
};

PurpleCmdRet PurpleLine::cmd_open(PurpleConversation *conv,
    const gchar *, gchar **args, gchar **error, void *)
{
    std::string token(args[0]);

    Attachment *att = conv_attachment_get(conv, token);
    if (!att) {
        *error = g_strdup("No such attachment.");
        return PURPLE_CMD_RET_FAILED;
    }

    if (att->path != "" && g_file_test(att->path.c_str(), G_FILE_TEST_EXISTS)) {
        purple_notify_uri(conn, att->path.c_str());
        return PURPLE_CMD_RET_OK;
    }

    // Ensure there's nothing funny about the id as we're going to use it as a path element
    try {
        std::stoll(att->id);
    } catch (...) {
        *error = g_strdup("Failed to download attachment.");
        return PURPLE_CMD_RET_FAILED;
    }

    std::string ext = ".jpg";
    if (attachment_extensions.count(att->type))
        ext = attachment_extensions[att->type];

    std::string dir = get_tmp_dir(true);

    gchar *path_p = g_build_filename(
        dir.c_str(),
        (att->id + ext).c_str(),
        nullptr);

    std::string path(path_p);

    g_free(path_p);

    purple_conversation_write(
        conv,
        "",
        "Downloading attachment...",
        (PurpleMessageFlags)PURPLE_MESSAGE_SYSTEM,
        time(NULL));

    std::string url = std::string(LINE_OS_URL) + "os/m/"+ att->id;

    http.request_auth(url,
        [this, path, token,
            ctype=purple_conversation_get_type(conv),
            cname=std::string(purple_conversation_get_name(conv))]
        (int status, const guchar *data, gsize len)
        {
            if (status == 200 && data && len > 0) {
                g_file_set_contents(path.c_str(), (const char *)data, len, nullptr);

                temp_files.push_back(path);

                PurpleConversation *conv = purple_find_conversation_with_account(
                    ctype, cname.c_str(), acct);

                if (conv) {
                    Attachment *att = conv_attachment_get(conv, token);
                    if (att)
                        att->path = path;
                }

                purple_notify_uri(conn, path.c_str());
            } else {
                notify_error("Failed to download attachment.");
            }
        });

    return PURPLE_CMD_RET_OK;
}

void PurpleLine::notify_error(std::string msg) {
    purple_notify_error(
        (void *)conn,
        "LINE error",
        msg.c_str(),
        nullptr);
}
