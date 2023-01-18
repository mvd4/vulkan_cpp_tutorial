# Vulkan Instance and Physical Device

Alright, enough of the foreplay, let's get our hands dirty with Vulkan.

## Vulkan Instance
The first thing we'll need to do is to connect our application to the Vulkan runtime. This is done by creating a so-called Vulkan instance, which is represented by the class `vk::Instance`. This object also encapsulates the application-specific state, so it needs to exist as long as the application uses Vulkan. 
Technically it is possible to have more than one instance object in your application. However, this is not recommended and might cause issues. The only real-world example I can think of where that might make sense is if your application links to a library that uses Vulkan internally as well.

Creating an instance is done by using the function that we've already seen as an example in the last chapter:
``` 
vk::UniqueInstance vk::createInstanceUnique( const vk::InstanceCreateInfo&, ... )
```
The function takes a `vk::InstanceCreateInfo` as its only required parameter, so let's have a look at how that one is defined:
```
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
So, apparently this struct has four data fields: some flags, the application info, a collection of enabled layers and another one of enabled extensions(1), whatever those might be. 
Turns out that the flags are actually reserved for future use, so we can just leave them alone. The layers and extensions will get a lesson of their own, for now we will ignore those two as well.

Which leaves the Application Info. I am not completely sure why there is only the C-style function available here, but that's the way it is, so let's use it. We'll need to create an instance of `vk::ApplicationInfo`. Here's a simplified version of its interface:
```
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
```
const auto appInfo = vk::ApplicationInfo{}
    .setPApplicationName( "Vulkan C++ Tutorial" )
    .setApplicationVersion( 1u )
    .setPEngineName( "Vulkan C++ Tutorial Engine" )
    .setEngineVersion( 1u )
    .setApiVersion( VK_API_VERSION_1_1 );
```
What you pass in the first four parameters is completely up to you, only the last one is somewhat predefined by the Vulkan spec: the API version must denote the version of Vulkan that the application is intending to use. We're using version 1.1, which was released in 2016, i.e roughly at the same time that Vulkan started to become more widespread. Chances are that if you're able to use Vulkan at all, your driver will at least support Vulkan 1.1. 

With the application info in place we can now create our instance:
```
const auto instanceCreateInfo = vk::InstanceCreateInfo{}
    .setPApplicationInfo( &appInfo );    
const auto instance = vk::createInstanceUnique( instanceCreateInfo );
```
As described in lesson 2, we don't have to worry about the destruction of the instance - the UniqueWrapper will take care of that. Compile and run your program, it should run through without any error (without any console output too though). 

Congratulations, the first step is done, you have successfully connected your application to Vulkan. We can now start to actually work with our GPUs.

I'd like to make two minor improvements before we continue: First I'll wrap all the instance creation code in a utility function:
```
auto create_instance()
{
    ...
    return vk::createInstanceUnique( instanceCreateInfo );
}
```
Second: since the Vulkan C++ interface can throw exceptions, I'll wrap all the code in the main function in a try-catch block:
```
int main()
{
    try
    {
        const auto instance = create_instance();
    }
    catch( const std::exception& e )
    {
        std::cout << "Exception thrown: " << e.what() << "\n";
        return -1;
    }
    return 0;
}
```
That looks much cleaner to me. And with that refactoring out of the way, let's move on to the next step: the physical device selection.

## Physical Devices
In many computers there will be only one GPU, either in the form of a dedicated graphics card or integrated in the main processor. But it is also very common to have both types in the same system (e.g in Notebooks), while high-end workstations, gaming machines, servers or specialized hardware might come with multiple dedicated graphics cards. The bottom line here is: You shouldn't make any assumptions about the available devices upfront but rather check what is there on startup and then make a decision. So let's do that by using the function `vk::Instance::enumeratePhysicalDevices`. This function is really convenient, as it takes no parameters and just returns a `std::vector<vk::PhysicalDevice>`, containing one entry for each device in your system that supports Vulkan:
```
const auto physicalDevices = instance->enumeratePhysicalDevices();
if ( physicalDevices.empty() )
    throw std::runtime_error( "No Vulkan devices found" );
```

