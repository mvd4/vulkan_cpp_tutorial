# Creating the Application Window

Alright, the time has finally come to look into the specifics of graphics programming with Vulkan, so let's dive straight in.

Our logical device creation function is still hardwired to create a compute queue. We need graphics capabilities now, so we want to be able to reconfigure that. Thanks to our utility function `get_queue_index` the necessary modification is very straightforward:
```
logical_device create_logical_device( 
    const vk::PhysicalDevice& physicalDevice,
    const vk::QueueFlags requiredFlags
)
{
    ...
    const auto queueFamilyIndex = get_suitable_queue_family(
        queueFamilies,
        requiredFlags
    );
    ...
}
```
With this change in place we can now create our logical device like this:
```
int main()
{
    ...
    const auto logicalDevice = vcpp::create_logical_device( 
        physicalDevice,
        vk::QueueFlagBits::eGraphics );
    ...
}
```

## GLFW
So far so good. The next thing we need for graphics programming is a window(1). After all, we'd like to be able to see what we're programming, right? Now, window handling is a whole universe of its own. Moreover, although the concepts are very similar across all platforms, the details and concrete implementation are completely platform specific. Vulkan was designed to be a platform agnostic API, so it doesn't meddle with that stuff at all (2). Luckily we still don't have to implement the window support ourselves because other people have done that work for us already. We'll use the GLFW library, which is a sort of quasi-standard for that purpose.
We're also going to use the format libary for string manipulation. It is part of the C++ standard library since C++20, but since we're still using the older C++17 standard here, we have to use it's open source predecessor.

To add those libraries to our project we need to add them to our conanfile.txt:
```
[requires]
    glfw/3.3.6
    fmt/6.1.2
    ...
```
Then, from within your build folder, run `conan install ..` and rebuild your CMake project to make sure everything works as before.

