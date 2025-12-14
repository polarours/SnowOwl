find_package(PkgConfig REQUIRED)
find_package(CURL REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED IMPORTED_TARGET gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0)

set(CLI_CORE_SOURCES
    core/cli_manager.cpp
)

set(CLI_MANAGER_SOURCES
    managers/server_manager.cpp
    managers/edge_manager.cpp
    managers/client_manager.cpp
    managers/device_manager.cpp
    managers/config_manager.cpp
)


add_library(owl_cli_managers STATIC ${CLI_CORE_SOURCES} ${CLI_MANAGER_SOURCES})

target_include_directories(owl_cli_managers PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/apps/client
)

target_link_libraries(owl_cli_managers
    PUBLIC
        snowowl_common::snowowl_common
        snowowl_server_core
        snowowl_edge_core
        snowowl_client_core
        ${Boost_LIBRARIES}
        ${OpenCV_LIBS}
        nlohmann_json::nlohmann_json
        Qt6::Core
        Qt6::Network
        Qt6::Widgets
        CURL::libcurl
        PkgConfig::GSTREAMER
)

set(CLI_APP_SOURCES
    main_owl.cpp
    main_owlctl.cpp
    commands/server_commands.cpp
    commands/edge_commands.cpp
    commands/client_commands.cpp
    commands/device_commands.cpp
    commands/config_commands.cpp
)

add_executable(owlctl main_owlctl.cpp commands/server_commands.cpp commands/edge_commands.cpp commands/client_commands.cpp commands/device_commands.cpp commands/config_commands.cpp)
target_link_libraries(owlctl
    PRIVATE
        owl_cli_managers
        Boost::program_options
)

add_executable(owl-cli main_owl.cpp core/cli_options.cpp commands/server_commands.cpp commands/edge_commands.cpp commands/client_commands.cpp commands/device_commands.cpp commands/config_commands.cpp)
target_link_libraries(owl-cli
    PRIVATE
        owl_cli_managers
        Boost::program_options
)

install(TARGETS owlctl owl-cli DESTINATION bin)
