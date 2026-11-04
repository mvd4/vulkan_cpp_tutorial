# Lesson 29: Push Constants and Ambient Lighting

As promised, after all the refactoring and bugfixing toil of the last two lessons, we're going to make some visible changes again today. I want to lay the groundwork for a lighting system, beginning with ambient light. For that, I'm going to introduce a concept called *push constants*. And while we're at it, I also want to use that technique to make one more improvement to our render loop.

## Push constants
Let's start with that improvement: One thing that still bothers me about our render loop is that we multiply each individual vertex by the transformation matrices on the CPU before uploading the whole buffer to the GPU every single frame. That seems very wasteful. All the more so since our geometry never changes, only the transformation matrices do. And with its massive parallelization capabilities, the GPU would be much better suited to transforming the vertices.

So ideally, we'd upload the vertex buffer only when it changes and otherwise just send the new transformation matrices to the GPU each frame to let it do the work.

Push constants are a technique that allows us to do that. They are the fastest, smallest way to hand a shader a few bytes of data that may change for every draw. They don't require us to handle buffers, descriptors, or synchronization - the driver just ships them inline with the command buffer.

So let's give that a try. To use push constants, we have to define a `PushConstantRange`. This struct is attached to the pipeline layout and essentially tells the pipeline which shader stage(s) may read which bytes of the push constant data.

The data itself is completely opaque to the pipeline; it only knows that there is a block of data of a certain size that will be passed each time the command buffer is recorded. The interface of `PushConstantRange` looks like this:
```cpp
struct PushConstantRange
{
    ...
    PushConstantRange& setStageFlags( ShaderStageFlags stageFlags_ );
    PushConstantRange& setOffset( uint32_t offset_ );
    PushConstantRange& setSize( uint32_t size_ );
    ...
}
```

- `stageFlags_` is a bitmask of the shader stages that may access this range.
- `offset_` and `size_` define the range within the whole push constant data block. Both must be multiples of 4.

A pipeline layout can define multiple ranges (e.g. one for the vertex stage, another one for the fragment stage), but there is only one push constant data block that is shared by all ranges. So all `PushConstantRange` declarations are views into the same block of data and must therefore be compatible with each other.


## Moving vertex transformations to the GPU
Let's put this into practice now by modifying our `createGraphicsPipelineLayout` function:
```cpp
// pipelines.cpp
auto createGraphicsPipelineLayout( const vk::Device& logicalDevice ) -> vk::UniquePipelineLayout
{
    const auto range = vk::PushConstantRange{}
        .setStageFlags( vk::ShaderStageFlagBits::eVertex )
        .setOffset( 0 )
        .setSize( sizeof( glm::mat4 ) );

    const auto info = vk::PipelineLayoutCreateInfo{}
        .setPushConstantRanges( range );

    return logicalDevice.createPipelineLayoutUnique( info );
}
```

We want to move the vertex transformation to the GPU, so the stage that needs the push constant data is obviously the vertex stage.

You might wonder why we set the size of the range to the size of only one matrix and not three of them. Don't we want to send three matrices? Well, no. First, as the vertex shader runs once for every vertex, we'd then do the same multiplication of model, view and projection matrix once for each vertex. That's a waste of resources. Second, the maximum size of the whole push constant data block is only guaranteed to be at least 128 bytes by the Vulkan spec. A 4x4 float matrix occupies 64 bytes, so we couldn't reliably send three of them using this technique. Instead, we'll have to multiply them on the CPU and send the combined model-view-projection (MVP) matrix to the pipeline.

Now that the pipeline layout declares the push constant range, we need to actually push the data while recording the command buffer. To do that, our `recordCommandBuffer` function needs the MVP matrix, and also the pipeline layout. Simply adding those to the parameter list would increase the parameter count of the function to 9. That's getting a bit too much, and since some of those parameters are actually members of the `Renderer` class, I'm going to turn `recordCommandBuffer` into a private member function of `Renderer`. I'm also going to add the MVP matrix to our `SceneData` struct - the transformation matrix is clearly part of the scene data that might change with every frame. A neat side effect is that we don't have to change anything about the function signature of `Renderer::drawFrame`:
```cpp
// rendering.hpp
struct SceneData
{
    std::span< const std::byte > vertexData;
    std::uint32_t vertexCount;
    glm::mat4 mvpMatrix;
};
```

```cpp
// rendering.cpp
auto Renderer::recordCommandBuffer(
    const vk::CommandBuffer& commandBuffer,
    const vk::Framebuffer& frameBuffer,
    const vk::Extent2D& renderExtent,
    const vk::Buffer& vertexBuffer,
    const SceneData& sceneData
) const -> void
{
    ...  // same as before, replace renderPass with *m_renderPass

    commandBuffer.beginRenderPass( renderPassBeginInfo, vk::SubpassContents::eInline );

    commandBuffer.bindPipeline( vk::PipelineBindPoint::eGraphics, *m_pipeline );

    commandBuffer.pushConstants< glm::mat4 >(
        *m_pipelineLayout,
        vk::ShaderStageFlagBits::eVertex,
        0,
        sceneData.mvpMatrix
    );

    const auto viewport = vk::Viewport{}
    ...  // same as before, replace vertexCount with sceneData.vertexCount
}
```

The rest of the changes in `rendering.cpp` and `rendering.hpp` are trivial, so I'm going to skip those here. In `main.cpp` we can now remove `verticesTemp` and with that the loop that multiplies each vertex by the MVP matrix. Instead, we pass the matrix to `Renderer::drawFrame` as part of `SceneData`:
```cpp
// main.cpp
...
model = glm::rotate( glm::identity< glm::mat4 >(), rotationAngle, glm::vec3{ 0.f, 1.f, 0.f } );
const auto mvp = projection * view * model;

// remove the loop here

try
{
    const auto frame = swapchain->getNextFrame();

    renderer.drawFrame(
        vcpp::SceneData{
            std::as_bytes( std::span{ vertices } ),
            vertexCount,
            mvp
        },
        queue,
        frame,
        swapchain->getImageExtent()
    );
...
```

This should now compile and run fine, but it won't really work yet - you will see a red rectangle that doesn't move at all. This is because while we're passing the MVP matrix to the pipeline, the vertex shader doesn't know anything about it yet, let alone do anything with it. So let's inform it about the push constants:
```glsl
// vertex.vert
...

layout( push_constant ) uniform PushConstants
{
    mat4 mvp;
} pushConstants;

void main()
{
    gl_Position = pushConstants.mvp * inPosition;
    outColor = inColor;
}
```

The layout declaration informs the shader about the push constant layout to expect, in our case only one 4x4 matrix. The block name (`PushConstants`) is required by the GLSL interface block grammar, but isn't used for matching the data: there can only be one push constant block per shader stage, and its members are matched to the pushed bytes by their offsets. That's why the shader's layout must agree with the `PushConstantRange`. Note that GLSL matrices are column-major by default, just like GLM's, so we can pass the matrix as it is without transposing it. The instance name (`pushConstants`) is optional, but it lets us qualify the members when using them and thus makes the code a bit more readable.

Compile and run this version: you should see the rotating cube again.