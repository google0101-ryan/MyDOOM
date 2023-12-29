#include "idlib/precompiled.h"
#include "DeclManager.h"
#include "CvarSystem.h"
#include "Filesystem.h"
#include <renderer/Material.h>
#include <vector>
#include <unordered_map>

class idDeclType
{
public:
    std::string typeName;
    declType_t type;
};

class idDeclFolder
{
public:
    std::string folder;
    std::string extension;
    declType_t defaultType;
};

class idDeclFile;

class idDeclLocal : public idDeclBase
{
    friend class idDeclFile;
    friend class idDeclManagerLocal;
public:
    idDeclLocal();
    virtual ~idDeclLocal() {}

	void AllocateSelf();
	void ParseLocal();
private:
	virtual bool SetDefaultText() {return false;}

    idDecl* self;

    std::string name;
    char* textSource;
    int textLength;
    int compressedLength;
    idDeclFile* sourceFile;
    int sourceTextOffset;
    int sourceTextLength;
    int sourceLine;
    int checksum;
    declType_t type;
    declState_t declState;
    int index;

    bool parsedOutsideLevelLoad;
    bool everReferenced;
    bool referencedThisLevel;
    bool redefinedInReload;

    idDeclLocal* nextInFile;
};

class idDeclFile
{
public:
    idDeclFile();
    idDeclFile(const char* fileName, declType_t defaultType);

    void Reload(bool force);
public:
    std::string fileName;
    declType_t defaultType;

    ID_TIME_T timestamp;
    int checksum;
    int fileSize;
    int numLines;

    idDeclLocal* decls;
};

class idDeclManagerLocal : public idDeclManager
{
public:
    virtual void Init();
    virtual void RegisterDeclType(const char* name, declType_t declType);
    virtual void RegisterDeclFolder(const char* name, const char *extension, declType_t declType);
	virtual idDecl* FindType(declType_t type, const char* name, bool makeDefault);
	idDeclLocal* FindTypeWithoutParsing( declType_t type, const char *name, bool makeDefault);

	const idMaterial* FindMaterial(const char* name, bool makeDefault = true);
public:
	static void MakeNameCanonical(const char* name, char* result, int maxLength);

	idDeclType* GetDeclType(int type) {return declTypes[type];}

	int indent;
private:
	idSysMutex mutex;

    static idCvar decl_show;

    std::unordered_map<int, idDeclType*> declTypes;
    std::vector<idDeclFolder*> declFolders;

    std::vector<idDeclFile*> loadedFiles;

	std::unordered_map<const char*, idDeclLocal*> linearList[DECL_MAX_TYPES];

	idDeclFile implicitDecls;

	bool insideLevelLoad;
};

idCvar idDeclManagerLocal::decl_show("decl_show", "0", CVAR_SYSTEM, "set to 1 to print parses, 2 to also print references");

idDeclManagerLocal declManagerLocal;
idDeclManager* declManager = &declManagerLocal;

idDeclLocal::idDeclLocal()
{
    name = "unnamed";
    textSource = NULL;
    textLength = 0;
    compressedLength = 0;
    sourceFile = NULL;
    sourceTextOffset = 0;
    sourceTextLength = 0;
    sourceLine = 0;
    checksum = 0;
    type = DECL_ENTITYDEF;
    index = 0;
    declState = DS_UNPARSED;
    parsedOutsideLevelLoad = false;
    referencedThisLevel = false;
    everReferenced = false;
    redefinedInReload = false;
    nextInFile = NULL;
}

void idDeclLocal::AllocateSelf()
{
	if (!self)
	{
		self = new idDecl;
		self->base = this;
	}
}

void idDeclLocal::ParseLocal()
{
	bool generatedDefaultText = false;

	AllocateSelf();

	printf("parsing %s %s\n", declManagerLocal.GetDeclType(type)->typeName.c_str(), name.c_str());

	if (!textSource)
		generatedDefaultText = SetDefaultText();
	
	declManagerLocal.indent++;

	if (!textSource)
	{
		
	}
}

idDeclFile::idDeclFile()
{
    this->fileName = "<implicit file>";
    this->defaultType = DECL_MAX_TYPES;
    this->timestamp = 0;
    this->checksum = 0;
    this->fileSize = 0;
    this->numLines = 0;
    this->decls = NULL;
}

idDeclFile::idDeclFile(const char *fileName, declType_t defaultType)
{
    this->fileName = fileName;
    this->defaultType = defaultType;
    this->timestamp = 0;
    this->checksum = 0;
    this->fileSize = 0;
    this->numLines = 0;
    this->decls = NULL;
}

void idDeclManagerLocal::Init()
{
    printf("----- Initializing Decls -----\n");

    RegisterDeclType("table", DECL_TABLE);
    RegisterDeclType("material", DECL_MATERIAL);
    RegisterDeclType("skin", DECL_SKIN);
    RegisterDeclType("sound", DECL_SOUND);
    
    RegisterDeclType("entityDef", DECL_ENTITYDEF);
    RegisterDeclType("mapDef", DECL_MAPDEF);
    RegisterDeclType("fx", DECL_FX);
    RegisterDeclType("particle", DECL_PARTICLE);
    RegisterDeclType("articulatedFigure", DECL_AF);
    RegisterDeclType("pda", DECL_PDA);
    RegisterDeclType("email", DECL_EMAIL);
    RegisterDeclType("video", DECL_VIDEO);
    RegisterDeclType("audio", DECL_AUDIO);

    RegisterDeclFolder("materials", ".mtr", DECL_MATERIAL);

    printf("------------------------------\n");
}

