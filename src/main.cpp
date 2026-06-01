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

#include <cassert>
#include <iostream>
#include <stdexcept>
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

constexpr auto operator >= ( const VersionNumber& lhs, const VersionNumber& rhs ) -> bool
{
    if ( lhs.majorVersion != rhs.majorVersion )
        return lhs.majorVersion > rhs.majorVersion;
    if ( lhs.minorVersion != rhs.minorVersion )
        return lhs.minorVersion > rhs.minorVersion;

    return lhs.patchVersion >= rhs.patchVersion;
}

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
        std::cout << "    " << l.layerName << "\n";

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

    auto instanceCreateInfo = vk::InstanceCreateInfo{}
        .setPApplicationInfo( &appInfo );

    // for newer versions of the sdk on macos we have to enable the portability extension
    auto extensionsToEnable = std::vector< const char* >{};
    if constexpr ( isMacOS() && getVulkanSDKVersion() >= VersionNumber{ 1, 3, 216 } )
    {
        extensionsToEnable.push_back( VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME );
        instanceCreateInfo.setFlags( vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR );
    }

    instanceCreateInfo.setPEnabledExtensionNames( extensionsToEnable );

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

auto createLogicalDevice( const vk::PhysicalDevice& physicalDevice ) -> vk::UniqueDevice
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

    const auto deviceCreateInfo = vk::DeviceCreateInfo{}
        .setQueueCreateInfos( queueCreateInfos );

    return physicalDevice.createDeviceUnique( deviceCreateInfo );
}


auto main() -> int
{
    try
    {
        const auto instance = createVulkanInstance();
        const auto physicalDevice = selectPhysicalDevice( *instance );
        const auto logicalDevice = createLogicalDevice( physicalDevice );
    }
    catch( const std::exception& e )
    {
        std::cerr << "Exception thrown: " << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
