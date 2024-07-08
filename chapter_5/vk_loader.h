#pragma once

#include <vk_types.h>
#include <vk_descriptors.h>
#include <unordered_map>
#include <filesystem>

class VulkanEngine;

struct GLTFMaterial{
    MaterialInstance data;
};

struct Bounds{
    vec3 origin;
    float sphereRadius;
    vec3 extents;
};


struct GeoSurface{
    u32 startIndex;
    u32 count;
    Bounds bounds;
    std::shared_ptr<GLTFMaterial> material;
};

struct MeshAsset{
    std::string name;

    std::vector<GeoSurface> surfaces;
    GPUMeshBuffers meshBuffers;
};




std::optional<std::vector<std::shared_ptr<MeshAsset>>> loadGltfMeshes(VulkanEngine* engine, std::filesystem::path filePath);




struct LoadedGLTF : public IRenderable{
    private:
    void clearAll();
    public:
    //storage for all the data on a given glTF file
    std::unordered_map<std::string, std::shared_ptr<MeshAsset>> meshes;
    std::unordered_map<std::string, std::shared_ptr<Node>> nodes;
    std::unordered_map<std::string, AllocatedImage> images;
    std::unordered_map<std::string, std::shared_ptr<GLTFMaterial>> materials;

    //nodes that don't have a parent, for iterating through the file in tree order
    std::vector<std::shared_ptr<Node>> topNodes;

    std::vector<VkSampler> samplers;

    DescriptorAllocatorGrowable descriptorPool;

    AllocatedBuffer materialDataBuffer;

    VulkanEngine* creator;

    ~LoadedGLTF(){ clearAll(); }

    virtual void Draw(const mat4& topMatrix, DrawContext& ctx);
};

std::optional<std::shared_ptr<LoadedGLTF>> loadGltf(VulkanEngine*engine, std::string_view filePath);