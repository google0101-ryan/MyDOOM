#include "ParallelJobList.h"
#include "framework/CvarSystem.h"
#include "idlib/precompiled.h"
#include <vector>

#define MAX_JOB_THREADS 32
#define NUM_JOB_THREADS "2"
#define JOB_THREAD_CORES {	CORE_ANY, CORE_ANY, CORE_ANY, CORE_ANY,	\
								CORE_ANY, CORE_ANY, CORE_ANY, CORE_ANY,	\
								CORE_ANY, CORE_ANY, CORE_ANY, CORE_ANY,	\
								CORE_ANY, CORE_ANY, CORE_ANY, CORE_ANY,	\
								CORE_ANY, CORE_ANY, CORE_ANY, CORE_ANY,	\
								CORE_ANY, CORE_ANY, CORE_ANY, CORE_ANY,	\
								CORE_ANY, CORE_ANY, CORE_ANY, CORE_ANY,	\
								CORE_ANY, CORE_ANY, CORE_ANY, CORE_ANY }

idCvar jobs_numThreads("jobs_numThreads", NUM_JOB_THREADS, CVAR_INTEGER | CVAR_NOCHEAT, "number of threads used to crunch through jobs");

const int JOB_THREAD_STACK_SIZE = 256*1024;

const static int MAX_THREADS = 32;

static idCvar jobs_prioritize( "jobs_prioritize", "1", CVAR_BOOL | CVAR_NOCHEAT, "prioritize job lists" );

struct threadJobListState_t
{
								threadJobListState_t() :
									jobList( NULL ),
									version( 0xFFFFFFFF ),
									signalIndex( 0 ),
									lastJobIndex( 0 ),
									nextJobIndex( -1 ) {}
								threadJobListState_t( int _version ) :
									jobList( NULL ),
									version( _version ),
									signalIndex( 0 ),
									lastJobIndex( 0 ),
									nextJobIndex( -1 ) {}

	idParallelJobList_Threads* jobList;
	int version;
	int signalIndex;
	int lastJobIndex;
	int nextJobIndex;
};

struct threadStats_t
{
	unsigned int numExecutedJobs;
	unsigned int numExecutedSyncs;
	uint64_t submitTime;
	uint64_t startTime;
	uint64_t endTime;
	uint64_t waitTime;
	uint64_t threadExecTime[MAX_THREADS];
	uint64_t threadTotalTime[MAX_THREADS];
};

class idParallelJobList_Threads
{
public:
	idParallelJobList_Threads(jobListId_t id, jobListPriority_t priority, unsigned int maxJobs, unsigned int maxSyncs);

	jobListId_t GetId() {return listId;}
private:
	static const int NUM_DONE_GUARDS = 4;
	bool threaded;
	bool done;
	bool hasSignal;
	jobListId_t listId;
	jobListPriority_t listPriority;
	unsigned int maxJobs;
	unsigned int maxSyncs;
	unsigned int numSyncs;
	int lastSignalJob;
	idSysInterlockedInteger* waitForGuard;
	idSysInterlockedInteger doneGuards[NUM_DONE_GUARDS];
	int currentDoneGuard;
	idSysInterlockedInteger version;
	struct job_t
	{
		jobRun_t function;
		void* data;
		int executed;
	};
	std::vector<job_t> jobList;
	std::vector<idSysInterlockedInteger> signalJobCount;
	idSysInterlockedInteger currentJob;
	idSysInterlockedInteger fetchLock;
	idSysInterlockedInteger numThreadsExecuting;
	
	threadStats_t deferredThreadStats;
	threadStats_t threadStats;

	static int JOB_SIGNAL;
	static int JOB_SYNCHRONIZE;
	static int JOB_LIST_DONE;
};

int idParallelJobList_Threads::JOB_SIGNAL;
int idParallelJobList_Threads::JOB_SYNCHRONIZE;
int idParallelJobList_Threads::JOB_LIST_DONE;

idParallelJobList_Threads::idParallelJobList_Threads(jobListId_t id, jobListPriority_t priority, unsigned int maxJobs, unsigned int maxSyncs)
: jobList()
{
	threaded = true;
	done = true;
	hasSignal = false;
	listId = id;
	listPriority = priority;
	numSyncs = 0;
	lastSignalJob = 0;
	waitForGuard = 0;
	currentDoneGuard = 0;

	assert(listPriority != JOBLIST_PRIORITY_NONE);

	this->maxJobs = maxJobs;
	this->maxSyncs = maxSyncs;
	jobList.reserve(maxJobs + maxSyncs * 2 + 1);
	jobList.resize(0);
	signalJobCount.reserve(maxSyncs+1);
	signalJobCount.resize(0);

	memset(&deferredThreadStats, 0, sizeof(deferredThreadStats));
	memset(&threadStats, 0, sizeof(threadStats));
}

class idJobThread : public idSysThread
{
public:
	idJobThread();
	~idJobThread();

	void Start(core_t core, unsigned int threadNum);
private:
	unsigned int firstJobList;
	unsigned int lastJobList;
	idSysMutex addJobMutex;

	unsigned int threadNum;
};

idJobThread::idJobThread() :
firstJobList(0),
lastJobList(0),
threadNum(0)
{
}

idJobThread::~idJobThread()
{
}

void idJobThread::Start(core_t core, unsigned int threadNum)
{
	this->threadNum = threadNum;

	char name[16];
	snprintf(name, 16, "JLProc_%d", threadNum);
	StartWorkerThread(name, core, THREAD_NORMAL, JOB_THREAD_STACK_SIZE);
}

class idParallelJobManagerLocal : public idParallelJobManager
{
public:
    virtual void Init();
	virtual idParallelJobList* AllocJobList(jobListId_t id, jobListPriority_t priority, unsigned int maxJobs, unsigned int maxSyncs);
private:
	idJobThread threads[MAX_JOB_THREADS];
	std::vector<idParallelJobList*> jobLists;
};

idParallelJobManagerLocal parallelJobManagerLocal;
idParallelJobManager* parallelJobManager = &parallelJobManagerLocal;

void idParallelJobManagerLocal::Init()
{
    core_t cores[] = JOB_THREAD_CORES;
    assert(sizeof(cores) / sizeof(cores[0]) >= MAX_JOB_THREADS);

    for (int i = 0; i < MAX_JOB_THREADS; i++)
    {
		threads[i].Start(cores[i], i);
    }
}

idParallelJobList *idParallelJobManagerLocal::AllocJobList(jobListId_t id, jobListPriority_t priority, unsigned int maxJobs, unsigned int maxSyncs)
{
	for (int i = 0; i < jobLists.size(); i++)
	{
		if (jobLists[i]->GetId() == id)
		{
		}
	}

	idParallelJobList* jobList = new idParallelJobList(id, priority, maxJobs, maxSyncs);
	jobLists.push_back(jobList);
	return jobList;
}

jobListId_t idParallelJobList::GetId()
{
	return jobListThreads->GetId();
}

idParallelJobList::idParallelJobList(jobListId_t id, jobListPriority_t priority, unsigned int maxJobs, unsigned int maxSyncs)
{
	this->jobListThreads = new idParallelJobList_Threads(id, priority, maxJobs, maxSyncs);
}
