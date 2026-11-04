# Lesson 29: Push Constants and Ambient Lighting

As promised, after all the refactoring and bugfixing toil of the last two lessons, we're going to make some visible changes again today. I want to lay the groundwork for a lighting system, beginning with ambient light. For that, I'm going to introduce a concept called *push constants*. And while we're at it, I also want to use that technique to make one more improvement to our render loop.

## Push constants
Let's start with that improvement: One thing that still bothers me about our render loop is that we multiply each individual vertex by the transformation matrices on the CPU before uploading the whole buffer to the GPU every single frame. That seems very wasteful. All the more so since our geometry never changes, only the transformation matrices do. And with its massive parallelization capabilities, the GPU would be much better suited to transforming the vertices.

So ideally, we'd upload the vertex buffer only when it changes and otherwise just send the new transformation matrices to the GPU each frame to let it do the work.

Push constants are a technique that allows us to do that. They are the fastest, smallest way to hand a shader a few bytes of data that may change for every draw. They don't require us to handle buffers, descriptors, or synchronization - the driver just ships them inline with the command buffer.

So let's give that a try. To use push constants, we have to define a `PushConstantRange`. This struct is attached to the pipeline layout and essentially tells the pipeline which shader stage(s) may read which bytes of the push constant data.

The data itself is completely opaque to the pipeline; it only knows that there is a block of data of a certain size that will be passed each time the command buffer is recorded. The interface of `PushConstantRange` looks like this:
```cpp
struct PushConstantRange
{
    ...
    PushConstantRange& setStageFlags( ShaderStageFlags stageFlags_ );
    PushConstantRange& setOffset( uint32_t offset_ );
    PushConstantRange& setSize( uint32_t size_ );
    ...
}
```

- `stageFlags_` is a bitmask of the shader stages that may access this range.
- `offset_` and `size_` define the range within the whole push constant data block. Both must be multiples of 4.

A pipeline layout can define multiple ranges (e.g. one for the vertex stage, another one for the fragment stage), but there is only one push constant data block that is shared by all ranges. So all `PushConstantRange` declarations are views into the same block of data and must therefore be compatible with each other.
