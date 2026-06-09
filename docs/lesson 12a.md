# Some Cleanup
Alright, minor change of plans again: this one is not yet going to be our start into the world of graphics programming after all. In fact we won't make any progress in terms of Vulkan programming whatsoever today. Instead I decided to slide in a chunk of code cleanup to improve the code structure that should make our future work much easier.

So far we've added all our C++ code to that one single source code file. I think that was okay up to now to keep the project simple and focus on the functionality itself. But looking at `main.cpp` now I think it's pretty obvious that this approach has reached its limits. If we were to add even more code here things would get really messy very soon. And as mentioned before: a graphics pipeline is considerably more complex than a compute one. So let's do a bit of housekeeping before we move on.

I think it's fair to assume that our program logic will look quite different, so the first thing I want to do is to get rid of all the code in `main()` that comes after the logical device creation. However, there are some valuable parts in there that we might want to keep, so we'll start by transferring those to individual functions.

I'd say being able to copy data from a C++ container to a GPU buffer is one of the things that we'll likely need again. It's not much code, but it's already somewhat duplicated right now and it clutters our `main()` function. So let's pull that functionality out:
```cpp
template< typename T, size_t N >
auto copyDataToBuffer(
    const vk::Device& logicalDevice,
    const std::array< T, N >& data,
    const GPUBuffer& buffer
) -> void
{
    const auto numBytesToCopy = sizeof( data );
    const auto mappedMemory = logicalDevice.mapMemory( *buffer.memory, 0, numBytesToCopy );
    std::memcpy( mappedMemory, data.data(), numBytesToCopy );
    logicalDevice.unmapMemory( *buffer.memory );
}

template< typename T, size_t N >
auto copyDataFromBuffer(
    const vk::Device& logicalDevice,
    const GPUBuffer& buffer,
    std::array< T, N >& data
) -> void
{
    const auto numBytesToCopy = sizeof( data );
    const auto mappedMemory = logicalDevice.mapMemory( *buffer.memory, 0, numBytesToCopy );
    std::memcpy( data.data(), mappedMemory, numBytesToCopy );
    logicalDevice.unmapMemory( *buffer.memory );
}
```
That alone doesn't help too much with the code duplication, I know. Still, the calling code becomes much more concise and we have reduced the potential for errors in terms of the number of bytes to copy, the order of arguments to `std::memcpy` or forgetting to unmap the memory[^1].

We will also have to work with command buffers and descriptors again, but at this point I do not see an obvious thing to extract from `main()`. If you want to keep the code for reference, feel free to comment it out and leave it in the file. My personal opinion here is that this is what we have git for, so I'll just go ahead now and delete everything after the logical device creation:
```cpp
int main()
{
    try
    {
        const auto instance = vcpp::createVulkanInstance();
        const auto physicalDevice = vcpp::selectPhysicalDevice( *instance );
        const auto logicalDevice = vcpp::createLogicalDevice( physicalDevice );
    }
    catch( const std::exception& e )
    {
        std::cerr << "Exception thrown: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
```
I'd say that looks clean enough, but we still have all that code above it. There's a lot in there that we'll be able to re-use so I don't want to simply delete it. Let's instead give the project a bit of structure.

I suggest to start by creating a source code file pair for everything related to the Vulkan instance and devices. `devices.hpp` should look something like this:
```cpp
#pragma once

#include <vulkan/vulkan.hpp>

namespace vcpp
{
    struct LogicalDevice
    {
        vk::UniqueDevice device;
        std::uint32_t queueFamilyIndex;

        operator const vk::Device&() const { return *device; }
    };

    auto printLayerProperties( const std::vector< vk::LayerProperties >& layers ) -> void;

    auto printExtensionProperties( const std::vector< vk::ExtensionProperties >& extensions ) -> void;

    auto createVulkanInstance() -> vk::UniqueInstance;

    auto printPhysicalDeviceProperties( const vk::PhysicalDevice& device ) -> void;

    auto selectPhysicalDevice( const vk::Instance& instance ) -> vk::PhysicalDevice;

    auto printQueueFamilyProperties( const vk::QueueFamilyProperties& props, std::uint32_t index ) -> void;

    auto findSuitableQueueFamily(
        const std::vector< vk::QueueFamilyProperties >& queueFamilies,
        vk::QueueFlags requiredFlags
    ) -> std::uint32_t;

    auto getRequiredDeviceExtensions(
        const std::vector< vk::ExtensionProperties >& availableExtensions
    ) -> std::vector< const char* >;

    auto createLogicalDevice( const vk::PhysicalDevice& physicalDevice ) -> LogicalDevice;
}
```
As you can see, I also wrapped our code in a namespace[^2]. This is good practice and will help us keeping our code unambiguous in the future. I'm not going to show the corresponding `.cpp` file here, I'm sure you can figure those out by yourself.

And of course we have to add the new files to our `CMakeLists.txt` to actually include them in the project:
```cmake
target_sources( ${PROJECT_NAME} PRIVATE devices.cpp  devices.hpp )
```

---

[^1]: Yes, we probably will have to make those functions more generic in the future, but I usually go with the YAGNI principle and only generalize as much as I need it at that point.

[^2]: `vcpp` for Vulkan C++
