// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martio Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_RENDERER_H_
#define _GLEST_GAME_RENDERER_H_

#include "vec.h"
#include "math_util.h"
#include "model.h"
#include "particle.h"
#include "pixmap.h"
#include "font.h"
#include "matrix.h"
#include "selection.h"
#include "components.h"
#include "texture.h"
#include "model_manager.h"
#include "graphics_factory_gl.h"
#include "font_manager.h"
#include "camera.h"
#include <vector>
#include "model_renderer.h"
#include "model.h"
#include "graphics_interface.h"

#ifdef DEBUG_RENDERING_ENABLED
#	define IF_DEBUG_EDITION(x) x
#	include "debug_renderer.h"
#else
#	define IF_DEBUG_EDITION(x)
#endif

#include "leak_dumper.h"

namespace Glest{ namespace Game{

using namespace Shared::Graphics;

// =====================================================
// 	class MeshCallbackTeamColor
// =====================================================

class MeshCallbackTeamColor: public MeshCallback{
private:
	const Texture *teamTexture;

public:
	void setTeamTexture(const Texture *teamTexture)	{this->teamTexture= teamTexture;}
	virtual void execute(const Mesh *mesh);

	static bool noTeamColors;
};

//non shared classes
class Config;
class Game;
class MainMenu;
class Console;
class MenuBackground;
class ChatManager;
class Object;

// ===========================================================
// 	class Renderer
//
///	OpenGL renderer, uses the shared library
// ===========================================================

class VisibleQuadContainerCache {
protected:

	void CopyAll(const VisibleQuadContainerCache &obj) {
		cacheFrame 			= obj.cacheFrame;
		cacheIsDirty		= obj.cacheIsDirty;
		visibleObjectList	= obj.visibleObjectList;
		visibleUnitList		= obj.visibleUnitList;
		visibleQuadUnitList = obj.visibleQuadUnitList;
		//inVisibleUnitList	= obj.inVisibleUnitList;
		//visibleCellList		= obj.visibleCellList;
		visibleScaledCellList = obj.visibleScaledCellList;
		lastVisibleQuad		= obj.lastVisibleQuad;
	}

public:

	VisibleQuadContainerCache() {
		cacheFrame = 0;
		cacheIsDirty = false;
		clearCacheData();
	}
	VisibleQuadContainerCache(const VisibleQuadContainerCache &obj) {
		CopyAll(obj);
	}
	VisibleQuadContainerCache & operator=(const VisibleQuadContainerCache &obj) {
		CopyAll(obj);
		return *this;
	}

	void clearCacheData() {
		clearVolatileCacheData();
		clearNonVolatileCacheData();
	}
	void clearVolatileCacheData() {
		visibleUnitList.clear();
		visibleQuadUnitList.clear();
		//inVisibleUnitList.clear();
	}
	void clearNonVolatileCacheData() {
		visibleObjectList.clear();
		visibleScaledCellList.clear();
	}

	int cacheFrame;
	Quad2i lastVisibleQuad;
	bool cacheIsDirty;
	std::vector<Object *> visibleObjectList;
	std::vector<Unit   *> visibleQuadUnitList;
	std::vector<Unit   *> visibleUnitList;
	//std::vector<Unit   *> inVisibleUnitList;
	//std::vector<Vec2i> visibleCellList;
	std::vector<Vec2i> visibleScaledCellList;
};


class Renderer : public RendererInterface {
public:
	//progress bar
	static const int maxProgressBar;
	static const Vec4f progressBarBack1;
	static const Vec4f progressBarBack2;
	static const Vec4f progressBarFront1;
	static const Vec4f progressBarFront2;

	//sun and moon
	static const float sunDist;
	static const float moonDist;
	static const float lightAmbFactor;

	//mouse
	static const int maxMouse2dAnim;

	//texture units
	static const GLenum baseTexUnit;
	static const GLenum fowTexUnit;
	static const GLenum shadowTexUnit;

	//selection
	static const float selectionCircleRadius;
	static const float magicCircleRadius;

	//perspective values
	static const float perspFov;
	static const float perspNearPlane;
	static const float perspFarPlane;

