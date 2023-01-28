cmake_minimum_required( VERSION 3.13 )

set( PROJECT_NAME vulkan-cpp-tutorial )

file( MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/bin/shaders" )

add_executable( ${PROJECT_NAME} main.cpp )

# set debug working directory for MSVC
set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin" )

target_include_directories( ${PROJECT_NAME} PRIVATE ${VULKAN_TUTORIAL_PROJECT_ROOT}/src )
target_include_directories( ${PROJECT_NAME} PRIVATE ${Vulkan_INCLUDE_DIRS} )

target_sources( ${PROJECT_NAME} PRIVATE devices.cpp  devices.hpp )
target_sources( ${PROJECT_NAME} PRIVATE glfw_utils.cpp  glfw_utils.hpp )
target_sources( ${PROJECT_NAME} PRIVATE memory.cpp  memory.hpp )
target_sources( ${PROJECT_NAME} PRIVATE pipelines.cpp  pipelines.hpp )
target_sources( ${PROJECT_NAME} PRIVATE presentation.cpp  presentation.hpp )
target_sources( ${PROJECT_NAME} PRIVATE rendering.cpp  rendering.hpp )


cmake_policy( SET CMP0076 NEW )

target_link_directories( ${PROJECT_NAME} PRIVATE ${VULKAN_TUTORIAL_PROJECT_ROOT}/build/lib )
target_link_directories( ${PROJECT_NAME} PRIVATE ${VULKAN_LIB_DIR} )

target_link_libraries( ${PROJECT_NAME} ${Vulkan_LIBRARIES} )
target_link_libraries( ${PROJECT_NAME} ${CONAN_LIBS} )

add_dependencies( ${PROJECT_NAME} vertex_shader fragment_shader )


# shaders
add_custom_target( vertex_shader
    COMMAND             "glslc" "${VULKAN_TUTORIAL_PROJECT_ROOT}/shaders/vertex.vert" -o "shaders/vertex.spv"
    WORKING_DIRECTORY   "${CMAKE_BINARY_DIR}/bin"
)

add_custom_target( fragment_shader
    COMMAND             "glslc" "${VULKAN_TUTORIAL_PROJECT_ROOT}/shaders/fragment.frag" -o "shaders/fragment.spv"
    WORKING_DIRECTORY   "${CMAKE_BINARY_DIR}/bin"
)
