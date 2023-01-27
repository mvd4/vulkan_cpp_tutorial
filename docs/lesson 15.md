# Creating the Graphics Pipeline - Part 1
Alright, our task for today is to start filling the `GraphicsPipelineCreateInfo` structure with information on how we want our graphics pipeline to be configured. So let's dive straight in:

## Setting up the pipeline stages
As mentioned in the last lesson, we can ignore the flags, so let's start with the `setStages` function. That one takes a container of `PipelineShaderStageCreateInfo`, which we already looked at back in lesson 8. The difference is that now we need at least a vertex and a fragment shader, so let's prepare our pipeline creation for that:
```
vk::UniquePipeline create_graphics_pipeline(
    const vk::Device& logicalDevice,
    const vk::ShaderModule& vertexShader,
    const vk::ShaderModule& fragmentShader
)
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
```
#version 450

vec4 positions[3] = vec4[](
    vec4(0.0, -0.5, 0.0, 1.0 ),
    vec4(0.5, 0.5, 0.0, 1.0 ),
    vec4(-0.5, 0.5, 0.0, 1.0 )
);
```
You might wonder why I use 4-dimensional vectors here to define positions in a 3D space. The answer is that the 4th component (the w-component) facilitates some important mathematical operations that we will want to perform on these coordinates at some point, e.g. transformations and perspective. We'll get into more detail later, as you can see we just set the 4th component to 1 for now.

So we have the coordinates for one triangle defined. These coordinates are already in normalized world space, therefore all we need to do now is tell the shader to output those coordinates to the next stage. Because the main responsibility of a vertex shader is so clearly defined, GLSL actually provides a builtin variable to assign the shader result to: `gl_Position`, which is also a 4-dimensional vector. The only problem is that this only takes one position at a time, as the shader is supposed to process individual vertices. But GLSL has us covered here as well, as there is an internal property that tells us the index of the vertex we're currently processing:
```
void main()
{
    gl_Position = positions[gl_VertexIndex];
}
```
That's already it for our minimal vertex shader. Let's save it in our shaders directory and modify the CMake file to compile that one instead of the compute shader:
```
add_custom_target( vertex_shader
    COMMAND             "glslc" "${VULKAN_TUTORIAL_PROJECT_ROOT}/shaders/vertex.vert" -o "shaders/vertex.spv"
    WORKING_DIRECTORY   "${CMAKE_BINARY_DIR}/bin"
)
```
We also need to change the dependency for our project accordingly:
```
add_dependencies( ${PROJECT_NAME} vertex_shader )
```

Running CMake and building your project should now compile the vertex shader without errors (there will of course be compile errors in `main.cpp` because we modified the function signature of `create_graphics_pipeline`)

Let's move on to the fragment shader now. The first step is to specify the output of the shader:
```
#version 450

layout(location = 0) out vec4 outColor;
```
You might wonder why we need to do that. Isn't there a builtin variable similar to `gl_Position` that takes in the fragment shader output? No there isn't, and for good reason: in a simple graphics pipeline like the one we're going to create at first, the output is indeed always a single 32-bit color value. But there are applications where you might want a different color format. Or, in more complex scenarios, you might want to output to multiple targets at the same time (e.g. not only a color value but also the depth value or texture coordinate for the respective fragment). Bottom line: to enable this kind of flexibility, Vulkan can not make any assumptions about the output of the fragment shader.

Alright, so now we need to fill that output color we defined with a value. We'll again choose the simplest path for now and just always output a pure red:
```
void main() 
{
    outColor = vec4(1.0, 0.0, 0.0, 1.0);
}
```
Add the shader to your CMake file in the same way as we did for the vertex shader and compile your project. It should work as before, only that the compiled fragment shader is now showing up in your output directory.

Alright, now we can load the shaders into our application and pass them to our pipeline creation function. This is trivial since we've already created the necessary loader function back in lesson 7:
```
const auto vertexShader = create_shader_module( logicalDevice, "./shaders/vertex.spv" );
const auto fragmentShader = create_shader_module( logicalDevice, "./shaders/fragment.spv" );

const auto pipeline = create_graphics_pipeline( 
    logicalDevice,
    *vertexShader,
    *fragmentShader );
```

Okay, we've completed the first step. If you run the program now and watch closely you will find that the first validation error has gone (the others and the exception are still there though). Looks like we're one step closer to a working pipeline, yay!

## Vertex input and input assembly state
One additional (temporary) advantage of defining the vertices in the shader is that we can pretty much ignore the `PipelineVertexInputStateCreateInfo` parameter because we don't pass in any vertices yet. Vulkan requires us to set this parameter though, so we'll have to give it the pointer to a default constructed instance.

Next thing we want to set is the `pInputAssemblyState_`. As described last time, this stage essentially prepares the input for the vertex shader stage. It's a fixed stage and we actually cannot control that many parameters for it:
```
struct PipelineInputAssemblyStateCreateInfo
{
    ...
    PipelineInputAssemblyStateCreateInfo& setFlags( PipelineInputAssemblyStateCreateFlags flags_ );
    PipelineInputAssemblyStateCreateInfo& setTopology( PrimitiveTopology topology_ );
    PipelineInputAssemblyStateCreateInfo& setPrimitiveRestartEnable( Bool32 primitiveRestartEnable_ );
    ...
};
```
- The `flags_` are once more only reserved for future use. 
- The `topology_` defines how to combine the vertices to primitives, i.e. to geometric shapes to be drawn. The possible values include:
  - `ePointList` if you want to draw single points
  - `eTriangleList` if you want to draw draw individual triangles
  - `eTriangleStrip` if you want to draw a series of triangles where every triangle shares two vertices with the previous one
  - `eTriangleFan` if you want to draw a series of triangles that all share one vertex
  - ... and several more
- `primitiveRestartEnable_` is only relevant when doing indexed drawing, something we'll look into later. At this point we just ignore it.

Since we are only drawing a single triangle for now, it doesn't really matter which of the `eTriangle...` topologies we set, `eTriangleList` seems like the most generic so let's just use that one.

And with that our pipeline creation function looks like that now:
```
vk::UniquePipeline create_graphics_pipeline(
    const vk::Device& logicalDevice,
    const vk::ShaderModule& vertexShader,
    const vk::ShaderModule& fragmentShader
)
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

    const auto vertexInputState = vk::PipelineVertexInputStateCreateInfo{};
    const auto inputAssemblyState = vk::PipelineInputAssemblyStateCreateInfo{}
        .setTopology( vk::PrimitiveTopology::eTriangleList );
    
    const auto pipelineCreateInfo = vk::GraphicsPipelineCreateInfo{}
        .setStages( shaderStageInfos )
        .setPVertexInputState( &vertexInputState )
        .setPInputAssemblyState( &inputAssemblyState );

    return logicalDevice.createGraphicsPipelineUnique( vk::PipelineCache{}, pipelineCreateInfo ).value;
}
```
If you compile and run this version, you'll still get the validation warnings and the crash, which tells us that we're by far not done yet. Nevertheless I want to end here for today and continue next time.
