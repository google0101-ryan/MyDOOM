#pragma once

#include "Material.h"
#include "RenderCommon.h"

#include <vector>

struct guiModelSurface_t
{
    const idMaterial* material;
    uint64_t glState;
    int firstIndex;
    int numIndices;
};

class idRenderMatrix;

class idGuiModel
{
public:
    idGuiModel();

    void Clear();
private:
    void AdvanceSurf();

    guiModelSurface_t* m_surf;
    float m_shaderParms[MAX_ENTITY_SHADER_PARMS];

    static const int MAX_INDEXES = (20000 * 6);
    static const int MAX_VERTS = ( 20000 * 4 );

    vertCacheHandle_t m_vertexBlock;
	vertCacheHandle_t			m_indexBlock;
	idDrawVert *				m_vertexPointer;

	int							m_numVerts;
	int							m_numIndexes;

    std::vector<guiModelSurface_t> m_surfaces;
};