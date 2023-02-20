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
    struct gpu_image
    {
        vk::UniqueImage image;
        vk::UniqueDeviceMemory memory;
    };



    class swapchain
    {
    public:

        struct frame_data
        {
            std::uint32_t swapchainImageIndex;
            std::uint32_t inFlightIndex;

            const vk::Framebuffer& framebuffer;

            const vk::Fence& inFlightFence;
            const vk::Semaphore& readyForRenderingSemaphore;
            const vk::Semaphore& readyForPresentingSemaphore;
        };


        swapchain(
            const vk::PhysicalDevice& physicalDevice,
            const vk::Device& logicalDevice,
            const vk::RenderPass& renderPass,
            const vk::SurfaceKHR& surface,
            const vk::SurfaceFormatKHR& surfaceFormat,
            const vk::Extent2D& imageExtent,
            std::uint32_t maxImagesInFlight );

        operator vk::SwapchainKHR() const { return *m_swapchain; }

        frame_data get_next_frame();


    private:

        vk::Device m_logicalDevice;
        vk::UniqueSwapchainKHR m_swapchain;

        std::uint32_t m_maxImagesInFlight;
        std::uint32_t m_currentFrameIndex = 0;

        std::vector< vk::UniqueImageView > m_imageViews;
        std::vector< vk::UniqueFramebuffer > m_framebuffers;

        gpu_image m_depthImage;
        vk::UniqueImageView m_depthImageView;

        std::vector< vk::UniqueFence > m_inFlightFences;
        std::vector< vk::UniqueSemaphore > m_readyForRenderingSemaphores;
        std::vector< vk::UniqueSemaphore > m_readyForPresentingSemaphores;
    };

    using swapchain_ptr_t = std::unique_ptr< vcpp::swapchain >;

    swapchain_ptr_t create_swapchain(
        const vk::PhysicalDevice& physicalDevice,
        const vk::Device& logicalDevice,
        const vk::RenderPass& renderPass,
        const vk::SurfaceKHR& surface,
        const vk::SurfaceFormatKHR& surfaceFormat,
        const vk::Extent2D& imageExtent,
        std::uint32_t maxImagesInFlight );
}
