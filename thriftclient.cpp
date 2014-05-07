#include <account.h>
#include <connection.h>

#include "thriftclient.hpp"

ThriftProtocol::ThriftProtocol(boost::shared_ptr<LineHttpTransport> trans)
    : apache::thrift::protocol::TCompactProtocolT<LineHttpTransport>(trans)
{
}

LineHttpTransport *ThriftProtocol::getTransport() {
    return trans_;
}

ThriftClient::ThriftClient(PurpleAccount *acct, PurpleConnection *conn, std::string path)
    : line::TalkServiceClientT<ThriftProtocol>(
        boost::make_shared<ThriftProtocol>(
            boost::make_shared<LineHttpTransport>(acct, conn, "gd2.line.naver.jp", 443, true))),
    path(path)
{
    http = piprot_->getTransport();
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
