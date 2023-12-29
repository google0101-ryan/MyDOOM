#include "Image.h"
#include "RenderBackend.h"
#include "Staging_VK.h"

int idImage::m_garbageIndex = 0;
std::vector<vulkanAllocation_t> idImage::m_allocationGarbage[NUM_FRAME_DATA];
std::vector< VkImage >		idImage::m_imageGarbage[ NUM_FRAME_DATA ];
std::vector< VkImageView >	idImage::m_viewGarbage[ NUM_FRAME_DATA ];
std::vector< VkSampler >		idImage::m_samplerGarbage[ NUM_FRAME_DATA ];

VkFormat VK_GetFormatFromTextureFormat( const textureFormat_t format ) {
	switch ( format ) {
		case FMT_RGBA8: return VK_FORMAT_R8G8B8A8_UNORM;
		case FMT_XRGB8: return VK_FORMAT_R8G8B8_UNORM;
		case FMT_ALPHA: return VK_FORMAT_R8_UNORM;
		case FMT_L8A8: return VK_FORMAT_R8G8_UNORM;
		case FMT_LUM8: return VK_FORMAT_R8_UNORM;
		case FMT_INT8: return VK_FORMAT_R8_UNORM;
		case FMT_DXT1: return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
		case FMT_DXT5: return VK_FORMAT_BC3_UNORM_BLOCK;
		case FMT_DEPTH: return vkcontext.depthFormat;
		case FMT_X16: return VK_FORMAT_R16_UNORM;
		case FMT_Y16_X16: return VK_FORMAT_R16G16_UNORM;
		case FMT_RGB565: return VK_FORMAT_R5G6B5_UNORM_PACK16;
		default:
			return VK_FORMAT_UNDEFINED;
	}
}


idImage::idImage(const char* name)
: m_imgName(name)
{
    m_bIsSwapChainImage = false;
    m_internalFormat = VK_FORMAT_UNDEFINED;
    m_image = VK_NULL_HANDLE;
    m_view = VK_NULL_HANDLE;
    m_layout = VK_IMAGE_LAYOUT_GENERAL;
    m_sampler = VK_NULL_HANDLE;
    m_referencedOutsideLevelLoad = false;
    m_levelLoadReferenced = false;
    m_refCount = 0;
    m_repeat = TR_REPEAT;
    m_filter = TF_DEFAULT;
    m_generatorFunction = NULL;
}

idImage::~idImage()
{
    if (!m_bIsSwapChainImage)
        PurgeImage();
}

void idImage::PurgeImage()
{
    if (m_sampler != VK_NULL_HANDLE)
    {
        m_samplerGarbage[m_garbageIndex].push_back(m_sampler);
        m_sampler = VK_NULL_HANDLE;
    }
    
    if (m_image != VK_NULL_HANDLE)
    {
        m_allocationGarbage[m_garbageIndex].push_back(m_allocation);
        m_viewGarbage[m_garbageIndex].push_back(m_view);
        m_imageGarbage[m_garbageIndex].push_back(m_image);

        m_allocation = vulkanAllocation_t();

        m_view = VK_NULL_HANDLE;
        m_image = VK_NULL_HANDLE;
    }
}

