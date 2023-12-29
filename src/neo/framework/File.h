#pragma once

#include <string>
#include <cstdint>
#include <cassert>

#include "sys/sys_public.h"

typedef enum
{
    FS_SEEK_CUR,
    FS_SEEK_END,
    FS_SEEK_SET
} fsOrigin_t;

class idFile
{
public:
    virtual ~idFile() {}

    virtual int Seek(long offset, fsOrigin_t origin);
    virtual int Length() = 0;
    
    virtual int Read(void* buffer, int len);
    virtual int ReadInt(int& value);
    virtual int ReadString(std::string& string);

#define SwapBytes( x, y )		{ uint8_t t = (x); (x) = (y); (y) = t; }

    template<class type>
    inline void Big(type& c)
    {
        if (sizeof(type) == 1)
        {}
        else if (sizeof(type) == 2)
        {
            uint8_t* b = (uint8_t*)&c;
            SwapBytes(b[0], b[1]);
        }
        else if (sizeof(type) == 4)
        {
            uint8_t* b = (uint8_t*)&c;
            SwapBytes(b[0], b[3]);
            SwapBytes(b[1], b[2]);
        }
        else if (sizeof(type) == 8)
        {
            uint8_t* b = (uint8_t*)&c;
            SwapBytes(b[0], b[7]);
            SwapBytes(b[1], b[6]);
            SwapBytes(b[2], b[5]);
            SwapBytes(b[3], b[4]);
        }
        else
        {
            assert(false);
        }
    }

    template<class type>
    inline size_t ReadBig(type& c)
    {
        size_t r = Read(&c, sizeof(c));
        Big(c);
        return r;
    }
};

class idFile_Memory : public idFile
{
    friend class idFileSystemLocal;
public:
    idFile_Memory();
    idFile_Memory(const char* name, const char* data, int length);

    virtual int Read(void* buffer, int size);
    virtual int Seek(long offset, fsOrigin_t origin);
    virtual int Length() {return allocated;}
protected:
    std::string name;
private:
    int mode;
    size_t maxSize;
    size_t fileSize;
    size_t allocated;
    int granularity;
    char* filePtr;
    char* curPtr;
};

class idFile_Permanent : public idFile
{
    friend class idFileSystemLocal;
public:
    idFile_Permanent();

    virtual int Read(void* buffer, int size);
    virtual int Seek(long offset, fsOrigin_t origin);
    virtual int Length() {return fileSize;}
private:
    std::string name;
    std::string fullpath;
    int mode;
    int fileSize;
    idFileHandle o;
    bool handleSync;
};