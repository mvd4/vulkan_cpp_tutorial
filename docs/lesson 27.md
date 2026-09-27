# Lesson 27: Improving the Render Loop I - Synchronization

## Enabling synchronization validation
In the last lesson, I actually introduced a subtle bug: I created `maxFramesInFlight` semaphores in `readyForPresentingSemaphores` and indexed the one to use by the current frame index. That's not correct: we do not control the actual number of swapchain images and so it is not necessarily identical to the number of frames in flight. So we need one semaphore per swapchain image. The version from last time will likely work in practice, but technically speaking it's a synchronization bug.

Before we set about fixing this, let's take a step back though: bugs like this are exactly the kind of thing that "works on my machine" right up until it doesn't. Who knows what other issues we have lurking in our code? Isn't there a way that we can get a bit more certainty?

Turns out there is. Back in lesson 5 we already enabled the `VK_LAYER_KHRONOS_validation` layer. That layer actually ships with a feature called *Synchronization Validation* which tracks the memory accesses your commands make and reports every pair of them that isn't correctly ordered against the other. It isn't part of the default checks though, so it was not enabled up to now. Let's fix this.

Technically this feature is part of the extended validation features, so to enable it we need to create a `vk::ValidationFeaturesEXT` instance and chain it into the instance creation via its `pNext` pointer[^1]:
```cpp
// devices.cpp
auto createVulkanInstance( const std::vector< std::string >& requiredExtensions ) -> vk::UniqueInstance
{
    ...
    constexpr auto validationFeaturesToEnable = std::array{
        vk::ValidationFeatureEnableEXT::eSynchronizationValidation
    };
    const auto validationFeatures = vk::ValidationFeaturesEXT{}
        .setEnabledValidationFeatures( validationFeaturesToEnable );

    auto instanceCreateInfo = vk::InstanceCreateInfo{}.setPNext( &validationFeatures );
    ...
}
```

Chaining a struct into `pNext` only works if the extension that defines it is enabled, so this snippet quietly depends on `VK_EXT_validation_features` being part of our instance extensions. It has been, since lesson 13 - it just never came up because we never had a reason to use it.

This solution works, but there's a caveat: the validation layer is a development tool and synchronization validation in particular is expensive because it tracks every memory access your commands make. On top of that, the layer only exists on machines that have the Vulkan SDK installed. We've been requesting it unconditionally since lesson 5, which means our release builds pay the price too, and our program refuses to start at all for anyone who only has a graphics driver. Let's make it conditional.

First a compile-time helper, right next to the `isMacOS()` one we already have:
```cpp
// devices.cpp
constexpr auto isDebugBuild() -> bool
{
    return
#if defined NDEBUG
        false;
#else
        true;
#endif
}
```

Then, in debug builds only, we check at runtime whether the layer is really there before asking for it, and do the same for the extensions we want from it:
```cpp
// devices.cpp
auto layersToEnable = std::vector< const char* >{};
auto extensionsToEnable = std::vector< const char* >{};
auto enableSynchronizationValidation = false;

if constexpr ( isDebugBuild() )
{
    const auto enableValidation = isLayerAvailable( layers, validationLayerName );

    if ( enableValidation )
    {
        layersToEnable.push_back( validationLayerName );
        std::cout << "Validation layer enabled\n";
    }

    const auto validationLayerExtensions = enableValidation
        ? vk::enumerateInstanceExtensionProperties( std::string{ validationLayerName } )
        : std::vector< vk::ExtensionProperties >{};

    if (
        isExtensionAvailable( instanceExtensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME ) ||
        isExtensionAvailable( validationLayerExtensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME )
    )
    {
        extensionsToEnable.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
    }

    enableSynchronizationValidation =
        enableValidation &&
        isExtensionAvailable( validationLayerExtensions, VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME );

    if ( enableSynchronizationValidation )
    {
        extensionsToEnable.push_back( VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME );
        std::cout << "Synchronization validation enabled\n";
    }
}
```
`validationLayerName` is just a file-local `constexpr auto validationLayerName = "VK_LAYER_KHRONOS_validation";`, so we no longer have to repeat the literal. `isLayerAvailable` and `isExtensionAvailable` are one-liners over the `layers` and extension collections we already enumerate at the top of the function - take a look at `devices.cpp` if you're curious, there's nothing surprising in them.

