#pragma once
#include <vk_types.h>

namespace vkutil{
    bool load_shader_module(ccharp filePath, VkDevice device, VkShaderModule* outShaderModule);
    bool compile_shader_module(ccharp filePath, VkDevice device,VkShaderStageFlagBits stage, VkShaderModule* outShaderModule);
}