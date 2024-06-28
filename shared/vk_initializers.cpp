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

    VkSemaphoreCreateInfo semaphore_create_info(VkSemaphoreCreateFlags flags){
        VkSemaphoreCreateInfo info{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        info.flags = flags;
        return info;
    }

    VkSemaphoreSubmitInfo semaphore_submit_info(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore){
        VkSemaphoreSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
        submitInfo.semaphore = semaphore;
        submitInfo.stageMask = stageMask;
        submitInfo.value = 1;
        return submitInfo;
    }

    VkFenceCreateInfo vkinit::fence_create_info(VkFenceCreateFlags flags){
        VkFenceCreateInfo info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        info.flags = flags;
        return info;
    }

    VkSubmitInfo2 vkinit::submit_info(VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* signalSemaphoreInfo, VkSemaphoreSubmitInfo* waitSemaphoreInfo){
        VkSubmitInfo2 info{VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
        info.waitSemaphoreInfoCount = waitSemaphoreInfo == nullptr ? 0 : 1;
        info.pWaitSemaphoreInfos = waitSemaphoreInfo;

        info.signalSemaphoreInfoCount = signalSemaphoreInfo == nullptr ? 0 : 1;
        info.pSignalSemaphoreInfos = signalSemaphoreInfo;

        info.commandBufferInfoCount = 1;
        info.pCommandBufferInfos = cmd;

        return info;

    }

    VkPresentInfoKHR vkinit::present_info(){
        VkPresentInfoKHR info{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        return info;
    }

    VkRenderingAttachmentInfo vkinit::attachment_info(VkImageView view, VkClearValue * clear, VkImageLayout layout){
        VkRenderingAttachmentInfo colorAttachment{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
        colorAttachment.imageView = view;
        colorAttachment.imageLayout = layout;
        colorAttachment.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        if(clear){
            colorAttachment.clearValue = *clear;
        }
        return colorAttachment;
    }

    VkRenderingAttachmentInfo vkinit::depth_attachment_info(VkImageView view, VkImageLayout layout){
        VkRenderingAttachmentInfo depth_attachment_info{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
        depth_attachment_info.imageView = view;
        depth_attachment_info.imageLayout = layout;
        depth_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depth_attachment_info.clearValue.depthStencil.depth = 0.f;
        return depth_attachment_info;
    }

    VkImageSubresourceRange image_subresource_range(VkImageAspectFlags aspectMask){
        VkImageSubresourceRange subImage{};
        subImage.aspectMask = aspectMask;
        subImage.levelCount = VK_REMAINING_MIP_LEVELS;
        subImage.layerCount = VK_REMAINING_ARRAY_LAYERS;
        return subImage;
    }
}