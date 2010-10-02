//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martio Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "renderer.h"

#include "texture_gl.h"
#include "main_menu.h"
#include "config.h"
#include "components.h"
#include "time_flow.h"
#include "graphics_interface.h"
#include "object.h"
#include "core_data.h"
#include "game.h"
#include "metrics.h"
#include "opengl.h"
#include "faction.h"
#include "factory_repository.h"
#include <cstdlib>
#include <algorithm>

#include "leak_dumper.h"

using namespace Shared::Graphics;
using namespace Shared::Graphics::Gl;
using namespace Shared::Util;

namespace Glest { namespace Game{

// =====================================================
// 	class MeshCallbackTeamColor
// =====================================================

bool MeshCallbackTeamColor::noTeamColors = false;

void MeshCallbackTeamColor::execute(const Mesh *mesh){

	//team color
	if( mesh->getCustomTexture() && teamTexture != NULL &&
		MeshCallbackTeamColor::noTeamColors == false) {
		//texture 0
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

		//set color to interpolation
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE);

		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);

		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_TEXTURE1);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);

		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB, GL_SRC_ALPHA);

		//set alpha to 1
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);

		//texture 1
		glActiveTexture(GL_TEXTURE1);
		glMultiTexCoord2f(GL_TEXTURE1, 0.f, 0.f);
		glEnable(GL_TEXTURE_2D);

		glBindTexture(GL_TEXTURE_2D, static_cast<const Texture2DGl*>(teamTexture)->getHandle());
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);

		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PRIMARY_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);

		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PREVIOUS);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);

		//set alpha to 1
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PRIMARY_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);

		glActiveTexture(GL_TEXTURE0);
	}
	else{
		glActiveTexture(GL_TEXTURE1);
		glDisable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	}
}

// ===========================================================
//	class Renderer
// ===========================================================

// ===================== PUBLIC ========================

const int Renderer::maxProgressBar= 100;
const Vec4f Renderer::progressBarBack1= Vec4f(0.7f, 0.7f, 0.7f, 0.7f);
const Vec4f Renderer::progressBarBack2= Vec4f(0.7f, 0.7f, 0.7f, 1.f);
const Vec4f Renderer::progressBarFront1= Vec4f(0.f, 0.5f, 0.f, 1.f);
const Vec4f Renderer::progressBarFront2= Vec4f(0.f, 0.1f, 0.f, 1.f);

const float Renderer::sunDist= 10e6;
const float Renderer::moonDist= 10e6;
const float Renderer::lightAmbFactor= 0.4f;

const int Renderer::maxMouse2dAnim= 100;

const GLenum Renderer::baseTexUnit= GL_TEXTURE0;
const GLenum Renderer::fowTexUnit= GL_TEXTURE1;
const GLenum Renderer::shadowTexUnit= GL_TEXTURE2;

const float Renderer::selectionCircleRadius= 0.7f;
const float Renderer::magicCircleRadius= 1.f;

//perspective values
const float Renderer::perspFov= 60.f;
const float Renderer::perspNearPlane= 1.f;
//const float Renderer::perspFarPlane= 50.f;
const float Renderer::perspFarPlane= 1000.f;

const float Renderer::ambFactor= 0.7f;
const Vec4f Renderer::fowColor= Vec4f(0.0f, 0.0f, 0.0f, 1.0f);
const Vec4f Renderer::defSpecularColor= Vec4f(0.8f, 0.8f, 0.8f, 1.f);
const Vec4f Renderer::defDiffuseColor= Vec4f(1.f, 1.f, 1.f, 1.f);
const Vec4f Renderer::defAmbientColor= Vec4f(1.f * ambFactor, 1.f * ambFactor, 1.f * ambFactor, 1.f);
const Vec4f Renderer::defColor= Vec4f(1.f, 1.f, 1.f, 1.f);

//const float Renderer::maxLightDist= 100.f;
const float Renderer::maxLightDist= 1000.f;

const int MIN_FPS_NORMAL_RENDERING = 10;
const int MIN_FPS_NORMAL_RENDERING_TOP_THRESHOLD = 35;

// ==================== constructor and destructor ====================

Renderer::Renderer() {
	this->useQuadCache = true;
	this->allowRenderUnitTitles = false;
	this->menu = NULL;
	this->game = NULL;
	showDebugUI = false;
	modelRenderer = NULL;
	textRenderer = NULL;
	particleRenderer = NULL;

	lastRenderFps=MIN_FPS_NORMAL_RENDERING;
	shadowsOffDueToMinRender=false;

	pixmapScreenShot = NULL;;

	//resources
	for(int i=0; i < rsCount; ++i) {
		modelManager[i] = NULL;
		textureManager[i] = NULL;
		particleManager[i] = NULL;
		fontManager[i] = NULL;
	}

	GraphicsInterface &gi= GraphicsInterface::getInstance();
	FactoryRepository &fr= FactoryRepository::getInstance();
	Config &config= Config::getInstance();

	this->useQuadCache = config.getBool("UseQuadCache","true");
	this->no2DMouseRendering = config.getBool("No2DMouseRendering","false");
	this->maxConsoleLines= config.getInt("ConsoleMaxLines");

	gi.setFactory(fr.getGraphicsFactory(config.getString("FactoryGraphics")));
	GraphicsFactory *graphicsFactory= GraphicsInterface::getInstance().getFactory();

	modelRenderer= graphicsFactory->newModelRenderer();
	textRenderer= graphicsFactory->newTextRenderer2D();
	particleRenderer= graphicsFactory->newParticleRenderer();

	//resources
	for(int i=0; i< rsCount; ++i) {
		modelManager[i]= graphicsFactory->newModelManager();
		textureManager[i]= graphicsFactory->newTextureManager();
		modelManager[i]->setTextureManager(textureManager[i]);
		particleManager[i]= graphicsFactory->newParticleManager();
		fontManager[i]= graphicsFactory->newFontManager();
	}
}

Renderer::~Renderer(){
	delete modelRenderer;
	modelRenderer = NULL;
	delete textRenderer;
	textRenderer = NULL;
	delete particleRenderer;
	particleRenderer = NULL;

	//resources
	for(int i=0; i<rsCount; ++i){
		delete modelManager[i];
		modelManager[i] = NULL;
		delete textureManager[i];
		textureManager[i] = NULL;
		delete particleManager[i];
		particleManager[i] = NULL;
		delete fontManager[i];
		fontManager[i] = NULL;
	}

	delete pixmapScreenShot;

	this->menu = NULL;
	this->game = NULL;
}

Renderer &Renderer::getInstance(){
	static Renderer renderer;
	return renderer;
}

void Renderer::reinitAll() {
	//resources
	for(int i=0; i<rsCount; ++i){
		//modelManager[i]->init();
		textureManager[i]->init(true);
		//particleManager[i]->init();
		//fontManager[i]->init();
	}
}
// ==================== init ====================

void Renderer::init(){

	Config &config= Config::getInstance();

	loadConfig();

	if(config.getBool("CheckGlCaps")){
		checkGlCaps();
	}

	if(config.getBool("FirstTime")){
		config.setBool("FirstTime", false);
		autoConfig();
		config.save();
	}

	modelManager[rsGlobal]->init();
	textureManager[rsGlobal]->init();
	fontManager[rsGlobal]->init();

	init2dList();

	glHint(GL_FOG_HINT, GL_FASTEST);
	//glHint(GL_GENERATE_MIPMAP_HINT, GL_FASTEST);
	glHint(GL_LINE_SMOOTH_HINT, GL_FASTEST);
	//glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
	//glHint(GL_POINT_SMOOTH_HINT, GL_FASTEST);

	//glHint(GL_POLYGON_SMOOTH_HINT, GL_FASTEST);
	glHint(GL_TEXTURE_COMPRESSION_HINT, GL_FASTEST);

}

void Renderer::initGame(const Game *game){

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	this->game= game;

	//check gl caps
	checkGlOptionalCaps();

	//vars
	shadowMapFrame= 0;
	waterAnim= 0;

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//shadows
	if(shadows==sProjected || shadows==sShadowMapping){
		static_cast<ModelRendererGl*>(modelRenderer)->setSecondaryTexCoordUnit(2);

		glGenTextures(1, &shadowMapHandle);
		glBindTexture(GL_TEXTURE_2D, shadowMapHandle);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if(shadows==sShadowMapping){

			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			//shadow mapping
			glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE_ARB);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FAIL_VALUE_ARB, 1.0f-shadowAlpha);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32,
				shadowTextureSize, shadowTextureSize,
				0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
		}
		else{

			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			//projected
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8,
				shadowTextureSize, shadowTextureSize,
				0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
		}

		shadowMapFrame= -1;
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	IF_DEBUG_EDITION( getDebugRenderer().init(); )

	//texture init
	modelManager[rsGame]->init();
	textureManager[rsGame]->init();
	fontManager[rsGame]->init();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	init3dList();
}

void Renderer::initMenu(const MainMenu *mm){
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	this->menu = mm;

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	modelManager[rsMenu]->init();
	textureManager[rsMenu]->init();
	fontManager[rsMenu]->init();
	//modelRenderer->setCustomTexture(CoreData::getInstance().getCustomTexture());

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	init3dListMenu(mm);

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void Renderer::reset3d(){
	assertGl();
	glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
	glCallList(list3d);
	pointCount= 0;
	triangleCount= 0;
	assertGl();
}

void Renderer::reset2d(){
	assertGl();
	glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SINGLE_COLOR);
	glCallList(list2d);
	assertGl();
}

void Renderer::reset3dMenu(){
	assertGl();
	glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SINGLE_COLOR);
	glCallList(list3dMenu);
	assertGl();
}

// ==================== end ====================

void Renderer::end(){

	//delete resources
	modelManager[rsGlobal]->end();
	textureManager[rsGlobal]->end();
	fontManager[rsGlobal]->end();
	particleManager[rsGlobal]->end();

	//delete 2d list
	glDeleteLists(list2d, 1);
}

void Renderer::endGame(){
	game= NULL;

	//delete resources
	modelManager[rsGame]->end();
	textureManager[rsGame]->end();
	fontManager[rsGame]->end();
	particleManager[rsGame]->end();

	if(shadows==sProjected || shadows==sShadowMapping){
		glDeleteTextures(1, &shadowMapHandle);
	}

	glDeleteLists(list3d, 1);
}

void Renderer::endMenu(){
	this->menu = NULL;
	//delete resources
	modelManager[rsMenu]->end();
	textureManager[rsMenu]->end();
	fontManager[rsMenu]->end();
	particleManager[rsMenu]->end();

	glDeleteLists(list3dMenu, 1);
}

void Renderer::reloadResources(){
	for(int i=0; i<rsCount; ++i){
		modelManager[i]->end();
		textureManager[i]->end();
		fontManager[i]->end();
	}
	for(int i=0; i<rsCount; ++i){
		modelManager[i]->init();
		textureManager[i]->init();
		fontManager[i]->init();
	}
}


// ==================== engine interface ====================

void Renderer::initTexture(ResourceScope rs, Texture *texture) {
	textureManager[rs]->initTexture(texture);
}

void Renderer::endTexture(ResourceScope rs, Texture *texture, bool mustExistInList) {
	textureManager[rs]->endTexture(texture,mustExistInList);
}
void Renderer::endLastTexture(ResourceScope rs, bool mustExistInList) {
	textureManager[rs]->endLastTexture(mustExistInList);
}

Model *Renderer::newModel(ResourceScope rs){
	return modelManager[rs]->newModel();
}

void Renderer::endModel(ResourceScope rs, Model *model,bool mustExistInList) {
	modelManager[rs]->endModel(model,mustExistInList);
}
void Renderer::endLastModel(ResourceScope rs, bool mustExistInList) {
	modelManager[rs]->endLastModel(mustExistInList);
}

Texture2D *Renderer::newTexture2D(ResourceScope rs){
	return textureManager[rs]->newTexture2D();
}

Texture3D *Renderer::newTexture3D(ResourceScope rs){
	return textureManager[rs]->newTexture3D();
}

Font2D *Renderer::newFont(ResourceScope rs){
	return fontManager[rs]->newFont2D();
}

void Renderer::manageParticleSystem(ParticleSystem *particleSystem, ResourceScope rs){
	particleManager[rs]->manage(particleSystem);
}

bool Renderer::validateParticleSystemStillExists(ParticleSystem * particleSystem,ResourceScope rs) const {
	return particleManager[rs]->validateParticleSystemStillExists(particleSystem);
}

void Renderer::cleanupParticleSystems(vector<ParticleSystem *> &particleSystems, ResourceScope rs) {
	particleManager[rs]->cleanupParticleSystems(particleSystems);
}

void Renderer::cleanupUnitParticleSystems(vector<UnitParticleSystem *> &particleSystems, ResourceScope rs) {
	particleManager[rs]->cleanupUnitParticleSystems(particleSystems);
}

void Renderer::updateParticleManager(ResourceScope rs, int renderFps) {
	particleManager[rs]->update(renderFps);
}

void Renderer::renderParticleManager(ResourceScope rs){
	glPushAttrib(GL_DEPTH_BUFFER_BIT  | GL_STENCIL_BUFFER_BIT);
	glDepthFunc(GL_LESS);
	particleRenderer->renderManager(particleManager[rs], modelRenderer);
	glPopAttrib();
}

void Renderer::swapBuffers(){

	//glFlush(); // should not be required - http://www.opengl.org/wiki/Common_Mistakes
	glFlush();

	GraphicsInterface::getInstance().getCurrentContext()->swapBuffers();
}

// ==================== lighting ====================

//places all the opengl lights
void Renderer::setupLighting(){

    int lightCount= 0;
	const World *world= game->getWorld();
	const GameCamera *gameCamera= game->getGameCamera();
	const TimeFlow *timeFlow= world->getTimeFlow();
	float time= timeFlow->getTime();

	assertGl();

    //sun/moon light
	Vec3f lightColor= computeLightColor(time);
	Vec3f fogColor= world->getTileset()->getFogColor();
	Vec4f lightPos= timeFlow->isDay()? computeSunPos(time): computeMoonPos(time);
	nearestLightPos= lightPos;

	glLightfv(GL_LIGHT0, GL_POSITION, lightPos.ptr());
	glLightfv(GL_LIGHT0, GL_AMBIENT, Vec4f(lightColor*lightAmbFactor, 1.f).ptr());
	glLightfv(GL_LIGHT0, GL_DIFFUSE, Vec4f(lightColor, 1.f).ptr());
	glLightfv(GL_LIGHT0, GL_SPECULAR, Vec4f(0.0f, 0.0f, 0.f, 1.f).ptr());

	glFogfv(GL_FOG_COLOR, Vec4f(fogColor*lightColor, 1.f).ptr());

    lightCount++;

	//disable all secondary lights
	for(int i= 1; i<maxLights; ++i){
		glDisable(GL_LIGHT0 + i);
	}

    //unit lights (not projectiles)
	if(timeFlow->isTotalNight()){
        for(int i=0; i<world->getFactionCount() && lightCount<maxLights; ++i){
            for(int j=0; j<world->getFaction(i)->getUnitCount() && lightCount<maxLights; ++j){
                Unit *unit= world->getFaction(i)->getUnit(j);
				if(world->toRenderUnit(unit) &&
					unit->getCurrVector().dist(gameCamera->getPos())<maxLightDist &&
                    unit->getType()->getLight() && unit->isOperative()){

					Vec4f pos= Vec4f(unit->getCurrVector());
                    pos.y+=4.f;

					GLenum lightEnum= GL_LIGHT0 + lightCount;

					glEnable(lightEnum);
					glLightfv(lightEnum, GL_POSITION, pos.ptr());
					glLightfv(lightEnum, GL_AMBIENT, Vec4f(unit->getType()->getLightColor()).ptr());
					glLightfv(lightEnum, GL_DIFFUSE, Vec4f(unit->getType()->getLightColor()).ptr());
					glLightfv(lightEnum, GL_SPECULAR, Vec4f(unit->getType()->getLightColor()*0.3f).ptr());
					glLightf(lightEnum, GL_QUADRATIC_ATTENUATION, 0.05f);

                    ++lightCount;

					const GameCamera *gameCamera= game->getGameCamera();

					if(Vec3f(pos).dist(gameCamera->getPos())<Vec3f(nearestLightPos).dist(gameCamera->getPos())){
						nearestLightPos= pos;
					}
                }
            }
        }
    }

	assertGl();
}

void Renderer::loadGameCameraMatrix(){
	const GameCamera *gameCamera= game->getGameCamera();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glRotatef(gameCamera->getVAng(), -1, 0, 0);
	glRotatef(gameCamera->getHAng(), 0, 1, 0);
	glTranslatef(-gameCamera->getPos().x, -gameCamera->getPos().y, -gameCamera->getPos().z);
}

void Renderer::loadCameraMatrix(const Camera *camera){
	const Vec3f &position= camera->getConstPosition();
	Quaternion orientation= camera->getOrientation().conjugate();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMultMatrixf(orientation.toMatrix4().ptr());
	glTranslatef(-position.x, -position.y, -position.z);
}

