#pragma once

#include "../math/Math.h"
#include "../math/Vector.h"
#include <cmath>

typedef unsigned short halfFloat_t;
// GPU half-float bit patterns
#define HF_MANTISSA(x)	(x&1023)
#define HF_EXP(x)		((x&32767)>>10)
#define HF_SIGN(x)		((x&32768)?-1:1)

/*
========================
F16toF32
========================
*/
inline float F16toF32( halfFloat_t x ) 
{
	int e = HF_EXP( x );
	int m = HF_MANTISSA( x );
	int s = HF_SIGN( x );

	if ( 0 < e && e < 31 ) {
		return s * powf( 2.0f, ( e - 15.0f ) ) * ( 1 + m / 1024.0f );
	} else if ( m == 0 ) {
        return s * 0.0f;
	}
    return s * powf( 2.0f, -14.0f ) * ( m / 1024.0f );
}

inline halfFloat_t F32toF16( float a ) {
	unsigned int f = *(unsigned *)( &a );
	unsigned int signbit  = ( f & 0x80000000 ) >> 16;
	int exponent = ( ( f & 0x7F800000 ) >> 23 ) - 112;
	unsigned int mantissa = ( f & 0x007FFFFF );

	if ( exponent <= 0 ) {
		return 0;
	}
	if ( exponent > 30 ) {
		return (halfFloat_t)( signbit | 0x7BFF );
	}

	return (halfFloat_t)( signbit | ( exponent << 10 ) | ( mantissa >> 13 ) );
}


class idDrawVert
{
public:
	void Clear();

	void SetTexCoord(float s, float t);
	void SetTexCoordS(float s);
	void SetTexCoordT(float t);

	void SetColor(uint32_t color);
public:
    idVec3 xyz;
    halfFloat_t st[2];
    uint8_t normal[4];
    uint8_t tangent[4];
    uint8_t color[4];
    uint8_t color2[4];
};

inline void idDrawVert::Clear()
{
	*reinterpret_cast<uint32_t*>(&this->xyz.x) = 0;
	*reinterpret_cast<uint32_t*>(&this->xyz.y) = 0;
	*reinterpret_cast<uint32_t*>(&this->xyz.z) = 0;
	*reinterpret_cast<uint32_t*>(&this->st) = 0;
	*reinterpret_cast<uint32_t*>(&this->normal) = 0x00FF8080;
	*reinterpret_cast<uint32_t*>(&this->tangent) = 0xFF8080FF;
	*reinterpret_cast<uint32_t*>(&this->color) = 0;
	*reinterpret_cast<uint32_t*>(&this->color2) = 0;
}

inline void idDrawVert::SetTexCoord(float s, float t)
{
	SetTexCoordS(s);
	SetTexCoordT(t);
}

inline void idDrawVert::SetTexCoordS(float s)
{
	st[0] = F32toF16(s);
}

inline void idDrawVert::SetTexCoordT(float t)
{
	st[1] = F32toF16(t);
}

inline void idDrawVert::SetColor(uint32_t color)
{
	*reinterpret_cast<uint32_t*>(this->color) = color;
}

class idShadowVert
{
public:
    idVec4 xyzw;
};

class idShadowVertSkinned
{
public:
    idVec4 xyzw;
    uint8_t color[4];
    uint8_t color2[4];
    uint8_t pad[8];
};