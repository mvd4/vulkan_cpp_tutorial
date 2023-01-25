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

#include <vulkan/vulkan.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>

struct version_number
{
    std::uint32_t major;
    std::uint32_t minor;
    std::uint32_t patch;
};

std::ostream& operator<<( std::ostream& os, const version_number& v )
{
    os << v.major << '.' << v.minor << '.' << v.patch;
    return os;
}

constexpr bool operator >= ( const version_number& lhs, const version_number& rhs )
{
    if ( lhs.major != rhs.major )
        return lhs.major > rhs.major;
    if ( lhs.minor != rhs.minor )
        return lhs.minor > rhs.minor;

    return lhs.patch >= rhs.patch;
}

constexpr version_number get_vulkan_sdk_version()
{
    return version_number{
        VK_API_VERSION_MAJOR(VK_HEADER_VERSION_COMPLETE ),
        VK_API_VERSION_MINOR(VK_HEADER_VERSION_COMPLETE ),
        VK_API_VERSION_PATCH(VK_HEADER_VERSION_COMPLETE )
    };
}

constexpr bool is_macos()
{
    return
#if defined __MACH__
        true;
#else
        false;
#endif
}


struct gpu_buffer
{
    vk::UniqueBuffer buffer;
    vk::UniqueDeviceMemory memory;
};


void print_layer_properties( const std::vector< vk::LayerProperties >& layers )
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

void print_extension_properties( const std::vector< vk::ExtensionProperties >& extensions )
{
    for ( const auto& e : extensions )
        std::cout << "    " << e.extensionName << "\n";

    std::cout << "\n";
}

vk::UniqueInstance create_instance()
{
    std::cout << "Vulkan SDK Version: " << get_vulkan_sdk_version() << "\n";

    const auto layers = vk::enumerateInstanceLayerProperties();
    std::cout << "Available instance layers: \n";
    print_layer_properties( layers );

    const auto instanceExtensions = vk::enumerateInstanceExtensionProperties();
    std::cout << "Available instance extensions: \n";
    print_extension_properties( instanceExtensions );

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
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME };

    auto instanceCreateInfo = vk::InstanceCreateInfo{};

    // for newer versions of the sdk on macos we have to enable the portability extension
    if constexpr ( is_macos() && get_vulkan_sdk_version() >= version_number{ 1, 3, 216 } )
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

void print_physical_device_properties( const vk::PhysicalDevice& device )
{
    const auto props = device.getProperties();
    const auto features = device.getFeatures();

    std::cout <<
        "  " << props.deviceName << ":" <<
        "\n      is discrete GPU: " << ( props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu ? "yes, " : "no, " ) <<
        "\n      has geometry shader: " << ( features.geometryShader ? "yes, " : "no, " ) <<
        "\n      has tesselation shader: " << ( features.tessellationShader ? "yes, " : "no, " ) <<
        "\n      supports anisotropic filtering: " << ( features.samplerAnisotropy ? "yes, " : "no, ") <<
        "\n";

    const auto deviceExtensions = device.enumerateDeviceExtensionProperties();
    std::cout << "\n  Available device extensions: \n";
    print_extension_properties( deviceExtensions );
}

vk::PhysicalDevice select_physical_device( const std::vector< vk::PhysicalDevice >& devices )
{
    size_t bestDeviceIndex = 0;
    size_t index = 0;
    for ( const auto& d : devices )
    {
        const auto props = d.getProperties();
        const auto features = d.getFeatures();

        const auto isDiscreteGPU = props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
        if ( isDiscreteGPU && bestDeviceIndex == 0 )
            bestDeviceIndex = index;

        ++index;
    }

    return devices[ bestDeviceIndex ];
}

