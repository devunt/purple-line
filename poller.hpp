#pragma once

#include <string>
#include <deque>

#include <debug.h>
#include <plugin.h>
#include <prpl.h>

#include "constants.hpp"
#include "thriftclient.hpp"

class PurpleLine;

class Poller {

    PurpleLine &parent;

    boost::shared_ptr<ThriftClient> client;
    int64_t local_rev;

public:

    Poller(PurpleLine &parent);
    ~Poller();

    void start();
    void set_auth_token(std::string token) { client->set_auth_token(token); }
    void set_local_rev(int64_t local_rev) { this->local_rev = local_rev; }

private:

    // Long poll return channel
    void fetch_operations();

    void op_notified_kickout_from_group(line::Operation &op);
    void op_notified_invite_into_group(line::Operation &op);

};
