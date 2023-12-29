#pragma once

enum core_t
{
    CORE_ANY = -1,
    CORE_0A,
    CORE_0B,
    CORE_1A,
    CORE_1B,
    CORE_2A,
    CORE_2B,
};

#include <pthread.h>

struct signalHandle_t
{
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    int waiting;
    bool manualReset;
    bool signaled;
};

typedef pthread_mutex_t mutexHandle_t;
typedef int interlockedInt_t;

/*
================================================================================================

	Interlocked Integer

================================================================================================
*/

/*
========================
Sys_InterlockedIncrement
========================
*/
interlockedInt_t Sys_InterlockedIncrement( interlockedInt_t& value );

/*
========================
Sys_InterlockedDecrement
========================
*/
interlockedInt_t Sys_InterlockedDecrement( interlockedInt_t& value );

/*
========================
Sys_InterlockedAdd
========================
*/
interlockedInt_t Sys_InterlockedAdd( interlockedInt_t& value, interlockedInt_t i );

/*
========================
Sys_InterlockedSub
========================
*/
interlockedInt_t Sys_InterlockedSub( interlockedInt_t& value, interlockedInt_t i );

/*
========================
Sys_InterlockedExchange
========================
*/
interlockedInt_t Sys_InterlockedExchange( interlockedInt_t& value, interlockedInt_t exchange );

/*
========================
Sys_InterlockedCompareExchange
========================
*/
interlockedInt_t Sys_InterlockedCompareExchange( interlockedInt_t& value, interlockedInt_t comparand, interlockedInt_t exchange );
/*
================================================================================================

	Interlocked Pointer

================================================================================================
*/

/*
========================
Sys_InterlockedExchangePointer
========================
*/
void* Sys_InterlockedExchangePointer( void*& ptr, void* exchange );

/*
========================
Sys_InterlockedCompareExchangePointer
========================
*/
void* Sys_InterlockedCompareExchangePointer( void*& ptr, void* comparand, void* exchange );

typedef unsigned int (*xthread_t)(void*);

enum xthreadPriority
{
    THREAD_LOWEST,
    THREAD_BELOW_NORMAL,
    THREAD_NORMAL,
    THREAD_ABOVE_NORMAL,
    THREAD_HIGHEST
};

#define DEFAULT_THREAD_STACK_SIZE (256*1024)