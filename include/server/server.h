// include/server/server.h
#ifndef COLLABORATIVE_EDITOR_SERVER_H
#define COLLABORATIVE_EDITOR_SERVER_H

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

/**
 * A thread pool for handling client sessions concurrently
 */
class ThreadPool {
public:
    /**
     * Creates a thread pool with a specified number of worker threads
     * 
     * @param numThreads The number of worker threads in the pool
     */
    ThreadPool(size_t numThreads) : stop_(false) {
        std::cout << "Creating thread pool with " << numThreads << " worker threads" << std::endl;
        
        for (size_t i = 0; i < numThreads; ++i) {
            workers_.emplace_back([this] {
                for (;;) {
                    std::function<void()> task;
                    
                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex_);
                        this->condition_.wait(lock, [this] {
                            return this->stop_ || !this->tasks_.empty();
                        });
                        
                        if (this->stop_ && this->tasks_.empty()) {
                            return;
                        }
                        
                        task = std::move(this->tasks_.front());
                        this->tasks_.pop();
                    }
                    
                    task();
                }
            });
        }
    }
    
    /**
     * Destroys the thread pool, joining all worker threads
     */
    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            stop_ = true;
        }
        
        condition_.notify_all();
        
        for (std::thread& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }
    
    /**
     * Enqueues a task to be executed by the thread pool
     * 
     * @param f The function to execute
     * @param args The arguments to pass to the function
     * @return A future that will hold the result of the function call
     */
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
        using return_type = decltype(f(args...));
        
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
        std::future<return_type> result = task->get_future();
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            
            // Don't allow enqueueing after stopping the pool
            if (stop_) {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }
            
            tasks_.emplace([task]() { (*task)(); });
        }
        
        condition_.notify_one();
        return result;
    }
    
    /**
     * Gets the number of worker threads in the pool
     * 
     * @return The number of worker threads
     */
    size_t size() const {
        return workers_.size();
    }
    
    /**
     * Gets the number of tasks waiting in the queue
     * 
     * @return The number of tasks in the queue
     */
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

/**
 * Server class using Boost.Asio that manages TCP connections for
 * the Collaborative Text Editor. Provides graceful shutdown on SIGINT.
 */
