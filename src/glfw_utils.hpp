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

#pragma once

#include <vulkan/vulkan.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <memory>
#include <vector>
#include <string>

namespace vcpp
{
    class glfw_instance
    {
    public:

        glfw_instance();
        ~glfw_instance();

        glfw_instance( const glfw_instance& ) = delete;
        glfw_instance( glfw_instance&& ) = delete;

        glfw_instance& operator= ( const glfw_instance& ) = delete;
        glfw_instance& operator= ( glfw_instance&& ) = delete;
    };

    using window_ptr_t = std::unique_ptr< GLFWwindow, decltype( &glfwDestroyWindow ) >;

    window_ptr_t create_window( int width, int height, const std::string& title );

    std::vector< std::string > get_required_extensions_for_glfw();

    vk::UniqueSurfaceKHR create_surface( 
        const vk::Instance& instance,
        GLFWwindow& window
    );
}
