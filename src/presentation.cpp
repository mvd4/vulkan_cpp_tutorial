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

#include "memory.hpp"

#include <array>
#include <cassert>
#include <limits>

namespace {

    auto createImageView(
        const vk::Device& logicalDevice,
        const vk::Image& image,
        const vk::Format& format,
        const vk::ImageAspectFlags flags = vk::ImageAspectFlagBits::eColor
    ) -> vk::UniqueImageView
    {
        const auto subresourceRange = vk::ImageSubresourceRange{}
            .setAspectMask( flags )
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
        const std::vector< vk::UniqueImageView >& depthImageViews,
        const vk::Extent2D& imageExtent,
        const vk::RenderPass& renderPass
    ) -> std::vector< vk::UniqueFramebuffer >
    {
        std::vector< vk::UniqueFramebuffer > result;
        for( std::size_t i = 0; i < imageViews.size(); ++i )
        {
            const std::array< vk::ImageView, 2 > attachments = { *imageViews[ i ], *depthImageViews[ i ] };
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

    auto createSwapchainInternal(
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

    auto createDepthImage(
        const vk::PhysicalDevice& physicalDevice,
        const vk::Device& logicalDevice,
        const vk::Extent2D& imageExtent
    ) -> vcpp::GPUImage
    {
        const auto createInfo = vk::ImageCreateInfo{}
            .setImageType( vk::ImageType::e2D )
            .setFormat( vcpp::depthFormat )
            .setExtent( vk::Extent3D{ imageExtent.width, imageExtent.height, 1 } )
            .setMipLevels( 1 )
            .setArrayLayers( 1 )
            .setSamples( vk::SampleCountFlagBits::e1 )
            .setTiling( vk::ImageTiling::eOptimal )
            .setUsage( vk::ImageUsageFlagBits::eDepthStencilAttachment )
            .setSharingMode( vk::SharingMode::eExclusive )
            .setInitialLayout( vk::ImageLayout::eUndefined );
        auto image = logicalDevice.createImageUnique( createInfo );

        const auto memoryRequirements = logicalDevice.getImageMemoryRequirements( *image );
        const auto memoryProperties = physicalDevice.getMemoryProperties();

        const auto memoryIndex = vcpp::findSuitableMemoryIndex(
            memoryProperties,
            memoryRequirements.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eDeviceLocal );

        const auto allocateInfo = vk::MemoryAllocateInfo{}
            .setAllocationSize( memoryRequirements.size )
            .setMemoryTypeIndex( memoryIndex );

        auto memory = logicalDevice.allocateMemoryUnique( allocateInfo );
        logicalDevice.bindImageMemory( *image, *memory, 0u );

        return { std::move( image ), std::move( memory ) };
    }

    auto createDepthImages(
        const vk::PhysicalDevice& physicalDevice,
        const vk::Device& logicalDevice,
        const vk::Extent2D& imageExtent,
        std::size_t count
    ) -> std::vector< vcpp::GPUImage >
    {
        std::vector< vcpp::GPUImage > result;
        result.reserve( count );
        for( std::size_t i = 0; i < count; ++i )
            result.push_back( createDepthImage( physicalDevice, logicalDevice, imageExtent ) );

        return result;
    }

    auto createDepthImageViews(
        const vk::Device& logicalDevice,
        const std::vector< vcpp::GPUImage >& depthImages
    ) -> std::vector< vk::UniqueImageView >
    {
        std::vector< vk::UniqueImageView > result;
        result.reserve( depthImages.size() );
        for( const auto& depthImage : depthImages )
        {
            result.push_back( createImageView(
                logicalDevice,
                *depthImage.image,
                vcpp::depthFormat,
                vk::ImageAspectFlagBits::eDepth
            ) );
        }

        return result;
    }
}


namespace vcpp
{
    Swapchain::Swapchain(
        const vk::PhysicalDevice& physicalDevice,
        const vk::Device& logicalDevice,
        const vk::RenderPass& renderPass,
        const vk::SurfaceKHR& surface,
        const vk::SurfaceFormatKHR& surfaceFormat,
        const vk::Extent2D& imageExtent,
        std::uint32_t maxFramesInFlight,
        std::uint32_t requestedSwapchainImageCount
    )
        : m_logicalDevice{ logicalDevice }
        , m_swapchain{ createSwapchainInternal( logicalDevice, surface, surfaceFormat, imageExtent, requestedSwapchainImageCount ) }
        , m_imageViews{ createSwapchainImageViews( logicalDevice, *m_swapchain, surfaceFormat.format ) }
        , m_depthImages{ createDepthImages( physicalDevice, logicalDevice, imageExtent, m_imageViews.size() ) }
        , m_depthImageViews{ createDepthImageViews( logicalDevice, m_depthImages ) }
        , m_framebuffers{ createFramebuffers( logicalDevice, m_imageViews, m_depthImageViews, imageExtent, renderPass ) }
    {
        assert( maxFramesInFlight > 0 );
        assert( requestedSwapchainImageCount > 0 );

        for( std::uint32_t f = 0; f < maxFramesInFlight; ++f )
        {
            m_inFlightFences.push_back( logicalDevice.createFenceUnique(
                vk::FenceCreateInfo{}.setFlags( vk::FenceCreateFlagBits::eSignaled )
            ) );

            m_readyForRenderingSemaphores.push_back( logicalDevice.createSemaphoreUnique(
                vk::SemaphoreCreateInfo{}
            ) );

            m_readyForPresentingSemaphores.push_back( logicalDevice.createSemaphoreUnique(
                vk::SemaphoreCreateInfo{}
            ) );
        }
    }

    auto Swapchain::getNextFrame() -> FrameData
    {
        [[maybe_unused]] const auto result = m_logicalDevice.waitForFences(
            *m_inFlightFences[ m_currentFrameIndex ],
            true,
            std::numeric_limits< std::uint64_t >::max() );
        m_logicalDevice.resetFences( *m_inFlightFences[ m_currentFrameIndex ] );

        const auto swapchainImageIndex = m_logicalDevice.acquireNextImageKHR(
            *m_swapchain,
            std::numeric_limits< std::uint64_t >::max(),
            *m_readyForRenderingSemaphores[ m_currentFrameIndex ] ).value;

        const auto frame = FrameData{
            m_currentFrameIndex,
            swapchainImageIndex,
            *m_framebuffers[ swapchainImageIndex ],
            *m_inFlightFences[ m_currentFrameIndex ],
            *m_readyForRenderingSemaphores[ m_currentFrameIndex ],
            *m_readyForPresentingSemaphores[ m_currentFrameIndex ]
        };

        m_currentFrameIndex = ( m_currentFrameIndex + 1 ) % m_inFlightFences.size();
        return frame;
    }

    auto createSwapchain(
        const vk::PhysicalDevice& physicalDevice,
        const vk::Device& logicalDevice,
        const vk::RenderPass& renderPass,
        const vk::SurfaceKHR& surface,
        const vk::SurfaceFormatKHR& surfaceFormat,
        const vk::Extent2D& imageExtent,
        std::uint32_t maxFramesInFlight,
        std::uint32_t requestedSwapchainImageCount
    ) -> std::unique_ptr< Swapchain >
    {
        return std::make_unique< Swapchain >(
            physicalDevice,
            logicalDevice,
            renderPass,
            surface,
            surfaceFormat,
            imageExtent,
            maxFramesInFlight,
            requestedSwapchainImageCount
         );
    }
} // namespace vcpp
