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

With these changes to the shaders you should now see a green triangle instead of a red one.