cmake_minimum_required( VERSION 3.13 )

project( "vulkan-cpp-tutorial" )

set( CMAKE_CXX_STANDARD 17)
set( CMAKE_CXX_STANDARD_REQUIRED ON)
set( CMAKE_CXX_EXTENSIONS OFF)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

set( VULKAN_TUTORIAL_PROJECT_ROOT ${CMAKE_CURRENT_LIST_DIR} )

# locate the vulkan sdk and set the paths accordingly
if ( NOT DEFINED VULKAN_SDK_ROOT ) 

    message( STATUS "VULKAN_SDK_ROOT not set, trying to locate Vulkan SDK using CMake..." )
    
    find_package(Vulkan)

    if ( Vulkan_FOUND )
        if ( NOT ${Vulkan_INCLUDE_DIRS} STREQUAL "" )
            set( VULKAN_SDK_ROOT ${Vulkan_INCLUDE_DIRS} )
            STRING(REGEX REPLACE "/Include" "" VULKAN_SDK_ROOT  ${VULKAN_SDK_ROOT} )
        endif()
        message( STATUS "Found Vulkan SDK at ${VULKAN_SDK_ROOT}" )
    else()
        message( "Error: Could not locate Vulkan SDK. Try setting VULKAN_SDK_ROOT manually" )
        return()
    endif()

else()

    if (WIN32)
        set( Vulkan_INCLUDE_DIRS "${VULKAN_SDK_ROOT}/Include" )
        set( Vulkan_LIBRARIES "${VULKAN_SDK_ROOT}/lib/vulkan-1.lib" )
    else()
        set( Vulkan_INCLUDE_DIRS "${VULKAN_SDK_ROOT}/include" )  
        set( Vulkan_LIBRARIES "${VULKAN_SDK_ROOT}/lib/libvulkan.dylib" )      
    endif()

endif()

add_subdirectory( src )
