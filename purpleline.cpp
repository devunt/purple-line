#include "purpleline.hpp"

#include <iostream>

#include <protocol/TCompactProtocol.h>

#include <conversation.h>
#include <debug.h>
#include <sslconn.h>

#include "purplehttpclient.hpp"

static const char *LINE_HOST = "gd2.line.naver.jp";
static const int LINE_PORT = 443;

static const char *LINE_GROUP = "LINE";

PurpleLine::PurpleLine(PurpleConnection *conn, PurpleAccount *acct)
    : conn(conn), acct(acct)
{
    http_out = boost::make_shared<PurpleHttpClient>(acct, LINE_HOST, LINE_PORT, "/S4");

    client_out = boost::make_shared<line::LineClient>(
        boost::make_shared<apache::thrift::protocol::TCompactProtocol>(http_out));

    http_in = boost::make_shared<PurpleHttpClient>(acct, LINE_HOST, LINE_PORT, "/P4");

    client_in = boost::make_shared<line::LineClient>(
        boost::make_shared<apache::thrift::protocol::TCompactProtocol>(http_in));
}

PurpleLine::~PurpleLine() {
    http_out->close();
    http_in->close();
}

const char *PurpleLine::list_icon(PurpleAccount *, PurpleBuddy *)
{
    return "line";
}

GList *PurpleLine::status_types(PurpleAccount *) {
    GList *types = NULL;
    PurpleStatusType *t;

    t = purple_status_type_new_full(PURPLE_STATUS_AVAILABLE, NULL, NULL, TRUE, TRUE, FALSE);
    types = g_list_append(types, t);

    t = purple_status_type_new_full(PURPLE_STATUS_OFFLINE, NULL, NULL, TRUE, TRUE, FALSE);
    types = g_list_append(types, t);

    return types;
}

static struct proto_chat_entry id_chat_entry {
    .label = "ID",
    .identifier = "id",
    .required = TRUE,
};

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
    delete this;
}

GList *PurpleLine::chat_info() {
    return g_list_append(NULL, g_memdup(&id_chat_entry, sizeof(struct proto_chat_entry)));
}

void PurpleLine::start_login() {
    purple_connection_set_state(conn, PURPLE_CONNECTING);
    purple_connection_update_progress(conn, "Connecting", 0, 2);

    client_out->send_loginWithIdentityCredentialForCertificate(
        purple_account_get_username(acct),
        purple_account_get_password(acct),
        true,
        "127.0.0.1",
        "libpurple",
        line::IdentityProvider::LINE,
        "");
    http_out->send([this](int status) {
        if (status != 200) {
            purple_debug_warning("line", "Login status: %d", status);
            close();
            return;
        }

        line::LoginResult result;
        client_out->recv_loginWithIdentityCredentialForCertificate(result);

        // Re-open output client to update persistent headers
        http_out->close();

        http_out->set_auth_token(result.authToken);
        http_in->set_auth_token(result.authToken);

        get_last_op_revision();
    });
}

void PurpleLine::get_last_op_revision() {
    client_out->send_getLastOpRevision();
    http_out->send([this](int status) {
        local_rev = client_out->recv_getLastOpRevision();

        get_profile();
    });
}

void PurpleLine::get_profile() {
    client_out->send_getProfile();
    http_out->send([this](int status) {
        client_out->recv_getProfile(profile);

        purple_account_set_alias(acct, profile.displayName.c_str());

        std::cout << "Your profile: " << std::endl
            << "  ID: " << profile.mid << std::endl
            << "  Display name: " << profile.displayName << std::endl
            << "  Picture status: " << profile.pictureStatus << std::endl
            << "  Status message: " << profile.statusMessage << std::endl;

        purple_connection_set_state(conn, PURPLE_CONNECTED);
        purple_connection_update_progress(conn, "Connected", 1, 2);

        get_contacts();
    });
}