Now that we have `isExtensionAvailable`, I also used it to replace the hand-written search in `getRequiredDeviceExtensions` further down in the file. And note that we changed the conditions for two extensions along the way: `VK_EXT_debug_utils` used to be enabled unconditionally and is now limited to debug builds, and `VK_EXT_validation_features` - also unconditional before - now additionally requires the validation layer to actually be present.

The only thing left to do is to chain the validation features in when - and only when - we really got the extension. This replaces the unconditional `setPNext` from the snippet above:
```cpp
// devices.cpp
auto instanceCreateFlags = vk::InstanceCreateFlags{};

// for newer versions of the sdk on macos we have to enable the portability extension
if constexpr ( isMacOS() && getVulkanSDKVersion() >= VersionNumber{ 1, 3, 216 } )
{
    extensionsToEnable.push_back( VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME );
    instanceCreateFlags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
}

constexpr auto validationFeaturesToEnable = std::array{
    vk::ValidationFeatureEnableEXT::eSynchronizationValidation
};
const auto validationFeatures = vk::ValidationFeaturesEXT{}
    .setEnabledValidationFeatures( validationFeaturesToEnable );

auto instanceCreateInfo = vk::InstanceCreateInfo{}
    .setFlags( instanceCreateFlags )
    .setPApplicationInfo( &appInfo )
    .setPEnabledLayerNames( layersToEnable )
    .setPEnabledExtensionNames( extensionsToEnable );

if ( enableSynchronizationValidation )
    instanceCreateInfo.setPNext( &validationFeatures );

return vk::createInstanceUnique( instanceCreateInfo );
```
Note that `validationFeatures` only stores a pointer to `validationFeaturesToEnable`, and `instanceCreateInfo` in turn only stores a pointer to `validationFeatures`. All three therefore have to stay alive until `createInstanceUnique` has run, which is why they live down here at the end of the function rather than next to the check that sets `enableSynchronizationValidation`.

You'll notice one more change in that snippet: the macOS portability flag now goes into a local `instanceCreateFlags` first. Previously we created an empty `instanceCreateInfo` at the top of the function just so that the `if constexpr` block could call `setFlags` on it, and then filled in the rest further down. Collecting the flag separately lets us declare and populate `instanceCreateInfo` in a single place.

Building and running this does yield errors, but - at least on my machine - they're actually not our semaphore mix-up: all I get is a `WRITE_AFTER_READ` hazard that mentions `vkQueueSubmit`, `vkCmdBeginRenderPass` and `vkAcquireNextImageKHR`.

So apparently we _do_ have a few synchronization bugs in the codebase. That's great, the validation already paid off. But those are not the error I talked about in the beginning. Why aren't we seeing that issue?

The reason is that I incidentally have the same number of swapchain images and frames in flight. Apparently, the driver returned exactly the minimum number of images that the program requested, which happens to be the same as the frames in flight. In this case there's no problem because we have the right number of semaphores and the indices also match. However, if you change `requestedSwapchainImageCount` to something greater than 2, you should see this validation error among the other ones:

```text
Swapchain image 1 was presented but was not re-acquired, so VkSemaphore 0x220000000022 may still be in use and cannot be safely reused with image index 0.
```

Alright, now that we have a clear line of sight on the error, let's fix this one first.

## Fixing the swapchain synchronization
As mentioned above, we actually need one semaphore for each swapchain image. So let's modify our `Swapchain` constructor implementation:
```cpp
// presentation.cpp
Swapchain::Swapchain(
    ...
)
    ...
{
    assert( maxFramesInFlight > 0 );
    assert( requestedSwapchainImageCount > 0 );

    for( std::uint32_t f = 0; f < maxFramesInFlight; ++f )
    {
        m_inFlightFences.push_back( logicalDevice.createFenceUnique(
            vk::FenceCreateInfo{}.setFlags( vk::FenceCreateFlagBits::eSignaled )
        ) );

        m_readyForRenderingSemaphores.push_back( logicalDevice.createSemaphoreUnique(
            vk::SemaphoreCreateInfo{}
        ) );
    }

    for ( const auto& v : m_imageViews )
    {
        m_readyForPresentingSemaphores.push_back( logicalDevice.createSemaphoreUnique(
            vk::SemaphoreCreateInfo{}
        ) );
    }
}
```

