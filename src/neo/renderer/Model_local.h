#pragma once

#include "Model.h"
#include <vector>
#include <idlib/geometry/JointTransform.h>
#include <idlib/bv/Bounds.h>

class idRenderModelStatic : public idRenderModel
{
public:
	idRenderModelStatic();

	virtual void InitEmpty(const char* fileName);
	virtual void PurgeModel();
	void MakeDefaultModel();
	virtual void AddSurface(modelSurface_t surf);
	virtual void FinishSurfaces();
	virtual void SetLevelLoadReferenced(bool referenced);
	virtual std::string& GetName() {return name;}
public:
	std::vector<modelSurface_t> surfaces;
	idBounds bounds;
	int overlaysAdded;

	int numInvertedJoints;
	idJointMat* jointsInverted;
	vertCacheHandle_t jointsInvertedBuffer;
protected:
	int lastModifiedFrame;
	int lastArchivedFrame;

	std::string name;
	bool isStaticWorldModel;
	bool defaulted;
	bool purged;
	bool fastLoad;
	bool reloadable;
	bool levelLoadReferenced;
};