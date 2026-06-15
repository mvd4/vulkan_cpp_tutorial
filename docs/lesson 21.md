# Lesson 21: Synchronization
So, we're rendering our triangle now and that's great. However, we get a constant flow of two different validation errors. This is probably not so good, let's try to fix them.

The first error looks something like that:
```
> ... Calling vkBeginCommandBuffer() on active VkCommandBuffer 0x1de3f331090[] before it has completed ...
```
The validation complains that we are trying to begin recording a command buffer which is in use. And that is true: while we said we only ever want two buffers to be 'in flight' at the same time, we're not actually enforcing that anywhere yet. Rendering one simple triangle is so fast that it outpaces the presentation easily, so eventually we are indeed calling `vk::CommandBuffer::begin` on a buffer that is still being executed for a previous image.

So we need to make sure that the buffer is not in use anymore before recording into it. We're going to learn how to properly do that later in this lesson. For now I want to implement a temporary fix that gets us rid of the error so we can make progress.

```cpp
...
while ( !glfwWindowShouldClose( window.get() ) )
{
    glfwPollEvents();

    logicalDevice.device->waitIdle();
    ...
}
...
```

We've already used `vk::Device::waitIdle` in lesson 11. This is a very blunt tool and it costs a lot of performance because now host and GPU work sequentially instead of in parallel (effectively limiting the number of frames 'in flight' to 1). But it does the job: the validation errors about the command buffer being in use are gone. As said, we're going to implement a better solution later in this lesson.

Let's look at the remaining errors now:
```
> ... vkAcquireNextImageKHR: Semaphore must not be currently signaled or in a wait state ...
```
Alright, I think it's time to finally talk about semaphores and fences now.

## Semaphores and Fences
Both, semaphores and fences, are synchronization primitives[^1]. This is not a tutorial about asynchronous programming or multithreading, so I'm not going to go into too much detail here. Suffice it to say that if multiple tasks on a computer run in parallel, there are situations where you want to be sure that one or more of them have reached a certain state before you start doing something in the scope of another. This is e.g. always the case if you want to transfer data from one thread to another.

Conceptually, semaphores and fences work very similar. Both can be in one of two states: unsignaled or signaled. Vulkan functions that make use of them are designed so that execution of the respective task halts at unsignaled semaphores/fences and only resumes when they become signaled.

![Visualization showing the function principle of semaphores and fences: One task is waiting for an unsignaled semaphore / fence to be signaled by another task running in parallel. Only then the first task resumes execution.](images/Synchronization.png "Fig. 1: Function principle of semaphores / fences")

The difference between semaphores and fences is that the former are used to synchronize internal operations on the GPU and you have no way to explicitly signal/unsignal them from your application code. Instead you pass them to Vulkan functions such as e.g. `Queue::submit` and let the implementation take care of that. Fences on the other hand are designed to synchronize the execution flow between the host application and the GPU. You can explicitly wait for a Fence to be set (signaled) by the GPU and then reset it (put them back to the unsignaled state). You cannot signal them actively though, that's the privilege of the GPU.

## Using Semaphores
So what does that now mean in practice? We've already created a semaphore, although we did not really understand it or look at the create info structure in detail back then. As it turns out we didn't miss too much there:

```cpp
struct SemaphoreCreateInfo
{
    ...
    SemaphoreCreateInfo& setFlags( SemaphoreCreateFlags flags_ );
    ...
};
```

So the only parameter we could set are the `flags_`, but even that one is only reserved for future use. Nothing more for us to do here.

The validation error complains that the semaphore passed to `vkAcquireNextImageKHR` is 'signaled or in a wait state'. Let's see: `acquireNextImageKHR` is the first function we pass our semaphore to. Since semaphores are created in an unsignaled state this cannot be the problem, at least not in the first iteration.

According to the documentation, the semaphore passed to `acquireNextImageKHR` will be signaled when the image is ready to be rendered to. That may sound strange at first, intuitively we might expect the image to be ready immediately when the function call completes. However, to allow maximum throughput `acquireNextImageKHR` returns the index of the next image even if the presentation engine is still using that image. This enables the host application to use the index already and prepare the rendering of the next frame. When the image is finally available, rendering can start right away.

So we have a signaled semaphore now, but take a look at our code: we're not actually doing anything with it. That explains the error: after the first call to `acquireNextImageKHR`, the semaphore remains signaled forever. Effectively we're not using the semaphore at all. We need to change that, so let's see where we need it.