void idImage::CreateSampler()
{
    VkSamplerCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    createInfo.maxAnisotropy = 1.0f;
    createInfo.anisotropyEnable = VK_FALSE;
    createInfo.compareEnable = ( m_opts.format == FMT_DEPTH );
	createInfo.compareOp = ( m_opts.format == FMT_DEPTH ) ? VK_COMPARE_OP_LESS_OR_EQUAL : VK_COMPARE_OP_NEVER;

    switch (m_filter)
    {
    case TF_DEFAULT:
    case TF_LINEAR:
        createInfo.minFilter = VK_FILTER_LINEAR;
        createInfo.magFilter = VK_FILTER_LINEAR;
        createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        break;
    case TF_NEAREST:
        createInfo.minFilter = VK_FILTER_NEAREST;
		createInfo.magFilter = VK_FILTER_NEAREST;
		createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		break;
	default:
        common->Error("idImage::CreateSampler: unrecognized texture filter %d", m_filter);
    }

    switch (m_repeat)
    {
    case TR_REPEAT:
        createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        break;
    case TR_CLAMP:
        createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        break;
    case TR_CLAMP_TO_ZERO_ALPHA:
        createInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        break;
    case TR_CLAMP_TO_ZERO:
        createInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
		createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        break;
    default:
        common->Error("idImage::CreateSampler: unrecognized texture repeat mode %d", m_repeat);
    }

    ID_VK_CHECK(vkCreateSampler(vkcontext.device, &createInfo, NULL, &m_sampler));
}

VkComponentMapping VK_GetComponentMappingFromTextureFormat( const textureFormat_t format, textureColor_t color ) {
	VkComponentMapping componentMapping = {
		VK_COMPONENT_SWIZZLE_ZERO,
		VK_COMPONENT_SWIZZLE_ZERO,
		VK_COMPONENT_SWIZZLE_ZERO,
		VK_COMPONENT_SWIZZLE_ZERO
	};

	if ( color == CFM_GREEN_ALPHA ) {
		componentMapping.r = VK_COMPONENT_SWIZZLE_ONE;
		componentMapping.g = VK_COMPONENT_SWIZZLE_ONE;
		componentMapping.b = VK_COMPONENT_SWIZZLE_ONE;
		componentMapping.a = VK_COMPONENT_SWIZZLE_G;
		return componentMapping;
	}

	switch ( format ) {
		case FMT_LUM8:
			componentMapping.r = VK_COMPONENT_SWIZZLE_R;
			componentMapping.g = VK_COMPONENT_SWIZZLE_R;
			componentMapping.b = VK_COMPONENT_SWIZZLE_R;
			componentMapping.a = VK_COMPONENT_SWIZZLE_ONE;
			break;
		case FMT_L8A8:
			componentMapping.r = VK_COMPONENT_SWIZZLE_R;
			componentMapping.g = VK_COMPONENT_SWIZZLE_R;
			componentMapping.b = VK_COMPONENT_SWIZZLE_R;
			componentMapping.a = VK_COMPONENT_SWIZZLE_G;
			break;
		case FMT_ALPHA:
			componentMapping.r = VK_COMPONENT_SWIZZLE_ONE;
			componentMapping.g = VK_COMPONENT_SWIZZLE_ONE;
			componentMapping.b = VK_COMPONENT_SWIZZLE_ONE;
			componentMapping.a = VK_COMPONENT_SWIZZLE_R;
			break;
		case FMT_INT8:
			componentMapping.r = VK_COMPONENT_SWIZZLE_R;
			componentMapping.g = VK_COMPONENT_SWIZZLE_R;
			componentMapping.b = VK_COMPONENT_SWIZZLE_R;
			componentMapping.a = VK_COMPONENT_SWIZZLE_R;
			break;
		default:
			componentMapping.r = VK_COMPONENT_SWIZZLE_R;
			componentMapping.g = VK_COMPONENT_SWIZZLE_G;
			componentMapping.b = VK_COMPONENT_SWIZZLE_B;
			componentMapping.a = VK_COMPONENT_SWIZZLE_A;
			break;
	}

	return componentMapping;
}

