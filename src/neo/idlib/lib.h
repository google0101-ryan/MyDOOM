#pragma once

#include <stddef.h>
#include "neo/framework/Common.h"

#define	MAX_STRING_CHARS		1024
typedef unsigned short triIndex_t;

class idLib
{
private:
    static thread_local int isMainThread;
    static bool mainThreadInitialized;
public:
    static void Init();

    static idCommon* common;
};