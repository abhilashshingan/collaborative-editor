#ifndef COLLABORATIVE_EDITOR_PROTOCOL_H
#define COLLABORATIVE_EDITOR_PROTOCOL_H

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <variant>
#include <chrono>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace collab {
namespace protocol {

// Forward declarations
struct Message;
struct AuthMessage;
struct DocumentMessage;
struct EditMessage;
struct SyncMessage;
struct PresenceMessage;

/**
 * Enumeration of message types
 */
enum class MessageType {
    // Authentication messages
    AUTH_LOGIN = 100,
    AUTH_LOGOUT = 101,
    AUTH_REGISTER = 102,
    AUTH_SUCCESS = 103,
    AUTH_FAILURE = 104,
    
    // Document management messages
    DOC_CREATE = 200,
    DOC_OPEN = 201,
    DOC_CLOSE = 202,
    DOC_LIST = 203,
    DOC_INFO = 204,
    DOC_DELETE = 205,
    DOC_RENAME = 206,
    DOC_RESPONSE = 207,
    
    // Edit operation messages
    EDIT_INSERT = 300,
    EDIT_DELETE = 301,
    EDIT_REPLACE = 302,
    EDIT_APPLY = 303,
    EDIT_REJECT = 304,
    
    // Synchronization messages
    SYNC_REQUEST = 400,
    SYNC_RESPONSE = 401,
    SYNC_STATE = 402,
    SYNC_ACK = 403,
    
    // Presence messages
    PRESENCE_JOIN = 500,
    PRESENCE_LEAVE = 501,
    PRESENCE_CURSOR = 502,
    PRESENCE_SELECTION = 503,
    PRESENCE_UPDATE = 504,
    
    // System messages
    SYS_ERROR = 900,
    SYS_INFO = 901,
    SYS_HEARTBEAT = 902,
    SYS_DISCONNECT = 903
};

/**
 * Base message structure
 */
struct Message {
    MessageType type;
    std::string clientId;
    std::string sessionId;
    std::uint64_t sequenceNumber;
    std::uint64_t timestamp;
    
    // Default constructor with current timestamp
    Message(MessageType type)
        : type(type)
        , sequenceNumber(0)
        , timestamp(std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::system_clock::now().time_since_epoch()).count()) {}
    
    // Virtual destructor for polymorphic behavior
    virtual ~Message() = default;
    
    // Utility for converting message to string
    virtual std::string toString() const {
        nlohmann::json j;
        j["type"] = static_cast<int>(type);
        j["clientId"] = clientId;
        j["sessionId"] = sessionId;
        j["sequenceNumber"] = sequenceNumber;
        j["timestamp"] = timestamp;
        return j.dump();
    }
    
    // Utility for parsing a message from string
    static std::variant<Message, AuthMessage, DocumentMessage, EditMessage, 
                        SyncMessage, PresenceMessage> fromString(const std::string& str);
};

/**
 * Authentication-related messages
 */
struct AuthMessage : public Message {
    std::string username;
    std::optional<std::string> password;
    std::optional<std::string> token;
    std::optional<std::string> errorMessage;
    std::map<std::string, std::string> metadata;
    
    AuthMessage(MessageType type)
        : Message(type) {
        if (type != MessageType::AUTH_LOGIN && 
            type != MessageType::AUTH_LOGOUT &&
            type != MessageType::AUTH_REGISTER && 
            type != MessageType::AUTH_SUCCESS && 
            type != MessageType::AUTH_FAILURE) {
            throw std::invalid_argument("Invalid authentication message type");
        }
    }
    
    std::string toString() const override {
        nlohmann::json j = nlohmann::json::parse(Message::toString());
        j["username"] = username;
        
        if (password.has_value()) j["password"] = *password;
        if (token.has_value()) j["token"] = *token;
        if (errorMessage.has_value()) j["errorMessage"] = *errorMessage;
        
        j["metadata"] = metadata;
        
        return j.dump();
    }
};

