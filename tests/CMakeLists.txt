# Test utils library
add_library(test_utils STATIC
    utils/test_helpers.cpp
)

target_include_directories(test_utils
    PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(test_utils
    PUBLIC
    common
    GTest::GTest
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

# Register tests with CTest
add_test(NAME CommonTests COMMAND common_tests)