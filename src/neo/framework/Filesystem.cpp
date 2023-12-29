#include "Filesystem.h"
#include "Common.h"
#include "CvarSystem.h"
#include "CmdSystem.h"
#include "File_Resource.h"

#include <string>
#include <vector>
#include <filesystem>
#include <unistd.h>

#define MAX_ZIPPED_FILE_NAME 2048
#define FILE_HASH_SIZE 1024

struct searchpath_t
{
    std::string path;
    std::string gamedir;
};

#define FSFLAG_SEARCH_DIRS		( 1 << 0 )
#define FSFLAG_RETURN_FILE_MEM	( 1 << 1 )

class idFileSystemLocal : public idFileSystem
{
public:
    void Init();
    idFileList* ListFiles(const char* relativePath, const char* extension, bool sort, bool fullRelativePath, const char* gamedir);

    idFile* OpenFileReadFlags(const char* relativePath, int searchFlags, bool allowCopyFiles = true, const char* gamedir = NULL);
    idFile* OpenFileRead(const char* relativePath, bool allowCopyFiles = true, const char* gamedir = NULL);
    idFile* OpenFileWrite(const char* relativePath, const char* basePath = "fs_savepath");
    int ReadFile(const char *relativePath, void **buffer, ID_TIME_T *timestamp = NULL);
    void CloseFile(idFile* f) {delete f;}
private:
    std::vector<searchpath_t> searchPaths;
    std::string gameFolder;

    std::vector<idResourceContainer*> resourceFiles;

    static idCvar			fs_debug;
	static idCvar			fs_debugResources;
	static idCvar			fs_copyfiles;
	static idCvar			fs_buildResources;
	static idCvar			fs_game;
	static idCvar			fs_game_base;
	static idCvar			fs_enableBGL;
	static idCvar			fs_debugBGL;

    int numFilesOpenedAsCached;
private:
    void AddGameDirectory(const char* name, const char* dir);
    int AddResourceFile(const char* resourceFileName);

    idFileHandle OpenOSFile(const char* name, fsMode_t mode);

    void InitPrecache();
    void SetupGameDirectories(const char* name);
    void Startup();

    static void Path_f(const idCmdArgs& args);
};

idFileSystemLocal fileSystemLocal;
idFileSystem* fileSystem = &fileSystemLocal;

idCvar	idFileSystemLocal::fs_debug( "fs_debug", "0", CVAR_SYSTEM | CVAR_INTEGER, "", 0, 2);
idCvar	idFileSystemLocal::fs_debugResources( "fs_debugResources", "0", CVAR_SYSTEM | CVAR_BOOL, "" );
idCvar	idFileSystemLocal::fs_enableBGL( "fs_enableBGL", "0", CVAR_SYSTEM | CVAR_BOOL, "" );
idCvar	idFileSystemLocal::fs_debugBGL( "fs_debugBGL", "0", CVAR_SYSTEM | CVAR_BOOL, "" );
idCvar	idFileSystemLocal::fs_copyfiles( "fs_copyfiles", "0", CVAR_SYSTEM | CVAR_INIT | CVAR_BOOL, "Copy every file touched to fs_savepath" );
idCvar	idFileSystemLocal::fs_buildResources( "fs_buildresources", "0", CVAR_SYSTEM | CVAR_BOOL | CVAR_INIT, "Copy every file touched to a resource file" );
idCvar	idFileSystemLocal::fs_game( "fs_game", "", CVAR_SYSTEM | CVAR_INIT | CVAR_SERVERINFO, "mod path" );
idCvar  idFileSystemLocal::fs_game_base( "fs_game_base", "", CVAR_SYSTEM | CVAR_INIT | CVAR_SERVERINFO, "alternate mod path, searched after the main fs_game path, before the basedir" );

