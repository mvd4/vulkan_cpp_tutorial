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

#include "presentation.hpp"

namespace vcpp
{

    vk::UniqueSwapchainKHR create_swapchain(
        const vk::Device& logicalDevice,
        const vk::SurfaceKHR& surface,
        const vk::SurfaceFormatKHR& surfaceFormat,
        const vk::Extent2D& surfaceExtent,
        const std::uint32_t numSwapchainImages
    )
    {
        const auto createInfo = vk::SwapchainCreateInfoKHR{}
            .setSurface( surface )
            .setMinImageCount( numSwapchainImages )
            .setImageFormat( surfaceFormat.format )
            .setImageColorSpace( surfaceFormat.colorSpace )
            .setImageExtent( surfaceExtent )
            .setImageArrayLayers( 1 )
            .setImageUsage( vk::ImageUsageFlagBits::eColorAttachment )
            .setImageSharingMode( vk::SharingMode::eExclusive )
            .setPreTransform( vk::SurfaceTransformFlagBitsKHR::eIdentity )
            .setCompositeAlpha( vk::CompositeAlphaFlagBitsKHR::eOpaque )
            .setPresentMode( vk::PresentModeKHR::eFifo )
            .setClipped( true );
        
        return logicalDevice.createSwapchainKHRUnique( createInfo );
    }


    vk::UniqueImageView create_image_view(
        const vk::Device& logicalDevice,
        const vk::Image& image,
        const vk::Format& format
    )
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


    std::vector< vk::UniqueImageView > create_swapchain_image_views(
        const vk::Device& logicalDevice,
        const vk::SwapchainKHR& swapChain,
        const vk::Format& imageFormat
    )
    {
        auto swapChainImages = logicalDevice.getSwapchainImagesKHR( swapChain );

        std::vector< vk::UniqueImageView > swapChainImageViews;
        for( const auto img : swapChainImages )
        {
            swapChainImageViews.push_back(
                create_image_view( logicalDevice, img, imageFormat )
            );
        }

        return swapChainImageViews;
    }


    std::vector< vk::UniqueFramebuffer > create_framebuffers(
        const vk::Device& logicalDevice,
        const std::vector< vk::UniqueImageView >& imageViews,
        const vk::Extent2D& imageExtent,
        const vk::RenderPass& renderPass
    )
    {
        std::vector< vk::UniqueFramebuffer > result;
        for( const auto& v : imageViews )
        {
            std::array< vk::ImageView, 1 > attachments = { *v };
            const auto frameBufferCreateInfo = vk::FramebufferCreateInfo{}
                .setRenderPass( renderPass )
                .setAttachments( attachments )
                .setWidth( imageExtent.width )
                .setHeight( imageExtent.height )
                .setLayers( 1 );

            result.push_back( logicalDevice.createFramebufferUnique( frameBufferCreateInfo ) );
        }

        return result;
    }
}