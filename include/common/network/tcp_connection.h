#ifndef COLLABORATIVE_EDITOR_TCP_CONNECTION_H
#define COLLABORATIVE_EDITOR_TCP_CONNECTION_H

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <string>
#include <deque>
#include <iostream>
#include <functional>
#include <atomic>
#include <mutex>
#include <thread>
#include <memory>
#include <set>

namespace collab {
namespace network {

/**
 * Base TCP connection class using Boost.Asio
 * 
 * This class handles the low-level TCP connection for both client and server sides.
 * It provides asynchronous read/write operations and connection management.
 */
class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
    using pointer = std::shared_ptr<TcpConnection>;
    using message_handler = std::function<void(pointer, const std::string&)>;
    using close_handler = std::function<void(pointer)>;

    /**
     * Create a new TCP connection
     * 
     * @param io_context The Boost.Asio io_context to use
     * @return A new TcpConnection instance
     */
    static pointer create(boost::asio::io_context& io_context) {
        return pointer(new TcpConnection(io_context));
    }

    /**
     * Get the socket associated with the connection
     * 
     * @return Reference to the socket
     */
    boost::asio::ip::tcp::socket& socket() {
        return socket_;
    }

    /**
     * Start the connection - should be called after connection is established
     */
    void start() {
        connected_ = true;
        
        // Get the remote endpoint information for connection tracking
        try {
            remote_endpoint_ = socket_.remote_endpoint();
            std::cout << "Connection established with " << remote_endpoint_ << std::endl;
        } catch (const boost::system::system_error& e) {
            std::cerr << "Error getting remote endpoint: " << e.what() << std::endl;
        }
        
        // Start the first async read operation
        read();
    }

    /**
     * Close the connection
     */
    void close() {
        // Only process close once
        if (!connected_) return;
        
        std::cout << "Closing connection to " << remote_endpoint_ << std::endl;
        
        boost::system::error_code ignored_ec;
        socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
        socket_.close(ignored_ec);
        
        connected_ = false;
        
        // Notify about the closed connection
        if (close_handler_) {
            close_handler_(shared_from_this());
        }
    }

    /**
     * Send data asynchronously
     * 
     * @param data The data to send
     */
    void write(const std::string& data) {
        // Post the write operation to the io_context to ensure thread safety
        boost::asio::post(socket_.get_executor(), [this, data]() {
            bool write_in_progress = !write_queue_.empty();
            write_queue_.push_back(data);
            if (!write_in_progress) {
                do_write();
            }
        });
    }

    /**
     * Set the message handler
     * 
     * @param handler The handler to call when a message is received
     */
    void set_message_handler(message_handler handler) {
        message_handler_ = handler;
    }

    /**
     * Set the close handler
     * 
     * @param handler The handler to call when the connection is closed
     */
    void set_close_handler(close_handler handler) {
        close_handler_ = handler;
    }
    
    /**
     * Check if the connection is still active
     * 
     * @return True if connected, false otherwise
     */
    bool is_connected() const {
        return connected_ && socket_.is_open();
    }
    
    /**
     * Get the remote endpoint
     * 
     * @return The remote endpoint as a string
     */
    std::string get_remote_address() const {
        return remote_endpoint_.address().to_string();
    }
    
    /**
     * Get the remote port
     * 
     * @return The remote port
     */
    unsigned short get_remote_port() const {
        return remote_endpoint_.port();
    }
    
    /**
     * Get a string representation of the connection
     * 
     * @return String representation
     */
    std::string to_string() const {
        return remote_endpoint_.address().to_string() + ":" + 
               std::to_string(remote_endpoint_.port());
    }

private:
    // Private constructor - use create() instead
    TcpConnection(boost::asio::io_context& io_context)
        : socket_(io_context)
        , connected_(false) {}
    
    // Asynchronous read operation
    void read() {
        auto self = shared_from_this();
        
        // Set up async read operation for the header first
        // For simplicity, we'll use \n as message delimiter
        boost::asio::async_read_until(
            socket_,
            read_buffer_,
            '\n',
            [this, self](const boost::system::error_code& ec, std::size_t bytes_transferred) {
                if (!ec) {
                    // Extract the message from the buffer
                    std::string message;
                    std::istream is(&read_buffer_);
                    std::getline(is, message);
                    
                    // Call the message handler
                    if (message_handler_) {
                        message_handler_(self, message);
                    }
                    
                    // Continue reading
                    read();
                } else if (ec != boost::asio::error::operation_aborted) {
                    std::cerr << "Read error: " << ec.message() << std::endl;
                    close();
                }
            });
    }
    
