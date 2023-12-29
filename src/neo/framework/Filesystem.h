#pragma once

#include <vector>
#include <string>
#include "File.h"
#include "../idlib/precompiled.h"

typedef std::vector<std::string> idStrList;

typedef enum
{
	FS_READ			= 0,
	FS_WRITE		= 1,
	FS_APPEND	= 2
} fsMode_t;

class idFileList
{
    friend class idFileSystemLocal;
public:
    const char* GetBasePath();
    int GetNumFiles() {return list.size();}
    const char* GetFile(int index) {return list[index].c_str();}
    const idStrList& GetList() const {return list;}
private:
    std::string basePath;
    idStrList list;
};

class idFileSystem
{
public:
    virtual void Init() = 0;
    virtual idFileList* ListFiles(const char* relativePath, const char* extension, bool sort = false, bool fullRelativePath = false, const char* gamedir = NULL) = 0;

    virtual idFile* OpenFileRead(const char* relativePath, bool allowCopyFiles = true, const char* gamedir = NULL) = 0;
    virtual idFile* OpenFileWrite(const char* relativePath, const char* basePath = "fs_savepath") = 0;
    virtual int ReadFile(const char *relativePath, void **buffer, ID_TIME_T *timestamp = NULL) = 0;
};

extern idFileSystem* fileSystem;