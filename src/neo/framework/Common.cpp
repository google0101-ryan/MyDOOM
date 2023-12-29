#include "idlib/precompiled.h"
#include "sys/sys_public.h"
#include "CmdSystem.h"
#include "CvarSystem.h"
#include "KeyInput.h"
#include "Filesystem.h"
#include "DeclManager.h"
#include "EventLoop.h"
#include "idlib/ParallelJobList.h"
#include "../renderer/RenderSystem.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_CONSOLE_LINES 32
int com_numConsoleLines;
idCmdArgs com_consoleLines[MAX_CONSOLE_LINES];

class idCommonLocal : public idCommon
{
public:
    void Init(int argc, const char** argv, const char* cmdline);
    
    void Error(const char* msg, ...);
    void Warning(const char* msg, ...);

    void StartupVariable(const char* match);
private:
    void ParseCommandLine(int argc, const char* const * argv);

    bool consoleUsed = false;
};

idCommonLocal commonLocal;
idCommon* common = (idCommon*)&commonLocal;

idCvar com_allowConsole("com_allowConsole", "0", CVAR_BOOL | CVAR_SYSTEM | CVAR_INIT, "allow toggling console with the tilde key");
idCvar com_forceGenericSIMD("com_forceGenericSIMD", "0", CVAR_BOOL | CVAR_SYSTEM | CVAR_NOCHEAT, "force generic platform independent SIMD");

const char* GetCmdline(int argc, const char** argv)
{
    char* buf = new char[argc*100];

    bool isSpace = false;

    for (int i = 0; i < argc; i++)
    {
        if (isSpace)
            strcat(buf, " ");
        strcat(buf, argv[i]);
        isSpace = true;
    }

    return buf;
}

void idCommonLocal::StartupVariable(const char* match)
{
    int i = 0;
    while (i < com_numConsoleLines)
    {
        if (strcmp(com_consoleLines[i].Argv(0), "set") != 0)
        {
            i++;
            continue;
        }
        const char* s = com_consoleLines[i].Argv(1);

        if (!match || !strcasecmp(s, match))
        {
            cvarSystem->SetCvarString(s, com_consoleLines[i].Argv(2));
        }
        i++;
    }
}

void idCommonLocal::Init(int argc, const char **argv, const char *cmdline)
{
    idLib::common = common;

    idLib::Init();

    if (cmdline)
        printf("TODO: tokenize cmdline\n");
    
    ParseCommandLine(argc, argv);

    cmdSystem->Init();

    cvarSystem->Init();

    idCvar::RegisterStaticCvars();

    printf("QA Timing INIT: %06dms\n", Sys_Milliseconds());

    printf("%s.%d%s %s %s %s\n", "D3BFG 1", 1400, "-debug", "linux-x86", __DATE__, __TIME__);

    idKeyInput::Init();

    Sys_InitNetworking();

    StartupVariable(NULL);
    
    consoleUsed = com_allowConsole.GetIntegerValue();

    idSimd::InitProcessor("doom", com_forceGenericSIMD.GetIntegerValue());

    fileSystem->Init();

    declManager->Init();

    eventLoop->Init();

    parallelJobManager->Init();

    renderSystem->Init();

	const idMaterial* splashScreen = declManager->FindMaterial("guis/assets/splash/legal_english");

	

    printf("--- Common Initialization Complete ---\n");

    printf("QA Timing IIS: %06dms\n", Sys_Milliseconds());
}

void idCommonLocal::Error(const char *msg, ...)
{
    va_list argptr;

    printf("ERROR: ");

    va_start(argptr, msg);
    vprintf(msg, argptr);
    va_end(argptr);

    printf("\n");

    exit(1);
}

void idCommonLocal::Warning(const char *msg, ...)
{
    va_list argptr;

    printf("WARN: ");

    va_start(argptr, msg);
    vprintf(msg, argptr);
    va_end(argptr);

    printf("\n");
}

void idCommonLocal::ParseCommandLine(int argc, const char *const *argv)
{
    int i, current_count;

    com_numConsoleLines = 0;
    current_count = 0;
    for (i = 0; i < argc; i++)
    {
        if (!strcmp(argv[i], "+connect_lobby"))
            Error("TODO: Handle Bootable invite");
        else if (argv[i][0] == '+')
        {
            com_numConsoleLines++;
            com_consoleLines[com_numConsoleLines-1].AppendArg(argv[i]+1);
        }
        else
        {
            if (!com_numConsoleLines)
                com_numConsoleLines++;
            com_consoleLines[com_numConsoleLines-1].AppendArg(argv[i]);
        }
    }
}
