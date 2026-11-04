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
#include <cassert>


namespace vcpp
{
    Renderer::Renderer(
        const vk::PhysicalDevice& physicalDevice,
        const LogicalDevice& logicalDevice,
        const vk::ShaderModule& vertexShader,
        const vk::ShaderModule& fragmentShader,
        vk::Format colorFormat,
        std::span< const vk::Format > vertexFormats,
        std::uint32_t maxFramesInFlight
    )
        : m_physicalDevice{ physicalDevice }
        , m_logicalDevice{ logicalDevice }
        , m_renderPass{ createRenderPass( logicalDevice, colorFormat ) }
        , m_pipelineLayout{ createGraphicsPipelineLayout( logicalDevice ) }
        , m_pipeline{ createGraphicsPipeline(
            logicalDevice,
            *m_pipelineLayout,
            vertexShader,
            fragmentShader,
            *m_renderPass,
            vertexFormats
        ) }
        , m_commandPool{ logicalDevice->createCommandPoolUnique(
            vk::CommandPoolCreateInfo{}
                .setFlags( vk::CommandPoolCreateFlagBits::eResetCommandBuffer )
                .setQueueFamilyIndex( logicalDevice.queueFamilyIndex )
        ) }
        , m_vertexBuffers( maxFramesInFlight )
        , m_vertexDataVersions( maxFramesInFlight, 0 )
    {
        const auto commandBufferAllocateInfo = vk::CommandBufferAllocateInfo{}
            .setCommandPool( *m_commandPool )
            .setLevel( vk::CommandBufferLevel::ePrimary )
            .setCommandBufferCount( maxFramesInFlight );
        m_commandBuffers = logicalDevice->allocateCommandBuffers( commandBufferAllocateInfo );
    }

    auto Renderer::drawFrame(
        const SceneData& sceneData,
        const vk::Queue& queue,
        const Swapchain::FrameData& frame,
        const vk::Extent2D& imageExtent
    ) -> void
    {
        assert( m_vertexBuffers.size() > frame.frameInFlightIndex );
        assert( !sceneData.vertexData.empty() );

        auto& gpuVertexBuffer = m_vertexBuffers[ frame.frameInFlightIndex ];
        bool needsUpload = m_vertexDataVersions[ frame.frameInFlightIndex ] != sceneData.vertexDataVersion;

        if ( gpuVertexBuffer.size < sceneData.vertexData.size() )
        {
            gpuVertexBuffer = createGPUBuffer(
                m_physicalDevice,
                m_logicalDevice,
                sceneData.vertexData.size(),
                vk::BufferUsageFlagBits::eVertexBuffer
            );
            needsUpload = true;
        }

        if ( needsUpload )
        {
            copyDataToBuffer( m_logicalDevice, sceneData.vertexData, gpuVertexBuffer );
            m_vertexDataVersions[ frame.frameInFlightIndex ] = sceneData.vertexDataVersion;
        }

        recordCommandBuffer(
            m_commandBuffers[ frame.frameInFlightIndex ],
            frame.framebuffer,
            imageExtent,
            *gpuVertexBuffer.buffer,
            sceneData
        );

        const vk::PipelineStageFlags waitStages[] = {
            vk::PipelineStageFlagBits::eColorAttachmentOutput };
        const auto submitInfo = vk::SubmitInfo{}
            .setCommandBuffers( m_commandBuffers[ frame.frameInFlightIndex ] )
            .setWaitSemaphores( frame.readyForRenderingSemaphore )
            .setSignalSemaphores( frame.readyForPresentingSemaphore )
            .setWaitDstStageMask( waitStages );

        queue.submit( submitInfo, frame.inFlightFence );
    }

    auto Renderer::recordCommandBuffer(
        const vk::CommandBuffer& commandBuffer,
        const vk::Framebuffer& frameBuffer,
        const vk::Extent2D& renderExtent,
        const vk::Buffer& vertexBuffer,
        const SceneData& sceneData
    ) -> void
    {
        const auto clearValues = std::array< vk::ClearValue, 2 >{
            vk::ClearValue{}.setColor( std::array< float, 4 >{ { 0.f, 0.f, .5f, 1.f } } ),
            vk::ClearValue{}.setDepthStencil( vk::ClearDepthStencilValue{ 1.f, 0 } )
        };

        const auto renderPassBeginInfo = vk::RenderPassBeginInfo{}
            .setRenderPass( *m_renderPass )
            .setFramebuffer( frameBuffer )
            .setRenderArea( vk::Rect2D{ vk::Offset2D{ 0, 0 }, renderExtent } )
            .setClearValues( clearValues );

        commandBuffer.begin( vk::CommandBufferBeginInfo{} );

        commandBuffer.beginRenderPass( renderPassBeginInfo, vk::SubpassContents::eInline );

        commandBuffer.bindPipeline( vk::PipelineBindPoint::eGraphics, *m_pipeline );

        commandBuffer.pushConstants< glm::mat4 >(
            *m_pipelineLayout,
            vk::ShaderStageFlagBits::eVertex,
            0,
            sceneData.mvpMatrix
        );

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

        commandBuffer.draw( sceneData.vertexCount, 1, 0, 0 );

        commandBuffer.endRenderPass();

        commandBuffer.end();
    }
}
