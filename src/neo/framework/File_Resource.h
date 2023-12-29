#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

#include "File.h"

class idResourceCacheEntry
{
public:
    idResourceCacheEntry()
    {
        Clear();
    }

    void Clear()
    {
        filename.clear();
        offset = 0;
        length = 0;
        containerIndex = 0;
    }
    size_t Read(idFile* f)
    {
        size_t sz = f->ReadString(filename);
        sz += f->ReadBig(offset);
        sz += f->ReadBig(length);
        return sz;
    }

    std::string filename;
    int offset;
    int length;
    uint8_t containerIndex;
};

static const uint32_t RESOURCE_FILE_MAGIC = 0xD000000D;
class idResourceContainer
{
    friend class idFileSystemLocal;
public:
    bool Init(const char* _filename, uint8_t containerIndex);
private:
    std::string fileName;
    idFile* resourceFile;

    int tableOffset;
    int tableLength;
    int resourceMagic;
    int numFileResources;
    std::unordered_map<int, idResourceCacheEntry> cacheTable;
    std::unordered_map<std::string, int> cacheHash;
};