void Renderer::computeVisibleQuad(){
	const GameCamera *gameCamera= game->getGameCamera();
	visibleQuad= gameCamera->computeVisibleQuad();
}

// =======================================
// basic rendering
// =======================================

void Renderer::renderMouse2d(int x, int y, int anim, float fade){
	if(no2DMouseRendering == true) {
		return;
	}
    float color1, color2;

	float fadeFactor= fade+1.f;

	anim= anim*2-maxMouse2dAnim;

    color2= (abs(anim*(int)fadeFactor)/static_cast<float>(maxMouse2dAnim))/2.f+0.4f;
    color1= (abs(anim*(int)fadeFactor)/static_cast<float>(maxMouse2dAnim))/2.f+0.8f;

    glPushAttrib(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_LINE_BIT);
        glEnable(GL_BLEND);

		//inside
		glColor4f(0.4f*fadeFactor, 0.2f*fadeFactor, 0.2f*fadeFactor, 0.5f*fadeFactor);
        glBegin(GL_TRIANGLES);
            glVertex2i(x, y);
            glVertex2i(x+20, y-10);
            glVertex2i(x+10, y-20);
        glEnd();

		//border
        glLineWidth(2);
        glBegin(GL_LINE_LOOP);
            glColor4f(1.f, 0.2f, 0, color1);
            glVertex2i(x, y);
            glColor4f(1.f, 0.4f, 0, color2);
            glVertex2i(x+20, y-10);
            glColor4f(1.f, 0.4f, 0, color2);
            glVertex2i(x+10, y-20);
        glEnd();
    glPopAttrib();
}

void Renderer::renderMouse3d() {
	if(game == NULL) {
		char szBuf[1024]="";
		sprintf(szBuf,"In [%s::%s] Line: %d game == NULL",__FILE__,__FUNCTION__,__LINE__);
		throw runtime_error(szBuf);
	}
	else if(game->getGui() == NULL) {
		char szBuf[1024]="";
		sprintf(szBuf,"In [%s::%s] Line: %d game->getGui() == NULL",__FILE__,__FUNCTION__,__LINE__);
		throw runtime_error(szBuf);
	}
	else if(game->getGui()->getMouse3d() == NULL) {
		char szBuf[1024]="";
		sprintf(szBuf,"In [%s::%s] Line: %d game->getGui()->getMouse3d() == NULL",__FILE__,__FUNCTION__,__LINE__);
		throw runtime_error(szBuf);
	}

	const Gui *gui= game->getGui();
	const Mouse3d *mouse3d= gui->getMouse3d();
	const Map *map= game->getWorld()->getMap();

	GLUquadricObj *cilQuadric;
	Vec4f color;

	assertGl();

	if((mouse3d->isEnabled() || gui->isPlacingBuilding()) && gui->isValidPosObjWorld()){
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glPushAttrib(GL_CURRENT_BIT | GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_BLEND);
		glDisable(GL_STENCIL_TEST);
		glDepthFunc(GL_LESS);
		glEnable(GL_COLOR_MATERIAL);
		glDepthMask(GL_FALSE);

		const Vec2i &pos= gui->getPosObjWorld();

		if(map == NULL) {
			char szBuf[1024]="";
			sprintf(szBuf,"In [%s::%s] Line: %d map == NULL",__FILE__,__FUNCTION__,__LINE__);
			throw runtime_error(szBuf);
		}

		Vec3f pos3f= Vec3f(pos.x, map->getCell(pos)->getHeight(), pos.y);

		if(gui->isPlacingBuilding()){

			const UnitType *building= gui->getBuilding();

			//selection building emplacement
			float offset= building->getSize()/2.f-0.5f;
			glTranslatef(pos3f.x+offset, pos3f.y, pos3f.z+offset);

			//choose color
			if(map->isFreeCells(pos, building->getSize(), fLand)){
				color= Vec4f(1.f, 1.f, 1.f, 0.5f);
			}
			else{
				color= Vec4f(1.f, 0.f, 0.f, 0.5f);
			}

			modelRenderer->begin(true, true, false);
			glColor4fv(color.ptr());
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, color.ptr());
			const Model *buildingModel= building->getFirstStOfClass(scStop)->getAnimation();

			if(gui->getSelectedFacing() != CardinalDir::NORTH) {
				float rotateAmount = gui->getSelectedFacing() * 90.f;
				if(rotateAmount > 0) {
					glRotatef(rotateAmount, 0.f, 1.f, 0.f);
				}
			}

			buildingModel->updateInterpolationData(0.f, false);
			modelRenderer->render(buildingModel);
			glDisable(GL_COLOR_MATERIAL);
			modelRenderer->end();

		}
		else{
			//standard mouse
			glDisable(GL_TEXTURE_2D);
			glDisable(GL_CULL_FACE);
			color= Vec4f(1.f, 0.f, 0.f, 1.f-mouse3d->getFade());
			glColor4fv(color.ptr());
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, color.ptr());

			glTranslatef(pos3f.x, pos3f.y+2.f, pos3f.z);
			glRotatef(90.f, 1.f, 0.f, 0.f);
			glRotatef(static_cast<float>(mouse3d->getRot()), 0.f, 0.f, 1.f);

			cilQuadric= gluNewQuadric();
			gluQuadricDrawStyle(cilQuadric, GLU_FILL);
			gluCylinder(cilQuadric, 0.5f, 0.f, 2.f, 4, 1);
			gluCylinder(cilQuadric, 0.5f, 0.f, 0.f, 4, 1);
			glTranslatef(0.f, 0.f, 1.f);
			gluCylinder(cilQuadric, 0.7f, 0.f, 1.f, 4, 1);
			gluCylinder(cilQuadric, 0.7f, 0.f, 0.f, 4, 1);
			gluDeleteQuadric(cilQuadric);
		}

		glPopAttrib();
		glPopMatrix();
	}

}

void Renderer::renderBackground(const Texture2D *texture) {

	const Metrics &metrics= Metrics::getInstance();

	assertGl();

    glPushAttrib(GL_ENABLE_BIT);

	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);

	renderQuad(0, 0, metrics.getVirtualW(), metrics.getVirtualH(), texture);

	glPopAttrib();

	assertGl();
}

void Renderer::renderTextureQuad(int x, int y, int w, int h, const Texture2D *texture, float alpha){
    assertGl();

	glPushAttrib(GL_ENABLE_BIT);

	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	glColor4f(1.f, 1.f, 1.f, alpha);
	renderQuad(x, y, w, h, texture);

	glPopAttrib();

	assertGl();
}

void Renderer::renderConsole(const Console *console,const bool showFullConsole,const bool showMenuConsole){

	if(console == NULL) {
		throw runtime_error("console == NULL");
	}

	glPushAttrib(GL_ENABLE_BIT);
	glEnable(GL_BLEND);
	Vec4f fontColor;

	if(game!=NULL){
		fontColor=game->getGui()->getDisplay()->getColor();
	}
	else {
		// white shadowed is default ( in the menu for example )
		fontColor=Vec4f(1.f, 1.f, 1.f, 0.0f);
	}
	if(showFullConsole){
		for(int i=0; i<console->getStoredLineCount(); ++i){
			renderTextShadow(
				console->getStoredLine(i),
				CoreData::getInstance().getConsoleFont(),
				fontColor,
				20, i*20+20);
		}
	}
	else if(showMenuConsole) {
		for(int i=0; i<console->getStoredLineCount() && i<maxConsoleLines; ++i){
			renderTextShadow(
				console->getStoredLine(i),
				CoreData::getInstance().getConsoleFont(),
				fontColor,
				20, i*20+20);
		}
	}
	else {
		for(int i=0; i<console->getLineCount(); ++i) {
			renderTextShadow(
				console->getLine(i),
				CoreData::getInstance().getConsoleFont(),
				fontColor,
				20, i*20+20);
		}
	}
	glPopAttrib();
}

void Renderer::renderChatManager(const ChatManager *chatManager){
	Vec4f fontColor;
	Lang &lang= Lang::getInstance();

	if(chatManager->getEditEnabled()){
		string text;

		if(chatManager->getTeamMode()){
			text+= lang.get("Team");
		}
		else
		{
			text+= lang.get("All");
		}
		text+= ": " + chatManager->getText() + "_";

		if(game!=NULL){
			fontColor=game->getGui()->getDisplay()->getColor();
		}
		else {
			// white shadowed is default ( in the menu for example )
			fontColor=Vec4f(1.f, 1.f, 1.f, 0.0f);
		}
		
		renderTextShadow(
				text,
				CoreData::getInstance().getConsoleFont(),
				fontColor,
				300, 150);
						
		//textRenderer->begin(CoreData::getInstance().getConsoleFont());
		//textRenderer->render(text, 300, 150);
		//textRenderer->end();
	}
}

void Renderer::renderResourceStatus(){

	const Metrics &metrics= Metrics::getInstance();
	const World *world= game->getWorld();
	const Faction *thisFaction= world->getFaction(world->getThisFactionIndex());
	const Vec4f fontColor=game->getGui()->getDisplay()->getColor();
	assertGl();

	glPushAttrib(GL_ENABLE_BIT);

	int j= 0;
	for(int i= 0; i<world->getTechTree()->getResourceTypeCount(); ++i){
		const ResourceType *rt= world->getTechTree()->getResourceType(i);
		const Resource *r= thisFaction->getResource(rt);

		//if any unit produces the resource
		bool showResource= false;
		for(int k=0; k<thisFaction->getType()->getUnitTypeCount(); ++k){
			const UnitType *ut= thisFaction->getType()->getUnitType(k);
			if(ut->getCost(rt)!=NULL){
				showResource= true;
				break;
			}
		}

		//draw resource status
		if(showResource){

			string str= intToStr(r->getAmount());

			glEnable(GL_TEXTURE_2D);
			glColor3f(1.f, 1.f, 1.f);

			renderQuad(j*100+200, metrics.getVirtualH()-30, 16, 16, rt->getImage());

			if(rt->getClass() != rcStatic)
			{
				str+= "/" + intToStr(thisFaction->getStoreAmount(rt));
			}
			if(rt->getClass()==rcConsumable){
				str+= "(";
				if(r->getBalance()>0){
					str+= "+";
				}
				str+= intToStr(r->getBalance()) + ")";
			}

			glDisable(GL_TEXTURE_2D);

			renderTextShadow(
				str, CoreData::getInstance().getDisplayFontSmall(),
				fontColor,
				j*100+220, metrics.getVirtualH()-30, false);
			++j;
		}

	}

	glPopAttrib();

	assertGl();
}

void Renderer::renderSelectionQuad(){

	const Gui *gui= game->getGui();
	const SelectionQuad *sq= gui->getSelectionQuad();

    Vec2i down= sq->getPosDown();
    Vec2i up= sq->getPosUp();

    if(gui->isSelecting()) {
        glPushAttrib(GL_CURRENT_BIT | GL_LINE_BIT);
            glColor3f(0,1,0);
            glBegin(GL_LINE_LOOP);
                glVertex2i(down.x, down.y);
                glVertex2i(up.x, down.y);
                glVertex2i(up.x, up.y);
                glVertex2i(down.x, up.y);
            glEnd();
        glPopAttrib();
    }
}

Vec2i computeCenteredPos(const string &text, const Font2D *font, int x, int y){
	if(font == NULL) {
		throw runtime_error("font == NULL");
	}
	const Metrics &metrics= Metrics::getInstance();
	const FontMetrics *fontMetrics= font->getMetrics();

	if(fontMetrics == NULL) {
		throw runtime_error("fontMetrics == NULL");
	}

	int virtualX = (fontMetrics->getTextWidth(text) > 0 ? static_cast<int>(fontMetrics->getTextWidth(text)/2.f) : 5);
	int virtualY = (fontMetrics->getHeight() > 0 ? static_cast<int>(fontMetrics->getHeight()/2.f) : 5);

	Vec2i textPos(
		x-metrics.toVirtualX(virtualX),
		y-metrics.toVirtualY(virtualY));

	return textPos;
}

void Renderer::renderText(const string &text, const Font2D *font, float alpha, int x, int y, bool centered){
	glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
	glEnable(GL_BLEND);
	glColor4fv(Vec4f(1.f, 1.f, 1.f, alpha).ptr());

	Vec2i pos= centered? computeCenteredPos(text, font, x, y): Vec2i(x, y);

	textRenderer->begin(font);
	textRenderer->render(text, pos.x, pos.y);
	textRenderer->end();

	glPopAttrib();
}

void Renderer::renderText(const string &text, const Font2D *font, const Vec3f &color, int x, int y, bool centered){
	glPushAttrib(GL_CURRENT_BIT);
	glColor3fv(color.ptr());

	Vec2i pos= centered? computeCenteredPos(text, font, x, y): Vec2i(x, y);

	textRenderer->begin(font);
	textRenderer->render(text, pos.x, pos.y);
	textRenderer->end();

	glPopAttrib();
}

void Renderer::renderText(const string &text, const Font2D *font, const Vec4f &color, int x, int y, bool centered){
	glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
	glEnable(GL_BLEND);
	glColor4fv(color.ptr());

	Vec2i pos= centered? computeCenteredPos(text, font, x, y): Vec2i(x, y);

	textRenderer->begin(font);
	textRenderer->render(text, pos.x, pos.y);
	textRenderer->end();

	glPopAttrib();
}

void Renderer::renderTextShadow(const string &text, const Font2D *font,const Vec4f &color, int x, int y, bool centered){
	if(font == NULL) {
		throw runtime_error("font == NULL");
	}

	glPushAttrib(GL_CURRENT_BIT);

	Vec2i pos= centered? computeCenteredPos(text, font, x, y): Vec2i(x, y);

	textRenderer->begin(font);
	if(color.w<0.5)	{
		glColor3f(0.0f, 0.0f, 0.0f);
		textRenderer->render(text, pos.x-1.0f, pos.y-1.0f);
	}
	glColor3f(color.x,color.y,color.z);
	textRenderer->render(text, pos.x, pos.y);
	textRenderer->end();

	glPopAttrib();
}

// ============= COMPONENTS =============================

void Renderer::renderLabel(const GraphicLabel *label) {
	if(label->getVisible() == false) {
		return;
	}
	glPushAttrib(GL_ENABLE_BIT);
	glEnable(GL_BLEND);

	Vec2i textPos;
	int x= label->getX();
    int y= label->getY();
    int h= label->getH();
    int w= label->getW();

	if(label->getCentered()){
		textPos= Vec2i(x+w/2, y+h/2);
	}
	else{
		textPos= Vec2i(x, y+h/4);
	}

	renderText(label->getText(), label->getFont(), GraphicComponent::getFade(), textPos.x, textPos.y, label->getCentered());

	glPopAttrib();
}

void Renderer::renderButton(const GraphicButton *button) {
	if(button->getVisible() == false) {
		return;
	}

    int x= button->getX();
    int y= button->getY();
    int h= button->getH();
    int w= button->getW();

	const Vec3f disabledTextColor= Vec3f(0.25f,0.25f,0.25f);

	glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT);

	//background
	CoreData &coreData= CoreData::getInstance();
	Texture2D *backTexture= w>3*h/2? coreData.getButtonBigTexture(): coreData.getButtonSmallTexture();

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	glBindTexture(GL_TEXTURE_2D, static_cast<Texture2DGl*>(backTexture)->getHandle());

	//button
	Vec4f fontColor;
	//if(game!=NULL){
	//	fontColor=game->getGui()->getDisplay()->getColor();
	//	fontColor.w = GraphicComponent::getFade();
	//}
	//else {
		// white shadowed is default ( in the menu for example )
		fontColor=Vec4f(1.f, 1.f, 1.f, GraphicComponent::getFade());
	//}

	//Vec4f color= Vec4f(1.f, 1.f, 1.f, GraphicComponent::getFade());
	Vec4f color= fontColor;
	glColor4fv(color.ptr());

	glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0.f, 0.f);
		glVertex2f(x, y);

		glTexCoord2f(0.f, 1.f);
		glVertex2f(x, y+h);

		glTexCoord2f(1.f, 0.f);
		glVertex2f(x+w, y);

		glTexCoord2f(1.f, 1.f);
		glVertex2f(x+w, y+h);

	glEnd();

	glDisable(GL_TEXTURE_2D);

	//lighting
	float anim= GraphicComponent::getAnim();
	if(anim>0.5f) anim= 1.f-anim;

	if(button->getLighted() && button->getEditable()){
		const int lightSize= 0;
		const Vec4f color1= Vec4f(color.x, color.y, color.z, 0.1f+anim*0.5f);
		const Vec4f color2= Vec4f(color.x, color.y, color.z, 0.3f+anim);

		glBegin(GL_TRIANGLE_FAN);

		glColor4fv(color2.ptr());
		glVertex2f(x+w/2, y+h/2);

		glColor4fv(color1.ptr());
		glVertex2f(x-lightSize, y-lightSize);

		glColor4fv(color1.ptr());
		glVertex2f(x+w+lightSize, y-lightSize);

		glColor4fv(color1.ptr());
		glVertex2f(x+w+lightSize, y+h+lightSize);

		glColor4fv(color1.ptr());
		glVertex2f(x+w+lightSize, y+h+lightSize);

		glColor4fv(color1.ptr());
		glVertex2f(x-lightSize, y+h+lightSize);

		glColor4fv(color1.ptr());
		glVertex2f(x-lightSize, y-lightSize);

		glEnd();
	}

	Vec2i textPos= Vec2i(x+w/2, y+h/2);

	if(button->getEditable()){

		renderText(
			button->getText(), button->getFont(), color,
			x+w/2, y+h/2, true);
	}
	else {
		renderText(
			button->getText(), button->getFont(),disabledTextColor,
			x+w/2, y+h/2, true);
	}

    glPopAttrib();
}

