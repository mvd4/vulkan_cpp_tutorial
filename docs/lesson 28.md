# Lesson 28: Improving the Render Loop - Part 2: Stability and Performance (and yes, some more refactoring)

Alright, now that we've sorted out the fundamental synchronization flaws in our render loop, let's make a few more improvements to it so that we're well prepared for the more interesting things to come, such as lighting and texturing.

## Avoiding busy-waiting while minimized
First, I want to tackle two issues with the way we handle a minimized window. Our current code does it like this:
```cpp
// main.cpp
...
void onFramebufferSizeChanged( [[maybe_unused]] GLFWwindow* window, int width, int height )
{
    windowMinimized = width == 0 && height == 0;
    framebufferSizeChanged = true;
}
...

while ( !glfwWindowShouldClose( window.get() ) )
{
    glfwPollEvents();

    if ( windowMinimized )
        continue;

    ...
}
```

There are two issues here, one bug and one pretty significant shortcoming:

The bug is that we consider the window to be minimized only when both dimensions are 0. If you resize the window so that only one of its dimensions becomes 0, the program will most likely crash because it tries to recreate the swapchain with a zero-sized extent, which is not allowed. Luckily that's trivial to fix, we just need to switch the operator in `onFramebufferSizeChanged`:
```cpp
// main.cpp
...
void onFramebufferSizeChanged( [[maybe_unused]] GLFWwindow* window, int width, int height )
{
    windowMinimized = width == 0 || height == 0;
    framebufferSizeChanged = true;
}
...
```
By considering the window minimized if height _or_ width is 0, we no longer try to create a swapchain in those cases and the crash is gone.

The second issue isn't technically a bug: the code is correct and works. However, because `glfwPollEvents` is non-blocking, we've essentially created a tight loop that burns ~100% of a CPU core doing nothing for as long as the window is minimized. That's not good, but it's fortunately also easy to fix by using another GLFW utility function:
```c
void glfwWaitEvents();
```

This function blocks until new events come in.[^1] So we can improve our loop like this:
```cpp
// main.cpp
while ( !glfwWindowShouldClose( window.get() ) )
{
    glfwPollEvents();

    if ( windowMinimized )
    {
        glfwWaitEvents();
        continue;
    }

    ...
}
```
This makes sure we're not wasting CPU time while minimized. Since `glfwWaitEvents` also processes the events it waits for, our `onFramebufferSizeChanged` callback is called as soon as the window is restored, so the swapchain is recreated correctly and the render loop picks up again.

