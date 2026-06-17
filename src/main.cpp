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

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>


auto main() -> int
{
    constexpr int windowWidth = 800;
    constexpr int windowHeight = 600;
    constexpr std::uint32_t requestedSwapchainImageCount = 2u;

    try
    {
        const auto glfw = vcpp::GlfwInstance{};
        const auto window = vcpp::createWindow( windowWidth, windowHeight, "Vulkan C++ Tutorial" );

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

        const auto swapchainExtent = vk::Extent2D{ windowWidth, windowHeight };

        const auto pipeline = vcpp::createGraphicsPipeline(
            logicalDevice,
            *pipelineLayout,
            *vertexShader,
            *fragmentShader,
            *renderPass,
            swapchainExtent
        );

        const auto swapchain = vcpp::createSwapchain(
            logicalDevice,
            *surface,
            surfaceFormats[0],
            swapchainExtent,
            requestedSwapchainImageCount
        );

        const auto imageViews = vcpp::createSwapchainImageViews(
            logicalDevice,
            *swapchain,
            surfaceFormats[0].format );

        const auto framebuffers = vcpp::createFramebuffers(
            logicalDevice,
            imageViews,
            swapchainExtent,
            *renderPass
        );

        const auto commandPool = logicalDevice->createCommandPoolUnique(
            vk::CommandPoolCreateInfo{}
                .setFlags( vk::CommandPoolCreateFlagBits::eResetCommandBuffer )
                .setQueueFamilyIndex( logicalDevice.queueFamilyIndex )
        );

        const auto commandBufferAllocateInfo = vk::CommandBufferAllocateInfo{}
            .setCommandPool( *commandPool )
            .setLevel( vk::CommandBufferLevel::ePrimary )
            .setCommandBufferCount( requestedSwapchainImageCount );
        const auto commandBuffers = logicalDevice->allocateCommandBuffers( commandBufferAllocateInfo );

        std::vector< vk::UniqueFence > inFlightFences;
        std::vector< vk::UniqueSemaphore > readyForRenderingSemaphores;
        std::vector< vk::UniqueSemaphore > readyForPresentingSemaphores;
        for( std::uint32_t i = 0; i < requestedSwapchainImageCount; ++i )
        {
            inFlightFences.push_back( logicalDevice.device->createFenceUnique(
                vk::FenceCreateInfo{}.setFlags( vk::FenceCreateFlagBits::eSignaled )
            ) );

            readyForRenderingSemaphores.push_back( logicalDevice.device->createSemaphoreUnique(
                vk::SemaphoreCreateInfo{}
            ) );

            readyForPresentingSemaphores.push_back( logicalDevice.device->createSemaphoreUnique(
                vk::SemaphoreCreateInfo{}
            ) );
        }
        const auto queue = logicalDevice->getQueue( logicalDevice.queueFamilyIndex, 0 );

        size_t frameInFlightIndex = 0;
        while ( !glfwWindowShouldClose( window.get() ) )
        {
            glfwPollEvents();

            auto result = logicalDevice.device->waitForFences(
                *inFlightFences[ frameInFlightIndex ],
                true,
                std::numeric_limits< std::uint64_t >::max()
            );
            logicalDevice.device->resetFences( *inFlightFences[ frameInFlightIndex ] );

            auto imageIndex = logicalDevice.device->acquireNextImageKHR(
                *swapchain,
                std::numeric_limits< std::uint64_t >::max(),
                *readyForRenderingSemaphores[ frameInFlightIndex ]
            ).value;

            vcpp::recordCommandBuffer(
                commandBuffers[ frameInFlightIndex ],
                *pipeline,
                *renderPass,
                *framebuffers[ imageIndex ],
                swapchainExtent
            );

            const vk::PipelineStageFlags waitStages[] = {
                vk::PipelineStageFlagBits::eColorAttachmentOutput };
            const auto submitInfo = vk::SubmitInfo{}
                .setCommandBuffers( commandBuffers[ frameInFlightIndex ] )
                .setWaitSemaphores( *readyForRenderingSemaphores[ frameInFlightIndex ] )
                .setSignalSemaphores( *readyForPresentingSemaphores[ frameInFlightIndex ] )
                .setPWaitDstStageMask( waitStages );

            queue.submit( submitInfo, *inFlightFences[ frameInFlightIndex ] );

            const auto presentInfo = vk::PresentInfoKHR{}
                .setSwapchains( *swapchain )
                .setImageIndices( imageIndex )
                .setWaitSemaphores( *readyForPresentingSemaphores[ frameInFlightIndex ] );

            result = queue.presentKHR( presentInfo );
            if ( result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR )
                throw std::runtime_error( "presenting failed" );

            frameInFlightIndex = ( frameInFlightIndex + 1 ) % requestedSwapchainImageCount;
        }

        logicalDevice.device->waitIdle();
    }
    catch( const std::exception& e )
    {
        std::cerr << "Exception thrown: " << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
