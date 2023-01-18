# A brief overview of the Vulkan C++ wrapper
The Vulkan C++ wrapper is part of the official SDK you just installed. To use it, all you have to do is `#include <vulkan/vulkan.hpp>` (1). There is also a [dedicated github repo](https://github.com/KhronosGroup/Vulkan-Hpp).
The structs, classes and functions provided by this header are all just thin wrappers around the actual Vulkan C-API, so in general it should be pretty easy to find your way around the C++ interface. However, I think it's useful to point out a few concepts and patterns that will make it easier to follow this tutorial.

## Namespace
Everything in the Vulkan C++ wrapper is by default located in the namespace `vk`. This namespace replaces the `Vk...` prefix of the functions and classes in the Vulkan C-API. So e.g. the C++ equivalent to Vullkan's `vkCreateInstance` is `vk::createInstance`.
You can override the default namespace name by defining the preprocessor constant `VULKAN_HPP_NAMESPACE` to something else before including `vulkan.hpp`. We'll not use this feature though and just go with the default in this tutorial.

## Error handling and function results
The Vulkan C-API uses result-codes for error handling. Most functions return `VK_SUCCESS` if they complete without errors and an error code otherwise. By default the C++ wrapper converts those error codes into exceptions. Therefore the C++ do not need to use out-parameters and instead can return their result directly. E.g. this Vulkan C function 
```
VkResult vkCreateInstance( const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator,  VkInstance*  pInstance );
```
becomes the following in the C++ wrapper:
```
vk::Instance createInstance( const InstanceCreateInfo& createInfo, vk::Optional< const AllocationCallbacks > allocator, ... );   // will throw in case of error
```

The exceptions thrown by Vulkan C++ are of a type that derives from `vk::LogicError` or `vk::SystemError`, which themselves derive from `std::logic_error` and `std::system_error` respectively, so you can catch all of them as `std::exception`. The exceptions wrap the original error code converted to a `std::error_code`.

It is also possible to turn off exceptions by defining `VULKAN_HPP_NO_EXCEPTIONS` before including the header. In that case, runtime errors will be handled by the macro `VULKAN_HPP_ASSERT` which defaults to `assert` as defined in the `cassert` header. You can override this macro as well to your custom assertion. Again, we'll stick to the default of using exceptions for error handling in this tutorial.

Some Vulkan functions may return a result that is not really an error, but neither has the function completed successfully. So the C++ wrapper would be wrong in throwing an exception, but it would equally be wrong to just return a result. For those cases the C++ wrapper uses the class `ResultValue`:
```
template <typename T>
struct ResultValue
{
    ...
    Result  result;   // Result is a scoped enum that enumerates the Vulkan result codes
    T       value;

    operator T& ();
    operator const T& ()
};
```
So, you still can use the result as if it was just the plain result type in most cases. However, this class also allows you to check the result code if it really was a full success.

For clarity reasons I'll simplify the return type to the `T` type when showing `vk` function signatures in most cases.

## Vulkan constants and flags
There is an vast number of constants and flags defined by Vulkan C-API. The C++ wrapper converts all of those to scoped enumerations, so you'd e.g. use `vk::BlendFactor::eOneMinusSrcColor` instead of the original Vulkan constant `VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR`. That doesn't save you much typing, but it adds type-safety and a bit of readability (at least imho).

One thing that might confuse you a bit is the fact that there are always two different C++ types for every set of Vulkan flags: one called `...Flags` (e.g. `BufferCreateFlags`) and one called `...FlagBits` (e.g. `BufferCreateFlagBits`). I'll go into a bit more detail in a second, but the TL;DR is: just use the `...FlagBits` as you would use the C-flags, even if a function expects the `...Flags` type.

Now for the explanation: Vulkan flags are implemented as bitmasks, so you can combine them with the `|` operator. The scoped enumerations in the C++ interface normally wouldn't allow that out of the box, which is why the C++ wrapper implements the `|` and `&` operators for every set of `...FlagBits`. 

However, that alone would still limit the usecases as you cannot define operators like `|=` or `&=` as freestanding functions. They need to be member functions, but scoped enums cannot have member functions. Therefore the creators of the Vulkan wrapper introduced the `...Flags` classes which wrap the corresponding `...FlagBits` and offer the full set of operators.  
This probably sounds a bit complicated, but for the users of the SDK it adds a lot of convnience as they can use the `...FlagBits` just as they'd use plain old bitmasks and still benefit from the advantages of C++ scoped enums.

The C++ wrapper also provides a `vk::to_string` function for all those scoped enums, so you can easily output their values to the console etc.

## RAII
One of the most important patterns to improve the clarity and correctness of C++ code is [RAII](https://en.cppreference.com/w/cpp/language/raii) (Resource Acquisition is Initialization). A good C++ library therefore better support this pattern.
Fortunately, the Vulkan C++ wrapper isn't letting us down here. However, its RAII support is somewhat opt-in as you have to use the right functions and types, so let's have a closer look at it.

My guess is that this design choice is based on the nature of the Vulkan C API: Vulkan entities are represented by handles there, you can think of them as being conceptually equivalent to unique IDs or pointers. Since the C-API requires you to do the cleanup yourself (and to do it correctly), you're free to copy those handles around as much as you wish. All that matters is that you release each acquired resource exactly once.

This concept obviously doesn't play well with RAII where you want the objects to automatically clean up the resources they acquired upon destruction. One potential solution would be to make the C++ types move-only, another one to use some sort of reference counting internally.

The creators of the Vulkan C++ wrapper chose another way. The classes and structs themselves are not releasing their resources automatically. To achieve RAII behaviour you have to wrap them in a template class called  `vk::UniqueHandle`. This class is very similar to `std::unique_ptr` in both it's interface and behaviour: 
* you call member functions on the wrapped object with the `->` operator
* you dereference the handle (i.e. obtain a reference to the wrapped object) using `*`
* `UniqueHandles` can be moved but not copied
* when destroyed, `UniqueHandles` automatically call the correct Vulkan function to free the underlying resources

In most cases you don't have to do the wrapping yourself. Instead, the C++ interface offers two versions of every creation function that returns an object which references a Vulkan resource. Here's an example
```
struct Device
{
    ...
    // simplified function declarations for clarity
    Buffer               createBuffer      ( const BufferCreateInfo& createInfo );
    UniqueHandle<Buffer> createBufferUnique( const BufferCreateInfo& createInfo );
    ...
};
```
I've simplified the function signatures and return types for more clarity. As you can see the functions are equivalent except that in the second case the returned `vk::Buffer` is wrapped in a `vk::UniqueHandle`. This pattern ( `create...` returns plain C++ object, `create...Unique` returns the object wrapped in a `UniqueHandle`) is the same throughout the Vulkan C++ interface.

We'll use the `create...Unique` versions exclusively in this tutorial.

## Named Parameter Idiom
The [named parameter idiom](https://www.cs.technion.ac.il/users/yechiel/c++-faq/named-parameter-idiom.html) is an emulation of a language feature that is available in other languages (e.g. Python, Dart, Ruby )(2). It also has quite a few similarities with the classic builder pattern, only that it doesn't require any additional classes. The basic idea is that instead of initializing all fields of a class/struct directly via constructor arguments, each field is initialized explicitly via a setter function. The trick is that the setters return a reference to the object itself (just like the assignment operators do) and thus can be chained. So you can initialize every struct in the Vulkan C++ wrapper using this pattern:
```
const auto appInfo = vk::ApplicationInfo{}
    .setPApplicationName( "Vulkan C++ Tutorial" )
    .setApplicationVersion( 1u )
    .setPEngineName( "Vulkan C++ Tutorial Engine" )
    .setEngineVersion( 1u )
    .setApiVersion( VK_API_VERSION_1_1 );
```
All data members of structs defined by the C++ header are public, so you could also directly manipulate their values. We're not going to do that though and stick to the setters.

## Arrays and containers
The original Vulkan API follows the common C-Pattern for dynamic arrays: they are passed around and stored as a pointer to the first element and an element count. This is usually considered bad-practice in C++ because it's error-prone. Instead higher-level constructs like `std` containers are preferred. The Vulkan C++ API is no exception here, it offers the template classes `vk::ArrayProxy` and `vk::ArrayProxyNoTemporaries` for that purpose. They may look intimidating at first, but they actually provide a great convenience to you: whenever a function expects a parameter of one of those types, you can pass in a `std::array`, `std::vector` or `std::initializer_list` without any sort of casting or conversion. Even more convenient: you can even pass in a single value. This makes the calling code really straightforward.

AFAIK the only difference between the two classes is that  `vk::ArrayProxyNoTemporaries` has explicitly `delete`d all move constructors, i.e. you can't pass in any temporary objects. 
I'll use the type alias `container_t` for either of the two in the code examples. My intention is to make it clear that this parameter essentially expects a container of values to be passed in and not a Vulkan object that you need to create explicitly.

Note that the Vulkan C++ API also always offers the C interface for arrays as you can see in the following example:

```
struct RenderPassBeginInfo
{
    ...
    // function signatures simplified for clarity reasons
    RenderPassBeginInfo& setClearValueCount( uint32_t clearValueCount_ );
    RenderPassBeginInfo& setPClearValues( const ClearValue* pClearValues_ );
    RenderPassBeginInfo& setClearValues( const container_t<const ClearValue>& clearValues_ );
    ...
};
```
So it's up to you whether you use the C-style function pair `set...Count` / `setP...` or the single C++ function. This is a C++ tutorial, so we'll be using the C++ API whenever possible.

## Dispatching
You will notice quickly that many functions in the C++ wrapper have an argument `Dispatch`. That always has a default value, so in most cases you can ignore it.  However, I think it's important to understand what it is for, therefore I'll try to quickly explain it:

The core Vulkan API is very stable, and so it is feasible to expose it via a static loader library. The Vulkan C++ wrapper in turn can just use those statically linked functions internally. The same is true for some of the most common extensions - they are included in the loader library. However, if you want to use functions from extensions that are not included, you need to find a way to make them accessible. There are basically two options here: you provide a loader library that includes those extension functions as well, or you link to them dynamically. 

The C++ wrapper provides maximum flexibility here by giving every function that calls into the Vulkan API the `Dispatch` argument which allows you to customize the loading. It defaults to using the standard static loader library as described above, and we'll just leave it at that whenever we can. This is why I'll also usually omit it when I show function signatures.

## Conclusion
You now should have a rough overview about the concepts used by the Vulkan C++ API. As mentioned previously: for clarity reasons I'll simplify the `vk` function signatures to the parts that are relevant for us and thus omit some parameters and qualifiers like `noexcept` in the code examples from now on.
With that being said, I think we're now prepared to dive into Vulkan for real, which we'll start doing in the next lesson.

---

1. Note that, while the C++ support comes as a header-only library, you still have to link against the Vulkan binary library to use it.
2. The C++ 20 standard actually does support named parameters (they are called 'designated inititalizers'), and the Vulkan C++ interface also can be configured to work with those. We'll not do that in this tutorial because the standard is pretty new and chances are that many people are not yet on a compiler supporting it fully.