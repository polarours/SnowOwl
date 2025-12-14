find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavcodec libavformat libavutil libswscale)
pkg_check_modules(GSTREAMER REQUIRED IMPORTED_TARGET gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0)
pkg_check_modules(TINYXML2 REQUIRED IMPORTED_TARGET tinyxml2)

# Find Protobuf and gRPC
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)

if(ARCTICOWL_ENABLE_ONNXRUNTIME)
    pkg_check_modules(ONNXRUNTIME libonnxruntime)
    if(ONNXRUNTIME_FOUND)
        message(STATUS "ONNX Runtime found for server component")
    else()
        message(WARNING "ARCTICOWL_ENABLE_ONNXRUNTIME is ON but libonnxruntime is missing; YOLO detection will be disabled")
    endif()
endif()

set(DETECTOR_SOURCES
    modules/detection/unified_detector.cpp
)

set(DETECTOR_HEADERS
    modules/detection/detector.hpp
    modules/detection/unified_detector.hpp
)

add_library(snowowl_detectors STATIC ${DETECTOR_SOURCES} ${DETECTOR_HEADERS})

target_include_directories(snowowl_detectors
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/libs
)

target_link_libraries(snowowl_detectors
    PUBLIC
        snowowl_common::snowowl_common
        ${OpenCV_LIBS}
)

if(ONNXRUNTIME_FOUND)
    target_include_directories(snowowl_detectors PRIVATE ${ONNXRUNTIME_INCLUDE_DIRS})
    target_link_libraries(snowowl_detectors PRIVATE ${ONNXRUNTIME_LIBRARIES})
    target_compile_definitions(snowowl_detectors PRIVATE HAVE_ONNXRUNTIME)
endif()

# Proto file
get_filename_component(snowowl_proto "./modules/api/grpc/snowowl.proto" ABSOLUTE)
get_filename_component(snowowl_proto_path "${snowowl_proto}" PATH)

# Generated sources
set(snowowl_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/snowowl.pb.cc")
set(snowowl_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/snowowl.pb.h")
set(snowowl_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/snowowl.grpc.pb.cc")
set(snowowl_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/snowowl.grpc.pb.h")

add_custom_command(
    OUTPUT "${snowowl_proto_srcs}" "${snowowl_proto_hdrs}" "${snowowl_grpc_srcs}" "${snowowl_grpc_hdrs}"
    COMMAND protobuf::protoc
    ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
         --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
         -I "${snowowl_proto_path}"
         --plugin=protoc-gen-grpc=/usr/bin/grpc_cpp_plugin
         "${snowowl_proto}"
    DEPENDS "${snowowl_proto}"
    COMMENT "Generating gRPC sources"
)

# Include generated files
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(SERVER_CORE_SOURCES
    core/streams/video_capture.cpp
    core/streams/video_processor.cpp
    core/streams/video_capture_manager.cpp
    core/streams/stream_dispatcher.cpp
    core/output/rtmp_output.cpp
    core/output/rtsp_output.cpp
    modules/network/network_server.cpp
    modules/api/rest/rest_server.cpp
    modules/api/websocket/websocket_server.cpp
    modules/api/unified/api_server.cpp
    modules/ingest/stream_receiver.cpp
    modules/utils/server_utils.cpp
    modules/discovery/network_scanner.cpp
    modules/discovery/device_discovery.cpp
)

set(SERVER_CORE_HEADERS
    core/streams/video_capture.hpp
    core/streams/video_processor.hpp
    core/streams/video_capture_manager.hpp
    core/streams/stream_dispatcher.hpp
    core/output/rtmp_output.hpp
    core/output/rtsp_output.hpp
    modules/network/network_server.hpp
    modules/api/rest/rest_server.hpp
    modules/api/websocket/websocket_server.hpp
    modules/api/unified/api_server.hpp
    modules/ingest/stream_receiver.hpp
    modules/utils/server_utils.hpp
    modules/discovery/network_scanner.hpp
    modules/discovery/device_discovery.hpp
)

list(APPEND SERVER_CORE_SOURCES 
    ${snowowl_proto_srcs} 
    ${snowowl_grpc_srcs}
    modules/api/grpc/grpc_server.cpp
)

list(APPEND SERVER_CORE_HEADERS 
    ${snowowl_proto_hdrs} 
    ${snowowl_grpc_hdrs}
    modules/api/grpc/grpc_server.hpp
)

add_library(snowowl_server_core STATIC ${SERVER_CORE_SOURCES} ${SERVER_CORE_HEADERS})

target_include_directories(snowowl_server_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}/generated
    PRIVATE
        ${FFMPEG_INCLUDE_DIRS}
        ${GSTREAMER_INCLUDE_DIRS}
        ${TINYXML2_INCLUDE_DIRS}
)

target_link_libraries(snowowl_server_core
    PUBLIC
        snowowl_common::snowowl_common
        snowowl_detectors
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
        ${Boost_LIBRARIES}
        Boost::program_options
        nlohmann_json::nlohmann_json
        ${OpenCV_LIBS}
        pthread
    PRIVATE
        PkgConfig::FFMPEG
        PkgConfig::GSTREAMER
        PkgConfig::TINYXML2
)

target_link_libraries(snowowl_server_core
    PRIVATE
        gRPC::grpc++
        gRPC::grpc
        protobuf::libprotobuf
)

if(ONNXRUNTIME_FOUND)
    target_include_directories(snowowl_server_core PRIVATE ${ONNXRUNTIME_INCLUDE_DIRS})
    target_link_libraries(snowowl_server_core PRIVATE ${ONNXRUNTIME_LIBRARIES})
endif()

add_executable(snowowl-server main_server.cpp)

qt_add_resources(snowowl-server snowowl_server_translations
    PREFIX "/i10n"
    FILES ${SnowOwl_QM_FILES}
)

target_link_libraries(snowowl-server
    PRIVATE
        snowowl_server_core
        owl_cli_managers
)

install(TARGETS snowowl-server DESTINATION bin)