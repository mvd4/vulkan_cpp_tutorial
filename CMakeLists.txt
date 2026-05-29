cmake_minimum_required( VERSION 3.21 )

project( 
    "vulkan-cpp-tutorial" 
    VERSION 1.0.0
    DESCRIPTION "Getting started with Vulkan using the C++ wrapper"
    LANGUAGES CXX
)

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" )

add_subdirectory( src )
