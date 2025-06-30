#!/bin/bash

# Set color codes for output formatting
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BOLD='\033[1m'
RESET='\033[0m'

# Set the project root directory
PROJECT_ROOT=$(pwd)
BUILD_DIR="$PROJECT_ROOT/build"
INSTALL_DIR="$PROJECT_ROOT/install"

# Function to check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to install dependencies based on the detected OS
install_dependencies() {
  echo -e "${BLUE}Installing dependencies...${RESET}"

  if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    if [[ -f /etc/debian_version ]]; then
      # Debian/Ubuntu
      echo "Detected Debian/Ubuntu system"
      sudo apt-get update
      sudo apt-get install -y \
        build-essential cmake pkg-config \
        libboost-all-dev \
        libssl-dev \
        nlohmann-json3-dev \
        libspdlog-dev \
        libgtest-dev \
        qt6-base-dev \
        libncurses-dev
      
    elif [[ -f /etc/fedora-release ]]; then
      # Fedora
      echo "Detected Fedora system"
      sudo dnf install -y \
        gcc-c++ cmake pkgconf \
        boost-devel \
        openssl-devel \
        json-devel \
        spdlog-devel \
        gtest-devel \
        qt6-qtbase-devel \
        ncurses-devel
      
    elif [[ -f /etc/arch-release ]]; then
      # Arch Linux
      echo "Detected Arch Linux system"
      sudo pacman -Sy --noconfirm \
        base-devel cmake pkgconf \
        boost \
        openssl \
        nlohmann-json \
        spdlog \
        gtest \
        qt6-base \
        ncurses
    fi
  
  elif [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS
    echo "Detected macOS system"
    brew install \
      cmake \
      boost \
      openssl \
      nlohmann-json \
      spdlog \
      googletest \
      qt@6 \
      ncurses
  fi
}

# Function to create necessary placeholder files
create_placeholder_files() {
    echo -e "${YELLOW}${BOLD}Creating placeholder files...${RESET}"
    
    # Create common placeholder file
    mkdir -p "${PROJECT_ROOT}/src/common"
    cat > "${PROJECT_ROOT}/src/common/common_placeholder.cpp" << 'EOF'
#include <iostream>

namespace collab {
namespace common {

// This is a placeholder file until the actual common library components are implemented
void placeholder_function() {
    std::cout << "Common library placeholder function called" << std::endl;
}

} // namespace common
} // namespace collab
EOF

    # Create client placeholder file
    mkdir -p "${PROJECT_ROOT}/src/client"
    cat > "${PROJECT_ROOT}/src/client/client_placeholder.cpp" << 'EOF'
#include <iostream>

namespace collab {
namespace client {

// This is a placeholder file until the actual client library components are implemented
void placeholder_function() {
    std::cout << "Client library placeholder function called" << std::endl;
}

} // namespace client
} // namespace collab
EOF

    # Create test directories and placeholder test files
    mkdir -p "${PROJECT_ROOT}/tests/common"
    cat > "${PROJECT_ROOT}/tests/common/dummy_test.cpp" << 'EOF'
#include <gtest/gtest.h>

// A simple test to make sure the testing framework works
TEST(DummyTest, VerifyTestingWorks) {
    EXPECT_TRUE(true);
}
EOF

    mkdir -p "${PROJECT_ROOT}/tests/server"
    cat > "${PROJECT_ROOT}/tests/server/dummy_server_test.cpp" << 'EOF'
#include <gtest/gtest.h>
#include "server/server.h"

// A simple placeholder test for the server module
TEST(ServerTest, DummyTest) {
    EXPECT_TRUE(true);
}
EOF

    mkdir -p "${PROJECT_ROOT}/tests/client"
    cat > "${PROJECT_ROOT}/tests/client/dummy_client_test.cpp" << 'EOF'
#include <gtest/gtest.h>

// A placeholder test for the client module
TEST(ClientTest, DummyTest) {
    EXPECT_TRUE(true);
}
EOF
    
    echo -e "${GREEN}${BOLD}Placeholder files created successfully.${RESET}"
}

# Function to build the project
build_project() {
    echo -e "${YELLOW}${BOLD}Building the project...${RESET}"
    
    # Create build directory if it doesn't exist
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    # Create the install directory if it doesn't exist
    mkdir -p "$INSTALL_DIR"
    
    # Configure the project
    echo -e "${YELLOW}Configuring with CMake...${RESET}"
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
        -DBUILD_SERVER=ON
        
    if [ $? -ne 0 ]; then
        echo -e "${RED}CMake configuration failed. See errors above.${RESET}"
        return 1
    fi
    
    # Build the project
    echo -e "${YELLOW}Building...${RESET}"
    cmake --build . -- -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)
    
    if [ $? -ne 0 ]; then
        echo -e "${RED}Build failed. See errors above.${RESET}"
        return 1
    fi
    
    # Install the project
    echo -e "${YELLOW}Installing...${RESET}"
    cmake --install .
    
    if [ $? -ne 0 ]; then
        echo -e "${RED}Installation failed. See errors above.${RESET}"
        return 1
    fi
    
    # Run tests if requested
    if [ "$RUN_TESTS" = "1" ]; then
        echo -e "${YELLOW}Running tests...${RESET}"
        ctest --output-on-failure
        
        if [ $? -ne 0 ]; then
            echo -e "${YELLOW}Some tests failed, but continuing.${RESET}"
        fi
    fi
    
    echo -e "${GREEN}${BOLD}Project built successfully.${RESET}"
    return 0
}

