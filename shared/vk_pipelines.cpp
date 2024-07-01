#include <vk_pipelines.h>
#include <vk_initializers.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <sstream>
#include <fstream>

#if defined(NDEBUG)
#pragma comment(lib,"glslang.lib")
#pragma comment(lib,"OGLCompiler.lib")
#pragma comment(lib,"MachineIndependent.lib")
#pragma comment(lib,"OSDependent.lib")
#pragma comment(lib,"GenericCodeGen.lib")
#pragma comment(lib,"SPIRV.lib")
#pragma comment(lib,"SPIRV-Tools-opt.lib")
#pragma comment(lib,"SPIRV-Tools.lib")

#else
#pragma comment(lib,"../ThirdParty/glslang/glslang/debug/glslangd.lib")
//#pragma comment(lib,"../ThirdParty/glslang/glslang/debug/OGLCompilerd.lib")
#pragma comment(lib,"../ThirdParty/glslang/glslang/debug/MachineIndependentd.lib")
#pragma comment(lib,"../ThirdParty/glslang/glslang/OSDependent/Windows/debug/OSDependentd.lib")
#pragma comment(lib,"../ThirdParty/glslang/glslang/debug/GenericCodeGend.lib")
#pragma comment(lib,"../ThirdParty/glslang/SPIRV/DEBUG/SPIRVd.lib")
#endif

//https://lxjk.github.io/2020/03/10/Translate-GLSL-to-SPIRV-for-Vulkan-at-Runtime.html
struct SpirvHelper
{
	static void Init() {
		glslang::InitializeProcess();
	}

	static void Finalize() {
		glslang::FinalizeProcess();
	}

