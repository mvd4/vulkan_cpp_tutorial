# Descriptors

Before we continue, let's do a quick recap on where we are at right now:

![Current status of our Vulkan pipeline setup](images/Compute_Pipeline_Objects.png "Fig. 1: Current status of our Vulkan pipeline setup")

- we have the logical device with the appropriate queue to run a compute pipeline
- we have our input data uploaded to a buffer on the GPU
- we also have a GPU and a main memory buffer ready to store the results
- we have the compute pipeline configured to have the right descriptor set layout for our buffers
- our compute shader is finished and attached to the pipeline

What is still missing for a fully functional setup is:
- although the pipeline knows the correct descriptor set layout, we did not yet create any actual descriptor set that we could bind to the pipeline. So the pipeline still has no way to access the data in our GPU buffers.
- our device still has no idea that it is supposed to run this pipeline now on its queue (remember, we might have configured a device with multiple queues as well as created multiple pipelines. Which pipeline should run on which queue? This is something Vulkan cannot simply guess, you have to tell it)
- and of course we'll need to copy our results back to main memory after the computation, so that we can use them

We'll take care of creating the descriptor sets today. In the next lesson we'll then finally put everything together.

## Allocating Descriptor Sets
If you search a bit in the Vulkan C++ interface you will find that the function we probably want to use to create our descriptor set is this one:
```cpp
class Device
{
    ...
    std::vector< DescriptorSet > allocateDescriptorSets( const DescriptorSetAllocateInfo&, ... );
    ...
};
```
Interesting, why is this function called `allocate...` instead of `create...`? And why is there no `...Unique` version of it? Let's look at the `DescriptorSetAllocateInfo` to try and answer these questions:
```cpp
struct DescriptorSetAllocateInfo
{
    ...
    DescriptorSetAllocateInfo& setSetLayouts( const container_t< const DescriptorSetLayout >& setLayouts_ );
    DescriptorSetAllocateInfo& setDescriptorPool( DescriptorPool descriptorPool_ );
    ...
};
```
`setSetLayouts` is straightforward enough: we need to provide a layout for each descriptor set we want to allocate. So in our case we just need to pass the one descriptor set layout we created in the last lesson, because that's the only descriptor set we need. But what about the `DescriptorPool`?

Turns out that descriptor sets are not just created like most other structures we've come across so far. Instead they are allocated from a `DescriptorPool`. The concept is pretty much the same as with memory pools in C / C++ and it's obviously another optimization in Vulkan. This also explains the absence of a unique handle for the descriptor sets - since they are allocated from a pool, they will be cleaned up automatically when the pool is destroyed[^1].

So we need to create such a pool before we can allocate the descriptor sets. Creating the pool follows the familiar pattern:
```cpp
vk::UniqueDescriptorPool createDescriptorPoolUnique( const DescriptorPoolCreateInfo& createInfo );
```
... and the `DescriptorPoolCreateInfo` looks like this:
```cpp
struct DescriptorPoolCreateInfo
{
    ...
    DescriptorPoolCreateInfo& setFlags( vk::DescriptorPoolCreateFlags flags_ );
    DescriptorPoolCreateInfo& setPoolSizes( const container_t<const vk::DescriptorPoolSize >& poolSizes_ );
    DescriptorPoolCreateInfo& setMaxSets( uint32_t maxSets_ );
    ...
};
```
There are a few `DescriptorPoolCreateFlags` defined but we can ignore them for now. Let's look at the pool sizes parameter:
```cpp
struct DescriptorPoolSize
{
    ...
    DescriptorPoolSize& setType( vk::DescriptorType type_ );
    DescriptorPoolSize& setDescriptorCount( uint32_t descriptorCount_ );
    ...
};
```
Okay, that looks pretty straightforward. One instance of `DescriptorPoolSize` just represents a descriptor type and a count. So with that we define the maximum number of descriptors of a certain type the pool will be able to provide. You can specify multiple `DescriptorPoolSizes` for the same descriptor type, in which case the total number of descriptors the pool can provide will simply be the sum of all specified sizes.

But what about the `maxSets_` parameter in `DescriptorPoolCreateInfo`? Well, this defines how many descriptor sets can be allocated from the pool in total. You have to adhere to both limits, the one for the number of sets and also the one for the number of descriptors. Since that relationship between `poolSizes_` and `maxSets_` is a bit confusing, let me give you an example:
- let's say you specify the pool sizes to be two `DescriptorType::eStorageBuffer`s and two `DescriptorType::eSampledImage`s
- let's also assume you set `maxSets_` to be two
- then you could either allocate two descriptor sets, each containing one buffer and one image (so the total number of descriptors allocated is two for each descriptor type)
- or you could allocate one set with two buffers and one with two images (same thing, total number of descriptors of each type is two)
- or you could allocate one set with two buffers and one image and another one with only one image
- etc, you get the idea.

So, as it looks we now have everything to create the pool according to our needs and allocate our one required descriptor set:
```cpp
auto createDescriptorPool( const vk::Device& logicalDevice ) -> vk::UniqueDescriptorPool
{
    const auto poolSize = vk::DescriptorPoolSize{}
        .setType( vk::DescriptorType::eStorageBuffer )
        .setDescriptorCount( 2 );
    const auto poolCreateInfo = vk::DescriptorPoolCreateInfo{}
        .setMaxSets( 1 )
        .setPoolSizes( poolSize );
    return logicalDevice.createDescriptorPoolUnique( poolCreateInfo );
}

int main()
{
    try
    {
        ...
        const auto descriptorPool = createDescriptorPool( *logicalDevice );
        const auto allocateInfo = vk::DescriptorSetAllocateInfo{}
            .setSetLayouts( *descriptorSetLayout )
            .setDescriptorPool( *descriptorPool );
        const auto descriptorSets = logicalDevice->allocateDescriptorSets( allocateInfo );
    }
    ...
}
```
Nice, we have the concrete descriptor set now.

---

[^1]: You can explicitly release individual descriptor sets instead of letting them be cleaned up automatically when the pool is destroyed. In that case you need to set the `DescriptorPoolCreateFlagBits::eFreeDescriptorSet` flag when creating the pool.
