#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <string>

#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glut.h>

#define BIT(x) (1 << x)

#define __ALIGN_MASK(a, b) (((a) + (b)) & ~(b))
#define ALIGN(a, b) __ALIGN_MASK(a, (typeof(a))(b) - 1)

#define ID_TIME_T int64_t

#include "CmdArgs.h"
#include "Token.h"
#include "../framework/Common.h"
#include "lib.h"
#include "framework/CvarSystem.h"
#include "idlib/math/simd.h"
#include "sys/sys_public.h"
#include "Thread.h"
#include "math/Vector.h"
#include "geometry/DrawVert.h"

class idCvar;

extern idCvar com_developer;