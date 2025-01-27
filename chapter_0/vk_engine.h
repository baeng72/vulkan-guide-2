#pragma once
#include <vk_types.h>
#include <vk_initializers.h>
#include <vk_images.h>
#include <glfw/glfw3.h>
#include <vector>


class VulkanEngine{
    
    void centreWindow();
    
    public:

    bool _isInitialized{false};
    int _frameNumber{0};    
    bool stop_rendering{false};
    bool _isRunning{false};
    VkExtent2D _windowExtent{1700, 900};
    
    GLFWwindow * pwindow{nullptr};
    


    static VulkanEngine& Get();

    void init();

    void cleanup();

    void draw();

    void run();
    
};