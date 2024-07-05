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

VkDescriptorPool DescriptorAllocatorGrowable::get_pool(VkDevice device){
    VkDescriptorPool newPool;
    if(readyPools.size()!=0){
        newPool = readyPools.back();
        readyPools.pop_back();
    }else{
        //need to create a new pool
        newPool = create_pool(device, setsPerPool, ratios);

        setsPerPool = (u32)(setsPerPool * 1.5f);
        if(setsPerPool > 4092){
            setsPerPool = 4092;
        }
    }
    return newPool;
}

VkDescriptorPool DescriptorAllocatorGrowable::create_pool(VkDevice device, u32 setCount, std::span<PoolSizeRatio> poolRatios){
    std::vector<VkDescriptorPoolSize> poolSizes;
    for(PoolSizeRatio ratio : poolRatios){
        poolSizes.push_back(VkDescriptorPoolSize{
            ratio.type,
            (u32)(ratio.ratio * setCount)
        });
    }

    VkDescriptorPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool_info.maxSets = setCount;
    pool_info.poolSizeCount = (u32)poolSizes.size();
    pool_info.pPoolSizes = poolSizes.data();

    VkDescriptorPool newPool;
    vkCreateDescriptorPool(device, &pool_info, nullptr, &newPool);
    return newPool;
}

void DescriptorAllocatorGrowable::init(VkDevice device, u32 maxSets, std::span<PoolSizeRatio> poolRatios){
    ratios.clear();

    for(auto r : poolRatios){
        ratios.push_back(r);
    }

    VkDescriptorPool newPool = create_pool(device, maxSets, poolRatios);

    setsPerPool = (u32)(maxSets * 1.5f);//grow it next allocation

    readyPools.push_back(newPool);
}

void DescriptorAllocatorGrowable::clear_pools(VkDevice device){
    for(auto p : readyPools){
        vkResetDescriptorPool(device, p, 0);
    }

    for(auto p : fullPools){
        vkResetDescriptorPool(device, p, 0);
        readyPools.push_back(p);
    }
    fullPools.clear();
}

void DescriptorAllocatorGrowable::destroy_pools(VkDevice device){
    for(auto p : readyPools){
        vkDestroyDescriptorPool(device, p, nullptr);
    }
    readyPools.clear();

    for(auto p : fullPools){
        vkDestroyDescriptorPool(device, p, nullptr);
    }
    fullPools.clear();
}

VkDescriptorSet DescriptorAllocatorGrowable::allocate(VkDevice device, VkDescriptorSetLayout layout, void * pNext){
    //get or create a pool to allocate from
    VkDescriptorPool poolToUse = get_pool(device);

    VkDescriptorSetAllocateInfo allocInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocInfo.descriptorPool = poolToUse;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    VkDescriptorSet ds;
    VkResult res = vkAllocateDescriptorSets(device, &allocInfo, &ds);

    //allocation failed. try again
    if(res == VK_ERROR_OUT_OF_POOL_MEMORY || res == VK_ERROR_FRAGMENTED_POOL){
        fullPools.push_back(poolToUse);

        poolToUse = get_pool(device);
        allocInfo.descriptorPool = poolToUse;

        VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &ds));
    }

    readyPools.push_back(poolToUse);
    return ds;
}

void DescriptorWriter::write_buffer(i32 binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type){
    VkDescriptorBufferInfo& info = bufferInfos.emplace_back(VkDescriptorBufferInfo{
        buffer,
        offset,
        size
    });

    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstBinding = binding;
    write.dstSet = VK_NULL_HANDLE;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pBufferInfo = &info;

    writes.push_back(write);
}

void DescriptorWriter::write_image(i32 binding, VkImageView view, VkSampler sampler, VkImageLayout layout, VkDescriptorType type){
    VkDescriptorImageInfo & info = imageInfos.emplace_back(VkDescriptorImageInfo{
        sampler,
        view,
        layout
    });

    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};

    write.dstBinding = binding;
    write.dstSet = VK_NULL_HANDLE;//left empty
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pImageInfo = &info;

    writes.push_back(write);
}

void DescriptorWriter::clear(){
    imageInfos.clear();
    writes.clear();
    bufferInfos.clear();
}

void DescriptorWriter::update_set(VkDevice device, VkDescriptorSet set){
    for(VkWriteDescriptorSet&write : writes){
        write.dstSet = set;
    }
    vkUpdateDescriptorSets(device, (u32)writes.size(), writes.data(), 0, nullptr);
}