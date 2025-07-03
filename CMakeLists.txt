cmake_minimum_required(VERSION 3.16)
project(collaborative_editor VERSION 0.1.0 LANGUAGES CXX)

# Set C++20 as the standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(BUILD_TESTS "Build tests" ON)
option(BUILD_SERVER "Build server component" ON)
option(BUILD_CLIENT "Build client component" ON)

# Find packages
find_package(Threads REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)
find_package(spdlog REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(GTest REQUIRED)
find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)

# Enable automoc for Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${GTEST_INCLUDE_DIRS}
)

# Common library
add_library(common STATIC
    src/common/ot/operation.cpp
    src/common/document/document_controller.cpp
    src/common/document/history_manager.cpp
    src/common/document/operation_manager.cpp
)

target_include_directories(common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(common PUBLIC
    Threads::Threads
    nlohmann_json::nlohmann_json
    Boost::system
    spdlog::spdlog
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Server application
if(BUILD_SERVER)
    add_executable(server
        src/server/main.cpp
    )
    
    target_link_libraries(server PRIVATE
        common
    )
endif()

# Client application
if(BUILD_CLIENT)
    add_executable(client
        src/client/main.cpp
        src/client/document_editor.cpp
        src/client/session_manager.cpp
    )
    
    target_link_libraries(client PRIVATE
        common
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
    )
endif()

# Tests
if(BUILD_TESTS)
    # Check if test files exist before adding them
    set(TEST_FILES "")
    
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/common/document/history_manager_test.cpp")
        list(APPEND TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/tests/common/document/history_manager_test.cpp")
    endif()
    
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/common/document/document_controller_test.cpp")
        list(APPEND TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/tests/common/document/document_controller_test.cpp")
    endif()
    
    # Only add test executable if test files exist
    if(TEST_FILES)
        enable_testing()
        
        add_executable(common_tests ${TEST_FILES})
        
        target_link_libraries(common_tests PRIVATE
            common
            GTest::GTest
            GTest::Main
        )
        
        add_test(NAME common_tests COMMAND common_tests)
    else()
        message(STATUS "No test files found. Skipping test compilation.")
    endif()
endif()

# Install
install(TARGETS common
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

if(BUILD_SERVER)
    install(TARGETS server RUNTIME DESTINATION bin)
endif()

if(BUILD_CLIENT)
    install(TARGETS client RUNTIME DESTINATION bin)
endif()