/**
 * Document management messages
 */
struct DocumentMessage : public Message {
    std::string documentId;
    std::optional<std::string> documentName;
    std::optional<std::string> documentContent;
    std::optional<std::string> documentPath;
    std::optional<uint64_t> documentVersion;
    std::vector<std::string> documentList;
    std::map<std::string, std::string> metadata;
    std::optional<bool> success;
    std::optional<std::string> errorMessage;
    
    DocumentMessage(MessageType type)
        : Message(type) {
        if (type != MessageType::DOC_CREATE && 
            type != MessageType::DOC_OPEN &&
            type != MessageType::DOC_CLOSE && 
            type != MessageType::DOC_LIST && 
            type != MessageType::DOC_INFO &&
            type != MessageType::DOC_DELETE &&
            type != MessageType::DOC_RENAME &&
            type != MessageType::DOC_RESPONSE) {
            throw std::invalid_argument("Invalid document message type");
        }
    }
    
    std::string toString() const override {
        nlohmann::json j = nlohmann::json::parse(Message::toString());
        j["documentId"] = documentId;
        
        if (documentName.has_value()) j["documentName"] = *documentName;
        if (documentContent.has_value()) j["documentContent"] = *documentContent;
        if (documentPath.has_value()) j["documentPath"] = *documentPath;
        if (documentVersion.has_value()) j["documentVersion"] = *documentVersion;
        
        j["documentList"] = documentList;
        j["metadata"] = metadata;
        
        if (success.has_value()) j["success"] = *success;
        if (errorMessage.has_value()) j["errorMessage"] = *errorMessage;
        
        return j.dump();
    }
};

/**
 * Edit operation messages
 */
struct EditMessage : public Message {
    std::string documentId;
    uint64_t documentVersion;
    std::string operationId;
    std::optional<std::size_t> position;
    std::optional<std::size_t> length;
    std::optional<std::string> text;
    std::optional<bool> success;
    std::optional<std::string> errorMessage;
    
    EditMessage(MessageType type)
        : Message(type)
        , documentVersion(0) {
        if (type != MessageType::EDIT_INSERT && 
            type != MessageType::EDIT_DELETE &&
            type != MessageType::EDIT_REPLACE && 
            type != MessageType::EDIT_APPLY && 
            type != MessageType::EDIT_REJECT) {
            throw std::invalid_argument("Invalid edit message type");
        }
    }
    
    std::string toString() const override {
        nlohmann::json j = nlohmann::json::parse(Message::toString());
        j["documentId"] = documentId;
        j["documentVersion"] = documentVersion;
        j["operationId"] = operationId;
        
        if (position.has_value()) j["position"] = *position;
        if (length.has_value()) j["length"] = *length;
        if (text.has_value()) j["text"] = *text;
        if (success.has_value()) j["success"] = *success;
        if (errorMessage.has_value()) j["errorMessage"] = *errorMessage;
        
        return j.dump();
    }
};

/**
 * Synchronization messages
 */
struct SyncMessage : public Message {
    std::string documentId;
    std::optional<uint64_t> fromVersion;
    std::optional<uint64_t> toVersion;
    std::vector<std::string> operations;
    std::optional<std::string> documentState;
    std::optional<bool> success;
    std::optional<std::string> errorMessage;
    
    SyncMessage(MessageType type)
        : Message(type) {
        if (type != MessageType::SYNC_REQUEST && 
            type != MessageType::SYNC_RESPONSE &&
            type != MessageType::SYNC_STATE && 
            type != MessageType::SYNC_ACK) {
            throw std::invalid_argument("Invalid synchronization message type");
        }
    }
    
    std::string toString() const override {
        nlohmann::json j = nlohmann::json::parse(Message::toString());
        j["documentId"] = documentId;
        
        if (fromVersion.has_value()) j["fromVersion"] = *fromVersion;
        if (toVersion.has_value()) j["toVersion"] = *toVersion;
        
        j["operations"] = operations;
        
        if (documentState.has_value()) j["documentState"] = *documentState;
        if (success.has_value()) j["success"] = *success;
        if (errorMessage.has_value()) j["errorMessage"] = *errorMessage;
        
        return j.dump();
    }
};

