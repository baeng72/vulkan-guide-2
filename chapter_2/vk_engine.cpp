#include "vk_engine.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#include <vk_initializers.h>
#include <vk_pipelines.h>
#include <vk_types.h>
#include <FileUtils.h>

#include <VkBootstrap.h>
#include <array>
#include <chrono>
#include <thread>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

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

    init_descriptors();

    init_pipelines();

    init_imgui();

    _isInitialized = true;
}

void VulkanEngine::cleanup(){
    if(_isInitialized){

        if(_device!=VK_NULL_HANDLE){
            vkDeviceWaitIdle(_device);
            _mainDeletionQueue.flush();//cleanup stuff

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

void VulkanEngine::draw_background(VkCommandBuffer cmd){
    //make a clear-color from the frame number. This will flash with a 120 frame period.
    VkClearColorValue clearValue;
    float flash = std::abs(std::sin(_frameNumber/120.f));
    clearValue = {{ 0.f, 0.f, flash, 1.f}};

    VkImageSubresourceRange clearRange = vkinit::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);

    ComputeEffect & effect = backgroundEffects[currentBackgroundEffect];

    //bind the gradient drawing compute pipeline
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, effect.pipeline);

    //bind the descriptor set containing the draw image for the compute pipeline
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _gradientPipelineLayout, 0, 1, &_drawImageDescriptors, 0, nullptr);

    ComputePushConstants pc;
    pc.data1 = vec4(1.f,0.f,0.f,1.f);
    pc.data2 = vec4(0.f,0.f,1.f,0.f);
    vkCmdPushConstants(cmd, _gradientPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &effect.data);

    //execute the compute pipeline dispatch. we are using 16x16 workgroup size so we need to divide by it
    vkCmdDispatch(cmd, (u32)std::ceil(_drawExtent.width/16.f), (u32)std::ceil(_drawExtent.height/16.f),1);


}

