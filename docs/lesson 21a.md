# Lesson 21a: Another round of refactoring
Before we address the remaining stability issues with our render pipeline I want to do another round of refactoring. This is going to make our work considerably easier moving forward.

One obvious shortcoming of the code in and around our render loop is that it is pretty cluttered and way less declarative than we would like it to be. There is a lot of detail work happening on the top level which makes it harder than necessary to follow the program flow. There are also a number of non-obvious invariants that make the code brittle and error-prone. So let's see what we can do about this.

A first thing to notice is that all synchronization primitives are tied to the number of images in flight and are addressed by the same index. That means we can either model it as an array of structs or a struct of arrays. I chose the latter one because it's closer to what we have right now. So let's move all our collections of synchronization objects to a struct:

```cpp
// presentation.hpp

struct SwapchainSync
{
    SwapchainSync( const vk::Device& logicalDevice, std::uint32_t maxImagesInFlight );

    std::vector< vk::UniqueFence > inFlightFences;
    std::vector< vk::UniqueSemaphore > readyForRenderingSemaphores;
    std::vector< vk::UniqueSemaphore > readyForPresentingSemaphores;
};
```

```cpp
// presentation.cpp

SwapchainSync::SwapchainSync( const vk::Device& logicalDevice, std::uint32_t maxImagesInFlight )
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

I'm not showing the required changes in `main` here because that would blow up the tutorial too much and it's going to change several times anyway. And in any case, so far it is debatable whether the code in `main` actually has improved. The loop to create the synchronization objects is gone, but we now have an additional level of indirection to access them. Looks like we need to do better to make the refactoring worthwhile.

Let's see. In each iteration of our render loop we're accessing the element with the same index from all the containers. So we would actually not need access to the containers themselves at all, we only need the respective synchronization objects for the current frame. And it would also be much less error prone if `SwapchainSync` would just make sure we use the right semaphores and fence. So we're going to move the whole frame-in-flight logic into our new struct:

```cpp
class SwapchainSync
{
public:

    struct FrameSync
    {
        vk::Fence inFlightFence;
        vk::Semaphore readyForRenderingSemaphore;
        vk::Semaphore readyForPresentingSemaphore;
    };


    SwapchainSync( const vk::Device& logicalDevice, std::uint32_t maxFramesInFlight );

    FrameSync getNextFrameSync();

private:

    std::uint32_t m_maxFramesInFlight;
    std::uint32_t m_currentFrameIndex = 0;

    std::vector< vk::UniqueFence > m_inFlightFences;
    std::vector< vk::UniqueSemaphore > m_readyForRenderingSemaphores;
    std::vector< vk::UniqueSemaphore > m_readyForPresentingSemaphores;
};
```

Since it now has invariants to maintain, I followed C++ best practice and made our struct a class[^1]. The implementation of `getNextFrameSync` is straightforward:

```cpp
auto SwapchainSync::getNextFrameSync() -> FrameSync
{
    const auto result = FrameSync{
        *m_inFlightFences[ m_currentFrameIndex ],
        *m_readyForRenderingSemaphores[ m_currentFrameIndex ],
        *m_readyForPresentingSemaphores[ m_currentFrameIndex ]
    };

    m_currentFrameIndex = (m_currentFrameIndex + 1) % m_maxFramesInFlight;
    return result;
}
```

With that we can remove most uses of the subscript operators from our render loop, making it quite a bit more concise. However, we still need to select the correct command buffer so that we now have to maintain the `frameInFlightIndex` in two different locations. That's not good, let's change it by adding the index to the `FrameSync` struct. I'm not going to show the code here since that change is almost trivial.

We have now encapsulated the synchronization objects in our `SwapchainSync` class, which already helped making the code in `main` significantly more declarative. Moving on in the same spirit: wouldn't it make sense to do the same for the framebuffers and images? For those we also do not need access to the whole collection, we're only ever interested in the framebuffer that we are currently processing. Let's see what happens when we add them to our class as well:

```cpp
class SwapchainState
{
public:

    ...

    SwapchainState(
        const vk::Device& logicalDevice,
        const vk::SwapchainKHR& swapchain,
        const vk::RenderPass& renderPass,
        const vk::Extent2D& imageExtent,
        const vk::Format& imageFormat,
        std::uint32_t maxFramesInFlight,
        std::uint32_t minSwapchainImageCount
    );

