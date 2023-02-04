# Vertex Input - Part 1
Up to now, the vertex coordinates and colors are hardcoded in our shaders. This is obviously not how a real world application works because we wouldn't be able to change our scene without recompiling the shaders. So our goal for today is to make it possible to pass the vertex data to the pipeline from our application. Let's get started.

As a first step let's remove the `positions` array from the vertex shader and paste it into the main function. Needless to say that we need to modify the code a bit to make it valid C++:

```C++
constexpr size_t vertexCount = 3;
const std::array< float, 4 * vertexCount > vertices = {
    0.0, -0.5, 0.0, 1.0,
    0.5, 0.5, 0.0, 1.0,
    -0.5, 0.5, 0.0, 1.0 };
```

So we have the vertex data in the main application now but the pipeline itself doesn't know anything about this change yet. That's what we should take care of next.

Since it is such an elementary aspect of any graphics pipeline, vertex data is not treated just like any other data. Instead there is a dedicated way to feed it into the pipeline: back in lesson 15 we glanced over it, but we actually already do create and set a `PipelineVertexInputStateCreateInfo` in our `create_graphics_pipeline` function. So far we've kept it to the default-constructed state because we didn't have any vertex input. That changes now, so let's have a look at this structure:

```C++
struct PipelineVertexInputStateCreateInfo
{
    ...
    PipelineVertexInputStateCreateInfo& setFlags( vk::PipelineVertexInputStateCreateFlags flags_ );
    PipelineVertexInputStateCreateInfo& setVertexBindingDescriptions( 
        const container_t< const vk::VertexInputBindingDescription >& vertexBindingDescriptions_ );
    PipelineVertexInputStateCreateInfo& setVertexAttributeDescriptions( 
        const container_t< const vk::VertexInputAttributeDescription >& vertexAttributeDescriptions_ );
    ...
};
```

Alright, so we need to set binding descriptions and attribute descriptions (once again there are no flags defined that we could set). 

First the binding description: Vulkan allows you to bind multiple vertex input buffers to a graphics pipeline. Think of the bindings as slots that you can plug vertex buffers in. You have to describe each slot with a `VertexInputBindingDescription` to inform the pipeline about the structure of the vertex data to expect:

```C++
struct VertexInputBindingDescription
{
    ...
    VertexInputBindingDescription& setBinding( uint32_t binding_ );
    VertexInputBindingDescription& setStride( uint32_t stride_ );
    VertexInputBindingDescription& setInputRate( vk::VertexInputRate inputRate_ ); 
    ...
};
```

- `binding_` is the index of the slot that the structure describes
- `stride_` is the distance (in bytes) between the first bytes of two consecutive vertices. This will often be identical to the size of one vertex, but may be greater e.g. if padding is used.
- `inputRate_` is related to a technique called 'instanced drawing' that we'll discuss in a later lesson. The parameter specifies whether the pipeline should move to the next entry in the buffer after each vertex or after each instance.

A `VertexInputAttributeDescription` describes the structure of a single vertex more closely. As mentioned earlier in this series, a vertex is a position in 3D space plus associated data such as e.g. color or texture information. Vulkan calls the different parts of a vertex its _attributes_ and you need to tell the pipeline how your vertex data is structured into those attributes:

```C++
struct VertexInputAttributeDescription
{
    ...
    VertexInputAttributeDescription& setBinding( uint32_t binding_ );
    VertexInputAttributeDescription& setLocation( uint32_t location_ );
    VertexInputAttributeDescription& setFormat( vk::Format format_ );
    VertexInputAttributeDescription& setOffset( uint32_t offset_ );
    ...
};
```

- `binding_` is the same as above. With this parameter you tell the pipeline which vertex buffer slot you are talking about
- `location_` is the index of the attribute within the vertex. E.g. if the first part of each of your vertices is the position and the second part is the color, the position would be at location 0 and the color would be at location 1.
- `format_` specifies a color format, so it might be a bit confusing to see it being required for all vertex attributes. However, since the pipeline can deduce the size of the attribute and the number of values it comprises from the format, using this across the board keeps the interface simple. You need to select a format that matches the number and size of the elements in your attribute. E.g. by convention a GLSL `vec4` is represented by `vk::Format::eR32G32B32A32Sfloat`, a format that consists of 4 float values.
- `offset_` finally is the starting point of this attribute within the vertex

Let's put that into practice now and update our vertex input state:

