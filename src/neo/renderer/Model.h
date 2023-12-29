#pragma once

#include <idlib/precompiled.h>
#include <idlib/geometry/DrawVert.h>
#include "RenderCommon.h"

class idMaterial;

#define MD5_VERSION_STRING "MD5Version"
#define MD5_MESH_EXT "md5mesh"

struct dominantTri_t
{
	triIndex_t v2, v3;
	float normalizationScale[3];
};

struct srfTriangles_t
{
	srfTriangles_t() {}

	bool generateNormals;
	bool tangentsCalculated;
	bool perfectHull;
	bool referencedVerts;
	bool referencedIndexes;

	int numVerts;
	idDrawVert* verts;

	int numIndexes;
	triIndex_t* indexes;

	triIndex_t* silIndexes;

	int numMirroredVerts;
	int* mirroredVerts;

	int numDupVerts;
	int* dupVerts;

	int numSilEdges;

	dominantTri_t* dominantTris;

	srfTriangles_t* ambientSurface;

	srfTriangles_t* nextDeferredFree;

	vertCacheHandle_t indexCache;
	vertCacheHandle_t ambientCache;
	vertCacheHandle_t shadowCache;
};

struct modelSurface_t
{
	int id;
	const idMaterial* shader;
	srfTriangles_t* geometry;
};

const int SHADOW_CAP_INFINITE = 64;

class idRenderModel
{
public:
	virtual ~idRenderModel() {}

	virtual std::string& GetName() = 0;
};