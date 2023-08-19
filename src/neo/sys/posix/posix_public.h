#pragma once

#include <signal.h>
#include "neo/sys/sys_public.h"

extern xthreadInfo asyncThread;

void Posix_EarlyInit(void);