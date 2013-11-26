// This file was generated with rethrift.py
// enum names were guessed, but the rest of the names should be accurate.

enum ApplicationType {
    IOS = 16;
    IOS_RC = 17;
    IOS_BETA = 18;
    IOS_ALPHA = 19;
    ANDROID = 32;
    ANDROID_RC = 33;
    ANDROID_BETA = 34;
    ANDROID_ALPHA = 35;
    WAP = 48;
    WAP_RC = 49;
    WAP_BETA = 50;
    WAP_ALPHA = 51;
    BOT = 64;
    BOT_RC = 65;
    BOT_BETA = 66;
    BOT_ALPHA = 67;
    WEB = 80;
    WEB_RC = 81;
    WEB_BETA = 82;
    WEB_ALPHA = 83;
    DESKTOPWIN = 96;
    DESKTOPWIN_RC = 97;
    DESKTOPWIN_BETA = 98;
    DESKTOPWIN_ALPHA = 99;
    DESKTOPMAC = 112;
    DESKTOPMAC_RC = 113;
    DESKTOPMAC_BETA = 114;
    DESKTOPMAC_ALPHA = 115;
    CHANNELGW = 128;
    CHANNELGW_RC = 129;
    CHANNELGW_BETA = 130;
    CHANNELGW_ALPHA = 131;
    CHANNELCP = 144;
    CHANNELCP_RC = 145;
    CHANNELCP_BETA = 146;
    CHANNELCP_ALPHA = 147;
    WINPHONE = 160;
    WINPHONE_RC = 161;
    WINPHONE_BETA = 162;
    WINPHONE_ALPHA = 163;
    BLACKBERRY = 176;
    BLACKBERRY_RC = 177;
    BLACKBERRY_BETA = 178;
    BLACKBERRY_ALPHA = 179;
    WINMETRO = 192;
    WINMETRO_RC = 193;
    WINMETRO_BETA = 194;
    WINMETRO_ALPHA = 195;
    S40 = 208;
    S40_RC = 209;
    S40_BETA = 210;
    S40_ALPHA = 211;
    CHRONO = 224;
    CHRONO_RC = 225;
    CHRONO_BETA = 226;
    CHRONO_ALPHA = 227;
    TIZEN = 256;
    TIZEN_RC = 257;
    TIZEN_BETA = 258;
    TIZEN_ALPHA = 259;
    VIRTUAL = 272;
    FIREFOXOS = 288;
    FIREFOXOS_RC = 289;
    FIREFOXOS_BETA = 290;
    FIREFOXOS_ALPHA = 291;
}

enum Carrier {
    NOT_SPECIFIED = 0;
    JP_DOCOMO = 1;
    JP_AU = 2;
    JP_SOFTBANK = 3;
    KR_SKT = 17;
    KR_KT = 18;
    KR_LGT = 19;
}

enum Category {
    PROFILE = 0;
    SETTINGS = 1;
    OPS = 2;
    CONTACT = 3;
    RECOMMEND = 4;
    BLOCK = 5;
    GROUP = 6;
    ROOM = 7;
    NOTIFICATION = 8;
}

enum Code {
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
    ACCOUNT_NOT_MATCHED = 34;
    ABUSE_BLOCK = 35;
    NOT_FRIEND = 36;
    NOT_ALLOWED_CALL = 37;
    BLOCK_FRIEND = 38;
    INCOMPATIBLE_VOIP_VERSION = 39;
    INVALID_SNS_ACCESS_TOKEN = 40;
    EXTERNAL_SERVICE_NOT_AVAILABLE = 41;
    NOT_ALLOWED_ADD_CONTACT = 42;
    NOT_CERTIFICATED = 43;
    NOT_ALLOWED_SECONDARY_DEVICE = 44;
    INVALID_PIN_CODE = 45;
    NOT_FOUND_IDENTITY_CREDENTIAL = 46;
    EXCEED_FILE_MAX_SIZE = 47;
    EXCEED_DAILY_QUOTA = 48;
    NOT_SUPPORT_SEND_FILE = 49;
    MUST_UPGRADE = 50;
    NOT_AVAILABLE_PIN_CODE_SESSION = 51;
}

