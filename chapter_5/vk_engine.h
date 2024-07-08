#pragma once
#include <vk_types.h>
#include <vk_initializers.h>
#include <vk_descriptors.h>
#include <vk_images.h>
#include <glfw/glfw3.h>
#include <vector>
#include <deque>
#include <functional>
#include "vk_loader.h"
#include <camera.h>

struct DeletionQueue{
    std::deque<std::function<void()>> deletors;
    void push_function(std::function<void()>&&function){
        deletors.push_back(function);
    }

    void flush(){
        //reverse iterate the deletion  to execute all the functions
        for(auto it = deletors.rbegin(); it != deletors.rend(); it++){
            (*it)();//call functors
        }
        deletors.clear();
    }
};

struct FrameData{
    VkSemaphore _swapchainSemaphore{VK_NULL_HANDLE};
    VkSemaphore _renderSemaphore{VK_NULL_HANDLE};
    VkFence _renderFence{VK_NULL_HANDLE};
    VkCommandPool _commandPool{VK_NULL_HANDLE};
    VkCommandBuffer _mainCommandBuffer{VK_NULL_HANDLE};
    DeletionQueue _deletionQueue;   
    DescriptorAllocatorGrowable _frameDescriptors; 
};

struct ComputePushConstants{
    vec4 data1;
    vec4 data2;
    vec4 data3;
    vec4 data4;
};



struct ComputeEffect{
    ccharp name{nullptr};
    VkPipeline pipeline{VK_NULL_HANDLE};
    VkPipelineLayout layout{VK_NULL_HANDLE};
    ComputePushConstants data;
};

struct RenderObject{
    u32 indexCount;
    u32 firstIndex;
    VkBuffer indexBuffer;
    MaterialInstance * material;
    Bounds bounds;
    mat4 transform;
    VkDeviceAddress vertexBufferAddress;
};

struct GLTFMetallic_Roughness{
    MaterialPipeline opaquePipeline;
    MaterialPipeline transparentPipeline;
    VkDescriptorSetLayout materialLayout;

    struct MaterialConstants{
        vec4 colorFactor;
        vec4 metalRoughnessFactor;
        //padding, we need it anyway for uniforms
        vec4 extra[14];
    };

    struct MaterialResources{
        AllocatedImage colorImage;
        VkSampler colorSampler;
        AllocatedImage metalRoughImage;
        VkSampler metalRoughSampler;
        VkBuffer dataBuffer;
        u32 dataBufferOffset;
    };

    DescriptorWriter writer;

    void build_pipelines(VulkanEngine*engine);
    void clear_resources(VkDevice device);
    MaterialInstance write_material(VkDevice device, MaterialPass pass, const MaterialResources&resources, DescriptorAllocatorGrowable& DescriptorAllocator);
};


struct DrawContext{
    std::vector<RenderObject> OpaqueSurfaces;
    std::vector<RenderObject> TransparentSurfaces;
};


struct MeshNode : public Node{
    std::shared_ptr<MeshAsset> mesh;
    virtual void Draw(const mat4 & topMatrix, DrawContext& ctx)override;
};

struct EngineStats{
    float frametime;
    int triangle_count;
    int drawcall_count;
    float scene_update_time;
    float mesh_draw_time;
};

constexpr unsigned int FRAME_OVERLAP = 2;//max frames?

class VulkanEngine{
    
    void centreWindow();
    void init_vulkan();
    void init_swapchain();
    void create_swapchain(u32 width, u32 height);
    void destroy_swapchain();
    void resize_swapchain();
    void init_commands();
    void init_sync_structures();
    void draw_background(VkCommandBuffer cmd);
    void init_descriptors();
    void init_pipelines();
    void init_background_pipelines();
    
    void init_imgui();
    void draw_imgui(VkCommandBuffer cmd, VkImageView targetImageView);
    void init_triangle_pipeline();
    void draw_geometry(VkCommandBuffer cmd);
    

    void init_mesh_pipeline();
    void init_default_data();
    public:

    bool _isInitialized{false};
    int _frameNumber{0};    
    bool stop_rendering{false};
    bool _isRunning{false};
    bool _resize_requested{false};
    VkExtent2D _windowExtent{1700, 900};
    
    
    
    GLFWwindow * pwindow{nullptr};
    //instance
    VkInstance _instance{VK_NULL_HANDLE};
    VkDebugUtilsMessengerEXT _debug_messenger;
    VkPhysicalDevice _physical{VK_NULL_HANDLE};
    VkDevice _device{VK_NULL_HANDLE};
    VkSurfaceKHR _surface{VK_NULL_HANDLE};

    //queues
    FrameData _frames[FRAME_OVERLAP];

    FrameData& get_current_frame(){return _frames[_frameNumber % FRAME_OVERLAP];}

    VkQueue _graphicsQueue{VK_NULL_HANDLE};
    u32 _graphicsQueueFamily{UINT32_MAX};

    VkSwapchainKHR _swapchain{VK_NULL_HANDLE};
    VkFormat _swapchainImageFormat{VK_FORMAT_UNDEFINED};

    std::vector<VkImage> _swapchainImages;
    std::vector<VkImageView> _swapchainImageViews;
    VkExtent2D _swapchainExtent;

    DeletionQueue _mainDeletionQueue;

    VmaAllocator _allocator{nullptr};

    //draw resources
    AllocatedImage _drawImage;
    AllocatedImage _depthImage;
    VkExtent2D _drawExtent;
    float renderScale = 1.f;

    //default images
    AllocatedImage _whiteImage;
    AllocatedImage _blackImage;
    AllocatedImage _greyImage;
    AllocatedImage _errorCheckerboardImage;

    VkSampler _defaultSamplerLinear;
    VkSampler _defaultSamplerNearest;

    DescriptorAllocatorGrowable globalDescriptorAllocator;

    VkDescriptorSet _drawImageDescriptors;
    VkDescriptorSetLayout _drawImageDescriptorLayout;

    VkDescriptorSetLayout _singleImageDescriptorLayout;

    VkPipeline _gradientPipeline;
    VkPipelineLayout _gradientPipelineLayout;

    VkPipelineLayout _trianglePipelineLayout;
    VkPipeline _trianglePipeline;

    VkPipelineLayout _meshPipelineLayout;
    VkPipeline _meshPipeline;

    GPUMeshBuffers rectangle;

    GPUSceneData sceneData;
    VkDescriptorSetLayout _gpuSceneDataDescriptorLayout;

    //immediate submit structures
    VkFence _immFence;
    VkCommandBuffer _immCommandBuffer;
    VkCommandPool _immCommandPool;

    std::vector<ComputeEffect> backgroundEffects;
    i32 currentBackgroundEffect{0};

    std::vector<std::shared_ptr<MeshAsset>> testMeshes;

    MaterialInstance defaultData;
    GLTFMetallic_Roughness metalRoughMaterial;

    DrawContext mainDrawContext;
    std::unordered_map<std::string, std::shared_ptr<Node>> loadedNodes;

    Camera mainCamera;
    float oldXPos{0.f};
    float oldYPos{0.f};

    std::unordered_map<std::string, std::shared_ptr<LoadedGLTF>> loadedScenes;

    EngineStats stats;

    void update_scene();

    AllocatedBuffer create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
    void destroy_buffer(const AllocatedBuffer& buffer);
    AllocatedImage create_image(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped=false);
    AllocatedImage create_image(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped=false);
    void destroy_image(const AllocatedImage& img);

    void immediate_submit(std::function<void(VkCommandBuffer cmd)>&&function);
    GPUMeshBuffers uploadMesh(std::span<u32> indices, std::span<Vertex> vertices);

    static VulkanEngine& Get();

    void init();

    void cleanup();    

    void draw();

    void run();

    VkShaderModule get_shader(ccharp shaderSrcPath, VkShaderStageFlagBits stage);
    
};