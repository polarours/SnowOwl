cmake_minimum_required(VERSION 3.16)
project(industrial_safety_plugin)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenCV REQUIRED)

# Plugin source files
set(PLUGIN_SOURCES
    src/industrial_safety_plugin.cpp
)

# Create the plugin library
add_library(industrial_safety SHARED ${PLUGIN_SOURCES})

# Include directories
target_include_directories(industrial_safety
    PRIVATE
        ${CMAKE_SOURCE_DIR}/../../include
        ${CMAKE_SOURCE_DIR}/../../libs
)

# Link libraries
target_link_libraries(industrial_safety
    PRIVATE
        ${OpenCV_LIBS}
)

# Set the output directory to the lib folder
set_target_properties(industrial_safety PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

# Copy the manifest file to the build directory
configure_file(manifest.json ${CMAKE_CURRENT_BINARY_DIR}/manifest.json COPYONLY)