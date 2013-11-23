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

Friends, chats and groups are identified by 32-digit hex GUIDs prefixed with one characte for the
type.

Internally any user is referred to as a Contact. Contacts are identified by a "mid" and the prefix
is "u" (presumably for "user")

Chats are called Rooms and are identified by a "mid" which is prefixed by "r" (for "room"). Groups
are called Groups internally as well and are identified by an "id" which is prefixed with a "c"
(presumably for "chat").

Any message is represented by a Message object. Message IDs are numeric but they are stored as
strings.

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

Object storage server
---------------------

Message attachment files are stored on a separate server at https://os.line.naver.jp/ which is
internally referred to as the "object storage server". It serves files via HTTPS with the same
authentication protocol as above.

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

(Not all methods are listed or implemented in Thrift yet - also list is incomplete)

    getLastOpRevision()

Gets the revision ID to use for the long poll return channel later. It's fetched first to ensure
nothing is missed even if something happens during the sync procedure.

    getProfile()

Gets the currently logged in user's profile, which includes their display name and status message
and so forth.

    getBlockedContactIds()

List of blocked user IDs.

    getRecommendationIds()

List of suggested friend IDs.

    getBlockedRecommendationIds()

List of suggested friend IDs that have been dismissed (why can't the previous method just not
return these...?)

    getAllContactIds()

Gets all contact IDs added as friends.

    getContacts(contactIds) - with IDs from the previous methods

Gets details for the users.

    getGroupIdsJoined()

Gets all groups current user is a member of.

    getGroups(groupIds) - with IDs from the previous method

Gets details for the groups. This included member lists.

IM conversations
----------------

sendMessage(msg) with a contact ID in the msg.to field.

Group conversations
-------------------

sendMessage(msg) with a group ID in the msg.to field.

Message types
-------------

LINE supports various types of messages from simple text messages to pictures and video. Each
message has a contentType field that specifies the type of content, and some messages include
attached files from various locations.

Messages can contain extra attributes in the contentMetadata map. One globally used attribute is
"BOT_CHECK" which is included with a value of "1" for automatic messages I've received from
"official accounts" - this could be an auto-reply indicator.

### NONE (0) (actually text)

The first contentType is called NONE internally but is actually text. It's the simplest content
type. The text field contains the message content encoded in UTF-8.

The only thing to watch out for is emoji which are sent as Unicode private use area codepoints.

TODO: make a list of emoji

### IMAGE (1)

Image message content can be delivered in one of two ways.

For normal image messages, a preview image is included as a plain JPEG in the contentPreview field.
However, for some reason the official desktop client seems to ignore id, and rather downloads
everything from the object storage server.

The preview image URLs are http://os.line.naver.jp/os/m/MSGID/preview and the full-size image URL
are http://os.line.naver.jp/os/m/MSGID where MSGID is the message's id field.

"Official accounts" broadcast messages to many clients at once, so their image message data is
stored on publicly accessible servers (currently seems to be Akamai CDN). For those messages no
embedded previes is included and the image URLs are stored in the contentMetadata map with the
following keys:

* PREVIEW_URL = absolute URL for preview image
* DOWNLOAD_URL = absolute URL for full-size image
* PUBLIC = TRUE (haven't seen other values)

As an example of a publicly available image message, have a Pikachu:

http://dl-obs.official.line.naver.jp/r/talk/o/u3ae3691f73c7a396fb6e5243a8718915-1379585871

Return channel
--------------

    fetchOperations(localRev, count)

For incoming messages, long polling with fetchOperations() to the /S4 path is used. A HTTP 410 Gone
response signals a timed out poll, in which case a new request should be issued.

When new data arrives, a list of Operation objects is returned. Each Operation (except the end
marker) comes with a version number, and the next localRev should be the highest revision number
received.

The official client uses a count parameter of 50.

Operation data is contained either as a Message object in the message field, or in the string fields
param1-param3.

The following is a list of operation types.

### END_OF_OPERATION (0)

Signifies the end of the list. This presumably means all operations were returned and none were left
out due to the count param. This message contains no data, not even a revision number.

### UPDATE_PROFILE (1)

Informs that the client should refresh its profile using getProfile()

* param1 = (mystery - seen "32")

### ADD_CONTACT (4)

Informs that the current user has added a contact as a friend.

* param1 = ID of the user that was added
* param2 = (mystery - seen "0")

### BLOCK_CONTACT (6)

Informs that the current user has blocked a contact (removed from friends).

* param1 = ID of the user that was removed
* param2 = (mystery, seen "NORMAL")

### SEND_MESSAGE (25)

Informs about a message that the current user sent. This is returned to all connected devices,
including the one that sent the message.

* message = sent message

### RECEIVE_MESSAGE (26)

Informs about a received message that another user sent either to the current user or to a chat. The
message field contains the message.

* message = received message

### RECEIVE_MESSAGE_RECEIPT (28)

Informs that another user has read (seen) messages sent by the current user.

* param1 = ID of the user that read the message
* param2 = IDs of the messages, multiple IDs are separated by U+001E INFORMATION SEPARATOR TWO

### UPDATE_SETTINGS (36)

Informs that the client should refresh its "settings" using getSettingsAttributes()

* param1 = (mystery - seen "1024", "8192")

### SEND_CHAT_CHECKED (40)

### CONTACT_CONTACT_SETTINGS (name guessed) (49)

Seems to be sent when the "hidden" setting of the contact changes. This causes the official client to
re-request data about the contact with getContact() (singular!).

* param1 = ID of the contact that was changed
* param2 = (mystery - seen "4")