    ...

private:

    std::uint32_t m_currentFrameIndex = 0;

    std::vector< vk::UniqueImageView > m_imageViews;
    std::vector< vk::UniqueFramebuffer > m_framebuffers;

    std::vector< vk::UniqueFence > m_inFlightFences;
    std::vector< vk::UniqueSemaphore > m_readyForRenderingSemaphores;
    std::vector< vk::UniqueSemaphore > m_readyForPresentingSemaphores;
};
```

The name SwapchainSync would no longer reflect the classes purpose, so I renamed it. Moving the framebuffers and image views into the class means that we also need to pass all the arguments necessary to create them to our constructor. Since we now have the `vector` holding the framebuffers directly in the class, there's no reason to keep its size around as a separate variable.

I also decided to take this opportunity to rectify the inaccuracy I already mentioned in lesson 20: there is no technical reason why the number of frames we allow to be in flight at the same time on the host / GPU side needs to be the same as the number of images in the swapchain. In fact, there are scenarios where a diveriging size makes sense. So I want to give the users of this new class the control to do so.

In the implementation we can make use of the functions that we already had in place to create the framebuffers and image views:
```cpp
SwapchainState::SwapchainState(
    const vk::Device& logicalDevice,
    const vk::SwapchainKHR& swapchain,
    const vk::RenderPass& renderPass,
    const vk::Extent2D& imageExtent,
    const vk::Format& imageFormat,
    std::uint32_t maxFramesInFlight,
    std::uint32_t minSwapchainImageCount
)
    : m_imageViews{ createSwapchainImageViews( logicalDevice, swapchain, imageFormat ) }
    , m_framebuffers{ createFramebuffers( logicalDevice, m_imageViews, imageExtent, renderPass ) }
{
    for( std::uint32_t f = 0; f < maxFramesInFlight; ++f )
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

In our render loop we need access to the current framebuffer, so we probably should return it alongside the synchronization objects and index. We have a problem in the implementation of `getNextFrameSync` though: to determine the correct framebuffer and in-flight fence, we need the current swapchain image index. We could pass that in as a parameter, but to acquire the index via`acquireNextImageKHR`, we need the `readyForRenderingSemaphore`. So we'd end up in a chicken-egg situation. We could resolve it by splitting the `getNextFrame` function into two. The other option is to acquire the image index within the function. The latter would result in this class declaration:

```cpp
class Swapchain
{
public:

    struct FrameData
    {
        std::uint32_t swapchainImageIndex;
        std::uint32_t frameInFlightIndex;

        vk::Framebuffer framebuffer;

        vk::Fence inFlightFence;
        vk::Semaphore readyForRenderingSemaphore;
        vk::Semaphore readyForPresentingSemaphore;
    };

    Swapchain(
        const vk::Device& logicalDevice,
        const vk::RenderPass& renderPass,
        const vk::SurfaceKHR& surface,
        const vk::SurfaceFormatKHR& surfaceFormat,
        const vk::Extent2D& imageExtent,
        std::uint32_t maxFramesInFlight,
        std::uint32_t minSwapchainImageCount
    );

    Swapchain( const Swapchain& ) = delete;
    Swapchain( Swapchain&& ) = delete;
    auto operator=( const Swapchain& ) -> Swapchain& = delete;
    auto operator=( Swapchain&& ) -> Swapchain& = delete;

    operator vk::SwapchainKHR() const { return *m_swapchain; }

    auto getNextFrame() -> FrameData;

private:

    vk::Device m_logicalDevice;
    vk::UniqueSwapchainKHR m_swapchain;

    ...
};
```

The struct we return from `getNextFrame` is now better named `FrameData` because it's much more than just the synchronization objects. To be able to call `acquireNextImageKHR` we need the handles to the swapchain and the logical device, so we store them as class members as well. With all that our `getNextFrame` implementation looks like this:

```cpp
auto Swapchain::getNextFrame() -> FrameData
{
    [[maybe_unused]] const auto result = m_logicalDevice.waitForFences(
        *m_inFlightFences[ m_currentFrameIndex ],
        true,
        std::numeric_limits< std::uint64_t >::max() );
    m_logicalDevice.resetFences( *m_inFlightFences[ m_currentFrameIndex ] );

    const auto swapchainImageIndex = m_logicalDevice.acquireNextImageKHR(
        *m_swapchain,
        std::numeric_limits< std::uint64_t >::max(),
        *m_readyForRenderingSemaphores[ m_currentFrameIndex ] ).value;

    const auto frame = FrameData{
        m_currentFrameIndex,
        swapchainImageIndex,
        *m_framebuffers[ swapchainImageIndex ],
        *m_inFlightFences[ m_currentFrameIndex ],
        *m_readyForRenderingSemaphores[ m_currentFrameIndex ],
        *m_readyForPresentingSemaphores[ m_currentFrameIndex ]
    };

    m_currentFrameIndex = (m_currentFrameIndex + 1) % m_framebuffers.size();
    return frame;
}
```

It doesn't really make sense to wait on the fence outside of this function when everything we need is already accessible within the function, so I also moved that call in. All our fences and semaphores are still synchronizing the flow on the host / GPU side, so nothing changes about how they work.

Two more details on the class declaration are worth pointing out. First, `Swapchain` owns a `vk::UniqueSwapchainKHR` and several other unique handles, so copying it would be a bug; we make that explicit by `= delete`-ing the copy and move special members. Second, we add a non-explicit conversion operator to `vk::SwapchainKHR`. This lets us pass a `Swapchain` instance wherever the underlying Vulkan handle is expected — for example when building the array we hand to `vk::PresentInfoKHR::setSwapchains` further down in `main`. Since the conversion just hands out the underlying handle and never transfers ownership, the implicit form is convenient and safe.

Now our class truly represents a complete Swapchain implementation, so it should be named accordingly. The changes to the constructor implementation are pretty straightforward because we can again make use of the function we already had in place.

And with that the code in `main` is now much more concise and declarative:

```cpp
auto swapchain = vcpp::Swapchain{
    logicalDevice,
    *renderPass,
    *surface,
    surfaceFormats[0],
    swapchainExtent,
    maxFramesInFlight,
    requestedSwapchainImageCount };

...

const auto swapchains = std::array< vk::SwapchainKHR, 1 >{ swapchain };

while ( !glfwWindowShouldClose( window.get() ) )
{
    glfwPollEvents();

    const auto frame = swapchain.getNextFrame();

    vcpp::recordCommandBuffer(
        commandBuffers[ frame.frameInFlightIndex ],
        *pipeline,
        *renderPass,
        frame.framebuffer,
        swapchainExtent );

    const auto waitStages = std::array< vk::PipelineStageFlags, 1 >{
        vk::PipelineStageFlagBits::eColorAttachmentOutput };
    const auto submitInfo = vk::SubmitInfo{}
        .setCommandBuffers( commandBuffers[ frame.frameInFlightIndex ] )
        .setWaitSemaphores( frame.readyForRenderingSemaphore )
        .setSignalSemaphores( frame.readyForPresentingSemaphore )
        .setPWaitDstStageMask( waitStages.data() );
    queue.submit( submitInfo, frame.inFlightFence );

    const auto presentInfo = vk::PresentInfoKHR{}
        .setSwapchains( swapchains )
        .setImageIndices( frame.swapchainImageIndex )
        .setWaitSemaphores( frame.readyForPresentingSemaphore );

    const auto result = queue.presentKHR( presentInfo );
    if ( result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR )
        throw std::runtime_error( "presenting failed" );
}
```
Play around with this, changing the `maxFramesInFlight` and `minSwapchainImages` constants in `main.cpp` independently from each other. The rendering loop should work without errors for all combinations[^2].

And with this done, we're now in a much better position to tackle the remaining problems with our rendering loop. That's what we're going to do in the next lesson.

---

[^1]: https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rc-struct
[^2]: The exception being of course if you set either of these constants to 0, which is why we asserted against it in the `Swapchain` constructor. You also need to make sure that `minSwapchainImages >= VkSurfaceCapabilitiesKHR::minImageCount` as obtained by `vkGetPhysicalDeviceSurfaceCapabilitiesKHR()`. We don't do that here, but in production code it'd be a good idea to check for that case.