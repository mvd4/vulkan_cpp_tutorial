# Lesson 15: Creating the Graphics Pipeline - Part 1
Alright, our task for today is to start filling the `GraphicsPipelineCreateInfo` structure with information on how we want our graphics pipeline to be configured. So let's dive straight in:

## Setting up the pipeline stages
As mentioned in the last lesson, we can ignore the flags, so let's start with the `setStages` function. That one takes a container of `PipelineShaderStageCreateInfo`, which we already looked at back in lesson 8. The difference is that now we need at least a vertex and a fragment shader, so let's modify our pipeline creation accordingly:
```cpp
auto createGraphicsPipeline(
    const vk::Device& logicalDevice,
    const vk::ShaderModule& vertexShader,
    const vk::ShaderModule& fragmentShader
) -> vk::UniquePipeline
{
    const auto shaderStageInfos = std::vector< vk::PipelineShaderStageCreateInfo >{
        vk::PipelineShaderStageCreateInfo{}
            .setStage( vk::ShaderStageFlagBits::eVertex )
            .setPName( "main" )
            .setModule( vertexShader ),
        vk::PipelineShaderStageCreateInfo{}
            .setStage( vk::ShaderStageFlagBits::eFragment )
            .setPName( "main" )
            .setModule( fragmentShader ),
    };

    const auto pipelineCreateInfo = vk::GraphicsPipelineCreateInfo{}
        .setStages( shaderStageInfos );

    return logicalDevice.createGraphicsPipelineUnique( vk::PipelineCache{}, pipelineCreateInfo ).value;
}
```
So far, so good. We don't have those shaders yet though. Let's change that.

## Vertex and fragment shader
We already know how to write a shader in principle, so let's get going right away. For the vertex shader I'll take a page from the original Vulkan Tutorial's book here and put all the vertex information into the shader initially. We'll have to change this later, but right now it will simplify things because it means the shader doesn't need any input:
```glsl
#version 450

vec4 positions[3] = vec4[](
    vec4(0.0, -0.5, 0.0, 1.0 ),
    vec4(0.5, 0.5, 0.0, 1.0 ),
    vec4(-0.5, 0.5, 0.0, 1.0 )
);
```
You might wonder why I use 4-dimensional vectors here to define positions in a 3D space. The answer is that the 4th component (the w-component[^1]) facilitates some important mathematical operations that we will want to perform on these coordinates at some point, e.g. transformations and perspective. We'll get into more detail later, as you can see we just set the 4th component to 1 for now.

So we have the coordinates for one triangle defined. These coordinates are already in normalized device coordinates[^2], therefore all we need to do now is tell the shader to output those coordinates to the next stage. Because the main responsibility of a vertex shader is so clearly defined, GLSL actually provides a builtin variable to assign the shader result to: `gl_Position`, which is also a 4-dimensional vector. The only problem is that this only takes one position at a time, as the shader is supposed to process individual vertices. But GLSL has us covered here as well, as there is an internal property that tells us the index of the vertex we're currently processing:
```glsl
void main()
{
    gl_Position = positions[gl_VertexIndex];
}
```
That's already it for our minimal vertex shader. Let's save it in our shaders directory and modify the CMake file to compile that one instead of the compute shader:
```cmake
add_custom_command(
    OUTPUT  "${SHADER_BINARY_DIR}/vertex.vert.spv"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_BINARY_DIR}"
    COMMAND ${Vulkan_GLSLC_EXECUTABLE} "${SHADER_SOURCE_DIR}/vertex.vert" -o "${SHADER_BINARY_DIR}/vertex.vert.spv"
    DEPENDS "${SHADER_SOURCE_DIR}/vertex.vert"
    COMMENT "Compiling vertex shader"
    VERBATIM
)

add_custom_target( shaders DEPENDS "${SHADER_BINARY_DIR}/vertex.vert.spv" )
add_dependencies( ${TARGET_NAME} shaders )
```
Running CMake and building your project should now compile the vertex shader without errors (there will of course be compile errors in `main.cpp` because we modified the function signature of `createGraphicsPipeline`)

Let's move on to the fragment shader now. The first step is to specify the output of the shader:
```glsl
#version 450

layout(location = 0) out vec4 outColor;
```
You might wonder why we need to do that. Isn't there a builtin variable similar to `gl_Position` that takes in the fragment shader output? No there isn't, and for good reason: in a simple graphics pipeline like the one we're going to create at first, the output is indeed always a single 32-bit color value. But there are applications where you might want a different color format. Or, in more complex scenarios, you might want to output to multiple targets at the same time (e.g. not only a color value but also the depth value or texture coordinate for the respective fragment). Bottom line: to enable this kind of flexibility, Vulkan can not make any assumptions about the output of the fragment shader.

Alright, so now we need to fill that output color we defined with a value. We'll again choose the simplest path for now and just always output a pure red:
```glsl
void main()
{
    outColor = vec4(1.0, 0.0, 0.0, 1.0);
}
```
Of course we also have to add the fragment shader to our CMake file[^3]:
```cmake
...
add_custom_command(
    OUTPUT  "${SHADER_BINARY_DIR}/fragment.frag.spv"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_BINARY_DIR}"
    COMMAND ${Vulkan_GLSLC_EXECUTABLE} "${SHADER_SOURCE_DIR}/fragment.frag" -o "${SHADER_BINARY_DIR}/fragment.frag.spv"
    DEPENDS "${SHADER_SOURCE_DIR}/fragment.frag"
    COMMENT "Compiling fragment shader"
    VERBATIM
)

add_custom_target( shaders DEPENDS
    "${SHADER_BINARY_DIR}/vertex.vert.spv"
    "${SHADER_BINARY_DIR}/fragment.frag.spv"
)
```
If you build this version, the compiled fragment shader should be showing up in your output directory as well. However, the compile error in `main.cpp` is of course still there, so let's fix this. Luckily, this is pretty trivial since we've already created the necessary loader function back in lesson 7:
```cpp
const auto vertexShader = vcpp::createShaderModule( logicalDevice, "./shaders/vertex.vert.spv" );
const auto fragmentShader = vcpp::createShaderModule( logicalDevice, "./shaders/fragment.frag.spv" );

const auto pipeline = vcpp::createGraphicsPipeline(
    logicalDevice,
    *vertexShader,
    *fragmentShader
);
```

Okay, we've completed the first step. If you run the program now and watch closely you will find that the first validation error has gone (the others and the exception are still there though). Looks like we're one step closer to a working pipeline, yay!


---

[^1]: The w-component is part of what's called homogeneous coordinates. It enables operations like perspective projection and affine transformations to be represented as matrix multiplications.
[^2]: Normalized device coordinates (NDC) in Vulkan range from -1.0 to 1.0 on both the X and Y axes, with the Y axis pointing downward. These are the coordinates that the GPU uses after all transformations have been applied.
[^3]: In a bigger project with many shaders this explicit adding of a custom command for each shader file will quickly become cumbersome and bloat the CMake file. In that case, a loop iterating over all shader source files would be the better option. For our small demo application the current solution is okay I think.