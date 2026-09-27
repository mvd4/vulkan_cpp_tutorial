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


---

[^1]: `VK_EXT_validation_features` - and with it `vk::ValidationFeaturesEXT` - has been deprecated in favour of `VK_EXT_layer_settings`, which provides a more general mechanism for configuring the behaviour of any layer, not just the validation layer. The older extension is still widely supported and keeps us consistent with what we've been requesting since lesson 13, so we'll stick with it here. Just be aware of its successor if you're working against a recent SDK.
