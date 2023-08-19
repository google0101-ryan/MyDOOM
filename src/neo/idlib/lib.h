#pragma once

#include <stddef.h>
#include "neo/framework/Common.h"

class idLib
{
private:
    static thread_local int isMainThread;
    static bool mainThreadInitialized;
public:
    static void Init();

    static idCommon* common;
};