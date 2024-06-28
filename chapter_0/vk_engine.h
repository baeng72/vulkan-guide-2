#pragma once
#include <vk_types.h>
#include <vk_initializers.h>
#include <vk_images.h>
#include <glfw/glfw3.h>
#include <vector>

struct FrameData{
    VkSemaphore _swapchainSemaphore{VK_NULL_HANDLE};
    VkSemaphore _renderSemaphore{VK_NULL_HANDLE};
    VkFence _renderFence{VK_NULL_HANDLE};
    VkCommandPool _commandPool{VK_NULL_HANDLE};
    VkCommandBuffer _mainCommandBuffer{VK_NULL_HANDLE};
};

constexpr unsigned int FRAME_OVERLAP = 2;//max frames?

class VulkanEngine{
    
    void centreWindow();
    void init_vulkan();
    void init_swapchain();
    void create_swapchain(u32 width, u32 height);
    void destroy_swapchain();
    void init_commands();
    void init_sync_structures();
    public:

    bool _isInitialized{false};
    int _frameNumber{0};    
    bool stop_rendering{false};
    bool _isRunning{false};
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






    static VulkanEngine& Get();

    void init();

    void cleanup();

    void draw();

    void run();
    
};