/**
 * Presence and collaboration messages
 */
struct PresenceMessage : public Message {
    std::string documentId;
    std::string username;
    std::optional<std::string> displayName;
    std::optional<std::size_t> cursorPosition;
    std::optional<std::size_t> selectionStart;
    std::optional<std::size_t> selectionEnd;
    std::optional<std::string> userColor;
    std::map<std::string, std::string> metadata;
    
    PresenceMessage(MessageType type)
        : Message(type) {
        if (type != MessageType::PRESENCE_JOIN && 
            type != MessageType::PRESENCE_LEAVE &&
            type != MessageType::PRESENCE_CURSOR && 
            type != MessageType::PRESENCE_SELECTION && 
            type != MessageType::PRESENCE_UPDATE) {
            throw std::invalid_argument("Invalid presence message type");
        }
    }
    
    std::string toString() const override {
        nlohmann::json j = nlohmann::json::parse(Message::toString());
        j["documentId"] = documentId;
        j["username"] = username;
        
        if (displayName.has_value()) j["displayName"] = *displayName;
        if (cursorPosition.has_value()) j["cursorPosition"] = *cursorPosition;
        if (selectionStart.has_value()) j["selectionStart"] = *selectionStart;
        if (selectionEnd.has_value()) j["selectionEnd"] = *selectionEnd;
        if (userColor.has_value()) j["userColor"] = *userColor;
        
        j["metadata"] = metadata;
        
        return j.dump();
    }
};

