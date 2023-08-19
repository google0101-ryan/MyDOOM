#include "lib.h"
#include "math/simd.h"
#include "math/Math.h"

bool idLib::mainThreadInitialized = false;
thread_local int idLib::isMainThread = 0;

idCommon* idLib::common = nullptr;

void idLib::Init()
{
    isMainThread = 1;
    mainThreadInitialized = true;

    idSimd::Init();

    idMath::Init();
}