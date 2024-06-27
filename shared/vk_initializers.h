#pragma once

#include <vk_types.h>

namespace vkinit{
    VkCommandPoolCreateInfo command_pool_create_info(u32 queueFamilyIndex, VkCommandPoolCreateFlags flags=0);
    VkCommandBufferAllocateInfo command_buffer_allocate_info(VkCommandPool pool, u32 count = 1);

    VkCommandBufferBeginInfo command_buffer_begin_info(VkCommandBufferUsageFlags flags=0);
    VkCommandBufferSubmitInfo command_buffer_submit_info(VkCommandBuffer cmd);
}