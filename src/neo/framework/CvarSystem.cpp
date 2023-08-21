#include "idlib/precompiled.h"
#include "CvarSystem.h"
#include "CmdSystem.h"

#include <string>
#include <unordered_map>

idCvar* idCvar::staticVars = NULL;

extern idCvar net_allowCheats;

const char** CopyValueStrings(const char** strings)
{
    int i, totalLength;
    const char** ptr;
    char* str;

    if (!strings)
        return NULL;
    
    totalLength = 0;
    for (i = 0; strings[i] != NULL; i++)
        totalLength += strlen(strings[i]);
    
    ptr = (const char**)malloc((i+1) * sizeof(char*) + totalLength);
    str = (char *) (((unsigned char *)ptr) + ( i + 1 ) * sizeof( char * ) );

    for ( i = 0; strings[i] != NULL; i++ ) 
    {
		ptr[i] = str;
		strcpy( str, strings[i] );
		str += strlen( strings[i] ) + 1;
	}
    ptr[i] = NULL;

    return ptr;
}

class idInternalCvar : public idCvar
{
    friend class idCvarSystemLocal;
public:
    idInternalCvar();
	idInternalCvar( const char *newName, const char *newValue, int newFlags );
	idInternalCvar( const idCvar *cvar );

    void Set(const char* newValue, bool force, bool fromServer);
private:
    std::string nameString;
    std::string resetString;
    std::string valueString;
    std::string descriptionString;
};

idInternalCvar::idInternalCvar()
{}

idInternalCvar::idInternalCvar( const char *newName, const char *newValue, int newFlags)
{
    nameString = newName;
    name = nameString.c_str();
    valueString = newValue;
    value = valueString.c_str();
    resetString = newValue;
    descriptionString = "";
    description = descriptionString.c_str();
    flags = (newFlags & ~CVAR_STATIC) | CVAR_MODIFIED;
    valueMin = 1;
    valueMax = -1;
    valueStrings = NULL;
    internalVar = this;
}

idInternalCvar::idInternalCvar(const idCvar *cvar)
{
    nameString = cvar->GetName();
    name = nameString.c_str();
    valueString = cvar->GetValueString();
    value = valueString.c_str();
    resetString = cvar->GetValueString();
    descriptionString = cvar->GetDescription();
    description = descriptionString.c_str();
    flags = cvar->GetFlags() | CVAR_MODIFIED;
    valueMin = cvar->GetMinValue();
    valueMax = cvar->GetMaxValue();
    valueStrings = CopyValueStrings(cvar->GetValueStrings());
    internalVar = this;
}

void idInternalCvar::Set(const char *newValue, bool force, bool fromServer)
{
    if (!newValue)
        newValue = resetString.c_str();
    
    if (!force)
    {
        if (flags & CVAR_ROM)
        {
            printf("%s is read-only\n", name);
            return;
        }
        if (flags & CVAR_INIT)
        {
            printf("%s is write-protected\n", name);
            return;
        }
    }

    if (!strcmp(valueString.c_str(), newValue))
    {
        return;
    }

    valueString = newValue;
    value = valueString.c_str();
    
    flags |= CVAR_MODIFIED;
}

class idCvarSystemLocal : public idCvarSystem
{
public:
    void Init();
    void Register(idCvar* cvar);

    idInternalCvar* FindInternal(const char* name);
    void SetInternal(const char* name, const char* value, int flags);

    static void Set_f(const idCmdArgs& args);
private:
    std::unordered_map<std::string, idInternalCvar*> cvars;
};

idCvarSystemLocal cvarSystemLocal;
idCvarSystem* cvarSystem = &cvarSystemLocal;

void idCvarSystemLocal::Init()
{
    cmdSystem->AddCommand("set", Set_f, CMD_FL_SYSTEM, "sets a cvar");
}

void idCvarSystemLocal::Register(idCvar *cvar)
{
    int hash;
    idInternalCvar* internal;

    cvar->SetInternalVar(cvar);

    internal = FindInternal(cvar->GetName());

    if (internal)
    {
        common->Error("TODO: Update CVAR\n");
    }
    else
    {
        internal = new idInternalCvar(cvar);
        cvars[internal->nameString.c_str()] = internal;
    }
}

idInternalCvar *idCvarSystemLocal::FindInternal(const char *name)
{
    if (cvars.find(name) == cvars.end())
    {
        return NULL;
    }

    return cvars[name];
}

void idCvarSystemLocal::SetInternal(const char *name, const char *value, int flags)
{
    idInternalCvar* internal;

    internal = FindInternal(name);

    if (internal)
    {
        internal->Set(value, true, false);
        internal->flags |= flags & ~CVAR_STATIC;
    }
    else
    {
        internal = new idInternalCvar(name, value, flags);
        cvars[name] = internal;
    }
}

void idCvarSystemLocal::Set_f(const idCmdArgs &args)
{
    const char* str;

    str = args.Args(2, args.Argc() - 1, false);
    cvarSystemLocal.SetInternal(args.Argv(1), str, 0);
}
