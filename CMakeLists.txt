cmake_minimum_required(VERSION 3.16)
project(CollaborativeEditor VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard (requiring C++20 or later)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(BUILD_CLIENT "Build the client application" ON)
option(BUILD_SERVER "Build the server application" ON)
option(BUILD_TESTS "Build the test suite" ON)

# Global compiler flags
if(MSVC)
    add_compile_options(/W4 /WX /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# External dependencies
find_package(Boost COMPONENTS system thread REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(OpenSSL REQUIRED)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Common library
add_subdirectory(src/common)

# Client application
if(BUILD_CLIENT)
    # Qt is only needed if building the client
    find_package(Qt6 COMPONENTS Core Widgets REQUIRED)
    add_subdirectory(src/client)
endif()

# Server application
if(BUILD_SERVER)
    add_subdirectory(src/server)
endif()

# Tests
if(BUILD_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)
    add_subdirectory(tests)
endif()

# External dependencies
add_subdirectory(third_party)

# Install rules
install(TARGETS 
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)