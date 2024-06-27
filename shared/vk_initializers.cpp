#include <vk_initializers.h>

namespace vkinit{

    VkCommandPoolCreateInfo command_pool_create_info(u32 queueFamilyIndex, VkCommandPoolCreateFlags flags){
        VkCommandPoolCreateInfo info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        info.queueFamilyIndex = queueFamilyIndex;
        info.flags = flags;
        return info;
    }

    VkCommandBufferAllocateInfo command_buffer_allocate_info(VkCommandPool pool, u32 count){
        VkCommandBufferAllocateInfo info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        info.commandPool = pool;
        info.commandBufferCount = count;
        info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        return info;
    }

    VkCommandBufferBeginInfo command_buffer_begin_info(VkCommandBufferUsageFlags flags){
        VkCommandBufferBeginInfo info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        info.flags = flags;
        return info;
    }

    VkCommandBufferSubmitInfo command_buffer_submit_info(VkCommandBuffer cmd){
        VkCommandBufferSubmitInfo info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
        info.commandBuffer = cmd;
        return info;
    }

    
}