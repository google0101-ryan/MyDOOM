#include "Image.h"
#include "BinaryImage.h"

void idBinaryImage::Load2DFromMemory(int width, int height, const uint8_t *pic_const, int numLevels, textureFormat_t &textureFormat, textureColor_t &colorFormat, bool gammaMips)
{
    fileData.textureType = TT_2D;
    fileData.format = textureFormat;
    fileData.colorFormat = colorFormat;
    fileData.width = width;
    fileData.height = height;
    fileData.numLevels = numLevels;

    uint8_t* pic = new uint8_t[width * height * 4];
    memcpy(pic, pic_const, width * height * 4);

    if (colorFormat == CFM_YCOCG_DXT5)
        common->Error("TODO: CFM_YCOCG_DXT5");
    else if (colorFormat == CFM_GREEN_ALPHA)
    {
        for (int i = 0; i < width * height; i++)
        {
            pic[i*4+1] = pic[i*4+3];
            pic[i*4+0] = 0;
            pic[i*4+2] = 0;
            pic[i*4+3] = 0;
        }
    }

    int scaledWidth = width;
    int scaledHeight = height;
    images.resize(numLevels);
    printf("%d\n", numLevels);
    assert(images.size() == 1);
    for (int level = 0; level < images.size(); level++)
    {
        idBinaryImageData& img = images[level];

        uint8_t* dxtPic = pic;
        int dxtWidth = 0, dxtHeight = 0;
        if (textureFormat == FMT_DXT5 || textureFormat == FMT_DXT1)
        {
            assert(0);
        }

        img.level = level;
        img.destZ = 0;
        img.width = scaledWidth;
        img.height = scaledHeight;

        if (textureFormat == FMT_LUM8 || textureFormat == FMT_INT8)
        {
            img.Alloc(scaledWidth * scaledHeight);
            for (int i = 0; i < img.dataSize; i++)
                img.data[i] = pic[i * 4 + 3];
        }
        else if (textureFormat == FMT_L8A8)
        {
            img.Alloc(scaledWidth * scaledHeight * 2);
            for (int i = 0; i < img.dataSize / 2; i++)
            {
                img.data[i * 2 + 0] = pic[i * 4 + 0]; // Read R component into L
                img.data[i * 2 + 1] = pic[i * 4 + 3]; // Read A component into A
            }
        }
        else if (textureFormat == FMT_RGB565)
        {
            img.Alloc( scaledWidth * scaledHeight * 2 );
			for ( int i = 0; i < img.dataSize / 2; i++ ) 
            {
                // Deswizzling bullshit
				unsigned short color = ( ( pic[ i * 4 + 0 ] >> 3 ) << 11 ) | ( ( pic[ i * 4 + 1 ] >> 2 ) << 5 ) | ( pic[ i * 4 + 2 ] >> 3 );
				img.data[ i * 2 + 0 ] = ( color >> 8 ) & 0xFF;
				img.data[ i * 2 + 1 ] = color & 0xFF;
			}
        }
        else
        {
            fileData.format = textureFormat = FMT_RGBA8;
			img.Alloc( scaledWidth * scaledHeight * 4 );
			for ( int i = 0; i < img.dataSize; i++ )
            {
                // Ah, normal RGBA8. Isn't this nice?
				img.data[ i ] = pic[ i ];
			}
        }

        if (pic != dxtPic)
        {
            delete[] dxtPic;
            dxtPic = NULL;
        }

        delete[] pic;
    }
}