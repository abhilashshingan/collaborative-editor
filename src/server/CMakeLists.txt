
# Server application
set(SERVER_SOURCES
    main.cpp
    auth/auth_manager.cpp
    auth/user_store.cpp
    document/document_manager.cpp
    document/document_store.cpp
    session/session_manager.cpp
    session/client_connection.cpp
    sync/sync_engine.cpp
    sync/operation_processor.cpp
)

file(GLOB_RECURSE SERVER_HEADERS
    "${CMAKE_SOURCE_DIR}/include/server/*.h"
    "${CMAKE_SOURCE_DIR}/include/server/*.hpp"
)

add_executable(collabServer ${SERVER_SOURCES} ${SERVER_HEADERS})

target_include_directories(collabServer
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(collabServer
    PRIVATE
    common
    Boost::system
    Boost::thread
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    websocketpp::websocketpp
    OpenSSL::SSL
    OpenSSL::Crypto
)

# C++20 specific compile features
target_compile_features(collabServer PRIVATE cxx_std_20)
