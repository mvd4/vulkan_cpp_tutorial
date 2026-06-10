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

#pragma once

#include <vulkan/vulkan.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace vcpp
{
    class GlfwInstance
    {
    public:

        GlfwInstance();
        ~GlfwInstance();

        GlfwInstance( const GlfwInstance& ) = delete;
        GlfwInstance( GlfwInstance&& ) = delete;

        GlfwInstance& operator= ( const GlfwInstance& ) = delete;
        GlfwInstance& operator= ( GlfwInstance&& ) = delete;
    };
}
