#include "common/util/logger.h"
#include <iostream>
#include <thread>
#include <vector>

using namespace collab::util;

// Function to demonstrate multi-threaded logging
void workerThread(int id) {
    auto& logger = getLogger();
    
    for (int i = 0; i < 5; ++i) {
        std::stringstream ss;
        ss << "Worker thread " << id << " - Log entry " << i;
        logger.info(ss.str());
        
        // Random delay
        std::this_thread::sleep_for(std::chrono::milliseconds(id * 10));
    }
}

int main() {
    // Create the logs directory if it doesn't exist
    std::filesystem::create_directories("logs");

    // Initialize the logger
    if (!initLogger("logs/application.log", LogLevel::DEBUG, true)) {
        std::cerr << "Failed to initialize logger" << std::endl;
        return 1;
    }
    
    auto& logger = getLogger();
    
    // Basic logging
    logger.info("Application started");
    logger.debug("Debug message");
    
    // Test all log levels
    logger.trace("This is a trace message");
    logger.debug("This is a debug message");
    logger.info("This is an info message");
    logger.warning("This is a warning message");
    logger.error("This is an error message");
    logger.fatal("This is a fatal message");
    
    // Log with stream operator
    LOG_INFO << "Stream logging example: " << 42 << " is the answer";
    
    // Demonstrate multi-threaded logging
    std::cout << "Starting worker threads..." << std::endl;
    std::vector<std::thread> threads;
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back(workerThread, i);
    }
    
    // Wait for threads to complete
    for (auto& t : threads) {
        t.join();
    }
    
    // Generate some log entries to demonstrate rotation
    logger.info("Generating log data to demonstrate rotation...");
    for (int i = 0; i < 1000; ++i) {
        std::string data(100, 'x');
        LOG_DEBUG << "Log entry " << i << ": " << data;
        
        // Add a small delay to make the process visible
        if (i % 100 == 0) {
            std::cout << "Generated " << i << " log entries..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    
    logger.info("Application shutdown");
    return 0;
}