# Lesson 25: Going 3D
So far all that we've created is one flat triangle, which is still pretty far from a proper 3D scene. I think it's time to change this.

Our vertices already have three dimensional coordinates, only that we're not using the third dimension yet. So we should be able to create a three-dimensional object without any changes to the pipeline. Let's modify our vertex buffer to contain a cube:

``` C++
constexpr size_t vertexCount = 36;
const std::array< float, 8 * vertexCount > vertices = {
    // front               (red)
    -.5f, -.5f, .5f, 1.f,  1.f, 0.f, 0.f, 1.f,
    .5f, -.5f, .5f, 1.f,   1.f, 0.f, 0.f, 1.f,
    -.5f, .5f, .5f, 1.f,   1.f, 0.f, 0.f, 1.f,
    .5f, -.5f, .5f, 1.f,   1.f, 0.f, 0.f, 1.f,
    .5f, .5f, .5f, 1.f,    1.f, 0.f, 0.f, 1.f,
    -.5f, .5f, .5f, 1.f,   1.f, 0.f, 0.f, 1.f,

    // back                (yellow)
    -.5f, -.5f, -.5f, 1.f, 1.f, 1.f, 0.f, 1.f,
    .5f, -.5f, -.5f, 1.f,  1.f, 1.f, 0.f, 1.f,
    -.5f, .5f, -.5f, 1.f,  1.f, 1.f, 0.f, 1.f,
    .5f, -.5f, -.5f, 1.f,  1.f, 1.f, 0.f, 1.f,
    .5f, .5f, -.5f, 1.f,   1.f, 1.f, 0.f, 1.f,
    -.5f, .5f, -.5f, 1.f,  1.f, 1.f, 0.f, 1.f,

    // left                (violet)
    -.5f, -.5f, .5f, 1.f,  1.f, 0.f, 1.f, 1.f,
    -.5f, -.5f, -.5f, 1.f, 1.f, 0.f, 1.f, 1.f,
    -.5f, .5f, .5f, 1.f,   1.f, 0.f, 1.f, 1.f,
    -.5f, -.5f, .5f, 1.f,  1.f, 0.f, 1.f, 1.f,
    -.5f, .5f, -.5f, 1.f,  1.f, 0.f, 1.f, 1.f,
    -.5f, .5f, .5f, 1.f,   1.f, 0.f, 1.f, 1.f,

    // right               (green)
    .5f, -.5f, .5f, 1.f,   0.f, 1.f, 0.f, 1.f,
    .5f, -.5f, -.5f, 1.f,  0.f, 1.f, 0.f, 1.f,
    .5f, .5f, -.5f, 1.f,   0.f, 1.f, 0.f, 1.f,
    .5f, -.5f, .5f, 1.f,   0.f, 1.f, 0.f, 1.f,
    .5f, .5f, -.5f, 1.f,   0.f, 1.f, 0.f, 1.f,
    .5f, .5f, .5f, 1.f,    0.f, 1.f, 0.f, 1.f,

    // top                 (turquoise)
    -.5f, -.5f, .5f, 1.f,  0.f, 1.f, 1.f, 1.f,
    .5f, -.5f, .5f, 1.f,   0.f, 1.f, 1.f, 1.f,
    .5f, -.5f, -.5f, 1.f,  0.f, 1.f, 1.f, 1.f,
    -.5f, -.5f, .5f, 1.f,  0.f, 1.f, 1.f, 1.f,
    .5f, -.5f, -.5f, 1.f,  0.f, 1.f, 1.f, 1.f,
    -.5f, -.5f, -.5f, 1.f, 0.f, 1.f, 1.f, 1.f,

    // bottom              (blue)
    -.5f, .5f, .5f, 1.f,   0.f, 0.f, 1.f, 1.f,
    .5f, .5f, .5f, 1.f,    0.f, 0.f, 1.f, 1.f,
    .5f, .5f, -.5f, 1.f,   0.f, 0.f, 1.f, 1.f,
    -.5f, .5f, .5f, 1.f,   0.f, 0.f, 1.f, 1.f,
    .5f, .5f, -.5f, 1.f,   0.f, 0.f, 1.f, 1.f,
    -.5f, .5f, -.5f, 1.f,  0.f, 0.f, 1.f, 1.f,
};
```

