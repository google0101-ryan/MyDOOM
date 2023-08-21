#include "idlib/precompiled.h"
#include "sys/sys_public.h"
#include "CmdSystem.h"
#include "CvarSystem.h"

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
private:
    void ParseCommandLine(int argc, const char* const * argv);
};

idCommonLocal commonLocal;
idCommon* common = (idCommon*)&commonLocal;

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
}

void idCommonLocal::Error(const char *msg, ...)
{
    va_list argptr;

    printf("ERROR: ");

    va_start(argptr, msg);
    vprintf(msg, argptr);
    va_end(argptr);

    printf("\n");
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
