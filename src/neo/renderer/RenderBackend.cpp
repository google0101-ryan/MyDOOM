#include "RenderBackend.h"
#include "Allocator_VK.h"
#include "Staging_VK.h"
#include "../framework/Common.h"
#include "Image.h"
#include "RenderProgs.h"
#include "VertexCache.h"

#include <vulkan/vulkan.h>
#include <stdio.h>
#include <vector>
#include <cstring>
#include <string>

vulkanContext_t vkcontext;

std::string get_result_string (VkResult vulkan_result)
{
    switch (vulkan_result)
    {
        case VK_SUCCESS:
            return "SUCCESS";
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return "OUT OF HOST MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return "OUT OF DEVICE MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED:
            return "INITIALIZATION FAILED";
        case VK_ERROR_LAYER_NOT_PRESENT:
            return "LAYER NOT PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            return "EXTENSION NOT PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            return "INCOMPATIBLE DRIVER";
        default:
            return "UNKNOWN RESULT '" + vulkan_result;
    }
}

#define ID_VK_CHECK( x ) {\
    VkResult ret = x; \
    if (ret != VK_SUCCESS) common->Error("VK: %s %s", get_result_string(ret).c_str(), #x); \
}

static const int g_numInstanceExtensions = 2;
static const char* g_instanceExtensions[g_numInstanceExtensions] =
{
    VK_KHR_SURFACE_EXTENSION_NAME,
};

static const int g_numDebugInstanceExtensions = 1;
static const char * g_debugInstanceExtensions[ g_numDebugInstanceExtensions ] = {
	VK_EXT_DEBUG_REPORT_EXTENSION_NAME
};

static const int g_numDeviceExtensions = 1;
static const char * g_deviceExtensions[ g_numDeviceExtensions ] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static const int g_numValidationLayers = 1;
static const char * g_validationLayers[ g_numValidationLayers ] = {
    "VK_LAYER_KHRONOS_validation"
};

GLFWwindow* window;

static bool VK_Init()
{
    printf("Initializing Vulkan subsystem with multisamples:%d fullscreen:%d\n",
                0, 0);

    glfwInit();
    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(1366, 768, "DOOM 3 Vulkan", NULL, NULL);

    if (!window)
        return false;

    return true;
}

void idRenderBackend::Init()
{
    printf("----- idRenderBackend::Init -----\n");

    if (!VK_Init())
    {
        common->Error("Unable to initialise Vulkan");
    }

    CreateInstance();

    ID_VK_CHECK(glfwCreateWindowSurface(m_instance, window, nullptr, &m_surface));

    SelectSuitablePhysicalDevice();

    CreateLogicalDeviceAndQueues();

    CreateSemaphores();

    CreateQueryPool();

    CreateCommandPool();

    CreateCommandBuffer();

    vulkanAllocator.Init();

    stagingManager.Init();

    CreateSwapChain();

    CreateRenderTargets();

    CreateRenderPass();

    CreatePipelineCache();

    CreateFrameBuffers();

    renderProgManager.Init();

    vertexCache.Init(vkcontext.gpu.props.limits.minUniformBufferOffsetAlignment);
}

void idRenderBackend::BlockingSwapBuffers()
{
	m_counter++;
	m_currentFrameData = m_counter % NUM_FRAME_DATA;

	if (m_commandBufferRecorded[m_currentFrameData] == false)
		return;
	
	ID_VK_CHECK(vkWaitForFences(vkcontext.device, 1, &m_commandBufferFences[m_currentFrameData], VK_TRUE, UINT64_MAX));
	ID_VK_CHECK(vkResetFences(vkcontext.device, 1, &m_commandBufferFences[m_currentFrameData]));
	m_commandBufferRecorded[m_currentFrameData] = false;
}

void idRenderBackend::CreateInstance()
{
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "DOOM";
    appInfo.applicationVersion = 1;
    appInfo.pEngineName = "idTech 4.5";
    appInfo.engineVersion = 1;
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    uint32_t glfwExtensionCount = 0;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;
    createInfo.enabledLayerCount = g_numValidationLayers;
    createInfo.ppEnabledLayerNames = g_validationLayers;

    ID_VK_CHECK(vkCreateInstance(&createInfo, nullptr, &m_instance));
}

