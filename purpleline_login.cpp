#include "purpleline.hpp"
#include <openssl/rsa.h>
#include <boost/tokenizer.hpp>


void PurpleLine::login_start() {
    purple_connection_set_state(conn, PURPLE_CONNECTING);
    purple_connection_update_progress(conn, "Logging in", 0, 3);

    std::string certificate(purple_account_get_string(acct, LINE_ACCOUNT_CERTIFICATE, ""));
    if (certificate != "") {
        got_auth_token(certificate);
        return;
    }

    http.request(LINE_SESSION_URL,
        [this](int status, const guchar *data, gsize len)
    {
        if (status != 200 || !data) {
            std::string msg = "Could not log in. Failed to get session key.";

            purple_connection_error(
                conn,
                msg.c_str());

            return;
        }

        std::string session_key, rsa_key;
        std::string json((const char *)data, len);
        std::string rsa_encrypted;
        char rsa_encrypted_arr[512];

        std::size_t pos = json.find("{\"session_key\":\"") + 16;
        std::size_t pos2 = json.find("\",\"rsa_key\":\"");
        session_key = json.substr(pos, pos2 - pos);
        rsa_key = json.substr(pos2 + 13, json.find("\"}") - pos2 - 13);

        std::string username = purple_account_get_username(acct);
        std::string password = purple_account_get_password(acct);

        std::ostringstream ss;
        ss
            << (char)session_key.length() << session_key
            << (char)username.length() << username
            << (char)password.length() << password;
        std::string str = ss.str();

        int i = 0;
        std::string rsa[3];
        boost::char_separator<char> sep(",");
        boost::tokenizer<boost::char_separator<char>> tokens(rsa_key, sep);
        for (const auto& t : tokens) {
            rsa[i++] = t;
        }

        RSA* pubkey = RSA_new();

        BN_hex2bn(&pubkey->n, rsa[1].c_str());
        BN_hex2bn(&pubkey->e, rsa[2].c_str());

        int rsalen = RSA_public_encrypt(str.length(), (const unsigned char*) str.c_str(), (unsigned char*) rsa_encrypted_arr, pubkey, RSA_PKCS1_PADDING);
        if (rsalen <= 0) {
            std::string msg = "Could not log in. Fail to encrypt string.";

            purple_connection_error(
                conn,
                msg.c_str());

            return;
        }

        const char hex[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
        for (int i = 0; i < rsalen; ++i) {
            const char ch = rsa_encrypted_arr[i];
            rsa_encrypted.append(&hex[(ch & 0xF0) >> 4], 1);
            rsa_encrypted.append(&hex[ch & 0x0F], 1);
        }

        c_out->send_loginWithIdentityCredentialForCertificate(
            line::IdentityProvider::NAVER_KR,
            username,
            password,
            rsa[0],
            rsa_encrypted,
            true,
            "127.0.0.1",
            "purple-line",
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
                        got_auth_token(certificate);
                    }
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
                http.request(LINE_OS_URL + pic_path, HTTPFlag::auth,
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
            login_done();
            return;
        }

        c_out->send_getGroups(gids);
        c_out->send([this]() {
            std::vector<line::Group> groups;
            c_out->recv_getGroups(groups);

            for (line::Group &g: groups)
                handle_group_invite(g, profile_contact, no_contact);

            login_done();
        });
    });
}

void PurpleLine::login_done() {
    poller.start();

    purple_connection_update_progress(conn, "Connected", 2, 3);
}
