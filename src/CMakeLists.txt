set( TARGET_NAME vulkan-cpp-tutorial )

find_package( glfw3 CONFIG REQUIRED )
find_package( Vulkan REQUIRED COMPONENTS glslc )

add_executable( ${TARGET_NAME} main.cpp )

target_sources( ${TARGET_NAME} PRIVATE devices.cpp  devices.hpp )
target_sources( ${TARGET_NAME} PRIVATE memory.cpp  memory.hpp )
target_sources( ${TARGET_NAME} PRIVATE pipelines.cpp  pipelines.hpp )

target_compile_features( ${TARGET_NAME} PRIVATE cxx_std_20 )
set_target_properties( ${TARGET_NAME} PROPERTIES
    CXX_EXTENSIONS OFF
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${TARGET_NAME}>"
)

target_link_libraries( ${TARGET_NAME} PRIVATE glfw Vulkan::Vulkan )


# shaders
set( SHADER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders" )
set( SHADER_BINARY_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/shaders" )

add_custom_command(
    OUTPUT  "${SHADER_BINARY_DIR}/compute.comp.spv"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_BINARY_DIR}"
    COMMAND ${Vulkan_GLSLC_EXECUTABLE} "${SHADER_SOURCE_DIR}/compute.comp" -o "${SHADER_BINARY_DIR}/compute.comp.spv"
    DEPENDS "${SHADER_SOURCE_DIR}/compute.comp"
    COMMENT "Compiling compute shader"
    VERBATIM
)

add_custom_target( shaders DEPENDS "${SHADER_BINARY_DIR}/compute.comp.spv" )
add_dependencies( ${TARGET_NAME} shaders )
