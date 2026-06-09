# Some Cleanup
Alright, minor change of plans again: this one is not yet going to be our start into the world of graphics programming after all. In fact we won't make any progress in terms of Vulkan programming whatsoever today. Instead I decided to slide in a chunk of code cleanup to improve the code structure that should make our future work much easier.

So far we've added all our C++ code to that one single source code file. I think that was okay up to now to keep the project simple and focus on the functionality itself. But looking at `main.cpp` now I think it's pretty obvious that this approach has reached its limits. If we were to add even more code here things would get really messy very soon. And as mentioned before: a graphics pipeline is considerably more complex than a compute one. So let's do a bit of housekeeping before we move on.

I think it's fair to assume that our program logic will look quite different, so the first thing I want to do is to get rid of all the code in `main()` that comes after the logical device creation. However, there are some valuable parts in there that we might want to keep, so we'll start by transferring those to individual functions.

I'd say being able to copy data from a C++ container to a GPU buffer is one of the things that we'll likely need again. It's not much code, but it's already somewhat duplicated right now and it clutters our `main()` function. So let's pull that functionality out:
```cpp
template< typename T, size_t N >
auto copyDataToBuffer(
    const vk::Device& logicalDevice,
    const std::array< T, N >& data,
    const GPUBuffer& buffer
) -> void
{
    const auto numBytesToCopy = sizeof( data );
    const auto mappedMemory = logicalDevice.mapMemory( *buffer.memory, 0, numBytesToCopy );
    std::memcpy( mappedMemory, data.data(), numBytesToCopy );
    logicalDevice.unmapMemory( *buffer.memory );
}

template< typename T, size_t N >
auto copyDataFromBuffer(
    const vk::Device& logicalDevice,
    const GPUBuffer& buffer,
    std::array< T, N >& data
) -> void
{
    const auto numBytesToCopy = sizeof( data );
    const auto mappedMemory = logicalDevice.mapMemory( *buffer.memory, 0, numBytesToCopy );
    std::memcpy( data.data(), mappedMemory, numBytesToCopy );
    logicalDevice.unmapMemory( *buffer.memory );
}
```
That alone doesn't help too much with the code duplication, I know. Still, the calling code becomes much more concise and we have reduced the potential for errors in terms of the number of bytes to copy, the order of arguments to `std::memcpy` or forgetting to unmap the memory[^1].

---

[^1]: Yes, we probably will have to make those functions more generic in the future, but I usually go with the YAGNI principle and only generalize as much as I need it at that point.
