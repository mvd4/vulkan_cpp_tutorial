# Lesson 30: Diffuse lighting I (flat shading)

In the last lesson we gave our scene a bit of ambient light. That's a good start, as we can now control the overall brightness and light color for the whole scene. However, it doesn't create any sense of depth, let alone realistic results: all six faces are lit equally, no matter which way they point. Rotate the cube and nothing about the shading changes - the faces are only distinguishable because we gave them different colors. Take those colors away and our three-dimensional object collapses into a flat silhouette. This is what I want to address today by implementing the first version of a concept called diffuse lighting.

Before we go there though, I want to upgrade our geometry a bit. The cube has served us well so far, but to really be able to observe the effects of our lighting models, a more complex shape is better suited. So let's replace the cube with a sphere.

## Building a sphere
The good thing about a sphere is that we can generate its geometry algorithmically. The usual way to do so is to treat it like a globe and cut it along lines of latitude and longitude[^1]. We walk from one pole to the other in a number of horizontal rings, which I'll call *stacks*, and around each of those rings in a number of steps, which I'll call *slices*. Between two neighboring rings and two neighboring slice steps lies a small quad, which we'll model as two triangles - exactly like the faces of our cube, just a great many more of them.

A point on that grid is described by two angles: `phi`, which runs from 0 at one pole to π at the other, and `theta`, which runs from 0 all the way around to 2π. So a point on the surface of the sphere can be calculated as:

```text
x = radius * sin( phi ) * cos( theta )
y = radius * cos( phi )
z = radius * sin( phi ) * sin( theta )
```

Which translates into code almost literally:

```cpp
// main.cpp
auto sphereSurfacePoint( float radius, float phi, float theta ) -> glm::vec3
{
    return radius * glm::vec3{
        std::sin( phi ) * std::cos( theta ),
        std::cos( phi ),
        std::sin( phi ) * std::sin( theta )
    };
}
```

With that in place, we can write the generator function itself. We walk the grid quad by quad. For each quad, we compute its four corners and pass the corners of its two triangles to a small helper function, `addTriangle`:

```cpp
// main.cpp
auto addTriangle(
    std::vector< Vertex >& vertices,
    const glm::vec3& a,
    const glm::vec3& b,
    const glm::vec3& c,
    const glm::vec4& color
) -> void
{
    vertices.push_back( Vertex{ glm::vec4{ a, 1.f }, color } );
    vertices.push_back( Vertex{ glm::vec4{ b, 1.f }, color } );
    vertices.push_back( Vertex{ glm::vec4{ c, 1.f }, color } );
}

auto createSphere(
    float radius,
    std::uint32_t numStacks,
    std::uint32_t numSlices,
    const glm::vec4& color
) -> std::vector< Vertex >
{
    assert( numStacks >= 2 && numSlices >= 3 );

    auto vertices = std::vector< Vertex >{};
    vertices.reserve( 6u * numStacks * numSlices );

    const auto angleOf = []( std::uint32_t step, std::uint32_t numSteps, float fullAngle ) -> float
    {
        return fullAngle * static_cast< float >( step ) / static_cast< float >( numSteps );
    };

    // sin( π ) and sin( 2π ) aren't exactly 0 in floating point, so we set the bottom pole and
    // the closing angle of the last slice explicitly to make neighboring triangles share
    // bit-identical vertices.
    const auto bottomPole = glm::vec3{ 0.f, -radius, 0.f };

    for ( auto stack = std::uint32_t{ 0 }; stack < numStacks; ++stack )
    {
        const auto isFirstStack = stack == 0;
        const auto isLastStack = stack + 1 == numStacks;

        const auto phiTop = angleOf( stack, numStacks, glm::pi< float >() );
        const auto phiBottom = angleOf( stack + 1, numStacks, glm::pi< float >() );

        for ( auto slice = std::uint32_t{ 0 }; slice < numSlices; ++slice )
        {
            const auto isLastSlice = slice + 1 == numSlices;

            const auto thetaLeft = angleOf( slice, numSlices, glm::two_pi< float >() );
            const auto thetaRight = isLastSlice ? 0.f : angleOf( slice + 1, numSlices, glm::two_pi< float >() );

            const auto topLeft = sphereSurfacePoint( radius, phiTop, thetaLeft );
            const auto topRight = sphereSurfacePoint( radius, phiTop, thetaRight );
            const auto bottomLeft = isLastStack ? bottomPole : sphereSurfacePoint( radius, phiBottom, thetaLeft );
            const auto bottomRight = isLastStack ? bottomPole : sphereSurfacePoint( radius, phiBottom, thetaRight );

            // At the poles the whole ring collapses into a single point, so one of the two
            // triangles of those quads would have no area at all.
            if ( !isFirstStack )
                addTriangle( vertices, topLeft, topRight, bottomLeft, color );

            if ( !isLastStack )
                addTriangle( vertices, topRight, bottomRight, bottomLeft, color );
        }
    }

    return vertices;
}
```

