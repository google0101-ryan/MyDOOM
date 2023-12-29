#pragma once

#include <cstdio>
#include <cassert>
#include <cstdint>

#include "sys_threading.h"

typedef struct
{
    const char* name;
    int threadHandle;
    unsigned long threadId;
} xthreadInfo;

extern xthreadInfo* g_threads[10];

enum cpuid_t 
{
	CPUID_NONE							= 0x00000,
	CPUID_UNSUPPORTED					= 0x00001,	// unsupported (386/486)
	CPUID_GENERIC						= 0x00002,	// unrecognized processor
	CPUID_INTEL							= 0x00004,	// Intel
	CPUID_AMD							= 0x00008,	// AMD
	CPUID_MMX							= 0x00010,	// Multi Media Extensions
	CPUID_3DNOW							= 0x00020,	// 3DNow!
	CPUID_SSE							= 0x00040,	// Streaming SIMD Extensions
	CPUID_SSE2							= 0x00080,	// Streaming SIMD Extensions 2
	CPUID_SSE3							= 0x00100,	// Streaming SIMD Extentions 3 aka Prescott's New Instructions
	CPUID_ALTIVEC						= 0x00200,	// AltiVec
	CPUID_HTT							= 0x01000,	// Hyper-Threading Technology
	CPUID_CMOV							= 0x02000,	// Conditional Move (CMOV) and fast floating point comparison (FCOMI) instructions
	CPUID_FTZ							= 0x04000,	// Flush-To-Zero mode (denormal results are flushed to zero)
	CPUID_DAZ							= 0x08000,	// Denormals-Are-Zero mode (denormal source operands are set to zero)
	CPUID_XENON							= 0x10000,	// Xbox 360
	CPUID_CELL							= 0x20000	// PS3
};

int Sys_Milliseconds(void);

void Sys_InitNetworking();

void Sys_SignalCreate(signalHandle_t& handle, bool manualReset);
void Sys_SignalDestroy(signalHandle_t& handle);
void Sys_SignalRaise(signalHandle_t& handle);
void Sys_SignalClear(signalHandle_t& handle);
bool Sys_SignalWait(signalHandle_t& handle, int timeout);

void Sys_MutexCreate(mutexHandle_t& handle);
void Sys_MutexDestroy(mutexHandle_t& handle);
bool Sys_MutexLock(mutexHandle_t& handle, bool blocking);
void Sys_MutexUnlock(mutexHandle_t& handle);

void Sys_DestroyThread(uintptr_t threadHandle);
uintptr_t Sys_CreateThread( xthread_t function, void* parms, xthreadPriority priority,
									  const char* name, core_t core, int stackSize = DEFAULT_THREAD_STACK_SIZE,
									  bool suspended = false );

typedef FILE* idFileHandle;

enum sysEventType_t
{
	SE_NONE,				// evTime is still valid
	SE_KEY,					// evValue is a key code, evValue2 is the down flag
	SE_CHAR,				// evValue is an Unicode UTF-32 char (or non-surrogate UTF-16)
	SE_MOUSE,				// evValue and evValue2 are relative signed x / y moves
	SE_MOUSE_ABSOLUTE,		// evValue and evValue2 are absolute coordinates in the window's client area.
	SE_MOUSE_LEAVE,			// evValue and evValue2 are meaninless, this indicates the mouse has left the client area.
	SE_JOYSTICK,			// evValue is an axis number and evValue2 is the current state (-127 to 127)
	SE_CONSOLE				// evPtr is a char*, from typing something at a non-game console
};

struct sysEvent_t
{
	sysEventType_t evType;
	int evValue;
	int evValue2;
	int evPtrLength;
	void* evPtr;
};