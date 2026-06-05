# Lesson 8: The Compute Pipeline

So, we have the data that we want to process uploaded to GPU memory. We have the shader module that is supposed to process the data precompiled and ready to use (at least technically). We have the logical device configured with a compute queue that would be able to run the processing commands. The problem is: none of these building blocks know anything about one another yet. The queue has no idea that it is supposed to run our shader module, and the shader has no idea where to find the data to process. We somehow need to bring those building blocks together. For that purpose we need to create a pipeline.

## Pipelines
A pipeline object represents the configuration of the whole processing chain on the GPU. This includes the layout of the different stages and how the data flows between them, as well as the concrete shaders that are to be used in the various stages and the layout of the data itself.
Graphics pipelines can get very very complex, with multiple shader stages and additional graphics functionality like blending modes, backface culling, primitive topology etc. plus all the associated data. Fortunately for us compute pipelines are much simpler as they essentially only have one stage: the compute shader stage.

Pipelines are created via the logical device. There are dedicated functions for each type of pipeline:
```cpp
class Device
{
    ...
    // return values are actually ResultValue< UniquePipeline >, see chapter 2
    UniquePipeline createGraphicsPipelineUnique( PipelineCache pipelineCache, const GraphicsPipelineCreateInfo& createInfo, ... );
    UniquePipeline createComputePipelineUnique( PipelineCache pipelineCache, const ComputePipelineCreateInfo& createInfo, ... );
    UniquePipeline createRayTracingPipelineKHRUnique( ... );
    ...
};
```
So we need two parameters for our compute pipeline. The `PipelineCache` is a helper object that can be used to speed up pipeline (re-)creation. It is recommended to use one, but you can also pass in a temporary object. That's what we will do in this tutorial.
The `ComputePipelineCreateInfo` looks like this:
```cpp
struct ComputePipelineCreateInfo
{
    ...
    ComputePipelineCreateInfo& setFlags( vk::PipelineCreateFlags flags_ );
    ComputePipelineCreateInfo& setStage( const vk::PipelineShaderStageCreateInfo& stage_ );
    ComputePipelineCreateInfo& setLayout( vk::PipelineLayout layout_ );
    ComputePipelineCreateInfo& setBasePipelineHandle( vk::Pipeline basePipelineHandle_ );
    ComputePipelineCreateInfo& setBasePipelineIndex( int32_t basePipelineIndex_ );
    ...
};
```
There is quite a number of flag bits we could set as the `PipelineCreateFlags`, but none of them are really relevant for us at this point. The functions related to the `BasePipeline` come into play when you want to derive one pipeline from another one (a bit like class inheritance in C++). We're not going to need that here either. Which leaves two functions that we need to look at: `setStage` and `setLayout`.

## The Compute Shader Stage
As said, compute pipelines are pretty straightforward in that they only have the compute stage. How that stage is to be configured concretely is determined by a `PipelineShaderStageCreateInfo` structure which looks like this:
```cpp
struct PipelineShaderStageCreateInfo
{
    ...
    PipelineShaderStageCreateInfo& setFlags( vk::PipelineShaderStageCreateFlags flags_ );
    PipelineShaderStageCreateInfo& setStage( vk::ShaderStageFlagBits stage_ );
    PipelineShaderStageCreateInfo& setModule( vk::ShaderModule module_ );
    PipelineShaderStageCreateInfo& setPName( const char* pName_ );
    PipelineShaderStageCreateInfo& setPSpecializationInfo( const vk::SpecializationInfo* pSpecializationInfo_ ) ;
    ...
};
```
That's quite a few potentially relevant fields, let's look at them one by one:
- once more, although there are some `PipelineShaderStageCreateFlagBits` specified, we can ignore the `flags_` parameter for our usecase.
- the `stage_` parameter determines the stage in the pipeline that this create info configures. For us that is obviously `ShaderStageFlagBits::eCompute`
- `module_` is the shader module we created in the last lesson.
- `setPName` is used to specify the entry point into the shader, i.e. the name of the top-level function to call for this shader stage. That makes sense because SPIR-V allows for multiple entry points in one shader. However, multiple entry points are to my knowledge not yet supported by GLSL, so creating such a shader module would be more involved than simply compiling GLSL code. We'll therefore stick to `main` as our shader entry point.
- `SpecializationInfo` can be used to configure so-called specialization constants. That's a mechanism that allows for configuring a shader at pipeline creation time, e.g. for configuring the local workgroup size according to the device's capabilities. We won't use this feature, so we'll ignore also this function.

That means we can create our `PipelineShaderStageCreateInfo` like so:
```cpp
const auto shaderStageInfo = vk::PipelineShaderStageCreateInfo{}
    .setStage( vk::ShaderStageFlagBits::eCompute )
    .setPName( "main" )
    .setModule( *computeShader );
```
That wasn't too hard, was it? With that our pipeline would know already which shader to use.
