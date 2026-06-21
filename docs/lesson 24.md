# Vertex Input - Part 2

So, we can send the positions of our vertices from our application now. But so far all of our geometry will always be red because the color information is still hardcoded in the fragment shader. I'd like to have a bit more flexibility here as well and this is what we'll address today.

It would be possible to send the color data directly to the fragment shader, e.g. by using descriptors. However, that wouldn't make much sense since it would imply that we already know the color of each fragment upfront. Remember: the fragment shader runs once for every fragment (think: every pixel, see lesson 14). So the data we send needs to be available and valid for every fragment. For our current triangle this is true because we only ever use the same red color. But it would break as soon as we were to move towards a bit more realistic use cases, so let's do it the proper way right from the start.

The usual way to pass color data to the fragment shader is via the vertex shader. The vertex shader outputs a color value for the respective vertex, the rasterization stage then interpolates the color for each fragment between the colors of the current triangle's vertices and passes that interpolated color to the fragment shader.

So the first step is to modify our fragment shader so that it uses color information that is coming in:

```glsl
#version 450

layout( location = 0 ) in vec4 inColor;

layout( location = 0 ) out vec4 outColor;

void main()
{
    outColor = inColor;
}
```

Next, let's modify the vertex shader to output a color for each vertex. To make it obvious whether it works let's change our color from red to green:

```glsl
#version 450

layout( location = 0 ) in vec4 inPosition;

layout( location = 0 ) out vec4 outColor;

void main()
{
    gl_Position = inPosition;
    outColor = vec4( 0.0, 1.0, 0.0, 1.0 );
}
```

With these changes to the shaders you should now see a green triangle instead of a red one. If we wanted to see the interpolation in action we could create an array of color values in the vertex shader and use the `gl_VertexIndex` variable just as we originally did with the positions. But actually we want to be able to pass the color information from the application, so let's not invest that effort and instead prepare the vertex shader to pass the colors through:

```glsl
#version 450

layout( location = 0 ) in vec4 inPosition;
layout( location = 1 ) in vec4 inColor;

layout( location = 0 ) out vec4 outColor;

void main()
{
    gl_Position = inPosition;
    outColor = inColor;
}
```

Now the shader expects each vertex to consist of two attributes: a `vec4` for the position and another `vec4` for the color. As described in the last lesson, `location` specifies the order of the attributes so that shader and pipeline know exactly which component comes first.

So let's add the color information to our vertices in the application:

```cpp
constexpr size_t vertexCount = 3;
    constexpr size_t floatsPerVertex = 8;
    const std::array< float, floatsPerVertex * vertexCount > vertices = {
    0.f, -.5f, 0.f, 1.f,    1.f, 0.f, 0.f, 1.f,
    .5f, .5f, 0.f, 1.f,     0.f, 1.f, 0.f, 1.f,
    -.5f, .5f, 0.f, 1.f,    1.f, 1.f, 0.f, 1.f
};
```

As you can see I use different vertex colors, so that we have an obvious visible indication that the colors are indeed interpolated.

This will not yet work however because the pipeline creation function still assumes only one 4 element vector per vertex as input to the vertex shader stage. So it would actually interpret the first color value as the second coordinate and so on. Let's change that:

```cpp
...
const auto vertexBindingDescription = vk::VertexInputBindingDescription{}
    .setBinding( 0 )
    .setStride( 8 * sizeof( float ) )
    .setInputRate( vk::VertexInputRate::eVertex );

const auto vertexAttributeDescriptions = std::array< vk::VertexInputAttributeDescription, 2 >{
    vk::VertexInputAttributeDescription{}
        .setBinding( 0 )
        .setLocation( 0 )
        .setOffset( 0 )
        .setFormat( vk::Format::eR32G32B32A32Sfloat ),
    vk::VertexInputAttributeDescription{}
        .setBinding( 0 )
        .setLocation( 1 )
        .setOffset( 4 * sizeof( float ) )
        .setFormat( vk::Format::eR32G32B32A32Sfloat )
};

const auto vertexInputState = vk::PipelineVertexInputStateCreateInfo{}
    .setVertexBindingDescriptions( vertexBindingDescription )
    .setVertexAttributeDescriptions( vertexAttributeDescriptions );
...
```

Since every vertex is now 8 floats, the `stride` needs to be changed accordingly. We also need to add the second attribute for the color and make sure that the attribute indices match those in the shader. The `offset` of the second attribute is the size of the first because our vertex data doesn't contain any padding.

Try out this version, you should see something like the following:

![Screenshot showing the rendering of our colored triangle on a dark blue background](images/Screenshot_2_Colored_Triangle.png "Fig. 1: The multi-colored triangle")

So we can now give each vertex an individual color and the interpolation works as we expect it. Nice.