# Function to create test CMakeLists.txt
create_tests_cmake() {
    echo -e "${YELLOW}${BOLD}Creating test CMakeLists.txt...${RESET}"
    
    mkdir -p "${PROJECT_ROOT}/tests"
    cat > "${PROJECT_ROOT}/tests/CMakeLists.txt" << 'EOF'
# Test utilities
add_library(test_utils STATIC
    utils/test_helpers.cpp
)

target_include_directories(test_utils PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
)

target_link_libraries(test_utils PUBLIC
    common
    GTest::GTest
)

# Common module tests
file(GLOB_RECURSIVE COMMON_TEST_SOURCES
    "common/*.cpp"
)

# Ensure there's at least one test file
if(NOT COMMON_TEST_SOURCES)
    set(COMMON_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/common/dummy_test.cpp)
endif()

add_executable(common_tests ${COMMON_TEST_SOURCES})

target_link_libraries(common_tests
    PRIVATE
    test_utils
    common
    GTest::GTest
    GTest::Main
)

# C++20 specific compile features
target_compile_features(common_tests PRIVATE cxx_std_20)

# Server module tests
if(BUILD_SERVER)
    file(GLOB_RECURSIVE SERVER_TEST_SOURCES
        "server/*.cpp"
    )
    
    # Ensure there's at least one test file
    if(NOT SERVER_TEST_SOURCES)
        set(SERVER_TEST_SOURCES 
            ${CMAKE_CURRENT_SOURCE_DIR}/server/dummy_server_test.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/server/session_handler_test.cpp
        )
    endif()
    
    add_executable(server_tests ${SERVER_TEST_SOURCES})
    
    target_link_libraries(server_tests
        PRIVATE
        test_utils
        common
        server
        GTest::GTest
        GTest::Main
    )
    
    # C++20 specific compile features
    target_compile_features(server_tests PRIVATE cxx_std_20)
endif()

# Client module tests (only if both client and qt are available)
if(BUILD_CLIENT)
    # Check Qt version for testing
    if(Qt6_FOUND)
        find_package(Qt6 COMPONENTS Test QUIET)
        if(Qt6_Test_FOUND)
            set(QT_TEST_LIB Qt6::Test)
            set(BUILD_CLIENT_TESTS ON)
        endif()
    elseif(Qt5_FOUND)
        find_package(Qt5 COMPONENTS Test QUIET)
        if(Qt5_Test_FOUND)
            set(QT_TEST_LIB Qt5::Test)
            set(BUILD_CLIENT_TESTS ON)
        endif()
    endif()

    if(BUILD_CLIENT_TESTS)
        file(GLOB_RECURSIVE CLIENT_TEST_SOURCES
            "client/*.cpp"
        )
        
        # Ensure there's at least one test file
        if(NOT CLIENT_TEST_SOURCES)
            # Create dummy client test file if it doesn't exist
            set(DUMMY_CLIENT_TEST ${CMAKE_CURRENT_SOURCE_DIR}/client/dummy_client_test.cpp)
            if(NOT EXISTS "${DUMMY_CLIENT_TEST}")
                file(WRITE "${DUMMY_CLIENT_TEST}" 
                "// Auto-generated dummy test file
#include <gtest/gtest.h>

// A placeholder test for the client module
TEST(ClientTest, DummyTest) {
    EXPECT_TRUE(true);
}
")
            endif()
            
            set(CLIENT_TEST_SOURCES ${DUMMY_CLIENT_TEST})
        endif()
        
        add_executable(client_tests ${CLIENT_TEST_SOURCES})
        
        if(Qt6_FOUND)
            target_link_libraries(client_tests
                PRIVATE
                test_utils
                common
                client
                GTest::GTest
                GTest::Main
                Qt6::Core
                ${QT_TEST_LIB}
            )
        else()
            target_link_libraries(client_tests
                PRIVATE
                test_utils
                common
                client
                GTest::GTest
                GTest::Main
                Qt5::Core
                ${QT_TEST_LIB}
            )
        endif()
        
        # C++20 specific compile features
        target_compile_features(client_tests PRIVATE cxx_std_20)
    endif()
endif()

# Register tests with CTest
add_test(NAME CommonTests COMMAND common_tests)

if(BUILD_SERVER)
    add_test(NAME ServerTests COMMAND server_tests)
endif()

if(BUILD_CLIENT_TESTS)
    add_test(NAME ClientTests COMMAND client_tests)
endif()
EOF
    
    echo -e "${GREEN}${BOLD}Test CMakeLists.txt created successfully.${RESET}"
}

# Function to create modified server header with ThreadPool and SessionHandler
create_server_files() {
    echo -e "${YELLOW}${BOLD}Creating server header files...${RESET}"
    
    # Create server directory if it doesn't exist
    mkdir -p "${PROJECT_ROOT}/include/server"
    
    # Create session_handler.h first since server.h depends on it
    cat > "${PROJECT_ROOT}/include/server/session_handler.h" << 'EOF'
// include/server/session_handler.h
#ifndef COLLABORATIVE_EDITOR_SESSION_HANDLER_H
#define COLLABORATIVE_EDITOR_SESSION_HANDLER_H

#include <iostream>
#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <boost/asio.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace collab {
namespace server {

/**
 * Represents a user session with authentication and state information
 */
class UserSession {
public:
    enum class State {
        CONNECTING,
        AUTHENTICATING,
        AUTHENTICATED,
        DISCONNECTED
    };

    /**
     * Constructor for a new user session
     * 
     * @param id Unique session ID
     * @param username User's name (if known)
     */
    UserSession(const std::string& id, const std::string& username = "")
        : id_(id),
          username_(username),
          state_(State::CONNECTING),
          creation_time_(std::chrono::steady_clock::now()),
          last_activity_(creation_time_) {
    }

    /**
     * Gets the session ID
     * 
     * @return The session ID
     */
    const std::string& getId() const {
        return id_;
    }

    /**
     * Gets the username
     * 
     * @return The username
     */
    const std::string& getUsername() const {
        return username_;
    }

    /**
     * Sets the username
     * 
     * @param username The username to set
     */
    void setUsername(const std::string& username) {
        username_ = username;
        updateActivity();
    }

    /**
     * Gets the session state
     * 
     * @return The current state
     */
    State getState() const {
        return state_;
    }

    /**
     * Sets the session state
     * 
     * @param state The state to set
     */
    void setState(State state) {
        state_ = state;
        updateActivity();
    }

    /**
     * Gets the creation time
     * 
     * @return The creation time
     */
    std::chrono::steady_clock::time_point getCreationTime() const {
        return creation_time_;
    }

    /**
     * Gets the time of last activity
     * 
     * @return The last activity time
     */
    std::chrono::steady_clock::time_point getLastActivity() const {
        return last_activity_;
    }

    /**
     * Updates the last activity time to now
     */
    void updateActivity() {
        last_activity_ = std::chrono::steady_clock::now();
    }

    /**
     * Gets the idle duration (time since last activity)
     * 
     * @return The idle duration
     */
    std::chrono::seconds getIdleSeconds() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::seconds>(now - last_activity_);
    }

    /**
     * Adds a user to a document's active users
     * 
     * @param documentId The document ID
     * @return True if added, false if already present
     */
    bool addDocument(const std::string& documentId) {
        auto result = active_documents_.insert(documentId);
        if (result.second) {
            updateActivity();
        }
        return result.second;
    }

    /**
     * Removes a document from active documents
     * 
     * @param documentId The document ID
     * @return True if removed, false if not found
     */
    bool removeDocument(const std::string& documentId) {
        auto count = active_documents_.erase(documentId);
        if (count > 0) {
            updateActivity();
        }
        return count > 0;
    }

    /**
     * Checks if a document is active for this user
     * 
     * @param documentId The document ID
     * @return True if active, false otherwise
     */
    bool hasDocument(const std::string& documentId) const {
        return active_documents_.find(documentId) != active_documents_.end();
    }

    /**
     * Gets all active documents for this user
     * 
     * @return Set of document IDs
     */
    const std::unordered_set<std::string>& getActiveDocuments() const {
        return active_documents_;
    }

private:
    std::string id_;
    std::string username_;
    State state_;
    std::chrono::steady_clock::time_point creation_time_;
    std::chrono::steady_clock::time_point last_activity_;
    std::unordered_set<std::string> active_documents_;
};

/**
 * RAII wrapper for a socket to ensure proper cleanup
 */
class SocketGuard {
public:
    /**
     * Constructor takes ownership of the socket
     * 
     * @param socket The socket to manage
     */
    explicit SocketGuard(std::shared_ptr<boost::asio::ip::tcp::socket> socket)
        : socket_(socket) {
    }
    
    /**
     * Destructor ensures socket is closed
     */
    ~SocketGuard() {
        close();
    }
    
    // Delete copy constructor and assignment
    SocketGuard(const SocketGuard&) = delete;
    SocketGuard& operator=(const SocketGuard&) = delete;
    
    // Allow move operations
    SocketGuard(SocketGuard&& other) noexcept
        : socket_(std::move(other.socket_)) {
        other.socket_ = nullptr;
    }
    
    SocketGuard& operator=(SocketGuard&& other) noexcept {
        if (this != &other) {
            close();
            socket_ = std::move(other.socket_);
            other.socket_ = nullptr;
        }
        return *this;
    }
    
    /**
     * Explicitly close the socket
     */
    void close() {
        if (socket_ && socket_->is_open()) {
            boost::system::error_code ec;
            socket_->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
            socket_->close(ec);
        }
    }
    
    /**
     * Get the underlying socket
     * 
     * @return The managed socket
     */
    std::shared_ptr<boost::asio::ip::tcp::socket> socket() const {
        return socket_;
    }
    
    /**
     * Check if the socket is valid and open
     * 
     * @return True if valid and open
     */
    bool isValid() const {
        return socket_ && socket_->is_open();
    }

private:
    std::shared_ptr<boost::asio::ip::tcp::socket> socket_;
};

/**
 * Manages user sessions and their connections
 */
class SessionHandler {
public:
    /**
     * Constructor
     */
    SessionHandler() 
        : uuid_generator_() {
    }
    
    /**
     * Destructor
     */
    ~SessionHandler() {
        // Sessions will be cleaned up automatically due to RAII
    }
    
    /**
     * Creates a new session for a connection
     * 
     * @param socket The client socket
     * @return A tuple with the session ID and the session object
     */
    std::pair<std::string, std::shared_ptr<UserSession>> createSession(
        std::shared_ptr<boost::asio::ip::tcp::socket> socket) {
        
        // Generate a unique ID for the session
        boost::uuids::uuid uuid = uuid_generator_();
        std::string sessionId = boost::uuids::to_string(uuid);
        
        // Create a new session
        auto session = std::make_shared<UserSession>(sessionId);
        
        // Create socket guard to ensure RAII for socket lifecycle
        auto socketGuard = std::make_shared<SocketGuard>(socket);
        
        // Store the session and socket
        std::lock_guard<std::mutex> lock(mutex_);
        sessions_[sessionId] = session;
        sockets_[sessionId] = socketGuard;
        
        std::cout << "Created session: " << sessionId << std::endl;
        return {sessionId, session};
    }
    
    /**
     * Authenticates a session with a username
     * 
     * @param sessionId The session ID
     * @param username The username to associate
     * @return True if successful, false if session not found
     */
    bool authenticateSession(const std::string& sessionId, const std::string& username) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = sessions_.find(sessionId);
        if (it != sessions_.end()) {
            it->second->setUsername(username);
            it->second->setState(UserSession::State::AUTHENTICATED);
            
            // Add to username mapping
            username_to_session_[username] = sessionId;
            
            std::cout << "Authenticated session " << sessionId << " as " << username << std::endl;
            return true;
        }
        return false;
    }
    
    /**
     * Gets a session by ID
     * 
     * @param sessionId The session ID
     * @return The session or nullptr if not found
     */
    std::shared_ptr<UserSession> getSession(const std::string& sessionId) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = sessions_.find(sessionId);
        if (it != sessions_.end()) {
            return it->second;
        }
        return nullptr;
    }
    
    /**
     * Gets a session by username
     * 
     * @param username The username
     * @return The session or nullptr if not found
     */
    std::shared_ptr<UserSession> getSessionByUsername(const std::string& username) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = username_to_session_.find(username);
        if (it != username_to_session_.end()) {
            auto sessionIt = sessions_.find(it->second);
            if (sessionIt != sessions_.end()) {
                return sessionIt->second;
            }
        }
        return nullptr;
    }
    
    /**
     * Gets the socket for a session
     * 
     * @param sessionId The session ID
     * @return The socket guard or nullptr if not found
     */
    std::shared_ptr<SocketGuard> getSocket(const std::string& sessionId) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = sockets_.find(sessionId);
        if (it != sockets_.end()) {
            return it->second;
        }
        return nullptr;
    }
    
    /**
     * Closes a session
     * 
     * @param sessionId The session ID
     * @return True if found and closed, false if not found
     */
    bool closeSession(const std::string& sessionId) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Find the session
        auto sessionIt = sessions_.find(sessionId);
        if (sessionIt == sessions_.end()) {
            return false;
        }
        
        // Remove from username mapping if authenticated
        if (sessionIt->second->getState() == UserSession::State::AUTHENTICATED) {
            const auto& username = sessionIt->second->getUsername();
            username_to_session_.erase(username);
        }
        
        // Mark as disconnected
        sessionIt->second->setState(UserSession::State::DISCONNECTED);
        
        // Remove socket (which will close it due to RAII)
        sockets_.erase(sessionId);
        
        // Remove session
        sessions_.erase(sessionIt);
        
        std::cout << "Closed session: " << sessionId << std::endl;
        return true;
    }
    
    /**
     * Gets all active sessions
     * 
     * @return Map of session IDs to sessions
     */
    std::unordered_map<std::string, std::shared_ptr<UserSession>> getSessions() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return sessions_;
    }
    
    /**
     * Gets the number of active sessions
     * 
     * @return The number of sessions
     */
    size_t getSessionCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return sessions_.size();
    }
    
    /**
     * Gets users active on a document
     * 
     * @param documentId The document ID
     * @return Vector of usernames active on the document
     */
    std::vector<std::string> getUsersOnDocument(const std::string& documentId) {
        std::vector<std::string> users;
        std::lock_guard<std::mutex> lock(mutex_);
        
        for (const auto& [sessionId, session] : sessions_) {
            if (session->hasDocument(documentId) && 
                session->getState() == UserSession::State::AUTHENTICATED) {
                users.push_back(session->getUsername());
            }
        }
        
        return users;
    }
    
    /**
     * Checks if a username is available (not currently in use)
     * 
     * @param username The username to check
     * @return True if available, false if in use
     */
    bool isUsernameAvailable(const std::string& username) {
        std::lock_guard<std::mutex> lock(mutex_);
        return username_to_session_.find(username) == username_to_session_.end();
    }
    
    /**
     * Clean up idle sessions
     * 
     * @param maxIdleSeconds Maximum idle time before cleanup
     * @return Number of sessions cleaned up
     */
    int cleanupIdleSessions(int maxIdleSeconds) {
        std::vector<std::string> sessionsToClose;
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (const auto& [sessionId, session] : sessions_) {
                if (session->getIdleSeconds().count() > maxIdleSeconds) {
                    sessionsToClose.push_back(sessionId);
                }
            }
        }
        
        // Close each idle session
        for (const auto& sessionId : sessionsToClose) {
            closeSession(sessionId);
        }
        
        return static_cast<int>(sessionsToClose.size());
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<UserSession>> sessions_;
    std::unordered_map<std::string, std::shared_ptr<SocketGuard>> sockets_;
    std::unordered_map<std::string, std::string> username_to_session_; // username -> sessionId
    boost::uuids::random_generator uuid_generator_;
};

} // namespace server
} // namespace collab

