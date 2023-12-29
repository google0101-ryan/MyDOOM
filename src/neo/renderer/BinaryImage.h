#pragma once

#include "BinaryImageData.h"
#include "../framework/Filesystem.h"

#include <string>
#include <vector>

class idBinaryImage
{
public:
    idBinaryImage(const char* name) : imgName(name) {}

    const char* GetName() const {return imgName.c_str();}
    void SetName(const char* name) {imgName = name;}

    void Load2DFromMemory(int width, int height, const uint8_t * pic_const, int numLevels, textureFormat_t & textureFormat, textureColor_t & colorFormat, bool gammaMips );
	void				LoadCubeFromMemory( int width, const uint8_t * pics[6], int numLevels, textureFormat_t & textureFormat, bool gammaMips );

	ID_TIME_T			LoadFromGeneratedFile( ID_TIME_T sourceFileTime );
	ID_TIME_T			WriteGeneratedFile( ID_TIME_T sourceFileTime );

	const bimageFile_t &	GetFileHeader() { return fileData; }

	int					NumImages() { return images.size(); }
	const bimageImage_t &	GetImageHeader( int i ) const { return images[i]; }
	const uint8_t *			GetImageData( int i ) const { return images[i].data; }
	static void			GetGeneratedFileName( std::string & gfn, const char *imageName);
private:
    std::string imgName;
    bimageFile_t fileData;

    class idBinaryImageData : public bimageImage_t
    {
    public:
        uint8_t* data;

        idBinaryImageData() : data(NULL) {}
        ~idBinaryImageData() {Free();}

        idBinaryImageData& operator=(idBinaryImageData& other)
        {
            if (this == &other)
                return *this;
            
            Alloc(other.dataSize);
            memcpy(data, other.data, other.dataSize);
            return *this;
        }

        void Free()
        {
            if (data)
            {
                delete data;
                data = NULL;
                dataSize = 0;
            }
        }

        void Alloc(int size)
        {
            Free();
            dataSize = size;
            data = new uint8_t[size];
        }
    };

    std::vector<idBinaryImageData> images;
private:
    void MakeGeneratedFileName(std::string& gfn);
    bool LoadFromGeneratedFile( idFile * f, ID_TIME_T sourceFileTime );
};