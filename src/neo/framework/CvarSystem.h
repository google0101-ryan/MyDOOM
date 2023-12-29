#pragma once

#include "idlib/precompiled.h"
#include <stdlib.h>
#include <string.h>

typedef enum 
{
	CVAR_ALL				= -1,		// all flags
	CVAR_BOOL				= BIT(0),	// variable is a boolean
	CVAR_INTEGER			= BIT(1),	// variable is an integer
	CVAR_FLOAT				= BIT(2),	// variable is a float
	CVAR_SYSTEM				= BIT(3),	// system variable
	CVAR_RENDERER			= BIT(4),	// renderer variable
	CVAR_SOUND				= BIT(5),	// sound variable
	CVAR_GUI				= BIT(6),	// gui variable
	CVAR_GAME				= BIT(7),	// game variable
	CVAR_TOOL				= BIT(8),	// tool variable
	CVAR_SERVERINFO			= BIT(10),	// sent from servers, available to menu
	CVAR_NETWORKSYNC		= BIT(11),	// cvar is synced from the server to clients
	CVAR_STATIC				= BIT(12),	// statically declared, not user created
	CVAR_CHEAT				= BIT(13),	// variable is considered a cheat
	CVAR_NOCHEAT			= BIT(14),	// variable is not considered a cheat
	CVAR_INIT				= BIT(15),	// can only be set from the command-line
	CVAR_ROM				= BIT(16),	// display only, cannot be set by user at all
	CVAR_ARCHIVE			= BIT(17),	// set to cause it to be saved to a config file
	CVAR_MODIFIED			= BIT(18)	// set when the variable is modified
} cvarFlags_t;

class idCvar
{
    friend class idCvarSystemLocal;
public:
    idCvar() {}
    idCvar( const char *name, const char *value, int flags, const char *description);
	idCvar( const char *name, const char *value, int flags, const char *description,
									float valueMin, float valueMax);
	idCvar( const char *name, const char *value, int flags, const char *description,
									const char **valueStrings);
    
    const char* GetName() const {return internalVar->name;}
    const char* GetValueString() const {return internalVar->value;}
    const char* GetDescription() const {return internalVar->description;}

    float GetMinValue() const {return internalVar->valueMin;}
    float GetMaxValue() const {return internalVar->valueMax;}
    int GetIntegerValue() const {return integerValue;}

    int GetFlags() const {return internalVar->flags;}
    const char** GetValueStrings() const {return valueStrings;}

    void SetInternalVar(idCvar* cvar) {internalVar = cvar;}
    
    static void RegisterStaticCvars();

    void SetValue(const char* val) {value = val; integerValue = atoi(value);}
protected:
    const char* name;
    const char* value;
    const char* description;
    int flags;
    float valueMin;
    float valueMax;
    const char** valueStrings;
    int integerValue;
    float floatValue;
    idCvar* internalVar;
    idCvar* next;
private:
    void Init(const char *name, const char *value, int flags, const char *description,
				float valueMin, float valueMax, const char **valueStrings);
    
    static idCvar* staticVars;
};

inline idCvar::idCvar(const char *name, const char *value, int flags, const char *description)
{
    Init(name, value, flags, description, 1, -1, NULL);
}

inline idCvar::idCvar( const char *name, const char *value, int flags, const char *description,
							float valueMin, float valueMax) {
	Init( name, value, flags, description, valueMin, valueMax, NULL);
}

inline idCvar::idCvar( const char *name, const char *value, int flags, const char *description,
							const char **valueStrings) {
	Init( name, value, flags, description, 1, -1, valueStrings);
}

class idCvarSystem
{
public:
    virtual void Init() = 0;

    virtual void Register(idCvar* cvar) = 0;
    virtual void SetModifiedFlags(int flags) = 0;

    virtual void SetCvarString(const char* cvar, const char* val) = 0;
    virtual const char* GetCvarString(const char* cvar) = 0;
};

extern idCvarSystem* cvarSystem;

inline void idCvar::Init(const char *name, const char *value, int flags, const char *description,
							float valueMin, float valueMax, const char **valueStrings)
{
    this->name = name;
    this->value = value;
    this->flags = flags;
    this->description = description;
    this->flags = flags | CVAR_STATIC;
    this->valueMin = valueMin;
    this->valueMax = valueMax;
    this->valueStrings = valueStrings;
    this->integerValue = 0;
    this->floatValue = 0.0f;
    this->internalVar = this;
    if (staticVars != (idCvar*)0xFFFFFFFF)
    {
        this->next = staticVars;
        staticVars = this;
    }
    else
    {
        cvarSystem->Register(this);
    }
}

inline void idCvar::RegisterStaticCvars()
{
    if ( staticVars != (idCvar *)0xFFFFFFFF ) 
    {
		for ( idCvar *cvar = staticVars; cvar; cvar = cvar->next ) {
			cvarSystem->Register( cvar );
		}
		staticVars = (idCvar *)0xFFFFFFFF;
	}
}
