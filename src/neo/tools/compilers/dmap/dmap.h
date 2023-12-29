#include "../../../renderer/RenderSystem_local.h"

struct primitive_t
{
    primitive_t* next;
    struct bspbrush_t* brush;
    struct mapTri_t* tris;
};

struct uArea_t
{
    struct optimizeGroup_t* groups;
};

struct textureVectors_t
{
    idVec4 v[2];
};

struct side_t
{
    int planenum;
    const idMaterial* material;
    textureVectors_t texVec;
};

struct bspbrush_t
{
    bspbrush_t* next, *original;

    int entitynum, brushnum;

    const idMaterial* contentShader;

    int contents;
    bool opaque;
    int outputNumber;
};