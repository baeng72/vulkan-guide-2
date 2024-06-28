#pragma once
#include <vulkan/vulkan.h>

namespace vkutil{
    void transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);

    void copy_image_to_image(VkCommandBuffer cmd, VkImage src, VkImage dst, VkExtent2D srcSize, VkExtent2D dstSize);

    void generate_mipmap(VkCommandBuffer cmd, VkImage image, VkExtent2D imageSize);
}