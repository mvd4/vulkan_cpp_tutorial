# Lesson 12: Staging Buffers

I know that most of you will probably be itching to finally move on to graphics programming. And we'll be getting there soon, I promise. But before we do that I want to introduce one important concept that you will definitely encounter at some point.

We've created the data buffers so that they are accessible from main memory. And that's just fine, after all we need a way to copy data to and from them. However, those host-visible buffers are often not very efficient to use for the GPU. A common technique is therefore to use host-visible buffers as staging buffers, but copy the data to GPU-internal buffers before actually using it in the shaders. This is what I want to do today.

So far we've created our buffers with the hardcoded memory property flags that make them accessible from our main application. Now we want to be able to create different buffer types, which means that we need to expose those flags in the parameters of `createGPUBuffer`:
```cpp
auto createGPUBuffer(
    const vk::PhysicalDevice& physicalDevice,
    const vk::Device& logicalDevice,
    std::uint64_t size,
    vk::MemoryPropertyFlags requiredMemoryFlags =
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
) -> GPUBuffer
{
    ...
    // remove these two lines:
    // const auto requiredMemoryFlags =
    //     vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
    ...
}
```
This should still compile and run as before because of the default argument. But we can now choose to pass other flags to the function, e.g. `vk::MemoryPropertyFlagBits::eDeviceLocal` which will create a buffer that is not accessible from the host. That is only half of the battle though, because we also need a way to copy the data from our host visible buffer to the device local buffer. This is not possible out of the box, instead we need to be explicit once again and tell Vulkan that we intend to transfer memory from one buffer to another. To do that we first need to make sure our queue supports memory transfer operations by requiring another flag:
```cpp
auto createLogicalDevice( const vk::PhysicalDevice& physicalDevice ) -> LogicalDevice
{
    ...
    const auto queueFamilyIndex = findSuitableQueueFamily(
        queueFamilies,
        vk::QueueFlagBits::eCompute | vk::QueueFlagBits::eTransfer
    );
    ...
}
```
Usually a queue that has compute capabilities will also provide memory transfer, so this change shouldn't cause any issues. Secondly, we need to be able to set additional buffer usage flags to prepare the buffers for the transfer operation:
```cpp
auto createGPUBuffer(
    const vk::PhysicalDevice& physicalDevice,
    const vk::Device& logicalDevice,
    std::uint64_t size,
    vk::BufferUsageFlags usageFlags = vk::BufferUsageFlagBits::eStorageBuffer,
    vk::MemoryPropertyFlags requiredMemoryFlags =
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
) -> GPUBuffer
{
    const auto bufferCreateInfo = vk::BufferCreateInfo{}
        .setSize( size )
        .setUsage( usageFlags )
        .setSharingMode( vk::SharingMode::eExclusive );
    ...
}
```
Again, this should compile and run without any further changes. With this modified function we can now create our device local buffer:
```cpp
const auto inputGPUBuffer = createGPUBuffer(
    physicalDevice,
    logicalDevice,
    sizeof( inputData ),
    vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
    vk::MemoryPropertyFlagBits::eDeviceLocal
);
```
We also need to change the creation of the host-visible input buffer a bit so that it can act as a source for data transfers:
```cpp
const auto inputStagingBuffer = createGPUBuffer(
    physicalDevice,
    logicalDevice,
    sizeof( inputData ),
    vk::BufferUsageFlagBits::eTransferSrc
);
```
Note what happened here: where we previously had a single input buffer, we now have two — a host-visible `inputStagingBuffer` that we can upload our data to, and a device-local `inputGPUBuffer` that the shader will actually read from. I've named them accordingly to make their intended usage as clear as possible. Obviously I need to refactor the code that references the input buffer accordingly, so the host-visible upload now targets `inputStagingBuffer`.

