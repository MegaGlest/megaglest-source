#ifndef _SHADER_G3DVIEWER_RENDERER_H_
#define _SHADER_G3DVIEWER_RENDERER_H_

#include "model_renderer.h"
#include "texture_manager.h"
#include "model.h"
#include "texture.h"

using Shared::Graphics::ModelRenderer;
using Shared::Graphics::TextureManager;
using Shared::Graphics::Model;
using Shared::Graphics::Texture2D;

#include "model_renderer.h"

using Shared::Graphics::MeshCallback;
using Shared::Graphics::Mesh;
using Shared::Graphics::Texture;

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

class Renderer{
public:
	static const int windowX= 100;
	static const int windowY= 100;
	static const int windowW= 640;
	static const int windowH= 480;

public:
	enum PlayerColor{
		pcRed,
		pcBlue,
		pcYellow,
		pcGreen
	};

private:
	bool wireframe;
	bool normals;
	bool grid;

	ModelRenderer *modelRenderer;
	TextureManager *textureManager;
	Texture2D *customTextureRed;
	Texture2D *customTextureBlue;
	Texture2D *customTextureYellow;
	Texture2D *customTextureGreen;
	MeshCallbackTeamColor meshCallbackTeamColor;

	Renderer();

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
};

}}//end namespace

#endif