enum ContactStatus {
    UNSPECIFIED = 0;
    FRIEND = 1;
    FRIEND_BLOCKED = 2;
    RECOMMEND = 3;
    RECOMMEND_BLOCKED = 4;
    DELETED = 5;
    DELETED_BLOCKED = 6;
}

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
    REPAIR = 128;
    FACEBOOK = 2305;
    SINA = 2306;
    RENREN = 2307;
    FEIXIN = 2308;
}

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
    GROUPBOARD = 10;
    APPLINK = 11;
    LINK = 12;
    CONTACT = 13;
    FILE = 14;
    LOCATION = 15;
    POSTNOTIFICATION = 16;
    RICH = 17;
    CHATEVENT = 18;
}

enum CustomModes {
    PROMOTION_FRIENDS_INVITE = 1;
    CAPABILITY_SERVER_SIDE_SMS = 2;
    LINE_CLIENT_ANALYTICS_CONFIGURATION = 3;
}

enum EmailConfirmationStatus {
    NOT_SPECIFIED = 0;
    NOT_YET = 1;
    DONE = 3;
}

enum EmailConfirmationType {
    SERVER_SIDE_EMAIL = 0;
    CLIENT_SIDE_EMAIL = 1;
}

enum FeatureType {
}

enum Flag {
    CONTACT_SETTING_NOTIFICATION_DISABLE = 1;
    CONTACT_SETTING_DISPLAY_NAME_OVERRIDE = 2;
    CONTACT_SETTING_CONTACT_HIDE = 4;
    CONTACT_SETTING_FAVORITE = 8;
    CONTACT_SETTING_DELETE = 16;
}

enum Method {
    NO_AVAILABLE = 0;
    PIN_VIA_SMS = 1;
    CALLERID_INDIGO = 2;
    PIN_VIA_TTS = 4;
    SKIP = 10;
}

enum OperationStatus {
    NORMAL = 0;
    ALERT_DISABLED = 1;
}

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
    UPDATE_CONTACT = 49;
    NOTIFIED_RECEIVED_CALL = 50;
    CANCEL_CALL = 51;
    NOTIFIED_REDIRECT = 52;
    NOTIFIED_CHANNEL_SYNC = 53;
    FAILED_SEND_MESSAGE = 54;
    NOTIFIED_READ_MESSAGE = 55;
    FAILED_EMAIL_CONFIRMATION = 56;
    NOTIFIED_PUSH_NOTICENTER_ITEM = 59;
    NOTIFIED_CHAT_CONTENT = 58;
}

enum Provider {
    UNKNOWN = 0;
    LINE = 1;
    NAVER_KR = 2;
}

enum Relation {
    ONEWAY = 0;
    BOTH = 1;
    NOT_REGISTERED = 2;
}

enum Result {
    FAILED = 0;
    OK_NOT_REGISTERED_YET = 1;
    OK_REGISTERED_WITH_SAME_DEVICE = 2;
    OK_REGISTERED_WITH_ANOTHER_DEVICE = 3;
}

enum SnsFriendModificationType {
    ADD = 0;
    REMOVE = 1;
    MODIFY = 2;
}

enum SnsIdType {
    FACEBOOK = 1;
    SINA = 2;
    RENREN = 3;
    FEIXIN = 4;
}

enum SpammerReasons {
    OTHER = 0;
    ADVERTISING = 1;
    GENDER_HARASSMENT = 2;
    HARASSMENT = 3;
}

enum ToType {
    USER = 0;
    ROOM = 1;
    GROUP = 2;
}

enum UpdateNotificationTokenType {
    APPLE_APNS = 1;
    GOOGLE_C2DM = 2;
    NHN_NNI = 3;
    SKT_AOM = 4;
    MS_MPNS = 5;
    RIM_BIS = 6;
    GOOGLE_GCM = 7;
    NOKIA_NNAPI = 8;
    TIZEN = 9;
    MOZILLA_SIMPLE = 10;
    LINE_BOT = 17;
    LINE_WAP = 18;
}

enum UpdateProfileAttributeAttr {
    ALL = 255;
    EMAIL = 1;
    DISPLAY_NAME = 2;
    PHONETIC_NAME = 4;
    PICTURE = 8;
    STATUS_MESSAGE = 16;
    ALLOW_SEARCH_BY_USERID = 32;
    ALLOW_SEARCH_BY_EMAIL = 64;
    BUDDY_STATUS = 128;
}

