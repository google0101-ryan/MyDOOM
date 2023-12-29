#include "Allocator_VK.h"
#include "RenderBackend.h"

extern vulkanContext_t vkcontext;

#define ALIGN_SIZE( x, a )       \
        (((x) + (a) - 1) & ~((a) - 1))

uint32_t FindMemoryTypeIndex(const uint32_t memoryTypeBits, const vulkanMemoryUsage_t usage)
{
    VkPhysicalDeviceMemoryProperties& physicalMemoryProperties = vkcontext.gpu.memProps;

    VkMemoryPropertyFlags required = 0;
    VkMemoryPropertyFlags preferred = 0;

    switch ( usage ) {
	case VULKAN_MEMORY_USAGE_GPU_ONLY:
		preferred |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		break;
	case VULKAN_MEMORY_USAGE_CPU_ONLY:
		required |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		break;
	case VULKAN_MEMORY_USAGE_CPU_TO_GPU:
		required |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
		preferred |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		break;
	case VULKAN_MEMORY_USAGE_GPU_TO_CPU:
		required |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
		preferred |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
		break;
	default:
		common->Error( "idVulkanAllocator::AllocateFromPools: Unknown memory usage." );
	}

    for (uint32_t i = 0; i < physicalMemoryProperties.memoryTypeCount; i++)
    {
        if (((memoryTypeBits >> i) & 1) == 0)
            continue;
        
        const VkMemoryPropertyFlags properties = physicalMemoryProperties.memoryTypes[i].propertyFlags;
        if ((properties & required) != required)
            continue;
        if ((properties & preferred) != preferred)
            continue;
        
        return i;
    }

    for (uint32_t i = 0; i < physicalMemoryProperties.memoryTypeCount; i++)
    {
        if (((memoryTypeBits >> i) & 1) == 0)
            continue;
        
        const VkMemoryPropertyFlags properties = physicalMemoryProperties.memoryTypes[i].propertyFlags;
        if ((properties & required) != required)
            continue;
        
        return i;
    }    

    return UINT32_MAX;
}

idVulkanBlock::idVulkanBlock(const uint32_t memoryTypeIndex, const VkDeviceSize size, vulkanMemoryUsage_t usage)
: m_nextBlockId(0),
m_size(size),
m_allocated(0),
m_memoryTypeIndex(memoryTypeIndex),
m_usage(usage),
m_deviceMemory(VK_NULL_HANDLE)
{
}

idVulkanBlock::~idVulkanBlock()
{
    Shutdown();
}

bool idVulkanBlock::Init()
{
    if (m_memoryTypeIndex == UINT64_MAX)
        return false;
    
    VkMemoryAllocateInfo memoryAllocInfo = {};
    memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocInfo.allocationSize = m_size;
    memoryAllocInfo.memoryTypeIndex = m_memoryTypeIndex;

    ID_VK_CHECK(vkAllocateMemory(vkcontext.device, &memoryAllocInfo, NULL, &m_deviceMemory));

    if (m_deviceMemory == VK_NULL_HANDLE)
        return false;
    
    if (IsHostVisible())
        ID_VK_CHECK(vkMapMemory(vkcontext.device, m_deviceMemory, 0, m_size, 0, (void**)&data));
    
    m_head = new chunk_t();
    m_head->id = m_nextBlockId++;
    m_head->size = m_size;
    m_head->offset = 0;
    m_head->prev = NULL;
    m_head->next = NULL;
    m_head->type = VULKAN_ALLOCATION_TYPE_FREE;

    return true;
}

void idVulkanBlock::Shutdown()
{
    if (IsHostVisible())
        vkUnmapMemory(vkcontext.device, m_deviceMemory);
    
    vkFreeMemory(vkcontext.device, m_deviceMemory, NULL);
    m_deviceMemory = VK_NULL_HANDLE;

    chunk_t * prev = NULL;
	chunk_t * current = m_head;
	while ( 1 ) 
    {
		if ( current->next == NULL ) 
        {
			delete current;
			break;
		} 
        else 
        {
			prev = current;
			current = current->next;
			delete prev;
		}
	}

	m_head = NULL;
}