	static void InitResources(TBuiltInResource& Resources) {
		Resources.maxLights = 32;
		Resources.maxClipPlanes = 6;
		Resources.maxTextureUnits = 32;
		Resources.maxTextureCoords = 32;
		Resources.maxVertexAttribs = 64;
		Resources.maxVertexUniformComponents = 4096;
		Resources.maxVaryingFloats = 64;
		Resources.maxVertexTextureImageUnits = 32;
		Resources.maxCombinedTextureImageUnits = 80;
		Resources.maxTextureImageUnits = 32;
		Resources.maxFragmentUniformComponents = 4096;
		Resources.maxDrawBuffers = 32;
		Resources.maxVertexUniformVectors = 128;
		Resources.maxVaryingVectors = 8;
		Resources.maxFragmentUniformVectors = 16;
		Resources.maxVertexOutputVectors = 16;
		Resources.maxFragmentInputVectors = 15;
		Resources.minProgramTexelOffset = -8;
		Resources.maxProgramTexelOffset = 7;
		Resources.maxClipDistances = 8;
		Resources.maxComputeWorkGroupCountX = 65535;
		Resources.maxComputeWorkGroupCountY = 65535;
		Resources.maxComputeWorkGroupCountZ = 65535;
		Resources.maxComputeWorkGroupSizeX = 1024;
		Resources.maxComputeWorkGroupSizeY = 1024;
		Resources.maxComputeWorkGroupSizeZ = 64;
		Resources.maxComputeUniformComponents = 1024;
		Resources.maxComputeTextureImageUnits = 16;
		Resources.maxComputeImageUniforms = 8;
		Resources.maxComputeAtomicCounters = 8;
		Resources.maxComputeAtomicCounterBuffers = 1;
		Resources.maxVaryingComponents = 60;
		Resources.maxVertexOutputComponents = 64;
		Resources.maxGeometryInputComponents = 64;
		Resources.maxGeometryOutputComponents = 128;
		Resources.maxFragmentInputComponents = 128;
		Resources.maxImageUnits = 8;
		Resources.maxCombinedImageUnitsAndFragmentOutputs = 8;
		Resources.maxCombinedShaderOutputResources = 8;
		Resources.maxImageSamples = 0;
		Resources.maxVertexImageUniforms = 0;
		Resources.maxTessControlImageUniforms = 0;
		Resources.maxTessEvaluationImageUniforms = 0;
		Resources.maxGeometryImageUniforms = 0;
		Resources.maxFragmentImageUniforms = 8;
		Resources.maxCombinedImageUniforms = 8;
		Resources.maxGeometryTextureImageUnits = 16;
		Resources.maxGeometryOutputVertices = 256;
		Resources.maxGeometryTotalOutputComponents = 1024;
		Resources.maxGeometryUniformComponents = 1024;
		Resources.maxGeometryVaryingComponents = 64;
		Resources.maxTessControlInputComponents = 128;
		Resources.maxTessControlOutputComponents = 128;
		Resources.maxTessControlTextureImageUnits = 16;
		Resources.maxTessControlUniformComponents = 1024;
		Resources.maxTessControlTotalOutputComponents = 4096;
		Resources.maxTessEvaluationInputComponents = 128;
		Resources.maxTessEvaluationOutputComponents = 128;
		Resources.maxTessEvaluationTextureImageUnits = 16;
		Resources.maxTessEvaluationUniformComponents = 1024;
		Resources.maxTessPatchComponents = 120;
		Resources.maxPatchVertices = 32;
		Resources.maxTessGenLevel = 64;
		Resources.maxViewports = 16;
		Resources.maxVertexAtomicCounters = 0;
		Resources.maxTessControlAtomicCounters = 0;
		Resources.maxTessEvaluationAtomicCounters = 0;
		Resources.maxGeometryAtomicCounters = 0;
		Resources.maxFragmentAtomicCounters = 8;
		Resources.maxCombinedAtomicCounters = 8;
		Resources.maxAtomicCounterBindings = 1;
		Resources.maxVertexAtomicCounterBuffers = 0;
		Resources.maxTessControlAtomicCounterBuffers = 0;
		Resources.maxTessEvaluationAtomicCounterBuffers = 0;
		Resources.maxGeometryAtomicCounterBuffers = 0;
		Resources.maxFragmentAtomicCounterBuffers = 1;
		Resources.maxCombinedAtomicCounterBuffers = 1;
		Resources.maxAtomicCounterBufferSize = 16384;
		Resources.maxTransformFeedbackBuffers = 4;
		Resources.maxTransformFeedbackInterleavedComponents = 64;
		Resources.maxCullDistances = 8;
		Resources.maxCombinedClipAndCullDistances = 8;
		Resources.maxSamples = 4;
		Resources.maxMeshOutputVerticesNV = 256;
		Resources.maxMeshOutputPrimitivesNV = 512;
		Resources.maxMeshWorkGroupSizeX_NV = 32;
		Resources.maxMeshWorkGroupSizeY_NV = 1;
		Resources.maxMeshWorkGroupSizeZ_NV = 1;
		Resources.maxTaskWorkGroupSizeX_NV = 32;
		Resources.maxTaskWorkGroupSizeY_NV = 1;
		Resources.maxTaskWorkGroupSizeZ_NV = 1;
		Resources.maxMeshViewCountNV = 4;
		Resources.limits.nonInductiveForLoops = 1;
		Resources.limits.whileLoops = 1;
		Resources.limits.doWhileLoops = 1;
		Resources.limits.generalUniformIndexing = 1;
		Resources.limits.generalAttributeMatrixVectorIndexing = 1;
		Resources.limits.generalVaryingIndexing = 1;
		Resources.limits.generalSamplerIndexing = 1;
		Resources.limits.generalVariableIndexing = 1;
		Resources.limits.generalConstantMatrixVectorIndexing = 1;
	}

	static EShLanguage FindLanguage(const VkShaderStageFlagBits shader_type) {
		switch (shader_type) {
		case VK_SHADER_STAGE_VERTEX_BIT:
			return EShLangVertex;
		case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
			return EShLangTessControl;
		case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
			return EShLangTessEvaluation;
		case VK_SHADER_STAGE_GEOMETRY_BIT:
			return EShLangGeometry;
		case VK_SHADER_STAGE_FRAGMENT_BIT:
			return EShLangFragment;
		case VK_SHADER_STAGE_COMPUTE_BIT:
			return EShLangCompute;
		default:
			return EShLangVertex;
		}
	}