enum UpdateSettingsAttributeAttr {
    NOTIFICATION_ENABLE = 1;
    NOTIFICATION_MUTE_EXPIRATION = 2;
    NOTIFICATION_NEW_MESSAGE = 4;
    NOTIFICATION_GROUP_INVITATION = 8;
    NOTIFICATION_SHOW_MESSAGE = 16;
    NOTIFICATION_INCOMING_CALL = 32;
    NOTIFICATION_SOUND_MESSAGE = 256;
    NOTIFICATION_SOUND_GROUP = 512;
    PRIVACY_SYNC_CONTACTS = 64;
    PRIVACY_SEARCH_BY_PHONE_NUMBER = 128;
    PRIVACY_SEARCH_BY_USERID = 8192;
    PRIVACY_SEARCH_BY_EMAIL = 16384;
    CONTACT_MY_TICKET = 1024;
    IDENTITY_PROVIDER = 2048;
    IDENTITY_IDENTIFIER = 4096;
    PREFERENCE_LOCALE = 32768;
}

struct Contact {
    1: string mid;
    2: i64 createdTime;
    10: ContactType type;
    11: ContactStatus status;
    21: Relation relation;
    22: string displayName;
    23: string phoneticName;
    24: string pictureStatus;
    25: string thumbnailUrl;
    26: string statusMessage;
    27: string displayNameOverridden;
    28: i64 favoriteTime;
    31: bool capableVoiceCall;
    32: bool capableVideoCall;
    33: bool capableMyhome;
    34: bool capableBuddy;
    35: i32 attributes;
    36: i64 settings;
    37: string picturePath;
}

struct ContactModification {
    1: SnsFriendModificationType type;
    2: string luid;
    11: list<string> phones;
    12: list<string> emails;
    13: list<string> userids;
}

struct ContactRegistration {
    1: Contact contact;
    10: string luid;
    11: ContactType contactType;
    12: string contactKey;
}

struct DeviceInfo {
    1: string deviceName;
    2: string systemName;
    3: string systemVersion;
    4: string model;
    10: Carrier carrierCode;
    11: string carrierName;
    20: ApplicationType applicationType;
}

struct EmailConfirmation {
    1: bool usePasswordSet;
    2: string email;
    3: string password;
    4: bool ignoreDuplication;
}

struct EmailConfirmationSession {
    1: EmailConfirmationType emailConfirmationType;
    2: string verifier;
    3: string targetEmail;
}

struct Group {
    1: string id;
    2: i64 createdTime;
    10: string name;
    11: string pictureStatus;
    20: list<Contact> members;
    21: Contact creator;
    22: list<Contact> invitee;
    31: bool notificationDisabled;
}

struct IdentityCredential {
    1: Provider provider;
    2: string identifier;
    3: string password;
}

struct Location {
    1: string title;
    2: string address;
    3: double latitude;
    4: double longitude;
    5: string phone;
}

struct LoginSession {
    1: string tokenKey;
    3: i64 expirationTime;
    11: ApplicationType applicationType;
    12: string systemName;
    22: string accessLocation;
}

struct Message {
    1: string from;
    2: string to;
    3: ToType toType;
    4: string id;
    5: i64 createdTime;
    6: i64 deliveredTime;
    10: string text;
    11: Location location;
    14: bool hasContent;
    15: ContentType contentType;
    17: string contentPreview;
    18: map<string, string> contentMetadata;
}

struct Operation {
    1: i64 revision;
    2: i64 createdTime;
    3: OperationType type;
    4: i32 reqSeq;
    5: string checksum;
    7: OperationStatus status;
    10: string param1;
    11: string param2;
    12: string param3;
    20: Message message;
}

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
    33: string picturePath;
}

struct RSAKey {
    1: string keynm;
    2: string nvalue;
    3: string evalue;
    4: string sessionKey;
}

struct RegisterWithSnsIdResult {
    1: string authToken;
    2: bool userCreated;
}

struct Room {
    1: string mid;
    2: i64 createdTime;
    10: list<Contact> contacts;
    31: bool notificationDisabled;
}

struct Settings {
    10: bool notificationEnable;
    11: i64 notificationMuteExpiration;
    12: bool notificationNewMessage;
    13: bool notificationGroupInvitation;
    14: bool notificationShowMessage;
    15: bool notificationIncomingCall;
    16: string notificationSoundMessage;
    17: string notificationSoundGroup;
    18: bool notificationDisabledWithSub;
    20: bool privacySyncContacts;
    21: bool privacySearchByPhoneNumber;
    22: bool privacySearchByUserid;
    23: bool privacySearchByEmail;
    24: bool privacyAllowSecondaryDeviceLogin;
    25: bool privacyProfileImagePostToMyhome;
    26: bool privacyReceiveMessagesFromNotFriend;
    30: string contactMyTicket;
    40: Provider identityProvider;
    41: string identityIdentifier;
    42: map<SnsIdType, string> snsAccounts;
    43: bool phoneRegistration;
    44: EmailConfirmationStatus emailConfirmationStatus;
    50: string preferenceLocale;
    60: map<CustomModes, string> customModes;
}