class Server {
public:
    /**
     * Constructor
     * 
     * @param io_context The Boost.Asio io_context to use
     * @param port The port to listen on
     * @param threadPoolSize The number of threads in the thread pool
     */
    explicit Server(
        boost::asio::io_context& io_context, 
        unsigned short port,
        size_t threadPoolSize = std::thread::hardware_concurrency(),
        int sessionCleanupIntervalSeconds = 300,
        int maxSessionIdleSeconds = 3600
    ) : io_context_(io_context),
        acceptor_(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
        signals_(io_context, SIGINT, SIGTERM),
        thread_pool_(threadPoolSize),
        session_handler_(),
        cleanup_timer_(io_context),
        session_cleanup_interval_(sessionCleanupIntervalSeconds),
        max_session_idle_(maxSessionIdleSeconds),
        running_(true) {
        
        std::cout << "Server starting on port: " << port 
                  << " with " << thread_pool_.size() << " worker threads" << std::endl;
        
        // Set up signal handling for graceful shutdown
        setupSignalHandling();
        
        // Start the session cleanup timer
        startSessionCleanup();
        
        // Start accepting connections
        startAccept();
    }

    /**
     * Destructor - ensures clean shutdown
     */
    ~Server() {
        stop();
    }

    /**
     * Stops the server and closes all connections
     */
    void stop() {
        if (!running_.exchange(false)) {
            return; // Already stopped
        }

        std::cout << "\nShutting down server..." << std::endl;

        // Cancel timer
        boost::system::error_code ec;
        cleanup_timer_.cancel(ec);
        
        // Close acceptor to stop accepting new connections
        acceptor_.close(ec);
        if (ec) {
            std::cerr << "Error closing acceptor: " << ec.message() << std::endl;
        }

        // Close all sessions (which will close all connections through RAII)
        // No need to manually close connections as that's handled by SessionHandler
        
        std::cout << "Server shutdown complete" << std::endl;
    }

    /**
     * Returns whether the server is running
     * 
     * @return True if the server is running, false otherwise
     */
    bool isRunning() const {
        return running_.load();
    }
    
    /**
     * Get the server's local endpoint
     * 
     * @return The endpoint the server is listening on
     */
    boost::asio::ip::tcp::endpoint getEndpoint() const {
        return acceptor_.local_endpoint();
    }
    
    /**
     * Simulate a signal being received - for testing purposes
     * 
     * @param signal_number The signal to simulate
     */
    void simulateSignal(int signal_number) {
        boost::system::error_code ec;
        signals_.async_wait(
            [this, signal_number](const boost::system::error_code& error, int) {
                if (!error) {
                    std::cout << "\nSimulated signal " << signal_number << " received" << std::endl;
                    stop();
                }
            });
        signals_.cancel(ec);
    }
    
    /**
     * Get the size of the thread pool
     * 
     * @return The number of threads in the pool
     */
    size_t getThreadPoolSize() const {
        return thread_pool_.size();
    }
    
    /**
     * Get the number of active sessions
     * 
     * @return The number of sessions
     */
    size_t getSessionCount() const {
        return session_handler_.getSessionCount();
    }
    
    /**
     * Access the session handler
     * 
     * @return Reference to the session handler
     */
    SessionHandler& getSessionHandler() {
        return session_handler_;
    }
    
    /**
     * Access the session handler (const version)
     * 
     * @return Const reference to the session handler
     */
    const SessionHandler& getSessionHandler() const {
        return session_handler_;
    }

private:
    // Forward declaration for Connection class
    class Connection;

    /**
     * Sets up signal handling for graceful shutdown
     */
    void setupSignalHandling() {
        signals_.async_wait(
            [this](const boost::system::error_code& error, int signal_number) {
                if (!error) {
                    std::cout << "\nReceived signal " << signal_number << std::endl;
                    stop();
                }
            });
    }
    
    /**
     * Starts the periodic session cleanup timer
     */
    void startSessionCleanup() {
        cleanup_timer_.expires_after(std::chrono::seconds(session_cleanup_interval_));
        cleanup_timer_.async_wait([this](const boost::system::error_code& error) {
            if (!error && running_) {
                // Run the cleanup
                int cleanedCount = session_handler_.cleanupIdleSessions(max_session_idle_);
                if (cleanedCount > 0) {
                    std::cout << "Cleaned up " << cleanedCount << " idle sessions" << std::endl;
                }
                
                // Restart the timer
                startSessionCleanup();
            }
        });
    }

    /**
     * Starts accepting new connections
     */
    void startAccept() {
        auto socket = std::make_shared<boost::asio::ip::tcp::socket>(io_context_);
        
        acceptor_.async_accept(
            *socket,
            [this, socket](const boost::system::error_code& error) {
                if (!error && running_) {
                    handleNewConnection(socket);
                    
                    // Continue accepting connections if we're still running
                    if (running_) {
                        startAccept();
                    }
                } else if (error && running_) {
                    std::cerr << "Accept error: " << error.message() << std::endl;
                    
                    // Try to recover by continuing to accept
                    if (running_) {
                        startAccept();
                    }
                }
            });
    }

    /**
     * Handles a new client connection
     * 
     * @param socket The connected socket
     */
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

/**
 * Connection class for handling individual client connections
 */
class Server::Connection : public std::enable_shared_from_this<Connection> {
public:
    /**
     * Constructor
     * 
     * @param socket The connected socket
     * @param server The server that created this connection
     * @param sessionId The session ID
     * @param session The user session
     */
    Connection(
        std::shared_ptr<boost::asio::ip::tcp::socket> socket, 
        Server& server,
        const std::string& sessionId,
        std::shared_ptr<UserSession> session)
        : socket_(socket),
          server_(server),
          sessionId_(sessionId),
          session_(session),
          buffer_(8192) {
    }
    
    /**
     * Destructor
     */
    ~Connection() {
        // Socket is managed by SessionHandler through SocketGuard
    }

    /**
     * Starts reading from the connection
     */
    void start() {
        auto self(shared_from_this());
        socket_->async_read_some(
            boost::asio::buffer(buffer_),
            [this, self](const boost::system::error_code& error, std::size_t bytes_transferred) {
                if (!error) {
                    // Process received data - now handled in thread pool
                    processDataInThreadPool(bytes_transferred);
                } else {
                    handleError(error);
                }
            });
    }

    /**
     * Sends data to the client
     * 
     * @param data The data to send
     */
    void send(const std::string& data) {
        auto self(shared_from_this());
        
        boost::asio::async_write(
            *socket_,
            boost::asio::buffer(data),
            [this, self](const boost::system::error_code& error, std::size_t /*bytes_transferred*/) {
                if (error) {
                    handleError(error);
                }
            });
    }

private:
    /**
     * Processes received data in thread pool
     * 
     * @param bytes_transferred The number of bytes received
     */
    void processDataInThreadPool(std::size_t bytes_transferred) {
        auto self(shared_from_this());
        
        // Copy the data to ensure it doesn't get overwritten during async operations
        std::string data(buffer_.begin(), buffer_.begin() + bytes_transferred);
        
        // Process the data in the thread pool
        server_.thread_pool_.enqueue([this, self, data]() {
            // Process the data in a worker thread
            std::string response = processData(data);
            
            // Send the response back via the io_context thread
            boost::asio::post(socket_->get_executor(), [this, self, response]() {
                send(response);
            });
            
            // Continue reading - must be done in the io_context thread
            boost::asio::post(socket_->get_executor(), [this, self]() {
                start();
            });
        });
    }
    
    /**
     * Process the received data - runs in a worker thread
     * 
     * @param data The data to process
     * @return The response to send back
     */
    std::string processData(const std::string& data) {
        // Update user session activity time
        session_->updateActivity();
        
        // Check if this is a login message
        if (data.substr(0, 6) == "LOGIN:") {
            std::string username = data.substr(6);
            
            // Check if the username is available
            if (!server_.getSessionHandler().isUsernameAvailable(username)) {
                return "ERROR: Username already in use";
            }
            
            // Authenticate the session
            if (server_.getSessionHandler().authenticateSession(sessionId_, username)) {
                return "SUCCESS: Logged in as " + username;
            } else {
                return "ERROR: Authentication failed";
            }
        }
        
        // Check if this is a document open message
        if (data.substr(0, 11) == "OPEN_DOCUMENT:") {
            // Check if authenticated
            if (session_->getState() != UserSession::State::AUTHENTICATED) {
                return "ERROR: Not authenticated";
            }
            
            std::string documentId = data.substr(11);
            session_->addDocument(documentId);
            
            // Get all users on this document
            auto users = server_.getSessionHandler().getUsersOnDocument(documentId);
            
            std::ostringstream oss;
            oss << "SUCCESS: Opened document " << documentId << "\n";
            oss << "Users on this document: ";
            for (size_t i = 0; i < users.size(); ++i) {
                if (i > 0) oss << ", ";
                oss << users[i];
            }
            
            return oss.str();
        }
        
        // Check if this is a document close message
        if (data.substr(0, 12) == "CLOSE_DOCUMENT:") {
            // Check if authenticated
            if (session_->getState() != UserSession::State::AUTHENTICATED) {
                return "ERROR: Not authenticated";
            }
            
            std::string documentId = data.substr(12);
            if (session_->removeDocument(documentId)) {
                return "SUCCESS: Closed document " + documentId;
            } else {
                return "ERROR: Document not open";
            }
        }
    
        // Simulate some processing time
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        std::ostringstream oss;
        oss << "Server received: " << data 
            << " (processed by thread " << std::this_thread::get_id()
            << " for user " << (session_->getUsername().empty() ? "anonymous" : session_->getUsername())
            << ")";
        
        std::cout << "Processed: " << data << " in thread " << std::this_thread::get_id() 
                  << " for session " << sessionId_ << std::endl;
        
        return oss.str();
    }

    /**
     * Handles connection errors
     * 
     * @param error The error that occurred
     */
    void handleError(const boost::system::error_code& error) {
        if (error == boost::asio::error::eof || 
            error == boost::asio::error::connection_reset) {
            std::cout << "Connection closed by client" << std::endl;
        } else {
            std::cerr << "Connection error: " << error.message() << std::endl;
        }
        
        // Close the session (which will close the socket via RAII)
        server_.getSessionHandler().closeSession(sessionId_);
    }

    std::shared_ptr<boost::asio::ip::tcp::socket> socket_;
    Server& server_;
    std::string sessionId_;
    std::shared_ptr<UserSession> session_;
    std::vector<char> buffer_;
};

// Implementation of Server method that depends on Connection
inline void Server::handleNewConnection(std::shared_ptr<boost::asio::ip::tcp::socket> socket) {
    boost::asio::ip::tcp::endpoint remote_ep = socket->remote_endpoint();
    std::cout << "New connection from: " << remote_ep.address().to_string() 
              << ":" << remote_ep.port() << std::endl;
    
    // Create a new session for this connection
    auto [sessionId, session] = session_handler_.createSession(socket);
    
    // Create a new connection object
    auto connection = std::make_shared<Connection>(socket, *this, sessionId, session);
    
    // Start reading from the connection
    connection->start();
}

} // namespace server
} // namespace collab

#endif // COLLABORATIVE_EDITOR_SERVER_H