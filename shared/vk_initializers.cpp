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

    VkRenderingInfo rendering_info(VkExtent2D renderExtent, VkRenderingAttachmentInfo* colorAttachment, VkRenderingAttachmentInfo* depthAttachment){
        VkRenderingInfo renderInfo{VK_STRUCTURE_TYPE_RENDERING_INFO};
        renderInfo.renderArea = VkRect2D({VkOffset2D{0, 0}, renderExtent});
        renderInfo.layerCount = 1;
        renderInfo.colorAttachmentCount = 1;
        renderInfo.pColorAttachments = colorAttachment;
        renderInfo.pDepthAttachment = depthAttachment;
        return renderInfo;
    }

    VkImageSubresourceRange image_subresource_range(VkImageAspectFlags aspectMask){
        VkImageSubresourceRange subImage{};
        subImage.aspectMask = aspectMask;
        subImage.levelCount = VK_REMAINING_MIP_LEVELS;
        subImage.layerCount = VK_REMAINING_ARRAY_LAYERS;
        return subImage;
    }

    VkImageCreateInfo image_create_info(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent){
        VkImageCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        info.imageType = VK_IMAGE_TYPE_2D;
        info.format = format;
        info.extent = extent;
        info.mipLevels = 1;
        info.arrayLayers = 1;

        //for MSAA, we will not be using it by default, so default it to 1 sampler per pixel.
        info.samples = VK_SAMPLE_COUNT_1_BIT;

        //optimal tiling, which means the image is stored on the best gpu format
        info.tiling = VK_IMAGE_TILING_OPTIMAL;
        info.usage = usageFlags; 

        return info;
    }

    VkImageViewCreateInfo imageview_create_info(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags){
        //build a image-view for the depth image to use for rendering
        VkImageViewCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        info.image = image;
        info.format = format;
        info.subresourceRange = {aspectFlags, 0,1,0,1};
        return info;
    }

    VkPipelineLayoutCreateInfo pipeline_layout_create_info(){
        VkPipelineLayoutCreateInfo info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        //empty default
        return info;
    }

    VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info(VkShaderStageFlagBits stage, VkShaderModule shaderModule, ccharp entry){
        VkPipelineShaderStageCreateInfo info{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
        //shader stage
        info.stage = stage;
        //module containing the code for this shader stage
        info.module = shaderModule;
        //the entry point of the shader
        info.pName = entry;
        return info;
    }
}