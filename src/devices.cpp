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
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

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
}


namespace vcpp
{
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

    auto createVulkanInstance( const std::vector< std::string >& requiredExtensions ) -> vk::UniqueInstance
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
            VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME };

        for ( const auto& e : requiredExtensions )
            extensionsToEnable.push_back( e.c_str() );

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
        const vk::PhysicalDevice& physicalDevice,
        vk::QueueFlags requiredFlags,
        std::optional< const vk::SurfaceKHR > surface
     ) -> std::uint32_t
     {
        const auto queueFamilies = physicalDevice.getQueueFamilyProperties();

        std::uint32_t index = 0;
        for ( const auto& qf : queueFamilies )
        {
            if (
                ( !surface.has_value() || physicalDevice.getSurfaceSupportKHR( index, *surface ) ) &&
                ( qf.queueFlags & requiredFlags ) == requiredFlags
            )
            {
                return index;
            }

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

    auto createLogicalDevice(
        const vk::PhysicalDevice& physicalDevice,
        vk::QueueFlags requiredFlags,
        std::optional< const vk::SurfaceKHR > surface
    ) -> LogicalDevice
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
            physicalDevice,
            requiredFlags,
            surface
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
}