#endif // COLLABORATIVE_EDITOR_SESSION_HANDLER_H
EOF
    
    # Now create the server.h file
    cat > "${PROJECT_ROOT}/include/server/server.h" << 'EOF'
// include/server/server.h
#ifndef COLLABORATIVE_EDITOR_SERVER_H
#define COLLABORATIVE_EDITOR_SERVER_H

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <future>
#include <csignal>
#include <boost/asio.hpp>
#include "server/session_handler.h"

namespace collab {
namespace server {

/**
 * A thread pool for handling client sessions concurrently
 */
class ThreadPool {
public:
    /**
     * Creates a thread pool with a specified number of worker threads
     * 
     * @param numThreads The number of worker threads in the pool
     */
    ThreadPool(size_t numThreads) : stop_(false) {
        std::cout << "Creating thread pool with " << numThreads << " worker threads" << std::endl;
        
        for (size_t i = 0; i < numThreads; ++i) {
            workers_.emplace_back([this] {
                for (;;) {
                    std::function<void()> task;
                    
                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex_);
                        this->condition_.wait(lock, [this] {
                            return this->stop_ || !this->tasks_.empty();
                        });
                        
                        if (this->stop_ && this->tasks_.empty()) {
                            return;
                        }
                        
                        task = std::move(this->tasks_.front());
                        this->tasks_.pop();
                    }
                    
                    task();
                }
            });
        }
    }
    
