# Server application
set(SERVER_SOURCES
    main.cpp
)

add_executable(collabServer ${SERVER_SOURCES})

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
    OpenSSL::SSL
    OpenSSL::Crypto
)

# C++20 specific compile features
target_compile_features(collabServer PRIVATE cxx_std_20)