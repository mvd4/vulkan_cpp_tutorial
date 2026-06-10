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

#include "glfw_utils.hpp"

#include <format>


namespace vcpp
{
    GlfwInstance::GlfwInstance()
    {
        if ( auto result = glfwInit(); result != GLFW_TRUE )
            throw std::runtime_error( std::format( "Could not init glfw. Error {}", result ) );
    }

    GlfwInstance::~GlfwInstance() { glfwTerminate(); }

    auto createWindow( int width, int height, const std::string& title ) -> WindowPtr
    {
        glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
        return WindowPtr{
            glfwCreateWindow( width, height, title.c_str(), nullptr, nullptr ),
            glfwDestroyWindow
        };
    }

    auto getRequiredExtensionsForGlfw() -> std::vector< std::string >
    {
        std::vector< std::string > result;
        std::uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );
        for ( std::uint32_t i = 0; i < glfwExtensionCount; ++i )
            result.push_back( glfwExtensions[i] );
        return result;
    }

    auto createSurface(
        const vk::Instance& instance,
        GLFWwindow& window
    ) -> vk::UniqueSurfaceKHR
    {
        VkSurfaceKHR surface;
        if (
            const auto result = glfwCreateWindowSurface( instance, &window, nullptr, &surface );
            result != VK_SUCCESS
        )
        {
            throw std::runtime_error( std::format( "failed to create window surface. Error: {}", static_cast< int >( result ) ) );
        }

        return vk::UniqueSurfaceKHR{ vk::SurfaceKHR( surface ), instance };
    }
}
