# Lesson 13: Creating the Application Window

Alright, the time has finally come to look into the specifics of graphics programming with Vulkan, so let's dive straight in.

Our logical device creation function is still hardwired to create a compute queue. We need graphics capabilities now, so we want to be able to reconfigure that. Thanks to our utility function `findSuitableQueueFamily` the necessary modification is very straightforward:
```cpp
auto createLogicalDevice(
    const vk::PhysicalDevice& physicalDevice,
    const vk::QueueFlags requiredFlags
) -> LogicalDevice
{
    ...
    const auto queueFamilyIndex = findSuitableQueueFamily(
        queueFamilies,
        requiredFlags
    );
    ...
}
```
With this change in place we can now create our logical device like this:
```cpp
int main()
{
    ...
    const auto logicalDevice = vcpp::createLogicalDevice(
        physicalDevice,
        vk::QueueFlagBits::eGraphics
    );
    ...
}
```

## GLFW
So far so good. The next thing we need for graphics programming is a window[^1]. After all, we'd like to be able to see what we're programming, right? Now, window handling is a whole universe of its own. Moreover, although the concepts are very similar across all platforms, the details and concrete implementation are completely platform specific. Vulkan was designed to be a platform agnostic API, so it doesn't meddle with that stuff at all[^2]. Luckily we still don't have to implement the window support ourselves because other people have done that work for us already. We'll use the GLFW library, which is a sort of quasi-standard for that purpose.

To add glfw to our project we need to add them to our `vcpkg.json`:
```json
{
    "dependencies": [
        "glfw3",
        ...
    ]
}
```
... and to our CMakeLists.txt:
```cmake
...
find_package( glfw3 CONFIG REQUIRED )
...
target_link_libraries( ${TARGET_NAME} PRIVATE glfw Vulkan::Vulkan )
...
```

Then rebuild your CMake project to make sure everything works as before.

---

[^1]: Even if we were to go full screen from the start, it would still technically be a window
[^2]: In fact, you can absolutely use Vulkan's graphics capabilities without ever rendering anything to a window / screen, e.g. if you just want to render stuff on a server and then save it to a file without displaying it anywhere.
