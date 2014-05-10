#include <iostream>

#include <time.h>

#include <debug.h>

#include "poller.hpp"
#include "purpleline.hpp"

Poller::Poller(PurpleLine &parent)
    : parent(parent)
{
    client = boost::make_shared<ThriftClient>(parent.acct, parent.conn, "/P4");
}

Poller::~Poller() {
    client.reset();
}

void Poller::start() {
    fetch_operations();
}

void Poller::fetch_operations() {
    client->send_fetchOperations(local_rev, 50);
    client->send([this]() {
        int status = client->status_code();

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
        client->recv_fetchOperations(operations);

        for (line::Operation &op: operations) {
            switch (op.type) {
                case line::OpType::END_OF_OPERATION: // 0
                    break;

                case line::OpType::ADD_CONTACT: // 4
                    parent.blist_update_buddy(op.param1);
                    break;

                case line::OpType::BLOCK_CONTACT: // 6
                    parent.blist_remove_buddy(op.param1);
                    break;

                case line::OpType::UNBLOCK_CONTACT: // 7
                    parent.blist_update_buddy(op.param1);
                    break;

                case line::OpType::CREATE_GROUP: // 9
                case line::OpType::UPDATE_GROUP: // 10
                case line::OpType::NOTIFIED_UPDATE_GROUP: // 11
                case line::OpType::INVITE_INTO_GROUP: // 12
                    parent.blist_update_chat(op.param1, ChatType::GROUP);
                    break;

                // case line::OpType::NOTIFIED_INVITE_INTO_GROUP: // 13
                    // TODO

                case line::OpType::LEAVE_GROUP: // 14
                    parent.blist_remove_chat(op.param1, ChatType::GROUP);
                    break;

                case line::OpType::NOTIFIED_LEAVE_GROUP: // 15
                    parent.blist_update_chat(op.param1, ChatType::GROUP);
                    break;

                case line::OpType::ACCEPT_GROUP_INVITATION: // 16
                    // TODO: When NOTIFIED_INIVITE_INTO_GROUP is implemented, hide group invitation
                    parent.blist_update_chat(op.param1, ChatType::GROUP);
                    break;

                case line::OpType::NOTIFIED_ACCEPT_GROUP_INVITATION: // 17
                case line::OpType::KICKOUT_FROM_GROUP: // 18
                    parent.blist_update_chat(op.param1, ChatType::GROUP);
                    break;

                case line::OpType::NOTIFIED_KICKOUT_FROM_GROUP: // 19
                    op_notified_kickout_from_group(op);
                    break;

                case line::OpType::CREATE_ROOM: // 20
                case line::OpType::INVITE_INTO_ROOM: // 21
                    parent.blist_update_chat(op.param1, ChatType::ROOM);
                    break;

                case line::OpType::NOTIFIED_INVITE_INTO_ROOM: // 22
                    // TODO: Perhaps show who invited the user (param2)
                    parent.blist_update_chat(op.param1, ChatType::ROOM);
                    break;

                case line::OpType::LEAVE_ROOM: // 23
                    parent.blist_remove_chat(op.param1, ChatType::ROOM);
                    break;

                case line::OpType::NOTIFIED_LEAVE_ROOM: // 24
                    parent.blist_update_chat(op.param1, ChatType::ROOM);

                case line::OpType::SEND_MESSAGE: // 25
                    parent.handle_message(op.message, true, false);
                    break;

                case line::OpType::RECEIVE_MESSAGE: // 26
                    parent.handle_message(op.message, false, false);
                    break;

                case line::OpType::DUMMY: // 48;
                    break;

                case line::OpType::UPDATE_CONTACT: // 49
                    parent.blist_update_buddy(op.param1);
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

void Poller::op_notified_kickout_from_group(line::Operation &op) {
    std::string msg;

    if (op.param3 == parent.profile.mid) {
        msg = "You were removed from the group by ";
        parent.blist_remove_chat(op.param1, ChatType::GROUP);
    } else {
        msg = "Removed from the group by ";
        parent.blist_update_chat(op.param1, ChatType::GROUP);
    }

    if (parent.contacts.count(op.param2) == 1)
        msg += parent.contacts[op.param2].displayName;
    else
        msg += "(unknown contact)";

    PurpleConversation *conv = purple_find_conversation_with_account(
        PURPLE_CONV_TYPE_CHAT,
        op.param1.c_str(),
        parent.acct);

    if (conv) {
        purple_conversation_write(
            conv,
            op.param3.c_str(),
            msg.c_str(),
            (PurpleMessageFlags)PURPLE_MESSAGE_SYSTEM,
            time(NULL));
    }
}