void PurpleLine::get_contacts() {
    client_out->send_getAllContactIds();
    http_out->send([this](int status) {
        std::vector<std::string> ids;
        client_out->recv_getAllContactIds(ids);

        client_out->send_getContacts(ids);
        http_out->send([this](int status) {
            std::vector<line::Contact> contacts;
            client_out->recv_getContacts(contacts);

            PurpleGroup *group = purple_find_group(LINE_GROUP);

            for (line::Contact &c: contacts) {
                std::cout << "Contact " << c.mid << std::endl
                    //<< "  Type: " << line::_ContactType_VALUES_TO_NAMES.at(c.type) << std::endl
                    << "  Status: " << line::_ContactStatus_VALUES_TO_NAMES.at(c.status) << std::endl
                    //<< "  Relation: " << line::_ContactRelation_VALUES_TO_NAMES.at(c.relation) << std::endl
                    << "  Display name: " << c.displayName << std::endl
                    << "  Picture status: " << c.pictureStatus << std::endl
                    << "  Status message: " << c.statusMessage << std::endl;

                if (c.status == line::ContactStatus::FRIEND) {
                    if (!group)
                        group = purple_group_new(LINE_GROUP);

                    PurpleBuddy *buddy = purple_find_buddy(acct, c.mid.c_str());
                    if (!buddy) {
                        buddy = purple_buddy_new(acct, c.mid.c_str(), c.displayName.c_str());
                        purple_blist_add_buddy(buddy, NULL, group, NULL);
                    }

                    purple_prpl_got_user_status(
                        acct,
                        c.mid.c_str(),
                        purple_primitive_get_id_from_type(PURPLE_STATUS_AVAILABLE),
                        NULL);

                    //purple_blist_alias_buddy(buddy, c.displayName.c_str());
                }
            }

            {
                // Add self as buddy for those longely debugging conversations

                PurpleBuddy *buddy = purple_find_buddy(acct, profile.mid.c_str());
                if (!buddy) {
                    buddy = purple_buddy_new(acct, profile.mid.c_str(), profile.displayName.c_str());
                    purple_blist_add_buddy(buddy, NULL, group, NULL);
                }

                purple_prpl_got_user_status(
                    acct,
                    profile.mid.c_str(),
                    purple_primitive_get_id_from_type(PURPLE_STATUS_AVAILABLE),
                    NULL);
            }

            get_groups();
        });
    });
}

void PurpleLine::get_groups() {
    client_out->send_getGroupIdsJoined();
    http_out->send([this](int status) {
        std::vector<std::string> ids;
        client_out->recv_getGroupIdsJoined(ids);

        client_out->send_getGroups(ids);
        http_out->send([this](int status) {
            std::vector<line::Group> groups;
            client_out->recv_getGroups(groups);

            PurpleGroup *group = purple_find_group(LINE_GROUP);

            for (line::Group &g: groups) {
                std::cout << "Group " << g.id << std::endl
                    << "  Name: " << g.name << std::endl
                    << "  Creator: " << g.creator.displayName << std::endl
                    << "  Member count: " << g.members.size() << std::endl;

                if (!group)
                    group = purple_group_new(LINE_GROUP);

                /*PurpleChat *chat = purple_blist_find_chat(acct, g.id.c_str());
                if (!chat) {
                    GHashTable *components = g_hash_table_new(g_str_hash, g_str_equal);
                    g_hash_table_insert(components, (gpointer)"id", (gpointer)g_strdup(g.id.c_str()));

                    chat = purple_chat_new(acct, g.name.c_str(), components);

                    purple_blist_add_chat(chat, group, NULL);
                }*/
            }

            // Start up return channel
            fetch_operations();
        });
    });
}

void PurpleLine::fetch_operations() {
    client_in->send_fetchOperations(local_rev, 50);
    http_in->send([this](int status) {
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
        client_in->recv_fetchOperations(operations);

        for (line::Operation &op: operations) {
            switch (op.type) {
                case line::OperationType::END_OF_OPERATION:
                    break;

                case line::OperationType::RECEIVE_MESSAGE:
                    handle_message(op.message, false);
                    break;

                case line::OperationType::SEND_MESSAGE:
                    handle_message(op.message, true);
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

void PurpleLine::handle_message(line::Message &msg, bool sent) {
    std::string text;
    int flags = 0;
    time_t mtime = (time_t)(msg.createdTime / 1000);

    if (std::find(recent_messages.cbegin(), recent_messages.cend(), msg.id) != recent_messages.cend()) {
        // We already processed this message. User is probably talking with himself.
        return;
    }

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
        flags |= PURPLE_MESSAGE_SEND;

        if (msg.toType == line::MessageToType::USER) {
            PurpleConversation *conv = purple_find_conversation_with_account(
                PURPLE_CONV_TYPE_IM,
                msg.to.c_str(),
                acct);

            if (conv) {
                purple_conv_im_write(
                    conv->u.im,
                    msg.from.c_str(),
                    text.c_str(),
                    (PurpleMessageFlags)flags,
                    mtime);
            }
        }
    } else {
        flags |= PURPLE_MESSAGE_RECV;

        if (msg.toType == line::MessageToType::USER && msg.to == profile.mid) {
            serv_got_im(
                conn,
                msg.from.c_str(),
                text.c_str(),
                (PurpleMessageFlags)flags,
                mtime);
        }
    }

    push_recent_message(msg.id);
}

int PurpleLine::send_im(const char *who, const char *message, PurpleMessageFlags flags) {
    line::Message msg;

    msg.from = profile.mid;
    msg.to = who;
    msg.text = message;

    client_out->send_sendMessage(0, msg);
    http_out->send([this](int status) {
        line::Message msg_back;
        client_out->recv_sendMessage(msg_back);

        push_recent_message(msg_back.id);
    });

    return 1;
}

void PurpleLine::push_recent_message(std::string id) {
    recent_messages.push_back(id);
    if (recent_messages.size() > 50)
        recent_messages.pop_front();
}
