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

    init_vulkan();

    init_swapchain();

    init_commands();

    init_sync_structures();

    _isInitialized = true;
}

void VulkanEngine::cleanup(){
    if(_isInitialized){

        if(_device!=VK_NULL_HANDLE){
            vkDeviceWaitIdle(_device);

            for(i32 i=0; i < FRAME_OVERLAP; ++i){
                vkDestroyCommandPool(_device, _frames[i]._commandPool,nullptr);
                _frames[i]._commandPool = VK_NULL_HANDLE;
                //destroy sync objects
                vkDestroyFence(_device, _frames[i]._renderFence, nullptr);
                _frames[i]._renderFence = VK_NULL_HANDLE;
                vkDestroySemaphore(_device, _frames[i]._renderSemaphore, nullptr);
                _frames[i]._renderSemaphore = VK_NULL_HANDLE;
                vkDestroySemaphore(_device, _frames[i]._swapchainSemaphore, nullptr);
                _frames[i]._swapchainSemaphore = VK_NULL_HANDLE;
            }

            destroy_swapchain();

            vkDestroySurfaceKHR(_instance, _surface, nullptr);
            _surface = VK_NULL_HANDLE;

            vkDestroyDevice(_device, nullptr);
            _device = VK_NULL_HANDLE;
            vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
            vkDestroyInstance(_instance, nullptr);
        }
        glfwDestroyWindow(pwindow);
        glfwTerminate();
        _isInitialized=false;
    }
}

