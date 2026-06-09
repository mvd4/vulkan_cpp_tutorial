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

#pragma once

#include <vulkan/vulkan.hpp>


namespace vcpp
{
    struct LogicalDevice
    {
        vk::UniqueDevice device;
        std::uint32_t queueFamilyIndex;

        operator const vk::Device&() const { return *device; }
        const vk::Device* operator->() const { return &*device; }
    };


    auto printLayerProperties( const std::vector< vk::LayerProperties >& layers ) -> void;

    auto printExtensionProperties( const std::vector< vk::ExtensionProperties >& extensions ) -> void;

    auto createVulkanInstance() -> vk::UniqueInstance;

    auto printPhysicalDeviceProperties( const vk::PhysicalDevice& device ) -> void;

    auto selectPhysicalDevice( const vk::Instance& instance ) -> vk::PhysicalDevice;

    auto printQueueFamilyProperties( const vk::QueueFamilyProperties& props, std::uint32_t index ) -> void;

    auto findSuitableQueueFamily(
        const std::vector< vk::QueueFamilyProperties >& queueFamilies,
        vk::QueueFlags requiredFlags
    ) -> std::uint32_t;

    auto getRequiredDeviceExtensions(
        const std::vector< vk::ExtensionProperties >& availableExtensions
    ) -> std::vector< const char* >;

    auto createLogicalDevice( const vk::PhysicalDevice& physicalDevice ) -> LogicalDevice;
}