Since our semaphore collections are already `std::vector`s, all we have to do is to create the correct number of semaphores for protecting the swapchain images and the frames we render into, and add them to the appropriate collection.

The only other thing we have to do now is to return the correct index for each frame:
```cpp
// presentation.cpp
auto Swapchain::getNextFrame() -> FrameData
{
    ...
    const auto frame = FrameData{
        m_currentFrameIndex,
        swapchainImageIndex,
        *m_framebuffers[ swapchainImageIndex ],
        *m_inFlightFences[ m_currentFrameIndex ],
        *m_readyForRenderingSemaphores[ m_currentFrameIndex ],
        *m_readyForPresentingSemaphores[ swapchainImageIndex ]
    };
    ...
}
```

If you run this version, you should no longer get the semaphore-related validation errors.

## Subpass Dependencies
Now to the second problem. This one is unrelated to the issue we just fixed, it was actually there from the beginning (it would obviously have been a good idea to enable this validation much earlier). To understand it, let's look a bit more closely at how a render pass is structured:

A render pass is a combination of one or more subpasses. For example, you could have one subpass take care of all the geometry calculations and a second one for the lighting. Execution order and dependencies between subpasses are defined with the help of `SubpassDependency` objects. You can think of them as the description of the edges in a dependency graph.

We only defined one subpass so far, so we didn't care about dependencies. But actually there is another 'virtual' subpass, which is 'all the work that happens before or after the render pass'. And even though we didn't define them explicitly, there are dependencies between our subpass and this virtual (a.k.a. _external_) one.

A `SubpassDependency` is defined like this:
```cpp
struct SubpassDependency
{
    ...
    SubpassDependency& setSrcSubpass( uint32_t srcSubpass_ );
    SubpassDependency& setDstSubpass( uint32_t dstSubpass_ );
    SubpassDependency& setSrcStageMask( vk::PipelineStageFlags srcStageMask_ );
    SubpassDependency& setDstStageMask( vk::PipelineStageFlags dstStageMask_ );
    SubpassDependency& setSrcAccessMask( vk::AccessFlags srcAccessMask_ );
    SubpassDependency& setDstAccessMask( vk::AccessFlags dstAccessMask_ );
    SubpassDependency& setDependencyFlags( vk::DependencyFlags dependencyFlags_ );
    ...
};
```

- `setSrcSubpass` specifies the source subpass of the dependency.
- `setDstSubpass` specifies the destination subpass of the dependency.
- `setSrcStageMask` and `setDstStageMask` narrow the dependency down to specific pipeline stages: srcSubpass's stages in srcStageMask must complete before dstSubpass's stages in dstStageMask execute, and any implicit layout transition happens in between.
- `setSrcAccessMask` and `setDstAccessMask` describe the memory accesses that need to be made available and visible across the dependency, so that writes on the source side are actually observable on the destination side.
- `setDependencyFlags` allows some finer control in some cases.

Since we didn't define an explicit dependency for the external subpass, Vulkan implicitly defines it for each attachment. It looks like this:
```c
VkSubpassDependency implicitDependency = {
    .srcSubpass      = VK_SUBPASS_EXTERNAL,
    .srcStageMask    = VK_PIPELINE_STAGE_NONE,  // used to be TOP_OF_PIPE in earlier versions of Vulkan
    .srcAccessMask   = 0,
    .dstSubpass      = < the first subpass that uses this attachment >,
    .dstStageMask    = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    .dstAccessMask   = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT |
                       VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                       VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                       VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                       VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    .dependencyFlags = 0,
};
```

What that essentially tells the GPU is that our subpass isn't dependent on anything that happens in the external subpass ( `VK_PIPELINE_STAGE_NONE` and `srcAccessMask=0`). This seems reasonable at first sight, but it's actually problematic in our case.

