#pragma once
#include <vk_types.h>
#include <vulkan/vulkan.h>

struct DescriptorLayoutBuilder{
    std::vector<VkDescriptorSetLayoutBinding> bindings;

    void add_binding(u32 binding, VkDescriptorType type);
    void clear();
    VkDescriptorSetLayout build(VkDevice device, VkShaderStageFlags shaderStages, void*pNext = nullptr, VkDescriptorSetLayoutCreateFlagBits flags=(VkDescriptorSetLayoutCreateFlagBits)0);
};

struct DescriptorAllocator{
    struct PoolSizeRatio{
        VkDescriptorType type;
        float ratio;
    };
    VkDescriptorPool pool;
    void init_pool(VkDevice device, u32 maxSets, std::span<PoolSizeRatio> poolRatios);
    void clear_descriptors(VkDevice device);
    void destroy_pool(VkDevice device);

    VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout);
};