#include <glib.h>

#include <account.h>
#include <config.h>
#include <debug.h>
#include <prpl.h>
#include <version.h>

#include "purpleline.hpp"
#include "wrapper.hpp"

static void line_plugin_destroy(PurplePlugin *plugin);
static void line_plugin_init(PurplePlugin *plugin);

static PurplePluginProtocolInfo prpl_info;

static PurplePluginInfo info;

static void init_icon_spec(PurpleBuddyIconSpec &s) {
    s.format = (char *)"jpeg";
    s.min_width = 0;
    s.min_height = 0;
    s.max_width = 200;
    s.max_height = 200;
    s.max_filesize = 100 * 1024; // random number
    s.scale_rules = PURPLE_ICON_SCALE_DISPLAY;
}

static void init_info(PurplePluginInfo &i) {
    i.magic = PURPLE_PLUGIN_MAGIC;
    i.major_version = PURPLE_MAJOR_VERSION;
    i.minor_version = PURPLE_MINOR_VERSION;
    i.type = PURPLE_PLUGIN_PROTOCOL;
    i.priority = PURPLE_PRIORITY_DEFAULT;

    i.id = (char *)"prpl-mvirkkunen-line";
    i.name = (char *)"LINE";
    i.version = (char *)"0.1";
    i.summary = (char *)"Plugin for Naver LINE";
    i.description = (char *)"Plugin for Naver LINE";
    i.author = (char *)"Matti Virkkunen <mvirkkunen@gmail.com>";
    i.homepage = (char *)"https://github.com/mvirkkunen/purple-line";

    i.destroy = line_plugin_destroy;
    i.extra_info = (void *)&prpl_info;
}

static void init_prpl_info(PurplePluginProtocolInfo &i) {
    i.options = OPT_PROTO_CHAT_TOPIC,
    init_icon_spec(i.icon_spec);

    i.list_icon = &PurpleLine::list_icon;
    i.status_types = &PurpleLine::status_types;
    //i.get_chat_name = &PurpleLine::get_chat_name;
    i.status_text = &PurpleLine::status_text;
    i.tooltip_text = &PurpleLine::tooltip_text;
    i.login = &PurpleLine::login;
    i.close = WRAPPER(PurpleLine::close);
    i.chat_info = WRAPPER(PurpleLine::chat_info);
    i.send_im = WRAPPER(PurpleLine::send_im);
    i.join_chat = WRAPPER(PurpleLine::join_chat);
    i.chat_leave = WRAPPER(PurpleLine::chat_leave);
    i.chat_send = WRAPPER(PurpleLine::chat_send);

    i.struct_size = sizeof(PurplePluginProtocolInfo);
}

static void line_plugin_destroy(PurplePlugin *plugin) {
    purple_debug_info("line", "shutting down\n");
}

static void line_plugin_init(PurplePlugin *plugin) {
    init_info(info);
    init_prpl_info(prpl_info);
}

extern "C" {
    PURPLE_INIT_PLUGIN(line, line_plugin_init, info);
}