Look at how we use the `readyForRenderingSemaphore`s when creating the `SubmitInfo` in `main`: we set the `waitDstStageMask` to `eColorAttachmentOutput`, which means that the semaphore blocks pipeline operations that access the color attachment until it becomes signalled. However, at the beginning the render pass has to transition the color attachment out of `eUndefined`, and that transition is a memory write.

Per the Vulkan spec, the transition of an attachment's initial layout to the subpass-layout is performed as part of the subpass dependency itself: after the source scope's availability operations and before the destination scope's visibility operations. That means with the dependency defined as above, the transition is required to happen before any command in our subpass, but it doesn't have to wait for our semaphores to be signalled. So it might actually happen while the attachment is still being read from by the presentation engine. In addition, we set the `loadOp` for the color attachment to `eClear` in `createRenderPass`, and that is another memory write that may happen before.

So, this implicit subpass dependency is clearly not what we need and the cause of the validation errors we're seeing. We can fix this by explicitly adding one ourselves. So let's do that:
```cpp
// pipelines.cpp
auto createRenderPass(
    const vk::Device& logicalDevice,
    vk::Format colorFormat
) -> vk::UniqueRenderPass
{
    ...
    const auto subpassDependency = vk::SubpassDependency{}
        .setSrcSubpass( VK_SUBPASS_EXTERNAL )
        .setSrcStageMask(
            vk::PipelineStageFlagBits::eColorAttachmentOutput |
            vk::PipelineStageFlagBits::eEarlyFragmentTests )
        .setSrcAccessMask( vk::AccessFlagBits::eNone )
        .setDstSubpass( 0 )
        .setDstStageMask(
            vk::PipelineStageFlagBits::eColorAttachmentOutput |
            vk::PipelineStageFlagBits::eEarlyFragmentTests )
        .setDstAccessMask(
            vk::AccessFlagBits::eColorAttachmentWrite |
            vk::AccessFlagBits::eDepthStencilAttachmentWrite );

    const auto renderPassCreateInfo = vk::RenderPassCreateInfo{}
        .setAttachments( attachments )
        .setSubpasses( subpass )
        .setDependencies( subpassDependency );
    ...
}
```
We define our source subpass as `VK_SUBPASS_EXTERNAL`, which refers to that 'virtual' subpass I mentioned above (in this case everything that happens before the render pass, because we define it as a src). We then say that we need all operations on `eColorAttachmentOutput` and `eEarlyFragmentTests` stages to complete before the stages in the destination subpass can be executed.

And this is what fixes our issue: since `eColorAttachmentOutput` is also the stage that our semaphore waits on, the transition from external to our subpass will now only happen when the semaphore is signalled. So the layout transition will also only happen then.

The destination subpass is the only one we defined (the one at index 0), and the stages in it that need to wait are again `eColorAttachmentOutput` and `eEarlyFragmentTests`.

With that in place the hazard is eliminated and you should no longer see any validation errors.

## Resetting the fence at the right time
There's one small improvement I want to make to our fence handling before we move on: we currently reset our fences directly after they were signaled and before the call to `acquireNextImageKHR`. If that throws an `OutOfDateKHRError`, the fence is left unsignaled with nothing to signal it. It's currently only working because `main` catches that exception and sets `framebufferSizeChanged`, which makes the next iteration of the render loop destroy and recreate the whole `Swapchain` - and with it the fence in question. That implicit coupling is fragile though, so let's move the reset to after the acquire call:
```cpp
// presentation.cpp
auto Swapchain::getNextFrame() -> FrameData
{
    [[maybe_unused]] const auto result = m_logicalDevice.waitForFences(
        *m_inFlightFences[ m_currentFrameIndex ],
        true,
        std::numeric_limits< std::uint64_t >::max()
    );

    const auto swapchainImageIndex = m_logicalDevice.acquireNextImageKHR(
        *m_swapchain,
        std::numeric_limits< std::uint64_t >::max(),
        *m_readyForRenderingSemaphores[ m_currentFrameIndex ]
    ).value;

    m_logicalDevice.resetFences( *m_inFlightFences[ m_currentFrameIndex ] );

    ...
}
```
This doesn't change the observable behaviour of our program, but it makes `getNextFrame` correct on its own terms instead of relying on a caller that happens to throw the whole `Swapchain` away.


