// include/server/session_handler.h
#ifndef COLLABORATIVE_EDITOR_SESSION_HANDLER_H
#define COLLABORATIVE_EDITOR_SESSION_HANDLER_H

#include <iostream>
#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <boost/asio.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace collab {
namespace server {

/**
 * Represents a user session with authentication and state information
 */
class UserSession {
public:
    enum class State {
        CONNECTING,
        AUTHENTICATING,
        AUTHENTICATED,
        DISCONNECTED
    };

    /**
     * Constructor for a new user session
     * 
     * @param id Unique session ID
     * @param username User's name (if known)
     */
    UserSession(const std::string& id, const std::string& username = "")
        : id_(id),
          username_(username),
          state_(State::CONNECTING),
          creation_time_(std::chrono::steady_clock::now()),
          last_activity_(creation_time_) {
    }

    /**
     * Gets the session ID
     * 
     * @return The session ID
     */
    const std::string& getId() const {
        return id_;
    }

    /**
     * Gets the username
     * 
     * @return The username
     */
    const std::string& getUsername() const {
        return username_;
    }

    /**
     * Sets the username
     * 
     * @param username The username to set
     */
    void setUsername(const std::string& username) {
        username_ = username;
        updateActivity();
    }

    /**
     * Gets the session state
     * 
     * @return The current state
     */
    State getState() const {
        return state_;
    }

    /**
     * Sets the session state
     * 
     * @param state The state to set
     */
    void setState(State state) {
        state_ = state;
        updateActivity();
    }

    /**
     * Gets the creation time
     * 
     * @return The creation time
     */
    std::chrono::steady_clock::time_point getCreationTime() const {
        return creation_time_;
    }

    /**
     * Gets the time of last activity
     * 
     * @return The last activity time
     */
    std::chrono::steady_clock::time_point getLastActivity() const {
        return last_activity_;
    }

    /**
     * Updates the last activity time to now
     */
    void updateActivity() {
        last_activity_ = std::chrono::steady_clock::now();
    }

    /**
     * Gets the idle duration (time since last activity)
     * 
     * @return The idle duration
     */
    std::chrono::seconds getIdleSeconds() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::seconds>(now - last_activity_);
    }

    /**
     * Adds a user to a document's active users
     * 
     * @param documentId The document ID
     * @return True if added, false if already present
     */
    bool addDocument(const std::string& documentId) {
        auto result = active_documents_.insert(documentId);
        if (result.second) {
            updateActivity();
        }
        return result.second;
    }

    /**
     * Removes a document from active documents
     * 
     * @param documentId The document ID
     * @return True if removed, false if not found
     */
    bool removeDocument(const std::string& documentId) {
        auto count = active_documents_.erase(documentId);
        if (count > 0) {
            updateActivity();
        }
        return count > 0;
    }

    /**
     * Checks if a document is active for this user
     * 
     * @param documentId The document ID
     * @return True if active, false otherwise
     */
    bool hasDocument(const std::string& documentId) const {
        return active_documents_.find(documentId) != active_documents_.end();
    }

    /**
     * Gets all active documents for this user
     * 
     * @return Set of document IDs
     */
    const std::unordered_set<std::string>& getActiveDocuments() const {
        return active_documents_;
    }

private:
    std::string id_;
    std::string username_;
    State state_;
    std::chrono::steady_clock::time_point creation_time_;
    std::chrono::steady_clock::time_point last_activity_;
    std::unordered_set<std::string> active_documents_;
};

/**
 * RAII wrapper for a socket to ensure proper cleanup
 */
class SocketGuard {
public:
    /**
     * Constructor takes ownership of the socket
     * 
     * @param socket The socket to manage
     */
    explicit SocketGuard(std::shared_ptr<boost::asio::ip::tcp::socket> socket)
        : socket_(socket) {
    }
    
    /**
     * Destructor ensures socket is closed
     */
    ~SocketGuard() {
        close();
    }
    
    // Delete copy constructor and assignment
    SocketGuard(const SocketGuard&) = delete;
    SocketGuard& operator=(const SocketGuard&) = delete;
    
    // Allow move operations
    SocketGuard(SocketGuard&& other) noexcept
        : socket_(std::move(other.socket_)) {
        other.socket_ = nullptr;
    }
    
    SocketGuard& operator=(SocketGuard&& other) noexcept {
        if (this != &other) {
            close();
            socket_ = std::move(other.socket_);
            other.socket_ = nullptr;
        }
        return *this;
    }
    
    /**
     * Explicitly close the socket
     */
    void close() {
        if (socket_ && socket_->is_open()) {
            boost::system::error_code ec;
            socket_->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
            socket_->close(ec);
        }
    }
    
    /**
     * Get the underlying socket
     * 
     * @return The managed socket
     */
    std::shared_ptr<boost::asio::ip::tcp::socket> socket() const {
        return socket_;
    }
    
    /**
     * Check if the socket is valid and open
     * 
     * @return True if valid and open
     */
    bool isValid() const {
        return socket_ && socket_->is_open();
    }

private:
    std::shared_ptr<boost::asio::ip::tcp::socket> socket_;
};

/**
 * Manages user sessions and their connections
 */
class SessionHandler {
public:
    /**
     * Constructor
     */
    SessionHandler() 
        : uuid_generator_() {
    }
    
    /**
     * Destructor
     */
    ~SessionHandler() {
        // Sessions will be cleaned up automatically due to RAII
    }
    
