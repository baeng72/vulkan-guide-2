#include <stb_image.h>
#include <iostream>
#include "vk_loader.h"

#include "vk_engine.h"
#include "vk_initializers.h"
#include "vk_types.h"
#include <glm/gtc/quaternion.hpp>
#include <fmt/core.h>
#include <glm/ext.hpp>

//#define __USE__FASTGLTF
#if defined __USE__FASTGLTF
#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/glm_element_traits.hpp>
//#include <fastgltf/parser.hpp>
#else
//#define TINYGLTF_NO_STB_IMAGE
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

#if defined(__USE__FASTGLTF)
VkFilter extract_filter(fastgltf::Filter filter){
    switch(filter){
        //nearest samplers
        case fastgltf::Filter::Nearest;
        case fastgltf::Filter::NearestMipMapNearest;
        case fastgltf::Filter::NearestMipMapLinear:
            return VK_FILTER_NEAREST;
        //linear samplers
        case fastgltf::Filter::Linear:
        case fastgltf::Filter::LinearMipMapNearest:
        case fastgltf::Filter::MipMapLinear:
        default:
            return VK_FILTER_LINEAR;
    }
}
VkSamplerMipmapMode extract_mipmap_mode(fastgltf::Filter filter){
    switch(filter){
        case fastgltf::Filter::NearestMipMapNearest:
        case fastgltf::Filter::LinearMipMapNearest:
            return VK_SAMPLER_MIPMAP_MODE_NEAREST;
        case fastgltf::Filter::NearestMipMapLinear:
        case fastgltf::Filter::LinearMipMapLinear:
        default:
            return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    }
}
#else
VkFilter extract_filter(int filter){
    switch(filter){
        case TINYGLTF_TEXTURE_FILTER_NEAREST:
        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
            return VK_FILTER_NEAREST;
        case TINYGLTF_TEXTURE_FILTER_LINEAR:
        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
        default:
            return VK_FILTER_LINEAR;

    }
    return VK_FILTER_MAX_ENUM;
}
VkSamplerMipmapMode extract_mipmap_mode(int filter){
    switch(filter){
        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
            return VK_SAMPLER_MIPMAP_MODE_NEAREST;
        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
        default:
            return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    }
    return VK_SAMPLER_MIPMAP_MODE_MAX_ENUM;
}


#endif

