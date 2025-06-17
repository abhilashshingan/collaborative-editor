
# Client application
set(CLIENT_SOURCES
    main.cpp
    editor/document.cpp
    editor/cursor.cpp
    editor/syntax_highlighter.cpp
    gui/main_window.cpp
    gui/editor_widget.cpp
    gui/collaboration_panel.cpp
    collaboration/document_sync.cpp
    collaboration/presence_manager.cpp
    network/websocket_client.cpp
    network/rest_client.cpp
)

# Use file(GLOB...) with caution as it can lead to build system issues
# when new files are added but not detected by the build system.
# However, it's convenient for early development stages.
file(GLOB_RECURSE CLIENT_HEADERS
    "${CMAKE_SOURCE_DIR}/include/client/*.h"
    "${CMAKE_SOURCE_DIR}/include/client/*.hpp"
)

add_executable(collabEditor ${CLIENT_SOURCES} ${CLIENT_HEADERS})

target_include_directories(collabEditor
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(collabEditor
    PRIVATE
    common
    Qt6::Core
    Qt6::Widgets
    Boost::system
    Boost::thread
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    websocketpp::websocketpp
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Qt-specific setup
set_target_properties(collabEditor PROPERTIES 
    AUTOMOC TRUE
    AUTOUIC TRUE
    AUTORCC TRUE
)

# C++20 specific compile features
target_compile_features(collabEditor PRIVATE cxx_std_20)
