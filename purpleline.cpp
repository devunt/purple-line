#include "purpleline.hpp"

static const char *LINE_SERVER = "ga2.line.naver.jp";
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
    apache::thrift::transport::TSSLSocketFactory factory;

    thrift_socket = factory.createSocket(LINE_SERVER, LINE_PORT);

    thrift_transport = boost::make_shared<apache::thrift::transport::THttpClient>(
        thrift_socket, LINE_SERVER, LINE_PATH);
    thrift_protocol = boost::make_shared<apache::thrift::protocol::TCompactProtocol>(
        thrift_transport);
}

void PurpleLine::close() {
    purple_debug_info("lineprpl", "close");
}