void idImage::AllocImage()
{
    PurgeImage();

    m_internalFormat = VK_GetFormatFromTextureFormat(m_opts.format);

    CreateSampler();

    VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT;
    if (m_opts.format == FMT_DEPTH)
        usageFlags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    else
        usageFlags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    
    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.flags = (m_opts.textureType == TT_CUBIC) ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = m_internalFormat;
    imageCreateInfo.extent.width = m_opts.width;
    imageCreateInfo.extent.height = m_opts.height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = m_opts.numLevels;
    imageCreateInfo.arrayLayers = (m_opts.textureType == TT_CUBIC) ? 6 : 1;
    imageCreateInfo.samples = static_cast<VkSampleCountFlagBits>(m_opts.samples);
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage = usageFlags;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    ID_VK_CHECK(vkCreateImage(vkcontext.device, &imageCreateInfo, NULL, &m_image));

    VkMemoryRequirements reqs;
    vkGetImageMemoryRequirements(vkcontext.device, m_image, &reqs);

    m_allocation = vulkanAllocator.Allocate(
        reqs.size,
        reqs.alignment,
        reqs.memoryTypeBits,
        VULKAN_MEMORY_USAGE_GPU_ONLY,
        VULKAN_ALLOCATION_TYPE_IMAGE_OPTIMAL
    );

    ID_VK_CHECK(vkBindImageMemory(vkcontext.device, m_image, m_allocation.deviceMemory, m_allocation.offset));

    VkImageViewCreateInfo viewCreateInfo = {};
    viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCreateInfo.image = m_image;
    viewCreateInfo.viewType = (m_opts.textureType == TT_CUBIC) ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
    viewCreateInfo.format = m_internalFormat;
    viewCreateInfo.components = VK_GetComponentMappingFromTextureFormat(m_opts.format, m_opts.colorFormat);
    viewCreateInfo.subresourceRange.aspectMask = ( m_opts.format == FMT_DEPTH ) ? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	viewCreateInfo.subresourceRange.levelCount = m_opts.numLevels;
	viewCreateInfo.subresourceRange.layerCount = ( m_opts.textureType == TT_CUBIC ) ? 6 : 1;
	viewCreateInfo.subresourceRange.baseMipLevel = 0;

    ID_VK_CHECK(vkCreateImageView(vkcontext.device, &viewCreateInfo, NULL, &m_view));
}

void idImage::ActuallyLoadImage(bool fromBackEnd)
{
    if (m_generatorFunction)
    {
        m_generatorFunction(this);
        return;
    }

    assert(0);
}

void idImage::GenerateImage(const uint8_t *pic, int width, int height, textureFilter_t filter, textureRepeat_t repeat, textureUsage_t usage)
{
    PurgeImage();

    m_filter = filter;
    m_repeat = repeat;
    m_usage = usage;
    
    m_opts.textureType = TT_2D;
    m_opts.width = width;
    m_opts.height = height;
    m_opts.numLevels = 0;
    DeriveOpts();

    idBinaryImage im(m_imgName.c_str());
    im.Load2DFromMemory(width, height, pic, m_opts.numLevels, m_opts.format, m_opts.colorFormat, m_opts.gammaMips);

    AllocImage();

    for (int i = 0; i < im.NumImages(); i++)
    {
        const bimageImage_t& image = im.GetImageHeader(i);
        const uint8_t* data = im.GetImageData(i);
        SubImageUploads(image.level, 0, 0, image.destZ, image.width, image.height, data);
    }
}