static bool IsOnSamePage( 
	VkDeviceSize rAOffset, VkDeviceSize rASize, 
	VkDeviceSize rBOffset, VkDeviceSize pageSize ) {

	assert( rAOffset + rASize <= rBOffset && rASize > 0 && pageSize > 0 );

	VkDeviceSize rAEnd = rAOffset + rASize - 1;
	VkDeviceSize rAEndPage = rAEnd & ~( pageSize -1 );
	VkDeviceSize rBStart = rBOffset;
	VkDeviceSize rBStartPage = rBStart & ~( pageSize - 1 );

	return rAEndPage == rBStartPage;
}

static bool HasGranularityConflict( vulkanAllocationType_t type1, vulkanAllocationType_t type2 ) {
	if ( type1 > type2 ) {
		auto tmp = type1;
        type1 = type2;
        type2 = tmp;
	}

	switch( type1 ) {
	case VULKAN_ALLOCATION_TYPE_FREE:
		return false;
	case VULKAN_ALLOCATION_TYPE_BUFFER:
		return	type2 == VULKAN_ALLOCATION_TYPE_IMAGE || 
				type2 == VULKAN_ALLOCATION_TYPE_IMAGE_OPTIMAL;
	case VULKAN_ALLOCATION_TYPE_IMAGE:
		return  type2 == VULKAN_ALLOCATION_TYPE_IMAGE ||
				type2 == VULKAN_ALLOCATION_TYPE_IMAGE_LINEAR ||
				type2 == VULKAN_ALLOCATION_TYPE_IMAGE_OPTIMAL;
	case VULKAN_ALLOCATION_TYPE_IMAGE_LINEAR:
		return type2 == VULKAN_ALLOCATION_TYPE_IMAGE_OPTIMAL;
	case VULKAN_ALLOCATION_TYPE_IMAGE_OPTIMAL:
		return false;
	default:
		assert( false );
		return true;
	}
}

bool idVulkanBlock::Allocate(const uint32_t size, const uint32_t align, const VkDeviceSize granularity, const vulkanAllocationType_t allocType, vulkanAllocation_t &alloc)
{
    const VkDeviceSize freeSize = m_size - m_allocated;
    if (freeSize < size)
        return false;
    
    chunk_t* current = NULL;
    chunk_t* bestFit = NULL;
    chunk_t* previous = NULL;

    VkDeviceSize padding = 0;
    VkDeviceSize offset = 0;
    VkDeviceSize alignedSize = 0;

    for (current = m_head; current; previous = current, current = current->next)
    {
        if (current->type != VULKAN_ALLOCATION_TYPE_FREE)
            continue;
        
        if (size > current->size)
            continue;
        
        offset = ALIGN_SIZE(current->offset, align);

        if (previous != NULL && granularity > 1)
        {
            if ( IsOnSamePage( previous->offset, previous->size, offset, granularity ) ) {
				if ( HasGranularityConflict( previous->type, allocType ) ) {
					offset = ALIGN_SIZE( offset, granularity );
				}
			}
        }

        padding = offset - current->offset;
        alignedSize = padding + size;

        if (alignedSize > current->size)
            continue;
        
        if (alignedSize + m_allocated >= m_size)
            return false;
        
        if ( granularity > 1 && current->next != NULL ) {
			chunk_t * next = current->next;
			if ( IsOnSamePage( offset, size, next->offset, granularity ) ) {
				if ( HasGranularityConflict( allocType, next->type ) ) {
					continue;
				}
			}
		}

		bestFit = current;
		break;
    }

    if (bestFit == NULL)
        return false;

    if (bestFit->size > size)
    {
        chunk_t* chunk = new chunk_t();
        chunk_t* next = bestFit->next;
        
        chunk->id = m_nextBlockId++;
        chunk->prev = bestFit;
        bestFit->next = chunk;

        chunk->next = next;
        if (next)
            next->prev = chunk;
        
        chunk->size = bestFit->size - alignedSize;
        chunk->offset = offset + size;
        chunk->type = VULKAN_ALLOCATION_TYPE_FREE;
    }

    bestFit->type = allocType;
    bestFit->size = size;

    m_allocated += alignedSize;

    alloc.size = bestFit->size;
    alloc.id = bestFit->id;
    alloc.deviceMemory = m_deviceMemory;
    if (IsHostVisible())
        alloc.data = data + offset;
    alloc.offset = offset;
    alloc.block = this;

    return true;
}

