#pragma once

#include <vulkan/vulkan.h>
#include "RenderConfig.h"

struct stagingBuffer_t
{
    stagingBuffer_t() {}

    bool submitted;
    VkCommandBuffer commandBuffer;
    VkBuffer buffer;
    VkFence fence;
    VkDeviceSize offset;
    uint8_t* data;
};

class idVulkanStagingManager
{
public:
    idVulkanStagingManager();
    ~idVulkanStagingManager() {}

    void Init();
    void Shutdown();

    uint8_t* Stage(const int size, const int alignment, VkCommandBuffer & commandBuffer, VkBuffer & buffer, int & bufferOffset);
    void Flush();
private:
    void Wait(stagingBuffer_t& stage);
private:
    int m_maxBufferSize;
    int m_currentBuffer;
    uint8_t* m_mappedData;
    VkDeviceMemory m_memory;
    VkCommandPool m_commandPool;

    stagingBuffer_t m_buffers[NUM_FRAME_DATA];
};

extern idVulkanStagingManager stagingManager;