#pragma once

#include "RenderSystem.h"
#include "RenderCommon.h"
#include "RenderConfig.h"
#include "RenderBackend.h"
#include "GuiModel.h"
#include "Model.h"
#include <idlib/ParallelJobList.h>
#include <framework/Common.h>

class idRenderSystemLocal : public idRenderSystem
{
public:
    virtual void Init();
private:
    void InitFrameData();
    void ShutdownFrameData();

    void ToggleSmpFrame();
private:
	void SwapCommandBuffers(frameTiming_t* frameTiming);
	void SwapCommandBuffers_FinishRendering(frameTiming_t* frameTiming);
	void SwapCommandBuffers_FinishCommandBuffers();

    int frameCount;
    int viewCount;

    bool m_bInitialized = false;

    unsigned int m_smpFrame;

    idFrameData m_smpFrameData[NUM_FRAME_DATA];
    idFrameData* m_frameData;

    idRenderBackend m_backend;

    idGuiModel* m_guiModel;

	idParallelJobList* m_frontEndJobList;

	srfTriangles_t* m_unitSquareTriangles;
	srfTriangles_t* m_zeroOneCubeTriangles;
	srfTriangles_t* m_testImageTriangles;

	drawSurf_t m_unitSquareSurface;
	drawSurf_t m_zeroOneCubeSurface;
	drawSurf_t m_testImageSurface;
};

extern idRenderSystemLocal tr;