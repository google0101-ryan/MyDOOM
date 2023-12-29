#pragma once

#include "idlib/precompiled.h"

#include <array>

typedef enum 
{
	TD_SPECULAR,			// may be compressed, and always zeros the alpha channel
	TD_DIFFUSE,				// may be compressed
	TD_DEFAULT,				// generic RGBA texture (particles, etc...)
	TD_BUMP,				// may be compressed with 8 bit lookup
	TD_FONT,				// Font image
	TD_LIGHT,				// Light image
	TD_LOOKUP_TABLE_MONO,	// Mono lookup table (including alpha)
	TD_LOOKUP_TABLE_ALPHA,	// Alpha lookup table with a white color channel
	TD_LOOKUP_TABLE_RGB1,	// RGB lookup table with a solid white alpha
	TD_LOOKUP_TABLE_RGBA,	// RGBA lookup table
	TD_COVERAGE,			// coverage map for fill depth pass when YCoCG is used
	TD_DEPTH,				// depth buffer copy for motion blur
} textureUsage_t;

typedef uint64_t vertCacheHandle_t;

static const int MAX_DESC_SET_UNIFORMS		= 48;
static const int MAX_IMAGE_PARMS			= 16;
static const int MAX_UBO_PARMS				= 2;
static const int NUM_TIMESTAMP_QUERIES		= 16;

static const int MAX_DESC_SETS				= 16384;
static const int MAX_DESC_UNIFORM_BUFFERS	= 8192;
static const int MAX_DESC_IMAGE_SAMPLERS	= 12384;
static const int MAX_DESC_SET_WRITES		= 32;

enum renderOp_t
{
    RC_NOP,
    RC_DRAW_VIEW,
    RC_COPY_RENDER,
};

struct renderCommand_t
{
    renderCommand_t()
        : op(RC_NOP),
        x(0),
        y(0),
        imageWidth(0),
        imageHeight(0),
        cubeFace(0),
        clearColorAfterCopy(false) {}

    renderOp_t op;

    int x;
    int y;
    int imageWidth;
    int imageHeight;
    int cubeFace;
    bool clearColorAfterCopy;
};

class idFrameData
{
public:
    idFrameData() {}

    idSysInterlockedInteger frameMemoryAllocated;
    idSysInterlockedInteger frameMemoryUsed;
    unsigned char* frameMemory;

    int highWaterAllocated;
    int highWaterUsed;
    
    int renderCommandIndex;
    std::array<renderCommand_t, 16> renderCommands;
};

struct srfTriangles_t;
struct viewEntity_t;
struct viewLight_t;
class idMaterial;

struct drawSurf_t
{
	const srfTriangles_t* frontEndGeo;
	int numIndexes;
	vertCacheHandle_t indexCache;
	vertCacheHandle_t ambientCache;
	vertCacheHandle_t shadowCache;
	vertCacheHandle_t jointCache;
	const idMaterial* material;
	uint64_t extraGlState;
	float sort;
	const float* shaderRegisters;
	drawSurf_t* nextOnLight;
	drawSurf_t** linkChain;
	int renderZFail;
};