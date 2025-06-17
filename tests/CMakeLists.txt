
# Test suite configuration
enable_testing()

# Common test utilities
set(TEST_UTILS_SOURCES
    utils/test_helpers.cpp
)

file(GLOB_RECURSE TEST_UTILS_HEADERS
    "${CMAKE_SOURCE_DIR}/tests/utils/*.h"
    "${CMAKE_SOURCE_DIR}/tests/utils/*.hpp"
)

add_library(test_utils STATIC ${TEST_UTILS_SOURCES} ${TEST_UTILS_HEADERS})

target_include_directories(test_utils
    PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/tests
)

target_link_libraries(test_utils
    PUBLIC
    GTest::GTest
    GTest::Main
    common
)

# Common module tests
file(GLOB_RECURSE COMMON_TEST_SOURCES
    "common/*.cpp"
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
        "client/*.cpp"
    )
    
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
endif()

# Server module tests
if(BUILD_SERVER)
    file(GLOB_RECURSE SERVER_TEST_SOURCES
        "server/*.cpp"
    )
    
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
endif()

# Register tests with CTest
add_test(NAME CommonTests COMMAND common_tests)

if(BUILD_CLIENT)
    add_test(NAME ClientTests COMMAND client_tests)
endif()

if(BUILD_SERVER)
    add_test(NAME ServerTests COMMAND server_tests)
endif()
