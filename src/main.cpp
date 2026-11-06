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

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <span>
#include <stdexcept>
#include <vector>

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

    auto sphereSurfacePoint( float radius, float phi, float theta ) -> glm::vec3
    {
        return radius * glm::vec3{
            std::sin( phi ) * std::cos( theta ),
            std::cos( phi ),
            std::sin( phi ) * std::sin( theta )
        };
    }

    auto addTriangle(
        std::vector< Vertex >& vertices,
        const glm::vec3& a,
        const glm::vec3& b,
        const glm::vec3& c,
        const glm::vec4& color
    ) -> void
    {
        vertices.push_back( Vertex{ glm::vec4{ a, 1.f }, color } );
        vertices.push_back( Vertex{ glm::vec4{ b, 1.f }, color } );
        vertices.push_back( Vertex{ glm::vec4{ c, 1.f }, color } );
    }

    auto createSphere(
        float radius,
        std::uint32_t numStacks,
        std::uint32_t numSlices,
        const glm::vec4& color
    ) -> std::vector< Vertex >
    {
        assert( numStacks >= 2 && numSlices >= 3 );

        auto vertices = std::vector< Vertex >{};
        vertices.reserve( 6u * numStacks * numSlices );

        const auto angleOf = []( std::uint32_t step, std::uint32_t numSteps, float fullAngle ) -> float
        {
            return fullAngle * static_cast< float >( step ) / static_cast< float >( numSteps );
        };

        // sin( π ) and sin( 2π ) aren't exactly 0 in floating point, so we set the bottom pole and
        // the closing angle of the last slice explicitly to make neighboring triangles share
        // bit-identical vertices.
        const auto bottomPole = glm::vec3{ 0.f, -radius, 0.f };

        for ( std::uint32_t stack = 0; stack < numStacks; ++stack )
        {
            const auto isFirstStack = stack == 0;
            const auto isLastStack = stack + 1 == numStacks;

            const auto phiTop = angleOf( stack, numStacks, glm::pi< float >() );
            const auto phiBottom = angleOf( stack + 1, numStacks, glm::pi< float >() );

            for ( std::uint32_t slice = 0; slice < numSlices; ++slice )
            {
                const auto isLastSlice = slice + 1 == numSlices;

                const auto thetaLeft = angleOf( slice, numSlices, glm::two_pi< float >() );
                const auto thetaRight = isLastSlice ? 0.f : angleOf( slice + 1, numSlices, glm::two_pi< float >() );

                const auto topLeft = sphereSurfacePoint( radius, phiTop, thetaLeft );
                const auto topRight = sphereSurfacePoint( radius, phiTop, thetaRight );
                const auto bottomLeft = isLastStack ? bottomPole : sphereSurfacePoint( radius, phiBottom, thetaLeft );
                const auto bottomRight = isLastStack ? bottomPole : sphereSurfacePoint( radius, phiBottom, thetaRight );

                // At the poles the whole ring collapses into a single point, so one of the two
                // triangles of those quads would have no area at all.
                if ( !isFirstStack )
                    addTriangle( vertices, topLeft, topRight, bottomLeft, color );

                if ( !isLastStack )
                    addTriangle( vertices, topRight, bottomRight, bottomLeft, color );
            }
        }

        return vertices;
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

        constexpr auto sphereRadius = .7f;
        constexpr std::uint32_t numSphereStacks = 24u;
        constexpr std::uint32_t numSphereSlices = 24u;
        constexpr auto sphereColor = glm::vec4{ 0.5f, 0.5f, 0.5f, 1.f };

        const auto vertices = createSphere( sphereRadius, numSphereStacks, numSphereSlices, sphereColor );
        const auto vertexCount = static_cast< std::uint32_t >( vertices.size() );

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

        float ambientColorPhase = 0.f;
        constexpr float twoThirdsPi = glm::radians( 120.f );

        auto lastFrameTime = std::chrono::steady_clock::now();

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

            ambientColorPhase = std::fmod( ambientColorPhase + deltaTime, glm::two_pi< float >() );
            const auto ambientColor = glm::vec4{
                0.5f + 0.5f * std::sin( ambientColorPhase ),
                0.5f + 0.5f * std::sin( ambientColorPhase + twoThirdsPi ),
                0.5f + 0.5f * std::sin( ambientColorPhase + 2.f * twoThirdsPi ),
                1.f
            };

            try
            {
                const auto frame = swapchain->getNextFrame();

                renderer.drawFrame(
                    vcpp::SceneData{
                        .vertexData = std::as_bytes( std::span{ vertices } ),
                        .vertexCount = vertexCount,
                        .vertexDataVersion = 0,
                        .mvpMatrix = mvp,
                        .ambientColor = ambientColor
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
