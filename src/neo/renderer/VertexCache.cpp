#include "VertexCache.h"
#include "RenderCommon.h"

#define Max(a, b) (((a) > (b)) ? (a) : (b))

idVertexCache vertexCache;

static void ClearGeoBufferSet( geoBufferSet_t &gbs )
{
	gbs.indexMemUsed.SetValue( 0 );
	gbs.vertexMemUsed.SetValue( 0 );
	gbs.jointMemUsed.SetValue( 0 );
	gbs.allocations = 0;
}

static void MapGeoBufferSet(geoBufferSet_t& gbs)
{
    if (!gbs.mappedVertexBase)
        gbs.mappedVertexBase = (uint8_t*)gbs.vertexBuffer.MapBuffer(BM_WRITE);
    if (!gbs.mappedIndexBase)
        gbs.mappedIndexBase = (uint8_t*)gbs.indexBuffer.MapBuffer(BM_WRITE);
    if (!gbs.mappedJoinBase && gbs.jointBuffer.GetAllocedSize())
        gbs.mappedJoinBase = (uint8_t*)gbs.jointBuffer.MapBuffer(BM_WRITE);
}

static void UnmapGeoBufferSet(geoBufferSet_t& gbs)
{
	if (gbs.mappedVertexBase)
	{
		gbs.vertexBuffer.UnmapBuffer();
		gbs.mappedVertexBase = NULL;
	}
	if (gbs.mappedIndexBase)
	{
		gbs.indexBuffer.UnmapBuffer();
		gbs.mappedIndexBase = NULL;
	}
	if (gbs.mappedJoinBase)
	{
		gbs.jointBuffer.UnmapBuffer();
		gbs.mappedJoinBase = NULL;
	}
}

static void AllocGeoBufferSet(geoBufferSet_t & gbs, const int vertexBytes, const int indexBytes, const int jointBytes, bufferUsageType_t usage)
{
    gbs.vertexBuffer.AllocBufferObject(NULL, vertexBytes, usage);
    gbs.indexBuffer.AllocBufferObject(NULL, indexBytes, usage);
    if (jointBytes > 0)
        gbs.jointBuffer.AllocBufferObject(NULL, jointBytes, usage);
    ClearGeoBufferSet(gbs);
}

void idVertexCache::Init(int uniformBufferOffsetAlignment)
{
    m_currentFrame = 0;
    m_listNum = 0;

    m_uniformBufferOffsetAlignment = uniformBufferOffsetAlignment;

    m_mostUsedVertex = 0;
    m_mostUsedIndex = 0;
    m_mostUsedJoint = 0;

    for (int i = 0; i < NUM_FRAME_DATA; i++)
        AllocGeoBufferSet(m_frameData[i], VERTCACHE_VERTEX_MEMORY_PER_FRAME, VERTCACHE_INDEX_MEMORY_PER_FRAME, VERTCACHE_JOINT_MEMORY_PER_FRAME, BU_DYNAMIC);
    
    AllocGeoBufferSet(m_staticData, STATIC_VERTEX_MEMORY, STATIC_INDEX_MEMORY, 0, BU_STATIC);
    MapGeoBufferSet(m_frameData[m_listNum]);
}

void idVertexCache::BeginBackend()
{
	m_mostUsedVertex = Max(m_mostUsedVertex, m_frameData[m_listNum].vertexMemUsed.GetValue());
	m_mostUsedIndex = Max(m_mostUsedIndex, m_frameData[m_listNum].indexMemUsed.GetValue());
	m_mostUsedJoint = Max(m_mostUsedJoint, m_frameData[m_listNum].jointMemUsed.GetValue());

	const int startUnmap = Sys_Milliseconds();
	UnmapGeoBufferSet(m_frameData[m_listNum]);
	UnmapGeoBufferSet(m_staticData);
	const int endUnmap = Sys_Milliseconds();
	m_drawListNum = m_listNum;

	m_currentFrame++;

	m_listNum = m_currentFrame % NUM_FRAME_DATA;
	const int startMap = Sys_Milliseconds();
	MapGeoBufferSet(m_frameData[m_listNum]);

	ClearGeoBufferSet(m_frameData[m_listNum]);
}

bool idVertexCache::CacheIsCurrent(const vertCacheHandle_t handle)
{
	const int isStatic = handle & VERTCACHE_STATIC;
	if (isStatic)
		return true;
	const uint64_t frameNum = (int)(handle >> VERTCACHE_FRAME_SHIFT) & VERTCACHE_FRAME_MASK;
	if (frameNum != (m_currentFrame & VERTCACHE_FRAME_MASK))
		return false;
	return true;
}

vertCacheHandle_t idVertexCache::ActuallyAlloc(geoBufferSet_t &vcs, const void *data, int bytes, cacheType_t type)
{
	if (bytes == 0)
		return 0;
	
	int endPos = 0;
	int offset = 0;

	switch (type)
	{
	case CACHE_INDEX:
	{
		endPos = vcs.indexMemUsed.Add(bytes);
		if (endPos > vcs.indexBuffer.GetAllocedSize())
		{
			printf("ERROR: Out of index cache\n");
			exit(1);
		}

		offset = endPos - bytes;

		if (data)
		{
			if (vcs.indexBuffer.GetUsage() == BU_DYNAMIC)
				MapGeoBufferSet(vcs);
			vcs.indexBuffer.Update(data, bytes, offset);
		}
		break;
	}
	case CACHE_VERTEX:
	{
		endPos = vcs.vertexMemUsed.Add(bytes);
		if (endPos > vcs.vertexBuffer.GetAllocedSize())
		{
			printf("ERROR: Out of vertex cache\n");
			exit(1);
		}

		offset = endPos - bytes;

		if (data)
		{
			if (vcs.vertexBuffer.GetUsage() == BU_DYNAMIC)
				MapGeoBufferSet(vcs);
			vcs.vertexBuffer.Update(data, bytes, offset);
		}
		break;
	}
	case CACHE_JOINT:
	{
		endPos = vcs.jointMemUsed.Add(bytes);
		if (endPos > vcs.jointBuffer.GetAllocedSize())
		{
			printf("ERROR: Out of vertex cache\n");
			exit(1);
		}

		offset = endPos - bytes;

		if (data)
		{
			if (vcs.jointBuffer.GetUsage() == BU_DYNAMIC)
				MapGeoBufferSet(vcs);
			vcs.jointBuffer.Update(data, bytes, offset);
		}
		break;
	}
	default:
		assert(false);
	}

	vcs.allocations++;

	vertCacheHandle_t handle = ((uint64_t)(m_currentFrame & VERTCACHE_FRAME_MASK) << VERTCACHE_FRAME_SHIFT) |
									((uint64_t)(offset & VERTCACHE_OFFSET_MASK) << VERTCACHE_OFFSET_SHIFT) |
									((uint64_t)(bytes & VERTCACHE_SIZE_MASK) << VERTCACHE_SIZE_SHIFT);
	if (&vcs == &m_staticData)
		handle |= VERTCACHE_STATIC;
	return handle;
}

vertCacheHandle_t idVertexCache::AllocVertex(const void *data, int num, size_t size)
{
	return ActuallyAlloc(m_frameData[m_listNum], data, ALIGN(num*size, VERTEX_CACHE_ALIGN), CACHE_VERTEX);
}

vertCacheHandle_t idVertexCache::AllocIndex(const void *data, int num, size_t size)
{
	return ActuallyAlloc(m_frameData[m_listNum], data, ALIGN(num*size, INDEX_CACHE_ALIGN), CACHE_INDEX);
}
