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

class DescriptorAllocatorGrowable{
public:
    struct PoolSizeRatio{
        VkDescriptorType type;
        float ratio;
    };
private:
    std::vector<PoolSizeRatio> ratios;
    std::vector<VkDescriptorPool> fullPools;
    std::vector<VkDescriptorPool> readyPools;
    u32 setsPerPool;

    VkDescriptorPool get_pool(VkDevice device);
    VkDescriptorPool create_pool(VkDevice device, u32 setCount, std::span<PoolSizeRatio> poolRatios);

public:

    

    void init(VkDevice device, u32 initialSets, std::span<PoolSizeRatio> poolRatios);
    void clear_pools(VkDevice device);
    void destroy_pools(VkDevice device);

    VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout, void*pNext=nullptr);
};

struct DescriptorWriter{
    std::deque<VkDescriptorImageInfo> imageInfos;
    std::deque<VkDescriptorBufferInfo> bufferInfos;
    std::vector<VkWriteDescriptorSet> writes;

    void write_image(i32 binding, VkImageView image, VkSampler sampler, VkImageLayout layout, VkDescriptorType type);
    void write_buffer(i32 binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type);
    void clear();
    void update_set(VkDevice device, VkDescriptorSet set);

};  