The cube is centered around the origin. Every face is made out of two triangles, therefore we need six vertices per face[^1]. I gave every face a different color to make it easier to see what's going on on the screen.

Running this version we still see just a plain red rectangle instead of a three dimensional object. That is okay because we're looking at our cube straight on and thus cannot expect to see anything but the front face. But something's not quite right here: a cube is supposed to have square faces, this thing on screen however is clearly wider than it is high. Worse even, if we resize the window it changes its dimensions and aspect ratio.

![Screenshot showing a red rectangle on a blue background](images/Screenshot_3_Cube_1.png "Fig. 1: This doesn't look like a cube yet")

To understand why that is we need to have a look at the coordinate system that the Vulkan rendering pipeline operates with. We already briefly touched upon the subject when we were discussing our graphics pipeline setup in lessons 14 - 16. The pipeline expects the output of the vertex shader to be so-called normalized device coordinates. By default this is a right-handed coordinate system with the y-axis pointing downwards[^2] and the origin in the middle of the window. That means the top of the window is mapped to a coordinate of y=-1 and the bottom to y=+1, the left window border is at x=-1 and the right border at x=+1. That explains why our cube face initially has a rectangular shape and changes dimensions when we resize the window: an x value of 0.5 is always halfway between the middle of the window and its right border, the same applies to the y axis.

![Visualization showing the 3 main axes of the Vulkan coordinate system with x pointing to the right, y pointing downwards and z pointing into the screen. The origin is in the middle of the window. The left window edge is at x = -1, the right one at x = +1. The top window edge is at y = -1 and the bottom one at y = +1](images/Vulkan_Coordinate_System.png "Fig. 2: The Vulkan coordinate system for normalized device coordinates")

Okay, that is good to know, but what do we do about it? We want our cube to look like a cube, so somehow need to modify our vertex coordinates so that they take the window dimensions into account.

This is probably a good point to talk about transformations. In the context of graphics a transformation is an operation that modifies a coordinate (i.e. a vector) in a specific way so that it ends up in a different location. If you apply the same transformation to all vertex coordinates of an object you can e.g. realize translations (i.e. move the object around), scaling (make the object bigger or smaller) and rotations. Mathematically transformations are expressed as a multiplication of a matrix with the coordinate vector. I'm not going to go into details here, if you are interested in a more extensive explanation I suggest you watch the video that is linked in the footnotes - it's by far the best tutorial on linear transformations that I know.

What we need right now is called a projection transformation, because it projects the 3D coordinates of our vertices onto the 2D plane of our screen. There are two common types of projection: perspective and orthographic. Perspective projection is what you are used to from most 3D games, it simulates the real world experience where objects appear to become smaller with increasing distance from the viewer. The main usecase for orthographic projections is in technical software such as 3D modelling and CAD programs. Distant objects retain the same size as close ones, which creates a somewhat weird visual appearance but makes it easier to judge proportions etc. Mathematically the difference between the two is: in a perspective projection the imaginary rays of light that project the 3D scene onto the screen intersect at your eye (the camera) and get farther apart with increasing depth (into the screen), whereas in an orthographic projection they run in parallel. This is the reason why the latter is also sometimes called a parallel projection.

![Visualization of the principles behind perspective and orthographic projection. The imaginary rays in perspective projection converge and intersect at the eye position while in orthographic projection they run in parallel](images/Projections.png "Fig. 3: Perspective and Orthographic (Parallel) Projection")

Alright, so we need to multiply our vertex coordinates with a matrix that applies the projection transformation. Does that mean we now have to implement matrix multiplication? And how do we know how the transformation matrix needs to look like?

Obviously this is a challenge that many others faced before us and so there are libraries that provide what we need. The one we will use is called GLM (short for OpenGL Mathematics), so let's start by adding it to our project:

Add glm to `vcpkg.json`:
```json
{
  "name": "vulkan-cpp-tutorial",
  "version-string": "lesson_25",
  "dependencies": [
    "glfw3",
    "glm",
    "vulkan"
  ]
}
```

