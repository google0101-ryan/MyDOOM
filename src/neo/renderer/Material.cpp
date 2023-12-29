#include "Material.h"

#include <cstring>

idMaterial::idMaterial()
{
    CommonInit();
}

void idMaterial::CommonInit()
{
    desc = "<none>";
    renderBump = "";
    contentFlags = CONTENTS_SOLID;
    surfaceFlags = SURFTYPE_NONE;
    materialFlags = 0;
    sort = SS_BAD;
    coverage = MC_BAD;
    cullType = CT_FRONT_SIDED;
    deform = DFRM_NONE;
    numOps = 0;
    ops = NULL;
    numRegisters = 0;
    expressionRegisters = NULL;
    constantRegisters = NULL;
    numStages = 0;
    numAmbientStages = 0;
    shouldCreateBackSides = false;
    entityGui = 0;
    fogLight = false;
    blendLight = false;
    ambientLight = false;
    noFog = false;
    hasSubview = false;
    allowOverlays = true;
    unsmoothedTangents = false;
    memset(deformRegisters, 0, sizeof(deformRegisters));
    spectrum = 0;
    polygonOffset = 0;
    refCount = 0;
    deformDecl = NULL;

    decalInfo.stayTime = 10000;
    decalInfo.fadeTime = 4000;
    decalInfo.start[0] = 1;
    decalInfo.start[1] = 1;
    decalInfo.start[2] = 1;
    decalInfo.start[3] = 1;
    decalInfo.end[0] = 0;
    decalInfo.end[1] = 0;
    decalInfo.end[2] = 0;
    decalInfo.end[3] = 0;
}
