# Executing the Compute Pipeline

We left the last lesson with the command buffer being ready to take in commands. Looking back to the beginning of that lesson, what we originally wanted to do was something like this:
```
// pseudo-code
use_our_pipeline();
use_our_descriptor_set_in_pipeline();
run_our_pipeline_on_compute_queue();
```
Let's see how well that is translatable to actual Vulkan code. Turns out that the first step is very straightforward as we have the following function at our avail:
```
class CommandBuffer
{
    ...
    void bindPipeline( PipelineBindPoint pipelineBindPoint, Pipeline pipeline, ... ) const noexcept;
    ...
};
```
`PipelineBindPoint` defines what sort of pipeline we are about to bind, i.e. whether it's a graphics-, compute- or raytracing pipeline. The `pipeline` is, well, our pipeline. 

So the fist step is covered, let's look at the next one. We want to tell the pipeline that it is supposed to use the descriptor set we created. And again, `CommandBuffer` seems to offer just what we need:
```
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
- `dynamicOffsets` would only be relevant if we'd use uniform buffers or shader storage buffers. We can pass an empty collection for now.

We seem to have all we need for that step as well. The last command we wanted to record is the one that actually runs our pipeline. This is called dispatching in Vulkan, and sure enough there's a function to do that:
```
class CommandBuffer
{
    ...
    void dispatch( uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ, ... ) const noexcept;
    ...
};
```
We talked about dispatching global workgroups already a bit in the lesson about shaders. What we're doing with this command is to start a global workgroup that consists of `groupCountX * groupCountY * groupCountZ` local workgroups. As with the local workgroups themselves, the 3-dimensional organization of the global workgroup is more or less pure convenience.

So how do we structure our global workgroup? We have a dataset of 512 values that need to be processed. In our shader code we specified the local workgroup size to be 64 invocations. Which means we need a total of `512/64 = 8` local workgroups. We also only used the x dimension of `gl_WorkGroupID` when calculating the index in the dataset in our shader code. We therefore need to make `groupCountX = 8` and set the other two parameters to 1.

Okay, that turned out to be easier than we thought, right? Let's convert our pseudo code into code that works. One minor challenge is that we encapsulated the pipeline layout in the pipeline creation function. So we need to pull it out:
```
vk::UniquePipelineLayout create_pipeline_layout(
    const vk::Device& logicalDevice,
    const vk::DescriptorSetLayout& descriptorSetLayout
)
{
    const auto pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo{}
            .setSetLayouts( descriptorSetLayout );
    return logicalDevice.createPipelineLayoutUnique( pipelineLayoutCreateInfo );        
}