std::optional<std::shared_ptr<LoadedGLTF>> loadGltf(VulkanEngine * pengine, std::string_view filePath){
    fmt::print("LOading GLTF: {}", filePath);

    std::shared_ptr<LoadedGLTF> scene = std::make_shared<LoadedGLTF>();

    scene->creator = pengine;
    LoadedGLTF& file = *scene.get();
#if defined (__USE__FASTGLTF)
    fastgltf::Parser parser{};

    constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetNumber | fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers;

    fastgltf::GltfDataBuffer data;
    data.loadFromFile(filePath);
    fastgltf::Asset gltf;

    std::filesystem::path path = filePath;

    auto type = fastgltf::determineGltfFileType(&data);
    if(type == fastgltf::GtlfType::glTF){
        auto load = parser.loadGLTF(&dat, path.parent_path(), gltfOptions);
        if(load){
            gltf = std::move(load.get());
        }else{
            std::cerr << "Failed to load glTF: " << fastgltf::to_underlying(load.error()) << std::endl;
            return {};
        }
    }else if(type == fastgltf::GltfType::GLB){
        auto load = parser.loadBinaryGLTF(&data, path.parent_path(), gltfOptions);
        if(load){
            gltf = std::move(load.get());
        }else{
            std::cerr << "Failed to load glTF: " << fastgltf::to_underlying(load.error()) << std::endl;
            return {};
        }
    }else{
        std::cerr << "Failed to determine glTF container" << std::endl;
        return {};
    }
    //we can estimate the descriptors we will need accurately
    std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,3},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,3},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1}
    };
    file.descriptorPool.init(pengine->_device, gltf.materials.size(), sizes);

    for(fastgltf::Sampler& sampler : gltf.samplers){
        VkSamplerCreateInfo sampl{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        sampl.maxLod = VK_LOD_CLAMP_NONE;
        sampl.minLod = 0;
        sampl.magFilter = extract_filter(sampler.magFilter.value_or(fastgltf::Filter::Nearest));
        sampl.minFilter = extract_filter(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

        sampl.mipmapMode = extract_mipmap_mode(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

        VkSampler newSampler;
        vkCreateSampler(pengine->_device, &sampl, nullptr, &newSampler);
        file.samplers.push_back(newSampler);
    }

    //temporal arrays for all the objects to use while creating the GLTF data
    std::vector<std::shared_ptr<MeshAsset>> meshes;
    std::vector<std::shared_ptr<Node>> nodes;
    std::vector<AllocatedImage> images;
    std::vector<std::shared_ptr<GLTFMaterial>> materials;
    //load all textures
    for(fastgltf::Image&image : gltf.images){
        images.push_back(pengine->_errorCheckerboardImage);
    }
    //create buffer to hote the material data
    file.materialDataBuffer = pengine->create_buffer(sizeof(GLTFMetallic_Roughness::MaterialConstants)* gltf.materials.size(),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    i32 data_index=0;
    GLTFMetallic_Roughness::MaterialConstants * sceneMaterialConstants = (GLTFMetallic_Roughness::MaterialConstants*)file.materialDataBuffer.info.pMappedData;
    for(fastgltf::Material& mat : gltf.materials){
        std::shared_ptr<GLTFMaterial> newMat = std::make_shared<GLTFMaterial>();
        materials.push_back(newMat);
        file.materials[mat.name.c_str()] = newMat;

        GLTFMetallic_Roughness::MaterialConstants constants;
        constants.colorFactors = mat.pbrData.baseColorFactor;
        constants.metalRoughFactor.x = mat.pbrData.metallicFactor;
        constants.metalRoughFactor.y = mat.pbrData.roughnessFactor;
        //write material parameters to buffer
        sceneMaterialConstants[data_index] = constants;

        MaterialPass passType = MaterialPass::MainColor;
        if(mat.alphaMode == fastgltf::AlphaMode::Blend){
            passType = MaterialPass::Transparent;
        }

        GLTFMetallic_Roughness::MaterialResources materialResources;
        //default the material textures
        materialResources.colorImage = pengine->_whiteImage;
        materialResources.colorSampler = pengine->_defaultSamplerLinear;
        materialResources.metalRoughImage = pengine->_whiteImage;
        materialResources.metalRoughSampler = pengine->_defaultSamplerLinar;

        //set the uniform buffer for the material data
        materialResources.dataBuffer = file.materialDataBuffer.buffer;
        materialResources.dataBufferOffset = data_index * sizeof(GLTFMetallic_Roughness::MaterialConstants);
        //grab textures from gltf file
        if(mat.pbrData.baseColorTexture.has_value()){
            size_t img = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].imageIndex.value();
            size_t sampler = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].samplerIndex.value();

            materialResources.colorImage = images[img];
            materialResources.colorSampler = file.samplers[sampler];            
        }
        //build material
        newMat->data = pengine->metalRoughMaterial.write_material(pengine->_device, passType, materialResources, file.descriptorPool);

        data_index++;
    }
    //use the same vectors for all meshes so that the memory doesn't reallocate often
    std::vector<u32> indices;
    std::vector<Vertex> vertices;
    for(fastgltf::Mesh& mesh : gltf.meshes){
        std::shared_ptr<MeshAsset> newmesh = std::make_shared<MeshAsset>();
        meshes.push_back(newmesh);
        file.meshes[mesh.name.c_str()] = newmesh;
        newmesh->name - mesh.name;

        //clear the mesh arrays each mesh, we don't want to merge them by error
        indices.clear();
        vertices.clear();
    }
#else
    tinygltf::TinyGLTF loader;
    tinygltf::Model gltf;
    std::string err;
    std::string warn;

    std::filesystem::path path = filePath;

    bool res=false;

    auto ext = path.extension();
    
    std::string pathstr = path.string();
    ccharp pathtoload = pathstr.c_str();
    if(ext == ".glb")
        res = loader.LoadBinaryFromFile(&gltf, &err, &warn, pathtoload);
    else
        res = loader.LoadASCIIFromFile(&gltf, &err, &warn, pathtoload);
    if(res){

    }else{
        std::cerr << "Failed to load glTF: " << warn << ", " << err << std::endl;
        return {};
    }
    //we can estimate the descriptors we will need accurately
    std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,3},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,3},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1}
    };
    file.descriptorPool.init(pengine->_device, (u32)gltf.materials.size(),sizes);

    for(auto & sampler : gltf.samplers){
        VkSamplerCreateInfo sampl{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        sampl.maxLod = VK_LOD_CLAMP_NONE;
        sampl.minLod = 0;
        sampl.magFilter = extract_filter(sampler.magFilter);
        sampl.minFilter = extract_filter(sampler.minFilter);
        sampl.mipmapMode = extract_mipmap_mode(sampler.minFilter);
        VkSampler newSampler;
        vkCreateSampler(pengine->_device, &sampl, nullptr, &newSampler);
        file.samplers.push_back(newSampler);
    }
    //temporal arrays for all the objects to use while creating the GLTF data
    std::vector<std::shared_ptr<MeshAsset>> meshes;
    std::vector<std::shared_ptr<Node>> nodes;
    std::vector<AllocatedImage> images;
    std::vector<std::shared_ptr<GLTFMaterial>> materials;
    for(auto&image : gltf.images){
        //images.push_back(pengine->_errorCheckerboardImage);
        VkExtent3D imageSize;
        imageSize.width = image.width;
        imageSize.height = image.height;
        imageSize.depth = 1;

        AllocatedImage newImage = pengine->create_image(image.image.data(), imageSize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);
        if(newImage.image == VK_NULL_HANDLE){
            images.push_back(pengine->_errorCheckerboardImage);
            fmt::print("Failed to load GLTF texture");
        }else{
            images.push_back(newImage);
            file.images[image.name.c_str()]= newImage;
        }
    }
    //create buffer to hote the material data
    file.materialDataBuffer = pengine->create_buffer(sizeof(GLTFMetallic_Roughness::MaterialConstants)* gltf.materials.size(),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    i32 data_index=0;
    GLTFMetallic_Roughness::MaterialConstants * sceneMaterialConstants = (GLTFMetallic_Roughness::MaterialConstants*)file.materialDataBuffer.allocationInfo.pMappedData;

    for(auto& mat : gltf.materials){
        std::shared_ptr<GLTFMaterial> newMat = std::make_shared<GLTFMaterial>();
        materials.push_back(newMat);
        file.materials[mat.name.c_str()] =  newMat;

        GLTFMetallic_Roughness::MaterialConstants constants;
        constants.colorFactor = vec4(mat.pbrMetallicRoughness.baseColorFactor[0],mat.pbrMetallicRoughness.baseColorFactor[1],
            mat.pbrMetallicRoughness.baseColorFactor[2], mat.pbrMetallicRoughness.baseColorFactor[3]);
        constants.metalRoughnessFactor.x = (float)mat.pbrMetallicRoughness.metallicFactor;
        constants.metalRoughnessFactor.y = (float)mat.pbrMetallicRoughness.roughnessFactor;
        //write material parameters to buffer
        sceneMaterialConstants[data_index] = constants;

        MaterialPass passType = MaterialPass::MainColor;
        if(mat.alphaMode == "BLEND"){
            passType = MaterialPass::Transparent;
        }

        GLTFMetallic_Roughness::MaterialResources materialResources;
        //default the material textures
        materialResources.colorImage = pengine->_whiteImage;
        materialResources.colorSampler = pengine->_defaultSamplerLinear;
        materialResources.metalRoughImage = pengine->_whiteImage;
        materialResources.metalRoughSampler = pengine->_defaultSamplerLinear;

        //set the uniform buffer for the material data
        materialResources.dataBuffer = file.materialDataBuffer.buffer;
        materialResources.dataBufferOffset = data_index * sizeof(GLTFMetallic_Roughness::MaterialConstants);
        //grab textures from gltf file
        if(mat.pbrMetallicRoughness.baseColorTexture.index>=0){
            size_t img = gltf.textures[mat.pbrMetallicRoughness.baseColorTexture.index].source;
            size_t sampler = gltf.textures[mat.pbrMetallicRoughness.baseColorTexture.index].sampler;

            materialResources.colorImage = images[img];
            materialResources.colorSampler = file.samplers[sampler];
        }
        //build material
        newMat->data = pengine->metalRoughMaterial.write_material(pengine->_device, passType, materialResources, file.descriptorPool);

        data_index++;
    }
    //use the same vectors for all meshes so that the memory doesn't reallocate often
    std::vector<u32> indices;
    std::vector<Vertex> vertices;
    for(auto& mesh : gltf.meshes){
        std::shared_ptr<MeshAsset> newmesh = std::make_shared<MeshAsset>();
        meshes.push_back(newmesh);
        file.meshes[mesh.name.c_str()] = newmesh;
        newmesh->name = mesh.name;

        for(auto&& p : mesh.primitives){
            GeoSurface newSurface;
            newSurface.startIndex = (u32)indices.size();
            newSurface.count = (u32)gltf.accessors[p.indices].count;

            u32 initial_vtx = (u32)vertices.size();
            //load indexes
            {
                const tinygltf::Accessor& accessor = gltf.accessors[p.indices];
                const tinygltf::BufferView& view = gltf.bufferViews[accessor.bufferView];
                newSurface.count = (u32)accessor.count;
                switch(accessor.componentType){
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:{
                        u16*ptr = (u16*)(gltf.buffers[view.buffer].data.data() + (accessor.byteOffset + view.byteOffset));
                        for(size_t i = 0; i < accessor.count; ++i){
                            indices.push_back(ptr[i]+initial_vtx);
                        }
                    }
                        break;
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:{
                        u32*ptr = (u32*)(gltf.buffers[view.buffer].data.data() + (accessor.byteOffset + view.byteOffset));
                        for(size_t i = 0; i < accessor.count; ++i){
                            indices.push_back(ptr[i]+initial_vtx);
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
                const tinygltf::Accessor& accessor = gltf.accessors[attr];
                const tinygltf::BufferView& view = gltf.bufferViews[accessor.bufferView];
                vec3*ptr = (vec3*)(gltf.buffers[view.buffer].data.data() + (accessor.byteOffset + view.byteOffset));
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
                //assert(p.attributes.find("NORMAL") != p.attributes.end());
                auto attr = p.attributes["NORMAL"];
                if(attr >= 0){
                    const tinygltf::Accessor& accessor = gltf.accessors[attr];
                    const tinygltf::BufferView& view = gltf.bufferViews[accessor.bufferView];
                    vec3*ptr = (vec3*)(gltf.buffers[view.buffer].data.data() + (accessor.byteOffset + view.byteOffset));
                    for(size_t i=0; i < accessor.count; i++){
                        vertices[initial_vtx+i].normal = *ptr++;                        
                    }
                }
            }
            //load UVs
            {
                //assert(p.attributes.find("TEXCOORD_0") != p.attributes.end());
                if(p.attributes.find("TEXCOORD_0")!=p.attributes.end()){
                    auto attr = p.attributes["TEXCOORD_0"];                
                    const tinygltf::Accessor& accessor = gltf.accessors[attr];
                    const tinygltf::BufferView& view = gltf.bufferViews[accessor.bufferView];
                    vec2*ptr = (vec2*)(gltf.buffers[view.buffer].data.data() + (accessor.byteOffset + view.byteOffset));
                    for(size_t i=0; i < accessor.count; i++){
                        vec2 uv = *ptr++;
                        vertices[initial_vtx+i].uv_x = uv.x;
                        vertices[initial_vtx+i].uv_y = uv.y;                        
                    }                
                }
            }
            //load vertex colors
            {
                //assert(p.attributes.find("COLOR_0") != p.attributes.end());
                if(p.attributes.find("COLOR_0")!=p.attributes.end()){
                    auto attr = p.attributes["COLOR_0"];                
                    const tinygltf::Accessor& accessor = gltf.accessors[attr];
                    const tinygltf::BufferView& view = gltf.bufferViews[accessor.bufferView];
                    vec4 *ptr = (vec4*)(gltf.buffers[view.buffer].data.data() + (accessor.byteOffset + view.byteOffset));
                    for(size_t i=0; i < accessor.count; i++){                    
                        vec4 color = *ptr++;
                        vertices[initial_vtx+i].color = color;                    
                    }                
                }
            }

            if(p.material>=0){
                newSurface.material = materials[p.material];
            }else{
                newSurface.material = materials[0];
            }
            //loop the vertices of this surface, find min/max bounds
            vec3 minpos = vertices[initial_vtx].position;
            vec3 maxpos = vertices[initial_vtx].position;
            for(i32 i=initial_vtx; i < (i32)vertices.size(); ++i){
                minpos = glm::min(minpos, vertices[i].position);
                maxpos = glm::max(maxpos, vertices[i].position);
            }
            //calculate origin and extents from the min/max, use extent length for radius
            newSurface.bounds.origin = (maxpos + minpos) * 0.5f;
            newSurface.bounds.extents = (maxpos - minpos) * 0.5f;
            newSurface.bounds.sphereRadius = glm::length(newSurface.bounds.extents);
            newmesh->surfaces.push_back(newSurface);
        }
        newmesh->meshBuffers = pengine->uploadMesh(indices,vertices);        
    }
    //load all noddes and their meshes
    for(auto& node : gltf.nodes){
        std::shared_ptr<Node> newNode;

        //find if the node has a mesh, and if it does hook it to the mesh pointer and allocate it with the meshnode class
        if(node.mesh>-1){
            newNode = std::make_shared<MeshNode>();
            static_cast<MeshNode*>(newNode.get())->mesh = meshes[node.mesh];            
        }else{
            newNode = std::make_shared<Node>();
        }

        nodes.push_back(newNode);
        file.nodes[node.name.c_str()];
        vec3 tl = vec3(0.f);
        vec3 sc = vec3(1.f);
        quat rot = quat();
        mat4 xform = mat4(1.f);
        if(node.rotation.size()==4)
			rot = glm::make_quat(node.rotation.data());
		if(node.translation.size() == 3)
			tl = glm::make_vec3(node.translation.data());
		if(node.scale.size() == 3)
			sc = glm::make_vec3(node.scale.data());
		if(node.matrix.size() == 16)
			newNode->localTransform = glm::make_mat4x4(node.matrix.data());
        else{
            mat4 tm = glm::translate(mat4(1.f),tl);
            mat4 rm = mat4(rot);
            mat4 sm = glm::scale(mat4(1.f),sc);
            newNode->localTransform = tm * rm * sm;
        }        
    }
    //run loop again to setup transform hierarchy
    for(size_t i=0; i < gltf.nodes.size(); ++i){
        auto& node = gltf.nodes[i];
        std::shared_ptr<Node>& sceneNode = nodes[i];

        for(auto& c : node.children){
            sceneNode->children.push_back(nodes[c]);
            nodes[c]->parent = sceneNode;
        }
    }
    //find the top nodes, with no parents
    for(auto& node : nodes){
        if(node->parent.lock()==nullptr){
            file.topNodes.push_back(node);
            node->refreshTransform(mat4(1.f));
        }
    }
    return scene;
#endif
    

}

void LoadedGLTF::Draw(const mat4 & topMatrix, DrawContext& ctx){
    //create renderables from the scenenodes
    for(auto& n : topNodes){
        n->Draw(topMatrix, ctx);
    }
}

void LoadedGLTF::clearAll(){
    VkDevice device  = creator->_device;

    descriptorPool.destroy_pools(device);
    creator->destroy_buffer(materialDataBuffer);

    for(auto& [k, v] : meshes){
        creator->destroy_buffer(v->meshBuffers.indexBuffer);
        creator->destroy_buffer(v->meshBuffers.vertexBuffer);
    }

    for(auto& [k, v] : images){
        if(v.image == creator->_errorCheckerboardImage.image){
            //done destroy default
            continue;
        }
        creator->destroy_image(v);
    }
    for(auto& sampler : samplers){
        vkDestroySampler(device, sampler, nullptr);

    }
}