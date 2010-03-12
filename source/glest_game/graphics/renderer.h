// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
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

namespace Glest{ namespace Game{

using Shared::Graphics::Texture2D;
using Shared::Graphics::Texture3D;
using Shared::Graphics::ModelRenderer;
using Shared::Graphics::TextRenderer2D;
using Shared::Graphics::ParticleRenderer;
using Shared::Graphics::ParticleManager;
using Shared::Graphics::ModelManager;
using Shared::Graphics::TextureManager;
using Shared::Graphics::FontManager;
using Shared::Graphics::Font2D;
using Shared::Graphics::Matrix4f;
using Shared::Graphics::Vec2i;
using Shared::Graphics::Quad2i;
using Shared::Graphics::Vec3f;
using Shared::Graphics::Model;
using Shared::Graphics::ParticleSystem;
using Shared::Graphics::Pixmap2D;
using Shared::Graphics::Camera;

//non shared classes
class Config;
class Game;
class MainMenu;
class Console;
class MenuBackground;
class ChatManager;

enum ResourceScope{
	rsGlobal,
	rsMenu,
	rsGame,

	rsCount
};

// ===========================================================
// 	class Renderer
//
///	OpenGL renderer, uses the shared library
// ===========================================================

class Renderer{
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
	enum Shadows{
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

	//game
	const Game *game;

	//misc
	int triangleCount;
	int pointCount;
	Quad2i visibleQuad;
	Vec4f nearestLightPos;

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

private:
	Renderer();
	~Renderer();

public:
	static Renderer &getInstance();

    //init
	void init();
	void initGame(Game *game);
	void initMenu(MainMenu *mm);
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
	Model *newModel(ResourceScope rs);
	Texture2D *newTexture2D(ResourceScope rs);
	Texture3D *newTexture3D(ResourceScope rs);
	Font2D *newFont(ResourceScope rs);
	TextRenderer2D *getTextRenderer() const	{return textRenderer;}
	void manageParticleSystem(ParticleSystem *particleSystem, ResourceScope rs);
	void updateParticleManager(ResourceScope rs);
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
	void renderConsole(const Console *console);
	void renderChatManager(const ChatManager *chatManager);
	void renderResourceStatus();
	void renderSelectionQuad();
	void renderText(const string &text, const Font2D *font, float alpha, int x, int y, bool centered= false);
	void renderText(const string &text, const Font2D *font, const Vec3f &color, int x, int y, bool centered= false);
	void renderTextShadow(const string &text, const Font2D *font, int x, int y, bool centered= false);

    //components
	void renderLabel(const GraphicLabel *label);
    void renderButton(const GraphicButton *button);
    void renderListBox(const GraphicListBox *listBox);
	void renderMessageBox(const GraphicMessageBox *listBox);

    //complex rendering
    void renderSurface();
	void renderObjects();
	void renderWater();
    void renderUnits();
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
	void renderShadowsToTexture();

	//misc
	void loadConfig();
	void saveScreen(const string &path);
	Quad2i getVisibleQuad() const		{return visibleQuad;}

	//static
	static Shadows strToShadows(const string &s);
	static string shadowsToStr(Shadows shadows);

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
	void init3dListMenu(MainMenu *mm);

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
