#include "CmdSystem.h"
#include "CvarSystem.h"

#include <string>

idCvar net_allowCheats("net_allowCheats", "0", CVAR_BOOL | CVAR_NOCHEAT, "Allows cheats in multiplayer");

struct commandDef_t
{
    commandDef_t* next;
    char* name;
    cmdFunction_t function;
    int flags;
    char* description;
};

class idCmdSystemLocal : public idCmdSystem
{
public:
    virtual void Init();

    virtual void AddCommand(const char *cmdName, cmdFunction_t function, int flags, const char *description);
private:
    static constexpr int MAX_CMD_BUFFER = 0x10000;

    commandDef_t* commands;

    int wait;
    int textLength;
    unsigned char textBuf[MAX_CMD_BUFFER];
};

idCmdSystemLocal cmdSystemLocal;
idCmdSystem* cmdSystem = &cmdSystemLocal;

void idCmdSystemLocal::Init()
{
    textLength = 0;
}

void idCmdSystemLocal::AddCommand(const char *cmdName, cmdFunction_t function, int flags, const char *description)
{
    commandDef_t* cmd;

    for (cmd = commands; cmd; cmd = cmd->next)
    {
        if (strcmp(cmdName, cmd->name) == 0)
        {
            if (function != cmd->function)
                printf("idCmdSystemLocal::AddCommand: %s already defined\n", cmdName);
            return;
        }
    }

    cmd = new commandDef_t;
    cmd->name = strdup(cmdName);
    cmd->function = function;
    cmd->flags = flags;
    cmd->description = strdup(description);
    cmd->next = commands;
    commands = cmd;
}
