Naver LINE protocol
===================

Matti Virkkunen <mvirkkunen@gmail.com>

Document is accurate as of 2013-11-21.

This unofficial document describes the LINE (by Naver / NHN Japan) instant messenger protocol.
Observations are based mostly on monitoring the desktop Windows client's traffic (under Wine), and
decompiled source code. Therefore, many things might not be accurate.

Also, it's not nearly finished yet.

Wire protocol
-------------

Attachment: line.thrift

Apache Thrift TCompactProtocol via HTTPS to gd2.line.naver.jp:443. The path is /S4 for most
messages, /P4 is used to get a long poll connection for fetchOperations.

Encryption is required.

If using a keep-alive connection (and you should be), headers from the first request can be
persisted. Headers in following requests temporarily override the persisted value. A mystery
header called X-LS apparently has something to do with this. If you want to persist headers, you
must remember and X-LS header value the server sends you and send it back in the next request. The
values seem to be integers. (Could LS be short for "Line Session"?)

Using persistent headers it's possible to send each request with just two headers - X-LS and
Content-Length.

The official protocol seems to be to first make one request to get an authentication key, and then
open a new connection so that the authentication key can be persisted along with the rest of the
headers.

Types and concepts
------------------

Internally any user is referred to as a Contact. Contacts are identified by a "mid" which is a hex
GUID prefixed with "u" (presumably for "user"). Groups are called just Groups and are identified by
an "id" which is a hex GUID prefixed with a "c" (presumably for "chat").

Timestamps are millisecond precision UNIX time represented as a 64-bit integer (TODO: check the
timezone just in case)

Message authentication
----------------------

The following HTTP headers are required for a successful request:

    X-Line-Application: DESKTOPWIN\t3.2.1.83\tWINDOWS\t5.1.2600-XP-x64
    X-Line-Access: authToken

The \t escape sequence represents a tab character. Other X-Line-Application names exist, but this is
one that works currently. An invalid application name results in an error.  The authToken is
obtained via the login procedure.

Login procedure
---------------

This Thrift method issues a new authToken for an e-mail address and password combination:

    loginWithIdentityCredentialForCertificate(
        "test@example.com", // identifier (e-mail address)
        "password", // password (in plain text)
        true, // keepLoggedIn
        "127.0.0.1", // accesslocation (presumably local IP?)
        "hostname", // systemName (will show up in "Devices")
        IdentityProvider.LINE, // identityProvider
        "") // certificate

For the login request, the X-Line-Access header must be specified but the value can be anything.

The official desktop client sends an encrypted e-mail/password involving RSA and no X-Line-Access
header, but it works just as fine in plain text.

TODO: Include description of RSA login procedure

Initial sync
------------

After logging in the client sends out a sequence of requests to synchronize with the server. It
seems some messages are not always sent - the client could be storing data locally somewhere and
comparing with the revision ID from getLastOpRevision()

(Not all methods are listed or implemented in Thrift yet)

getLastOpRevision()

Gets the revision ID to use for the long poll return channel later. It's fetched first to ensure
nothing is missed even if something happens during the sync procedure.

getProfile()

Gets the currently logged in user's profile, which includes their display name and status message
and so forth.

getBlockedContactIds()

List of blocked users IDs.

getRecommendationIds()

List of suggested friend IDs.

getBlockedRecommendationIds()

List of suggested friend IDs that have been dismissed (why can't the previous method just not
return these...?)

getAllContactIds()

Gets all contact IDs added as friends.

getContacts(list<string> contactIds) - with IDs from the previous methods

Gets details for the users.

getGroupIdsJoined()

Gets all groups current user is a member of.

getGroups(list<string> groupIds) - with IDs from the previous method

Gets details for the groups. This included member lists.

IM conversations
----------------

sendMessage(msg) with a contact ID in the msg.to field.

Group conversations
-------------------

sendMessage(msg) with a group ID in the msg.to field.

Message types
-------------

LINE supports various types of messages from simple text messages to pictures and video.

Return channel
--------------

fetchOperations(localRev, count)

For incoming messages, long polling with fetchOperations() to the /S4 path is used. A HTTP 410 Gone
response signals a timed out poll, in which case a new request should be issued.

When new data arrives, a list of Operation objects is returned. Each Operation comes with a version
number, and the next localRev should be the highest revision number received.
