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

#pragma once

#include <vulkan/vulkan.hpp>


namespace vcpp
{
    struct logical_device {
        vk::UniqueDevice device;
        std::uint32_t queueFamilyIndex;

        operator const vk::Device&() const { return *device; }
    };

    void print_layer_properties( const std::vector< vk::LayerProperties >& layers );

    void print_extension_properties( const std::vector< vk::ExtensionProperties >& extensions );

    vk::UniqueInstance create_instance();

    void print_physical_device_properties( const vk::PhysicalDevice& device );

    vk::PhysicalDevice select_physical_device( const std::vector< vk::PhysicalDevice >& devices );

    vk::PhysicalDevice create_physical_device( const vk::Instance& instance );

    void print_queue_family_properties( const vk::QueueFamilyProperties& props, unsigned index );

    std::uint32_t get_suitable_queue_family(
        const std::vector< vk::QueueFamilyProperties >& queueFamilies,
        vk::QueueFlags requiredFlags
    );

    std::vector< const char* > get_required_device_extensions(
        const std::vector< vk::ExtensionProperties >& availableExtensions
    );

    logical_device create_logical_device( const vk::PhysicalDevice& physicalDevice );
}