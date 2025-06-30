cmake_minimum_required(VERSION 3.16)
project(collaborative-editor VERSION 0.1.0 LANGUAGES CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(BUILD_CLIENT "Build the client" ON)
option(BUILD_SERVER "Build the server" ON)
option(BUILD_TESTS "Build tests" ON)

# Find required packages
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# Common library - shared code between client and server
add_library(common STATIC
    src/common/crdt/document.cpp
)

target_include_directories(common
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(common
    PUBLIC
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    Boost::system
    OpenSSL::SSL
    OpenSSL::Crypto
)

# C++20 specific compile features
target_compile_features(common PRIVATE cxx_std_20)

# Server executable
if(BUILD_SERVER)
    add_executable(server
        src/server/main.cpp
    )
    
    target_link_libraries(server
        PRIVATE
        common
        Boost::system
        Boost::thread
        Threads::Threads
    )
    
    # C++20 specific compile features
    target_compile_features(server PRIVATE cxx_std_20)
    
    # Install server
    install(TARGETS server
        RUNTIME DESTINATION bin
    )
endif()

# Client executable
if(BUILD_CLIENT)
    # Qt GUI Client
    find_package(Qt6 COMPONENTS Core Widgets REQUIRED)
    
    add_executable(client
        src/client/main.cpp
    )
    
    target_link_libraries(client
        PRIVATE
        common
        Qt6::Core
        Qt6::Widgets
        Boost::system
        Boost::thread
        Threads::Threads
    )
    
    # C++20 specific compile features
    target_compile_features(client PRIVATE cxx_std_20)
    
    # Install client
    install(TARGETS client
        RUNTIME DESTINATION bin
    )
    
    # Terminal Client with NCurses
    find_package(Curses REQUIRED)
    
    add_executable(ncurses_client 
        src/client/ncurses_client.cpp
    )
    
    target_link_libraries(ncurses_client
        PRIVATE
        common
        ${CURSES_LIBRARIES}
        Boost::system
        Boost::thread
        Threads::Threads
    )
    
    target_include_directories(ncurses_client
        PRIVATE
        ${CURSES_INCLUDE_DIR}
        ${PROJECT_SOURCE_DIR}/include
    )
    
    # C++20 specific compile features
    target_compile_features(ncurses_client PRIVATE cxx_std_20)
    
    # Install ncurses client
    install(TARGETS ncurses_client
        RUNTIME DESTINATION bin
    )
endif()

# Tests
if(BUILD_TESTS)
    # Enable testing
    enable_testing()
    
    # Find GTest
    find_package(GTest REQUIRED)
    
    # Test utilities
    add_library(test_utils STATIC
        tests/utils/test_helpers.cpp
    )
    
    target_include_directories(test_utils
        PUBLIC
        ${PROJECT_SOURCE_DIR}/tests/utils
    )
    
    target_link_libraries(test_utils
        PUBLIC
        common
        GTest::GTest
    )
    
    # C++20 specific compile features
    target_compile_features(test_utils PRIVATE cxx_std_20)
    
    # Common module tests
    file(GLOB_RECURSE COMMON_TEST_SOURCES
        "tests/common/*.cpp"
    )

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
    
    # Client module tests
    if(BUILD_CLIENT)
        file(GLOB_RECURSE CLIENT_TEST_SOURCES
            "tests/client/*.cpp"
        )
        
        if(CLIENT_TEST_SOURCES)
            add_executable(client_tests ${CLIENT_TEST_SOURCES})
            
            target_link_libraries(client_tests
                PRIVATE
                test_utils
                common
                GTest::GTest
                GTest::Main
                Qt6::Core
                Qt6::Test
            )
            
            # C++20 specific compile features
            target_compile_features(client_tests PRIVATE cxx_std_20)
            
            # Register with CTest
            add_test(NAME ClientTests COMMAND client_tests)
        endif()
    endif()
    
    # Server module tests
    if(BUILD_SERVER)
        file(GLOB_RECURSE SERVER_TEST_SOURCES
            "tests/server/*.cpp"
        )
        
        if(SERVER_TEST_SOURCES)
            add_executable(server_tests ${SERVER_TEST_SOURCES})
            
            target_link_libraries(server_tests
                PRIVATE
                test_utils
                common
                GTest::GTest
                GTest::Main
            )
            
            # C++20 specific compile features
            target_compile_features(server_tests PRIVATE cxx_std_20)
            
            # Register with CTest
            add_test(NAME ServerTests COMMAND server_tests)
        endif()
    endif()
    
    # Register common tests with CTest
    add_test(NAME CommonTests COMMAND common_tests)
endif()