void Renderer::renderListBox(const GraphicListBox *listBox) {
	if(listBox->getVisible() == false) {
		return;
	}

	renderButton(listBox->getButton1());
    renderButton(listBox->getButton2());

	glPushAttrib(GL_ENABLE_BIT);
	glEnable(GL_BLEND);

	GraphicLabel label;
	label.init(listBox->getX(), listBox->getY(), listBox->getW(), listBox->getH(), true);
	label.setText(listBox->getText());
	label.setFont(listBox->getFont());
	renderLabel(&label);

	glPopAttrib();
}

void Renderer::renderMessageBox(const GraphicMessageBox *messageBox) {
	if(messageBox->getVisible() == false) {
		return;
	}

	//background
	glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
	glEnable(GL_BLEND);

	glColor4f(0.0f, 0.0f, 0.0f, 0.5f) ;
	glBegin(GL_TRIANGLE_STRIP);
		glVertex2i(messageBox->getX(), messageBox->getY()+9*messageBox->getH()/10);
		glVertex2i(messageBox->getX(), messageBox->getY());
		glVertex2i(messageBox->getX() + messageBox->getW(), messageBox->getY() + 9*messageBox->getH()/10);
		glVertex2i(messageBox->getX() + messageBox->getW(), messageBox->getY());
	glEnd();

	glColor4f(0.0f, 0.0f, 0.0f, 0.8f) ;
	glBegin(GL_TRIANGLE_STRIP);
		glVertex2i(messageBox->getX(), messageBox->getY()+messageBox->getH());
		glVertex2i(messageBox->getX(), messageBox->getY()+9*messageBox->getH()/10);
		glVertex2i(messageBox->getX() + messageBox->getW(), messageBox->getY() + messageBox->getH());
		glVertex2i(messageBox->getX() + messageBox->getW(), messageBox->getY()+9*messageBox->getH()/10);
	glEnd();

	glBegin(GL_LINE_LOOP);
		glColor4f(0.5f, 0.5f, 0.5f, 0.25f) ;
		glVertex2i(messageBox->getX(), messageBox->getY());

		glColor4f(0.0f, 0.0f, 0.0f, 0.25f) ;
		glVertex2i(messageBox->getX()+ messageBox->getW(), messageBox->getY());

		glColor4f(0.5f, 0.5f, 0.5f, 0.25f) ;
		glVertex2i(messageBox->getX()+ messageBox->getW(), messageBox->getY() + messageBox->getH());

		glColor4f(0.25f, 0.25f, 0.25f, 0.25f) ;
		glVertex2i(messageBox->getX(), messageBox->getY() + messageBox->getH());
	glEnd();

	glBegin(GL_LINE_STRIP);
		glColor4f(1.0f, 1.0f, 1.0f, 0.25f) ;
		glVertex2i(messageBox->getX(), messageBox->getY() + 90*messageBox->getH()/100);

		glColor4f(0.5f, 0.5f, 0.5f, 0.25f) ;
		glVertex2i(messageBox->getX()+ messageBox->getW(), messageBox->getY() + 90*messageBox->getH()/100);
	glEnd();

	glPopAttrib();

	//buttons
	renderButton(messageBox->getButton1());
	if(messageBox->getButtonCount()==2){
		renderButton(messageBox->getButton2());
	}

	Vec4f fontColor;
	//if(game!=NULL){
	//	fontColor=game->getGui()->getDisplay()->getColor();
	//}
	//else {
		// white shadowed is default ( in the menu for example )
		fontColor=Vec4f(1.f, 1.f, 1.f, 1.0f);
	//}

	//text
	renderTextShadow(
		messageBox->getText(), messageBox->getFont(), fontColor,
		messageBox->getX()+15, messageBox->getY()+7*messageBox->getH()/10,
		false );

	renderTextShadow(
		messageBox->getHeader(), messageBox->getFont(),fontColor,
		messageBox->getX()+15, messageBox->getY()+93*messageBox->getH()/100,
		false );
}

// ==================== complex rendering ====================

void Renderer::renderSurface(const int renderFps, const int worldFrameCount) {
	IF_DEBUG_EDITION(
		if (getDebugRenderer().willRenderSurface()) {
			getDebugRenderer().renderSurface(visibleQuad / Map::cellScale);
		} else {
	)
	int lastTex=-1;
	int currTex;
	const World *world= game->getWorld();
	const Map *map= world->getMap();
	const Rect2i mapBounds(0, 0, map->getSurfaceW()-1, map->getSurfaceH()-1);
	float coordStep= world->getTileset()->getSurfaceAtlas()->getCoordStep();

	assertGl();

	const Texture2D *fowTex= world->getMinimap()->getFowTexture();

	glPushAttrib(GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_FOG_BIT | GL_TEXTURE_BIT);

	glEnable(GL_BLEND);
	glEnable(GL_COLOR_MATERIAL);
	glDisable(GL_ALPHA_TEST);
	glEnable(GL_CULL_FACE);

	//fog of war tex unit
	glActiveTexture(fowTexUnit);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, static_cast<const Texture2DGl*>(fowTex)->getHandle());
	glTexSubImage2D(
		GL_TEXTURE_2D, 0, 0, 0,
		fowTex->getPixmap()->getW(), fowTex->getPixmap()->getH(),
		GL_ALPHA, GL_UNSIGNED_BYTE, fowTex->getPixmap()->getPixels());

	if(lastRenderFps >= MIN_FPS_NORMAL_RENDERING && renderFps >= MIN_FPS_NORMAL_RENDERING_TOP_THRESHOLD) {
		//shadow texture
		if(shadows==sProjected || shadows==sShadowMapping){
			glActiveTexture(shadowTexUnit);
			glEnable(GL_TEXTURE_2D);

			glBindTexture(GL_TEXTURE_2D, shadowMapHandle);

			static_cast<ModelRendererGl*>(modelRenderer)->setDuplicateTexCoords(true);
			enableProjectiveTexturing();
		}
	}

	glActiveTexture(baseTexUnit);

	if(useQuadCache == true) {
		VisibleQuadContainerCache &qCache = getQuadCache();
		if(qCache.visibleScaledCellList.size() > 0) {
			for(int visibleIndex = 0;
					visibleIndex < qCache.visibleScaledCellList.size(); ++visibleIndex) {
				Vec2i &pos = qCache.visibleScaledCellList[visibleIndex];

				SurfaceCell *tc00= map->getSurfaceCell(pos.x, pos.y);
				SurfaceCell *tc10= map->getSurfaceCell(pos.x+1, pos.y);
				SurfaceCell *tc01= map->getSurfaceCell(pos.x, pos.y+1);
				SurfaceCell *tc11= map->getSurfaceCell(pos.x+1, pos.y+1);

				if(tc00 == NULL) {
					throw runtime_error("tc00 == NULL");
				}
				if(tc10 == NULL) {
					throw runtime_error("tc10 == NULL");
				}
				if(tc01 == NULL) {
					throw runtime_error("tc01 == NULL");
				}
				if(tc11 == NULL) {
					throw runtime_error("tc11 == NULL");
				}

				triangleCount+= 2;
				pointCount+= 4;

				//set texture
				if(tc00->getSurfaceTexture() == NULL) {
					throw runtime_error("tc00->getSurfaceTexture() == NULL");
				}
				currTex= static_cast<const Texture2DGl*>(tc00->getSurfaceTexture())->getHandle();
				if(currTex != lastTex) {
					lastTex = currTex;
					glBindTexture(GL_TEXTURE_2D, lastTex);
				}

				const Vec2f &surfCoord= tc00->getSurfTexCoord();

				glBegin(GL_TRIANGLE_STRIP);

				//draw quad using immediate mode
				glMultiTexCoord2fv(fowTexUnit, tc01->getFowTexCoord().ptr());
				glMultiTexCoord2f(baseTexUnit, surfCoord.x, surfCoord.y + coordStep);
				glNormal3fv(tc01->getNormal().ptr());
				glVertex3fv(tc01->getVertex().ptr());

				glMultiTexCoord2fv(fowTexUnit, tc00->getFowTexCoord().ptr());
				glMultiTexCoord2f(baseTexUnit, surfCoord.x, surfCoord.y);
				glNormal3fv(tc00->getNormal().ptr());
				glVertex3fv(tc00->getVertex().ptr());

				glMultiTexCoord2fv(fowTexUnit, tc11->getFowTexCoord().ptr());
				glMultiTexCoord2f(baseTexUnit, surfCoord.x+coordStep, surfCoord.y+coordStep);
				glNormal3fv(tc11->getNormal().ptr());
				glVertex3fv(tc11->getVertex().ptr());

				glMultiTexCoord2fv(fowTexUnit, tc10->getFowTexCoord().ptr());
				glMultiTexCoord2f(baseTexUnit, surfCoord.x + coordStep, surfCoord.y);
				glNormal3fv(tc10->getNormal().ptr());
				glVertex3fv(tc10->getVertex().ptr());

				glEnd();
			}
		}
	}
	else {
		Quad2i scaledQuad= visibleQuad/Map::cellScale;

		PosQuadIterator pqi(scaledQuad);
		while(pqi.next()) {
			const Vec2i &pos= pqi.getPos();
			if(mapBounds.isInside(pos)){

				SurfaceCell *tc00= map->getSurfaceCell(pos.x, pos.y);
				SurfaceCell *tc10= map->getSurfaceCell(pos.x+1, pos.y);
				SurfaceCell *tc01= map->getSurfaceCell(pos.x, pos.y+1);
				SurfaceCell *tc11= map->getSurfaceCell(pos.x+1, pos.y+1);

				if(tc00 == NULL) {
					throw runtime_error("tc00 == NULL");
				}
				if(tc10 == NULL) {
					throw runtime_error("tc10 == NULL");
				}
				if(tc01 == NULL) {
					throw runtime_error("tc01 == NULL");
				}
				if(tc11 == NULL) {
					throw runtime_error("tc11 == NULL");
				}

				triangleCount+= 2;
				pointCount+= 4;

				//set texture
				currTex= static_cast<const Texture2DGl*>(tc00->getSurfaceTexture())->getHandle();
				if(currTex!=lastTex){
					lastTex=currTex;
					glBindTexture(GL_TEXTURE_2D, lastTex);
				}

				const Vec2f &surfCoord= tc00->getSurfTexCoord();

				glBegin(GL_TRIANGLE_STRIP);

				//draw quad using immediate mode
				glMultiTexCoord2fv(fowTexUnit, tc01->getFowTexCoord().ptr());
				glMultiTexCoord2f(baseTexUnit, surfCoord.x, surfCoord.y + coordStep);
				glNormal3fv(tc01->getNormal().ptr());
				glVertex3fv(tc01->getVertex().ptr());

				glMultiTexCoord2fv(fowTexUnit, tc00->getFowTexCoord().ptr());
				glMultiTexCoord2f(baseTexUnit, surfCoord.x, surfCoord.y);
				glNormal3fv(tc00->getNormal().ptr());
				glVertex3fv(tc00->getVertex().ptr());

				glMultiTexCoord2fv(fowTexUnit, tc11->getFowTexCoord().ptr());
				glMultiTexCoord2f(baseTexUnit, surfCoord.x+coordStep, surfCoord.y+coordStep);
				glNormal3fv(tc11->getNormal().ptr());
				glVertex3fv(tc11->getVertex().ptr());

				glMultiTexCoord2fv(fowTexUnit, tc10->getFowTexCoord().ptr());
				glMultiTexCoord2f(baseTexUnit, surfCoord.x + coordStep, surfCoord.y);
				glNormal3fv(tc10->getNormal().ptr());
				glVertex3fv(tc10->getVertex().ptr());

				glEnd();
			}
		}
		//glEnd();
	}

	//Restore
	static_cast<ModelRendererGl*>(modelRenderer)->setDuplicateTexCoords(false);
	glPopAttrib();

	//assert
	glGetError();	//remove when first mtex problem solved
	assertGl();

	IF_DEBUG_EDITION(
		} // end else, if not renderering debug textures instead of regular terrain
		getDebugRenderer().renderEffects(visibleQuad / Map::cellScale);
	)
}

void Renderer::renderObjects(const int renderFps, const int worldFrameCount) {
	const World *world= game->getWorld();
	const Map *map= world->getMap();

    assertGl();

	const Texture2D *fowTex= NULL;
	Vec3f baseFogColor;


    bool modelRenderStarted = false;

    if(useQuadCache == true) {
		VisibleQuadContainerCache &qCache = getQuadCache();
		for(int visibleIndex = 0;
				visibleIndex < qCache.visibleObjectList.size(); ++visibleIndex) {
			Object *o = qCache.visibleObjectList[visibleIndex];

			const Model *objModel= o->getModel();
			const Vec3f &v= o->getConstPos();

			if(modelRenderStarted == false) {
				modelRenderStarted = true;

				fowTex= world->getMinimap()->getFowTexture();
				baseFogColor= world->getTileset()->getFogColor() * computeLightColor(world->getTimeFlow()->getTime());

				glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_FOG_BIT | GL_LIGHTING_BIT | GL_TEXTURE_BIT);

				if(lastRenderFps >= MIN_FPS_NORMAL_RENDERING && renderFps >= MIN_FPS_NORMAL_RENDERING_TOP_THRESHOLD &&
					shadows == sShadowMapping) {
					glActiveTexture(shadowTexUnit);
					glEnable(GL_TEXTURE_2D);

					glBindTexture(GL_TEXTURE_2D, shadowMapHandle);

					static_cast<ModelRendererGl*>(modelRenderer)->setDuplicateTexCoords(true);
					enableProjectiveTexturing();
				}

				glActiveTexture(baseTexUnit);

				glEnable(GL_COLOR_MATERIAL);
				glAlphaFunc(GL_GREATER, 0.5f);

				modelRenderer->begin(true, true, false);
			}
			//ambient and diffuse color is taken from cell color

			float fowFactor= fowTex->getPixmap()->getPixelf(o->getMapPos().x / Map::cellScale, o->getMapPos().y / Map::cellScale);
			Vec4f color= Vec4f(Vec3f(fowFactor), 1.f);
			glColor4fv(color.ptr());
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, (color * ambFactor).ptr());
			glFogfv(GL_FOG_COLOR, (baseFogColor * fowFactor).ptr());

			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glTranslatef(v.x, v.y, v.z);
			glRotatef(o->getRotation(), 0.f, 1.f, 0.f);

			objModel->updateInterpolationData(0.f, true);
			modelRenderer->render(objModel);

			triangleCount+= objModel->getTriangleCount();
			pointCount+= objModel->getVertexCount();

			glPopMatrix();
		}

		if(modelRenderStarted == true) {
			modelRenderer->end();
			glPopAttrib();
		}

		//restore
		static_cast<ModelRendererGl*>(modelRenderer)->setDuplicateTexCoords(true);
    }
    else {
		int thisTeamIndex= world->getThisTeamIndex();
		bool modelRenderStarted = false;

		PosQuadIterator pqi(visibleQuad, Map::cellScale);
		while(pqi.next()) {
			const Vec2i &pos= pqi.getPos();
			if(map->isInside(pos)){
				SurfaceCell *sc= map->getSurfaceCell(Map::toSurfCoords(pos));
				Object *o= sc->getObject();
				if(sc->isExplored(thisTeamIndex) && o!=NULL){
					const Model *objModel= sc->getObject()->getModel();
					const Vec3f &v= o->getConstPos();

					if(modelRenderStarted == false) {
						modelRenderStarted = true;

						fowTex= world->getMinimap()->getFowTexture();
						baseFogColor= world->getTileset()->getFogColor()*computeLightColor(world->getTimeFlow()->getTime());

						glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_FOG_BIT | GL_LIGHTING_BIT | GL_TEXTURE_BIT);

						if(lastRenderFps >= MIN_FPS_NORMAL_RENDERING && renderFps >= MIN_FPS_NORMAL_RENDERING_TOP_THRESHOLD &&
							shadows == sShadowMapping) {
							glActiveTexture(shadowTexUnit);
							glEnable(GL_TEXTURE_2D);

							glBindTexture(GL_TEXTURE_2D, shadowMapHandle);

							static_cast<ModelRendererGl*>(modelRenderer)->setDuplicateTexCoords(true);
							enableProjectiveTexturing();
						}

						glActiveTexture(baseTexUnit);

						glEnable(GL_COLOR_MATERIAL);
						glAlphaFunc(GL_GREATER, 0.5f);

						modelRenderer->begin(true, true, false);
					}
					//ambient and diffuse color is taken from cell color

					float fowFactor= fowTex->getPixmap()->getPixelf(pos.x / Map::cellScale, pos.y / Map::cellScale);
					Vec4f color= Vec4f(Vec3f(fowFactor), 1.f);
					glColor4fv(color.ptr());
					glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, (color*ambFactor).ptr());
					glFogfv(GL_FOG_COLOR, (baseFogColor*fowFactor).ptr());

					glMatrixMode(GL_MODELVIEW);
					glPushMatrix();
					glTranslatef(v.x, v.y, v.z);
					glRotatef(o->getRotation(), 0.f, 1.f, 0.f);

					objModel->updateInterpolationData(0.f, true);
					modelRenderer->render(objModel);

					triangleCount+= objModel->getTriangleCount();
					pointCount+= objModel->getVertexCount();

					glPopMatrix();
				}
			}
		}

		if(modelRenderStarted == true) {
			modelRenderer->end();
			glPopAttrib();
		}

		//restore
		static_cast<ModelRendererGl*>(modelRenderer)->setDuplicateTexCoords(true);
    }

	assertGl();
}

