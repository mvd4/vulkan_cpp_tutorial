# Some Cleanup
Alright, minor change of plans again: this one is not yet going to be our start into the world of graphics programming after all. In fact we won't make any progress in terms of Vulkan programming whatsoever today. Instead I decided to slide in a chunk of code cleanup to improve the code structure that should make our future work much easier.

So far we've added all our C++ code to that one single source code file. I think that was okay up to now to keep the project simple and focus on the functionality itself. But looking at `main.cpp` now I think it's pretty obvious that this approach has reached its limits. If we were to add even more code here things would get really messy very soon. And as mentioned before: a graphics pipeline is considerably more complex than a compute one. So let's do a bit of housekeeping before we move on.

I think it's fair to assume that our program logic will look quite different, so the first thing I want to do is to get rid of all the code in `main()` that comes after the logical device creation. However, there are some valuable parts in there that we might want to keep, so we'll start by transferring those to individual functions.

I'd say being able to copy data from a C++ container to a GPU buffer is one of the things that we'll likely need again. It's not much code, but it's already somewhat duplicated right now and it clutters our `main()` function. So let's pull that functionality out:
```
template< typename T, size_t N >
void copy_data_to_buffer( 
    const vk::Device& logicalDevice,
    const std::array< T, N >& data,
    const gpu_buffer& buffer
)
{
    const auto numBytesToCopy = sizeof( data );
    const auto mappedMemory = logicalDevice.mapMemory( *buffer.memory, 0, numBytesToCopy );
    memcpy( mappedMemory, data.data(), numBytesToCopy );
    logicalDevice.unmapMemory( *buffer.memory );
}

template< typename T, size_t N >
void copy_data_from_buffer(
    const vk::Device& logicalDevice,
    const gpu_buffer& buffer, 
    std::array< T, N >& data
)
{
    const auto numBytesToCopy = sizeof( data );
    const auto mappedMemory = logicalDevice.mapMemory( *buffer.memory, 0, numBytesToCopy );
    memcpy( data.data(), mappedMemory, numBytesToCopy );
    logicalDevice.unmapMemory( *buffer.memory );
}
```
That alone doesn't help too much with the code duplication, I know. Still, the calling code becomes much more concise and we have reduced the potential for errors in terms of the number of bytes to copy, the order of arguments to `memcpy` or forgetting to unmap the memory (1).

We will also have to work with command buffers and descriptors again, but at this point I do not see an obvious thing to extract from `main()`. If you want to keep the code for reference, feel free to comment it out and leave it in the file. My personal opinion here is that this is what we have git for, so I'll just go ahead now and delete everything after the logical device creation:
```
int main()
{
    try
    {
        const auto instance = create_instance();
        const auto physicalDevice = create_physical_device( *instance );
        const auto logicalDevice = create_logical_device( physicalDevice );
    }
    catch( const std::exception& e )
    {
        std::cout << "Exception thrown: " << e.what() << "\n";
        return -1;
    }
    return 0;
}
```
I'd say that looks clean enough, but we still have all that code above it. There's a lot in there that we'll be able to re-use so I don't want to simply delete it. Let's instead give the project a bit of structure.

I suggest to start by creating a source code file pair for everything related to the Vulkan instance and devices. `devices.hpp` should look something like this:
```
#pragma once

#include <vulkan/vulkan.hpp>

namespace vcpp
{
    struct logical_device {
        vk::UniqueDevice device;
        std::uint32_t queueFamilyIndex;

        operator const vk::Device&() const { return *device; }
    };

    void print_layer_properties( const std::vector< vk::LayerProperties >& layers );

    void print_extension_properties( const std::vector< vk::ExtensionProperties >& extensions );

    vk::UniqueInstance create_instance();

    void print_physical_device_properties( const vk::PhysicalDevice& device );

    vk::PhysicalDevice select_physical_device( const std::vector< vk::PhysicalDevice >& devices );

    vk::PhysicalDevice create_physical_device( const vk::Instance& instance );

    void print_queue_family_properties( const vk::QueueFamilyProperties& props, unsigned index );

    std::uint32_t get_suitable_queue_family(
        const std::vector< vk::QueueFamilyProperties >& queueFamilies,
        vk::QueueFlags requiredFlags
    );

    std::vector< const char* > get_required_device_extensions(
        const std::vector< vk::ExtensionProperties >& availableExtensions
    );

    logical_device create_logical_device( const vk::PhysicalDevice& physicalDevice );
}
```
As you can see, I also wrapped our code in a namespace(2). This is good practice and will help us keeping our code unambiguous in the future. I'm not going to show the corresponding `.cpp` file here, I'm sure you can figure that one out by yourself.

