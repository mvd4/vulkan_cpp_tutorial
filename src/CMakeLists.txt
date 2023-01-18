cmake_minimum_required( VERSION 3.13 )

set( PROJECT_NAME vulkan-cpp-tutorial )


add_executable( ${PROJECT_NAME} main.cpp )

target_include_directories( ${PROJECT_NAME} PRIVATE ${VULKAN_TUTORIAL_PROJECT_ROOT}/src )
target_include_directories( ${PROJECT_NAME} PRIVATE ${Vulkan_INCLUDE_DIRS} )

cmake_policy( SET CMP0076 NEW )

target_link_directories( ${PROJECT_NAME} PRIVATE ${VULKAN_TUTORIAL_PROJECT_ROOT}/build/lib )
target_link_directories( ${PROJECT_NAME} PRIVATE ${VULKAN_LIB_DIR} )

target_link_libraries( ${PROJECT_NAME} ${Vulkan_LIBRARIES} )
target_link_libraries( ${PROJECT_NAME} ${CONAN_LIBS} )