void Renderer::renderWater() {

	bool closed= false;
	const World *world= game->getWorld();
	const Map *map= world->getMap();

	float waterAnim= world->getWaterEffects()->getAmin();

	//assert
    assertGl();

	glPushAttrib(GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);

	//water texture nit
    glDisable(GL_TEXTURE_2D);

	glEnable(GL_BLEND);
	if(textures3D){
		Texture3D *waterTex= world->getTileset()->getWaterTex();
		if(waterTex == NULL) {
			throw runtime_error("waterTex == NULL");
		}
		glEnable(GL_TEXTURE_3D);
		glBindTexture(GL_TEXTURE_3D, static_cast<Texture3DGl*>(waterTex)->getHandle());
	}
	else{
		glEnable(GL_COLOR_MATERIAL);
		glColor4f(0.5f, 0.5f, 1.0f, 0.5f);
		glBindTexture(GL_TEXTURE_3D, 0);
	}

	assertGl();

	//fog of War texture Unit
	const Texture2D *fowTex= world->getMinimap()->getFowTexture();
	glActiveTexture(fowTexUnit);
	glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, static_cast<const Texture2DGl*>(fowTex)->getHandle());
    glActiveTexture(baseTexUnit);

	assertGl();

	Rect2i boundingRect= visibleQuad.computeBoundingRect();
	Rect2i scaledRect= boundingRect/Map::cellScale;
	scaledRect.clamp(0, 0, map->getSurfaceW()-1, map->getSurfaceH()-1);

	float waterLevel= world->getMap()->getWaterLevel();
    for(int j=scaledRect.p[0].y; j<scaledRect.p[1].y; ++j){
        glBegin(GL_TRIANGLE_STRIP);

		for(int i=scaledRect.p[0].x; i<=scaledRect.p[1].x; ++i){

			SurfaceCell *tc0= map->getSurfaceCell(i, j);
            SurfaceCell *tc1= map->getSurfaceCell(i, j+1);

			int thisTeamIndex= world->getThisTeamIndex();
			if(tc0->getNearSubmerged() && (tc0->isExplored(thisTeamIndex) || tc1->isExplored(thisTeamIndex))){
				glNormal3f(0.f, 1.f, 0.f);
                closed= false;

                triangleCount+= 2;
				pointCount+= 2;

                //vertex 1
				glMaterialfv(
					GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE,
					computeWaterColor(waterLevel, tc1->getHeight()).ptr());
				glMultiTexCoord2fv(GL_TEXTURE1, tc1->getFowTexCoord().ptr());
                glTexCoord3f(i, 1.f, waterAnim);
				glVertex3f(
					static_cast<float>(i)*Map::mapScale,
					waterLevel,
					static_cast<float>(j+1)*Map::mapScale);

                //vertex 2
				glMaterialfv(
					GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE,
					computeWaterColor(waterLevel, tc0->getHeight()).ptr());
				glMultiTexCoord2fv(GL_TEXTURE1, tc0->getFowTexCoord().ptr());
                glTexCoord3f(i, 0.f, waterAnim);
                glVertex3f(
					static_cast<float>(i)*Map::mapScale,
					waterLevel,
					static_cast<float>(j)*Map::mapScale);

            }
            else{
				if(!closed){

					pointCount+= 2;

					//vertex 1
					glMaterialfv(
						GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE,
						computeWaterColor(waterLevel, tc1->getHeight()).ptr());
					glMultiTexCoord2fv(GL_TEXTURE1, tc1->getFowTexCoord().ptr());
					glTexCoord3f(i, 1.f, waterAnim);
					glVertex3f(
						static_cast<float>(i)*Map::mapScale,
						waterLevel,
						static_cast<float>(j+1)*Map::mapScale);

					//vertex 2
					glMaterialfv(
						GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE,
						computeWaterColor(waterLevel, tc0->getHeight()).ptr());
					glMultiTexCoord2fv(GL_TEXTURE1, tc0->getFowTexCoord().ptr());
					glTexCoord3f(i, 0.f, waterAnim);
					glVertex3f(
						static_cast<float>(i)*Map::mapScale,
						waterLevel,
						static_cast<float>(j)*Map::mapScale);

					glEnd();
					glBegin(GL_TRIANGLE_STRIP);
					closed= true;
				}
           	}
        }
        glEnd();
    }

	//restore
	glPopAttrib();

	assertGl();
}

void Renderer::renderUnits(const int renderFps, const int worldFrameCount) {
	Unit *unit=NULL;
	const World *world= game->getWorld();
	MeshCallbackTeamColor meshCallbackTeamColor;

	//assert
	assertGl();

	visibleFrameUnitList.clear();

	bool modelRenderStarted = false;

	if(useQuadCache == true) {
		VisibleQuadContainerCache &qCache = getQuadCache();
		if(qCache.visibleQuadUnitList.size() > 0) {
			for(int visibleUnitIndex = 0;
					visibleUnitIndex < qCache.visibleQuadUnitList.size(); ++visibleUnitIndex) {
				Unit *unit = qCache.visibleQuadUnitList[visibleUnitIndex];

				meshCallbackTeamColor.setTeamTexture(unit->getFaction()->getTexture());

				if(modelRenderStarted == false) {
					modelRenderStarted = true;

					glPushAttrib(GL_ENABLE_BIT | GL_FOG_BIT | GL_LIGHTING_BIT | GL_TEXTURE_BIT);
					glEnable(GL_COLOR_MATERIAL);

					if(lastRenderFps >= MIN_FPS_NORMAL_RENDERING && renderFps >= MIN_FPS_NORMAL_RENDERING_TOP_THRESHOLD) {
						if(shadows == sShadowMapping) {
							glActiveTexture(shadowTexUnit);
							glEnable(GL_TEXTURE_2D);

							glBindTexture(GL_TEXTURE_2D, shadowMapHandle);

							static_cast<ModelRendererGl*>(modelRenderer)->setDuplicateTexCoords(true);
							enableProjectiveTexturing();
						}
					}
					glActiveTexture(baseTexUnit);

					modelRenderer->begin(true, true, true, &meshCallbackTeamColor);
				}

				glMatrixMode(GL_MODELVIEW);
				glPushMatrix();

				//translate
				Vec3f currVec= unit->getCurrVectorFlat();
				glTranslatef(currVec.x, currVec.y, currVec.z);

				//rotate
				glRotatef(unit->getRotation(), 0.f, 1.f, 0.f);
				glRotatef(unit->getVerticalRotation(), 1.f, 0.f, 0.f);

				//dead alpha
				float alpha= 1.0f;
				const SkillType *st= unit->getCurrSkill();
				if(st->getClass() == scDie && static_cast<const DieSkillType*>(st)->getFade()) {
					alpha= 1.0f-unit->getAnimProgress();
					glDisable(GL_COLOR_MATERIAL);
					glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, Vec4f(1.0f, 1.0f, 1.0f, alpha).ptr());
				}
				else {
					glEnable(GL_COLOR_MATERIAL);
					glAlphaFunc(GL_GREATER, 0.4f);
				}

				//render
				const Model *model= unit->getCurrentModel();
				model->updateInterpolationData(unit->getAnimProgress(), unit->isAlive());

				modelRenderer->render(model);
				triangleCount+= model->getTriangleCount();
				pointCount+= model->getVertexCount();

				glPopMatrix();
				unit->setVisible(true);
				unit->setScreenPos(computeScreenPosition(unit->getCurrVectorFlat()));
				visibleFrameUnitList.push_back(unit);

				if(allowRenderUnitTitles == true) {
					// Add to the pending render unit title list
					renderUnitTitleList.push_back(std::pair<Unit *,Vec3f>(unit,computeScreenPosition(unit->getCurrVectorFlat())) );
				}
			}

			if(modelRenderStarted == true) {
				modelRenderer->end();
				glPopAttrib();
			}
		}

		//restore
		static_cast<ModelRendererGl*>(modelRenderer)->setDuplicateTexCoords(true);
	}
	else {
		bool modelRenderStarted = false;

		for(int i=0; i<world->getFactionCount(); ++i){
			meshCallbackTeamColor.setTeamTexture(world->getFaction(i)->getTexture());
			for(int j=0; j<world->getFaction(i)->getUnitCount(); ++j){
				unit= world->getFaction(i)->getUnit(j);
				if(world->toRenderUnit(unit, visibleQuad)) {

					if(modelRenderStarted == false) {
						modelRenderStarted = true;

						glPushAttrib(GL_ENABLE_BIT | GL_FOG_BIT | GL_LIGHTING_BIT | GL_TEXTURE_BIT);
						glEnable(GL_COLOR_MATERIAL);

						if(lastRenderFps >= MIN_FPS_NORMAL_RENDERING && renderFps >= MIN_FPS_NORMAL_RENDERING_TOP_THRESHOLD) {
							if(shadows==sShadowMapping){
								glActiveTexture(shadowTexUnit);
								glEnable(GL_TEXTURE_2D);

								glBindTexture(GL_TEXTURE_2D, shadowMapHandle);

								static_cast<ModelRendererGl*>(modelRenderer)->setDuplicateTexCoords(true);
								enableProjectiveTexturing();
							}
						}
						glActiveTexture(baseTexUnit);

						modelRenderer->begin(true, true, true, &meshCallbackTeamColor);
					}

					glMatrixMode(GL_MODELVIEW);
					glPushMatrix();

					//translate
					Vec3f currVec= unit->getCurrVectorFlat();
					glTranslatef(currVec.x, currVec.y, currVec.z);

					//rotate
					glRotatef(unit->getRotation(), 0.f, 1.f, 0.f);
					glRotatef(unit->getVerticalRotation(), 1.f, 0.f, 0.f);

					//dead alpha
					float alpha= 1.0f;
					const SkillType *st= unit->getCurrSkill();
					if(st->getClass()==scDie && static_cast<const DieSkillType*>(st)->getFade()){
						alpha= 1.0f-unit->getAnimProgress();
						glDisable(GL_COLOR_MATERIAL);
						glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, Vec4f(1.0f, 1.0f, 1.0f, alpha).ptr());
					}
					else{
						glEnable(GL_COLOR_MATERIAL);
						glAlphaFunc(GL_GREATER, 0.4f);
					}

					//render
					const Model *model= unit->getCurrentModel();
					model->updateInterpolationData(unit->getAnimProgress(), unit->isAlive());

					modelRenderer->render(model);
					triangleCount+= model->getTriangleCount();
					pointCount+= model->getVertexCount();

					glPopMatrix();
					unit->setVisible(true);
					unit->setScreenPos(computeScreenPosition(unit->getCurrVectorFlat()));
					visibleFrameUnitList.push_back(unit);

					if(allowRenderUnitTitles == true) {
						// Add to the pending render unit title list
						renderUnitTitleList.push_back(std::pair<Unit *,Vec3f>(unit,computeScreenPosition(unit->getCurrVectorFlat())) );
					}
				}
				else
				{
					unit->setVisible(false);
				}
			}
		}
		if(modelRenderStarted == true) {
			modelRenderer->end();
			glPopAttrib();
		}

		//restore
		static_cast<ModelRendererGl*>(modelRenderer)->setDuplicateTexCoords(true);
	}
	// reset alpha
	glAlphaFunc(GL_GREATER, 0.0f);
	//assert
	assertGl();
}

void Renderer::renderSelectionEffects(){

	const World *world= game->getWorld();
	const Map *map= world->getMap();
	const Selection *selection= game->getGui()->getSelection();

	glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	glDepthFunc(GL_ALWAYS);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glLineWidth(2.f);

	//units
	for(int i=0; i<selection->getCount(); ++i){

		const Unit *unit= selection->getUnit(i);

		//translate
		Vec3f currVec= unit->getCurrVectorFlat();
		currVec.y+= 0.3f;

		//selection circle
		if(world->getThisFactionIndex() == unit->getFactionIndex()) {
			if(	showDebugUI == true && unit->getCommandSize() > 0 &&
				dynamic_cast<const BuildCommandType *>(unit->getCurrCommand()->getCommandType()) != NULL) {
				glColor4f(unit->getHpRatio(), unit->getHpRatio(), unit->getHpRatio(), 0.3f);
			}
			else {
				glColor4f(0, unit->getHpRatio(), 0, 0.3f);
			}
		}
		else if ( world->getThisTeamIndex() == unit->getTeam()) {
			glColor4f(unit->getHpRatio(), unit->getHpRatio(), 0, 0.3f);
		}
		else{
			glColor4f(unit->getHpRatio(), 0, 0, 0.3f);
		}
		renderSelectionCircle(currVec, unit->getType()->getSize(), selectionCircleRadius);

		//magic circle
		if(world->getThisFactionIndex() == unit->getFactionIndex() && unit->getType()->getMaxEp() > 0) {
			glColor4f(unit->getEpRatio()/2.f, unit->getEpRatio(), unit->getEpRatio(), 0.5f);
			renderSelectionCircle(currVec, unit->getType()->getSize(), magicCircleRadius);
		}
	}

	//target arrow
	if(selection->getCount() == 1) {
		const Unit *unit= selection->getUnit(0);

		//comand arrow
		if(focusArrows && unit->anyCommand()) {
			const CommandType *ct= unit->getCurrCommand()->getCommandType();
			if(ct->getClicks() != cOne){

				//arrow color
				Vec3f arrowColor;
				switch(ct->getClass()) {
				case ccMove:
					arrowColor= Vec3f(0.f, 1.f, 0.f);
					break;
				case ccAttack:
				case ccAttackStopped:
					arrowColor= Vec3f(1.f, 0.f, 0.f);
					break;
				default:
					arrowColor= Vec3f(1.f, 1.f, 0.f);
				}

				//arrow target
				Vec3f arrowTarget;
				Command *c= unit->getCurrCommand();
				if(c->getUnit() != NULL) {
					arrowTarget= c->getUnit()->getCurrVectorFlat();
				}
				else {
					Vec2i pos= c->getPos();
					arrowTarget= Vec3f(pos.x, map->getCell(pos)->getHeight(), pos.y);
				}

				renderArrow(unit->getCurrVectorFlat(), arrowTarget, arrowColor, 0.3f);
			}
		}

		//meeting point arrow
		if(unit->getType()->getMeetingPoint()) {
			Vec2i pos= unit->getMeetingPos();
			Vec3f arrowTarget= Vec3f(pos.x, map->getCell(pos)->getHeight(), pos.y);
			renderArrow(unit->getCurrVectorFlat(), arrowTarget, Vec3f(0.f, 0.f, 1.f), 0.3f);
		}

	}

	//render selection hightlights
	for(int i=0; i < world->getFactionCount(); ++i) {
		for(int j=0; j < world->getFaction(i)->getUnitCount(); ++j) {
			const Unit *unit= world->getFaction(i)->getUnit(j);

			if(unit->isHighlighted()) {
				float highlight= unit->getHightlight();
				if(game->getWorld()->getThisFactionIndex() == unit->getFactionIndex()) {
					glColor4f(0.f, 1.f, 0.f, highlight);
				}
				else{
					glColor4f(1.f, 0.f, 0.f, highlight);
				}

				Vec3f v= unit->getCurrVectorFlat();
				v.y+= 0.3f;
				renderSelectionCircle(v, unit->getType()->getSize(), selectionCircleRadius);
			}
		}
	}

	glPopAttrib();
}