	static bool GLSLtoSPV(const VkShaderStageFlagBits shader_type, const char* pshader, std::vector<u32>& spirv, bool isVulkan) {

		EShLanguage stage = FindLanguage(shader_type);
		glslang::TShader shader(stage);
		glslang::TProgram program;
		const char* shaderStrings[1];
		TBuiltInResource Resources = {};
		InitResources(Resources);

		// Enable SPIR-V and Vulkan rules when parsing GLSL
		EShMessages messages;
		if(isVulkan)
			messages= (EShMessages)(EShMsgSpvRules | EShMsgBuiltinSymbolTable | EShMsgVulkanRules);
		else
			messages = (EShMessages)(EShMsgSpvRules );

		shaderStrings[0] = pshader;
		shader.setStrings(shaderStrings, 1);
		glslang::EShClient client = isVulkan ? glslang::EShClient::EShClientVulkan : glslang::EShClient::EShClientOpenGL;
		glslang::EshTargetClientVersion version =  glslang::EshTargetClientVersion::EShTargetVulkan_1_2;
		shader.setEnvInput(glslang::EShSource::EShSourceGlsl, stage, client, 100);
		shader.setEnvClient(client, version);
		shader.setEnvTarget(glslang::EShTargetLanguage::EShTargetSpv, glslang::EShTargetLanguageVersion::EShTargetSpv_1_0);
		if (!shader.parse(&Resources, 100, false, messages)) {
			std::string info = shader.getInfoLog();
			if (info.substr(0, 6) == "ERROR:") {
				std::string remainder = info.substr(7, info.length() - 7);
				int lineNo = 0;
				int colNo = 0;
				std::string lineColStr = remainder.substr(0,remainder.find('\'')-2);
				size_t pos = lineColStr.find(':');
				if (pos != std::string::npos) {
					std::string lineStr = lineColStr.substr(0, pos);
					std::string colStr = lineColStr.substr(pos + 1, lineColStr.length() - pos - 1);
					lineNo = std::atoi(lineStr.c_str());
					colNo = std::atoi(colStr.c_str());
					std::string errorLine = shader.getInfoDebugLog();
					puts(info.c_str());
					int i = 0;
					std::stringstream str(pshader);
					std::string prevLine;
					std::string errLine;
					do {
						prevLine = errLine;
						std::getline(str, errLine);
						i++;
					} while (i < colNo);
					char buffer[256];
					if (colNo > 0) {
						sprintf_s(buffer, "Line %d: %s", colNo - 1, prevLine.c_str());
						puts(buffer);
					}
					sprintf_s(buffer, "Line %d: %s", colNo, errLine.c_str());
					puts(buffer);
				}


			}else
				puts(shader.getInfoLog());
			//puts(shader.getInfoDebugLog());
			return false;  // something didn't work
		}


		
		program.addShader(&shader);

		//
		// Program-level processing...
		//

		if (!program.link(messages)) {
			//puts(shader.getInfoLog());
			puts(shader.getInfoDebugLog());
			fflush(stdout);
			return false;
		}
		

		glslang::GlslangToSpv(*program.getIntermediate(stage), spirv);
		return true;
	}
};

class ShaderCompiler {
public:
	ShaderCompiler();
	~ShaderCompiler();
	std::vector<uint32_t> compileShader(const char* shaderSrc, VkShaderStageFlagBits shaderStage, bool isVulkan=true);
};


ShaderCompiler::ShaderCompiler()
{
	SpirvHelper::Init();
}
ShaderCompiler::~ShaderCompiler()
{
	SpirvHelper::Finalize();
}
std::vector<u32> ShaderCompiler::compileShader(const char* shaderSrc, VkShaderStageFlagBits shaderStage,bool isVulkan)
{
	std::vector<u32> spirv;
	SpirvHelper::GLSLtoSPV(shaderStage, shaderSrc, spirv,isVulkan);
	return spirv;
}

