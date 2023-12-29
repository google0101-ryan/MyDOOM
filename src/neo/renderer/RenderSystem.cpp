#include "../framework/Common.h"
#include "RenderSystem_local.h"
#include "Image.h"
#include "ModelManager.h"
#include "VertexCache.h"
#include "RenderBackend.h"
#include "Model.h"
#include <idlib/ParallelJobList.h>

#include <stdio.h>

static const unsigned int FRAME_ALLOC_ALIGNMENT = 128;
static const unsigned int MAX_FRAME_MEMORY = 64 * 1024 * 1024;

idRenderSystemLocal	tr;
idRenderSystem * renderSystem = &tr;

static srfTriangles_t* R_MakeFullScreenTris()
{
	srfTriangles_t* tri = new srfTriangles_t;
	memset(tri, 0, sizeof(srfTriangles_t));

	tri->numIndexes = 6;
	tri->numVerts = 4;

	tri->indexes = new triIndex_t[tri->numIndexes];
	tri->verts = new idDrawVert[tri->numVerts];

	idDrawVert* verts = tri->verts;

	triIndex_t tempIndexes[6] = {3, 0, 2, 2, 0, 1};
	memcpy(tri->indexes, tempIndexes, tri->numIndexes * sizeof(tri->indexes[0]));

	verts[0].xyz[0] = -1.0f;
	verts[0].xyz[1] = 1.0f;
	verts[0].SetTexCoord(0.0f, 1.0f);

	verts[1].xyz[0] = 1.0f;
	verts[1].xyz[1] = 1.0f;
	verts[1].SetTexCoord(1.0f, 1.0f);

	verts[2].xyz[0] = 1.0f;
	verts[2].xyz[1] = -1.0f;
	verts[2].SetTexCoord(1.0f, 0.0f);

	verts[3].xyz[0] = -1.0f;
	verts[3].xyz[1] = -1.0f;
	verts[3].SetTexCoord(0.0f, 0.0f);

	for (int i = 0; i < 4; i++)
		verts[i].SetColor(0xffffffff);
	
	return tri;
}

srfTriangles_t* R_MakeZeroOneCubeTris()
{
	srfTriangles_t* tri = new srfTriangles_t;

	tri->numVerts = 8;
	tri->numIndexes = 36;

	tri->indexes = new triIndex_t[tri->numIndexes];
	tri->verts = new idDrawVert[tri->numVerts];

	idDrawVert * verts = tri->verts;

	const float low = 0.0f;
	const float high = 1.0f;

	idVec3 center( 0.0f, 0.0f, 0.0f );
	idVec3 mx(  low, 0.0f, 0.0f );
	idVec3 px( high, 0.0f, 0.0f );
	idVec3 my( 0.0f,  low, 0.0f );
	idVec3 py( 0.0f, high, 0.0f );
	idVec3 mz( 0.0f, 0.0f,  low );
	idVec3 pz( 0.0f, 0.0f, high );

	verts[0].xyz = center + mx + my + mz;
	verts[1].xyz = center + px + my + mz;
	verts[2].xyz = center + px + py + mz;
	verts[3].xyz = center + mx + py + mz;
	verts[4].xyz = center + mx + my + pz;
	verts[5].xyz = center + px + my + pz;
	verts[6].xyz = center + px + py + pz;
	verts[7].xyz = center + mx + py + pz;

	// bottom
	tri->indexes[ 0*3+0] = 2;
	tri->indexes[ 0*3+1] = 3;
	tri->indexes[ 0*3+2] = 0;
	tri->indexes[ 1*3+0] = 1;
	tri->indexes[ 1*3+1] = 2;
	tri->indexes[ 1*3+2] = 0;
	// back
	tri->indexes[ 2*3+0] = 5;
	tri->indexes[ 2*3+1] = 1;
	tri->indexes[ 2*3+2] = 0;
	tri->indexes[ 3*3+0] = 4;
	tri->indexes[ 3*3+1] = 5;
	tri->indexes[ 3*3+2] = 0;
	// left
	tri->indexes[ 4*3+0] = 7;
	tri->indexes[ 4*3+1] = 4;
	tri->indexes[ 4*3+2] = 0;
	tri->indexes[ 5*3+0] = 3;
	tri->indexes[ 5*3+1] = 7;
	tri->indexes[ 5*3+2] = 0;
	// right
	tri->indexes[ 6*3+0] = 1;
	tri->indexes[ 6*3+1] = 5;
	tri->indexes[ 6*3+2] = 6;
	tri->indexes[ 7*3+0] = 2;
	tri->indexes[ 7*3+1] = 1;
	tri->indexes[ 7*3+2] = 6;
	// front
	tri->indexes[ 8*3+0] = 3;
	tri->indexes[ 8*3+1] = 2;
	tri->indexes[ 8*3+2] = 6;
	tri->indexes[ 9*3+0] = 7;
	tri->indexes[ 9*3+1] = 3;
	tri->indexes[ 9*3+2] = 6;
	// top
	tri->indexes[10*3+0] = 4;
	tri->indexes[10*3+1] = 7;
	tri->indexes[10*3+2] = 6;
	tri->indexes[11*3+0] = 5;
	tri->indexes[11*3+1] = 4;
	tri->indexes[11*3+2] = 6;

	for ( int i = 0 ; i < 4 ; i++ ) {
		verts[i].SetColor( 0xffffffff );
	}

	return tri;
}