idCvar	fs_basepath( "fs_basepath", "", CVAR_SYSTEM | CVAR_INIT, "" );
idCvar	fs_savepath( "fs_savepath", "", CVAR_SYSTEM | CVAR_INIT, "" );
idCvar	fs_resourceLoadPriority( "fs_resourceLoadPriority", "0", CVAR_SYSTEM , "if 1, open requests will be honored from resource files first; if 0, the resource files are checked after normal search paths" );
idCvar	fs_enableBackgroundCaching( "fs_enableBackgroundCaching", "1", CVAR_SYSTEM , "if 1 allow the 360 to precache game files in the background" );

std::vector<std::string> ListOSFiles(const char* path, const char* ext)
{
    std::vector<std::string> fileList;

    if (!std::filesystem::exists(path))
        return fileList;

    for (auto& p : std::filesystem::recursive_directory_iterator(path))
    {
        if (!ext)
            fileList.push_back(p.path().stem().string());
        else if (p.path().extension() == ext)
            fileList.push_back(p.path().stem().string());
    }

    return fileList;
}

void idFileSystemLocal::AddGameDirectory(const char *path, const char *dir)
{
    for (int i = 0; i < searchPaths.size(); i++)
    {
        if (searchPaths[i].path == path && searchPaths[i].gamedir == dir)
            return;
    }

    gameFolder = dir;

    searchpath_t search;
    search.path = path;
    search.gamedir = dir;
    searchPaths.push_back(search);

    std::string pakfile;
    for (int i = 0; i < 2; i++)
    {
        if (i == 1)
        {
            pakfile = std::string(path) + "/" + std::string(dir) + "/maps";
        }
        else
        {
            pakfile = std::string(path) + "/" + std::string(dir);
        }

        idStrList pakfiles = ListOSFiles(pakfile.c_str(), ".resources");
        if (pakfiles.size() > 0)
        {
            for (int j = 0; j < pakfiles.size(); j++)
            {
                pakfile = pakfiles[j];

                if (i == 1)
                {
                    pakfile = "maps/" + pakfile;
                }

                idResourceContainer* rc = new idResourceContainer();
                if (rc->Init(pakfile.c_str(), resourceFiles.size()))
                {
                    resourceFiles.push_back(rc);
                    printf("Loaded resource file %s\n", pakfile.c_str());
                }
            }
        }
    }
}

int idFileSystemLocal::AddResourceFile(const char *resourceFileName)
{
    auto resourceFile = "maps/" + std::string(resourceFileName);
    idResourceContainer* rc = new idResourceContainer();
    if (rc->Init(resourceFile.c_str(), resourceFiles.size()))
    {
        resourceFiles.push_back(rc);
        printf("Loaded resource file %s\n", resourceFile.c_str());
        return resourceFiles.size() - 1;
    }

    return -1;
}

idFileHandle idFileSystemLocal::OpenOSFile(const char *name, fsMode_t mode)
{
    idFileHandle fp;

    if (mode == FS_WRITE)
    {
        fp = fopen(name, "wb");
    }
    else if (mode == FS_READ)
    {
        fp = fopen(name, "rb");
    }
    else if (mode == FS_APPEND)
    {
        fp = fopen(name, "ab");
    }

    if (!fp)
    {
        std::filesystem::path fpath, entry;
        idStrList list;

        fpath = name;
        fpath.replace_extension();
        list = ListOSFiles(fpath.c_str(), NULL);

        for (int i = 0; i < list.size(); i++)
        {
            entry = fpath.string() + "/" + list[i];
            if (entry == name)
            {
                if (mode == FS_WRITE)
                {
                    fp = fopen(entry.c_str(), "wb");
                }
                else if (mode == FS_READ)
                {
                    fp = fopen(entry.c_str(), "rb");
                }
                else if (mode == FS_APPEND)
                {
                    fp = fopen(entry.c_str(), "ab");
                }

                if (fp)
                {
                    if (fs_debug.GetIntegerValue())
                        printf("idFileSystemLocal::OpenFileRead: changed %s to %s\n", name, entry.c_str());
                    break;
                }
                else
                {
                    common->Warning("dFileSystemLocal::OpenFileRead: fs_caseSensitiveOS 1 could not open %s", entry.c_str());
                }
            }
        }
    }

    return fp;
}

