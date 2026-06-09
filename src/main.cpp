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

#include "devices.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>


struct GPUBuffer
{
    vk::UniqueBuffer buffer;
    vk::UniqueDeviceMemory memory;
};

auto findSuitableMemoryIndex(
    const vk::PhysicalDeviceMemoryProperties& memoryProperties,
    std::uint32_t allowedTypesMask,
    vk::MemoryPropertyFlags requiredMemoryFlags
) -> std::uint32_t
{
    for(
        std::uint32_t memoryType = 1, i = 0;
        i < memoryProperties.memoryTypeCount;
        ++i, memoryType <<= 1
    )
    {
        if(
            ( allowedTypesMask & memoryType ) > 0 &&
            ( ( memoryProperties.memoryTypes[ i ].propertyFlags & requiredMemoryFlags ) == requiredMemoryFlags )
        )
        {
            return i;
        }
    }

    throw std::runtime_error( "could not find suitable gpu memory" );
}

auto createGPUBuffer(
    const vk::PhysicalDevice& physicalDevice,
    const vk::Device& logicalDevice,
    std::uint64_t size,
    vk::BufferUsageFlags usageFlags = vk::BufferUsageFlagBits::eStorageBuffer,
    vk::MemoryPropertyFlags requiredMemoryFlags =
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
) -> GPUBuffer
{
    const auto bufferCreateInfo = vk::BufferCreateInfo{}
        .setSize( size )
        .setUsage( usageFlags )
        .setSharingMode( vk::SharingMode::eExclusive );
    auto buffer = logicalDevice.createBufferUnique( bufferCreateInfo );

    const auto memoryRequirements = logicalDevice.getBufferMemoryRequirements( *buffer );
    const auto memoryProperties = physicalDevice.getMemoryProperties();

    const auto memoryIndex = findSuitableMemoryIndex(
        memoryProperties,
        memoryRequirements.memoryTypeBits,
        requiredMemoryFlags );

    const auto allocateInfo = vk::MemoryAllocateInfo{}
        .setAllocationSize( memoryRequirements.size )
        .setMemoryTypeIndex( memoryIndex );

    auto memory = logicalDevice.allocateMemoryUnique( allocateInfo );

    logicalDevice.bindBufferMemory( *buffer, *memory, 0u );

    return { std::move( buffer ), std::move( memory ) };
}

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

template< typename T, size_t N >
auto copyDataToBuffer(
    const vk::Device& logicalDevice,
    const std::array< T, N >& data,
    const GPUBuffer& buffer
) -> void
{
    const auto numBytesToCopy = sizeof( data );
    const auto mappedMemory = logicalDevice.mapMemory( *buffer.memory, 0, numBytesToCopy );
    std::memcpy( mappedMemory, data.data(), numBytesToCopy );
    logicalDevice.unmapMemory( *buffer.memory );
}

template< typename T, size_t N >
auto copyDataFromBuffer(
    const vk::Device& logicalDevice,
    const GPUBuffer& buffer,
    std::array< T, N >& data
) -> void
{
    const auto numBytesToCopy = sizeof( data );
    const auto mappedMemory = logicalDevice.mapMemory( *buffer.memory, 0, numBytesToCopy );
    std::memcpy( data.data(), mappedMemory, numBytesToCopy );
    logicalDevice.unmapMemory( *buffer.memory );
}

auto main() -> int
{
    try
    {
        const auto instance = vcpp::createVulkanInstance();
        const auto physicalDevice = vcpp::selectPhysicalDevice( *instance );
        const auto logicalDevice = vcpp::createLogicalDevice( physicalDevice );
    }
    catch( const std::exception& e )
    {
        std::cerr << "Exception thrown: " << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
