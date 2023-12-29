#include "RenderProgs.h"
#include "../framework/Filesystem.h"
#include "../idlib/precompiled.h"
#include "../idlib/Lexer.h"

#include <filesystem>

idRenderProgManager renderProgManager;

static shader_t defaultShader;

const char * GLSLParmNames[ RENDERPARM_TOTAL ] = {
	"rpScreenCorrectionFactor",
	"rpWindowCoord",
	"rpDiffuseModifier",
	"rpSpecularModifier",

	"rpLocalLightOrigin",
	"rpLocalViewOrigin",

	"rpLightProjectionS",
	"rpLightProjectionT",
	"rpLightProjectionQ",
	"rpLightFalloffS",

	"rpBumpMatrixS",
	"rpBumpMatrixT",

	"rpDiffuseMatrixS",
	"rpDiffuseMatrixT",

	"rpSpecularMatrixS",
	"rpSpecularMatrixT",

	"rpVertexColorModulate",
	"rpVertexColorAdd",

	"rpColor",
	"rpViewOrigin",
	"rpGlobalEyePos",

	"rpMVPmatrixX",
	"rpMVPmatrixY",
	"rpMVPmatrixZ",
	"rpMVPmatrixW",

	"rpModelMatrixX",
	"rpModelMatrixY",
	"rpModelMatrixZ",
	"rpModelMatrixW",

	"rpProjectionMatrixX",
	"rpProjectionMatrixY",
	"rpProjectionMatrixZ",
	"rpProjectionMatrixW",

	"rpModelViewMatrixX",
	"rpModelViewMatrixY",
	"rpModelViewMatrixZ",
	"rpModelViewMatrixW",

	"rpTextureMatrixS",
	"rpTextureMatrixT",

	"rpTexGen0S",
	"rpTexGen0T",
	"rpTexGen0Q",
	"rpTexGen0Enabled",

	"rpTexGen1S",
	"rpTexGen1T",
	"rpTexGen1Q",
	"rpTexGen1Enabled",

	"rpWobbleSkyX",
	"rpWobbleSkyY",
	"rpWobbleSkyZ",

	"rpOverbright",
	"rpEnableSkinning",
	"rpAlphaTest",

	"rpUser0",
	"rpUser1",
	"rpUser2",
	"rpUser3",
	"rpUser4",
	"rpUser5",
	"rpUser6",
	"rpUser7"
};

static const char * renderProgBindingStrings[ BINDING_TYPE_MAX ] = 
{
	"ubo",
	"sampler"
};

struct vertexLayout_t {
	VkPipelineVertexInputStateCreateInfo inputState;
	std::vector< VkVertexInputBindingDescription > bindingDesc;
	std::vector< VkVertexInputAttributeDescription > attributeDesc;
};

static vertexLayout_t vertexLayouts[NUM_VERTEX_LAYOUTS];

static idUniformBuffer emptyUBO;

idRenderProgManager::idRenderProgManager()
{
    m_current = 0;
    m_counter = 0;
    m_currentData = 0;
    m_currentDescSet = 0;
    m_currentParmBufferOffset = 0;

    memset(m_parmBuffers, 0, sizeof(m_parmBuffers));
}

static VkDescriptorType GetDescriptorType( rpBinding_t type ) {
	switch ( type ) {
	case BINDING_TYPE_UNIFORM_BUFFER: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	case BINDING_TYPE_SAMPLER: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	default: 
		common->Error( "Unknown rpBinding_t %d", static_cast< int >( type ) );
		return VK_DESCRIPTOR_TYPE_MAX_ENUM;
	}
}

