# Layers and Extensions

I promised already twice that we'd talk about layers and extensions eventually, and now is the time I deliver on that promise.

Both, layers and extensions are a way to enhance Vulkan's builtin functionality. The main difference between the two is that layers only modify or enhance behaviour that is already present, while extensions do add new functionality. We'll work with both in the course of this tutorial.

## Layers
Layers can be thought of as just what their name suggests: additional levels of functionality that an API call passes before it reaches the actual Vulkan core implementation. Layers do not necessarily modify all function calls, depending on the purpose they may leave some alone. Originally Vulkan supported layers for the entire Vulkan environment (so-called instance layers) as well as individual per-physical-device layers. The latter have been deprecated now because it became apparent that there was no real use case for them.

Speaking of use cases: One major use case for layers is adding debugging support to Vulkan. Since the Vulkan core is so optimized for maximum performance, it only does the checks that are absolutely necessary. That means that you can easily issue a function call to the Vulkan core that seems to work fine, but actually does nothing because of a faulty parameter. Or your application might crash without you having the slightest clue what went wrong. Layers can add diagnostics, logging, profiling and other helpful functionality. And because they have to be explicitly switched on to become active, you can just leave them disabled when you ship your application and get the maximum Vulkan performance in production.

Of course, to be able to activate a layer, one should be able to detect whether it is actually supported on the respective system. Here's the function that lists all the available layers for the instance:
```
std::vector< vk::LayerProperties > vk::enumerateInstanceLayerProperties( ... );
```
As you can see, this function is not a member of `vk::instance`. That makes sense because we already need to pass in the names of the layers we want to enable when we create the instance. The returned `vk::LayerProperties` structs have the following properties:
```
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
```
...
void print_layer_properties( const std::vector< vk::LayerProperties >& layers )
{
    for ( const auto& l : layers )
        std::cout << "    " << l.layerName << "\n";        
    
    std::cout << "\n";
}

vk::UniqueInstance create_instance()
{
    const auto layers = vk::enumerateInstanceLayerProperties();
    std::cout << "Available instance layers: \n";    
    print_layer_properties( layers );
    ...
}
...
```
When you run the program now you should see something like this as the first output:
```
Available instance layers:
    VK_LAYER_NV_optimus
    VK_LAYER_LUNARG_api_dump
    VK_LAYER_LUNARG_device_simulation
    VK_LAYER_LUNARG_gfxreconstruct
    VK_LAYER_KHRONOS_synchronization2
    VK_LAYER_KHRONOS_validation
    VK_LAYER_LUNARG_monitor
    VK_LAYER_LUNARG_screenshot
    VK_LAYER_LUNARG_standard_validation
