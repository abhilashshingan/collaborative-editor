#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <boost/asio.hpp>
#include "server/server.h"
#include "common/crdt/document.h"

namespace collab {
namespace server {

class CollaborativeServer {
public:
    CollaborativeServer(unsigned short port)
        : io_context_(),
          server_(io_context_, port),
          document_(std::make_shared<crdt::Document>()) {
        
        // Set up message handlers
        server_.setMessageHandler([this](const std::string& client_id, const std::string& message) {
            handleMessage(client_id, message);
        });
        
        // Set up connection handlers
        server_.setConnectionHandler([this](const std::string& client_id) {
            handleNewConnection(client_id);
        });
        
        server_.setDisconnectionHandler([this](const std::string& client_id) {
            handleDisconnection(client_id);
        });
    }
    
    void run() {
        try {
            std::cout << "Collaborative Text Editor Server is running...\n";
            io_context_.run();
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
        }
    }
    
private:
    void handleMessage(const std::string& client_id, const std::string& message) {
        if (message.substr(0, 6) == "UPDATE") {
            // Extract content part of the message
            std::string content = message.substr(7); // Skip "UPDATE "
            
            // Update the document
            {
                std::lock_guard<std::mutex> lock(document_mutex_);
                document_->updateContent(content);
            }
            
            // Broadcast to all other clients
            broadcastUpdate(client_id);
        }
    }
    
    void handleNewConnection(const std::string& client_id) {
        std::cout << "New client connected: " << client_id << "\n";
        
        // Send the current document state to the new client
        std::string content;
        {
            std::lock_guard<std::mutex> lock(document_mutex_);
            content = document_->getContent();
        }
        
        server_.sendMessage(client_id, "CONTENT " + content);
    }
    
    void handleDisconnection(const std::string& client_id) {
        std::cout << "Client disconnected: " << client_id << "\n";
    }
    
    void broadcastUpdate(const std::string& source_client_id) {
        std::string content;
        {
            std::lock_guard<std::mutex> lock(document_mutex_);
            content = document_->getContent();
        }
        
        // Send to all clients except the source
        server_.broadcastMessage("CONTENT " + content, source_client_id);
    }
    
    boost::asio::io_context io_context_;
    Server server_;
    std::shared_ptr<crdt::Document> document_;
    std::mutex document_mutex_;
};

} // namespace server
} // namespace collab

int main() {
    try {
        collab::server::CollaborativeServer server(8080);
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}