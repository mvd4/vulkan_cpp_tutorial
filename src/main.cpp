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

#include <vulkan/vulkan.hpp>

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


struct VersionNumber
{
    std::uint32_t majorVersion;
    std::uint32_t minorVersion;
    std::uint32_t patchVersion;
};

auto operator<<( std::ostream& os, const VersionNumber& v ) -> std::ostream&
{
    os << v.majorVersion << '.' << v.minorVersion << '.' << v.patchVersion;
    return os;
}

constexpr auto operator>=( const VersionNumber& lhs, const VersionNumber& rhs ) -> bool
{
    if ( lhs.majorVersion != rhs.majorVersion )
        return lhs.majorVersion > rhs.majorVersion;
    if ( lhs.minorVersion != rhs.minorVersion )
        return lhs.minorVersion > rhs.minorVersion;

    return lhs.patchVersion >= rhs.patchVersion;
}

struct GPUBuffer
{
    vk::UniqueBuffer buffer;
    vk::UniqueDeviceMemory memory;
};

struct LogicalDevice
{
    vk::UniqueDevice device;
    std::uint32_t queueFamilyIndex;

    operator const vk::Device&() const { return *device; }
    const vk::Device* operator->() const { return &*device; }
};

constexpr auto getVulkanSDKVersion() -> VersionNumber
{
    return VersionNumber{
        VK_API_VERSION_MAJOR( VK_HEADER_VERSION_COMPLETE ),
        VK_API_VERSION_MINOR( VK_HEADER_VERSION_COMPLETE ),
        VK_API_VERSION_PATCH( VK_HEADER_VERSION_COMPLETE )
    };
}

constexpr auto isMacOS() -> bool
{
    return
#if defined __MACH__
        true;
#else
        false;
#endif
}

auto printLayerProperties( const std::vector< vk::LayerProperties >& layers ) -> void
{
    for ( const auto& l : layers )
    {
        std::cout << "    " << l.layerName << "\n";
        const auto extensions = vk::enumerateInstanceExtensionProperties( l.layerName.operator std::string() );
        for ( const auto& e : extensions )
            std::cout << "       Extension: " << e.extensionName << "\n";
    }

    std::cout << "\n";
}

auto printExtensionProperties( const std::vector< vk::ExtensionProperties >& extensions ) -> void
{
    for ( const auto& e : extensions )
        std::cout << "    " << e.extensionName << "\n";

    std::cout << "\n";
}

auto createVulkanInstance() -> vk::UniqueInstance
{
    std::cout << "Vulkan SDK Version: " << getVulkanSDKVersion() << "\n";

    const auto layers = vk::enumerateInstanceLayerProperties();
    std::cout << "Available instance layers: \n";
    printLayerProperties( layers );

    const auto instanceExtensions = vk::enumerateInstanceExtensionProperties();
    std::cout << "Available instance extensions: \n";
    printExtensionProperties( instanceExtensions );

    const auto appInfo = vk::ApplicationInfo{}
        .setPApplicationName( "Vulkan C++ Tutorial" )
        .setApplicationVersion( 1u )
        .setPEngineName( "Vulkan C++ Tutorial Engine" )
        .setEngineVersion( 1u )
        .setApiVersion( VK_API_VERSION_1_1 );

    const auto layersToEnable = std::vector< const char* >{
        "VK_LAYER_KHRONOS_validation"
    };

    auto extensionsToEnable = std::vector< const char* >{
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
    };

    auto instanceCreateInfo = vk::InstanceCreateInfo{};

    // for newer versions of the sdk on macos we have to enable the portability extension
    if constexpr ( isMacOS() && getVulkanSDKVersion() >= VersionNumber{ 1, 3, 216 } )
    {
        extensionsToEnable.push_back( VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME );
        instanceCreateInfo.setFlags( vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR );
    }

    instanceCreateInfo
        .setPApplicationInfo( &appInfo )
        .setPEnabledLayerNames( layersToEnable )
        .setPEnabledExtensionNames( extensionsToEnable );

    return vk::createInstanceUnique( instanceCreateInfo );
}

auto printPhysicalDeviceProperties( const vk::PhysicalDevice& device ) -> void
{
    const auto props = device.getProperties();
    const auto features = device.getFeatures();

    std::cout <<
        "    " << props.deviceName << ":" <<
        "\n      is discrete GPU: " << ( props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu ? "yes" : "no" ) <<
        "\n      has geometry shader: " << ( features.geometryShader ? "yes" : "no" ) <<
        "\n      has tessellation shader: " << ( features.tessellationShader ? "yes" : "no" ) <<
        "\n      supports anisotropic filtering: " << ( features.samplerAnisotropy ? "yes" : "no" ) <<
        "\n";

    const auto deviceExtensions = device.enumerateDeviceExtensionProperties();
    std::cout << "\n    Available device extensions: \n";
    printExtensionProperties( deviceExtensions );
}

