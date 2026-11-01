# Lesson 28: Improving the Render Loop - Part 2: Stability and Performance (and yes, some more refactoring)

Alright, now that we've sorted out the fundamental synchronization flaws in our render loop, let's make a few more improvements to it so that we're well prepared for the more interesting things to come, such as lighting and texturing.

## Avoiding busy-waiting while minimized
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

The second issue isn't technically a bug: the code is correct and works. However, because `glfwPollEvents` is non-blocking, we've essentially created a tight loop that burns ~100% of a CPU core doing nothing for as long as the window is minimized. That's not good, but it's fortunately also easy to fix by using another GLFW utility function:
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
This makes sure we're not wasting CPU time while minimized. Since `glfwWaitEvents` also processes the events it waits for, our `onFramebufferSizeChanged` callback is called as soon as the window is restored, so the swapchain is recreated correctly and the render loop picks up again.

## Making the animation framerate-independent
In our current implementation, we advance the rotation of the cube by a fixed amount for every frame. The problem is that this directly couples the rotation speed to the framerate, which is in turn synced to the display refresh rate (we're waiting for vsync to show the next frame). In other words: our cube will rotate more than twice as fast on a 144Hz display than it will on a 60Hz display. That's not ideal; we want our animation to be framerate-independent[^2].

The solution is to not add a fixed increment to the rotation angle with each frame, but instead to define a rotation speed and calculate the increment by measuring the time elapsed since the last frame. We do that right at the start of each frame, directly after the minimize check:
```cpp
// main.cpp
...
float rotationAngle = 0.f;
constexpr float rotationSpeed = glm::radians( 45.f );
auto lastFrameTime = std::chrono::steady_clock::now();
...
while ( !glfwWindowShouldClose( window.get() ) )
{
    glfwPollEvents();

    if ( windowMinimized )
    {
        ...
    }

    const auto now = std::chrono::steady_clock::now();
    const float deltaTime = std::chrono::duration< float >( now - lastFrameTime ).count();
    lastFrameTime = now;
    rotationAngle += deltaTime * rotationSpeed;
    ...
}
```
As you can see, we're using `std::chrono` for the time measurement. That's the modern C++ way to measure time, and `steady_clock` is guaranteed to be monotonic - it never jumps backwards, unlike `system_clock`, which can be adjusted while our program is running.

This works, but there's two more issues we should consider: if the window is minimized or the render loop is blocked for another reason, our `deltaTime` can become pretty large, which would lead to jumps in the animation. A common way to handle this is to clamp the delta to a maximum.
The second issue is that `rotationAngle` currently grows without bound. Because a `float` only has about seven significant decimal digits, the larger the angle gets, the coarser the steps between representable values become. If the program runs long enough, our small per-frame increments will start to get rounded away, and the animation will begin to stutter. Since a rotation by 2π is the same as no rotation at all, we can simply address this by wrapping the angle back into the range [0, 2π). So let's apply both fixes:
```cpp
// main.cpp
...
const float deltaTime = std::min( std::chrono::duration< float >( now - lastFrameTime ).count(), 0.1f );
lastFrameTime = now;
rotationAngle = std::fmod( rotationAngle + deltaTime * rotationSpeed, glm::two_pi< float >() );
...
```

And with this, we have addressed another issue with our render loop.

---

[^1]: If your application still needs to do work while the window is minimized - background tasks, network I/O, animations that should keep running - you can use `glfwWaitEventsTimeout( double timeout )` instead. It behaves just like `glfwWaitEvents`, but returns after at most `timeout` seconds so you get a chance to do periodic work.
[^2]: In fact, we'd ideally want to decouple the *simulation* entirely from the *rendering*, but that's out of scope for this lesson.
