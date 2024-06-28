#include "vk_engine.h"

#include <GLFW/glfw3.h>
#include <vk_initializers.h>
#include <vk_types.h>

#include <VkBootstrap.h>
#include <array>
#include <chrono>
#include <thread>

constexpr bool bUseValidationLayers = false;
VulkanEngine * loadedEngine{nullptr};
VulkanEngine& VulkanEngine::Get(){ return *loadedEngine;}

void VulkanEngine::init(){
    //only one engine initialization is allowed with the application.
    assert(loadedEngine == nullptr);
    loadedEngine = this;

    if (glfwInit() == GLFW_TRUE){
        
        if (glfwVulkanSupported() == GLFW_FALSE) {
            fmt::println("GLFW doesn't support Vulkan.");
            return;
        }
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);//inform glfw we don't want OpenGL
        _isInitialized = true;
    }

    glfwSetErrorCallback([](int error, ccharp description) {
        fmt::println("GLFW error %d, %s\n.", error, description);
        });
    pwindow = glfwCreateWindow(_windowExtent.width, _windowExtent.height, "Vulkan Engine", nullptr, nullptr);   

    centreWindow(); 

    glfwSetWindowUserPointer(pwindow, this);

    glfwSetWindowSizeCallback(pwindow, [](GLFWwindow* window, int width, int height) {
        //glfwindow* pwindow = (glfwindow*)glfwGetWindowUserPointer(window);
        VulkanEngine*peng=(VulkanEngine*)glfwGetWindowUserPointer(window);
        if(width==0 || height == 0){
            peng->stop_rendering = true;
        }
        else{
            peng->stop_rendering = false;
        }
        });
    glfwSetWindowCloseCallback(pwindow, [](GLFWwindow* window) {
        VulkanEngine*peng=(VulkanEngine*)glfwGetWindowUserPointer(window);
        peng->_isRunning=false;
        });
    glfwSetKeyCallback(pwindow, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        VulkanEngine*peng=(VulkanEngine*)glfwGetWindowUserPointer(window);
        if(action==GLFW_PRESS){
            switch(key){
                case GLFW_KEY_ESCAPE:
                peng->_isRunning=false;
                break;

            }
        }
        });
    glfwSetMouseButtonCallback(pwindow, [](GLFWwindow* window, int button, int action, int mods) {
        });
    glfwSetScrollCallback(pwindow, [](GLFWwindow* window, double xoffset, double yoffset) {
        });
    glfwSetCursorPosCallback(pwindow, [](GLFWwindow* window, double xpos, double ypos) {
        });

    
    _isInitialized = true;
}

void VulkanEngine::cleanup(){
    if(_isInitialized){

        
        glfwDestroyWindow(pwindow);
        glfwTerminate();
        _isInitialized=false;
    }
}

void VulkanEngine::draw(){

    
    _frameNumber++;


}

void VulkanEngine::run(){
    _isRunning=true;
    while(_isRunning){
        glfwPollEvents();
        if(stop_rendering){
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }else{
            draw();
        }
    }
}

void VulkanEngine::centreWindow()
{
    // Get window position and size
    int posX, posY;
    glfwGetWindowPos(pwindow, &posX, &posY);

    int width, height;
    glfwGetWindowSize(pwindow, &width, &height);

    // Halve the window size and use it to adjust the window position to the center of the window
    width >>= 1;
    height >>= 1;

    posX += width;
    posY += height;

    // Get the list of monitors
    int count;
    GLFWmonitor** monitors = glfwGetMonitors(&count);
    if (monitors == nullptr)
        return;

    // Figure out which monitor the window is in
    GLFWmonitor* owner = NULL;
    int owner_x, owner_y, owner_width, owner_height;

    for (int i = 0; i < count; i++)
    {
        // Get the monitor position
        int monitor_x, monitor_y;
        glfwGetMonitorPos(monitors[i], &monitor_x, &monitor_y);

        // Get the monitor size from its video mode
        int monitor_width, monitor_height;
        GLFWvidmode* monitor_vidmode = (GLFWvidmode*)glfwGetVideoMode(monitors[i]);

        if (monitor_vidmode == NULL)
            continue;

        monitor_width = monitor_vidmode->width;
        monitor_height = monitor_vidmode->height;

        // Set the owner to this monitor if the center of the window is within its bounding box
        if ((posX > monitor_x && posX < (monitor_x + monitor_width)) && (posY > monitor_y && posY < (monitor_y + monitor_height)))
        {
            owner = monitors[i];
            owner_x = monitor_x;
            owner_y = monitor_y;
            owner_width = monitor_width;
            owner_height = monitor_height;
        }
    }

    // Set the window position to the center of the owner monitor
    if (owner != NULL)
        glfwSetWindowPos(pwindow, owner_x + (owner_width >> 1) - width, owner_y + (owner_height >> 1) - height);
}