#pragma once

#include "BufferObject.h"
#include "RenderBackend.h"
#include <string>
#include <vector>

#define VERTEX_UNIFORM_ARRAY_NAME "_va_"
#define FRAGMENT_UNIFORM_ARRAY_NAME	"_fa_"

static const int PC_ATTRIB_INDEX_VERTEX		= 0;
static const int PC_ATTRIB_INDEX_NORMAL		= 2;
static const int PC_ATTRIB_INDEX_COLOR		= 3;
static const int PC_ATTRIB_INDEX_COLOR2		= 4;
static const int PC_ATTRIB_INDEX_ST			= 8;
static const int PC_ATTRIB_INDEX_TANGENT	= 9;

enum vertexMask_t
{
    VERTEX_MASK_XYZ = 1 << PC_ATTRIB_INDEX_VERTEX,
    VERTEX_MASK_ST			= 1 << PC_ATTRIB_INDEX_ST,
	VERTEX_MASK_NORMAL		= 1 << PC_ATTRIB_INDEX_NORMAL,
	VERTEX_MASK_COLOR		= 1 << PC_ATTRIB_INDEX_COLOR,
	VERTEX_MASK_TANGENT		= 1 << PC_ATTRIB_INDEX_TANGENT,
	VERTEX_MASK_COLOR2		= 1 << PC_ATTRIB_INDEX_COLOR2,
};

enum vertexLayoutType_t
{
    LAYOUT_UNKNOWN = -1,
    LAYOUT_DRAW_VERT,
    LAYOUT_DRAW_SHADOW_VERT,
    LAYOUT_DRAW_SHADOW_VERT_SKINNED,
    NUM_VERTEX_LAYOUTS
};

enum renderParm_t
{
    RENDERPARM_SCREENCORRECTIONFACTOR = 0,
	RENDERPARM_WINDOWCOORD,
	RENDERPARM_DIFFUSEMODIFIER,
	RENDERPARM_SPECULARMODIFIER,

	RENDERPARM_LOCALLIGHTORIGIN,
	RENDERPARM_LOCALVIEWORIGIN,

	RENDERPARM_LIGHTPROJECTION_S,
	RENDERPARM_LIGHTPROJECTION_T,
	RENDERPARM_LIGHTPROJECTION_Q,
	RENDERPARM_LIGHTFALLOFF_S,

	RENDERPARM_BUMPMATRIX_S,
	RENDERPARM_BUMPMATRIX_T,

	RENDERPARM_DIFFUSEMATRIX_S,
	RENDERPARM_DIFFUSEMATRIX_T,

	RENDERPARM_SPECULARMATRIX_S,
	RENDERPARM_SPECULARMATRIX_T,

	RENDERPARM_VERTEXCOLOR_MODULATE,
	RENDERPARM_VERTEXCOLOR_ADD,

	// The following are new and can be in any order
	
	RENDERPARM_COLOR,
	RENDERPARM_VIEWORIGIN,
	RENDERPARM_GLOBALEYEPOS,

	RENDERPARM_MVPMATRIX_X,
	RENDERPARM_MVPMATRIX_Y,
	RENDERPARM_MVPMATRIX_Z,
	RENDERPARM_MVPMATRIX_W,

	RENDERPARM_MODELMATRIX_X,
	RENDERPARM_MODELMATRIX_Y,
	RENDERPARM_MODELMATRIX_Z,
	RENDERPARM_MODELMATRIX_W,

	RENDERPARM_PROJMATRIX_X,
	RENDERPARM_PROJMATRIX_Y,
	RENDERPARM_PROJMATRIX_Z,
	RENDERPARM_PROJMATRIX_W,

	RENDERPARM_MODELVIEWMATRIX_X,
	RENDERPARM_MODELVIEWMATRIX_Y,
	RENDERPARM_MODELVIEWMATRIX_Z,
	RENDERPARM_MODELVIEWMATRIX_W,

	RENDERPARM_TEXTUREMATRIX_S,
	RENDERPARM_TEXTUREMATRIX_T,

	RENDERPARM_TEXGEN_0_S,
	RENDERPARM_TEXGEN_0_T,
	RENDERPARM_TEXGEN_0_Q,
	RENDERPARM_TEXGEN_0_ENABLED,

	RENDERPARM_TEXGEN_1_S,
	RENDERPARM_TEXGEN_1_T,
	RENDERPARM_TEXGEN_1_Q,
	RENDERPARM_TEXGEN_1_ENABLED,

	RENDERPARM_WOBBLESKY_X,
	RENDERPARM_WOBBLESKY_Y,
	RENDERPARM_WOBBLESKY_Z,