vk::PhysicalDevice create_physical_device( const vk::Instance& instance )
{
    const auto physicalDevices = instance.enumeratePhysicalDevices();
    if ( physicalDevices.empty() )
        throw std::runtime_error( "No Vulkan devices found" );

    std::cout << "Available physical devices:\n";
    for ( const auto& d : physicalDevices )
        print_physical_device_properties( d );

    const auto physicalDevice = select_physical_device( physicalDevices );
    std::cout << "\nSelected Device: " << physicalDevice.getProperties().deviceName << "\n";
    return physicalDevice;
}

void print_queue_family_properties( const vk::QueueFamilyProperties& props, unsigned index )
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

std::uint32_t get_suitable_queue_family(
    const std::vector< vk::QueueFamilyProperties >& queueFamilies,
    vk::QueueFlags requiredFlags
)
{
    std::uint32_t index = 0;
    for ( const auto& q : queueFamilies )
    {
        if ( ( q.queueFlags & requiredFlags ) == requiredFlags )
            return index;
        ++index;
    }
    throw std::runtime_error( "No suitable queue family found" );
}

std::vector< const char* > get_required_device_extensions(
    const std::vector< vk::ExtensionProperties >& availableExtensions
)
{
    auto result = std::vector< const char* >{};

    static const std::string compatibilityExtensionName = "VK_KHR_portability_subset";
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


struct logical_device {
    vk::UniqueDevice device;
    std::uint32_t queueFamilyIndex;

    operator const vk::Device&() const { return *device; }
};

logical_device create_logical_device( const vk::PhysicalDevice& physicalDevice )
{
    const auto queueFamilies = physicalDevice.getQueueFamilyProperties();
    std::cout << "\nAvailable queue families:\n";
    unsigned familyIndex = 0;
    for ( const auto& qf : queueFamilies )
    {
        print_queue_family_properties( qf, familyIndex );
        ++familyIndex;
    }

    const auto queueFamilyIndex = get_suitable_queue_family(
        queueFamilies,
        vk::QueueFlagBits::eCompute
    );
    std::cout << "\nSelected queue family index: " << queueFamilyIndex << "\n";

    const auto queuePriority = 1.f;
    const auto queueCreateInfos = std::vector< vk::DeviceQueueCreateInfo >{
        vk::DeviceQueueCreateInfo{}
            .setQueueFamilyIndex( queueFamilyIndex )
            .setQueueCount( 1 )
            .setQueuePriorities( queuePriority )
    };

    const auto enabledDeviceExtensions = get_required_device_extensions(
        physicalDevice.enumerateDeviceExtensionProperties()
    );
    const auto deviceCreateInfo = vk::DeviceCreateInfo{}
        .setQueueCreateInfos( queueCreateInfos )
        .setPEnabledExtensionNames( enabledDeviceExtensions );

    return logical_device{
        std::move( physicalDevice.createDeviceUnique( deviceCreateInfo ) ),
        queueFamilyIndex
    };
}

std::uint32_t find_suitable_memory_index(
    const vk::PhysicalDeviceMemoryProperties& memoryProperties,
    std::uint32_t allowedTypesMask,
    vk::MemoryPropertyFlags requiredMemoryFlags
)
{
    for(
        std::uint32_t memoryType = 1, i = 0;
        i < memoryProperties.memoryTypeCount;
        ++i, memoryType <<= 1
    )
    {
        if(
            ( allowedTypesMask & memoryType ) > 0 &&
            ( ( memoryProperties.memoryTypes[i].propertyFlags & requiredMemoryFlags ) == requiredMemoryFlags )
        )
        {
            return i;
        }
    }

    throw std::runtime_error( "could not find suitable gpu memory" );
}


gpu_buffer create_gpu_buffer(
    const vk::PhysicalDevice& physicalDevice,
    const vk::Device& logicalDevice,
    std::uint32_t size
)
{
    const auto bufferCreateInfo = vk::BufferCreateInfo{}
        .setSize( size )
        .setUsage( vk::BufferUsageFlagBits::eStorageBuffer )
        .setSharingMode( vk::SharingMode::eExclusive );
    auto buffer = logicalDevice.createBufferUnique( bufferCreateInfo );

    const auto memoryRequirements = logicalDevice.getBufferMemoryRequirements( *buffer );
    const auto memoryProperties = physicalDevice.getMemoryProperties();
    const auto requiredMemoryFlags =
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;

    const auto memoryIndex = find_suitable_memory_index(
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


int main()
{
    try
    {
        const auto instance = create_instance();
        const auto physicalDevice = create_physical_device( *instance );
        const auto logicalDevice = create_logical_device( physicalDevice );

        constexpr size_t numElements = 500;
        auto inputData = std::array< int, numElements >{};
        int counter = 0;
        std::generate( inputData.begin(), inputData.end(), [&counter]() { return counter++; } );

        auto outputData = std::array< float, numElements >{};

        const auto inputBuffer = create_gpu_buffer( physicalDevice, logicalDevice, sizeof( inputData ) );
        const auto outputBuffer = create_gpu_buffer( physicalDevice, logicalDevice, sizeof( outputData ) );

        const auto mappedInputMemory = logicalDevice.device->mapMemory( *inputBuffer.memory, 0, sizeof( inputData ) );
        memcpy( mappedInputMemory, inputData.data(), sizeof( inputData ) );
        logicalDevice.device->unmapMemory( *inputBuffer.memory );

        const auto computeShader = create_shader_module( logicalDevice, "./shaders/compute.spv" );

        const auto descriptorSetLayout = create_descriptor_set_layout( logicalDevice );
        const auto pipelineLayout = create_pipeline_layout( logicalDevice, *descriptorSetLayout );
        const auto pipeline = create_compute_pipeline( logicalDevice, *pipelineLayout, *computeShader );

        const auto descriptorPool = create_descriptor_pool( logicalDevice );
        const auto allocateInfo = vk::DescriptorSetAllocateInfo{}
            .setSetLayouts( *descriptorSetLayout )
            .setDescriptorPool( *descriptorPool );
        const auto descriptorSets = logicalDevice.device->allocateDescriptorSets( allocateInfo );

        const auto bufferInfos = std::vector< vk::DescriptorBufferInfo >{
            vk::DescriptorBufferInfo{}
                .setBuffer( *inputBuffer.buffer )
                .setOffset( 0 )
                .setRange( sizeof( inputData ) ),
            vk::DescriptorBufferInfo{}
                .setBuffer( *outputBuffer.buffer )
                .setOffset( 0 )
                .setRange( sizeof( outputData ) ),
        };
        const auto writeDescriptorSet = vk::WriteDescriptorSet{}
            .setDstSet( descriptorSets[0] )
            .setDstBinding( 0 )
            .setDescriptorType( vk::DescriptorType::eStorageBuffer )
            .setBufferInfo( bufferInfos );

        logicalDevice.device->updateDescriptorSets( writeDescriptorSet, {} );

        const auto commandPool = logicalDevice.device->createCommandPoolUnique(
            vk::CommandPoolCreateInfo{}.setQueueFamilyIndex( logicalDevice.queueFamilyIndex )
        );

        const auto commandBufferAllocateInfo = vk::CommandBufferAllocateInfo{}
            .setCommandPool( *commandPool )
            .setLevel( vk::CommandBufferLevel::ePrimary )
            .setCommandBufferCount( 1 );
        const auto commandBuffer = logicalDevice.device->allocateCommandBuffers( commandBufferAllocateInfo )[0];

        const auto beginInfo = vk::CommandBufferBeginInfo{}
            .setFlags( vk::CommandBufferUsageFlagBits::eOneTimeSubmit );
        commandBuffer.begin( beginInfo );

        commandBuffer.end();
    }
    catch( const std::exception& e )
    {
        std::cout << "Exception thrown: " << e.what() << "\n";
        return -1;
    }
    return 0;
}