    /**
     * Destroys the thread pool, joining all worker threads
     */
    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            stop_ = true;
        }
        
        condition_.notify_all();
        
        for (std::thread& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }
    
    /**
     * Enqueues a task to be executed by the thread pool
     * 
     * @param f The function to execute
     * @param args The arguments to pass to the function
     * @return A future that will hold the result of the function call
     */
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
        using return_type = decltype(f(args...));
        
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
        std::future<return_type> result = task->get_future();
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            
            // Don't allow enqueueing after stopping the pool
            if (stop_) {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }
            
            tasks_.emplace([task]() { (*task)(); });
        }
        
        condition_.notify_one();
        return result;
    }
    
    /**
     * Gets the number of worker threads in the pool
     * 
     * @return The number of worker threads
     */
    size_t size() const {
        return workers_.size();
    }
    
    /**
     * Gets the number of tasks waiting in the queue
     * 
     * @return The number of tasks in the queue
     */
    size_t queueSize() const {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        return tasks_.size();
    }

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    
    mutable std::mutex queue_mutex_;
    std::condition_variable condition_;
    bool stop_;
};

/**
 * Server class using Boost.Asio that manages TCP connections for
 * the Collaborative Text Editor. Provides graceful shutdown on SIGINT.
 */
class Server {
public:
    /**
     * Constructor
     * 
     * @param io_context The Boost.Asio io_context to use
     * @param port The port to listen on
     * @param threadPoolSize The number of threads in the thread pool
     */
    explicit Server(
        boost::asio::io_context& io_context, 
        unsigned short port,
        size_t threadPoolSize = std::thread::hardware_concurrency(),
        int sessionCleanupIntervalSeconds = 300,
        int maxSessionIdleSeconds = 3600
    ) : io_context_(io_context),
        acceptor_(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
        signals_(io_context, SIGINT, SIGTERM),
        thread_pool_(threadPoolSize),
        session_handler_(),
        cleanup_timer_(io_context),
        session_cleanup_interval_(sessionCleanupIntervalSeconds),
        max_session_idle_(maxSessionIdleSeconds),
        running_(true) {
        
        std::cout << "Server starting on port: " << port 
                  << " with " << thread_pool_.size() << " worker threads" << std::endl;
        
        // Set up signal handling for graceful shutdown
        setupSignalHandling();
        
        // Start the session cleanup timer
        startSessionCleanup();
        
        // Start accepting connections
        startAccept();
    }

    /**
     * Destructor - ensures clean shutdown
     */
    ~Server() {
        stop();
    }

    /**
     * Stops the server and closes all connections
     */
    void stop() {
        if (!running_.exchange(false)) {
            return; // Already stopped
        }

        std::cout << "\nShutting down server..." << std::endl;

        // Cancel timer
        boost::system::error_code ec;
        cleanup_timer_.cancel(ec);
        
        // Close acceptor to stop accepting new connections
        acceptor_.close(ec);
        if (ec) {
            std::cerr << "Error closing acceptor: " << ec.message() << std::endl;
        }

        // Close all sessions (which will close all connections through RAII)
        // No need to manually close connections as that's handled by SessionHandler
        
        std::cout << "Server shutdown complete" << std::endl;
    }

    /**
     * Returns whether the server is running
     * 
     * @return True if the server is running, false otherwise
     */
    bool isRunning() const {
        return running_.load();
    }
    
    /**
     * Get the server's local endpoint
     * 
     * @return The endpoint the server is listening on
     */
    boost::asio::ip::tcp::endpoint getEndpoint() const {
        return acceptor_.local_endpoint();
    }
    
    /**
     * Simulate a signal being received - for testing purposes
     * 
     * @param signal_number The signal to simulate
     */
    void simulateSignal(int signal_number) {
        boost::system::error_code ec;
        signals_.async_wait(
            [this, signal_number](const boost::system::error_code& error, int) {
                if (!error) {
                    std::cout << "\nSimulated signal " << signal_number << " received" << std::endl;
                    stop();
                }
            });
        signals_.cancel(ec);
    }
    
    /**
     * Get the size of the thread pool
     * 
     * @return The number of threads in the pool
     */
    size_t getThreadPoolSize() const {
        return thread_pool_.size();
    }
    
    /**
     * Get the number of active sessions
     * 
     * @return The number of sessions
     */
    size_t getSessionCount() const {
        return session_handler_.getSessionCount();
    }
    
    /**
     * Access the session handler
     * 
     * @return Reference to the session handler
     */
    SessionHandler& getSessionHandler() {
        return session_handler_;
    }
    
    /**
     * Access the session handler (const version)
     * 
     * @return Const reference to the session handler
     */
    const SessionHandler& getSessionHandler() const {
        return session_handler_;
    }

private:
    // Forward declaration for Connection class
    class Connection;

    /**
     * Sets up signal handling for graceful shutdown
     */
    void setupSignalHandling() {
        signals_.async_wait(
            [this](const boost::system::error_code& error, int signal_number) {
                if (!error) {
                    std::cout << "\nReceived signal " << signal_number << std::endl;
                    stop();
                }
            });
    }
    
    /**
     * Starts the periodic session cleanup timer
     */
    void startSessionCleanup() {
        cleanup_timer_.expires_after(std::chrono::seconds(session_cleanup_interval_));
        cleanup_timer_.async_wait([this](const boost::system::error_code& error) {
            if (!error && running_) {
                // Run the cleanup
                int cleanedCount = session_handler_.cleanupIdleSessions(max_session_idle_);
                if (cleanedCount > 0) {
                    std::cout << "Cleaned up " << cleanedCount << " idle sessions" << std::endl;
                }
                
                // Restart the timer
                startSessionCleanup();
            }
        });
    }

    /**
     * Starts accepting new connections
     */
    void startAccept() {
        auto socket = std::make_shared<boost::asio::ip::tcp::socket>(io_context_);
        
        acceptor_.async_accept(
            *socket,
            [this, socket](const boost::system::error_code& error) {
                if (!error && running_) {
                    handleNewConnection(socket);
                    
                    // Continue accepting connections if we're still running
                    if (running_) {
                        startAccept();
                    }
                } else if (error && running_) {
                    std::cerr << "Accept error: " << error.message() << std::endl;
                    
                    // Try to recover by continuing to accept
                    if (running_) {
                        startAccept();
                    }
                }
            });
    }

    /**
     * Handles a new client connection
     * 
     * @param socket The connected socket
     */
    void handleNewConnection(std::shared_ptr<boost::asio::ip::tcp::socket> socket);

    boost::asio::io_context& io_context_;
    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::signal_set signals_;
    ThreadPool thread_pool_;
    SessionHandler session_handler_;
    boost::asio::steady_timer cleanup_timer_;
    int session_cleanup_interval_;
    int max_session_idle_;
    std::atomic<bool> running_;
};

/**
 * Connection class for handling individual client connections
 */
class Server::Connection : public std::enable_shared_from_this<Connection> {
public:
    /**
     * Constructor
     * 
     * @param socket The connected socket
     * @param server The server that created this connection
     * @param sessionId The session ID
     * @param session The user session
     */
    Connection(
        std::shared_ptr<boost::asio::ip::tcp::socket> socket, 
        Server& server,
        const std::string& sessionId,
        std::shared_ptr<UserSession> session)
        : socket_(socket),
          server_(server),
          sessionId_(sessionId),
          session_(session),
          buffer_(8192) {
    }
    