    // Asynchronous write operation
    void do_write() {
        auto self = shared_from_this();
        
        // Add newline as message delimiter
        std::string message = write_queue_.front() + "\n";
        
        boost::asio::async_write(
            socket_,
            boost::asio::buffer(message),
            [this, self](const boost::system::error_code& ec, std::size_t /*bytes_transferred*/) {
                if (!ec) {
                    // Message successfully sent, remove it from the queue
                    write_queue_.pop_front();
                    
                    // If there are more messages to send, continue with the next one
                    if (!write_queue_.empty()) {
                        do_write();
                    }
                } else {
                    std::cerr << "Write error: " << ec.message() << std::endl;
                    close();
                }
            });
    }

private:
    boost::asio::ip::tcp::socket socket_;
    boost::asio::streambuf read_buffer_;
    std::deque<std::string> write_queue_;
    message_handler message_handler_;
    close_handler close_handler_;
    boost::asio::ip::tcp::endpoint remote_endpoint_;
    std::atomic<bool> connected_;
};

/**
 * TCP client class for establishing connections to a server
 */
class TcpClient {
public:
    using pointer = std::shared_ptr<TcpClient>;
    using connection_handler = std::function<void(TcpConnection::pointer)>;
    using error_handler = std::function<void(const std::string&)>;

    /**
     * Constructor
     * 
     * @param io_context The Boost.Asio io_context to use
     */
    TcpClient(boost::asio::io_context& io_context)
        : io_context_(io_context)
        , socket_(io_context)
        , resolver_(io_context) {}

    /**
     * Connect to a server
     * 
     * @param host The server hostname or IP address
     * @param port The server port number
     */
    void connect(const std::string& host, const std::string& port) {
        // Start the asynchronous resolve operation
        resolver_.async_resolve(
            host,
            port,
            [this](const boost::system::error_code& ec, boost::asio::ip::tcp::resolver::results_type endpoints) {
                if (!ec) {
                    connect_to_endpoints(endpoints);
                } else {
                    if (error_handler_) {
                        error_handler_("Resolve error: " + ec.message());
                    }
                }
            });
    }
    
    /**
     * Set connection handler
     * 
     * @param handler The handler to call when a connection is established
     */
    void set_connection_handler(connection_handler handler) {
        connection_handler_ = handler;
    }
    
    /**
     * Set error handler
     * 
     * @param handler The handler to call when an error occurs
     */
    void set_error_handler(error_handler handler) {
        error_handler_ = handler;
    }

private:
    // Connect to the resolved endpoints
    void connect_to_endpoints(const boost::asio::ip::tcp::resolver::results_type& endpoints) {
        // Start the asynchronous connect operation
        boost::asio::async_connect(
            socket_,
            endpoints,
            [this](const boost::system::error_code& ec, const boost::asio::ip::tcp::endpoint& /*endpoint*/) {
                if (!ec) {
                    // Create and start the connection
                    connection_ = TcpConnection::create(io_context_);
                    socket_.swap(connection_->socket());
                    connection_->start();
                    
                    // Notify about the established connection
                    if (connection_handler_) {
                        connection_handler_(connection_);
                    }
                } else {
                    if (error_handler_) {
                        error_handler_("Connect error: " + ec.message());
                    }
                }
            });
    }

private:
    boost::asio::io_context& io_context_;
    boost::asio::ip::tcp::socket socket_;
    boost::asio::ip::tcp::resolver resolver_;
    TcpConnection::pointer connection_;
    connection_handler connection_handler_;
    error_handler error_handler_;
};

/**
 * TCP server class for accepting incoming connections
 */
class TcpServer {
public:
    using pointer = std::shared_ptr<TcpServer>;
    using connection_handler = std::function<void(TcpConnection::pointer)>;
    using error_handler = std::function<void(const std::string&)>;