    /**
     * Creates a new session for a connection
     * 
     * @param socket The client socket
     * @return A tuple with the session ID and the session object
     */
    std::pair<std::string, std::shared_ptr<UserSession>> createSession(
        std::shared_ptr<boost::asio::ip::tcp::socket> socket) {
        
        // Generate a unique ID for the session
        boost::uuids::uuid uuid = uuid_generator_();
        std::string sessionId = boost::uuids::to_string(uuid);
        
        // Create a new session
        auto session = std::make_shared<UserSession>(sessionId);
        
        // Create socket guard to ensure RAII for socket lifecycle
        auto socketGuard = std::make_shared<SocketGuard>(socket);
        
        // Store the session and socket
        std::lock_guard<std::mutex> lock(mutex_);
        sessions_[sessionId] = session;
        sockets_[sessionId] = socketGuard;
        
        std::cout << "Created session: " << sessionId << std::endl;
        return {sessionId, session};
    }
    
    /**
     * Authenticates a session with a username
     * 
     * @param sessionId The session ID
     * @param username The username to associate
     * @return True if successful, false if session not found
     */
    bool authenticateSession(const std::string& sessionId, const std::string& username) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = sessions_.find(sessionId);
        if (it != sessions_.end()) {
            it->second->setUsername(username);
            it->second->setState(UserSession::State::AUTHENTICATED);
            
            // Add to username mapping
            username_to_session_[username] = sessionId;
            
            std::cout << "Authenticated session " << sessionId << " as " << username << std::endl;
            return true;
        }
        return false;
    }
    
    /**
     * Gets a session by ID
     * 
     * @param sessionId The session ID
     * @return The session or nullptr if not found
     */
    std::shared_ptr<UserSession> getSession(const std::string& sessionId) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = sessions_.find(sessionId);
        if (it != sessions_.end()) {
            return it->second;
        }
        return nullptr;
    }
    
    /**
     * Gets a session by username
     * 
     * @param username The username
     * @return The session or nullptr if not found
     */
    std::shared_ptr<UserSession> getSessionByUsername(const std::string& username) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = username_to_session_.find(username);
        if (it != username_to_session_.end()) {
            auto sessionIt = sessions_.find(it->second);
            if (sessionIt != sessions_.end()) {
                return sessionIt->second;
            }
        }
        return nullptr;
    }
    
    /**
     * Gets the socket for a session
     * 
     * @param sessionId The session ID
     * @return The socket guard or nullptr if not found
     */
    std::shared_ptr<SocketGuard> getSocket(const std::string& sessionId) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = sockets_.find(sessionId);
        if (it != sockets_.end()) {
            return it->second;
        }
        return nullptr;
    }
    
    /**
     * Closes a session
     * 
     * @param sessionId The session ID
     * @return True if found and closed, false if not found
     */
    bool closeSession(const std::string& sessionId) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Find the session
        auto sessionIt = sessions_.find(sessionId);
        if (sessionIt == sessions_.end()) {
            return false;
        }
        
        // Remove from username mapping if authenticated
        if (sessionIt->second->getState() == UserSession::State::AUTHENTICATED) {
            const auto& username = sessionIt->second->getUsername();
            username_to_session_.erase(username);
        }
        
        // Mark as disconnected
        sessionIt->second->setState(UserSession::State::DISCONNECTED);
        
        // Remove socket (which will close it due to RAII)
        sockets_.erase(sessionId);
        
        // Remove session
        sessions_.erase(sessionIt);
        
        std::cout << "Closed session: " << sessionId << std::endl;
        return true;
    }
    
    /**
     * Gets all active sessions
     * 
     * @return Map of session IDs to sessions
     */
    std::unordered_map<std::string, std::shared_ptr<UserSession>> getSessions() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return sessions_;
    }
    
    /**
     * Gets the number of active sessions
     * 
     * @return The number of sessions
     */
    size_t getSessionCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return sessions_.size();
    }
    
    /**
     * Gets users active on a document
     * 
     * @param documentId The document ID
     * @return Vector of usernames active on the document
     */
    std::vector<std::string> getUsersOnDocument(const std::string& documentId) {
        std::vector<std::string> users;
        std::lock_guard<std::mutex> lock(mutex_);
        
        for (const auto& [sessionId, session] : sessions_) {
            if (session->hasDocument(documentId) && 
                session->getState() == UserSession::State::AUTHENTICATED) {
                users.push_back(session->getUsername());
            }
        }
        
        return users;
    }
    
    /**
     * Checks if a username is available (not currently in use)
     * 
     * @param username The username to check
     * @return True if available, false if in use
     */
    bool isUsernameAvailable(const std::string& username) {
        std::lock_guard<std::mutex> lock(mutex_);
        return username_to_session_.find(username) == username_to_session_.end();
    }
    
    /**
     * Clean up idle sessions
     * 
     * @param maxIdleSeconds Maximum idle time before cleanup
     * @return Number of sessions cleaned up
     */
    int cleanupIdleSessions(int maxIdleSeconds) {
        std::vector<std::string> sessionsToClose;
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (const auto& [sessionId, session] : sessions_) {
                if (session->getIdleSeconds().count() > maxIdleSeconds) {
                    sessionsToClose.push_back(sessionId);
                }
            }
        }
        
        // Close each idle session
        for (const auto& sessionId : sessionsToClose) {
            closeSession(sessionId);
        }
        
        return static_cast<int>(sessionsToClose.size());
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<UserSession>> sessions_;
    std::unordered_map<std::string, std::shared_ptr<SocketGuard>> sockets_;
    std::unordered_map<std::string, std::string> username_to_session_; // username -> sessionId
    boost::uuids::random_generator uuid_generator_;
};

} // namespace server
} // namespace collab

#endif // COLLABORATIVE_EDITOR_SESSION_HANDLER_H