	//default values
	static const float ambFactor;
	static const Vec4f defSpecularColor;
	static const Vec4f defDiffuseColor;
	static const Vec4f defAmbientColor;
	static const Vec4f defColor;
	static const Vec4f fowColor;

	//light
	static const float maxLightDist;
	
public:
	enum Shadows {
		sDisabled,
		sProjected,
		sShadowMapping,

		sCount
	};

private:
	//config
	int maxLights;
    bool photoMode;
	int shadowTextureSize;
	int shadowFrameSkip;
	float shadowAlpha;
	bool focusArrows;
	bool textures3D;
	Shadows shadows;
	int maxConsoleLines;

	//game
	const Game *game;
	const MainMenu *menu;

	//misc
	int triangleCount;
	int pointCount;
	Quad2i visibleQuad;
	Vec4f nearestLightPos;
	VisibleQuadContainerCache quadCache;

	//renderers
	ModelRenderer *modelRenderer;
	TextRenderer2D *textRenderer;
	ParticleRenderer *particleRenderer;

	//texture managers
	ModelManager *modelManager[rsCount];
	TextureManager *textureManager[rsCount];
	FontManager *fontManager[rsCount];
	ParticleManager *particleManager[rsCount];

	//state lists
	GLuint list3d;
	GLuint list2d;
	GLuint list3dMenu;

	//shadows
	GLuint shadowMapHandle;
	Matrix4f shadowMapMatrix;
	int shadowMapFrame;

	//water
	float waterAnim;

	bool allowRenderUnitTitles;
	std::vector<std::pair<Unit *,Vec3f> > renderUnitTitleList;
	std::vector<Unit *> visibleFrameUnitList;

	bool no2DMouseRendering;
	bool showDebugUI;

	int lastRenderFps;
	bool shadowsOffDueToMinRender;

	bool useQuadCache;
private:
	Renderer();
	~Renderer();

public:
	static Renderer &getInstance();

	void reinitAll();

    //init
	void init();
	void initGame(const Game *game);
	void initMenu(const MainMenu *mm);
	void reset3d();
	void reset2d();
	void reset3dMenu();

	//end
	void end();
	void endMenu();
	void endGame();

	//get
	int getTriangleCount() const	{return triangleCount;}
	int getPointCount() const		{return pointCount;}

	//misc
	void reloadResources();

	//engine interface
	void initTexture(ResourceScope rs, Texture *texture);
	void endTexture(ResourceScope rs, Texture *texture,bool mustExistInList=false);
	void endLastTexture(ResourceScope rs, bool mustExistInList=false);

	Model *newModel(ResourceScope rs);
	void endModel(ResourceScope rs, Model *model, bool mustExistInList=false);
	void endLastModel(ResourceScope rs, bool mustExistInList=false);

	Texture2D *newTexture2D(ResourceScope rs);
	Texture3D *newTexture3D(ResourceScope rs);
	Font2D *newFont(ResourceScope rs);
	TextRenderer2D *getTextRenderer() const	{return textRenderer;}
	void manageParticleSystem(ParticleSystem *particleSystem, ResourceScope rs);
	void cleanupParticleSystems(vector<ParticleSystem *> &particleSystems,ResourceScope rs);
	void cleanupUnitParticleSystems(vector<UnitParticleSystem *> &particleSystems,ResourceScope rs);
	bool validateParticleSystemStillExists(ParticleSystem * particleSystem,ResourceScope rs) const;
	void updateParticleManager(ResourceScope rs,int renderFps=-1);
	void renderParticleManager(ResourceScope rs);
	void swapBuffers();

    //lights and camera
	void setupLighting();
	void loadGameCameraMatrix();
	void loadCameraMatrix(const Camera *camera);
	void computeVisibleQuad();