static void CreateDescriptorSet(const shader_t& vShader, const shader_t& fragShader, renderProg_t& prog)
{
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
    VkDescriptorSetLayoutBinding binding = {};
    binding.descriptorCount = 1;

    uint32_t bindingId = 0;

    binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    for (int i = 0; i < vShader.bindings.size(); i++)
    {
        binding.binding = bindingId++;
        binding.descriptorType = GetDescriptorType(vShader.bindings[i]);
        prog.bindings.push_back(vShader.bindings[i]);
        layoutBindings.push_back(binding);
    }

    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    for (int i = 0; i < fragShader.bindings.size(); i++)
    {
        binding.binding = bindingId++;
        binding.descriptorType = GetDescriptorType(fragShader.bindings[i]);
        prog.bindings.push_back(fragShader.bindings[i]);
        layoutBindings.push_back(binding);
    }

    VkDescriptorSetLayoutCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.bindingCount = layoutBindings.size();
    createInfo.pBindings = layoutBindings.data();

    vkCreateDescriptorSetLayout(vkcontext.device, &createInfo, NULL, &prog.descriptorSetLayout);

    VkPipelineLayoutCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineInfo.setLayoutCount = 1;
    pipelineInfo.pSetLayouts = &prog.descriptorSetLayout;

    vkCreatePipelineLayout(vkcontext.device, &pipelineInfo, NULL, &prog.pipelineLayout);
}

static void CreateVertexDescriptions()
{
    VkPipelineVertexInputStateCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.flags = 0;

    VkVertexInputBindingDescription binding = {};
    VkVertexInputAttributeDescription attribute = {};

    {
        vertexLayout_t& layout = vertexLayouts[LAYOUT_DRAW_VERT];
        layout.inputState = createInfo;

        uint32_t locationNo = 0;
        uint32_t offset = 0;

        binding.stride = sizeof(idDrawVert);
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        layout.bindingDesc.push_back(binding);

        // Position
        attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
        attribute.location = locationNo++;
        attribute.offset = offset;
        layout.attributeDesc.push_back(attribute);
        offset += sizeof(idDrawVert::xyz);

        // TexCoord
        attribute.format = VK_FORMAT_R16G16_SFLOAT;
        attribute.location = locationNo++;
        attribute.offset = offset;
        layout.attributeDesc.push_back(attribute);
        offset += sizeof(idDrawVert::st);

        // Normal
        attribute.format = VK_FORMAT_R8G8B8A8_UNORM;
        attribute.location = locationNo++;
        attribute.offset = offset;
        layout.attributeDesc.push_back(attribute);
        offset += sizeof(idDrawVert::normal);

        // Tangent
        attribute.format = VK_FORMAT_R8G8B8A8_UNORM;
		attribute.location = locationNo++;
		attribute.offset = offset;
		layout.attributeDesc.push_back( attribute );
		offset += sizeof( idDrawVert::tangent );

		// Color1
		attribute.format = VK_FORMAT_R8G8B8A8_UNORM;
		attribute.location = locationNo++;
		attribute.offset = offset;
		layout.attributeDesc.push_back( attribute );
		offset += sizeof( idDrawVert::color );

		// Color2
		attribute.format = VK_FORMAT_R8G8B8A8_UNORM;
		attribute.location = locationNo++;
		attribute.offset = offset;
		layout.attributeDesc.push_back( attribute );
    }

    {
		vertexLayout_t & layout = vertexLayouts[ LAYOUT_DRAW_SHADOW_VERT_SKINNED ];
		layout.inputState = createInfo;

		uint32_t locationNo = 0;
		uint32_t offset = 0;

		binding.stride = sizeof( idShadowVertSkinned );
		binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		layout.bindingDesc.push_back( binding );

		// Position
		attribute.format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attribute.location = locationNo++;
		attribute.offset = offset;
		layout.attributeDesc.push_back( attribute );
		offset += sizeof( idShadowVertSkinned::xyzw );

		// Color1
		attribute.format = VK_FORMAT_R8G8B8A8_UNORM;
		attribute.location = locationNo++;
		attribute.offset = offset;
		layout.attributeDesc.push_back( attribute );
		offset += sizeof( idShadowVertSkinned::color );

		// Color2
		attribute.format = VK_FORMAT_R8G8B8A8_UNORM;
		attribute.location = locationNo++;
		attribute.offset = offset;
		layout.attributeDesc.push_back( attribute );
	}

    {
        vertexLayout_t& layout = vertexLayouts[LAYOUT_DRAW_SHADOW_VERT];
        layout.inputState = createInfo;

        uint32_t locationNo = 0;
        uint32_t offset = 0;

        binding.stride = sizeof(idShadowVert);
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        layout.bindingDesc.push_back(binding);

        attribute.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attribute.location = 0;
        attribute.offset = 0;
        layout.attributeDesc.push_back(attribute);
    }
}