vk::UniquePipeline create_compute_pipeline(
    const vk::Device& logicalDevice,
    const vk::PipelineLayout& pipelineLayout,
    const vk::ShaderModule& computeShader
)
{
    const auto pipelineCreateInfo = vk::ComputePipelineCreateInfo{}
        .setStage(
            vk::PipelineShaderStageCreateInfo{}
                .setStage( vk::ShaderStageFlagBits::eCompute )
                .setPName( "main" )
                .setModule( computeShader )
        )
        .setLayout( pipelineLayout );

    return logicalDevice.createComputePipelineUnique( vk::PipelineCache{}, pipelineCreateInfo ).value;
}
```
And with that we can record our commands into the buffer, so the relevant parts of `main` look like this now:
```
int main()
{
    try
    {
        ...
        const auto descriptorSetLayout = create_descriptor_set_layout( logicalDevice );
        const auto pipelineLayout = create_pipeline_layout( logicalDevice, *descriptorSetLayout );
        const auto pipeline = create_compute_pipeline( logicalDevice, *pipelineLayout, *computeShader );
        
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
Cool, we've got our command buffer ready. But remember, in Vulkan there's a difference between recording the commands and actually executing them. So far the GPU didn't do anything with our pipeline or the data. We need to submit the command buffer to the queue we want to run it on. This is done by the following command:
```
class Queue
{
    ...
    void Queue::submit( const container_t< const SubmitInfo >& submitInfo, ... ) const;
    ...
};
```
Apparently we need an instance of the class `vk::Queue` that represents our compute queue. Where can we get that from? Well, since we configured our logical device to provide a queue with compute capabilities it might be worth looking at the interface of Device to see if we find something:
```
class Device
{
    ...
    Queue getQueue( uint32_t queueFamilyIndex, uint32_t queueIndex, ... ) const noexcept;
    ...
};
```
Et voilÃ  - piece of cake. We also need to have a look at the `SubmitInfo` struct before we can actually submit our command buffer:
```
struct SubmitInfo
{
    ...
    SubmitInfo& setCommandBuffers( const container_t< const CommandBuffer >& commandBuffers_ );
    SubmitInfo& setWaitSemaphores( const container_t< const Semaphore >& waitSemaphores_ ); 
    SubmitInfo& setSignalSemaphores( const container_t< const Semaphore >& signalSemaphores_ );
    SubmitInfo& setWaitDstStageMask( const container_t< PipelineStageFlags >& const waitDstStageMask_ );
    ...
};
```
That looks a bit intimidating. The first parameter is clear enough, but what about the others? The good news is that we don't yet have to care about any of them:
- Semaphores are synchronization objects, they exist in many programming languages to support multithreaded programming. In Vulkan they are used to synchronize the programs running on the GPU with the main application, or the programs running on different queues. Since we just want to run our program from start to end and then get the results, we don't need any synchronization at this point and we can ignore the semaphores for now. We'll come back to them later though.
- `waitDstStageMask_` is related to the `waitSemaphores`, so we don't need it for now either.

Which means we can actually now submit our program to the GPU:
```
const auto queue = logicalDevice.device->getQueue( logicalDevice.queueFamilyIndex, 0 );

const auto submitInfo = vk::SubmitInfo{}
    .setCommandBuffers( commandBuffer );
queue.submit( submitInfo );
```
We did configure the device to only have one queue of the family, so in our case the `queueIndex` parameter needs to be set to 0.

If you compile and run the program now, you will see the validation layer shouting at you, saying something like this:
```
> ... Attempt to destroy command pool with VkCommandBuffer 0x2147f035ea0[] which is in use. ...
```
... and more errors.

The reason is that after we submit the command buffer, we are at the end of our `main` function and consequently the application terminates. When it does, the unique handles try to destroy their respective Vulkan objects. But the command buffer has just started to execute on the GPU. GPUs are fast, but not that fast. So we get an error because the command buffer we are destroying is still in use.

Apart from the error, we also probably want to see the result of the calculation eventually. So we need to wait until the GPU has finished it's work. We could do so by using the semaphores mentioned above. However, if we just want to wait for the GPU to finish our program, there's an easier way: we just ask the logical device to do that:
```
class Device
{
    ...
    void waitIdle( ... );
    ...
};
```
This function only returns when the logical device has finished all the work. You normally would not want to do that, as it essentially blocks the program flow and causes the  pipeline to run empty. In our case however there's nothing more to do except wait for the results, therefore it's okay. If you run the program now, all errors should be gone.

Congratulations! 

You've just run your first Vulkan program on the GPU. The experience is probably a bit underwhelming though because you don't see any results. Let's change that.

We already have the GPU buffer for the output data in place. And if everything worked as expected this should also already contain our computation results. Which means that the only thing missing for us too actually get hold of the results is transferring this data back to main memory. This is essentially the same as we did when we uploaded the input data to the GPU buffer, so I'll just post the code here:
```
const auto mappedOutputMemory = logicalDevice->mapMemory( *outputBuffer.memory, 0, outputSize );
memcpy( outputData.data(), mappedOutputMemory, outputSize );
logicalDevice->unmapMemory( *outputBuffer.memory );
```
And now finally we can print the data to the console:
```
for( size_t i = 0; i < outputData.size(); ++i )
{
    std::cout << outputData[i] << ";\t";
    if ( ( i % 16 ) == 0 )
        std::cout << "\n";
}
```
Now you should see the output values that confirm that the GPU indeed executed our shader on every data point in the input buffer. At this point it might be a good idea to play around a bit with the shader and the workgroup sizes to get a feel for how they behave.

In the next lesson I want to show you an important technique related to the memory buffers before we finally get going with graphics programming.