srfTriangles_t* R_MakeTestImageTriangles()
{
	srfTriangles_t* tri = new srfTriangles_t;

	tri->numIndexes = 6;
	tri->numVerts = 4;

	tri->indexes = new triIndex_t[tri->numIndexes];
	tri->verts = new idDrawVert[tri->numVerts];

	triIndex_t tempIndexes[6] = {3, 0, 2, 2, 0, 1};
	memcpy(tri->indexes, tempIndexes, tri->numIndexes * sizeof(tri->indexes[0]));

	idDrawVert* tempVerts = tri->verts;
	tempVerts[0].xyz[0] = 0.0f;
	tempVerts[0].xyz[1] = 0.0f;
	tempVerts[0].xyz[2] = 0;
	tempVerts[0].SetTexCoord( 0.0, 0.0f );

	tempVerts[1].xyz[0] = 1.0f;
	tempVerts[1].xyz[1] = 0.0f;
	tempVerts[1].xyz[2] = 0;
	tempVerts[1].SetTexCoord( 1.0f, 0.0f );

	tempVerts[2].xyz[0] = 1.0f;
	tempVerts[2].xyz[1] = 1.0f;
	tempVerts[2].xyz[2] = 0;
	tempVerts[2].SetTexCoord( 1.0f, 1.0f );

	tempVerts[3].xyz[0] = 0.0f;
	tempVerts[3].xyz[1] = 1.0f;
	tempVerts[3].xyz[2] = 0;
	tempVerts[3].SetTexCoord( 0.0f, 1.0f );

	for ( int i = 0; i < 4; i++ ) {
		tempVerts[i].SetColor( 0xFFFFFFFF );
	}
	return tri;
}

void idRenderSystemLocal::Init()
{
    if (m_bInitialized)
    {
        common->Warning("RenderSystem already initialized");
        return;
    }

    InitFrameData();

    printf("------- Initializing renderSystem --------\n");

    InitFrameData();

    m_backend.Init();

    viewCount = 0;

    m_guiModel = new idGuiModel;
    m_guiModel->Clear();

    globalImages->Init();

	renderModelManager->Init();

	if (!m_unitSquareTriangles)
		m_unitSquareTriangles = R_MakeFullScreenTris();
	if (!m_zeroOneCubeTriangles)
		m_zeroOneCubeTriangles = R_MakeZeroOneCubeTris();
	if (!m_testImageTriangles)
		m_testImageTriangles = R_MakeTestImageTriangles();
	
	m_frontEndJobList = parallelJobManager->AllocJobList(JOBLIST_RENDERER_FRONTEND, JOBLIST_PRIORITY_MEDIUM, 2048, 0);	

	m_bInitialized = true;

	SwapCommandBuffers(NULL);

    printf("renderSystem initialized.\n");
    printf("--------------------------------------\n");
}

void idRenderSystemLocal::InitFrameData()
{
    ShutdownFrameData();

    for (int i = 0; i < NUM_FRAME_DATA; i++)
        m_smpFrameData[i].frameMemory = new unsigned char[MAX_FRAME_MEMORY];
    
    m_frameData = &m_smpFrameData[0];

    ToggleSmpFrame();
}

void idRenderSystemLocal::ShutdownFrameData()
{
    m_frameData = NULL;
    for (int i = 0; i < NUM_FRAME_DATA; i++)
    {
        delete m_smpFrameData[i].frameMemory;
        m_smpFrameData[i].frameMemory = NULL;
    }
}

void idRenderSystemLocal::ToggleSmpFrame()
{
    if (m_frameData->frameMemoryAllocated.GetValue() > m_frameData->highWaterAllocated)
    {
        m_frameData->highWaterAllocated = m_frameData->frameMemoryAllocated.GetValue();
    }

    m_smpFrame++;
    m_frameData = &m_smpFrameData[m_smpFrame % NUM_FRAME_DATA];

    const unsigned int bytesNeededForAlignment = FRAME_ALLOC_ALIGNMENT - ((unsigned long)m_frameData->frameMemory & (FRAME_ALLOC_ALIGNMENT - 1));
    m_frameData->frameMemoryAllocated.SetValue(bytesNeededForAlignment);
    m_frameData->frameMemoryUsed.SetValue(0);

    m_frameData->renderCommandIndex = 0;
    m_frameData->renderCommands.fill({});
}

void idRenderSystemLocal::SwapCommandBuffers(frameTiming_t *frameTiming)
{
	SwapCommandBuffers_FinishRendering(frameTiming);
	SwapCommandBuffers_FinishCommandBuffers();
}

void idRenderSystemLocal::SwapCommandBuffers_FinishRendering(frameTiming_t *frameTiming)
{
	if (!m_bInitialized)
		return;
	
	m_backend.BlockingSwapBuffers();
}

void R_InitDrawSurfFromTri(drawSurf_t& ds, srfTriangles_t& tri);
void idRenderSystemLocal::SwapCommandBuffers_FinishCommandBuffers()
{
	if (!m_bInitialized)
		return;
	
	vertexCache.BeginBackend();

	ToggleSmpFrame();

	R_InitDrawSurfFromTri(m_unitSquareSurface, *m_unitSquareTriangles);
	R_InitDrawSurfFromTri(m_zeroOneCubeSurface, *m_zeroOneCubeTriangles);
	R_InitDrawSurfFromTri(m_testImageSurface, *m_testImageTriangles);

	frameCount++;
}
