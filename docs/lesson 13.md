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

In the last lesson we invested all that effort to clean up our codebase, so let's stick to the good habits from now on and try to avoid cluttering `main.cpp`. We'll have quite a bit of code relating to GLFW, therefore I suggest to create a new source code file pair `glfw_utils` (don't forget to add the files to the CMakeLists.txt).

Before we can do anything useful with it we need to initialize the library. GLFW offers the function `int glfwInit()` for that purpose, which seems reasonable enough. Unfortunately, as a well behaved program, we are also supposed to call the corresponding `glfwTerminate()` function when we're done using GLFW. As C++ programmers we tend to dislike this pattern and would much rather use RAII in such cases. So let's do exactly that and wrap the calls in a wrapper class[^3]:
```cpp
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace vcpp
{
    class GlfwInstance
    {
    public:

        GlfwInstance();
        ~GlfwInstance();

        GlfwInstance( const GlfwInstance& ) = delete;
        GlfwInstance( GlfwInstance&& ) = delete;

        GlfwInstance& operator= ( const GlfwInstance& ) = delete;
        GlfwInstance& operator= ( GlfwInstance&& ) = delete;
    };
}
```
GLFW was originally written for OpenGL (hence the name). Since we are already including `vulkan.hpp` above, we define `GLFW_INCLUDE_NONE` to prevent GLFW from redundantly pulling in the Vulkan C headers — all the Vulkan types GLFW needs are already available through our existing include. I've deleted the copy constructors and assignment operators for our class to make sure we don't accidentally terminate GLFW by creating another instance of our class. The corresponding implementation in the `.cpp` file looks like this:
```cpp
namespace vcpp
{
    GlfwInstance::GlfwInstance()
    {
        if ( auto result = glfwInit(); result != GLFW_TRUE )
            throw std::runtime_error( std::format( "Could not init glfw. Error {}", result ) );
    }

    GlfwInstance::~GlfwInstance() { glfwTerminate(); }
}
```
Using GLFW correctly is now a simple one-liner (not counting the necessary `#include`):
```cpp
const auto glfw = vcpp::GlfwInstance{};
```

Don't forget to add the new files to `CMakeLists.txt`, then the project should build and run fine.

---

[^1]: Even if we were to go full screen from the start, it would still technically be a window
[^2]: In fact, you can absolutely use Vulkan's graphics capabilities without ever rendering anything to a window / screen, e.g. if you just want to render stuff on a server and then save it to a file without displaying it anywhere.
[^3]: A note here: in many tutorials you will see people wrap GLFW initialization, window-creation, application run-loop and more in one big class. I am personally not a fan of this approach as this quickly leads to a loss of flexibility and clarity and has negative effects on modularity and testability of the code. So I keep my classes as small as possible until I see a clear benefit in making them larger. As far as I can tell this also corresponds to a general move to more functional patterns in C++ and other languages.
