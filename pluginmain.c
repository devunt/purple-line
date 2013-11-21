#include <glib.h>

#include "config.h"
#include "debug.h"
#include "plugin.h"
#include "prpl.h"
#include "sslconn.h"
#include "version.h"

#include "wrappers.h"


static PurplePluginProtocolInfo prpl_info = {
    .options = OPT_PROTO_CHAT_TOPIC,
    .icon_spec = {
        .format = "png",
        .min_width = 0,
        .min_height = 0,
        .max_width = 128,
        .max_height = 128,
        .max_filesize = 10000,
        .scale_rules = PURPLE_ICON_SCALE_DISPLAY,
    },
    //.icon_spec = NO_BUDDY_ICONS,

    .list_icon = line_list_icon,
    .status_types = line_status_types,
    .chat_info = line_chat_info,
    .get_chat_name = line_get_chat_name,
    .login = line_login,
    .close = line_close,
    .send_im = line_send_im,

    .struct_size = sizeof(PurplePluginProtocolInfo),
};

static void lineprpl_destroy(PurplePlugin *plugin) {
    purple_debug_info("line", "shutting down\n");
}

static PurplePluginInfo info = {
    .magic = PURPLE_PLUGIN_MAGIC,
    .major_version = PURPLE_MAJOR_VERSION,
    .minor_version = PURPLE_MINOR_VERSION,
    .type = PURPLE_PLUGIN_PROTOCOL,
    .priority = PURPLE_PRIORITY_DEFAULT,

    .id = "prpl-mvirkkunen-line",
    .name = "LINE",
    .version = "0.1",
    .summary = "Plugin for Naver LINE",
    .description = "Plugin for Naver LINE",
    .author = "Matti Virkkunen <mvirkkunen@gmail.com>",
    .homepage = "https://github.com/mvirkkunen/purple-line",

    .destroy = lineprpl_destroy,
    .extra_info = (void *)&prpl_info,
};

static void line_plugin_init(PurplePlugin *plugin) {
    PurpleAccountOption *option = purple_account_option_string_new(
        "Example option", "example", "default");

    prpl_info.protocol_options = g_list_append(NULL, option);
}

PURPLE_INIT_PLUGIN(line, line_plugin_init, info);