static bool CheckPhysicalDeviceExtensionSupport(GPUInfo_t& gpu, const char** requiredExtensions, int required)
{
    int available = 0;
    int count = required;

    for (int i = 0; i < count; i++)
    {
        for (int j = 0; j < gpu.extensionProps.size(); j++)
        {
            if (!strcmp(requiredExtensions[i], gpu.extensionProps[j].extensionName))
            {
                available++;
                break;
            }
        }
    }

    return available == count;
}

void idRenderBackend::SelectSuitablePhysicalDevice()
{
    uint32_t numDevices = 0;
    ID_VK_CHECK(vkEnumeratePhysicalDevices(m_instance, &numDevices, NULL));

    std::vector<VkPhysicalDevice> devices;
    devices.resize(numDevices);

    ID_VK_CHECK(vkEnumeratePhysicalDevices(m_instance, &numDevices, devices.data()));

    std::vector<GPUInfo_t> gpus;
    gpus.resize(numDevices);

    for (uint32_t i = 0; i < numDevices; i++)
    {
        GPUInfo_t& gpu = gpus[i];
        gpu.device = devices[i];

        {
            uint32_t numQueues = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(gpu.device, &numQueues, NULL);
            gpu.queueFamilyProps.resize(numQueues);
            vkGetPhysicalDeviceQueueFamilyProperties(gpu.device, &numQueues, gpu.queueFamilyProps.data());
        }

        {
            uint32_t numExtension = 0;
            ID_VK_CHECK(vkEnumerateDeviceExtensionProperties(gpu.device, NULL, &numExtension, NULL));

            gpu.extensionProps.resize(numExtension);
            ID_VK_CHECK(vkEnumerateDeviceExtensionProperties(gpu.device, NULL, &numExtension, gpu.extensionProps.data()));
        }

        ID_VK_CHECK( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( gpu.device, m_surface, &gpu.surfaceCaps ) );

        {
			uint32_t numFormats;
			ID_VK_CHECK( vkGetPhysicalDeviceSurfaceFormatsKHR( gpu.device, m_surface, &numFormats, NULL ) );
			
			gpu.surfaceFormats.resize( numFormats );
			ID_VK_CHECK( vkGetPhysicalDeviceSurfaceFormatsKHR( gpu.device, m_surface, &numFormats, gpu.surfaceFormats.data() ) );
		}

		{
			uint32_t numPresentModes;
			ID_VK_CHECK( vkGetPhysicalDeviceSurfacePresentModesKHR( gpu.device, m_surface, &numPresentModes, NULL ) );
			
			gpu.presentModes.resize( numPresentModes );
			ID_VK_CHECK( vkGetPhysicalDeviceSurfacePresentModesKHR( gpu.device, m_surface, &numPresentModes, gpu.presentModes.data() ) );
		}

        vkGetPhysicalDeviceMemoryProperties(gpu.device, &gpu.memProps);
        vkGetPhysicalDeviceProperties(gpu.device, &gpu.props);
        vkGetPhysicalDeviceFeatures(gpu.device, &gpu.features);
    }

    for (int i = 0; i < gpus.size(); i++)
    {
        GPUInfo_t& gpu = gpus[i];

        int graphicsIdx = -1;
        int presentIdx = -1;

        if (!CheckPhysicalDeviceExtensionSupport(gpu, g_deviceExtensions, g_numDeviceExtensions))
            continue;
        
        if (gpu.surfaceFormats.size() == 0)
            continue;
        
        if (gpu.presentModes.size() == 0)
            continue;
        
        for (int j = 0; j < gpu.queueFamilyProps.size(); j++)
        {
            VkQueueFamilyProperties& props = gpu.queueFamilyProps[j];

            if (props.queueCount == 0)
                continue;
            
            if (props.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                graphicsIdx = j;
                break;
            }
        }

        for ( int j = 0; j < gpu.queueFamilyProps.size(); ++j ) 
        {
			VkQueueFamilyProperties & props = gpu.queueFamilyProps[ j ];

			if ( props.queueCount == 0 ) 
            {
				continue;
			}

			VkBool32 supportsPresent = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR( gpu.device, j, m_surface, &supportsPresent );
			if ( supportsPresent ) 
            {
				presentIdx = j;
				break;
			}
		}

        if (graphicsIdx >= 0 && presentIdx >= 0)
        {
            vkcontext.graphicsFamilyIdx = graphicsIdx;
            vkcontext.presentFamilyIdx = presentIdx;
            m_physicalDevice = gpu.device;
            vkcontext.gpu = gpu;

            return;
        }
    }

    common->Error("Could not find a physical device which fits our desired profile");
}

