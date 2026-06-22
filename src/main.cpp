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

#include "devices.hpp"
#include "glfw_utils.hpp"
#include "memory.hpp"
#include "pipelines.hpp"
#include "presentation.hpp"
#include "rendering.hpp"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace
{
    bool windowMinimized = false;
    bool framebufferSizeChanged = true;

    struct Vertex
    {
        glm::vec4 position;
        glm::vec4 color;
    };

    void onFramebufferSizeChanged( [[maybe_unused]] GLFWwindow* window, int width, int height )
    {
        windowMinimized = width == 0 && height == 0;
        framebufferSizeChanged = true;
    }
}

auto main() -> int
{
    constexpr int windowWidth = 800;
    constexpr int windowHeight = 600;
    constexpr std::uint32_t requestedSwapchainImageCount = 2u;
    constexpr std::uint32_t maxFramesInFlight = 2u;

    try
    {
        const auto glfw = vcpp::GlfwInstance{};
        const auto window = vcpp::createWindow( windowWidth, windowHeight, "Vulkan C++ Tutorial" );
        glfwSetFramebufferSizeCallback( window.get(), onFramebufferSizeChanged );

        const auto instance = vcpp::createVulkanInstance( vcpp::getRequiredExtensionsForGlfw() );
        const auto surface = vcpp::createSurface( *instance, *window );

        const auto physicalDevice = vcpp::selectPhysicalDevice( *instance );
        const auto logicalDevice = vcpp::createLogicalDevice(
            physicalDevice,
            vk::QueueFlagBits::eGraphics,
            *surface
        );

        const auto vertexShader = vcpp::createShaderModule( logicalDevice, "./shaders/vertex.vert.spv" );
        const auto fragmentShader = vcpp::createShaderModule( logicalDevice, "./shaders/fragment.frag.spv" );

        const auto surfaceFormats = physicalDevice.getSurfaceFormatsKHR( *surface );
        if ( surfaceFormats.empty() )
            throw std::runtime_error( "Surface does not support any formats" );
        const auto renderPass = vcpp::createRenderPass( logicalDevice, surfaceFormats[0].format );

        const auto pipelineLayout = vcpp::createGraphicsPipelineLayout( logicalDevice );

        const auto commandPool = logicalDevice->createCommandPoolUnique(
            vk::CommandPoolCreateInfo{}
                .setFlags( vk::CommandPoolCreateFlagBits::eResetCommandBuffer )
                .setQueueFamilyIndex( logicalDevice.queueFamilyIndex )
        );

        const auto commandBufferAllocateInfo = vk::CommandBufferAllocateInfo{}
            .setCommandPool( *commandPool )
            .setLevel( vk::CommandBufferLevel::ePrimary )
            .setCommandBufferCount( maxFramesInFlight );
        const auto commandBuffers = logicalDevice->allocateCommandBuffers( commandBufferAllocateInfo );

        const auto queue = logicalDevice->getQueue( logicalDevice.queueFamilyIndex, 0 );

        vk::UniquePipeline pipeline;
        std::unique_ptr< vcpp::Swapchain > swapchain;
        vk::Extent2D swapchainExtent;

        constexpr size_t floatsPerVertex = 8;
        constexpr auto vertexFormats = std::array< vk::Format, 2 >{
            vk::Format::eR32G32B32A32Sfloat,
            vk::Format::eR32G32B32A32Sfloat,
        };

        constexpr size_t vertexCount = 36;
        const std::array< Vertex, vertexCount > vertices = {
            // front (red)
            Vertex{ glm::vec4{ -.5f, -.5f,  .5f, 1.f }, glm::vec4{ 1.f, 0.f, 0.f, 1.f } },
            Vertex{ glm::vec4{  .5f, -.5f,  .5f, 1.f }, glm::vec4{ 1.f, 0.f, 0.f, 1.f } },
            Vertex{ glm::vec4{ -.5f,  .5f,  .5f, 1.f }, glm::vec4{ 1.f, 0.f, 0.f, 1.f } },
            Vertex{ glm::vec4{  .5f, -.5f,  .5f, 1.f }, glm::vec4{ 1.f, 0.f, 0.f, 1.f } },
            Vertex{ glm::vec4{  .5f,  .5f,  .5f, 1.f }, glm::vec4{ 1.f, 0.f, 0.f, 1.f } },
            Vertex{ glm::vec4{ -.5f,  .5f,  .5f, 1.f }, glm::vec4{ 1.f, 0.f, 0.f, 1.f } },

            // back (yellow)
            Vertex{ glm::vec4{ -.5f, -.5f, -.5f, 1.f }, glm::vec4{ 1.f, 1.f, 0.f, 1.f } },
            Vertex{ glm::vec4{  .5f, -.5f, -.5f, 1.f }, glm::vec4{ 1.f, 1.f, 0.f, 1.f } },
            Vertex{ glm::vec4{ -.5f,  .5f, -.5f, 1.f }, glm::vec4{ 1.f, 1.f, 0.f, 1.f } },
            Vertex{ glm::vec4{  .5f, -.5f, -.5f, 1.f }, glm::vec4{ 1.f, 1.f, 0.f, 1.f } },
            Vertex{ glm::vec4{  .5f,  .5f, -.5f, 1.f }, glm::vec4{ 1.f, 1.f, 0.f, 1.f } },
            Vertex{ glm::vec4{ -.5f,  .5f, -.5f, 1.f }, glm::vec4{ 1.f, 1.f, 0.f, 1.f } },

            // left (violet)
            Vertex{ glm::vec4{ -.5f, -.5f,  .5f, 1.f }, glm::vec4{ 1.f, 0.f, 1.f, 1.f } },
            Vertex{ glm::vec4{ -.5f, -.5f, -.5f, 1.f }, glm::vec4{ 1.f, 0.f, 1.f, 1.f } },
            Vertex{ glm::vec4{ -.5f,  .5f, -.5f, 1.f }, glm::vec4{ 1.f, 0.f, 1.f, 1.f } },
            Vertex{ glm::vec4{ -.5f, -.5f,  .5f, 1.f }, glm::vec4{ 1.f, 0.f, 1.f, 1.f } },
            Vertex{ glm::vec4{ -.5f,  .5f, -.5f, 1.f }, glm::vec4{ 1.f, 0.f, 1.f, 1.f } },
            Vertex{ glm::vec4{ -.5f,  .5f,  .5f, 1.f }, glm::vec4{ 1.f, 0.f, 1.f, 1.f } },

            // right (green)
            Vertex{ glm::vec4{  .5f, -.5f,  .5f, 1.f }, glm::vec4{ 0.f, 1.f, 0.f, 1.f } },
            Vertex{ glm::vec4{  .5f, -.5f, -.5f, 1.f }, glm::vec4{ 0.f, 1.f, 0.f, 1.f } },
            Vertex{ glm::vec4{  .5f,  .5f, -.5f, 1.f }, glm::vec4{ 0.f, 1.f, 0.f, 1.f } },
            Vertex{ glm::vec4{  .5f, -.5f,  .5f, 1.f }, glm::vec4{ 0.f, 1.f, 0.f, 1.f } },
            Vertex{ glm::vec4{  .5f,  .5f, -.5f, 1.f }, glm::vec4{ 0.f, 1.f, 0.f, 1.f } },
            Vertex{ glm::vec4{  .5f,  .5f,  .5f, 1.f }, glm::vec4{ 0.f, 1.f, 0.f, 1.f } },

            // top (turquoise)
            Vertex{ glm::vec4{ -.5f, -.5f,  .5f, 1.f }, glm::vec4{ 0.f, 1.f, 1.f, 1.f } },
            Vertex{ glm::vec4{  .5f, -.5f,  .5f, 1.f }, glm::vec4{ 0.f, 1.f, 1.f, 1.f } },
            Vertex{ glm::vec4{  .5f, -.5f, -.5f, 1.f }, glm::vec4{ 0.f, 1.f, 1.f, 1.f } },
            Vertex{ glm::vec4{ -.5f, -.5f,  .5f, 1.f }, glm::vec4{ 0.f, 1.f, 1.f, 1.f } },
            Vertex{ glm::vec4{  .5f, -.5f, -.5f, 1.f }, glm::vec4{ 0.f, 1.f, 1.f, 1.f } },
            Vertex{ glm::vec4{ -.5f, -.5f, -.5f, 1.f }, glm::vec4{ 0.f, 1.f, 1.f, 1.f } },

            // bottom (blue)
            Vertex{ glm::vec4{ -.5f,  .5f,  .5f, 1.f }, glm::vec4{ 0.f, 0.f, 1.f, 1.f } },
            Vertex{ glm::vec4{  .5f,  .5f,  .5f, 1.f }, glm::vec4{ 0.f, 0.f, 1.f, 1.f } },
            Vertex{ glm::vec4{  .5f,  .5f, -.5f, 1.f }, glm::vec4{ 0.f, 0.f, 1.f, 1.f } },
            Vertex{ glm::vec4{ -.5f,  .5f,  .5f, 1.f }, glm::vec4{ 0.f, 0.f, 1.f, 1.f } },
            Vertex{ glm::vec4{  .5f,  .5f, -.5f, 1.f }, glm::vec4{ 0.f, 0.f, 1.f, 1.f } },
            Vertex{ glm::vec4{ -.5f,  .5f, -.5f, 1.f }, glm::vec4{ 0.f, 0.f, 1.f, 1.f } },
        };

        const auto gpuVertexBuffer = vcpp::createGPUBuffer(
            physicalDevice,
            logicalDevice,
            sizeof( vertices ),
            vk::BufferUsageFlagBits::eVertexBuffer
        );

        auto model = glm::identity< glm::mat4 >();
        auto view = glm::identity< glm::mat4 >();
        auto projection = glm::identity< glm::mat4 >();
        float rotationAngle = 0.f;

        auto verticesTemp = vertices;

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

                pipeline = vcpp::createGraphicsPipeline(
                    logicalDevice,
                    *pipelineLayout,
                    *vertexShader,
                    *fragmentShader,
                    *renderPass,
                    swapchainExtent,
                    vertexFormats
                );

                swapchain = vcpp::createSwapchain(
                    logicalDevice,
                    *renderPass,
                    *surface,
                    surfaceFormats[0],
                    swapchainExtent,
                    maxFramesInFlight,
                    requestedSwapchainImageCount
                );

                view = glm::translate( glm::identity< glm::mat4 >(), glm::vec3{ 0.f, 0.f, -3.f } );

                projection = glm::perspective(
                    glm::radians( 30.0f ),
                    swapchainExtent.width / static_cast< float >( swapchainExtent.height ),
                    0.1f,
                    10.0f
                );

                framebufferSizeChanged = false;
            }

            model = glm::rotate( glm::identity< glm::mat4 >(), rotationAngle, glm::vec3{ 0.f, 1.f, 0.f } );

            for ( std::uint32_t i = 0; i < vertexCount; ++i )
            {
                verticesTemp[ i ].position = projection * view * model * vertices[ i ].position;
            }

            vcpp::copyDataToBuffer( *logicalDevice.device, verticesTemp, gpuVertexBuffer );
            rotationAngle += 0.01f;

            try
            {
                const auto frame = swapchain->getNextFrame();

                vcpp::recordCommandBuffer(
                    commandBuffers[ frame.frameInFlightIndex ],
                    *pipeline,
                    *renderPass,
                    frame.framebuffer,
                    swapchainExtent,
                    *gpuVertexBuffer.buffer,
                    vertexCount
                );

                const vk::PipelineStageFlags waitStages[] = {
                    vk::PipelineStageFlagBits::eColorAttachmentOutput };
                const auto submitInfo = vk::SubmitInfo{}
                    .setCommandBuffers( commandBuffers[ frame.frameInFlightIndex ] )
                    .setWaitSemaphores( frame.readyForRenderingSemaphore )
                    .setSignalSemaphores( frame.readyForPresentingSemaphore )
                    .setWaitDstStageMask( waitStages );

                queue.submit( submitInfo, frame.inFlightFence );

                const auto swapchains = std::array< vk::SwapchainKHR, 1 >{ *swapchain };
                const auto presentInfo = vk::PresentInfoKHR{}
                    .setSwapchains( swapchains )
                    .setImageIndices( frame.swapchainImageIndex )
                    .setWaitSemaphores( frame.readyForPresentingSemaphore );

                const auto result = queue.presentKHR( presentInfo );
                if ( result == vk::Result::eSuboptimalKHR )
                    framebufferSizeChanged = true;
            }
            catch ( const vk::OutOfDateKHRError& )
            {
                // the swapchain no longer matches the surface and needs to be recreated
                framebufferSizeChanged = true;
            }
        }

        logicalDevice->waitIdle();
    }
    catch( const std::exception& e )
    {
        std::cerr << "Exception thrown: " << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
