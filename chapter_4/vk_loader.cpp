#include <stb_image.h>
#include <iostream>
#include "vk_loader.h"

#include "vk_engine.h"
#include "vk_initializers.h"
#include "vk_types.h"
#include <glm/gtc/quaternion.hpp>
#include <fmt/core.h>

//#define __USE__FASTGLTF
#if defined __USE__FASTGLTF
#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/glm_element_traits.hpp>
//#include <fastgltf/parser.hpp>
#else
#define TINYGLTF_NO_STB_IMAGE
#include <tiny_gltf.h>
#endif

std::optional<std::vector<std::shared_ptr<MeshAsset>>> loadGltfMeshes(VulkanEngine* engine, std::filesystem::path filePath){
    std::cout << "Loading GLTF:" << filePath << std::endl;
#if defined __USE__FASTGLTF
    fastgltf::GltfDataBuffer data;
    data.loadFromFile(filePath);

    constexpr auto gltfOptions = fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers;

    fastgltf::Asset gltf;
    fastgltf::Parser parser{};
    
    auto load = parser.loadBinaryGLTF(&data, filePath.parent_path(), gltfOptions);
    if(load){
        gltf = std::move(load.get());
    }else{
        fmt::print("Failed to load glTF: {} \n", fastgltf::to_underlying(load.error()));
        return {};
    }

#else
    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    std::string err;
    std::string warn;

    auto ext = filePath.extension();
    bool res=false;
    std::string pathstr = filePath.string();
    ccharp path = pathstr.c_str();
    if(ext == ".glb")
        res = loader.LoadBinaryFromFile(&model, &err, &warn, path);
    else
        res = loader.LoadASCIIFromFile(&model, &err, &warn, path);

    std::vector<std::shared_ptr<MeshAsset>> meshes;
    std::vector<u32> indices;
    std::vector<Vertex> vertices;
    if(res){
        for(auto&mesh : model.meshes){
            MeshAsset newMesh;

            newMesh.name = mesh.name;

            //clear the mesh arrays each mesh, we don't want to merge them by error
            indices.clear();
            vertices.clear();

            for(auto&&p : mesh.primitives){
                
                GeoSurface newSurface;
                newSurface.startIndex = (u32)indices.size();
                //newSurface.count = (u32)accessor.count;

                size_t initial_vtx = vertices.size();

                //load indexes
                {
                    const tinygltf::Accessor& accessor = model.accessors[p.indices];
				    const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
                    newSurface.count = (u32)accessor.count;
                    switch(accessor.componentType){
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:{
                            u16*ptr = (u16*)(model.buffers[view.buffer].data.data() + (accessor.byteOffset + view.byteOffset));
                            for(size_t i = 0; i < accessor.count; ++i){
                                indices.push_back(ptr[i]);
                            }
                        }
                            break;
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:{
                            u32*ptr = (u32*)(model.buffers[view.buffer].data.data() + (accessor.byteOffset + view.byteOffset));
                            for(size_t i = 0; i < accessor.count; ++i){
                                indices.push_back(ptr[i]);
                            }
                        }
                            break;
                        default:
                        assert(0);
                            break;
                    }
                }

                //load vertex positions
                {
                    auto attr = p.attributes["POSITION"];
                    const tinygltf::Accessor& accessor = model.accessors[attr];
				    const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
                    vec3*ptr = (vec3*)(model.buffers[view.buffer].data.data() + (accessor.byteOffset + view.byteOffset));
                    vertices.resize(vertices.size() + accessor.count);
                    for(size_t i=0; i < accessor.count; i++){
                        Vertex vtx{};
                        vtx.position = *ptr++;                                           
                        vtx.normal = vec3(1.f, 0.f, 0.f);
                        vtx.color = vec4(1.f);
                        vertices[initial_vtx+i] = vtx;

                    }
                }
                //load vtx normals
                {
                    assert(p.attributes.find("NORMAL") != p.attributes.end());
                    auto attr = p.attributes["NORMAL"];
                    const tinygltf::Accessor& accessor = model.accessors[attr];
				    const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
                    vec3*ptr = (vec3*)(model.buffers[view.buffer].data.data() + (accessor.byteOffset + view.byteOffset));
                    for(size_t i=0; i < accessor.count; i++){
                        vertices[initial_vtx+i].normal = *ptr++;                        
                    }
                }
                //load UVs
                {
                    assert(p.attributes.find("TEXCOORD_0") != p.attributes.end());
                    auto attr = p.attributes["TEXCOORD_0"];
                    const tinygltf::Accessor& accessor = model.accessors[attr];
				    const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
                    vec2*ptr = (vec2*)(model.buffers[view.buffer].data.data() + (accessor.byteOffset + view.byteOffset));
                    for(size_t i=0; i < accessor.count; i++){
                        vec2 uv = *ptr++;
                        vertices[initial_vtx+i].uv_x = uv.x;
                        vertices[initial_vtx+i].uv_y = uv.y;                        
                    }
                }
                newMesh.surfaces.push_back(newSurface);
            }

            constexpr bool OverrideColors = false;

            if(OverrideColors){
                for(Vertex&vtx : vertices){
                    vtx.color = glm::vec4(vtx.normal, 1.f);
                }
            }

            newMesh.meshBuffers = engine->uploadMesh(indices, vertices);
            meshes.emplace_back(std::make_shared<MeshAsset>(std::move(newMesh)));
        }

        
    }        
    return meshes;
#endif
}