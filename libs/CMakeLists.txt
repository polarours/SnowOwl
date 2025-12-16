add_library(snowowl_libs STATIC)

set(LIBS_SOURCES
    config/config_manager.cpp
    config/device_registry.cpp
    hal/basic_camera.cpp
    hal/thermal_camera.cpp
    plugin/plugin_manager.cpp
    protocol/message_parser.cpp
    utils/logger.cpp
    utils/app_paths.cpp
    utils/health_monitor.cpp
    utils/resource_tracker.cpp
    utils/system_probe.cpp
)

set(LIBS_HEADERS
    config/config_manager.hpp
    config/device_registry.hpp
    detection/detection_types.hpp
    hal/basic_camera.hpp
    hal/camera_interface.hpp
    hal/gpu_accelerator.hpp
    hal/industrial_io.hpp
    hal/thermal_camera.hpp
    plugin/plugin_manager.hpp
    protocol/message_parser.hpp
    protocol/message_types.hpp
    utils/logger.hpp
    utils/app_paths.hpp
    utils/health_monitor.hpp
    utils/resource_tracker.hpp
    utils/system_probe.hpp
)

foreach(file IN LISTS LIBS_SOURCES)
    target_sources(snowowl_libs PRIVATE ${file})
endforeach()

foreach(file IN LISTS LIBS_HEADERS)
    target_sources(snowowl_libs PRIVATE ${file})
endforeach()

add_library(snowowl_libs::snowowl_libs ALIAS snowowl_libs)
add_library(snowowl_common::snowowl_common ALIAS snowowl_libs)

target_include_directories(snowowl_libs
    PUBLIC
        ${CMAKE_BINARY_DIR}/generated
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/libs
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/config
        ${CMAKE_CURRENT_SOURCE_DIR}/utils
        ${CMAKE_CURRENT_SOURCE_DIR}/hal
        ${CMAKE_CURRENT_SOURCE_DIR}/plugin
        ${CMAKE_CURRENT_SOURCE_DIR}/protocol
        ${CMAKE_CURRENT_SOURCE_DIR}/detection
)

target_link_libraries(snowowl_libs
    PUBLIC
        ${OpenCV_LIBS}
        ${Boost_LIBRARIES}
        Qt6::Core
    ${PostgreSQL_LIBRARIES}
)

if (WIN32)
    target_link_libraries(snowowl_libs
        PRIVATE
            shell32
    )
endif()

target_include_directories(snowowl_libs PRIVATE ${PostgreSQL_INCLUDE_DIRS})
target_compile_options(snowowl_libs PRIVATE ${PostgreSQL_CFLAGS_OTHER})
