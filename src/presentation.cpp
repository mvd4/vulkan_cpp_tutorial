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

#include "presentation.hpp"


namespace {

    auto createImageView(
        const vk::Device& logicalDevice,
        const vk::Image& image,
        const vk::Format& format
    ) -> vk::UniqueImageView
    {
        const auto subresourceRange = vk::ImageSubresourceRange{}
            .setAspectMask( vk::ImageAspectFlagBits::eColor )
            .setBaseMipLevel( 0 )
            .setLevelCount( 1 )
            .setBaseArrayLayer( 0 )
            .setLayerCount( 1 );

        const auto createInfo = vk::ImageViewCreateInfo{}
            .setImage( image )
            .setViewType( vk::ImageViewType::e2D )
            .setFormat( format )
            .setSubresourceRange( subresourceRange );

        return logicalDevice.createImageViewUnique( createInfo );
    }

}


namespace vcpp
{

    auto createSwapchain(
        const vk::Device& logicalDevice,
        const vk::SurfaceKHR& surface,
        const vk::SurfaceFormatKHR& surfaceFormat,
        const vk::Extent2D& surfaceExtent,
        std::uint32_t minSwapchainImageCount
    ) -> vk::UniqueSwapchainKHR
    {
        const auto createInfo = vk::SwapchainCreateInfoKHR{}
            .setSurface( surface )
            .setMinImageCount( minSwapchainImageCount )
            .setImageFormat( surfaceFormat.format )
            .setImageColorSpace( surfaceFormat.colorSpace )
            .setImageExtent( surfaceExtent )
            .setImageArrayLayers( 1u )
            .setImageUsage( vk::ImageUsageFlagBits::eColorAttachment )
            .setImageSharingMode( vk::SharingMode::eExclusive )
            .setPreTransform( vk::SurfaceTransformFlagBitsKHR::eIdentity )
            .setCompositeAlpha( vk::CompositeAlphaFlagBitsKHR::eOpaque )
            .setPresentMode( vk::PresentModeKHR::eFifo )
            .setClipped( true );

        return logicalDevice.createSwapchainKHRUnique( createInfo );
    }

    auto createSwapchainImageViews(
        const vk::Device& logicalDevice,
        const vk::SwapchainKHR& swapchain,
        const vk::Format& imageFormat
    ) -> std::vector< vk::UniqueImageView >
    {
        const auto swapchainImages = logicalDevice.getSwapchainImagesKHR( swapchain );

        std::vector< vk::UniqueImageView > swapchainImageViews;
        for( const auto& img : swapchainImages )
        {
            swapchainImageViews.push_back(
                createImageView( logicalDevice, img, imageFormat )
            );
        }

        return swapchainImageViews;
    }

    auto createFramebuffers(
        const vk::Device& logicalDevice,
        const std::vector< vk::UniqueImageView >& imageViews,
        const vk::Extent2D& imageExtent,
        const vk::RenderPass& renderPass
    ) -> std::vector< vk::UniqueFramebuffer >
    {
        std::vector< vk::UniqueFramebuffer > result;
        for( const auto& view : imageViews )
        {
            const std::array< vk::ImageView, 1 > attachments = { *view };
            const auto framebufferCreateInfo = vk::FramebufferCreateInfo{}
                .setRenderPass( renderPass )
                .setAttachments( attachments )
                .setWidth( imageExtent.width )
                .setHeight( imageExtent.height )
                .setLayers( 1 );

            result.push_back( logicalDevice.createFramebufferUnique( framebufferCreateInfo ) );
        }

        return result;
    }

} // namespace vcpp
