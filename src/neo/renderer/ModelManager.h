#pragma once

class idRenderModelManager
{
public:
	virtual ~idRenderModelManager() {}

	virtual void Init() = 0;
};

extern idRenderModelManager* renderModelManager;