void Renderer::renderWaterEffects(){
	const World *world= game->getWorld();
	const WaterEffects *we= world->getWaterEffects();
	const Map *map= world->getMap();
	const CoreData &coreData= CoreData::getInstance();
	float height= map->getWaterLevel()+0.001f;

	assertGl();

	glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glDepthMask(GL_FALSE);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_COLOR_MATERIAL);

	glNormal3f(0.f, 1.f, 0.f);

	//splashes
	glBindTexture(GL_TEXTURE_2D, static_cast<Texture2DGl*>(coreData.getWaterSplashTexture())->getHandle());
	for(int i=0; i<we->getWaterSplashCount(); ++i){
		const WaterSplash *ws= we->getWaterSplash(i);

		//render only if enabled
		if(ws->getEnabled()){

			//render only if visible
			Vec2i intPos= Vec2i(static_cast<int>(ws->getPos().x), static_cast<int>(ws->getPos().y));
			const Vec2i &mapPos = Map::toSurfCoords(intPos);
			if(map->getSurfaceCell(mapPos)->isVisible(world->getThisTeamIndex())){

				float scale= ws->getAnim()*ws->getSize();

				glColor4f(1.f, 1.f, 1.f, 1.f-ws->getAnim());
				glBegin(GL_TRIANGLE_STRIP);
					glTexCoord2f(0.f, 1.f);
					glVertex3f(ws->getPos().x-scale, height, ws->getPos().y+scale);
					glTexCoord2f(0.f, 0.f);
					glVertex3f(ws->getPos().x-scale, height, ws->getPos().y-scale);
					glTexCoord2f(1.f, 1.f);
					glVertex3f(ws->getPos().x+scale, height, ws->getPos().y+scale);
					glTexCoord2f(1.f, 0.f);
					glVertex3f(ws->getPos().x+scale, height, ws->getPos().y-scale);
				glEnd();
			}
		}
	}

	glPopAttrib();

	assertGl();
}

void Renderer::renderMinimap(){
    const World *world= game->getWorld();
	const Minimap *minimap= world->getMinimap();
	const GameCamera *gameCamera= game->getGameCamera();
	const Pixmap2D *pixmap= minimap->getTexture()->getPixmap();
	const Metrics &metrics= Metrics::getInstance();

	int mx= metrics.getMinimapX();
	int my= metrics.getMinimapY();
	int mw= metrics.getMinimapW();
	int mh= metrics.getMinimapH();

	Vec2f zoom= Vec2f(
		static_cast<float>(mw)/ pixmap->getW(),
		static_cast<float>(mh)/ pixmap->getH());

	assertGl();

    glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_TEXTURE_BIT);

    //draw map
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	glActiveTexture(fowTexUnit);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, static_cast<const Texture2DGl*>(minimap->getFowTexture())->getHandle());
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB, GL_TEXTURE);

	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_ADD);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PRIMARY_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_TEXTURE);

	glActiveTexture(baseTexUnit);
	glBindTexture(GL_TEXTURE_2D, static_cast<const Texture2DGl*>(minimap->getTexture())->getHandle());

	glColor4f(0.5f, 0.5f, 0.5f, 0.1f);
	glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0.0f, 1.0f);
		glMultiTexCoord2f(fowTexUnit, 0.0f, 1.0f);
		glVertex2i(mx, my);
		glTexCoord2f(0.0f, 0.0f);
		glMultiTexCoord2f(fowTexUnit, 0.0f, 0.0f);
		glVertex2i(mx, my+mh);
		glTexCoord2f(1.0f, 1.0f);
		glMultiTexCoord2f(fowTexUnit, 1.0f, 1.0f);
		glVertex2i(mx+mw, my);
		glTexCoord2f(1.0f, 0.0f);
		glMultiTexCoord2f(fowTexUnit, 1.0f, 0.0f);
		glVertex2i(mx+mw, my+mh);
	glEnd();

    glDisable(GL_BLEND);

	glActiveTexture(fowTexUnit);
	glDisable(GL_TEXTURE_2D);
	glActiveTexture(baseTexUnit);
	glDisable(GL_TEXTURE_2D);

	//draw units
	if(useQuadCache == true) {
		VisibleQuadContainerCache &qCache = getQuadCache();
		if(qCache.visibleUnitList.size() > 0) {
			for(int visibleIndex = 0;
					visibleIndex < qCache.visibleUnitList.size(); ++visibleIndex) {
				Unit *unit = qCache.visibleUnitList[visibleIndex];
				if (!unit->isAlive()) {
					continue;
				}

				Vec2i pos= unit->getPos() / Map::cellScale;
				int size= unit->getType()->getSize();
				Vec3f color=  unit->getFaction()->getTexture()->getPixmap()->getPixel3f(0, 0);
				glColor3fv(color.ptr());

				glBegin(GL_QUADS);

				glVertex2f(mx + pos.x*zoom.x, my + mh - (pos.y*zoom.y));
				glVertex2f(mx + (pos.x+1)*zoom.x+size, my + mh - (pos.y*zoom.y));
				glVertex2f(mx + (pos.x+1)*zoom.x+size, my + mh - ((pos.y+size)*zoom.y));
				glVertex2f(mx + pos.x*zoom.x, my + mh - ((pos.y+size)*zoom.y));

				glEnd();
			}
		}
	}
	else {
		glBegin(GL_QUADS);
		for(int i=0; i<world->getFactionCount(); ++i){
			for(int j=0; j<world->getFaction(i)->getUnitCount(); ++j){
				Unit *unit= world->getFaction(i)->getUnit(j);
				if(world->toRenderUnit(unit)){
					if (!unit->isAlive()) {
						continue;
					}
					Vec2i pos= unit->getPos()/Map::cellScale;
					int size= unit->getType()->getSize();
					Vec3f color=  world->getFaction(i)->getTexture()->getPixmap()->getPixel3f(0, 0);
					glColor3fv(color.ptr());
					glVertex2f(mx + pos.x*zoom.x, my + mh - (pos.y*zoom.y));
					glVertex2f(mx + (pos.x+1)*zoom.x+size, my + mh - (pos.y*zoom.y));
					glVertex2f(mx + (pos.x+1)*zoom.x+size, my + mh - ((pos.y+size)*zoom.y));
					glVertex2f(mx + pos.x*zoom.x, my + mh - ((pos.y+size)*zoom.y));
				}
			}
		}
		glEnd();
	}

    //draw camera
	float wRatio= static_cast<float>(metrics.getMinimapW()) / world->getMap()->getW();
	float hRatio= static_cast<float>(metrics.getMinimapH()) / world->getMap()->getH();

    int x= static_cast<int>(gameCamera->getPos().x * wRatio);
    int y= static_cast<int>(gameCamera->getPos().z * hRatio);

    float ang= degToRad(gameCamera->getHAng());

    glEnable(GL_BLEND);

    int x1 = 0;
    int y1 = 0;
#ifdef USE_STREFLOP
    x1 = mx + x + static_cast<int>(20*streflop::sin(ang-pi/5));
	y1 = my + mh - (y-static_cast<int>(20*streflop::cos(ang-pi/5)));
#else
	x1 = mx + x + static_cast<int>(20*sin(ang-pi/5));
	y1 = my + mh - (y-static_cast<int>(20*cos(ang-pi/5)));
#endif

    int x2 = 0;
    int y2 = 0;
#ifdef USE_STREFLOP
    x2 = mx + x + static_cast<int>(20*streflop::sin(ang+pi/5));
	y2 = my + mh - (y-static_cast<int>(20*streflop::cos(ang+pi/5)));
#else
	x2 = mx + x + static_cast<int>(20*sin(ang+pi/5));
	y2 = my + mh - (y-static_cast<int>(20*cos(ang+pi/5)));
#endif

    glColor4f(1.f, 1.f, 1.f, 1.f);
    glBegin(GL_TRIANGLES);

	glVertex2i(mx+x, my+mh-y);
	glColor4f(1.f, 1.f, 1.f, 0.0f);
	glVertex2i(x1,y1);
	glColor4f(1.f, 1.f, 1.f, 0.0f);
    glVertex2i(x2,y2);

    glEnd();
    glPopAttrib();

	assertGl();
}

void Renderer::renderDisplay(){

	CoreData &coreData= CoreData::getInstance();
	const Metrics &metrics= Metrics::getInstance();
	const Display *display= game->getGui()->getDisplay();

	glPushAttrib(GL_ENABLE_BIT);

	//infoString
	renderTextShadow(
		display->getInfoText().c_str(),
		coreData.getDisplayFont(),
		display->getColor(),
		metrics.getDisplayX(),
		metrics.getDisplayY()+Display::infoStringY);

	//title
	renderTextShadow(
		display->getTitle().c_str(),
		coreData.getDisplayFont(),
		display->getColor(),
		metrics.getDisplayX()+40,
		metrics.getDisplayY() + metrics.getDisplayH() - 20);

    glColor3f(0.0f, 0.0f, 0.0f);

	//text
	renderTextShadow(
		display->getText().c_str(),
		coreData.getDisplayFont(),
		display->getColor(),
		metrics.getDisplayX() -1,
		metrics.getDisplayY() + metrics.getDisplayH() - 56);

	//progress Bar
	if(display->getProgressBar()!=-1){
		renderProgressBar(
			display->getProgressBar(),
			metrics.getDisplayX(),
			metrics.getDisplayY() + metrics.getDisplayH()-50,
			coreData.getDisplayFontSmall());
	}

	//up images
	glEnable(GL_TEXTURE_2D);

	glColor3f(1.f, 1.f, 1.f);
	for(int i=0; i<Display::upCellCount; ++i){
		if(display->getUpImage(i)!=NULL){
			renderQuad(
				metrics.getDisplayX()+display->computeUpX(i),
				metrics.getDisplayY()+display->computeUpY(i),
				Display::imageSize, Display::imageSize, display->getUpImage(i));
		}
	}

 	//down images
	for(int i=0; i<Display::downCellCount; ++i){
		if(display->getDownImage(i)!=NULL){
			if(display->getDownLighted(i)){
                glColor3f(1.f, 1.f, 1.f);
			}
			else{
                glColor3f(0.3f, 0.3f, 0.3f);
			}

			int x= metrics.getDisplayX()+display->computeDownX(i);
			int y= metrics.getDisplayY()+display->computeDownY(i);
			int size= Display::imageSize;

			if(display->getDownSelectedPos()==i){
				x-= 3;
				y-= 3;
				size+= 6;
			}

			renderQuad(x, y, size, size, display->getDownImage(i));
		}
	}

	//selection
	int downPos= display->getDownSelectedPos();
	if(downPos!=Display::invalidPos){
		const Texture2D *texture= display->getDownImage(downPos);
		if(texture!=NULL){
			int x= metrics.getDisplayX()+display->computeDownX(downPos)-3;
			int y= metrics.getDisplayY()+display->computeDownY(downPos)-3;
			int size= Display::imageSize+6;
			renderQuad(x, y, size, size, display->getDownImage(downPos));
		}
    }

	glPopAttrib();
}

void Renderer::renderMenuBackground(const MenuBackground *menuBackground){

	assertGl();

	const Vec3f &cameraPosition= menuBackground->getCamera()->getConstPosition();

	glPushAttrib(GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT);

	//clear
	Vec4f fogColor= Vec4f(0.4f, 0.4f, 0.4f, 1.f) * menuBackground->getFade();
	glClearColor(fogColor.x, fogColor.y, fogColor.z, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glFogfv(GL_FOG_COLOR, fogColor.ptr());

	//light
	Vec4f lightPos= Vec4f(10.f, 10.f, 10.f, 1.f)* menuBackground->getFade();
	Vec4f diffLight= Vec4f(0.9f, 0.9f, 0.9f, 1.f)* menuBackground->getFade();
	Vec4f ambLight= Vec4f(0.3f, 0.3f, 0.3f, 1.f)* menuBackground->getFade();
	Vec4f specLight= Vec4f(0.1f, 0.1f, 0.1f, 1.f)* menuBackground->getFade();

	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_POSITION, lightPos.ptr());
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffLight.ptr());
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambLight.ptr());
	glLightfv(GL_LIGHT0, GL_SPECULAR, specLight.ptr());

	//main model
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.5f);
	modelRenderer->begin(true, true, true);
	modelRenderer->render(menuBackground->getMainModel());
	modelRenderer->end();
	glDisable(GL_ALPHA_TEST);

	//characters
	float dist= menuBackground->getAboutPosition().dist(cameraPosition);
	float minDist= 3.f;
	if(dist < minDist) {

		glAlphaFunc(GL_GREATER, 0.0f);
		float alpha= clamp((minDist-dist) / minDist, 0.f, 1.f);
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, Vec4f(1.0f, 1.0f, 1.0f, alpha).ptr());
		modelRenderer->begin(true, true, false);

		for(int i=0; i < MenuBackground::characterCount; ++i) {
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glLoadIdentity();
			glTranslatef(i*2.f-4.f, -1.4f, -7.5f);
			menuBackground->getCharacterModel(i)->updateInterpolationData(menuBackground->getAnim(), true);
			modelRenderer->render(menuBackground->getCharacterModel(i));
			glPopMatrix();
		}
		modelRenderer->end();
	}

	//water
	if(menuBackground->getWater()) {

		//water surface
		const int waterTesselation= 10;
		const int waterSize= 250;
		const int waterQuadSize= 2*waterSize/waterTesselation;
		const float waterHeight= menuBackground->getWaterHeight();

		glEnable(GL_BLEND);

		glNormal3f(0.f, 1.f, 0.f);
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, Vec4f(1.f, 1.f, 1.f, 1.f).ptr());
		GLuint waterHandle= static_cast<Texture2DGl*>(menuBackground->getWaterTexture())->getHandle();
		glBindTexture(GL_TEXTURE_2D, waterHandle);
		for(int i=1; i < waterTesselation; ++i) {
			glBegin(GL_TRIANGLE_STRIP);
			for(int j=1; j < waterTesselation; ++j) {
				glTexCoord2i(1, 2 % j);
				glVertex3f(-waterSize+i*waterQuadSize, waterHeight, -waterSize+j*waterQuadSize);
				glTexCoord2i(0, 2 % j);
				glVertex3f(-waterSize+(i+1)*waterQuadSize, waterHeight, -waterSize+j*waterQuadSize);
			}
			glEnd();
		}
		glDisable(GL_BLEND);

		//raindrops
		if(menuBackground->getRain()) {
			const float maxRaindropAlpha= 0.5f;

			glEnable(GL_BLEND);
			glDisable(GL_LIGHTING);
			glDisable(GL_ALPHA_TEST);
			glDepthMask(GL_FALSE);

			//splashes
			CoreData &coreData= CoreData::getInstance();
			glBindTexture(GL_TEXTURE_2D, static_cast<Texture2DGl*>(coreData.getWaterSplashTexture())->getHandle());
			for(int i=0; i< MenuBackground::raindropCount; ++i) {

				Vec2f pos= menuBackground->getRaindropPos(i);
				float scale= menuBackground->getRaindropState(i);
				float alpha= maxRaindropAlpha-scale*maxRaindropAlpha;

				glMatrixMode(GL_MODELVIEW);
				glPushMatrix();

				glColor4f(1.f, 1.f, 1.f, alpha);
				glTranslatef(pos.x, waterHeight+0.01f, pos.y);

				glBegin(GL_TRIANGLE_STRIP);
					glTexCoord2f(0.f, 1.f);
					glVertex3f(-scale, 0, scale);
					glTexCoord2f(0.f, 0.f);
					glVertex3f(-scale, 0, -scale);
					glTexCoord2f(1.f, 1.f);
					glVertex3f(scale, 0, scale);
					glTexCoord2f(1.f, 0.f);
					glVertex3f(scale, 0, -scale);
				glEnd();

				glPopMatrix();
			}
		}
	}

	glPopAttrib();

	assertGl();
}

// ==================== computing ====================

bool Renderer::computePosition(const Vec2i &screenPos, Vec2i &worldPos){

	assertGl();
	const Map* map= game->getWorld()->getMap();
	const Metrics &metrics= Metrics::getInstance();
	float depth= 0.0f;
	GLdouble modelviewMatrix[16];
	GLdouble projectionMatrix[16];
	GLint viewport[4]= {0, 0, metrics.getScreenW(), metrics.getScreenH()};
	GLdouble worldX;
	GLdouble worldY;
	GLdouble worldZ;
	GLint screenX= (screenPos.x * metrics.getScreenW() / metrics.getVirtualW());
	GLint screenY= (screenPos.y * metrics.getScreenH() / metrics.getVirtualH());

	//get the depth in the cursor pixel
	glReadPixels(screenX, screenY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);

	//load matrices
	loadProjectionMatrix();
    loadGameCameraMatrix();

	//get matrices
	glGetDoublev(GL_MODELVIEW_MATRIX, modelviewMatrix);
	glGetDoublev(GL_PROJECTION_MATRIX, projectionMatrix);

	//get the world coordinates
	gluUnProject(
		screenX, screenY, depth,
		modelviewMatrix, projectionMatrix, viewport,
		&worldX, &worldY, &worldZ);

	//conver coords to int
	worldPos= Vec2i(static_cast<int>(worldX+0.5f), static_cast<int>(worldZ+0.5f));

	//clamp coords to map size
	return map->isInside(worldPos);
}

