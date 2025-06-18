#ifndef COLLABORATIVE_EDITOR_CLIENT_MANAGER_H
#define COLLABORATIVE_EDITOR_CLIENT_MANAGER_H

#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <functional>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include "common/network/tcp_connection.h"
#include "common/protocol/protocol.h"

namespace collab {
namespace client {

/**
 * Client manager class to handle the network communication
 * with the server for the collaborative editor
 */
class ClientManager {
public:
    // Singleton instance
    static ClientManager& getInstance() {
        static ClientManager instance;
        return instance;
    }

    // Connection status callback
    using ConnectionStatusCallback = std::function<void(bool connected)>;
    
    // Message callback
    using MessageCallback = std::function<void(const protocol::Message&)>;

    // Connect to the server
    bool connect(const std::string& host, int port) {
        if (connected_) {
            return true;
        }

        try {
            // Create a new io_context
            io_context_ = std::make_unique<boost::asio::io_context>();
            
            // Create a TCP client
            client_ = std::make_shared<network::TcpClient>(*io_context_);
            
            // Set the connection handler
            client_->set_connection_handler([this](network::TcpConnection::pointer connection) {
                // Create a message channel
                channel_ = std::make_shared<network::MessageChannel<protocol::Message>>(connection);
                
                // Set the message handler
                channel_->set_message_handler([this](auto, const protocol::Message& message) {
                    handleMessage(message);
                });
                
                // Set connected flag
                connected_ = true;
                
                // Notify status change
                if (connectionStatusCallback_) {
                    connectionStatusCallback_(true);
                }
                
                // Send any pending messages
                sendPendingMessages();
            });
            
            // Set the error handler
            client_->set_error_handler([this](const std::string& error) {
                std::cerr << "Client error: " << error << std::endl;
                
                if (connected_) {
                    connected_ = false;
                    
                    if (connectionStatusCallback_) {
                        connectionStatusCallback_(false);
                    }
                }
            });
            
            // Connect to the server
            client_->connect(host, std::to_string(port));
            
            // Start the io_context in a separate thread
            io_thread_ = std::thread([this]() {
                io_context_->run();
            });
            
            // Wait for connection or timeout
            std::this_thread::sleep_for(std::chrono::seconds(2));
            
            return connected_;
        } catch (const std::exception& e) {
            std::cerr << "Error connecting to server: " << e.what() << std::endl;
            return false;
        }
    }
    
    // Disconnect from the server
    void disconnect() {
        if (!connected_) {
            return;
        }
        
        // Close the channel
        if (channel_) {
            channel_->close();
            channel_.reset();
        }
        
        // Stop the io_context
        if (io_context_) {
            io_context_->stop();
        }
        
        // Wait for the thread to finish
        if (io_thread_.joinable()) {
            io_thread_.join();
        }
        
        // Reset client
        client_.reset();
        
        // Set connected flag
        connected_ = false;
        
        // Notify status change
        if (connectionStatusCallback_) {
            connectionStatusCallback_(false);
        }
    }
    
    // Send a message to the server
    bool sendMessage(const protocol::Message& message) {
        if (!connected_) {
            // Queue message for when we connect
            std::lock_guard<std::mutex> lock(pendingMessagesMutex_);
            pendingMessages_.push(message);
            return false;
        }
        
        if (channel_) {
            channel_->send_message(message);
            return true;
        }
        
        return false;
    }
    
    // Register a connection status callback
    void setConnectionStatusCallback(ConnectionStatusCallback callback) {
        connectionStatusCallback_ = callback;
        
        // Immediately call with current status
        if (callback) {
            callback(connected_);
        }
    }
    
    // Register a message callback
    void setMessageCallback(MessageCallback callback) {
        messageCallback_ = callback;
    }
    
    // Check if connected
    bool isConnected() const {
        return connected_;
    }

private:
    // Private constructor for singleton
    ClientManager()
        : connected_(false) {}
    
    // Handle an incoming message
    void handleMessage(const protocol::Message& message) {
        if (messageCallback_) {
            messageCallback_(message);
        }
    }
    
    // Send any pending messages
    void sendPendingMessages() {
        std::lock_guard<std::mutex> lock(pendingMessagesMutex_);
        
        while (!pendingMessages_.empty() && connected_ && channel_) {
            channel_->send_message(pendingMessages_.front());
            pendingMessages_.pop();
        }
    }

private:
    std::unique_ptr<boost::asio::io_context> io_context_;
    std::shared_ptr<network::TcpClient> client_;
    std::shared_ptr<network::MessageChannel<protocol::Message>> channel_;
    std::thread io_thread_;
    std::atomic<bool> connected_;
    
    ConnectionStatusCallback connectionStatusCallback_;
    MessageCallback messageCallback_;
    
    std::queue<protocol::Message> pendingMessages_;
    std::mutex pendingMessagesMutex_;
};

} // namespace client
} // namespace collab

#endif // COLLABORATIVE_EDITOR_CLIENT_MANAGER_H