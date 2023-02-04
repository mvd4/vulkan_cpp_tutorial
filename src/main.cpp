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

#include "devices.hpp"
#include "glfw_utils.hpp"
#include "memory.hpp"
#include "pipelines.hpp"
#include "presentation.hpp"
#include "rendering.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

bool windowMinimized = false;
bool framebufferSizeChanged = true;
void on_framebuffer_size_changed( GLFWwindow* window, int width, int height )
{
    windowMinimized = width == 0 && height == 0;
    framebufferSizeChanged = true;
}

int main()
{
    constexpr int windowWidth = 800;
    constexpr int windowHeight = 600;
    constexpr uint32_t requestedSwapchainImageCount = 2u;

    try
    {
        const auto glfw = vcpp::glfw_instance{};
        const auto window = vcpp::create_window( windowWidth, windowHeight, "Vulkan C++ Tutorial" );
        glfwSetFramebufferSizeCallback( window.get(), on_framebuffer_size_changed );

        const auto instance = vcpp::create_instance( vcpp::get_required_extensions_for_glfw() );
        const auto surface = vcpp::create_surface( *instance, *window );

        const auto physicalDevice = vcpp::create_physical_device( *instance );
        const auto logicalDevice = vcpp::create_logical_device(
            physicalDevice,
            vk::QueueFlagBits::eGraphics,
            *surface );

        const auto vertexShader = create_shader_module( logicalDevice, "./shaders/vertex.spv" );
        const auto fragmentShader = create_shader_module( logicalDevice, "./shaders/fragment.spv" );

        const auto surfaceFormats = physicalDevice.getSurfaceFormatsKHR( *surface );
        const auto renderPass = vcpp::create_render_pass( logicalDevice, surfaceFormats[0].format );

        const auto commandPool = logicalDevice.device->createCommandPoolUnique(
            vk::CommandPoolCreateInfo{}
                .setFlags( vk::CommandPoolCreateFlagBits::eResetCommandBuffer )
                .setQueueFamilyIndex( logicalDevice.queueFamilyIndex )
        );
        const auto commandBufferAllocateInfo = vk::CommandBufferAllocateInfo{}
            .setCommandPool( *commandPool )
            .setLevel( vk::CommandBufferLevel::ePrimary )
            .setCommandBufferCount( requestedSwapchainImageCount );
        const auto commandBuffers = logicalDevice.device->allocateCommandBuffers( commandBufferAllocateInfo );

        const auto queue = logicalDevice.device->getQueue( logicalDevice.queueFamilyIndex, 0 );

        vk::UniquePipeline pipeline;
        vcpp::swapchain_ptr_t swapchain;
        vk::Extent2D swapchainExtent;

        constexpr size_t vertexCount = 3;
        const std::array< float, 4 * vertexCount > vertices = {
            0.0, -0.5, 0.0, 1.0,
            0.5, 0.5, 0.0, 1.0,
            -0.5, 0.5, 0.0, 1.0 };

        const auto gpuVertexBuffer = create_gpu_buffer(
            physicalDevice,
            logicalDevice,
            sizeof( vertices ),
            vk::BufferUsageFlagBits::eVertexBuffer
        );

        vcpp::copy_data_to_buffer( *logicalDevice.device, vertices, gpuVertexBuffer );

        while ( !glfwWindowShouldClose( window.get() ) )
        {
            glfwPollEvents();

            if ( windowMinimized )
                continue;

            if ( framebufferSizeChanged )
            {
                logicalDevice.device->waitIdle();

                pipeline.reset();
                swapchain.reset();

                const auto capabilities = physicalDevice.getSurfaceCapabilitiesKHR( *surface );
                swapchainExtent = capabilities.currentExtent;

                pipeline = create_graphics_pipeline(
                    logicalDevice,
                    *vertexShader,
                    *fragmentShader,
                    *renderPass,
                    swapchainExtent );

                swapchain = create_swapchain(
                    logicalDevice,
                    *renderPass,
                    *surface,
                    surfaceFormats[0],
                    swapchainExtent,
                    requestedSwapchainImageCount );

                framebufferSizeChanged = false;
            }

            const auto frame = swapchain->get_next_frame();

            vcpp::record_command_buffer(
                commandBuffers[ frame.inFlightIndex ],
                *pipeline,
                *renderPass,
                frame.framebuffer,
                swapchainExtent,
                *gpuVertexBuffer.buffer,
                vertexCount );

            const vk::PipelineStageFlags waitStages[] = {
                vk::PipelineStageFlagBits::eColorAttachmentOutput };
            const auto submitInfo = vk::SubmitInfo{}
                .setCommandBuffers( commandBuffers[ frame.inFlightIndex ] )
                .setWaitSemaphores( frame.readyForRenderingSemaphore )
                .setSignalSemaphores( frame.readyForPresentingSemaphore )
                .setPWaitDstStageMask( waitStages );
            queue.submit( submitInfo, frame.inFlightFence );

            const auto swapchains = std::vector< vk::SwapchainKHR >{ *swapchain };
            const auto presentInfo = vk::PresentInfoKHR{}
                .setSwapchains( swapchains )
                .setImageIndices( frame.swapchainImageIndex )
                .setWaitSemaphores( frame.readyForPresentingSemaphore );

            const auto result = queue.presentKHR( presentInfo );
            if ( result != vk::Result::eSuccess )
                throw std::runtime_error( "presenting failed" );
        }

        logicalDevice.device->waitIdle();
    }
    catch( const std::exception& e )
    {
        std::cout << "Exception thrown: " << e.what() << "\n";
        return -1;
    }

    return 0;
}
