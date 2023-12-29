#pragma once

class idJointMat
{
public:
	float mat[3*4];
};

#define JOINTMAT_SIZE (4*3*4)
static_assert(sizeof(idJointMat) == JOINTMAT_SIZE);