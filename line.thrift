namespace cpp line

// :%s/.*"\([^"]*\)", \d\+, \(\d\+\).*/\1 = \2;/

// er.class
enum ErrorCode {
    ILLEGAL_ARGUMENT = 0;
    AUTHENTICATION_FAILED = 1;
    DB_FAILED = 2;
    INVALID_STATE = 3;
    EXCESSIVE_ACCESS = 4;
    NOT_FOUND = 5;
    INVALID_MID = 9;
    NOT_A_MEMBER = 10;
    INVALID_LENGTH = 6;
    NOT_AVAILABLE_USER = 7;
    NOT_AUTHORIZED_DEVICE = 8;
    NOT_AUTHORIZED_SESSION = 14;
    INCOMPATIBLE_APP_VERSION = 11;
    NOT_READY = 12;
    NOT_AVAILABLE_SESSION = 13;
    SYSTEM_ERROR = 15;
    NO_AVAILABLE_VERIFICATION_METHOD = 16;
    NOT_AUTHENTICATED = 17;
    INVALID_IDENTITY_CREDENTIAL = 18;
    NOT_AVAILABLE_IDENTITY_IDENTIFIER = 19;
    INTERNAL_ERROR = 20;
    NO_SUCH_IDENTITY_IDENFIER = 21;
    DEACTIVATED_ACCOUNT_BOUND_TO_THIS_IDENTITY = 22;
    ILLEGAL_IDENTITY_CREDENTIAL = 23;
    UNKNOWN_CHANNEL = 24;
    NO_SUCH_MESSAGE_BOX = 25;
    NOT_AVAILABLE_MESSAGE_BOX = 26;
    CHANNEL_DOES_NOT_MATCH = 27;
    NOT_YOUR_MESSAGE = 28;
    MESSAGE_DEFINED_ERROR = 29;
    USER_CANNOT_ACCEPT_PRESENTS = 30;
    USER_NOT_STICKER_OWNER = 32;
    MAINTENANCE_ERROR = 33;
}

// jq.class
exception Error {
    1: ErrorCode code;
    2: string reason;
    3: map<string, string> parameterMap;
}

// ex.class
struct Location {
    1: string title;
    2: string address;
    3: double latitude;
    4: double longitude
    5: string phone;
}

// fb.class
enum MessageToType {
    USER = 0;
    ROOM = 1;
    GROUP = 2;
}

// eo.class
enum ContentType {
    NONE = 0;
    IMAGE = 1;
    VIDEO = 2;
    AUDIO = 3;
    HTML = 4;
    PDF = 5;
    CALL = 6;
    STICKER = 7;
    PRESENCE = 8;
    GIFT = 9;
    GROUBOARD = 10;
    APPLINK = 11;
}

// fc.class
struct Message {
    1: string from;
    2: string to;
    3: MessageToType toType;
    4: string id;
    5: i64 createdTime;
    6: i64 deliveredTime;
    10: string text;
    11: optional Location location;
    14: bool hasContent;
    15: ContentType contentType;
    17: string contentPreview;
    18: optional map<string, string> contentMetadata;
}

// fn.class
enum OperationType {
    END_OF_OPERATION = 0;
    UPDATE_PROFILE = 1;
    UPDATE_SETTINGS = 36;
    NOTIFIED_UPDATE_PROFILE = 2;
    REGISTER_USERID = 3;
    ADD_CONTACT = 4;
    NOTIFIED_ADD_CONTACT = 5;
    BLOCK_CONTACT = 6;
    UNBLOCK_CONTACT = 7;
    NOTIFIED_RECOMMEND_CONTACT = 8;
    CREATE_GROUP = 9;
    UPDATE_GROUP = 10;
    NOTIFIED_UPDATE_GROUP = 11;
    INVITE_INTO_GROUP = 12;
    NOTIFIED_INVITE_INTO_GROUP = 13;
    CANCEL_INVITATION_GROUP = 31;
    NOTIFIED_CANCEL_INVITATION_GROUP = 32;
    LEAVE_GROUP = 14;
    NOTIFIED_LEAVE_GROUP = 15;
    ACCEPT_GROUP_INVITATION = 16;
    NOTIFIED_ACCEPT_GROUP_INVITATION = 17;
    REJECT_GROUP_INVITATION = 34;
    NOTIFIED_REJECT_GROUP_INVITATION = 35;
    KICKOUT_FROM_GROUP = 18;
    NOTIFIED_KICKOUT_FROM_GROUP = 19;
    CREATE_ROOM = 20;
    INVITE_INTO_ROOM = 21;
    NOTIFIED_INVITE_INTO_ROOM = 22;
    LEAVE_ROOM = 23;
    NOTIFIED_LEAVE_ROOM = 24;
    SEND_MESSAGE = 25;
    RECEIVE_MESSAGE = 26;
    SEND_MESSAGE_RECEIPT = 27;
    RECEIVE_MESSAGE_RECEIPT = 28;
    SEND_CONTENT_RECEIPT = 29;
    SEND_CHAT_CHECKED = 40;
    SEND_CHAT_REMOVED = 41;
    RECEIVE_ANNOUNCEMENT = 30;
    INVITE_VIA_EMAIL = 38;
    NOTIFIED_REGISTER_USER = 37;
    NOTIFIED_UNREGISTER_USER = 33;
    NOTIFIED_REQUEST_RECOVERY = 39;
    NOTIFIED_FORCE_SYNC = 42;
    SEND_CONTENT = 43;
    SEND_MESSAGE_MYHOME = 44;
    NOTIFIED_UPDATE_CONTENT_PREVIEW = 45;
    REMOVE_ALL_MESSAGES = 46;
    NOTIFIED_UPDATE_PURCHASES = 47;
    DUMMY = 48;
}

