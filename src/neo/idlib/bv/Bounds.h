#pragma once

#include <idlib/math/Vector.h>

class idBounds
{
public:
	void Zero();
private:
	idVec3 b[2];
};

inline void idBounds::Zero()
{
	b[0][0] = b[0][1] = b[0][2] =
	b[1][0] = b[1][1] = b[1][2] = 0;
}