The poles are the one place where the neat grid breaks down, because the quads touching a pole degenerate into triangles: their top (or bottom) edge collapses into a single point. That's what the two `if`s are there for. Emitting those triangles anyway wouldn't be a catastrophe, since they have no area and the rasterizer would throw them away without drawing a single pixel. But it's still better to just not generate them.

You may also have noticed that we don't simply calculate every corner with `sphereSurfacePoint`: the bottom pole and the right edge of the last slice get special treatment. This is to avoid tiny cracks in our sphere[^2]. The `assert` at the top makes sure we get at least a minimal sphere: with fewer than two stacks, every triangle would be skipped, and with fewer than three slices, the result would have no volume.

We've already made our rendering pipeline able to handle any (or even a changing) number of vertices, so all that's left to do now is to replace the cube's vertex definitions in `main()`:

```cpp
// main.cpp
...
constexpr auto sphereRadius = .7f;
constexpr auto numSphereStacks = std::uint32_t{ 24 };
constexpr auto numSphereSlices = std::uint32_t{ 24 };
constexpr auto sphereColor = glm::vec4{ 0.5f, 0.5f, 0.5f, 1.f };

const auto vertices = createSphere( sphereRadius, numSphereStacks, numSphereSlices, sphereColor );
const auto vertexCount = static_cast< std::uint32_t >( vertices.size() );
...
```

Compile and run this, and you'll see that the cube has been replaced by a pulsating, color-changing disc. I chose a neutral gray so that the effect of the lighting will be easy to see. As described at the beginning, ambient light alone gives the eye no clue about the orientation of a surface, so the sphere can't be told apart from a flat disc. That's what I want to change now.

## Diffuse shading
In the real world, a surface's orientation relative to the light source(s) matters a lot. A face turned toward a light catches more of it; a face turned away catches less. These differences in brightness are what our eyes rely on to perceive shape and depth.

Many everyday surfaces - paper, chalk, unfinished wood - are matte: when light hits them, they scatter it back out roughly equally in all directions rather than reflecting it like a mirror. This is called diffuse reflection, and it has a convenient consequence: how bright such a surface appears depends only on how much light reaches it, not on where we happen to be looking from. And for a given light, the amount a surface receives depends on its orientation relative to that light.

Mathematically, we can simulate diffuse reflection by calculating the dot product of two normalized vectors: the surface normal **N**, and the light direction **L**, which points from the surface toward the light source. For normalized vectors, the dot product equals the cosine of the angle between them, and that's exactly how the received light falls off: the further a surface tilts away from the light, the larger the area the same amount of light is spread over. If N and L point in exactly the same direction, the dot product is 1 and the light contributes fully. If they are perpendicular, the result is 0, and this particular light source doesn't light the surface at all.

![Three visualizations outlining the effect of different orientations of a 2D plane and its normal relative to a light source](images/Diffuse_Shading.png "Fig. 1: Diffuse shading")

