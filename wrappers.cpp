#include "wrappers.h"
#include "purpleline.hpp"

/*#define ACCOUNT_WRAPPER(RETURN, METHOD, ...) \
static void line_##method(PurpleAccount *acct, __VA_ARGS__) { \
    ((LinePlugin *)(cct->gc->prot_data)->method(acct, __VA_ARGS__); \
}

#define CONNECTION_WRAPPER(RETURN, METHOD, ...) \
static void line_##method(PurpleConnection *conn, __VA_ARGS__) { \
    ((LinePlugin *)conn->prot_data)->method(__VA_ARGS__); \
}*/

const char *line_list_icon(PurpleAccount *acct, PurpleBuddy *buddy) {
    return PurpleLine::list_icon(buddy);
}

GList *line_status_types(PurpleAccount *acct) {
    return PurpleLine::status_types();
}

void line_login(PurpleAccount *acct) {
    PurpleConnection *conn = purple_account_get_connection(acct);

    PurpleLine *plugin = new PurpleLine(conn, acct);
    conn->proto_data = (void *)plugin;

    plugin->login();
}

void line_close(PurpleConnection *conn) {
    PurpleLine *plugin = (PurpleLine *)conn->proto_data;

    plugin->close();

    delete plugin;
}