```C++
vk::UniquePipeline create_graphics_pipeline(
    const vk::Device& logicalDevice,
    const vk::ShaderModule& vertexShader,
    const vk::ShaderModule& fragmentShader,
    const vk::RenderPass& renderPass,
    const vk::Extent2D& viewportExtent
)
{
    ...
    const auto vertexBindingDescription = vk::VertexInputBindingDescription{}
        .setBinding( 0 )
        .setStride( 4 * sizeof( float ) )
        .setInputRate( vk::VertexInputRate::eVertex );

    const auto vertexAttributeDescription = vk::VertexInputAttributeDescription{}
        .setBinding( 0 )
        .setLocation( 0 )
        .setOffset( 0 )
        .setFormat( vk::Format::eR32G32B32A32Sfloat );

    const auto vertexInputState = vk::PipelineVertexInputStateCreateInfo{}
        .setVertexBindingDescriptions( vertexBindingDescription )
        .setVertexAttributeDescriptions( vertexAttributeDescription );
    ...
}
```

We only want to bind one buffer, so the binding index is 0. Our vertices consist of just a 4-dimensional position, so the location index is 0 and our stride is four times the size of a float. As mentioned above, the format usually used for `vec4` attributes is `vk::Format::eR32G32B32A32Sfloat`, so that's what we use here.

Now that the pipeline is prepared to send the vertex data to the shader, we need to adapt the shader code to make use of it. For this we need to declare a 'Shader stage input variable'(1) by using another variant of the `layout` directive (the same that we've actually already used in our fragment shader but which I didn't explain in detail so far):

```GLSL 
#version 450

layout( location = 0 ) in vec4 inPosition;

void main()
{
    gl_Position = inPosition;
}
```

The `location` is the same as above, i.e. it's the index of the attribute as defined in `vertexAttributeDescription`. `in` is a keyword that makes this an input variable (for the fragment shader output we used `out` instead ). You already know the `vec4` data type and the last part is simply the variable name. As you can see the vertex shader is now really simple: it just passes the vertex coordinate through to the builtin output variable. Let's move on.

Pipeline and vertex shader are now ready to receive vertex data. The problem is: they don't actually get any yet. To change that we need to bind a GPU buffer containing the vertex data to the pipeline. Which means that we first need to transfer our input data to a GPU buffer in the same way as we did with the compute data back in lesson 6:

```C++
const auto gpuVertexBuffer = create_gpu_buffer(
    physicalDevice,
    logicalDevice,
    sizeof( vertices ),
    vk::BufferUsageFlagBits::eVertexBuffer
);

vcpp::copy_data_to_buffer( *logicalDevice.device, vertices, gpuVertexBuffer );
```

Now that we have the data in a GPU buffer the next step is to bind it to the pipeline, just like we had to do with the descriptor sets for our compute pipeline. But again, since vertex input is so essential Vulkan offers a dedicated function to do that for vertex data:

```C++
class CommandBuffer
{
    ...
    void bindVertexBuffers( 
        uint32_t firstBinding, 
        const container_t< const vk::Buffer >& buffers, 
        const container_t< const vk::DeviceSize >& offsets, 
        ... ) const;
    ...
};
```

This function would actually allow us to bind multiple vertex buffers at once. `firstBinding` is the index of the first slot to fill with the buffers in this function call. If you pass more than one buffer, the others would fill the consecutive next slots. The `buffers` parameter should be self-explanatory, `offsets` allows you to specify the starting points in the respective buffers.

We only want to pass one vertex buffer completely, so our call is pretty straightforward:

```C++
void record_command_buffer(
    const vk::CommandBuffer& commandBuffer,
    const vk::Pipeline& pipeline,
    const vk::RenderPass& renderPass,
    const vk::Framebuffer& frameBuffer,
    const vk::Extent2D& renderExtent,
    const vk::Buffer& vertexBuffer,
    const std::uint32_t vertexCount
)
{
    ...
    commandBuffer.bindPipeline( vk::PipelineBindPoint::eGraphics, pipeline );

    commandBuffer.beginRenderPass( renderPassBeginInfo, vk::SubpassContents::eInline );
    
    commandBuffer.bindVertexBuffers( 0, vertexBuffer, { 0 } );

    commandBuffer.draw( vertexCount, 1, 0, 0 );
    ...
}
```

Since we need the vertex buffer in the recording function, we need to adapt the call to `record_command_buffer` accordingly:

```C++
vcpp::record_command_buffer(
    commandBuffers[ frame.inFlightIndex ],
    *pipeline,
    *renderPass,
    frame.framebuffer,
    swapchainExtent,
    *gpuVertexBuffer.buffer,
    vertexCount );
```

And that's it. Try running this version and make sure it's working try modifying the C++ vertex coordinates. You could even change the number of vertices and draw a different shape.

In the next lesson we're going to further increase the flexibility by enabling us to also pass color information to the shaders.


1. https://www.khronos.org/opengl/wiki/Layout_Qualifier_(GLSL)#Interface_layouts