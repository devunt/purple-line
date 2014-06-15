#include "purpleline.hpp"

std::map<ChatType, std::string> PurpleLine::chat_type_to_string {
    { ChatType::GROUP, "group" },
    { ChatType::ROOM, "room" },
};

ChatType PurpleLine::get_chat_type(const char *type_ptr) {
    if (!type_ptr)
        return ChatType::ANY; // Invalid

    std::string type_str(type_ptr);

    if (type_str == chat_type_to_string[ChatType::GROUP])
        return ChatType::GROUP;
    else if (type_str == chat_type_to_string[ChatType::ROOM])
        return ChatType::ROOM;
    else if (type_str == chat_type_to_string[ChatType::GROUP_INVITE])
        return ChatType::GROUP_INVITE;

    return ChatType::ANY; // Invalid
}

std::string PurpleLine::get_room_display_name(line::Room &room) {
    std::vector<line::Contact *> rcontacts;

    for (line::Contact &rc: room.contacts) {
        if (contacts.count(rc.mid) == 1)
            rcontacts.push_back(&contacts[rc.mid]);
    }

    if (rcontacts.size() == 0)
        return "Empty chat";

    std::stringstream ss;
    ss << "Chat with ";

    switch (rcontacts.size()) {
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

void PurpleLine::handle_group_invite(
    line::Group &group,
    line::Contact &invitee,
    line::Contact &inviter)
{
    blist_update_buddy(invitee, true);

    if (invitee.mid == profile.mid) {
        // Current user was invited - show popup

        GHashTable *components = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

        g_hash_table_insert(components, g_strdup("type"),
            g_strdup(chat_type_to_string[ChatType::GROUP_INVITE].c_str()));
        g_hash_table_insert(components, g_strdup("id"), g_strdup(group.id.c_str()));

        // Invites on initial sync do not have inviter data
        std::string who = (inviter.__isset.mid)
            ? inviter.displayName
            : std::string("A member");

        serv_got_chat_invite(
            conn,
            group.name.c_str(),
            who.c_str(),
            nullptr,
            components);
    } else {
        // Another user was invited - if a chat is open, add the user

        PurpleConversation *conv = purple_find_conversation_with_account(
            PURPLE_CONV_TYPE_CHAT,
            group.id.c_str(),
            acct);

        if (conv) {
            std::string msg = "Invited by " + inviter.displayName;

            purple_conv_chat_add_user(
                PURPLE_CONV_CHAT(conv),
                invitee.mid.c_str(),
                msg.c_str(),
                PURPLE_CBFLAGS_AWAY,
                TRUE);
        }
    }
}

void PurpleLine::set_chat_participants(PurpleConvChat *chat, line::Group &group) {
    purple_conv_chat_clear_users(chat);

    GList *users = NULL, *flags = NULL;

    for (line::Contact &c: group.members) {
        line::Contact &contact = get_up_to_date_contact(c);

        blist_update_buddy(contact, true);

        int cbflags = 0;

        if (contact.mid == group.creator.mid)
            cbflags |= PURPLE_CBFLAGS_FOUNDER;

        users = g_list_prepend(users, (gpointer)contact.mid.c_str());
        flags = g_list_prepend(flags, GINT_TO_POINTER(cbflags));
    }

    for (line::Contact &c: group.invitee) {
        line::Contact &contact = get_up_to_date_contact(c);

        blist_update_buddy(contact, true);

        users = g_list_prepend(users, (gpointer)contact.mid.c_str());
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
        if (contacts.count(rc.mid) == 0)
            blist_update_buddy(rc.mid, true);
        else
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

char *PurpleLine::get_chat_name(GHashTable *components) {
    return g_strdup((char *)g_hash_table_lookup(components, (gconstpointer)"id"));
}

GList *PurpleLine::chat_info() {
    struct proto_chat_entry *entry = g_new0(struct proto_chat_entry, 1);

    entry->label = "Group _Name";
    entry->identifier = "name",
    entry->required = TRUE;

    return g_list_append(nullptr, entry);
}

void PurpleLine::join_chat(GHashTable *components) {
    char *id_ptr = (char *)g_hash_table_lookup(components, "id");
    if (!id_ptr) {
        purple_debug_warning("line", "Tried to join a chat with no id.");
        return;
    }

    std::string id(id_ptr);

    ChatType type = get_chat_type((char *)g_hash_table_lookup(components, "type"));

    if (type == ChatType::ANY) {
        purple_debug_warning("line", "Tried to join a chat with weird type.");
        return;
    }

    if (type == ChatType::GROUP_INVITE) {
        c_out->send_acceptGroupInvitation(0, id);
        c_out->send([this, id]{
            try {
                c_out->recv_acceptGroupInvitation();
            } catch (line::TalkException &err) {
                purple_notify_warning(
                    (void *)conn,
                    "Failed to join LINE group",
                    "Your invitation was probably cancelled.",
                    err.reason.c_str());
                return;
            }

            c_out->send_getGroup(id);
            c_out->send([this]{
                line::Group group;
                c_out->recv_getGroup(group);

                if (!group.__isset.id) {
                    purple_debug_warning("line", "Couldn't get group: %s", group.id.c_str());
                    return;
                }

                join_chat_success(ChatType::GROUP, group.id);
            });
        });

        return;
    }

    join_chat_success(type, id);
}

void PurpleLine::join_chat_success(ChatType type, std::string id) {
    // Assume chat is on buddy list for now.

    PurpleConversation *conv = serv_got_joined_chat(
        conn,
        next_purple_id++,
        id.c_str());

    if (type == ChatType::GROUP) {
        line::Group &group = groups[id];

        set_chat_participants(PURPLE_CONV_CHAT(conv), group);
    } else if (type == ChatType::ROOM) {
        line::Room &room = rooms[id];

        set_chat_participants(PURPLE_CONV_CHAT(conv), room);
    }
}

void PurpleLine::reject_chat(GHashTable *components) {
    char *id_ptr = (char *)g_hash_table_lookup(components, "id");
    if (!id_ptr) {
        purple_debug_warning("line", "Tried to reject an invitation with no id.");
        return;
    }

    std::string id(id_ptr);

    c_out->send_rejectGroupInvitation(0, id);
    c_out->send([this]() {
        try {
            c_out->recv_rejectGroupInvitation();
        } catch (line::TalkException &err) {
            notify_error(err.reason);
        }
    });
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

PurpleChat *PurpleLine::find_blist_chat(const char *name) {
    return blist_find_chat(name, ChatType::ANY);
}