In the last lesson we invested all that effort to clean up our codebase, so let's stick to the good habits from now on and try to avoid cluttering `main.cpp`. We'll have quite a bit of code relating to GLFW, therefore I suggest to create a new source code file pair `glfw_utils` (don't forget to add the files to the CMakeLists.txt).

Before we can do anything useful with it we need to initialize the library. GLFW offers the function `int glfwInit()` for that purpose, which seems reasonable enough. Unfortunately, as a well behaved program, we are also supposed to call the corresponding `glfwTerminate()` function when we're done using GLFW. As C++ programmers we tend to dislike this pattern and would much rather use RAII in such cases. So let's do exactly that and wrap the calls in a wrapper class(3):
```
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace vcpp
{
    class glfw_instance
    {
    public:

        glfw_instance();
        ~glfw_instance();

        glfw_instance( const glfw_instance& ) = delete;
        glfw_instance( glfw_instance&& ) = delete;

        glfw_instance& operator= ( const glfw_instance& ) = delete;
        glfw_instance& operator= ( glfw_instance&& ) = delete;
    };
}
```
GLFW was originally written for OpenGL (hence the name). Therefore we need to define `GLFW_INCLUDE_VULKAN` to make it work with Vulkan as well. I've deleted the copy constructors and assignment operators for our class to make sure we don't accidentally terminate GLFW by creating another instance of our class. The corresponding implementation in the `.cpp` file looks like this:
```
namespace vcpp
{
    glfw_instance::glfw_instance()
    {
        if ( auto result = glfwInit(); result != GLFW_TRUE )
            throw std::runtime_error( fmt::format( "Could not init glfw. Error {}", result ) );
    }

    glfw_instance::~glfw_instance() { glfwTerminate(); }
}
```
Using GLFW correctly is now a simple one-liner (not counting the necessary `#include`):
```
const auto glfw = vcpp::glfw_instance{};
```

## Creating the Window
Now we'd like to have a window. Again GLFW uses the default C pattern by providing the functions `glfwCreateWindow` and `glfwDestroyWindow`. And again, we'd like to be able to package that into an RAII pattern. This time, because `glfwCreateWindow` returns a pointer to the created window, we can make use of C++' `unique_ptr`:
```
using window_ptr_t = std::unique_ptr< GLFWwindow, decltype( &glfwDestroyWindow ) >;
```
Since the call for creating the window pointer is not very concise, and since we'll probably want to set some properties for the window in the future, we'll put the window creation into a utility function again:
```
window_ptr_t create_window( int width, int height, const std::string& title )
{
    return window_ptr_t{ 
        glfwCreateWindow( width, height, title.c_str(), nullptr, nullptr ),
        glfwDestroyWindow
    };
}
```
And now we can again create the window with one simple call:
```
int main()
{
    try
    {
        const auto glfw = vcpp::glfw_instance{};
        const auto window = vcpp::create_window( 800, 600, "Vulkan C++ Tutorial" );
    ...
```
If you run the program now, you'll probably see a window flashing up for a moment and then vanishing again. That's perfectly correct - our program executes its `main` function and when it reaches the end of that it terminates and thus destroys the window. Obviously this is not how we want our application to behave though. We would like the application to run and the window to stay open until we close it explicitly. That's where the _'run loop'_ or _'event loop'_ comes into play. I'm not going to go into any details about that in this tutorial, it would blow up the scope way too much. Suffice it to say that the run loop is essentially just a normal loop which in every cycle checks for OS events (such as mouse events or key strokes) and processes them. If the user or the operating system tell the application to terminate, the loop is exited. For GLFW a very basic run loop looks like this:
```
while ( !glfwWindowShouldClose( window.get() ) )
{
    glfwPollEvents();
}
```
So in every iteration of the loop we let GLFW poll for new operating system events. We don't do anything explicit with them yet, but calling the poll function enables GLFW to do some magic under the hood (without that, the call to `glfwWindowshouldClose` wouldn't work correctly and we couldn't exit the application by closing the window). Compile and run the program now and you will see that we get a window that behaves exactly as we wanted it to.

Alright, we have our window, now we'd like to draw to it. The trouble is: since the Vulkan core itself has no idea about windows, it also doesn't know how to render into one. So what do we do?

## Window System Integration and Surfaces
Well, the creators of Vulkan obviously knew that presentation (i.e. rendering to a screen or window) would be a very common requirement, so they took care that this problem be solved. The solution they came up with is to have the presentation support be implemented in instance extensions which are commonly referred to as the _'Windows System Integration (WSI)'_ extensions. There is a platform-independent _'VK_KHR_surface'_ extension which defines a  generic interface for a concept called 'surface'. You can think of a surface as a sort of a canvas that Vulkan can render to. The actual implementation of the surface is then provided by additional platform-specific extensions. So the whole thing works pretty much the same as abstract base classes and derived implementation classes in C++. Vulkan can use the abstract interface and the platform-specific implementation takes care of the actual presentation.

If you want to verify that you have the surface extensions installed take a look at the instance extensions that our application prints out. You should find `VK_KHR_surface` among the names, along with a few other surface-related extensions.

For us this means that if we want to render to our window we need to enable those extensions. To do that we could now simply add the respective extension names to our `extensionsToEnable` vector in `create_instance`. The problem with that however is that some of the required extensions are obviously platform specific. So we'd need to use preprocessor `#defines` or something similar to keep our application platform agnostic. Luckily there is an easier way because GLFW already has a function that tells us which extensions we need to enable on the current system:
```
const char** glfwGetRequiredInstanceExtensions( uint32_t* count );
```
This one returns a C-array of C-Strings with the names of the required extensions. The size of that array is returned in the output parameter count. Since I want to keep all GLFW code in `glfw_utils`, I'll add a function to wrap that call(4):
```
std::vector< std::string > get_required_extensions_for_glfw()
{
    std::vector< std::string > result;
    std::uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );
    for( std::uint32_t i = 0; i < glfwExtensionCount; ++i )
        result.push_back( glfwExtensions[i] );
    return result;
}
```
We could now call this function from inside `create_instance` directly and add the extensions to our vector of extensions to enable. That would create a tight coupling between `glfw_utils` and `devices` though, therefore I'll go with a different approach and change `create_instance` as follows:
```
vk::UniqueInstance create_instance( const std::vector< std::string >& requiredExtensions )
{
    ...
    auto extensionsToEnable = std::vector< const char* >{
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME 
    };

    for ( const auto& e : requiredExtensions )
        extensionsToEnable.push_back( e.c_str() );
    ...
}
```
The call in main then changes to:
```
const auto instance = vcpp::create_instance( vcpp::get_required_extensions_for_glfw() );
```

Now that we have the extensions enabled, we can actually create the surface. Like with the necessary extensions, GLFW abstracts away all the platform specifics here, so that we only have to use this function:
```
VkResult glfwCreateWindowSurface( VkInstance instance, GLFWwindow* window, const VkAllocationCallbacks* allocator, VkSurfaceKHR* surface );
```
We can ignore the allocator callback, the rest of the parameters should be straightforward. It's of course a C function again, so we'd have to manually manage the `surface` pointer which we don't really want to do. Luckily the creators of the C++ wrapper seemed to have thought the same, so they created a `vk::UniqueSurfaceKHR` class. We don't get away from using the C function (the Vulkan c++ wrapper only seems to have C++ versions of the platform specific functions), but at least we can then wrap the returned pointer in a c++ class:
```
vk::UniqueSurfaceKHR create_surface( 
    const vk::Instance& instance,
    GLFWwindow& window
)
{
    VkSurfaceKHR surface;
    if ( 
        const auto result = glfwCreateWindowSurface( instance, &window, nullptr, &surface );
        result != VK_SUCCESS 
    ) 
    {
        throw std::runtime_error( fmt::format( "failed to create window surface. Error: {}", result ) );
    }

    vk::ObjectDestroy< vk::Instance, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE > deleter{ instance };
    return vk::UniqueSurfaceKHR{ vk::SurfaceKHR( surface ), deleter };    
}
```
For correctness we should create the surface before the logical device, because the selection of the appropriate physical device and queue may actually depend on the surface(5). We therefore call our new function right after creating the instance:
```
int main()
{
    try
    {
        const auto glfw = vcpp::glfw_instance{};
        const auto window = vcpp::create_window( 800, 600, "Vulkan C++ Tutorial" );

        const auto instance = vcpp::create_instance();
        const auto surface = vcpp::create_surface( *instance, *window );
        ...
```

However, if you run the program now you will get an exception because the surface creation failed. Looking up the error code that is returned from the GLFW function yields the constant `VK_ERROR_NATIVE_WINDOW_IN_USE_KHR`. How can that be? I mean we just created the window and definitely didn't use it yet.

What bites us here is again the fact that GLFW was originally written for OpenGL and only extended to use Vulkan later. When creating the window, GLFW already also created an OpenGL context for that window under the hood. That is not compatible with a Vulkan surface, hence our attempt to create one fails. Fortunately the solution for this is easy, we just need to tell GLFW to not create that OpenGL context. To do that we have to call the function `glfwWindowHint` with the appropriate parameters:

window_ptr_t create_window( int width, int height, const std::string& title )
{
    glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
    ...
}

And with that everything should work again.

That's it for today. We've covered quite a bit of ground and are now well prepared to start looking into how to setup a graphics pipeline in Vulkan. That's what we'll do next time.

1. Even if we were to go full screen from the start, it would still technically be a window
2. In fact, you can absolutely use Vulkan's graphics capabilities without ever rendering anything to a window / screen, e.g. if you just want to render stuff on a server and then save it to a file without displaying it anywhere.
3. A note here: in many tutorials you will see people wrap GLFW initialization, window-creation, application run-loop and more in one big class. I am personally not a fan of this approach as this quickly leads to a loss of flexibility and clarity and has negative effects on modularity and testability of the code. So I keep my classes as small as possible until I see a clear benefit in making them larger. As far as I can tell this also corresponds to a general move to more functional patterns in C++ and other languages.
4. A `vector< const char* >` would have done as well here as the pointers point to static strings within GLFW. But it's never a good idea to rely on implementation details, especially not in code that you don't control. Therefore I'll rather accept the small overhead of creating strings here - the function is probably not going to be called more than once anyway.
5. Theoretically it is possible that the queue we selected does not support drawing to a surface (aka 'presenting' in Vulkan lingo). I have personally never encountered that and will therefore ignore this possibility to not make the tutorial more complicated than necessary. If you really find yourself in this situation, please drop me a message and I'll update this lesson to handle that scenario as well.