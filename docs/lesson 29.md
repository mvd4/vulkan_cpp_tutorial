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

There's one thing that still isn't great about this solution, though: our vertex data no longer changes, but we still upload it every frame. There are multiple ways to address this,[^1] but since I want to move on to ambient lighting as soon as possible, I'm going to go for a simple one for now: let's give our vertex data a version and only upload it when that version changes:
```cpp
// rendering.hpp
struct SceneData
{
    std::span< const std::byte > vertexData;
    std::uint32_t vertexCount;
    std::uint32_t vertexDataVersion;
    glm::mat4 mvpMatrix;
};
```

The renderer, in turn, needs to remember which version of the data each of its vertex buffers currently holds. We have one vertex buffer per frame in flight, so a single version number wouldn't be enough: after a change, each of the buffers has to be updated the next time its frame comes around. So we add a vector of versions next to the vector of buffers:
```cpp
// rendering.hpp
class Renderer
{
    ...
private:
    ...
    std::vector< GPUBuffer > m_vertexBuffers;
    std::vector< std::uint32_t > m_vertexDataVersions;
    std::vector< vk::CommandBuffer > m_commandBuffers;
};
```

```cpp
// rendering.cpp
Renderer::Renderer(
    ...
)
    : ...
    , m_vertexBuffers( maxFramesInFlight )
    , m_vertexDataVersions( maxFramesInFlight, 0 )
{
    ...
}
```

With that in place, `drawFrame` only copies the vertex data if it's needed:
```cpp
// rendering.cpp
auto Renderer::drawFrame(
    const SceneData& sceneData,
    const vk::Queue& queue,
    const Swapchain::FrameData& frame,
    const vk::Extent2D& imageExtent
) -> void
{
    ...
    auto& gpuVertexBuffer = m_vertexBuffers[ frame.frameInFlightIndex ];
    bool needsUpload = m_vertexDataVersions[ frame.frameInFlightIndex ] != sceneData.vertexDataVersion;

    if ( gpuVertexBuffer.size < sceneData.vertexData.size() )
    {
        gpuVertexBuffer = createGPUBuffer(
            m_physicalDevice,
            m_logicalDevice,
            sceneData.vertexData.size(),
            vk::BufferUsageFlagBits::eVertexBuffer
        );
        needsUpload = true;
    }

    if ( needsUpload )
    {
        copyDataToBuffer( m_logicalDevice, sceneData.vertexData, gpuVertexBuffer );
        m_vertexDataVersions[ frame.frameInFlightIndex ] = sceneData.vertexDataVersion;
    }

    recordCommandBuffer( ... );
    ...
}
```

A newly created buffer is empty, so it always needs to be filled, no matter which version it's supposed to hold. This also takes care of the very first frame: the stored versions and the version we pass in are both 0, but since all buffers start out with a size of 0, they are created and filled anyway.

The only thing left is to pass a version in `main.cpp`. Since our cube never changes, we can always pass 0:
```cpp
// main.cpp
renderer.drawFrame(
    vcpp::SceneData{
        .vertexData = std::as_bytes( std::span{ vertices } ),
        .vertexCount = vertexCount,
        .vertexDataVersion = 0,
        .mvpMatrix = mvp
    },
    queue,
    frame,
    swapchain->getImageExtent()
);
```

Note that I've switched to *designated initializers*, a C++20 feature that lets us name each member we initialize. The members must be listed in the order in which they're declared, and the compiler checks that for us.

Compile and run this version, and the cube should rotate just as before. But now each vertex buffer is only filled once, instead of in every single frame.


---

[^1]: For example, we could upload the vertex data only once, in a separate function that waits for the GPU to become idle first. Or we could keep the vertex data in device-local memory, which is faster for the GPU to read, and fill it via a staging buffer, as mentioned in lesson 27. We already used staging buffers for our compute shader in lesson 12, and we'll come back to them for vertex data later in the tutorial.