void VulkanEngine::draw(){

    //wait until the gpu has finished rendering the last frame. 
    VK_CHECK(vkWaitForFences(_device, 1, &get_current_frame()._renderFence, VK_TRUE, UINT64_MAX));

    get_current_frame()._deletionQueue.flush();//flush this frames resources

    VK_CHECK(vkResetFences(_device, 1, &get_current_frame()._renderFence));

    //require image from the swapchain
    u32 swapchainImageIndex;
    VK_CHECK(vkAcquireNextImageKHR( _device, _swapchain, UINT64_MAX, get_current_frame()._swapchainSemaphore, nullptr, &swapchainImageIndex));

    VkCommandBuffer cmd = get_current_frame()._mainCommandBuffer;

    //now that we are sure that the commands finished executing, we can safely
    //reset the command buffer to begin recording again.
    VK_CHECK(vkResetCommandBuffer(cmd, 0));

    _drawExtent.width = _drawImage.imageExtent.width;
    _drawExtent.height = _drawImage.imageExtent.height;

    //begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
    VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    //start the command buffer recording
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    //make the swapchain image into writeable mode before rendering
    vkutil::transition_image(cmd, _drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    draw_background(cmd);

    //make the swapchain image into presentable mode
    vkutil::transition_image(cmd, _drawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    vkutil::transition_image(cmd, _swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    //

    //execute a copy from the draw image into the swapchain
    vkutil::copy_image_to_image(cmd, _drawImage.image, _swapchainImages[swapchainImageIndex], _drawExtent, _swapchainExtent);

    //set swapchain image layout to Attachment Optimal so we can draw it
    vkutil::transition_image(cmd, _swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    
    //draw imgui into the swapchain image
    draw_imgui(cmd, _swapchainImageViews[swapchainImageIndex]);
    
    vkutil::transition_image(cmd, _swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

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

void VulkanEngine::draw_imgui(VkCommandBuffer cmd, VkImageView targetImageView){
    VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(targetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkRenderingInfo renderInfo = vkinit::rendering_info(_swapchainExtent, &colorAttachment, nullptr);

    vkCmdBeginRendering(cmd, &renderInfo);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

    vkCmdEndRendering(cmd);
}

void VulkanEngine::run(){
    _isRunning=true;
    while(_isRunning){
        glfwPollEvents();
        if(stop_rendering){
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }else{
            //imgui new frame
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            if(ImGui::Begin("background")){
                ComputeEffect & selected = backgroundEffects[currentBackgroundEffect];

                ImGui::Text("Selected effect: ", selected.name);

                ImGui::SliderInt("Effect Index", &currentBackgroundEffect, 0, (i32)backgroundEffects.size()-1);

                ImGui::InputFloat4("data1", (float*)&selected.data.data1);
                ImGui::InputFloat4("data2", (float*)&selected.data.data2);
                ImGui::InputFloat4("data3", (float*)&selected.data.data3);
                ImGui::InputFloat4("data4", (float*)&selected.data.data4);

                ImGui::End();
            }

            //make imgui calculate internal draw structures
            ImGui::Render();

            
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

    //init vma 
    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.physicalDevice = physicalDevice;
    allocatorInfo.device = _device;
    allocatorInfo.instance = _instance;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&allocatorInfo, &_allocator);
    _mainDeletionQueue.push_function([&](){
        vmaDestroyAllocator(_allocator);
    });
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

    //draw image size will match the window
    VkExtent3D drawImageExtent{_windowExtent.width,_windowExtent.height,1};

    //hardcoding the draw format to 32 bit float
    _drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    _drawImage.imageExtent = drawImageExtent;

    VkImageUsageFlags drawImageUsages{};
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VkImageCreateInfo ring_info = vkinit::image_create_info(_drawImage.imageFormat, drawImageUsages, drawImageExtent);

    //for the draw image, we want to allocate it from gpu local memory
    VmaAllocationCreateInfo ring_allocInfo{};
    ring_allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    ring_allocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    //allocate and create the image
    vmaCreateImage(_allocator, &ring_info, &ring_allocInfo, &_drawImage.image, &_drawImage.allocation, &_drawImage.allocationInfo);

    //build a image-view for the draw image to use for rendering
    VkImageViewCreateInfo rview_info = vkinit::imageview_create_info(_drawImage.imageFormat, _drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

    VK_CHECK(vkCreateImageView(_device, &rview_info, nullptr, &_drawImage.imageView));

    //add to deletion queues
    _mainDeletionQueue.push_function([=](){
        vkDestroyImageView(_device, _drawImage.imageView, nullptr);
        vmaDestroyImage(_allocator, _drawImage.image, _drawImage.allocation);
    });
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

    VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_immCommandPool));

    //allocate the command buffer for immediate submits
    VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_immCommandPool, 1);

    VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_immCommandBuffer));

    _mainDeletionQueue.push_function([=](){
        vkDestroyCommandPool(_device, _immCommandPool, nullptr);
    });
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

    VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_immFence));
    _mainDeletionQueue.push_function([=](){
        vkDestroyFence(_device, _immFence, nullptr);
    });
}

void VulkanEngine::init_descriptors(){
    //create a descriptor pool that will hold 10 sets with 1 image each
    std::vector<DescriptorAllocator::PoolSizeRatio> sizes = {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}
    };

    globalDescriptorAllocator.init_pool(_device, 10, sizes);

    //make the descriptor set layout for our compute draw
    {
        DescriptorLayoutBuilder builder;
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        _drawImageDescriptorLayout = builder.build(_device, VK_SHADER_STAGE_COMPUTE_BIT);

        //allocate a descriptor set for our draw image
        _drawImageDescriptors = globalDescriptorAllocator.allocate(_device, _drawImageDescriptorLayout);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageInfo.imageView = _drawImage.imageView;

        VkWriteDescriptorSet drawImageWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        drawImageWrite.dstBinding = 0;
        drawImageWrite.dstSet = _drawImageDescriptors;
        drawImageWrite.descriptorCount = 1;
        drawImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        drawImageWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(_device, 1, &drawImageWrite, 0, nullptr);

        //make sure both the descriptor allocator and the new layout get cleaned up properly
        _mainDeletionQueue.push_function([&](){
            globalDescriptorAllocator.destroy_pool(_device);
            vkDestroyDescriptorSetLayout(_device, _drawImageDescriptorLayout, nullptr);
        });
    }
}

void VulkanEngine::init_pipelines(){
    VkPipelineLayoutCreateInfo computeLayout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    computeLayout.pSetLayouts = &_drawImageDescriptorLayout;
    computeLayout.setLayoutCount = 1;

    VkPushConstantRange pushConstant{};
    pushConstant.size = sizeof(ComputePushConstants);
    pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    
    computeLayout.pPushConstantRanges = &pushConstant;
    computeLayout.pushConstantRangeCount = 1;

    VK_CHECK(vkCreatePipelineLayout(_device, &computeLayout, nullptr, &_gradientPipelineLayout));

    init_background_pipelines();
}

void VulkanEngine::init_background_pipelines(){
    VkPipelineLayoutCreateInfo computeLayout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    computeLayout.pSetLayouts = &_drawImageDescriptorLayout;
    computeLayout.setLayoutCount = 1;
    VK_CHECK(vkCreatePipelineLayout(_device, &computeLayout, nullptr, &_gradientPipelineLayout));

    ccharp shaderSrcPath = "../shaders/gradient_color.comp";//"../shaders/gradient.comp";
    VkShaderModule computeDrawShader = get_shader(shaderSrcPath, VK_SHADER_STAGE_COMPUTE_BIT);

    ccharp skyShaderPath = "../shaders/sky.comp";    
    VkShaderModule skyShader = get_shader(skyShaderPath, VK_SHADER_STAGE_COMPUTE_BIT);

    VkPipelineShaderStageCreateInfo stageInfo{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageInfo.module = computeDrawShader;
    stageInfo.pName = "main";

    VkComputePipelineCreateInfo computePipelineCreateInfo{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    computePipelineCreateInfo.layout = _gradientPipelineLayout;
    computePipelineCreateInfo.stage = stageInfo;

    ComputeEffect gradient;
    gradient.layout = _gradientPipelineLayout;
    gradient.name = "gradient";
    gradient.data={};

    //default colors
    gradient.data.data1 = vec4(1.f, 0.f, 0.f, 1.f);
    gradient.data.data2 = vec4(0.f, 0.f, 1.f, 1.f);

    VK_CHECK(vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &gradient.pipeline));

    //change the shader module only to create the sky shader
    computePipelineCreateInfo.stage.module = skyShader;

    ComputeEffect sky;
    sky.layout = _gradientPipelineLayout;
    sky.name = "sky";
    sky.data = {};
    //default sky parameters
    sky.data.data1 = vec4(0.1f, 0.2f, 0.4f, 0.97f);

    VK_CHECK(vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &sky.pipeline));

    //add the 2 background effects into the array
    backgroundEffects.push_back(gradient);
    backgroundEffects.push_back(sky);

    vkDestroyShaderModule(_device, computeDrawShader, nullptr);
    vkDestroyShaderModule(_device, skyShader, nullptr);

    _mainDeletionQueue.push_function([=](){
        vkDestroyPipelineLayout(_device, _gradientPipelineLayout, nullptr);
        vkDestroyPipeline(_device, sky.pipeline, nullptr);
        vkDestroyPipeline(_device, gradient.pipeline, nullptr);
    });
}

VkShaderModule VulkanEngine::get_shader(ccharp shaderSrcPath, VkShaderStageFlagBits stage){
    char shaderPath[256];
    VkShaderModule shader{VK_NULL_HANDLE};
    strcpy_s(shaderPath,shaderSrcPath);
    strcat_s(shaderPath,".spv");
    bool needCompile =false;
    if(fileExists(shaderPath)){
        if(fileTime(shaderPath) < fileTime(shaderSrcPath)){
            needCompile = true;
        }
        else{
            if(!vkutil::load_shader_module(shaderPath, _device, &shader)){
                fmt::print("Error when building the compute shader \n");
                return VK_NULL_HANDLE;
            }
        }
    }else{
        needCompile = true;
    }

    if(needCompile){
        if(!vkutil::compile_shader_module(shaderSrcPath, _device, VK_SHADER_STAGE_COMPUTE_BIT, &shader)){
            fmt::print("Error when building the compute shader\n");
            return VK_NULL_HANDLE;
        }
    }
    return shader;
}

void VulkanEngine::immediate_submit(std::function<void(VkCommandBuffer cmd)>&&function){
    VK_CHECK(vkResetFences(_device, 1, &_immFence));
    VK_CHECK(vkResetCommandBuffer(_immCommandBuffer, 0));

    VkCommandBuffer cmd = _immCommandBuffer;

    VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    function(cmd);

    VK_CHECK(vkEndCommandBuffer(cmd));

    VkCommandBufferSubmitInfo cmdInfo = vkinit::command_buffer_submit_info(cmd);
    VkSubmitInfo2 submit = vkinit::submit_info(&cmdInfo, nullptr, nullptr);

    //submit command buffer to the queue and execute it.
    // _renderFence will now block until the graphic commands finish execution
    VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submit, _immFence));

    VK_CHECK(vkWaitForFences(_device, 1, &_immFence, VK_TRUE, UINT64_MAX));
}

