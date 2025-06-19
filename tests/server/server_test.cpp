// tests/server/server_test.cpp
#include <gtest/gtest.h>
#include <boost/asio.hpp>
#include <thread>
#include <chrono>
#include <future>
#include <set>
#include <mutex>
#include <atomic>
#include "server/server.h"

using namespace collab::server;

class ServerTest : public ::testing::Test {
protected:
    boost::asio::io_context io_context;
    std::unique_ptr<Server> server;
    
    void SetUp() override {
        io_context.restart();
    }
    
    void TearDown() override {
        if (server) {
            server->stop();
        }
        io_context.stop();
    }
    
    // Helper to run the io_context in a separate thread
    std::thread runIoContextInThread() {
        return std::thread([this]() { io_context.run(); });
    }
};

// Test that the server can be created and destroyed
TEST_F(ServerTest, CreateAndDestroy) {
    server = std::make_unique<Server>(io_context, 0); // Use port 0 for auto-assign
    EXPECT_TRUE(server->isRunning());
    server.reset();
}

// Test configurable thread pool size
TEST_F(ServerTest, ThreadPoolSize) {
    // Test with explicit thread pool size
    size_t threadPoolSize = 4;
    server = std::make_unique<Server>(io_context, 0, threadPoolSize); // Use port 0 for auto-assign
    
    EXPECT_EQ(server->getThreadPoolSize(), threadPoolSize);
    
    server.reset();
    
    // Test with default (hardware concurrency)
    server = std::make_unique<Server>(io_context, 0); // Use port 0 for auto-assign
    
    EXPECT_GE(server->getThreadPoolSize(), 1u); // Should be at least 1
    EXPECT_EQ(server->getThreadPoolSize(), std::thread::hardware_concurrency() > 0 
                                         ? std::thread::hardware_concurrency() 
                                         : 1u);
    
    server.reset();
}

// Test session handler integration
TEST_F(ServerTest, SessionHandlerIntegration) {
    // Test with session cleanup settings
    server = std::make_unique<Server>(io_context, 0, 2, 30, 60);
    
    // Initially no sessions
    EXPECT_EQ(server->getSessionCount(), 0);
    
    // Get session handler reference
    auto& sessionHandler = server->getSessionHandler();
    
    // We can't create sessions directly without connections in this test
    // but we can verify the session handler is initialized
    EXPECT_TRUE(sessionHandler.isUsernameAvailable("testuser"));
    
    server.reset();
}

