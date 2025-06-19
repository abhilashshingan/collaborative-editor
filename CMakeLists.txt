cmake_minimum_required(VERSION 3.16)
project(CollaborativeEditor VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(BUILD_SERVER "Build the server" ON)
option(BUILD_CLIENT "Build the client" ON)
option(BUILD_TESTS "Build the tests" ON)
option(ENABLE_COVERAGE "Enable coverage reporting" OFF)

# Display options status
message(STATUS "Server build: ${BUILD_SERVER}")
message(STATUS "Client build: ${BUILD_CLIENT}")
message(STATUS "Tests build: ${BUILD_TESTS}")

# Find required packages
find_package(Boost 1.71 REQUIRED COMPONENTS system program_options)
find_package(Threads REQUIRED)
find_package(nlohmann_json REQUIRED)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
)

# Create source directories if they don't exist
file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src/common")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src/server")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src/client")

# Check for common placeholder file
set(COMMON_PLACEHOLDER "${CMAKE_CURRENT_SOURCE_DIR}/src/common/common_placeholder.cpp")
if(NOT EXISTS "${COMMON_PLACEHOLDER}")
    # Create a placeholder file
    file(WRITE "${COMMON_PLACEHOLDER}" 
"#include <iostream>

namespace collab {
namespace common {

// This is a placeholder file until the actual common library components are implemented
void placeholder_function() {
    std::cout << \"Common library placeholder function called\" << std::endl;
}

} // namespace common
} // namespace collab
")
    message(STATUS "Created common placeholder file: ${COMMON_PLACEHOLDER}")
endif()

# Common library (shared between client and server)
add_library(common STATIC
    # Use the placeholder file for now
    "${CMAKE_CURRENT_SOURCE_DIR}/src/common/common_placeholder.cpp"
)

target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(common PUBLIC
    ${Boost_LIBRARIES}
    Threads::Threads
    nlohmann_json::nlohmann_json
)

# Check for server.cpp
set(SERVER_IMPL "${CMAKE_CURRENT_SOURCE_DIR}/src/server/server.cpp")
if(NOT EXISTS "${SERVER_IMPL}")
    # Create a placeholder file
    file(WRITE "${SERVER_IMPL}" 
"// src/server/server.cpp
#include \"server/server.h\"

namespace collab {
namespace server {

// Note: Most of the implementation is already in the header file as inline methods
// This file is primarily for any non-inline implementations that might be added later

} // namespace server
} // namespace collab
")
    message(STATUS "Created server implementation file: ${SERVER_IMPL}")
endif()

# Check for session_handler.cpp
set(SESSION_HANDLER_IMPL "${CMAKE_CURRENT_SOURCE_DIR}/src/server/session_handler.cpp")
if(NOT EXISTS "${SESSION_HANDLER_IMPL}")
    # Create a placeholder file
    file(WRITE "${SESSION_HANDLER_IMPL}" 
"// src/server/session_handler.cpp
#include \"server/session_handler.h\"

namespace collab {
namespace server {

// Most of the implementation is in the header as inline methods
// This file is for any non-inline implementations that might be added later

} // namespace server
} // namespace collab
")
    message(STATUS "Created session handler implementation file: ${SESSION_HANDLER_IMPL}")
endif()

# Server component
if(BUILD_SERVER)
    add_library(server STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/server/server.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/server/session_handler.cpp
    )
    
    target_link_libraries(server PUBLIC
        common
        ${Boost_LIBRARIES}
    )
    
    # Check for main.cpp
    set(SERVER_MAIN "${CMAKE_CURRENT_SOURCE_DIR}/src/server/main.cpp")
    if(NOT EXISTS "${SERVER_MAIN}")
        # Create a placeholder file
        file(WRITE "${SERVER_MAIN}" 
"// src/server/main.cpp
#include <iostream>
#include <string>
#include <memory>
#include <csignal>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>

#include \"server/server.h\"

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
        po::options_description desc(\"Collaborative Text Editor Server\");
        desc.add_options()
            (\"help\", \"Display this help message\")
            (\"port,p\", po::value<unsigned short>(&port)->default_value(DEFAULT_PORT), 
             \"Port to listen on\")
            (\"threads,t\", po::value<unsigned int>(&thread_pool_size)->default_value(DEFAULT_THREAD_POOL_SIZE),
             \"Number of worker threads (0 = use hardware concurrency)\")
            (\"cleanup-interval,c\", po::value<int>(&session_cleanup_interval)->default_value(DEFAULT_SESSION_CLEANUP_INTERVAL),
             \"Session cleanup interval in seconds\")
            (\"max-idle,i\", po::value<int>(&max_session_idle_time)->default_value(DEFAULT_MAX_SESSION_IDLE_TIME),
             \"Maximum session idle time before cleanup in seconds\");
        
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
        
        if (vm.count(\"help\")) {
            std::cout << desc << \"\\n\";
            return 0;
        }
    } 
    catch (const std::exception& e) {
        std::cerr << \"Error parsing command line: \" << e.what() << \"\\n\";
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
        std::cout << \"Starting Collaborative Text Editor Server...\\n\";
        collab::server::Server server(io_context, port, thread_pool_size, 
                                      session_cleanup_interval, max_session_idle_time);
        
        // Output the actual number of threads being used
        std::cout << \"Server using \" << server.getThreadPoolSize() << \" worker threads\\n\";
        std::cout << \"Session cleanup interval: \" << session_cleanup_interval << \" seconds\\n\";
        std::cout << \"Maximum session idle time: \" << max_session_idle_time << \" seconds\\n\";
        
        // The io_context will run until the server shuts down due to SIGINT/SIGTERM
        io_context.run();
        
        std::cout << \"Server stopped gracefully\\n\";
    }
    catch (const std::exception& e) {
        std::cerr << \"Fatal error: \" << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
")
        message(STATUS "Created server main file: ${SERVER_MAIN}")
    endif()
    
    # Server executable
    add_executable(server_app
        ${CMAKE_CURRENT_SOURCE_DIR}/src/server/main.cpp
    )
    
    target_link_libraries(server_app PRIVATE
        server
        common
        ${Boost_LIBRARIES}
    )
    
    set_target_properties(server_app PROPERTIES
        OUTPUT_NAME "server"
    )
endif()

# If Qt client is enabled, build the client
if(BUILD_CLIENT)
    # Make Qt finding optional
    find_package(Qt6 COMPONENTS Core Widgets QUIET)
    if(NOT Qt6_FOUND)
        find_package(Qt5 COMPONENTS Core Widgets QUIET)
        if(NOT Qt5_FOUND)
            message(WARNING "Qt not found. Client build will be disabled.")
            set(BUILD_CLIENT OFF)
        endif()
    endif()
endif()

if(BUILD_CLIENT)
    # Check for client placeholder file
    set(CLIENT_PLACEHOLDER "${CMAKE_CURRENT_SOURCE_DIR}/src/client/client_placeholder.cpp")
    if(NOT EXISTS "${CLIENT_PLACEHOLDER}")
        # Create a placeholder file
        file(WRITE "${CLIENT_PLACEHOLDER}" 
"#include <iostream>

namespace collab {
namespace client {

// This is a placeholder file until the actual client library components are implemented
void placeholder_function() {
    std::cout << \"Client library placeholder function called\" << std::endl;
}

} // namespace client
} // namespace collab
")
        message(STATUS "Created client placeholder file: ${CLIENT_PLACEHOLDER}")
    endif()

    add_library(client STATIC
        "${CLIENT_PLACEHOLDER}"
    )
    
    if(Qt6_FOUND)
        target_link_libraries(client PUBLIC
            common
            ${Boost_LIBRARIES}
            Qt6::Core
            Qt6::Widgets
        )
    else()
        target_link_libraries(client PUBLIC
            common
            ${Boost_LIBRARIES}
            Qt5::Core
            Qt5::Widgets
        )
    endif()
    
    # Check for client main.cpp
    set(CLIENT_MAIN "${CMAKE_CURRENT_SOURCE_DIR}/src/client/main.cpp")
    if(NOT EXISTS "${CLIENT_MAIN}")
        # Create a placeholder file
        file(WRITE "${CLIENT_MAIN}" 
"#include <iostream>
#ifdef QT_VERSION
#include <QApplication>
#include <QLabel>
#endif

int main(int argc, char *argv[]) {
#ifdef QT_VERSION
    QApplication app(argc, argv);
    
    QLabel label(\"Collaborative Editor (Client)\");
    label.setMinimumSize(400, 200);
    label.setAlignment(Qt::AlignCenter);
    label.show();
    
    std::cout << \"Collaborative Editor Client started\\n\";
    
    return app.exec();
#else
    std::cout << \"Collaborative Editor Client (console)\\n\";
    std::cout << \"Qt not available, running in console mode\\n\";
    
    return 0;
#endif
}
")
        message(STATUS "Created client main file: ${CLIENT_MAIN}")
    endif()
    
    # Client executable
    add_executable(client_app
        ${CMAKE_CURRENT_SOURCE_DIR}/src/client/main.cpp
    )
    
    if(Qt6_FOUND)
        target_link_libraries(client_app PRIVATE
            client
            common
            ${Boost_LIBRARIES}
            Qt6::Core
            Qt6::Widgets
        )
    else()
        target_link_libraries(client_app PRIVATE
            client
            common
            ${Boost_LIBRARIES}
            Qt5::Core
            Qt5::Widgets
        )
    endif()
    
    set_target_properties(client_app PROPERTIES
        OUTPUT_NAME "client"
    )
endif()

# Testing
if(BUILD_TESTS)
    # Make GTest finding optional
    find_package(GTest QUIET)
    if(NOT GTest_FOUND)
        message(WARNING "GTest not found. Tests will be disabled.")
        set(BUILD_TESTS OFF)
    else()
        enable_testing()
        
        # Create test directories
        file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests")
        file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests/common")
        file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests/server")
        file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests/client")
        file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests/utils")
        
        # Check for test helper
        set(TEST_HELPER "${CMAKE_CURRENT_SOURCE_DIR}/tests/utils/test_helpers.cpp")
        if(NOT EXISTS "${TEST_HELPER}")
            # Create a placeholder file
            file(WRITE "${TEST_HELPER}" 
"#include <string>
#include <iostream>

namespace collab {
namespace test {

// Placeholder for test utilities

} // namespace test
} // namespace collab
")
            message(STATUS "Created test helper file: ${TEST_HELPER}")
        endif()
        
        # Check for dummy test
        set(DUMMY_TEST "${CMAKE_CURRENT_SOURCE_DIR}/tests/common/dummy_test.cpp")
        if(NOT EXISTS "${DUMMY_TEST}")
            # Create a placeholder file
            file(WRITE "${DUMMY_TEST}" 
"#include <gtest/gtest.h>

// A simple test to make sure the testing framework works
TEST(DummyTest, VerifyTestingWorks) {
    EXPECT_TRUE(true);
}
")
            message(STATUS "Created dummy test file: ${DUMMY_TEST}")
        endif()
        
        # Check for session handler test
        set(SESSION_HANDLER_TEST "${CMAKE_CURRENT_SOURCE_DIR}/tests/server/session_handler_test.cpp")
        if(NOT EXISTS "${SESSION_HANDLER_TEST}")
            # Create a placeholder file
            file(WRITE "${SESSION_HANDLER_TEST}" 
"// tests/server/session_handler_test.cpp
#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <boost/asio.hpp>
#include \"server/session_handler.h\"

using namespace collab::server;

class SessionHandlerTest : public ::testing::Test {
protected:
    boost::asio::io_context io_context;
    
    // Helper to create a socket
    std::shared_ptr<boost::asio::ip::tcp::socket> createSocket() {
        return std::make_shared<boost::asio::ip::tcp::socket>(io_context);
    }
};

// Test the SocketGuard RAII behavior
TEST_F(SessionHandlerTest, SocketGuardRAII) {
    auto socket = createSocket();
    
    // Create a scope for the SocketGuard
    {
        SocketGuard guard(socket);
        EXPECT_TRUE(guard.isValid());
    }
    
    // After the scope ends, the socket should be closed
    EXPECT_FALSE(socket->is_open());
}

// Test user session state tracking
TEST_F(SessionHandlerTest, UserSessionState) {
    UserSession session(\"test-session\", \"\");
    
    // Initial state should be CONNECTING
    EXPECT_EQ(session.getState(), UserSession::State::CONNECTING);
    
    // Set state to AUTHENTICATING
    session.setState(UserSession::State::AUTHENTICATING);
    EXPECT_EQ(session.getState(), UserSession::State::AUTHENTICATING);
    
    // Set username and state to AUTHENTICATED
    session.setUsername(\"testuser\");
    session.setState(UserSession::State::AUTHENTICATED);
    
    EXPECT_EQ(session.getUsername(), \"testuser\");
    EXPECT_EQ(session.getState(), UserSession::State::AUTHENTICATED);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
")
            message(STATUS "Created session handler test file: ${SESSION_HANDLER_TEST}")
        endif()
        
        # Check for server test
        set(SERVER_TEST "${CMAKE_CURRENT_SOURCE_DIR}/tests/server/server_test.cpp")
        if(NOT EXISTS "${SERVER_TEST}")
            # Create a placeholder file
            file(WRITE "${SERVER_TEST}" 
"// tests/server/server_test.cpp
#include <gtest/gtest.h>
#include \"server/server.h\"

// A simple placeholder test for the server module
TEST(ServerTest, DummyTest) {
    EXPECT_TRUE(true);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
")
            message(STATUS "Created server test file: ${SERVER_TEST}")
        endif()
        
        # Check for dummy client test
        set(DUMMY_CLIENT_TEST "${CMAKE_CURRENT_SOURCE_DIR}/tests/client/dummy_client_test.cpp")
        if(NOT EXISTS "${DUMMY_CLIENT_TEST}")
            # Create a placeholder file
            file(WRITE "${DUMMY_CLIENT_TEST}" 
"#include <gtest/gtest.h>

// A placeholder test for the client module
TEST(ClientTest, DummyTest) {
    EXPECT_TRUE(true);
}
")
            message(STATUS "Created dummy client test file: ${DUMMY_CLIENT_TEST}")
        endif()
        
        add_subdirectory(tests)
    endif()
endif()

# Install targets
install(TARGETS common
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

if(BUILD_SERVER)
    install(TARGETS server server_app
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
    )
endif()

if(BUILD_CLIENT)
    install(TARGETS client client_app
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
    )
endif()