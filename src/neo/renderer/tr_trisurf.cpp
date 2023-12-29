#include "Model.h"
#include "VertexCache.h"

void R_FreeStaticTriSurfVertexCache(srfTriangles_t* tri)
{
	tri->ambientCache = 0;
	tri->indexCache = 0;
	tri->shadowCache = 0;
}

void R_FreeStaticTriSurf(srfTriangles_t* tri)
{
	if (!tri)
		return;
	
	R_FreeStaticTriSurfVertexCache(tri);

	if (!tri->referencedVerts)
	{
		if (tri->verts)
		{
			if (tri->ambientSurface == NULL || tri->verts != tri->ambientSurface->verts)
				delete tri->verts;
		}
	}

	if (!tri->referencedIndexes)
	{
		if (tri->indexes)
		{
			if (tri->ambientSurface == NULL || tri->indexes != tri->ambientSurface->indexes)
				delete tri->indexes;
		}
		if (tri->silIndexes)
			delete tri->silIndexes;
		if (tri->dominantTris)
			delete tri->dominantTris;
		if (tri->mirroredVerts)
			delete tri->mirroredVerts;
		if (tri->dupVerts)
			delete tri->dupVerts;
	}

	memset(tri, 0, sizeof(srfTriangles_t));
	delete tri;
}

srfTriangles_t* R_AllocStaticTriSurf()
{
	srfTriangles_t* tris = new srfTriangles_t;
	memset(tris, 0, sizeof(srfTriangles_t));
	return tris;
}

void R_AllocStaticTriSurfVerts(srfTriangles_t* tri, int numVerts)
{
	tri->verts = new idDrawVert[numVerts];
}

void R_AllocStaticTriSurfIndexes(srfTriangles_t* tri, int numIndexes)
{
	tri->indexes = new triIndex_t[numIndexes];
}

void R_InitDrawSurfFromTri(drawSurf_t& ds, srfTriangles_t& tri)
{
	if (tri.numIndexes == 0)
	{
		ds.numIndexes = 0;
		return;
	}

	if ((tri.verts == NULL) && !tri.referencedIndexes)
		tri.ambientCache = 0;
	else if (!vertexCache.CacheIsCurrent(tri.ambientCache))
		tri.ambientCache = vertexCache.AllocVertex(tri.verts, tri.numVerts, sizeof(idDrawVert));
	if (!vertexCache.CacheIsCurrent(tri.indexCache))
		tri.indexCache = vertexCache.AllocIndex(tri.indexes, tri.numIndexes, sizeof(triIndex_t));
	
	ds.numIndexes = tri.numIndexes;
	ds.ambientCache = tri.ambientCache;
	ds.indexCache = tri.indexCache;
	ds.shadowCache = tri.shadowCache;
	ds.jointCache = 0;
}