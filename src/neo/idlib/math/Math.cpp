#include "Math.h"

#include <math.h>

union _flint
{
    int i;
    float f;
};

uint32_t idMath::iSqrt[SQRT_TABLE_SIZE];

void idMath::Init()
{
    _flint fi, fo;

    for (int i = 0; i < SQRT_TABLE_SIZE; i++)
    {
        fi.i	 = ((EXP_BIAS-1) << EXP_POS) | (i << LOOKUP_POS);
        fo.f	 = (float)( 1.0 / sqrt( fi.f ) );
        iSqrt[i] = ((uint32_t)(((fo.i + (1<<(SEED_POS-2))) >> SEED_POS) & 0xFF))<<SEED_POS;
    }

    iSqrt[SQRT_TABLE_SIZE / 2] = ((uint32_t)(0xFF)) << (SEED_POS);
}