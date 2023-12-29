#pragma once

#include <cstdlib>
#include <cstring>
#include <unordered_map>
#include <string>

#include "Allocator_VK.h"
#include "RenderCommon.h"

enum textureType_t
{
    TT_DISABLED,
    TT_2D,
    TT_CUBIC,
};

enum textureFormat_t
{
    FMT_NONE,

    //------------------------
    // Standard color image formats
    //------------------------

    FMT_RGBA8, // 32bpp
    FMT_XRGB8, // 32bpp

    //------------------------
    // Alpha channel only
    //------------------------

    // Alpha ends up being the same as L8A8 in our current implementation because
    // alpha gives 0 for color, but we want 1
    FMT_ALPHA,

    //------------------------
	// Luminance replicates the value across RGB with a constant A of 255
	// Intensity replicates the value across RGBA
	//------------------------

	FMT_L8A8,			// 16 bpp
	FMT_LUM8,			//  8 bpp
	FMT_INT8,			//  8 bpp

    //------------------------
	// Compressed texture formats
	//------------------------

    FMT_DXT1,
    FMT_DXT5,

    //------------------------
	// Depth buffer formats
	//------------------------

	FMT_DEPTH,			// 24 bpp

	//------------------------
	//
	//------------------------

	FMT_X16,			// 16 bpp
	FMT_Y16_X16,		// 32 bpp
	FMT_RGB565,			// 16 bpp
};

int BitsForFormat(textureFormat_t format);

enum textureSamples_t
{
    SAMPLE_1 = 1 << 0,
    SAMPLE_2 = 1 << 1,
    SAMPLE_4 = 1 << 2,
    SAMPLE_8 = 1 << 3,
    SAMPLE_16 = 1 << 4,
};

enum textureColor_t
{
    CFM_DEFAULT, // RGBA
    CFM_NORMAL_DXT5, // XY format
    CFM_YCOCG_DXT5, // convert RGBA to CoCgCy format
    CFM_GREEN_ALPHA // copy the alpha channel to green
};

class idImageOpts
{
public:
    idImageOpts();

    bool operator==(const idImageOpts& opts);

    textureType_t textureType;
    textureFormat_t format;
    textureColor_t colorFormat;
    textureSamples_t samples;
    int width;
    int height;
    int numLevels;
    bool gammaMips;
    bool readback;
};

inline idImageOpts::idImageOpts()
{
    format = FMT_NONE;
    colorFormat = CFM_DEFAULT;
    samples = SAMPLE_1;
    width = 0;
    height = 0;
    numLevels = 0;
    textureType = TT_2D;
    gammaMips = false;
    readback = false;
}

inline bool idImageOpts::operator==(const idImageOpts& opts)
{
    return (memcmp(this, &opts, sizeof(*this)) == 0);
}

#include "BinaryImage.h"
#include "Material.h"

int BitsForFormat( textureFormat_t format );

#define MAX_IMAGE_NAME 256

class idImage
{
public:
    idImage(const char* name);
    ~idImage();

    void GenerateImage(const uint8_t* pic, int width, int height, textureFilter_t filter, textureRepeat_t repeat, textureUsage_t usage);
    void PurgeImage();
    void CreateSampler();

    VkImageView GetView() {return m_view;}
private:
    friend class idImageManager;
    friend class idRenderBackend;

    void AllocImage();
    void ActuallyLoadImage(bool fromBackEnd);
    void DeriveOpts();
    void SubImageUploads(int mipLevel, int x, int y, int z, int width, int height, const void * pic, int pixelPitch = 0);

    std::string m_imgName;
    idImageOpts m_opts;

    textureFilter_t m_filter;
    textureRepeat_t m_repeat;
    textureUsage_t m_usage;

    void (*m_generatorFunction)( idImage *image );

    bool				m_referencedOutsideLevelLoad;
	bool				m_levelLoadReferenced;	// for determining if it needs to be purged
	ID_TIME_T			m_sourceFileTime;		// the most recent of all images used in creation, for reloadImages command
	ID_TIME_T			m_binaryFileTime;		// the time stamp of the binary file

	int					m_refCount;	

    bool m_bIsSwapChainImage;

    VkFormat m_internalFormat;
    VkImage m_image;
    VkImageView m_view;
    VkImageLayout m_layout;
    VkSampler m_sampler;

    vulkanAllocation_t m_allocation;
    static std::vector<vulkanAllocation_t> m_allocationGarbage[ NUM_FRAME_DATA ];

    static int m_garbageIndex;
	static std::vector< VkImage >		m_imageGarbage[ NUM_FRAME_DATA ];
	static std::vector< VkImageView >	m_viewGarbage[ NUM_FRAME_DATA ];
	static std::vector< VkSampler >		m_samplerGarbage[ NUM_FRAME_DATA ];
};

class idImageManager
{
public:
    idImageManager()
    {
        m_insideLevelLoad = false;
        m_preloadingMapImages = false;
    }

    void Init();

    idImage* GetImage( const char *name ) const;
    idImage* ScratchImage(const char * name, const idImageOpts & opts);
    idImage* AllocImage(const char* name);

    idImage* ImageFromFunction(const char* _name, void (*generatorFunction)(idImage *image));

    void CreateInstrinsicImages();
public:
    bool				m_insideLevelLoad;			// don't actually load images now
	bool				m_preloadingMapImages;		// unless this is set

    idImage* m_defaultImage;
    idImage* m_whiteImage;
    idImage* m_blackImage;
    idImage* m_currentDepthImage;

    std::unordered_map<std::string, idImage*> m_images;
};

extern idImageManager* globalImages;