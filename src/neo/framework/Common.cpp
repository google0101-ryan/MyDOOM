#include "neo/framework/Common.h"
#include "neo/idlib/lib.h"

#include <stdio.h>
#include <string.h>

class idCommonLocal : public idCommon
{
public:
    void Init(int argc, const char** argv, const char* cmdline);
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
    
    
}
