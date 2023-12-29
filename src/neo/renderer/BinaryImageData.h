#pragma once

#include "../idlib/precompiled.h"

#define BIMAGE_VERSION 10
#define BIMAGE_MAGIC (unsigned int)(('B' << 0) | ('I'<<8) | ('M'<<16) | (BIMAGE_VERSION << 24))

struct bimageImage_t
{
    int level;
    int destZ;
    int width;
    int height;
    int dataSize;
};

#pragma pack(push, 1)
struct bimageFile_t
{
    ID_TIME_T sourceFileTime;
    int headerMagic;
    int textureType;
    int format;
    int colorFormat;
    int width;
    int height;
    int numLevels;
};
#pragma pack(pop)