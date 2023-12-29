#include "idlib/precompiled.h"
#include "CvarSystem.h"
#include "CmdSystem.h"

#include <string>
#include <unordered_map>
#include <algorithm>

idCvar* idCvar::staticVars = NULL;

extern idCvar net_allowCheats;

void to_lower(std::string& str)
{
    std::transform(str.begin(), str.end(), str.begin(),
     [](unsigned char c){ return std::tolower(c); });
}

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
	idInternalCvar( idCvar *cvar );

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

idInternalCvar::idInternalCvar(idCvar *cvar)
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
    internalVar = cvar;
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

    if (!strcasecmp(valueString.c_str(), newValue))
    {
        return;
    }

    valueString = newValue;
    value = valueString.c_str();
    internalVar->SetValue(value);
    
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

    void SetModifiedFlags(int flags) {modifiedFlags |= flags;}
    void SetCvarString(const char* cvar, const char* val);
    const char* GetCvarString(const char* cvar);
private:
    std::unordered_map<std::string, idInternalCvar*> cvars;
    int modifiedFlags;
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
        to_lower(internal->nameString);
        cvars[internal->nameString.c_str()] = internal;
    }
}

idInternalCvar *idCvarSystemLocal::FindInternal(const char *name)
{
    std::string _name = name;
    to_lower(_name);
    
    if (cvars.find(_name.c_str()) == cvars.end())
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
        std::string _name = name;
        to_lower(_name);
        cvars[_name.c_str()] = internal;
    }
}

void idCvarSystemLocal::Set_f(const idCmdArgs &args)
{
    const char* str;

    str = args.Args(2, args.Argc() - 1, false);
    cvarSystemLocal.SetInternal(args.Argv(1), str, 0);
}

extern idCvar com_allowConsole;

void idCvarSystemLocal::SetCvarString(const char *cvar, const char *val)
{
    idInternalCvar* internal;

    internal = FindInternal(cvar);

    if (internal)
    {
        internal->Set(val, true, false);
    }
    else
    {
        printf("Making new cvar\n");
        internal = new idInternalCvar(cvar, val, 0);
        std::string _name = cvar;
        to_lower(_name);
        cvars[_name.c_str()] = internal;
    }
}

const char *idCvarSystemLocal::GetCvarString(const char *cvar)
{
    idInternalCvar* internal;

    internal = FindInternal(cvar);

    if (internal)
    {
        return internal->GetValueString();
    }
    else
    {
        common->Warning("Tried to read non-existant cvar!\n");
        return "";
    }
}
