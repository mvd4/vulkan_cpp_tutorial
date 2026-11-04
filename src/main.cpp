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
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
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
        windowMinimized = width == 0 || height == 0;
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

        const auto queue = logicalDevice->getQueue( logicalDevice.queueFamilyIndex, 0 );

        constexpr auto vertexFormats = std::array< vk::Format, 2 >{
            vk::Format::eR32G32B32A32Sfloat,
            vk::Format::eR32G32B32A32Sfloat,
        };

        std::unique_ptr< vcpp::Swapchain > swapchain;

        constexpr size_t vertexCount = 36;
        const std::array< Vertex, vertexCount > vertices = {
            // front (red)
            Vertex{ { -.5f, -.5f,  .5f, 1.f }, { 1.f, 0.f, 0.f, 1.f } },
            Vertex{ {  .5f, -.5f,  .5f, 1.f }, { 1.f, 0.f, 0.f, 1.f } },
            Vertex{ { -.5f,  .5f,  .5f, 1.f }, { 1.f, 0.f, 0.f, 1.f } },
            Vertex{ {  .5f, -.5f,  .5f, 1.f }, { 1.f, 0.f, 0.f, 1.f } },
            Vertex{ {  .5f,  .5f,  .5f, 1.f }, { 1.f, 0.f, 0.f, 1.f } },
            Vertex{ { -.5f,  .5f,  .5f, 1.f }, { 1.f, 0.f, 0.f, 1.f } },

            // back (yellow)
            Vertex{ { -.5f, -.5f, -.5f, 1.f }, { 1.f, 1.f, 0.f, 1.f } },
            Vertex{ {  .5f, -.5f, -.5f, 1.f }, { 1.f, 1.f, 0.f, 1.f } },
            Vertex{ { -.5f,  .5f, -.5f, 1.f }, { 1.f, 1.f, 0.f, 1.f } },
            Vertex{ {  .5f, -.5f, -.5f, 1.f }, { 1.f, 1.f, 0.f, 1.f } },
            Vertex{ {  .5f,  .5f, -.5f, 1.f }, { 1.f, 1.f, 0.f, 1.f } },
            Vertex{ { -.5f,  .5f, -.5f, 1.f }, { 1.f, 1.f, 0.f, 1.f } },

            // left (violet)
            Vertex{ { -.5f, -.5f,  .5f, 1.f }, { 1.f, 0.f, 1.f, 1.f } },
            Vertex{ { -.5f, -.5f, -.5f, 1.f }, { 1.f, 0.f, 1.f, 1.f } },
            Vertex{ { -.5f,  .5f, -.5f, 1.f }, { 1.f, 0.f, 1.f, 1.f } },
            Vertex{ { -.5f, -.5f,  .5f, 1.f }, { 1.f, 0.f, 1.f, 1.f } },
            Vertex{ { -.5f,  .5f, -.5f, 1.f }, { 1.f, 0.f, 1.f, 1.f } },
            Vertex{ { -.5f,  .5f,  .5f, 1.f }, { 1.f, 0.f, 1.f, 1.f } },

            // right (green)
            Vertex{ {  .5f, -.5f,  .5f, 1.f }, { 0.f, 1.f, 0.f, 1.f } },
            Vertex{ {  .5f, -.5f, -.5f, 1.f }, { 0.f, 1.f, 0.f, 1.f } },
            Vertex{ {  .5f,  .5f, -.5f, 1.f }, { 0.f, 1.f, 0.f, 1.f } },
            Vertex{ {  .5f, -.5f,  .5f, 1.f }, { 0.f, 1.f, 0.f, 1.f } },
            Vertex{ {  .5f,  .5f, -.5f, 1.f }, { 0.f, 1.f, 0.f, 1.f } },
            Vertex{ {  .5f,  .5f,  .5f, 1.f }, { 0.f, 1.f, 0.f, 1.f } },

            // top (turquoise)
            Vertex{ { -.5f, -.5f,  .5f, 1.f }, { 0.f, 1.f, 1.f, 1.f } },
            Vertex{ {  .5f, -.5f,  .5f, 1.f }, { 0.f, 1.f, 1.f, 1.f } },
            Vertex{ {  .5f, -.5f, -.5f, 1.f }, { 0.f, 1.f, 1.f, 1.f } },
            Vertex{ { -.5f, -.5f,  .5f, 1.f }, { 0.f, 1.f, 1.f, 1.f } },
            Vertex{ {  .5f, -.5f, -.5f, 1.f }, { 0.f, 1.f, 1.f, 1.f } },
            Vertex{ { -.5f, -.5f, -.5f, 1.f }, { 0.f, 1.f, 1.f, 1.f } },

            // bottom (blue)
            Vertex{ { -.5f,  .5f,  .5f, 1.f }, { 0.f, 0.f, 1.f, 1.f } },
            Vertex{ {  .5f,  .5f,  .5f, 1.f }, { 0.f, 0.f, 1.f, 1.f } },
            Vertex{ {  .5f,  .5f, -.5f, 1.f }, { 0.f, 0.f, 1.f, 1.f } },
            Vertex{ { -.5f,  .5f,  .5f, 1.f }, { 0.f, 0.f, 1.f, 1.f } },
            Vertex{ {  .5f,  .5f, -.5f, 1.f }, { 0.f, 0.f, 1.f, 1.f } },
            Vertex{ { -.5f,  .5f, -.5f, 1.f }, { 0.f, 0.f, 1.f, 1.f } },
        };

        auto renderer = vcpp::Renderer{
            physicalDevice,
            logicalDevice,
            *vertexShader,
            *fragmentShader,
            surfaceFormats[0].format,
            vertexFormats,
            maxFramesInFlight
        };


        auto model = glm::identity< glm::mat4 >();
        auto view = glm::identity< glm::mat4 >();
        auto projection = glm::identity< glm::mat4 >();

        float rotationAngle = 0.f;
        constexpr float rotationSpeed = glm::radians( 45.f );
        auto lastFrameTime = std::chrono::steady_clock::now();

        auto verticesTemp = vertices;

        while ( !glfwWindowShouldClose( window.get() ) )
        {
            glfwPollEvents();

            if ( windowMinimized )
            {
                glfwWaitEvents();
                continue;
            }

            if ( framebufferSizeChanged )
            {
                logicalDevice->waitIdle();

                swapchain.reset();

                const auto capabilities = physicalDevice.getSurfaceCapabilitiesKHR( *surface );
                const auto swapchainExtent = capabilities.currentExtent;

                swapchain = vcpp::createSwapchain(
                    physicalDevice,
                    logicalDevice,
                    renderer.getRenderPass(),
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

            const auto now = std::chrono::steady_clock::now();
            const float deltaTime = std::min( std::chrono::duration< float >( now - lastFrameTime ).count(), 0.1f );
            lastFrameTime = now;
            rotationAngle = std::fmod( rotationAngle + deltaTime * rotationSpeed, glm::two_pi< float >() );

            model = glm::rotate( glm::identity< glm::mat4 >(), rotationAngle, glm::vec3{ 0.f, 1.f, 0.f } );
            const auto mvp = projection * view * model;

            try
            {
                const auto frame = swapchain->getNextFrame();

                renderer.drawFrame(
                    vcpp::SceneData{
                        std::as_bytes( std::span{ vertices } ),
                        vertexCount,
                        mvp
                    },
                    queue,
                    frame,
                    swapchain->getImageExtent()
                );

                const auto result = swapchain->present( queue, frame );
                if ( result != vk::Result::eSuccess )
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
