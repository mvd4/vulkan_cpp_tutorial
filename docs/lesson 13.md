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
