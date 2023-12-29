#include "ModelManager.h"
#include "Model_local.h"
#include "RenderSystem_local.h"
#include <unordered_map>

class idRenderModelManagerLocal : public idRenderModelManager
{
public:
	idRenderModelManagerLocal();
	virtual ~idRenderModelManagerLocal() {}

	virtual void Init();

	virtual void AddModel(idRenderModel* model);
private:
	idRenderModel* m_defaultModel;
	idRenderModel* m_beamModel;
	idRenderModel* m_spriteModel;
	bool m_insideLevelLoad;
	std::unordered_map<std::string, idRenderModel*> models;
};

idRenderModelManagerLocal localModelManager;
idRenderModelManager* renderModelManager = &localModelManager;

idRenderModelManagerLocal::idRenderModelManagerLocal()
{
	m_defaultModel = NULL;
	m_beamModel = NULL;
	m_spriteModel = NULL;
	m_insideLevelLoad = false;
}

void idRenderModelManagerLocal::Init()
{
	m_insideLevelLoad = false;

	idRenderModelStatic* model = new idRenderModelStatic;
	model->InitEmpty("_DEFAULT");
	model->MakeDefaultModel();
	model->SetLevelLoadReferenced(true);
	m_defaultModel = model;
	AddModel(model);

	idRenderModelStatic* beam = new idRenderModelStatic;
	beam->InitEmpty("_BEAM");
	beam->SetLevelLoadReferenced(true);
	m_beamModel = beam;
	AddModel(beam);

	idRenderModelStatic* sprite = new idRenderModelStatic;
	sprite->InitEmpty("_SPRITE");
	sprite->SetLevelLoadReferenced(true);
	m_spriteModel = sprite;
	AddModel(sprite);
}

void idRenderModelManagerLocal::AddModel(idRenderModel *model)
{
	models[model->GetName()] = model;
}