```
In this example the NVIDIA Optimus layer is available on the system, along with some by the Khronos Group (the industry consortium that created the Vulkan standard) and some by LunarG (the company that maintains the official Vulkan SDK).

So far, so good. We'll get back to some of those layers in a minute. Let's look at extensions first (as said, device-specific layers have been deprecated, so we'll not cover them here).

## Extensions
In contrast to layers, extensions can actually add new functionality to Vulkan. Many of those extensions will only become relevant for you once you start exploring more advanced stuff. Still, there is one family of extensions that is widely used and that we will need in this tutorial as well: the Khronos surface extensions. We'll be talking about surfaces in depth when we get to the graphics stuff. But let me give you a quick intro here:

One of the main design principles for Vulkan is it's platform-independence. There's nothing in the Vulkan core that is specific to one platform. Drawing onto a screen on the other hand is extremely platform specific, especially in a windowed context. Obviously the drawing functionality can not go into the Vulkan core, therefore it is realized by platform-specific extensions.

But first things first, let's now have a look which extensions we actually have available. The pattern is the same as the one for layers:
```
...
void print_extension_properties( const std::vector< vk::ExtensionProperties >& extensions )
{
    for ( const auto& e : extensions )
        std::cout << "    " << e.extensionName << "\n";

    std::cout << "\n";
}
...
vk::UniqueInstance create_instance()
{
    const auto layers = vk::enumerateInstanceLayerProperties();
    std::cout << "Available instance layers: \n";    
    print_layer_properties( layers );

    const auto instanceExtensions = vk::enumerateInstanceExtensionProperties();
    std::cout << "Available instance extensions: \n";
    print_extension_properties( instanceExtensions );

    ...
```
If you run now, you'll hopefully see output along the lines of the following:
```
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
Et voilà, there they are: `VK_KHR_surface`, `VK_KHR_win32_surface` and `VK_KHR_surface_protected_capabilities`(1).

But wait, we're not done yet: in contrast to the layers, a specific device can have it's own set of extensions on top of that. And as if that weren't enough already, instance layers can also come with extensions. Since we're currently working with the instance, let's finish that off first. 

As it turns out you can pass the name of a layer to `enumerateInstanceExtensionProperties` and that'll give you the extensions for that specific layer. Let's enhance our `print_layer_properties` function accordingly(2):
```
void print_layer_properties( const std::vector< vk::LayerProperties >& layers )
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
Indeed, if you run that version you'll see that e.g. the Khronos validation layer comes with three extensions. 

Now let's complete our tour by looking at the device specific extensions. For that purpose we extend the `print_physical_device_properties` function as follows:
```
void print_physical_device_properties( const vk::PhysicalDevice& device )
{
    const auto props = device.getProperties();
    const auto features = device.getFeatures();

    std::cout <<
        "  " << props.deviceName << ":" <<
        "\n      is discrete GPU: " << ( props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu ? "yes, " : "no, " ) <<
        "\n      has geometry shader: " << ( features.geometryShader ? "yes, " : "no, " ) <<
        "\n      has tesselation shader: " << ( features.tessellationShader ? "yes, " : "no, " ) <<
        "\n      supports anisotropic filtering: " << ( features.samplerAnisotropy ? "yes, " : "no, ") <<
        "\n";

    const auto deviceExtensions = device.enumerateDeviceExtensionProperties();
    std::cout << "\n  Available device extensions: \n";
    print_extension_properties( deviceExtensions );
}
```
This is probably going to give you a long list of device specific extensions. Most of them will be irrelevant for this tutorial, but it's good to know how much functionality you could potentially use.

## Debugging
So, now that we know more about layers and extensions, let's make use of some. As said, one main usecase for layers is debugging, so that's what we'll do here as well. 

You might remember that we can set a list of instance layers to enable in the `InstanceCreateInfo`. So let's do that and enable the Khronos validation layer and the corresponding extensions:
```
vk::UniqueInstance create_instance()
{
    ...
    const auto layersToEnable = std::vector< const char* >{
        "VK_LAYER_KHRONOS_validation"
    };
    const auto extensionsToEnable = std::vector< const char* >{
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME };

    const auto instanceCreateInfo = vk::InstanceCreateInfo{}
        .setPApplicationInfo( &appInfo )
        .setPEnabledLayerNames( layersToEnable )
        .setPEnabledExtensionNames( extensionsToEnable );

    return vk::createInstanceUnique( instanceCreateInfo );
}
```
As you can see, there are constants defined for the names of some extensions (in `vulkan_core.h`), not for the layers though.

If you build and run the program now you might already see the layer in action, depending on the platform you're on. The (quite lengthy) error message seems to be complaining about the `VK_KHR_portability_subset` extension not being enabled. This extension is used on systems where Vulkan is in fact implemented as a wrapper aroud another graphics API. You'll likely encounter this on apple computers for example, where Vulkan is implemented on top of Apple's own Metal API. 

So it looks like we'll have to enable this extension, but only if it's present. Let's do that by modifying our `create_logical_device` function a bit:

```
vk::UniqueDevice create_logical_device( const vk::PhysicalDevice& physicalDevice )
{
    ...
    const auto enabledDeviceExtensions = get_required_device_extensions(
        physicalDevice.enumerateDeviceExtensionProperties()
    );
    const auto deviceCreateInfo = vk::DeviceCreateInfo{}
        .setQueueCreateInfos( queueCreateInfos )
        .setPEnabledExtensionNames( enabledDeviceExtensions );

    return physicalDevice.createDeviceUnique( deviceCreateInfo );
}
```
with
```
std::vector< const char* > get_required_device_extensions(
    const std::vector< vk::ExtensionProperties >& availableExtensions
)
{
    auto result = std::vector< const char* >{};
    
    static const std::string compatibilityExtensionName = "VK_KHR_portability_subset";    
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


1. These are the extensions that are present on my Windows system. Obviously you will not have the `...win32...` extension on a macos or Linux machine but something platform specific.
2. In case you're wondering about the `l.layerName.operator std::string()`: an implicit or explicit cast don't work here. I'm not entirely sure why that is because obviously there is a casting operator available. But it only works if I create a named `std::string` variable or call the casting operator explicitly.