Okay, let's now do the same for all the code that we have relating to memory and buffer management and put that in a separate source file pair. Here's `memory.hpp`:
```
#pragma once

#include <vulkan/vulkan.hpp>

namespace vcpp
{
    struct gpu_buffer
    {
        vk::UniqueBuffer buffer;
        vk::UniqueDeviceMemory memory;
    };


    std::uint32_t find_suitable_memory_index(
        const vk::PhysicalDeviceMemoryProperties& memoryProperties,
        std::uint32_t allowedTypesMask,
        vk::MemoryPropertyFlags requiredMemoryFlags
    );


    gpu_buffer create_gpu_buffer(
        const vk::PhysicalDevice& physicalDevice,
        const vk::Device& logicalDevice,
        std::uint32_t size,
        vk::BufferUsageFlags usageFlags = vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlags requiredMemoryFlags =
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );

    template< typename T, size_t N >
    void copy_data_to_buffer( 
        const vk::Device& logicalDevice,
        const std::array< T, N >& data,
        const gpu_buffer& buffer
    )
    {
        const auto numBytesToCopy = sizeof( data );
        const auto mappedMemory = logicalDevice.mapMemory( *buffer.memory, 0, numBytesToCopy );
        memcpy( mappedMemory, data.data(), numBytesToCopy );
        logicalDevice.unmapMemory( *buffer.memory );
    }

    template< typename T, size_t N >
    void copy_data_from_buffer( 
        const vk::Device& logicalDevice,
        const gpu_buffer& buffer, 
        std::array< T, N >& data
    )
    {
        const auto numBytesToCopy = sizeof( data );
        const auto mappedMemory = logicalDevice.mapMemory( *buffer.memory, 0, numBytesToCopy );
        memcpy( data.data(), mappedMemory, numBytesToCopy );
        logicalDevice.unmapMemory( *buffer.memory );
    }
}
```
The rest of the code in `main.cpp` is related to the creation of our compute pipeline, so let's put that in a `pipelines` source file pair:
```
#pragma once

#include <vulkan/vulkan.hpp>

#include <filesystem>

namespace vcpp
{
    vk::UniqueShaderModule create_shader_module(
        const vk::Device& logicalDevice,
        const std::filesystem::path& path
    );

    vk::UniqueDescriptorSetLayout create_descriptor_set_layout( const vk::Device& logicalDevice );
 
    vk::UniquePipelineLayout create_pipeline_layout(
        const vk::Device& logicalDevice,
        const vk::DescriptorSetLayout& descriptorSetLayout
    );

    vk::UniquePipeline create_compute_pipeline(
        const vk::Device& logicalDevice,
        const vk::PipelineLayout& pipelineLayout,
        const vk::ShaderModule& computeShader
    );

    vk::UniqueDescriptorPool create_descriptor_pool( const vk::Device& logicalDevice );
}
```

And finally we'll of course have to add all those new source files to our `CMakeLists.txt` to actually include them in the project:
```
target_sources( ${PROJECT_NAME} PRIVATE devices.cpp  devices.hpp )
target_sources( ${PROJECT_NAME} PRIVATE memory.cpp  memory.hpp )
target_sources( ${PROJECT_NAME} PRIVATE pipelines.cpp  pipelines.hpp )
```

And that's already it. Compile and run the project once more to make sure we have a working state as a starting point for the next lesson.


1. Yes, we probably will have to make those functions more generic in the future, but I usually go with the YAGNI principle and only generalize as much as I need it at that point.

2. `vcpp` for Vulkan C++