// fm.class
enum OperationStatus {
    NORMAL = 1;
    ALERT_DISABLED = 1;
}

// fo.class
struct Operation {
    1: i64 revision;
    2: i64 createdTime;
    3: OperationType type;
    4: i32 reqSeq;
    5: string checkSum;
    7: OperationStatus status;
    10: string param1;
    11: string param2;
    12: string param3;
    20: Message message;
}

enum IdentityProvider {
    UNKNOWN = 0;
    LINE = 1;
    NAVER_KR = 2;
}

struct LoginResult {
    1: string authToken;
    // 2: certificate;
    // 3: verifier;
    // 4: pinCode;
    5: i32 type;
}

// gg.class
struct Profile {
    1: string mid;
    3: string userid;
    10: string phone;
    11: string email;
    12: string regionCode;
    20: string displayName;
    21: string phoneticName;
    22: string pictureStatus;
    23: string thumbnailUrl;
    24: string statusMessage;
    31: bool allowSearchByUserid;
    32: bool allowSearchByEmail;
}

// en.class
enum ContactType {
    MID = 0;
    PHONE = 1;
    EMAIL = 2;
    USERID = 3;
    PROXIMITY = 4;
    GROUP = 5;
    USER = 6;
    QRCODE = 7;
    PROMOTION_BOT = 8;
}

// em.class
enum ContactStatus {
    UNSPECIFIED = 0;
    FRIEND = 1;
    FRIEND_BLOCKED = 2;
    RECOMMEND = 3;
    RECOMMEND_BLOCKED = 4;
}

enum ContactRelation {
    ONEWAY = 0;
    BOTH = 1;
    NOT_REGISTERED = 2;
}

// ef.class
struct Contact {
    1: string mid;
    2: i64 createdTime;
    10: ContactType type;
    11: ContactStatus status;
    21: ContactRelation relation;
    22: string displayName;
    23: string phoneticName;
    24: string pictureStatus;
    25: string thumbnailUrl;
    26: string statusMessage;
    31: bool capableVoiceCall;
    32: bool capableVideoCall;
    33: bool capableMyhome;
    34: bool capableBuddy;
    35: i32 attributes; // Bitfield? 32 = "official account"
    36: i64 settings; // Bitfield? 4 = hidden
}

// et.class
struct Group {
    1: string id;
    2: i64 createdTime;
    10: string name;
    11: string pictureStatus;
    20: list<Contact> members;
    21: Contact creator;
    22: list<Contact> invitee;
}

// jt.class
service Line {
    // Gets authentication key
    LoginResult loginWithIdentityCredentialForCertificate(
        3: string identifier,
        4: string password,
        5: bool keepLoggedIn,
        6: string accessLocation,
        7: string systemName,
        8: IdentityProvider identityProvider,
        9: string certificate) throws (1:Error e);

    //void verifyIdentityCredential(
    //    3: string identifier,
    //    4: string password,
    //    8: IdentityProvider identityProvider)

    // Gets current user's profile
    Profile getProfile();

    // Gets list of current user's contact IDs
    list<string> getAllContactIds();

    // Gets list of current user's recommended contacts IDs
    list<string> getRecommendationIds();

    // Gets detailed information on contacts
    list<Contact> getContacts(2: list<string> ids);

    // Gets list of current user's joined groups
    list<string> getGroupIdsJoined();

    list<Group> getGroups(2: list<string> ids);

    // Get recent messages from a group chat (n.b. arg names guessed)
    list<Message> getRecentMessages(2: string gid, 3: i32 count);

    // Returns incoming events
    list<Operation> fetchOperations(2: i64 localRev, 3: i32 count);

    // Returns current revision ID for use with fetchOperations
    i64 getLastOpRevision();

    // Sends a message to chat or user
    Message sendMessage(1: i32 seq, 2: Message message);

    // Probably sends "message read" notification
    // sendChatChecked
}
