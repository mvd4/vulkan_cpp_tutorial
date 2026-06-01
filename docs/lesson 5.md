# Lesson 5: Layers and Extensions

I promised already twice that we'd talk about layers and extensions eventually, and now is the time I deliver on that promise.

Both layers and extensions are a way to enhance Vulkan's builtin functionality. The main difference between the two is that layers only modify or enhance behaviour that is already present, while extensions do add new functionality. We'll work with both in the course of this tutorial.

## Layers
Layers can be thought of as just what their name suggests: additional levels of functionality that an API call passes before it reaches the actual Vulkan core implementation. Layers do not necessarily modify all function calls, depending on the purpose they may leave some alone. Originally Vulkan supported layers for the entire Vulkan environment (so-called instance layers) as well as individual per-physical-device layers. The latter have been deprecated now because it became apparent that there was no real use case for them.

Speaking of use cases: One major use case for layers is adding debugging support to Vulkan. Since the Vulkan core is so optimized for maximum performance, it only does the checks that are absolutely necessary. That means that you can easily issue a function call to the Vulkan core that seems to work fine, but actually does nothing because of a faulty parameter. Or your application might crash without you having the slightest clue what went wrong. Layers can add diagnostics, logging, profiling and other helpful functionality. And because they have to be explicitly switched on to become active, you can just leave them disabled when you ship your application and get the maximum Vulkan performance in production.

Of course, to be able to activate a layer, one should be able to detect whether it is actually supported on the respective system. Here's the function that lists all the available layers for the instance:
```cpp
std::vector< vk::LayerProperties > vk::enumerateInstanceLayerProperties( ... );
```
As you can see, this function is not a member of `vk::instance`. That makes sense because we already need to pass in the names of the layers we want to enable when we create the instance. The returned `vk::LayerProperties` structs have the following properties:
```cpp
struct LayerProperties
{
    ...
    string_t layerName;
    string_t description;
    uint32_t specVersion;
    uint32_t implementationVersion;
    ...
};
```
What I dubbed `string_t` here is actually a `vk::ArrayWrapper1D`, a class that wraps a fixed-size C-style array and adds some convenience functions for strings. It behaves pretty much like a plain old C-string in many ways, so I think it's clearer to write it that way. The most important property in `LayerProperties` is the `layerName`, as that is what we need to pass to the `InstanceCreateInfo` to turn the layer on.

Okay, so let's list all the layers that are available to us:
```cpp
...
auto printLayerProperties( const std::vector< vk::LayerProperties >& layers ) -> void
{
    for ( const auto& l : layers )
        std::cout << "    " << l.layerName << "\n";

    std::cout << "\n";
}

auto createVulkanInstance() -> vk::UniqueInstance
{
    ...
    const auto layers = vk::enumerateInstanceLayerProperties();
    std::cout << "Available instance layers: \n";
    printLayerProperties( layers );
    ...
}
...
```
When you run the program now you should see something like this as the first output:
```text
Available instance layers:
    VK_LAYER_NV_optimus
    VK_LAYER_LUNARG_api_dump
    VK_LAYER_LUNARG_device_simulation
    VK_LAYER_LUNARG_gfxreconstruct
    VK_LAYER_KHRONOS_synchronization2
    VK_LAYER_KHRONOS_validation
    VK_LAYER_LUNARG_monitor
    VK_LAYER_LUNARG_screenshot
```
In this example the NVIDIA Optimus layer is available on the system, along with some by the Khronos Group (the industry consortium that created the Vulkan standard) and some by LunarG (the company that maintains the official Vulkan SDK).