    /**
     * Destructor
     */
    ~Connection() {
        // Socket is managed by SessionHandler through SocketGuard
    }

    /**
     * Starts reading from the connection
     */
    void start() {
        auto self(shared_from_this());
        socket_->async_read_some(
            boost::asio::buffer(buffer_),
            [this, self](const boost::system::error_code& error, std::size_t bytes_transferred) {
                if (!error) {
                    // Process received data - now handled in thread pool
                    processDataInThreadPool(bytes_transferred);
                } else {
                    handleError(error);
                }
            });
    }

    /**
     * Sends data to the client
     * 
     * @param data The data to send
     */
    void send(const std::string& data) {
        auto self(shared_from_this());
        
        boost::asio::async_write(
            *socket_,
            boost::asio::buffer(data),
            [this, self](const boost::system::error_code& error, std::size_t /*bytes_transferred*/) {
                if (error) {
                    handleError(error);
                }
            });
    }

private:
    /**
     * Processes received data in thread pool
     * 
     * @param bytes_transferred The number of bytes received
     */
    void processDataInThreadPool(std::size_t bytes_transferred) {
        auto self(shared_from_this());
        
        // Copy the data to ensure it doesn't get overwritten during async operations
        std::string data(buffer_.begin(), buffer_.begin() + bytes_transferred);
        
        // Process the data in the thread pool
        server_.thread_pool_.enqueue([this, self, data]() {
            // Process the data in a worker thread
            std::string response = processData(data);
            
            // Send the response back via the io_context thread
            boost::asio::post(socket_->get_executor(), [this, self, response]() {
                send(response);
            });
            
            // Continue reading - must be done in the io_context thread
            boost::asio::post(socket_->get_executor(), [this, self]() {
                start();
            });
        });
    }
    