void idVulkanBlock::Free(vulkanAllocation_t &alloc)
{
    chunk_t* current = NULL;
    for (current = m_head; current; current = current->next)
    {
        if (current->id == alloc.id)
            break;
    }

    if (current == NULL)
    {
        common->Warning("idVulkanBlock::Free: Tried to free an unknown allocation. %p - %lu", this, alloc.id);
        return;
    }

    current->type = VULKAN_ALLOCATION_TYPE_FREE;

    if (current->prev && current->prev->type == VULKAN_ALLOCATION_TYPE_FREE)
    {
        chunk_t* prev = current->prev;

        prev->next = current->next;
        if (current->next)
            current->next->prev = prev;

        prev->size += current->size;

        delete current;
        current = prev;
    }

    if (current->next && current->next->type == VULKAN_ALLOCATION_TYPE_FREE)
    {
        chunk_t * next = current->next;

		if ( next->next ) {
			next->next->prev = current;
		}

		current->next = next->next;

		current->size += next->size;

		delete next;
    }

    m_allocated -= alloc.size;
}

idVulkanAllocator vulkanAllocator;

idVulkanAllocator::idVulkanAllocator()
: m_garbageIndex(0),
m_deviceLocalMemoryBytes(0),
m_hostVisibleMemoryBytes(0),
m_bufferImageGranularity(0)
{
}

void idVulkanAllocator::Init()
{
    m_deviceLocalMemoryBytes = 128 * 1024 * 1024;
    m_hostVisibleMemoryBytes = 64 * 1024 * 1024;
    m_bufferImageGranularity = vkcontext.gpu.props.limits.bufferImageGranularity;
}

void idVulkanAllocator::Shutdown()
{
    EmptyGarbage();
    for (int i = 0; i < VK_MAX_MEMORY_TYPES; i++)
    {
        auto& blocks = m_blocks[i];
        const int numBlocks = blocks.size();
        for (int j = 0; j < numBlocks; j++)
        {
            delete blocks[j];
        }

        blocks.clear();
    }
}

vulkanAllocation_t idVulkanAllocator::Allocate(const uint32_t size, const uint32_t align, const uint32_t memoryTypeBits, const vulkanMemoryUsage_t usage, const vulkanAllocationType_t allocType)
{
    vulkanAllocation_t alloc;

    uint32_t memoryTypeIndex = FindMemoryTypeIndex(memoryTypeBits, usage);
    if (memoryTypeIndex == UINT32_MAX)
        common->Error("idVulkanAllocator::Allocate: Unable to find a memoryTypeIndex for allocation request.");
    
    std::vector<idVulkanBlock*>& blocks = m_blocks[memoryTypeIndex];
    const int numBlocks = blocks.size();
    for (int i = 0; i < numBlocks; i++)
    {
        idVulkanBlock* block = blocks[i];

        if (block->m_memoryTypeIndex != memoryTypeIndex || block->m_usage != usage)
            continue;
        
        if (block->Allocate(size, align, m_bufferImageGranularity, allocType, alloc))
            return alloc;
    }

    VkDeviceSize blockSize = (usage == VULKAN_MEMORY_USAGE_GPU_ONLY) ? m_deviceLocalMemoryBytes : m_hostVisibleMemoryBytes;

    idVulkanBlock* block = new idVulkanBlock(memoryTypeIndex, blockSize, usage);
    if (block->Init())
        blocks.push_back(block);
    else
        common->Error("idVulkanAllocator::Allocate: Could not allocate new memory block.");
    
    block->Allocate(size, align, m_bufferImageGranularity, allocType, alloc);
    return alloc;
}

void idVulkanAllocator::Free(const vulkanAllocation_t alloc)
{
    m_garbage[m_garbageIndex].push_back(alloc);
}

void idVulkanAllocator::EmptyGarbage()
{
    m_garbageIndex = (m_garbageIndex + 1) % NUM_FRAME_DATA;

    auto& garbage = m_garbage[m_garbageIndex];

    const int numAllocs = garbage.size();
    for (auto& alloc : garbage)
    {
        alloc.block->Free(alloc);

        if (alloc.block->m_allocated == 0)
        {
            for (int i = 0; i < m_blocks[alloc.block->m_memoryTypeIndex].size(); i++)
            {
                if (m_blocks[alloc.block->m_memoryTypeIndex][i] == alloc.block)
                    m_blocks[alloc.block->m_memoryTypeIndex].erase(m_blocks[alloc.block->m_memoryTypeIndex].begin()+i);
            }

            delete alloc.block;
            alloc.block = NULL;
        }
    }

    garbage.clear();
}
