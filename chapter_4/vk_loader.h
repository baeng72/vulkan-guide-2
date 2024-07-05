#pragma once

#include <vk_types.h>
#include <unordered_map>
#include <filesystem>

class VulkanEngine;

struct GLTFMaterial{
    MaterialInstance data;
};

struct GeoSurface{
    u32 startIndex;
    u32 count;
    std::shared_ptr<GLTFMaterial> material;
};

struct MeshAsset{
    std::string name;

    std::vector<GeoSurface> surfaces;
    GPUMeshBuffers meshBuffers;
};

std::optional<std::vector<std::shared_ptr<MeshAsset>>> loadGltfMeshes(VulkanEngine* engine, std::filesystem::path filePath);