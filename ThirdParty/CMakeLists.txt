cmake_minimum_required(VERSION 3.6 FATAL_ERROR)
cmake_policy(VERSION 3.6)

#fmt
add_subdirectory(fmt)

#glm
add_library(glm INTERFACE)
#set(GLM_DIR ${CMAKE_CURRENT_DIR}/glm)
#target_sources(glm INTERFACE ${GLM_DIR}/glm/glm.hpp)
#target_include_directories(glm INTERFACE ${GLM_DIR})

#target_compile_definitions(glm INTERFACE
    #GLM_FORCE_RADIANS
    #GLM_FORCE_ZERO_ON
    #)

#glfw
add_subdirectory(glfw)

#fast gltf
add_subdirectory(fastgltf)

#imgui
add_subdirectory(imgui)

#stb
add_library(stb INTERFACE)
set(STB_DIR ${CMAKE_CURRENT_DIR}/stb)
target_sources(stb INTERFACE ${STB_DIR}/stb_image.h)
target_include_directories(stb INTERFACE ${STB_DIR})

#vkbootstrap
add_subdirectory(vk-bootstrap)

#vma
#add_subdirectory(VulkanMemoryAllocator)
add_library(VulkanMemoryAllocator INTERFACE)



#both vma and glm and header only libs so we only need the include path
target_include_directories(VulkanMemoryAllocator INTERFACE VulkanMemoryAllocator)

target_include_directories(glm INTERFACE glm)