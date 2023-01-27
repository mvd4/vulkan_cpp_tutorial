/***************************************************************************************************

mvd Vulkan C++ Tutorial


Copyright 2022 mvd

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of
the MPL was not distributed with this file, You can obtain one at

http://mozilla.org/MPL/2.0/.

Unless required by applicable law or agreed to in writing, software distributed under the License
is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
implied. See the License for the specific language governing permissions and limitations under the
License.

***************************************************************************************************/

#include "pipelines.hpp"

#include <fstream>


namespace vcpp
{
    vk::UniqueShaderModule create_shader_module(
        const vk::Device& logicalDevice,
        const std::filesystem::path& path
    )
    {
        std::ifstream is{ path, std::ios::binary };
        if ( !is.is_open() )
            throw std::runtime_error( "Could not open file" );

        auto buffer = std::vector< std::uint32_t >{};
        const auto bufferSizeInBytes = std::filesystem::file_size( path );
        buffer.resize( std::filesystem::file_size( path ) / sizeof( std::uint32_t ) );

        is.seekg( 0 );
        is.read( reinterpret_cast< char* >( buffer.data() ), bufferSizeInBytes );

        const auto createInfo = vk::ShaderModuleCreateInfo{}.setCode( buffer );
        return logicalDevice.createShaderModuleUnique( createInfo );
    }

    vk::UniqueDescriptorSetLayout create_descriptor_set_layout( const vk::Device& logicalDevice )
    {
        const auto bindings = std::array< vk::DescriptorSetLayoutBinding, 2 >{
            vk::DescriptorSetLayoutBinding{}
                .setBinding( 0 )
                .setStageFlags( vk::ShaderStageFlagBits::eCompute )
                .setDescriptorType( vk::DescriptorType::eStorageBuffer )
                .setDescriptorCount( 1 ),
            vk::DescriptorSetLayoutBinding{}
                .setBinding( 1 )
                .setStageFlags( vk::ShaderStageFlagBits::eCompute )
                .setDescriptorType( vk::DescriptorType::eStorageBuffer )
                .setDescriptorCount( 1 ),
        };
        const auto descriptorSetLayoutCreateInfo = vk::DescriptorSetLayoutCreateInfo{}
            .setBindings( bindings );

        return logicalDevice.createDescriptorSetLayoutUnique( descriptorSetLayoutCreateInfo );
    }

    vk::UniquePipelineLayout create_pipeline_layout(
        const vk::Device& logicalDevice,
        const vk::DescriptorSetLayout& descriptorSetLayout
    )
    {
        const auto pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo{}
                .setSetLayouts( descriptorSetLayout );
        return logicalDevice.createPipelineLayoutUnique( pipelineLayoutCreateInfo );
    }

    vk::UniquePipeline create_compute_pipeline(
        const vk::Device& logicalDevice,
        const vk::PipelineLayout& pipelineLayout,
        const vk::ShaderModule& computeShader
    )
    {
        const auto pipelineCreateInfo = vk::ComputePipelineCreateInfo{}
            .setStage(
                vk::PipelineShaderStageCreateInfo{}
                    .setStage( vk::ShaderStageFlagBits::eCompute )
                    .setPName( "main" )
                    .setModule( computeShader )
            )
            .setLayout( pipelineLayout );

        return logicalDevice.createComputePipelineUnique( vk::PipelineCache{}, pipelineCreateInfo ).value;
    }

    vk::UniqueDescriptorPool create_descriptor_pool( const vk::Device& logicalDevice )
    {
        const auto poolSize = vk::DescriptorPoolSize{}
            .setType( vk::DescriptorType::eStorageBuffer )
            .setDescriptorCount( 2 );
        const auto poolCreateInfo = vk::DescriptorPoolCreateInfo{}
            .setMaxSets( 1 )
            .setPoolSizes( poolSize );
        return logicalDevice.createDescriptorPoolUnique( poolCreateInfo );
    }


    vk::UniquePipeline create_graphics_pipeline(
        const vk::Device& logicalDevice,
        const vk::ShaderModule& vertexShader,
        const vk::ShaderModule& fragmentShader
    )
    {
        const auto shaderStageInfos = std::vector< vk::PipelineShaderStageCreateInfo >{
            vk::PipelineShaderStageCreateInfo{}
                .setStage( vk::ShaderStageFlagBits::eVertex )
                .setPName( "main" )
                .setModule( vertexShader ),
            vk::PipelineShaderStageCreateInfo{}
                .setStage( vk::ShaderStageFlagBits::eFragment )
                .setPName( "main" )
                .setModule( fragmentShader ),
        };

        const auto vertexInputState = vk::PipelineVertexInputStateCreateInfo{};
        const auto inputAssemblyState = vk::PipelineInputAssemblyStateCreateInfo{}
            .setTopology( vk::PrimitiveTopology::eTriangleList );

        const auto pipelineCreateInfo = vk::GraphicsPipelineCreateInfo{}
            .setStages( shaderStageInfos )
            .setPVertexInputState( &vertexInputState )
            .setPInputAssemblyState( &inputAssemblyState );

        return logicalDevice.createGraphicsPipelineUnique( vk::PipelineCache{}, pipelineCreateInfo ).value;
    }
}
