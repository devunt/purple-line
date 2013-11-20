Naver LINE protocol
===================

This unofficial document describes the LINE (by Naver / NHN Japan) instant messenger protocol.

It's not finished yet.

Document is accurate as of 2013-11-20.

Wire protocol
-------------

Attachment: line.thrift

Apache Thrift TCompactProtocol via HTTPS to https://gd2.line.naver.jp/S4

Encryption is required.

Message authentication
----------------------

The following HTTP headers are required for a successful request:

    X-Line-Application: DESKTOPWIN\t3.2.1.83\tWINDOWS\t5.1.2600-XP-x64
    X-Line-Access: authToken

The \t escape sequence represents a tab character. The AUTHKEY is obtained via the login procedure.

Login procedure
---------------

Issues a new authToken for an e-mail address and password combination.

    loginWithIdentityCredentialForCertificate(
        "test@example.com", // identifier
        "plaintextpassword", // password
        true, // keepLoggedIn
        "127.0.0.1", // accesslocation
        "hostname", // systemName
        IdentityProvider.LINE, // identityProvider
        "") // certificate

For the login request, the X-Line-Access header must be specified but the value can be anything.

The official desktop client sends an encrypted e-mail/password involving RSA and no X-Line-Access
header, but it works just as fine in plain text.

Message types
-------------

* getProfile
* sendMessage

Return channel
--------------

Long polling with fetchOperations(). HTTP 410 Gone signals a timed out poll, in which case a new
request should be issued.