void idFileSystemLocal::InitPrecache()
{
    if (!fs_enableBackgroundCaching.GetIntegerValue())
        return;
    numFilesOpenedAsCached = 0;
}

void idFileSystemLocal::SetupGameDirectories(const char *name)
{
    if (fs_basepath.GetValueString()[0])
    {
        AddGameDirectory(fs_basepath.GetValueString(), name);
    }
    if (fs_savepath.GetValueString()[0])
    {
        AddGameDirectory(fs_savepath.GetValueString(), name);
    }

    if (fs_game_base.GetValueString()[0] && strcasecmp(fs_game_base.GetValueString(), "base"))
    {
        SetupGameDirectories(fs_game_base.GetValueString());
    }

    if (fs_game.GetValueString()[0] &&
        strcasecmp(fs_game.GetValueString(), fs_game_base.GetValueString())
        && strcasecmp(fs_game.GetValueString(), "base"))
    {
        SetupGameDirectories(fs_game.GetValueString());
    }
}

void idFileSystemLocal::Startup()
{
    printf("------ Initializing File System ------\n");

    InitPrecache();

    SetupGameDirectories("base");

    cmdSystem->AddCommand("path", Path_f, CMD_FL_SYSTEM, "lists search paths");

    Path_f(idCmdArgs());

    printf("file system initialized.\n");
    printf("--------------------------------------\n");
}

void idFileSystemLocal::Path_f(const idCmdArgs &args)
{
    printf("Current search path:\n");
    for (int i = 0; i < fileSystemLocal.searchPaths.size(); i++)
        printf("%s/%s\n", fileSystemLocal.searchPaths[i].path.c_str(), fileSystemLocal.searchPaths[i].gamedir.c_str());
}

void idFileSystemLocal::Init()
{
    common->StartupVariable("fs_basepath");
    common->StartupVariable("fs_savepath");
    common->StartupVariable("fs_game");
    common->StartupVariable("fs_game_base");
    common->StartupVariable("fs_copyfiles");

    if (fs_basepath.GetValueString()[0] == '\0')
    {
        fs_basepath.SetValue(".");
    }
    if (fs_savepath.GetValueString()[0] == '\0')
    {
        fs_savepath.SetValue("./saves");
    }

    Startup();
}

idFileList *idFileSystemLocal::ListFiles(const char *relativePath, const char *extension, bool sort, bool fullRelativePath, const char *gamedir)
{
    idStrList extensionList;

    idFileList* fileList = new idFileList;
    fileList->basePath = relativePath;

    if (!std::filesystem::exists(relativePath))
        return fileList;

    for (auto& p : std::filesystem::recursive_directory_iterator(relativePath))
    {
        if (p.path().extension() == extension)
            fileList->list.push_back(p.path().stem().string());
    }

    return fileList;
}

int DirectFileLength(idFileHandle o)
{
    int pos;
    int end;

    pos = ftell(o);
    fseek(o, 0, SEEK_END);
    end = ftell(o);
    fseek(o, pos, SEEK_SET);

    return end;
}

