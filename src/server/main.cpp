// src/server/main.cpp
#include <iostream>
#include <string>
#include <memory>
#include <csignal>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>

#include "server/server.h"

// Default port to listen on
constexpr unsigned short DEFAULT_PORT = 8080;
// Default thread pool size (0 = use hardware concurrency)
constexpr unsigned int DEFAULT_THREAD_POOL_SIZE = 0;
// Default session cleanup interval (in seconds)
constexpr int DEFAULT_SESSION_CLEANUP_INTERVAL = 300; // 5 minutes
// Default maximum session idle time before cleanup (in seconds)
constexpr int DEFAULT_MAX_SESSION_IDLE_TIME = 3600; // 1 hour

int main(int argc, char* argv[]) {
    unsigned short port = DEFAULT_PORT;
    unsigned int thread_pool_size = DEFAULT_THREAD_POOL_SIZE;
    int session_cleanup_interval = DEFAULT_SESSION_CLEANUP_INTERVAL;
    int max_session_idle_time = DEFAULT_MAX_SESSION_IDLE_TIME;
    
    // Parse command line options if provided
    try {
        namespace po = boost::program_options;
        po::options_description desc("Collaborative Text Editor Server");
        desc.add_options()
            ("help", "Display this help message")
            ("port,p", po::value<unsigned short>(&port)->default_value(DEFAULT_PORT), 
             "Port to listen on")
            ("threads,t", po::value<unsigned int>(&thread_pool_size)->default_value(DEFAULT_THREAD_POOL_SIZE),
             "Number of worker threads (0 = use hardware concurrency)")
            ("cleanup-interval,c", po::value<int>(&session_cleanup_interval)->default_value(DEFAULT_SESSION_CLEANUP_INTERVAL),
             "Session cleanup interval in seconds")
            ("max-idle,i", po::value<int>(&max_session_idle_time)->default_value(DEFAULT_MAX_SESSION_IDLE_TIME),
             "Maximum session idle time before cleanup in seconds");
        
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
        
        if (vm.count("help")) {
            std::cout << desc << "\n";
            return 0;
        }
    } 
    catch (const std::exception& e) {
        std::cerr << "Error parsing command line: " << e.what() << "\n";
        return 1;
    }
    
    // If thread pool size is 0, use hardware concurrency
    if (thread_pool_size == 0) {
        thread_pool_size = std::thread::hardware_concurrency();
        // Ensure at least 2 threads
        if (thread_pool_size < 2) {
            thread_pool_size = 2;
        }
    }
    
    try {
        // Initialize the io_context
        boost::asio::io_context io_context;
        
        // Create and start the server
        std::cout << "Starting Collaborative Text Editor Server...\n";
        collab::server::Server server(io_context, port, thread_pool_size, 
                                      session_cleanup_interval, max_session_idle_time);
        
        // Output the actual number of threads being used
        std::cout << "Server using " << server.getThreadPoolSize() << " worker threads\n";
        std::cout << "Session cleanup interval: " << session_cleanup_interval << " seconds\n";
        std::cout << "Maximum session idle time: " << max_session_idle_time << " seconds\n";
        
        // The io_context will run until the server shuts down due to SIGINT/SIGTERM
        io_context.run();
        
        std::cout << "Server stopped gracefully\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}