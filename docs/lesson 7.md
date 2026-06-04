# Lesson 7: Shaders
Shaders are small programs that are executed on the GPU hardware, usually a lot of them in parallel. Vulkan and OpenGL shaders are commonly written in a language called GLSL (GL Shading Language)[^1], which is very similar to C. The shaders are compiled and uploaded to the graphics hardware every time the Vulkan application runs. There are multiple types of shaders that allow you to customize the graphics pipeline at different points, e.g. vertex-, fragment- or geometry shaders. For our compute pipeline however there is only one relevant type, which is the compute shader.

## GLSL basics
The most basic shader we can write in GLSL looks something like this:
```glsl
#version 450

void main()
{

}
```
To C and C++ programmers this looks pretty familiar, right? The first line, starting with a hash, is indeed called a preprocessor directive just like in C and C++. Every shader should start with this version identifier that denotes the GLSL language version the shader is written in. This allows the compiler to process the code in the best possible way and do detailed checks according to the denoted version.

Most of the other preprocessor directives will be intuitive for C and C++ programmers: `#define`, `#undef`, `#ifdef`, `#pragma`, `#error` etc. There are also a few that are specific to GLSL, like `#version`, but ultimately there's nothing new here.

The main function is slightly different compared to its C/C++ counterparts in that it doesn't have a return value. Shaders do not return values. But otherwise functions are declared and used in the same way as in C/C++.

## Compiling the shader
Another difference between Vulkan and OpenGL is that Vulkan requires us to precompile the shaders. In OpenGL the GLSL code is loaded by the application and then compiled and linked into a program explicitly. Since all of that happens at runtime, the respective driver on the user's device needs to do all the required heavy lifting. This makes OpenGL drivers even more complex than they need to be anyway and may lead to subtle differences in behaviour or even bugs because of the different compiler implementations.

In Vulkan, the GLSL code has to be precompiled into an intermediate binary representation called SPIR-V before loading it into the application and handing it over to the runtime. So instead of raw GLSL the shaders are shipped with the application as SPIR-V bytecode. Compiling SPIR-V to the native binary code for the GPU is much less complex, the drivers can thus be simpler and the potential for bugs and divergences is reduced quite significantly.

The GLSL to SPIR-V compiler that comes with the Vulkan SDK is located in its `bin` folder and is called `glslc`. Its basic usage is very straightforward:
```shell
> glslc  <glsl_shader_filename>  -o <compiled_shader_filename>
```
So let's compile our minimal shader from above. Create a folder `shaders` in the `src` directory. In that folder create a text file named `compute.comp`[^2] and paste the above code into the file. Then create a folder `shaders` inside your build configuration output directory (e.g. `build/bin/Debug/`) and finally run this command[^3]:
```shell
$ glslc src/shaders/compute.comp -o build/bin/Debug/shaders/compute.comp.spv
```
That command should terminate without any output (indicating success) and you now should see the file `compute.comp.spv`[^4] in your build output's `shaders` folder.

Nice, we now have the shader code in a format that can be used by Vulkan. However, I don't want to manually repeat the compilation whenever I change the shader code, so let's instead add it as a build step to our `CMakeLists.txt`.

First, let's define two convenience variables for the shader source and output locations so we don't have to repeat the paths:
```cmake
set( SHADER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders" )
set( SHADER_BINARY_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/shaders" )
```
For this to work, we need to set `CMAKE_RUNTIME_OUTPUT_DIRECTORY` in our `CMakePresets.json`[^5]:
```json
{
    ...
    "configurePresets": [
        {
        "name": "base",
        "hidden": true,
        "binaryDir": "${sourceDir}/build/${presetName}",
        "toolchainFile": "$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake",
        "cacheVariables": {
            "CMAKE_RUNTIME_OUTPUT_DIRECTORY": "${sourceDir}/build/bin"
        }
        },
        ...
    ]
}
```

With the convenience variables in place, we can now define a custom command for the shader compilation:
```cmake
add_custom_command(
    OUTPUT  "${SHADER_BINARY_DIR}/compute.comp.spv"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_BINARY_DIR}"
    COMMAND ${Vulkan_GLSLC_EXECUTABLE} "${SHADER_SOURCE_DIR}/compute.comp" -o "${SHADER_BINARY_DIR}/compute.comp.spv"
    DEPENDS "${SHADER_SOURCE_DIR}/compute.comp"
    COMMENT "Compiling compute shader"
    VERBATIM
)
```
The `OUTPUT` clause tells CMake what file this command produces. CMake uses this to compare timestamps: if `compute.comp.spv` is newer than `compute.comp`, the shader is not recompiled. The `DEPENDS` clause names the source file to compare against.