void VulkanEngine::draw(){

    //wait until the gpu has finished rendering the last frame. 
    VK_CHECK(vkWaitForFences(_device, 1, &get_current_frame()._renderFence, VK_TRUE, UINT64_MAX));
    VK_CHECK(vkResetFences(_device, 1, &get_current_frame()._renderFence));

    //require image from the swapchain
    u32 swapchainImageIndex;
    VK_CHECK(vkAcquireNextImageKHR( _device, _swapchain, UINT64_MAX, get_current_frame()._swapchainSemaphore, nullptr, &swapchainImageIndex));

    VkCommandBuffer cmd = get_current_frame()._mainCommandBuffer;

    //now that we are sure that the commands finished executing, we can safely
    //reset the command buffer to begin recording again.
    VK_CHECK(vkResetCommandBuffer(cmd, 0));

    //begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
    VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    //start the command buffer recording
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    //make the swapchain image into writeable mode before rendering
    vkutil::transition_image(cmd, _swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    //make a clear-color from the frame number. This will flash with a 120 frame period.
    VkClearColorValue clearValue;
    float flash = std::abs(std::sin(_frameNumber/120.f));
    clearValue = {{ 0.f, 0.f, flash, 1.f}};

    VkImageSubresourceRange clearRange = vkinit::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);

    //clear image
    vkCmdClearColorImage(cmd, _swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);

    //make the swapchain image into presentable mode
    vkutil::transition_image(cmd, _swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    //finialize the command buffer (we can no longer add commands, but it can now be executed)
    VK_CHECK(vkEndCommandBuffer(cmd));

    //prepare the submission to the queue.
    //we want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready
    //we will signal the _renderSemaphore, to signal that rendering has finished

    VkCommandBufferSubmitInfo cmdinfo = vkinit::command_buffer_submit_info(cmd);

    VkSemaphoreSubmitInfo waitInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, get_current_frame()._swapchainSemaphore);
    VkSemaphoreSubmitInfo signalInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, get_current_frame()._renderSemaphore);

    VkSubmitInfo2 submit = vkinit::submit_info(&cmdinfo, &signalInfo, &waitInfo);

    //submit command buffer to the queue and execute it
    //_renderFence will now block until the graphic commands finish execution
    VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submit, get_current_frame()._renderFence));

    //prepare present
    //this will put the image we just rendered into the visible window.
    //we want to wait on the _renderSemaphore for that,
    //as its necessary that drawing commands have finished before the image is displayed to the user
    VkPresentInfoKHR presentInfo{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    presentInfo.pSwapchains = &_swapchain;
    presentInfo.swapchainCount = 1;
    presentInfo.pWaitSemaphores = &get_current_frame()._renderSemaphore;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pImageIndices = &swapchainImageIndex;

    VK_CHECK(vkQueuePresentKHR(_graphicsQueue, &presentInfo));

    //increate the number of frames draw
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

void VulkanEngine::init_vulkan(){
    //init instance
    vkb::InstanceBuilder builder;

    //make the vulkan instance, with basic debug features
    auto inst_ret = builder.set_app_name("Example Vulkan Application")
    .request_validation_layers(bUseValidationLayers)
    .use_default_debug_messenger()
    .require_api_version(1, 3, 0)
    .build();

    vkb::Instance vkb_inst = inst_ret.value();

    //grab the instance
    _instance = vkb_inst.instance;
    _debug_messenger = vkb_inst.debug_messenger;

    //init device
    glfwCreateWindowSurface(_instance, pwindow, nullptr, &_surface);

    //vulkan 1.3 features
    VkPhysicalDeviceVulkan13Features features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features.dynamicRendering = VK_TRUE;
    features.synchronization2 = VK_TRUE;

    //vulkan 1.2 features
    VkPhysicalDeviceVulkan12Features features12{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    features12.bufferDeviceAddress = VK_TRUE;
    features12.descriptorIndexing = VK_TRUE;

    //use vkbootstrap to select gpu.
    //we want a gpu that can write to the surface and supports vulkan 1.3 with the correct features
    vkb::PhysicalDeviceSelector selector{vkb_inst};
    vkb::PhysicalDevice physicalDevice = selector.set_minimum_version(1, 3)
                            .set_required_features_13(features)
                            .set_required_features_12(features12)
                            .set_surface(_surface)
                            .select()
                            .value();

    //create the final vulkan device
    vkb::DeviceBuilder deviceBuilder(physicalDevice);

    vkb::Device vkbDevice = deviceBuilder.build().value();

    //Get the VkDevice handle used in the reset of a vulkan application
    _device = vkbDevice.device;
    _physical = physicalDevice.physical_device;

    //init queue
    _graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    _graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
}

void VulkanEngine::create_swapchain(u32 width, u32 height){
    vkb::SwapchainBuilder swapchainBuilder(_physical, _device, _surface);

    _swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

    VkSurfaceFormatKHR format;
    format.format = _swapchainImageFormat;
    format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    vkb::Swapchain vkbSwapchain = swapchainBuilder.set_desired_format(format)
                    .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                    .set_desired_extent(width, height)
                    .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                    .build()
                    .value();

    _swapchainExtent = vkbSwapchain.extent;
    _swapchain = vkbSwapchain.swapchain;
    _swapchainImages = vkbSwapchain.get_images().value();
    _swapchainImageViews = vkbSwapchain.get_image_views().value();                    
}

void VulkanEngine::init_swapchain(){
    create_swapchain(_windowExtent.width, _windowExtent.height);
}

void VulkanEngine::destroy_swapchain(){
    if(_swapchain!=VK_NULL_HANDLE){
        vkDestroySwapchainKHR(_device, _swapchain,nullptr);
        _swapchain = VK_NULL_HANDLE;

        for(size_t i=0; i< _swapchainImageViews.size(); ++i){
            vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
        }
        
    }
}

void VulkanEngine::init_commands(){
    //create a command pool for commands submitted to the graphics queue.
    //we also want the pool to allow for resetting of individual command buffers
    VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    for(i32 i = 0; i < FRAME_OVERLAP; ++i){
        VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_frames[i]._commandPool));

        //allocate the default command buffer that we will use for rendering
        VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_frames[i]._commandPool, 1);

        VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_frames[i]._mainCommandBuffer));
    }
}

void VulkanEngine::init_sync_structures(){
    //create synchronization structures
    //one fence to control when the gpu has finished rendering the frame,
    //and 2 semaphores to syncronize rendering with swapchain
    //we want the fence to start signalled so we can wait on it on the first frame
    VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
    VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();

    for(i32 i = 0; i < FRAME_OVERLAP; ++i){
        VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_frames[i]._renderFence));
        VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._swapchainSemaphore));
        VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._renderSemaphore));
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