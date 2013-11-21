#include <glib.h>
#include "account.h"
#include "accountopt.h"
#include "debug.h"
#include "plugin.h"
#include "prpl.h"
#include "version.h"

#ifdef __cplusplus
extern "C" {
#endif

const char *line_list_icon(PurpleAccount *acct, PurpleBuddy *buddy);
GList *line_status_types(PurpleAccount *acct);
GList *line_chat_info(PurpleConnection *conn);
char *line_get_chat_name(GHashTable *components);

void line_login(PurpleAccount *acct);
void line_close(PurpleConnection *conn);

int line_send_im(PurpleConnection *conn, const char *who, const char *message, PurpleMessageFlags flags);


#ifdef __cplusplus
}
#endif
