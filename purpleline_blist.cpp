#include <conversation.h>
#include <debug.h>
#include <sslconn.h>
#include <util.h>

#include "purpleline.hpp"

static const char *LINE_GROUP = "LINE";
static const char *LINE_TEMP_GROUP = "LINE Temporary";

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
        int flags = purple_blist_node_get_flags(PURPLE_BLIST_NODE(buddy));

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

        if (temporary)
            purple_blist_node_set_flags(PURPLE_BLIST_NODE(buddy), PURPLE_BLIST_NODE_FLAG_NO_SAVE);

        purple_blist_add_buddy(
            buddy,
            nullptr,
            blist_ensure_group(temporary ? LINE_TEMP_GROUP : LINE_GROUP, temporary),
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
PurpleBuddy *PurpleLine::blist_update_buddy(line::Contact &contact, bool temporary) {
    contacts[contact.mid] = contact;

    PurpleBuddy *buddy = blist_ensure_buddy(contact.mid.c_str(), temporary);
    if (!buddy) {
        purple_debug_warning("line", "Tried to update a non-existent buddy %s\n",
            contact.mid.c_str());
        return nullptr;
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

    // Set actual friends as available and temporary friends as temporary. Also set status text.
    purple_prpl_got_user_status(
        acct,
        contact.mid.c_str(),
        PURPLE_BLIST_NODE_HAS_FLAG(buddy, PURPLE_BLIST_NODE_FLAG_NO_SAVE)
            ? "temporary"
            : purple_primitive_get_id_from_type(PURPLE_STATUS_AVAILABLE),
        "message", contact.statusMessage.c_str(),
        nullptr);

    if (contact.attributes & 32)
        purple_blist_node_set_bool(PURPLE_BLIST_NODE(buddy), "official_account", TRUE);
    else
        purple_blist_node_remove_setting(PURPLE_BLIST_NODE(buddy), "official_account");

    return buddy;
}

bool PurpleLine::blist_is_buddy_in_any_conversation(std::string uid,
    PurpleConvChat *ignore_chat)
{
    for (GList * convs = purple_get_conversations();
        convs;
        convs = g_list_next(convs))
    {
        PurpleConversation *conv = (PurpleConversation *)convs->data;

        if (purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_IM) {
            if (uid == purple_conversation_get_name(conv))
                return true;
        } else if (purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_CHAT) {
            if (PURPLE_CONV_CHAT(conv) == ignore_chat)
                continue;

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

void PurpleLine::blist_remove_buddy(std::string uid,
    bool temporary_only, PurpleConvChat *ignore_chat)
{
    PurpleBuddy *buddy = purple_find_buddy(acct, uid.c_str());
    if (!buddy)
        return;

    bool temporary = PURPLE_BLIST_NODE_HAS_FLAG(buddy, PURPLE_BLIST_NODE_FLAG_NO_SAVE);

    if (temporary_only && !temporary)
        return;

    if (blist_is_buddy_in_any_conversation(uid, ignore_chat)) {
        // If the buddy exists in a conversation, demote them to a temporary buddy instead of
        // deleting them completely.

        if (temporary)
            return;

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

std::set<PurpleChat *> PurpleLine::blist_find_chats_by_type(ChatType type) {
    std::string type_string = chat_type_to_string[type];

    return blist_find<PurpleChat>([type_string](PurpleChat *chat) {
        GHashTable *components = purple_chat_get_components(chat);

        return type_string == (char *)g_hash_table_lookup(components, "type");
    });
}

// Maybe put into function below
PurpleChat *PurpleLine::blist_find_chat(std::string id, ChatType type) {
    std::string type_string = chat_type_to_string[type];

    std::set<PurpleChat *> chats = blist_find<PurpleChat>([type, type_string, id](PurpleChat *chat){
        GHashTable *components = purple_chat_get_components(chat);

        return
            (type == ChatType::ANY
                || type_string == (char *)g_hash_table_lookup(components, "type"))
            && id == (char *)g_hash_table_lookup(components, "id");
    });

    return chats.empty() ? nullptr : *chats.begin();
}

PurpleChat *PurpleLine::blist_ensure_chat(std::string id, ChatType type) {
    PurpleChat *chat = blist_find_chat(id, type);
    if (!chat) {
        GHashTable *components = g_hash_table_new_full(
            g_str_hash, g_str_equal, g_free, g_free);

        g_hash_table_insert(components, g_strdup("type"),
            g_strdup(chat_type_to_string[type].c_str()));
        g_hash_table_insert(components, g_strdup("id"), g_strdup(id.c_str()));

        chat = purple_chat_new(acct, id.c_str(), components);

        purple_blist_add_chat(chat, blist_ensure_group(LINE_GROUP), nullptr);
    }

    return chat;
}

// Fetch information for chat and call the other update_chat
void PurpleLine::blist_update_chat(std::string id, ChatType type) {
    // Put buddy on list already so it shows up as loading
    blist_ensure_chat(id.c_str(), type);

    if (type == ChatType::GROUP) {
        c_out->send_getGroups(std::vector<std::string> { id });
        c_out->send([this, type]{
            std::vector<line::Group> groups;
            c_out->recv_getGroups(groups);

            if (groups.size() == 1)
                blist_update_chat(groups[0]);
        });
    } else if (type == ChatType::ROOM) {
        purple_debug_warning("line", "ROOM: TODO\n");
    }
}

PurpleChat *PurpleLine::blist_update_chat(line::Group &group) {
    groups[group.id] = group;

    PurpleChat *chat = blist_ensure_chat(group.id, ChatType::GROUP);

    purple_blist_alias_chat(chat, group.name.c_str());

    // If a conversation is somehow already open, set its members

    PurpleConversation *conv = purple_find_conversation_with_account(
        PURPLE_CONV_TYPE_CHAT,
        group.id.c_str(),
        acct);

    if (conv)
        set_chat_participants(PURPLE_CONV_CHAT(conv), group);

    return chat;
}

PurpleChat *PurpleLine::blist_update_chat(line::Room &room) {
    rooms[room.mid] = room;

    PurpleChat *chat = blist_ensure_chat(room.mid, ChatType::ROOM);

    purple_blist_alias_chat(chat, get_room_display_name(room).c_str());

    // If a conversation is somehow already open, set its members

    PurpleConversation *conv = purple_find_conversation_with_account(
        PURPLE_CONV_TYPE_CHAT,
        room.mid.c_str(),
        acct);

    if (conv)
        set_chat_participants(PURPLE_CONV_CHAT(conv), room);

    return chat;
}

void PurpleLine::blist_remove_chat(std::string id, ChatType type) {
    PurpleChat *chat = blist_find_chat(id, type);

    if (chat)
        purple_blist_remove_chat(chat);
}
