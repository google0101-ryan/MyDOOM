#include "Staging_VK.h"
#include "Allocator_VK.h"
#include "RenderBackend.h"
#include "../idlib/precompiled.h"

idVulkanStagingManager stagingManager;

idVulkanStagingManager::idVulkanStagingManager()
: m_maxBufferSize(0),
m_currentBuffer(0),
m_mappedData(NULL),
m_memory(VK_NULL_HANDLE),
m_commandPool(VK_NULL_HANDLE)
{
}

void idVulkanStagingManager::Init()
{
    m_maxBufferSize = (size_t)(64 * 1024 * 1024);
    printf("Allocating staging buffers (%ld bytes each)\n", m_maxBufferSize);

    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = m_maxBufferSize;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    for (int i = 0; i < NUM_FRAME_DATA; i++)
    {
        m_buffers[i].offset = 0;
        ID_VK_CHECK(vkCreateBuffer(vkcontext.device, &bufferCreateInfo, NULL, &m_buffers[i].buffer));
    }

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(vkcontext.device, m_buffers[0].buffer, &memoryRequirements);

    const VkDeviceSize alignMod = memoryRequirements.size % memoryRequirements.alignment;
	const VkDeviceSize alignedSize = ( alignMod == 0 ) ? memoryRequirements.size : ( memoryRequirements.size + memoryRequirements.alignment - alignMod );

    VkMemoryAllocateInfo memoryAllocInfo = {};
    memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocInfo.allocationSize = alignedSize * NUM_FRAME_DATA;
    memoryAllocInfo.memoryTypeIndex = FindMemoryTypeIndex(memoryRequirements.memoryTypeBits, VULKAN_MEMORY_USAGE_CPU_TO_GPU);

    ID_VK_CHECK(vkAllocateMemory(vkcontext.device, &memoryAllocInfo, NULL, &m_memory));

    for (int i = 0; i < NUM_FRAME_DATA; i++)
        ID_VK_CHECK(vkBindBufferMemory(vkcontext.device, m_buffers[i].buffer, m_memory, i * alignedSize));
    
    ID_VK_CHECK(vkMapMemory(vkcontext.device, m_memory, 0, alignedSize * NUM_FRAME_DATA, 0, (void**)&m_mappedData));

    VkCommandPoolCreateInfo commandPoolCreateInfo = {};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolCreateInfo.queueFamilyIndex = vkcontext.graphicsFamilyIdx;
    ID_VK_CHECK(vkCreateCommandPool(vkcontext.device, &commandPoolCreateInfo, NULL, &m_commandPool));

    VkCommandBufferAllocateInfo cmdBufAlloc = {};
    cmdBufAlloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufAlloc.commandPool = m_commandPool;
    cmdBufAlloc.commandBufferCount = 1;

    VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	for ( int i = 0; i < NUM_FRAME_DATA; ++i ) 
    {
		ID_VK_CHECK( vkAllocateCommandBuffers( vkcontext.device, &cmdBufAlloc, &m_buffers[ i ].commandBuffer ) );
		ID_VK_CHECK( vkCreateFence( vkcontext.device, &fenceCreateInfo, NULL, &m_buffers[ i ].fence ) );
		ID_VK_CHECK( vkBeginCommandBuffer( m_buffers[ i ].commandBuffer, &commandBufferBeginInfo ) );

		m_buffers[ i ].data = (uint8_t *)m_mappedData + (i * alignedSize);
	}
}

uint8_t *idVulkanStagingManager::Stage(const int size, const int alignment, VkCommandBuffer &commandBuffer, VkBuffer &buffer, int &bufferOffset)
{
    if (size > m_maxBufferSize)
        common->Error( "Can't allocate %d MB in gpu transfer buffer", (int)( size / 1024 / 1024 ) );
    
    stagingBuffer_t* stage = &m_buffers[m_currentBuffer];
    const int alignMod = stage->offset % alignment;
    stage->offset = ( ( stage->offset % alignment ) == 0 ) ? stage->offset : ( stage->offset + alignment - alignMod );

    if ((stage->offset + size) >= ( m_maxBufferSize ) && !stage->submitted)
        Flush();
    
    stage = &m_buffers[ m_currentBuffer ];
	if ( stage->submitted ) {
		Wait( *stage );
	}

	commandBuffer = stage->commandBuffer;
	buffer = stage->buffer;
	bufferOffset = stage->offset;

	uint8_t * data = stage->data + stage->offset;
	stage->offset += size;

	return data;
}

void idVulkanStagingManager::Flush()
{
    stagingBuffer_t& stage = m_buffers[m_currentBuffer];
    if (stage.submitted || stage.offset == 0)
        return;
    
    VkMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT;

    vkCmdPipelineBarrier(stage.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 1, &barrier, 0, NULL, 0, NULL);

    vkEndCommandBuffer(stage.commandBuffer);

    VkMappedMemoryRange memoryRange = {};
	memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	memoryRange.memory = m_memory;
	memoryRange.size = VK_WHOLE_SIZE;
	vkFlushMappedMemoryRanges( vkcontext.device, 1, &memoryRange );

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &stage.commandBuffer;

	vkQueueSubmit( vkcontext.graphicsQueue, 1, &submitInfo, stage.fence );

	stage.submitted = true;

	m_currentBuffer = ( m_currentBuffer + 1 ) % NUM_FRAME_DATA;
}

void idVulkanStagingManager::Wait(stagingBuffer_t &stage)
{
    if ( stage.submitted == false ) {
		return;
	}

	ID_VK_CHECK( vkWaitForFences( vkcontext.device, 1, &stage.fence, VK_TRUE, UINT64_MAX ) );
	ID_VK_CHECK( vkResetFences( vkcontext.device, 1, &stage.fence ) );

	stage.offset = 0;
	stage.submitted = false;

	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	ID_VK_CHECK( vkBeginCommandBuffer( stage.commandBuffer, &commandBufferBeginInfo ) );
}
