#include "sys/sys_public.h"

#include "idlib/precompiled.h"


void Sys_SignalCreate(signalHandle_t &handle, bool manualReset)
{
    handle.manualReset = manualReset;
    handle.signaled = false;
    handle.waiting = 0;
    pthread_mutex_init(&handle.mutex, NULL);
    pthread_cond_init(&handle.cond, NULL);
}

void Sys_SignalDestroy(signalHandle_t &handle)
{
    handle.signaled = false;
    handle.waiting = 0;
    pthread_mutex_destroy(&handle.mutex);
    pthread_cond_destroy(&handle.cond);
}
void Sys_SignalRaise(signalHandle_t &handle)
{
    pthread_mutex_lock(&handle.mutex);

    if (handle.manualReset)
    {
        handle.signaled = true;
        pthread_cond_broadcast(&handle.cond);
    }
    else
    {
        if (handle.waiting > 0)
        {
            pthread_cond_signal(&handle.cond);
        }
        else
        {
            handle.signaled = true;
        }
    }

    pthread_mutex_unlock(&handle.mutex);
}

void Sys_SignalClear(signalHandle_t &handle)
{
    pthread_mutex_lock(&handle.mutex);
    handle.signaled = false;
    pthread_mutex_unlock(&handle.mutex);
}

bool Sys_SignalWait(signalHandle_t &handle, int timeout)
{
    int status;
    pthread_mutex_lock(&handle.mutex);

    if (handle.signaled)
    {
        if (!handle.manualReset)
        {
            handle.signaled = false;
        }

        status = 0;
    }
    else
    {
        ++handle.waiting;
        if (timeout == idSysSignal::WAIT_INFINITE)
        {
            status = pthread_cond_wait(&handle.cond, &handle.mutex);
        }
        else
        {
            timespec ts;

            clock_gettime(CLOCK_REALTIME, &ts);

            ts.tv_nsec += (timeout % 1000) * 1000000;
            ts.tv_sec += timeout / 1000;
            if (ts.tv_nsec >= 1000000000)
            {
                ts.tv_nsec -= 1000000000;
                ts.tv_sec += 1;
            }
            status = pthread_cond_timedwait(&handle.cond, &handle.mutex, &ts);
        }
        --handle.waiting;
    }

    pthread_mutex_unlock(&handle.mutex);

    assert(status == 0 || (timeout != idSysSignal::WAIT_INFINITE && status == ETIMEDOUT));

    return (status == 0);
}

void Sys_MutexCreate(mutexHandle_t &handle)
{
    pthread_mutexattr_t attr;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    pthread_mutex_init(&handle, &attr);

    pthread_mutexattr_destroy(&attr);
}

void Sys_MutexDestroy(mutexHandle_t &handle)
{
    pthread_mutex_destroy(&handle);
}

bool Sys_MutexLock(mutexHandle_t &handle, bool blocking)
{
    if (pthread_mutex_trylock(&handle))
    {
        if (!blocking)
            return false;
        pthread_mutex_lock(&handle);
    }
    return true;
}

void Sys_MutexUnlock(mutexHandle_t &handle)
{
    pthread_mutex_unlock(&handle);
}

void Sys_DestroyThread(uintptr_t threadHandle)
{
    if (!threadHandle)
        return;

    if (pthread_join((pthread_t)threadHandle, NULL))
    {
        idLib::common->Error("ERROR: pthread_join failed\n");
    }
}

typedef void* ( *pthread_function_t )( void* );

uintptr_t Sys_CreateThread(xthread_t function, void *parms, xthreadPriority priority, const char *name, core_t core, int stackSize, bool suspended)
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE))
    {
        idLib::common->Error("ERROR: pthread_attr_setdetachstate %s failed\n", name);
        return (uintptr_t)0;
    }

    pthread_t handle;
    if (pthread_create(&handle, &attr, (pthread_function_t)function, parms))
    {
        idLib::common->Error("ERROR: pthread_create %s failed\n", name);
        return (uintptr_t)0;
    }

    pthread_attr_destroy(&attr);

    return (uintptr_t)handle;
}

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
interlockedInt_t Sys_InterlockedIncrement( interlockedInt_t& value )
{
	// return InterlockedIncrementAcquire( & value );
	return __sync_add_and_fetch( &value, 1 );
}

/*
========================
Sys_InterlockedDecrement
========================
*/
interlockedInt_t Sys_InterlockedDecrement( interlockedInt_t& value )
{
	// return InterlockedDecrementRelease( & value );
	return __sync_sub_and_fetch( &value, 1 );
}

/*
========================
Sys_InterlockedAdd
========================
*/
interlockedInt_t Sys_InterlockedAdd( interlockedInt_t& value, interlockedInt_t i )
{
	//return InterlockedExchangeAdd( & value, i ) + i;
	return __sync_add_and_fetch( &value, i );
}

/*
========================
Sys_InterlockedSub
========================
*/
interlockedInt_t Sys_InterlockedSub( interlockedInt_t& value, interlockedInt_t i )
{
	//return InterlockedExchangeAdd( & value, - i ) - i;
	return __sync_sub_and_fetch( &value, i );
}

/*
========================
Sys_InterlockedExchange
========================
*/
interlockedInt_t Sys_InterlockedExchange( interlockedInt_t& value, interlockedInt_t exchange )
{
	//return InterlockedExchange( & value, exchange );

	// source: http://gcc.gnu.org/onlinedocs/gcc-4.1.1/gcc/Atomic-Builtins.html
	// These builtins perform an atomic compare and swap. That is, if the current value of *ptr is oldval, then write newval into *ptr.
	return __sync_val_compare_and_swap( &value, value, exchange );
}

/*
========================
Sys_InterlockedCompareExchange
========================
*/
interlockedInt_t Sys_InterlockedCompareExchange( interlockedInt_t& value, interlockedInt_t comparand, interlockedInt_t exchange )
{
	//return InterlockedCompareExchange( & value, exchange, comparand );
	return __sync_val_compare_and_swap( &value, comparand, exchange );
}

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
void* Sys_InterlockedExchangePointer( void*& ptr, void* exchange )
{
	//return InterlockedExchangePointer( & ptr, exchange );
	return __sync_val_compare_and_swap( &ptr, ptr, exchange );
}

/*
========================
Sys_InterlockedCompareExchangePointer
========================
*/
void* Sys_InterlockedCompareExchangePointer( void*& ptr, void* comparand, void* exchange )
{
	//return InterlockedCompareExchangePointer( & ptr, exchange, comparand );
	return __sync_val_compare_and_swap( &ptr, comparand, exchange );
}