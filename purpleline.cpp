#include "purpleline.hpp"

#include <iostream>

#include <protocol/TCompactProtocol.h>

#include <debug.h>
#include <sslconn.h>

#include "purplehttpclient.hpp"

static const char *LINE_HOST = "gd2.line.naver.jp";
static const char *LINE_PATH = "/S4";
static const int LINE_PORT = 443;

PurpleLine::PurpleLine(PurpleConnection *conn, PurpleAccount *acct)
    : conn(conn), acct(acct)
{

}

const char *PurpleLine::list_icon(PurpleBuddy *buddy)
{
    return "line";
}

GList *PurpleLine::status_types() {
    GList *types = NULL;
    PurpleStatusType *t;

    t = purple_status_type_new_full(PURPLE_STATUS_AVAILABLE, NULL, NULL, TRUE, TRUE, FALSE);
    types = g_list_append(types, t);

    t = purple_status_type_new_full(PURPLE_STATUS_OFFLINE, NULL, NULL, TRUE, TRUE, FALSE);
    types = g_list_append(types, t);

    return types;
}

void PurpleLine::login() {
    // Create http clients

    http_out = boost::make_shared<PurpleHttpClient>(acct, LINE_HOST, LINE_PORT, LINE_PATH);

    client_out = boost::make_shared<line::LineClient>(
        boost::make_shared<apache::thrift::protocol::TCompactProtocol>(http_out));

    http_in = boost::make_shared<PurpleHttpClient>(acct, LINE_HOST, LINE_PORT, LINE_PATH);

    client_in = boost::make_shared<line::LineClient>(
        boost::make_shared<apache::thrift::protocol::TCompactProtocol>(http_in));

    // Log in

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

        http_out->set_auth_token(result.authToken);
        http_in->set_auth_token(result.authToken);

        get_profile();
    });
}

void PurpleLine::get_profile() {
    client_out->send_getProfile();
    http_out->send([this](int status) {
        line::Profile profile;
        client_out->recv_getProfile(profile);

        std::cout << "Your profile: " << std::endl
            << "  ID: " << profile.mid << std::endl
            << "  Display name: " << profile.displayName << std::endl
            << "  Phonetic name: " << profile.phoneticName << std::endl
            << "  Picture status name: " << profile.pictureStatus << std::endl
            << "  Thumbnail URL: " << profile.thumbnailUrl << std::endl
            << "  Status message: " << profile.statusMessage << std::endl;

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

            PurpleGroup *group = purple_find_group("LINE");

            for (line::Contact &c: contacts) {

                std::cout << "Contact " << c.mid << std::endl
                    //<< "  Type: " << line::_ContactType_VALUES_TO_NAMES.at(c.type) << std::endl
                    << "  Status: " << line::_ContactStatus_VALUES_TO_NAMES.at(c.status) << std::endl
                    //<< "  Relation: " << line::_ContactRelation_VALUES_TO_NAMES.at(c.relation) << std::endl
                    << "  Display name: " << c.displayName << std::endl
                    << "  Phonetic name: " << c.phoneticName << std::endl
                    << "  Picture status name: " << c.pictureStatus << std::endl
                    << "  Thumbnail URL: " << c.thumbnailUrl << std::endl
                    << "  Status message: " << c.statusMessage << std::endl;

                if (c.status == line::ContactStatus::FRIEND) {
                    if (!group)
                        group = purple_group_new("LINE");

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

                    //purple_blist_alias_contact(PURPLE_CONTACT(buddy), c.displayName.c_str());

                    std::cout << " added " << std::endl;
                }
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

            PurpleGroup *group = purple_find_group("LINE");

            for (line::Group &g: groups) {
                if (!group)
                    group = purple_group_new("LINE");

                std::cout << "Group " << g.id << std::endl
                    << "  Name: " << g.name << std::endl
                    << "  Creator: " << g.creator.displayName << std::endl
                    << "  Member count: " << g.members.size() << std::endl;
            }
        });
    });
}

void PurpleLine::close() {
    purple_debug_info("lineprpl", "close");

    http_out->close();
    http_in->close();
}