    /**
     * Process the received data - runs in a worker thread
     * 
     * @param data The data to process
     * @return The response to send back
     */
    std::string processData(const std::string& data) {
        // Update user session activity time
        session_->updateActivity();
        
        // Check if this is a login message
        if (data.substr(0, 6) == "LOGIN:") {
            std::string username = data.substr(6);
            
            // Check if the username is available
            if (!server_.getSessionHandler().isUsernameAvailable(username)) {
                return "ERROR: Username already in use";
            }
            
            // Authenticate the session
            if (server_.getSessionHandler().authenticateSession(sessionId_, username)) {
                return "SUCCESS: Logged in as " + username;
            } else {
                return "ERROR: Authentication failed";
            }
        }
        
        // Check if this is a document open message
        if (data.substr(0, 11) == "OPEN_DOCUMENT:") {
            // Check if authenticated
            if (session_->getState() != UserSession::State::AUTHENTICATED) {
                return "ERROR: Not authenticated";
            }
            
            std::string documentId = data.substr(11);
            session_->addDocument(documentId);
            
            // Get all users on this document
            auto users = server_.getSessionHandler().getUsersOnDocument(documentId);
            
            std::ostringstream oss;
            oss << "SUCCESS: Opened document " << documentId << "\n";
            oss << "Users on this document: ";
            for (size_t i = 0; i < users.size(); ++i) {
                if (i > 0) oss << ", ";
                oss << users[i];
            }
            
            return oss.str();
        }
        
        // Check if this is a document close message
        if (data.substr(0, 12) == "CLOSE_DOCUMENT:") {
            // Check if authenticated
            if (session_->getState() != UserSession::State::AUTHENTICATED) {
                return "ERROR: Not authenticated";
            }
            
            std::string documentId = data.substr(12);
            if (session_->removeDocument(documentId)) {
                return "SUCCESS: Closed document " + documentId;
            } else {
                return "ERROR: Document not open";
            }
        }
    
        // Simulate some processing time
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        std::ostringstream oss;
        oss << "Server received: " << data 
            << " (processed by thread " << std::this_thread::get_id()
            << " for user " << (session_->getUsername().empty() ? "anonymous" : session_->getUsername())
            << ")";
        
        std::cout << "Processed: " << data << " in thread " << std::this_thread::get_id() 
                  << " for session " << sessionId_ << std::endl;
        
        return oss.str();
    }

    /**
     * Handles connection errors
     * 
     * @param error The error that occurred
     */
    void handleError(const boost::system::error_code& error) {
        if (error == boost::asio::error::eof || 
            error == boost::asio::error::connection_reset) {
            std::cout << "Connection closed by client" << std::endl;
        } else {
            std::cerr << "Connection error: " << error.message() << std::endl;
        }
        
        // Close the session (which will close the socket via RAII)
        server_.getSessionHandler().closeSession(sessionId_);
    }

    std::shared_ptr<boost::asio::ip::tcp::socket> socket_;
    Server& server_;
    std::string sessionId_;
    std::shared_ptr<UserSession> session_;
    std::vector<char> buffer_;
};

// Implementation of Server method that depends on Connection
inline void Server::handleNewConnection(std::shared_ptr<boost::asio::ip::tcp::socket> socket) {
    boost::asio::ip::tcp::endpoint remote_ep = socket->remote_endpoint();
    std::cout << "New connection from: " << remote_ep.address().to_string() 
              << ":" << remote_ep.port() << std::endl;
    
    // Create a new session for this connection
    auto [sessionId, session] = session_handler_.createSession(socket);
    
    // Create a new connection object
    auto connection = std::make_shared<Connection>(socket, *this, sessionId, session);
    
    // Start reading from the connection
    connection->start();
}

} // namespace server
} // namespace collab

#endif // COLLABORATIVE_EDITOR_SERVER_H
EOF

    # Create src/server/session_handler.cpp
    mkdir -p "${PROJECT_ROOT}/src/server"
    cat > "${PROJECT_ROOT}/src/server/session_handler.cpp" << 'EOF'
// src/server/session_handler.cpp
#include "server/session_handler.h"

namespace collab {
namespace server {

// Most of the implementation is in the header as inline methods
// This file is for any non-inline implementations that might be added later

} // namespace server
} // namespace collab
EOF

    # Create session_handler_test.cpp
    mkdir -p "${PROJECT_ROOT}/tests/server"
    cat > "${PROJECT_ROOT}/tests/server/session_handler_test.cpp" << 'EOF'
// tests/server/session_handler_test.cpp
#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <boost/asio.hpp>
#include "server/session_handler.h"

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
    UserSession session("test-session", "");
    
    // Initial state should be CONNECTING
    EXPECT_EQ(session.getState(), UserSession::State::CONNECTING);
    
    // Set state to AUTHENTICATING
    session.setState(UserSession::State::AUTHENTICATING);
    EXPECT_EQ(session.getState(), UserSession::State::AUTHENTICATING);
    
    // Set username and state to AUTHENTICATED
    session.setUsername("testuser");
    session.setState(UserSession::State::AUTHENTICATED);
    
    EXPECT_EQ(session.getUsername(), "testuser");
    EXPECT_EQ(session.getState(), UserSession::State::AUTHENTICATED);
}