namespace vkutil{
    bool compile_shader_module(ccharp filePath, VkDevice device, VkShaderStageFlagBits stage, VkShaderModule* outShaderModule){

        FILE * fp;
        fopen_s(&fp, filePath, "rb");
        if(fp){
            fseek(fp,0,SEEK_END);
            size_t fileSize = ftell(fp);
            fseek(fp,0,SEEK_SET);
            std::vector<i8> buffer(fileSize / sizeof(i8) + sizeof(u32));
            bool ok = fread(buffer.data(),1,fileSize,fp)==fileSize;
            buffer[fileSize] = 0;
            fclose(fp);

            if(ok){
                ShaderCompiler compiler;
                auto spirv = compiler.compileShader((ccharp)buffer.data(),stage,true);

                if(spirv.size()==0){
                    return false;
                }

                //Create a new shader module, using the spirv
                VkShaderModuleCreateInfo createInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
                size_t codeSize = spirv.size() * sizeof(u32);
                createInfo.codeSize = codeSize;
                createInfo.pCode = spirv.data();
                {
                    char buffer[256];
                    strcpy_s(buffer,filePath);
                    strcat_s(buffer,".spv");
                    FILE * fp;
                    fopen_s(&fp,buffer,"wb");
                    if(fp){
                        bool ok = fwrite(spirv.data(),sizeof(u32),spirv.size(),fp)==spirv.size();
                        assert(ok);
                        fclose(fp);
                    }
                    // std::ofstream outfile(buffer, std::ios::ate | std::ios::binary);
                    // if(outfile.good()){
                    //     outfile.write(reinterpret_cast<char*>(spirv.data()), codeSize);
                    //     outfile.close();
                    // }
                }
                //check that the creation goes well
                VkShaderModule shaderModule;
                if(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS){
                    return false;
                }
                *outShaderModule = shaderModule;
                return true;
            }


        }
        return false;
    }

    bool load_shader_module(ccharp filePath, VkDevice device, VkShaderModule* outShaderModule){
        FILE * fp;
        fopen_s(&fp, filePath, "rb");
        if(fp){
            fseek(fp,0,SEEK_END);
            size_t fileSize = ftell(fp);
            fseek(fp,0,SEEK_SET);
            std::vector<u32> buffer(fileSize / sizeof(u32));
            bool ok = fread(buffer.data(),sizeof(u8),fileSize,fp)==fileSize;
            fclose(fp);
            //create a new shader module, using the buffer we loaded
            VkShaderModuleCreateInfo createInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
            createInfo.codeSize = buffer.size() * sizeof(u32);
            createInfo.pCode = buffer.data();

            VkShaderModule shaderModule;
            if(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule)!=VK_SUCCESS){
                return false;
            }
            *outShaderModule = shaderModule;
            return true;
        }
        return false;
        
    }

    
}

