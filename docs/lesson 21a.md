# Another round of refactoring
Before we address the remaining stability issues with our render pipeline I want to do another round of refactoring. This is going to make our work considerably more easy moving forward.

One obvious shortcoming of the code in and around our render loop is that it is pretty cluttered and way less declarative than we would like it to be. There is a lot of detail work happening on the top level which makes it harder than necessary to follow the program flow. There are also a number of non-obvious invariants that make the code brittle and error-prone. So let's see what we can do about this.

A first thing to notice is that all synchronization primitives are tied to the number of images in flight and are addressed by the same index. That means we can either model it as an array of structs or a struct of arrays. I chose the latter one because it's closer to what we have right now. So let's move all our collections of synchronization objects to a struct:

```C++ 
// presentation.hpp

struct swapchain_sync
{
    swapchain_sync( const vk::Device& logicalDevice, std::uint32_t maxImagesInFlight );

    std::vector< vk::UniqueFence > inFlightFences;
    std::vector< vk::UniqueSemaphore > readyForRenderingSemaphores;
    std::vector< vk::UniqueSemaphore > readyForPresentingSemaphores;
};
```

```C++
// presentation.cpp

swapchain_sync::swapchain_sync( const vk::Device& logicalDevice, std::uint32_t maxImagesInFlight )
{
    for( std::uint32_t i = 0; i < maxImagesInFlight; ++i )
    {
        inFlightFences.push_back( logicalDevice.createFenceUnique(
            vk::FenceCreateInfo{}.setFlags( vk::FenceCreateFlagBits::eSignaled )
        ) );

        readyForRenderingSemaphores.push_back( logicalDevice.createSemaphoreUnique(
            vk::SemaphoreCreateInfo{}
        ) );

        readyForPresentingSemaphores.push_back( logicalDevice.createSemaphoreUnique(
            vk::SemaphoreCreateInfo{}
        ) );
    }
}
```

I'm not showing the required changes in `main` here because that would blow up the tutorial too much and it's going to change several times anyway. And in any case, so far it is debatable whether the code in `main` actually has improved. The loop to create the synchronization objects is gone, but we now have an additional level of indirection to access them. Looks like we need to do better to make the refactoring worthwhile.

Let's see. In each iteration of our render loop we're accessing the element with the same index from all the containers. So we would actually not need access to the containers themselves at all, we only need the respective synchronization objects for the current frame. And it would also be much less error prone if `swapchain_sync` would just make sure we use the right semaphores and fence. So we're going to move the whole frame-in-flight logic into our new struct:

```C++
class swapchain_sync
{
public:

    struct frame_sync
    {
        const vk::Fence& inFlightFence; 
        const vk::Semaphore& readyForRenderingSemaphore;
        const vk::Semaphore& readyForPresentingSemaphore;
    };


    swapchain_sync( const vk::Device& logicalDevice, std::uint32_t maxImagesInFlight );

    frame_sync get_next_frame_sync();

private:

    std::uint32_t m_maxImagesInFlight;
    std::uint32_t m_currentFrameIndex = 0;

    std::vector< vk::UniqueFence > m_inFlightFences;
    std::vector< vk::UniqueSemaphore > m_readyForRenderingSemaphores;
    std::vector< vk::UniqueSemaphore > m_readyForPresentingSemaphores;
};
```

Since it now has invariants to maintain, I followed C++ best practice and made our struct a class(1). The implementation of `get_next_frame_sync` is straightforward:

```C++
swapchain_sync::frame_sync swapchain_sync::get_next_frame_sync()
{
    const auto result = frame_sync{
        *m_inFlightFences[ m_currentFrameIndex ],
        *m_readyForRenderingSemaphores[ m_currentFrameIndex ],
        *m_readyForPresentingSemaphores[ m_currentFrameIndex ]
    };

    m_currentFrameIndex = ++m_currentFrameIndex % m_maxImagesInFlight;
    return result;
}
```

With that we can remove most uses of the subscript operators from our render loop, making it quite a bit more concise. However, we still need to select the correct command buffer so that we now have to maintain the `frameInFlightIndex` in two different locations. That's not good, let's change it by adding the index to the `frame_sync` struct. I'm not going to show the code here since that change is almost trivial.

We have now encapsulated the synchronization objects in our `swapchain_sync` class, which already helped making the code in `main` significantly more declarative. Moving on in the same spirit: wouldn't it make sense to do the same for the framebuffers and images? For those we also do not need access to the whole collection, we're only ever interested in the framebuffer that we are currently processing. Let's see what happens when we add them to our class as well:

```C++
class swapchain_state
{
public:

    ...

    swapchain_state(
        const vk::Device& logicalDevice,
        const vk::SwapchainKHR& swapchain,
        const vk::RenderPass& renderPass,
        const vk::Extent2D& imageExtent,
        const vk::Format& imageFormat,
        std::uint32_t maxImagesInFlight );

    ...

private:

    std::uint32_t m_maxImagesInFlight;
    std::uint32_t m_currentFrameIndex = 0;

    std::vector< vk::UniqueImageView > m_imageViews;
    std::vector< vk::UniqueFramebuffer > m_framebuffers;

    std::vector< vk::UniqueFence > m_inFlightFences;
    std::vector< vk::UniqueSemaphore > m_readyForRenderingSemaphores;
    std::vector< vk::UniqueSemaphore > m_readyForPresentingSemaphores;
};
```

The name swapchain_sync would no longer reflect the classes purpose, so I renamed it. Moving the framebuffers and image views into the class means that we also need to pass all the arguments necessary to create them to our constructor. In its implementation we can make use of the functions that we already had in place to create the framebuffers and image views:

