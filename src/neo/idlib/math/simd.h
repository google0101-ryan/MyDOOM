#pragma once

#include "neo/sys/sys_public.h"

class idSimd
{
public:
    static void Init();
    static void InitProcessor(const char* module, bool forceGeneric);
    static void Shutdown();
};

class idSimdProcessor
{
public:
    cpuid_t cpuid;
};