The first `COMMAND` creates the output directory at build time if it doesn't exist yet, and the second runs the actual compilation. `${Vulkan_GLSLC_EXECUTABLE}` is a CMake variable that is populated by `find_package( Vulkan REQUIRED )` with the full path to the `glslc` compiler that ships with the Vulkan SDK.

`add_custom_command` on its own only defines the command; it runs only when another target actually consumes its `OUTPUT`. As things stand nothing does, so CMake would never execute it. We therefore create a custom target that depends on the output file and make it a dependency of our main target.
```cmake
add_custom_target( shaders DEPENDS "${SHADER_BINARY_DIR}/compute.comp.spv" )
add_dependencies( ${TARGET_NAME} shaders )
```
Now the shader will be compiled on the first build and recompiled only when `compute.comp` changes.

## Loading the shader
Okay, we have the precompiled shader now. The next step is to load it into the application and pass it on to Vulkan. The Vulkan C++ representation of a shader is `vk::ShaderModule` and it is created like this:
```cpp
class Device
{
    ...
    UniqueShaderModule createShaderModuleUnique( const ShaderModuleCreateInfo& createInfo, ... );
    ...
};
```
No surprises so far, let's look at `ShaderModuleCreateInfo`:
```cpp
struct ShaderModuleCreateInfo
{
    ...
    ShaderModuleCreateInfo& setFlags( vk::ShaderModuleCreateFlags );
    ShaderModuleCreateInfo& setCode( const vk::container_t< const std::uint32_t >& );
    ...
};
```
As so often, the flags are just there for future use, which means that we really only have one parameter to set - the shader code in the form of a 32bit uint buffer. Which means that the whole process of creating the `ShaderModule` is very straightforward:
```cpp
auto createShaderModule(
    const vk::Device& logicalDevice,
    const std::filesystem::path& path
) -> vk::UniqueShaderModule
{
    std::ifstream is{ path, std::ios::binary };
    if ( !is.is_open() )
        throw std::runtime_error( "Could not open file" );

    auto buffer = std::vector< std::uint32_t >{};
    const auto bufferSizeInBytes = std::filesystem::file_size( path );
    if ( bufferSizeInBytes % sizeof( std::uint32_t ) != 0 )
        throw std::runtime_error( "Shader file size is not a multiple of 4 bytes" );
    buffer.resize( bufferSizeInBytes / sizeof( std::uint32_t ) );

    is.seekg( 0 );
    is.read( reinterpret_cast< char* >( buffer.data() ), bufferSizeInBytes );

    const auto createInfo = vk::ShaderModuleCreateInfo{}.setCode( buffer );
    return logicalDevice.createShaderModuleUnique( createInfo );
}
```
The only thing we need to pay a bit of attention to is the mismatch between what the standard library expects when reading data (a pointer to a byte buffer) and what Vulkan expects (a `uint32_t` buffer). I've packaged the shader module creation into its own function from the start because it makes the code in main clearer and we're definitely going to need it more often in the future when we get to the graphics shaders. So with that we can load our compiled compute shader:
```cpp
const auto computeShader = createShaderModule( *logicalDevice, "./shaders/compute.comp.spv" );
```

---

[^1]: The GLSL Vulkan profile differs slightly from the one for OpenGL, mostly in that it removes deprecations. Some OpenGL shaders therefore might not compile directly for Vulkan without modifications. However, those modifications should usually be pretty minor.
[^2]: There is no official standard that specifies the extension of .glsl shaders. However, the extensions `.vert`, `.frag` and `.comp` are very common for vertex, fragment and compute shaders respectively. They are also recognized by most tools that work with GLSL (e.g. VS code extensions).
[^3]: Installing the Vulkan SDK should have put its `bin` directory in your path so that the executable is found automatically. If that is not the case you should add that directory to your path by hand and try again.
[^4]: The output file name `compute.comp.spv` may seem unnecessarily repetitive. This is true in our case, where we only have one compute shader. In larger projects however, you might have multiple vertex-, fragment- and compute-shaders. In that case, adding the shader type to the output filename helps avoid collisions and avoid confusion.
[^5]: For the created program binaries, `CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG` and `CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE` take precedence over `CMAKE_RUNTIME_OUTPUT_DIRECTORY`. They ensure that our setup works with both single-config generators (e.g. Ninja) and multi-config generators (e.g. Visual Studio). If we wanted to use those variables for the compiled shaders as well, however, we'd have to add a conditional in the cmake code, so I opted for this approach.
