#include <vk_images.h>
#include <vk_initializers.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace vkutil{


    void transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout){
        VkImageMemoryBarrier2 imageBarrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
        imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
        imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

        imageBarrier.oldLayout = currentLayout;
        imageBarrier.newLayout = newLayout;

        VkImageAspectFlags aspectMask = (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
        imageBarrier.subresourceRange = vkinit::image_subresource_range(aspectMask);
        imageBarrier.image = image;

        VkDependencyInfo depInfo{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
        depInfo.imageMemoryBarrierCount = 1;
        depInfo.pImageMemoryBarriers = &imageBarrier;

        vkCmdPipelineBarrier2(cmd, &depInfo);
    }

    void copy_image_to_image(VkCommandBuffer cmd, VkImage src, VkImage dst, VkExtent2D srcSize, VkExtent2D dstSize){
        VkImageBlit2 blitRegion{VK_STRUCTURE_TYPE_IMAGE_BLIT_2};
        blitRegion.srcOffsets[1].x = srcSize.width;
        blitRegion.srcOffsets[1].y = srcSize.height;
        blitRegion.srcOffsets[1].z = 1;

        blitRegion.dstOffsets[1].x = dstSize.width;
        blitRegion.dstOffsets[1].y = dstSize.height;
        blitRegion.dstOffsets[1].z = 1;

        blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blitRegion.srcSubresource.layerCount = 1;

        blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blitRegion.dstSubresource.layerCount = 1;

        VkBlitImageInfo2 blitInfo{VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2};
        blitInfo.dstImage = dst;
        blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        blitInfo.srcImage = src;
        blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        blitInfo.filter = VK_FILTER_LINEAR;
        blitInfo.regionCount = 1;
        blitInfo.pRegions = &blitRegion;

        vkCmdBlitImage2(cmd, &blitInfo);
    }

    void generate_mipmaps(VkCommandBuffer cmd, VkImage image, VkExtent2D imageSize){
        i32 mipLevels = i32(std::floor(std::log2(std::max(imageSize.width,imageSize.height))))+1;
        for(i32 mip = 0; mip < mipLevels; ++mip){
            VkExtent2D halfSize = imageSize;
            halfSize.width /= 2;
            halfSize.height /= 2;

            VkImageMemoryBarrier2 imageBarrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
            imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
            imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

            imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

            VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageBarrier.subresourceRange = vkinit::image_subresource_range(aspectMask);
            imageBarrier.subresourceRange.levelCount = 1;
            imageBarrier.subresourceRange.baseMipLevel = mip;

            VkDependencyInfo depInfo{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
            depInfo.imageMemoryBarrierCount = 1;
            depInfo.pImageMemoryBarriers = &imageBarrier;

            vkCmdPipelineBarrier2(cmd, &depInfo);

            if(mip < mipLevels-1){
                VkImageBlit2 blitRegion{VK_STRUCTURE_TYPE_IMAGE_BLIT_2};
                blitRegion.srcOffsets[1].x = imageSize.width;
                blitRegion.srcOffsets[1].y = imageSize.height;
                blitRegion.srcOffsets[1].z = 1;

                blitRegion.dstOffsets[1].x = halfSize.width;
                blitRegion.dstOffsets[1].y = halfSize.height;
                blitRegion.dstOffsets[1].z = 1;

                blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blitRegion.srcSubresource.layerCount = 1;
                blitRegion.srcSubresource.mipLevel = mip;

                blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blitRegion.dstSubresource.layerCount = 1;
                blitRegion.dstSubresource.mipLevel = mip + 1;

                VkBlitImageInfo2 blitInfo{VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2};
                blitInfo.dstImage = image;
                blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                blitInfo.srcImage = image;
                blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                blitInfo.filter = VK_FILTER_LINEAR;
                blitInfo.regionCount = 1;
                blitInfo.pRegions = &blitRegion;

                vkCmdBlitImage2(cmd, &blitInfo);
                imageSize = halfSize;
            }
        }
        transition_image(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
}