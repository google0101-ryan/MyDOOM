#pragma once

#include <stdint.h>
#include <array>
#include <vector>
#include <vulkan/vulkan.h>

#include "RenderConfig.h"

enum vulkanMemoryUsage_t
{
    VULKAN_MEMORY_USAGE_UNKNOWN,
    VULKAN_MEMORY_USAGE_GPU_ONLY,
    VULKAN_MEMORY_USAGE_CPU_ONLY,
    VULKAN_MEMORY_USAGE_CPU_TO_GPU,
	VULKAN_MEMORY_USAGE_GPU_TO_CPU,
	VULKAN_MEMORY_USAGES,
};

enum vulkanAllocationType_t
{
    VULKAN_ALLOCATION_TYPE_FREE,
    VULKAN_ALLOCATION_TYPE_BUFFER,
	VULKAN_ALLOCATION_TYPE_IMAGE,
	VULKAN_ALLOCATION_TYPE_IMAGE_LINEAR,
	VULKAN_ALLOCATION_TYPE_IMAGE_OPTIMAL,
	VULKAN_ALLOCATION_TYPES,
};

uint32_t FindMemoryTypeIndex(const uint32_t memoryTypeBits, const vulkanMemoryUsage_t usage);

class idVulkanBlock;

struct vulkanAllocation_t
{
    vulkanAllocation_t()
        : block(NULL),
          id(0),
          deviceMemory(VK_NULL_HANDLE),
          offset(0),
          size(0),
          data(NULL)
    {}

    idVulkanBlock* block;
    uint32_t id;
    VkDeviceMemory deviceMemory;
    VkDeviceSize offset;
    VkDeviceSize size;
    uint8_t* data;
};

typedef std::array<std::vector<idVulkanBlock*>, VK_MAX_MEMORY_TYPES> idVulkanBlocks;

class idVulkanBlock
{
    friend class idVulkanAllocator;
public:
    idVulkanBlock(const uint32_t memoryTypeIndex, const VkDeviceSize size, vulkanMemoryUsage_t usage);
    ~idVulkanBlock();

    bool Init();
    void Shutdown();

    bool IsHostVisible() const { return m_usage != VULKAN_MEMORY_USAGE_GPU_ONLY; }

    bool Allocate(const uint32_t size, const uint32_t align, const VkDeviceSize granularity, const vulkanAllocationType_t allocType, vulkanAllocation_t& alloc);

    void Free(vulkanAllocation_t& alloc);

private:
    struct chunk_t
    {
        uint32_t id;
        VkDeviceSize size;
        VkDeviceSize offset;
        chunk_t* prev;
        chunk_t* next;
        vulkanAllocationType_t type;
    };

    chunk_t* m_head;

    uint32_t m_nextBlockId;
    uint32_t m_memoryTypeIndex;
    vulkanMemoryUsage_t m_usage;
    VkDeviceMemory m_deviceMemory;
    VkDeviceSize m_size;
    VkDeviceSize m_allocated;
    uint8_t* data;
};

class idVulkanAllocator
{
public:
    idVulkanAllocator();

    void Init();
    void Shutdown();

    vulkanAllocation_t Allocate(const uint32_t size, 
                                const uint32_t align, 
								const uint32_t memoryTypeBits, 
								const vulkanMemoryUsage_t usage,
								const vulkanAllocationType_t allocType);
    
    void Free(const vulkanAllocation_t alloc);
    void EmptyGarbage();
private:
    int m_garbageIndex;
    int m_deviceLocalMemoryBytes;
	int	m_hostVisibleMemoryBytes;
	VkDeviceSize m_bufferImageGranularity;

    idVulkanBlocks m_blocks;
    std::vector<vulkanAllocation_t> m_garbage[NUM_FRAME_DATA];
};

extern idVulkanAllocator vulkanAllocator;