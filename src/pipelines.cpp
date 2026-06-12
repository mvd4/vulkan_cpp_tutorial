/***************************************************************************************************

mvd Vulkan C++ Tutorial


Copyright 2026 mvd

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of
the MPL was not distributed with this file, You can obtain one at

http://mozilla.org/MPL/2.0/.

Unless required by applicable law or agreed to in writing, software distributed under the License
is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
implied. See the License for the specific language governing permissions and limitations under the
License.

***************************************************************************************************/

#include "pipelines.hpp"

#include <array>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace vcpp
{
    auto createShaderModule(
        const vk::Device& logicalDevice,
        const std::filesystem::path& path
    ) -> vk::UniqueShaderModule
    {
        std::ifstream is{ path, std::ios::binary };
        if ( !is.is_open() )
            throw std::runtime_error( "Could not open file" );

        auto buffer = std::vector< std::uint32_t >{};
        const auto bufferSizeInBytes = std::filesystem::file_size( path );
        if ( bufferSizeInBytes % sizeof( std::uint32_t ) != 0 )
            throw std::runtime_error( "Shader file size is not a multiple of 4 bytes" );
        buffer.resize( bufferSizeInBytes / sizeof( std::uint32_t ) );

        is.seekg( 0 );
        is.read( reinterpret_cast< char* >( buffer.data() ), bufferSizeInBytes );

        const auto createInfo = vk::ShaderModuleCreateInfo{}.setCode( buffer );
        return logicalDevice.createShaderModuleUnique( createInfo );
    }

    auto createDescriptorSetLayout( const vk::Device& logicalDevice ) -> vk::UniqueDescriptorSetLayout
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

    auto createPipelineLayout(
        const vk::Device& logicalDevice,
        const vk::DescriptorSetLayout& descriptorSetLayout
    ) -> vk::UniquePipelineLayout
    {
        const auto pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo{}
            .setSetLayouts( descriptorSetLayout );
        return logicalDevice.createPipelineLayoutUnique( pipelineLayoutCreateInfo );
    }

    auto createComputePipeline(
        const vk::Device& logicalDevice,
        const vk::PipelineLayout& pipelineLayout,
        const vk::ShaderModule& computeShader
    ) -> vk::UniquePipeline
    {
        const auto shaderStageInfo = vk::PipelineShaderStageCreateInfo{}
            .setStage( vk::ShaderStageFlagBits::eCompute )
            .setPName( "main" )
            .setModule( computeShader );

        const auto pipelineCreateInfo = vk::ComputePipelineCreateInfo{}
            .setStage( shaderStageInfo )
            .setLayout( pipelineLayout );

        return logicalDevice.createComputePipelineUnique( vk::PipelineCache{}, pipelineCreateInfo ).value;
    }

    auto createDescriptorPool( const vk::Device& logicalDevice ) -> vk::UniqueDescriptorPool
    {
        const auto poolSize = vk::DescriptorPoolSize{}
            .setType( vk::DescriptorType::eStorageBuffer )
            .setDescriptorCount( 2 );
        const auto poolCreateInfo = vk::DescriptorPoolCreateInfo{}
            .setMaxSets( 1 )
            .setPoolSizes( poolSize );
        return logicalDevice.createDescriptorPoolUnique( poolCreateInfo );
    }

    auto createGraphicsPipeline(
        const vk::Device& logicalDevice,
        const vk::ShaderModule& vertexShader,
        const vk::ShaderModule& fragmentShader,
        const vk::Extent2D& viewportExtent
    ) -> vk::UniquePipeline
    {
        const auto shaderStageInfos = std::array< vk::PipelineShaderStageCreateInfo, 2 >{
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

        const auto viewport = vk::Viewport{}
            .setX( 0.f )
            .setY( 0.f )
            .setWidth( static_cast< float >( viewportExtent.width ) )
            .setHeight( static_cast< float >( viewportExtent.height ) )
            .setMinDepth( 0.f )
            .setMaxDepth( 1.f );

        const auto scissor = vk::Rect2D{ { 0, 0 }, viewportExtent };

        const auto viewportState = vk::PipelineViewportStateCreateInfo{}
            .setViewports( viewport )
            .setScissors( scissor );

        const auto pipelineCreateInfo = vk::GraphicsPipelineCreateInfo{}
            .setStages( shaderStageInfos )
            .setPVertexInputState( &vertexInputState )
            .setPInputAssemblyState( &inputAssemblyState )
            .setPViewportState( &viewportState );

        return logicalDevice.createGraphicsPipelineUnique(
            vk::PipelineCache{},
            pipelineCreateInfo
        ).value;
    }
}