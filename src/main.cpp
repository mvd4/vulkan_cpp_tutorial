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

#include <iostream>


auto createVulkanInstance() -> vk::UniqueInstance
{
    const auto appInfo = vk::ApplicationInfo{}
        .setPApplicationName( "Vulkan C++ Tutorial" )
        .setApplicationVersion( 1u )
        .setPEngineName( "Vulkan C++ Tutorial Engine" )
        .setEngineVersion( 1u )
        .setApiVersion( VK_API_VERSION_1_1 );

    auto instanceCreateInfo = vk::InstanceCreateInfo{}
        .setPApplicationInfo( &appInfo );

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
    for ( const auto& d : physicalDevices )
        printPhysicalDeviceProperties( d );

    const auto physicalDevice = findBestPhysicalDevice( physicalDevices );
    std::cout << "\nSelected Device: " << physicalDevice.getProperties().deviceName << "\n";

    return physicalDevice;
}


auto main() -> int
{
    try
    {
        const auto instance = createVulkanInstance();
        const auto physicalDevice = selectPhysicalDevice( *instance );
    }
    catch( const std::exception& e )
    {
        std::cerr << "Exception thrown: " << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
