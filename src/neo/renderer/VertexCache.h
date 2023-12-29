#pragma once

#include "BufferObject.h"
#include "../idlib/precompiled.h"
#include "RenderCommon.h"

const int VERTCACHE_INDEX_MEMORY_PER_FRAME = 31 * 1024 * 1024;
const int VERTCACHE_VERTEX_MEMORY_PER_FRAME = 31 * 1024 * 1024;
const int VERTCACHE_JOINT_MEMORY_PER_FRAME = 256 * 1024;

const int STATIC_INDEX_MEMORY = 31 * 1024 * 1024;
const int STATIC_VERTEX_MEMORY = 31 * 1024 * 1024;

const int VERTCACHE_STATIC = 1;					// in the static set, not the per-frame set
const int VERTCACHE_SIZE_SHIFT = 1;
const int VERTCACHE_SIZE_MASK = 0x7fffff;		// 8 megs 
const int VERTCACHE_OFFSET_SHIFT = 24;
const int VERTCACHE_OFFSET_MASK = 0x1ffffff;	// 32 megs 
const int VERTCACHE_FRAME_SHIFT = 49;
const int VERTCACHE_FRAME_MASK = 0x7fff;		// 15 bits = 32k frames to wrap around

const int VERTEX_CACHE_ALIGN = 32;
const int INDEX_CACHE_ALIGN = 16;
const int JOINT_CACHE_ALIGN = 16;

enum cacheType_t
{
    CACHE_VERTEX,
    CACHE_INDEX,
    CACHE_JOINT
};

struct geoBufferSet_t
{
    idIndexBuffer indexBuffer;
    idVertexBuffer vertexBuffer;
    idUniformBuffer jointBuffer;
    uint8_t* mappedVertexBase;
    uint8_t* mappedIndexBase;
    uint8_t* mappedJoinBase;
    idSysInterlockedInteger	indexMemUsed;
	idSysInterlockedInteger	vertexMemUsed;
	idSysInterlockedInteger	jointMemUsed;
	int						allocations;
};

class idVertexCache
{
public:
    void Init(int uniformBufferOffsetAlignment);
	void BeginBackend();
	bool CacheIsCurrent(const vertCacheHandle_t handle);

	vertCacheHandle_t ActuallyAlloc(geoBufferSet_t & vcs, const void * data, int bytes, cacheType_t type);
	vertCacheHandle_t AllocVertex(const void* data, int num, size_t size);
	vertCacheHandle_t AllocIndex(const void* data, int num, size_t size);
public:
    int m_currentFrame;
    int m_listNum;
    int m_drawListNum;

    geoBufferSet_t m_staticData;
    geoBufferSet_t m_frameData[NUM_FRAME_DATA];

    int m_uniformBufferOffsetAlignment;

    int m_mostUsedVertex;
    int m_mostUsedIndex;
    int m_mostUsedJoint;
};

extern idVertexCache vertexCache;