// This method takes world co-ordinates and translates them to screen co-ords
Vec3f Renderer::computeScreenPosition(const Vec3f &worldPos) {
	assertGl();

	const Metrics &metrics= Metrics::getInstance();
	GLint viewport[]= {0, 0, metrics.getVirtualW(), metrics.getVirtualH()};
	GLdouble worldX = worldPos.x;
	GLdouble worldY = worldPos.y;
	GLdouble worldZ = worldPos.z;

	//load matrices
	loadProjectionMatrix();
    loadGameCameraMatrix();

	//get matrices
	GLdouble modelviewMatrix[16];
	glGetDoublev(GL_MODELVIEW_MATRIX, modelviewMatrix);
	GLdouble projectionMatrix[16];
	glGetDoublev(GL_PROJECTION_MATRIX, projectionMatrix);

	//get the screen coordinates
	GLdouble screenX;
	GLdouble screenY;
	GLdouble screenZ;
	gluProject(worldX, worldY, worldZ,
		modelviewMatrix, projectionMatrix, viewport,
		&screenX, &screenY, &screenZ);

	Vec3f screenPos(screenX,screenY,screenZ);
	return screenPos;
}

void Renderer::computeSelected(	Selection::UnitContainer &units,
								const Vec2i &posDown, const Vec2i &posUp) {

	//declarations
	GLuint selectBuffer[Gui::maxSelBuff];
	const Metrics &metrics= Metrics::getInstance();

	//compute center and dimensions of selection rectangle
	int x= (posDown.x+posUp.x) / 2;
	int y= (posDown.y+posUp.y) / 2;
	int w= abs(posDown.x-posUp.x);
	int h= abs(posDown.y-posUp.y);
	if(w<1) w=1;
	if(h<1) h=1;

	//setup matrices
	glSelectBuffer(Gui::maxSelBuff, selectBuffer);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	GLint view[]= {0, 0, metrics.getVirtualW(), metrics.getVirtualH()};
	glRenderMode(GL_SELECT);
	glLoadIdentity();
	gluPickMatrix(x, y, w, h, view);
	gluPerspective(perspFov, metrics.getAspectRatio(), perspNearPlane, perspFarPlane);
	loadGameCameraMatrix();

	//render units to find which ones should be selected
	renderUnitsFast();

	//pop matrices
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	//select units by checking the selected buffer
	int selCount= glRenderMode(GL_RENDER);
	if(selCount > 0) {
		VisibleQuadContainerCache &qCache = getQuadCache();
		for(int i=1; i <= selCount; ++i) {
			int visibleUnitIndex= selectBuffer[i*4-1];
			Unit *unit = qCache.visibleQuadUnitList[visibleUnitIndex];
			if(unit != NULL && unit->isAlive()) {
				units.push_back(unit);
			}
		}
	}
}

// ==================== shadows ====================

void Renderer::renderShadowsToTexture(const int renderFps){
	if(lastRenderFps >= MIN_FPS_NORMAL_RENDERING && renderFps >= MIN_FPS_NORMAL_RENDERING_TOP_THRESHOLD &&
		(shadows == sProjected || shadows == sShadowMapping)) {

		shadowMapFrame= (shadowMapFrame + 1) % (shadowFrameSkip + 1);

		if(shadowMapFrame == 0){
			assertGl();

			glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_VIEWPORT_BIT | GL_POLYGON_BIT);

			if(shadows==sShadowMapping){
				glClear(GL_DEPTH_BUFFER_BIT);
			}
			else{
				float color= 1.0f-shadowAlpha;
				glColor3f(color, color, color);
				glClearColor(1.f, 1.f, 1.f, 1.f);
				glDisable(GL_DEPTH_TEST);
				glClear(GL_COLOR_BUFFER_BIT);
			}

			//clear color buffer
			//
			//set viewport, we leave one texel always in white to avoid problems
			glViewport(1, 1, shadowTextureSize-2, shadowTextureSize-2);

			if(nearestLightPos.w==0.f){
				//directional light

				//light pos
				assert(game != NULL);
				assert(game->getWorld() != NULL);
				const TimeFlow *tf= game->getWorld()->getTimeFlow();
				assert(tf != NULL);
				float ang= tf->isDay()? computeSunAngle(tf->getTime()): computeMoonAngle(tf->getTime());
				ang= radToDeg(ang);

				//push and set projection
				glMatrixMode(GL_PROJECTION);
				glPushMatrix();
				glLoadIdentity();
				if(game->getGameCamera()->getState()==GameCamera::sGame){
					glOrtho(-35, 5, -15, 15, -1000, 1000);
				}
				else{
					glOrtho(-30, 30, -20, 20, -1000, 1000);
				}

				//push and set modelview
				glMatrixMode(GL_MODELVIEW);
				glPushMatrix();
				glLoadIdentity();

				glRotatef(15, 0, 1, 0);

				glRotatef(ang, 1, 0, 0);
				glRotatef(90, 0, 1, 0);
				const Vec3f &pos= game->getGameCamera()->getPos();

				glTranslatef(static_cast<int>(-pos.x), 0, static_cast<int>(-pos.z));

			}
			else{
				//non directional light

				//push projection
				glMatrixMode(GL_PROJECTION);
				glPushMatrix();
				glLoadIdentity();
				gluPerspective(150, 1.f, perspNearPlane, perspFarPlane);

				//push modelview
				glMatrixMode(GL_MODELVIEW);
				glPushMatrix();
				glLoadIdentity();
				glRotatef(-90, -1, 0, 0);
				glTranslatef(-nearestLightPos.x, -nearestLightPos.y-2, -nearestLightPos.z);
			}

			if(shadows == sShadowMapping) {
				glEnable(GL_POLYGON_OFFSET_FILL);
				glPolygonOffset(1.0f, 0.001f);
			}

			//render 3d
			renderUnitsFast(true);
			renderObjectsFast();

			//read color buffer
			glBindTexture(GL_TEXTURE_2D, shadowMapHandle);
			glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, shadowTextureSize, shadowTextureSize);

			//get elemental matrices
			static Matrix4f matrix1;
			static bool matrix1Populate = true;
			if(matrix1Populate == true) {
				matrix1Populate = false;
				matrix1[0]= 0.5f;	matrix1[4]= 0.f;	matrix1[8]= 0.f;	matrix1[12]= 0.5f;
				matrix1[1]= 0.f;	matrix1[5]= 0.5f;	matrix1[9]= 0.f;	matrix1[13]= 0.5f;
				matrix1[2]= 0.f;	matrix1[6]= 0.f;	matrix1[10]= 0.5f;	matrix1[14]= 0.5f;
				matrix1[3]= 0.f;	matrix1[7]= 0.f;	matrix1[11]= 0.f;	matrix1[15]= 1.f;
			}
			Matrix4f matrix2;
			glGetFloatv(GL_PROJECTION_MATRIX, matrix2.ptr());

			Matrix4f matrix3;
			glGetFloatv(GL_MODELVIEW_MATRIX, matrix3.ptr());

			//pop both matrices
			glPopMatrix();
			glMatrixMode(GL_PROJECTION);
			glPopMatrix();

			glMatrixMode(GL_PROJECTION);
			glPushMatrix();

			//compute texture matrix
			glLoadMatrixf(matrix1.ptr());
			glMultMatrixf(matrix2.ptr());
			glMultMatrixf(matrix3.ptr());
			glGetFloatv(GL_TRANSPOSE_PROJECTION_MATRIX_ARB, shadowMapMatrix.ptr());

			//pop
			glPopMatrix();

			glPopAttrib();

			assertGl();
		}
	}
}


// ==================== gl wrap ====================

string Renderer::getGlInfo(){
	string infoStr;
	Lang &lang= Lang::getInstance();

	infoStr+= lang.get("OpenGlInfo")+":\n";
	infoStr+= "   "+lang.get("OpenGlVersion")+": ";
    infoStr+= string((getGlVersion() != NULL ? getGlVersion() : "?"))+"\n";
    infoStr+= "   "+lang.get("OpenGlRenderer")+": ";
    infoStr+= string((getGlVersion() != NULL ? getGlVersion() : "?"))+"\n";
    infoStr+= "   "+lang.get("OpenGlVendor")+": ";
    infoStr+= string((getGlVendor() != NULL ? getGlVendor() : "?"))+"\n";
	infoStr+= "   "+lang.get("OpenGlMaxLights")+": ";
    infoStr+= intToStr(getGlMaxLights())+"\n";
	infoStr+= "   "+lang.get("OpenGlMaxTextureSize")+": ";
    infoStr+= intToStr(getGlMaxTextureSize())+"\n";
	infoStr+= "   "+lang.get("OpenGlMaxTextureUnits")+": ";
    infoStr+= intToStr(getGlMaxTextureUnits())+"\n";
	infoStr+= "   "+lang.get("OpenGlModelviewStack")+": ";
    infoStr+= intToStr(getGlModelviewMatrixStackDepth())+"\n";
	infoStr+= "   "+lang.get("OpenGlProjectionStack")+": ";
    infoStr+= intToStr(getGlProjectionMatrixStackDepth())+"\n";

	return infoStr;
}

string Renderer::getGlMoreInfo(){
	string infoStr;
	Lang &lang= Lang::getInstance();

	//gl extensions
	infoStr+= lang.get("OpenGlExtensions")+":\n   ";

	string extensions= getGlExtensions();
	int charCount= 0;
	for(int i=0; i<extensions.size(); ++i){
		infoStr+= extensions[i];
		if(charCount>120 && extensions[i]==' '){
			infoStr+= "\n   ";
			charCount= 0;
		}
		++charCount;
	}

	//platform extensions
	infoStr+= "\n\n";
	infoStr+= lang.get("OpenGlPlatformExtensions")+":\n   ";

	charCount= 0;
	string platformExtensions= getGlPlatformExtensions();
	for(int i=0; i<platformExtensions.size(); ++i){
		infoStr+= platformExtensions[i];
		if(charCount>120 && platformExtensions[i]==' '){
			infoStr+= "\n   ";
			charCount= 0;
		}
		++charCount;
	}

	return infoStr;
}

void Renderer::autoConfig(){

	Config &config= Config::getInstance();
	bool nvidiaCard= toLower(getGlVendor()).find("nvidia")!=string::npos;
	bool atiCard= toLower(getGlVendor()).find("ati")!=string::npos;
	bool shadowExtensions = isGlExtensionSupported("GL_ARB_shadow") && isGlExtensionSupported("GL_ARB_shadow_ambient");

	//3D textures
	config.setBool("Textures3D", isGlExtensionSupported("GL_EXT_texture3D"));

	//shadows
	string shadows;
	if(getGlMaxTextureUnits()>=3){
		if(nvidiaCard && shadowExtensions){
			shadows= shadowsToStr(sShadowMapping);
		}
		else{
			shadows= shadowsToStr(sProjected);
		}
	}
	else{
		shadows=shadowsToStr(sDisabled);
	}
	config.setString("Shadows", shadows);

	//lights
	config.setInt("MaxLights", atiCard? 1: 4);

	//filter
	config.setString("Filter", "Bilinear");
}

void Renderer::clearBuffers(){
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::clearZBuffer(){
	glClear(GL_DEPTH_BUFFER_BIT);
}

void Renderer::loadConfig(){
	Config &config= Config::getInstance();

	//cache most used config params
	maxLights= config.getInt("MaxLights");
	photoMode= config.getBool("PhotoMode");
	focusArrows= config.getBool("FocusArrows");
	textures3D= config.getBool("Textures3D");

	//load shadows
	shadows= strToShadows(config.getString("Shadows"));
	if(shadows==sProjected || shadows==sShadowMapping){
		shadowTextureSize= config.getInt("ShadowTextureSize");
		shadowFrameSkip= config.getInt("ShadowFrameSkip");
		shadowAlpha= config.getFloat("ShadowAlpha");
	}

	//load filter settings
	Texture2D::Filter textureFilter= strToTextureFilter(config.getString("Filter"));
	int maxAnisotropy= config.getInt("FilterMaxAnisotropy");
	for(int i=0; i<rsCount; ++i){
		textureManager[i]->setFilter(textureFilter);
		textureManager[i]->setMaxAnisotropy(maxAnisotropy);
	}
}

void Renderer::saveScreen(const string &path){

	const Metrics &sm= Metrics::getInstance();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//Pixmap2D pixmap(sm.getScreenW(), sm.getScreenH(), 3);
	if( pixmapScreenShot == NULL ||
		pixmapScreenShot->getW() != sm.getScreenW() ||
		pixmapScreenShot->getH() != sm.getScreenH()) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		delete pixmapScreenShot;
		pixmapScreenShot = new Pixmap2D(sm.getScreenW(), sm.getScreenH(), 3);
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	glFinish();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	glReadPixels(0, 0, pixmapScreenShot->getW(), pixmapScreenShot->getH(),
				 GL_RGB, GL_UNSIGNED_BYTE, pixmapScreenShot->getPixels());

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	pixmapScreenShot->saveTga(path);

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

// ==================== PRIVATE ====================

float Renderer::computeSunAngle(float time) {

	float dayTime= TimeFlow::dusk-TimeFlow::dawn;
	float fTime= (time-TimeFlow::dawn)/dayTime;
	return clamp(fTime*pi, pi/8.f, 7.f*pi/8.f);
}

float Renderer::computeMoonAngle(float time) {
	float nightTime= 24-(TimeFlow::dusk-TimeFlow::dawn);

	if(time<TimeFlow::dawn){
		time+= 24.f;
	}

	float fTime= (time-TimeFlow::dusk)/nightTime;
	return clamp((1.0f-fTime)*pi, pi/8.f, 7.f*pi/8.f);
}

Vec4f Renderer::computeSunPos(float time) {
	float ang= computeSunAngle(time);
#ifdef USE_STREFLOP
	return Vec4f(-streflop::cos(ang)*sunDist, streflop::sin(ang)*sunDist, 0.f, 0.f);
#else
	return Vec4f(-cos(ang)*sunDist, sin(ang)*sunDist, 0.f, 0.f);
#endif
}

Vec4f Renderer::computeMoonPos(float time) {
	float ang= computeMoonAngle(time);
#ifdef USE_STREFLOP
	return Vec4f(-streflop::cos(ang)*moonDist, streflop::sin(ang)*moonDist, 0.f, 0.f);
#else
	return Vec4f(-cos(ang)*moonDist, sin(ang)*moonDist, 0.f, 0.f);
#endif
}

Vec3f Renderer::computeLightColor(float time) {
	const Tileset *tileset= game->getWorld()->getTileset();
	Vec3f color;

	const float transition= 2;
	const float dayStart= TimeFlow::dawn;
	const float dayEnd= TimeFlow::dusk-transition;
	const float nightStart= TimeFlow::dusk;
	const float nightEnd= TimeFlow::dawn-transition;

	if(time>dayStart && time<dayEnd) {
		color= tileset->getSunLightColor();
	}
	else if(time>nightStart || time<nightEnd) {
		color= tileset->getMoonLightColor();
	}
	else if(time>=dayEnd && time<=nightStart) {
		color= tileset->getSunLightColor().lerp((time-dayEnd)/transition, tileset->getMoonLightColor());
	}
	else if(time>=nightEnd && time<=dayStart) {
		color= tileset->getMoonLightColor().lerp((time-nightEnd)/transition, tileset->getSunLightColor());
	}
	else {
		assert(false);
		color= tileset->getSunLightColor();
	}
	return color;
}

Vec4f Renderer::computeWaterColor(float waterLevel, float cellHeight) {
	const float waterFactor= 1.5f;
	return Vec4f(1.f, 1.f, 1.f, clamp((waterLevel-cellHeight)*waterFactor, 0.f, 1.f));
}

// ==================== fast render ====================

//render units for selection purposes
void Renderer::renderUnitsFast(bool renderingShadows) {
	assert(game != NULL);
	const World *world= game->getWorld();
	assert(world != NULL);

	assertGl();

	bool modelRenderStarted = false;
	//bool modelRenderFactionStarted = false;

	if(useQuadCache == true) {
		VisibleQuadContainerCache &qCache = getQuadCache();
		if(qCache.visibleQuadUnitList.size() > 0) {
			for(int visibleUnitIndex = 0;
					visibleUnitIndex < qCache.visibleQuadUnitList.size(); ++visibleUnitIndex) {
				Unit *unit = qCache.visibleQuadUnitList[visibleUnitIndex];

				if(modelRenderStarted == false) {
					modelRenderStarted = true;
					//glPushAttrib(GL_ENABLE_BIT| GL_TEXTURE_BIT);
					glDisable(GL_LIGHTING);
					if (!renderingShadows) {
						glPushAttrib(GL_ENABLE_BIT);
						glDisable(GL_TEXTURE_2D);
					} else {
						glPushAttrib(GL_ENABLE_BIT| GL_TEXTURE_BIT);
						glEnable(GL_TEXTURE_2D);
						glAlphaFunc(GL_GREATER, 0.4f);
				
						glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
				
						//set color to the texture alpha
						glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
						glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PRIMARY_COLOR);
						glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
				
						//set alpha to the texture alpha
						glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
						glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE);
						glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
					}
				
					modelRenderer->begin(false, renderingShadows, false);
					glInitNames();
				}

				glPushName(visibleUnitIndex);

				glMatrixMode(GL_MODELVIEW);
				//debuxar modelo
				glPushMatrix();

				//translate
				Vec3f currVec= unit->getCurrVectorFlat();
				glTranslatef(currVec.x, currVec.y, currVec.z);

				//rotate
				glRotatef(unit->getRotation(), 0.f, 1.f, 0.f);

				//render
				const Model *model= unit->getCurrentModel();
				model->updateInterpolationVertices(unit->getAnimProgress(), unit->isAlive());
				modelRenderer->render(model);

				glPopMatrix();
				//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] calling glPopName() for unit->getId() = %d\n",__FILE__,__FUNCTION__,__LINE__,unit->getId());
				glPopName();
			}

			//if(modelRenderFactionStarted == true) {
				//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] calling glPopName() for lastFactionIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,lastFactionIndex);
				//glPopName();
			//}

			if(modelRenderStarted == true) {
				modelRenderer->end();
				glPopAttrib();
			}
		}
	}
	else {
		bool modelRenderStarted = false;
		bool modelRenderFactionStarted = false;
		for(int i=0; i<world->getFactionCount(); ++i){
			modelRenderFactionStarted = false;
			for(int j=0; j<world->getFaction(i)->getUnitCount(); ++j){
				Unit *unit= world->getFaction(i)->getUnit(j);
				if(world->toRenderUnit(unit, visibleQuad)) {
					if(modelRenderStarted == false) {
						modelRenderStarted = true;
						glPushAttrib(GL_ENABLE_BIT| GL_TEXTURE_BIT);
						glDisable(GL_LIGHTING);
						
						if (!renderingShadows) {
							glDisable(GL_TEXTURE_2D);
						} else {
							glEnable(GL_TEXTURE_2D);
							glAlphaFunc(GL_GREATER, 0.4f);
					
							glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
					
							//set color to the texture alpha
							glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
							glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PRIMARY_COLOR);
							glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
					
							//set alpha to the texture alpha
							glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
							glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE);
							glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
						}
					
						modelRenderer->begin(false, renderingShadows, false);

						glInitNames();
					}
					if(modelRenderFactionStarted == false) {
						modelRenderFactionStarted = true;
						glPushName(i);
					}
					glPushName(j);

					glMatrixMode(GL_MODELVIEW);

					//debuxar modelo
					glPushMatrix();

					//translate
					Vec3f currVec= unit->getCurrVectorFlat();
					glTranslatef(currVec.x, currVec.y, currVec.z);

					//rotate
					glRotatef(unit->getRotation(), 0.f, 1.f, 0.f);

					//render
					const Model *model= unit->getCurrentModel();
					model->updateInterpolationVertices(unit->getAnimProgress(), unit->isAlive());
					modelRenderer->render(model);

					glPopMatrix();
					glPopName();
				}
			}
			if(modelRenderFactionStarted == true) {
				glPopName();
			}
		}
		if(modelRenderStarted == true) {
			modelRenderer->end();
			glPopAttrib();
		}
	}

	assertGl();
}

