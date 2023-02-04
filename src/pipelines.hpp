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

#include <filesystem>

namespace vcpp
{
    vk::UniqueShaderModule create_shader_module(
        const vk::Device& logicalDevice,
        const std::filesystem::path& path
    );

    vk::UniqueDescriptorSetLayout create_descriptor_set_layout( const vk::Device& logicalDevice );
 
    vk::UniquePipelineLayout create_pipeline_layout(
        const vk::Device& logicalDevice,
        const vk::DescriptorSetLayout& descriptorSetLayout
    );

    vk::UniquePipeline create_compute_pipeline(
        const vk::Device& logicalDevice,
        const vk::PipelineLayout& pipelineLayout,
        const vk::ShaderModule& computeShader
    );

    vk::UniqueDescriptorPool create_descriptor_pool( const vk::Device& logicalDevice );

    vk::UniqueRenderPass create_render_pass(
        const vk::Device& logicalDevice,
        const vk::Format& colorFormat
    );

    vk::UniquePipeline create_graphics_pipeline(
        const vk::Device& logicalDevice,
        const vk::ShaderModule& vertexShader,
        const vk::ShaderModule& fragmentShader,
        const vk::RenderPass& renderPass,
        const vk::Extent2D& viewportExtent,
        const std::vector< vk::Format >& vertexFormats
    );

}
