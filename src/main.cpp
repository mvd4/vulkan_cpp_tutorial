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

#include <filesystem>
#include <fstream>
#include <iostream>



int main()
{
    try
    {
        const auto glfw = vcpp::glfw_instance{};
        const auto window = vcpp::create_window( 800, 600, "Vulkan C++ Tutorial" );

        const auto instance = vcpp::create_instance( vcpp::get_required_extensions_for_glfw() );
        const auto surface = vcpp::create_surface( *instance, *window );

        const auto physicalDevice = vcpp::create_physical_device( *instance );
        const auto logicalDevice = vcpp::create_logical_device(
            physicalDevice,
            vk::QueueFlagBits::eGraphics,
            *surface );


        while ( !glfwWindowShouldClose( window.get() ) )
        {
            glfwPollEvents();
        }
    }
    catch( const std::exception& e )
    {
        std::cout << "Exception thrown: " << e.what() << "\n";
        return -1;
    }
    return 0;
}
