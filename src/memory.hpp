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

#include <cstring>

namespace vcpp
{
    struct GPUBuffer
    {
        vk::UniqueBuffer buffer;
        vk::UniqueDeviceMemory memory;
    };

    auto findSuitableMemoryIndex(
        const vk::PhysicalDeviceMemoryProperties& memoryProperties,
        std::uint32_t allowedTypesMask,
        vk::MemoryPropertyFlags requiredMemoryFlags
    ) -> std::uint32_t;

    auto createGPUBuffer(
        const vk::PhysicalDevice& physicalDevice,
        const vk::Device& logicalDevice,
        std::uint64_t size,
        vk::BufferUsageFlags usageFlags = vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlags requiredMemoryFlags =
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    ) -> GPUBuffer;

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
}