struct SnsFriend {
    1: string snsUserId;
    2: string snsUserName;
    3: SnsIdType snsIdType;
}

struct SnsFriendContactRegistration {
    1: Contact contact;
    2: SnsIdType snsIdType;
    3: string snsUserId;
}

struct SnsFriendModification {
    1: SnsFriendModificationType type;
    2: SnsFriend snsFriend;
}

struct SnsIdUserStatus {
    1: bool userExisting;
    2: bool phoneNumberRegistered;
    3: bool sameDevice;
}

struct TMessageBox {
    1: string id;
    2: string channelId;
    5: i64 lastSeq;
    6: i64 unreadCount;
    7: i64 lastModifiedTime;
    8: i32 status;
    9: ToType midType;
    10: list<Message> lastMessages;
}

exception TalkException {
    1: Code code;
    2: string reason;
    3: map<string, string> parameterMap;
}

struct Ticket {
    1: string id;
    10: i64 expirationTime;
    21: i32 maxUseCount;
}

struct UserAuthStatus {
    1: bool phoneNumberRegistered;
    2: list<SnsIdType> registeredSnsIdTypes;
}

struct VerificationSessionData {
    1: string sessionId;
    2: Method method;
    3: string callback;
    4: string normalizedPhone;
    5: string countryCode;
    6: string nationalSignificantNumber;
    7: list<Method> availableVerificationMethods;
}