static void CreateDescriptorPool(VkDescriptorPool (&pools)[ NUM_FRAME_DATA ])
{
    const int numPools = 2;
    VkDescriptorPoolSize poolSizes[numPools];
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = MAX_DESC_SET_UNIFORMS;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = MAX_DESC_IMAGE_SAMPLERS;

    VkDescriptorPoolCreateInfo poolCreateInfo = {};
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolCreateInfo.maxSets = MAX_DESC_SETS;
    poolCreateInfo.poolSizeCount = numPools;
    poolCreateInfo.pPoolSizes = poolSizes;

    for (int i = 0; i < NUM_FRAME_DATA; i++)
        ID_VK_CHECK( vkCreateDescriptorPool( vkcontext.device, &poolCreateInfo, NULL, &pools[ i ] ) );
}

void idRenderProgManager::Init()
{
    printf("----- Initializing Render Shaders -----\n");

    struct builtinShaders_t
    {
        int index;
        const char* name;
        rpStage_t stages;
        vertexLayoutType_t layout;
    } builtins[MAX_BUILTINS] =
    {
        { BUILTIN_GUI, "gui", SHADER_STAGE_ALL, LAYOUT_DRAW_VERT },
		{ BUILTIN_COLOR, "color", SHADER_STAGE_ALL, LAYOUT_DRAW_VERT },
		{ BUILTIN_SIMPLESHADE, "simpleshade", SHADER_STAGE_ALL, LAYOUT_DRAW_VERT },
		{ BUILTIN_TEXTURED, "texture", SHADER_STAGE_ALL, LAYOUT_DRAW_VERT },
		{ BUILTIN_TEXTURE_VERTEXCOLOR, "texture_color", SHADER_STAGE_ALL, LAYOUT_DRAW_VERT },
		{ BUILTIN_TEXTURE_VERTEXCOLOR_SKINNED, "texture_color_skinned", SHADER_STAGE_ALL, LAYOUT_DRAW_VERT },
		{ BUILTIN_TEXTURE_TEXGEN_VERTEXCOLOR, "texture_color_texgen", SHADER_STAGE_ALL, LAYOUT_DRAW_VERT },
		{ BUILTIN_INTERACTION, "interaction", SHADER_STAGE_ALL, LAYOUT_DRAW_VERT },
		{ BUILTIN_INTERACTION_SKINNED, "interaction_skinned", SHADER_STAGE_ALL, LAYOUT_DRAW_VERT },
		{ BUILTIN_INTERACTION_AMBIENT, "interactionAmbient", SHADER_STAGE_ALL, LAYOUT_DRAW_VERT },
		{ BUILTIN_INTERACTION_AMBIENT_SKINNED, "interactionAmbient_skinned", SHADER_STAGE_ALL, LAYOUT_DRAW_VERT },
		{ BUILTIN_ENVIRONMENT, "environment", SHADER_STAGE_ALL, LAYOUT_DRAW_VERT },
		{ BUILTIN_ENVIRONMENT_SKINNED, "environment_skinned", SHADER_STAGE_ALL, LAYOUT_DRAW_VERT },
		{ BUILTIN_BUMPY_ENVIRONMENT, "bumpyEnvironment", SHADER_STAGE_ALL, LAYOUT_DRAW_VERT },
		{ BUILTIN_BUMPY_ENVIRONMENT_SKINNED, "bumpyEnvironment_skinned", SHADER_STAGE_ALL, LAYOUT_DRAW_VERT },

		{ BUILTIN_DEPTH, "depth", SHADER_STAGE_ALL, LAYOUT_DRAW_VERT },
		{ BUILTIN_DEPTH_SKINNED, "depth_skinned", SHADER_STAGE_ALL, LAYOUT_DRAW_VERT },
		{ BUILTIN_SHADOW, "shadow", SHADER_STAGE_VERTEX, LAYOUT_DRAW_SHADOW_VERT },
		{ BUILTIN_SHADOW_SKINNED, "shadow_skinned", SHADER_STAGE_VERTEX, LAYOUT_DRAW_SHADOW_VERT_SKINNED },
		{ BUILTIN_SHADOW_DEBUG, "shadowDebug", SHADER_STAGE_ALL, LAYOUT_DRAW_SHADOW_VERT },
		{ BUILTIN_SHADOW_DEBUG_SKINNED, "shadowDebug_skinned", SHADER_STAGE_ALL, LAYOUT_DRAW_SHADOW_VERT_SKINNED },

		{ BUILTIN_BLENDLIGHT, "blendlight", SHADER_STAGE_ALL, LAYOUT_DRAW_VERT },
		{ BUILTIN_FOG, "fog", SHADER_STAGE_ALL, LAYOUT_DRAW_VERT },
		{ BUILTIN_FOG_SKINNED, "fog_skinned", SHADER_STAGE_ALL, LAYOUT_DRAW_VERT },
		{ BUILTIN_SKYBOX, "skybox", SHADER_STAGE_ALL, LAYOUT_DRAW_VERT },
		{ BUILTIN_WOBBLESKY, "wobblesky", SHADER_STAGE_ALL, LAYOUT_DRAW_VERT },
		{ BUILTIN_BINK, "bink", SHADER_STAGE_ALL, LAYOUT_DRAW_VERT },
		{ BUILTIN_BINK_GUI, "bink_gui", SHADER_STAGE_ALL, LAYOUT_DRAW_VERT },
    };

    m_renderProgs.resize(MAX_BUILTINS);

    for (int i = 0; i < MAX_BUILTINS; i++)
    {
        int vIndex = -1;
        if (builtins[i].stages & SHADER_STAGE_VERTEX)
            vIndex = FindShader(builtins[i].name, builtins[i].stages);
        
        int fIndex = -1;
        if (builtins[i].stages & SHADER_STAGE_FRAGMENT)
            fIndex = FindShader(builtins[i].name, builtins[i].stages);
        
        renderProg_t& prog = m_renderProgs[i];
        prog.name = builtins[i].name;
        prog.vertexShaderIndex = vIndex;
        prog.fragmentShaderIndex = fIndex;
        prog.vertexLayoutType = builtins[i].layout;

        CreateDescriptorSet(m_shaders[vIndex], ( fIndex > -1 ) ? m_shaders[ fIndex ] : defaultShader, prog);
    }

    m_renderProgs[BUILTIN_TEXTURE_VERTEXCOLOR_SKINNED].useJoints = true;
    m_renderProgs[BUILTIN_INTERACTION_SKINNED].useJoints = true;
    m_renderProgs[BUILTIN_INTERACTION_AMBIENT_SKINNED].useJoints = true;
    m_renderProgs[BUILTIN_ENVIRONMENT_SKINNED].useJoints = true;
    m_renderProgs[BUILTIN_BUMPY_ENVIRONMENT_SKINNED].useJoints = true;
    m_renderProgs[BUILTIN_DEPTH_SKINNED].useJoints = true;
    m_renderProgs[BUILTIN_SHADOW_SKINNED].useJoints = true;
    m_renderProgs[BUILTIN_SHADOW_DEBUG_SKINNED].useJoints = true;
    m_renderProgs[BUILTIN_FOG_SKINNED].useJoints = true;

    CreateVertexDescriptions();

    CreateDescriptorPool(m_descriptorPools);

    for (int i = 0; i < NUM_FRAME_DATA; i++)
    {
        m_parmBuffers[i] = new idUniformBuffer();
        m_parmBuffers[i]->AllocBufferObject(NULL, MAX_DESC_SETS * MAX_DESC_SET_UNIFORMS * sizeof( idVec4 ), BU_DYNAMIC);
    }

    emptyUBO.AllocBufferObject(NULL, sizeof(idVec4), BU_DYNAMIC);
}