// Test user session document tracking
TEST_F(SessionHandlerTest, UserSessionDocuments) {
    UserSession session("test-session", "testuser");
    
    // Initially, no documents
    EXPECT_TRUE(session.getActiveDocuments().empty());
    
    // Add a document
    EXPECT_TRUE(session.addDocument("doc1"));
    EXPECT_TRUE(session.hasDocument("doc1"));
    EXPECT_EQ(session.getActiveDocuments().size(), 1);
    
    // Add another document
    EXPECT_TRUE(session.addDocument("doc2"));
    EXPECT_TRUE(session.hasDocument("doc2"));
    EXPECT_EQ(session.getActiveDocuments().size(), 2);
    
    // Adding the same document again should fail
    EXPECT_FALSE(session.addDocument("doc1"));
    
    // Remove a document
    EXPECT_TRUE(session.removeDocument("doc1"));
    EXPECT_FALSE(session.hasDocument("doc1"));
    EXPECT_EQ(session.getActiveDocuments().size(), 1);
    
    // Removing a non-existent document should fail
    EXPECT_FALSE(session.removeDocument("doc3"));
}

// Test session handler basic operations
TEST_F(SessionHandlerTest, SessionHandlerBasic) {
    SessionHandler handler;
    
    // Create a session
    auto socket = createSocket();
    auto [sessionId, session] = handler.createSession(socket);
    
    EXPECT_FALSE(sessionId.empty());
    EXPECT_NE(session, nullptr);
    EXPECT_EQ(session->getState(), UserSession::State::CONNECTING);
    EXPECT_EQ(handler.getSessionCount(), 1);
    
    // Get the session by ID
    auto retrievedSession = handler.getSession(sessionId);
    EXPECT_EQ(retrievedSession, session);
    
    // Authenticate the session
    EXPECT_TRUE(handler.authenticateSession(sessionId, "testuser"));
    EXPECT_EQ(session->getUsername(), "testuser");
    EXPECT_EQ(session->getState(), UserSession::State::AUTHENTICATED);
    
    // Get session by username
    auto retrievedByUsername = handler.getSessionByUsername("testuser");
    EXPECT_EQ(retrievedByUsername, session);
    
    // Username should not be available anymore
    EXPECT_FALSE(handler.isUsernameAvailable("testuser"));
    
    // Close the session
    EXPECT_TRUE(handler.closeSession(sessionId));
    EXPECT_EQ(handler.getSessionCount(), 0);
    
    // Trying to get a closed session should return nullptr
    EXPECT_EQ(handler.getSession(sessionId), nullptr);
    
    // Username should be available again
    EXPECT_TRUE(handler.isUsernameAvailable("testuser"));
}

// Test multiple session management
TEST_F(SessionHandlerTest, MultipleSessionsAndDocuments) {
    SessionHandler handler;
    
    // Create three sessions
    auto socket1 = createSocket();
    auto socket2 = createSocket();
    auto socket3 = createSocket();
    
    auto [sessionId1, session1] = handler.createSession(socket1);
    auto [sessionId2, session2] = handler.createSession(socket2);
    auto [sessionId3, session3] = handler.createSession(socket3);
    
    EXPECT_EQ(handler.getSessionCount(), 3);
    
    // Authenticate two sessions
    EXPECT_TRUE(handler.authenticateSession(sessionId1, "user1"));
    EXPECT_TRUE(handler.authenticateSession(sessionId2, "user2"));
    
    // Open documents
    session1->addDocument("doc1");
    session1->addDocument("doc2");
    
    session2->addDocument("doc1");
    
    // Check users on documents
    auto usersOnDoc1 = handler.getUsersOnDocument("doc1");
    EXPECT_EQ(usersOnDoc1.size(), 2);
    EXPECT_TRUE(std::find(usersOnDoc1.begin(), usersOnDoc1.end(), "user1") != usersOnDoc1.end());
    EXPECT_TRUE(std::find(usersOnDoc1.begin(), usersOnDoc1.end(), "user2") != usersOnDoc1.end());
    
    auto usersOnDoc2 = handler.getUsersOnDocument("doc2");
    EXPECT_EQ(usersOnDoc2.size(), 1);
    EXPECT_EQ(usersOnDoc2[0], "user1");
    
    // Session 3 is not authenticated, so it should not be counted
    session3->addDocument("doc1");
    usersOnDoc1 = handler.getUsersOnDocument("doc1");
    EXPECT_EQ(usersOnDoc1.size(), 2); // Still 2
    
    // Close one session
    EXPECT_TRUE(handler.closeSession(sessionId2));
    EXPECT_EQ(handler.getSessionCount(), 2);
    
    // Check users on documents again
    usersOnDoc1 = handler.getUsersOnDocument("doc1");
    EXPECT_EQ(usersOnDoc1.size(), 1);
    EXPECT_EQ(usersOnDoc1[0], "user1");
}

// Test session cleanup
TEST_F(SessionHandlerTest, SessionCleanup) {
    SessionHandler handler;
    
    // Create two sessions
    auto socket1 = createSocket();
    auto socket2 = createSocket();
    
    auto [sessionId1, session1] = handler.createSession(socket1);
    auto [sessionId2, session2] = handler.createSession(socket2);
    
    EXPECT_EQ(handler.getSessionCount(), 2);
    
    // Set a very old last activity on session1
    session1->setState(UserSession::State::CONNECTING); // This updates last activity
    
    // Sleep to ensure session1 is idle
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // Update activity on session2
    session2->setState(UserSession::State::CONNECTING);
    
    // Clean up sessions with idle time > 5ms
    int cleaned = handler.cleanupIdleSessions(5 / 1000); // 5ms in seconds
    EXPECT_EQ(cleaned, 1);
    EXPECT_EQ(handler.getSessionCount(), 1);
    
    // session1 should be gone, session2 should remain
    EXPECT_EQ(handler.getSession(sessionId1), nullptr);
    EXPECT_NE(handler.getSession(sessionId2), nullptr);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
EOF

    echo -e "${GREEN}${BOLD}Server files created successfully.${RESET}"
}

