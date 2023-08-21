#include "idlib/precompiled.h"
#include "CvarSystem.h"

#include <string>

idCvar* idCvar::staticVars = NULL;

extern idCvar net_allowCheats;

class idInternalCvar : public idCvar
{
    friend class idCvarSystemLocal;
public:
    idInternalCvar();
	idInternalCvar( const char *newName, const char *newValue, int newFlags );
	idInternalCvar( const idCvar *cvar );
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