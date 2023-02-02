# Pipeline and Swapchain Recreation
In lesson 21 we invested quite a bit of effort to make our render loop more stable. And we made significant progress. However, if you play around with our application you'll notice that there's still a few things that don't work as we would like them to: try minimizing the application or resizing the window and you'll get an exception. There's also the check for `vk::Result::eSuboptimalKHR` that we needed to add for the app to work on all systems and which feels more like a workaround than a solution.

All of those issues are symptoms of the same underlying problem: we create our pipeline and swapchain with a fixed window size, and as soon as that is no longer matching the actual dimensions of the window we're in trouble. Fixing this is pretty simple in theory: we just need to destroy the existing pipeline and swapchain and recreate it with the correct size. In practice this is a bit easier said than done however, so this is what we'll look into today.

The first thing we should probably take care of is making our application actually aware of a window size change. GLFW provides a utility function for this purpose:

```C++
typedef void (* GLFWframebuffersizefun)(GLFWwindow* window, int width, int height);

GLFWframebuffersizefun glfwSetFramebufferSizeCallback( GLFWwindow* window, GLFWframebuffersizefun callback );
```

Whenever the size of the window (and thus the framebuffer) changes, GLFW will call the function you set here. Unfortunately the signature doesn't allow for custom data to be passed to the function, so we need to use global variables:

```C++
bool windowMinimized = false;
void on_framebuffer_size_changed( GLFWwindow* window, int width, int height )
{
    windowMinimized = width == 0 && height == 0;
}

int main()
{
    ...
    try
    {
        const auto glfw = vcpp::glfw_instance{};
        const auto window = vcpp::create_window( windowWidth, windowHeight, "Vulkan C++ Tutorial" );
        glfwSetFramebufferSizeCallback( window.get(), on_framebuffer_size_changed );
        ...
```

With that we can skip rendering altogether if the window is minimized:

```C++
...
while ( !glfwWindowShouldClose( window.get() ) )
{
    glfwPollEvents();

    if ( windowMinimized )
        continue;
    ...
```

Now we can minimize and restore our window without getting an exception. One problem solved, nice.

But what if the window size has actually changed? As said in the beginning: in that case we need to recreate all the objects that are dependent on the framebuffer size (aka `swapchainExtent` in our code), which are the pipeline and the swapchain. We don't have to think about the command buffers because they are being re-recorded for each frame anyway.

Before we look into recreating those objects though it might be helpful to find out what the new size actually is. To do this we could use the values that we receive in `on_framebuffer_size_changed`. However, that would require two more global variables and we would like to avoid that if possible. Luckily there's a better way:

```C++
class PhysicalDevice
{
    ...
    SurfaceCapabilitiesKHR getSurfaceCapabilitiesKHR( SurfaceKHR surface, ... ) const
    ...
};
```

The returned `vk::SurfaceCapabilitiesKHR` struct has a member `currentExtent` which will always be the current extent of the swapchain framebuffers - exactly what we need. With that in mind we can get started. First we need another flag that tells us when the window has changed size:

```C++
bool windowMinimized = false;
bool framebufferSizeChanged = true;
void on_framebuffer_size_changed( GLFWwindow* window, int width, int height )
{
    windowMinimized = width == 0 && height == 0;
    framebufferSizeChanged = true;
}
```

We simply set the flag whenever the callback is invoked(1). Note that we start out with the flag being set to true - this will come in handy as we'll see.

Now we want to react to a changed size by recreating the pipeline and the swapchain. Let's start with the pipeline as that's the more obvious one:

```C++
while ( !glfwWindowShouldClose( window.get() ) )
{
    glfwPollEvents();

    if ( windowMinimized )
        continue;

    if ( framebufferSizeChanged )
    {
        logicalDevice.device->waitIdle();

        pipeline.reset();

        const auto capabilities = physicalDevice.getSurfaceCapabilitiesKHR( *surface );
        
        pipeline = create_graphics_pipeline(
            logicalDevice,
            *vertexShader,
            *fragmentShader,
            *renderPass,
            capabilities.currentExtent );

        framebufferSizeChanged = false;
    }
    ...
}
```

Before we delete the pipeline we need to wait until the GPU isn't using it anymore, so we're using `waitIdle` again. Then we reset `pipeline`, which is strictly speaking not necessary because it would implicitly be done anyway when we reassign the pointer. But I think it doesn't hurt and makes the code a bit more expressive. Then we query the swapchain capabilities and create a new pipeline with the updated `currentExtent`. This all would not yet work because `pipeline` is declared as a `const` variable. We could simply remove the qualifier, but actually we don't need to create the pipeline before the render loop at all anymore. Since we initialized `framebufferSizeChanged` to `true`, the program will enter our recreation code right at the first cycle of the render loop. So we can change the variable declaration to:

```C++
vk::UniquePipeline pipeline;
```

That was pretty easy. Too bad our swapchain is not a `unique_ptr` so that we could do exactly the same with it. But wait, why don't we just make it a `unique_ptr` and implement the same pattern for it? Let's give it a try. We'll implement a creation function to abstract away the call to `make_unique` and to match the pattern for the pipeline creation:

``` C++
using swapchain_ptr_t = std::unique_ptr< vcpp::swapchain >;

swapchain_ptr_t create_swapchain(
    const vk::Device& logicalDevice,
    const vk::RenderPass& renderPass,
    const vk::SurfaceKHR& surface,
    const vk::SurfaceFormatKHR& surfaceFormat,
    const vk::Extent2D& imageExtent,
    std::uint32_t maxImagesInFlight 
)
{
    return std::make_unique< vcpp::swapchain >(
        logicalDevice,
        renderPass,
        surface,
        surfaceFormat,
        imageExtent,
        maxImagesInFlight );
}
```

And with that we can do exactly the same with the swapchain as with the pipeline:

``` C++
...

vk::UniquePipeline pipeline;
vcpp::swapchain_ptr_t swapchain;
vk::Extent2D swapchainExtent;

while ( !glfwWindowShouldClose( window.get() ) )
{
    glfwPollEvents();

    if ( windowMinimized )
        continue;

    if ( framebufferSizeChanged )
    {
        logicalDevice.device->waitIdle();

        pipeline.reset();
        swapchain.reset();

        const auto capabilities = physicalDevice.getSurfaceCapabilitiesKHR( *surface );
        swapchainExtent = capabilities.currentExtent;

        pipeline = create_graphics_pipeline(
            logicalDevice,
            *vertexShader,
            *fragmentShader,
            *renderPass,
            swapchainExtent );

        swapchain = create_swapchain(
            logicalDevice,
            *renderPass,
            *surface,
            surfaceFormats[0],
            swapchainExtent,
            requestedSwapchainImageCount );

        framebufferSizeChanged = false;
    }

    ...
}
```

If you compile and run this version you'll find that it does not have any of the resizing problems anymore. We can also get rid of the check for `vk::Result::eSuboptimalKHR`, so we've achieved everything that we set out to do. Thanks to our refactoring last time this turned out to be pretty simple in the end.

So far, so good. We're still quite far from a fully functional rendering loop for real-world usage, but we're making progress. Next time I want to look at how we can move the geometry to render out of the shader and into our application.


1. Yes, there is a small optimization possible here: we could check whether the new extent is equal to the old one and avoid a pipeline / swapchain recreation in the case of a restore after a minimize. Personally I think this is not worth the effort because won't happen often and a minimal delay won't hurt the user experience either in this case.