```C++
swapchain_state::swapchain_state(
    const vk::Device& logicalDevice,
    const vk::SwapchainKHR& swapchain,
    const vk::RenderPass& renderPass,
    const vk::Extent2D& imageExtent,
    const vk::Format& imageFormat,
    std::uint32_t maxImagesInFlight
)
    : m_maxImagesInFlight{ maxImagesInFlight }
    , m_imageViews{ create_swapchain_image_views( logicalDevice, swapChain, imageFormat ) }
    , m_framebuffers{ create_framebuffers( logicalDevice, m_imageViews, imageExtent, renderPass ) }
{
    for( std::uint32_t i = 0; i < maxImagesInFlight; ++i )
    {
        m_inFlightFences.push_back( logicalDevice.createFenceUnique(
            vk::FenceCreateInfo{}.setFlags( vk::FenceCreateFlagBits::eSignaled )
        ) );

        m_readyForRenderingSemaphores.push_back( logicalDevice.createSemaphoreUnique(
            vk::SemaphoreCreateInfo{}
        ) );

        m_readyForPresentingSemaphores.push_back( logicalDevice.createSemaphoreUnique(
            vk::SemaphoreCreateInfo{}
        ) );
    }
}
```

In our render loop we need access to the current framebuffer, so we should return it alongside the synchronization objects and index. We get a problem in the implementation of `get_next_frame_sync` though: to determine the correct framebuffer we need the swapchain image index. Simply passing that in as a parameter wouldn't work because we need the `readyForRenderingSemaphore` to pass to `acquireNextImageKHR`. So we'd end up in a chicken-egg situation. The only solution is to acquire the image index within the function. That means we end up with this class declaration:

```C++
class swapchain
{
public:

    struct frame_data
    {
        std::uint32_t swapchainImageIndex;
        std::uint32_t inFlightIndex;
        
        const vk::Framebuffer& framebuffer;

        const vk::Fence& inFlightFence; 
        const vk::Semaphore& readyForRenderingSemaphore;
        const vk::Semaphore& readyForPresentingSemaphore;
    };

    swapchain( 
        const vk::Device& logicalDevice, 
        const vk::RenderPass& renderPass,
        const vk::SurfaceKHR& surface,
        const vk::SurfaceFormatKHR& surfaceFormat,
        const vk::Extent2D& imageExtent,
        std::uint32_t maxImagesInFlight );

    operator vk::SwapchainKHR() const { return *m_swapchain; }

    frame_data get_next_frame();

private:

    vk::Device m_logicalDevice;
    vk::SwapchainKHR m_swapchain;

    ...
};
```

The struct we return from `get_next_frame` is now better named `frame_data` because it's much more than just the synchronization objects. To be able to call `acquireNextImageKHR` we need the handles to the swapchain and the logical device, so we store them as class members as well. With all that our `get_next_frame` implementation looks like this:

```C++
swapchain::frame_data swapchain::get_next_frame()
{
    auto imageIndex = m_logicalDevice.acquireNextImageKHR(
        m_swapchain,
        std::numeric_limits< std::uint64_t >::max(),
        *m_readyForRenderingSemaphores[ m_currentFrameIndex ] ).value;

    const auto result = m_logicalDevice.waitForFences(
        *m_inFlightFences[ m_currentFrameIndex ],
        true,
        std::numeric_limits< std::uint64_t >::max() );
    m_logicalDevice.resetFences( *m_inFlightFences[ m_currentFrameIndex ] );

    const auto frame = frame_data{
        imageIndex,
        m_currentFrameIndex,
        *m_framebuffers[ imageIndex ],
        *m_inFlightFences[ m_currentFrameIndex ],
        *m_readyForRenderingSemaphores[ m_currentFrameIndex ],
        *m_readyForPresentingSemaphores[ m_currentFrameIndex ]
    };

    m_currentFrameIndex = ++m_currentFrameIndex % m_maxImagesInFlight;
    return frame;
}
```

It doesn't really make sense to wait on the fence outside of this function when everything we need is already accessible within the function, so I also moved that call in.

Now our class truly represents a complete swapchain implementation, so it should be named accordingly. The changes to the constructor implementation are pretty straightforward because we can again make use of the function we already had in place.

And with that the code in `main` is now much more concise and declarative:

```C++
auto swapchain = vcpp::swapchain{
    logicalDevice,
    *renderPass,
    *surface,
    surfaceFormats[0],
    swapchainExtent,
    requestedSwapchainImageCount };

...

while ( !glfwWindowShouldClose( window.get() ) )
{
    glfwPollEvents();

    const auto frame = swapchain.get_next_frame();

    vcpp::record_command_buffer(
        commandBuffers[ frame.inFlightIndex ],
        *pipeline,
        *renderPass,
        frame.framebuffer,
        swapchainExtent );

    const vk::PipelineStageFlags waitStages[] = {
        vk::PipelineStageFlagBits::eColorAttachmentOutput };
    const auto submitInfo = vk::SubmitInfo{}
        .setCommandBuffers( commandBuffers[ frame.inFlightIndex ] )
        .setWaitSemaphores( frame.readyForRenderingSemaphore )
        .setSignalSemaphores( frame.readyForPresentingSemaphore )
        .setPWaitDstStageMask( waitStages );
    queue.submit( submitInfo, frame.inFlightFence );

    const auto presentInfo = vk::PresentInfoKHR{}
        .setSwapchains( swapchain )
        .setImageIndices( frame.swapchainImageIndex )
        .setWaitSemaphores( frame.readyForPresentingSemaphore );

    const auto result = queue.presentKHR( presentInfo );
    if ( result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR )
        throw std::runtime_error( "presenting failed" );
}
```

We're now in a much better position to tackle the remaining problems with our rendering loop. That's what we're going to do in the next lesson.




1. https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rc-struct