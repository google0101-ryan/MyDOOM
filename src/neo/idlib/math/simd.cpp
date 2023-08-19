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