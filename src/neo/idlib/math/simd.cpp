#include "simd.h"
#include "simd_generic.h"

idSimdProcessor* generic;
idSimdProcessor* processor;
idSimdProcessor* simdProcessor;

void idSimd::Init()
{
    generic = new idSIMD_generic();
    generic->cpuid = CPUID_GENERIC;
    processor = nullptr;
    simdProcessor = generic;
}

void idSimd::InitProcessor(const char *module, bool forceGeneric)
{
    cpuid_t cpuid;
    idSimdProcessor* newProcessor;

    cpuid = CPUID_GENERIC;

    if (forceGeneric)
    {
        newProcessor = generic;
    }
    else
    {
        if (processor == nullptr)
        {
            processor = generic;
            processor->cpuid = cpuid;
        }
        newProcessor = processor;
    }
}
