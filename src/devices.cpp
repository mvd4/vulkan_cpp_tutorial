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

#include "devices.hpp"

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

namespace vcpp
{
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
}
