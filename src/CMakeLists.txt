set( TARGET_NAME vulkan-cpp-tutorial )

find_package( Vulkan REQUIRED )

add_executable( ${TARGET_NAME} main.cpp )

target_compile_features( ${TARGET_NAME} PRIVATE cxx_std_20 )
set_target_properties( ${TARGET_NAME} PROPERTIES
    CXX_EXTENSIONS OFF
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${TARGET_NAME}>"
)

target_link_libraries( ${TARGET_NAME} PRIVATE Vulkan::Vulkan )
