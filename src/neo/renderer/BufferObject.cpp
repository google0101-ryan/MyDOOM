#include "BufferObject.h"
#include "../framework/Common.h"

#include "RenderBackend.h"
#include "Staging_VK.h"

#include <cassert>

void UnbindBufferObjects() {}

idVertexBuffer::idVertexBuffer()
{
    SetUnmapped();
}

bool idVertexBuffer::AllocBufferObject(const void *data, int allocSize, bufferUsageType_t usage)
{
    assert(m_apiObject == VK_NULL_HANDLE);

    if (allocSize <= 0)
        common->Error("idVertexBuffer::AllocBufferObject: allocSize = %i", allocSize);
    
    m_size = allocSize;
    m_usage = usage;

    bool allocationFailed = false;

	int numBytes = GetAllocedSize();

	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.pNext = NULL;
	bufferCreateInfo.size = numBytes;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if ( m_usage == BU_STATIC ) {
		bufferCreateInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	}

    VkResult ret = vkCreateBuffer( vkcontext.device, &bufferCreateInfo, NULL, &m_apiObject );
	assert( ret == VK_SUCCESS );

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements( vkcontext.device, m_apiObject, &memoryRequirements );

	vulkanMemoryUsage_t memUsage = ( m_usage == BU_STATIC ) ? VULKAN_MEMORY_USAGE_GPU_ONLY : VULKAN_MEMORY_USAGE_CPU_TO_GPU;

	m_allocation = vulkanAllocator.Allocate( 
		memoryRequirements.size, 
		memoryRequirements.alignment, 
		memoryRequirements.memoryTypeBits, 
		memUsage,
		VULKAN_ALLOCATION_TYPE_BUFFER );

	ID_VK_CHECK( vkBindBufferMemory( vkcontext.device, m_apiObject, m_allocation.deviceMemory, m_allocation.offset ) );

    printf("vertex buffer alloc %p, (%i bytes)\n", this, GetSize());

    if (data)
        Update(data, allocSize);
    
    return !allocationFailed;
}

void idVertexBuffer::Update(const void *data, int size, int offset) const
{
    assert(m_apiObject != VK_NULL_HANDLE);
    assert((GetOffset() & 15) == 0);

    if (size > GetSize())
        common->Error( "idVertexBuffer::Update: size overrun, %i > %i\n", size, GetSize() );
    
    if ( m_usage == BU_DYNAMIC ) {
		CopyBuffer( 
#if defined( ID_USE_AMD_ALLOCATOR )
			(byte *)m_allocation.pMappedData + GetOffset() + offset, 
#else
			m_allocation.data + GetOffset() + offset,
#endif
			(const uint8_t *)data, size );
	} else {
		VkBuffer stageBuffer;
		VkCommandBuffer commandBuffer;
		int stageOffset = 0;
		uint8_t * stageData = stagingManager.Stage( size, 1, commandBuffer, stageBuffer, stageOffset );

		memcpy( stageData, data, size );

		VkBufferCopy bufferCopy = {};
		bufferCopy.srcOffset = stageOffset;
		bufferCopy.dstOffset = GetOffset() + offset;
		bufferCopy.size = size;

		vkCmdCopyBuffer( commandBuffer, stageBuffer, m_apiObject, 1, &bufferCopy );
	}
}

void* idVertexBuffer::MapBuffer(bufferMapType_t mapType)
{
    assert(m_apiObject != VK_NULL_HANDLE);

    if (m_usage == BU_STATIC)
        common->Error("idVertexBuffer::MapBuffer: Cannot map a buffer marked as BU_STATIC.");
    
    void* buffer = m_allocation.data + GetOffset();

    SetMapped();

    if (!buffer)
        common->Error( "idVertexBuffer::MapBuffer: failed" );
    return buffer;
}

idBufferObject::idBufferObject()
{
    m_size = 0;
    m_offsetInOtherBuffer = OWNS_BUFFER_FLAG;
    m_usage = BU_STATIC;

    m_apiObject = VK_NULL_HANDLE;
}

