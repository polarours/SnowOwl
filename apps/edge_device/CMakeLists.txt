find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED IMPORTED_TARGET gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0)

set(EDGE_CORE_SOURCES
	core/device_controller.cpp
	core/stream_capture.cpp
	core/stream_forwarder.cpp
	core/audio_processor.cpp
	modules/config/device_config.cpp
	modules/config/device_profile.cpp
	modules/network/connection_manager.cpp
	modules/network/forward_client.cpp
	modules/utils/encoder_selector.cpp
	modules/utils/power_manager.cpp
)

set(EDGE_HEADERS
	core/device_controller.hpp
	core/stream_capture.hpp
	core/stream_forwarder.hpp
	core/audio_processor.hpp
	modules/config/device_config.hpp
	modules/config/device_profile.hpp
	modules/network/connection_manager.hpp
	modules/network/forward_client.hpp
	modules/utils/encoder_selector.hpp
 	modules/utils/power_manager.hpp
)

add_library(snowowl_edge_core STATIC ${EDGE_CORE_SOURCES} ${EDGE_HEADERS})

target_include_directories(snowowl_edge_core
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_SOURCE_DIR}/include
		${CMAKE_BINARY_DIR}/generated
		${CMAKE_SOURCE_DIR}/libs/common
		${CMAKE_SOURCE_DIR}/apps/cli
	PRIVATE
		${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(snowowl_edge_core
	PUBLIC
		${Boost_LIBRARIES}
		${OpenCV_LIBS}
		snowowl_common::snowowl_common
		nlohmann_json::nlohmann_json
		PkgConfig::GSTREAMER
)

add_executable(snowowl-edge main_edge.cpp)

target_link_libraries(snowowl-edge
	PRIVATE
		snowowl_edge_core
		owl_cli_managers
)

install(TARGETS snowowl-edge DESTINATION bin)