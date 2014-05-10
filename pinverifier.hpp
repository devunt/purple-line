#pragma once

#include <string>
#include <deque>

#include <debug.h>
#include <plugin.h>
#include <prpl.h>

#include "thriftclient.hpp"
#include "poller.hpp"

#define LINEPRPL_ID "prpl-mvirkkunen-line"

#define LINE_ACCOUNT_CERTIFICATE "line-certificate"

class PurpleLine;

class PINVerifier {

    PurpleLine &parent;

    boost::shared_ptr<LineHttpTransport> http;

    void *notification;
    guint timeout;

public:

    PINVerifier(PurpleLine &parent);
    ~PINVerifier();

    void verify(
        line::LoginResult loginResult,
        std::function<void(std::string, std::string)> success);

private:

    int timeout_cb();
    void cancel_cb(int);
    void end();
    void error(std::string error);
};
