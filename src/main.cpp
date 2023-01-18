/***************************************************************************************************

mvd Vulkan C++ Tutorial


Copyright 2021 mvd

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

auto create_instance()
{
    const auto appInfo = vk::ApplicationInfo{}
        .setPApplicationName( "Vulkan C++ Tutorial" )
        .setApplicationVersion( 1u )
        .setPEngineName( "Vulkan C++ Tutorial Engine" )
        .setEngineVersion( 1u )
        .setApiVersion( VK_API_VERSION_1_1 );

    const auto instanceCreateInfo = vk::InstanceCreateInfo{}
        .setPApplicationInfo( &appInfo );

    return vk::createInstanceUnique( instanceCreateInfo );
}

void print_physical_device_properties( const vk::PhysicalDevice& device )
{
    const auto props = device.getProperties();
    const auto features = device.getFeatures();

    std::cout <<
        "    " << props.deviceName << ":" <<
        "\n      is discrete GPU: " << ( props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu ? "yes, " : "no, " ) <<
        "\n      has geometry shader: " << ( features.geometryShader ? "yes, " : "no, " ) <<
        "\n      has tesselation shader: " << ( features.tessellationShader ? "yes, " : "no, " ) <<
        "\n      supports anisotropic filtering: " << ( features.samplerAnisotropy ? "yes, " : "no, ") <<
        "\n";
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

int main()
{
    try
    {
        const auto instance = create_instance();
        const auto physicalDevices = create_physical_device( *instance );
    }
    catch( const std::exception& e )
    {
        std::cout << "Exception thrown: " << e.what() << "\n";
        return -1;
    }
    return 0;
}
