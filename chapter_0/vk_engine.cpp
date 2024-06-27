#include "vk_engine.h"

#include <GLFW/glfw3.h>
#include <vk_initializers.h>
#include <vk_types.h>

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
}

void VulkanEngine::cleanup(){
    if(_isInitialized){
        glfwDestroyWindow(pwindow);
        glfwTerminate();
        _isInitialized=false;
    }
}

void VulkanEngine::draw(){

}

void VulkanEngine::run(){
    _isRunning=true;
    while(_isRunning){
        glfwPollEvents();
        if(stop_rendering){
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        draw();
    }
}