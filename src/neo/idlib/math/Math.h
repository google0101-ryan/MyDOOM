#pragma once

#include <xmmintrin.h>
#include <stdint.h>

class idMath
{
public:
    static void Init();
private:
    enum
    {
        LOOKUP_BITS = 8,
        EXP_POS = 23,
        EXP_BIAS = 127,
        LOOKUP_POS				= (EXP_POS-LOOKUP_BITS),
		SEED_POS				= (EXP_POS-8),
        SQRT_TABLE_SIZE = (2 << LOOKUP_BITS)
    };

    static uint32_t iSqrt[SQRT_TABLE_SIZE];
};