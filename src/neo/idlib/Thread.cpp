#include "Thread.h"
#include "idlib/precompiled.h"

idSysThread::idSysThread()
: threadHandle(0),
isWorker(false),
isRunning(false),
isTerminating(false),
moreWorkToDo(false),
signalWorkerDone(true)
{
}

idSysThread::~idSysThread()
{
    StopThread(true);
    if (threadHandle)
    {
        Sys_DestroyThread(threadHandle);
    }
}

void idSysThread::StopThread(bool wait)
{
    if (!isRunning)
        return;
    if (isWorker)
    {
        signalMutex.Lock();
        moreWorkToDo = true;
        signalWorkerDone.Clear();
        isTerminating = true;
        signalMoreWorkToDo.Raise();
        signalMutex.Unlock();
    }
    else
    {
        isTerminating = true;
    }
    if (wait)
    {
        WaitForThread();
    }
}

bool idSysThread::StartThread(const char *name_, core_t core, xthreadPriority priority, int stackSize)
{
    if (isRunning)
        return false;
    
    name = name_;

    isTerminating = false;

    if (threadHandle)
    {
        Sys_DestroyThread(threadHandle);
    }

    threadHandle = Sys_CreateThread((xthread_t)ThreadProc, this, priority, name_, core, stackSize, false);

    isRunning = true;
    return true;
}

void idSysThread::WaitForThread()
{
    if (isWorker)
    {
        signalWorkerDone.Wait(idSysSignal::WAIT_INFINITE);
    }
    else if (isRunning)
    {
        Sys_DestroyThread(threadHandle);
        threadHandle = 0;
    }
}

bool idSysThread::StartWorkerThread(const char *name_, core_t core, xthreadPriority priority, int stackSize)
{
    if (isRunning)
        return false;
    
    isWorker = true;
    bool result = StartThread(name_, core, priority, stackSize);

    signalWorkerDone.Wait(idSysSignal::WAIT_INFINITE);

    return result;
}

int idSysThread::Run()
{
    return 0;
}

int idSysThread::ThreadProc(idSysThread *thread)
{
    int retVal = 0;

    try
    {
        if (thread->isWorker)
        {
            for (;;)
            {
                thread->signalMutex.Lock();
                if (thread->moreWorkToDo)
                {
                    thread->moreWorkToDo = false;
                    thread->signalMoreWorkToDo.Clear();
                    thread->signalMutex.Unlock();
                }
                else
                {
                    thread->signalWorkerDone.Raise();
					thread->signalMutex.Unlock();
					thread->signalMoreWorkToDo.Wait( idSysSignal::WAIT_INFINITE );
					continue;
                }

                if (thread->isTerminating)
                    break;
                
                retVal = thread->Run();
            }
            thread->signalWorkerDone.Raise();
        }
        else
        {
            retVal = thread->Run();
        }
    }
    catch (std::exception& ex)
    {
        idLib::common->Warning("Fatal error in thread %s: %s", thread->GetName(), ex.what());
        exit(0);
    }

    thread->isRunning = false;
    return retVal;
}