	RENDERPARM_OVERBRIGHT,
	RENDERPARM_ENABLE_SKINNING,
	RENDERPARM_ALPHA_TEST,

	RENDERPARM_USER0,
	RENDERPARM_USER1,
	RENDERPARM_USER2,
	RENDERPARM_USER3,
	RENDERPARM_USER4,
	RENDERPARM_USER5,
	RENDERPARM_USER6,
	RENDERPARM_USER7,

	RENDERPARM_TOTAL
};

enum rpBuiltIn_t 
{
	BUILTIN_GUI,
	BUILTIN_COLOR,
	BUILTIN_SIMPLESHADE,
	BUILTIN_TEXTURED,
	BUILTIN_TEXTURE_VERTEXCOLOR,
	BUILTIN_TEXTURE_VERTEXCOLOR_SKINNED,
	BUILTIN_TEXTURE_TEXGEN_VERTEXCOLOR,
	BUILTIN_INTERACTION,
	BUILTIN_INTERACTION_SKINNED,
	BUILTIN_INTERACTION_AMBIENT,
	BUILTIN_INTERACTION_AMBIENT_SKINNED,
	BUILTIN_ENVIRONMENT,
	BUILTIN_ENVIRONMENT_SKINNED,
	BUILTIN_BUMPY_ENVIRONMENT,
	BUILTIN_BUMPY_ENVIRONMENT_SKINNED,

	BUILTIN_DEPTH,
	BUILTIN_DEPTH_SKINNED,
	BUILTIN_SHADOW,
	BUILTIN_SHADOW_SKINNED,
	BUILTIN_SHADOW_DEBUG,
	BUILTIN_SHADOW_DEBUG_SKINNED,

	BUILTIN_BLENDLIGHT,
	BUILTIN_FOG,
	BUILTIN_FOG_SKINNED,
	BUILTIN_SKYBOX,
	BUILTIN_WOBBLESKY,
	BUILTIN_BINK,
	BUILTIN_BINK_GUI,

	MAX_BUILTINS
};

enum rpStage_t 
{
	SHADER_STAGE_VERTEX		= BIT( 0 ),
	SHADER_STAGE_FRAGMENT	= BIT( 1 ),
	SHADER_STAGE_ALL		= SHADER_STAGE_VERTEX | SHADER_STAGE_FRAGMENT
};

enum rpBinding_t 
{
	BINDING_TYPE_UNIFORM_BUFFER,
	BINDING_TYPE_SAMPLER,
	BINDING_TYPE_MAX
};

struct shader_t
{
    shader_t() {}

    std::string name;
    rpStage_t stage;
    VkShaderModule shaderModule;
    std::vector<rpBinding_t> bindings;
    std::vector<int> parmIndices;
};

struct renderProg_t
{
    renderProg_t()
        : useJoints(false),
        optionalSkinning(false),
        vertexShaderIndex(-1),
        fragmentShaderIndex(-1),
        vertexLayoutType(LAYOUT_DRAW_VERT),
        pipelineLayout(VK_NULL_HANDLE),
        descriptorSetLayout(VK_NULL_HANDLE) {}

    struct pipelineState_t
    {
        pipelineState_t()
            : stateBits(0),
            pipeline(VK_NULL_HANDLE) {}

        uint64_t stateBits;
        VkPipeline pipeline;
    };

    VkPipeline GetPipeline(uint64_t stateBits, VkShaderModule vertexShader, VkShaderModule fragmentShader);

    std::string name;
    bool useJoints;
    bool optionalSkinning;
    int vertexShaderIndex;
    int fragmentShaderIndex;
    vertexLayoutType_t vertexLayoutType;
    VkPipelineLayout pipelineLayout;
    VkDescriptorSetLayout descriptorSetLayout;
    std::vector<rpBinding_t> bindings;
    std::vector<pipelineState_t> pipelines;
};

class idRenderProgManager
{
public:
    idRenderProgManager();

    void Init();
public:
    int FindShader(const char* name, rpStage_t stage);

    void LoadShader(int shaderIndex);
    void LoadShader(shader_t& shader);

    std::vector<renderProg_t> m_renderProgs;
private:
    int m_current;
    std::array<idVec4, RENDERPARM_TOTAL> m_uniforms;

    int m_builtinShaders[MAX_BUILTINS];
    std::vector<shader_t> m_shaders;

    int m_counter;
    int m_currentData;
    int m_currentDescSet;
    int m_currentParmBufferOffset;
    VkDescriptorPool m_descriptorPools[NUM_FRAME_DATA];
    VkDescriptorSet m_descriptorSets[NUM_FRAME_DATA];

    idUniformBuffer* m_parmBuffers[NUM_FRAME_DATA];
};

extern idRenderProgManager renderProgManager;