    /**
     * Constructor
     * 
     * @param io_context The Boost.Asio io_context to use
     * @param port The port to listen on
     */
    TcpServer(boost::asio::io_context& io_context, unsigned short port)
        : io_context_(io_context)
        , acceptor_(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
        , running_(false) {}

    /**
     * Start the server
     */
    void start() {
        if (running_) return;
        
        running_ = true;
        std::cout << "Server started on port " << acceptor_.local_endpoint().port() << std::endl;
        
        // Start accepting connections
        accept_connection();
    }
    
    /**
     * Stop the server
     */
    void stop() {
        if (!running_) return;
        
        std::cout << "Stopping server..." << std::endl;
        
        // Close the acceptor to stop accepting new connections
        boost::system::error_code ec;
        acceptor_.close(ec);
        
        // Close all active connections
        std::lock_guard<std::mutex> lock(connections_mutex_);
        for (auto& conn : active_connections_) {
            conn->close();
        }
        active_connections_.clear();
        
        running_ = false;
    }
    
    /**
     * Set connection handler
     * 
     * @param handler The handler to call when a new connection is accepted
     */
    void set_connection_handler(connection_handler handler) {
        connection_handler_ = handler;
    }
    
    /**
     * Set error handler
     * 
     * @param handler The handler to call when an error occurs
     */
    void set_error_handler(error_handler handler) {
        error_handler_ = handler;
    }
    
    /**
     * Get the number of active connections
     * 
     * @return The number of active connections
     */
    size_t connection_count() const {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        return active_connections_.size();
    }
    
    /**
     * Get the server port
     * 
     * @return The server port
     */
    unsigned short port() const {
        return acceptor_.local_endpoint().port();
    }
    
    /**
     * Check if the server is running
     * 
     * @return True if the server is running, false otherwise
     */
    bool is_running() const {
        return running_;
    }
    
private:
    // Accept a new connection
    void accept_connection() {
        // Create a new connection
        auto new_connection = TcpConnection::create(io_context_);
        
        // Set up an asynchronous accept operation
        acceptor_.async_accept(
            new_connection->socket(),
            [this, new_connection](const boost::system::error_code& ec) {
                if (!ec) {
                    // Set up a handler to remove the connection when it closes
                    new_connection->set_close_handler([this](TcpConnection::pointer conn) {
                        remove_connection(conn);
                    });
                    
                    // Start the connection
                    new_connection->start();
                    
                    // Add to active connections
                    {
                        std::lock_guard<std::mutex> lock(connections_mutex_);
                        active_connections_.insert(new_connection);
                    }
                    
                    // Notify about the new connection
                    if (connection_handler_) {
                        connection_handler_(new_connection);
                    }
                    
                    // Continue accepting connections
                    if (running_) {
                        accept_connection();
                    }
                } else if (ec != boost::asio::error::operation_aborted) {
                    if (error_handler_) {
                        error_handler_("Accept error: " + ec.message());
                    }
                    
                    // Try to recover and continue accepting
                    if (running_) {
                        accept_connection();
                    }
                }
            });
    }
    
    // Remove a closed connection from the active connections list
    void remove_connection(TcpConnection::pointer connection) {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        active_connections_.erase(connection);
    }
    
private:
    boost::asio::io_context& io_context_;
    boost::asio::ip::tcp::acceptor acceptor_;
    std::set<TcpConnection::pointer> active_connections_;
    mutable std::mutex connections_mutex_;
    connection_handler connection_handler_;
    error_handler error_handler_;
    std::atomic<bool> running_;
};

/**
 * Message channel that uses the underlying TCP connection for sending/receiving messages
 * and provides a higher-level API for using the protocol messages
 */
template<typename MessageType>
class MessageChannel : public std::enable_shared_from_this<MessageChannel<MessageType>> {
public:
    using pointer = std::shared_ptr<MessageChannel<MessageType>>;
    using message_handler = std::function<void(pointer, const MessageType&)>;
    
    /**
     * Constructor
     * 
     * @param connection The TCP connection to use
     */
    explicit MessageChannel(TcpConnection::pointer connection)
        : connection_(connection) {
        
        // Set up the message handler on the TCP connection
        connection_->set_message_handler([this](TcpConnection::pointer, const std::string& data) {
            handle_raw_message(data);
        });
    }
    
    /**
     * Send a message
     * 
     * @param message The message to send
     */
    void send_message(const MessageType& message) {
        if (!connection_->is_connected()) {
            std::cerr << "Cannot send message: Connection closed" << std::endl;
            return;
        }
        
        try {
            // Serialize the message to string and send
            std::string data = message.toString();
            connection_->write(data);
        } catch (const std::exception& e) {
            std::cerr << "Error serializing message: " << e.what() << std::endl;
        }
    }
    
    /**
     * Set message handler
     * 
     * @param handler The handler to call when a message is received
     */
    void set_message_handler(message_handler handler) {
        message_handler_ = handler;
    }
    
    /**
     * Get the underlying TCP connection
     * 
     * @return The TCP connection
     */
    TcpConnection::pointer get_connection() const {
        return connection_;
    }
    
    /**
     * Check if the channel is active
     * 
     * @return True if the channel is active, false otherwise
     */
    bool is_active() const {
        return connection_ && connection_->is_connected();
    }
    
    /**
     * Close the channel
     */
    void close() {
        if (connection_) {
            connection_->close();
        }
    }
    
private:
    // Handle a raw message from the TCP connection
    void handle_raw_message(const std::string& data) {
        try {
            // Parse the message and call the message handler
            auto message = MessageType::fromString(data);
            
            if (message_handler_) {
                message_handler_(this->shared_from_this(), message);
            }
        } catch (const std::exception& e) {
            std::cerr << "Error parsing message: " << e.what() << std::endl;
        }
    }
    
private:
    TcpConnection::pointer connection_;
    message_handler message_handler_;
};

} // namespace network
} // namespace collab

#endif // COLLABORATIVE_EDITOR_TCP_CONNECTION_H