void idImage::DeriveOpts()
{
    if (m_opts.format == FMT_NONE)
    {
        m_opts.colorFormat = CFM_DEFAULT;

        switch (m_usage)
        {
        case TD_COVERAGE:
            m_opts.format = FMT_DXT1;
            m_opts.colorFormat = CFM_GREEN_ALPHA;
            break;
        case TD_DEPTH:
            m_opts.format = FMT_DEPTH;
			break;
		case TD_DIFFUSE: 
			// TD_DIFFUSE gets only set to when its a diffuse texture for an interaction
			m_opts.gammaMips = true;
			m_opts.format = FMT_DXT5;
			m_opts.colorFormat = CFM_YCOCG_DXT5;
			break;
		case TD_SPECULAR:
			m_opts.gammaMips = true;
			m_opts.format = FMT_DXT1;
			m_opts.colorFormat = CFM_DEFAULT;
			break;
		case TD_DEFAULT:
			m_opts.gammaMips = true;
			m_opts.format = FMT_DXT5;
			m_opts.colorFormat = CFM_DEFAULT;
			break;
		case TD_BUMP:
			m_opts.format = FMT_DXT5;
			m_opts.colorFormat = CFM_NORMAL_DXT5;
			break;
		case TD_FONT:
			m_opts.format = FMT_DXT1;
			m_opts.colorFormat = CFM_GREEN_ALPHA;
			m_opts.numLevels = 4; // We only support 4 levels because we align to 16 in the exporter
			m_opts.gammaMips = true;
			break;
		case TD_LIGHT:
			m_opts.format = FMT_RGB565;
			m_opts.gammaMips = true;
			break;
		case TD_LOOKUP_TABLE_MONO:
			m_opts.format = FMT_INT8;
			break;
		case TD_LOOKUP_TABLE_ALPHA:
			m_opts.format = FMT_ALPHA;
			break;
		case TD_LOOKUP_TABLE_RGB1:
		case TD_LOOKUP_TABLE_RGBA:
			m_opts.format = FMT_RGBA8;
			break;
		default:
			assert( false );
			m_opts.format = FMT_RGBA8;
        }
    }

    if (m_opts.numLevels == 0)
    {
        m_opts.numLevels = 1;

        if (m_filter == TF_LINEAR || m_filter == TF_NEAREST)
            ;
        else
        {
            int temp_width = m_opts.width;
            int temp_height = m_opts.height;
            while (temp_width > 1 || temp_height > 1)
            {
                temp_width >>= 1;
                temp_height >>= 1;
                if (( m_opts.format == FMT_DXT1 || m_opts.format == FMT_DXT5 ) &&
					( ( temp_width & 0x3 ) != 0 || ( temp_height & 0x3 ) != 0 ))
                    break;
                m_opts.numLevels++;
            }
        }
    }
}

void idImage::SubImageUploads(int mipLevel, int x, int y, int z, int width, int height, const void *pic, int pixelPitch)
{
    assert( x >= 0 && y >= 0 && mipLevel >= 0 && width >= 0 && height >= 0 && mipLevel < m_opts.numLevels );

    int size = width * height * BitsForFormat(m_opts.format) / 8;

    VkBuffer buffer;
    VkCommandBuffer commandBuffer;
    int offset = 0;
    uint8_t* data = stagingManager.Stage(size, 16, commandBuffer, buffer, offset);
    if (m_opts.format == FMT_RGB565)
    {
        uint8_t* imgData = (uint8_t*)pic;
        for (int i = 0; i < size; i += 2)
        {
            data[i] = imgData[i + 1];
            data[i+1] = imgData[i];
        }
    }
    else
    {
        memcpy(data, pic, size);
    }

    VkBufferImageCopy imgCopy = {};
    imgCopy.bufferOffset = offset;
    imgCopy.bufferRowLength = pixelPitch;
    imgCopy.bufferImageHeight = height;
    imgCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imgCopy.imageSubresource.layerCount = 1;
    imgCopy.imageSubresource.mipLevel = mipLevel;
    imgCopy.imageSubresource.baseArrayLayer = z;
    imgCopy.imageOffset.x = x;
    imgCopy.imageOffset.y = y;
    imgCopy.imageOffset.z = 0;
    imgCopy.imageExtent.width = width;
    imgCopy.imageExtent.height = height;
    imgCopy.imageExtent.depth = 1;

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = m_image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = m_opts.numLevels;
	barrier.subresourceRange.baseArrayLayer = z;
	barrier.subresourceRange.layerCount = 1;
	
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,  VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

    vkCmdCopyBufferToImage(commandBuffer, buffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imgCopy);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	vkCmdPipelineBarrier( commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, NULL, 0, NULL, 1, &barrier );

	m_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}
