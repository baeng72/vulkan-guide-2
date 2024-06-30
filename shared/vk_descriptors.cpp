#include <vk_descriptors.h>

void DescriptorLayoutBuilder::add_binding(u32 binding, VkDescriptorType type){
    VkDescriptorSetLayoutBinding newbind{};
    newbind.binding = binding;
    newbind.descriptorType = type;
    newbind.descriptorCount = 1;
    bindings.push_back(newbind);
}

void DescriptorLayoutBuilder::clear(){
    bindings.clear();
}

VkDescriptorSetLayout DescriptorLayoutBuilder::build(VkDevice device, VkShaderStageFlags shaderStages, void* pNext, VkDescriptorSetLayoutCreateFlagBits flags){
    for(auto & b: bindings){
        b.stageFlags |= shaderStages;
    }

    VkDescriptorSetLayoutCreateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    info.pBindings = bindings.data();
    info.bindingCount = (u32)bindings.size();
    info.flags = flags;

    VkDescriptorSetLayout set;
    VK_CHECK(vkCreateDescriptorSetLayout(device, &info, nullptr, &set));
    return set;
}

void DescriptorAllocator::init_pool(VkDevice device, u32 maxSets, std::span<PoolSizeRatio> poolRatios){
    std::vector<VkDescriptorPoolSize> poolSizes;
    for(auto ratio : poolRatios){
        poolSizes.push_back(VkDescriptorPoolSize{ratio.type, (u32)(ratio.ratio * maxSets)});
    }

    VkDescriptorPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool_info.maxSets = maxSets;
    pool_info.poolSizeCount = (u32)poolSizes.size();
    pool_info.pPoolSizes = poolSizes.data();

    vkCreateDescriptorPool(device, &pool_info, nullptr, &pool);    
}

void DescriptorAllocator::clear_descriptors(VkDevice device){
    vkResetDescriptorPool(device, pool, 0);
}

void DescriptorAllocator::destroy_pool(VkDevice device){
    vkDestroyDescriptorPool(device, pool, nullptr);
}

VkDescriptorSet DescriptorAllocator::allocate(VkDevice device, VkDescriptorSetLayout layout){
    VkDescriptorSetAllocateInfo allocInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;
    VkDescriptorSet set;
    VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &set));

    return set;
}