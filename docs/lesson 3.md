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


[^1]: The `P` in the function names refers to the fact that the containers contain `const char*` pointers. The corresponding C-style functions are named `setPpEnabledLayerNames` and `setPpEnabledExtensionNames` because they take `const char* const*` as their argument.
