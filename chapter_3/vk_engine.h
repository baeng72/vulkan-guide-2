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
    VkShaderModule get_shader(ccharp shaderSrcPath, VkShaderStageFlagBits stage);
    void init_imgui();
    void draw_imgui(VkCommandBuffer cmd, VkImageView targetImageView);
    void init_triangle_pipeline();
    void draw_geometry(VkCommandBuffer cmd);
    AllocatedBuffer create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
    void destroy_buffer(const AllocatedBuffer& buffer);
    
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

    DescriptorAllocator globalDescriptorAllocator;

    VkDescriptorSet _drawImageDescriptors;
    VkDescriptorSetLayout _drawImageDescriptorLayout;

    VkPipeline _gradientPipeline;
    VkPipelineLayout _gradientPipelineLayout;

    VkPipelineLayout _trianglePipelineLayout;
    VkPipeline _trianglePipeline;

    VkPipelineLayout _meshPipelineLayout;
    VkPipeline _meshPipeline;

    GPUMeshBuffers rectangle;



    //immediate submit structures
    VkFence _immFence;
    VkCommandBuffer _immCommandBuffer;
    VkCommandPool _immCommandPool;

    std::vector<ComputeEffect> backgroundEffects;
    i32 currentBackgroundEffect{0};

    std::vector<std::shared_ptr<MeshAsset>> testMeshes;

    void immediate_submit(std::function<void(VkCommandBuffer cmd)>&&function);
    GPUMeshBuffers uploadMesh(std::span<u32> indices, std::span<Vertex> vertices);

    static VulkanEngine& Get();

    void init();

    void cleanup();    

    void draw();

    void run();
    
};