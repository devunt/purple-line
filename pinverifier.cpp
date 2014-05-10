#include <iostream>

#include <conversation.h>
#include <request.h>
#include <notify.h>
#include <util.h>
#include <eventloop.h>

#include "purpleline.hpp"
#include "wrapper.hpp"

PINVerifier::PINVerifier(PurpleLine &parent)
    : parent(parent)
{
}

PINVerifier::~PINVerifier() {
    http.reset();
}

void PINVerifier::verify(
    line::LoginResult loginResult,
    std::function<void(std::string, std::string)> success)
{
    const int minutes = 3;

    const std::string verifier = loginResult.verifier;

    std::stringstream ss;
    ss
        << loginResult.pinCode
        << "\n\nThe number has to be entered into the LINE mobile application within "
        << minutes
        << " minutes. If the time runs out, reconnect to try again."
        << "\n\nYou will only have to verify your account once per computer.";
    std::string pin_msg = ss.str();

    notification = purple_request_action(
        (void *)parent.conn,
        "LINE account verification",
        "Enter this number on your mobile device",
        pin_msg.c_str(),
        0,
        parent.acct,
        nullptr,
        nullptr,
        (gpointer)this,
        1,
        "Cancel",
        (PurpleRequestActionCb)WRAPPER(PINVerifier::cancel_cb));

    timeout = purple_timeout_add_seconds(
        minutes * 60,
        WRAPPER(PINVerifier::timeout_cb),
        (gpointer)this);

    http = boost::make_shared<LineHttpTransport>(
        parent.acct, parent.conn,
        "gd2.line.naver.jp", 443,
        false);
    http->set_auth_token(verifier);

    http->request("GET", "/Q", [this, verifier, success]() {
        if (http->status_code() != 200) {
            std::stringstream ss;
            ss << "Account verification failed: invalid status code: " << http->status_code();

            error(ss.str());
            return;
        }

        std::string json((size_t)http->content_length(), '\0');
        http->read((uint8_t *)&json[0], json.size());

        // Don't feel like invoking an entire JSON parser for this, this should be good enough.
        if (json.find("\"QRCODE_VERIFIED\"") == std::string::npos) {
            error("Account verification failed: server would not verify.");
            return;
        }

        // TODO: ick, maybe create a new client instead of borrowing one from parent
        parent.c_out->send_loginWithVerifierForCertificate(verifier);
        parent.c_out->send([this, verifier, success]() {
            line::LoginResult result;

            try {
                parent.c_out->recv_loginWithVerifierForCertificate(result);
            } catch (line::TalkException &err) {
                error("Account verification failed: Exception: " + err.reason);
                return;
            }

            if (result.authToken == "") {
                error("Account verification failed: no auth token.");
                return;
            }

            end();

            success(result.authToken, result.certificate);
        });
    });
}

int PINVerifier::timeout_cb() {
    error("Account verification timed out.");

    return FALSE;
}

void PINVerifier::cancel_cb(int) {
    error("Account verification cancelled.");
}

void PINVerifier::end() {
    http.reset();

    if (timeout) {
        purple_timeout_remove(timeout);
        timeout = 0;
    }

    if (notification) {
        purple_request_close(PURPLE_REQUEST_ACTION, notification);
        notification = nullptr;
    }
}

void PINVerifier::error(std::string error) {
    end();
    purple_connection_error(parent.conn, error.c_str());
}