auto findBestPhysicalDevice( const std::vector< vk::PhysicalDevice >& devices ) -> vk::PhysicalDevice
{
    assert( !devices.empty() );

    for ( const auto& device : devices )
    {
        if ( device.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu )
            return device;
    }

    return devices[ 0 ];
}

auto selectPhysicalDevice( const vk::Instance& instance ) -> vk::PhysicalDevice
{
    const auto physicalDevices = instance.enumeratePhysicalDevices();
    if ( physicalDevices.empty() )
        throw std::runtime_error( "No Vulkan devices found" );

    std::cout << "Available physical devices:\n";
    for ( const auto& device : physicalDevices )
        printPhysicalDeviceProperties( device );

    const auto physicalDevice = findBestPhysicalDevice( physicalDevices );
    std::cout << "\nSelected Device: " << physicalDevice.getProperties().deviceName << "\n";

    return physicalDevice;
}

auto printQueueFamilyProperties( const vk::QueueFamilyProperties& props, std::uint32_t index ) -> void
{
    std::cout <<
        "\n    Queue Family " << index << ":\n" <<
        "\n        queue count: " << props.queueCount <<
        "\n        supports graphics operations: " << ( props.queueFlags & vk::QueueFlagBits::eGraphics ? "yes" : "no" ) <<
        "\n        supports compute operations: " << ( props.queueFlags & vk::QueueFlagBits::eCompute ? "yes" : "no" ) <<
        "\n        supports transfer operations: " << ( props.queueFlags & vk::QueueFlagBits::eTransfer ? "yes" : "no" ) <<
        "\n        supports sparse binding operations: " << ( props.queueFlags & vk::QueueFlagBits::eSparseBinding ? "yes" : "no" ) <<
        "\n";
}

auto findSuitableQueueFamily(
    const std::vector< vk::QueueFamilyProperties >& queueFamilies,
    vk::QueueFlags requiredFlags
) -> std::uint32_t
{
    std::uint32_t index = 0;
    for ( const auto& qf : queueFamilies )
    {
        if ( ( qf.queueFlags & requiredFlags ) == requiredFlags )
            return index;
        ++index;
    }
    throw std::runtime_error( "No suitable queue family found" );
}

auto getRequiredDeviceExtensions(
    const std::vector< vk::ExtensionProperties >& availableExtensions
) -> std::vector< const char* >
{
    // extension name strings need to be static, because we're returning a vector with pointers to the underlying char arrays
    static const std::string compatibilityExtensionName = "VK_KHR_portability_subset";

    auto result = std::vector< const char* >{};

    const auto it = std::find_if(
        availableExtensions.begin(),
        availableExtensions.end(),
        []( const vk::ExtensionProperties& e )
        {
            return compatibilityExtensionName == e.extensionName;
        }
    );

    if ( it != availableExtensions.end() )
        result.push_back( compatibilityExtensionName.c_str() );

    return result;
}

