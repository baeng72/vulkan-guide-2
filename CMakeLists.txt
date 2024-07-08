cmake_minimum_required(VERSION 3.6.0)
project(Vulkan-Guide VERSION 0.1.0 LANGUAGES C CXX)

find_package(Vulkan)
include_directories(AFTER ${Vulkan_INCLUDE_DIRS})
# GLFW options
set(GLFW_VULKAN_STATIC ON CACHE BOOL "")
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "")
set(GLFW_INSTALL OFF CACHE BOOL "")
## GLSLANG options
set(ENABLE_GLSLANG_BINARIES  OFF  CACHE BOOL "")
set(ENABLE_HLSL              OFF  CACHE BOOL "")
set(ENABLE_CTEST             OFF  CACHE BOOL "")

set(SKIP_GLSLANG_INSTALL     ON   CACHE BOOL "")

#includes
include_directories(ThirdParty/glm)
include_directories(ThirdParty/glfw)
#include_directories(ThirdParty/glslang)
include_directories(ThirdParty/SPIRV-Reflect)
include_directories(ThirdParty/SPIRV-Reflect/include)
include_directories(ThirdParty/stb)
include_directories(ThirdParty/VulkanMemoryAllocator/include)
include_directories(ThirdParty/imgui)
include_directories(ThirdParty/imgui/backends)
include_directories(ThirdParty/fastgltf/include)
include_directories(ThirdParty/tinygltf)
add_subdirectory(ThirdParty)

#shared lib
add_subdirectory(shared)

##
add_subdirectory(chapter_0)
add_subdirectory(chapter_1)
add_subdirectory(chapter_2)
add_subdirectory(chapter_3)
add_subdirectory(chapter_4)
add_subdirectory(chapter_5)