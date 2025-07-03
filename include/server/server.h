#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <future>
#include <csignal>
#include <boost/asio.hpp>
#include "server/session_handler.h"

namespace collab {
namespace server {

class ThreadPool {
public:
    ThreadPool(size_t numThreads) : stop_(false) {
        std::cout << "Creating thread pool with " << numThreads << " worker threads" << std::endl;
        for (size_t i = 0; i < numThreads; ++i) {
            workers_.emplace_back([this] {
                for (;;) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex_);
                        condition_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
                        if (stop_ && tasks_.empty()) return;
                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }
                    task();
                }
            });
        }
    }
    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            stop_ = true;
        }
        condition_.notify_all();
        for (std::thread& worker : workers_) {
            if (worker.joinable()) worker.join();
        }
    }
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
        using return_type = decltype(f(args...));
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        std::future<return_type> result = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            if (stop_) throw std::runtime_error("enqueue on stopped ThreadPool");
            tasks_.emplace([task]() { (*task)(); });
        }
        condition_.notify_one();
        return result;
    }
    size_t size() const { return workers_.size(); }
    size_t queueSize() const {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        return tasks_.size();
    }
private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    mutable std::mutex queue_mutex_;
    std::condition_variable condition_;
    bool stop_;
};

class Server {
public:
    explicit Server(
        boost::asio::io_context& io_context,
        unsigned short port,
        size_t threadPoolSize = std::thread::hardware_concurrency(),
        int sessionCleanupIntervalSeconds = 300,
        int maxSessionIdleSeconds = 3600)
        : io_context_(io_context),
          acceptor_(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
          signals_(io_context, SIGINT, SIGTERM),
          thread_pool_(threadPoolSize),
          session_handler_(),
          cleanup_timer_(io_context),
          session_cleanup_interval_(sessionCleanupIntervalSeconds),
          max_session_idle_(maxSessionIdleSeconds),
          running_(true) {
        std::cout << "Server starting on port: " << port << " with " << thread_pool_.size() << " worker threads" << std::endl;
        setupSignalHandling();
        startSessionCleanup();
        startAccept();
    }
    ~Server() { stop(); }
    void stop() {
        if (!running_.exchange(false)) return;
        std::cout << "\nShutting down server..." << std::endl;
        boost::system::error_code ec;
        cleanup_timer_.cancel(ec);
        acceptor_.close(ec);
        if (ec) std::cerr << "Error closing acceptor: " << ec.message() << std::endl;
        std::cout << "Server shutdown complete" << std::endl;
    }
    bool isRunning() const { return running_.load(); }
    boost::asio::ip::tcp::endpoint getEndpoint() const { return acceptor_.local_endpoint(); }
    void simulateSignal(int signal_number) {
        boost::system::error_code ec;
        signals_.async_wait([this, signal_number](const boost::system::error_code& error, int) {
            if (!error) {
                std::cout << "\nSimulated signal " << signal_number << " received" << std::endl;
                stop();
            }
        });
        signals_.cancel(ec);
    }
    size_t getThreadPoolSize() const { return thread_pool_.size(); }
    size_t getSessionCount() const { return session_handler_.getSessionCount(); }
    SessionHandler& getSessionHandler() { return session_handler_; }
    const SessionHandler& getSessionHandler() const { return session_handler_; }
private:
    class Connection;
    void setupSignalHandling() {
        signals_.async_wait([this](const boost::system::error_code& error, int signal_number) {
            if (!error) {
                std::cout << "\nReceived signal " << signal_number << std::endl;
                stop();
            }
        });
    }
    void startSessionCleanup() {
        cleanup_timer_.expires_after(std::chrono::seconds(session_cleanup_interval_));
        cleanup_timer_.async_wait([this](const boost::system::error_code& error) {
            if (!error && running_) {
                int cleanedCount = session_handler_.cleanupIdleSessions(max_session_idle_);
                if (cleanedCount > 0) {
                    std::cout << "Cleaned up " << cleanedCount << " idle sessions" << std::endl;
                }
                startSessionCleanup();
            }
        });
    }
    void startAccept() {
        auto socket = std::make_shared<boost::asio::ip::tcp::socket>(io_context_);
        acceptor_.async_accept(*socket, [this, socket](const boost::system::error_code& error) {
            if (!error && running_) {
                handleNewConnection(socket);
                if (running_) startAccept();
            } else if (error && running_) {
                std::cerr << "Accept error: " << error.message() << std::endl;
                if (running_) startAccept();
            }
        });
    }
    void handleNewConnection(std::shared_ptr<boost::asio::ip::tcp::socket> socket);
    boost::asio::io_context& io_context_;
    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::signal_set signals_;
    ThreadPool thread_pool_;
    SessionHandler session_handler_;
    boost::asio::steady_timer cleanup_timer_;
    int session_cleanup_interval_;
    int max_session_idle_;
    std::atomic<bool> running_;
};

class Server::Connection : public std::enable_shared_from_this<Connection> {
public:
    Connection(std::shared_ptr<boost::asio::ip::tcp::socket> socket, Server& server,
               const std::string& sessionId, std::shared_ptr<UserSession> session)
        : socket_(socket), server_(server), sessionId_(sessionId), session_(session), buffer_(8192) {}
    ~Connection() {}
    void start() {
        auto self(shared_from_this());
        socket_->async_read_some(boost::asio::buffer(buffer_),
            [this, self](const boost::system::error_code& error, std::size_t bytes_transferred) {
                if (!error) {
                    processDataInThreadPool(bytes_transferred);
                } else {
                    handleError(error);
                }
            });
    }
    void send(const std::string& data) {
        auto self(shared_from_this());
        boost::asio::async_write(*socket_, boost::asio::buffer(data),
            [this, self](const boost::system::error_code& error, std::size_t) {
                if (error) handleError(error);
            });
    }
private:
    void processDataInThreadPool(std::size_t bytes_transferred) {
        auto self(shared_from_this());
        std::string data(buffer_.begin(), buffer_.begin() + bytes_transferred);
        server_.thread_pool_.enqueue([this, self, data]() {
            std::string response = processData(data);
            boost::asio::post(socket_->get_executor(), [this, self, response]() { send(response); });
            boost::asio::post(socket_->get_executor(), [this, self]() { start(); });
        });
    }
    std::string processData(const std::string& data) {
        session_->updateActivity();
        if (data.substr(0, 6) == "LOGIN:") {
            std::string username = data.substr(6);
            if (!server_.getSessionHandler().isUsernameAvailable(username)) {
                return "ERROR: Username already in use";
            }
            return server_.getSessionHandler().authenticateSession(sessionId_, username)
                ? "SUCCESS: Logged in as " + username : "ERROR: Authentication failed";
        }
        if (data.substr(0, 11) == "OPEN_DOCUMENT:") {
            if (session_->getState() != UserSession::State::AUTHENTICATED) {
                return "ERROR: Not authenticated";
            }
            std::string documentId = data.substr(11);
            session_->addDocument(documentId);
            auto users = server_.getSessionHandler().getUsersOnDocument(documentId);
            std::ostringstream oss;
            oss << "SUCCESS: Opened document " << documentId << "\nUsers on this document: ";
            for (size_t i = 0; i < users.size(); ++i) {
                if (i > 0) oss << ", ";
                oss << users[i];
            }
            return oss.str();
        }
        if (data.substr(0, 12) == "CLOSE_DOCUMENT:") {
            if (session_->getState() != UserSession::State::AUTHENTICATED) {
                return "ERROR: Not authenticated";
            }
            std::string documentId = data.substr(12);
            return session_->removeDocument(documentId)
                ? "SUCCESS: Closed document " + documentId : "ERROR: Document not open";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        std::ostringstream oss;
        oss << "Server received: " << data << " (processed by thread " << std::this_thread::get_id()
            << " for user " << (session_->getUsername().empty() ? "anonymous" : session_->getUsername()) << ")";
        std::cout << "Processed: " << data << " in thread " << std::this_thread::get_id()
                  << " for session " << sessionId_ << std::endl;
        return oss.str();
    }
    void handleError(const boost::system::error_code& error) {
        if (error == boost::asio::error::eof || error == boost::asio::error::connection_reset) {
            std::cout << "Connection closed by client" << std::endl;
        } else {
            std::cerr << "Connection error: " << error.message() << std::endl;
        }
        server_.getSessionHandler().closeSession(sessionId_);
    }
    std::shared_ptr<boost::asio::ip::tcp::socket> socket_;
    Server& server_;
    std::string sessionId_;
    std::shared_ptr<UserSession> session_;
    std::vector<char> buffer_;
};

inline void Server::handleNewConnection(std::shared_ptr<boost::asio::ip::tcp::socket> socket) {
    boost::asio::ip::tcp::endpoint remote_ep = socket->remote_endpoint();
    std::cout << "New connection from: " << remote_ep.address().to_string() << ":" << remote_ep.port() << std::endl;
    auto [sessionId, session] = session_handler_.createSession(socket);
    auto connection = std::make_shared<Connection>(socket, *this, sessionId, session);
    connection->start();
}

} // namespace server
} // namespace collab

#endif // COLLABORATIVE_EDITOR_SERVER_H
