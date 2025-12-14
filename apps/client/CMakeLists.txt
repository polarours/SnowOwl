set(CLIENT_CORE_SOURCES
    core/client_api.cpp
    core/client_config.cpp
    core/client_state.cpp
    web/web_client_launcher.cpp
    web/web_client.cpp
    flutter/flutter_client.cpp
)

set(CLIENT_HEADERS
    core/client_api.hpp
    core/client_config.hpp
    core/client_state.hpp
    web/web_client_launcher.hpp
    web/web_client.hpp
    flutter/flutter_client.hpp
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(CURL REQUIRED IMPORTED_TARGET libcurl)

add_library(snowowl_client_core STATIC ${CLIENT_CORE_SOURCES} ${CLIENT_HEADERS})

target_include_directories(snowowl_client_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}/generated
)

target_link_libraries(snowowl_client_core
    PUBLIC
        snowowl_common::snowowl_common
        ${OpenCV_LIBS}
        ${Boost_LIBRARIES}
        nlohmann_json::nlohmann_json
        PkgConfig::CURL
        pthread
)

add_executable(snowowl-client main_client.cpp)

target_link_libraries(snowowl-client
    PRIVATE
        snowowl_client_core
        owl_cli_managers
)

install(TARGETS snowowl-client DESTINATION bin)