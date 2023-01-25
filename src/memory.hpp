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
    struct gpu_buffer
    {
        vk::UniqueBuffer buffer;
        vk::UniqueDeviceMemory memory;
    };


    std::uint32_t find_suitable_memory_index(
        const vk::PhysicalDeviceMemoryProperties& memoryProperties,
        std::uint32_t allowedTypesMask,
        vk::MemoryPropertyFlags requiredMemoryFlags
    );


    gpu_buffer create_gpu_buffer(
        const vk::PhysicalDevice& physicalDevice,
        const vk::Device& logicalDevice,
        std::uint32_t size,
        vk::BufferUsageFlags usageFlags = vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlags requiredMemoryFlags =
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );

    template< typename T, size_t N >
    void copy_data_to_buffer( 
        const vk::Device& logicalDevice,
        const std::array< T, N >& data,
        const gpu_buffer& buffer
    )
    {
        const auto numBytesToCopy = sizeof( data );
        const auto mappedMemory = logicalDevice.mapMemory( *buffer.memory, 0, numBytesToCopy );
        memcpy( mappedMemory, data.data(), numBytesToCopy );
        logicalDevice.unmapMemory( *buffer.memory );
    }

    template< typename T, size_t N >
    void copy_data_from_buffer( 
        const vk::Device& logicalDevice,
        const gpu_buffer& buffer, 
        std::array< T, N >& data
    )
    {
        const auto numBytesToCopy = sizeof( data );
        const auto mappedMemory = logicalDevice.mapMemory( *buffer.memory, 0, numBytesToCopy );
        memcpy( data.data(), mappedMemory, numBytesToCopy );
        logicalDevice.unmapMemory( *buffer.memory );
    }

}