## Making the animation framerate-independent
In our current implementation, we advance the rotation of the cube by a fixed amount for every frame. The problem is that this directly couples the rotation speed to the framerate, which is in turn synced to the display refresh rate (we're waiting for vsync to show the next frame). In other words: our cube will rotate more than twice as fast on a 144Hz display than it will on a 60Hz display. That's not ideal; we want our animation to be framerate-independent[^2].

The solution is to not add a fixed increment to the rotation angle with each frame, but instead to define a rotation speed and calculate the increment by measuring the time elapsed since the last frame. We do that once per frame, directly after the swapchain recreation block and before we calculate the model matrix:
```cpp
// main.cpp
...
float rotationAngle = 0.f;
constexpr float rotationSpeed = glm::radians( 45.f );
auto lastFrameTime = std::chrono::steady_clock::now();
...
while ( !glfwWindowShouldClose( window.get() ) )
{
    glfwPollEvents();

    if ( windowMinimized )
    {
        ...
    }

    if ( framebufferSizeChanged )
    {
        ...
    }

    const auto now = std::chrono::steady_clock::now();
    const float deltaTime = std::chrono::duration< float >( now - lastFrameTime ).count();
    lastFrameTime = now;
    rotationAngle += deltaTime * rotationSpeed;

    model = glm::rotate( glm::identity< glm::mat4 >(), rotationAngle, glm::vec3{ 0.f, 1.f, 0.f } );
    ...
}
```
As you can see, we're using `std::chrono` for the time measurement. That's the modern C++ way to measure time, and `steady_clock` is guaranteed to be monotonic - it never jumps backwards, unlike `system_clock`, which can be adjusted while our program is running.

This works, but there are two more issues we should consider. First, if the window is minimized or the render loop is blocked for another reason, our `deltaTime` can become pretty large, which would lead to jumps in the animation. A common way to handle this is to clamp the delta to a maximum.

The second issue is that `rotationAngle` currently grows without bound. Because a `float` only has about seven significant decimal digits, the larger the angle gets, the coarser the steps between representable values become. If the program runs long enough, our small per-frame increments will start to get rounded away, and the animation will begin to stutter. Since a rotation by 2π is the same as no rotation at all, we can simply address this by wrapping the angle back into the range [0, 2π). So let's apply both fixes:
```cpp
// main.cpp
...
const float deltaTime = std::min( std::chrono::duration< float >( now - lastFrameTime ).count(), 0.1f );
lastFrameTime = now;
rotationAngle = std::fmod( rotationAngle + deltaTime * rotationSpeed, glm::two_pi< float >() );
...
```

And with this, our animation is framerate-independent and robust against stalls.

## Avoiding pipeline recreation on resize
One more shortcoming of the current implementation is that we need to drop and recreate the whole pipeline whenever the window size (or, to be more exact, the framebuffer/swapchain size) changes. Pipeline recreation can be expensive, and structurally nothing changes when the viewport is resized. The configuration of the pipeline's stages, the shaders, the vertex input layout - it all stays the same.

It would therefore be much better if we could just communicate the new values of width and height to the existing pipeline instead of tearing it down entirely and rebuilding it. The way to do this in Vulkan is to turn the viewport and scissor into so-called *dynamic state*[^3]: the pipeline declares only that they exist, while the actual values are recorded into the command buffer each frame. Let's do that.

The first step is to remove the viewport and scissor definition from our pipeline creation function. Instead of setting the viewport and scissor directly, we just set a count to let the pipeline know how many viewports and scissors to expect and declare them dynamic state. The setup code for viewport and scissor moves to `rendering.cpp`, because the pipeline now expects the dynamic state to be passed with every command buffer:
```cpp
// pipelines.cpp
auto createGraphicsPipeline(
    const vk::Device& logicalDevice,
    const vk::PipelineLayout& pipelineLayout,
    const vk::ShaderModule& vertexShader,
    const vk::ShaderModule& fragmentShader,
    const vk::RenderPass& renderPass,
    std::span< const vk::Format > vertexFormats
) -> vk::UniquePipeline
{
    ...
    // move the existing viewport and scissor creation to recordCommandBuffer, see below
    const auto viewportState = vk::PipelineViewportStateCreateInfo{}
        .setViewportCount( 1 )
        .setScissorCount( 1 );

    ...
    const auto dynamicStates = std::array< vk::DynamicState, 2 >{
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };
    const auto dynamicState = vk::PipelineDynamicStateCreateInfo{}
        .setDynamicStates( dynamicStates );

    const auto pipelineCreateInfo = vk::GraphicsPipelineCreateInfo{}
        .setStages( shaderStageInfos )
        .setPDynamicState( &dynamicState )
    ...
}
```

```cpp
// rendering.cpp
auto recordCommandBuffer(
    const vk::CommandBuffer& commandBuffer,
    const vk::Pipeline& pipeline,
    const vk::RenderPass& renderPass,
    const vk::Framebuffer& frameBuffer,
    const vk::Extent2D& renderExtent,
    const vk::Buffer& vertexBuffer,
    std::uint32_t vertexCount
) -> void
{
    ...

    commandBuffer.bindPipeline( vk::PipelineBindPoint::eGraphics, pipeline );

    const auto viewport = vk::Viewport{}
        .setX( 0.f )
        .setY( 0.f )
        .setWidth( static_cast< float >( renderExtent.width ) )
        .setHeight( static_cast< float >( renderExtent.height ) )
        .setMinDepth( 0.f )
        .setMaxDepth( 1.f );
    commandBuffer.setViewport( 0, viewport );

    const auto scissor = vk::Rect2D{ { 0, 0 }, renderExtent };
    commandBuffer.setScissor( 0, scissor );

    commandBuffer.bindVertexBuffers( 0, vertexBuffer, vk::DeviceSize{ 0 } );
    ...
}
```

`vk::PipelineDynamicStateCreateInfo` is simple enough that it doesn't need much explanation: it just holds the list of states we want to set dynamically, which we pass via `setDynamicStates`. Note that I also removed the `viewportExtent` parameter from `createGraphicsPipeline`, as we don't need it for pipeline creation anymore, so you'll have to adjust the declaration and the function call.

If you compile and run this version now, everything should work as before. However, we're not reaping the benefits of our refactoring yet, as the pipeline is still destroyed and recreated whenever the window size changes. Fixing that is the last step.

To do so, we move the pipeline creation in front of the render loop and remove all pipeline-related code from the `if ( framebufferSizeChanged )` block. Your `main` function should now look something like this:

```cpp
// main.cpp
...
auto main() -> int
{
    constexpr int windowWidth = 800;
    constexpr int windowHeight = 600;
    constexpr std::uint32_t requestedSwapchainImageCount = 2u;
    constexpr std::uint32_t maxFramesInFlight = 2u;

    try
    {
        ...
        const auto pipeline = vcpp::createGraphicsPipeline(
            logicalDevice,
            *pipelineLayout,
            *vertexShader,
            *fragmentShader,
            *renderPass,
            vertexFormats
        );

        ...

        while ( !glfwWindowShouldClose( window.get() ) )
        {
            glfwPollEvents();

            if ( windowMinimized )
            {
                glfwWaitEvents();
                continue;
            }

            if ( framebufferSizeChanged )
            {
                logicalDevice->waitIdle();

                swapchain.reset();

                const auto capabilities = physicalDevice.getSurfaceCapabilitiesKHR( *surface );
                swapchainExtent = capabilities.currentExtent;

                swapchain = vcpp::createSwapchain(
                    physicalDevice,
                    logicalDevice,
                    *renderPass,
                    *surface,
                    surfaceFormats[0],
                    swapchainExtent,
                    maxFramesInFlight,
                    requestedSwapchainImageCount
                );

                view = glm::translate( glm::identity< glm::mat4 >(), glm::vec3{ 0.f, 0.f, -3.f } );

                projection = glm::perspective(
                    glm::radians( 30.0f ),
                    swapchainExtent.width / static_cast< float >( swapchainExtent.height ),
                    0.1f,
                    10.0f
                );

                framebufferSizeChanged = false;
            }

            ...
        }

        logicalDevice->waitIdle();
    }
    catch( const std::exception& e )
    {
        std::cerr << "Exception thrown: " << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
```

Compile and run this version, play around with resizing and minimizing the window. You shouldn't experience any errors or changes in behavior. Under the hood, however, resizing has become considerably cheaper: only the swapchain and its framebuffers are recreated now, while the pipeline lives for the entire lifetime of the program.

## Pulling the drawing code out of `main()`
Our `main()` function has become pretty long and somewhat cluttered again, so the last thing I want to do before calling it a day is to extract parts of the rendering code and put them into their own function. The inner try-catch block seems like a good candidate.

Before we naively pull code out, though, let's take a step back and look at the data that is involved in rendering and presenting a scene on screen. We can group it into three categories:
1. The scene data itself, i.e. everything that describes *what* we want to draw. At this point, this is pretty much our vertex data, but in the future it will encompass the transformations, the lighting data, etc. These are the things that potentially change frame by frame, and they are independent of how the pipeline actually processes them and whether and how they are finally presented to the user.
2. The rendering pipeline. This includes everything that is needed to process the scene data and create a 2D image out of it. This normally doesn't change at all after it has been created, at least not in a simple tutorial like ours.
3. The swapchain, which is responsible for presenting the rendered image on screen. As we've already established, this changes whenever the user resizes the window.

We already have an abstraction for the third category: our `Swapchain` class. Let's look at the other two. As mentioned, in the first category we currently only have the vertex data. Let's nevertheless put that data into an explicit data structure:
```cpp
// rendering.hpp
struct SceneData
{
    std::span< const std::byte > vertexData;
    std::uint32_t vertexCount;
};
```
As you can see, I took the opportunity to make the data structure independent of the concrete vertex format and the way we store it on the host. Thanks to the `span`, the data can live in a `std::array`, a `std::vector`, a plain C array or any other contiguous container. Getting a `span< const std::byte >` from typed data is a one-liner: `std::as_bytes( std::span{ vertices } )`. Keep in mind, though, that a `span` doesn't own the data it refers to, so the underlying storage has to outlive the `SceneData` that points to it. And since the pipeline needs to be informed about how to interpret the data anyway, we don't need to duplicate that information here. We could theoretically also derive the vertex count from the size of the buffer and the vertex formats, but I think it's better to keep track of it explicitly.

Now let's look at the second category. Wrapping every resource related to the rendering pipeline into a struct might yield something like this:
```cpp
// rendering.hpp
struct Renderer
{
    vk::UniqueRenderPass renderPass;
    vk::UniquePipelineLayout pipelineLayout;
    vk::UniquePipeline pipeline;
    vk::UniqueCommandPool commandPool;

    std::vector< GPUBuffer > vertexBuffers;
    std::vector< vk::CommandBuffer > commandBuffers;
};
```

However, since some of the members actually depend on other members for their initialization (e.g. `pipeline` depending on `pipelineLayout`), we can't initialize this struct in place. Creating the handles beforehand and moving them into the struct would also be tricky and error-prone. So let's give our `Renderer` a constructor:

```cpp
// rendering.cpp
Renderer::Renderer(
    const vk::PhysicalDevice& physicalDevice,
    const LogicalDevice& logicalDevice,
    const vk::ShaderModule& vertexShader,
    const vk::ShaderModule& fragmentShader,
    vk::Format colorFormat,
    std::span< const vk::Format > vertexFormats,
    std::uint64_t sizeVertexDataInBytes,
    std::uint32_t maxFramesInFlight
)
    : renderPass{ createRenderPass( logicalDevice, colorFormat ) }
    , pipelineLayout{ createGraphicsPipelineLayout( logicalDevice ) }
    , pipeline{ createGraphicsPipeline(
        logicalDevice,
        *pipelineLayout,
        vertexShader,
        fragmentShader,
        *renderPass,
        vertexFormats
    ) }
    , commandPool{ logicalDevice->createCommandPoolUnique(
        vk::CommandPoolCreateInfo{}
            .setFlags( vk::CommandPoolCreateFlagBits::eResetCommandBuffer )
            .setQueueFamilyIndex( logicalDevice.queueFamilyIndex )
    ) }
{
    for ( std::uint32_t f = 0; f < maxFramesInFlight; ++f )
    {
        vertexBuffers.push_back( createGPUBuffer(
            physicalDevice,
            logicalDevice,
            sizeVertexDataInBytes,
            vk::BufferUsageFlagBits::eVertexBuffer
        ) );
    }

    const auto commandBufferAllocateInfo = vk::CommandBufferAllocateInfo{}
        .setCommandPool( *commandPool )
        .setLevel( vk::CommandBufferLevel::ePrimary )
        .setCommandBufferCount( maxFramesInFlight );
    commandBuffers = logicalDevice->allocateCommandBuffers( commandBufferAllocateInfo );
}
```

I'll leave the necessary changes in `main.cpp` to make use of this struct as an exercise for you and not show them here. In short, you need to:
- remove the creation of the render pass, the pipeline layout, the pipeline, the vertex buffers, the command pool and the command buffers from `main()`
- create a `Renderer` instance instead, somewhere before the render loop
- use the `renderer`'s members wherever the removed objects were used before

If you compile and run this version, everything should work as before.

Alright, this helped us move quite a bit of code that was scattered across our `main()` function to a more structured place, which is good. But the actual drawing code still resides in `main()`, and moving that out was the whole point of the exercise. So let's finally do that and create a `drawFrame` function:

```cpp
// rendering.cpp
auto drawFrame(
    const vk::Device& logicalDevice,
    const SceneData& sceneData,
    const Renderer& renderer,
    const vk::Queue& queue,
    const Swapchain::FrameData& frame,
    const vk::Extent2D& imageExtent
) -> void
{
    const auto& gpuVertexBuffer = renderer.vertexBuffers[ frame.frameInFlightIndex ];

    copyDataToBuffer( logicalDevice, sceneData.vertexData, gpuVertexBuffer );

    recordCommandBuffer(
        renderer.commandBuffers[ frame.frameInFlightIndex ],
        *renderer.pipeline,
        *renderer.renderPass,
        frame.framebuffer,
        imageExtent,
        *gpuVertexBuffer.buffer,
        sceneData.vertexCount
    );

    const vk::PipelineStageFlags waitStages[] = {
        vk::PipelineStageFlagBits::eColorAttachmentOutput };
    const auto submitInfo = vk::SubmitInfo{}
        .setCommandBuffers( renderer.commandBuffers[ frame.frameInFlightIndex ] )
        .setWaitSemaphores( frame.readyForRenderingSemaphore )
        .setSignalSemaphores( frame.readyForPresentingSemaphore )
        .setWaitDstStageMask( waitStages );

    queue.submit( submitInfo, frame.inFlightFence );
}
```

For this code to work, we need to add an overload to `copyDataToBuffer` that accepts a span. While we're at it, we can actually implement the array-based implementation with a forwarding call to this new function:
```cpp
// memory.hpp
auto copyDataToBuffer(
    const vk::Device& logicalDevice,
    std::span< const std::byte > data,
    const GPUBuffer& buffer
) -> void;

template< typename T, size_t N >
auto copyDataToBuffer(
    const vk::Device& logicalDevice,
    const std::array< T, N >& data,
    const GPUBuffer& buffer
) -> void
{
    copyDataToBuffer( logicalDevice, std::as_bytes( std::span{ data } ), buffer );
}
```

Replacing the drawing code in `main()` with a call to `drawFrame` does indeed make the loop a bit shorter and more readable. But we can do better: the function call is at a different level of abstraction than the presentation code that follows it, which is something I try to avoid. So let's move the presenting code to the swapchain, along with the swapchain extent (which I should have put there in the first place):
```cpp
// presentation.hpp
class Swapchain
{
public:
    ...
    auto getImageExtent() const -> vk::Extent2D { return m_imageExtent; }

    auto getNextFrame() -> FrameData;
    auto present(
        const vk::Queue& queue,
        const FrameData& frame
    ) -> vk::Result;

private:

    vk::Device m_logicalDevice;
    vk::UniqueSwapchainKHR m_swapchain;

    vk::Extent2D m_imageExtent;
    ...
};
```

```cpp
// presentation.cpp
auto Swapchain::present(
    const vk::Queue& queue,
    const FrameData& frame
) -> vk::Result
{
    const auto swapchains = std::array< vk::SwapchainKHR, 1 >{ *m_swapchain };
    const auto presentInfo = vk::PresentInfoKHR{}
        .setSwapchains( swapchains )
        .setImageIndices( frame.swapchainImageIndex )
        .setWaitSemaphores( frame.readyForPresentingSemaphore );

    try
    {
        return queue.presentKHR( presentInfo );
    }
    catch ( const vk::OutOfDateKHRError& )
    {
        // the swapchain no longer matches the surface and needs to be recreated
        return vk::Result::eErrorOutOfDateKHR;
    }
}
```

With that, the try-catch block in the main loop is reduced to:
```cpp
// main.cpp
try
{
    const auto frame = swapchain->getNextFrame();

    vcpp::drawFrame(
        logicalDevice,
        vcpp::SceneData{
            std::as_bytes( std::span{ verticesTemp } ),
            vertexCount
        },
        renderer,
        queue,
        frame,
        swapchain->getImageExtent()
    );

    const auto result = swapchain->present( queue, frame );
    if ( result != vk::Result::eSuccess )
        framebufferSizeChanged = true;
}
catch ( const vk::OutOfDateKHRError& )
{
    // the swapchain no longer matches the surface and needs to be recreated
    framebufferSizeChanged = true;
}
```

...which, I think, is much better than before[^4]


---

[^1]: If your application still needs to do work while the window is minimized - background tasks, network I/O, animations that should keep running - you can use `glfwWaitEventsTimeout( double timeout )` instead. It behaves just like `glfwWaitEvents`, but returns after at most `timeout` seconds so you get a chance to do periodic work.
[^2]: In fact, we'd ideally want to decouple the *simulation* entirely from the *rendering*, but that's out of scope for this lesson.
[^3]: See the [Dynamic State](https://docs.vulkan.org/spec/latest/chapters/pipelines.html#pipelines-dynamic-state) section of the Vulkan specification for the full list of states that can be made dynamic.
[^4]: We still have a few asymmetries that I'd want to fix eventually, but for now I think it's good enough. Also note that I had to change the result-check to `!=eSuccess`, because the function can now return different result codes instead of throwing.
