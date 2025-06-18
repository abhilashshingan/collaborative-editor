# Common library
set(COMMON_SOURCES
    models/file_system.cpp
    models/document.cpp
    protocol/protocol.cpp
    crdt/crdt_document.cpp
    util/uuid_generator.cpp
)

add_library(common STATIC ${COMMON_SOURCES})

target_include_directories(common
    PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(common
    PUBLIC
    Boost::system
    Boost::thread
    nlohmann_json::nlohmann_json
    OpenSSL::SSL
    OpenSSL::Crypto
)

# C++20 specific compile features
target_compile_features(common PRIVATE cxx_std_20)