    //basic rendering
	void renderMouse2d(int mouseX, int mouseY, int anim, float fade= 0.f);
    void renderMouse3d();
    void renderBackground(const Texture2D *texture);
	void renderTextureQuad(int x, int y, int w, int h, const Texture2D *texture, float alpha=1.f);
	void renderConsole(const Console *console, const bool showAll=false, const bool showMenuConsole=false);
	void renderChatManager(const ChatManager *chatManager);
	void renderResourceStatus();
	void renderSelectionQuad();
	void renderText(const string &text, const Font2D *font, float alpha, int x, int y, bool centered= false);
	void renderText(const string &text, const Font2D *font, const Vec3f &color, int x, int y, bool centered= false);
	void renderText(const string &text, const Font2D *font, const Vec4f &color, int x, int y, bool centered=false);
	void renderTextShadow(const string &text, const Font2D *font,const Vec4f &color, int x, int y, bool centered= false);

    //components
	void renderLabel(const GraphicLabel *label);
    void renderButton(const GraphicButton *button);
    void renderListBox(const GraphicListBox *listBox);
	void renderMessageBox(const GraphicMessageBox *listBox);

    //complex rendering
    void renderSurface(const int renderFps, const int worldFrameCount);
	void renderObjects(const int renderFps, const int worldFrameCount);

	void renderWater();
    void renderUnits(const int renderFps, const int worldFrameCount);

	void renderSelectionEffects();
	void renderWaterEffects();
	void renderMinimap();
    void renderDisplay();
	void renderMenuBackground(const MenuBackground *menuBackground);

	//computing
    bool computePosition(const Vec2i &screenPos, Vec2i &worldPos);
	void computeSelected(Selection::UnitContainer &units, const Vec2i &posDown, const Vec2i &posUp);

    //gl wrap
	string getGlInfo();
	string getGlMoreInfo();
	void autoConfig();

	//clear
    void clearBuffers();
	void clearZBuffer();

	//shadows
	void renderShadowsToTexture(const int renderFps);

	//misc
	void loadConfig();
	void saveScreen(const string &path);
	Quad2i getVisibleQuad() const		{return visibleQuad;}

	//static
	static Shadows strToShadows(const string &s);
	static string shadowsToStr(Shadows shadows);

	const Game * getGame() { return game; }

	void setAllowRenderUnitTitles(bool value);
	bool getAllowRenderUnitTitles() { return allowRenderUnitTitles; }
	void renderUnitTitles(Font2D *font, Vec3f color);
	Vec3f computeScreenPosition(const Vec3f &worldPos);

	void setPhotoMode(bool value) { photoMode = value; }

	bool getNo2DMouseRendering() const { return no2DMouseRendering; }
	void setNo2DMouseRendering(bool value) { no2DMouseRendering = value; }

	bool getShowDebugUI() const { return showDebugUI; }
	void setShowDebugUI(bool value) { showDebugUI = value; }

	void setLastRenderFps(int value) { lastRenderFps = value;}
	int getLastRenderFps() const { return lastRenderFps;}

	VisibleQuadContainerCache & getQuadCache(bool updateOnDirtyFrame=true,bool forceNew=false);
	void setQuadCacheDirty(bool value);

private:
	//private misc
	float computeSunAngle(float time);
	float computeMoonAngle(float time);
	Vec4f computeSunPos(float time);
	Vec4f computeMoonPos(float time);
	Vec3f computeLightColor(float time);
	Vec4f computeWaterColor(float waterLevel, float cellHeight);
	void checkExtension(const string &extension, const string &msg);

	//selection render
	void renderObjectsFast();
	void renderUnitsFast();

	//gl requirements
	void checkGlCaps();
	void checkGlOptionalCaps();

	//gl init
	void init3dList();
    void init2dList();
	void init3dListMenu(const MainMenu *mm);

	//misc
	void loadProjectionMatrix();
	void enableProjectiveTexturing();

	//private aux drawing
	void renderSelectionCircle(Vec3f v, int size, float radius);
	void renderArrow(const Vec3f &pos1, const Vec3f &pos2, const Vec3f &color, float width);
	void renderProgressBar(int size, int x, int y, Font2D *font);
	void renderTile(const Vec2i &pos);
	void renderQuad(int x, int y, int w, int h, const Texture2D *texture);

	//static
    static Texture2D::Filter strToTextureFilter(const string &s);
};

}} //end namespace

#endif
