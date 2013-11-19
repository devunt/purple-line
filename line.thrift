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
    GROUP = 3,
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
    UNKNOWN = 0,
    LINE = 1,
    NAVER_KR = 2,
}

struct LoginResult {
    1: string authToken,
    // 2: certificate,
    // 3: verifier
    // 4: pinCode
    5: i32 type,
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

    void verifyIdentityCredential(
        3: string identifier,
        4: string password,
        8: IdentityProvider identityProvider)

    // Sends a message to chat or user
    Message sendMessage(1: i32 seq, 2: Message message);

    // Returns incoming events
    list<Operation> fetchOperations(2: i64 localRev, 3: i32 count);

    // Probably sends "message read" notification
    // sendChatChecked
}
