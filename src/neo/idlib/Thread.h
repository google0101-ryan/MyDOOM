#pragma once

#include <string>
#include <cstddef>
#include <cstdint>

#include "sys/sys_public.h"
#include "sys/sys_threading.h"

class idSysMutex
{
public:
    idSysMutex()
    {
        Sys_MutexCreate(handle);
    }

    ~idSysMutex()
    {
        Sys_MutexDestroy(handle);
    }

    bool Lock(bool blocking = true)
    {
        return Sys_MutexLock(handle, blocking);
    }

    void Unlock()
    {
        Sys_MutexUnlock(handle);
    }
private:
    mutexHandle_t handle;
};

class idSysSignal
{
public:
    static const int WAIT_INFINITE = -1;

    idSysSignal(bool manualReset = false)
    {
        Sys_SignalCreate(handle, manualReset);
    }

    ~idSysSignal()
    {
        Sys_SignalDestroy(handle);
    }

    void Raise()
    {
        Sys_SignalRaise(handle);
    }

    void Clear()
    {
        Sys_SignalClear(handle);
    }

    bool Wait(int timeout = WAIT_INFINITE)
    {
        return Sys_SignalWait(handle, timeout);
    }

private:
    signalHandle_t handle;

    idSysSignal(const idSysSignal&) {}
    void operator=(const idSysSignal&) {}
};

class idSysInterlockedInteger
{
public:
    idSysInterlockedInteger() : value( 0 ) {}

	// atomically increments the integer and returns the new value
	int					Increment() { return Sys_InterlockedIncrement( value ); }

	// atomically decrements the integer and returns the new value
	int					Decrement() { return Sys_InterlockedDecrement( value ); }

	// atomically adds a value to the integer and returns the new value
	int					Add( int v ) { return Sys_InterlockedAdd( value, (interlockedInt_t) v ); }

	// atomically subtracts a value from the integer and returns the new value
	int					Sub( int v ) { return Sys_InterlockedSub( value, (interlockedInt_t) v ); }

	// returns the current value of the integer
	int					GetValue() const { return value; }

	// sets a new value, Note: this operation is not atomic
	void				SetValue( int v ) { value = (interlockedInt_t)v; }

private:
	interlockedInt_t	value;
};

class idSysThread
{
public:
    idSysThread();
    virtual ~idSysThread();

    const char* GetName() {return name.c_str();}

    void StopThread(bool wait = true);
    bool StartThread(const char* name_, core_t core, xthreadPriority priority = THREAD_NORMAL, int stackSize = DEFAULT_THREAD_STACK_SIZE);

    void WaitForThread();

    bool StartWorkerThread(const char* name_, core_t core, xthreadPriority priority = THREAD_NORMAL, int stackSize = DEFAULT_THREAD_STACK_SIZE);
protected:
    virtual int Run();
private:
    std::string name;
    uintptr_t threadHandle;
    bool isWorker;
    bool isRunning;
    volatile bool isTerminating;
    volatile bool moreWorkToDo;
    idSysSignal signalWorkerDone;
    idSysSignal signalMoreWorkToDo;
    idSysMutex signalMutex;

    static int		ThreadProc( idSysThread* thread );

    void operator=(const idSysThread&) {}
};

class idScopedCriticalSection
{
public:
	idScopedCriticalSection(idSysMutex& m) : mutex(&m) {mutex->Lock();}
	~idScopedCriticalSection() {mutex->Unlock();}
private:
	idSysMutex* mutex;
};