void VulkanEngine::init_imgui(){
    //1. create descriptor pool for IMGUI
    // the size of the pool is very oversize, but it's copied from imgui emo
    // itself.
    VkDescriptorPoolSize pool_sizes[] = {
        {        VK_DESCRIPTOR_TYPE_SAMPLER, 10},
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 10},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 10},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 10},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 10},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 10}
    };

    VkDescriptorPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 10;
    pool_info.poolSizeCount = (u32)std::size(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;

    VkDescriptorPool imguiPool;
    VK_CHECK(vkCreateDescriptorPool(_device, &pool_info, nullptr, &imguiPool));

    //2: initialize imgui library

    //this initializes the core structures of imgui
    ImGui::CreateContext();

    //this initializes imgui for GLFW
    ImGui_ImplGlfw_InitForVulkan(pwindow,true);
    //ImGui_ImplGlfw_InstallCallbacks(pwindow);
    //this initializes imgui for Vulkan
    ImGui_ImplVulkan_InitInfo init_info{};
    init_info.Instance = _instance;
    init_info.PhysicalDevice = _physical;
    init_info.Device = _device;
    init_info.Queue = _graphicsQueue;
    init_info.DescriptorPool = imguiPool;
    init_info.MinImageCount = 3;
    init_info.ImageCount = 3;
    init_info.UseDynamicRendering = true;

    init_info.PipelineRenderingCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
    init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &_swapchainImageFormat;

    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_Init(&init_info);

    ImGui_ImplVulkan_CreateFontsTexture();

    //add the destroy the imgui created structures
    _mainDeletionQueue.push_function([=](){
        ImGui_ImplVulkan_Shutdown();
        vkDestroyDescriptorPool(_device, imguiPool, nullptr);
    });



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