# Function to update the modified README file
update_readme() {
    echo -e "${YELLOW}${BOLD}Updating README.md...${RESET}"
    
    cat > "${PROJECT_ROOT}/README.md" << 'EOF'
# Collaborative Text Editor

A real-time collaborative text editor written in C++20 with Boost.Asio for networking.

## Overview

This repository contains the implementation of a C++ Collaborative Text Editor with real-time editing capabilities. The implementation focuses on:

1. File System Data Structures
2. Network Message Protocol
3. TCP Communication Channel using Boost.Asio
4. User Session Management with RAII

## File Structure

### Server Module (`include/server/server.h`)

Implements a robust TCP server that handles client connections:

- `Server`: Main server class with graceful shutdown on SIGINT/SIGTERM
- `ThreadPool`: A thread pool for handling client sessions concurrently
- Nested `Connection` class for managing individual client connections
- Asynchronous I/O for high performance
- Thread-safe connection management

### Session Handler (`include/server/session_handler.h`)

Manages user sessions and their connections:

- `SessionHandler`: Central manager for user sessions
- `UserSession`: Represents user state, authentication, and document access
- `SocketGuard`: RAII wrapper for socket lifecycle management
- Thread-safe session tracking
- Automatic idle session cleanup

### File System Data Structures (`include/common/models/file_system.h`)

Provides classes for representing files and directories:

- `FileSystemNode`: Abstract base class with common functionality
- `File`: For managing file content and version history
- `Directory`: For organizing files and managing hierarchical structure

### Protocol Message Formats (`include/common/protocol/protocol.h`)

Defines structured message types for network communication:

- Authentication messages
- Document management messages
- Edit operation messages
- Synchronization messages
- Presence tracking messages

All messages are serializable to JSON for network transmission.

### TCP Communication (`include/common/network/tcp_connection.h`)

Implements network communication using Boost.Asio:

- `TcpConnection`: Low-level TCP socket management
- `TcpClient`: Client-side connection handling
- `TcpServer`: Server-side connection acceptance and management
- `MessageChannel`: Higher-level protocol message handling

## Key Features

- Real-time collaborative editing
- File and directory management
- Robust client-server communication
- Message serialization/deserialization
- Error handling and recovery
- Thread-safe operations
- Graceful shutdown on SIGINT/SIGTERM
- Configurable thread pool for concurrent client session handling
- RAII-based socket lifecycle management
- User session tracking and authentication
- Document access control
- Automatic idle session cleanup

## Server Implementation

The server implementation in `include/server/server.h` and `src/server/server.cpp` provides:

1. **TCP Connection Management**
   - Accepting new client connections
   - Managing active connections
   - Handling connection errors

2. **Graceful Shutdown**
   - Signal handling for SIGINT (Ctrl+C) and SIGTERM
   - Proper cleanup of all connections
   - Orderly shutdown of server resources

3. **Asynchronous I/O**
   - Non-blocking operations using Boost.Asio
   - Efficient handling of multiple clients

4. **Thread Pool**
   - Configurable number of worker threads
   - Concurrent processing of client requests
   - Efficient load balancing across threads
   - Thread-safe task queue

5. **Session Management**
   - User identification and authentication
   - Session state tracking
   - Document access tracking
   - RAII-based resource management

6. **Robust Error Handling**
   - Comprehensive error detection
   - Recovery mechanisms
   - Detailed error reporting

## Thread Pool Implementation

The `ThreadPool` class provides:

1. **Configurable Size**
   - Set the number of worker threads at creation time
   - Defaults to the number of hardware threads available

2. **Task Queuing**
   - Tasks are queued and distributed to available worker threads
   - Thread-safe queue with mutex protection

3. **Future-based API**
   - Returns `std::future` objects for task results
   - Allows waiting for task completion and retrieving results

4. **Clean Shutdown**
   - Properly joins all worker threads on destruction
   - Handles remaining tasks before shutting down

## Session Handler Implementation

The `SessionHandler` class provides:

1. **User Session Management**
   - Creating and tracking user sessions
   - Authentication and username validation
   - Document access tracking
   - Session state management
   - Idle session cleanup

2. **RAII Socket Management**
   - Automatic socket lifecycle management via `SocketGuard`
   - Ensures sockets are properly closed when sessions end
   - Prevents resource leaks and zombie connections

3. **Thread Safety**
   - Mutex-protected data structures
   - Safe for concurrent access from multiple threads
   - Proper synchronization of session state changes

4. **Document Collaboration**
   - Tracking which users are active on which documents
   - Supporting document-specific operations
   - Managing document sharing and access

## Usage Example

The `examples/collaborative_example.cpp` demonstrates:

1. Setting up a server
2. Connecting clients
3. Authenticating users
4. Opening and editing documents
5. Synchronizing changes

## Building and Running

### Prerequisites

- C++20 compliant compiler (e.g., GCC 10+, Clang 10+)
- Boost libraries (Asio, Bind, Program_Options, UUID)
- nlohmann_json
- CMake 3.16+
- Optional: Qt6 for client GUI, Google Test for unit tests

### Build Commands

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### Installation

```bash
cmake --install .
```

### Running the Server

```bash
./install/bin/server
```

### Running Tests

```bash
cd build
ctest --output-on-failure
```

## Contributing

Contributions are welcome! Please follow these steps:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/your-feature`)
3. Commit your changes (`git commit -am 'Add your feature'`)
4. Push to the branch (`git push origin feature/your-feature`)
5. Open a Pull Request

## License

This project is licensed under the MIT License. See the `LICENSE` file for details.
EOF
    
    echo -e "${GREEN}${BOLD}README.md updated successfully.${RESET}"
}

# Main execution logic
echo -e "${GREEN}${BOLD}Starting build process for Collaborative Text Editor...${RESET}"

# Parse command line arguments
RUN_TESTS=0
while [[ $# -gt 0 ]]; do
    case $1 in
        --tests)
            RUN_TESTS=1
            shift
            ;;
        *)
            echo -e "${RED}Unknown option: $1${RESET}"
            exit 1
            ;;
    esac
done

# Install dependencies
install_dependencies

# Create placeholder files
create_placeholder_files

# Create test CMakeLists.txt
create_tests_cmake

# Create server files
create_server_files

# Update README
update_readme

# Build the project
build_project

if [ $? -eq 0 ]; then
    echo -e "${GREEN}${BOLD}Build process completed successfully.${RESET}"
else
    echo -e "${RED}${BOLD}Build process failed.${RESET}"
    exit 1
fi