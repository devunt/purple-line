#pragma once

#include <glib.h>
#include <account.h>
#include <connection.h>
#include <sslconn.h>

// Template magic to wrap instance methods as C function pointers

template<typename Sig, Sig F>
struct wrapper_;

template<typename Class, typename Ret, typename... Args, Ret(Class::*Func)(Args...)>
struct wrapper_<Ret(Class::*)(Args...), Func> {
    Ret static f(PurpleConnection *conn, Args... args)
    {
        return (((Class *)conn->proto_data)->*Func)(args...);
    }

    Ret static f(PurpleAccount *acct, Args... args)
    {
        return (((Class *)purple_account_get_connection(acct)->proto_data)->*Func)(args...);
    }

    Ret static f(gpointer context, Args... args) {
        return (((Class *)context)->*Func)(args...);
    }

    Ret static f_end(Args... args, gpointer context) {
        return (((Class *)context)->*Func)(args...);
    }
};

#define WRAPPER(MEMBER) (wrapper_<decltype(&MEMBER), &MEMBER>::f)
#define WRAPPER_AT(MEMBER, WHERE) (wrapper_<decltype(&MEMBER), &MEMBER>::f_##WHERE)
