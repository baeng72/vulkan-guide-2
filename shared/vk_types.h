#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <span>
#include <array>
#include <functional>
#include <deque>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vk_mem_alloc.h>

#include <fmt/core.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/quaternion.hpp>


using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;
using mat4 = glm::mat4;
using quat = glm::quat;

using ccharp = const char *;

//we'll add our main reusable types here

struct AllocatedImage{
    VkImage image{VK_NULL_HANDLE};
    VkImageView imageView{VK_NULL_HANDLE};
    VmaAllocation allocation{VK_NULL_HANDLE};
    VmaAllocationInfo allocationInfo{};//might be useful?
    VkExtent2D imageExtent{};
    VkFormat imageFormat{VK_FORMAT_UNDEFINED};
};

struct AllocatedBuffer{
    VkBuffer buffer{VK_NULL_HANDLE};
    VmaAllocation allocation{VK_NULL_HANDLE};
    VmaAllocationInfo allocationInfo{};//has VkDeviceMemory I  think
};

struct GPUGLTFMaterial{
    vec4    colorFactors;
    vec4 metal_rough_factors;
    vec4 extra[14];
};

static_assert(sizeof(GPUGLTFMaterial) == 256);

struct GPUSceneData{
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    vec4 ambientColor;
    vec4 sunlightDirection;//(x,y,z) = dir, w = sun power
    vec4 sunlightColor;
};

//material types
enum class MaterialPass : u8{
    MainColor,
    Transparent,
    Other
};

struct MaterialPipeline{
    VkPipeline pipeline{VK_NULL_HANDLE};
    VkPipelineLayout layout{VK_NULL_HANDLE};
};

//vbuf types
struct Vertex{
    vec3 position;
    float uv_x;
    vec3 normal;
    float uv_y;
    vec4 color;
};

//holds the resources needed for a mesh
struct GPUMeshBuffers{
    AllocatedBuffer indexBuffer;
    AllocatedBuffer vertexBuffer;
    VkDeviceAddress vertexBufferAddress;
};

//push constants for our mesh object draws
struct GPUDrawPushConstants{
    mat4 worldMatrix;
    VkDeviceAddress vertexBuffer;
};

//node types
struct DrawContext;

//base class for a renderable dynamic object
class IRenderable{
    virtual void Draw(const mat4 & topMatrix, DrawContext & ctx) = 0;
};

//Implementation of a drawable scene node.
//the scene node can hold children andwill also keep a transform to propagate to them
struct Node : IRenderable{
    //parent pointer must be a weak pointer to avoid circular dependencies
    std::weak_ptr<Node> parent;
    std::vector<std::shared_ptr<Node>> children;

    mat4 localTransform;
    mat4 worldTransform;

    void refreshTransform(const mat4& parentMatrix){
        worldTransform = parentMatrix * localTransform;
        for(auto c : children){
            c->refreshTransform(worldTransform);
        }
    }

    virtual void Draw(const mat4& topMatrix, DrawContext&ctx){
        //draw children
        for(auto& c : children){
            c->Draw(topMatrix, ctx);
        }
    }
};

#define VK_CHECK(x)                                         \
    do {                                                    \
        VkResult res = x;                                   \
        if(err) {                                           \
            fmt::print("Detected Vulkan error: {}", string_VkResult(err));\
            abort();                                        \
        }                                                   \
    }while(0)