Since we want our shader to operate on the device local GPU buffer now, we need to update our descriptor set, more exactly the buffer info for the `writeDescriptorSet`, accordingly:
```cpp
const auto bufferInfos = std::vector< vk::DescriptorBufferInfo >{
    vk::DescriptorBufferInfo{}
        .setBuffer( *inputGPUBuffer.buffer )
        .setOffset( 0 )
        .setRange( sizeof( inputData ) ),
    vk::DescriptorBufferInfo{}
        .setBuffer( *outputBuffer.buffer )
        .setOffset( 0 )
        .setRange( sizeof( outputData ) ),
};
```
If you compile and run the program now it will complete without errors but you'll see that the output data is no longer correct. That is because our shader uses the device local input buffer now, but that one doesn't contain our data yet. We still need to transfer it from the staging buffer to the device-local one. Both buffers are using GPU-managed memory and memory transfer on the GPU is a capability of the queue. So it shouldn't come as a surprise that the queue is responsible for the actual copying:
```cpp
class CommandBuffer
{
    ...
    void copyBuffer( Buffer srcBuffer, Buffer dstBuffer, const container_t< BufferCopy >& regions, ... ) const noexcept;
    ...
};
```
The first two parameters are clear, only the container of `BufferCopy`s needs a bit more attention. It turns out that `copyBuffer` can actually copy multiple blocks of memory in the same call. Each block is specified by one element in that container. The interface of `BufferCopy` is accordingly pretty straightforward.
```cpp
struct BufferCopy
{
    ...
    BufferCopy& setSrcOffset( DeviceSize srcOffset_ );
    BufferCopy& setDstOffset( DeviceSize dstOffset_ );
    BufferCopy& setSize( DeviceSize size_ );
    ...
}
```
We only have to copy one block of data, so the command we issue looks like this:
```cpp
commandBuffer.copyBuffer(
    *inputStagingBuffer.buffer,
    *inputGPUBuffer.buffer,
    vk::BufferCopy{}.setSize( sizeof( inputData ) )
);
```
However, just issuing the copy command is not enough. GPU commands within a command buffer can execute out of order or overlap, so we need to make sure the transfer has actually completed before the compute shader tries to read from the buffer. This is where pipeline barriers come in. A pipeline barrier is a synchronization primitive that creates an execution and memory dependency between commands recorded before and after it. In our case, we need a buffer memory barrier that ensures all transfer writes are visible to subsequent shader reads:
```cpp
const auto bufferBarrier = vk::BufferMemoryBarrier{}
    .setSrcAccessMask( vk::AccessFlagBits::eTransferWrite )
    .setDstAccessMask( vk::AccessFlagBits::eShaderRead )
    .setSrcQueueFamilyIndex( vk::QueueFamilyIgnored )
    .setDstQueueFamilyIndex( vk::QueueFamilyIgnored )
    .setBuffer( *inputGPUBuffer.buffer )
    .setOffset( 0 )
    .setSize( sizeof( inputData ) );
commandBuffer.pipelineBarrier(
    vk::PipelineStageFlagBits::eTransfer,
    vk::PipelineStageFlagBits::eComputeShader,
    {},
    {},
    bufferBarrier,
    {}
);
```
The `srcAccessMask` and `dstAccessMask` specify what kind of memory access needs to be synchronized. The first two arguments to `pipelineBarrier` specify the source and destination pipeline stages — in our case, we want the transfer stage to complete before the compute shader stage begins. Without this barrier, the compute shader could start reading from the buffer before the copy has finished, leading to undefined results.

The copying and the barrier can happen before we bind the pipeline, or even after we've bound the descriptor sets — remember, the descriptor set essentially tells the pipeline where to find the data and in which format it is. The actual memory is only accessed once the shader program is dispatched, so as long as the data is there and properly synchronized when that happens, all is good. If you run the program now, you should see the correct results as before.

Note that we only staged the input buffer here — the output buffer is still host-visible so we can read the results back directly. If you wanted to, you could apply the exact same technique in reverse: compute into a device-local buffer, then copy it back into a host-visible staging buffer before mapping it. I'll leave that as an exercise for you.

Whether or not this technique actually produces a speedup depends on your concrete use case. Creating the additional buffer takes time, so does the data transfer between the buffers. The specific GPU and its memory layout will also have an impact. If you only have a small data set, and/or you access the data only once, it might actually be faster to just go with the plain host-visible buffers. If your buffer holds data that is uploaded once and then used over and over again, you'll almost certainly get a speedup. As always when trying to optimize performance, you'll need to measure to be able to assess the impact.

So, that's it for our compute pipeline. There's a lot more to encounter here, but this is not supposed to be an in-depth Vulkan compute tutorial. In the next lesson we're finally going to move on to graphics.