auto createLogicalDevice( const vk::PhysicalDevice& physicalDevice ) -> LogicalDevice
{
    const auto queueFamilies = physicalDevice.getQueueFamilyProperties();
    std::cout << "\nAvailable queue families:\n";
    std::uint32_t familyIndex = 0;
    for ( const auto& qf : queueFamilies )
    {
        printQueueFamilyProperties( qf, familyIndex );
        ++familyIndex;
    }

    const auto queueFamilyIndex = findSuitableQueueFamily(
        queueFamilies,
        vk::QueueFlagBits::eCompute | vk::QueueFlagBits::eTransfer
    );
    std::cout << "\nSelected queue family index: " << queueFamilyIndex << "\n";

    const auto queuePriority = 1.f;
    const auto queueCreateInfos = std::vector< vk::DeviceQueueCreateInfo >{
        vk::DeviceQueueCreateInfo{}
            .setQueueFamilyIndex( queueFamilyIndex )
            .setQueueCount( 1 )
            .setQueuePriorities( queuePriority )
    };

    const auto enabledDeviceExtensions = getRequiredDeviceExtensions(
        physicalDevice.enumerateDeviceExtensionProperties()
    );
    const auto deviceCreateInfo = vk::DeviceCreateInfo{}
        .setQueueCreateInfos( queueCreateInfos )
        .setPEnabledExtensionNames( enabledDeviceExtensions );

    return LogicalDevice{
        std::move( physicalDevice.createDeviceUnique( deviceCreateInfo ) ),
        queueFamilyIndex
    };
}

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
        const auto instance = createVulkanInstance();
        const auto physicalDevice = selectPhysicalDevice( *instance );
        const auto logicalDevice = createLogicalDevice( physicalDevice );

        constexpr size_t numElements = 500;
        auto inputData = std::array< int, numElements >{};
        std::iota( inputData.begin(), inputData.end(), 0 );

        auto outputData = std::array< float, numElements >{};

        const auto inputStagingBuffer = createGPUBuffer(
            physicalDevice,
            logicalDevice,
            sizeof( inputData ),
            vk::BufferUsageFlagBits::eTransferSrc
        );
        const auto inputGPUBuffer = createGPUBuffer(
            physicalDevice,
            logicalDevice,
            sizeof( inputData ),
            vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        );

        const auto outputBuffer = createGPUBuffer( physicalDevice, logicalDevice, sizeof( outputData ) );

        copyDataToBuffer( logicalDevice, inputData, inputStagingBuffer );

        const auto computeShader = createShaderModule( logicalDevice, "./shaders/compute.comp.spv" );

        const auto descriptorSetLayout = createDescriptorSetLayout( logicalDevice );
        const auto pipelineLayout = createPipelineLayout( logicalDevice, *descriptorSetLayout );
        const auto pipeline = createComputePipeline( logicalDevice, *pipelineLayout, *computeShader );

        const auto descriptorPool = createDescriptorPool( logicalDevice );
        const auto allocateInfo = vk::DescriptorSetAllocateInfo{}
            .setSetLayouts( *descriptorSetLayout )
            .setDescriptorPool( *descriptorPool );
        const auto descriptorSets = logicalDevice->allocateDescriptorSets( allocateInfo );

        const auto bufferInfos = std::vector< vk::DescriptorBufferInfo >{
            vk::DescriptorBufferInfo{}
                .setBuffer( *inputGPUBuffer.buffer )
                .setOffset( 0 )
                .setRange( sizeof( inputData ) ),
            vk::DescriptorBufferInfo{}
                .setBuffer( *outputBuffer.buffer )
                .setOffset( 0 )
                .setRange( sizeof( outputData ) ),
        };
        // dstBinding is the *first* binding to update; Vulkan updates consecutive
        // bindings for each element in bufferInfos (so binding 0 and 1 here).
        const auto writeDescriptorSet = vk::WriteDescriptorSet{}
            .setDstSet( descriptorSets[ 0 ] )
            .setDstBinding( 0 )
            .setDescriptorType( vk::DescriptorType::eStorageBuffer )
            .setBufferInfo( bufferInfos );

        logicalDevice->updateDescriptorSets( writeDescriptorSet, {} );

        const auto commandPool = logicalDevice->createCommandPoolUnique(
            vk::CommandPoolCreateInfo{}.setQueueFamilyIndex( logicalDevice.queueFamilyIndex )
        );

        const auto commandBufferAllocateInfo = vk::CommandBufferAllocateInfo{}
            .setCommandPool( *commandPool )
            .setLevel( vk::CommandBufferLevel::ePrimary )
            .setCommandBufferCount( 1 );
        const auto commandBuffer = logicalDevice->allocateCommandBuffers( commandBufferAllocateInfo )[0];

        const auto beginInfo = vk::CommandBufferBeginInfo{}
            .setFlags( vk::CommandBufferUsageFlagBits::eOneTimeSubmit );
        commandBuffer.begin( beginInfo );

        commandBuffer.bindPipeline( vk::PipelineBindPoint::eCompute, *pipeline );
        commandBuffer.bindDescriptorSets( vk::PipelineBindPoint::eCompute, *pipelineLayout, 0, descriptorSets, {} );
        commandBuffer.copyBuffer(
            *inputStagingBuffer.buffer,
            *inputGPUBuffer.buffer,
            vk::BufferCopy{}.setSize( sizeof( inputData ) )
        );

        const auto bufferBarrier = vk::BufferMemoryBarrier{}
            .setSrcAccessMask( vk::AccessFlagBits::eTransferWrite )
            .setDstAccessMask( vk::AccessFlagBits::eShaderRead )
            .setSrcQueueFamilyIndex( vk::QueueFamilyIgnored )
            .setDstQueueFamilyIndex( vk::QueueFamilyIgnored )
            .setBuffer( *inputGPUBuffer.buffer )
            .setOffset( 0 )
            .setSize( sizeof( inputData ) );
        commandBuffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eComputeShader,
            {},
            {},
            bufferBarrier,
            {}
        );

        commandBuffer.dispatch( 8, 1, 1 );

        commandBuffer.end();

        const auto queue = logicalDevice->getQueue( logicalDevice.queueFamilyIndex, 0 );

        const auto submitInfo = vk::SubmitInfo{}
            .setCommandBuffers( commandBuffer );
        queue.submit( submitInfo );

        logicalDevice->waitIdle();

        copyDataFromBuffer( logicalDevice, outputBuffer, outputData );

        for ( size_t i = 0; i < outputData.size(); ++i )
        {
            std::cout << outputData[i] << ";\t";
            if ( ( i % 16 ) == 15 )
                std::cout << "\n";
        }
    }
    catch( const std::exception& e )
    {
        std::cerr << "Exception thrown: " << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
