namespace cpp line

// er.class
enum ErrorCode {
    ILLEGAL_ARGUMENT = 0;
    AUTHENTICATION_FAILED = 1;
}

// jq.class
struct Error {
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
    GROUP = 3;
}

// eo.class
enum MessageContentType {
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
    1: string from_;
    2: string to;
    3: MessageToType toType;
    4: string id;
    5: i64 createdTime;
    6: i64 deliveredTime;
    10: string text;
    11: Location location;
    14: bool hasContent;
    15: MessageContentType contentType;
    17: string contentPreview;
    18: map<string, string> contentMetadata;
}

// fn.class
enum OperationType {
    SEND_MESSAGE = 25;
    RECEIVE_MESSAGE = 26;
    SEND_MESSAGE_RECEIPT = 27;
    RECEIVE_MESSAGE_RECEIPT = 28;
    SEND_CHAT_CHECKED = 40;
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
    35: i32 attributes;
    36: i64 settings;
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
        9: string certificate)

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

    // Returns incoming events
    list<Operation> fetchOperations(2: i64 localRev, 3: i32 count);

    // Sends a message to chat or user
    Message sendMessage(1: i32 seq, 2: Message message);

    // Probably sends "message read" notification
    // sendChatChecked
}