int idRenderProgManager::FindShader(const char *name, rpStage_t stage)
{
    std::string shaderName = name;
    std::filesystem::path p = shaderName;
    p.replace_extension();
    shaderName = p.string();

    for (int i = 0; i < m_shaders.size(); i++)
    {
        shader_t& shader = m_shaders[i];
        if (shader.name == shaderName)
        {
            LoadShader(i);
            return i;
        }
    }

    shader_t shader;
    shader.name = shaderName;
    shader.stage = stage;
    int index = m_shaders.size();
    m_shaders.push_back(shader);
    LoadShader(index);
    return index;
}

void idRenderProgManager::LoadShader(int shaderIndex)
{
    if ( m_shaders[ shaderIndex ].shaderModule != VK_NULL_HANDLE ) {
		return; // Already loaded
	}

	LoadShader( m_shaders[ shaderIndex ] );
}

void idRenderProgManager::LoadShader(shader_t &shader)
{
    std::string spirvPath, layoutPath;
    spirvPath = "renderprogs/spirv/" + shader.name;
    layoutPath = "renderprogs/vkglsl/" + shader.name;
    if (shader.stage == SHADER_STAGE_FRAGMENT)
    {
        spirvPath += ".fspv";
        layoutPath += ".frag.layout";
    }
    else
    {
        spirvPath += ".vspv";
        layoutPath += ".vert.layout";
    }

    void* spirvBuffer = NULL;
    int spirvLen = fileSystem->ReadFile(spirvPath.c_str(), &spirvBuffer);
    if (spirvLen <= 0)
        common->Error("idRenderProgManager::LoadShader: Unable to load SPIRV shader file %s.", spirvPath.c_str());
    
    void * layoutBuffer = NULL;
	int layoutLen = fileSystem->ReadFile( layoutPath.c_str(), &layoutBuffer );
	if ( layoutLen <= 0 )
        common->Error("idRenderProgManager::LoadShader: Unable to load layout file %s.", spirvPath.c_str());
    
    std::string layout = (const char*)layoutBuffer;

    idLexer src(layout.c_str(), layout.length(), "layout");
    idToken token;

    if (src.ExpectTokenString("uniforms"))
    {
        src.ExpectTokenString("[");

        while (!src.CheckTokenString("]"))
        {
            src.ReadToken(&token);

            int index = -1;
            for (int i = 0; i < RENDERPARM_TOTAL && index == -1; i++)
                if (!strcasecmp(token.c_str(), GLSLParmNames[i]))
                    index = i;
            
            if (index == -1)
                idLib::common->Error("Invalid uniform %s", token.c_str());
            
            shader.parmIndices.push_back(static_cast<renderParm_t>(index));
        }
    }

    if (src.ExpectTokenString("bindings"))
    {
        src.ExpectTokenString( "[" );

		while ( !src.CheckTokenString( "]" ) ) 
        {
			src.ReadToken( &token );

			int index = -1;
			for ( int i = 0; i < BINDING_TYPE_MAX; ++i )
				if (!strcasecmp(token.c_str(), renderProgBindingStrings[ i ]) )
					index = i;

			if ( index == -1 ) {
				common->Error( "Invalid binding %s", token.c_str() );
			}

			shader.bindings.push_back( static_cast< rpBinding_t >( index ) );
		}
    }

    VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.codeSize = spirvLen;
    shaderModuleCreateInfo.pCode = (uint32_t*)spirvBuffer;

    ID_VK_CHECK(vkCreateShaderModule(vkcontext.device, &shaderModuleCreateInfo, NULL, &shader.shaderModule));

    delete layoutBuffer;
    delete spirvBuffer;
}
