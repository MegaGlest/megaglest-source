#ifndef _SHADER_G3DVIEWER_RENDERER_H_
#define _SHADER_G3DVIEWER_RENDERER_H_

/*
#include "model_renderer.h"
#include "texture_manager.h"
#include "model.h"
#include "texture.h"
*/

#include "model_renderer.h"
#include "texture_manager.h"
#include "model.h"
#include "texture.h"

#include "particle_renderer.h"
#include "model_manager.h"
#include "graphics_interface.h"

//#include "model_manager.h"
//#include "graphics_factory_gl.h"

using Shared::Graphics::ModelRenderer;
using Shared::Graphics::TextureManager;
using Shared::Graphics::ModelManager;
using Shared::Graphics::Model;
using Shared::Graphics::Texture2D;
using Shared::Graphics::ParticleRenderer;
using Shared::Graphics::ParticleManager;
using Shared::Graphics::ParticleSystem;
//#include "model_renderer.h"

using Shared::Graphics::MeshCallback;
using Shared::Graphics::Mesh;
using Shared::Graphics::Texture;

using namespace Shared::Graphics;

namespace Shared{ namespace G3dViewer{

// ===============================================
// 	class MeshCallbackTeamColor
// ===============================================

class MeshCallbackTeamColor: public MeshCallback{
private:
	const Texture *teamTexture;

public:
	void setTeamTexture(const Texture *teamTexture)	{this->teamTexture= teamTexture;}
	virtual void execute(const Mesh *mesh);
};

// ===============================
// 	class Renderer
// ===============================

class Renderer : public RendererInterface {
public:
	static const int windowX= 100;
	static const int windowY= 100;
	static const int windowW= 640;
	static const int windowH= 480;

public:
	enum PlayerColor{
		pcRed,
		pcBlue,
		pcGreen,
		pcYellow,
		pcWhite,
		pcCyan,
		pcOrange,
		pcMagenta
	};

private:
	bool wireframe;
	bool normals;
	bool grid;

	int width;
	int height;

	ModelRenderer *modelRenderer;
	TextureManager *textureManager;
	ParticleRenderer *particleRenderer;

	ParticleManager *particleManager;
	ModelManager *modelManager;

	Texture2D *customTextureRed;
	Texture2D *customTextureBlue;
	Texture2D *customTextureGreen;
	Texture2D *customTextureYellow;
	Texture2D *customTextureWhite;
	Texture2D *customTextureCyan;
	Texture2D *customTextureOrange;
	Texture2D *customTextureMagenta;
	MeshCallbackTeamColor meshCallbackTeamColor;

	float red;
	float green;
	float blue;
	float alpha;

	Renderer();
	void checkGlCaps();
	void checkExtension(const string &extension, const string &msg);

public:
	~Renderer();
	static Renderer *getInstance();

	void init();
	void reset(int w, int h, PlayerColor playerColor);
	void transform(float rotX, float rotY, float zoom);
	void renderGrid();

	bool getNormals() const		{return normals;}
	bool getWireframe() const	{return wireframe;}
	bool getGrid() const		{return grid;}

	void toggleNormals();
	void toggleWireframe();
	void toggleGrid();

	void loadTheModel(Model *model, string file);
	void renderTheModel(Model *model, float f);

	void manageParticleSystem(ParticleSystem *particleSystem);
	void updateParticleManager();
	void renderParticleManager();
	Texture2D *getPlayerColorTexture(PlayerColor playerColor);

	Texture2D * getNewTexture2D();
	Model * getNewModel();

	Model *newModel(ResourceScope rs) { return getNewModel(); }
	Texture2D *newTexture2D(ResourceScope rs) { return getNewTexture2D(); }

	void initTextureManager();
	void initModelManager();

	void end();

	void setBackgroundColor(float red, float green, float blue);
	void setAlphaColor(float alpha);
	void saveScreen(const string &path);
};

}}//end namespace

#endif
