
# Common library shared between client and server
set(COMMON_SOURCES
    protocol/message.cpp
    protocol/serialization.cpp
    crdt/document.cpp
    crdt/operation.cpp
    util/logger.cpp
    models/user.cpp
    models/document.cpp
)

file(GLOB_RECURSE COMMON_HEADERS
    "${CMAKE_SOURCE_DIR}/include/common/*.h"
    "${CMAKE_SOURCE_DIR}/include/common/*.hpp"
)

add_library(common STATIC ${COMMON_SOURCES} ${COMMON_HEADERS})

target_include_directories(common
    PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(common
    PUBLIC
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    Boost::system
    OpenSSL::SSL
)

# C++20 specific compile features
target_compile_features(common PRIVATE cxx_std_20)
