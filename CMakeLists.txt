cmake_minimum_required(VERSION 3.16)

project(SnowOwl VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O2")

option(SNOWOWL_BUILD_CLIENT "Build SnowOwl client application" ON)
option(SNOWOWL_BUILD_SERVER "Build SnowOwl server application" ON)
option(SNOWOWL_BUILD_EDGE "Build SnowOwl edge device agent" ON)
option(SNOWOWL_BUILD_TESTS "Build SnowOwl tests" OFF)
option(SNOWOWL_ENABLE_ONNXRUNTIME "Enable ONNX Runtime support for YOLO detection" OFF)

# ----------------------------
# Dependencies
# ----------------------------

# OpenCV
find_package(OpenCV REQUIRED COMPONENTS core imgproc highgui imgcodecs videoio video features2d objdetect)

# Boost (prefer classic FindBoost module to match distro packaging)
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 OLD)
endif()
set(Boost_NO_BOOST_CMAKE ON)
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.65 REQUIRED COMPONENTS thread program_options MODULE)

# Boost 1.89 marks boost::system header-only on many distros
add_compile_definitions(BOOST_ERROR_CODE_HEADER_ONLY)

# Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network LinguistTools Concurrent)

# PostgreSQL
find_package(PkgConfig REQUIRED)
pkg_check_modules(PostgreSQL REQUIRED libpq)

# CURL
find_package(CURL REQUIRED)

# gRPC
find_package(gRPC CONFIG REQUIRED)
if(gRPC_FOUND)
    message(STATUS "gRPC found, enabling gRPC API support")
    add_definitions(-DHAVE_GRPC)
else()
    message(WARNING "gRPC not found, gRPC API will not be available")
endif()

# ONNX Runtime
if(SNOWOWL_ENABLE_ONNXRUNTIME)
    pkg_check_modules(ONNXRUNTIME libonnxruntime)
    if(ONNXRUNTIME_FOUND)
        message(STATUS "ONNX Runtime found, enabling YOLO detection support")
        add_definitions(-DHAVE_ONNXRUNTIME)
    else()
        message(WARNING "ONNX Runtime not found, YOLO detection will not be available")
    endif()
endif()

# Google Test
include(CTest)
find_package(GTest QUIET)

# nlohmann_json
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
        DOWNLOAD_EXTRACT_TIMESTAMP true 
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

# ----------------------------
# Qt Auto
# ----------------------------
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Version header
configure_file(
    ${CMAKE_SOURCE_DIR}/include/snow_owl/version.h.in
    ${CMAKE_BINARY_DIR}/generated/snow_owl/version_config.h
    @ONLY
)

# ----------------------------
# Subdirectories
# ----------------------------
add_subdirectory(libs)

if (SNOWOWL_BUILD_SERVER)
    add_subdirectory(apps/server)
endif()

if (SNOWOWL_BUILD_CLIENT)
    add_subdirectory(apps/client)
endif()

if (SNOWOWL_BUILD_EDGE)
    add_subdirectory(apps/edge_device)
endif()

include_directories(
    ${PROJECT_SOURCE_DIR}/apps/server
    ${PROJECT_SOURCE_DIR}/apps/edge_device
    ${PROJECT_SOURCE_DIR}/apps/client
    ${PROJECT_SOURCE_DIR}/apps/cli
)

add_subdirectory(apps/cli)

if (SNOWOWL_BUILD_TESTS AND GTest_FOUND)
    add_subdirectory(tests)
endif()

# ----------------------------
# Translations
# ----------------------------
set(SnowOwl_TS_FILES
    ${CMAKE_SOURCE_DIR}/apps/client/resources/translations/snowowl_zh_CN.ts
)

add_custom_target(SnowOwl_translations)

qt_add_translations(SnowOwl_translations
    TS_FILES ${SnowOwl_TS_FILES}
    QM_FILES_OUTPUT_VARIABLE SnowOwl_QM_FILES
)

foreach(qm_file IN LISTS SnowOwl_QM_FILES)
    get_filename_component(qm_name "${qm_file}" NAME)
    set_source_files_properties("${qm_file}" PROPERTIES QT_RESOURCE_ALIAS "${qm_name}")
endforeach()

qt_add_resources(SnowOwlTranslations snowowl_translations
    PREFIX "/i18n"
    FILES ${SnowOwl_QM_FILES}
)

# ----------------------------
# CLI Executable
# ----------------------------
add_executable(owl apps/main.cpp)

target_link_libraries(owl
    PRIVATE
        snowowl_common::snowowl_common
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
        ${OpenCV_LIBS}
        Boost::thread
        Boost::program_options
        ${PostgreSQL_LIBRARIES}
        nlohmann_json::nlohmann_json
        owl_cli_managers
        snowowl_detectors
)

install(TARGETS owl
    RUNTIME DESTINATION bin
)
