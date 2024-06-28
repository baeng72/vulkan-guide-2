cmake_minimum_required(VERSION 3.6.0)
project(Vulkan-Guide VERSION 0.1.0 LANGUAGES C CXX)

find_package(Vulkan)
include_directories(AFTER ${Vulkan_INCLUDE_DIRS})

#includes
include_directories(ThirdParty/glm)
include_directories(ThirdParty/glfw)
include_directories(ThirdParty/stb)
include_directories(ThirdParty/VulkanMemoryAllocator/include)
add_subdirectory(ThirdParty)

#shared lib
add_subdirectory(shared)

##
add_subdirectory(chapter_0)
add_subdirectory(chapter_1)