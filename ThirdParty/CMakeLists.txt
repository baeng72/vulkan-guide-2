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

#glslang
set(ENABLE_SPVREMAPPER OFF)
set(ENABLE_GLSLANG_BINARIES OFF)
set(ENABLE_HSLS OFF)
set(BUILD_TESTING OFF)
set(BUILD_EXTERNAL OFF)
set(BUILD_SHARED_LIBS OFF)
set(ENABLE_OPT OFF)
set(GLSLANG_ENABLE_INSTALL OFF)

add_subdirectory(glslang)

if (NOT TARGET glslang-default-resource-limits)
    add_library(glslang-default-resource-limits
                glslang/StandAlone/ResourceLimits.cpp)

    set_target_properties(glslang-default-resource-limits PROPERTIES FOLDER "ThirdParty" POSITION_INDEPENDENT_CODE ON)

    target_include_directories(glslang-default-resource-limits
            PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/glslang)
endif()

if(NOT MSVC)
    target_compile_options(glslang PRIVATE
        "-Wno-logical-op-parentheses"
        "-Wno-unused-parameter")

    target_compile_options(SPIRV PRIVATE
        "-Wno-logical-op-parentheses"
        "-Wno-unused-parameter")
endif()


#fast gltf
add_subdirectory(fastgltf)



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

#glfw
add_subdirectory(glfw)

#SPIRV-Reflect
set(SPIRV_REFLECT_EXECUTABLE OFF)
set(SPIRV_REFLECT_EXAMPLES OFF)
set(SPIRV_REFLECT_STATIC_LIB ON)
add_subdirectory(SPIRV-Reflect)

#imgui
#add_subdirectory(imgui STATIC)

add_library(imgui STATIC)

target_sources(imgui PRIVATE 
  imgui/imgui.h
  imgui/imgui.cpp

  imgui/imgui_demo.cpp
  imgui/imgui_draw.cpp
  imgui/imgui_widgets.cpp
  imgui/imgui_tables.cpp
  imgui/backends/imgui_impl_vulkan.cpp
  imgui/backends/imgui_impl_glfw.cpp
  )

target_link_libraries(imgui PUBLIC glfw)

#both vma and glm and header only libs so we only need the include path
target_include_directories(VulkanMemoryAllocator INTERFACE VulkanMemoryAllocator)

target_include_directories(glm INTERFACE glm)