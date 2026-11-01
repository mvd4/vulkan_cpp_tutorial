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

#include "rendering.hpp"

#include "pipelines.hpp"

#include <array>


namespace vcpp
{
    Renderer::Renderer(
        const vk::PhysicalDevice& physicalDevice,
        const LogicalDevice& logicalDevice,
        const vk::ShaderModule& vertexShader,
        const vk::ShaderModule& fragmentShader,
        vk::Format colorFormat,
        std::span< const vk::Format > vertexFormats,
        std::uint64_t sizeVertexDataInBytes,
        std::uint32_t maxFramesInFlight
    )
        : renderPass{ createRenderPass( logicalDevice, colorFormat ) }
        , pipelineLayout{ createGraphicsPipelineLayout( logicalDevice ) }
        , pipeline{ createGraphicsPipeline(
            logicalDevice,
            *pipelineLayout,
            vertexShader,
            fragmentShader,
            *renderPass,
            vertexFormats
        ) }
        , commandPool{ logicalDevice->createCommandPoolUnique(
            vk::CommandPoolCreateInfo{}
                .setFlags( vk::CommandPoolCreateFlagBits::eResetCommandBuffer )
                .setQueueFamilyIndex( logicalDevice.queueFamilyIndex )
        ) }
    {
        for ( std::uint32_t f = 0; f < maxFramesInFlight; ++f )
        {
            vertexBuffers.push_back( createGPUBuffer(
                physicalDevice,
                logicalDevice,
                sizeVertexDataInBytes,
                vk::BufferUsageFlagBits::eVertexBuffer
            ) );
        }

        const auto commandBufferAllocateInfo = vk::CommandBufferAllocateInfo{}
            .setCommandPool( *commandPool )
            .setLevel( vk::CommandBufferLevel::ePrimary )
            .setCommandBufferCount( maxFramesInFlight );
        commandBuffers = logicalDevice->allocateCommandBuffers( commandBufferAllocateInfo );
    }

    auto recordCommandBuffer(
        const vk::CommandBuffer& commandBuffer,
        const vk::Pipeline& pipeline,
        const vk::RenderPass& renderPass,
        const vk::Framebuffer& frameBuffer,
        const vk::Extent2D& renderExtent,
        const vk::Buffer& vertexBuffer,
        std::uint32_t vertexCount
    ) -> void
    {
        const auto clearValues = std::array< vk::ClearValue, 2 >{
            vk::ClearValue{}.setColor( std::array< float, 4 >{ { 0.f, 0.f, .5f, 1.f } } ),
            vk::ClearValue{}.setDepthStencil( vk::ClearDepthStencilValue{ 1.f, 0 } )
        };

        const auto renderPassBeginInfo = vk::RenderPassBeginInfo{}
            .setRenderPass( renderPass )
            .setFramebuffer( frameBuffer )
            .setRenderArea( vk::Rect2D{ vk::Offset2D{ 0, 0 }, renderExtent } )
            .setClearValues( clearValues );

        commandBuffer.begin( vk::CommandBufferBeginInfo{} );

        commandBuffer.beginRenderPass( renderPassBeginInfo, vk::SubpassContents::eInline );

        commandBuffer.bindPipeline( vk::PipelineBindPoint::eGraphics, pipeline );

        const auto viewport = vk::Viewport{}
            .setX( 0.f )
            .setY( 0.f )
            .setWidth( static_cast< float >( renderExtent.width ) )
            .setHeight( static_cast< float >( renderExtent.height ) )
            .setMinDepth( 0.f )
            .setMaxDepth( 1.f );
        commandBuffer.setViewport( 0, viewport );

        const auto scissor = vk::Rect2D{ { 0, 0 }, renderExtent };
        commandBuffer.setScissor( 0, scissor );

        commandBuffer.bindVertexBuffers( 0, vertexBuffer, vk::DeviceSize{ 0 } );

        commandBuffer.draw( vertexCount, 1, 0, 0 );

        commandBuffer.endRenderPass();

        commandBuffer.end();
    }
}
