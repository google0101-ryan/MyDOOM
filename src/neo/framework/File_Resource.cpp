#include "File_Resource.h"
#include "Filesystem.h"
#include "Common.h"
#include <string.h>
#include <algorithm>

bool idResourceContainer::Init(const char *_filename, uint8_t containerIndex)
{
    if (strcasecmp(_filename, "_ordered.resources") == 0)
        common->Error("TODO: _ordered.resources");
    else
        resourceFile = fileSystem->OpenFileRead((std::string(_filename) + ".resources").c_str());
    
    if (!resourceFile)
    {
        return false;
    }

    resourceFile->ReadBig(resourceMagic);
    if (resourceMagic != RESOURCE_FILE_MAGIC)
    {
        common->Error("resourceFileMagic != RESOURCE_FILE_MAGIC");
    }

    fileName = _filename;

    resourceFile->ReadBig(tableOffset);
    resourceFile->ReadBig(tableLength);
    char* const buf = new char[tableLength];

    resourceFile->Seek(tableOffset, FS_SEEK_SET);
    resourceFile->Read(buf, tableLength);

    idFile_Memory memFile("resourceHeader", (const char*)buf, tableLength);

    memFile.ReadBig(numFileResources);
    cacheTable.reserve(numFileResources);

    for (int i = 0; i < numFileResources; i++)
    {
        idResourceCacheEntry& rt = cacheTable[i];
        rt.Read(&memFile);
        rt.containerIndex = containerIndex;

		printf("\tLoaded resource entry \"%s\"\n", rt.filename.c_str());

        cacheHash[rt.filename] = i;
    }

    delete[] buf;

    return true;
}