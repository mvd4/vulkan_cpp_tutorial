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
What I dubbed `string_t` here is actually a `vk::ArrayWrapper1D`, a class that extends `std::array` with some convenience functions for strings. It behaves pretty much like a plain old C-string in many ways, so I think it's clearer to write it that way. The most important property in `LayerProperties` is the `layerName`, as that is what we need to pass to the `InstanceCreateInfo` to turn the layer on.

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

So far, so good.