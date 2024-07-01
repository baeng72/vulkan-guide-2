#pragma once

#include <vk_types.h>
#include <unordered_map>
#include <filesystem>

class VulkanEngine;

struct GeoSurface{
    u32 startIndex;
    u32 count;
};

struct MeshAsset{
    std::string name;

    std::vector<GeoSurface> surfaces;
    GPUMeshBuffers meshBuffers;
};

std::optional<std::vector<std::shared_ptr<MeshAsset>>> loadGltfMeshes(VulkanEngine* engine, std::filesystem::path filePath);