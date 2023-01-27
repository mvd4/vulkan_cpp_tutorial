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
    vk::UniqueSwapchainKHR create_swapchain(
        const vk::Device& logicalDevice,
        const vk::SurfaceKHR& surface,
        const vk::SurfaceFormatKHR& surfaceFormat,
        const vk::Extent2D& surfaceExtent,
        const std::uint32_t numSwapchainImages
    );

    std::vector< vk::UniqueImageView > create_swapchain_image_views(
        const vk::Device& logicalDevice,
        const vk::SwapchainKHR& swapChain,
        const vk::Format& imageFormat
    );

    std::vector< vk::UniqueFramebuffer > create_framebuffers(
        const vk::Device& logicalDevice,
        const std::vector< vk::UniqueImageView >& imageViews,
        const vk::Extent2D& imageExtent,
        const vk::RenderPass& renderPass
    );
}
