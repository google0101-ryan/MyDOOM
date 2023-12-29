#include "GuiModel.h"
#include "RenderSystem.h"
#include "../idlib/precompiled.h"

idGuiModel::idGuiModel()
{
    m_surf = NULL;
    m_vertexBlock = 0;
    m_indexBlock = 0;
    m_vertexPointer = NULL;
    m_numVerts = 0;
    m_numIndexes = 0;

    for (int i = 0; i < MAX_ENTITY_SHADER_PARMS; i++)
        m_shaderParms[i] = 1.0f;
}

void idGuiModel::Clear()
{
    m_surfaces.clear();
    AdvanceSurf();
}

void idGuiModel::AdvanceSurf()
{
    guiModelSurface_t s;

    if (m_surfaces.size())
    {
        s.material = m_surf->material;
        s.glState = m_surf->glState;
    }
    else
    {
        s.material = NULL;
        s.glState = 0;
    }

    m_numIndexes = ALIGN(m_numIndexes, 8);

    s.numIndices = 0;
    s.firstIndex = m_numIndexes;

    m_surfaces.push_back(s);
    m_surf = &m_surfaces[m_surfaces.size() - 1];
}
