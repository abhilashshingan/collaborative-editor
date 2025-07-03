// FILE: src/server/main.cpp
// Description: Main entry point for server application

#include <iostream>
#include <string>
#include <map>
#include <memory>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include "common/document/document_controller.h"
#include "common/document/operation_manager.h"
#include "common/ot/operation.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

// A simple WebSocket server that handles collaborative editing operations
class CollaborativeEditingServer {
public:
    CollaborativeEditingServer(net::io_context& ioc, uint16_t port)
        : acceptor_(ioc, {tcp::v4(), port}),
          socket_(ioc),
          documentController_(std::make_shared<collab::DocumentController>()),
          operationManager_(std::make_shared<collab::OperationManager>()) {
        
        // Start accepting connections
        doAccept();
        
        std::cout << "Server running on port " << port << std::endl;
    }
    
private:
    tcp::acceptor acceptor_;
    tcp::socket socket_;
    std::shared_ptr<collab::DocumentController> documentController_;
    std::shared_ptr<collab::OperationManager> operationManager_;
    std::map<std::string, std::shared_ptr<websocket::stream<tcp::socket>>> clients_;
    
    void doAccept() {
        acceptor_.async_accept(socket_,
            [this](boost::system::error_code ec) {
                if (!ec) {
                    // Create a new WebSocket session
                    std::cout << "New client connected" << std::endl;
                    
                    // Create WebSocket from the raw socket
                    auto ws = std::make_shared<websocket::stream<tcp::socket>>(std::move(socket_));
                    
                    // Accept the WebSocket handshake
                    ws->async_accept(
                        [this, ws](boost::system::error_code ec) {
                            if (!ec) {
                                // Generate a client ID
                                std::string clientId = "client_" + std::to_string(clients_.size() + 1);
                                
                                // Store the client
                                clients_[clientId] = ws;
                                
                                // Start reading from this client
                                doRead(clientId, ws);
                            }
                        });
                }
                
                // Continue accepting connections
                doAccept();
            });
    }
    
    void doRead(const std::string& clientId, std::shared_ptr<websocket::stream<tcp::socket>> ws) {
        // Create a buffer for reading
        auto buffer = std::make_shared<beast::flat_buffer>();
        
        // Read a message into our buffer
        ws->async_read(*buffer,
            [this, clientId, ws, buffer](boost::system::error_code ec, std::size_t bytes) {
                if (!ec) {
                    // Convert the buffer to a string
                    std::string message(boost::asio::buffer_cast<const char*>(buffer->data()), buffer->size());
                    
                    // Process the message
                    processMessage(clientId, message);
                    
                    // Continue reading
                    doRead(clientId, ws);
                } else {
                    // Handle disconnect
                    std::cout << "Client " << clientId << " disconnected" << std::endl;
                    clients_.erase(clientId);
                }
            });
    }
    
    void processMessage(const std::string& clientId, const std::string& message) {
        // In a real implementation, you would parse the message and extract:
        // 1. Operation data
        // 2. Base revision
        
        // For this example, we'll assume the message is the serialized operation
        try {
            // Parse the operation
            auto op = collab::ot::OperationFactory::deserialize(message);
            
            // Base revision would be included in the message
            int64_t baseRevision = documentController_->getRevision();
            
            // Process operation through the operation manager
            auto transformedOp = operationManager_->processOperation(op, clientId, baseRevision);
            
            // Apply the transformed operation to the document
            if (transformedOp && documentController_->applyOperation(transformedOp, clientId)) {
                // Record the operation
                operationManager_->recordOperation(transformedOp);
                
                // Broadcast to all other clients
                broadcastOperation(clientId, transformedOp);
            }
        } catch (const std::exception& e) {
            std::cerr << "Error processing message: " << e.what() << std::endl;
        }
    }
    
    void broadcastOperation(const std::string& sourceClientId, const collab::ot::OperationPtr& op) {
        // Serialize the operation
        std::string message = op->serialize();
        
        // Send to all clients except the source
        for (const auto& [clientId, ws] : clients_) {
            if (clientId != sourceClientId) {
                try {
                    ws->write(net::buffer(message));
                } catch (const std::exception& e) {
                    std::cerr << "Error sending to client " << clientId << ": " << e.what() << std::endl;
                }
            }
        }
    }
};

// Main entry point for the server
int main() {
    try {
        // Create an I/O context
        net::io_context ioc{1};
        
        // Create and run the server
        CollaborativeEditingServer server(ioc, 9002);
        
        // Run the I/O service
        ioc.run();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}