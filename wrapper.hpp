#pragma once

#include <glib.h>
#include <account.h>
#include <connection.h>
#include <conversation.h>
#include <sslconn.h>

#include <cmds.h>

#include <boost/shared_ptr.hpp>

// Template magic to wrap instance methods as C function pointers

template<typename Sig, Sig F>
struct wrapper_;

template<typename Class, typename Ret, typename... Args, Ret(Class::*Func)(Args...)>
struct wrapper_<Ret(Class::*)(Args...), Func> {
    Ret static f(PurpleConnection *conn, Args... args)
    {
        return (((Class *)purple_connection_get_protocol_data(conn))->*Func)(args...);
    }

    Ret static f(PurpleAccount *acct, Args... args)
    {
        return f(purple_account_get_connection(acct), args...);
    }

    Ret static f(PurpleConversation *conv, const gchar *cmd,
        gchar **args, gchar **error, void *data)
    {
        return f(purple_conversation_get_account(conv), conv, cmd, args, error, data);
    }

    Ret static f(gpointer context, Args... args) {
        return (((Class *)context)->*Func)(args...);
    }

    Ret static f_end(Args... args, gpointer context) {
        return (((Class *)context)->*Func)(args...);
    }

    Ret static f_signal(Args... args, void *context) {
        return (((Class *)context)->*Func)(args...);
    }
};

#define WRAPPER(MEMBER) (wrapper_<decltype(&MEMBER), &MEMBER>::f)
#define WRAPPER_TYPE(MEMBER, TYPE) (wrapper_<decltype(&MEMBER), &MEMBER>::f_##TYPE)
