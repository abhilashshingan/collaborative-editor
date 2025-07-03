#include <iostream>
#include <string>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <chrono>
#include <boost/asio.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace collab {
namespace server {

class UserSession {
public:
    enum class State { CONNECTING, AUTHENTICATING, AUTHENTICATED, DISCONNECTED };
    UserSession(const std::string& id, const std::string& username = "")
        : id_(id), username_(username), state_(State::CONNECTING),
          creation_time_(std::chrono::steady_clock::now()), last_activity_(creation_time_) {}
    const std::string& getId() const { return id_; }
    const std::string& getUsername() const { return username_; }
    void setUsername(const std::string& username) { username_ = username; updateActivity(); }
    State getState() const { return state_; }
    void setState(State state) { state_ = state; updateActivity(); }
    std::chrono::steady_clock::time_point getCreationTime() const { return creation_time_; }
    std::chrono::steady_clock::time_point getLastActivity() const { return last_activity_; }
    void updateActivity() { last_activity_ = std::chrono::steady_clock::now(); }
    std::chrono::seconds getIdleSeconds() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::seconds>(now - last_activity_);
    }
    bool addDocument(const std::string& documentId) {
        auto result = active_documents_.insert(documentId);
        if (result.second) updateActivity();
        return result.second;
    }
    bool removeDocument(const std::string& documentId) {
        auto count = active_documents_.erase(documentId);
        if (count > 0) updateActivity();
        return count > 0;
    }
    bool hasDocument(const std::string& documentId) const {
        return active_documents_.find(documentId) != active_documents_.end();
    }
    const std::unordered_set<std::string>& getActiveDocuments() const { return active_documents_; }
private:
    std::string id_;
    std::string username_;
    State state_;
    std::chrono::steady_clock::time_point creation_time_;
    std::chrono::steady_clock::time_point last_activity_;
    std::unordered_set<std::string> active_documents_;
};

class SocketGuard {
public:
    explicit SocketGuard(std::shared_ptr<boost::asio::ip::tcp::socket> socket) : socket_(socket) {}
    ~SocketGuard() { close(); }
    SocketGuard(const SocketGuard&) = delete;
    SocketGuard& operator=(const SocketGuard&) = delete;
    SocketGuard(SocketGuard&& other) noexcept : socket_(std::move(other.socket_)) { other.socket_ = nullptr; }
    SocketGuard& operator=(SocketGuard&& other) noexcept {
        if (this != &other) { close(); socket_ = std::move(other.socket_); other.socket_ = nullptr; }
        return *this;
    }
    void close() {
        if (socket_ && socket_->is_open()) {
            boost::system::error_code ec;
            socket_->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
            socket_->close(ec);
        }
    }
    std::shared_ptr<boost::asio::ip::tcp::socket> socket() const { return socket_; }
    bool isValid() const { return socket_ && socket_->is_open(); }
private:
    std::shared_ptr<boost::asio::ip::tcp::socket> socket_;
};

class SessionHandler {
public:
    SessionHandler() : uuid_generator_() {}
    ~SessionHandler() {}
    std::pair<std::string, std::shared_ptr<UserSession>> createSession(
        std::shared_ptr<boost::asio::ip::tcp::socket> socket) {
        boost::uuids::uuid uuid = uuid_generator_();
        std::string sessionId = boost::uuids::to_string(uuid);
        auto session = std::make_shared<UserSession>(sessionId);
        auto socketGuard = std::make_shared<SocketGuard>(socket);
        std::lock_guard<std::mutex> lock(mutex_);
        sessions_[sessionId] = session;
        sockets_[sessionId] = socketGuard;
        std::cout << "Created session: " << sessionId << std::endl;
        return {sessionId, session};
    }
    bool authenticateSession(const std::string& sessionId, const std::string& username) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = sessions_.find(sessionId);
        if (it != sessions_.end()) {
            it->second->setUsername(username);
            it->second->setState(UserSession::State::AUTHENTICATED);
            username_to_session_[username] = sessionId;
            std::cout << "Authenticated session " << sessionId << " as " << username << std::endl;
            return true;
        }
        return false;
    }
    std::shared_ptr<UserSession> getSession(const std::string& sessionId) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = sessions_.find(sessionId);
        return it != sessions_.end() ? it->second : nullptr;
    }
    std::shared_ptr<UserSession> getSessionByUsername(const std::string& username) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = username_to_session_.find(username);
        if (it != username_to_session_.end()) {
            auto sessionIt = sessions_.find(it->second);
            return sessionIt != sessions_.end() ? sessionIt->second : nullptr;
        }
        return nullptr;
    }
    std::shared_ptr<SocketGuard> getSocket(const std::string& sessionId) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = sockets_.find(sessionId);
        return it != sockets_.end() ? it->second : nullptr;
    }
    bool closeSession(const std::string& sessionId) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto sessionIt = sessions_.find(sessionId);
        if (sessionIt == sessions_.end()) return false;
        if (sessionIt->second->getState() == UserSession::State::AUTHENTICATED) {
            username_to_session_.erase(sessionIt->second->getUsername());
        }
        sessionIt->second->setState(UserSession::State::DISCONNECTED);
        sockets_.erase(sessionId);
        sessions_.erase(sessionIt);
        std::cout << "Closed session: " << sessionId << std::endl;
        return true;
    }
    std::unordered_map<std::string, std::shared_ptr<UserSession>> getSessions() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return sessions_;
    }
    size_t getSessionCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return sessions_.size();
    }
    std::vector<std::string> getUsersOnDocument(const std::string& documentId) {
        std::vector<std::string> users;
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& [sessionId, session] : sessions_) {
            if (session->hasDocument(documentId) && session->getState() == UserSession::State::AUTHENTICATED) {
                users.push_back(session->getUsername());
            }
        }
        return users;
    }
    bool isUsernameAvailable(const std::string& username) {
        std::lock_guard<std::mutex> lock(mutex_);
        return username_to_session_.find(username) == username_to_session_.end();
    }
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
        for (const auto& sessionId : sessionsToClose) {
            closeSession(sessionId);
        }
        return static_cast<int>(sessionsToClose.size());
    }
private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<UserSession>> sessions_;
    std::unordered_map<std::string, std::shared_ptr<SocketGuard>> sockets_;
    std::unordered_map<std::string, std::string> username_to_session_;
    boost::uuids::random_generator uuid_generator_;
};

} // namespace server
} // namespace collab

#endif // COLLABORATIVE_EDITOR_SESSION_HANDLER_H
