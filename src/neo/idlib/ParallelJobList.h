#pragma once

typedef void (*jobRun_t)(void*);

enum jobListId_t
{
	JOBLIST_RENDERER_FRONTEND = 0,
	JOBLIST_RENDERER_BACKEND = 1,
	JOBLIST_UTILITY = 9,
	MAX_JOBLISTS = 32
};

enum jobListPriority_t
{
	JOBLIST_PRIORITY_NONE,
	JOBLIST_PRIORITY_LOW,
	JOBLIST_PRIORITY_MEDIUM,
	JOBLIST_PRIORITY_HIGH,
};

enum jobListParallelism_t
{
	JOBLIST_PARALLELISM_DEFAULT = -1,
	JOBLIST_PARALLELISM_MAX_CORES = -2,
	JOBLIST_PARALLELISM_MAX_THREADS = -3,
};

class idParallelJobList
{
	friend class idParallelJobManagerLocal;
public:
	jobListId_t GetId();
private:
	class idParallelJobList_Threads* jobListThreads;

	idParallelJobList(jobListId_t id, jobListPriority_t priority, unsigned int maxJobs, unsigned int maxSyncs);
	~idParallelJobList();
};

class idParallelJobManager
{
public:
    virtual void Init() = 0;

	virtual idParallelJobList* AllocJobList(jobListId_t id, jobListPriority_t priority, unsigned int maxJobs, unsigned int maxSyncs) = 0;
};

extern idParallelJobManager* parallelJobManager;