If the normal points more than 90° away from the light direction, the dot product becomes negative. A surface can't receive negative light, though, so we clamp the result to 0. That gives us the formula for the diffuse component:

```text
diffuse = max( dot( N, L ), 0 )
```

This is known as the *Lambert term*. In the classic Phong reflection model, it's the second of three components: ambient, diffuse, and specular.

On its own, the Lambert term is just a factor between 0 and 1 that tells us how strongly a surface is lit. To get the actual diffuse light, we multiply it by the light's color, and then multiply the result by the surface color - just like we did with the ambient term in the last lesson:

```glsl
vec3 diffuse = max( dot( N, L ), 0.0 ) * lightColor;
outColor = vec4( ( ambient + diffuse ) * objectColor.rgb, objectColor.a );
```

One decision we have to make now is _where_ in the pipeline we want to evaluate that formula. We have two candidates: the vertex shader, which runs once per vertex, or the fragment shader, which runs once per fragment (roughly, once per pixel covered by a triangle). Looking at the Lambert term, this only makes a difference if N or L vary across a triangle. In more advanced lighting systems they do, but we'll start with the simple case and assume that both are constant for each triangle.

For L, that means we'll use a *directional light*: a light source so far away - like the sun - that its light hits every point of the scene from the same direction. For N, it means that each triangle gets one single normal. This is known as *flat shading*, and it means that each triangle will be lit uniformly and end up with a single color. So for now, we can evaluate the formula in the vertex shader.

Let's implement this for our sphere.

## Vertex normals
First of all, we need the normals. Since our pipeline operates on vertices or fragments, not on surfaces, we add a normal vector to each vertex. That's the standard way of doing it, and it has advantages for the more sophisticated lighting models we'll get to later.

```cpp
// main.cpp
...
struct Vertex
{
    glm::vec4 position;
    glm::vec4 color;
    glm::vec4 normal;
};

constexpr auto vertexFormats = std::array< vk::Format, 3 >{
    vk::Format::eR32G32B32A32Sfloat,    // position
    vk::Format::eR32G32B32A32Sfloat,    // color
    vk::Format::eR32G32B32A32Sfloat,    // normal
};
...
```

Now we actually have to generate the normals for our sphere surfaces. Luckily this is quite straightforward. You might remember from your math lessons that the cross product of two vectors always produces a vector that is perpendicular to both. In other words, it points in the direction of the normal of the plane spanned by the two vectors. Two edges of a triangle - `b - a` and `c - a` - span exactly the plane the triangle lies in, so all we need to do is calculate their cross product when we add a triangle:

```cpp
// main.cpp
auto addTriangle(
    std::vector< Vertex >& vertices,
    const glm::vec3& a,
    const glm::vec3& b,
    const glm::vec3& c,
    const glm::vec4& color
) -> void
{
    const auto normal = glm::vec4{ glm::normalize( glm::cross( b - a, c - a ) ), 0.f };

    vertices.push_back( Vertex{ glm::vec4{ a, 1.f }, color, normal } );
    vertices.push_back( Vertex{ glm::vec4{ b, 1.f }, color, normal } );
    vertices.push_back( Vertex{ glm::vec4{ c, 1.f }, color, normal } );
}
```

Note that we set the normal's `w` component to 0: a normal is a direction, not a position, and with `w = 0` the translation part of a 4x4 transformation matrix has no effect on it.