void idUniformBuffer::Update(const void *data, int size, int offset) const
{
    assert(m_apiObject != VK_NULL_HANDLE);
    assert((GetOffset() & 15) == 0);

    if (size > GetSize())
        common->Error("idUniformBuffer::Update: size overrun, %i > %i", size, m_size);
    
    if (m_usage == BU_DYNAMIC)
    {
        CopyBuffer(m_allocation.data + GetOffset(), (const uint8_t *)data, size);
    }
    else
    {
        VkBuffer stageBuffer;
        VkCommandBuffer commandBuffer;
        int stageOffset = 0;
        uint8_t* stageData = stagingManager.Stage(size, 1, commandBuffer, stageBuffer, stageOffset);

        memcpy(stageData, data, size);

        VkBufferCopy bufferCopy = {};
        bufferCopy.srcOffset = stageOffset;
        bufferCopy.dstOffset = GetOffset() + offset;
        bufferCopy.size = size;

        vkCmdCopyBuffer(commandBuffer, stageBuffer, m_apiObject, 1, &bufferCopy);
    }
}

void *idUniformBuffer::MapBuffer(bufferMapType_t mapType)
{
    assert(m_apiObject != VK_NULL_HANDLE);

    if (m_usage == BU_STATIC)
        common->Error("idUniformBuffer::MapBuffer: Cannot map a buffer marked as BU_STATIC.");
    
    void* buffer = m_allocation.data + GetOffset();

    SetMapped();

    if (!buffer)
        common->Error( "idUniformBuffer::MapBuffer: failed" );
    return buffer;
}

idUniformBuffer::idUniformBuffer()
{
    m_usage = BU_DYNAMIC;
    SetUnmapped();
}

bool idUniformBuffer::AllocBufferObject(const void *data, int allocSize, bufferUsageType_t usage)
{
    assert(m_apiObject == VK_NULL_HANDLE);
    
    if (allocSize <= 0)
        common->Error("idUniformBuffer::AllocBufferObject: allocSize = %i", allocSize);
    
    m_size = allocSize;
    m_usage = usage;

    bool allocFailed = false;

    const int numBytes = GetAllocedSize();

    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = numBytes;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    if (m_usage == BU_STATIC)
        bufferCreateInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    
    VkResult ret = vkCreateBuffer(vkcontext.device, &bufferCreateInfo, NULL, &m_apiObject);
    assert(ret == VK_SUCCESS);

    VkMemoryRequirements memReqs = {};
    vkGetBufferMemoryRequirements(vkcontext.device, m_apiObject, &memReqs);

    vulkanMemoryUsage_t memUsage = (m_usage == BU_STATIC) ? VULKAN_MEMORY_USAGE_GPU_ONLY : VULKAN_MEMORY_USAGE_CPU_TO_GPU;

    m_allocation = vulkanAllocator.Allocate(memReqs.size, memReqs.alignment, memReqs.memoryTypeBits, memUsage, VULKAN_ALLOCATION_TYPE_BUFFER);

    ID_VK_CHECK(vkBindBufferMemory(vkcontext.device, m_apiObject, m_allocation.deviceMemory, m_allocation.offset));

    printf( "joint buffer alloc %p, (%i bytes)\n", this, GetSize() );

    if (data)
        Update(data, allocSize);
    
    return !allocFailed;
}

void CopyBuffer(uint8_t *dst, const uint8_t *src, int numBytes)
{
    int i = 0;
    for (; i + 128 <= numBytes; i += 128)
    {
        __m128i d0 = _mm_load_si128((__m128i*)&src[i + 0*16]);
        __m128i d1 = _mm_load_si128((__m128i*)&src[i + 1*16]);
        __m128i d2 = _mm_load_si128((__m128i*)&src[i + 2*16]);
        __m128i d3 = _mm_load_si128((__m128i*)&src[i + 3*16]);
        __m128i d4 = _mm_load_si128((__m128i*)&src[i + 4*16]);
        __m128i d5 = _mm_load_si128((__m128i*)&src[i + 5*16]);
        __m128i d6 = _mm_load_si128((__m128i*)&src[i + 6*16]);
        __m128i d7 = _mm_load_si128((__m128i*)&src[i + 7*16]);
        _mm_stream_si128((__m128i *)&dst[i + 0*16], d0);
        _mm_stream_si128((__m128i *)&dst[i + 1*16], d1);
        _mm_stream_si128((__m128i *)&dst[i + 2*16], d2);
        _mm_stream_si128((__m128i *)&dst[i + 3*16], d3);
        _mm_stream_si128((__m128i *)&dst[i + 4*16], d4);
        _mm_stream_si128((__m128i *)&dst[i + 5*16], d5);
        _mm_stream_si128((__m128i *)&dst[i + 6*16], d6);
        _mm_stream_si128((__m128i *)&dst[i + 7*16], d7);
    }

    for (; i + 16 <= numBytes; i += 16)
    {
        __m128i d = _mm_load_si128((__m128i *)&src[i]);
        _mm_stream_si128((__m128i*)&dst[i], d);
    }

    for (; i + 4 <= numBytes; i += 4)
        *(uint32_t*)&dst[i] = *(const uint32_t*)&src[i];
    
    for (; i < numBytes; i++)
        dst[i] = src[i];
    
    _mm_sfence();
}

