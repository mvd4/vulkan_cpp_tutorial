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

#include "rendering.hpp"


namespace vcpp
{
    void record_command_buffer(
        const vk::CommandBuffer& commandBuffer,
        const vk::Pipeline& pipeline,
        const vk::RenderPass& renderPass,
        const vk::Framebuffer& frameBuffer,
        const vk::Extent2D& renderExtent,
        const vk::Buffer& vertexBuffer,
        const std::uint32_t vertexCount
    )
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
        commandBuffer.bindPipeline( vk::PipelineBindPoint::eGraphics, pipeline );

        commandBuffer.beginRenderPass( renderPassBeginInfo, vk::SubpassContents::eInline );

        commandBuffer.bindVertexBuffers( 0, vertexBuffer, { 0 } );

        commandBuffer.draw( vertexCount, 1, 0, 0 );
        commandBuffer.endRenderPass();

        commandBuffer.end();
    }

}