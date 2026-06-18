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

#include <cstdint>

namespace vcpp
{
    class Swapchain
    {
    public:

        struct FrameData
        {
            std::uint32_t frameInFlightIndex;
            std::uint32_t swapchainImageIndex;

            vk::Framebuffer framebuffer;

            vk::Fence inFlightFence;
            vk::Semaphore readyForRenderingSemaphore;
            vk::Semaphore readyForPresentingSemaphore;
        };

        Swapchain(
            const vk::Device& logicalDevice,
            const vk::RenderPass& renderPass,
            const vk::SurfaceKHR& surface,
            const vk::SurfaceFormatKHR& surfaceFormat,
            const vk::Extent2D& imageExtent,
            std::uint32_t maxFramesInFlight,
            std::uint32_t requestedSwapchainImageCount
        );

        Swapchain( const Swapchain& ) = delete;
        Swapchain( Swapchain&& ) = delete;
        auto operator=( const Swapchain& ) -> Swapchain& = delete;
        auto operator=( Swapchain&& ) -> Swapchain& = delete;

        operator vk::SwapchainKHR() const { return *m_swapchain; }

        auto getNextFrame() -> FrameData;

    private:

        vk::Device m_logicalDevice;
        vk::UniqueSwapchainKHR m_swapchain;

        std::uint32_t m_currentFrameIndex = 0;

        std::vector< vk::UniqueImageView > m_imageViews;
        std::vector< vk::UniqueFramebuffer > m_framebuffers;

        std::vector< vk::UniqueFence > m_inFlightFences;
        std::vector< vk::UniqueSemaphore > m_readyForRenderingSemaphores;
        std::vector< vk::UniqueSemaphore > m_readyForPresentingSemaphores;
    };
}