The next thing we do after acquiring the image is to record the command buffer. This is safe: we don't actually render anything here, so the image is not used yet and there's no problem if it's still being presented. What is not safe anymore is to submit the command buffer. Once that happens the GPU can start rendering to the image at any time, so here we need to make sure that our image is actually ready. This is where two members of `SubmitInfo` come into play that I've glossed over back in lesson 11: `setWaitSemaphores` and `setWaitDstStageMask`:

- the GPU will wait for the semaphores that are passed to `setWaitSemaphores` to be signaled before continuing execution of this command buffer.
- however, the GPU might actually be able to do some useful stuff before accessing the resources that are protected by the semaphores. So for maximum parallelization Vulkan enables you to specify exactly where in the pipeline it is supposed to wait for each semaphore. This is what `setWaitDstStageMask` is for: for each entry in this list, the GPU will wait for the corresponding semaphore to be signaled before entering the respective stage. So the containers passed to `setWaitSemaphores` and `setWaitDstStageMask` need to be of the same size.

For us that means that we can pass our semaphore directly as a wait semaphore to the submit info. The GPU will then wait until it is signaled and automatically reset it. Since this semaphore protects the image that is used as the color attachment, the wait stage should be set to `eColorAttachmentOutput`:

```cpp
...
const vk::PipelineStageFlags waitStages[] = {
    vk::PipelineStageFlagBits::eColorAttachmentOutput };
const auto submitInfo = vk::SubmitInfo{}
    .setCommandBuffers( commandBuffers[ frameInFlightIndex ] )
    .setWaitSemaphores( *semaphore )
    .setPWaitDstStageMask( waitStages );
queue.submit( submitInfo );
...
```

Cool, with that version the validation errors are gone. We're not done yet though. You might have spotted another synchronization issue already: we currently schedule the image for presentation immediately after submitting the command buffer. How is the presentation engine supposed to know when the image is actually fully rendered and thus ready to be put on screen?

The answer is: it doesn't know. It works for us at the moment because we render only one triangle, which is really fast. If we were to render a real scene chances are that the presentation would access an unfinished image. That's not good. So we need a way to tell the presentation to wait until rendering the image has completed.

In the last lesson we already learned that `PresentInfoKHR` - just like `SubmitInfo` has a member `setWaitSemaphores` which we can use to specify a list of semaphores which will block the presentation until they all become signaled. So telling the presentation to wait would be easy if we could make the GPU signal a semaphore once it's done with the rendering. And it turns out that there is yet another parameter in `SubmitInfo` that we ignored so far: `setSignalSemaphores`. This allows us to specify a number of semaphores that will be signaled once the command buffer(s) in this batch have completed execution. Which means the whole thing is actually pretty straightforward:

```cpp
...
auto readyForRenderingSemaphore = logicalDevice.device->createSemaphoreUnique(
    vk::SemaphoreCreateInfo{}
);
auto readyForPresentingSemaphore = logicalDevice.device->createSemaphoreUnique(
    vk::SemaphoreCreateInfo{}
);
...
while ( !glfwWindowShouldClose( window.get() ) )
{
    ...
    const auto imageIndex = logicalDevice->acquireNextImageKHR(
        *swapchain,
        std::numeric_limits< std::uint64_t >::max(),
        *readyForRenderingSemaphore
    ).value;
    ...
    const auto submitInfo = vk::SubmitInfo{}
        .setCommandBuffers( commandBuffers[ frameInFlightIndex ] )
        .setWaitSemaphores( *readyForRenderingSemaphore )
        .setSignalSemaphores( *readyForPresentingSemaphore )
        .setPWaitDstStageMask( waitStages );
    queue.submit( submitInfo );

    const auto presentInfo = vk::PresentInfoKHR{}
        .setSwapchains( *swapchain )
        .setImageIndices( imageIndex )
        .setWaitSemaphores( *readyForPresentingSemaphore );
    ...
}
...
```
We create another semaphore[^2] just as before and add it to the `SubmitInfo` as semaphore to signal when the command buffer has completed. We also add it to the `PresentInfoKHR` as semaphore to wait for. That way presenting will not happen before the command buffer is done with the respective image. I've renamed the semaphore we already had to make the usage of both clearer.


---

[^1]: There's a third type of synchronization primitive: events. They are used in more advanced cases, so we're not going to talk about them here.
[^2]: You might be tempted to reuse the same semaphore as for acquiring the image. After all that one will be reset once the rendering starts, so it should be fine to use it to signal render completion. The problem is that if `submit` is still waiting on the semaphore (because the image hasn't been acquired yet), the host may already have issued the `presentKHR` call by the time the semaphore gets signaled. Both calls would then be waiting on the same semaphore and would resume at the same time, effectively synchronizing rendering and presentation to start together — defeating the purpose.