void idDeclManagerLocal::RegisterDeclType(const char *typeName, declType_t type)
{
    idDeclType* declType;

    if (declTypes[(int)type])
    {
        common->Warning("idDeclManager::RegisterDeclType: type \"%s\" already exists", typeName);
        return;
    }

    declType = new idDeclType;
    declType->typeName = typeName;
    declType->type = type;
    
    if ((int)type + 1 > declTypes.size())
        declTypes.reserve((int)type + 1);
    declTypes[type] = declType;
}

void idDeclManagerLocal::RegisterDeclFolder(const char *folder, const char *extension, declType_t defaultType)
{
    int i, j;
    std::string fileName;
    idDeclFolder *declFolder;
    idFileList* fileList;
    idDeclFile* df;

    for (i = 0; i < declFolders.size(); i++)
        if (declFolders[i]->folder == folder && declFolders[i]->extension == extension)
            break;
    if (i < declFolders.size())
    {
        declFolder = declFolders[i];
    }
    else
    {
        declFolder = new idDeclFolder;
        declFolder->folder = folder;
        declFolder->extension = extension;
        declFolder->defaultType = defaultType;
        declFolders.push_back(declFolder);
    }

    fileList = fileSystem->ListFiles(declFolder->folder.c_str(), declFolder->extension.c_str(), true);

    for (i = 0; i < fileList->GetNumFiles(); i++)
    {
        fileName = declFolder->folder + "/" + fileList->GetFile(i);

        for (j = 0; j < loadedFiles.size(); j++)
        {
            if (fileName == loadedFiles[j]->fileName)
                break;
        }
        if (j < loadedFiles.size())
            df = loadedFiles[j];
        else
        {
            df = new idDeclFile(fileName.c_str(), defaultType);
            loadedFiles.push_back(df);
        }

        printf("Loading and parsing \"%s\"\n", df->fileName.c_str());
    }
    
    delete fileList;
}

void idDeclManagerLocal::MakeNameCanonical(const char *name, char *result, int maxLength)
{
	int i, lastDot;

	lastDot = -1;
	for (i = 0; i < maxLength && name[i] != '\0'; i++)
	{
		int c = name[i];
		if (c == '\\')
			result[i] = '/';
		else if (c == '.')
		{
			lastDot = i;
			result[i] = c;
		}
		else
			result[i] = tolower(c);
	}
	if (lastDot != -1)
		result[lastDot] = '\0';
	else
		result[i] = '\0';
}

idDecl *idDeclManagerLocal::FindType(declType_t type, const char *name, bool makeDefault)
{
	idDeclLocal* decl;

	idScopedCriticalSection cs(mutex);

	if (!name || !name[0])
	{
		name = "_emptyName";
	}

	decl = FindTypeWithoutParsing(type, name, makeDefault);
	if (!decl)
		return NULL;
	
	decl->AllocateSelf();

	if (decl->declState == DS_UNPARSED)
	{
		
	}

	decl->referencedThisLevel = true;
	decl->everReferenced = true;
	if (insideLevelLoad)
		decl->parsedOutsideLevelLoad = false;
	return decl->self;
}

idDeclLocal *idDeclManagerLocal::FindTypeWithoutParsing(declType_t type, const char *name, bool makeDefault)
{
	int typeIndex = (int)type;
	int i, hash;

	if (typeIndex < 0 || declTypes[typeIndex] == NULL || typeIndex >= DECL_MAX_TYPES)
	{
		common->Error("idDeclManager::FindTypeWithoutParsing: bad type: %i (%ld %p)\n", typeIndex, declTypes.size(), declTypes[typeIndex]);
		return NULL;
	}

	char canonicalName[MAX_STRING_CHARS];

	MakeNameCanonical(name, canonicalName, sizeof(canonicalName));

	if (linearList[typeIndex][canonicalName] != NULL)
	{
		return linearList[typeIndex][canonicalName];
	}

	if (!makeDefault)
		return NULL;
	
	idDeclLocal* decl = new idDeclLocal;
	decl->self = NULL;
	decl->name = canonicalName;
	decl->type = type;
	decl->declState = DS_UNPARSED;
	decl->textSource = NULL;
	decl->textLength = 0;
	decl->sourceFile = &implicitDecls;
	decl->referencedThisLevel = false;
	decl->everReferenced = false;
	decl->parsedOutsideLevelLoad = !insideLevelLoad;

	linearList[typeIndex][canonicalName] = decl;
	return decl;
}

const idMaterial *idDeclManagerLocal::FindMaterial(const char *name, bool makeDefault)
{
	return static_cast<const idMaterial*>(FindType(DECL_MATERIAL, name, makeDefault));
}