service LineTalk {
    void acceptGroupInvitation(
        1: i32 reqSeq,
        2: string groupId) throws (1: TalkException e);

    void acceptProximityMatches(
        2: string sessionId,
        3: list<string> ids) throws (1: TalkException e);

    list<string> acquireCallRoute(
        2: string to) throws (1: TalkException e);

    string acquireEncryptedAccessToken(
        2: FeatureType featureType) throws (1: TalkException e);

    string addSnsId(
        2: SnsIdType snsIdType,
        3: string snsAccessToken) throws (1: TalkException e);

    void blockContact(
        1: i32 reqSeq,
        2: string id) throws (1: TalkException e);

    void blockRecommendation(
        1: i32 reqSeq,
        2: string id) throws (1: TalkException e);

    void cancelGroupInvitation(
        1: i32 reqSeq,
        2: string groupId,
        3: list<string> contactIds) throws (1: TalkException e);

    VerificationSessionData changeVerificationMethod(
        2: string sessionId,
        3: Method method) throws (1: TalkException e);

    void clearIdentityCredential() throws (1: TalkException e);

    void closeProximityMatch(
        2: string sessionId) throws (1: TalkException e);

    void confirmEmail(
        2: string verifier,
        3: string pinCode) throws (1: TalkException e);

    Group createGroup(
        1: i32 seq,
        2: string name,
        3: list<string> contactIds) throws (1: TalkException e);

    Room createRoom(
        1: i32 reqSeq,
        2: list<string> contactIds) throws (1: TalkException e);

    list<Operation> fetchOps(
        2: i64 localRev,
        3: i32 count,
        4: i64 globalRev,
        5: i64 individualRev) throws (1: TalkException e);

    map<string, Contact> findAndAddContactsByMid(
        1: i32 reqSeq,
        2: string mid) throws (1: TalkException e);

    map<string, Contact> findAndAddContactsByUserid(
        1: i32 reqSeq,
        2: string userid) throws (1: TalkException e);

    Contact findContactByUserTicket(
        2: string ticketId) throws (1: TalkException e);

    Contact findContactByUserid(
        2: string userid) throws (1: TalkException e);

    SnsIdUserStatus findSnsIdUserStatus(
        2: SnsIdType snsIdType,
        3: string snsAccessToken,
        4: string udidHash) throws (1: TalkException e);

    void finishUpdateVerification(
        2: string sessionId) throws (1: TalkException e);

    Ticket generateUserTicket(
        3: i64 expirationTime,
        4: i32 maxUseCount) throws (1: TalkException e);

    list<string> getAcceptedProximityMatches(
        2: string sessionId) throws (1: TalkException e);

    list<string> getAllContactIds() throws (1: TalkException e);

    list<string> getBlockedContactIds() throws (1: TalkException e);

    list<string> getBlockedRecommendationIds() throws (1: TalkException e);

    Contact getContact(
        2: string id) throws (1: TalkException e);

    list<Contact> getContacts(
        2: list<string> ids) throws (1: TalkException e);

    Group getGroup(
        2: string groupId) throws (1: TalkException e);

    list<string> getGroupIdsInvited() throws (1: TalkException e);

    list<string> getGroupIdsJoined() throws (1: TalkException e);

    string getIdentityIdentifier() throws (1: TalkException e);

    TMessageBox getMessageBox(
        2: string channelId,
        3: string messageBoxId,
        4: i32 lastMessagesCount) throws (1: TalkException e);

    list<Message> getMessagesBySequenceNumber(
        2: string channelId,
        3: string messageBoxId,
        4: i64 startSeq,
        5: i64 endSeq) throws (1: TalkException e);

    Profile getProfile() throws (1: TalkException e);

    list<Contact> getProximityMatchCandidates(
        2: string sessionId) throws (1: TalkException e);

    RSAKey getRSAKeyInfo(
        2: Provider provider) throws (1: TalkException e);

    list<string> getRecommendationIds() throws (1: TalkException e);

    Room getRoom(
        2: string roomId) throws (1: TalkException e);

    i64 getServerTime() throws (1: TalkException e);

    list<LoginSession> getSessions() throws (1: TalkException e);

    Settings getSettings() throws (1: TalkException e);

    Settings getSettingsAttributes(
        2: i32 attrBitset) throws (1: TalkException e);

    void inviteFriendsBySms(
        2: list<string> phoneNumberList) throws (1: TalkException e);

    void inviteIntoGroup(
        1: i32 reqSeq,
        2: string groupId,
        3: list<string> contactIds) throws (1: TalkException e);

    void inviteIntoRoom(
        1: i32 reqSeq,
        2: string roomId,
        3: list<string> contactIds) throws (1: TalkException e);

    bool isUseridAvailable(
        2: string userid) throws (1: TalkException e);

    void kickoutFromGroup(
        1: i32 reqSeq,
        2: string groupId,
        3: list<string> contactIds) throws (1: TalkException e);

    void leaveGroup(
        1: i32 reqSeq,
        2: string groupId) throws (1: TalkException e);

    void leaveRoom(
        1: i32 reqSeq,
        2: string roomId) throws (1: TalkException e);

    void logoutSession(
        2: string tokenKey) throws (1: TalkException e);

    void noop() throws (1: TalkException e);

    void notifyInstalled(
        2: string udidHash,
        3: string applicationTypeWithExtensions);

    void notifyRegistrationComplete(
        2: string udidHash,
        3: string applicationTypeWithExtensions);

    void notifySleep(
        2: i64 lastRev,
        3: i32 badge) throws (1: TalkException e);

    void notifyUpdated(
        2: i64 lastRev,
        3: DeviceInfo deviceInfo) throws (1: TalkException e);

    string openProximityMatch(
        2: Location location) throws (1: TalkException e);

    string registerDevice(
        2: string sessionId) throws (1: TalkException e);

    string registerDeviceWithIdentityCredential(
        2: string sessionId,
        3: string identifier,
        4: string verifier,
        5: Provider provider) throws (1: TalkException e);

    bool registerUserid(
        1: i32 reqSeq,
        2: string userid) throws (1: TalkException e);

    string registerWithExistingSnsIdAndIdentityCredential(
        2: IdentityCredential identityCredential,
        3: string region,
        4: string udidHash,
        5: DeviceInfo deviceInfo) throws (1: TalkException e);

    RegisterWithSnsIdResult registerWithSnsId(
        2: SnsIdType snsIdType,
        3: string snsAccessToken,
        4: string region,
        5: string udidHash,
        6: DeviceInfo deviceInfo,
        7: string mid) throws (1: TalkException e);

    string registerWithSnsIdAndIdentityCredential(
        2: SnsIdType snsIdType,
        3: string snsAccessToken,
        4: IdentityCredential identityCredential,
        5: string region,
        6: string udidHash,
        7: DeviceInfo deviceInfo) throws (1: TalkException e);

    void rejectGroupInvitation(
        1: i32 reqSeq,
        2: string groupId) throws (1: TalkException e);

    void removeAllMessages(
        1: i32 seq,
        2: string lastMessageId) throws (1: TalkException e);

    bool removeMessageFromMyHome(
        2: string messageId) throws (1: TalkException e);

    string removeSnsId(
        2: SnsIdType snsIdType) throws (1: TalkException e);

    void report(
        2: i64 syncOpRevision,
        3: Category category,
        4: string report) throws (1: TalkException e);

    void reportProfile(
        2: i64 syncOpRevision,
        3: Profile profile) throws (1: TalkException e);

    void reportSettings(
        2: i64 syncOpRevision,
        3: Settings settings) throws (1: TalkException e);

    void reportSpammer(
        2: string spammerMid,
        3: list<SpammerReasons> spammerReasons,
        4: list<string> spamMessageIds) throws (1: TalkException e);

    void requestAccountPasswordReset(
        2: string identifier,
        4: Provider provider,
        5: string locale) throws (1: TalkException e);

    EmailConfirmationSession requestEmailConfirmation(
        2: EmailConfirmation emailConfirmation) throws (1: TalkException e);

    EmailConfirmationSession resendEmailConfirmation(
        2: string verifier) throws (1: TalkException e);

    void resendPinCode(
        2: string sessionId) throws (1: TalkException e);

    void sendChatChecked(
        1: i32 seq,
        2: string consumer,
        3: string lastMessageId) throws (1: TalkException e);

    void sendChatRemoved(
        1: i32 seq,
        2: string consumer,
        3: string lastMessageId) throws (1: TalkException e);

    void sendContentReceipt(
        1: i32 seq,
        2: string consumer,
        3: string messageId) throws (1: TalkException e);

    Message sendMessage(
        1: i32 seq,
        2: Message message) throws (1: TalkException e);

    Message sendMessageToMyHome(
        1: i32 seq,
        2: Message message) throws (1: TalkException e);

    void setIdentityCredential(
        2: string identifier,
        3: string verifier,
        4: Provider provider) throws (1: TalkException e);

    void setNotificationsEnabled(
        1: i32 reqSeq,
        2: ToType type,
        3: string target,
        4: bool enablement) throws (1: TalkException e);

    VerificationSessionData startUpdateVerification(
        2: string region,
        3: Carrier carrier,
        4: string phone,
        5: string udidHash,
        6: DeviceInfo deviceInfo,
        7: string networkCode,
        8: string locale) throws (1: TalkException e);

    VerificationSessionData startVerification(
        2: string region,
        3: Carrier carrier,
        4: string phone,
        5: string udidHash,
        6: DeviceInfo deviceInfo,
        7: string networkCode,
        8: string mid,
        9: string locale) throws (1: TalkException e);

    list<SnsFriendContactRegistration> syncContactBySnsIds(
        1: i32 reqSeq,
        2: list<SnsFriendModification> modifications) throws (1: TalkException e);

    map<string, ContactRegistration> syncContacts(
        1: i32 reqSeq,
        2: list<ContactModification> localContacts) throws (1: TalkException e);

    void unblockContact(
        1: i32 reqSeq,
        2: string id) throws (1: TalkException e);

    void unblockRecommendation(
        1: i32 reqSeq,
        2: string id) throws (1: TalkException e);

    string unregisterUserAndDevice() throws (1: TalkException e);

    void updateContactSetting(
        1: i32 reqSeq,
        2: string mid,
        3: Flag flag,
        4: string value) throws (1: TalkException e);

    void updateGroup(
        1: i32 reqSeq,
        2: Group group) throws (1: TalkException e);

    void updateNotificationToken(
        2: string token,
        3: UpdateNotificationTokenType type) throws (1: TalkException e);

    void updateProfileAttribute(
        1: i32 reqSeq,
        2: UpdateProfileAttributeAttr attr,
        3: string value) throws (1: TalkException e);

    void updateSettingsAttribute(
        1: i32 reqSeq,
        2: UpdateSettingsAttributeAttr attr,
        3: string value) throws (1: TalkException e);

    void verifyIdentityCredential(
        3: string identifier,
        4: string password,
        8: Provider identityProvider) throws (1: TalkException e);

    UserAuthStatus verifyIdentityCredentialWithResult(
        2: IdentityCredential identityCredential) throws (1: TalkException e);

    Result verifyPhone(
        2: string sessionId,
        3: string pinCode,
        4: string udidHash) throws (1: TalkException e);

    string verifyQrcode(
        2: string verifier,
        3: string pinCode) throws (1: TalkException e);
}

