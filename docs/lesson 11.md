# Lesson 11: Executing the Compute Pipeline

We left the last lesson with the command buffer being ready to take in commands. Looking back to the beginning of that lesson, what we originally wanted to do was something like this:
```cpp
// pseudo-code
use_our_pipeline();
use_our_descriptor_set_in_pipeline();
run_our_pipeline_on_compute_queue();
```
Let's see how well that is translatable to actual Vulkan code. Turns out that the first step is very straightforward as we have the following function at our avail:
```cpp
class CommandBuffer
{
    ...
    void bindPipeline( PipelineBindPoint pipelineBindPoint, Pipeline pipeline, ... ) const noexcept;
    ...
};
```
`PipelineBindPoint` defines what sort of pipeline we are about to bind, i.e. whether it's a graphics-, compute- or raytracing pipeline. The `pipeline` is, well, our pipeline.

So the first step is covered, let's look at the next one. We want to tell the pipeline that it is supposed to use the descriptor set we created. And again, `CommandBuffer` seems to offer just what we need:
```cpp
class CommandBuffer
{
    ...
    void bindDescriptorSets(
        PipelineBindPoint pipelineBindPoint,
        PipelineLayout layout,
        uint32_t firstSet,
        const container_t< const vk::DescriptorSet >& descriptorSets,
        const container_t< const uint32_t >& dynamicOffsets,
        ... ) const noexcept;
    ...
};
```
That's quite a few parameters, let's unpack:
- `pipelineBindPoint` is the same as before
- `layout` is exactly what it says it is: the pipeline layout which we defined for our pipeline before actually creating it.
- `firstSet`: As mentioned earlier, a pipeline can use multiple descriptor sets. Those can be bound individually, so here we define which is the first descriptor set 'slot' in the pipeline that we want to bind our descriptor set to.
- `descriptorSets` is straightforward again.
- `dynamicOffsets` would only be relevant if we'd use dynamic uniform buffers or dynamic storage buffers. We can pass an empty collection for now.

We seem to have all we need for that step as well. The last command we wanted to record is the one that actually runs our pipeline. This is called dispatching in Vulkan, and sure enough there's a function to do that:
```cpp
class CommandBuffer
{
    ...
    void dispatch( uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ, ... ) const noexcept;
    ...
};
```
We talked about dispatching global workgroups already a bit in the lesson about shaders. What we're doing with this command is to start a global workgroup that consists of `groupCountX * groupCountY * groupCountZ` local workgroups. As with the local workgroups themselves, the 3-dimensional organization of the global workgroup is more or less pure convenience.

So how do we structure our global workgroup? We have a dataset of 500 values that need to be processed. In our shader code we specified the local workgroup size to be 64 invocations. Which means we need `ceil(500/64) = 8` local workgroups, covering 512 invocations in total. The shader's bounds check (`processingIndex >= BUFFER_SIZE`) ensures the 12 extra invocations exit early without touching out-of-bounds memory. We also only used the x dimension of `gl_WorkGroupID` when calculating the index in the dataset in our shader code. We therefore need to make `groupCountX = 8` and set the other two parameters to 1.

Okay, that turned out to be easier than we thought, right? Let's convert our pseudo code into code that works. One minor challenge is that we encapsulated the pipeline layout in the pipeline creation function. So we need to pull it out:
```cpp
auto createPipelineLayout(
    const vk::Device& logicalDevice,
    const vk::DescriptorSetLayout& descriptorSetLayout
) -> vk::UniquePipelineLayout
{
    const auto pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo{}
        .setSetLayouts( descriptorSetLayout );
    return logicalDevice.createPipelineLayoutUnique( pipelineLayoutCreateInfo );
}

auto createComputePipeline(
    const vk::Device& logicalDevice,
    const vk::PipelineLayout& pipelineLayout,
    const vk::ShaderModule& computeShader
) -> vk::UniquePipeline
{
    const auto shaderStageInfo = vk::PipelineShaderStageCreateInfo{}
        .setStage( vk::ShaderStageFlagBits::eCompute )
        .setPName( "main" )
        .setModule( computeShader );

    const auto pipelineCreateInfo = vk::ComputePipelineCreateInfo{}
        .setStage( shaderStageInfo )
        .setLayout( pipelineLayout );

    return logicalDevice.createComputePipelineUnique( vk::PipelineCache{}, pipelineCreateInfo ).value;
}
```
And with that we can record our commands into the buffer, so the relevant parts of `main` look like this now:
```cpp
int main()
{
    try
    {
        ...
        const auto descriptorSetLayout = createDescriptorSetLayout( logicalDevice );
        const auto pipelineLayout = createPipelineLayout( logicalDevice, *descriptorSetLayout );
        const auto pipeline = createComputePipeline( logicalDevice, *pipelineLayout, *computeShader );

        ...

        const auto beginInfo = vk::CommandBufferBeginInfo{}
            .setFlags( vk::CommandBufferUsageFlagBits::eOneTimeSubmit );
        commandBuffer.begin( beginInfo );

        commandBuffer.bindPipeline( vk::PipelineBindPoint::eCompute, *pipeline );
        commandBuffer.bindDescriptorSets( vk::PipelineBindPoint::eCompute, *pipelineLayout, 0, descriptorSets, {} );
        commandBuffer.dispatch( 8, 1, 1 );

        commandBuffer.end();
    }
    ...
}
```
Cool, we've got our command buffer ready.