//render objects for selection purposes
void Renderer::renderObjectsFast() {
	const World *world= game->getWorld();
	const Map *map= world->getMap();

    assertGl();

	bool modelRenderStarted = false;

	if(useQuadCache == true) {
		VisibleQuadContainerCache &qCache = getQuadCache();
		if(qCache.visibleObjectList.size() > 0) {
			for(int visibleIndex = 0;
					visibleIndex < qCache.visibleObjectList.size(); ++visibleIndex) {
				Object *o = qCache.visibleObjectList[visibleIndex];

				const Model *objModel= o->getModel();
				const Vec3f &v= o->getConstPos();

				if(modelRenderStarted == false) {
					modelRenderStarted = true;
					glPushAttrib(GL_ENABLE_BIT| GL_TEXTURE_BIT);
					glDisable(GL_LIGHTING);

					glAlphaFunc(GL_GREATER, 0.5f);

					glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

					//set color to the texture alpha
					glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
					glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PRIMARY_COLOR);
					glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);

					//set alpha to the texture alpha
					glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
					glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE);
					glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);

					modelRenderer->begin(false, true, false);
				}
				glMatrixMode(GL_MODELVIEW);
				glPushMatrix();
				glTranslatef(v.x, v.y, v.z);
				glRotatef(o->getRotation(), 0.f, 1.f, 0.f);

				modelRenderer->render(objModel);

				glPopMatrix();

			}

			if(modelRenderStarted == true) {
				modelRenderer->end();
				glPopAttrib();
			}
		}
	}
	else {
		int thisTeamIndex= world->getThisTeamIndex();
		bool modelRenderStarted = false;

		PosQuadIterator pqi(visibleQuad, Map::cellScale);
		while(pqi.next()) {
			const Vec2i &pos= pqi.getPos();
			if(map->isInside(pos)){
				const Vec2i &mapPos = Map::toSurfCoords(pos);
				SurfaceCell *sc= map->getSurfaceCell(mapPos);
				Object *o= sc->getObject();
				bool isExplored = (sc->isExplored(thisTeamIndex) && o!=NULL);
				//bool isVisible = (sc->isVisible(thisTeamIndex) && o!=NULL);
				bool isVisible = true;

				if(isExplored == true && isVisible == true) {
					const Model *objModel= sc->getObject()->getModel();
					const Vec3f &v= o->getConstPos();

					if(modelRenderStarted == false) {
						modelRenderStarted = true;
						glPushAttrib(GL_ENABLE_BIT| GL_TEXTURE_BIT);
						glDisable(GL_LIGHTING);

						glAlphaFunc(GL_GREATER, 0.5f);

						glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

						//set color to the texture alpha
						glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
						glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PRIMARY_COLOR);
						glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);

						//set alpha to the texture alpha
						glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
						glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE);
						glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);

						modelRenderer->begin(false, true, false);
					}
					glMatrixMode(GL_MODELVIEW);
					glPushMatrix();
					glTranslatef(v.x, v.y, v.z);
					glRotatef(o->getRotation(), 0.f, 1.f, 0.f);

					modelRenderer->render(objModel);

					glPopMatrix();

	//				vctEntity.push_back(RenderEntity(o,mapPos));
				}
			}
		}

	//	modelRenderer->begin(false, true, false);
	//	renderObjectFastList(vctEntity);
		if(modelRenderStarted == true) {
			modelRenderer->end();
			glPopAttrib();
		}
	}

	assertGl();
}

// ==================== gl caps ====================

void Renderer::checkGlCaps() {

	//opengl 1.3
	if(!isGlVersionSupported(1, 3, 0)) {
		string message;

		message += "Your system supports OpenGL version \"";
 		message += getGlVersion() + string("\"\n");
 		message += "Glest needs at least version 1.3 to work\n";
 		message += "You may solve this problem by installing your latest video card drivers";

 		throw runtime_error(message.c_str());
	}

	//opengl 1.4 or extension
	if(!isGlVersionSupported(1, 4, 0)){
		checkExtension("GL_ARB_texture_env_crossbar", "Glest");
	}
}

void Renderer::checkGlOptionalCaps() {

	//shadows
	if(shadows == sProjected || shadows == sShadowMapping) {
		if(getGlMaxTextureUnits() < 3) {
			throw runtime_error("Your system doesn't support 3 texture units, required for shadows");
		}
	}

	//shadow mapping
	if(shadows == sShadowMapping) {
		checkExtension("GL_ARB_shadow", "Shadow Mapping");
		checkExtension("GL_ARB_shadow_ambient", "Shadow Mapping");
	}
}

void Renderer::checkExtension(const string &extension, const string &msg) {
	if(!isGlExtensionSupported(extension.c_str())) {
		string str= "OpenGL extension not supported: " + extension +  ", required for " + msg;
		throw runtime_error(str);
	}
}

// ==================== init 3d lists ====================

void Renderer::init3dList() {

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	const Metrics &metrics= Metrics::getInstance();

    assertGl();

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	list3d= glGenLists(1);

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	glNewList(list3d, GL_COMPILE_AND_EXECUTE);
	//need to execute, because if not gluPerspective takes no effect and gluLoadMatrix is wrong

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		//misc
		glViewport(0, 0, metrics.getScreenW(), metrics.getScreenH());
		glClearColor(fowColor.x, fowColor.y, fowColor.z, fowColor.w);
		glFrontFace(GL_CW);
		glEnable(GL_CULL_FACE);
		loadProjectionMatrix();

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		//texture state
		glActiveTexture(shadowTexUnit);
		glDisable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		glActiveTexture(fowTexUnit);
		glDisable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		glActiveTexture(baseTexUnit);
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		//material state
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, defSpecularColor.ptr());
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, defAmbientColor.ptr());
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, defDiffuseColor.ptr());
		glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
		glColor4fv(defColor.ptr());

		//blend state
		glDisable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		//alpha test state
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.f);

		//depth test state
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
		glDepthFunc(GL_LESS);

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		//lighting state
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);

		//matrix mode
		glMatrixMode(GL_MODELVIEW);

		//stencil test
		glDisable(GL_STENCIL_TEST);

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		//fog
		const Tileset *tileset= NULL;
		if(game != NULL && game->getWorld() != NULL) {
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			tileset = game->getWorld()->getTileset();
		}

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		if(tileset != NULL && tileset->getFog()) {

			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			glEnable(GL_FOG);
			if(tileset->getFogMode()==fmExp) {
				glFogi(GL_FOG_MODE, GL_EXP);
			}
			else {
				glFogi(GL_FOG_MODE, GL_EXP2);
			}

			glFogf(GL_FOG_DENSITY, tileset->getFogDensity());
			glFogfv(GL_FOG_COLOR, tileset->getFogColor().ptr());
		}

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	glEndList();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//assert
	assertGl();

}

void Renderer::init2dList() {

	const Metrics &metrics= Metrics::getInstance();

	//this list sets the state for the 2d rendering
	list2d= glGenLists(1);
	glNewList(list2d, GL_COMPILE);

		//projection
		glViewport(0, 0, metrics.getScreenW(), metrics.getScreenH());
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, metrics.getVirtualW(), 0, metrics.getVirtualH(), 0, 1);

		//modelview
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		//disable everything
		glDisable(GL_BLEND);
		glDisable(GL_LIGHTING);
		glDisable(GL_ALPHA_TEST);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_STENCIL_TEST);
		glDisable(GL_FOG);
		glDisable(GL_CULL_FACE);
		glFrontFace(GL_CCW);
		glActiveTexture(baseTexUnit);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glDisable(GL_TEXTURE_2D);

		//blend func
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		//color
		glColor4f(1.f, 1.f, 1.f, 1.f);

	glEndList();

	assertGl();
}

void Renderer::init3dListMenu(const MainMenu *mm) {
    assertGl();

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	const Metrics &metrics= Metrics::getInstance();
	const MenuBackground *mb= mm->getConstMenuBackground();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	list3dMenu= glGenLists(1);

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	glNewList(list3dMenu, GL_COMPILE);

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		//misc
		glViewport(0, 0, metrics.getScreenW(), metrics.getScreenH());
		glClearColor(0.4f, 0.4f, 0.4f, 1.f);
		glFrontFace(GL_CW);
		glEnable(GL_CULL_FACE);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(perspFov, metrics.getAspectRatio(), perspNearPlane, 1000);

		//texture state
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		//material state
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, defSpecularColor.ptr());
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, defAmbientColor.ptr());
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, defDiffuseColor.ptr());
		glColor4fv(defColor.ptr());
		glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);

		//blend state
		glDisable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		//alpha test state
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.f);

		//depth test state
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
		glDepthFunc(GL_LESS);

		//lighting state
		glEnable(GL_LIGHTING);

		//matrix mode
		glMatrixMode(GL_MODELVIEW);

		//stencil test
		glDisable(GL_STENCIL_TEST);

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		//fog
		if(mb != NULL && mb->getFog()){
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			glEnable(GL_FOG);
			glFogi(GL_FOG_MODE, GL_EXP2);
			glFogf(GL_FOG_DENSITY, mb->getFogDensity());
		}

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	glEndList();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//assert
	assertGl();
}


// ==================== misc ====================

void Renderer::loadProjectionMatrix() {
	GLdouble clipping;
	const Metrics &metrics= Metrics::getInstance();

    assertGl();

	clipping= photoMode ? perspFarPlane*100 : perspFarPlane;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(perspFov, metrics.getAspectRatio(), perspNearPlane, clipping);

    assertGl();
}

void Renderer::enableProjectiveTexturing() {
	glTexGenfv(GL_S, GL_EYE_PLANE, &shadowMapMatrix[0]);
	glTexGenfv(GL_T, GL_EYE_PLANE, &shadowMapMatrix[4]);
	glTexGenfv(GL_R, GL_EYE_PLANE, &shadowMapMatrix[8]);
	glTexGenfv(GL_Q, GL_EYE_PLANE, &shadowMapMatrix[12]);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glEnable(GL_TEXTURE_GEN_R);
	glEnable(GL_TEXTURE_GEN_Q);
}

// ==================== private aux drawing ====================

void Renderer::renderSelectionCircle(Vec3f v, int size, float radius) {
	GLUquadricObj *disc;

	glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

	glTranslatef(v.x, v.y, v.z);
	glRotatef(90.f, 1.f, 0.f, 0.f);
	disc= gluNewQuadric();
	gluQuadricDrawStyle(disc, GLU_FILL);
	gluCylinder(disc, radius*(size-0.2f), radius*size, 0.2f, 30, 1);
	gluDeleteQuadric(disc);

    glPopMatrix();
}

void Renderer::renderArrow(const Vec3f &pos1, const Vec3f &pos2,
						   const Vec3f &color, float width) {
	const int tesselation= 3;
	const float arrowEndSize= 0.4f;
	const float maxlen= 25;
	const float blendDelay= 5.f;

	Vec3f dir= Vec3f(pos2-pos1);
	float len= dir.length();

	if(len>maxlen) {
		return;
	}
	float alphaFactor= clamp((maxlen-len)/blendDelay, 0.f, 1.f);

	dir.normalize();
	Vec3f normal= dir.cross(Vec3f(0, 1, 0));

	Vec3f pos2Left= pos2 + normal*(width-0.05f) - dir*arrowEndSize*width;
	Vec3f pos2Right= pos2 - normal*(width-0.05f) - dir*arrowEndSize*width;
	Vec3f pos1Left= pos1 + normal*(width+0.05f);
	Vec3f pos1Right= pos1 - normal*(width+0.05f);

	//arrow body
	glBegin(GL_TRIANGLE_STRIP);
	for(int i=0; i<=tesselation; ++i) {
		float t= static_cast<float>(i)/tesselation;
		Vec3f a= pos1Left.lerp(t, pos2Left);
		Vec3f b= pos1Right.lerp(t, pos2Right);
		Vec4f c= Vec4f(color, t*0.25f*alphaFactor);

		glColor4fv(c.ptr());

		glVertex3fv(a.ptr());
		glVertex3fv(b.ptr());

	}

	glEnd();
	//arrow end
	glBegin(GL_TRIANGLES);
		glVertex3fv((pos2Left + normal*(arrowEndSize-0.1f)).ptr());
		glVertex3fv((pos2Right - normal*(arrowEndSize-0.1f)).ptr());
		glVertex3fv((pos2 + dir*(arrowEndSize-0.1f)).ptr());
	glEnd();
}

void Renderer::renderProgressBar(int size, int x, int y, Font2D *font) {

	//bar
	glBegin(GL_QUADS);
		glColor4fv(progressBarFront2.ptr());
		glVertex2i(x, y);
		glVertex2i(x, y+10);
		glColor4fv(progressBarFront1.ptr());
		glVertex2i(x+size, y+10);
		glVertex2i(x+size, y);
	glEnd();

	//transp bar
	glEnable(GL_BLEND);
	glBegin(GL_QUADS);
		glColor4fv(progressBarBack2.ptr());
		glVertex2i(x+size, y);
		glVertex2i(x+size, y+10);
		glColor4fv(progressBarBack1.ptr());
		glVertex2i(x+maxProgressBar, y+10);
		glVertex2i(x+maxProgressBar, y);
	glEnd();
	glDisable(GL_BLEND);

	//text
	glColor3fv(defColor.ptr());
	textRenderer->begin(font);
	textRenderer->render(intToStr(static_cast<int>(size))+"%", x+maxProgressBar/2, y, true);
	textRenderer->end();
}