void PipelineBuilder::clear(){
        //clear all structs we need back to 0 with there correct stype
        _inputAssembly = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
        _rasterizer = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        _colorBlendAttachment = {};
        _multisampling = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
        _pipelineLayout = VK_NULL_HANDLE;
        _depthStencil = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
        _renderInfo = {VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
        _shaderStages.clear();

    }

    void PipelineBuilder::set_shaders(VkShaderModule vertexShader, VkShaderModule fragmentShader){
        _shaderStages.clear();

        _shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertexShader));
        _shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader));
    }


    void PipelineBuilder::set_input_topology(VkPrimitiveTopology topology){
        _inputAssembly.topology = topology;
        //we are not going to use primitive restart on the entire tutorial so leave it false
        _inputAssembly.primitiveRestartEnable = VK_FALSE;
    }

    void PipelineBuilder::set_polygon_mode(VkPolygonMode mode){
        _rasterizer.polygonMode = mode;
        _rasterizer.lineWidth = 1.f;
    }

    void PipelineBuilder::set_cull_mode(VkCullModeFlags cullMode, VkFrontFace frontFace){
        _rasterizer.cullMode = cullMode;
        _rasterizer.frontFace = frontFace;
    }

    void PipelineBuilder::set_multisampling_none(){
        _multisampling.sampleShadingEnable = VK_FALSE;
        //multisampling defaulted to no multisampling (1 sampler per pixel)
        _multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        _multisampling.minSampleShading = 1.f;
        _multisampling.pSampleMask = nullptr;
        //no alpha to coverage either
        _multisampling.alphaToCoverageEnable = VK_FALSE;
        _multisampling.alphaToOneEnable = VK_FALSE;
    }

    void PipelineBuilder::disable_blending(){
        //default write mask
        _colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        //no blending
        _colorBlendAttachment.blendEnable = VK_FALSE;
    }

	void PipelineBuilder::enable_blending_additive(){
		_colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		_colorBlendAttachment.blendEnable = VK_TRUE;
		_colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		_colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
		_colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		_colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		_colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		_colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	}

	void PipelineBuilder::enable_blending_alphablend(){
		_colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		_colorBlendAttachment.blendEnable = VK_TRUE;
		_colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		_colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		_colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		_colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		_colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	}

    void PipelineBuilder::set_color_attachment_format(VkFormat format){
        _colorAttachmentFormat = format;
        //connect the format to the renderInfo structure
        _renderInfo.colorAttachmentCount = 1;
        _renderInfo.pColorAttachmentFormats = &_colorAttachmentFormat;
    }

    void PipelineBuilder::set_depth_format(VkFormat format){
        _renderInfo.depthAttachmentFormat = format;
    }

    void PipelineBuilder::disable_depthtest(){
        _depthStencil.depthTestEnable = VK_FALSE;
        _depthStencil.depthWriteEnable = VK_FALSE;
        _depthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
        _depthStencil.depthBoundsTestEnable = VK_FALSE;
        _depthStencil.stencilTestEnable = VK_FALSE;
        _depthStencil.front = {};
        _depthStencil.back = {};
        _depthStencil.minDepthBounds = 0.f;
        _depthStencil.maxDepthBounds = 1.f;

    }

    void PipelineBuilder::enable_depthtest(bool depthWriteEnable, VkCompareOp op){
        _depthStencil.depthTestEnable = VK_TRUE;
        _depthStencil.depthWriteEnable = depthWriteEnable;
        _depthStencil.depthCompareOp = op;
        _depthStencil.depthBoundsTestEnable = VK_FALSE;
        _depthStencil.stencilTestEnable = VK_FALSE;
        _depthStencil.front = {};
        _depthStencil.back = {};
        _depthStencil.minDepthBounds = 0.f;
        _depthStencil.maxDepthBounds = 1.f;
    }

    VkPipeline PipelineBuilder::build_pipeline(VkDevice device){
        //make viewport state from our stored viewport and scissor.
        //at the moment we won't support multiple viewports or scissors
        VkPipelineViewportStateCreateInfo viewportState{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        //setup dummy color blending. We aren't using transparent objects yet
        //the blending is just "no blend", but we do write to the color attachment
        VkPipelineColorBlendStateCreateInfo colorBlending{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &_colorBlendAttachment;

        //completely clear VertexInputStateCreateInfo, as we have no need for it
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};

        //build pipeline state
        //we now use all of the info structs we have been writing into this one
        //to create the pipeline
        VkGraphicsPipelineCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
        pipelineInfo.pNext = &_renderInfo;
        
        pipelineInfo.stageCount = (u32)_shaderStages.size();
        pipelineInfo.pStages = _shaderStages.data();
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &_inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &_rasterizer;
        pipelineInfo.pMultisampleState = &_multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDepthStencilState = &_depthStencil;
        pipelineInfo.layout = _pipelineLayout;

        VkDynamicState state[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

        VkPipelineDynamicStateCreateInfo dynamicInfo{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
        dynamicInfo.pDynamicStates = state;
        dynamicInfo.dynamicStateCount = 2;

        pipelineInfo.pDynamicState = &dynamicInfo;

        //it's easy to error out on create graphics pipeline, so we hand it a bit better than VK_CHECK
        VkPipeline pipeline;
        if(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline)!= VK_SUCCESS){
            fmt::println("Faited to create pipelne");
            return VK_NULL_HANDLE;
        }
        return pipeline;
    }