idFile *idFileSystemLocal::OpenFileReadFlags(const char *relativePath, int searchFlags, bool allowCopyFiles, const char *gamedir)
{
    if (relativePath == NULL)
    {
        common->Error("idFileSystemLocal::OpenFileRead: NULL 'relativePath' parameter passed\n");
        return NULL;
    }

    if (relativePath[0] == '/' || relativePath[0] == '\\')
        relativePath++;
    
    if (strstr(relativePath, "..") || strstr(relativePath, "::"))
        return NULL;
    
    if (relativePath[0] == '\0')
        return NULL;
    
    if (fs_debug.GetIntegerValue())
        printf("FILE DEBUG: opening %s\n", relativePath);
    
    if (resourceFiles.size() > 0 && fs_resourceLoadPriority.GetIntegerValue() == 1)
    {
        printf("TODO: ResourceFile!\n");
    }

    if (searchFlags & FSFLAG_SEARCH_DIRS)
    {
        for (int sp = searchPaths.size() - 1; sp >= 0; sp--)
        {
            if (gamedir != NULL && gamedir[0] != 0)
                if (searchPaths[sp].gamedir != gamedir)
                    continue;
            
            std::string netpath = (searchPaths[sp].path + "/" + searchPaths[sp].gamedir + "/" + relativePath);
            idFileHandle fp = OpenOSFile(netpath.c_str(), FS_READ);
            if (!fp)
                continue;
            
            idFile_Permanent* file = new idFile_Permanent();
            file->o = fp;
            file->name = relativePath;
            file->fullpath = netpath;
            file->mode = (1 << FS_READ);
            file->fileSize = DirectFileLength(fp);
            
            if (fs_debug.GetIntegerValue())
                printf("idFileSystem::OpenFileRead: %s (found in '%s/%s')\n", relativePath, searchPaths[sp].path.c_str(), searchPaths[sp].gamedir.c_str());
            
            // if (allowCopyFiles)
            // {
            //     common->Error("TODO: allowcopyfiles");
            // }

            if (searchFlags & FSFLAG_RETURN_FILE_MEM)
            {
                common->Error("TODO: FSFLAG_RETURN_FILE_MEM");
            }

            return file;
        }
    }

    if (fs_debug.GetIntegerValue())
    {
        printf("Can't find %s\n", relativePath);
    }

    return NULL;
}

idFile *idFileSystemLocal::OpenFileRead(const char *relativePath, bool allowCopyFiles, const char *gamedir)
{
    return OpenFileReadFlags(relativePath, FSFLAG_SEARCH_DIRS, allowCopyFiles, gamedir);
}

void CreateOSPath(const char* OSPath)
{
    char* ofs;

    if ( strstr( OSPath, ".." ) || strstr( OSPath, "::" ) )
	{
		return;
	}

    for (ofs = (char*)&OSPath[1]; *ofs; ofs++)
    {
        if (*ofs == '/')
        {
            *ofs = 0;
            std::filesystem::create_directory(OSPath);
            *ofs = '/';
        }
    }
}

idFile *idFileSystemLocal::OpenFileWrite(const char *relativePath, const char *basePath)
{
    const char* path;
    std::string OSpath;
    idFile_Permanent* f;

    path = cvarSystem->GetCvarString(basePath);
    if (!path[0])
    {
        path = fs_savepath.GetValueString();
    }

    OSpath = std::string(path) + "/" + gameFolder + "/" + relativePath;

    printf("writing to: %s\n", OSpath.c_str());
    CreateOSPath(OSpath.c_str());

    f = new idFile_Permanent();
    f->o = OpenOSFile(OSpath.c_str(), FS_WRITE);
    if (!f->o)
    {
        delete f;
        return NULL;
    }
    f->name = relativePath;
    f->fullpath = OSpath;
    f->mode = (1 << FS_WRITE);
    f->handleSync = false;
    f->fileSize = 0;

    return f;
}

int idFileSystemLocal::ReadFile(const char *relativePath, void **buffer, ID_TIME_T *timestamp)
{
    idFile* f;
    uint8_t* buf;
    int len;
    bool isConfig;

    if (relativePath == NULL || !relativePath[0])
    {
        common->Error("idFileSystemLocal::ReadFile with empty name\n");
        return 0;
    }

    if (buffer)
        *buffer = NULL;
    
    if (!buffer && timestamp && resourceFiles.size() > 0)
    {
        static idResourceCacheEntry rc;
        int size = 0;
        assert(0);
    }

    buf = NULL;

    if (strstr(relativePath, ".cfg") == relativePath + strlen( relativePath ) - 4)
        isConfig = true;
    else
        isConfig = false;
    
    f = OpenFileRead(relativePath, (buffer != NULL));
    if (!f)
    {
        if (buffer)
            *buffer = NULL;
        return -1;
    }

    len = f->Length();

    if (!buffer)
    {
        CloseFile(f);
        return len;
    }

    buf = new uint8_t[len+1];
    *buffer = buf;

    f->Read(buf, len);

    buf[len] = 0;
    CloseFile(f);

    return len;
}