void idRenderBackend::CreateLogicalDeviceAndQueues()
{
    std::vector<int> uniqueIdx;
    uniqueIdx.push_back(vkcontext.graphicsFamilyIdx);
	if (vkcontext.graphicsFamilyIdx != vkcontext.presentFamilyIdx)
	    uniqueIdx.push_back(vkcontext.presentFamilyIdx);

    std::vector<VkDeviceQueueCreateInfo> devqInfo;

    const float priority = 1.0f;
    for (int i = 0; i < uniqueIdx.size(); i++)
    {
        VkDeviceQueueCreateInfo qinfo = {};
        qinfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qinfo.queueFamilyIndex = uniqueIdx[i];
        qinfo.queueCount = 1;
        qinfo.pQueuePriorities = &priority;

        devqInfo.push_back(qinfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.textureCompressionBC = VK_TRUE;
    deviceFeatures.imageCubeArray = VK_TRUE;
    deviceFeatures.depthClamp = VK_TRUE;
    deviceFeatures.depthBiasClamp = VK_TRUE;
    deviceFeatures.depthBounds = vkcontext.gpu.features.depthBounds;
    deviceFeatures.fillModeNonSolid = VK_TRUE;

    VkDeviceCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    info.queueCreateInfoCount = devqInfo.size();
    info.pQueueCreateInfos = devqInfo.data();
    info.pEnabledFeatures = &deviceFeatures;
    info.enabledExtensionCount = g_numDeviceExtensions;
    info.ppEnabledExtensionNames = g_deviceExtensions;

    // info.enabledLayerCount = g_numValidationLayers;
    // info.ppEnabledLayerNames = g_validationLayers;

    ID_VK_CHECK(vkCreateDevice(m_physicalDevice, &info, NULL, &vkcontext.device));

    vkGetDeviceQueue(vkcontext.device, vkcontext.graphicsFamilyIdx, 0, &vkcontext.graphicsQueue);
    vkGetDeviceQueue(vkcontext.device, vkcontext.presentFamilyIdx, 0, &vkcontext.presentQueue);
}

void idRenderBackend::CreateSemaphores()
{
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    for (int i = 0; i < NUM_FRAME_DATA; i++)
    {
        ID_VK_CHECK(vkCreateSemaphore(vkcontext.device, &semaphoreCreateInfo, NULL, &m_acquireSemaphores[i]));
        ID_VK_CHECK(vkCreateSemaphore(vkcontext.device, &semaphoreCreateInfo, NULL, &m_renderCompleteSemaphores[i]));
    }
}

void idRenderBackend::CreateQueryPool()
{
    VkQueryPoolCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    createInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
    createInfo.queryCount = NUM_TIMESTAMP_QUERIES;

    for (int i = 0; i < NUM_FRAME_DATA; i++)
    {
        ID_VK_CHECK(vkCreateQueryPool(vkcontext.device, &createInfo, NULL, &m_queryPools[i]));
    }
}

void idRenderBackend::CreateCommandPool()
{
    VkCommandPoolCreateInfo commandPoolCreateInfo = {};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolCreateInfo.queueFamilyIndex = vkcontext.graphicsFamilyIdx;

    ID_VK_CHECK(vkCreateCommandPool(vkcontext.device, &commandPoolCreateInfo, NULL, &m_commandPool));
}

void idRenderBackend::CreateCommandBuffer()
{
    VkCommandBufferAllocateInfo commandBufferAllocInfo = {};
    commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocInfo.commandPool = m_commandPool;
    commandBufferAllocInfo.commandBufferCount = NUM_FRAME_DATA;

    ID_VK_CHECK(vkAllocateCommandBuffers(vkcontext.device, &commandBufferAllocInfo, m_commandBuffers));

    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    for (int i = 0; i < NUM_FRAME_DATA; i++)
        ID_VK_CHECK(vkCreateFence(vkcontext.device, &fenceCreateInfo, NULL, &m_commandBufferFences[i]));
}

VkSurfaceFormatKHR ChooseFormatSurface(std::vector<VkSurfaceFormatKHR>& formats)
{
    VkSurfaceFormatKHR result;

    if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
    {
        result.format = VK_FORMAT_B8G8R8A8_UNORM;
        result.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        return result;
    }

    for (int i = 0; i < formats.size(); i++)
    {
        VkSurfaceFormatKHR& fmt = formats[i];
        if (fmt.format == VK_FORMAT_B8G8R8A8_UNORM && fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return fmt;
    }

    return formats[0];
}

VkPresentModeKHR ChoosePresentMode(std::vector<VkPresentModeKHR>& modes)
{
    VkPresentModeKHR desiredMode = VK_PRESENT_MODE_MAILBOX_KHR;

    for (int i = 0; i < modes.size(); i++)
        if (modes[i] == desiredMode)
            return desiredMode;
    
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D ChooseSurfaceExtent(VkSurfaceCapabilitiesKHR& caps)
{
    VkExtent2D extent;

    if (caps.currentExtent.width == -1)
    {
        extent.width = 1366;
        extent.height = 768;
    }
    else
        extent = caps.currentExtent;
    
    return extent;
}

void idRenderBackend::CreateSwapChain()
{
    GPUInfo_t& gpu = vkcontext.gpu;

    VkSurfaceFormatKHR surfaceFormat = ChooseFormatSurface(gpu.surfaceFormats);
    VkPresentModeKHR presentMode = ChoosePresentMode(gpu.presentModes);
    VkExtent2D extent = ChooseSurfaceExtent(gpu.surfaceCaps);

    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu.device, m_surface, &caps);

    VkSwapchainCreateInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info.surface = m_surface;
    info.minImageCount = NUM_FRAME_DATA;
    if (info.minImageCount < caps.minImageCount)
        info.minImageCount = caps.minImageCount;
    info.imageFormat = surfaceFormat.format;
    info.imageColorSpace = surfaceFormat.colorSpace;
    info.imageExtent = extent;
    info.imageArrayLayers = 1;
    info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (vkcontext.graphicsFamilyIdx != vkcontext.presentFamilyIdx)
    {
        uint32_t indices[] = { (uint32_t)vkcontext.graphicsFamilyIdx, (uint32_t)vkcontext.presentFamilyIdx };

		info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		info.queueFamilyIndexCount = 2;
		info.pQueueFamilyIndices = indices;
    }
    else
        info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info.presentMode = presentMode;
    info.clipped = VK_TRUE;

    ID_VK_CHECK(vkCreateSwapchainKHR(vkcontext.device, &info, NULL, &m_swapchain));

    m_swapchainFormat = surfaceFormat.format;
    m_presentMode = presentMode;
    m_swapchainExtent = extent;

    uint32_t numImages = 0;
    ID_VK_CHECK(vkGetSwapchainImagesKHR(vkcontext.device, m_swapchain, &numImages, NULL));
    ID_VK_CHECK(vkGetSwapchainImagesKHR(vkcontext.device, m_swapchain, &numImages, m_swapchainImages.data()));
    

    for (uint32_t i = 0; i < NUM_FRAME_DATA; i++)
    {
        VkImageViewCreateInfo imageViewCreateInfo = {};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.image = m_swapchainImages[ i ];
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = m_swapchainFormat;
		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;
		imageViewCreateInfo.flags = 0;

        ID_VK_CHECK(vkCreateImageView(vkcontext.device, &imageViewCreateInfo, NULL, &m_swapchainImageViews[i]));
    }
}

static VkFormat ChooseSupportedFormat( VkPhysicalDevice physicalDevice, VkFormat * formats, int numFormats, VkImageTiling tiling, VkFormatFeatureFlags features ) {
	for ( int i = 0; i < numFormats; ++i ) {
		VkFormat format = formats[ i ];

		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties( physicalDevice, format, &props );

		if ( tiling == VK_IMAGE_TILING_LINEAR && ( props.linearTilingFeatures & features ) == features ) {
			return format;
		} else if ( tiling == VK_IMAGE_TILING_OPTIMAL && ( props.optimalTilingFeatures & features ) == features ) {
			return format;
		}
	}

	common->Error( "Failed to find a supported format." );

	return VK_FORMAT_UNDEFINED;
}

void idRenderBackend::CreateRenderTargets()
{
    VkImageFormatProperties fmtProps = {};
    vkGetPhysicalDeviceImageFormatProperties(m_physicalDevice, m_swapchainFormat, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 0, &fmtProps);

    const int samples = 0;

    {
        VkFormat formats[] =
        {
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT,
        };
        vkcontext.depthFormat = ChooseSupportedFormat(m_physicalDevice, formats, 2, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
        printf("Choose format %d\n", vkcontext.depthFormat);
    }

    idImageOpts depthOptions;
    depthOptions.format = FMT_DEPTH;
    depthOptions.width = 1366;
    depthOptions.height = 768;
    depthOptions.numLevels = 1;
    depthOptions.samples = static_cast<textureSamples_t>(vkcontext.sampleCount);    

    globalImages->ScratchImage("_viewDepth", depthOptions);
}

void idRenderBackend::CreateRenderPass()
{
    VkAttachmentDescription attachments[2];
    memset(attachments, 0, sizeof(attachments));

    VkAttachmentDescription& colorAttachment = attachments[0];
    colorAttachment.format = m_swapchainFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkAttachmentDescription& depthAttachment = attachments[ 1 ];
	depthAttachment.format = vkcontext.depthFormat;
	depthAttachment.samples = vkcontext.sampleCount;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkAttachmentReference colorRef = {};
    colorRef.attachment = 0;
    colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthRef = {};
    depthRef.attachment = 1;
    depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;
    subpass.pDepthStencilAttachment = &depthRef;
    
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 2;
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 0;

    ID_VK_CHECK(vkCreateRenderPass(vkcontext.device, &renderPassInfo, NULL, &vkcontext.renderPass));

    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkRenderPassCreateInfo renderPassResumeCreateInfo = {};
	renderPassResumeCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassResumeCreateInfo.attachmentCount = 2;
	renderPassResumeCreateInfo.pAttachments = attachments;
	renderPassResumeCreateInfo.subpassCount = 1;
	renderPassResumeCreateInfo.pSubpasses = &subpass;
	renderPassResumeCreateInfo.dependencyCount = 0;

    ID_VK_CHECK(vkCreateRenderPass(vkcontext.device, &renderPassResumeCreateInfo, NULL, &vkcontext.renderPassResume));
}

void idRenderBackend::CreatePipelineCache()
{
    VkPipelineCacheCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    ID_VK_CHECK( vkCreatePipelineCache( vkcontext.device, &createInfo, NULL, &vkcontext.pipelineCache ) );
}

void idRenderBackend::CreateFrameBuffers()
{
    VkImageView attachments[2] = {};

    idImage* depthImg = globalImages->GetImage("_viewDepth");
    if (!depthImg)
        common->Error("CreateFrameBuffers: No _viewDepth image.");
    attachments[1] = depthImg->GetView();

    VkFramebufferCreateInfo frameBufferCreateInfo = {};
	frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frameBufferCreateInfo.renderPass = vkcontext.renderPass;
	frameBufferCreateInfo.attachmentCount = 2;
	frameBufferCreateInfo.pAttachments = attachments;
	frameBufferCreateInfo.width = 1366;
	frameBufferCreateInfo.height = 768;
	frameBufferCreateInfo.layers = 1;

    for (int i = 0; i < NUM_FRAME_DATA; i++)
    {
        attachments[0] = m_swapchainImageViews[i];
        ID_VK_CHECK(vkCreateFramebuffer(vkcontext.device, &frameBufferCreateInfo, NULL, &m_frameBuffers[i]));
    }
}