We also need to make CMake aware of the new dependency, so let's add another `find_package` call to the `CMakeLists.txt` file:
```cmake
find_package( glm CONFIG REQUIRED )
```

... and link the resulting target in `src/CMakeLists.txt`:
```cmake
target_link_libraries( ${TARGET_NAME} PRIVATE glfw glm::glm Vulkan::Vulkan )
```

Note the `glm::glm` namespaced target name. Linking against a bare `glm` would make CMake look for a library file called `glm.lib` (or `libglm.a`), which does not exist - GLM is a header-only library and only exposes its include paths through the imported target.

Finally, add the corresponding `#includes` to `main.cpp`:

```C++
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
...
```

GLM - like glfw - was originally written for OpenGL. Therefore we need to add some `#defines` before the `#includes` to configure it for use with Vulkan.

GLM provides types for vectors and matrices that are compatible with Vulkan. So the next step is to modify our vertex array to make use of GLM's `vec4` type. While we're at it, let's also wrap the position and color of a vertex in a small struct so the layout of the array becomes clearer:

``` C++
struct Vertex
{
    glm::vec4 position;
    glm::vec4 color;
};

...

constexpr size_t vertexCount = 36;
const std::array< Vertex, vertexCount > vertices = {
    // front (red)
    Vertex{ glm::vec4{ -.5f, -.5f,  .5f, 1.f }, glm::vec4{ 1.f, 0.f, 0.f, 1.f } },
    Vertex{ glm::vec4{  .5f, -.5f,  .5f, 1.f }, glm::vec4{ 1.f, 0.f, 0.f, 1.f } },
    Vertex{ glm::vec4{ -.5f,  .5f,  .5f, 1.f }, glm::vec4{ 1.f, 0.f, 0.f, 1.f } },
    Vertex{ glm::vec4{  .5f, -.5f,  .5f, 1.f }, glm::vec4{ 1.f, 0.f, 0.f, 1.f } },
    Vertex{ glm::vec4{  .5f,  .5f,  .5f, 1.f }, glm::vec4{ 1.f, 0.f, 0.f, 1.f } },
    Vertex{ glm::vec4{ -.5f,  .5f,  .5f, 1.f }, glm::vec4{ 1.f, 0.f, 0.f, 1.f } },

    // back (yellow)
    Vertex{ glm::vec4{ -.5f, -.5f, -.5f, 1.f }, glm::vec4{ 1.f, 1.f, 0.f, 1.f } },
    Vertex{ glm::vec4{  .5f, -.5f, -.5f, 1.f }, glm::vec4{ 1.f, 1.f, 0.f, 1.f } },
    Vertex{ glm::vec4{ -.5f,  .5f, -.5f, 1.f }, glm::vec4{ 1.f, 1.f, 0.f, 1.f } },
    Vertex{ glm::vec4{  .5f, -.5f, -.5f, 1.f }, glm::vec4{ 1.f, 1.f, 0.f, 1.f } },
    Vertex{ glm::vec4{  .5f,  .5f, -.5f, 1.f }, glm::vec4{ 1.f, 1.f, 0.f, 1.f } },
    Vertex{ glm::vec4{ -.5f,  .5f, -.5f, 1.f }, glm::vec4{ 1.f, 1.f, 0.f, 1.f } },

    // left (violet)
    Vertex{ glm::vec4{ -.5f, -.5f,  .5f, 1.f }, glm::vec4{ 1.f, 0.f, 1.f, 1.f } },
    Vertex{ glm::vec4{ -.5f, -.5f, -.5f, 1.f }, glm::vec4{ 1.f, 0.f, 1.f, 1.f } },
    Vertex{ glm::vec4{ -.5f,  .5f, -.5f, 1.f }, glm::vec4{ 1.f, 0.f, 1.f, 1.f } },
    Vertex{ glm::vec4{ -.5f, -.5f,  .5f, 1.f }, glm::vec4{ 1.f, 0.f, 1.f, 1.f } },
    Vertex{ glm::vec4{ -.5f,  .5f, -.5f, 1.f }, glm::vec4{ 1.f, 0.f, 1.f, 1.f } },
    Vertex{ glm::vec4{ -.5f,  .5f,  .5f, 1.f }, glm::vec4{ 1.f, 0.f, 1.f, 1.f } },

    // right (green)
    Vertex{ glm::vec4{  .5f, -.5f,  .5f, 1.f }, glm::vec4{ 0.f, 1.f, 0.f, 1.f } },
    Vertex{ glm::vec4{  .5f, -.5f, -.5f, 1.f }, glm::vec4{ 0.f, 1.f, 0.f, 1.f } },
    Vertex{ glm::vec4{  .5f,  .5f, -.5f, 1.f }, glm::vec4{ 0.f, 1.f, 0.f, 1.f } },
    Vertex{ glm::vec4{  .5f, -.5f,  .5f, 1.f }, glm::vec4{ 0.f, 1.f, 0.f, 1.f } },
    Vertex{ glm::vec4{  .5f,  .5f, -.5f, 1.f }, glm::vec4{ 0.f, 1.f, 0.f, 1.f } },
    Vertex{ glm::vec4{  .5f,  .5f,  .5f, 1.f }, glm::vec4{ 0.f, 1.f, 0.f, 1.f } },

    // top (turquoise)
    Vertex{ glm::vec4{ -.5f, -.5f,  .5f, 1.f }, glm::vec4{ 0.f, 1.f, 1.f, 1.f } },
    Vertex{ glm::vec4{  .5f, -.5f,  .5f, 1.f }, glm::vec4{ 0.f, 1.f, 1.f, 1.f } },
    Vertex{ glm::vec4{  .5f, -.5f, -.5f, 1.f }, glm::vec4{ 0.f, 1.f, 1.f, 1.f } },
    Vertex{ glm::vec4{ -.5f, -.5f,  .5f, 1.f }, glm::vec4{ 0.f, 1.f, 1.f, 1.f } },
    Vertex{ glm::vec4{  .5f, -.5f, -.5f, 1.f }, glm::vec4{ 0.f, 1.f, 1.f, 1.f } },
    Vertex{ glm::vec4{ -.5f, -.5f, -.5f, 1.f }, glm::vec4{ 0.f, 1.f, 1.f, 1.f } },

    // bottom (blue)
    Vertex{ glm::vec4{ -.5f,  .5f,  .5f, 1.f }, glm::vec4{ 0.f, 0.f, 1.f, 1.f } },
    Vertex{ glm::vec4{  .5f,  .5f,  .5f, 1.f }, glm::vec4{ 0.f, 0.f, 1.f, 1.f } },
    Vertex{ glm::vec4{  .5f,  .5f, -.5f, 1.f }, glm::vec4{ 0.f, 0.f, 1.f, 1.f } },
    Vertex{ glm::vec4{ -.5f,  .5f,  .5f, 1.f }, glm::vec4{ 0.f, 0.f, 1.f, 1.f } },
    Vertex{ glm::vec4{  .5f,  .5f, -.5f, 1.f }, glm::vec4{ 0.f, 0.f, 1.f, 1.f } },
    Vertex{ glm::vec4{ -.5f,  .5f, -.5f, 1.f }, glm::vec4{ 0.f, 0.f, 1.f, 1.f } },
};
```

We have to explicitly spell out the `Vertex` and `glm::vec4` constructors because both are marked as `explicit`. Quite a lot of typing, I know. The good news is that this layout is binary-compatible with our existing pipeline setup: a `Vertex` is just two `vec4`s back to back, which matches the two `R32G32B32A32_SFLOAT` attributes we configured earlier. So compiling and running this program should work fine.


---

[^1]: If this sounds a bit wasteful to you, you are right. We'll take care of the duplication in a later lesson.
[^2]: This is a bit uncommon: Direct 3D and Metal use a left-handed system with y pointing upwards, and OpenGL uses a right-handed system, also with y pointing upwards. The downwards pointing y Axis is more intuitive for people that are used to work with rasterized images on the computer (e.g. if you open a graphics application like GIMP or Photoshop). On the other hand the cartesian coordinate system most of us know from our geometry lessons in school has y going upwards. The same applies to how 3D models are usually created. So we'll need to deal with that at some point.
