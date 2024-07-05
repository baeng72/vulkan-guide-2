#include "vk_engine.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// #ifdef _DEBUG
// #define VMA_DEBUG_LOG(format, ...) do { \
//         printf(format, __VA_ARGS__); \
//         printf("\n"); \
//     } while(false)

// #endif
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#include <vk_initializers.h>
#include <vk_pipelines.h>
#include <vk_types.h>
#include <FileUtils.h>

#include <VkBootstrap.h>

#include <glm/packing.hpp>

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

    init_default_data();

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
                _frames[i]._deletionQueue.flush();
            }

            

            for(auto&mesh : testMeshes){
                destroy_buffer(mesh->meshBuffers.indexBuffer);
                destroy_buffer(mesh->meshBuffers.vertexBuffer);
            }

            _mainDeletionQueue.flush();//cleanup stuff

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

    

    _drawExtent.height = (u32)(std::min(_swapchainExtent.height, _drawImage.imageExtent.height) * renderScale);
    _drawExtent.width = (u32)(std::min(_swapchainExtent.width, _drawImage.imageExtent.width) * renderScale);

    update_scene();

    //wait until the gpu has finished rendering the last frame. 
    VK_CHECK(vkWaitForFences(_device, 1, &get_current_frame()._renderFence, VK_TRUE, UINT64_MAX));

    get_current_frame()._deletionQueue.flush();//flush this frames resources
    get_current_frame()._frameDescriptors.clear_pools(_device); 

    VK_CHECK(vkResetFences(_device, 1, &get_current_frame()._renderFence));

    //require image from the swapchain
    u32 swapchainImageIndex;
    //VK_CHECK(vkAcquireNextImageKHR( _device, _swapchain, UINT64_MAX, get_current_frame()._swapchainSemaphore, nullptr, &swapchainImageIndex));
    VkResult err = vkAcquireNextImageKHR( _device, _swapchain, UINT64_MAX, get_current_frame()._swapchainSemaphore, nullptr, &swapchainImageIndex);
    if(err == VK_ERROR_OUT_OF_DATE_KHR){
        _resize_requested = true;
        return;
    }

    VkCommandBuffer cmd = get_current_frame()._mainCommandBuffer;

    //now that we are sure that the commands finished executing, we can safely
    //reset the command buffer to begin recording again.
    VK_CHECK(vkResetCommandBuffer(cmd, 0));

    //_drawExtent.width = _drawImage.imageExtent.width;
    //_drawExtent.height = _drawImage.imageExtent.height;

    //begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
    VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    //start the command buffer recording
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    //make the swapchain image into writeable mode before rendering
    vkutil::transition_image(cmd, _drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    draw_background(cmd);

    vkutil::transition_image(cmd, _drawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    vkutil::transition_image(cmd, _depthImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
    draw_geometry(cmd);

    //make the swapchain image into presentable mode
    vkutil::transition_image(cmd, _drawImage.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
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

    //VK_CHECK(vkQueuePresentKHR(_graphicsQueue, &presentInfo));
    err = vkQueuePresentKHR(_graphicsQueue, &presentInfo);
    if(err == VK_ERROR_OUT_OF_DATE_KHR){
        _resize_requested=true;
    }

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

void VulkanEngine::draw_geometry(VkCommandBuffer cmd){

    //allocate a new uniform buffer for the scene data
    AllocatedBuffer gpuSceneDataBuffer = create_buffer(sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    //add it to the deletion queue of this frame so it gets deleted once its been used
    get_current_frame()._deletionQueue.push_function([=, this](){
        destroy_buffer(gpuSceneDataBuffer);
    });

    //write the buffer
    GPUSceneData * sceneUniformData = (GPUSceneData*)gpuSceneDataBuffer.allocation->GetMappedData();
    *sceneUniformData = sceneData;

    //create a descriptor set that binds that buffer and update it
    VkDescriptorSet globalDescriptor = get_current_frame()._frameDescriptors.allocate(_device, _gpuSceneDataDescriptorLayout);

    DescriptorWriter writer;
    writer.write_buffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    writer.update_set(_device, globalDescriptor);

    //begin a render pass connected to our draw image
    VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(_drawImage.imageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkRenderingAttachmentInfo depthAttachment = vkinit::depth_attachment_info(_depthImage.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    VkRenderingInfo renderInfo = vkinit::rendering_info(_drawExtent, &colorAttachment, &depthAttachment);
    vkCmdBeginRendering(cmd, &renderInfo);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _trianglePipeline);

    //set dynamic viewport and scissor
    VkViewport viewport{};
    viewport.x = 0.f;
    viewport.y = 0.f;
    viewport.width = (float)_drawExtent.width;
    viewport.height = (float)_drawExtent.height;
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;

    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = _drawExtent.width;
    scissor.extent.height = _drawExtent.height;

    vkCmdSetScissor(cmd, 0, 1, &scissor);

    //launch a draw command to draw 3 vertices
   vkCmdDraw(cmd, 3, 1, 0, 0);

    for(const RenderObject & draw : mainDrawContext.OpaqueSurfaces){
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, draw.material->pipeline->pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, draw.material->pipeline->layout, 0, 1, &globalDescriptor, 0, nullptr);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, draw.material->pipeline->layout, 1, 1, &draw.material->materialSet,0,nullptr);
        vkCmdBindIndexBuffer(cmd,draw.indexBuffer,0, VK_INDEX_TYPE_UINT32);

        GPUDrawPushConstants push_constants;    
    push_constants.vertexBuffer = draw.vertexBufferAddress;
    push_constants.worldMatrix = draw.transform;
    
    vkCmdPushConstants(cmd, _meshPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants), &push_constants);
    vkCmdDrawIndexed(cmd,draw.indexCount, 1, draw.firstIndex, 0, 0);

    }


    vkCmdEndRendering(cmd);
}

void VulkanEngine::update_scene(){
    mainDrawContext.OpaqueSurfaces.clear();

    //loadedNodes["Suzanne"]->Draw(mat4(1.f),mainDrawContext);

    for(auto & m : loadedNodes){
        m.second->Draw(mat4(1.f),mainDrawContext);
    }

    for(i32 x = -3; x < 3; x++){
        mat4 scale = glm::scale(mat4(1.f), vec3(0.2f));
        mat4 translation = glm::translate(mat4(1.f), vec3((float)x, 1.f, 0.f));
        loadedNodes["Cube"]->Draw(translation * scale, mainDrawContext);
    }

    sceneData.view = glm::translate(mat4(1.f), vec3(0.f,0.f,-5.f));
    //camera projection
    sceneData.proj = glm::perspective(glm::radians(70.f),(float)_windowExtent.width / (float)_windowExtent.height, 10000.f, 0.1f);

    //invert the Y direction on projection matrix so that we are more similar to OpenGL and GLTF
    sceneData.proj[1][1] *= -1;
    sceneData.viewproj = sceneData.proj * sceneData.view;

    //some default lighting parameters
    sceneData.ambientColor = vec4(.1f);
    sceneData.sunlightColor = vec4(1.f);
    sceneData.sunlightDirection = vec4(0.f,1.f,0.5f,1.f);

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
            if(_resize_requested){
                resize_swapchain();
            }
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            if(ImGui::Begin("background")){
                ImGui::SliderFloat("Render Scale", &renderScale, 0.3f, 1.f);
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

    _depthImage.imageFormat = VK_FORMAT_D32_SFLOAT;
    _depthImage.imageExtent = drawImageExtent;
    VkImageUsageFlags depthImageUsages{};
    depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    VkImageCreateInfo dimg_info = vkinit::image_create_info(_depthImage.imageFormat, depthImageUsages, drawImageExtent);

    //allocate and create the image
    vmaCreateImage(_allocator, &dimg_info, &ring_allocInfo, &_depthImage.image, &_depthImage.allocation, &_depthImage.allocationInfo);

    //build a image-view for the draw iamge to use for rendering
    VkImageViewCreateInfo dview_info = vkinit::imageview_create_info(_depthImage.imageFormat, _depthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);

    VK_CHECK(vkCreateImageView(_device, &dview_info, nullptr, &_depthImage.imageView));


    //add to deletion queues
    _mainDeletionQueue.push_function([=](){
        vkDestroyImageView(_device, _drawImage.imageView, nullptr);
        vmaDestroyImage(_allocator, _drawImage.image, _drawImage.allocation);

        vkDestroyImageView(_device, _depthImage.imageView, nullptr);
        vmaDestroyImage(_allocator, _depthImage.image, _depthImage.allocation);
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

void VulkanEngine::resize_swapchain(){
    vkDeviceWaitIdle(_device);
    destroy_swapchain();

    int w, h;
    glfwGetWindowSize(pwindow,&w,&h);
    _windowExtent.width = w;
    _windowExtent.height = h;
    create_swapchain(_windowExtent.width, _windowExtent.height);

    _resize_requested = false;
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
    std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1}

    };

    globalDescriptorAllocator.init(_device, 10, sizes);

    //make the descriptor set layout for our compute draw
    {
        DescriptorLayoutBuilder builder;
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        _drawImageDescriptorLayout = builder.build(_device, VK_SHADER_STAGE_COMPUTE_BIT);
    }
    {
        DescriptorLayoutBuilder builder;
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        _singleImageDescriptorLayout = builder.build(_device, VK_SHADER_STAGE_FRAGMENT_BIT);
    }
    {
        DescriptorLayoutBuilder builder;
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        _gpuSceneDataDescriptorLayout = builder.build(_device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    }
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
        globalDescriptorAllocator.destroy_pools(_device);
        vkDestroyDescriptorSetLayout(_device, _drawImageDescriptorLayout, nullptr);
        vkDestroyDescriptorSetLayout(_device, _singleImageDescriptorLayout, nullptr);
        vkDestroyDescriptorSetLayout(_device, _gpuSceneDataDescriptorLayout, nullptr);
    });

    for(i32 i=0; i < FRAME_OVERLAP; ++i){
        //create a descriptor pool
        std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> frame_sizes = {
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3},
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3},
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4},
        };

        _frames[i]._frameDescriptors = DescriptorAllocatorGrowable();
        _frames[i]._frameDescriptors.init(_device, 1000, frame_sizes);

        _mainDeletionQueue.push_function([&, i](){
            _frames[i]._frameDescriptors.destroy_pools(_device);
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

    init_triangle_pipeline();

    init_mesh_pipeline();

    metalRoughMaterial.build_pipelines(this);
}

void VulkanEngine::init_triangle_pipeline(){
    ccharp fragmentSrcPath = "../shaders/colored_triangle.frag";
    VkShaderModule triangleFragShader = get_shader(fragmentSrcPath,VK_SHADER_STAGE_FRAGMENT_BIT);

    ccharp vertexSrcPath = "../shaders/colored_triangle.vert";
    VkShaderModule triangleVertShader = get_shader(vertexSrcPath, VK_SHADER_STAGE_VERTEX_BIT);

    //build the pipeline layout that controls the inputs/outputs of the shader
    //we are not using descriptor sets or other systems yet, so no need to use anything other than empty default
    VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::pipeline_layout_create_info();
    VK_CHECK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_trianglePipelineLayout));

    PipelineBuilder pipelineBuilder;
    //use the triangle layout we created
    pipelineBuilder._pipelineLayout = _trianglePipelineLayout;
    //connecting the vertex and pixel shaders to the pipeline
    pipelineBuilder.set_shaders(triangleVertShader, triangleFragShader);
    //it will draw triangles
    pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    //filled triangles
    pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
    //no backface calling
    pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    //no multisampling
    pipelineBuilder.set_multisampling_none();
    //no blending
    pipelineBuilder.disable_blending();
    //no depth testing
    pipelineBuilder.disable_depthtest();

    //connect the image format we will draw into, from draw image
    pipelineBuilder.set_color_attachment_format(_drawImage.imageFormat);
    pipelineBuilder.set_depth_format(VK_FORMAT_UNDEFINED);

    //finaly build the pipeline
    _trianglePipeline = pipelineBuilder.build_pipeline(_device);

    //clean structures
    vkDestroyShaderModule(_device, triangleFragShader, nullptr);
    vkDestroyShaderModule(_device, triangleVertShader, nullptr);

    _mainDeletionQueue.push_function([&](){
        vkDestroyPipelineLayout(_device, _trianglePipelineLayout, nullptr);
        vkDestroyPipeline(_device, _trianglePipeline, nullptr);
    });


}

void VulkanEngine::init_mesh_pipeline(){
    ccharp fragmentSrcPath = "../shaders/tex_image.frag";
    VkShaderModule meshFragShader = get_shader(fragmentSrcPath,VK_SHADER_STAGE_FRAGMENT_BIT);

    ccharp vertexSrcPath = "../shaders/colored_triangle_mesh.vert";
    VkShaderModule meshVertShader = get_shader(vertexSrcPath, VK_SHADER_STAGE_VERTEX_BIT);

    VkPushConstantRange bufferRange{};
    bufferRange.offset = 0;
    bufferRange.size = sizeof(GPUDrawPushConstants);
    bufferRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    //build the pipeline layout that controls the inputs/outputs of the shader
    //we are not using descriptor sets or other systems yet, so no need to use anything other than empty default
    VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::pipeline_layout_create_info();
    pipeline_layout_info.pPushConstantRanges = &bufferRange;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pSetLayouts = &_singleImageDescriptorLayout;
    pipeline_layout_info.setLayoutCount = 1;
    VK_CHECK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_meshPipelineLayout));

    PipelineBuilder pipelineBuilder;
    //use the triangle layout we created
    pipelineBuilder._pipelineLayout = _meshPipelineLayout;
    //connecting the vertex and pixel shaders to the pipeline
    pipelineBuilder.set_shaders(meshVertShader, meshFragShader);
    //it will draw triangles
    pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    //filled triangles
    pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
    //no backface calling
    pipelineBuilder.set_cull_mode(VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_CLOCKWISE);
    //no multisampling
    pipelineBuilder.set_multisampling_none();
    //no blending
    //pipelineBuilder.disable_blending();
    pipelineBuilder.enable_blending_additive();
    //no depth testing
    //pipelineBuilder.disable_depthtest();
    pipelineBuilder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

    //connect the image format we will draw into, from draw image
    pipelineBuilder.set_color_attachment_format(_drawImage.imageFormat);
    pipelineBuilder.set_depth_format(VK_FORMAT_UNDEFINED);

    //finaly build the pipeline
    _meshPipeline = pipelineBuilder.build_pipeline(_device);

    //clean structures
    vkDestroyShaderModule(_device, meshFragShader, nullptr);
    vkDestroyShaderModule(_device, meshVertShader, nullptr);

    _mainDeletionQueue.push_function([&](){
        vkDestroyPipelineLayout(_device, _meshPipelineLayout, nullptr);
        vkDestroyPipeline(_device, _meshPipeline, nullptr);
    });


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

void VulkanEngine::init_default_data(){
    std::array<Vertex,4> rect_vertices;
    rect_vertices[0].position = {0.5f,-0.5f, 0.f};
    rect_vertices[1].position = {0.5f,0.5f,0.f};
    rect_vertices[2].position = {-0.5f,-0.5f,0.f};
    rect_vertices[3].position = {-0.5f,0.5f,0.f};

    rect_vertices[0].color = {0.f,0.f,0.f,1.f};
    rect_vertices[1].color = {0.5f, 0.5f, 0.5f, 1.f};
    rect_vertices[2].color = {1.0, 0.f, 0.f, 1.f};
    rect_vertices[3].color = {0.f, 1.f, 0.f, 1.f};

    std::array<u32,6> rect_indices;
    rect_indices[0] = 0;
    rect_indices[1] = 1;
    rect_indices[2] = 2;

    rect_indices[3] = 2;
    rect_indices[4] = 1;
    rect_indices[5] = 3;

    rectangle = uploadMesh(rect_indices, rect_vertices);

    

    _mainDeletionQueue.push_function([&](){
        destroy_buffer(rectangle.indexBuffer);
        destroy_buffer(rectangle.vertexBuffer);
    });

    //3 default textures, white, grey, black, 1 pixel each
    u32 white = glm::packUnorm4x8(vec4(1.f, 1.f, 1.f, 1.f));
    _whiteImage = create_image((void*)&white, VkExtent3D{1, 1 , 1}, VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_SAMPLED_BIT);

    u32 grey = glm::packUnorm4x8(vec4(0.66f, 0.66f, 0.66f, 1.f));        
    _greyImage = create_image((void*)&grey, VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_SAMPLED_BIT);

    u32 black = glm::packUnorm4x8(vec4(0.f));
    _blackImage = create_image((void*)&black, VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_SAMPLED_BIT);

    //checkerboard image
    u32 magenta = glm::packUnorm4x8(vec4(1.f, 0.f, 1.f, 1.f));
    std::array<u32, 16*16> pixels;      //for 16x16 checkerboard texture
    for(i32 x = 0; x < 16; x++){
        for(i32 y = 0; y < 16; y++){
            pixels[y*16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
        }
    }        
    _errorCheckerboardImage = create_image(pixels.data(), VkExtent3D{16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_SAMPLED_BIT);

    VkSamplerCreateInfo smpl{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    smpl.magFilter = VK_FILTER_NEAREST;
    smpl.minFilter = VK_FILTER_NEAREST;
    vkCreateSampler(_device, &smpl, nullptr, &_defaultSamplerNearest);

    smpl.magFilter = VK_FILTER_LINEAR;
    smpl.minFilter = VK_FILTER_LINEAR;
    vkCreateSampler(_device, &smpl, nullptr, &_defaultSamplerLinear);

    _mainDeletionQueue.push_function([&](){
        vkDestroySampler(_device, _defaultSamplerNearest, nullptr);
        vkDestroySampler(_device, _defaultSamplerLinear, nullptr);

        destroy_image(_whiteImage);
        destroy_image(_greyImage);
        destroy_image(_blackImage);
        destroy_image(_errorCheckerboardImage);
    });

    GLTFMetallic_Roughness::MaterialResources materialResources;
    //default the material textures
    materialResources.colorImage = _whiteImage;
    materialResources.colorSampler = _defaultSamplerLinear;
    materialResources.metalRoughImage = _whiteImage;
    materialResources.metalRoughSampler = _defaultSamplerLinear;

    //set the uniform buffer for the material data
    AllocatedBuffer materialConstants = create_buffer(sizeof(GLTFMetallic_Roughness::MaterialConstants), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    //write the buffer
    GLTFMetallic_Roughness::MaterialConstants* sceneUniformData = (GLTFMetallic_Roughness::MaterialConstants*)materialConstants.allocation->GetMappedData();
    sceneUniformData->colorFactor = vec4(1.f);
    sceneUniformData->metalRoughnessFactor = vec4(1.f, 0.5f, 0.f, 0.f);

    _mainDeletionQueue.push_function([=, this](){
        destroy_buffer(materialConstants);
    });

    materialResources.dataBuffer  = materialConstants.buffer;
    materialResources.dataBufferOffset = 0;

    defaultData = metalRoughMaterial.write_material(_device, MaterialPass::MainColor, materialResources, globalDescriptorAllocator);
    
    testMeshes = loadGltfMeshes(this,"../assets/basicmesh.glb").value();

    for(auto &m : testMeshes){
        std::shared_ptr<MeshNode> newNode = std::make_shared<MeshNode>();
        newNode->mesh = m;
        newNode->localTransform = mat4(1.f);
        newNode->worldTransform = mat4(1.f);

        for(auto& s : newNode->mesh->surfaces){
            s.material = std::make_shared<GLTFMaterial>(defaultData);
        }
        loadedNodes[m->name] = std::move(newNode);
    }
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

AllocatedBuffer VulkanEngine::create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memory){
    //allocate buffer
    VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = allocSize;
    bufferInfo.usage = usage;

    VmaAllocationCreateInfo vmaAlloc{};
    vmaAlloc.usage = memory;
    vmaAlloc.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    AllocatedBuffer newBuffer;
    //allocate the buffer
    VK_CHECK(vmaCreateBuffer(_allocator, &bufferInfo, &vmaAlloc, &newBuffer.buffer, &newBuffer.allocation, &newBuffer.allocationInfo));

    return newBuffer;
}

void VulkanEngine::destroy_buffer(const AllocatedBuffer& buffer){
    vmaDestroyBuffer(_allocator, buffer.buffer, buffer.allocation);
}

AllocatedImage VulkanEngine::create_image(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped){
    AllocatedImage newImage;
    newImage.imageFormat = format;
    newImage.imageExtent = size;

    VkImageCreateInfo img_info = vkinit::image_create_info(format, usage, size);
    if(mipmapped){
        img_info.mipLevels = static_cast<u32>(std::floor(std::log2(std::max(size.width, size.height))))+1;
    }

    //always allocate images on dedicated GPU memory
    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    //allocate and create the image
    VK_CHECK(vmaCreateImage(_allocator, &img_info, &allocInfo, &newImage.image, &newImage.allocation, &newImage.allocationInfo));

    //if the format is a depth format, we will need to have it use the correct
    //aspect flag
    VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
    if(format == VK_FORMAT_D32_SFLOAT){
        aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    //build a image-view for the image
    VkImageViewCreateInfo view_info = vkinit::imageview_create_info(format, newImage.image, aspectFlag);
    view_info.subresourceRange.layerCount = img_info.mipLevels;

    VK_CHECK(vkCreateImageView(_device, &view_info, nullptr, &newImage.imageView));

    return newImage;
}

AllocatedImage VulkanEngine::create_image(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped){
    size_t data_size = size.depth * size.width * size.height * 4;
    AllocatedBuffer uploadbuffer = create_buffer(data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    memcpy(uploadbuffer.allocationInfo.pMappedData, data, data_size);

    AllocatedImage new_image = create_image(size, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, mipmapped);

    immediate_submit([&](VkCommandBuffer cmd){
        vkutil::transition_image(cmd, new_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VkBufferImageCopy copyRegion{};
        
        copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.layerCount = 1;
        copyRegion.imageExtent = size;

        //copy the buffer into the image
        vkCmdCopyBufferToImage(cmd, uploadbuffer.buffer, new_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
            &copyRegion);

        vkutil::transition_image(cmd, new_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    });

    destroy_buffer(uploadbuffer);

    return new_image;
}

void VulkanEngine::destroy_image(const AllocatedImage& img){
    vkDestroyImageView(_device, img.imageView, nullptr);
    vmaDestroyImage(_allocator, img.image, img.allocation);
}

GPUMeshBuffers VulkanEngine::uploadMesh(std::span<u32> indices, std::span<Vertex> vertices){
    const size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
    const size_t indexBufferSize = indices.size() * sizeof(u32);

    GPUMeshBuffers newMesh;

    //create vertex buffer
    newMesh.vertexBuffer = create_buffer(vertexBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    //find the address ofthe vertex buffer
    VkBufferDeviceAddressInfo addrInfo{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
    addrInfo.buffer = newMesh.vertexBuffer.buffer;
    newMesh.vertexBufferAddress = vkGetBufferDeviceAddress(_device, &addrInfo);

    //create index buffer
    newMesh.indexBuffer = create_buffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    AllocatedBuffer staging = create_buffer(vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    void * data = staging.allocation->GetMappedData();

    //copy vertex buffer
    memcpy(data, vertices.data(),vertexBufferSize);
    //copy index buffer
    memcpy((char*)data+vertexBufferSize,indices.data(),indexBufferSize);

    immediate_submit([&](VkCommandBuffer cmd){
        VkBufferCopy vertexCopy{0};
        vertexCopy.dstOffset = 0;
        vertexCopy.srcOffset = 0;
        vertexCopy.size = vertexBufferSize;

        vkCmdCopyBuffer(cmd, staging.buffer, newMesh.vertexBuffer.buffer, 1, &vertexCopy);

        VkBufferCopy indexCopy{};
        indexCopy.dstOffset = 0;
        indexCopy.srcOffset = vertexBufferSize;
        indexCopy.size = indexBufferSize;

        vkCmdCopyBuffer(cmd, staging.buffer, newMesh.indexBuffer.buffer, 1, &indexCopy);
    });

    destroy_buffer(staging);

    return newMesh;


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

void GLTFMetallic_Roughness::build_pipelines(VulkanEngine * engine){    
    ccharp meshFragPath = "../shaders/mesh.frag";
    const VkShaderModule meshFragShader = engine->get_shader(meshFragPath, VK_SHADER_STAGE_FRAGMENT_BIT);    
    ccharp meshVertPath = "../shaders/mesh.vert";
    const VkShaderModule meshVertexShader = engine->get_shader(meshVertPath, VK_SHADER_STAGE_VERTEX_BIT);

    VkPushConstantRange matrixRange{};
    matrixRange.offset = 0;
    matrixRange.size = sizeof(GPUDrawPushConstants);
    matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    DescriptorLayoutBuilder layoutBuilder;
    layoutBuilder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    layoutBuilder.add_binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    layoutBuilder.add_binding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    materialLayout = layoutBuilder.build(engine->_device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

    VkDescriptorSetLayout layouts[] = {engine->_gpuSceneDataDescriptorLayout, materialLayout};

    VkPipelineLayoutCreateInfo mesh_layout_info = vkinit::pipeline_layout_create_info();
    mesh_layout_info.setLayoutCount = 2;
    mesh_layout_info.pSetLayouts = layouts;
    mesh_layout_info.pPushConstantRanges = &matrixRange;
    mesh_layout_info.pushConstantRangeCount = 1;

    VkPipelineLayout newLayout;
    VK_CHECK(vkCreatePipelineLayout(engine->_device, &mesh_layout_info, nullptr, &newLayout));

    opaquePipeline.layout  = newLayout;
    transparentPipeline.layout = newLayout;

    //build the stage-create info for both vertex and frag stages. This lets 
    //the pipeline know the shader modules per stage.
    PipelineBuilder pipelineBuilder;
    pipelineBuilder.set_shaders(meshVertexShader, meshFragShader);
    pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
    pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    pipelineBuilder.set_multisampling_none();
    pipelineBuilder.disable_blending();
    pipelineBuilder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

    //render format
    pipelineBuilder.set_color_attachment_format(engine->_drawImage.imageFormat);
    pipelineBuilder.set_depth_format(engine->_depthImage.imageFormat);

    //use the triangle layout we created
    pipelineBuilder._pipelineLayout = newLayout;

    //finally build the pipeline
    opaquePipeline.pipeline = pipelineBuilder.build_pipeline(engine->_device);

    //create the transparent variant
    pipelineBuilder.enable_blending_additive();

    pipelineBuilder.enable_depthtest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);

    transparentPipeline.pipeline = pipelineBuilder.build_pipeline(engine->_device);

    vkDestroyShaderModule(engine->_device, meshFragShader, nullptr);
    vkDestroyShaderModule(engine->_device, meshVertexShader, nullptr);

}

MaterialInstance GLTFMetallic_Roughness::write_material(VkDevice device, MaterialPass pass, const MaterialResources & resources, DescriptorAllocatorGrowable& descriptorAllocator){
    MaterialInstance matData;
    matData.passType = pass;
    if(pass == MaterialPass::Transparent){
        matData.pipeline = &transparentPipeline;
    }else{
        matData.pipeline = &opaquePipeline;
    }

    matData.materialSet = descriptorAllocator.allocate(device, materialLayout);

    writer.clear();
    writer.write_buffer(0, resources.dataBuffer, sizeof(MaterialConstants), resources.dataBufferOffset, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    writer.write_image(1, resources.colorImage.imageView, resources.colorSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    writer.write_image(2, resources.metalRoughImage.imageView, resources.metalRoughSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    writer.update_set(device, matData.materialSet);

    return matData;
}

void MeshNode::Draw(const mat4& topMatrix, DrawContext&ctx){
    mat4 nodeMatrix = topMatrix * worldTransform;

    for(auto& s : mesh->surfaces){
        RenderObject def;
        def.indexCount = s.count;
        def.firstIndex = s.startIndex;
        def.indexBuffer = mesh->meshBuffers.indexBuffer.buffer;
        def.material = &s.material->data;

        def.transform = nodeMatrix;
        def.vertexBufferAddress = mesh->meshBuffers.vertexBufferAddress;

        ctx.OpaqueSurfaces.push_back(def);
    }

    //recurse down
    Node::Draw(topMatrix, ctx);
}