void Renderer::renderTile(const Vec2i &pos) {

	const Map *map= game->getWorld()->getMap();
	Vec2i scaledPos= pos * Map::cellScale;

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslatef(-0.5f, 0.f, -0.5f);

	glInitNames();
	for(int i=0; i < Map::cellScale; ++i) {
		for(int j=0; j < Map::cellScale; ++j) {

			Vec2i renderPos= scaledPos + Vec2i(i, j);

			glPushName(renderPos.y);
			glPushName(renderPos.x);

			glDisable(GL_CULL_FACE);

			float h1 = map->getCell(renderPos.x, renderPos.y)->getHeight();
			float h2 = map->getCell(renderPos.x, renderPos.y+1)->getHeight();
			float h3 = map->getCell(renderPos.x+1, renderPos.y)->getHeight();
			float h4 = map->getCell(renderPos.x+1, renderPos.y+1)->getHeight();

			glBegin(GL_TRIANGLE_STRIP);
			glVertex3f(
				static_cast<float>(renderPos.x),
				h1,
				static_cast<float>(renderPos.y));
			glVertex3f(
				static_cast<float>(renderPos.x),
				h2,
				static_cast<float>(renderPos.y+1));
			glVertex3f(
				static_cast<float>(renderPos.x+1),
				h3,
				static_cast<float>(renderPos.y));
			glVertex3f(
				static_cast<float>(renderPos.x+1),
				h4,
				static_cast<float>(renderPos.y+1));
			glEnd();

			glPopName();
			glPopName();
		}
	}

	glPopMatrix();
}

void Renderer::renderQuad(int x, int y, int w, int h, const Texture2D *texture) {
	glBindTexture(GL_TEXTURE_2D, static_cast<const Texture2DGl*>(texture)->getHandle());
	glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2i(0, 1);
		glVertex2i(x, y+h);
		glTexCoord2i(0, 0);
		glVertex2i(x, y);
		glTexCoord2i(1, 1);
		glVertex2i(x+w, y+h);
		glTexCoord2i(1, 0);
		glVertex2i(x+w, y);
	glEnd();
}

Renderer::Shadows Renderer::strToShadows(const string &s){
	if(s=="Projected"){
		return sProjected;
	}
	else if(s=="ShadowMapping"){
		return sShadowMapping;
	}
	return sDisabled;
}

string Renderer::shadowsToStr(Shadows shadows){
	switch(shadows){
	case sDisabled:
		return "Disabled";
	case sProjected:
		return "Projected";
	case sShadowMapping:
		return "ShadowMapping";
	default:
		assert(false);
		return "";
	}
}

Texture2D::Filter Renderer::strToTextureFilter(const string &s){
	if(s=="Bilinear"){
		return Texture2D::fBilinear;
	}
	else if(s=="Trilinear"){
		return Texture2D::fTrilinear;
	}

	throw runtime_error("Error converting from string to FilterType, found: "+s);
}

void Renderer::setAllowRenderUnitTitles(bool value) {
	allowRenderUnitTitles = value;
	if(allowRenderUnitTitles == false) {
		renderUnitTitleList.clear();
	}
}

// This method renders titles for units
void Renderer::renderUnitTitles(Font2D *font, Vec3f color) {
	std::map<int,bool> unitRenderedList;

	if(visibleFrameUnitList.size() > 0) {
		for(int idx = 0; idx < visibleFrameUnitList.size(); idx++) {
			const Unit *unit = visibleFrameUnitList[idx];
			if(unit != NULL && unit->getCurrentUnitTitle() != "") {
				//get the screen coordinates
				Vec3f screenPos = unit->getScreenPos();
				renderText(unit->getCurrentUnitTitle(), font, color, fabs(screenPos.x) + 5, fabs(screenPos.y) + 5, false);
				//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] screenPos.x = %f, screenPos.y = %f, screenPos.z = %f\n",__FILE__,__FUNCTION__,__LINE__,screenPos.x,screenPos.y,screenPos.z);

				unitRenderedList[unit->getId()] = true;
			}
		}
		visibleFrameUnitList.clear();
	}

	if(renderUnitTitleList.size() > 0) {
		for(int idx = 0; idx < renderUnitTitleList.size(); idx++) {
			std::pair<Unit *,Vec3f> &unitInfo = renderUnitTitleList[idx];
			Unit *unit = unitInfo.first;
			if(unit != NULL && unitRenderedList.find(unit->getId()) == unitRenderedList.end()) {
				string str = unit->getFullName() + " - " + intToStr(unit->getId());
				//get the screen coordinates
				Vec3f &screenPos = unitInfo.second;
				renderText(str, font, color, fabs(screenPos.x) + 5, fabs(screenPos.y) + 5, false);
				//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] screenPos.x = %f, screenPos.y = %f, screenPos.z = %f\n",__FILE__,__FUNCTION__,__LINE__,screenPos.x,screenPos.y,screenPos.z);
			}
		}
		renderUnitTitleList.clear();
	}
}

void Renderer::setQuadCacheDirty(bool value) {
	quadCache.cacheIsDirty = value;
}

void Renderer::removeObjectFromQuadCache(const Object *o) {
	VisibleQuadContainerCache &qCache = getQuadCache();
	for(int visibleIndex = 0;
			visibleIndex < qCache.visibleObjectList.size(); ++visibleIndex) {
		Object *currentObj = qCache.visibleObjectList[visibleIndex];
		if(currentObj == o) {
			qCache.visibleObjectList.erase(qCache.visibleObjectList.begin() + visibleIndex);
			break;
		}
	}
}

void Renderer::removeUnitFromQuadCache(const Unit *unit) {
	VisibleQuadContainerCache &qCache = getQuadCache();
	for(int visibleIndex = 0;
			visibleIndex < qCache.visibleQuadUnitList.size(); ++visibleIndex) {
		Unit *currentUnit = qCache.visibleQuadUnitList[visibleIndex];
		if(currentUnit == unit) {
			qCache.visibleQuadUnitList.erase(qCache.visibleQuadUnitList.begin() + visibleIndex);
			break;
		}
	}
	for(int visibleIndex = 0;
			visibleIndex < qCache.visibleUnitList.size(); ++visibleIndex) {
		Unit *currentUnit = qCache.visibleUnitList[visibleIndex];
		if(currentUnit == unit) {
			qCache.visibleUnitList.erase(qCache.visibleUnitList.begin() + visibleIndex);
			break;
		}
	}
}

VisibleQuadContainerCache & Renderer::getQuadCache(	bool updateOnDirtyFrame,
													bool forceNew) {
	//forceNew = true;
	if(game != NULL && game->getWorld() != NULL) {
		const World *world= game->getWorld();

		if(quadCache.cacheIsDirty == true) {
			forceNew = true;
		}
		if(forceNew == true ||
			(updateOnDirtyFrame == true &&
			(world->getFrameCount() != quadCache.cacheFrame ||
			 visibleQuad != quadCache.lastVisibleQuad))) {

			// Dump cached info
			//if(forceNew == true || visibleQuad != quadCache.lastVisibleQuad) {
			//quadCache.clearCacheData();
			//}
			//else {
			quadCache.clearVolatileCacheData();
			//}

			// Unit calculations
			for(int i = 0; i < world->getFactionCount(); ++i) {
				const Faction *faction = world->getFaction(i);
				for(int j = 0; j < faction->getUnitCount(); ++j) {
					Unit *unit= faction->getUnit(j);

					bool insideQuad 	= visibleQuad.isInside(unit->getPos());
					bool renderInMap 	= world->toRenderUnit(unit);
					if(insideQuad == true && renderInMap == true) {
						quadCache.visibleQuadUnitList.push_back(unit);
					}
					else {
						unit->setVisible(false);
						// Currently don't need this list
						//quadCache.inVisibleUnitList.push_back(unit);
					}

					if(renderInMap == true) {
						quadCache.visibleUnitList.push_back(unit);
					}
				}
			}

			if(forceNew == true || visibleQuad != quadCache.lastVisibleQuad) {
				// Object calculations
				const Map *map= world->getMap();

				quadCache.clearNonVolatileCacheData();

				PosQuadIterator pqi(visibleQuad, Map::cellScale);
				while(pqi.next()) {
					const Vec2i &pos= pqi.getPos();
					if(map->isInside(pos)) {
						const Vec2i &mapPos = Map::toSurfCoords(pos);

						//quadCache.visibleCellList.push_back(mapPos);

						SurfaceCell *sc = map->getSurfaceCell(mapPos);
						Object *o = sc->getObject();
						bool isExplored = (sc->isExplored(world->getThisTeamIndex()) && o != NULL);
						//bool isVisible = (sc->isVisible(world->getThisTeamIndex()) && o != NULL);
						bool isVisible = true;

						if(isExplored == true && isVisible == true) {
							quadCache.visibleObjectList.push_back(o);
						}
					}
				}

				const Rect2i mapBounds(0, 0, map->getSurfaceW()-1, map->getSurfaceH()-1);
				Quad2i scaledQuad = visibleQuad / Map::cellScale;
				PosQuadIterator pqis(scaledQuad);
				while(pqis.next()) {
					const Vec2i &pos= pqis.getPos();
					if(mapBounds.isInside(pos)) {
						quadCache.visibleScaledCellList.push_back(pos);
					}
				}
			}
			quadCache.cacheFrame = world->getFrameCount();
			quadCache.cacheIsDirty = false;
			quadCache.lastVisibleQuad = visibleQuad;
		}
	}

	return quadCache;
}

void Renderer::renderMapPreview( const MapPreview *map, bool renderAll, 
									 int screenPosX, int screenPosY) {
	float alt=0;
	float showWater=0;
	int renderMapHeight=64;
	int renderMapWidth=64;
	float cellSize=2;
	float playerCrossSize=2;
	float clientW=renderMapWidth*cellSize;
	float clientH=renderMapHeight*cellSize;;
	
	const Metrics &metrics= Metrics::getInstance();

	// stretch small maps to 128x128
	if(map->getW() < map->getH()) {
		cellSize = cellSize * renderMapHeight / map->getH();
	}
	else {
		cellSize = cellSize * renderMapWidth / map->getW();
	}

	assertGl();

	glFrontFace(GL_CW);
	glEnable(GL_CULL_FACE);

	glMatrixMode(GL_PROJECTION);
    glPushMatrix();

	glLoadIdentity();
	//	glViewport(screenPosX, screenPosY, sizeH,sizeW);
	//	glOrtho(0, clientW, 0, clientH, -1, 1);
	glOrtho(0, metrics.getVirtualW(), 0, metrics.getVirtualH(), 0, 1);
	
	glMatrixMode(GL_MODELVIEW);

	glPushMatrix();
	glLoadIdentity();
	glTranslatef(static_cast<float>(screenPosX),static_cast<float>(screenPosY)-clientH,0.0f);
	
	glPushAttrib(GL_CURRENT_BIT);
	//glTranslatef(static_cast<float>(x), static_cast<float>(y), 0.0f);
	glLineWidth(1);
	//glClear(GL_COLOR_BUFFER_BIT);
	glColor3f(0, 0, 0);

	for (int j = 0; j < map->getH(); j++) {
		for (int i = 0; i < map->getW(); i++) {
		//surface
		alt = map->getHeight(i, j) / 20.f;
		showWater = map->getWaterLevel()/ 20.f - alt;
		showWater = (showWater > 0)? showWater:0;
		Vec3f surfColor;
		switch (map->getSurface(i, j)) {
			case st_Grass: surfColor = Vec3f(0.0, 0.8f * alt, 0.f + showWater); break;
			case st_Secondary_Grass: surfColor = Vec3f(0.4f * alt, 0.6f * alt, 0.f + showWater); break;
			case st_Road: surfColor = Vec3f(0.6f * alt, 0.3f * alt, 0.f + showWater); break;
			case st_Stone: surfColor = Vec3f(0.7f * alt, 0.7f * alt, 0.7f * alt + showWater); break;
			case st_Ground: surfColor = Vec3f(0.7f * alt, 0.5f * alt, 0.3f * alt + showWater); break;
		}

		glColor3fv(surfColor.ptr());

		glBegin(GL_TRIANGLE_STRIP);
		glVertex2f(i * cellSize, clientH - j * cellSize - cellSize);
		glVertex2f(i * cellSize, clientH - j * cellSize);
		glVertex2f(i * cellSize + cellSize, clientH - j * cellSize - cellSize);
		glVertex2f(i * cellSize + cellSize, clientH - j * cellSize);
		glEnd();

		//objects
		if(renderAll == true) {
			switch (map->getObject(i, j)) {
				case 0: glColor3f(0.f, 0.f, 0.f); break;
				case 1: glColor3f(1.f, 0.f, 0.f); break;
				case 2: glColor3f(1.f, 1.f, 1.f); break;
				case 3: glColor3f(0.5f, 0.5f, 1.f); break;
				case 4: glColor3f(0.f, 0.f, 1.f); break;
				case 5: glColor3f(0.5f, 0.5f, 0.5f); break;
				case 6: glColor3f(1.f, 0.8f, 0.5f); break;
				case 7: glColor3f(0.f, 1.f, 1.f); break;
				case 8: glColor3f(0.7f, 0.1f, 0.3f); break;
				case 9: glColor3f(0.5f, 1.f, 0.1f); break;
				case 10: glColor3f(1.f, 0.2f, 0.8f); break;// we don't render unvisible blocking objects
			}

			if ( renderAll && (map->getObject(i, j) != 0) && (map->getObject(i, j) != 10) ){
				glPointSize(cellSize / 2.f);
				glBegin(GL_POINTS);
				glVertex2f(i * cellSize + cellSize / 2.f, clientH - j * cellSize - cellSize / 2.f);
				glEnd();
			}
		}

//		bool found = false;

		//height lines
//		if (!found) {

			//left
			if (i > 0 && map->getHeight(i - 1, j) > map->getHeight(i, j)) {
				glColor3fv((surfColor*0.5f).ptr());
				glBegin(GL_LINES);
				glVertex2f(i * cellSize, clientH - (j + 1) * cellSize);
				glVertex2f(i * cellSize, clientH - j * cellSize);
				glEnd();
			}
			//down
			if (j > 0 && map->getHeight(i, j - 1) > map->getHeight(i, j)) {
				glColor3fv((surfColor*0.5f).ptr());
				glBegin(GL_LINES);
				glVertex2f(i * cellSize, clientH - j * cellSize);
				glVertex2f((i + 1) * cellSize, clientH - j * cellSize);
				glEnd();
			}


			//left
			if (i > 0 && map->getHeight(i - 1, j) < map->getHeight(i, j)) {
				glColor3fv((surfColor*2.f).ptr());
				glBegin(GL_LINES);
				glVertex2f(i * cellSize, clientH - (j + 1)  * cellSize);
				glVertex2f(i * cellSize, clientH - j * cellSize);
				glEnd();
			}
			if (j > 0 && map->getHeight(i, j - 1) < map->getHeight(i, j)) {
				glColor3fv((surfColor*2.f).ptr());
				glBegin(GL_LINES);
				glVertex2f(i * cellSize, clientH - j * cellSize);
				glVertex2f((i + 1) * cellSize, clientH - j * cellSize);
				glEnd();
			}
//				}

			//resources
			if(renderAll == true) {
				switch (map->getResource(i, j)) {
					case 1: glColor3f(1.f, 1.f, 0.f); break;
					case 2: glColor3f(0.5f, 0.5f, 0.5f); break;
					case 3: glColor3f(1.f, 0.f, 0.f); break;
					case 4: glColor3f(0.f, 0.f, 1.f); break;
					case 5: glColor3f(0.5f, 0.5f, 1.f); break;
				}

				if (renderAll && map->getResource(i, j) != 0) {
					glBegin(GL_LINES);
					glVertex2f(i * cellSize, clientH - j * cellSize - cellSize);
					glVertex2f(i * cellSize + cellSize, clientH - j * cellSize);
					glVertex2f(i * cellSize, clientH - j * cellSize);
					glVertex2f(i * cellSize + cellSize, clientH - j * cellSize - cellSize);
					glEnd();
				}
			}
		}

	}


	//start locations
	glLineWidth(3);

	// force playerCrossSize to be at least of size 4
	if(cellSize<4)
		playerCrossSize=4;
	else
		playerCrossSize=cellSize;

	for (int i = 0; i < map->getMaxFactions(); i++) {
		switch (i) {
			case 0: glColor3f(1.f, 0.f, 0.f); break;
			case 1: glColor3f(0.f, 0.f, 1.f); break;
			case 2: glColor3f(0.f, 1.f, 0.f); break;
			case 3: glColor3f(1.f, 1.f, 0.f); break;
			case 4: glColor3f(1.f, 1.f, 1.f); break;
			case 5: glColor3f(0.f, 1.f, 0.8f); break;
			case 6: glColor3f(1.f, 0.5f, 0.f); break;
			case 7: glColor3f(1.f, 0.5f, 1.f); break;
   		}
		glBegin(GL_LINES);
		glVertex2f((map->getStartLocationX(i) - 1) * cellSize, clientH - (map->getStartLocationY(i) - 1) * cellSize);
		glVertex2f((map->getStartLocationX(i) + 1) * cellSize + playerCrossSize, clientH - (map->getStartLocationY(i) + 1) * cellSize - playerCrossSize);
		glVertex2f((map->getStartLocationX(i) - 1) * cellSize, clientH - (map->getStartLocationY(i) + 1) * cellSize - playerCrossSize);
		glVertex2f((map->getStartLocationX(i) + 1) * cellSize + playerCrossSize, clientH - (map->getStartLocationY(i) - 1) * cellSize);
		glEnd();
	}

	glPopMatrix();
	glPopAttrib();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	//glViewport(0, 0, 0, 0);

	assertGl();
}


}}//end namespace
