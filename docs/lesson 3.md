# Lesson 3: Vulkan Instance and Physical Device

Alright, enough of the foreplay, let's get our hands dirty with Vulkan.

## Vulkan Instance

The first thing we'll need to do is to connect our application to the Vulkan runtime. This is done by creating a so-called Vulkan instance, which is represented by the class `vk::Instance`. This object also encapsulates the application-specific state, so it needs to exist as long as the application uses Vulkan.
Technically it is possible to have more than one instance object in your application. However, this is not recommended and might cause issues. The only real-world example I can think of where that might make sense is if your application links to a library that uses Vulkan internally as well.

Creating an instance is done by using the function that we've already seen as an example in the last chapter:
```cpp
vk::UniqueInstance vk::createInstanceUnique( const vk::InstanceCreateInfo&, ... )
```
The function takes a `vk::InstanceCreateInfo` as its only required parameter, so let's have a look at how that one is defined:
```cpp
struct InstanceCreateInfo
{
    ...
    InstanceCreateInfo& setFlags( vk::InstanceCreateFlags flags_ );
    InstanceCreateInfo& setPApplicationInfo( const vk::ApplicationInfo* pApplicationInfo_ );
    InstanceCreateInfo& setPEnabledLayerNames( const container_t<const char* const >& pEnabledLayerNames_ );
    InstanceCreateInfo& setPEnabledExtensionNames( const container_t<const char* const >& pEnabledExtensionNames_ );
    ...
};
```
So, apparently this struct has four data fields: some flags, the application info, a collection of enabled layers and another one of enabled extensions[^1], whatever those might be.
Turns out that, for the most part, we can just leave the flags alone for now. The layers and extensions will get a lesson of their own, for now we will ignore those two as well.

