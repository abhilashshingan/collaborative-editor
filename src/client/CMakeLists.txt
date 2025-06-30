# Client application
set(CLIENT_SOURCES
    main.cpp
)

add_executable(collabEditor ${CLIENT_SOURCES})

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

# Add this to the project's main CMakeLists.txt under the client section

# Find ncurses
find_package(Curses REQUIRED)

# Add ncurses client executable
add_executable(ncurses_client 
    src/client/ncurses_client.cpp
)

target_link_libraries(ncurses_client
    PRIVATE
    ${CURSES_LIBRARIES}
    Boost::system
    Boost::thread
    pthread
    common
)

target_include_directories(ncurses_client
    PRIVATE
    ${CURSES_INCLUDE_DIR}
    ${PROJECT_SOURCE_DIR}/include
)

# Install ncurses client
install(TARGETS ncurses_client
    RUNTIME DESTINATION bin
)