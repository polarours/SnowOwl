add_library(snowowl_libs STATIC)

set(LIBS_SOURCES
    #common/config/config_loader.cpp
    common/config/config_manager.cpp
    #common/config/config_schema.cpp
    #common/config/device_profile.cpp
    common/config/device_registry.cpp
    hal/basic_camera.cpp
    hal/thermal_camera.cpp
    plugin/plugin_manager.cpp
    protocol/message_parser.cpp
    common/utils/logger.cpp
    common/utils/app_paths.cpp
)

set(LIBS_HEADERS
    #common/config/config_loader.hpp
    common/config/config_manager.hpp
    #common/config/config_schema.hpp
    #common/config/device_profile.hpp
    common/config/device_registry.hpp
    detection/detection_types.hpp
    hal/basic_camera.hpp
    hal/camera_interface.hpp
    hal/gpu_accelerator.hpp
    hal/industrial_io.hpp
    hal/thermal_camera.hpp
    plugin/plugin_manager.hpp
    protocol/message_parser.hpp
    protocol/message_types.hpp
    common/utils/logger.hpp
    common/utils/app_paths.hpp
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
        ${CMAKE_CURRENT_SOURCE_DIR}/common
        ${CMAKE_CURRENT_SOURCE_DIR}/common/config
        ${CMAKE_CURRENT_SOURCE_DIR}/common/utils
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
