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

#include "devices.hpp"
#include "memory.hpp"

#include <vulkan/vulkan.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>


namespace vcpp
{
    struct SceneData
    {
        std::span< const std::byte > vertexData;
        std::uint32_t vertexCount;
    };

    struct Renderer
    {
        Renderer(
            const vk::PhysicalDevice& physicalDevice,
            const LogicalDevice& logicalDevice,
            const vk::ShaderModule& vertexShader,
            const vk::ShaderModule& fragmentShader,
            vk::Format colorFormat,
            std::span< const vk::Format > vertexFormats,
            std::uint64_t sizeVertexDataInBytes,
            std::uint32_t maxFramesInFlight
        );

        vk::UniqueRenderPass renderPass;
        vk::UniquePipelineLayout pipelineLayout;
        vk::UniquePipeline pipeline;
        vk::UniqueCommandPool commandPool;

        std::vector< GPUBuffer > vertexBuffers;
        std::vector< vk::CommandBuffer > commandBuffers;
    };

    auto recordCommandBuffer(
        const vk::CommandBuffer& commandBuffer,
        const vk::Pipeline& pipeline,
        const vk::RenderPass& renderPass,
        const vk::Framebuffer& frameBuffer,
        const vk::Extent2D& renderExtent,
        const vk::Buffer& vertexBuffer,
        std::uint32_t vertexCount
    ) -> void;
}