// Test that the server can handle a simple connection
TEST_F(ServerTest, HandleConnection) {
    // Start the server on an auto-assigned port
    unsigned short port = 0;
    server = std::make_unique<Server>(io_context, port);
    
    // Get the actual port assigned
    boost::asio::ip::tcp::endpoint endpoint = server->getEndpoint();
    port = endpoint.port();
    
    // Run the io_context in a separate thread
    std::thread io_thread = runIoContextInThread();
    
    try {
        // Create a client socket
        boost::asio::ip::tcp::socket client_socket(io_context);
        boost::asio::ip::tcp::endpoint server_endpoint(
            boost::asio::ip::address::from_string("127.0.0.1"), port);
        
        // Connect to the server
        client_socket.connect(server_endpoint);
        EXPECT_TRUE(client_socket.is_open());
        
        // Send data to server
        std::string message = "Hello, Server!";
        boost::asio::write(client_socket, boost::asio::buffer(message));
        
        // Receive response from server
        std::vector<char> recv_buffer(1024);
        size_t len = client_socket.read_some(boost::asio::buffer(recv_buffer));
        std::string response(recv_buffer.begin(), recv_buffer.begin() + len);
        
        // Check the response contains our message
        EXPECT_TRUE(response.find("Hello, Server!") != std::string::npos);
        EXPECT_TRUE(response.find("Server received:") != std::string::npos);
        
        // After connection, the server should have one session
        EXPECT_EQ(server->getSessionCount(), 1);
        
        // Close the socket
        client_socket.close();
        
        // Give the server some time to clean up the session
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    catch (const std::exception& e) {
        FAIL() << "Exception occurred: " << e.what();
    }
    
    // Stop server and join thread
    server->stop();
    io_context.stop();
    if (io_thread.joinable()) {
        io_thread.join();
    }
}

// Test that multiple connections are handled by the thread pool
TEST_F(ServerTest, MultipleConcurrentConnections) {
    // Start the server with 4 threads
    unsigned short port = 0;
    size_t threadPoolSize = 4;
    server = std::make_unique<Server>(io_context, port, threadPoolSize);
    
    // Get the actual port assigned
    boost::asio::ip::tcp::endpoint endpoint = server->getEndpoint();
    port = endpoint.port();
    
    // Run the io_context in a separate thread
    std::thread io_thread = runIoContextInThread();
    
    // Set to collect thread IDs that process our requests
    std::set<std::thread::id> threadIds;
    std::mutex threadIdsMutex;
    
    // Number of messages each client will send
    const int messagesPerClient = 5;
    // Number of clients
    const int numClients = 10;
    
    try {
        std::vector<std::thread> clientThreads;
        std::atomic<int> clientsCompleted(0);
        
        for (int i = 0; i < numClients; ++i) {
            clientThreads.emplace_back([port, i, &threadIds, &threadIdsMutex, &clientsCompleted]() {
                try {
                    // Create a separate io_context for each client
                    boost::asio::io_context client_io_context;
                    
                    // Create a client socket
                    boost::asio::ip::tcp::socket client_socket(client_io_context);
                    boost::asio::ip::tcp::endpoint server_endpoint(
                        boost::asio::ip::address::from_string("127.0.0.1"), port);
                    
                    // Connect to the server
                    client_socket.connect(server_endpoint);
                    
                    for (int j = 0; j < messagesPerClient; ++j) {
                        // Send data to server
                        std::string message = "Client " + std::to_string(i) + 
                                             " Message " + std::to_string(j);
                        
                        boost::asio::write(client_socket, boost::asio::buffer(message));
                        
                        // Receive response from server
                        std::vector<char> recv_buffer(1024);
                        size_t len = client_socket.read_some(boost::asio::buffer(recv_buffer));
                        
                        std::string response(recv_buffer.begin(), recv_buffer.begin() + len);
                        
                        // Extract the thread ID that processed this message
                        size_t pos1 = response.find("thread ");
                        if (pos1 != std::string::npos) {
                            pos1 += 7; // Length of "thread "
                            std::string threadIdStr = response.substr(pos1);
                            std::thread::id threadId;
                            std::istringstream iss(threadIdStr);
                            iss >> threadId;
                            
                            // Add this thread ID to our set
                            std::lock_guard<std::mutex> lock(threadIdsMutex);
                            threadIds.insert(threadId);
                        }
                        
                        // Small delay to avoid overwhelming the server
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    }
                    
                    // Close the socket
                    client_socket.close();
                    
                    // Mark this client as completed
                    clientsCompleted++;
                }
                catch (const std::exception& e) {
                    std::cerr << "Client " << i << " exception: " << e.what() << std::endl;
                }
            });
        }
        
        // Wait for all clients to complete or timeout
        for (int i = 0; i < 50 && clientsCompleted < numClients; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Join all client threads
        for (auto& t : clientThreads) {
            if (t.joinable()) {
                t.join();
            }
        }
        
        // We should have seen at least 2 different thread IDs process our requests
        EXPECT_GE(threadIds.size(), 2u);
        
        // Ideally, we should see all thread pool threads being used
        // But this isn't guaranteed, so we just check that we got multiple threads
        std::cout << "Requests were processed by " << threadIds.size() 
                  << " different worker threads" << std::endl;
        
        // The server should have created sessions for all clients
        EXPECT_EQ(server->getSessionCount(), numClients);
    }
    catch (const std::exception& e) {
        FAIL() << "Exception occurred: " << e.what();
    }
    
    // Stop server and join thread
    server->stop();
    io_context.stop();
    if (io_thread.joinable()) {
        io_thread.join();
    }
}

// Test that the server handles SIGINT
TEST_F(ServerTest, GracefulShutdownOnSignal) {
    server = std::make_unique<Server>(io_context, 0);
    EXPECT_TRUE(server->isRunning());
    
    // Run io_context in a separate thread
    std::thread io_thread = runIoContextInThread();
    
    // Create a future to wait for the server to stop
    auto future = std::async(std::launch::async, [this]() {
        // Wait a bit for the server to fully start
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Simulate sending a signal to the server
        server->simulateSignal(SIGINT);
        
        // Wait for server to stop
        for (int i = 0; i < 50 && server->isRunning(); ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        return !server->isRunning();
    });
    
    // Wait for the future to complete
    bool result = future.get();
    EXPECT_TRUE(result) << "Server did not shut down gracefully on SIGINT";
    
    // Cleanup
    io_context.stop();
    if (io_thread.joinable()) {
        io_thread.join();
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}