We can now iterate over the devices to get some more information about each of them. `vk::PhysicalDevice` has a pretty big interface, but here's the relevant parts for our current goal:
```
class PhysicalDevice
{
    ...
    PhysicalDeviceProperties getProperties( ... );
    PhysicalDeviceFeatures getFeatures( ... );
    ...
};
```
The properties mainly contain metadata about the physical device, such as it's name, vendor and driver version. They also contain a sub-structure called limits which contains information about the supported range of certain parameters, e.g. the maximum dimensions of the framebuffers (think: the maximum size of the images that can be rendered) or the minimum and maximum width of lines that can be drawn (if you render in Wireframe mode).
The device features are a essentially a long list of boolean flags that are set to true if the respective feature is supported by that device. Those features include things like the availability of certain shader types, the supported texture compression algorithms, anisotropic filtering and much more. 
To get a better overview about the hardware we have at hand, let's write a small function that prints out some properties and features of a device:
```
void print_physical_device_properties( const vk::PhysicalDevice& device )
{
    const auto props = device.getProperties();
    const auto features = device.getFeatures();

    std::cout <<
        "    " << props.deviceName << ":" <<
        "\n      is discrete GPU: " << ( props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu ? "yes, " : "no, " ) <<
        "\n      has geometry shader: " << ( features.geometryShader ? "yes, " : "no, " ) <<
        "\n      has tesselation shader: " << ( features.tessellationShader ? "yes, " : "no, " ) << 
        "\n      supports anisotropic filtering: " << ( features.samplerAnisotropy ? "yes, " : "no, ") <<
        "\n";
}
```
... and then call it for the device list we just obtained:
```
std::cout << "Available physical devices:\n";
for ( const auto& d : physicalDevices )
    print_physical_device_properties( d );
```
There are of course many more properties and features in the respective structs, so feel free to add output for whatever you're interested in. If you compile and run the program as described here you should see a list of the available graphics hardware on your system, similar to this one:
```
AMD Radeon Pro 560:
    is discrete GPU: yes, 
    has geometry shader: no, 
    has tesselation shader: yes, 
    supports anisotropic filtering: yes, 
Intel(R) HD Graphics 630:
    is discrete GPU: no, 
    has geometry shader: no, 
    has tesselation shader: yes, 
    supports anisotropic filtering: yes,
```
Sometimes there will be only one available device, so there isn't much of a choice: use it or forget about Vulkan. In our case here we have an integrated and a dedicated GPU so we'll have to select one, either automatically or by asking the user of your application. In many cases you'll prefer a discrete GPU over the integrated one because those are usually more powerful and support more functionality. If your application requires specific features you obviously also need to make sure that those are supported and choose the device accordingly. 
The takeaway here is: it's impossible to suggest a generic solution for device selection that will work in all cases. Since this is a tutorial, we'll just use the first discrete GPU that is available, otherwise the first physical device in the list. We'll be only using standard features, so this should be fine for our purposes(2):
```
vk::PhysicalDevice select_physical_device( const std::vector< vk::PhysicalDevice >& devices )
{
    size_t bestDeviceIndex = 0;
    size_t index = 0;
    for ( const auto& d : devices )
    {
        const auto props = d.getProperties();
        const auto features = d.getFeatures();

        const auto isDiscreteGPU = props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
        if ( isDiscreteGPU && bestDeviceIndex == 0 )
            bestDeviceIndex = index;
    
        ++index;
    }
    
    return devices[ bestDeviceIndex ];
}
```
And of course we have to call it from our main function:
```
const auto physicalDevice = select_physical_device( physicalDevices );
std::cout << "\nSelected Device: " << physicalDevice.getProperties().deviceName << "\n";
```
That's it, the next step is done: we have selected the physical device that we're going to work with. Our main function is starting to look a bit cluttered again though. So let's wrap the whole physical device creation in a function, just as we did with the instance:
```
vk::PhysicalDevice create_physical_device( const vk::Instance& instance )
{
    ...
    return physicalDevice;
}

int main()
{
    try
    {
        const auto instance = create_instance();
        const auto physicalDevices = create_physical_device( *instance );
    }
    ...
}
```
That's much better I think. Now that we have the physical device selected we need to configure it in a way so that it suits our application's needs. This is what we're going to do in the next episode.

1. The `P` in the function names refers to the fact that the containers contain `const char*` pointers. The corresponding C-style functions are named `setPpEnabledLayerNames` and `setPpEnabledExtensionNames` because they take `const char* const*` as their argument.
2. Yes, I know, I'm doing the loop over the physical devices and all calls to `getProperties` and `getFeatures` twice. So I'm duplicating code and work here. In this case I think that's okay because it improves the clarity of the code: printing information and selecting the device are two different things. You might want to do those things independently from each other, or you may want to change the implementation of either without affecting the other. So they don't belong in the same function. The performance penalty is also not relevant her, since `cout` calls are several orders of magnitude slower than everything else. But of course you're free to modify the implementation if you have other priorities.