> A note for those of you working on MacOS: with Vulkan SDK version 1.3.216 there has been a change that requires you to enable the portability subset extension explicitly. Failing to do so will yield an exception "Incompatible Driver".
>
> We'll cover extensions in detail in one of the next lessons, but I've updated the code in the repository with a patch that allows you to run the app already now. For more information refer to [this article](https://stackoverflow.com/questions/72789012/why-does-vkcreateinstance-return-vk-error-incompatible-driver-on-macos-despite)

Which leaves the Application Info. I am not completely sure why there is only the C-style function available here, but that's the way it is, so let's use it. We'll need to create an instance of `vk::ApplicationInfo`. Here's a simplified version of its interface:
```cpp
struct ApplicationInfo
{
    ...
    ApplicationInfo& setPApplicationName( const char* pApplicationName_ );
    ApplicationInfo& setApplicationVersion( uint32_t applicationVersion_ );
    ApplicationInfo& setPEngineName( const char* pEngineName_ );
    ApplicationInfo& setEngineVersion( uint32_t engineVersion_ );
    ApplicationInfo& setApiVersion( uint32_t apiVersion_ );
    ...
};
```
As you can see this structure contains some meta-information about the application that is about to use Vulkan. It is actually optional to set this data, but a well-behaved program should do so. The information enables the driver to identify your application and potentially adjust some parameters accordingly. This will be absolutely irrelevant for the small tutorial app we're going to write, but AMD and NVIDIA do optimize their drivers for performance of big AAA games. So, it's a best practice that doesn't cost us much, therefore lets just adhere to it:
```cpp
const auto appInfo = vk::ApplicationInfo{}
    .setPApplicationName( "Vulkan C++ Tutorial" )
    .setApplicationVersion( 1u )
    .setPEngineName( "Vulkan C++ Tutorial Engine" )
    .setEngineVersion( 1u )
    .setApiVersion( VK_API_VERSION_1_1 );
```
What you pass in the first four parameters is completely up to you, only the last one is somewhat predefined by the Vulkan spec: the API version must denote the version of Vulkan that the application is intending to use. We're using version 1.1, which was released in 2018, i.e. roughly two years after the initial Vulkan release. Chances are that if you're able to use Vulkan at all, your driver will at least support Vulkan 1.1.

With the application info in place we can now create our instance:
```cpp
auto instanceCreateInfo = vk::InstanceCreateInfo{}
    .setPApplicationInfo( &appInfo );
const auto instance = vk::createInstanceUnique( instanceCreateInfo );
```
As described in lesson 2, we don't have to worry about the destruction of the instance - the UniqueWrapper will take care of that. Compile and run your program, it should run through without any error (without any console output too though).

Congratulations, the first step is done, you have successfully connected your application to Vulkan. We can now start to actually work with our GPUs.

I'd like to make two minor improvements before we continue: First I'll wrap all the instance creation code in a utility function:
```cpp
auto createVulkanInstance() -> vk::UniqueInstance
{
    ...
    return vk::createInstanceUnique( instanceCreateInfo );
}
```
Second: since the Vulkan C++ interface can throw exceptions, I'll wrap all the code in the main function in a try-catch block:
```cpp
int main()
{
    try
    {
        const auto instance = createVulkanInstance();
    }
    catch( const std::exception& e )
    {
        std::cerr << "Exception thrown: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
```
That looks much cleaner to me. And with that refactoring out of the way, let's move on to the next step: the physical device selection.

## Physical Devices
In many computers there will be only one GPU, either in the form of a dedicated graphics card or integrated in the main processor. But it is also very common to have both types in the same system (e.g in Notebooks), while high-end workstations, gaming machines, servers or specialized hardware might come with multiple dedicated graphics cards. The bottom line here is: You shouldn't make any assumptions about the available devices upfront but rather check what is there on startup and then make a decision. So let's do that by using the function `vk::Instance::enumeratePhysicalDevices`. This function is really convenient, as it takes no parameters and just returns a `std::vector<vk::PhysicalDevice>`, containing one entry for each device in your system that supports Vulkan:
```cpp
// instance is a vk::UniqueInstance; -> dereferences the smart pointer to access vk::Instance
const auto physicalDevices = instance->enumeratePhysicalDevices();
if ( physicalDevices.empty() )
    throw std::runtime_error( "No Vulkan devices found" );
```

We can now iterate over the devices to get some more information about each of them. `vk::PhysicalDevice` has a pretty big interface, but here's the relevant parts for our current goal:
```cpp
class PhysicalDevice
{
    ...
    PhysicalDeviceProperties getProperties( ... );
    PhysicalDeviceFeatures getFeatures( ... );
    ...
};
```
The properties mainly contain metadata about the physical device, such as its name, vendor and driver version. They also contain a sub-structure called limits which contains information about the supported range of certain parameters, e.g. the maximum dimensions of the framebuffers (think: the maximum size of the images that can be rendered) or the minimum and maximum width of lines that can be drawn (if you render in Wireframe mode).
The device features are essentially a long list of boolean flags that are set to true if the respective feature is supported by that device. Those features include things like the availability of certain shader types, the supported texture compression algorithms, anisotropic filtering and much more.
To get a better overview about the hardware we have at hand, let's write a small function that prints out some properties and features of a device:
```cpp
auto printPhysicalDeviceProperties( const vk::PhysicalDevice& device ) -> void
{
    const auto props = device.getProperties();
    const auto features = device.getFeatures();

    std::cout <<
        "    " << props.deviceName << ":" <<
        "\n      is discrete GPU: " << ( props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu ? "yes" : "no" ) <<
        "\n      has geometry shader: " << ( features.geometryShader ? "yes" : "no" ) <<
        "\n      has tessellation shader: " << ( features.tessellationShader ? "yes" : "no" ) <<
        "\n      supports anisotropic filtering: " << ( features.samplerAnisotropy ? "yes" : "no" ) <<
        "\n";
}
```
... and then call it for the device list we just obtained:
```cpp
std::cout << "Available physical devices:\n";
for ( const auto& d : physicalDevices )
    printPhysicalDeviceProperties( d );
```
There are of course many more properties and features in the respective structs, so feel free to add output for whatever you're interested in. If you compile and run the program as described here you should see a list of the available graphics hardware on your system, similar to this one:
```text
    AMD Radeon Pro 560:
      is discrete GPU: yes
      has geometry shader: no
      has tessellation shader: yes
      supports anisotropic filtering: yes
    Intel(R) HD Graphics 630:
      is discrete GPU: no
      has geometry shader: no
      has tessellation shader: yes
      supports anisotropic filtering: yes
```


[^1]: The `P` in the function names refers to the fact that the containers contain `const char*` pointers. The corresponding C-style functions are named `setPpEnabledLayerNames` and `setPpEnabledExtensionNames` because they take `const char* const*` as their argument.
