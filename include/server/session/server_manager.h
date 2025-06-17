#ifndef COLLABORATIVE_EDITOR_SERVER_MANAGER_H
#define COLLABORATIVE_EDITOR_SERVER_MANAGER_H

#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>
#include <functional>
#include <boost/asio.hpp>

#include "common/network/tcp_connection.h"
#include "common/protocol/protocol.h"
#include "common/util/uuid_generator.h"

namespace collab {
namespace server {

/**
 * Class that manages the server side of the collaborative editor
 */
class ServerManager {
public:
    // Singleton instance
    static ServerManager& getInstance() {
        static ServerManager instance;
        return instance;
    }
    
    // Start the server
    bool start(int port = 12345) {
        if (running_) {
            return true;
        }
        
        try {
            // Create a new io_context
            io_context_ = std::make_unique<boost::asio::io_context>();
            
            // Create a TCP server
            server_ = std::make_shared<network::TcpServer>(*io_context_, port);
            
            // Set the connection handler
            server_->set_connection_handler([this](network::TcpConnection::pointer connection) {
                handleNewConnection(connection);
            });
            
            // Set the error handler
            server_->set_error_handler([](const std::string& error) {
                std::cerr << "Server error: " << error << std::endl;
            });
            
            // Start the server
            server_->start();
            
            // Start the io_context in a separate thread
            io_thread_ = std::thread([this]() {
                io_context_->run();
            });
            
            running_ = true;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Error starting server: " << e.what() << std::endl;
            return false;
        }
    }
    
    // Stop the server
    void stop() {
        if (!running_) {
            return;
        }
        
        // Stop the server
        if (server_) {
            server_->stop();
        }
        
        // Stop the io_context
        if (io_context_) {
            io_context_->stop();
        }
        
        // Wait for the thread to finish
        if (io_thread_.joinable()) {
            io_thread_.join();
        }
        
        // Clear all clients
        {
            std::lock_guard<std::mutex> lock(clientsMutex_);
            clients_.clear();
        }
        
        running_ = false;
    }
    
    // Send a message to a specific client
    bool sendMessage(const std::string& clientId, const protocol::Message& message) {
        std::lock_guard<std::mutex> lock(clientsMutex_);
        
        auto it = clients_.find(clientId);
        if (it != clients_.end()) {
            it->second->send_message(message);
            return true;
        }
        
        return false;
    }
    
    // Broadcast a message to all clients
    void broadcastMessage(const protocol::Message& message) {
        std::lock_guard<std::mutex> lock(clientsMutex_);
        
        for (auto& client : clients_) {
            client.second->send_message(message);
        }
    }
    
    // Set a function to handle incoming messages
    using MessageHandler = std::function<void(const std::string& clientId, const protocol::Message& message)>;
    void setMessageHandler(MessageHandler handler) {
        messageHandler_ = handler;
    }
    
    // Check if the server is running
    bool isRunning() const {
        return running_;
    }
    
    // Get the number of connected clients
    size_t getClientCount() const {
        std::lock_guard<std::mutex> lock(clientsMutex_);
        return clients_.size();
    }

private:
    // Private constructor for singleton
    ServerManager()
        : running_(false) {}
    
    // Handle a new client connection
    void handleNewConnection(network::TcpConnection::pointer connection) {
        // Generate a client ID
        std::string clientId = util::UuidGenerator::getInstance().generateUuid();
        
        // Create a message channel
        auto channel = std::make_shared<network::MessageChannel<protocol::Message>>(connection);
        
        // Set the message handler
        channel->set_message_handler([this, clientId](auto, const protocol::Message& message) {
            // Call the message handler
            if (messageHandler_) {
                messageHandler_(clientId, message);
            }
        });
        
        // Add the client to the list
        {
            std::lock_guard<std::mutex> lock(clientsMutex_);
            clients_[clientId] = channel;
        }
        
        // Set up a handler to remove the client when the connection is closed
        connection->set_close_handler([this, clientId](auto) {
            std::lock_guard<std::mutex> lock(clientsMutex_);
            clients_.erase(clientId);
        });
    }

private:
    std::unique_ptr<boost::asio::io_context> io_context_;
    std::shared_ptr<network::TcpServer> server_;
    std::thread io_thread_;
    std::atomic<bool> running_;
    
    std::unordered_map<std::string, std::shared_ptr<network::MessageChannel<protocol::Message>>> clients_;
    mutable std::mutex clientsMutex_;
    
    MessageHandler messageHandler_;
};

} // namespace server
} // namespace collab

#endif // COLLABORATIVE_EDITOR_SERVER_MANAGER_H