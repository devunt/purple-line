#include <account.h>
#include <connection.h>

#include <thrift/protocol/TCompactProtocol.h>

#include "constants.hpp"
#include "thriftclient.hpp"

ThriftClient::ThriftClient(PurpleAccount *acct, PurpleConnection *conn, std::string path)
    : line::TalkServiceClient(
        boost::make_shared<apache::thrift::protocol::TCompactProtocol>(
            boost::make_shared<LineHttpTransport>(acct, conn, LINE_THRIFT_SERVER, 443, true))),
    path(path)
{
    http = boost::static_pointer_cast<LineHttpTransport>(getInputProtocol()->getTransport());
}

void ThriftClient::set_path(std::string path) {
    this->path = path;
}

void ThriftClient::set_auth_token(std::string token) {
    http->set_auth_token(token);
}

void ThriftClient::send(std::function<void()> callback) {
    http->request("POST", path, callback);
}

int ThriftClient::status_code() {
    return http->status_code();
}

void ThriftClient::close() {
    http->close();
}

// Required for the single set<Contact> in the interface

bool line::Contact::operator<(const Contact &other) const {
    return mid < other.mid;
}
