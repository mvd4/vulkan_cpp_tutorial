# Lesson 7: Shaders
Shaders are small programs that are executed on the GPU hardware, usually a lot of them in parallel. Vulkan and OpenGL shaders are commonly written in a language called GLSL (GL Shading Language)[^1], which is very similar to C. The shaders are compiled and uploaded to the graphics hardware every time the Vulkan application runs. There are multiple types of shaders that allow you to customize the graphics pipeline at different points, e.g. vertex-, fragment- or geometry shaders. For our compute pipeline however there is only one relevant type, which is the compute shader.

## GLSL basics
The most basic shader we can write in GLSL looks something like this:
```glsl
#version 450

void main()
{

}
```
To C and C++ programmers this looks pretty familiar, right? The first line, starting with a hash, is indeed called a preprocessor directive just like in C and C++. Every shader should start with this version identifier that denotes the GLSL language version the shader is written in. This allows the compiler to process the code in the best possible way and do detailed checks according to the denoted version.

Most of the other preprocessor directives will be intuitive for C and C++ programmers: `#define`, `#undef`, `#ifdef`, `#pragma`, `#error` etc. There are also a few that are specific to GLSL, like `#version`, but ultimately there's nothing new here.

The main function is slightly different compared to its C/C++ counterparts in that it doesn't have a return value. Shaders do not return values. But otherwise functions are declared and used in the same way as in C/C++.

## Compiling the shader
Another difference between Vulkan and OpenGL is that Vulkan requires us to precompile the shaders. In OpenGL the GLSL code is loaded by the application and then compiled and linked into a program explicitly. Since all of that happens at runtime, the respective driver on the user's device needs to do all the required heavy lifting. This makes OpenGL drivers even more complex than they need to be anyway and may lead to subtle differences in behaviour or even bugs because of the different compiler implementations.

In Vulkan, the GLSL code has to be precompiled into an intermediate binary representation called SPIR-V before loading it into the application and handing it over to the runtime. So instead of raw GLSL the shaders are shipped with the application as SPIR-V bytecode. Compiling SPIR-V to the native binary code for the GPU is much less complex, the drivers can thus be simpler and the potential for bugs and divergences is reduced quite significantly.

The GLSL to SPIR-V compiler that comes with the Vulkan SDK is located in its `bin` folder and is called `glslc`. Its basic usage is very straightforward:
```shell
> glslc  <glsl_shader_filename>  -o <compiled_shader_filename>
```
So let's compile our minimal shader from above. Create a folder `shaders` in the `src` directory. In that folder create a text file named `compute.comp`[^2] and paste the above code into the file. Then create a folder `shaders` inside your build configuration output directory (e.g. `build/bin/Debug/`) and finally run this command[^3]:
```shell
$ glslc src/shaders/compute.comp -o build/bin/Debug/shaders/compute.comp.spv
```
That command should terminate without any output (indicating success) and you now should see the file `compute.comp.spv`[^4] in your build output's `shaders` folder.

---

[^1]: The GLSL Vulkan profile differs slightly from the one for OpenGL, mostly in that it removes deprecations. Some OpenGL shaders therefore might not compile directly for Vulkan without modifications. However, those modifications should usually be pretty minor.
[^2]: There is no official standard that specifies the extension of .glsl shaders. However, the extensions `.vert`, `.frag` and `.comp` are very common for vertex, fragment and compute shaders respectively. They are also recognized by most tools that work with GLSL (e.g. VS code extensions).
[^3]: Installing the Vulkan SDK should have put its `bin` directory in your path so that the executable is found automatically. If that is not the case you should add that directory to your path by hand and try again.
[^4]: The output file name `compute.comp.spv` may seem unnecessarily repetitive. This is true in our case, where we only have one compute shader. In larger projects however, you might have multiple vertex-, fragment- and compute-shaders. In that case, adding the shader type to the output filename helps avoid collisions and avoid confusion.