// Implementation of the fromString method
inline std::variant<Message, AuthMessage, DocumentMessage, EditMessage, 
                     SyncMessage, PresenceMessage> Message::fromString(const std::string& str) {
    nlohmann::json j = nlohmann::json::parse(str);
    MessageType type = static_cast<MessageType>(j["type"].get<int>());
    
    // Authentication messages
    if (type == MessageType::AUTH_LOGIN || 
        type == MessageType::AUTH_LOGOUT ||
        type == MessageType::AUTH_REGISTER ||
        type == MessageType::AUTH_SUCCESS ||
        type == MessageType::AUTH_FAILURE) {
        
        AuthMessage msg(type);
        msg.clientId = j["clientId"];
        msg.sessionId = j["sessionId"];
        msg.sequenceNumber = j["sequenceNumber"];
        msg.timestamp = j["timestamp"];
        msg.username = j["username"];
        
        if (j.contains("password")) msg.password = j["password"];
        if (j.contains("token")) msg.token = j["token"];
        if (j.contains("errorMessage")) msg.errorMessage = j["errorMessage"];
        if (j.contains("metadata")) msg.metadata = j["metadata"].get<std::map<std::string, std::string>>();
        
        return msg;
    }
    
    // Document messages
    else if (type == MessageType::DOC_CREATE ||
             type == MessageType::DOC_OPEN ||
             type == MessageType::DOC_CLOSE ||
             type == MessageType::DOC_LIST ||
             type == MessageType::DOC_INFO ||
             type == MessageType::DOC_DELETE ||
             type == MessageType::DOC_RENAME ||
             type == MessageType::DOC_RESPONSE) {
        
        DocumentMessage msg(type);
        msg.clientId = j["clientId"];
        msg.sessionId = j["sessionId"];
        msg.sequenceNumber = j["sequenceNumber"];
        msg.timestamp = j["timestamp"];
        msg.documentId = j["documentId"];
        
        if (j.contains("documentName")) msg.documentName = j["documentName"];
        if (j.contains("documentContent")) msg.documentContent = j["documentContent"];
        if (j.contains("documentPath")) msg.documentPath = j["documentPath"];
        if (j.contains("documentVersion")) msg.documentVersion = j["documentVersion"];
        if (j.contains("documentList")) msg.documentList = j["documentList"].get<std::vector<std::string>>();
        if (j.contains("metadata")) msg.metadata = j["metadata"].get<std::map<std::string, std::string>>();
        if (j.contains("success")) msg.success = j["success"];
        if (j.contains("errorMessage")) msg.errorMessage = j["errorMessage"];
        
        return msg;
    }
    
    // Edit messages
    else if (type == MessageType::EDIT_INSERT ||
             type == MessageType::EDIT_DELETE ||
             type == MessageType::EDIT_REPLACE ||
             type == MessageType::EDIT_APPLY ||
             type == MessageType::EDIT_REJECT) {
        
        EditMessage msg(type);
        msg.clientId = j["clientId"];
        msg.sessionId = j["sessionId"];
        msg.sequenceNumber = j["sequenceNumber"];
        msg.timestamp = j["timestamp"];
        msg.documentId = j["documentId"];
        msg.documentVersion = j["documentVersion"];
        msg.operationId = j["operationId"];
        
        if (j.contains("position")) msg.position = j["position"];
        if (j.contains("length")) msg.length = j["length"];
        if (j.contains("text")) msg.text = j["text"];
        if (j.contains("success")) msg.success = j["success"];
        if (j.contains("errorMessage")) msg.errorMessage = j["errorMessage"];
        
        return msg;
    }
    
    // Sync messages
    else if (type == MessageType::SYNC_REQUEST ||
             type == MessageType::SYNC_RESPONSE ||
             type == MessageType::SYNC_STATE ||
             type == MessageType::SYNC_ACK) {
        
        SyncMessage msg(type);
        msg.clientId = j["clientId"];
        msg.sessionId = j["sessionId"];
        msg.sequenceNumber = j["sequenceNumber"];
        msg.timestamp = j["timestamp"];
        msg.documentId = j["documentId"];
        
        if (j.contains("fromVersion")) msg.fromVersion = j["fromVersion"];
        if (j.contains("toVersion")) msg.toVersion = j["toVersion"];
        if (j.contains("operations")) msg.operations = j["operations"].get<std::vector<std::string>>();
        if (j.contains("documentState")) msg.documentState = j["documentState"];
        if (j.contains("success")) msg.success = j["success"];
        if (j.contains("errorMessage")) msg.errorMessage = j["errorMessage"];
        
        return msg;
    }
    
    // Presence messages
    else if (type == MessageType::PRESENCE_JOIN ||
             type == MessageType::PRESENCE_LEAVE ||
             type == MessageType::PRESENCE_CURSOR ||
             type == MessageType::PRESENCE_SELECTION ||
             type == MessageType::PRESENCE_UPDATE) {
        
        PresenceMessage msg(type);
        msg.clientId = j["clientId"];
        msg.sessionId = j["sessionId"];
        msg.sequenceNumber = j["sequenceNumber"];
        msg.timestamp = j["timestamp"];
        msg.documentId = j["documentId"];
        msg.username = j["username"];
        
        if (j.contains("displayName")) msg.displayName = j["displayName"];
        if (j.contains("cursorPosition")) msg.cursorPosition = j["cursorPosition"];
        if (j.contains("selectionStart")) msg.selectionStart = j["selectionStart"];
        if (j.contains("selectionEnd")) msg.selectionEnd = j["selectionEnd"];
        if (j.contains("userColor")) msg.userColor = j["userColor"];
        if (j.contains("metadata")) msg.metadata = j["metadata"].get<std::map<std::string, std::string>>();
        
        return msg;
    }
    
    // Default case: basic message
    Message msg(type);
    msg.clientId = j["clientId"];
    msg.sessionId = j["sessionId"];
    msg.sequenceNumber = j["sequenceNumber"];
    msg.timestamp = j["timestamp"];
    
    return msg;
}

} // namespace protocol
} // namespace collab

#endif // COLLABORATIVE_EDITOR_PROTOCOL_H