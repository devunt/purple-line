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
    purple_debug_info("lineprpl", "constructed");
}

const char *PurpleLine::list_icon(PurpleBuddy *buddy)
{
    return "line";
}

GList *PurpleLine::status_types() {
    GList *types = NULL;
    PurpleStatusType *t;

    purple_debug_info("lineprpl", "status_types");

    t = purple_status_type_new(
        PURPLE_STATUS_AVAILABLE,
        "online",
        "Online",
        TRUE);
    types = g_list_append(types, t);

    t = purple_status_type_new(
        PURPLE_STATUS_OFFLINE,
        "offline",
        "Offline",
        TRUE);
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

    http_out->send(std::bind(&PurpleLine::login_complete, this, std::placeholders::_1));
}

void PurpleLine::login_complete(int status) {
    if (status != 200) {
        close();
        return;
    }

    line::LoginResult result;
    client_out->recv_loginWithIdentityCredentialForCertificate(result);

    http_out->set_auth_token(result.authToken);
    http_in->set_auth_token(result.authToken);

    client_out->send_getProfile();

    http_out->send(std::bind(&PurpleLine::get_profile_complete, this, std::placeholders::_1));
}

void PurpleLine::get_profile_complete(int status) {
    line::Profile profile;
    client_out->recv_getProfile(profile);

    std::cout << "mid: " << profile.mid << " email: " << profile.email << " userid: " << profile.userid << std::endl;
}

void PurpleLine::close() {
    purple_debug_info("lineprpl", "close");

    http_out->close();
    http_in->close();
}