## One vertex buffer per frame in flight
With the validation layer happy again, it might seem we can call it a day. But there is one more hazard in our code which the validation layer can never tell us about. To see it, let's look at what our render loop does with the vertex data:
```cpp
// main.cpp
for ( std::uint32_t i = 0; i < vertexCount; ++i )
{
    verticesTemp[ i ].position = projection * view * model * vertices[ i ].position;
}

vcpp::copyDataToBuffer( logicalDevice, verticesTemp, gpuVertexBuffer );
```
We transform all our vertices on the CPU and then copy the result into the one single vertex buffer that we created before the loop. But that is obviously the same buffer that the command buffer we submitted in the _previous_ iteration binds and reads from.

Remember that `queue.submit` doesn't wait for anything: it hands the command buffer over to the GPU and returns immediately. So by the time we're back at the top of the loop, the GPU may very well still be busy drawing the previous frame - which means it may still be reading from that buffer. And there we are, overwriting it. That's another write-after-read hazard, the very same class of bug as the two we just fixed. The synchronization validation cannot warn us about it because it doesn't see our write.

So how do we fix it? The standard answer is to give each frame in flight its own buffer, in the same way we already do for the command buffers, the fences and the `readyForRendering` semaphores. So let's create one buffer per frame in flight instead of a single one:
```cpp
// main.cpp
auto gpuVertexBuffers = std::vector< vcpp::GPUBuffer >{};
for ( std::uint32_t f = 0; f < maxFramesInFlight; ++f )
{
    gpuVertexBuffers.push_back( vcpp::createGPUBuffer(
        physicalDevice,
        logicalDevice,
        sizeof( vertices ),
        vk::BufferUsageFlagBits::eVertexBuffer
    ) );
}
```
`GPUBuffer` owns its handles through `vk::UniqueBuffer` and `vk::UniqueDeviceMemory`, so it is move-only - which is fine, `push_back` moves the temporary returned by `createGPUBuffer` into the vector, and the buffers are destroyed in the right order when `gpuVertexBuffers` goes out of scope at the end of `main`.

And in the render loop we move the copy into the `try` block, where we know the frame index, and target the buffer that belongs to it:
```cpp
// main.cpp
rotationAngle += 0.01f;

try
{
    const auto frame = swapchain->getNextFrame();
    const auto& gpuVertexBuffer = gpuVertexBuffers[ frame.frameInFlightIndex ];

    vcpp::copyDataToBuffer( logicalDevice, verticesTemp, gpuVertexBuffer );

    vcpp::recordCommandBuffer(
        commandBuffers[ frame.frameInFlightIndex ],
        *pipeline,
        *renderPass,
        frame.framebuffer,
        swapchainExtent,
        *gpuVertexBuffer.buffer,
        vertexCount
    );

    ...
}
```
Note that we bind the frame's buffer to a local reference called `gpuVertexBuffer`, so neither the `copyDataToBuffer` nor the `recordCommandBuffer` call below it has to change at all.

There's a pattern here that's worth taking away from this lesson, because it will come up again and again: every resource that the CPU writes to while the GPU may still be using it needs to exist once per frame in flight. Resources that are only ever read by the GPU - our shader modules, the pipeline, the render pass - can happily be shared.[^2]

Build and run this version and you should see the same rotating cube as before, without any validation errors. Our synchronization is now sound, and with that out of the way we can finally get to improving the render loop itself - which is what the next lesson will be about.

---

[^1]: `VK_EXT_validation_features` - and with it `vk::ValidationFeaturesEXT` - has been deprecated in favour of `VK_EXT_layer_settings`, which provides a more general mechanism for configuring the behaviour of any layer, not just the validation layer. The older extension is still widely supported and keeps us consistent with what we've been requesting since lesson 13, so we'll stick with it here. Just be aware of its successor if you're working against a recent SDK.
[^2]: The alternative to duplicating the buffer is to keep one device-local vertex buffer and upload into it from a per-frame staging buffer with `vkCmdCopyBuffer`. That has the advantage of making the transfer a real command - so synchronization validation can see it - and device-local memory is faster for the GPU to read. But it also needs a per-frame staging buffer plus explicit barriers between transfer and vertex input, so it's more work. We'll get to staging buffers later in the tutorial.
