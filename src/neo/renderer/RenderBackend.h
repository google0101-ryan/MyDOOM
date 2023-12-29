#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

#include "RenderCommon.h"
#include "RenderConfig.h"


std::string get_result_string (VkResult vulkan_result);

#define ID_VK_CHECK( x ) {\
    VkResult ret = x; \
    if (ret != VK_SUCCESS) common->Error("VK: %s %s", get_result_string(ret).c_str(), #x); \
}

struct tmu_t
{
    unsigned int current2DMap;
    unsigned int currentCubeMap;
};

struct GPUInfo_t
{
    VkPhysicalDevice device;
	VkPhysicalDeviceProperties			props;
	VkPhysicalDeviceMemoryProperties	memProps;
	VkPhysicalDeviceFeatures			features;
	VkSurfaceCapabilitiesKHR			surfaceCaps;
	std::vector< VkSurfaceFormatKHR >		surfaceFormats;
	std::vector< VkPresentModeKHR >			presentModes;
	std::vector< VkQueueFamilyProperties >	queueFamilyProps;
	std::vector< VkExtensionProperties >		extensionProps;
};

struct vulkanContext_t
{
    GPUInfo_t gpu;
    VkDevice device;
    
    int								graphicsFamilyIdx;
	int								presentFamilyIdx;
	VkQueue							graphicsQueue;
	VkQueue							presentQueue;

	VkFormat						depthFormat;
	VkRenderPass					renderPass;
	VkRenderPass					renderPassResume;
	VkPipelineCache					pipelineCache;
	VkSampleCountFlagBits			sampleCount = VK_SAMPLE_COUNT_1_BIT;
	bool							supersampling;
};

extern vulkanContext_t vkcontext;

class idRenderBackend
{
public:
    void Init();
	void BlockingSwapBuffers();
private:
    void CreateInstance();
    void SelectSuitablePhysicalDevice();
    void CreateLogicalDeviceAndQueues();
    void CreateSemaphores();
    void CreateQueryPool();
    void CreateCommandPool();
    void CreateCommandBuffer();
    void CreateSwapChain();
    void CreateRenderTargets();
    void CreateRenderPass();
    void CreatePipelineCache();
    void CreateFrameBuffers();
private:
	uint64_t m_counter;
	uint32_t m_currentFrameData;

    VkPhysicalDevice m_physicalDevice;
    VkInstance m_instance;
    VkSurfaceKHR m_surface;

    // Synchronization
    VkSemaphore m_acquireSemaphores[NUM_FRAME_DATA];
    VkSemaphore m_renderCompleteSemaphores[NUM_FRAME_DATA];
    VkFence m_commandBufferFences[NUM_FRAME_DATA];

    // Pools
    VkQueryPool m_queryPools[NUM_FRAME_DATA];
    VkCommandPool m_commandPool;

    // Buffers
    VkCommandBuffer m_commandBuffers[NUM_FRAME_DATA];

    // Swapchain and swapchain images
    VkSwapchainKHR m_swapchain;
    VkFormat						m_swapchainFormat;
	VkExtent2D						m_swapchainExtent;
    VkPresentModeKHR m_presentMode;

    std::array<VkImage, NUM_FRAME_DATA> m_swapchainImages;
    std::array<VkImageView, NUM_FRAME_DATA> m_swapchainImageViews;
    std::array<VkFramebuffer, NUM_FRAME_DATA> m_frameBuffers;
	std::array<bool, NUM_FRAME_DATA> m_commandBufferRecorded;
};