idIndexBuffer::idIndexBuffer()
{
    SetUnmapped();
}

bool idIndexBuffer::AllocBufferObject(const void *data, int allocSize, bufferUsageType_t usage)
{
    assert(m_apiObject == VK_NULL_HANDLE);

    if ( allocSize <= 0 )
		common->Error( "idIndexBuffer::AllocBufferObject: allocSize = %i", allocSize );

	m_size = allocSize;
	m_usage = usage;

	bool allocationFailed = false;

	int numBytes = GetAllocedSize();

	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.pNext = NULL;
	bufferCreateInfo.size = numBytes;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	if ( m_usage == BU_STATIC )
		bufferCreateInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

#if defined( ID_USE_AMD_ALLOCATOR )
	VmaMemoryRequirements vmaReq = {};
	if ( m_usage == BU_STATIC ) {
		vmaReq.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	} else if ( m_usage == BU_DYNAMIC ) {
		vmaReq.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		vmaReq.flags = VMA_MEMORY_REQUIREMENT_PERSISTENT_MAP_BIT;
	}

	ID_VK_CHECK( vmaCreateBuffer( vmaAllocator, &bufferCreateInfo, &vmaReq, &m_apiObject, &m_vmaAllocation, &m_allocation ) );

#else
	VkResult ret = vkCreateBuffer( vkcontext.device, &bufferCreateInfo, NULL, &m_apiObject );
	assert( ret == VK_SUCCESS );

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements( vkcontext.device, m_apiObject, &memoryRequirements );

	vulkanMemoryUsage_t memUsage = ( m_usage == BU_STATIC ) ? VULKAN_MEMORY_USAGE_GPU_ONLY : VULKAN_MEMORY_USAGE_CPU_TO_GPU;

	m_allocation = vulkanAllocator.Allocate( 
		memoryRequirements.size, 
		memoryRequirements.alignment, 
		memoryRequirements.memoryTypeBits, 
		memUsage,
		VULKAN_ALLOCATION_TYPE_BUFFER );

	ID_VK_CHECK( vkBindBufferMemory( vkcontext.device, m_apiObject, m_allocation.deviceMemory, m_allocation.offset ) );
#endif

	printf( "index buffer alloc %p, (%i bytes)\n", this, GetSize() );

	// copy the data
	if (data)
		Update( data, allocSize );

	return !allocationFailed;
}

void idIndexBuffer::Update(const void *data, int size, int offset) const
{
    assert( m_apiObject != VK_NULL_HANDLE );
	assert( ( GetOffset() & 15 ) == 0 );

	if ( size > GetSize() )
		common->Error( "idIndexBuffer::Update: size overrun, %i > %i\n", size, GetSize() );

	if ( m_usage == BU_DYNAMIC ) 
    {
		CopyBuffer( 
#if defined( ID_USE_AMD_ALLOCATOR )
			(byte *)m_allocation.pMappedData + GetOffset() + offset, 
#else
			m_allocation.data + GetOffset() + offset,
#endif
			(const uint8_t *)data, size );
	} 
    else
    {
		VkBuffer stageBuffer;
		VkCommandBuffer commandBuffer;
		int stageOffset = 0;
		uint8_t * stageData = stagingManager.Stage( size, 1, commandBuffer, stageBuffer, stageOffset );

		memcpy( stageData, data, size );

		VkBufferCopy bufferCopy = {};
		bufferCopy.srcOffset = stageOffset;
		bufferCopy.dstOffset = GetOffset() + offset;
		bufferCopy.size = size;

		vkCmdCopyBuffer( commandBuffer, stageBuffer, m_apiObject, 1, &bufferCopy );
	}
}

void *idIndexBuffer::MapBuffer(bufferMapType_t mapType)
{
    assert(m_apiObject != VK_NULL_HANDLE);

    if (m_usage == BU_STATIC)
        common->Error("idIndexBuffer::MapBuffer: Cannot map a buffer marked as BU_STATIC.");
    
    void* buffer = m_allocation.data + GetOffset();

    SetMapped();

    if (!buffer)
        common->Error( "idVertexBuffer::MapBuffer: failed" );
    return buffer;
}
