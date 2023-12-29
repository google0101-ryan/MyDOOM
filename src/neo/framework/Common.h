#pragma once

#include <stdint.h>

struct frameTiming_t
{
	frameTiming_t()
	{
		Reset();
	}

	void Reset()
	{
		startSyncTime = 0;
		finishSyncTime = 0;
		startGameTime = 0;
		finishGameTime = 0;
		finishDrawTime = 0;
		startRenderTime = 0;
		finishRenderTime = 0;

		frontendTime = 0;
		backendTime = 0;
		depthTime = 0;
		interactionTime = 0;
		shaderTime = 0;
		shadowTime = 0;
		gpuTime = 0;

		samples = 0;
		backendTotalTime = 0;
		gpuTotalTime = 0;
		backendTimeAvg = 0;
		gpuTimeAvg = 0;
	}

	void Update( frameTiming_t & other ) 
	{
		startSyncTime		= other.startSyncTime;
		finishSyncTime		= other.finishSyncTime;
		startGameTime		= other.startGameTime;
		finishGameTime		= other.finishGameTime;
		finishDrawTime		= other.finishDrawTime;
		startRenderTime		= other.startRenderTime;
		finishRenderTime	= other.finishRenderTime;
		
		frontendTime		= other.frontendTime;
		backendTime			= other.backendTime;
		depthTime			= other.depthTime;
		interactionTime		= other.interactionTime;
		shaderTime			= other.shaderTime;
		shadowTime			= other.shadowTime;
		gpuTime				= other.gpuTime;

		samples++;
		backendTotalTime	+= backendTime;
		gpuTotalTime		+= gpuTime;
		backendTimeAvg		= backendTotalTime / samples;
		gpuTimeAvg			= gpuTotalTime / samples;
	}

	uint64_t	startSyncTime;
	uint64_t	finishSyncTime;
	uint64_t	startGameTime;
	uint64_t 	finishGameTime;
	uint64_t	finishDrawTime;
	uint64_t	startRenderTime;
	uint64_t	finishRenderTime;

	uint64_t	frontendTime;
	uint64_t	backendTime;
	uint64_t	depthTime;
	uint64_t	interactionTime;
	uint64_t	shaderTime;
	uint64_t	shadowTime;
	uint64_t	gpuTime;

	uint64_t	samples;
	uint64_t	backendTotalTime;
	uint64_t	gpuTotalTime;
	double		backendTimeAvg;
	double		gpuTimeAvg;
};

class idCommon
{
public:
    virtual void Init(int argc, const char** argv, const char* cmdline) = 0;

    virtual void Error(const char* msg, ...) = 0;
    virtual void Warning(const char* msg, ...) = 0;

    virtual void StartupVariable(const char* match) = 0;
};

extern idCommon* common;