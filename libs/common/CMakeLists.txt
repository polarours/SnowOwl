# CMakeLists.txt for libs/common directory

# Define the common library interface
add_library(snowowl_common_subdir INTERFACE)

# Specify include directories for the common library interface
target_include_directories(snowowl_common_subdir
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/config
        ${CMAKE_CURRENT_SOURCE_DIR}/utils
)