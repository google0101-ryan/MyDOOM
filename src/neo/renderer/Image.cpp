#include "Image.h"

idImageManager imageManager;
idImageManager* globalImages = &imageManager;

#define	DEFAULT_SIZE	16

int BitsForFormat( textureFormat_t format ) 
{
	switch ( format ) 
    {
		case FMT_NONE:		return 0;
		case FMT_RGBA8:		return 32;
		case FMT_XRGB8:		return 32;
		case FMT_RGB565:	return 16;
		case FMT_L8A8:		return 16;
		case FMT_ALPHA:		return 8;
		case FMT_LUM8:		return 8;
		case FMT_INT8:		return 8;
		case FMT_DXT1:		return 4;
		case FMT_DXT5:		return 8;
		case FMT_DEPTH:		return 32;
		case FMT_X16:		return 16;
		case FMT_Y16_X16:	return 32;
		default:
			assert( 0 );
			return 0;
	}
}

bool replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

void idImageManager::Init()
{
    CreateInstrinsicImages();
}

idImage *idImageManager::GetImage(const char *_name) const
{
    if (!_name || !_name[0] || strcasecmp(_name, "default") == 0 || strcasecmp(_name, "_default") == 0)
    {
        printf("DEFAULTED\n");
        common->Error("DEFAULTED\n");
    }

    std::string name = _name;
    replace(name, ".tga", "");

    if (m_images.find(name) == m_images.end())
    {
        return NULL;
    }

    return m_images.at(name);
}

idImage *idImageManager::ScratchImage(const char *name, const idImageOpts &opts)
{
    if (!name || !name[0])
        common->Error("idImageManager::ScratchImage");
    
    idImage* image = GetImage(name);
    if (!image)
        image = AllocImage(name);
    else
        image->PurgeImage();
    
    image->m_opts = opts;
    image->AllocImage();
    image->m_referencedOutsideLevelLoad = true;

    return image;
}

idImage *idImageManager::AllocImage(const char* name)
{
    if (strlen(name) >= 256)
        common->Error("idImageManager::AllocImage: Failed to allocate image");
    
    idImage* image = new idImage(name);

    m_images[name] = image;

    return image;
}

static void R_DefaultImage(idImage* image)
{
    int x, y;
    uint8_t data[DEFAULT_SIZE][DEFAULT_SIZE][4];

    memset(data, 0, sizeof(data));

    for (y = 0; y < DEFAULT_SIZE; y++)
    {
        for (x = 0; x < DEFAULT_SIZE; x++)
        {
            data[y][x][0] = 32;
            data[y][x][1] = 32;
            data[y][x][2] = 32;
            data[y][x][3] = 255;
        }
    }

    for ( x = 0 ; x < DEFAULT_SIZE ; x++ ) 
    {
		data[0][x][0] =
			data[0][x][1] =
			data[0][x][2] =
			data[0][x][3] = 255;

		data[x][0][0] =
			data[x][0][1] =
			data[x][0][2] =
			data[x][0][3] = 255;

		data[DEFAULT_SIZE-1][x][0] =
			data[DEFAULT_SIZE-1][x][1] =
			data[DEFAULT_SIZE-1][x][2] =
			data[DEFAULT_SIZE-1][x][3] = 255;

		data[x][DEFAULT_SIZE-1][0] =
			data[x][DEFAULT_SIZE-1][1] =
			data[x][DEFAULT_SIZE-1][2] =
			data[x][DEFAULT_SIZE-1][3] = 255;
	}

    image->GenerateImage((uint8_t*)data, DEFAULT_SIZE, DEFAULT_SIZE, TF_LINEAR, TR_REPEAT, TD_LOOKUP_TABLE_RGBA);
}

static void R_WhiteImage(idImage* image)
{
    uint8_t data[DEFAULT_SIZE][DEFAULT_SIZE][4];

    memset(data, 255, sizeof(data));

    image->GenerateImage((uint8_t*)data, DEFAULT_SIZE, DEFAULT_SIZE, TF_LINEAR, TR_REPEAT, TD_LOOKUP_TABLE_RGBA);
}

static void R_BlackImage(idImage* image)
{
    uint8_t data[DEFAULT_SIZE][DEFAULT_SIZE][4];

    memset(data, 0, sizeof(data));

    image->GenerateImage((uint8_t*)data, DEFAULT_SIZE, DEFAULT_SIZE, TF_LINEAR, TR_REPEAT, TD_LOOKUP_TABLE_RGBA);
}

static void R_DepthImage( idImage *image ) 
{
	const int width = 1920;
	const int height = 1080;
	const int size = width * height * 4;

	uint8_t * data = new uint8_t[size];

	image->GenerateImage( 
		(uint8_t *)data, 
		width, height, 
		TF_NEAREST, TR_CLAMP, TD_DEPTH );

    delete[] data;
}


idImage *idImageManager::ImageFromFunction(const char *_name, void (*generatorFunction)(idImage *image))
{
    std::string name = _name;
    replace(name, ".tga", "");

    if (m_images.find(name) != m_images.end())
    {
        idImage* image = m_images[name];
        if (image->m_generatorFunction != generatorFunction)
            printf("WARNING: reused image %s with mixed generators\n", name.c_str());
        return image;
    }

    idImage* image = AllocImage(name.c_str());

    image->m_generatorFunction = generatorFunction;
    image->m_referencedOutsideLevelLoad = true;
    image->ActuallyLoadImage(false);
    return image;
}

void idImageManager::CreateInstrinsicImages()
{
    m_defaultImage = ImageFromFunction("_default", R_DefaultImage);
    m_whiteImage = ImageFromFunction("_white", R_WhiteImage);
    m_blackImage = ImageFromFunction("_black", R_BlackImage);
    m_currentDepthImage = ImageFromFunction("_currentDepth", R_DepthImage);
}
