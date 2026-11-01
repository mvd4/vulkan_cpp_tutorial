# Lesson 28: Improving the Render Loop - Part 2: Stability and Performance (and yes, some more refactoring)

Alright, now that we've sorted out the fundamental synchronization flaws in our render loop, let's make a few more improvements to it so that we're well prepared for the more interesting things to come, such as lighting and texturing.

## Avoid busy-waiting when minimized
First, I want to tackle two issues with the way we handle a minimized window. Our current code does it like this:
```cpp
// main.cpp
...
void onFramebufferSizeChanged( [[maybe_unused]] GLFWwindow* window, int width, int height )
{
    windowMinimized = width == 0 && height == 0;
    framebufferSizeChanged = true;
}
...

while ( !glfwWindowShouldClose( window.get() ) )
{
    glfwPollEvents();

    if ( windowMinimized )
        continue;

    ...
}
```

There are two issues here, one bug and one pretty significant shortcoming:

The bug is that we consider the window to be minimized only when both dimensions are 0. If you resize the window so that only one of its dimensions becomes 0, the program will most likely crash because it tries to recreate the swapchain with a zero-sized extent, which is not allowed. Luckily that's trivial to fix, we just need to switch the operator in `onFramebufferSizeChanged`:
```cpp
// main.cpp
...
void onFramebufferSizeChanged( [[maybe_unused]] GLFWwindow* window, int width, int height )
{
    windowMinimized = width == 0 || height == 0;
    framebufferSizeChanged = true;
}
...
```
By considering the window minimized if height _or_ width is 0, we no longer try to create a swapchain in those cases and the crash is gone.

The second issue isn't technically a bug: the code is correct and works. However, because glfwPollEvents is non-blocking, we've essentially created a tight loop that burns ~100% of a CPU core doing nothing for as long as the window is minimized. That's not good, but it's fortunately also easy to fix by using another `glfw` utility function:
```c
void glfwWaitEvents();
```

This function blocks until new events come in.[^1] So we can improve our loop like this:
```cpp
// main.cpp
while ( !glfwWindowShouldClose( window.get() ) )
{
    glfwPollEvents();

    if ( windowMinimized )
    {
        glfwWaitEvents();
        continue;
    }

    ...
}
```
This makes sure we're not wasting CPU time while minimized. Since `glfwWaitEvents` also processes the events it waits for, our `onFramebufferSizeChanged`callback is called as soon as the window is restored, so the swapchain is recreated correctly and the render loop picks up again.


---

[^1]: If your application still needs to do work while the window is minimized - background tasks, network I/O, animations that should keep running - you can use `glfwWaitEventsTimeout( double timeout )` instead. It behaves just like `glfwWaitEvents`, but returns after at most `timeout` seconds so you get a chance to do periodic work.