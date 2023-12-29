#pragma once

#include <cstddef>

class idMaterial;

typedef enum
{
    DECL_TABLE = 0,
    DECL_MATERIAL,
    DECL_SKIN,
    DECL_SOUND,
    DECL_ENTITYDEF,
    DECL_MODELDEF,
    DECL_FX,
    DECL_PARTICLE,
    DECL_AF,
    DECL_PDA,
    DECL_VIDEO,
    DECL_AUDIO,
    DECL_EMAIL,
    DECL_MODELEXPORT,
    DECL_MAPDEF,
    DECL_MAX_TYPES = 32
} declType_t;

typedef enum
{
    DS_UNPARSED,
    DS_DEFAULTED,
    DS_PARSED,
} declState_t;

class idDeclBase
{
public:
    ~idDeclBase() {};
};

class idDecl
{
public:
    idDecl() {base = NULL;}
    virtual ~idDecl() {}
public:
    idDeclBase* base;
};

class idDeclManager
{
public:
    virtual void Init() = 0;
    virtual void RegisterDeclType(const char* name, declType_t declType) = 0;
	virtual idDecl* FindType(declType_t type, const char* name, bool makeDefault) = 0;

	virtual const idMaterial* FindMaterial( const char *name, bool makeDefault = true ) = 0;
};

extern idDeclManager* declManager;