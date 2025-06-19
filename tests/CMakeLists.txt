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
file(GLOB_RECURSE COMMON_TEST_SOURCES
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
    file(GLOB_RECURSE SERVER_TEST_SOURCES
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
        file(GLOB_RECURSE CLIENT_TEST_SOURCES
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