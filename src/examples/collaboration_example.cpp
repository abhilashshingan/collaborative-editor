/**
 * Example showing how to use the TCP communication channel with protocol messages
 */
#include <iostream>
#include <string>
#include <thread>
#include <memory>
#include <boost/asio.hpp>

#include "common/protocol/protocol.h"
#include "common/network/tcp_connection.h"

using namespace collab;

namespace collab {
namespace examples {

// Client-side example
void run_client(const std::string& host, const std::string& port) {
    try {
        // Create io_context
        boost::asio::io_context io_context;
        
        // Create a client
        network::TcpClient client(io_context);
        
        // Set up connection handler
        client.set_connection_handler([&io_context](network::TcpConnection::pointer connection) {
            std::cout << "Connected to server!" << std::endl;
            
            // Create a message channel using the connection
            auto channel = std::make_shared<network::MessageChannel<protocol::Message>>(connection);
            
            // Set up message handler
            channel->set_message_handler([](std::shared_ptr<network::MessageChannel<protocol::Message>> channel, 
                                         const protocol::Message& message) {
                std::cout << "Received message: " << message.toString() << std::endl;
                
                // Process different message types
                if (message.type == protocol::MessageType::AUTH_SUCCESS) {
                    std::cout << "Authentication successful!" << std::endl;
                    
                    // Send a document open request
                    protocol::DocumentMessage docOpenMsg(protocol::MessageType::DOC_OPEN);
                    docOpenMsg.documentId = "doc123";
                    docOpenMsg.documentName = "example.txt";
                    channel->send_message(docOpenMsg);
                }
                else if (message.type == protocol::MessageType::DOC_RESPONSE) {
                    // Parse as appropriate message type using std::visit
                    auto docMsg = std::visit([](auto&& msg) -> protocol::DocumentMessage {
                        if constexpr (std::is_same_v<std::decay_t<decltype(msg)>, protocol::DocumentMessage>) {
                            return msg;
                        } else {
                            throw std::runtime_error("Not a DocumentMessage");
                        }
                    }, protocol::Message::fromString(message.toString()));
                    
                    std::cout << "Document response received: " << 
                        (docMsg.success.value_or(false) ? "Success" : "Failure") << std::endl;
                    
                    if (docMsg.documentContent.has_value()) {
                        std::cout << "Document content: " << *docMsg.documentContent << std::endl;
                        
                        // Send an edit message to insert text
                        protocol::EditMessage editMsg(protocol::MessageType::EDIT_INSERT);
                        editMsg.documentId = docMsg.documentId;
                        editMsg.documentVersion = docMsg.documentVersion.value_or(0);
                        editMsg.operationId = "op_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
                        editMsg.position = 0;  // Insert at beginning
                        editMsg.text = "Hello, world!";
                        channel->send_message(editMsg);
                    }
                }
                else if (message.type == protocol::MessageType::EDIT_APPLY) {
                    std::cout << "Edit operation applied by server" << std::endl;
                }
            });
            
            // Send authentication message
            protocol::AuthMessage authMsg(protocol::MessageType::AUTH_LOGIN);
            authMsg.username = "testuser";
            authMsg.password = "password";
            channel->send_message(authMsg);
        });
        
        // Set up error handler
        client.set_error_handler([](const std::string& error) {
            std::cerr << "Client error: " << error << std::endl;
        });
        
        // Connect to server
        std::cout << "Connecting to " << host << ":" << port << "..." << std::endl;
        client.connect(host, port);
        
        // Run the io_context
        io_context.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}

// Server-side example
void run_server(unsigned short port) {
    try {
        // Create io_context
        boost::asio::io_context io_context;
        
        // Create server
        network::TcpServer server(io_context, port);
        
        // Set up connection handler
        server.set_connection_handler([](network::TcpConnection::pointer connection) {
            std::cout << "New client connected: " << connection->get_remote_address() << ":" 
                     << connection->get_remote_port() << std::endl;
            
            // Create a message channel
            auto channel = std::make_shared<network::MessageChannel<protocol::Message>>(connection);
            
            // Set up message handler
            channel->set_message_handler([](std::shared_ptr<network::MessageChannel<protocol::Message>> channel, 
                                         const protocol::Message& message) {
                std::cout << "Received message from client: " << message.toString() << std::endl;
                
                // Process different message types
                if (message.type == protocol::MessageType::AUTH_LOGIN) {
                    // Parse as appropriate message type using std::visit
                    auto authMsg = std::visit([](auto&& msg) -> protocol::AuthMessage {
                        if constexpr (std::is_same_v<std::decay_t<decltype(msg)>, protocol::AuthMessage>) {
                            return msg;
                        } else {
                            throw std::runtime_error("Not an AuthMessage");
                        }
                    }, protocol::Message::fromString(message.toString()));
                    
                    std::cout << "Login request from user: " << authMsg.username << std::endl;
                    
                    // Send authentication success
                    protocol::AuthMessage response(protocol::MessageType::AUTH_SUCCESS);
                    response.username = authMsg.username;
                    response.token = "session_token_123";
                    channel->send_message(response);
                }
                else if (message.type == protocol::MessageType::DOC_OPEN) {
                    // Parse as document message
                    auto docMsg = std::visit([](auto&& msg) -> protocol::DocumentMessage {
                        if constexpr (std::is_same_v<std::decay_t<decltype(msg)>, protocol::DocumentMessage>) {
                            return msg;
                        } else {
                            throw std::runtime_error("Not a DocumentMessage");
                        }
                    }, protocol::Message::fromString(message.toString()));
                    
                    std::cout << "Document open request for: " << *docMsg.documentName << std::endl;
                    
                    // Send document response with content
                    protocol::DocumentMessage response(protocol::MessageType::DOC_RESPONSE);
                    response.documentId = docMsg.documentId;
                    response.documentName = *docMsg.documentName;
                    response.documentContent = "This is the document content.";
                    response.documentVersion = 1;
                    response.success = true;
                    channel->send_message(response);
                }
                else if (message.type == protocol::MessageType::EDIT_INSERT ||
                         message.type == protocol::MessageType::EDIT_DELETE || 
                         message.type == protocol::MessageType::EDIT_REPLACE) {
                    
                    // Parse as edit message
                    auto editMsg = std::visit([](auto&& msg) -> protocol::EditMessage {
                        if constexpr (std::is_same_v<std::decay_t<decltype(msg)>, protocol::EditMessage>) {
                            return msg;
                        } else {
                            throw std::runtime_error("Not an EditMessage");
                        }
                    }, protocol::Message::fromString(message.toString()));
                    
                    std::cout << "Edit operation received for document: " << editMsg.documentId << std::endl;
                    
                    if (message.type == protocol::MessageType::EDIT_INSERT && editMsg.text.has_value()) {
                        std::cout << "Insert operation: \"" << *editMsg.text << "\" at position " 
                                 << *editMsg.position << std::endl;
                    }
                        
                    // Send edit application confirmation
                    protocol::EditMessage response(protocol::MessageType::EDIT_APPLY);
                    response.documentId = editMsg.documentId;
                    response.documentVersion = editMsg.documentVersion + 1;
                    response.operationId = editMsg.operationId;
                    response.success = true;
                    channel->send_message(response);
                }
            });
        });
        
        // Set up error handler
        server.set_error_handler([](const std::string& error) {
            std::cerr << "Server error: " << error << std::endl;
        });
        
        // Start the server
        std::cout << "Starting server on port " << port << "..." << std::endl;
        server.start();
        
        // Run the io_context
        io_context.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}

/**
 * Main entry point for example application
 */
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <client|server> [host] [port]\n";
        return 1;
    }
    
    std::string mode = argv[1];
    
    if (mode == "client") {
        if (argc < 4) {
            std::cerr << "Client mode requires host and port.\n";
            return 1;
        }
        run_client(argv[2], argv[3]);
    }
    else if (mode == "server") {
        unsigned short port = 12345;
        if (argc >= 3) {
            port = static_cast<unsigned short>(std::stoi(argv[2]));
        }
        run_server(port);
    }
    else {
        std::cerr << "Unknown mode. Use 'client' or 'server'.\n";
        return 1;
    }
    
    return 0;
}

} // namespace examples
} // namespace collab

// Main function to run the example
int main(int argc, char* argv[]) {
    return collab::examples::main(argc, argv);
}