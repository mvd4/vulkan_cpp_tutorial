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

#include "memory.hpp"


namespace vcpp
{
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
        std::uint32_t size,
        vk::BufferUsageFlags usageFlags,
        vk::MemoryPropertyFlags requiredMemoryFlags
    )
    {
        const auto bufferCreateInfo = vk::BufferCreateInfo{}
            .setSize( size )
            .setUsage( usageFlags )
            .setSharingMode( vk::SharingMode::eExclusive );
        auto buffer = logicalDevice.createBufferUnique( bufferCreateInfo );

        const auto memoryRequirements = logicalDevice.getBufferMemoryRequirements( *buffer );
        const auto memoryProperties = physicalDevice.getMemoryProperties();

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

}