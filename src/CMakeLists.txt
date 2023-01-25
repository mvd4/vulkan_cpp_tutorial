cmake_minimum_required( VERSION 3.13 )

set( PROJECT_NAME vulkan-cpp-tutorial )

file( MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/bin/shaders" )

add_executable( ${PROJECT_NAME} main.cpp )

# set debug working directory for MSVC
set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin" )

target_include_directories( ${PROJECT_NAME} PRIVATE ${VULKAN_TUTORIAL_PROJECT_ROOT}/src )
target_include_directories( ${PROJECT_NAME} PRIVATE ${Vulkan_INCLUDE_DIRS} )

target_sources( ${PROJECT_NAME} PRIVATE devices.cpp  devices.hpp )
target_sources( ${PROJECT_NAME} PRIVATE memory.cpp  memory.hpp )
target_sources( ${PROJECT_NAME} PRIVATE pipelines.cpp  pipelines.hpp )

cmake_policy( SET CMP0076 NEW )

target_link_directories( ${PROJECT_NAME} PRIVATE ${VULKAN_TUTORIAL_PROJECT_ROOT}/build/lib )
target_link_directories( ${PROJECT_NAME} PRIVATE ${VULKAN_LIB_DIR} )

target_link_libraries( ${PROJECT_NAME} ${Vulkan_LIBRARIES} )
target_link_libraries( ${PROJECT_NAME} ${CONAN_LIBS} )

add_dependencies( ${PROJECT_NAME} compute_shader )


# shaders
add_custom_target( compute_shader
    COMMAND             "glslc" "${VULKAN_TUTORIAL_PROJECT_ROOT}/shaders/compute.comp" -o "shaders/compute.spv"
    WORKING_DIRECTORY   "${CMAKE_BINARY_DIR}/bin"
)