The order of the three corners decides which way that normal ends up pointing. If you swap any two of them, it flips to face the other way (rotating all three, i.e. `a, b, c` → `b, c, a`, doesn't change it, though). We want the normal to point outwards, away from the center of the sphere, which means the corners have to be ordered counter-clockwise when seen from outside the sphere. Luckily, `createSphere` already passes them in that order, so there's nothing to change there.

Remember the degenerate triangles at the poles that we skipped earlier? Now it turns out that skipping them isn't just tidier, it's necessary: we'd end up with one edge being a zero vector, and thus the cross product would become zero as well. Normalizing a zero vector divides by zero, and we'd end up with NaN normals.

Now we can access the new normals in the vertex shader:

```glsl
#version 450

layout( location = 0 ) in vec4 inPosition;
layout( location = 1 ) in vec4 inColor;
layout( location = 2 ) in vec4 inNormal;

layout( location = 0 ) out vec4 outColor;

...
```

We could use the normal in the shader right away; everything else in the pipeline is already set up. There's a problem though: our sphere rotates, and with it so should the normals. Otherwise the lighting would rotate along with the sphere, as if it were painted on. We moved transformation of the vertices to the GPU in the last lesson, so now we need to transform the normals there too.

## The normal matrix
When doing these lighting calculations, there's one rule we absolutely have to obey: the normals and the light direction need to be in the _same_ coordinate space, otherwise the dot product doesn't make sense. Which space we pick is ultimately up to us: we could use object space[^3], world space, or view space. We'll go with world space here, as this is the most straightforward way to do it and also the one I find most natural to think in. That means we apply the model transformation to the normals, but neither the view nor the projection matrix.

Unfortunately, applying the model matrix to the normals isn't always correct: if an object is scaled non-uniformly, e.g. stretched along one axis, normals transformed with the same matrix are no longer perpendicular to the surface. We don't do any non-uniform scaling here, so we could get away with using the model matrix as-is. However, it doesn't cost us much to implement the proper solution, so let's pass a dedicated transformation matrix for the normals. This is the so-called *normal matrix*, which is calculated as the inverse transpose of the upper-left 3x3 part of the model matrix[^4]:

```text
normalMatrix = transpose( inverse( mat3( model ) ) );
```

Each normal needs to be multiplied by this matrix before calculating the Lambert term. Since there's one normal per vertex, we'll do that in the vertex shader. But first we create the normal matrix[^5]:

```cpp
// main.cpp
...
model = glm::rotate( glm::identity< glm::mat4 >(), rotationAngle, glm::vec3{ 0.f, 1.f, 0.f } );

const auto mvp = projection * view * model;
const auto normalMatrix = glm::mat4{ glm::transpose( glm::inverse( glm::mat3{ model } ) ) };
...
```

That leaves us in an uncomfortably tight spot though. The Vulkan specification only guarantees 128 bytes of push constant space[^6] - many GPUs offer more, but we cannot rely on it. Two `mat4` matrices already consume exactly those 128 bytes, leaving nothing for the ambient color vector. Even if we did pack the normal matrix as a padded `mat3`, we'd land at 64 + 48 + 16 = 128 bytes: it would technically fit, but with precisely zero bytes to spare, and the next thing we want to send to the GPU would break it again. So push constants are evidently not the proper long-term solution for this kind of information. We'll get to that soon. For now, let's remove the push constant for the ambient color again and give the two matrices the whole budget - we'll bring ambient light back once we have a better mechanism for it.

So, we remove the calculation of the ambient color from the render loop in `main()` (together with the `ambientColorPhase` and `twoThirdsPi` variables), modify `SceneData` to hold the normal matrix instead of the ambient color, change the pipeline layout to accept two matrices, and set the modified push constants in `recordCommandBuffer`:

```cpp
// rendering.hpp
struct SceneData
{
    std::span< const std::byte > vertexData;
    std::uint32_t vertexCount;
    std::uint32_t vertexDataVersion;
    glm::mat4 mvpMatrix;
    glm::mat4 normalMatrix;
};
```

```cpp
// pipelines.cpp
auto createGraphicsPipelineLayout( const vk::Device& logicalDevice ) -> vk::UniquePipelineLayout
{
    const auto ranges = std::array< vk::PushConstantRange, 1 >{
        vk::PushConstantRange{}
            .setStageFlags( vk::ShaderStageFlagBits::eVertex )
            .setOffset( 0 )
            .setSize( 2 * sizeof( glm::mat4 ) )   // mvp + normalMatrix
    };

    const auto info = vk::PipelineLayoutCreateInfo{}
        .setPushConstantRanges( ranges );

    return logicalDevice.createPipelineLayoutUnique( info );
}
```

```cpp
// rendering.cpp
auto Renderer::recordCommandBuffer(
    const vk::CommandBuffer& commandBuffer,
    const vk::Framebuffer& frameBuffer,
    const vk::Extent2D& renderExtent,
    const vk::Buffer& vertexBuffer,
    const SceneData& sceneData
) const -> void
{
    ...

    commandBuffer.bindPipeline( vk::PipelineBindPoint::eGraphics, *m_pipeline );

    commandBuffer.pushConstants< glm::mat4 >(
        *m_pipelineLayout,
        vk::ShaderStageFlagBits::eVertex,
        0,
        sceneData.mvpMatrix
    );

    commandBuffer.pushConstants< glm::mat4 >(
        *m_pipelineLayout,
        vk::ShaderStageFlagBits::eVertex,
        sizeof( glm::mat4 ),
        sceneData.normalMatrix
    );

    const auto viewport = vk::Viewport{}
    ...
}
```

Now we can pass our normal matrix as part of `SceneData` from main.

If you run this version, you'll likely notice two things: the validation layers report an error, and the disc still changes color. That's because while we've modified our application code to push the normal matrix through the pipeline to the shaders, we did not update the shaders themselves yet. The fragment shader still declares the ambient color push constant at offset 64, but the pipeline layout no longer assigns any push constant memory to the fragment stage - that's what the validation layers complain about. The fragment shader now reads data that isn't meant for it. What exactly you see depends on your driver: on many GPUs, push constants are stored in one shared block, so the shader reads the first column of our normal matrix and the disc changes color in sync with the rotation. On others, you might get a static or black disc. Officially, this is undefined behavior, which is why the validation layers complain.

## Implementing the Lambert term
So let's first adapt our vertex shader to work with the changed push constant layout:

```glsl
// vertex.vert
...
layout( push_constant ) uniform PushConstants
{
    mat4 mvp;
    mat4 normalMatrix;
} pushConstants;
...
```

Now, for the Lambert term, we need to calculate the dot product of the vertex normal and the normalized direction vector from the vertex to the light source. We have the vertex normal available in our vertex shader now, but we don't have the direction to the light source yet. In a real-world 3D engine, the light sources would be part of the scene description, and the light direction would be passed into the shader. For our tutorial purposes though, we'll take the shortcut that we already used when we initially drew our triangle and define the direction to the light source directly in the vertex shader:

```glsl
const vec3 lightDirection = normalize( vec3( 1.0, 0.0, 2.0 ) );
```
This simulates a distant light source sitting in front of the sphere and a bit to the right - about 27 degrees off the viewing axis. Feel free to play with these numbers later.

Next we need to transform the vertex normal using the normal matrix:

```glsl
vec3 worldNormal = normalize( mat3( pushConstants.normalMatrix ) * inNormal.xyz );
```
I called the variable `worldNormal`, because it's the normal transformed to world space. One important detail is that we only need the upper-left 3x3 part of the normal matrix: we only widened it to a `mat4` because of the memory layout, and the translation part is irrelevant for directions anyway. The rest should be pretty straightforward. We normalize the result because the normal matrix does not generally preserve vector lengths, and an unnormalized normal would make the Lambert term come out too bright or too dark.

With these two vectors in place, we can now calculate the diffuse light component for that vertex and pass it on to the fragment shader. So our new vertex shader implementation looks like this:

```glsl
#version 450

layout( location = 0 ) in vec4 inPosition;
layout( location = 1 ) in vec4 inColor;
layout( location = 2 ) in vec4 inNormal;

layout( location = 0 ) out vec4 outColor;
layout( location = 1 ) out float outDiffuse;

layout( push_constant ) uniform PushConstants
{
    mat4 mvp;
    mat4 normalMatrix;
} pushConstants;

// Direction from the surface towards a fixed directional light, in world space.
const vec3 lightDirection = normalize( vec3( 1.0, 0.0, 2.0 ) );

void main()
{
    gl_Position = pushConstants.mvp * inPosition;

    vec3 worldNormal = normalize( mat3( pushConstants.normalMatrix ) * inNormal.xyz );
    outDiffuse = max( dot( worldNormal, lightDirection ), 0.0 );

    outColor = inColor;
}
```

The rasterizer interpolates that `outDiffuse` value across each triangle for us. In our case, that interpolation doesn't actually change anything: all three vertices of a triangle share the same normal, so they all get the same diffuse value, and interpolating between three identical values yields that very value for every fragment. So all the fragment shader has left to do is scale the surface color by the light that reached it:

```glsl
#version 450

layout( location = 0 ) in vec4 inColor;
layout( location = 1 ) in float inDiffuse;

layout( location = 0 ) out vec4 outColor;

void main()
{
    vec3 lighting = vec3( inDiffuse );
    outColor = vec4( inColor.rgb * lighting, inColor.a );
}
```

The `lighting` variable looks like a pointless detour right now - it holds nothing but `inDiffuse` in all three components. I'm keeping it because it is the slot where the other terms of the lighting equation will be summed up in the coming lessons, and it's easier to follow if the structure is already there.

Compile and run this. You should see that our sphere finally looks like a three-dimensional object again. It's not exactly a sphere though, more like a disco ball. That's something we'll be addressing in the upcoming tutorials. Still, I think this is already much more impressive than our cube. So for now, lean back and enjoy what we've achieved.

That's it for today. Thanks for reading, I hope you enjoyed it.


---

[^1]: A latitude/longitude sphere like ours is the easiest one to derive, but it isn't the only option, and it does have the drawback that its triangles come out wildly uneven in size: tiny slivers crowding around the poles, comparatively huge ones at the equator. The common alternative is an icosphere, built by starting from an icosahedron and repeatedly subdividing each of its triangles into four. That gives you an almost uniform distribution of triangles over the whole surface and no poles to special-case, at the cost of less obvious code - which is why we're not building one here.
[^2]: The GPU's rasterization rules guarantee that two triangles sharing an edge leave no gap between them, but only if the shared vertices are bit-for-bit identical. Floating-point math doesn't give us that for free: `std::sin( glm::two_pi< float >() )` is about `1.7e-7`, not 0, so the vertices along the seam at `theta = 2π` would end up slightly off from the ones at `theta = 0`. The same happens at the bottom pole. The deviations are tiny, but they can occasionally show up as single-pixel holes along the seam. Using `0` as the closing angle of the last slice and setting the bottom pole explicitly removes the problem. The top pole doesn't need this, because `sin( 0 )` is exactly 0.
[^3]: In that case, we could keep the normals as they are and transform the light direction into each object's space instead. That's cheap, but less intuitive, and it has to be redone for every object.
[^4]: I'm not going into the mathematical explanation here. If you're interested in the details, please refer to [the Lighthouse3D tutorial on the normal matrix](https://www.lighthouse3d.com/tutorials/glsl-12-tutorial/the-normal-matrix/).
[^5]: For the normal matrix we only need the upper-left 3x3 part of the model matrix, which is then inverted and transposed. However, passing a `mat3` to the GLSL shaders is tricky, because they'd use a different memory layout: in a push constant block each of the three columns gets padded up to 16 bytes, so a `mat3` takes 48 bytes on the shader side while `glm::mat3` is only 36 bytes. Rather than juggling manual padding, we accept wasting a few bytes, convert it back to a `mat4`, and pass that.
[^6]: The limit is called `maxPushConstantsSize`. 128 bytes is the minimum a conformant implementation has to provide; you can query the actual value of your device via `physicalDevice.getProperties().limits.maxPushConstantsSize`.