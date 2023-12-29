#include "Model_local.h"
#include "RenderSystem_local.h"

void R_FreeStaticTriSurf(srfTriangles_t* tri);
srfTriangles_t* R_AllocStaticTriSurf();
void R_AllocStaticTriSurfVerts(srfTriangles_t* tri, int numVerts);
void R_AllocStaticTriSurfIndexes(srfTriangles_t* tri, int numIndexes);

idRenderModelStatic::idRenderModelStatic()
{
	name = "<undefined>";
	bounds.Zero();
	lastModifiedFrame = 0;
	lastArchivedFrame = 0;
	overlaysAdded = 0;
	isStaticWorldModel = false;
	defaulted = false;
	purged = false;
	fastLoad = false;
	reloadable = true;
	levelLoadReferenced = false;
	numInvertedJoints = 0;
	jointsInverted = NULL;
	jointsInvertedBuffer = 0;
}

void idRenderModelStatic::InitEmpty(const char* fileName)
{
	if (strncmp(fileName, "_area", 5))
		isStaticWorldModel = true;
	else
		isStaticWorldModel = false;
	
	name = fileName;
	reloadable = false;
	PurgeModel();
	purged = false;
	bounds.Zero();
}

void idRenderModelStatic::PurgeModel()
{
	for (int i = 0; i < surfaces.size(); i++)
	{
		modelSurface_t* surf = &surfaces[i];
		if (surf->geometry)
			R_FreeStaticTriSurf(surf->geometry);
	}
	surfaces.clear();

	if (jointsInverted != NULL)
	{
		delete jointsInverted;
		jointsInverted = NULL;
	}

	purged = true;
}

static void AddCubeFace(srfTriangles_t* tri, idVec3 v1, idVec3 v2, idVec3 v3, idVec3 v4)
{
	tri->verts[tri->numVerts+0].Clear();
	tri->verts[tri->numVerts+0].xyz = v1 * 8;
	tri->verts[tri->numVerts+0].SetTexCoord(0, 0);

	tri->verts[tri->numVerts+1].Clear();
	tri->verts[tri->numVerts+1].xyz = v2 * 8;
	tri->verts[tri->numVerts+1].SetTexCoord(1, 0);

	tri->verts[tri->numVerts+2].Clear();
	tri->verts[tri->numVerts+2].xyz = v3 * 8;
	tri->verts[tri->numVerts+2].SetTexCoord(1, 1);

	tri->verts[tri->numVerts+3].Clear();
	tri->verts[tri->numVerts+3].xyz = v4 * 8;
	tri->verts[tri->numVerts+3].SetTexCoord(0, 1);

	tri->indexes[tri->numIndexes+0] = tri->numVerts+0;
	tri->indexes[tri->numIndexes+1] = tri->numVerts+1;
	tri->indexes[tri->numIndexes+2] = tri->numVerts+2;
	tri->indexes[tri->numIndexes+3] = tri->numVerts+0;
	tri->indexes[tri->numIndexes+4] = tri->numVerts+2;
	tri->indexes[tri->numIndexes+5] = tri->numVerts+3;

	tri->numVerts += 4;
	tri->numIndexes += 6;
}

void idRenderModelStatic::MakeDefaultModel()
{
	defaulted = true;

	PurgeModel();

	modelSurface_t surf;

	srfTriangles_t* tri = R_AllocStaticTriSurf();

	surf.geometry = tri;

	R_AllocStaticTriSurfVerts(tri, 24);
	R_AllocStaticTriSurfIndexes(tri, 36);

	AddCubeFace( tri, idVec3(-1, 1, 1), idVec3(1, 1, 1), idVec3(1, -1, 1), idVec3(-1, -1, 1) );
	AddCubeFace( tri, idVec3(-1, 1, -1), idVec3(-1, -1, -1), idVec3(1, -1, -1), idVec3(1, 1, -1) );

	AddCubeFace( tri, idVec3(1, -1, 1), idVec3(1, 1, 1), idVec3(1, 1, -1), idVec3(1, -1, -1) );
	AddCubeFace( tri, idVec3(-1, -1, 1), idVec3(-1, -1, -1), idVec3(-1, 1, -1), idVec3(-1, 1, 1) );

	AddCubeFace( tri, idVec3(-1, -1, 1), idVec3(1, -1, 1), idVec3(1, -1, -1), idVec3(-1, -1, -1) );
	AddCubeFace( tri, idVec3(-1, 1, 1), idVec3(-1, 1, -1), idVec3(1, 1, -1), idVec3(1, 1, 1) );

	tri->generateNormals = true;

	AddSurface(surf);
}

void idRenderModelStatic::AddSurface(modelSurface_t surf)
{
	surfaces.push_back(surf);
}

void idRenderModelStatic::FinishSurfaces()
{
}

void idRenderModelStatic::SetLevelLoadReferenced(bool referenced)
{
	levelLoadReferenced = referenced;
}
