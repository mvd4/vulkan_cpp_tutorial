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
#include "presentation.hpp"

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

    class Renderer
    {
    public:

        Renderer(
            const vk::PhysicalDevice& physicalDevice,
            const LogicalDevice& logicalDevice,
            const vk::ShaderModule& vertexShader,
            const vk::ShaderModule& fragmentShader,
            vk::Format colorFormat,
            std::span< const vk::Format > vertexFormats,
            std::uint32_t maxFramesInFlight
        );

        Renderer( const Renderer& ) = delete;
        Renderer( Renderer&& ) = delete;
        auto operator=( const Renderer& ) -> Renderer& = delete;
        auto operator=( Renderer&& ) -> Renderer& = delete;

        auto drawFrame(
            const SceneData& sceneData,
            const vk::Queue& queue,
            const Swapchain::FrameData& frame,
            const vk::Extent2D& imageExtent
        ) -> void;

        auto getRenderPass() const -> vk::RenderPass { return *m_renderPass; }

    private:

        vk::PhysicalDevice m_physicalDevice;
        vk::Device m_logicalDevice;

        vk::UniqueRenderPass m_renderPass;
        vk::UniquePipelineLayout m_pipelineLayout;
        vk::UniquePipeline m_pipeline;
        vk::UniqueCommandPool m_commandPool;

        std::vector< GPUBuffer > m_vertexBuffers;
        std::vector< vk::CommandBuffer > m_commandBuffers;
    };
}
