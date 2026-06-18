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

---

[^1]: https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rc-struct
