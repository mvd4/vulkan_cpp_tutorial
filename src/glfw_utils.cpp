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

#include "glfw_utils.hpp"

#include <fmt/format.h>

namespace vcpp
{
    glfw_instance::glfw_instance()
    {
        if ( auto result = glfwInit(); result != GLFW_TRUE )
            throw std::runtime_error( fmt::format( "Could not init glfw. Error {}", result ) );
    }

    glfw_instance::~glfw_instance() { glfwTerminate(); }


    window_ptr_t create_window( int width, int height, const std::string& title )
    {
        glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
        return window_ptr_t{ 
            glfwCreateWindow( width, height, title.c_str(), nullptr, nullptr ),
            glfwDestroyWindow
        };
    }

    std::vector< std::string > get_required_extensions_for_glfw()
    {
        std::vector< std::string > result;
        std::uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );
        for( std::uint32_t i = 0; i < glfwExtensionCount; ++i )
            result.push_back( glfwExtensions[i] );
        return result;
    }

    vk::UniqueSurfaceKHR create_surface( 
        const vk::Instance& instance,
        GLFWwindow& window
    )
    {
        VkSurfaceKHR surface;
        if ( 
            const auto result = glfwCreateWindowSurface( instance, &window, nullptr, &surface );
            result != VK_SUCCESS 
        ) 
        {
            throw std::runtime_error( fmt::format( "failed to create window surface. Error: {}", result ) );
        }

        vk::ObjectDestroy< vk::Instance, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE > deleter{ instance };
        return vk::UniqueSurfaceKHR{ vk::SurfaceKHR( surface ), deleter };    
    }
}
