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
void line_login(PurpleAccount *acct);
void line_close(PurpleConnection *conn);

#ifdef __cplusplus
}
#endif