So far, so good. We'll get back to some of those layers in a minute. Let's look at extensions first (as said, device-specific layers have been deprecated, so we'll not cover them here).

## Extensions
In contrast to layers, extensions can actually add new functionality to Vulkan. Many of those extensions will only become relevant for you once you start exploring more advanced stuff. Still, there is one family of extensions that is widely used and that we will need in this tutorial as well: the Khronos surface extensions. We'll be talking about surfaces in depth when we get to the graphics stuff. But let me give you a quick intro here:

One of the main design principles for Vulkan is its platform-independence. There's nothing in the Vulkan core that is specific to one platform. Drawing onto a screen on the other hand is extremely platform specific, especially in a windowed context. Obviously the drawing functionality can not go into the Vulkan core, therefore it is realized by platform-specific extensions.

But first things first, let's now have a look which extensions we actually have available. The pattern is the same as the one for layers:
```cpp
...
auto printExtensionProperties( const std::vector< vk::ExtensionProperties >& extensions ) -> void
{
    for ( const auto& e : extensions )
        std::cout << "    " << e.extensionName << "\n";

    std::cout << "\n";
}
...
auto createVulkanInstance() -> vk::UniqueInstance
{
    ...

    const auto layers = vk::enumerateInstanceLayerProperties();
    std::cout << "Available instance layers: \n";
    printLayerProperties( layers );

    const auto instanceExtensions = vk::enumerateInstanceExtensionProperties();
    std::cout << "Available instance extensions: \n";
    printExtensionProperties( instanceExtensions );

    ...
```
If you run now, you'll hopefully see output along the lines of the following:
```text
Available instance extensions:
    VK_KHR_device_group_creation
    VK_KHR_external_fence_capabilities
    VK_KHR_external_memory_capabilities
    VK_KHR_external_semaphore_capabilities
    VK_KHR_get_physical_device_properties2
    VK_KHR_get_surface_capabilities2
    VK_KHR_surface
    VK_KHR_surface_protected_capabilities
    VK_KHR_win32_surface
    VK_EXT_debug_report
    VK_EXT_debug_utils
    VK_EXT_swapchain_colorspace
    VK_NV_external_memory_capabilities
```
Et voilà, there they are: `VK_KHR_surface`, `VK_KHR_win32_surface` and `VK_KHR_surface_protected_capabilities`[^1].

But wait, we're not done yet: in contrast to the layers, a specific device can have its own set of extensions on top of that. And as if that weren't enough already, instance layers can also come with extensions. Since we're currently working with the instance, let's finish that off first.

As it turns out you can pass the name of a layer to `enumerateInstanceExtensionProperties` and that'll give you the extensions for that specific layer. Let's enhance our `printLayerProperties` function accordingly[^2]:
```cpp
auto printLayerProperties( const std::vector< vk::LayerProperties >& layers ) -> void
{
    for ( const auto& l : layers )
    {
        std::cout << "    " << l.layerName << "\n";
        const auto extensions = vk::enumerateInstanceExtensionProperties( l.layerName.operator std::string() );
        for ( const auto& e : extensions )
            std::cout << "       Extension: " << e.extensionName << "\n";
    }

    std::cout << "\n";
}
```
Indeed, if you run that version you'll see that e.g. the Khronos validation layer comes with a few extensions of its own (the exact number depends on your SDK version).

Now let's complete our tour by looking at the device specific extensions. For that purpose we extend the `printPhysicalDeviceProperties` function as follows:
```cpp
auto printPhysicalDeviceProperties( const vk::PhysicalDevice& device ) -> void
{
    ...

    const auto deviceExtensions = device.enumerateDeviceExtensionProperties();
    std::cout << "\n  Available device extensions: \n";
    printExtensionProperties( deviceExtensions );
}
```
This is probably going to give you a long list of device specific extensions. Most of them will be irrelevant for this tutorial, but it's good to know how much functionality you could potentially use.

## Enabling Layers and Extensions
So, now that we know more about layers and extensions, let's make use of some. We actually already did that in lesson 3 where we enabled the portability extension to make this tutorial work on macos. Now you hopefully have a better understanding of what we did there.

As said, one main use case for layers is debugging, so that's what we'll do now as well. Let's extend our instance creation to enable the Khronos debug utilities extension and the validation layer:
```cpp
auto createVulkanInstance() -> vk::UniqueInstance
{
    ...
    const auto layersToEnable = std::vector< const char* >{
        "VK_LAYER_KHRONOS_validation"
    };

    auto extensionsToEnable = std::vector< const char* >{
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
    };

    auto instanceCreateInfo = vk::InstanceCreateInfo{};

    // for newer versions of the sdk on macos we have to enable the portability extension
    if constexpr ( isMacOS() && getVulkanSDKVersion() >= VersionNumber{ 1, 3, 216 } )
    {
        extensionsToEnable.push_back( VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME );
        instanceCreateInfo.setFlags( vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR );
    }

    instanceCreateInfo
        .setPApplicationInfo( &appInfo )
        .setPEnabledLayerNames( layersToEnable )
        .setPEnabledExtensionNames( extensionsToEnable );

    return vk::createInstanceUnique( instanceCreateInfo );
}
```
As you can see, there are constants defined for the names of some extensions (in `vulkan_core.h`), not for the layers though.

If you build and run the program now you might already see the layer in action, depending on the platform you're on. On macos you get a (quite lengthy) error message that tells you that `VK_KHR_portability_subset` must be enabled if the extension is listed in the available device extensions as returned by the call to
```cpp
std::vector< vk::ExtensionProperties > vk::PhysicalDevice::enumerateDeviceExtensionProperties()
```

So it looks like we'll have to enable this extension, but only if it's present. Let's do that by modifying our `createLogicalDevice` function a bit:
```cpp
auto createLogicalDevice( const vk::PhysicalDevice& physicalDevice ) -> vk::UniqueDevice
{
    ...
    const auto enabledDeviceExtensions = getRequiredDeviceExtensions(
        physicalDevice.enumerateDeviceExtensionProperties()
    );
    const auto deviceCreateInfo = vk::DeviceCreateInfo{}
        .setQueueCreateInfos( queueCreateInfos )
        .setPEnabledExtensionNames( enabledDeviceExtensions );

    return physicalDevice.createDeviceUnique( deviceCreateInfo );
}
```
with
```cpp
auto getRequiredDeviceExtensions(
    const std::vector< vk::ExtensionProperties >& availableExtensions
) -> std::vector< const char* >
{
    // extension name strings need to be static, because we're returning a vector with pointers to the underlying char arrays
    static const std::string compatibilityExtensionName = "VK_KHR_portability_subset";

    auto result = std::vector< const char* >{};

    const auto it = std::find_if(
        availableExtensions.begin(),
        availableExtensions.end(),
        []( const vk::ExtensionProperties& e )
        {
            return compatibilityExtensionName == e.extensionName;
        }
    );

    if ( it != availableExtensions.end() )
        result.push_back( compatibilityExtensionName.c_str() );

    return result;
}
```
If you run the program again now, that validation message should no longer be displayed.

Cool, that's it for now for layers and extensions. Next time we'll start to implement our first pipeline.

---

[^1]: These are the extensions that are present on my Windows system. Obviously you will not have the `...win32...` extension on a macOS or Linux machine but something platform specific.
[^2]: In case you're wondering about the `l.layerName.operator std::string()`: implicit conversion does not work here due to the templated context. However, calling the conversion operator explicitly via `.operator std::string()` does work.
