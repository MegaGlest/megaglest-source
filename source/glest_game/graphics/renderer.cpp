//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#define NOMINMAX
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
#include "cache_manager.h"
#include "network_manager.h"
#include <algorithm>
#include "leak_dumper.h"

using namespace Shared::Graphics;
using namespace Shared::Graphics::Gl;
using namespace Shared::Util;
using namespace Shared::Graphics;

namespace Glest { namespace Game{

uint32 Renderer::SurfaceData::nextUniqueId = 1;

bool Renderer::renderText3DEnabled = true;

// =====================================================
// 	class MeshCallbackTeamColor
// =====================================================

bool MeshCallbackTeamColor::noTeamColors = false;
const string DEFAULT_CHAR_FOR_WIDTH_CALC = "V";

void MeshCallbackTeamColor::execute(const Mesh *mesh) {
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
	else {
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
float Renderer::perspFarPlane= 1000000.f;

const float Renderer::ambFactor= 0.7f;
const Vec4f Renderer::fowColor= Vec4f(0.0f, 0.0f, 0.0f, 1.0f);
const Vec4f Renderer::defSpecularColor= Vec4f(0.8f, 0.8f, 0.8f, 1.f);
const Vec4f Renderer::defDiffuseColor= Vec4f(1.f, 1.f, 1.f, 1.f);
const Vec4f Renderer::defAmbientColor= Vec4f(1.f * ambFactor, 1.f * ambFactor, 1.f * ambFactor, 1.f);
const Vec4f Renderer::defColor= Vec4f(1.f, 1.f, 1.f, 1.f);

//const float Renderer::maxLightDist= 100.f;
const float Renderer::maxLightDist= 100.f;

bool Renderer::rendererEnded = true;

const int MIN_FPS_NORMAL_RENDERING = 15;
const int MIN_FPS_NORMAL_RENDERING_TOP_THRESHOLD = 25;

const int OBJECT_SELECT_OFFSET=100000000;

bool VisibleQuadContainerCache::enableFrustumCalcs = true;

// ==================== constructor and destructor ====================

Renderer::Renderer() : BaseRenderer() {
	//this->masterserverMode = masterserverMode;
	//printf("this->masterserverMode = %d\n",this->masterserverMode);
	//assert(0==1);

	Renderer::rendererEnded = false;
	this->allowRenderUnitTitles = false;
	this->menu = NULL;
	this->game = NULL;
	this->gameCamera = NULL;
	showDebugUI = false;
	showDebugUILevel = debugui_fps;
	modelRenderer = NULL;
	textRenderer = NULL;
	textRenderer3D = NULL;
	particleRenderer = NULL;
	saveScreenShotThread = NULL;
	mapSurfaceData.clear();
	visibleFrameUnitList.clear();
	visibleFrameUnitListCameraKey = "";

	quadCache = VisibleQuadContainerCache();
	quadCache.clearFrustrumData();

	lastRenderFps=MIN_FPS_NORMAL_RENDERING;
	shadowsOffDueToMinRender=false;
	shadowMapHandle=0;
	shadowMapHandleValid=false;

	//list3d=0;
	//list3dValid=false;
	//list2d=0;
	//list2dValid=false;
	//list3dMenu=0;
	//list3dMenuValid=false;
	//customlist3dMenu=NULL;
	//this->mm3d = NULL;
	this->custom_mm3d = NULL;

	this->program = NULL;

	//resources
	for(int i=0; i < rsCount; ++i) {
		modelManager[i] = NULL;
		textureManager[i] = NULL;
		particleManager[i] = NULL;
		fontManager[i] = NULL;
	}

	Config &config= Config::getInstance();

	Renderer::perspFarPlane = config.getFloat("PerspectiveFarPlane",floatToStr(Renderer::perspFarPlane).c_str());
	this->no2DMouseRendering = config.getBool("No2DMouseRendering","false");
	this->maxConsoleLines= config.getInt("ConsoleMaxLines");

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] Renderer::perspFarPlane [%f] this->no2DMouseRendering [%d] this->maxConsoleLines [%d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,Renderer::perspFarPlane,this->no2DMouseRendering,this->maxConsoleLines);

	GraphicsInterface &gi= GraphicsInterface::getInstance();
	FactoryRepository &fr= FactoryRepository::getInstance();
	gi.setFactory(fr.getGraphicsFactory(config.getString("FactoryGraphics")));
	GraphicsFactory *graphicsFactory= GraphicsInterface::getInstance().getFactory();

	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false) {
		modelRenderer= graphicsFactory->newModelRenderer();
		textRenderer= graphicsFactory->newTextRenderer2D();
		textRenderer3D = graphicsFactory->newTextRenderer3D();
		particleRenderer= graphicsFactory->newParticleRenderer();
	}

	//resources
	for(int i=0; i< rsCount; ++i) {
		if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false) {
			modelManager[i]= graphicsFactory->newModelManager();
			textureManager[i]= graphicsFactory->newTextureManager();
			modelManager[i]->setTextureManager(textureManager[i]);
			fontManager[i]= graphicsFactory->newFontManager();
		}
		particleManager[i]= graphicsFactory->newParticleManager();
	}

	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false) {
		saveScreenShotThread = new SimpleTaskThread(this,0,25);
		saveScreenShotThread->setUniqueID(extractFileFromDirectoryPath(__FILE__).c_str());
		saveScreenShotThread->start();
	}
}

void Renderer::cleanupScreenshotThread() {
    if(saveScreenShotThread) {
		saveScreenShotThread->signalQuit();
		for(time_t elapsed = time(NULL);
			getSaveScreenQueueSize() > 0 && difftime(time(NULL),elapsed) <= 7;) {
			sleep(0);
		}
		if(saveScreenShotThread->canShutdown(true) == true &&
				saveScreenShotThread->shutdownAndWait() == true) {
			//printf("IN MenuStateCustomGame cleanup - C\n");
			delete saveScreenShotThread;
		}
		saveScreenShotThread = NULL;

		if(getSaveScreenQueueSize() > 0) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] FORCING MEMORY CLEANUP and NOT SAVING screenshots, saveScreenQueue.size() = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,saveScreenQueue.size());

			for(std::list<std::pair<string,Pixmap2D *> >::iterator iter = saveScreenQueue.begin();
				iter != saveScreenQueue.end(); ++iter) {
				delete iter->second;
			}
			saveScreenQueue.clear();
		}
	}
}

Renderer::~Renderer() {
	delete modelRenderer;
	modelRenderer = NULL;
	delete textRenderer;
	textRenderer = NULL;
	delete textRenderer3D;
	textRenderer3D = NULL;
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

	// Wait for the queue to become empty or timeout the thread at 7 seconds
    cleanupScreenshotThread();

	mapSurfaceData.clear();
	quadCache = VisibleQuadContainerCache();
	quadCache.clearFrustrumData();

	this->menu = NULL;
	this->game = NULL;
	this->gameCamera = NULL;
}

void Renderer::simpleTask(BaseThread *callingThread) {
	// This code reads pixmaps from a queue and saves them to disk
	Pixmap2D *savePixMapBuffer=NULL;
	string path="";
	static string mutexOwnerId = string(extractFileFromDirectoryPath(__FILE__).c_str()) + string("_") + intToStr(__LINE__);
	MutexSafeWrapper safeMutex(&saveScreenShotThreadAccessor,mutexOwnerId);
	if(saveScreenQueue.empty() == false) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] saveScreenQueue.size() = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,saveScreenQueue.size());

		savePixMapBuffer = saveScreenQueue.front().second;
		path = saveScreenQueue.front().first;

		saveScreenQueue.pop_front();
	}
	safeMutex.ReleaseLock();

	if(savePixMapBuffer != NULL) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] about to save [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,path.c_str());

		savePixMapBuffer->save(path);
		delete savePixMapBuffer;
	}
}

bool Renderer::isEnded() {
	return Renderer::rendererEnded;
}

Renderer &Renderer::getInstance() {
	static Renderer renderer;
	return renderer;
}

void Renderer::reinitAll() {
	//resources
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}
	for(int i=0; i<rsCount; ++i){
		//modelManager[i]->init();
		textureManager[i]->init(true);
		//particleManager[i]->init();
		//fontManager[i]->init();
	}
}
// ==================== init ====================

void Renderer::init() {
	Config &config= Config::getInstance();
	loadConfig();

	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

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

void Renderer::initGame(const Game *game, GameCamera *gameCamera) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	this->gameCamera = gameCamera;
	VisibleQuadContainerCache::enableFrustumCalcs = Config::getInstance().getBool("EnableFrustrumCalcs","true");
	quadCache = VisibleQuadContainerCache();
	quadCache.clearFrustrumData();

	SurfaceData::nextUniqueId = 1;
	mapSurfaceData.clear();
	this->game= game;
	worldToScreenPosCache.clear();

	//vars
	shadowMapFrame= 0;
	waterAnim= 0;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	//check gl caps
	checkGlOptionalCaps();

	//shadows
	if(shadows == sProjected || shadows == sShadowMapping) {
		static_cast<ModelRendererGl*>(modelRenderer)->setSecondaryTexCoordUnit(2);

		glGenTextures(1, &shadowMapHandle);
		shadowMapHandleValid=true;
		glBindTexture(GL_TEXTURE_2D, shadowMapHandle);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if(shadows == sShadowMapping) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

			//shadow mapping
			glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE_ARB);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
			//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FAIL_VALUE_ARB, 1.0f-shadowAlpha);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32,
				shadowTextureSize, shadowTextureSize,
				0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
		}
		else {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

			//projected
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8,
				shadowTextureSize, shadowTextureSize,
				0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
		}

		shadowMapFrame= -1;
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	IF_DEBUG_EDITION( getDebugRenderer().init(); )

	//texture init
	modelManager[rsGame]->init();
	textureManager[rsGame]->init();
	fontManager[rsGame]->init();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	init3dList();
}

void Renderer::manageDeferredParticleSystems() {
	for(unsigned int i = 0; i < deferredParticleSystems.size(); ++i) {
		std::pair<ParticleSystem *, ResourceScope> &deferredParticleSystem = deferredParticleSystems[i];
		ParticleSystem *ps = deferredParticleSystem.first;
		ResourceScope rs = deferredParticleSystem.second;
		if(ps->getTextureFileLoadDeferred() != "" && ps->getTexture() == NULL) {
			CoreData::TextureSystemType textureSystemId =
					static_cast<CoreData::TextureSystemType>(
							ps->getTextureFileLoadDeferredSystemId());
			if(textureSystemId != CoreData::tsyst_NONE) {
				Texture2D *texture= CoreData::getInstance().getTextureBySystemId(textureSystemId);
				//printf("Loading texture from system [%d] [%p]\n",textureSystemId,texture);
				ps->setTexture(texture);
			}
			else {
				Texture2D *texture= newTexture2D(rs);
				if(texture) {
					texture->setFormat(ps->getTextureFileLoadDeferredFormat());
					texture->getPixmap()->init(ps->getTextureFileLoadDeferredComponents());
				}
				if(texture) {
					string textureFile = ps->getTextureFileLoadDeferred();
					if(fileExists(textureFile) == false) {
						textureFile = Config::findValidLocalFileFromPath(textureFile);
					}
					texture->load(textureFile);
					ps->setTexture(texture);
				}
			}
		}
		if(dynamic_cast<GameParticleSystem *>(ps) != NULL) {
			GameParticleSystem *gps = dynamic_cast<GameParticleSystem *>(ps);
			if(gps->getModelFileLoadDeferred() != "" && gps->getModel() == NULL) {
				Model *model= newModel(rsGame);
				if(model) {
					std::map<string,vector<pair<string, string> > > loadedFileList;
					model->load(gps->getModelFileLoadDeferred(), false, &loadedFileList, NULL);
					gps->setModel(model);
				}
			}
		}
		manageParticleSystem(ps, rs);
	}
	deferredParticleSystems.clear();
	//printf("After deferredParticleSystems.size() = %d\n",deferredParticleSystems.size());
}

void Renderer::initMenu(const MainMenu *mm) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	this->menu = mm;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	modelManager[rsMenu]->init();
	textureManager[rsMenu]->init();
	fontManager[rsMenu]->init();
	//modelRenderer->setCustomTexture(CoreData::getInstance().getCustomTexture());

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	//init3dListMenu(mm);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

void Renderer::reset3d() {
	assertGl();
	glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
	//glCallList(list3d);
	render3dSetup();

	pointCount= 0;
	triangleCount= 0;
	assertGl();
}

void Renderer::reset2d() {
	assertGl();
	glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SINGLE_COLOR);
	//glCallList(list2d);
	render2dMenuSetup();
	assertGl();
}

void Renderer::reset3dMenu() {
	assertGl();
	glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SINGLE_COLOR);

	//printf("In [%s::%s Line: %d] this->custom_mm3d [%p] this->mm3d [%p]\n",__FILE__,__FUNCTION__,__LINE__,this->custom_mm3d,this->mm3d);

	if(this->custom_mm3d != NULL) {
		render3dMenuSetup(this->custom_mm3d);
		//glCallList(*this->customlist3dMenu);
	}
	else {
		render3dMenuSetup(this->menu);
		//render3dMenuSetup(this->mm3d);
		//glCallList(list3dMenu);
	}

	assertGl();
}

// ==================== end ====================

void Renderer::end() {
	quadCache = VisibleQuadContainerCache();
	quadCache.clearFrustrumData();

	if(Renderer::rendererEnded == true) {
		return;
	}
	std::map<string,Texture2D *> &crcFactionPreviewTextureCache = CacheManager::getCachedItem< std::map<string,Texture2D *> >(GameConstants::factionPreviewTextureCacheLookupKey);
	crcFactionPreviewTextureCache.clear();

	// Wait for the queue to become empty or timeout the thread at 7 seconds
	cleanupScreenshotThread();

	mapSurfaceData.clear();

	//delete resources
	if(modelManager[rsGlobal]) {
		modelManager[rsGlobal]->end();
	}
	if(textureManager[rsGlobal]) {
		textureManager[rsGlobal]->end();
	}
	if(fontManager[rsGlobal]) {
		fontManager[rsGlobal]->end();
	}
	if(particleManager[rsGlobal]) {
		particleManager[rsGlobal]->end();
	}

	//delete 2d list
	//if(list2dValid == true) {
	//	glDeleteLists(list2d, 1);
	//	list2dValid=false;
	//}

	Renderer::rendererEnded = true;
}

void Renderer::endScenario() {
	this->game= NULL;
	this->gameCamera = NULL;
	quadCache = VisibleQuadContainerCache();
	quadCache.clearFrustrumData();

	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	//delete resources
	//modelManager[rsGame]->end();
	//textureManager[rsGame]->end();
	//fontManager[rsGame]->end();
	//particleManager[rsGame]->end();

	if(shadowMapHandleValid == true &&
		(shadows == sProjected || shadows == sShadowMapping)) {
		glDeleteTextures(1, &shadowMapHandle);
		shadowMapHandleValid=false;
	}

	//if(list3dValid == true) {
	//	glDeleteLists(list3d, 1);
	//	list3dValid=false;
	//}

	worldToScreenPosCache.clear();
	ReleaseSurfaceVBOs();
	mapSurfaceData.clear();
}

void Renderer::endGame(bool isFinalEnd) {
	this->game= NULL;
	this->gameCamera = NULL;

	quadCache = VisibleQuadContainerCache();
	quadCache.clearFrustrumData();

	if(isFinalEnd) {
		//delete resources
		if(modelManager[rsGame] != NULL) {
			modelManager[rsGame]->end();
		}
		if(textureManager[rsGame] != NULL) {
			textureManager[rsGame]->end();
		}
		if(fontManager[rsGame] != NULL) {
			fontManager[rsGame]->end();
		}
		if(particleManager[rsGame] != NULL) {
			particleManager[rsGame]->end();
		}
	}

	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	if(shadowMapHandleValid == true &&
		(shadows == sProjected || shadows == sShadowMapping)) {
		glDeleteTextures(1, &shadowMapHandle);
		shadowMapHandleValid=false;
	}

	//if(list3dValid == true) {
	//	glDeleteLists(list3d, 1);
	//	list3dValid=false;
	//}

	worldToScreenPosCache.clear();
	ReleaseSurfaceVBOs();
	mapSurfaceData.clear();
}

void Renderer::endMenu() {
	this->menu = NULL;

	//delete resources
	if(modelManager[rsMenu]) {
		modelManager[rsMenu]->end();
	}
	if(textureManager[rsMenu]) {
		textureManager[rsMenu]->end();
	}
	if(fontManager[rsMenu]) {
		fontManager[rsMenu]->end();
	}
	if(particleManager[rsMenu]) {
		particleManager[rsMenu]->end();
	}

	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	//if(this->customlist3dMenu != NULL) {
	//	glDeleteLists(*this->customlist3dMenu,1);
	//}
	//else {
	//	glDeleteLists(list3dMenu, 1);
	//}
}

void Renderer::reloadResources() {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	for(int i=0; i<rsCount; ++i) {
		modelManager[i]->end();
		textureManager[i]->end();
		fontManager[i]->end();
	}

	for(int i=0; i<rsCount; ++i) {
		modelManager[i]->init();
		textureManager[i]->init();
		fontManager[i]->init();
	}
}

// ==================== engine interface ====================

void Renderer::initTexture(ResourceScope rs, Texture *texture) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	textureManager[rs]->initTexture(texture);
}

void Renderer::endTexture(ResourceScope rs, Texture *texture, bool mustExistInList) {
	string textureFilename = texture->getPath();

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] free texture from manager [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,textureFilename.c_str());

	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	textureManager[rs]->endTexture(texture,mustExistInList);

	if(rs == rsGlobal) {
		std::map<string,Texture2D *> &crcFactionPreviewTextureCache = CacheManager::getCachedItem< std::map<string,Texture2D *> >(GameConstants::factionPreviewTextureCacheLookupKey);
		if(crcFactionPreviewTextureCache.find(textureFilename) != crcFactionPreviewTextureCache.end()) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] textureFilename [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,textureFilename.c_str());
			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] free texture from cache [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,textureFilename.c_str());

			crcFactionPreviewTextureCache.erase(textureFilename);
		}
	}
}
void Renderer::endLastTexture(ResourceScope rs, bool mustExistInList) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	textureManager[rs]->endLastTexture(mustExistInList);
}

Model *Renderer::newModel(ResourceScope rs){
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return NULL;
	}

	return modelManager[rs]->newModel();
}

void Renderer::endModel(ResourceScope rs, Model *model,bool mustExistInList) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	modelManager[rs]->endModel(model,mustExistInList);
}
void Renderer::endLastModel(ResourceScope rs, bool mustExistInList) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	modelManager[rs]->endLastModel(mustExistInList);
}

Texture2D *Renderer::newTexture2D(ResourceScope rs){
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return NULL;
	}

	return textureManager[rs]->newTexture2D();
}

Texture3D *Renderer::newTexture3D(ResourceScope rs){
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return NULL;
	}

	return textureManager[rs]->newTexture3D();
}

Font2D *Renderer::newFont(ResourceScope rs){
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return NULL;
	}

	return fontManager[rs]->newFont2D();
}

Font3D *Renderer::newFont3D(ResourceScope rs){
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return NULL;
	}

	return fontManager[rs]->newFont3D();
}

void Renderer::endFont(Font *font, ResourceScope rs, bool mustExistInList) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	fontManager[rs]->endFont(font,mustExistInList);
}

void Renderer::resetFontManager(ResourceScope rs) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}
	fontManager[rs]->end();
	fontManager[rsGlobal]->init();
}

void Renderer::addToDeferredParticleSystemList(std::pair<ParticleSystem *, ResourceScope> deferredParticleSystem) {
	deferredParticleSystems.push_back(deferredParticleSystem);
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
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	glPushAttrib(GL_DEPTH_BUFFER_BIT  | GL_STENCIL_BUFFER_BIT);
	glDepthFunc(GL_LESS);
	particleRenderer->renderManager(particleManager[rs], modelRenderer);
	glPopAttrib();
}

void Renderer::swapBuffers() {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}
	//glFlush(); // should not be required - http://www.opengl.org/wiki/Common_Mistakes
	glFlush();

	GraphicsInterface::getInstance().getCurrentContext()->swapBuffers();
}

// ==================== lighting ====================

//places all the opengl lights
void Renderer::setupLighting() {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

    int lightCount= 0;
	const World *world= game->getWorld();
	const GameCamera *gameCamera= game->getGameCamera();
	const TimeFlow *timeFlow= world->getTimeFlow();
	float time= timeFlow->getTime();

	assertGl();

    //sun/moon light
	Vec3f lightColor= timeFlow->computeLightColor();
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
	for(int i= 1; i < maxLights; ++i) {
		glDisable(GL_LIGHT0 + i);
	}

    //unit lights (not projectiles)
	if(timeFlow->isTotalNight()) {
        for(int i = 0; i < world->getFactionCount() && lightCount < maxLights; ++i) {
            for(int j = 0; j < world->getFaction(i)->getUnitCount() && lightCount < maxLights; ++j) {
                Unit *unit= world->getFaction(i)->getUnit(j);
				if(world->toRenderUnit(unit) &&
					unit->getCurrVector().dist(gameCamera->getPos()) < maxLightDist &&
                    unit->getType()->getLight() && unit->isOperative()){
					//printf("$$$ Show light # %d / %d for Unit [%d - %s]\n",lightCount,maxLights,unit->getId(),unit->getFullName().c_str());

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

void Renderer::loadGameCameraMatrix() {
	const GameCamera *gameCamera= game->getGameCamera();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	if(gameCamera != NULL) {
		glRotatef(gameCamera->getVAng(), -1, 0, 0);
		glRotatef(gameCamera->getHAng(), 0, 1, 0);
		glTranslatef(-gameCamera->getPos().x, -gameCamera->getPos().y, -gameCamera->getPos().z);
	}
}

void Renderer::loadCameraMatrix(const Camera *camera) {
	const Vec3f &position= camera->getConstPosition();
	Quaternion orientation= camera->getOrientation().conjugate();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMultMatrixf(orientation.toMatrix4().ptr());
	glTranslatef(-position.x, -position.y, -position.z);
}

enum PROJECTION_TO_INFINITY {
	D_IS_ZERO,
	N_OVER_D_IS_OUTSIDE
};

static Vec2i _unprojectMap(const Vec2i& pt,const GLdouble* model,const GLdouble* projection,const GLint* viewport,const char* label=NULL) {
	Vec3d a,b;
	/*  note viewport[3] is height of window in pixels  */
	GLint realy = viewport[3] - (GLint) pt.y;
	gluUnProject(pt.x,realy,0,model,projection,viewport,&a.x,&a.y,&a.z);
	gluUnProject(pt.x,realy,1,model,projection,viewport,&b.x,&b.y,&b.z);

/*
	//We could use some vector3d class, but this will do fine for now
	//ray
	b.x -= a.x;
	b.y -= a.y;
	b.z -= a.z;
	float rayLength = streflop::sqrtf(a.x*a.x + a.y*a.y + a.z*a.z);
	//normalize
	b.x /= rayLength;
	b.y /= rayLength;
	b.z /= rayLength;

	//T = [planeNormal.(pointOnPlane - rayOrigin)]/planeNormal.rayDirection;
	//pointInPlane = rayOrigin + (rayDirection * T);

	float dot1, dot2;

	float pointInPlaneX = 0;
	float pointInPlaneY = 0;
	float pointInPlaneZ = 0;
	float planeNormalX = 0;
	float planeNormalY = 0;
	float planeNormalZ = -1;

	pointInPlaneX -= a.x;
	pointInPlaneY -= a.y;
	pointInPlaneZ -= a.z;

	dot1 = (planeNormalX * pointInPlaneX) + (planeNormalY * pointInPlaneY) + (planeNormalZ * pointInPlaneZ);
	dot2 = (planeNormalX * b.x) + (planeNormalY * b.y) + (planeNormalZ * b.z);

	float t = dot1/dot2;

	b.x *= t;
	b.y *= t;
	//b.z *= t;
	//we don't need the z coordinate in my case

	//return Vec2i(b.x + a.x, b.z + a.z);
	return Vec2i(b.x + a.x, b.z + a.z);
*/



	// junk values if you were looking parallel to the XZ plane; this shouldn't happen as the camera can't do this?
	const Vec3f
		start(a.x,a.y,a.z),
		stop(b.x,b.y,b.z),
		plane(0,0,0),
		norm(0,1,0),
		u = stop-start,
		w = start-plane;
	const float d = norm.x*u.x + norm.y*u.y + norm.z*u.z;
#ifdef USE_STREFLOP
	if(streflop::fabs(static_cast<streflop::Simple>(d)) < 0.00001)
#else
	if(fabs(d) < 0.00001)
#endif
		throw D_IS_ZERO;

	const float nd = -(norm.x*w.x + norm.y*w.y + norm.z*w.z) / d;
	if(nd < 0.0 || nd >= 1.0)
		throw N_OVER_D_IS_OUTSIDE;

	const Vec3f i = start + u*nd;
	//const Vec2i pos(i.x,i.z);

	Vec2i pos;
	if(strcmp(label,"tl") == 0) {
#ifdef USE_STREFLOP
		pos = Vec2i(streflop::floor(static_cast<streflop::Simple>(i.x)),streflop::floor(static_cast<streflop::Simple>(i.z)));
#else
		pos = Vec2i(floor(i.x),floor(i.z));
#endif
	}
	else if(strcmp(label,"tr") == 0) {
#ifdef USE_STREFLOP
		pos = Vec2i(streflop::ceil(static_cast<streflop::Simple>(i.x)),streflop::floor(static_cast<streflop::Simple>(i.z)));
#else
		pos = Vec2i(ceil(i.x),floor(i.z));
#endif
	}
	else if(strcmp(label,"bl") == 0) {
#ifdef USE_STREFLOP
		pos = Vec2i(streflop::floor(static_cast<streflop::Simple>(i.x)),streflop::ceil(static_cast<streflop::Simple>(i.z)));
#else
		pos = Vec2i(floor(i.x),ceil(i.z));
#endif
	}
	else if(strcmp(label,"br") == 0) {
#ifdef USE_STREFLOP
		pos = Vec2i(streflop::ceil(static_cast<streflop::Simple>(i.x)),streflop::ceil(static_cast<streflop::Simple>(i.z)));
#else
		pos = Vec2i(ceil(i.x),ceil(i.z));
#endif
	}

	if(false) { // print debug info
		if(label) printf("%s ",label);
		printf("%d,%d -> %f,%f,%f -> %f,%f,%f -> %f,%f,%f -> %d,%d\n",
			pt.x,pt.y,
			start.x,start.y,start.z,
			stop.x,stop.y,stop.z,
			i.x,i.y,i.z,
			pos.x,pos.y);
	}
	return pos;

}

//Matrix4 LookAt( Vector3 eye, Vector3 target, Vector3 up ) {
//    Vector3 zaxis = normal(target - eye);    // The "look-at" vector.
//    Vector3 xaxis = normal(cross(up, zaxis));// The "right" vector.
//    Vector3 yaxis = cross(zaxis, xaxis);     // The "up" vector.
//
//    // Create a 4x4 orientation matrix from the right, up, and at vectors
//    Matrix4 orientation = {
//        xaxis.x, yaxis.x, zaxis.x, 0,
//        xaxis.y, yaxis.y, zaxis.y, 0,
//        xaxis.z, yaxis.z, zaxis.z, 0,
//          0,       0,       0,     1
//    };
//
//    // Create a 4x4 translation matrix by negating the eye position.
//    Matrix4 translation = {
//          1,      0,      0,     0,
//          0,      1,      0,     0,
//          0,      0,      1,     0,
//        -eye.x, -eye.y, -eye.z,  1
//    };
//
//    // Combine the orientation and translation to compute the view matrix
//    return ( translation * orientation );
//}


bool Renderer::ExtractFrustum(VisibleQuadContainerCache &quadCacheItem) {
   bool frustrumChanged = false;
   vector<float> proj(16,0);
   vector<float> modl(16,0);

   /* Get the current PROJECTION matrix from OpenGL */
   glGetFloatv( GL_PROJECTION_MATRIX, &proj[0] );

   /* Get the current MODELVIEW matrix from OpenGL */
   glGetFloatv( GL_MODELVIEW_MATRIX, &modl[0] );

//   for(unsigned int i = 0; i < proj.size(); ++i) {
//	   //printf("\ni = %d proj [%f][%f] modl [%f][%f]\n",i,proj[i],quadCacheItem.proj[i],modl[i],quadCacheItem.modl[i]);
//	   if(proj[i] != quadCacheItem.proj[i]) {
//		   frustrumChanged = true;
//		   break;
//	   }
//	   if(modl[i] != quadCacheItem.modl[i]) {
//		   frustrumChanged = true;
//		   break;
//	   }
//   }

   // Check the frustum cache
   const bool useFrustumCache = Config::getInstance().getBool("EnableFrustrumCache","false");
   pair<vector<float>,vector<float> > lookupKey;
   if(useFrustumCache == true) {
	   lookupKey = make_pair(proj,modl);
	   map<pair<vector<float>,vector<float> >, vector<vector<float> > >::iterator iterFind = quadCacheItem.frustumDataCache.find(lookupKey);
	   if(iterFind != quadCacheItem.frustumDataCache.end()) {
		   if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\nCalc Frustrum found in cache\n");

		   quadCacheItem.frustumData = iterFind->second;
		   frustrumChanged = (quadCacheItem.proj != proj || quadCacheItem.modl != modl);
		   if(frustrumChanged == true) {
			   quadCacheItem.proj = proj;
			   quadCacheItem.modl = modl;
		   }

		   return frustrumChanged;
	   }
   }

   if(quadCacheItem.proj != proj || quadCacheItem.modl != modl) {
   //if(frustrumChanged == true) {
	   frustrumChanged = true;
	   vector<vector<float> > &frustum = quadCacheItem.frustumData;
	   //assert(frustum.size() == 6);
	   //assert(frustum[0].size() == 4);

	   quadCacheItem.proj = proj;
	   quadCacheItem.modl = modl;

	   float   clip[16];
	   float   t=0;

	   /* Combine the two matrices (multiply projection by modelview) */
	   clip[ 0] = modl[ 0] * proj[ 0] + modl[ 1] * proj[ 4] + modl[ 2] * proj[ 8] + modl[ 3] * proj[12];
	   clip[ 1] = modl[ 0] * proj[ 1] + modl[ 1] * proj[ 5] + modl[ 2] * proj[ 9] + modl[ 3] * proj[13];
	   clip[ 2] = modl[ 0] * proj[ 2] + modl[ 1] * proj[ 6] + modl[ 2] * proj[10] + modl[ 3] * proj[14];
	   clip[ 3] = modl[ 0] * proj[ 3] + modl[ 1] * proj[ 7] + modl[ 2] * proj[11] + modl[ 3] * proj[15];

	   clip[ 4] = modl[ 4] * proj[ 0] + modl[ 5] * proj[ 4] + modl[ 6] * proj[ 8] + modl[ 7] * proj[12];
	   clip[ 5] = modl[ 4] * proj[ 1] + modl[ 5] * proj[ 5] + modl[ 6] * proj[ 9] + modl[ 7] * proj[13];
	   clip[ 6] = modl[ 4] * proj[ 2] + modl[ 5] * proj[ 6] + modl[ 6] * proj[10] + modl[ 7] * proj[14];
	   clip[ 7] = modl[ 4] * proj[ 3] + modl[ 5] * proj[ 7] + modl[ 6] * proj[11] + modl[ 7] * proj[15];

	   clip[ 8] = modl[ 8] * proj[ 0] + modl[ 9] * proj[ 4] + modl[10] * proj[ 8] + modl[11] * proj[12];
	   clip[ 9] = modl[ 8] * proj[ 1] + modl[ 9] * proj[ 5] + modl[10] * proj[ 9] + modl[11] * proj[13];
	   clip[10] = modl[ 8] * proj[ 2] + modl[ 9] * proj[ 6] + modl[10] * proj[10] + modl[11] * proj[14];
	   clip[11] = modl[ 8] * proj[ 3] + modl[ 9] * proj[ 7] + modl[10] * proj[11] + modl[11] * proj[15];

	   clip[12] = modl[12] * proj[ 0] + modl[13] * proj[ 4] + modl[14] * proj[ 8] + modl[15] * proj[12];
	   clip[13] = modl[12] * proj[ 1] + modl[13] * proj[ 5] + modl[14] * proj[ 9] + modl[15] * proj[13];
	   clip[14] = modl[12] * proj[ 2] + modl[13] * proj[ 6] + modl[14] * proj[10] + modl[15] * proj[14];
	   clip[15] = modl[12] * proj[ 3] + modl[13] * proj[ 7] + modl[14] * proj[11] + modl[15] * proj[15];

	   /* Extract the numbers for the RIGHT plane */
	   frustum[0][0] = clip[ 3] - clip[ 0];
	   frustum[0][1] = clip[ 7] - clip[ 4];
	   frustum[0][2] = clip[11] - clip[ 8];
	   frustum[0][3] = clip[15] - clip[12];

	   if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\nCalc Frustrum #%da: [%f][%f][%f][%f]\n",0,frustum[0][0],frustum[0][1],frustum[0][2],frustum[0][3]);

	   /* Normalize the result */
	#ifdef USE_STREFLOP
	   t = streflop::sqrt( static_cast<streflop::Simple>(frustum[0][0] * frustum[0][0] + frustum[0][1] * frustum[0][1] + frustum[0][2] * frustum[0][2]) );
	#else
	   t = sqrt( frustum[0][0] * frustum[0][0] + frustum[0][1] * frustum[0][1] + frustum[0][2] * frustum[0][2] );
	#endif
	   if(t != 0.0) {
		   frustum[0][0] /= t;
		   frustum[0][1] /= t;
		   frustum[0][2] /= t;
		   frustum[0][3] /= t;

		   if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\nCalc Frustrum #%db: [%f][%f][%f][%f] t = %f\n",0,frustum[0][0],frustum[0][1],frustum[0][2],frustum[0][3],t);
	   }

	   /* Extract the numbers for the LEFT plane */
	   frustum[1][0] = clip[ 3] + clip[ 0];
	   frustum[1][1] = clip[ 7] + clip[ 4];
	   frustum[1][2] = clip[11] + clip[ 8];
	   frustum[1][3] = clip[15] + clip[12];

	   if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\nCalc Frustrum #%da: [%f][%f][%f][%f]\n",1,frustum[1][0],frustum[1][1],frustum[1][2],frustum[1][3]);

	   /* Normalize the result */
	#ifdef USE_STREFLOP
	   t = streflop::sqrt( static_cast<streflop::Simple>(frustum[1][0] * frustum[1][0] + frustum[1][1] * frustum[1][1] + frustum[1][2] * frustum[1][2]) );
	#else
	   t = sqrt( frustum[1][0] * frustum[1][0] + frustum[1][1] * frustum[1][1] + frustum[1][2] * frustum[1][2] );
	#endif
	   if(t != 0.0) {
		   frustum[1][0] /= t;
		   frustum[1][1] /= t;
		   frustum[1][2] /= t;
		   frustum[1][3] /= t;

		   if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\nCalc Frustrum #%db: [%f][%f][%f][%f] t = %f\n",1,frustum[1][0],frustum[1][1],frustum[1][2],frustum[1][3],t);
	   }

	   /* Extract the BOTTOM plane */
	   frustum[2][0] = clip[ 3] + clip[ 1];
	   frustum[2][1] = clip[ 7] + clip[ 5];
	   frustum[2][2] = clip[11] + clip[ 9];
	   frustum[2][3] = clip[15] + clip[13];

	   if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\nCalc Frustrum #%da: [%f][%f][%f][%f]\n",2,frustum[2][0],frustum[2][1],frustum[2][2],frustum[2][3]);

	   /* Normalize the result */
	#ifdef USE_STREFLOP
	   t = streflop::sqrt( static_cast<streflop::Simple>(frustum[2][0] * frustum[2][0] + frustum[2][1] * frustum[2][1] + frustum[2][2] * frustum[2][2]) );
	#else
	   t = sqrt( frustum[2][0] * frustum[2][0] + frustum[2][1] * frustum[2][1] + frustum[2][2] * frustum[2][2] );
	#endif
	   if(t != 0.0) {
		   frustum[2][0] /= t;
		   frustum[2][1] /= t;
		   frustum[2][2] /= t;
		   frustum[2][3] /= t;

		   if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\nCalc Frustrum #%db: [%f][%f][%f][%f] t = %f\n",2,frustum[2][0],frustum[2][1],frustum[2][2],frustum[2][3],t);
	   }

	   /* Extract the TOP plane */
	   frustum[3][0] = clip[ 3] - clip[ 1];
	   frustum[3][1] = clip[ 7] - clip[ 5];
	   frustum[3][2] = clip[11] - clip[ 9];
	   frustum[3][3] = clip[15] - clip[13];

	   if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\nCalc Frustrum #%da: [%f][%f][%f][%f]\n",3,frustum[3][0],frustum[3][1],frustum[3][2],frustum[3][3]);

	   /* Normalize the result */
	#ifdef USE_STREFLOP
	   t = streflop::sqrt( static_cast<streflop::Simple>(frustum[3][0] * frustum[3][0] + frustum[3][1] * frustum[3][1] + frustum[3][2] * frustum[3][2]) );
	#else
	   t = sqrt( frustum[3][0] * frustum[3][0] + frustum[3][1] * frustum[3][1] + frustum[3][2] * frustum[3][2] );
	#endif

	   if(t != 0.0) {
		   frustum[3][0] /= t;
		   frustum[3][1] /= t;
		   frustum[3][2] /= t;
		   frustum[3][3] /= t;

		   if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\nCalc Frustrum #%db: [%f][%f][%f][%f] t = %f\n",3,frustum[3][0],frustum[3][1],frustum[3][2],frustum[3][3],t);
	   }

	   /* Extract the FAR plane */
	   frustum[4][0] = clip[ 3] - clip[ 2];
	   frustum[4][1] = clip[ 7] - clip[ 6];
	   frustum[4][2] = clip[11] - clip[10];
	   frustum[4][3] = clip[15] - clip[14];

	   if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\nCalc Frustrum #%da: [%f][%f][%f][%f]\n",4,frustum[4][0],frustum[4][1],frustum[4][2],frustum[4][3]);

	   /* Normalize the result */
	#ifdef USE_STREFLOP
	   t = streflop::sqrt( static_cast<streflop::Simple>(frustum[4][0] * frustum[4][0] + frustum[4][1] * frustum[4][1] + frustum[4][2] * frustum[4][2]) );
	#else
	   t = sqrt( frustum[4][0] * frustum[4][0] + frustum[4][1] * frustum[4][1] + frustum[4][2] * frustum[4][2] );
	#endif

	   if(t != 0.0) {
		   frustum[4][0] /= t;
		   frustum[4][1] /= t;
		   frustum[4][2] /= t;
		   frustum[4][3] /= t;

		   if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\nCalc Frustrum #%db: [%f][%f][%f][%f] t = %f\n",4,frustum[4][0],frustum[4][1],frustum[4][2],frustum[4][3],t);
	   }

	   /* Extract the NEAR plane */
	   frustum[5][0] = clip[ 3] + clip[ 2];
	   frustum[5][1] = clip[ 7] + clip[ 6];
	   frustum[5][2] = clip[11] + clip[10];
	   frustum[5][3] = clip[15] + clip[14];

	   if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\nCalc Frustrum #%da: [%f][%f][%f][%f]\n",5,frustum[5][0],frustum[5][1],frustum[5][2],frustum[5][3]);

	   /* Normalize the result */
	#ifdef USE_STREFLOP
	   t = streflop::sqrt( static_cast<streflop::Simple>(frustum[5][0] * frustum[5][0] + frustum[5][1] * frustum[5][1] + frustum[5][2] * frustum[5][2]) );
	#else
	   t = sqrt( frustum[5][0] * frustum[5][0] + frustum[5][1] * frustum[5][1] + frustum[5][2] * frustum[5][2] );
	#endif

	   if(t != 0.0) {
		   frustum[5][0] /= t;
		   frustum[5][1] /= t;
		   frustum[5][2] /= t;
		   frustum[5][3] /= t;

		   if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\nCalc Frustrum #%db: [%f][%f][%f][%f] t = %f\n",5,frustum[5][0],frustum[5][1],frustum[5][2],frustum[5][3],t);
	   }

	   if(useFrustumCache == true) {
		   quadCacheItem.frustumDataCache[lookupKey] = frustum;
	   }
   }
   return frustrumChanged;
}

bool Renderer::PointInFrustum(vector<vector<float> > &frustum, float x, float y, float z ) {
   unsigned int p=0;

   for( p = 0; p < frustum.size(); p++ ) {
      if( frustum[p][0] * x + frustum[p][1] * y + frustum[p][2] * z + frustum[p][3] <= 0 ) {
         return false;
      }
   }
   return true;
}

bool Renderer::SphereInFrustum(vector<vector<float> > &frustum,  float x, float y, float z, float radius) {
	// Go through all the sides of the frustum
	for(int i = 0; i < frustum.size(); i++ ) {
		// If the center of the sphere is farther away from the plane than the radius
		if(frustum[i][0] * x + frustum[i][1] * y + frustum[i][2] * z + frustum[i][3] <= -radius ) {
			// The distance was greater than the radius so the sphere is outside of the frustum
			return false;
		}
	}

	// The sphere was inside of the frustum!
	return true;
}

bool Renderer::CubeInFrustum(vector<vector<float> > &frustum, float x, float y, float z, float size ) {
   unsigned int p=0;

   for( p = 0; p < frustum.size(); p++ ) {
      if( frustum[p][0] * (x - size) + frustum[p][1] * (y - size) + frustum[p][2] * (z - size) + frustum[p][3] > 0 )
         continue;
      if( frustum[p][0] * (x + size) + frustum[p][1] * (y - size) + frustum[p][2] * (z - size) + frustum[p][3] > 0 )
         continue;
      if( frustum[p][0] * (x - size) + frustum[p][1] * (y + size) + frustum[p][2] * (z - size) + frustum[p][3] > 0 )
         continue;
      if( frustum[p][0] * (x + size) + frustum[p][1] * (y + size) + frustum[p][2] * (z - size) + frustum[p][3] > 0 )
         continue;
      if( frustum[p][0] * (x - size) + frustum[p][1] * (y - size) + frustum[p][2] * (z + size) + frustum[p][3] > 0 )
         continue;
      if( frustum[p][0] * (x + size) + frustum[p][1] * (y - size) + frustum[p][2] * (z + size) + frustum[p][3] > 0 )
         continue;
      if( frustum[p][0] * (x - size) + frustum[p][1] * (y + size) + frustum[p][2] * (z + size) + frustum[p][3] > 0 )
         continue;
      if( frustum[p][0] * (x + size) + frustum[p][1] * (y + size) + frustum[p][2] * (z + size) + frustum[p][3] > 0 )
         continue;
      return false;
   }
   return true;
}

void Renderer::computeVisibleQuad() {
	visibleQuad = this->gameCamera->computeVisibleQuad();

	//Matrix4 LookAt( gameCamera->getPos(), gameCamera->getPos(), Vector3 up );
	//gluLookAt

	//const Metrics &metrics= Metrics::getInstance();
	//float Hnear = 2.0 * streflop::tanf(gameCamera->getFov() / 2.0) * perspNearPlane;
	//float Hnear = 2.0 * streflop::tanf(perspFov / 2.0) * perspNearPlane;
	//float Wnear = Hnear * metrics.getAspectRatio();
	//The same reasoning can be applied to the far plane:
	//float Hfar = 2.0 * streflop::tanf(perspFov / 2.0) * perspFarPlane;
	//float Hfar = 2.0 * streflop::tanf(gameCamera->getFov() / 2.0) * perspFarPlane;
	//float Wfar = Hfar * metrics.getAspectRatio();
	//printf("Hnear = %f, Wnear = %f, Hfar = %f, Wfar = %f\n",Hnear,Wnear,Hfar,Wfar);

	bool frustrumChanged = false;
	if(VisibleQuadContainerCache::enableFrustumCalcs == true) {
		frustrumChanged = ExtractFrustum(quadCache);
	}

	if(frustrumChanged && SystemFlags::VERBOSE_MODE_ENABLED) {
		printf("\nCamera: %d,%d %d,%d %d,%d %d,%d\n",
			visibleQuad.p[0].x,visibleQuad.p[0].y,
			visibleQuad.p[1].x,visibleQuad.p[1].y,
			visibleQuad.p[2].x,visibleQuad.p[2].y,
			visibleQuad.p[3].x,visibleQuad.p[3].y);

		for(unsigned int i = 0; i < quadCache.frustumData.size(); ++i) {
			printf("\nFrustrum #%d [%lu]: ",i,quadCache.frustumData.size());
			vector<float> &frustumDataInner = quadCache.frustumData[i];
			for(unsigned int j = 0; j < frustumDataInner.size(); ++j) {
				printf("[%f]",quadCache.frustumData[i][j]);
			}
		}

		printf("\nEND\n");
	}

	const bool newVisibleQuadCalc = false;
	if(newVisibleQuadCalc) {
		const bool debug = false;
		try {
			if(debug) {
				visibleQuad = gameCamera->computeVisibleQuad();
				printf("Camera: %d,%d %d,%d %d,%d %d,%d\n",
					visibleQuad.p[0].x,visibleQuad.p[0].y,
					visibleQuad.p[1].x,visibleQuad.p[1].y,
					visibleQuad.p[2].x,visibleQuad.p[2].y,
					visibleQuad.p[3].x,visibleQuad.p[3].y);
			}


			// compute the four corners using OpenGL
			GLdouble model[16], projection[16];
			GLint viewport[4];
			glGetDoublev(GL_MODELVIEW_MATRIX,model);
			glGetDoublev(GL_PROJECTION_MATRIX,projection);
			glGetIntegerv(GL_VIEWPORT,viewport);
			Vec2i
				tl = _unprojectMap(Vec2i(0,0),model,projection,viewport,"tl"),
				tr = _unprojectMap(Vec2i(viewport[2],0),model,projection,viewport,"tr"),
				br = _unprojectMap(Vec2i(viewport[2],viewport[3]),model,projection,viewport,"br"),
				bl = _unprojectMap(Vec2i(0,viewport[3]),model,projection,viewport,"bl");


			// orientate it for map iterator
			//bool swapRequiredX = false;
			bool swapRequiredY = false;
			int const cellBuffer = 4;
			if((tl.x > tr.x) || (bl.x > br.x)) {
				if(debug) printf("Swap X???\n");

				//std::swap(tl,bl);
				//std::swap(tr,br);
				if(tl.x > tr.x) {
					if(debug) printf("Swap X1???\n");

					tr.x += cellBuffer;
					tl.x -= cellBuffer;

					std::swap(tl.x,tr.x);
					//swapRequiredX = true;
				}
				else {
					tl.x += cellBuffer;
					tr.x -= cellBuffer;
				}
				if(bl.x > br.x) {
					if(debug) printf("Swap X2???\n");

					bl.x += cellBuffer;
					br.x -= cellBuffer;

					std::swap(bl.x,br.x);
					//swapRequiredX = true;
				}
				else {
					br.x += cellBuffer;
					bl.x -= cellBuffer;
				}
			}

			if((tl.y > bl.y) || (tr.y > br.y)) {
				visibleQuad = this->gameCamera->computeVisibleQuad();

				if(debug) printf("Swap Y???\n");

				if(tl.y > bl.y) {
					if(debug) printf("Swap Y1???\n");

					tl.y += cellBuffer;
					bl.y -= cellBuffer;

					std::swap(tl.y,bl.y);
					swapRequiredY = true;
				}
				else {
					bl.y += cellBuffer;
					tl.y -= cellBuffer;
				}
				if(tr.y > br.y) {
					if(debug) printf("Swap Y2???\n");

					tr.y += cellBuffer;
					br.y -= cellBuffer;

					std::swap(tr.y,br.y);
					swapRequiredY = true;
				}
				else {
					br.y += cellBuffer;
					tr.y -= cellBuffer;
				}


				//std::swap(tl,tr);
				//std::swap(bl,br);
			}
			if(swapRequiredY == false) {
				tl.y -= cellBuffer;
				tr.y -= cellBuffer;
				bl.y += cellBuffer;
				br.y += cellBuffer;
			}

			// set it as the frustum
			visibleQuad = Quad2i(tl,bl,tr,br); // strange order
			if(debug) {
				printf("Will:   %d,%d %d,%d %d,%d %d,%d\n",
					visibleQuad.p[0].x,visibleQuad.p[0].y,
					visibleQuad.p[1].x,visibleQuad.p[1].y,
					visibleQuad.p[2].x,visibleQuad.p[2].y,
					visibleQuad.p[3].x,visibleQuad.p[3].y);
			}
		}
		catch(PROJECTION_TO_INFINITY e) {
			if(debug) printf("hmm staring at the horizon %d\n",(int)e);
			// use historic code solution
			visibleQuad = this->gameCamera->computeVisibleQuad();
		}
	}
}

// =======================================
// basic rendering
// =======================================

void Renderer::renderMouse2d(int x, int y, int anim, float fade) {
	if(no2DMouseRendering == true) {
			return;
	}
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}
//	float blue=0.0f;
//	float green=0.4f;
	if(game != NULL && game->getGui() != NULL) {
		const Gui *gui=game->getGui();
		const Display *display=gui->getDisplay();
		int downPos= display->getDownSelectedPos();
		if(downPos != Display::invalidPos){
			// in state of doing something
			const Texture2D *texture= display->getDownImage(downPos);
			renderTextureQuad(x+18,y-50,32,32,texture,0.8f);
		}
//		else {
//			// Display current commandtype
//			const Unit *unit=NULL;
//			if(gui->getSelection()->isEmpty()){
//				blue=0.0f;
//				green=0.1f;
//			}
//			else{
//				unit=gui->getSelection()->getFrontUnit();
//				if(unit->getCurrCommand()!=NULL && unit->getCurrCommand()->getCommandType()->getImage()!=NULL){
//					const Texture2D *texture = unit->getCurrCommand()->getCommandType()->getImage();
//					renderTextureQuad(x+18,y-50,32,32,texture,0.2f);
//				}
//			}
//		}

		if(game->isMarkCellMode() == true) {
			const Texture2D *texture= game->getMarkCellTexture();
			renderTextureQuad(x-18,y-50,32,32,texture,0.8f);
		}
		if(game->isUnMarkCellMode() == true) {
			const Texture2D *texture= game->getUnMarkCellTexture();
			renderTextureQuad(x-18,y-50,32,32,texture,0.8f);
		}
	}

	float color1 = 0.0, color2 = 0.0;

	float fadeFactor = fade + 1.f;

	anim= anim * 2 - maxMouse2dAnim;

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


/*
	if(no2DMouseRendering == true) {
		return;
	}
    float color1 = 0.0, color2 = 0.0;

	float fadeFactor = fade + 1.f;

	anim= anim * 2 - maxMouse2dAnim;

    color2= (abs(anim*(int)fadeFactor)/static_cast<float>(maxMouse2dAnim))/2.f+0.4f;
    color1= (abs(anim*(int)fadeFactor)/static_cast<float>(maxMouse2dAnim))/2.f+0.8f;

    glPushAttrib(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_LINE_BIT);
    glEnable(GL_BLEND);

	//inside
	Vec2i vertices[3];
	vertices[0] = Vec2i(x, y);
	vertices[1] = Vec2i(x+20, y-10);
	vertices[2] = Vec2i(x+10, y-20);

	glColor4f(0.4f*fadeFactor, 0.2f*fadeFactor, 0.2f*fadeFactor, 0.5f*fadeFactor);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_INT, 0, &vertices[0]);
	glDrawArrays(GL_TRIANGLES, 0, 3);
    glDisableClientState(GL_VERTEX_ARRAY);

	//border
	vertices[0] = Vec2i(x, y);
	vertices[1] = Vec2i(x+20, y-10);
	vertices[2] = Vec2i(x+10, y-20);

	Vec4f colors[4];
	colors[0] = Vec4f(1.f, 0.2f, 0, color1);
	colors[1] = Vec4f(1.f, 0.4f, 0, color2);
	colors[2] = Vec4f(1.f, 0.4f, 0, color2);

	glLineWidth(2);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_INT, 0, &vertices[0]);
	glEnableClientState(GL_COLOR_ARRAY);
	glColorPointer(4, GL_FLOAT, 0, &colors[0]);
    glDrawArrays(GL_LINE_LOOP, 0, 3);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

    glPopAttrib();
*/
}

void Renderer::renderMouse3d() {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	Config &config= Config::getInstance();
	if(config.getBool("RecordMode","false") == true) {
		return;
	}

	if(game == NULL) {
		char szBuf[1024]="";
		sprintf(szBuf,"In [%s::%s] Line: %d game == NULL",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}
	else if(game->getGui() == NULL) {
		char szBuf[1024]="";
		sprintf(szBuf,"In [%s::%s] Line: %d game->getGui() == NULL",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}
	else if(game->getGui()->getMouse3d() == NULL) {
		char szBuf[1024]="";
		sprintf(szBuf,"In [%s::%s] Line: %d game->getGui()->getMouse3d() == NULL",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}

	const Gui *gui= game->getGui();
	const Mouse3d *mouse3d= gui->getMouse3d();
	const Map *map= game->getWorld()->getMap();
	if(map == NULL) {
		char szBuf[1024]="";
		sprintf(szBuf,"In [%s::%s] Line: %d map == NULL",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}

	assertGl();

	if((mouse3d->isEnabled() || gui->isPlacingBuilding()) && gui->isValidPosObjWorld()) {
		const Vec2i &pos= gui->getPosObjWorld();

		glMatrixMode(GL_MODELVIEW);

		glPushAttrib(GL_CURRENT_BIT | GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_BLEND);
		glDisable(GL_STENCIL_TEST);
		glDepthFunc(GL_LESS);
		glEnable(GL_COLOR_MATERIAL);
		glDepthMask(GL_FALSE);

		if(gui->isPlacingBuilding()) {

			modelRenderer->begin(true, true, false);

			const UnitType *building= gui->getBuilding();
			const Gui *gui= game->getGui();
			renderGhostModel(building, pos, gui->getSelectedFacing());

			modelRenderer->end();

			glDisable(GL_COLOR_MATERIAL);
			glPopAttrib();
		}
		else {
			glPushMatrix();
			Vec3f pos3f= Vec3f(pos.x, map->getCell(pos)->getHeight(), pos.y);
			Vec4f color;
			GLUquadricObj *cilQuadric;
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

			glPopAttrib();
			glPopMatrix();
		}
	}

}

void Renderer::renderBackground(const Texture2D *texture) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	const Metrics &metrics= Metrics::getInstance();

	assertGl();

    glPushAttrib(GL_ENABLE_BIT);

	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);

	renderQuad(0, 0, metrics.getVirtualW(), metrics.getVirtualH(), texture);

	glPopAttrib();

	assertGl();
}

void Renderer::renderTextureQuad(int x, int y, int w, int h, const Texture2D *texture, float alpha,const Vec3f *color) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

    assertGl();

	glPushAttrib(GL_ENABLE_BIT);

	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	if(color != NULL) {
		Vec4f newColor(*color);
		newColor.w = alpha;
		glColor4fv(newColor.ptr());
	}
	else {
		glColor4f(1.f, 1.f, 1.f, alpha);
	}
	renderQuad(x, y, w, h, texture);

	glPopAttrib();

	assertGl();
}

void Renderer::renderConsoleLine3D(int lineIndex, int xPosition, int yPosition, int lineHeight,
								Font3D* font, string stringToHightlight, const ConsoleLineInfo *lineInfo) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	Vec4f fontColor;
	Lang &lang= Lang::getInstance();
	//const Metrics &metrics= Metrics::getInstance();
	FontMetrics *fontMetrics= font->getMetrics();

	if(game != NULL) {
		fontColor = game->getGui()->getDisplay()->getColor();
	}
	else {
		// white shadowed is default ( in the menu for example )
		//fontColor=Vec4f(1.f, 1.f, 1.f, 0.0f);
		fontColor=Vec4f(lineInfo->color.x,lineInfo->color.y,lineInfo->color.z, 0.0f);
	}

	Vec4f defaultFontColor = fontColor;

	if(lineInfo->PlayerIndex >= 0) {
		std::map<int,Texture2D *> &crcPlayerTextureCache = CacheManager::getCachedItem< std::map<int,Texture2D *> >(GameConstants::playerTextureCacheLookupKey);
		Vec3f playerColor = crcPlayerTextureCache[lineInfo->PlayerIndex]->getPixmap()->getPixel3f(0, 0);
		fontColor.x = playerColor.x;
		fontColor.y = playerColor.y;
		fontColor.z = playerColor.z;

		GameNetworkInterface *gameNetInterface = NetworkManager::getInstance().getGameNetworkInterface();
		if(gameNetInterface != NULL && gameNetInterface->getGameSettings() != NULL) {
			const GameSettings *gameSettings = gameNetInterface->getGameSettings();
			string playerName = gameSettings->getNetworkPlayerNameByPlayerIndex(lineInfo->PlayerIndex);
			if(playerName != lineInfo->originalPlayerName && lineInfo->originalPlayerName != "") {
				playerName = lineInfo->originalPlayerName;
			}
			//printf("playerName [%s], line [%s]\n",playerName.c_str(),line.c_str());

			//string headerLine = "*" + playerName + ":";
			//string headerLine = playerName + ": ";
			string headerLine = playerName;
			if(lineInfo->teamMode == true) {
				headerLine += " (" + lang.get("Team") + ")";
			}
			headerLine += ": ";

			if(fontMetrics == NULL) {
				throw megaglest_runtime_error("fontMetrics == NULL");
			}

			renderTextShadow3D(
					headerLine,
					font,
					fontColor,
                	xPosition, lineIndex * lineHeight + yPosition);

			fontColor = defaultFontColor;
			//xPosition += (8 * (playerName.length() + 2));
			// Proper font spacing after username portion of chat text rendering

			//xPosition += (metrics.toVirtualX(fontMetrics->getTextWidth(headerLine)));
			xPosition += fontMetrics->getTextWidth(headerLine);
		}
	}
	else if(lineInfo->originalPlayerName != "") {
        string playerName = lineInfo->originalPlayerName;
        //string headerLine = playerName + ": ";
		string headerLine = playerName;
		if(lineInfo->teamMode == true) {
			headerLine += " (" + lang.get("Team") + ")";
		}
		headerLine += ": ";

        if(fontMetrics == NULL) {
            throw megaglest_runtime_error("fontMetrics == NULL");
        }

        renderTextShadow3D(
                headerLine,
                font,
                fontColor,
                xPosition, lineIndex * lineHeight + yPosition);

        fontColor = defaultFontColor;
        //xPosition += (8 * (playerName.length() + 2));
        // Proper font spacing after username portion of chat text rendering
        //xPosition += (metrics.toVirtualX(fontMetrics->getTextWidth(headerLine)));
        xPosition += fontMetrics->getTextWidth(headerLine);
	}
	else {
		fontColor = defaultFontColor;
	}

	if(stringToHightlight!="" && lineInfo->text.find(stringToHightlight)!=string::npos){
		fontColor=Vec4f(1.f, 0.5f, 0.5f, 0.0f);
	}
	renderTextShadow3D(
			lineInfo->text,
		font,
		fontColor,
        xPosition, (lineIndex * lineHeight) + yPosition);
}

void Renderer::renderConsoleLine(int lineIndex, int xPosition, int yPosition, int lineHeight,
								Font2D* font, string stringToHightlight, const ConsoleLineInfo *lineInfo) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	Vec4f fontColor;
	Lang &lang= Lang::getInstance();

	const Metrics &metrics= Metrics::getInstance();
	FontMetrics *fontMetrics= font->getMetrics();

	if(game != NULL) {
		fontColor = game->getGui()->getDisplay()->getColor();
	}
	else {
		// white shadowed is default ( in the menu for example )
		//fontColor=Vec4f(1.f, 1.f, 1.f, 0.0f);
		fontColor=Vec4f(lineInfo->color.x,lineInfo->color.y,lineInfo->color.z, 0.0f);
	}

	Vec4f defaultFontColor = fontColor;

	if(lineInfo->PlayerIndex >= 0) {
		std::map<int,Texture2D *> &crcPlayerTextureCache = CacheManager::getCachedItem< std::map<int,Texture2D *> >(GameConstants::playerTextureCacheLookupKey);
		Vec3f playerColor = crcPlayerTextureCache[lineInfo->PlayerIndex]->getPixmap()->getPixel3f(0, 0);
		fontColor.x = playerColor.x;
		fontColor.y = playerColor.y;
		fontColor.z = playerColor.z;

		GameNetworkInterface *gameNetInterface = NetworkManager::getInstance().getGameNetworkInterface();
		if(gameNetInterface != NULL && gameNetInterface->getGameSettings() != NULL) {
			const GameSettings *gameSettings = gameNetInterface->getGameSettings();
			string playerName = gameSettings->getNetworkPlayerNameByPlayerIndex(lineInfo->PlayerIndex);
			if(playerName != lineInfo->originalPlayerName && lineInfo->originalPlayerName != "") {
				playerName = lineInfo->originalPlayerName;
			}
			//printf("playerName [%s], line [%s]\n",playerName.c_str(),line.c_str());

			//string headerLine = "*" + playerName + ":";
			//string headerLine = playerName + ": ";
			string headerLine = playerName;
			if(lineInfo->teamMode == true) {
				headerLine += " (" + lang.get("Team") + ")";
			}
			headerLine += ": ";

			if(fontMetrics == NULL) {
				throw megaglest_runtime_error("fontMetrics == NULL");
			}

			renderTextShadow(
					headerLine,
					font,
					fontColor,
                	xPosition, lineIndex * lineHeight + yPosition);

			fontColor = defaultFontColor;
			//xPosition += (8 * (playerName.length() + 2));
			// Proper font spacing after username portion of chat text rendering
			xPosition += (metrics.toVirtualX(fontMetrics->getTextWidth(headerLine)));
		}
	}
	else if(lineInfo->originalPlayerName != "") {
        string playerName = lineInfo->originalPlayerName;
        //string headerLine = playerName + ": ";
        string headerLine = playerName;
		if(lineInfo->teamMode == true) {
			headerLine += " (" + lang.get("Team") + ")";
		}
		headerLine += ": ";

        if(fontMetrics == NULL) {
            throw megaglest_runtime_error("fontMetrics == NULL");
        }

        renderTextShadow(
                headerLine,
                font,
                fontColor,
                xPosition, lineIndex * lineHeight + yPosition);

        fontColor = defaultFontColor;
        //xPosition += (8 * (playerName.length() + 2));
        // Proper font spacing after username portion of chat text rendering
        xPosition += (metrics.toVirtualX(fontMetrics->getTextWidth(headerLine)));
	}
	else {
		fontColor = defaultFontColor;
	}

	if(stringToHightlight!="" && lineInfo->text.find(stringToHightlight)!=string::npos){
		fontColor=Vec4f(1.f, 0.5f, 0.5f, 0.0f);
	}
	renderTextShadow(
			lineInfo->text,
		font,
		fontColor,
        xPosition, (lineIndex * lineHeight) + yPosition);
}

void Renderer::renderConsole(const Console *console,const bool showFullConsole,
		const bool showMenuConsole, int overrideMaxConsoleLines){
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	if(console == NULL) {
		throw megaglest_runtime_error("console == NULL");
	}

	glPushAttrib(GL_ENABLE_BIT);
	glEnable(GL_BLEND);

	if(showFullConsole) {
		for(int i = 0; i < console->getStoredLineCount(); ++i) {
			const ConsoleLineInfo &lineInfo = console->getStoredLineItem(i);
			if(renderText3DEnabled == true) {
				renderConsoleLine3D(i, console->getXPos(), console->getYPos(),
						console->getLineHeight(), console->getFont3D(),
						console->getStringToHighlight(), &lineInfo);
			}
			else {
				renderConsoleLine(i, console->getXPos(), console->getYPos(),
						console->getLineHeight(), console->getFont(),
						console->getStringToHighlight(), &lineInfo);
			}
		}
	}
	else if(showMenuConsole) {
		int allowedMaxLines = (overrideMaxConsoleLines >= 0 ? overrideMaxConsoleLines : maxConsoleLines);
		for(int i = 0; i < console->getStoredLineCount() && i < allowedMaxLines; ++i) {
			const ConsoleLineInfo &lineInfo = console->getStoredLineItem(i);
			if(renderText3DEnabled == true) {
				renderConsoleLine3D(i, console->getXPos(), console->getYPos(),
						console->getLineHeight(), console->getFont3D(), console->getStringToHighlight(), &lineInfo);
			}
			else {
				renderConsoleLine(i, console->getXPos(), console->getYPos(),
						console->getLineHeight(), console->getFont(), console->getStringToHighlight(), &lineInfo);
			}
		}
	}
	else {
		for(int i = 0; i < console->getLineCount(); ++i) {
			const ConsoleLineInfo &lineInfo = console->getLineItem(i);
			if(renderText3DEnabled == true) {
				renderConsoleLine3D(i, console->getXPos(), console->getYPos(),
						console->getLineHeight(), console->getFont3D(), console->getStringToHighlight(), &lineInfo);
			}
			else {
				renderConsoleLine(i, console->getXPos(), console->getYPos(),
						console->getLineHeight(), console->getFont(), console->getStringToHighlight(), &lineInfo);
			}
		}
	}
	glPopAttrib();
}

void Renderer::renderChatManager(const ChatManager *chatManager) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	Vec4f fontColor;
	Lang &lang= Lang::getInstance();

	if(chatManager->getEditEnabled()) {
		string text="";

		if(chatManager->getInMenu()) {
			text += lang.get("Chat");
		}
		else if(chatManager->getTeamMode()) {
			text += lang.get("Team");
		}
		else {
			text += lang.get("All");
		}
		text += ": " + chatManager->getText() + "_";

		if(game != NULL) {
			fontColor = game->getGui()->getDisplay()->getColor();
		}
		else {
			// white shadowed is default ( in the menu for example )
			fontColor=Vec4f(1.f, 1.f, 1.f, 0.0f);
		}

		if(renderText3DEnabled == true) {
			renderTextShadow3D(
					text,
					chatManager->getFont3D(),
					fontColor,
					chatManager->getXPos(), chatManager->getYPos());
		}
		else {
			renderTextShadow(
					text,
					chatManager->getFont(),
					fontColor,
					chatManager->getXPos(), chatManager->getYPos());
		}
	}
	else
	{
		if (chatManager->getInMenu()) {
			string text = ">> "+lang.get("PressEnterToChat")+" <<";
			fontColor = Vec4f(0.5f, 0.5f, 0.5f, 0.5f);

			if(renderText3DEnabled == true) {
				renderTextShadow3D(text, chatManager->getFont3D(), fontColor,
						chatManager->getXPos(), chatManager->getYPos());
			}
			else {
				renderTextShadow(text, chatManager->getFont(), fontColor,
						chatManager->getXPos(), chatManager->getYPos());
			}
		}
	}
}

void Renderer::renderResourceStatus() {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	const Metrics &metrics= Metrics::getInstance();
	const World *world= game->getWorld();

	if(world->getThisFactionIndex() < 0 || world->getThisFactionIndex() >= world->getFactionCount()) {
		return;
	}

	const Faction *thisFaction= world->getFaction(world->getThisFactionIndex());
	const Vec4f fontColor = game->getGui()->getDisplay()->getColor();
	assertGl();

	glPushAttrib(GL_ENABLE_BIT);

	int j= 0;
	for(int i= 0; i < world->getTechTree()->getResourceTypeCount(); ++i) {
		const ResourceType *rt = world->getTechTree()->getResourceType(i);
		const Resource *r = thisFaction->getResource(rt);

		if ( rt->getDisplayInHud() == false ){
			continue;
		}
		//if any unit produces the resource
		bool showResource= false;
		for(int k=0; k < thisFaction->getType()->getUnitTypeCount(); ++k) {
			const UnitType *ut = thisFaction->getType()->getUnitType(k);
			if(ut->getCost(rt) != NULL) {
				showResource = true;
				break;
			}
		}

		//draw resource status
		if(showResource) {
			string str= intToStr(r->getAmount());

			glEnable(GL_TEXTURE_2D);

			Vec4f resourceFontColor = fontColor;

			bool isNegativeConsumableDisplayCycle = false;
			if(rt->getClass() == rcConsumable) {
				// Show in yellow/orange/red font if negative
				if(r->getBalance()*5+r->getAmount()<0){
					if(time(NULL) % 2 == 0) {
						isNegativeConsumableDisplayCycle = true;
						if(r->getBalance()*1+r->getAmount()<0){
							glColor3f(RED.x,RED.y,RED.z);
							resourceFontColor = RED;
						}
						else if(r->getBalance()*3+r->getAmount()<0){
							glColor3f(ORANGE.x,ORANGE.y,ORANGE.z);
							resourceFontColor = ORANGE;
						}
						else if(r->getBalance()*5+r->getAmount()<0){
							glColor3f(YELLOW.x,YELLOW.y,YELLOW.z);
							resourceFontColor = YELLOW;
						}
					}
				}
			}

			if(isNegativeConsumableDisplayCycle == false) {
				glColor3f(1.f, 1.f, 1.f);
			}
			renderQuad(j*100+200, metrics.getVirtualH()-30, 16, 16, rt->getImage());

			if(rt->getClass() != rcStatic) {
				str+= "/" + intToStr(thisFaction->getStoreAmount(rt));
			}
			if(rt->getClass() == rcConsumable) {
				str+= "(";
				if(r->getBalance() > 0) {
					str+= "+";
				}
				str+= intToStr(r->getBalance()) + ")";
			}

			glDisable(GL_TEXTURE_2D);

			if(renderText3DEnabled == true) {
				renderTextShadow3D(
					str, CoreData::getInstance().getDisplayFontSmall3D(),
					resourceFontColor,
					j*100+220, metrics.getVirtualH()-30, false);
			}
			else {
				renderTextShadow(
					str, CoreData::getInstance().getDisplayFontSmall(),
					resourceFontColor,
					j*100+220, metrics.getVirtualH()-30, false);
			}
			++j;
		}
	}

	glPopAttrib();

	assertGl();
}

void Renderer::renderSelectionQuad() {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	Config &config= Config::getInstance();
	if(config.getBool("RecordMode","false") == true) {
		return;
	}

	const Gui *gui= game->getGui();
	const SelectionQuad *sq= gui->getSelectionQuad();

    Vec2i down= sq->getPosDown();
    Vec2i up= sq->getPosUp();

    if(gui->isSelecting()) {
        glPushAttrib(GL_CURRENT_BIT | GL_LINE_BIT);

    	Vec2i vertices[4];
    	vertices[0] = Vec2i(down.x, down.y);
    	vertices[1] = Vec2i(up.x, down.y);
    	vertices[2] = Vec2i(up.x, up.y);
    	vertices[3] = Vec2i(down.x, up.y);

    	glColor3f(0,1,0);
    	glEnableClientState(GL_VERTEX_ARRAY);
    	glVertexPointer(2, GL_INT, 0, &vertices[0]);
        glDrawArrays(GL_LINE_LOOP, 0, 4);
        glDisableClientState(GL_VERTEX_ARRAY);

/*
            glColor3f(0,1,0);
            glBegin(GL_LINE_LOOP);
                glVertex2i(down.x, down.y);
                glVertex2i(up.x, down.y);
                glVertex2i(up.x, up.y);
                glVertex2i(down.x, up.y);
            glEnd();
*/
        glPopAttrib();
    }
}

Vec2i computeCenteredPos(const string &text, Font2D *font, int x, int y) {
	if(font == NULL) {
		//abort();
		throw megaglest_runtime_error("font == NULL (1)");
	}
	const Metrics &metrics= Metrics::getInstance();
	FontMetrics *fontMetrics= font->getMetrics();

	if(fontMetrics == NULL) {
		throw megaglest_runtime_error("fontMetrics == NULL (1)");
	}

	int virtualX = (fontMetrics->getTextWidth(text) > 0 ? static_cast<int>(fontMetrics->getTextWidth(text)/2.f) : 5);
	int virtualY = (fontMetrics->getHeight(text) > 0 ? static_cast<int>(fontMetrics->getHeight(text)/2.f) : 5);

	Vec2i textPos(
		x-metrics.toVirtualX(virtualX),
		y-metrics.toVirtualY(virtualY));

	//printf("text [%s] x = %d y = %d virtualX = %d virtualY = %d fontMetrics->getHeight() = %f\n",text.c_str(),x,y,virtualX,virtualY,fontMetrics->getHeight());

	return textPos;
}

Vec2i computeCenteredPos(const string &text, Font3D *font, int x, int y) {
	if(font == NULL) {
		throw megaglest_runtime_error("font == NULL (2)");
	}
	const Metrics &metrics= Metrics::getInstance();
	FontMetrics *fontMetrics= font->getMetrics();

	if(fontMetrics == NULL) {
		throw megaglest_runtime_error("fontMetrics == NULL (2)");
	}

	int virtualX = (fontMetrics->getTextWidth(text) > 0 ? static_cast<int>(fontMetrics->getTextWidth(text) / 2.f) : 5);
	int virtualY = (fontMetrics->getHeight(text) > 0 ? static_cast<int>(fontMetrics->getHeight(text) / 2.f) : 5);

	Vec2i textPos(
		x-metrics.toVirtualX(virtualX),
		y-metrics.toVirtualY(virtualY));

	return textPos;
}

void Renderer::renderTextSurroundingBox(int x, int y, int w, int h,int maxEditWidth) {
	//glColor4fv(color.ptr());
	//glBegin(GL_QUADS); // Start drawing a quad primitive

	if(maxEditWidth >= 0 && maxEditWidth > w) {
		//printf("B w = %d maxEditWidth = %d\n",w,maxEditWidth);
		w = maxEditWidth;
	}
	//printf("HI!!!\n");
	glPointSize(20.0f);

	int margin = 4;
	//glBegin(GL_POINTS); // Start drawing a point primitive
	glBegin(GL_LINE_LOOP); // Start drawing a line primitive

	glVertex3f(x, y+h, 0.0f); // The bottom left corner
	glVertex3f(x, y-margin, 0.0f); // The top left corner
	glVertex3f(x+w, y-margin, 0.0f); // The top right corner
	glVertex3f(x+w, y+h, 0.0f); // The bottom right corner
	glEnd();
}

void Renderer::renderTextBoundingBox3D(const string &text, Font3D *font,
		float alpha, int x, int y, int w, int h, bool centeredW, bool centeredH,
		bool editModeEnabled,int maxEditWidth) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
	glEnable(GL_BLEND);
	glColor4fv(Vec4f(1.f, 1.f, 1.f, alpha).ptr());

	Vec2f pos= Vec2f(x, y);
	//Vec2i pos= centered? computeCenteredPos(text, font, x, y): Vec2i(x, y);
	if(centeredW == true || centeredH == true) {
		getCentered3DPos(text, font, pos, w, h, centeredW, centeredH);
	}

	if(editModeEnabled) {
		if(maxEditWidth >= 0) {
			string temp = "";
			for(int i = 0; i < maxEditWidth; ++i) {
				temp += DEFAULT_CHAR_FOR_WIDTH_CALC;
			}
			float lineWidth = (font->getTextHandler()->Advance(temp.c_str()) * Font::scaleFontValue);
			maxEditWidth = (int)lineWidth;
		}

		renderTextSurroundingBox(pos.x, pos.y, w, h,maxEditWidth);
	}
	glColor4fv(Vec4f(1.f, 1.f, 1.f, alpha).ptr());
	TextRendererSafeWrapper safeTextRender(textRenderer3D,font);
	//textRenderer3D->begin(font);
	textRenderer3D->render(text, pos.x, pos.y);
	//textRenderer3D->end();
	safeTextRender.end();

	glDisable(GL_BLEND);
	glPopAttrib();
}

void Renderer::renderText3D(const string &text, Font3D *font, float alpha, int x, int y, bool centered) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
	glEnable(GL_BLEND);
	glColor4fv(Vec4f(1.f, 1.f, 1.f, alpha).ptr());

	Vec2i pos= Vec2i(x, y);
	//Vec2i pos= centered? computeCenteredPos(text, font, x, y): Vec2i(x, y);

	//textRenderer3D->begin(font);
	TextRendererSafeWrapper safeTextRender(textRenderer3D,font);
	textRenderer3D->render(text, pos.x, pos.y, centered);
	//textRenderer3D->end();
	safeTextRender.end();

	glDisable(GL_BLEND);
	glPopAttrib();
}

void Renderer::renderText(const string &text, Font2D *font, float alpha, int x, int y, bool centered) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
	glEnable(GL_BLEND);
	glColor4fv(Vec4f(1.f, 1.f, 1.f, alpha).ptr());

	Vec2i pos= centered? computeCenteredPos(text, font, x, y): Vec2i(x, y);

	//textRenderer->begin(font);
	TextRendererSafeWrapper safeTextRender(textRenderer,font);
	textRenderer->render(text, pos.x, pos.y);
	//textRenderer->end();
	safeTextRender.end();

	glPopAttrib();
}

Vec2f Renderer::getCentered3DPos(const string &text, Font3D *font, Vec2f &pos, int w, int h,bool centeredW, bool centeredH) {
	if(centeredW == true) {
		if(font == NULL) {
			//abort();
			throw megaglest_runtime_error("font == NULL (5)");
		}
		else if(font->getTextHandler() == NULL) {
			throw megaglest_runtime_error("font->getTextHandler() == NULL (5)");
		}

		float lineWidth = (font->getTextHandler()->Advance(text.c_str()) * Font::scaleFontValue);
		if(lineWidth < w) {
			pos.x += ((w / 2.f) - (lineWidth / 2.f));
		}
	}

	if(centeredH) {
		if(font == NULL) {
			throw megaglest_runtime_error("font == NULL (6)");
		}
		else if(font->getTextHandler() == NULL) {
			throw megaglest_runtime_error("font->getTextHandler() == NULL (6)");
		}

		//const Metrics &metrics= Metrics::getInstance();
		//float lineHeight = (font->getTextHandler()->LineHeight(text.c_str()) * Font::scaleFontValue);
		float lineHeight = (font->getTextHandler()->LineHeight(text.c_str()) * Font::scaleFontValue);
		//lineHeight=metrics.toVirtualY(lineHeight);
		//lineHeight= lineHeight / (2.f + 0.2f * FontMetrics::DEFAULT_Y_OFFSET_FACTOR);
		//pos.y += (h / 2.f) - (lineHeight / 2.f);
		//pos.y += (h / 2.f) - (lineHeight);
		//pos.y += (lineHeight / 2.f); // y starts at the middle of the render position, so only move up 1/2 the font height

		if(lineHeight < h) {
			//printf("line %d, lineHeight [%f] h [%d] text [%s]\n",__LINE__,lineHeight,h,text.c_str());

			//if(Font::forceFTGLFonts == true) {
				// First go to top of bounding box
				pos.y += (h - lineHeight);
				pos.y -= ((h - lineHeight) / Font::scaleFontValueCenterHFactor);
//			}
//			else {
//				pos.y += (float)(((float)h) / 2.0);
//				float heightGap = (float)(((float)h - lineHeight) / 2.0);
//				pos.y -= heightGap;
//
//				//printf("h = %d lineHeight = %f heightGap = %f\n",h,lineHeight,heightGap);
//
//			// Now calculate till we get text to middle
//			//pos.y -= (realHeight / 2);
//			//pos.y += (lineHeight / 2);
//			}
		}
		else if(lineHeight > h) {
			//printf("line %d, lineHeight [%f] h [%d] text [%s]\n",__LINE__,lineHeight,h,text.c_str());

#ifdef USE_STREFLOP
			pos.y += (float)(streflop::ceil( static_cast<streflop::Simple>( ((float)lineHeight - (float)h)) ));
#else
			pos.y += (ceil(lineHeight - h));
#endif
		}
	}
	return pos;
}

void Renderer::renderTextBoundingBox3D(const string &text, Font3D *font,
		const Vec3f &color, int x, int y, int w, int h, bool centeredW,
		bool centeredH, bool editModeEnabled,int maxEditWidth) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	glPushAttrib(GL_CURRENT_BIT);
	glColor3fv(color.ptr());

	Vec2f pos= Vec2f(x, y);
	//Vec2i pos= centered? computeCenteredPos(text, font, x, y): Vec2i(x, y);

	if(centeredW == true || centeredH == true) {
		getCentered3DPos(text, font, pos, w, h,centeredW,centeredH);
	}

	if(editModeEnabled) {
		if(maxEditWidth >= 0) {
			string temp = "";
			for(int i = 0; i < maxEditWidth; ++i) {
				temp += DEFAULT_CHAR_FOR_WIDTH_CALC;
			}
			float lineWidth = (font->getTextHandler()->Advance(temp.c_str()) * Font::scaleFontValue);
			maxEditWidth = (int)lineWidth;
		}

		renderTextSurroundingBox(pos.x, pos.y, w, h,maxEditWidth);
	}
	glColor3fv(color.ptr());
	//textRenderer3D->begin(font);
	TextRendererSafeWrapper safeTextRender(textRenderer3D,font);
	textRenderer3D->render(text, pos.x, pos.y);
	//textRenderer3D->end();
	safeTextRender.end();

	glPopAttrib();
}

void Renderer::renderText3D(const string &text, Font3D *font, const Vec3f &color, int x, int y, bool centered) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	glPushAttrib(GL_CURRENT_BIT);
	glColor3fv(color.ptr());

	Vec2i pos= Vec2i(x, y);
	//Vec2i pos= centered? computeCenteredPos(text, font, x, y): Vec2i(x, y);

	//textRenderer3D->begin(font);
	TextRendererSafeWrapper safeTextRender(textRenderer3D,font);
	textRenderer3D->render(text, pos.x, pos.y, centered);
	//textRenderer3D->end();
	safeTextRender.end();

	glPopAttrib();
}

void Renderer::renderText(const string &text, Font2D *font, const Vec3f &color, int x, int y, bool centered){
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	glPushAttrib(GL_CURRENT_BIT);
	glColor3fv(color.ptr());

	Vec2i pos= centered? computeCenteredPos(text, font, x, y): Vec2i(x, y);

	//textRenderer->begin(font);
	TextRendererSafeWrapper safeTextRender(textRenderer,font);
	textRenderer->render(text, pos.x, pos.y);
	//textRenderer->end();
	safeTextRender.end();

	glPopAttrib();
}

void Renderer::renderTextBoundingBox3D(const string &text, Font3D *font,
		const Vec4f &color, int x, int y, int w, int h, bool centeredW,
		bool centeredH, bool editModeEnabled,int maxEditWidth) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
	glEnable(GL_BLEND);
	glColor4fv(color.ptr());

	Vec2f pos= Vec2f(x, y);
	//Vec2i pos= centered? computeCenteredPos(text, font, x, y): Vec2i(x, y);

	if(centeredW == true || centeredH == true) {
		getCentered3DPos(text, font, pos, w, h,centeredW,centeredH);
	}

	if(editModeEnabled) {
		if(maxEditWidth >= 0) {
			string temp = "";
			for(int i = 0; i < maxEditWidth; ++i) {
				temp += DEFAULT_CHAR_FOR_WIDTH_CALC;
			}
			float lineWidth = (font->getTextHandler()->Advance(temp.c_str()) * Font::scaleFontValue);
			maxEditWidth = (int)lineWidth;
		}

		renderTextSurroundingBox(pos.x, pos.y, w, h,maxEditWidth);
	}
	glColor4fv(color.ptr());
	//textRenderer3D->begin(font);
	TextRendererSafeWrapper safeTextRender(textRenderer3D,font);
	textRenderer3D->render(text, pos.x, pos.y);
	//textRenderer3D->end();
	safeTextRender.end();

	glDisable(GL_BLEND);
	glPopAttrib();
}

void Renderer::renderText3D(const string &text, Font3D *font, const Vec4f &color, int x, int y, bool centered) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
	glEnable(GL_BLEND);
	glColor4fv(color.ptr());

	Vec2i pos= Vec2i(x, y);
	//Vec2i pos= centered? computeCenteredPos(text, font, x, y): Vec2i(x, y);

	//textRenderer3D->begin(font);
	TextRendererSafeWrapper safeTextRender(textRenderer3D,font);
	textRenderer3D->render(text, pos.x, pos.y, centered);
	//textRenderer3D->end();
	safeTextRender.end();

	glDisable(GL_BLEND);
	glPopAttrib();
}

void Renderer::renderText(const string &text, Font2D *font, const Vec4f &color, int x, int y, bool centered){
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
	glEnable(GL_BLEND);
	glColor4fv(color.ptr());

	Vec2i pos= centered? computeCenteredPos(text, font, x, y): Vec2i(x, y);

	//textRenderer->begin(font);
	TextRendererSafeWrapper safeTextRender(textRenderer,font);
	textRenderer->render(text, pos.x, pos.y);
	//textRenderer->end();
	safeTextRender.end();

	glPopAttrib();
}

void Renderer::renderTextShadow3D(const string &text, Font3D *font,const Vec4f &color, int x, int y, bool centered) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	if(font == NULL) {
		throw megaglest_runtime_error("font == NULL (3)");
	}

	glPushAttrib(GL_CURRENT_BIT);

	Vec2i pos= centered? computeCenteredPos(text, font, x, y): Vec2i(x, y);

	//textRenderer3D->begin(font);
	TextRendererSafeWrapper safeTextRender(textRenderer3D,font);
	if(color.w < 0.5)	{
		glColor3f(0.0f, 0.0f, 0.0f);

		textRenderer3D->render(text, pos.x-1.0f, pos.y-1.0f);
	}
	glColor3f(color.x,color.y,color.z);

	textRenderer3D->render(text, pos.x, pos.y);
	//textRenderer3D->end();
	safeTextRender.end();

	glPopAttrib();
}

void Renderer::renderTextShadow(const string &text, Font2D *font,const Vec4f &color, int x, int y, bool centered){
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	if(font == NULL) {
		throw megaglest_runtime_error("font == NULL (4)");
	}

	glPushAttrib(GL_CURRENT_BIT);

	Vec2i pos= centered? computeCenteredPos(text, font, x, y): Vec2i(x, y);

	//textRenderer->begin(font);
	TextRendererSafeWrapper safeTextRender(textRenderer,font);
	if(color.w < 0.5)	{
		glColor3f(0.0f, 0.0f, 0.0f);

		textRenderer->render(text, pos.x-1.0f, pos.y-1.0f);
	}
	glColor3f(color.x,color.y,color.z);

	textRenderer->render(text, pos.x, pos.y);
	//textRenderer->end();
	safeTextRender.end();

	glPopAttrib();
}

// ============= COMPONENTS =============================

void Renderer::renderLabel(GraphicLabel *label) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	Vec3f labelColor=label->getTextColor();
	Vec4f colorWithAlpha = Vec4f(labelColor.x,labelColor.y,labelColor.z,GraphicComponent::getFade());
	renderLabel(label,&colorWithAlpha);
}

void Renderer::renderLabel(GraphicLabel *label,const Vec3f *color) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	if(color != NULL) {
		Vec4f colorWithAlpha = Vec4f(*color);
		colorWithAlpha.w = GraphicComponent::getFade();
		renderLabel(label,&colorWithAlpha);
	}
	else {
		Vec4f *colorWithAlpha = NULL;
		renderLabel(label,colorWithAlpha);
	}
}

void Renderer::renderLabel(GraphicLabel *label,const Vec4f *color) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	if(label->getVisible() == false) {
		return;
	}
	glPushAttrib(GL_ENABLE_BIT);
	glEnable(GL_BLEND);

	vector<string> lines;
	if(label->getWordWrap() == true) {
		Tokenize(label->getText(),lines,"\n");
	}
	else {
		lines.push_back(label->getText());
	}

	for(unsigned int i = 0; i < lines.size(); ++i) {
		Vec2i textPos;
		int x= label->getX();
		int y= label->getY() - (i * label->getH());
		int h= label->getH();
		int w= label->getW();
		//if(label->getInstanceName() == "modDescrLabel") printf("~~~ lines.size() [%u] i = %d lines[i] [%s] y = %d\n",lines.size(),i,lines[i].c_str(),y);

		if(label->getCentered()) {
			textPos= Vec2i(x+w/2, y+h/2);
		}
		else {
			textPos= Vec2i(x, y+h/4);
		}

		if(color != NULL) {
			if(renderText3DEnabled == true) {
				//renderText3D(lines[i], label->getFont3D(), (*color), textPos.x, textPos.y, label->getCentered());
				//printf("Text Render3D [%s] font3d [%p]\n",lines[i].c_str(),label->getFont3D());
				//printf("Label render C\n");
				renderTextBoundingBox3D(lines[i], label->getFont3D(), (*color),
						x, y, w, h, label->getCenteredW(),label->getCenteredH(),
						label->getEditModeEnabled(),label->getMaxEditWidth());
			}
			else {
				//printf("Label render D\n");
				renderText(lines[i], label->getFont(), (*color), textPos.x, textPos.y, label->getCentered());
			}
		}
		else {
			if(renderText3DEnabled == true) {
				//renderText3D(lines[i], label->getFont3D(), GraphicComponent::getFade(), textPos.x, textPos.y, label->getCentered());
				//printf("Text Render3D [%s] font3d [%p]\n",lines[i].c_str(),label->getFont3D());
				//printf("Label render E\n");
				renderTextBoundingBox3D(lines[i], label->getFont3D(),
						GraphicComponent::getFade(), x, y, w, h,
						label->getCenteredW(),label->getCenteredH(),
						label->getEditModeEnabled(),label->getMaxEditWidth());
			}
			else {
				//printf("Label render F\n");
				renderText(lines[i], label->getFont(), GraphicComponent::getFade(), textPos.x, textPos.y, label->getCentered());
			}
		}
	}
	glPopAttrib();
}

void Renderer::renderButton(GraphicButton *button, const Vec4f *fontColorOverride, bool *lightedOverride) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

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
	Texture2D *backTexture = NULL;

	if(button->getUseCustomTexture() == true) {
		backTexture = dynamic_cast<Texture2D *>(button->getCustomTexture());
	}
	else {
		backTexture = w > 3 * h / 2 ? coreData.getButtonBigTexture(): coreData.getButtonSmallTexture();
	}

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	if(backTexture != NULL) {
		glBindTexture(GL_TEXTURE_2D, static_cast<Texture2DGl*>(backTexture)->getHandle());
	}
	else {
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	//button
	Vec4f fontColor(GraphicComponent::getCustomTextColor());

	if(fontColorOverride != NULL) {
		fontColor= *fontColorOverride;
	}
	else {
		// white shadowed is default ( in the menu for example )
		fontColor.w = GraphicComponent::getFade();
	}

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

	bool renderLighted = (button->getLighted() && button->getEditable());
	if(lightedOverride != NULL) {
		renderLighted = *lightedOverride;
	}
	if(renderLighted) {
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

	//Vec2i textPos= Vec2i(x + w / 2, y + h / 2);

	if(button->getEditable()) {
		if(renderText3DEnabled == true) {
			//renderText3D(button->getText(), button->getFont3D(), color,x + (w / 2), y + (h / 2), true);
			renderTextBoundingBox3D(button->getText(), button->getFont3D(),
					color, x, y, w, h, true, true,false,-1);
		}
		else {
			renderText(button->getText(), button->getFont(), color,x + (w / 2), y + (h / 2), true);
		}
	}
	else {
		if(renderText3DEnabled == true) {
			//renderText3D(button->getText(), button->getFont3D(),disabledTextColor,
			//       x + (w / 2), y + (h / 2), true);
			renderTextBoundingBox3D(button->getText(), button->getFont3D(),disabledTextColor,
						       x, y, w, h, true, true,false,-1);
		}
		else {
			renderText(button->getText(), button->getFont(),disabledTextColor,
			       x + (w / 2), y + (h / 2), true);
		}
	}

    glPopAttrib();
}

void Renderer::renderCheckBox(const GraphicCheckBox *box) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	if(box->getVisible() == false) {
		return;
	}

    int x= box->getX();
    int y= box->getY();
    int h= box->getH();
    int w= box->getW();

	const Vec3f disabledTextColor= Vec3f(0.25f,0.25f,0.25f);

	glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT);

	//background
	CoreData &coreData= CoreData::getInstance();
	Texture2D *backTexture= box->getValue()? coreData.getCheckedCheckBoxTexture(): coreData.getCheckBoxTexture();

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	glBindTexture(GL_TEXTURE_2D, static_cast<Texture2DGl*>(backTexture)->getHandle());

	//box
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
	if(anim > 0.5f) {
		anim = 1.f - anim;
	}

	if(box->getLighted() && box->getEditable()) {
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

    glPopAttrib();
}


void Renderer::renderLine(const GraphicLine *line) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	if(line->getVisible() == false) {
		return;
	}

    int x= line->getX();
    int y= line->getY();
    int h= line->getH();
    int w= line->getW();

	const Vec3f disabledTextColor= Vec3f(0.25f,0.25f,0.25f);

	glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT);

	//background
	CoreData &coreData= CoreData::getInstance();
	Texture2D *backTexture= line->getHorizontal()? coreData.getHorizontalLineTexture(): coreData.getVerticalLineTexture();

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	glBindTexture(GL_TEXTURE_2D, static_cast<Texture2DGl*>(backTexture)->getHandle());

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
    glPopAttrib();
}

void Renderer::renderScrollBar(const GraphicScrollBar *sb) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	if(sb->getVisible() == false) {
		return;
	}

    int x= sb->getX();
    int y= sb->getY();
    int h= sb->getH();
    int w= sb->getW();

	const Vec3f disabledTextColor= Vec3f(0.25f,0.25f,0.25f);

	glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT);
	/////////////////////
	//background
	////////////////////
	CoreData &coreData= CoreData::getInstance();
	Texture2D *backTexture= coreData.getHorizontalLineTexture();

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	glBindTexture(GL_TEXTURE_2D, static_cast<Texture2DGl*>(backTexture)->getHandle());

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

	////////////////////
	// selectBlock
	////////////////////

    x= sb->getX();
    y= sb->getY();
    h= sb->getH();
    w= sb->getW();

    if( sb->getHorizontal()) {
    	x=x+sb->getVisibleCompPosStart();
    	w=sb->getVisibleCompPosEnd()-sb->getVisibleCompPosStart();
    }
    else {
    	y=y+sb->getVisibleCompPosStart();
    	h=sb->getVisibleCompPosEnd()-sb->getVisibleCompPosStart();
    }

	Texture2D *selectTexture= coreData.getButtonBigTexture();
	assert(selectTexture != NULL);

	glBindTexture(GL_TEXTURE_2D, static_cast<Texture2DGl*>(selectTexture)->getHandle());

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

	if(sb->getLighted() && sb->getEditable()){
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

    glPopAttrib();
}

void Renderer::renderListBox(GraphicListBox *listBox) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	if(listBox->getVisible() == false) {
		return;
	}

	renderButton(listBox->getButton1());
    renderButton(listBox->getButton2());

	glPushAttrib(GL_ENABLE_BIT);
	glEnable(GL_BLEND);

	GraphicLabel label;
	label.init(listBox->getX(), listBox->getY(), listBox->getW(), listBox->getH(), true,listBox->getTextColor());
	label.setText(listBox->getText());
	label.setFont(listBox->getFont());
	label.setFont3D(listBox->getFont3D());
	renderLabel(&label);


	//lighting

		bool renderLighted= (listBox->getLighted());


		if(renderLighted) {
			float anim= GraphicComponent::getAnim();
			if(anim>0.5f) anim= 1.f-anim;

			Vec3f color=listBox->getTextColor();
		    int x= listBox->getX()+listBox->getButton1()->getW();
		    int y= listBox->getY();
		    int h= listBox->getH();
		    int w= listBox->getW()-listBox->getButton1()->getW()-listBox->getButton2()->getW();

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

	glPopAttrib();
}

void Renderer::renderMessageBox(GraphicMessageBox *messageBox) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	try {
		if(messageBox->getVisible() == false) {
			return;
		}

		if((renderText3DEnabled == false && messageBox->getFont() == NULL) ||
		   (renderText3DEnabled == true && messageBox->getFont3D() == NULL)) {
			messageBox->setFont(CoreData::getInstance().getMenuFontNormal());
			messageBox->setFont3D(CoreData::getInstance().getMenuFontNormal3D());
		}
		//background
		glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
		glEnable(GL_BLEND);

		glColor4f(0.0f, 0.0f, 0.0f, 0.8f) ;
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
		for(int i=0; i<messageBox->getButtonCount();i++){

			if((renderText3DEnabled == false && messageBox->getButton(i)->getFont() == NULL) ||
			   (renderText3DEnabled == true && messageBox->getButton(i)->getFont3D() == NULL)) {
				messageBox->getButton(i)->setFont(CoreData::getInstance().getMenuFontNormal());
				messageBox->getButton(i)->setFont3D(CoreData::getInstance().getMenuFontNormal3D());
			}

			renderButton(messageBox->getButton(i));
		}

		Vec4f fontColor;
		//if(game!=NULL){
		//	fontColor=game->getGui()->getDisplay()->getColor();
		//}
		//else {
			// white shadowed is default ( in the menu for example )
			fontColor=Vec4f(1.f, 1.f, 1.f, 1.0f);
		//}

		if(renderText3DEnabled == true) {
			//text
			renderTextShadow3D(
				messageBox->getText(), messageBox->getFont3D(), fontColor,
				messageBox->getX()+15, messageBox->getY()+7*messageBox->getH()/10,
				false );

			renderTextShadow3D(
				messageBox->getHeader(), messageBox->getFont3D(),fontColor,
				messageBox->getX()+15, messageBox->getY()+93*messageBox->getH()/100,
				false );

		}
		else {
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
	}
	catch(const exception &e) {
		char szBuf[8096]="";
		sprintf(szBuf,"In [%s::%s Line: %d]\nError [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,e.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,szBuf);

		throw megaglest_runtime_error(szBuf);
	}
}

// ==================== complex rendering ====================

VisibleQuadContainerVBOCache * Renderer::GetSurfaceVBOs(SurfaceData *cellData) {
	std::map<uint32,VisibleQuadContainerVBOCache >::iterator iterFind = mapSurfaceVBOCache.find(cellData->uniqueId);
	if(iterFind == mapSurfaceVBOCache.end()) {
		Vec2f *texCoords		= &cellData->texCoords[0];
		Vec2f *texCoordsSurface	= &cellData->texCoordsSurface[0];
		Vec3f *vertices			= &cellData->vertices[0];
		Vec3f *normals			= &cellData->normals[0];

		VisibleQuadContainerVBOCache vboCache;

		// Generate And Bind The Vertex Buffer
		glGenBuffersARB( 1, (GLuint*)&vboCache.m_nVBOVertices );					// Get A Valid Name
		glBindBufferARB( GL_ARRAY_BUFFER_ARB, vboCache.m_nVBOVertices );			// Bind The Buffer
		// Load The Data
		glBufferDataARB( GL_ARRAY_BUFFER_ARB,  sizeof(Vec3f) * cellData->bufferCount, vertices, GL_STATIC_DRAW_ARB );
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

		assertGl();
		// Generate And Bind The Texture Coordinate Buffer
		glGenBuffersARB( 1, (GLuint*)&vboCache.m_nVBOFowTexCoords );					// Get A Valid Name
		glBindBufferARB( GL_ARRAY_BUFFER_ARB, vboCache.m_nVBOFowTexCoords );		// Bind The Buffer
		// Load The Data
		glBufferDataARB( GL_ARRAY_BUFFER_ARB, sizeof(Vec2f) * cellData->bufferCount, texCoords, GL_STATIC_DRAW_ARB );
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

		assertGl();
		// Generate And Bind The Texture Coordinate Buffer
		glGenBuffersARB( 1, (GLuint*)&vboCache.m_nVBOSurfaceTexCoords );					// Get A Valid Name
		glBindBufferARB( GL_ARRAY_BUFFER_ARB, vboCache.m_nVBOSurfaceTexCoords );		// Bind The Buffer
		// Load The Data
		glBufferDataARB( GL_ARRAY_BUFFER_ARB, sizeof(Vec2f) * cellData->bufferCount, texCoordsSurface, GL_STATIC_DRAW_ARB );
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

		assertGl();
		// Generate And Bind The Normal Buffer
		glGenBuffersARB( 1, (GLuint*)&vboCache.m_nVBONormals );					// Get A Valid Name
		glBindBufferARB( GL_ARRAY_BUFFER_ARB, vboCache.m_nVBONormals );			// Bind The Buffer
		// Load The Data
		glBufferDataARB( GL_ARRAY_BUFFER_ARB,  sizeof(Vec3f) * cellData->bufferCount, normals, GL_STATIC_DRAW_ARB );
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

		vboCache.hasBuiltVBOs = true;

		mapSurfaceVBOCache[cellData->uniqueId] = vboCache;

		// don't need the data in computer RAM anymore its in the GPU now
		cellData->texCoords.clear();
		cellData->texCoordsSurface.clear();
		cellData->vertices.clear();
		cellData->normals.clear();
	}

	return &mapSurfaceVBOCache[cellData->uniqueId];
}

void Renderer::ReleaseSurfaceVBOs() {
	for(std::map<uint32,VisibleQuadContainerVBOCache>::iterator iterFind = mapSurfaceVBOCache.begin();
			iterFind != mapSurfaceVBOCache.end(); ++iterFind) {

		VisibleQuadContainerVBOCache &item = iterFind->second;
		if(item.hasBuiltVBOs == true) {
			glDeleteBuffersARB( 1, (GLuint*)&item.m_nVBOVertices );					// Get A Valid Name
			glDeleteBuffersARB( 1, (GLuint*)&item.m_nVBOFowTexCoords );					// Get A Valid Name
			glDeleteBuffersARB( 1, (GLuint*)&item.m_nVBOSurfaceTexCoords );					// Get A Valid Name
			glDeleteBuffersARB( 1, (GLuint*)&item.m_nVBONormals );					// Get A Valid Name
			//glDeleteBuffersARB( 1, &item.m_nVBOIndexes );					// Get A Valid Name
		}
	}

	mapSurfaceVBOCache.clear();
}

Renderer::MapRenderer::Layer::~Layer() {
	if(vbo_vertices) glDeleteBuffersARB(1,&vbo_vertices);
	if(vbo_normals) glDeleteBuffersARB(1,&vbo_normals);
	if(vbo_fowTexCoords) glDeleteBuffersARB(1,&vbo_fowTexCoords);
	if(vbo_surfTexCoords) glDeleteBuffersARB(1,&vbo_surfTexCoords);
	if(vbo_indices) glDeleteBuffersARB(1,&vbo_indices);
	
}

template<typename T> void _loadVBO(GLuint &vbo,std::vector<T> buf,int target=GL_ARRAY_BUFFER_ARB) {
	assert(buf.size());
	if(true /* vbo enabled? */) {
		glGenBuffersARB(1,&vbo);
		assert(vbo);
		glBindBufferARB(target,vbo);
		glBufferDataARB(target,sizeof(T)*buf.size(),&buf[0],GL_STATIC_DRAW_ARB);
		glBindBufferARB(target,0);
		assertGl();
		buf.clear();
	}
}

void Renderer::MapRenderer::Layer::load_vbos(bool vboEnabled) {
	indexCount = indices.size();
	if(vboEnabled) {
		_loadVBO(vbo_vertices,vertices);
		_loadVBO(vbo_normals,normals);
		_loadVBO(vbo_fowTexCoords,fowTexCoords);
		_loadVBO(vbo_surfTexCoords,surfTexCoords);

		_loadVBO(vbo_indices,indices,GL_ELEMENT_ARRAY_BUFFER_ARB);
	}
	else {
		vbo_vertices = 0;
		vbo_normals = 0;
		vbo_fowTexCoords = 0;
		vbo_surfTexCoords = 0;
		vbo_indices = 0;
	}
}

//int32 CalculatePixelsCRC(const Texture2DGl *texture) {
//	const uint8 *pixels = static_cast<const Pixmap2D *>(texture->getPixmapConst())->getPixels();
//	uint64 pixelByteCount = static_cast<const Pixmap2D *>(texture->getPixmapConst())->getPixelByteCount();
//	Checksum crc;
//	for(uint64 i = 0; i < pixelByteCount; ++i) {
//		crc.addByte(pixels[i]);
//	}
//
//	return crc.getSum();
//}

void Renderer::MapRenderer::loadVisibleLayers(float coordStep,VisibleQuadContainerCache &qCache) {
	int totalCellCount = 0;
	// we create a layer for each visible texture in the map
	for(int visibleIndex = 0;
			visibleIndex < qCache.visibleScaledCellList.size(); ++visibleIndex) {
		Vec2i &pos = qCache.visibleScaledCellList[visibleIndex];

		totalCellCount++;

		SurfaceCell *tc00= map->getSurfaceCell(pos.x, pos.y);
		SurfaceCell *tc10= map->getSurfaceCell(pos.x+1, pos.y);
		SurfaceCell *tc01= map->getSurfaceCell(pos.x, pos.y+1);
		SurfaceCell *tc11= map->getSurfaceCell(pos.x+1, pos.y+1);

		const Vec2f &surfCoord= tc00->getSurfTexCoord();

		SurfaceCell *tc[4] = {
				tc00,
				tc10,
				tc01,
				tc11
		};
		int textureHandle = static_cast<const Texture2DGl*>(tc[0]->getSurfaceTexture())->getHandle();
		string texturePath = static_cast<const Texture2DGl*>(tc[0]->getSurfaceTexture())->getPath();
		//int32 textureCRC = CalculatePixelsCRC(static_cast<const Texture2DGl*>(tc[0]->getSurfaceTexture()));
		Layer* layer = NULL;
		for(Layers::iterator it= layers.begin(); it!= layers.end(); ++it) {
			if((*it)->textureHandle == textureHandle) {
			//if((*it)->texturePath == texturePath) {
			//if((*it)->textureCRC == textureCRC) {
				layer = *it;
				break;
			}
		}
		if(!layer) {
			layer = new Layer(textureHandle);
			layer->texturePath = texturePath;
			//layer->textureCRC = textureCRC;
			layers.push_back(layer);

			//printf("Ading new unique texture [%s]\n",texturePath.c_str());
		}
		// we'll be super-lazy and re-emit all four corners just because its easier
		int index[4];
		int loopIndexes[4] = { 2,0,3,1 };
		for(int i=0; i < 4; i++) {
			index[i] = layer->vertices.size();
			SurfaceCell *corner = tc[loopIndexes[i]];

			layer->vertices.push_back(corner->getVertex());
			layer->normals.push_back(corner->getNormal());
			layer->fowTexCoords.push_back(corner->getFowTexCoord());
		}

		layer->surfTexCoords.push_back(Vec2f(surfCoord.x, surfCoord.y + coordStep));
		layer->surfTexCoords.push_back(Vec2f(surfCoord.x, surfCoord.y));
		layer->surfTexCoords.push_back(Vec2f(surfCoord.x+coordStep, surfCoord.y+coordStep));
		layer->surfTexCoords.push_back(Vec2f(surfCoord.x+coordStep, surfCoord.y));

		// and make two triangles (no strip, we may be disjoint)
		layer->indices.push_back(index[0]);
		layer->indices.push_back(index[1]);
		layer->indices.push_back(index[2]);
		layer->indices.push_back(index[1]);
		layer->indices.push_back(index[3]);
		layer->indices.push_back(index[2]);

	}
	// turn them into vbos (actually this method will just calc the index count)
	for(Layers::iterator layer= layers.begin(); layer!= layers.end(); ++layer){
		(*layer)->load_vbos(false);
	}

	//printf("Total # of layers for this map = %d totalCellCount = %d overall render reduction ratio = %d times\n",layers.size(),totalCellCount,(totalCellCount / layers.size()));
}

void Renderer::MapRenderer::load(float coordStep) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	int totalCellCount = 0;
	// we create a layer for each texture in the map
	for(int y=0; y<map->getSurfaceH()-1; y++) {
		for(int x=0; x<map->getSurfaceW()-1; x++) {
			totalCellCount++;

			SurfaceCell *tc[4] = {
				map->getSurfaceCell(x,y),
				map->getSurfaceCell(x+1,y),
				map->getSurfaceCell(x,y+1),
				map->getSurfaceCell(x+1,y+1)
			};
			int textureHandle = static_cast<const Texture2DGl*>(tc[0]->getSurfaceTexture())->getHandle();
			string texturePath = static_cast<const Texture2DGl*>(tc[0]->getSurfaceTexture())->getPath();
			//int32 textureCRC = CalculatePixelsCRC(static_cast<const Texture2DGl*>(tc[0]->getSurfaceTexture()));
			Layer* layer = NULL;
			for(Layers::iterator it= layers.begin(); it!= layers.end(); ++it) {
				if((*it)->textureHandle == textureHandle) {
				//if((*it)->texturePath == texturePath) {
				//if((*it)->textureCRC == textureCRC) {
					layer = *it;
					break;
				}
			}
			if(!layer) {
				layer = new Layer(textureHandle);
				layer->texturePath = texturePath;
				//layer->textureCRC = textureCRC;
				layers.push_back(layer);

				//printf("Ading new unique texture [%s]\n",texturePath.c_str());
			}
			// we'll be super-lazy and re-emit all four corners just because its easier
			int index[4];
			int loopIndexes[4] = { 2,0,3,1 };
			for(int i=0; i < 4; i++) {
				index[i] = layer->vertices.size();
				SurfaceCell *corner = tc[loopIndexes[i]];
				layer->vertices.push_back(corner->getVertex());
				layer->normals.push_back(corner->getNormal());
			}

			// the texture coords are all on the current texture obviously
			layer->fowTexCoords.push_back(tc[loopIndexes[0]]->getFowTexCoord());
			layer->fowTexCoords.push_back(tc[loopIndexes[1]]->getFowTexCoord());
			layer->fowTexCoords.push_back(tc[loopIndexes[2]]->getFowTexCoord());
			layer->fowTexCoords.push_back(tc[loopIndexes[3]]->getFowTexCoord());

			layer->surfTexCoords.push_back(tc[0]->getSurfTexCoord()+Vec2f(0,coordStep));
			layer->surfTexCoords.push_back(tc[0]->getSurfTexCoord()+Vec2f(0,0));
			layer->surfTexCoords.push_back(tc[0]->getSurfTexCoord()+Vec2f(coordStep,coordStep));
			layer->surfTexCoords.push_back(tc[0]->getSurfTexCoord()+Vec2f(coordStep,0));

			layer->cellToIndicesMap[Vec2i(x,y)] = layer->indices.size();

			// and make two triangles (no strip, we may be disjoint)
			layer->indices.push_back(index[0]);
			layer->indices.push_back(index[1]);
			layer->indices.push_back(index[2]);
			layer->indices.push_back(index[1]);
			layer->indices.push_back(index[3]);
			layer->indices.push_back(index[2]);
		}
	}
	// turn them into vbos
	for(Layers::iterator layer= layers.begin(); layer!= layers.end(); ++layer){
		(*layer)->load_vbos(true);
	}

	//printf("Total # of layers for this map = %d totalCellCount = %d overall render reduction ratio = %d times\n",layers.size(),totalCellCount,(totalCellCount / layers.size()));
}

template<typename T> void* _bindVBO(GLuint vbo,std::vector<T> buf,int target=GL_ARRAY_BUFFER_ARB) {
	if(vbo) {
		glBindBuffer(target,vbo);
		return NULL;
	} else {
		return &buf[0];
	}
}

void Renderer::MapRenderer::Layer::renderVisibleLayer() {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	//glBindTexture(GL_TEXTURE_2D, static_cast<const Texture2DGl*>(fowTex)->getHandle());
	glClientActiveTexture(Renderer::fowTexUnit);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0,&fowTexCoords[0]);

	glBindTexture(GL_TEXTURE_2D, textureHandle);
	glClientActiveTexture(Renderer::baseTexUnit);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, &surfTexCoords[0]);

	glVertexPointer(3, GL_FLOAT, 0, &vertices[0]);
	glNormalPointer(GL_FLOAT, 0, &normals[0]);

	//glDrawArrays(GL_TRIANGLE_STRIP, 0, vertices.size());
	//unsigned short faceIndices[4] = {0, 1, 2, 3};
	//glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, &faceIndices[0]);
	glDrawElements(GL_TRIANGLES,indexCount,GL_UNSIGNED_INT,&indices[0]);

	glClientActiveTexture(Renderer::fowTexUnit);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glClientActiveTexture(Renderer::baseTexUnit);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);




//	glVertexPointer(3,GL_FLOAT,0,_bindVBO(vbo_vertices,vertices));
//	glNormalPointer(GL_FLOAT,0,_bindVBO(vbo_normals,normals));
//
//	glClientActiveTexture(Renderer::fowTexUnit);
//	glTexCoordPointer(2,GL_FLOAT,0,_bindVBO(vbo_fowTexCoords,fowTexCoords));
//
//	glClientActiveTexture(Renderer::baseTexUnit);
//	glBindTexture(GL_TEXTURE_2D,textureHandle);
//	glTexCoordPointer(2,GL_FLOAT,0,_bindVBO(vbo_surfTexCoords,surfTexCoords));
//
//	//glDrawElements(GL_TRIANGLES,indexCount,GL_UNSIGNED_INT,_bindVBO(vbo_indices,indices,GL_ELEMENT_ARRAY_BUFFER_ARB));
//	glDrawArrays(GL_TRIANGLE_STRIP, 0, vertices.size());
//	//unsigned short faceIndices[4] = {0, 1, 2, 3};
//	//glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, &faceIndices[0]);

}

void Renderer::MapRenderer::Layer::render(VisibleQuadContainerCache &qCache) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	const bool renderOnlyVisibleQuad = true;

	if(renderOnlyVisibleQuad == true) {
		vector<pair<int,int> > rowsToRender;

		if(rowsToRenderCache.find(qCache.lastVisibleQuad) != rowsToRenderCache.end()) {
			rowsToRender = rowsToRenderCache[qCache.lastVisibleQuad];
		}
		else {
			int startIndex = -1;
			int lastValidIndex = -1;

			for(int visibleIndex = 0;
					visibleIndex < qCache.visibleScaledCellList.size(); ++visibleIndex) {
				Vec2i &pos = qCache.visibleScaledCellList[visibleIndex];

				if(cellToIndicesMap.find(pos) != cellToIndicesMap.end()) {
					//printf("Layer Render, visibleindex = %d pos [%s] cellToIndicesMap[pos] = %d lastValidIndex = %d\n",visibleIndex,pos.getString().c_str(),cellToIndicesMap[pos],lastValidIndex);

					if(startIndex < 0 || cellToIndicesMap[pos] == lastValidIndex + 6) {
						lastValidIndex = cellToIndicesMap[pos];
						if(startIndex < 0) {
							startIndex = lastValidIndex;
						}
					}
					else if(startIndex >= 0) {
						rowsToRender.push_back(make_pair(startIndex,lastValidIndex));

						lastValidIndex = cellToIndicesMap[pos];
						startIndex = lastValidIndex;
					}
				}
			}
			if(startIndex >= 0) {
				rowsToRender.push_back(make_pair(startIndex,lastValidIndex));
			}

			rowsToRenderCache[qCache.lastVisibleQuad] = rowsToRender;
		}

		if(rowsToRender.empty() == false) {
			//printf("Layer has %d rows in visible quad, visible quad has %d cells\n",rowsToRender.size(),qCache.visibleScaledCellList.size());

			glVertexPointer(3,GL_FLOAT,0,_bindVBO(vbo_vertices,vertices));
			glNormalPointer(GL_FLOAT,0,_bindVBO(vbo_normals,normals));

			glClientActiveTexture(Renderer::fowTexUnit);
			glTexCoordPointer(2,GL_FLOAT,0,_bindVBO(vbo_fowTexCoords,fowTexCoords));

			glClientActiveTexture(Renderer::baseTexUnit);
			glBindTexture(GL_TEXTURE_2D,textureHandle);
			glTexCoordPointer(2,GL_FLOAT,0,_bindVBO(vbo_surfTexCoords,surfTexCoords));

			for(unsigned int i = 0; i < rowsToRender.size(); ++i) {
				//glDrawElements(GL_TRIANGLES,indexCount,GL_UNSIGNED_INT,_bindVBO(vbo_indices,indices,GL_ELEMENT_ARRAY_BUFFER_ARB));
				glDrawRangeElements(GL_TRIANGLES,rowsToRender[i].first,rowsToRender[i].second,indexCount,GL_UNSIGNED_INT,_bindVBO(vbo_indices,indices,GL_ELEMENT_ARRAY_BUFFER_ARB));
			}
		}
	}
	else {
		glVertexPointer(3,GL_FLOAT,0,_bindVBO(vbo_vertices,vertices));
		glNormalPointer(GL_FLOAT,0,_bindVBO(vbo_normals,normals));

		glClientActiveTexture(Renderer::fowTexUnit);
		glTexCoordPointer(2,GL_FLOAT,0,_bindVBO(vbo_fowTexCoords,fowTexCoords));

		glClientActiveTexture(Renderer::baseTexUnit);
		glBindTexture(GL_TEXTURE_2D,textureHandle);
		glTexCoordPointer(2,GL_FLOAT,0,_bindVBO(vbo_surfTexCoords,surfTexCoords));

		glDrawElements(GL_TRIANGLES,indexCount,GL_UNSIGNED_INT,_bindVBO(vbo_indices,indices,GL_ELEMENT_ARRAY_BUFFER_ARB));
	}
}

void Renderer::MapRenderer::renderVisibleLayers(const Map* map,float coordStep,VisibleQuadContainerCache &qCache) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	if(map != this->map) {
		//printf("New Map loading\n");
		destroy(); // clear any previous map data
		this->map = map;
		loadVisibleLayers(coordStep,qCache);
	}
	else if(lastVisibleQuad != qCache.lastVisibleQuad) {
		//printf("New Visible Quad loading\n");
		destroy(); // clear any previous map data
		this->map = map;
		loadVisibleLayers(coordStep,qCache);
	}

	lastVisibleQuad = qCache.lastVisibleQuad;
	//printf("About to render %d layers\n",layers.size());

	glClientActiveTexture(fowTexUnit);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glClientActiveTexture(baseTexUnit);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	for(Layers::iterator layer= layers.begin(); layer!= layers.end(); ++layer)
		(*layer)->renderVisibleLayer();
	glDisableClientState(GL_VERTEX_ARRAY);
	glBindBuffer(GL_ARRAY_BUFFER_ARB,0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB,0);
	glDisableClientState(GL_NORMAL_ARRAY);
	glClientActiveTexture(fowTexUnit);
	glBindTexture(GL_TEXTURE_2D,0);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glClientActiveTexture(baseTexUnit);
	glBindTexture(GL_TEXTURE_2D,0);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	assertGl();
}

void Renderer::MapRenderer::render(const Map* map,float coordStep,VisibleQuadContainerCache &qCache) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	if(map != this->map) {
		destroy(); // clear any previous map data
		this->map = map;
		load(coordStep);
	}

	//printf("About to render %d layers\n",layers.size());

	glClientActiveTexture(fowTexUnit);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glClientActiveTexture(baseTexUnit);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	for(Layers::iterator layer= layers.begin(); layer!= layers.end(); ++layer)
		(*layer)->render(qCache);
	glDisableClientState(GL_VERTEX_ARRAY);
	glBindBuffer(GL_ARRAY_BUFFER_ARB,0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB,0);
	glDisableClientState(GL_NORMAL_ARRAY);
	glClientActiveTexture(fowTexUnit);
	glBindTexture(GL_TEXTURE_2D,0);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glClientActiveTexture(baseTexUnit);
	glBindTexture(GL_TEXTURE_2D,0);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	assertGl();
}

void Renderer::MapRenderer::destroy() {
	while(layers.empty() == false) {
		delete layers.back();
		layers.pop_back();
	}
	map = NULL;
}

void Renderer::renderSurface(const int renderFps) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	IF_DEBUG_EDITION(
		if (getDebugRenderer().willRenderSurface()) {
			getDebugRenderer().renderSurface(visibleQuad / Map::cellScale);
		} else {
	)
	assertGl();

	const World *world= game->getWorld();
	const Map *map= world->getMap();
	float coordStep= world->getTileset()->getSurfaceAtlas()->getCoordStep();

	const Texture2D *fowTex= world->getMinimap()->getFowTexture();
	if(fowTex == NULL) {
		return;
	}

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
		fowTex->getPixmapConst()->getW(), fowTex->getPixmapConst()->getH(),
		GL_ALPHA, GL_UNSIGNED_BYTE, fowTex->getPixmapConst()->getPixels());

	if(shadowsOffDueToMinRender == false) {
		//shadow texture
		if(shadows == sProjected || shadows == sShadowMapping) {
			glActiveTexture(shadowTexUnit);
			glEnable(GL_TEXTURE_2D);

			glBindTexture(GL_TEXTURE_2D, shadowMapHandle);

			static_cast<ModelRendererGl*>(modelRenderer)->setDuplicateTexCoords(true);
			enableProjectiveTexturing();
		}
	}

	const Rect2i mapBounds(0, 0, map->getSurfaceW()-1, map->getSurfaceH()-1);

	glActiveTexture(baseTexUnit);

	VisibleQuadContainerCache &qCache = getQuadCache();
	
	bool useVBORendering = getVBOSupported();
	if(useVBORendering == true) {
		VisibleQuadContainerCache &qCache = getQuadCache();
		//mapRenderer.render(map,coordStep,qCache);
		mapRenderer.renderVisibleLayers(map,coordStep,qCache);
	}
	else if(qCache.visibleScaledCellList.empty() == false) {

		int lastTex=-1;
		int currTex=-1;

		Quad2i snapshotOfvisibleQuad = visibleQuad;

		//bool useVertexArrayRendering = getVBOSupported();
		bool useVertexArrayRendering = false;
		if(useVertexArrayRendering == false) {
		    //printf("\LEGACY qCache.visibleScaledCellList.size() = %d \n",qCache.visibleScaledCellList.size());

			Vec2f texCoords[4];
			Vec2f texCoordsSurface[4];
			Vec3f vertices[4];
			Vec3f normals[4];
		    glEnableClientState(GL_VERTEX_ARRAY);
		    glEnableClientState(GL_NORMAL_ARRAY);

		    std::map<int,int> uniqueVisibleTextures;
			for(int visibleIndex = 0;
					visibleIndex < qCache.visibleScaledCellList.size(); ++visibleIndex) {
				Vec2i &pos = qCache.visibleScaledCellList[visibleIndex];
				SurfaceCell *tc00= map->getSurfaceCell(pos.x, pos.y);
				int cellTex= static_cast<const Texture2DGl*>(tc00->getSurfaceTexture())->getHandle();

				uniqueVisibleTextures[cellTex]++;
			}

			//printf("Current renders = %d possible = %d\n",qCache.visibleScaledCellList.size(),uniqueVisibleTextures.size());

			for(int visibleIndex = 0;
					visibleIndex < qCache.visibleScaledCellList.size(); ++visibleIndex) {
				Vec2i &pos = qCache.visibleScaledCellList[visibleIndex];

				SurfaceCell *tc00= map->getSurfaceCell(pos.x, pos.y);
				SurfaceCell *tc10= map->getSurfaceCell(pos.x+1, pos.y);
				SurfaceCell *tc01= map->getSurfaceCell(pos.x, pos.y+1);
				SurfaceCell *tc11= map->getSurfaceCell(pos.x+1, pos.y+1);

				if(tc00 == NULL) {
					throw megaglest_runtime_error("tc00 == NULL");
				}
				if(tc10 == NULL) {
					throw megaglest_runtime_error("tc10 == NULL");
				}
				if(tc01 == NULL) {
					throw megaglest_runtime_error("tc01 == NULL");
				}
				if(tc11 == NULL) {
					throw megaglest_runtime_error("tc11 == NULL");
				}

				triangleCount+= 2;
				pointCount+= 4;

				//set texture
				if(tc00->getSurfaceTexture() == NULL) {
					throw megaglest_runtime_error("tc00->getSurfaceTexture() == NULL");
				}
				currTex= static_cast<const Texture2DGl*>(tc00->getSurfaceTexture())->getHandle();
				if(currTex != lastTex) {
					lastTex = currTex;
					//glBindTexture(GL_TEXTURE_2D, lastTex);
				}

				const Vec2f &surfCoord= tc00->getSurfTexCoord();

				texCoords[0]		= tc01->getFowTexCoord();
				texCoordsSurface[0]	= Vec2f(surfCoord.x, surfCoord.y + coordStep);
				vertices[0]			= tc01->getVertex();
				normals[0]			= tc01->getNormal();;

				texCoords[1]		= tc00->getFowTexCoord();
				texCoordsSurface[1]	= Vec2f(surfCoord.x, surfCoord.y);
				vertices[1]			= tc00->getVertex();
				normals[1]			= tc00->getNormal();

				texCoords[2]		= tc11->getFowTexCoord();
				texCoordsSurface[2]	= Vec2f(surfCoord.x+coordStep, surfCoord.y+coordStep);
				vertices[2]			= tc11->getVertex();
				normals[2]			= tc11->getNormal();

				texCoords[3]		= tc10->getFowTexCoord();
				texCoordsSurface[3]	= Vec2f(surfCoord.x+coordStep, surfCoord.y);
				vertices[3]			= tc10->getVertex();
				normals[3]			= tc10->getNormal();

				//glBindTexture(GL_TEXTURE_2D, static_cast<const Texture2DGl*>(fowTex)->getHandle());
				glClientActiveTexture(fowTexUnit);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glTexCoordPointer(2, GL_FLOAT, 0,&texCoords[0]);

				glBindTexture(GL_TEXTURE_2D, lastTex);
				glClientActiveTexture(baseTexUnit);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glTexCoordPointer(2, GL_FLOAT, 0, &texCoordsSurface[0]);

				glVertexPointer(3, GL_FLOAT, 0, &vertices[0]);
				glNormalPointer(GL_FLOAT, 0, &normals[0]);

				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
				//unsigned short faceIndices[4] = {0, 1, 2, 3};
				//glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, &faceIndices[0]);

				glClientActiveTexture(fowTexUnit);
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
				glClientActiveTexture(baseTexUnit);
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);

/*
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
*/
			}

			glDisableClientState(GL_NORMAL_ARRAY);
			glDisableClientState(GL_VERTEX_ARRAY);

		}
		else {
		    int lastSurfaceDataIndex = -1;

		    const bool useVBOs = false;
		    const bool useSurfaceCache = false;

		    std::vector<SurfaceData> surfaceData;
		    bool recalcSurface = false;

		    if(useSurfaceCache == true) {
				std::map<string,std::pair<Chrono, std::vector<SurfaceData> > >::iterator iterFind = mapSurfaceData.find(snapshotOfvisibleQuad.getString());
				if(iterFind == mapSurfaceData.end()) {
					recalcSurface = true;
					//printf("#1 Calculating surface for Rendering using VA's [%s]\n",snapshotOfvisibleQuad.getString().c_str());
				}
/*
				else if(iterFind->second.first.getMillis() >= 250) {
					recalcSurface = true;
					mapSurfaceData.erase(snapshotOfvisibleQuad.getString());
					//printf("#2 RE-Calculating surface for Rendering using VA's [%s]\n",snapshotOfvisibleQuad.getString().c_str());
				}
*/
		    }
		    else {
		    	recalcSurface = true;
		    }

		    if(recalcSurface == true) {
				//printf("Calculating surface for Rendering using VA's [%s]\n",snapshotOfvisibleQuad.getString().c_str());

		    	std::vector<SurfaceData> *surface = &surfaceData;
		    	if(useSurfaceCache == true) {
					std::pair<Chrono, std::vector<SurfaceData> > &surfaceCacheEntity = mapSurfaceData[snapshotOfvisibleQuad.getString()];
					surface = &surfaceCacheEntity.second;
					//surface.reserve(qCache.visibleScaledCellList.size());
		    	}
		    	surface->reserve(qCache.visibleScaledCellList.size());

				for(int visibleIndex = 0;
						visibleIndex < qCache.visibleScaledCellList.size(); ++visibleIndex) {
					Vec2i &pos = qCache.visibleScaledCellList[visibleIndex];

					SurfaceCell *tc00= map->getSurfaceCell(pos.x, pos.y);
					SurfaceCell *tc10= map->getSurfaceCell(pos.x+1, pos.y);
					SurfaceCell *tc01= map->getSurfaceCell(pos.x, pos.y+1);
					SurfaceCell *tc11= map->getSurfaceCell(pos.x+1, pos.y+1);

					if(tc00 == NULL) {
						throw megaglest_runtime_error("tc00 == NULL");
					}
					if(tc10 == NULL) {
						throw megaglest_runtime_error("tc10 == NULL");
					}
					if(tc01 == NULL) {
						throw megaglest_runtime_error("tc01 == NULL");
					}
					if(tc11 == NULL) {
						throw megaglest_runtime_error("tc11 == NULL");
					}

					triangleCount+= 2;
					pointCount+= 4;

					//set texture
					if(tc00->getSurfaceTexture() == NULL) {
						throw megaglest_runtime_error("tc00->getSurfaceTexture() == NULL");
					}

					int surfaceDataIndex = -1;
					currTex= static_cast<const Texture2DGl*>(tc00->getSurfaceTexture())->getHandle();
					if(currTex != lastTex) {
						lastTex = currTex;
					}
					else {
						surfaceDataIndex = lastSurfaceDataIndex;
					}

					if(surfaceDataIndex < 0) {
						SurfaceData newData;
						newData.uniqueId = SurfaceData::nextUniqueId;
						SurfaceData::nextUniqueId++;
						newData.bufferCount=0;
						newData.textureHandle = currTex;
						surface->push_back(newData);

						surfaceDataIndex = surface->size() - 1;
					}

					lastSurfaceDataIndex = surfaceDataIndex;

					SurfaceData *cellData = &(*surface)[surfaceDataIndex];

					const Vec2f &surfCoord= tc00->getSurfTexCoord();

					cellData->texCoords.push_back(tc01->getFowTexCoord());
					cellData->texCoordsSurface.push_back(Vec2f(surfCoord.x, surfCoord.y + coordStep));
					cellData->vertices.push_back(tc01->getVertex());
					cellData->normals.push_back(tc01->getNormal());
					cellData->bufferCount++;

					cellData->texCoords.push_back(tc00->getFowTexCoord());
					cellData->texCoordsSurface.push_back(Vec2f(surfCoord.x, surfCoord.y));
					cellData->vertices.push_back(tc00->getVertex());
					cellData->normals.push_back(tc00->getNormal());
					cellData->bufferCount++;

					cellData->texCoords.push_back(tc11->getFowTexCoord());
					cellData->texCoordsSurface.push_back(Vec2f(surfCoord.x+coordStep, surfCoord.y+coordStep));
					cellData->vertices.push_back(tc11->getVertex());
					cellData->normals.push_back(tc11->getNormal());
					cellData->bufferCount++;

					cellData->texCoords.push_back(tc10->getFowTexCoord());
					cellData->texCoordsSurface.push_back(Vec2f(surfCoord.x+coordStep, surfCoord.y));
					cellData->vertices.push_back(tc10->getVertex());
					cellData->normals.push_back(tc10->getNormal());
					cellData->bufferCount++;
				}

/*
				if(useSurfaceCache == true) {
					std::pair<Chrono, std::vector<SurfaceData> > &surfaceCacheEntity = mapSurfaceData[snapshotOfvisibleQuad.getString()];
					surfaceCacheEntity.first.start();
				}
*/
			}
            //printf("\nsurface.size() = %d vs qCache.visibleScaledCellList.size() = %d snapshotOfvisibleQuad [%s]\n",surface.size(),qCache.visibleScaledCellList.size(),snapshotOfvisibleQuad.getString().c_str());

		    std::vector<SurfaceData> *surface = &surfaceData;
		    if(useSurfaceCache == true) {
		    	std::pair<Chrono, std::vector<SurfaceData> > &surfaceCacheEntity = mapSurfaceData[snapshotOfvisibleQuad.getString()];
		    	surface = &surfaceCacheEntity.second;

		    	//printf("Surface Cache Size for Rendering using VA's = %lu\n",mapSurfaceData.size());
		    }

		    glEnableClientState(GL_VERTEX_ARRAY);
		    glEnableClientState(GL_NORMAL_ARRAY);

			for(int i = 0; i < surface->size(); ++i) {
				SurfaceData &data = (*surface)[i];

				if(useVBOs == true) {
					VisibleQuadContainerVBOCache *vboCache = GetSurfaceVBOs(&data);

					//glBindTexture(GL_TEXTURE_2D, static_cast<const Texture2DGl*>(fowTex)->getHandle());
					glClientActiveTexture(fowTexUnit);
					glEnableClientState(GL_TEXTURE_COORD_ARRAY);

					glBindBufferARB( GL_ARRAY_BUFFER_ARB, vboCache->m_nVBOFowTexCoords);
					glTexCoordPointer(2, GL_FLOAT, 0,(char *) NULL);

					glBindTexture(GL_TEXTURE_2D, data.textureHandle);
					glClientActiveTexture(baseTexUnit);
					glEnableClientState(GL_TEXTURE_COORD_ARRAY);

					glBindBufferARB( GL_ARRAY_BUFFER_ARB, vboCache->m_nVBOSurfaceTexCoords);
					glTexCoordPointer(2, GL_FLOAT, 0, (char *) NULL);

					glBindBufferARB( GL_ARRAY_BUFFER_ARB, vboCache->m_nVBOVertices);
					glVertexPointer(3, GL_FLOAT, 0, (char *) NULL);

					glBindBufferARB( GL_ARRAY_BUFFER_ARB, vboCache->m_nVBONormals);
					glNormalPointer(GL_FLOAT, 0, (char *) NULL);

					glDrawArrays(GL_TRIANGLE_STRIP, 0, data.bufferCount);

					glBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );

					glClientActiveTexture(fowTexUnit);
					glDisableClientState(GL_TEXTURE_COORD_ARRAY);
					glClientActiveTexture(baseTexUnit);
					glDisableClientState(GL_TEXTURE_COORD_ARRAY);

				}
				else {
					Vec2f *texCoords		= &data.texCoords[0];
					Vec2f *texCoordsSurface	= &data.texCoordsSurface[0];
					Vec3f *vertices			= &data.vertices[0];
					Vec3f *normals			= &data.normals[0];

					//glBindTexture(GL_TEXTURE_2D, static_cast<const Texture2DGl*>(fowTex)->getHandle());
					glClientActiveTexture(fowTexUnit);
					glEnableClientState(GL_TEXTURE_COORD_ARRAY);
					glTexCoordPointer(2, GL_FLOAT, 0,texCoords);

					glBindTexture(GL_TEXTURE_2D, data.textureHandle);
					glClientActiveTexture(baseTexUnit);
					glEnableClientState(GL_TEXTURE_COORD_ARRAY);
					glTexCoordPointer(2, GL_FLOAT, 0, texCoordsSurface);

					glVertexPointer(3, GL_FLOAT, 0, vertices);
					glNormalPointer(GL_FLOAT, 0, normals);

					glDrawArrays(GL_TRIANGLE_STRIP, 0, data.bufferCount);

					glClientActiveTexture(fowTexUnit);
					glDisableClientState(GL_TEXTURE_COORD_ARRAY);
					glClientActiveTexture(baseTexUnit);
					glDisableClientState(GL_TEXTURE_COORD_ARRAY);
				}
			}

			glDisableClientState(GL_NORMAL_ARRAY);
			glDisableClientState(GL_VERTEX_ARRAY);

			//printf("Surface Render before [%d] after [%d]\n",qCache.visibleScaledCellList.size(),surface.size());
		}
	}

	//Restore
	static_cast<ModelRendererGl*>(modelRenderer)->setDuplicateTexCoords(false);

	glDisable(GL_TEXTURE_2D);

	glPopAttrib();

	//assert
	glGetError();	//remove when first mtex problem solved
	assertGl();

	IF_DEBUG_EDITION(
		} // end else, if not renderering debug textures instead of regular terrain
		getDebugRenderer().renderEffects(visibleQuad / Map::cellScale);
	)
}

void Renderer::renderObjects(const int renderFps) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	const World *world= game->getWorld();
	const Map *map= world->getMap();

    assertGl();

	const Texture2D *fowTex = world->getMinimap()->getFowTexture();
	const Pixmap2D *fowTexPixmap = fowTex->getPixmapConst();
	Vec3f baseFogColor = world->getTileset()->getFogColor() * world->getTimeFlow()->computeLightColor();

    bool modelRenderStarted = false;

	VisibleQuadContainerCache &qCache = getQuadCache();
	for(int visibleIndex = 0;
			visibleIndex < qCache.visibleObjectList.size(); ++visibleIndex) {
		Object *o = qCache.visibleObjectList[visibleIndex];

		Model *objModel= o->getModelPtr();
		//objModel->updateInterpolationData(o->getAnimProgress(), true);
		const Vec3f &v= o->getConstPos();

		if(modelRenderStarted == false) {
			modelRenderStarted = true;

			glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_FOG_BIT | GL_LIGHTING_BIT | GL_TEXTURE_BIT);

			if(shadowsOffDueToMinRender == false &&
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

		float fowFactor= fowTexPixmap->getPixelf(o->getMapPos().x / Map::cellScale, o->getMapPos().y / Map::cellScale);
		Vec4f color= Vec4f(Vec3f(fowFactor), 1.f);
		glColor4fv(color.ptr());
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, (color * ambFactor).ptr());
		glFogfv(GL_FOG_COLOR, (baseFogColor * fowFactor).ptr());

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glTranslatef(v.x, v.y, v.z);
		glRotatef(o->getRotation(), 0.f, 1.f, 0.f);

		//objModel->updateInterpolationData(0.f, true);
		objModel->updateInterpolationData(o->getAnimProgress(), true);
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

	assertGl();
}

void Renderer::renderWater() {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	const World *world= game->getWorld();
	const Map *map= world->getMap();

	const Texture2D *fowTex= world->getMinimap()->getFowTexture();
	if(fowTex == NULL) {
		return;
	}

	float waterAnim= world->getWaterEffects()->getAmin();

	//assert
    assertGl();

	glPushAttrib(GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);

	//water texture nit
    glDisable(GL_TEXTURE_2D);

	glEnable(GL_BLEND);
	if(textures3D) {
		Texture3D *waterTex= world->getTileset()->getWaterTex();
		if(waterTex == NULL) {
			throw megaglest_runtime_error("waterTex == NULL");
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
	//const Texture2D *fowTex= world->getMinimap()->getFowTexture();
	glActiveTexture(fowTexUnit);
	glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, static_cast<const Texture2DGl*>(fowTex)->getHandle());
    glActiveTexture(baseTexUnit);

	assertGl();

	int thisTeamIndex= world->getThisTeamIndex();
	bool cellExplored = world->showWorldForPlayer(world->getThisFactionIndex());
	bool closed= false;

	Rect2i boundingRect= visibleQuad.computeBoundingRect();
	Rect2i scaledRect= boundingRect/Map::cellScale;
	scaledRect.clamp(0, 0, map->getSurfaceW()-1, map->getSurfaceH()-1);

	float waterLevel= world->getMap()->getWaterLevel();
    for(int j=scaledRect.p[0].y; j<scaledRect.p[1].y; ++j){
        glBegin(GL_TRIANGLE_STRIP);

		for(int i=scaledRect.p[0].x; i<=scaledRect.p[1].x; ++i){
			SurfaceCell *tc0= map->getSurfaceCell(i, j);
            SurfaceCell *tc1= map->getSurfaceCell(i, j+1);
			if(tc0 == NULL) {
				throw megaglest_runtime_error("tc0 == NULL");
			}
			if(tc1 == NULL) {
				throw megaglest_runtime_error("tc1 == NULL");
			}

            if(cellExplored == false) {
                cellExplored = (tc0->isExplored(thisTeamIndex) || tc1->isExplored(thisTeamIndex));
            }

			if(cellExplored == true && tc0->getNearSubmerged()) {
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
				if(closed == false) {
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

void Renderer::renderTeamColorCircle(){
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	VisibleQuadContainerCache &qCache = getQuadCache();
	if(qCache.visibleQuadUnitList.empty() == false) {

		glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_LIGHTING);
		glDisable(GL_TEXTURE_2D);
		glDepthFunc(GL_ALWAYS);
		glDisable(GL_STENCIL_TEST);
		glDisable(GL_CULL_FACE);
		glEnable(GL_BLEND);
		glLineWidth(2.f);

		for(int visibleUnitIndex = 0;
							visibleUnitIndex < qCache.visibleQuadUnitList.size(); ++visibleUnitIndex) {
				Unit *unit = qCache.visibleQuadUnitList[visibleUnitIndex];
				Vec3f currVec= unit->getCurrVectorFlat();
				Vec3f color=unit->getFaction()->getTexture()->getPixmapConst()->getPixel3f(0,0);
				glColor4f(color.x, color.y, color.z, 0.7f);
				renderSelectionCircle(currVec, unit->getType()->getSize(), 0.8f, 0.05f);
			}
		glPopAttrib();
	}
}

void Renderer::renderTeamColorPlane(){
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	VisibleQuadContainerCache &qCache = getQuadCache();
	if(qCache.visibleQuadUnitList.empty() == false){
		glPushAttrib(GL_ENABLE_BIT);
		glDisable(GL_LIGHTING);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glEnable(GL_COLOR_MATERIAL);
		const Texture2D *texture=CoreData::getInstance().getTeamColorTexture();
		for(int visibleUnitIndex = 0;
				visibleUnitIndex < qCache.visibleQuadUnitList.size(); ++visibleUnitIndex){
			Unit *unit = qCache.visibleQuadUnitList[visibleUnitIndex];
			Vec3f currVec= unit->getCurrVectorFlat();
			renderTeamColorEffect(currVec,visibleUnitIndex,unit->getType()->getSize(),
					unit->getFaction()->getTexture()->getPixmapConst()->getPixel3f(0,0),texture);
		}
		glDisable(GL_COLOR_MATERIAL);
		glPopAttrib();
	}
}

void Renderer::renderGhostModel(const UnitType *building, const Vec2i pos,CardinalDir facing, Vec4f *forceColor) {
	//const UnitType *building= gui->getBuilding();
	//const Vec2i &pos= gui->getPosObjWorld();

	const Gui *gui= game->getGui();
	//const Mouse3d *mouse3d= gui->getMouse3d();
	const Map *map= game->getWorld()->getMap();
	if(map == NULL) {
		char szBuf[1024]="";
		sprintf(szBuf,"In [%s::%s] Line: %d map == NULL",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}

	glPushMatrix();
	Vec3f pos3f= Vec3f(pos.x, map->getCell(pos)->getHeight(), pos.y);

	//selection building placement
	float offset= building->getSize()/2.f-0.5f;
	glTranslatef(pos3f.x+offset, pos3f.y, pos3f.z+offset);

	//choose color
	Vec4f color;
	if(forceColor != NULL) {
		color = *forceColor;
	}
	else {
		if(map->isFreeCells(pos, building->getSize(), fLand)) {
			color= Vec4f(1.f, 1.f, 1.f, 0.5f);
		}
		else {
//			Uint64 tc=game->getTickCount();
//			float red=0.49f+((tc%4*1.0f)/2);
			color= Vec4f(1.0f, 0.f, 0.f, 0.5f);
		}
	}

	glColor4fv(color.ptr());
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, color.ptr());
	Model *buildingModel= building->getFirstStOfClass(scStop)->getAnimation();

	if(facing != CardinalDir::NORTH) {
		float rotateAmount = facing * 90.f;
		if(rotateAmount > 0) {
			glRotatef(rotateAmount, 0.f, 1.f, 0.f);
		}
	}

	buildingModel->updateInterpolationData(0.f, false);
	modelRenderer->render(buildingModel);

	glPopMatrix();
}

void Renderer::renderUnits(const int renderFps) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	//Unit *unit=NULL;
	//const World *world= game->getWorld();
	MeshCallbackTeamColor meshCallbackTeamColor;

	//assert
	assertGl();

	if(visibleFrameUnitList.empty() == false) {
		visibleFrameUnitList.clear();
		//visibleFrameUnitListCameraKey = "";
		//if(visibleFrameUnitListCameraKey != game->getGameCamera()->getCameraMovementKey()) {
		//	worldToScreenPosCache.clear();
		//}
	}

	bool modelRenderStarted = false;

	VisibleQuadContainerCache &qCache = getQuadCache();
	if(qCache.visibleQuadUnitList.empty() == false) {
		for(int visibleUnitIndex = 0;
				visibleUnitIndex < qCache.visibleQuadUnitList.size(); ++visibleUnitIndex) {
			Unit *unit = qCache.visibleQuadUnitList[visibleUnitIndex];

			meshCallbackTeamColor.setTeamTexture(unit->getFaction()->getTexture());

			if(modelRenderStarted == false) {
				modelRenderStarted = true;

				glPushAttrib(GL_ENABLE_BIT | GL_FOG_BIT | GL_LIGHTING_BIT | GL_TEXTURE_BIT);
				glEnable(GL_COLOR_MATERIAL);

				if(!shadowsOffDueToMinRender) {
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
			float zrot=unit->getRotationZ();
			float xrot=unit->getRotationX();
			if(zrot!=.0f){
				glRotatef(zrot, 0.f, 0.f, 1.f);
			}
			if(xrot!=.0f){
				glRotatef(xrot, 1.f, 0.f, 0.f);
			}
			glRotatef(unit->getRotation(), 0.f, 1.f, 0.f);

			//dead alpha
			const SkillType *st= unit->getCurrSkill();
			if(st->getClass() == scDie && static_cast<const DieSkillType*>(st)->getFade()) {
				float alpha= 1.0f-unit->getAnimProgress();
				glDisable(GL_COLOR_MATERIAL);
				glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, Vec4f(1.0f, 1.0f, 1.0f, alpha).ptr());
			}
			else {
				glEnable(GL_COLOR_MATERIAL);
				glAlphaFunc(GL_GREATER, 0.4f);
			}

			//render
			Model *model= unit->getCurrentModelPtr();
			model->updateInterpolationData(unit->getAnimProgress(), unit->isAlive() && !unit->isAnimProgressBound());

			modelRenderer->render(model);
			triangleCount+= model->getTriangleCount();
			pointCount+= model->getVertexCount();

			glPopMatrix();
			unit->setVisible(true);

			if(	showDebugUI == true &&
				(showDebugUILevel & debugui_unit_titles) == debugui_unit_titles) {

				unit->setScreenPos(computeScreenPosition(currVec));
				visibleFrameUnitList.push_back(unit);
				visibleFrameUnitListCameraKey = game->getGameCamera()->getCameraMovementKey();
			}
		}

		if(modelRenderStarted == true) {
			modelRenderer->end();
			glPopAttrib();
		}
	}

	//restore
	static_cast<ModelRendererGl*>(modelRenderer)->setDuplicateTexCoords(true);

	// reset alpha
	glAlphaFunc(GL_GREATER, 0.0f);
	//assert
	assertGl();

}

void Renderer::renderUnitsToBuild(const int renderFps) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	//assert
	assertGl();

	VisibleQuadContainerCache &qCache = getQuadCache();
	if(qCache.visibleQuadUnitBuildList.empty() == false) {

		glMatrixMode(GL_MODELVIEW);
		glPushAttrib(GL_CURRENT_BIT | GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_BLEND);
		glDisable(GL_STENCIL_TEST);
		glDepthFunc(GL_LESS);
		glEnable(GL_COLOR_MATERIAL);
		glDepthMask(GL_FALSE);

		modelRenderer->begin(true, true, false);

		for(int visibleUnitIndex = 0;
				visibleUnitIndex < qCache.visibleQuadUnitBuildList.size(); ++visibleUnitIndex) {
			const UnitBuildInfo &buildUnit = qCache.visibleQuadUnitBuildList[visibleUnitIndex];
			//Vec4f modelColor= Vec4f(0.f, 1.f, 0.f, 0.5f);
			const Vec3f teamColor = buildUnit.unit->getFaction()->getTexture()->getPixmapConst()->getPixel3f(0,0);
			Vec4f modelColor= Vec4f(teamColor.x,teamColor.y,teamColor.z,0.4f);
			renderGhostModel(buildUnit.buildUnit, buildUnit.pos, buildUnit.facing, &modelColor);

			//printf("Rendering to build unit index = %d\n",visibleUnitIndex);
		}

		modelRenderer->end();

		glDisable(GL_COLOR_MATERIAL);
		glPopAttrib();
	}

	//assert
	assertGl();

}

void Renderer::renderTeamColorEffect(Vec3f &v, int heigth, int size, Vec3f color, const Texture2D *texture) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	//GLUquadricObj *disc;
	float halfSize=size;
	//halfSize=halfSize;
	float heigthoffset=0.5+heigth%25*0.004;
	glPushMatrix();
	glBindTexture(GL_TEXTURE_2D, static_cast<const Texture2DGl*>(texture)->getHandle());
	glColor4f(color.x, color.y, color.z, 1.0f);
		glBegin(GL_TRIANGLE_STRIP);
			glTexCoord2i(0, 1);
			glVertex3f(v.x-halfSize,v.y+heigthoffset,v.z+halfSize);
			glTexCoord2i(0, 0);
			glVertex3f(v.x-halfSize,v.y+heigthoffset, v.z-halfSize);
			glTexCoord2i(1, 1);

			glVertex3f(v.x+halfSize,v.y+heigthoffset, v.z+halfSize);
			glTexCoord2i(1, 0);
			glVertex3f(v.x+halfSize,v.y+heigthoffset, v.z-halfSize);
		glEnd();
	glPopMatrix();

}



void Renderer::renderSelectionEffects() {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	Config &config= Config::getInstance();
	if(config.getBool("RecordMode","false") == true) {
		return;
	}

	const World *world= game->getWorld();
	const Map *map= world->getMap();
	const Selection *selection= game->getGui()->getSelection();
	const Object *selectedResourceObject= game->getGui()->getSelectedResourceObject();

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
			if(	showDebugUI == true &&
				((showDebugUILevel & debugui_unit_titles) == debugui_unit_titles) &&
				unit->getCommandSize() > 0 &&
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

		if(	showDebugUI == true &&
			(showDebugUILevel & debugui_unit_titles) == debugui_unit_titles) {

			const UnitPathInterface *path= unit->getPath();
			if(path != NULL && dynamic_cast<const UnitPathBasic *>(path)) {
				vector<Vec2i> pathList = dynamic_cast<const UnitPathBasic *>(path)->getLastPathCacheQueue();

				Vec2i lastPosValue;
				for(int i = 0; i < pathList.size(); ++i) {
					Vec2i curPosValue = pathList[i];
					if(i == 0) {
						lastPosValue = curPosValue;
					}
					Vec3f currVec2 = unit->getVectorFlat(lastPosValue,curPosValue);
					currVec2.y+= 0.3f;
					renderSelectionCircle(currVec2, 1, selectionCircleRadius);
					//renderSelectionCircle(currVec2, unit->getType()->getSize(), selectionCircleRadius);

					//SurfaceCell *cell= map->getSurfaceCell(currVec2.x, currVec2.y);
					//currVec2.z = cell->getHeight() + 2.0;
					//renderSelectionCircle(currVec2, unit->getType()->getSize(), selectionCircleRadius);
				}
			}
		}

		//magic circle
		if(world->getThisFactionIndex() == unit->getFactionIndex() && unit->getType()->getMaxEp() > 0) {
			glColor4f(unit->getEpRatio()/2.f, unit->getEpRatio(), unit->getEpRatio(), 0.5f);
			renderSelectionCircle(currVec, unit->getType()->getSize(), magicCircleRadius);
		}

		// Render Attack-boost circles
		if(showDebugUI == true) {
			//const std::pair<const SkillType *,std::vector<Unit *> > &currentAttackBoostUnits = unit->getCurrentAttackBoostUnits();
			const UnitAttackBoostEffectOriginator &effect = unit->getAttackBoostOriginatorEffect();

			if(effect.skillType->isAttackBoostEnabled() == true) {
				glColor4f(MAGENTA.x,MAGENTA.y,MAGENTA.z,MAGENTA.w);
				renderSelectionCircle(currVec, unit->getType()->getSize(), effect.skillType->getAttackBoost()->radius);

				for(unsigned int i = 0; i < effect.currentAttackBoostUnits.size(); ++i) {
					// Remove attack boost upgrades from unit
					int findUnitId = effect.currentAttackBoostUnits[i];
					Unit *affectedUnit = game->getWorld()->findUnitById(findUnitId);
					if(affectedUnit != NULL) {
						Vec3f currVecBoost = affectedUnit->getCurrVectorFlat();
						currVecBoost.y += 0.3f;

						renderSelectionCircle(currVecBoost, affectedUnit->getType()->getSize(), 1.f);
					}
				}
			}
		}

	}
	if(selectedResourceObject!=NULL)
	{
		Resource *r= selectedResourceObject->getResource();
		int defaultValue= r->getType()->getDefResPerPatch();
		float colorValue=static_cast<float>(r->getAmount())/static_cast<float>(defaultValue);
		glColor4f(0.1f, 0.1f , colorValue, 0.4f);
		renderSelectionCircle(selectedResourceObject->getPos(),2, selectionCircleRadius);
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
					map->clampPos(pos);

					arrowTarget= Vec3f(pos.x, map->getCell(pos)->getHeight(), pos.y);
				}

				renderArrow(unit->getCurrVectorFlat(), arrowTarget, arrowColor, 0.3f);
			}
		}

		//meeting point arrow
		if(unit->getType()->getMeetingPoint()) {
			Vec2i pos= unit->getMeetingPos();
			map->clampPos(pos);

			Vec3f arrowTarget= Vec3f(pos.x, map->getCell(pos)->getHeight(), pos.y);
			renderArrow(unit->getCurrVectorFlat(), arrowTarget, Vec3f(0.f, 0.f, 1.f), 0.3f);
		}

	}

	//render selection hightlights
	if(game->getGui()->getHighlightedUnit()!=NULL)
	{
		const Unit *unit=game->getGui()->getHighlightedUnit() ;

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
			renderSelectionCircle(v, unit->getType()->getSize(), 0.5f+0.4f*highlight );
		}
	}
// old inefficient way to render highlights
//	for(int i=0; i < world->getFactionCount(); ++i) {
//		for(int j=0; j < world->getFaction(i)->getUnitCount(); ++j) {
//			const Unit *unit= world->getFaction(i)->getUnit(j);
//
//			if(unit->isHighlighted()) {
//				float highlight= unit->getHightlight();
//				if(game->getWorld()->getThisFactionIndex() == unit->getFactionIndex()) {
//					glColor4f(0.f, 1.f, 0.f, highlight);
//				}
//				else{
//					glColor4f(1.f, 0.f, 0.f, highlight);
//				}
//
//				Vec3f v= unit->getCurrVectorFlat();
//				v.y+= 0.3f;
//				renderSelectionCircle(v, unit->getType()->getSize(), 0.5f+0.4f*highlight );
//			}
//		}
//	}
	//render resource selection highlight
	if(game->getGui()->getHighlightedResourceObject()!=NULL)
	{
		const Object* object=game->getGui()->getHighlightedResourceObject();
		if(object->isHighlighted())
		{
			float highlight= object->getHightlight();
			glColor4f(0.1f, 0.1f , 1.0f, highlight);
			Vec3f v= object->getPos();
			v.y+= 0.3f;
			renderSelectionCircle(v, 2, 0.4f+0.4f*highlight );
		}
	}

	glPopAttrib();
}

void Renderer::renderWaterEffects(){
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

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

	//glNormal3f(0.f, 1.f, 0.f);

	//splashes
	glBindTexture(GL_TEXTURE_2D, static_cast<Texture2DGl*>(coreData.getWaterSplashTexture())->getHandle());

	//!!!
	Vec2f texCoords[4];
	Vec3f vertices[4];
	Vec3f normals[4];

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	for(int i=0; i<we->getWaterSplashCount(); ++i){
		const WaterSplash *ws= we->getWaterSplash(i);

		//render only if enabled
		if(ws->getEnabled()){

			//render only if visible
			Vec2i intPos= Vec2i(static_cast<int>(ws->getPos().x), static_cast<int>(ws->getPos().y));
			const Vec2i &mapPos = Map::toSurfCoords(intPos);

			bool visible = map->getSurfaceCell(mapPos)->isVisible(world->getThisTeamIndex());
			if(visible == false && world->showWorldForPlayer(world->getThisFactionIndex()) == true) {
				visible = true;
			}

			if(visible == true) {
				float scale= ws->getAnim()*ws->getSize();
				texCoords[0]		= Vec2f(0.f, 1.f);
				vertices[0]			= Vec3f(ws->getPos().x-scale, height, ws->getPos().y+scale);
				normals[0]			= Vec3f(0.f, 1.f, 0.f);

				texCoords[1]		= Vec2f(0.f, 0.f);
				vertices[1]			= Vec3f(ws->getPos().x-scale, height, ws->getPos().y-scale);
				normals[1]			= Vec3f(0.f, 1.f, 0.f);

				texCoords[2]		= Vec2f(1.f, 1.f);
				vertices[2]			= Vec3f(ws->getPos().x+scale, height, ws->getPos().y+scale);
				normals[2]			= Vec3f(0.f, 1.f, 0.f);

				texCoords[3]		= Vec2f(1.f, 0.f);
				vertices[3]			= Vec3f(ws->getPos().x+scale, height, ws->getPos().y-scale);
				normals[3]			= Vec3f(0.f, 1.f, 0.f);

				glColor4f(1.f, 1.f, 1.f, 1.f - ws->getAnim());
				glTexCoordPointer(2, GL_FLOAT, 0,&texCoords[0]);
				glVertexPointer(3, GL_FLOAT, 0, &vertices[0]);
				glNormalPointer(GL_FLOAT, 0, &normals[0]);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);


/*
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
*/
			}
		}
	}

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);

	glPopAttrib();

	assertGl();
}

void Renderer::renderHud(){
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	Texture2D *hudTexture=game->getGui()->getHudTexture();
	if(hudTexture!=NULL){
		const Metrics &metrics= Metrics::getInstance();
		renderTextureQuad(0, 0, metrics.getVirtualW(), metrics.getVirtualH(),hudTexture,1.0f);
	}
}

void Renderer::renderMinimap(){
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

    const World *world= game->getWorld();
	const Minimap *minimap= world->getMinimap();

	if(minimap == NULL || minimap->getTexture() == NULL) {
		return;
	}

	const GameCamera *gameCamera= game->getGameCamera();
	const Pixmap2D *pixmap= minimap->getTexture()->getPixmapConst();
	const Metrics &metrics= Metrics::getInstance();
	const WaterEffects *attackEffects= world->getAttackEffects();

	int mx= metrics.getMinimapX();
	int my= metrics.getMinimapY();
	int mw= metrics.getMinimapW();
	int mh= metrics.getMinimapH();

	Vec2f zoom= Vec2f(
		static_cast<float>(mw)/ pixmap->getW(),
		static_cast<float>(mh)/ pixmap->getH());

	assertGl();

//	CoreData &coreData= CoreData::getInstance();
//	Texture2D *backTexture =coreData.getButtonBigTexture();
//	glEnable(GL_TEXTURE_2D);
//	glEnable(GL_BLEND);
//	glBindTexture(GL_TEXTURE_2D, static_cast<Texture2DGl*>(backTexture)->getHandle());
//
//	glBegin(GL_TRIANGLE_STRIP);
//			glTexCoord2f(0.f, 0.f);
//			glVertex2f(mx-8, my-8);
//
//			glTexCoord2f(0.f, 1.f);
//			glVertex2f(mx-8, my+mh+8);
//
//			glTexCoord2f(1.f, 0.f);
//			glVertex2f(mx+mw+8, my-8);
//
//			glTexCoord2f(1.f, 1.f);
//			glVertex2f(mx+mw+8, my+mh+8);
//	glEnd();
//
//	glDisable(GL_TEXTURE_2D);


	// render minimap border
	Vec4f col= game->getGui()->getDisplay()->getColor();
	glColor4f(col.x*0.5f,col.y*0.5f,col.z*0.5f,1.0 );

	int borderWidth=2;
	glBegin(GL_QUADS);
	glVertex2i(mx-borderWidth, my-borderWidth);
	glVertex2i(mx-borderWidth, my);
	glVertex2i(mx+mw+borderWidth, my);
	glVertex2i(mx+mw+borderWidth, my-borderWidth);
	glEnd();

	glBegin(GL_QUADS);
	glVertex2i(mx-borderWidth, my+mh+borderWidth);
	glVertex2i(mx-borderWidth, my+mh);
	glVertex2i(mx+mw+borderWidth, my+mh);
	glVertex2i(mx+mw+borderWidth, my+mh+borderWidth);
	glEnd();

	glBegin(GL_QUADS);
	glVertex2i(mx-borderWidth, my);
	glVertex2i(mx-borderWidth, my+mh);
	glVertex2i(mx, my+mh);
	glVertex2i(mx, my);
	glEnd();

	glBegin(GL_QUADS);
	glVertex2i(mx+mw, my);
	glVertex2i(mx+mw, my+mh);
	glVertex2i(mx+mw+borderWidth, my+mh);
	glVertex2i(mx+mw+borderWidth, my);
	glEnd();


//	Vec4f col= game->getGui()->getDisplay()->getColor();
//	glBegin(GL_QUADS);
//	glColor4f(col.x*0.5f,col.y*0.5f,col.z*0.5f,1.0 );
//	glVertex2i(mx-4, my-4);
//	glVertex2i(mx-4, my+mh+4);
//	glVertex2i(mx+mw+4, my+mh+4);
//	glVertex2i(mx+mw+4, my-4);
//
//	glEnd();


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

/*
	Vec2f texCoords[4];
	Vec2f texCoords2[4];
	Vec2i vertices[4];

    texCoords[0] = Vec2f(0.0f, 1.0f);
    texCoords2[0] = Vec2f(0.0f, 1.0f);
    vertices[0] = Vec2i(mx, my);

    texCoords[1] = Vec2f(0.0f, 0.0f);
    texCoords2[1] = Vec2f(0.0f, 0.0f);
    vertices[1] = Vec2i(mx, my+mh);

    texCoords[2] = Vec2f(1.0f, 1.0f);
    texCoords2[2] = Vec2f(1.0f, 1.0f);
    vertices[2] = Vec2i(mx+mw, my);

    texCoords[3] = Vec2f(1.0f, 0.0f);
    texCoords2[3] = Vec2f(1.0f, 0.0f);
    vertices[3] = Vec2i(mx+mw, my+mh);

    glClientActiveTexture(baseTexUnit);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(2, GL_FLOAT, 0,&texCoords[0]);

	glClientActiveTexture(fowTexUnit);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0,&texCoords2[0]);

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_INT, 0, &vertices[0]);

	glColor4f(0.5f, 0.5f, 0.5f, 0.1f);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableClientState(GL_VERTEX_ARRAY);

    glClientActiveTexture(baseTexUnit);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glClientActiveTexture(fowTexUnit);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
*/

	//glColor4f(0.3f, 0.3f, 0.3f, 0.90f);
	glColor4f(0.5f, 0.5f, 0.5f, 0.2f);

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

	glEnable(GL_BLEND);

	const int itemCount = attackEffects->getWaterSplashCount() * 12;
	if(itemCount > 0) {
		vector<Vec2f> vertices;
		vertices.resize(itemCount);
		vector<Vec4f> colors;
		colors.resize(itemCount);

		// draw attack alarm
		int vertexIndex = 0;
		for(int i = 0; i < attackEffects->getWaterSplashCount(); ++i) {
			const WaterSplash *ws = attackEffects->getWaterSplash(i);
			float scale= (1/ws->getAnim()*ws->getSize())*5;
			//glColor4f(1.f, 1.f, 0.f, 1.f-ws->getAnim());
			float alpha=(1.f-ws->getAnim())*0.01f;
			Vec2f pos= ws->getPos()/Map::cellScale;
			float attackX=mx +pos.x*zoom.x;
			float attackY=my +mh -pos.y*zoom.y;
			if(ws->getEnabled()){
	//			glBegin(GL_QUADS);
	//				glVertex2f(attackX-scale, attackY-scale);
	//				glVertex2f(attackX-scale, attackY+scale);
	//				glVertex2f(attackX+scale, attackY+scale);
	//				glVertex2f(attackX+scale, attackY-scale);
	//			glEnd();

				colors[vertexIndex] = Vec4f(1.f, 1.f, 0.f, alpha);
				vertices[vertexIndex] = Vec2f(attackX-scale, attackY-scale);
				vertexIndex++;
				colors[vertexIndex] = Vec4f(1.f, 1.f, 0.f, alpha);
				vertices[vertexIndex] = Vec2f(attackX-scale, attackY+scale);
				vertexIndex++;
				colors[vertexIndex] = Vec4f(1.f, 1.f, 0.f, 0.8f);
				vertices[vertexIndex] = Vec2f(attackX, attackY);
				vertexIndex++;

				colors[vertexIndex] = Vec4f(1.f, 1.f, 0.f, alpha);
				vertices[vertexIndex] = Vec2f(attackX+scale, attackY+scale);
				vertexIndex++;
				colors[vertexIndex] = Vec4f(1.f, 1.f, 0.f, alpha);
				vertices[vertexIndex] = Vec2f(attackX-scale, attackY+scale);
				vertexIndex++;
				colors[vertexIndex] = Vec4f(1.f, 1.f, 0.f, 0.8f);
				vertices[vertexIndex] = Vec2f(attackX, attackY);
				vertexIndex++;

				colors[vertexIndex] = Vec4f(1.f, 1.f, 0.f, alpha);
				vertices[vertexIndex] = Vec2f(attackX+scale, attackY+scale);
				vertexIndex++;
				colors[vertexIndex] = Vec4f(1.f, 1.f, 0.f, alpha);
				vertices[vertexIndex] = Vec2f(attackX+scale, attackY-scale);
				vertexIndex++;
				colors[vertexIndex] = Vec4f(1.f, 1.f, 0.f, 0.8f);
				vertices[vertexIndex] = Vec2f(attackX, attackY);
				vertexIndex++;

				colors[vertexIndex] = Vec4f(1.f, 1.f, 0.f, alpha);
				vertices[vertexIndex] = Vec2f(attackX+scale, attackY-scale);
				vertexIndex++;
				colors[vertexIndex] = Vec4f(1.f, 1.f, 0.f, alpha);
				vertices[vertexIndex] = Vec2f(attackX-scale, attackY-scale);
				vertexIndex++;
				colors[vertexIndex] = Vec4f(1.f, 1.f, 0.f, 0.8f);
				vertices[vertexIndex] = Vec2f(attackX, attackY);
				vertexIndex++;

	/*
				glBegin(GL_TRIANGLES);
					glColor4f(1.f, 1.f, 0.f, alpha);
					glVertex2f(attackX-scale, attackY-scale);
					glVertex2f(attackX-scale, attackY+scale);
					glColor4f(1.f, 1.f, 0.f, 0.8f);
					glVertex2f(attackX, attackY);
				glEnd();
				glBegin(GL_TRIANGLES);
					glColor4f(1.f, 1.f, 0.f, alpha);
					glVertex2f(attackX-scale, attackY+scale);
					glVertex2f(attackX+scale, attackY+scale);
					glColor4f(1.f, 1.f, 0.f, 0.8f);
					glVertex2f(attackX, attackY);
				glEnd();
				glBegin(GL_TRIANGLES);
					glColor4f(1.f, 1.f, 0.f, alpha);
					glVertex2f(attackX+scale, attackY+scale);
					glVertex2f(attackX+scale, attackY-scale);
					glColor4f(1.f, 1.f, 0.f, 0.8f);
					glVertex2f(attackX, attackY);
				glEnd();
				glBegin(GL_TRIANGLES);
					glColor4f(1.f, 1.f, 0.f, alpha);
					glVertex2f(attackX+scale, attackY-scale);
					glVertex2f(attackX-scale, attackY-scale);
					glColor4f(1.f, 1.f, 0.f, 0.8f);
					glVertex2f(attackX, attackY);
				glEnd();
	*/

			}
		}

		if(vertexIndex > 0) {
			glEnableClientState(GL_COLOR_ARRAY);
			glEnableClientState(GL_VERTEX_ARRAY);
			glColorPointer(4,GL_FLOAT, 0, &colors[0]);
			glVertexPointer(2, GL_FLOAT, 0, &vertices[0]);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, vertexIndex);
			glDisableClientState(GL_COLOR_ARRAY);
			glDisableClientState(GL_VERTEX_ARRAY);
		}
	}

    glDisable(GL_BLEND);

	//draw units
	VisibleQuadContainerCache &qCache = getQuadCache();
	if(qCache.visibleUnitList.empty() == false) {
		uint32 unitIdx=0;
		vector<Vec2f> unit_vertices;
		unit_vertices.resize(qCache.visibleUnitList.size()*4);
		vector<Vec3f> unit_colors;
		unit_colors.resize(qCache.visibleUnitList.size()*4);

		for(int visibleIndex = 0;
				visibleIndex < qCache.visibleUnitList.size(); ++visibleIndex) {
			Unit *unit = qCache.visibleUnitList[visibleIndex];
			if (unit->isAlive() == false) {
				continue;
			}

			Vec2i pos= unit->getPos() / Map::cellScale;
			int size= unit->getType()->getSize();
			Vec3f color=  unit->getFaction()->getTexture()->getPixmapConst()->getPixel3f(0, 0);

			unit_colors[unitIdx] = color;
			unit_vertices[unitIdx] = Vec2f(mx + pos.x*zoom.x, my + mh - (pos.y*zoom.y));
			unitIdx++;

			unit_colors[unitIdx] = color;
			unit_vertices[unitIdx] = Vec2f(mx + (pos.x+1)*zoom.x+size, my + mh - (pos.y*zoom.y));
			unitIdx++;

			unit_colors[unitIdx] = color;
			unit_vertices[unitIdx] = Vec2f(mx + (pos.x+1)*zoom.x+size, my + mh - ((pos.y+size)*zoom.y));
			unitIdx++;

			unit_colors[unitIdx] = color;
			unit_vertices[unitIdx] = Vec2f(mx + pos.x*zoom.x, my + mh - ((pos.y+size)*zoom.y));
			unitIdx++;

/*
			glColor3fv(color.ptr());

			glBegin(GL_QUADS);

			glVertex2f(mx + pos.x*zoom.x, my + mh - (pos.y*zoom.y));
			glVertex2f(mx + (pos.x+1)*zoom.x+size, my + mh - (pos.y*zoom.y));
			glVertex2f(mx + (pos.x+1)*zoom.x+size, my + mh - ((pos.y+size)*zoom.y));
			glVertex2f(mx + pos.x*zoom.x, my + mh - ((pos.y+size)*zoom.y));

			glEnd();
*/
		}

		if(unitIdx > 0) {
			glEnableClientState(GL_COLOR_ARRAY);
			glEnableClientState(GL_VERTEX_ARRAY);
			glColorPointer(3,GL_FLOAT, 0, &unit_colors[0]);
			glVertexPointer(2, GL_FLOAT, 0, &unit_vertices[0]);
			glDrawArrays(GL_QUADS, 0, unitIdx);
			glDisableClientState(GL_COLOR_ARRAY);
			glDisableClientState(GL_VERTEX_ARRAY);
		}

	}

	renderMarkedCellsOnMinimap();

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
    x1 = mx + x + static_cast<int>(20*streflop::sin(static_cast<streflop::Simple>(ang-pi/5)));
	y1 = my + mh - (y-static_cast<int>(20*streflop::cos(static_cast<streflop::Simple>(ang-pi/5))));
#else
	x1 = mx + x + static_cast<int>(20*sin(ang-pi/5));
	y1 = my + mh - (y-static_cast<int>(20*cos(ang-pi/5)));
#endif

    int x2 = 0;
    int y2 = 0;
#ifdef USE_STREFLOP
    x2 = mx + x + static_cast<int>(20*streflop::sin(static_cast<streflop::Simple>(ang+pi/5)));
	y2 = my + mh - (y-static_cast<int>(20*streflop::cos(static_cast<streflop::Simple>(ang+pi/5))));
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

void Renderer::renderHighlightedCellsOnMinimap() {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}
	// Draw marked cells
	const std::vector<MarkedCell> *highlightedCells = game->getHighlightedCells();
	if(highlightedCells->empty() == false) {
		const Map *map= game->getWorld()->getMap();
	    const World *world= game->getWorld();
		const Minimap *minimap= world->getMinimap();
		int pointersize=10;
		if(minimap == NULL || minimap->getTexture() == NULL) {
			return;
		}

		const GameCamera *gameCamera= game->getGameCamera();
		const Pixmap2D *pixmap= minimap->getTexture()->getPixmapConst();
		const Metrics &metrics= Metrics::getInstance();


		int mx= metrics.getMinimapX();
		int my= metrics.getMinimapY();
		int mw= metrics.getMinimapW();
		int mh= metrics.getMinimapH();

		Vec2f zoom= Vec2f(
			static_cast<float>(mw)/ pixmap->getW()/2,
			static_cast<float>(mh)/ pixmap->getH()/2);

		for(int i=0;i<highlightedCells->size();i++) {
			const MarkedCell *mc=&highlightedCells->at(i);
			if(mc->getFaction() != NULL && mc->getFaction()->getTeam() == game->getWorld()->getThisFaction()->getTeam()) {
				const Texture2D *texture= game->getHighlightCellTexture();
				Vec3f color=  mc->getFaction()->getTexture()->getPixmapConst()->getPixel3f(0, 0);
				float alpha = 0.49f+0.5f/(mc->getAliveCount()%15);
				Vec2i pos=mc->getTargetPos();
				if(texture!=NULL){
					renderTextureQuad((int)(pos.x*zoom.x)+pointersize, my + mh-(int)(pos.y*zoom.y), pointersize, pointersize, texture, alpha,&color);
				}
			}
		}
	}
}

void Renderer::renderMarkedCellsOnMinimap() {
	// Draw marked cells
	std::map<Vec2i, MarkedCell> markedCells = game->getMapMarkedCellList();
	if(markedCells.empty() == false) {
		const Map *map= game->getWorld()->getMap();
	    const World *world= game->getWorld();
		const Minimap *minimap= world->getMinimap();

		if(minimap == NULL || minimap->getTexture() == NULL) {
			return;
		}

		const GameCamera *gameCamera= game->getGameCamera();
		const Pixmap2D *pixmap= minimap->getTexture()->getPixmapConst();
		const Metrics &metrics= Metrics::getInstance();
		const WaterEffects *attackEffects= world->getAttackEffects();

		int mx= metrics.getMinimapX();
		int my= metrics.getMinimapY();
		int mw= metrics.getMinimapW();
		int mh= metrics.getMinimapH();

		Vec2f zoom= Vec2f(
			static_cast<float>(mw)/ pixmap->getW(),
			static_cast<float>(mh)/ pixmap->getH());

		uint32 unitIdx=0;
		vector<Vec2f> unit_vertices;
		unit_vertices.resize(markedCells.size()*4);
		vector<Vec4f> unit_colors;
		unit_colors.resize(markedCells.size()*4);

		for(std::map<Vec2i, MarkedCell>::iterator iterMap =markedCells.begin();
				iterMap != markedCells.end(); ++iterMap) {
			MarkedCell &bm = iterMap->second;
			if(bm.getFaction() != NULL && bm.getFaction()->getTeam() == game->getWorld()->getThisFaction()->getTeam()) {
				Vec2i pos= bm.getTargetPos() / Map::cellScale;
				float size= 0.5f;
				//Vec3f color=  bm.color;
				Vec3f color=  bm.getFaction()->getTexture()->getPixmapConst()->getPixel3f(0, 0);
				float alpha = 0.65;

				unit_colors[unitIdx] = Vec4f(color.x,color.y,color.z,alpha);
				unit_vertices[unitIdx] = Vec2f(mx + pos.x*zoom.x, my + mh - (pos.y*zoom.y));
				unitIdx++;

				unit_colors[unitIdx] = Vec4f(color.x,color.y,color.z,alpha);
				unit_vertices[unitIdx] = Vec2f(mx + (pos.x+1)*zoom.x+size, my + mh - (pos.y*zoom.y));
				unitIdx++;

				unit_colors[unitIdx] = Vec4f(color.x,color.y,color.z,alpha);
				unit_vertices[unitIdx] = Vec2f(mx + (pos.x+1)*zoom.x+size, my + mh - ((pos.y+size)*zoom.y));
				unitIdx++;

				unit_colors[unitIdx] = Vec4f(color.x,color.y,color.z,alpha);
				unit_vertices[unitIdx] = Vec2f(mx + pos.x*zoom.x, my + mh - ((pos.y+size)*zoom.y));
				unitIdx++;
			}
		}

		if(unitIdx > 0) {
			glEnable(GL_BLEND);

			glEnableClientState(GL_COLOR_ARRAY);
			glEnableClientState(GL_VERTEX_ARRAY);
			glColorPointer(4,GL_FLOAT, 0, &unit_colors[0]);
			glVertexPointer(2, GL_FLOAT, 0, &unit_vertices[0]);
			glDrawArrays(GL_QUADS, 0, unitIdx);
			//glDrawArrays(GL_TRIANGLE_STRIP, 0, unitIdx);
			glDisableClientState(GL_COLOR_ARRAY);
			glDisableClientState(GL_VERTEX_ARRAY);

			glDisable(GL_BLEND);
		}
	}
}
void Renderer::renderVisibleMarkedCells() {
	// Draw marked cells
	std::map<Vec2i, MarkedCell> markedCells = game->getMapMarkedCellList();
	if(markedCells.empty() == false) {
		for(std::map<Vec2i, MarkedCell>::iterator iterMap =markedCells.begin();
				iterMap != markedCells.end(); ++iterMap) {
			MarkedCell &bm = iterMap->second;
			if(bm.getFaction() != NULL && bm.getFaction()->getTeam() == game->getWorld()->getThisFaction()->getTeam()) {
				const Map *map= game->getWorld()->getMap();
				std::pair<bool,Vec3f> bmVisible = posInCellQuadCache(
						map->toSurfCoords(bm.getTargetPos()));
				if(bmVisible.first == true) {
					const Texture2D *texture= game->getMarkCellTexture();
					renderTextureQuad(bmVisible.second.x,bmVisible.second.y+10,32,32,texture,0.8f);
				}
			}
		}
	}
}

void Renderer::renderDisplay() {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	CoreData &coreData= CoreData::getInstance();
	const Metrics &metrics= Metrics::getInstance();
	const Display *display= game->getGui()->getDisplay();

	glPushAttrib(GL_ENABLE_BIT);

	if(renderText3DEnabled == true) {
		//infoString
		renderTextShadow3D(
			display->getInfoText().c_str(),
			coreData.getDisplayFont3D(),
			display->getColor(),
			metrics.getDisplayX(),
			metrics.getDisplayY()+Display::infoStringY);

		//title
		renderTextShadow3D(
			display->getTitle().c_str(),
			coreData.getDisplayFont3D(),
			display->getColor(),
			metrics.getDisplayX()+40,
			metrics.getDisplayY() + metrics.getDisplayH() - 20);

		glColor3f(0.0f, 0.0f, 0.0f);

		//text
		renderTextShadow3D(
			display->getText().c_str(),
			coreData.getDisplayFont3D(),
			display->getColor(),
			metrics.getDisplayX() -1,
			metrics.getDisplayY() + metrics.getDisplayH() - 56);

		//progress Bar
		if(display->getProgressBar() != -1) {
			renderProgressBar3D(
				display->getProgressBar(),
				metrics.getDisplayX(),
				metrics.getDisplayY() + metrics.getDisplayH()-50,
				coreData.getDisplayFontSmall3D());
		}
	}
	else {
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
	}

	//up images
	glEnable(GL_TEXTURE_2D);

	glColor3f(1.f, 1.f, 1.f);
	for(int i=0; i<Display::upCellCount; ++i){
		if(display->getUpImage(i)!=NULL){
			renderQuad(
				metrics.getDisplayX()+display->computeUpX(i),
				metrics.getDisplayY()+display->computeUpY(i),
				display->getUpImageSize(), display->getUpImageSize(), display->getUpImage(i));
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

void Renderer::renderMenuBackground(const MenuBackground *menuBackground) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

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
	menuBackground->getMainModelPtr()->updateInterpolationData(menuBackground->getAnim(), true);
	modelRenderer->render(menuBackground->getMainModelPtr());
	modelRenderer->end();
	glDisable(GL_ALPHA_TEST);

	//characters
	float dist= menuBackground->getAboutPosition().dist(cameraPosition);
	float minDist= 3.f;
	if(dist < minDist) {

		glAlphaFunc(GL_GREATER, 0.0f);
		float alpha= clamp((minDist-dist) / minDist, 0.f, 1.f);
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, Vec4f(1.0f, 1.0f, 1.0f, alpha).ptr());

		std::vector<Vec3f> &characterMenuScreenPositionListCache =
				CacheManager::getCachedItem< std::vector<Vec3f> >(GameConstants::characterMenuScreenPositionListCacheLookupKey);
		characterMenuScreenPositionListCache.clear();

		modelRenderer->begin(true, true, false);

		for(int i=0; i < MenuBackground::characterCount; ++i) {
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glLoadIdentity();

			Vec3f worldPos(i*2.f-4.f, -1.4f, -7.5f);
			glTranslatef(worldPos.x,worldPos.y,worldPos.z);

			//
			// Get the screen coordinates for each character model - START
			//std::vector<GLdouble> projection(16);
			//std::vector<GLdouble> modelview(16);
			//std::vector<GLdouble> screen_coords(3);

			//glGetDoublev(GL_PROJECTION_MATRIX, projection.data());
			//glGetDoublev(GL_MODELVIEW_MATRIX, modelview.data());

			const Metrics &metrics= Metrics::getInstance();
			GLint viewport[]= {0, 0, metrics.getVirtualW(), metrics.getVirtualH()};

			//get matrices
			GLdouble projection[16];
			glGetDoublev(GL_PROJECTION_MATRIX, projection);

			GLdouble modelview[16];
			glGetDoublev(GL_MODELVIEW_MATRIX, modelview);

			//get the screen coordinates
			GLdouble screen_coords[3];

			gluProject(worldPos.x, worldPos.y, worldPos.z,
			    modelview, projection, viewport,
			    &screen_coords[0], &screen_coords[1], &screen_coords[2]);
			characterMenuScreenPositionListCache.push_back(Vec3f(screen_coords[0],screen_coords[1],screen_coords[2]));
			// Get the screen coordinates for each character model - END
			//

			menuBackground->getCharacterModelPtr(i)->updateInterpolationData(menuBackground->getAnim(), true);
			modelRenderer->render(menuBackground->getCharacterModelPtr(i));

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

void Renderer::renderMenuBackground(Camera *camera, float fade, Model *mainModel, vector<Model *> characterModels,const Vec3f characterPosition, float anim) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	assertGl();

	const Vec3f &cameraPosition= camera->getConstPosition();

	glPushAttrib(GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT);

	//clear
	//Vec4f fogColor= Vec4f(0.4f, 0.4f, 0.4f, 1.f) * fade;
	// Show black bacground
	Vec4f fogColor= Vec4f(0.f, 0.f, 0.f, 1.f);
	glClearColor(fogColor.x, fogColor.y, fogColor.z, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glFogfv(GL_FOG_COLOR, fogColor.ptr());

	//light
	Vec4f lightPos= Vec4f(10.f, 10.f, 10.f, 1.f)  * fade;
	Vec4f diffLight= Vec4f(0.9f, 0.9f, 0.9f, 1.f) * fade;
	Vec4f ambLight= Vec4f(0.3f, 0.3f, 0.3f, 1.f)  * fade;
	Vec4f specLight= Vec4f(0.1f, 0.1f, 0.1f, 1.f) * fade;

	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_POSITION, lightPos.ptr());
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffLight.ptr());
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambLight.ptr());
	glLightfv(GL_LIGHT0, GL_SPECULAR, specLight.ptr());

	//main model
	if(mainModel) {
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.5f);
		modelRenderer->begin(true, true, true);
		mainModel->updateInterpolationData(anim, true);
		modelRenderer->render(mainModel);
		modelRenderer->end();
		glDisable(GL_ALPHA_TEST);
	}

	//characters
	if(characterModels.empty() == false) {
		float dist= characterPosition.dist(cameraPosition);
		float minDist= 3.f;
		if(dist < minDist) {
			glAlphaFunc(GL_GREATER, 0.0f);
			float alpha= clamp((minDist-dist) / minDist, 0.f, 1.f);
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, Vec4f(1.0f, 1.0f, 1.0f, alpha).ptr());
			modelRenderer->begin(true, true, false);

			for(unsigned int i = 0; i < characterModels.size(); ++i) {
				if(characterModels[i]) {
					glMatrixMode(GL_MODELVIEW);
					glPushMatrix();
					glLoadIdentity();
					glTranslatef(i*2.f-4.f, -1.4f, -7.5f);
					characterModels[i]->updateInterpolationData(anim, true);
					modelRenderer->render(characterModels[i]);
					glPopMatrix();
				}
			}
			modelRenderer->end();
		}
	}


	glPopAttrib();

	assertGl();
}

// ==================== computing ====================

bool Renderer::computePosition(const Vec2i &screenPos, Vec2i &worldPos, bool exactCoords) {
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
	if(exactCoords == true) {
		worldPos= Vec2i(static_cast<int>(worldX), static_cast<int>(worldZ));
	}
	else {
		worldPos= Vec2i(static_cast<int>(worldX+0.5f), static_cast<int>(worldZ+0.5f));
	}

	//clamp coords to map size
	return map->isInside(worldPos);
}

// This method takes world co-ordinates and translates them to screen co-ords
Vec3f Renderer::computeScreenPosition(const Vec3f &worldPos) {
	if(worldToScreenPosCache.find(worldPos) != worldToScreenPosCache.end()) {
		return worldToScreenPosCache[worldPos];
	}
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
	worldToScreenPosCache[worldPos]=screenPos;

	return screenPos;
}

void Renderer::computeSelected(	Selection::UnitContainer &units, const Object *&obj,
								const bool withObjectSelection,
								const Vec2i &posDown, const Vec2i &posUp) {
	const bool colorPickingSelection 	= Config::getInstance().getBool("EnableColorPicking","false");
	const bool frustumPickingSelection 	= Config::getInstance().getBool("EnableFrustumPicking","false");

	if(colorPickingSelection == true) {
		selectUsingColorPicking(units,obj, withObjectSelection,posDown, posUp);
	}
	/// Frustrum approach --> Currently not accurate enough
	else if(frustumPickingSelection == true) {
		selectUsingFrustumSelection(units,obj, withObjectSelection,posDown, posUp);
	}
	else {
		selectUsingSelectionBuffer(units,obj, withObjectSelection,posDown, posUp);
	}
}

void Renderer::selectUsingFrustumSelection(Selection::UnitContainer &units,
		const Object *&obj, const bool withObjectSelection,
		const Vec2i &posDown, const Vec2i &posUp) {
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	const Metrics &metrics= Metrics::getInstance();
	GLint view[]= {0, 0, metrics.getVirtualW(), metrics.getVirtualH()};

	//compute center and dimensions of selection rectangle
	int x = (posDown.x+posUp.x) / 2;
	int y = (posDown.y+posUp.y) / 2;
	int w = abs(posDown.x-posUp.x);
	int h = abs(posDown.y-posUp.y);
	if(w < 1) {
		w = 1;
	}
	if(h < 1) {
		h = 1;
	}

	gluPickMatrix(x, y, w, h, view);
	gluPerspective(perspFov, metrics.getAspectRatio(), perspNearPlane, perspFarPlane);
	loadGameCameraMatrix();

	VisibleQuadContainerCache quadSelectionCacheItem;
	ExtractFrustum(quadSelectionCacheItem);

	//pop matrices
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	VisibleQuadContainerCache &qCache = getQuadCache();
	if(qCache.visibleQuadUnitList.empty() == false) {
		for(int visibleUnitIndex = 0;
				visibleUnitIndex < qCache.visibleQuadUnitList.size(); ++visibleUnitIndex) {
			Unit *unit = qCache.visibleQuadUnitList[visibleUnitIndex];
			if(unit != NULL && unit->isAlive()) {
				Vec3f unitPos = unit->getCurrVector();
				bool insideQuad = CubeInFrustum(quadSelectionCacheItem.frustumData,
						unitPos.x, unitPos.y, unitPos.z, unit->getType()->getSize());
				if(insideQuad == true) {
					units.push_back(unit);
				}
			}
		}
	}

	if(withObjectSelection == true) {
		if(qCache.visibleObjectList.empty() == false) {
			for(int visibleIndex = 0;
					visibleIndex < qCache.visibleObjectList.size(); ++visibleIndex) {
				Object *object = qCache.visibleObjectList[visibleIndex];
				if(object != NULL) {
					bool insideQuad = CubeInFrustum(quadSelectionCacheItem.frustumData,
							object->getPos().x, object->getPos().y, object->getPos().z, 1);
					if(insideQuad == true) {
						obj = object;
						if(withObjectSelection == true) {
							break;
						}
					}
				}
			}
		}
	}
}

void Renderer::selectUsingSelectionBuffer(Selection::UnitContainer &units,
		const Object *&obj, const bool withObjectSelection,
		const Vec2i &posDown, const Vec2i &posUp) {
	//compute center and dimensions of selection rectangle
	int x = (posDown.x+posUp.x) / 2;
	int y = (posDown.y+posUp.y) / 2;
	int w = abs(posDown.x-posUp.x);
	int h = abs(posDown.y-posUp.y);
	if(w < 1) {
		w = 1;
	}
	if(h < 1) {
		h = 1;
	}

	//declarations
	GLuint selectBuffer[Gui::maxSelBuff];

	//setup matrices
	glSelectBuffer(Gui::maxSelBuff, selectBuffer);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();

	GLint renderModeResult = glRenderMode(GL_SELECT);
	if(renderModeResult < 0) {
		const char *errorString= reinterpret_cast<const char*>(gluErrorString(renderModeResult));
		char szBuf[4096]="";
		sprintf(szBuf,"OpenGL error #%d [0x%X] : [%s] at file: [%s], line: %d",renderModeResult,renderModeResult,errorString,extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__);

		printf("%s\n",szBuf);
	}
	glLoadIdentity();

	const Metrics &metrics= Metrics::getInstance();
	GLint view[]= {0, 0, metrics.getVirtualW(), metrics.getVirtualH()};
	gluPickMatrix(x, y, w, h, view);
	gluPerspective(perspFov, metrics.getAspectRatio(), perspNearPlane, perspFarPlane);
	loadGameCameraMatrix();

	//render units to find which ones should be selected
	renderUnitsFast();
	if(withObjectSelection == true) {
		renderObjectsFast(false,true);
	}

	//pop matrices
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	// Added this to ensure all the selection calls are done now
	// (see http://www.unknownroad.com/rtfm/graphics/glselection.html section: [0x4])
	glFlush();

	//select units by checking the selected buffer
	int selCount= glRenderMode(GL_RENDER);
	if(selCount > 0) {
		VisibleQuadContainerCache &qCache = getQuadCache();
		for(int i = 1; i <= selCount; ++i) {
			int index = selectBuffer[i*4-1];
			if(index >= OBJECT_SELECT_OFFSET) {
				Object *object = qCache.visibleObjectList[index - OBJECT_SELECT_OFFSET];
				if(object != NULL) {
					obj = object;
					if(withObjectSelection == true) {
						break;
					}
				}
			}
			else {
				Unit *unit = qCache.visibleQuadUnitList[index];
				if(unit != NULL && unit->isAlive()) {
					units.push_back(unit);
				}
			}
		}
	}
	else if(selCount < 0) {
		const char *errorString= reinterpret_cast<const char*>(gluErrorString(selCount));
		char szBuf[4096]="";
		sprintf(szBuf,"OpenGL error #%d [0x%X] : [%s] at file: [%s], line: %d",selCount,selCount,errorString,extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__);

		printf("%s\n",szBuf);
	}
}

void Renderer::selectUsingColorPicking(Selection::UnitContainer &units,
		const Object *&obj, const bool withObjectSelection,
		const Vec2i &posDown, const Vec2i &posUp) {
	int x1 = posDown.x;
	int y1 = posDown.y;
	int x2 = posUp.x;
	int y2 = posUp.y;

	int x = min(x1,x2);
	int y = min(y1,y2);
	int w = max(x1,x2) - min(x1,x2);
	int h = max(y1,y2) - min(y1,y2);
	if(w < 1) {
		w = 1;
	}
	if(h < 1) {
		h = 1;
	}

	const Metrics &metrics= Metrics::getInstance();
	x= (x * metrics.getScreenW() / metrics.getVirtualW());
	y= (y * metrics.getScreenH() / metrics.getVirtualH());

	w= (w * metrics.getScreenW() / metrics.getVirtualW());
	h= (h * metrics.getScreenH() / metrics.getVirtualH());

	PixelBufferWrapper::begin();

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	//GLint view[]= {0, 0, metrics.getVirtualW(), metrics.getVirtualH()};
	//gluPickMatrix(x, y, w, h, view);
	gluPerspective(perspFov, metrics.getAspectRatio(), perspNearPlane, perspFarPlane);
	loadGameCameraMatrix();

	//render units to find which ones should be selected
	//printf("In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	vector<Unit *> rendererUnits = renderUnitsFast(false, true);
	//printf("In [%s::%s] Line: %d rendererUnits = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,rendererUnits.size());

	vector<Object *> rendererObjects;
	if(withObjectSelection == true) {
		rendererObjects = renderObjectsFast(false,true,true);
	}
	//pop matrices
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	// Added this to ensure all the selection calls are done now
	// (see http://www.unknownroad.com/rtfm/graphics/glselection.html section: [0x4])
	//glFlush();

	//GraphicsInterface::getInstance().getCurrentContext()->swapBuffers();

	PixelBufferWrapper::end();

	vector<BaseColorPickEntity *> rendererModels;
	for(unsigned int i = 0; i < rendererObjects.size(); ++i) {
		Object *object = rendererObjects[i];
		rendererModels.push_back(object);
		//printf("In [%s::%s] Line: %d rendered object i = %d [%s] [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,i,object->getUniquePickName().c_str(),object->getColorDescription().c_str());
		//printf("In [%s::%s] Line: %d\ni = %d [%d - %s] ptr[%p] color[%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,i,unit->getId(),unit->getType()->getName().c_str(),unit->getCurrentModelPtr(),unit->getColorDescription().c_str());
	}

	//printf("In [%s::%s] Line: %d\nLooking for picks inside [%d,%d,%d,%d] posdown [%s] posUp [%s]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,x,y,w,h,posDown.getString().c_str(),posUp.getString().c_str());
	for(unsigned int i = 0; i < rendererUnits.size(); ++i) {
		Unit *unit = rendererUnits[i];
		rendererModels.push_back(unit);
		//printf("In [%s::%s] Line: %d rendered unit i = %d [%s] [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,i,unit->getUniquePickName().c_str(),unit->getColorDescription().c_str());
		//printf("In [%s::%s] Line: %d\ni = %d [%d - %s] ptr[%p] color[%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,i,unit->getId(),unit->getType()->getName().c_str(),unit->getCurrentModelPtr(),unit->getColorDescription().c_str());
	}

	vector<int> pickedList = BaseColorPickEntity::getPickedList(x,y,w,h, rendererModels);
	//printf("In [%s::%s] Line: %d pickedList = %d models rendered = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,pickedList.size(),rendererModels.size());

	if(pickedList.empty() == false) {
		for(unsigned int i = 0; i < pickedList.size(); ++i) {
			int index = pickedList[i];
			//printf("In [%s::%s] Line: %d searching for selected object i = %d index = %d units = %d objects = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,i,index,rendererUnits.size(),rendererObjects.size());

			if(rendererObjects.empty() == false && index < rendererObjects.size()) {
				Object *object = rendererObjects[index];
				//printf("In [%s::%s] Line: %d searching for selected object i = %d index = %d [%p]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,i,index,object);

				if(object != NULL) {
					obj = object;
					if(withObjectSelection == true) {
						//printf("In [%s::%s] Line: %d found selected object [%p]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,obj);
						return;
					}
				}
			}
			else {
				index -= rendererObjects.size();
				Unit *unit = rendererUnits[index];
				if(unit != NULL && unit->isAlive()) {
					units.push_back(unit);
				}

			}
		}
	}
}

// ==================== shadows ====================

void Renderer::renderShadowsToTexture(const int renderFps){
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	if(shadowsOffDueToMinRender == false &&
		(shadows == sProjected || shadows == sShadowMapping)) {

		shadowMapFrame= (shadowMapFrame + 1) % (shadowFrameSkip + 1);

		if(shadowMapFrame == 0){
			assertGl();

			glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_VIEWPORT_BIT | GL_POLYGON_BIT);

			if(shadows == sShadowMapping) {
				glClear(GL_DEPTH_BUFFER_BIT);
			}
			else {
				float color= 1.0f-shadowAlpha;
				glColor3f(color, color, color);
				glClearColor(1.f, 1.f, 1.f, 1.f);
				glDisable(GL_DEPTH_TEST);
				glClear(GL_COLOR_BUFFER_BIT);
			}

			//assertGl();

			//clear color buffer
			//
			//set viewport, we leave one texel always in white to avoid problems
			glViewport(1, 1, shadowTextureSize-2, shadowTextureSize-2);

			//assertGl();

			if(nearestLightPos.w == 0.f) {
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

				//assertGl();

				if(game->getGameCamera()->getState()==GameCamera::sGame){
					//glOrtho(-35, 5, -15, 15, -1000, 1000);
					//glOrtho(-30, 30, -20, 20, -1000, 1000);
					glOrtho(-30, 5, -20, 20, -1000, 1000);
				}
				else{
					glOrtho(-30, 30, -20, 20, -1000, 1000);
				}

				//assertGl();

				//push and set modelview
				glMatrixMode(GL_MODELVIEW);
				glPushMatrix();
				glLoadIdentity();

				glRotatef(15, 0, 1, 0);

				glRotatef(ang, 1, 0, 0);
				glRotatef(90, 0, 1, 0);
				const Vec3f &pos= game->getGameCamera()->getPos();

				glTranslatef(static_cast<int>(-pos.x), 0, static_cast<int>(-pos.z));

				//assertGl();
			}
			else {
				//non directional light

				//push projection
				glMatrixMode(GL_PROJECTION);
				glPushMatrix();
				glLoadIdentity();

				//assertGl();

				gluPerspective(perspFov, 1.f, perspNearPlane, perspFarPlane);
				//const Metrics &metrics= Metrics::getInstance();
				//gluPerspective(perspFov, metrics.getAspectRatio(), perspNearPlane, perspFarPlane);


				assertGl();

				//push modelview
				glMatrixMode(GL_MODELVIEW);
				glPushMatrix();
				glLoadIdentity();
				glRotatef(-90, -1, 0, 0);
				glTranslatef(-nearestLightPos.x, -nearestLightPos.y-2, -nearestLightPos.z);

				//assertGl();
			}

			if(shadows == sShadowMapping) {
				glEnable(GL_POLYGON_OFFSET_FILL);
				glPolygonOffset(1.0f, 16.0f);

				//assertGl();
			}

			//render 3d
			renderUnitsFast(true);
			renderObjectsFast(true,false);

			//assertGl();

			//read color buffer
			glBindTexture(GL_TEXTURE_2D, shadowMapHandle);
			assertGl();
			glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, shadowTextureSize, shadowTextureSize);
			GLenum error = glGetError();
			// This error can happen when a Linux user switches from an X session
			// back to a running game, and 'seems' to be safe to ignore it
			if(error != GL_INVALID_OPERATION) {
				assertGlWithErrorNumber(error);
			}

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

			//assertGl();

			Matrix4f matrix3;
			glGetFloatv(GL_MODELVIEW_MATRIX, matrix3.ptr());

			//pop both matrices
			glPopMatrix();
			glMatrixMode(GL_PROJECTION);
			glPopMatrix();

			glMatrixMode(GL_PROJECTION);
			glPushMatrix();

			//assertGl();

			//compute texture matrix
			glLoadMatrixf(matrix1.ptr());
			glMultMatrixf(matrix2.ptr());
			glMultMatrixf(matrix3.ptr());
			glGetFloatv(GL_TRANSPOSE_PROJECTION_MATRIX_ARB, shadowMapMatrix.ptr());

			//assertGl();

			//if(shadows == sShadowMapping) {
			//	glDisable(GL_POLYGON_OFFSET_FILL);
			//	glPolygonOffset(0.0f, 0.0f);
			//}

			//pop
			glPopMatrix();

			//assertGl();

			glPopAttrib();

			assertGl();
		}
	}
}


// ==================== gl wrap ====================

string Renderer::getGlInfo(){
	string infoStr="";
	Lang &lang= Lang::getInstance();

	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false) {
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
	}
	return infoStr;
}

string Renderer::getGlMoreInfo(){
	string infoStr="";
	Lang &lang= Lang::getInstance();

	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false) {
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
	}

	return infoStr;
}

void Renderer::autoConfig() {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false) {
		Config &config= Config::getInstance();

		bool nvidiaCard= toLower(getGlVendor()).find("nvidia")!=string::npos;
		bool atiCard= toLower(getGlVendor()).find("ati")!=string::npos;
		//bool shadowExtensions = isGlExtensionSupported("GL_ARB_shadow") && isGlExtensionSupported("GL_ARB_shadow_ambient");
		bool shadowExtensions = isGlExtensionSupported("GL_ARB_shadow");

		//3D textures
		config.setBool("Textures3D", isGlExtensionSupported("GL_EXT_texture3D"));

		//shadows
		string shadows="";
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
}

void Renderer::clearBuffers() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::clearZBuffer() {
	glClear(GL_DEPTH_BUFFER_BIT);
}

void Renderer::loadConfig() {
	Config &config= Config::getInstance();

	//cache most used config params
	maxLights= config.getInt("MaxLights");
	photoMode= config.getBool("PhotoMode");
	focusArrows= config.getBool("FocusArrows");
	textures3D= config.getBool("Textures3D");
	float gammaValue=config.getFloat("GammaValue","0.0");
	if(this->program == NULL) {
		throw megaglest_runtime_error("this->program == NULL");
	}
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false) {
		//if(this->program != NULL) {
		if(gammaValue != 0.0) {
			this->program->getWindow()->setGamma(gammaValue);
			SDL_SetGamma(gammaValue, gammaValue, gammaValue);
		}
		//}
	}
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

	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false) {
		for(int i=0; i<rsCount; ++i){
			textureManager[i]->setFilter(textureFilter);
			textureManager[i]->setMaxAnisotropy(maxAnisotropy);
		}
	}
}

Texture2D *Renderer::saveScreenToTexture(int x, int y, int width, int height) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	Config &config= Config::getInstance();
	Texture2D::Filter textureFilter = strToTextureFilter(config.getString("Filter"));
	int maxAnisotropy				= config.getInt("FilterMaxAnisotropy");

	Texture2D *texture = GraphicsInterface::getInstance().getFactory()->newTexture2D();
	texture->setForceCompressionDisabled(true);
	texture->setMipmap(false);
	Pixmap2D *pixmapScreenShot = texture->getPixmap();
	pixmapScreenShot->init(width, height, 3);
	texture->init(textureFilter,maxAnisotropy);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	//glFinish();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	glReadPixels(x, y, pixmapScreenShot->getW(), pixmapScreenShot->getH(),
				 GL_RGB, GL_UNSIGNED_BYTE, pixmapScreenShot->getPixels());

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	return texture;
}

void Renderer::saveScreen(const string &path,int w, int h) {
	const Metrics &sm= Metrics::getInstance();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	Pixmap2D *pixmapScreenShot = new Pixmap2D(sm.getScreenW(),sm.getScreenH(), 3);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	//glFinish();

	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	glReadPixels(0, 0, pixmapScreenShot->getW(), pixmapScreenShot->getH(),
				 GL_RGB, GL_UNSIGNED_BYTE, pixmapScreenShot->getPixels());

	if(w==0 || h==0){
		h=sm.getScreenH();
		w=sm.getScreenW();
	}
	else{
		pixmapScreenShot->Scale(GL_RGB,w,h);
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// Signal the threads queue to add a screenshot save request
	MutexSafeWrapper safeMutex(&saveScreenShotThreadAccessor,string(extractFileFromDirectoryPath(__FILE__).c_str()) + "_" + intToStr(__LINE__));
	saveScreenQueue.push_back(make_pair(path,pixmapScreenShot));
	safeMutex.ReleaseLock();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

unsigned int Renderer::getSaveScreenQueueSize() {
	MutexSafeWrapper safeMutex(&saveScreenShotThreadAccessor,string(extractFileFromDirectoryPath(__FILE__).c_str()) + "_" + intToStr(__LINE__));
	int queueSize = saveScreenQueue.size();
	safeMutex.ReleaseLock();

	return queueSize;
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
	return Vec4f(-streflop::cos(static_cast<streflop::Simple>(ang))*sunDist, streflop::sin(static_cast<streflop::Simple>(ang))*sunDist, 0.f, 0.f);
#else
	return Vec4f(-cos(ang)*sunDist, sin(ang)*sunDist, 0.f, 0.f);
#endif
}

Vec4f Renderer::computeMoonPos(float time) {
	float ang= computeMoonAngle(time);
#ifdef USE_STREFLOP
	return Vec4f(-streflop::cos(static_cast<streflop::Simple>(ang))*moonDist, streflop::sin(static_cast<streflop::Simple>(ang))*moonDist, 0.f, 0.f);
#else
	return Vec4f(-cos(ang)*moonDist, sin(ang)*moonDist, 0.f, 0.f);
#endif
}

// ==================== fast render ====================

//render units for selection purposes
vector<Unit *> Renderer::renderUnitsFast(bool renderingShadows, bool colorPickingSelection) {
	vector<Unit *> unitsList;
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return unitsList;
	}

	assert(game != NULL);
	const World *world= game->getWorld();
	assert(world != NULL);

	//assertGl();

	bool modelRenderStarted = false;
	VisibleQuadContainerCache &qCache = getQuadCache();
	if(qCache.visibleQuadUnitList.empty() == false) {
		for(int visibleUnitIndex = 0;
				visibleUnitIndex < qCache.visibleQuadUnitList.size(); ++visibleUnitIndex) {
			Unit *unit = qCache.visibleQuadUnitList[visibleUnitIndex];

			if(modelRenderStarted == false) {
				modelRenderStarted = true;

				if(colorPickingSelection == false) {
					//glPushAttrib(GL_ENABLE_BIT| GL_TEXTURE_BIT);
					glDisable(GL_LIGHTING);
					if (renderingShadows == false) {
						glPushAttrib(GL_ENABLE_BIT);
						glDisable(GL_TEXTURE_2D);
					}
					else {
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
				}

				//assertGl();

				modelRenderer->begin(false, renderingShadows, false);

				//assertGl();

				if(colorPickingSelection == true) {
					BaseColorPickEntity::beginPicking();
				}
				else {
					glInitNames();
				}

				//assertGl();
			}

			if(colorPickingSelection == false) {
				glPushName(visibleUnitIndex);
			}

			//assertGl();

			glMatrixMode(GL_MODELVIEW);
			//debuxar modelo
			glPushMatrix();

			//translate
			Vec3f currVec= unit->getCurrVectorFlat();
			glTranslatef(currVec.x, currVec.y, currVec.z);

			//rotate
			glRotatef(unit->getRotation(), 0.f, 1.f, 0.f);

			//render
			Model *model= unit->getCurrentModelPtr();
			model->updateInterpolationVertices(unit->getAnimProgress(), unit->isAlive() && !unit->isAnimProgressBound());

			if(colorPickingSelection == true) {
				unit->setUniquePickingColor();
				unitsList.push_back(unit);

				//assertGl();
			}

			//assertGl();

			modelRenderer->render(model,rmSelection);

			glPopMatrix();

			if(colorPickingSelection == false) {
				glPopName();
			}

			//assertGl();
		}

		if(modelRenderStarted == true) {
			//assertGl();

			modelRenderer->end();

			//assertGl();

			if(colorPickingSelection == true) {
				BaseColorPickEntity::endPicking();
			}
			else {
				glPopAttrib();
			}

//			assertGl();
		}
	}

//	assertGl();

	return unitsList;
}

//render objects for selection purposes
vector<Object *>  Renderer::renderObjectsFast(bool renderingShadows, bool resourceOnly, bool colorPickingSelection) {
	vector<Object *> objectList;
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return objectList;
	}

	//const World *world= game->getWorld();
	//const Map *map= world->getMap();

    assertGl();

	bool modelRenderStarted = false;

	VisibleQuadContainerCache &qCache = getQuadCache();
	if(qCache.visibleObjectList.empty() == false) {
		for(int visibleIndex = 0;
				visibleIndex < qCache.visibleObjectList.size(); ++visibleIndex) {
			Object *o = qCache.visibleObjectList[visibleIndex];


			if(modelRenderStarted == false) {
				modelRenderStarted = true;

				if(colorPickingSelection == false) {
					glDisable(GL_LIGHTING);

					if (renderingShadows == false){
						glPushAttrib(GL_ENABLE_BIT);
						glDisable(GL_TEXTURE_2D);
					}
					else {
						glPushAttrib(GL_ENABLE_BIT| GL_TEXTURE_BIT);
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
					}
				}

				modelRenderer->begin(false, renderingShadows, false);

				if(colorPickingSelection == true) {
					BaseColorPickEntity::beginPicking();
				}
				else {
					glInitNames();
				}
			}

			if(resourceOnly == false || o->getResource()!= NULL) {
				Model *objModel= o->getModelPtr();
				objModel->updateInterpolationData(o->getAnimProgress(), true);
				const Vec3f &v= o->getConstPos();

				if(colorPickingSelection == false) {
					glPushName(OBJECT_SELECT_OFFSET+visibleIndex);
				}

				glMatrixMode(GL_MODELVIEW);
				glPushMatrix();
				glTranslatef(v.x, v.y, v.z);
				glRotatef(o->getRotation(), 0.f, 1.f, 0.f);

				if(colorPickingSelection == true) {
					o->setUniquePickingColor();
					objectList.push_back(o);

					//assertGl();
				}

				modelRenderer->render(objModel);

				glPopMatrix();

				if(colorPickingSelection == false) {
					glPopName();
				}
			}
		}

		if(modelRenderStarted == true) {
			modelRenderer->end();

			if(colorPickingSelection == true) {
				BaseColorPickEntity::endPicking();
			}
			else {
				glPopAttrib();
			}
		}
	}

	assertGl();

	return objectList;
}

// ==================== gl caps ====================

void Renderer::checkGlCaps() {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	//opengl 1.3
	//if(!isGlVersionSupported(1, 3, 0)) {
	if(glewIsSupported("GL_VERSION_1_3") == false) {
		string message;

		message += "Your system supports OpenGL version \"";
 		message += getGlVersion() + string("\"\n");
 		message += "MegaGlest needs at least version 1.3 to work\n";
 		message += "You may solve this problem by installing your latest video card drivers";

 		throw megaglest_runtime_error(message.c_str());
	}

	//opengl 1.4 or extension
	//if(!isGlVersionSupported(1, 4, 0)){
	if(glewIsSupported("GL_VERSION_1_4") == false) {
		checkExtension("GL_ARB_texture_env_crossbar", "MegaGlest");
	}
}

void Renderer::checkGlOptionalCaps() {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	//shadows
	if(shadows == sProjected || shadows == sShadowMapping) {
		if(getGlMaxTextureUnits() < 3) {
			throw megaglest_runtime_error("Your system doesn't support 3 texture units, required for shadows");
		}
	}

	//shadow mapping
	if(shadows == sShadowMapping) {
		checkExtension("GL_ARB_shadow", "Shadow Mapping");
		//checkExtension("GL_ARB_shadow_ambient", "Shadow Mapping");
		//checkExtension("GL_ARB_depth_texture", "Shadow Mapping");
	}
}

void Renderer::checkExtension(const string &extension, const string &msg) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	if(!isGlExtensionSupported(extension.c_str())) {
		string str= "OpenGL extension not supported: " + extension +  ", required for " + msg;
		throw megaglest_runtime_error(str);
	}
}

// ==================== init 3d lists ====================

void Renderer::init3dList() {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	render3dSetup();
	//const Metrics &metrics= Metrics::getInstance();

    //assertGl();

    //if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	//list3d= glGenLists(1);
	//assertGl();
	//list3dValid=true;

	//if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	//glNewList(list3d, GL_COMPILE_AND_EXECUTE);
	//need to execute, because if not gluPerspective takes no effect and gluLoadMatrix is wrong
	//render3dSetup();
	//glEndList();

	//if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	//assert
	//assertGl();
}

void Renderer::render3dSetup() {
	const Metrics &metrics= Metrics::getInstance();
	//if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	//misc
	glViewport(0, 0, metrics.getScreenW(), metrics.getScreenH());
	glClearColor(fowColor.x, fowColor.y, fowColor.z, fowColor.w);
	glFrontFace(GL_CW);
	glEnable(GL_CULL_FACE);
	loadProjectionMatrix();

	//if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

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

	//if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

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

	//if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	//lighting state
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	//matrix mode
	glMatrixMode(GL_MODELVIEW);

	//stencil test
	glDisable(GL_STENCIL_TEST);

	//if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	//fog
	const Tileset *tileset= NULL;
	if(game != NULL && game->getWorld() != NULL) {
		//if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		tileset = game->getWorld()->getTileset();
	}

	//if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	if(tileset != NULL && tileset->getFog()) {
		//if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

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

	//if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

void Renderer::init2dList() {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

//	//this list sets the state for the 2d rendering
//	list2d= glGenLists(1);
//	assertGl();
//	list2dValid=true;
//
//	glNewList(list2d, GL_COMPILE);
//	render2dMenuSetup();
//	glEndList();
//
//	assertGl();
}

void Renderer::render2dMenuSetup() {
	const Metrics &metrics= Metrics::getInstance();
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
}

void Renderer::init3dListMenu(const MainMenu *mm) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	//this->mm3d = mm;
	//printf("In [%s::%s Line: %d] this->custom_mm3d [%p] this->mm3d [%p]\n",__FILE__,__FUNCTION__,__LINE__,this->custom_mm3d,this->mm3d);

/*
	assertGl();

    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	const Metrics &metrics= Metrics::getInstance();
	//const MenuBackground *mb= mm->getConstMenuBackground();
	const MenuBackground *mb = NULL;
	if(mm != NULL) {
		mb = mm->getConstMenuBackground();
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	if(this->customlist3dMenu != NULL) {
		*this->customlist3dMenu = glGenLists(1);
		assertGl();
	}
	else {
		list3dMenu= glGenLists(1);
		assertGl();
		list3dMenuValid=true;
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	if(this->customlist3dMenu != NULL) {
		glNewList(*this->customlist3dMenu, GL_COMPILE);
	}
	else {
		glNewList(list3dMenu, GL_COMPILE);
	}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		//misc
		glViewport(0, 0, metrics.getScreenW(), metrics.getScreenH());
		glClearColor(0.4f, 0.4f, 0.4f, 1.f);
		glFrontFace(GL_CW);
		glEnable(GL_CULL_FACE);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(perspFov, metrics.getAspectRatio(), perspNearPlane, 1000000);

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

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		//fog
		if(mb != NULL && mb->getFog()){
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

			glEnable(GL_FOG);
			glFogi(GL_FOG_MODE, GL_EXP2);
			glFogf(GL_FOG_DENSITY, mb->getFogDensity());
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	glEndList();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	//assert
	assertGl();
*/
}

void Renderer::render3dMenuSetup(const MainMenu *mm) {
	const Metrics &metrics= Metrics::getInstance();
	const MenuBackground *mb = NULL;
	if(mm != NULL) {
		mb = mm->getConstMenuBackground();
	}

	//if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	//misc
	glViewport(0, 0, metrics.getScreenW(), metrics.getScreenH());
	glClearColor(0.4f, 0.4f, 0.4f, 1.f);
	glFrontFace(GL_CW);
	glEnable(GL_CULL_FACE);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(perspFov, metrics.getAspectRatio(), perspNearPlane, 1000000);

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

	//if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	//fog
	if(mb != NULL && mb->getFog()) {
		//if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		glEnable(GL_FOG);
		glFogi(GL_FOG_MODE, GL_EXP2);
		glFogf(GL_FOG_DENSITY, mb->getFogDensity());
	}

	//assert
	assertGl();

	//if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}
// ==================== misc ====================

void Renderer::loadProjectionMatrix() {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

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
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

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

void Renderer::renderSelectionCircle(Vec3f v, int size, float radius, float thickness) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	GLUquadricObj *disc;

	glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

	glTranslatef(v.x, v.y, v.z);
	glRotatef(90.f, 1.f, 0.f, 0.f);
	disc= gluNewQuadric();
	gluQuadricDrawStyle(disc, GLU_FILL);
	gluCylinder(disc, radius*(size-thickness), radius*size, thickness, 30, 1);
	gluDeleteQuadric(disc);

    glPopMatrix();
}

void Renderer::renderArrow(const Vec3f &pos1, const Vec3f &pos2,
						   const Vec3f &color, float width) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	Config &config= Config::getInstance();
	if(config.getBool("RecordMode","false") == true) {
		return;
	}

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

void Renderer::renderProgressBar3D(int size, int x, int y, Font3D *font, int customWidth,
		string prefixLabel,bool centeredText) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	int progressbarHeight	= 12;
    int currentSize     	= size;
    int maxSize         	= maxProgressBar;
    string renderText   	= intToStr(static_cast<int>(size)) + "%";
    if(customWidth > 0) {
        if(size > 0) {
            currentSize     = (int)((double)customWidth * ((double)size / 100.0));
        }
        maxSize         = customWidth;
        if(maxSize <= 0) {
        	maxSize = maxProgressBar;
        }
    }
    if(prefixLabel != "") {
        renderText = prefixLabel + renderText;
    }

	//bar
	glBegin(GL_QUADS);
		glColor4fv(progressBarFront2.ptr());
		glVertex2i(x, y);
		glVertex2i(x, y + progressbarHeight);
		glColor4fv(progressBarFront1.ptr());
		glVertex2i(x + currentSize, y + progressbarHeight);
		glVertex2i(x + currentSize, y);
	glEnd();

	//transp bar
	glEnable(GL_BLEND);
	glBegin(GL_QUADS);
		glColor4fv(progressBarBack2.ptr());
		glVertex2i(x + currentSize, y);
		glVertex2i(x + currentSize, y + progressbarHeight);
		glColor4fv(progressBarBack1.ptr());
		glVertex2i(x + maxSize, y + progressbarHeight);
		glVertex2i(x + maxSize, y);
	glEnd();
	glDisable(GL_BLEND);

	//text
	//glColor3fv(defColor.ptr());
	//printf("Render progress bar3d renderText [%s] y = %d, centeredText = %d\n",renderText.c_str(),y, centeredText);

	renderTextBoundingBox3D(renderText, font, defColor, x, y, maxSize,
			progressbarHeight, true, true, false,-1);
}

void Renderer::renderProgressBar(int size, int x, int y, Font2D *font, int customWidth,
		string prefixLabel,bool centeredText) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

    int currentSize     = size;
    int maxSize         = maxProgressBar;
    string renderText   = intToStr(static_cast<int>(size)) + "%";
    if(customWidth > 0) {
        if(size > 0) {
            currentSize     = (int)((double)customWidth * ((double)size / 100.0));
        }
        maxSize         = customWidth;
        if(maxSize <= 0) {
        	maxSize = maxProgressBar;
        }
    }
    if(prefixLabel != "") {
        renderText = prefixLabel + renderText;
    }

	//bar
	glBegin(GL_QUADS);
		glColor4fv(progressBarFront2.ptr());
		glVertex2i(x, y);
		glVertex2i(x, y+10);
		glColor4fv(progressBarFront1.ptr());
		glVertex2i(x + currentSize, y+10);
		glVertex2i(x + currentSize, y);
	glEnd();

	//transp bar
	glEnable(GL_BLEND);
	glBegin(GL_QUADS);
		glColor4fv(progressBarBack2.ptr());
		glVertex2i(x + currentSize, y);
		glVertex2i(x + currentSize, y+10);
		glColor4fv(progressBarBack1.ptr());
		glVertex2i(x + maxSize, y+10);
		glVertex2i(x + maxSize, y);
	glEnd();
	glDisable(GL_BLEND);

	//text
	glColor3fv(defColor.ptr());

	//textRenderer->begin(font);
	TextRendererSafeWrapper safeTextRender(textRenderer,font);
	if(centeredText == true) {
		textRenderer->render(renderText.c_str(), x + maxSize / 2, y, centeredText);
	}
	else {
		textRenderer->render(renderText.c_str(), x, y, centeredText);
	}
	//textRenderer->end();
	safeTextRender.end();
}


void Renderer::renderTile(const Vec2i &pos) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

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
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

/* Revert back to 3.4.0 render logic due to issues on some ATI cards
 *

	if(w < 0) {
		w = texture->getPixmapConst()->getW();
	}
	if(h < 0) {
		h = texture->getPixmapConst()->getH();
	}

	if(texture != NULL) {
		glBindTexture(GL_TEXTURE_2D, static_cast<const Texture2DGl*>(texture)->getHandle());
	}

	Vec2i texCoords[4];
	Vec2i vertices[4];
    texCoords[0] 	= Vec2i(0, 1);
    vertices[0] 	= Vec2i(x, y+h);
    texCoords[1] 	= Vec2i(0, 0);
    vertices[1] 	= Vec2i(x, y);
    texCoords[2] 	= Vec2i(1, 1);
    vertices[2] 	= Vec2i(x+w, y+h);
    texCoords[3] 	= Vec2i(1, 0);
    vertices[3] 	= Vec2i(x+w, y);

	//glClientActiveTexture(GL_TEXTURE0);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_INT, 0,&texCoords[0]);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_INT, 0, &vertices[0]);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableClientState(GL_VERTEX_ARRAY);
    //glClientActiveTexture(GL_TEXTURE0);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
*/

	if(w < 0) {
		w = texture->getPixmapConst()->getW();
	}
	if(h < 0) {
		h = texture->getPixmapConst()->getH();
	}

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

	throw megaglest_runtime_error("Error converting from string to FilterType, found: "+s);
}

void Renderer::setAllowRenderUnitTitles(bool value) {
	allowRenderUnitTitles = value;
}

// This method renders titles for units
void Renderer::renderUnitTitles3D(Font3D *font, Vec3f color) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	std::map<int,bool> unitRenderedList;

	if(visibleFrameUnitList.empty() == false) {
		//printf("Render Unit titles ON\n");

		for(int idx = 0; idx < visibleFrameUnitList.size(); idx++) {
			const Unit *unit = visibleFrameUnitList[idx];
			if(unit != NULL) {
				if(unit->getVisible() == true) {
					if(unit->getCurrentUnitTitle() != "") {
						//get the screen coordinates
						Vec3f screenPos = unit->getScreenPos();
	#ifdef USE_STREFLOP
						renderText3D(unit->getCurrentUnitTitle(), font, color, streflop::fabs(static_cast<streflop::Simple>(screenPos.x)) + 5, streflop::fabs(static_cast<streflop::Simple>(screenPos.y)) + 5, false);
	#else
						renderText3D(unit->getCurrentUnitTitle(), font, color, fabs(screenPos.x) + 5, fabs(screenPos.y) + 5, false);
	#endif

						unitRenderedList[unit->getId()] = true;
					}
					else {
						string str = unit->getFullName() + " - " + intToStr(unit->getId()) + " [" + unit->getPos().getString() + "]";
						Vec3f screenPos = unit->getScreenPos();
	#ifdef USE_STREFLOP
						renderText3D(str, font, color, streflop::fabs(static_cast<streflop::Simple>(screenPos.x)) + 5, streflop::fabs(static_cast<streflop::Simple>(screenPos.y)) + 5, false);
	#else
						renderText3D(str, font, color, fabs(screenPos.x) + 5, fabs(screenPos.y) + 5, false);
	#endif
					}
				}
			}
		}
		visibleFrameUnitList.clear();
	}

	/*
	if(renderUnitTitleList.empty() == false) {
		for(int idx = 0; idx < renderUnitTitleList.size(); idx++) {
			std::pair<Unit *,Vec3f> &unitInfo = renderUnitTitleList[idx];
			Unit *unit = unitInfo.first;

			const World *world= game->getWorld();
			Unit *validUnit = world->findUnitById(unit->getId());

			if(validUnit != NULL && unitRenderedList.find(validUnit->getId()) == unitRenderedList.end()) {
				string str = validUnit->getFullName() + " - " + intToStr(validUnit->getId());
				//get the screen coordinates
				Vec3f &screenPos = unitInfo.second;
				renderText(str, font, color, fabs(screenPos.x) + 5, fabs(screenPos.y) + 5, false);
				//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] screenPos.x = %f, screenPos.y = %f, screenPos.z = %f\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,screenPos.x,screenPos.y,screenPos.z);
			}
		}
		renderUnitTitleList.clear();
	}
	*/
}

// This method renders titles for units
void Renderer::renderUnitTitles(Font2D *font, Vec3f color) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	std::map<int,bool> unitRenderedList;

	if(visibleFrameUnitList.empty() == false) {
		//printf("Render Unit titles ON\n");

		for(int idx = 0; idx < visibleFrameUnitList.size(); idx++) {
			const Unit *unit = visibleFrameUnitList[idx];
			if(unit != NULL) {
				if(unit->getCurrentUnitTitle() != "") {
					//get the screen coordinates
					Vec3f screenPos = unit->getScreenPos();
#ifdef USE_STREFLOP
					renderText(unit->getCurrentUnitTitle(), font, color, streflop::fabs(static_cast<streflop::Simple>(screenPos.x)) + 5, streflop::fabs(static_cast<streflop::Simple>(screenPos.y)) + 5, false);
#else
					renderText(unit->getCurrentUnitTitle(), font, color, fabs(screenPos.x) + 5, fabs(screenPos.y) + 5, false);
#endif

					unitRenderedList[unit->getId()] = true;
				}
				else {
					string str = unit->getFullName() + " - " + intToStr(unit->getId()) + " [" + unit->getPos().getString() + "]";
					Vec3f screenPos = unit->getScreenPos();
#ifdef USE_STREFLOP
					renderText(str, font, color, streflop::fabs(static_cast<streflop::Simple>(screenPos.x)) + 5, streflop::fabs(static_cast<streflop::Simple>(screenPos.y)) + 5, false);
#else
					renderText(str, font, color, fabs(screenPos.x) + 5, fabs(screenPos.y) + 5, false);
#endif
				}
			}
		}
		visibleFrameUnitList.clear();
	}

	/*
	if(renderUnitTitleList.empty() == false) {
		for(int idx = 0; idx < renderUnitTitleList.size(); idx++) {
			std::pair<Unit *,Vec3f> &unitInfo = renderUnitTitleList[idx];
			Unit *unit = unitInfo.first;

			const World *world= game->getWorld();
			Unit *validUnit = world->findUnitById(unit->getId());

			if(validUnit != NULL && unitRenderedList.find(validUnit->getId()) == unitRenderedList.end()) {
				string str = validUnit->getFullName() + " - " + intToStr(validUnit->getId());
				//get the screen coordinates
				Vec3f &screenPos = unitInfo.second;
				renderText(str, font, color, fabs(screenPos.x) + 5, fabs(screenPos.y) + 5, false);
				//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] screenPos.x = %f, screenPos.y = %f, screenPos.z = %f\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,screenPos.x,screenPos.y,screenPos.z);
			}
		}
		renderUnitTitleList.clear();
	}
	*/
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
			worldToScreenPosCache.clear();
			//}

			// Unit calculations
			for(int i = 0; i < world->getFactionCount(); ++i) {
				const Faction *faction = world->getFaction(i);
				for(int j = 0; j < faction->getUnitCount(); ++j) {
					Unit *unit= faction->getUnit(j);

					bool unitCheckedForRender = false;
					if(VisibleQuadContainerCache::enableFrustumCalcs == true) {
						//bool insideQuad 	= PointInFrustum(quadCache.frustumData, unit->getCurrVector().x, unit->getCurrVector().y, unit->getCurrVector().z );
						bool insideQuad 	= CubeInFrustum(quadCache.frustumData, unit->getCurrVector().x, unit->getCurrVector().y, unit->getCurrVector().z, unit->getType()->getSize());
						bool renderInMap 	= world->toRenderUnit(unit);
						if(insideQuad == false || renderInMap == false) {
							unit->setVisible(false);
							if(renderInMap == true) {
								quadCache.visibleUnitList.push_back(unit);
							}
							unitCheckedForRender = true; // no more need to check any further;
							// Currently don't need this list
							//quadCache.inVisibleUnitList.push_back(unit);
						}
					}
					if(unitCheckedForRender == false) {
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

					bool unitBuildPending = unit->isBuildCommandPending();
					if(unitBuildPending == true) {
						const UnitBuildInfo &pendingUnit = unit->getBuildCommandPendingInfo();
						const Vec2i &pos = pendingUnit.pos;
						const Map *map= world->getMap();

						bool unitBuildCheckedForRender = false;

						//printf("#1 Unit is about to build another unit\n");

						if(VisibleQuadContainerCache::enableFrustumCalcs == true) {
							Vec3f pos3f= Vec3f(pos.x, map->getCell(pos)->getHeight(), pos.y);
							//bool insideQuad 	= PointInFrustum(quadCache.frustumData, unit->getCurrVector().x, unit->getCurrVector().y, unit->getCurrVector().z );
							bool insideQuad 	= CubeInFrustum(quadCache.frustumData, pos3f.x, pos3f.y, pos3f.z, pendingUnit.buildUnit->getSize());
							bool renderInMap 	= world->toRenderUnit(pendingUnit);
							if(insideQuad == false || renderInMap == false) {
								if(renderInMap == true) {
									quadCache.visibleQuadUnitBuildList.push_back(pendingUnit);
								}
								unitBuildCheckedForRender = true; // no more need to check any further;
								// Currently don't need this list
								//quadCache.inVisibleUnitList.push_back(unit);
							}

							//printf("#2 Unit build added? insideQuad = %d, renderInMap = %d\n",insideQuad,renderInMap);
						}

						if(unitBuildCheckedForRender == false) {
							bool insideQuad 	= visibleQuad.isInside(pos);
							bool renderInMap 	= world->toRenderUnit(pendingUnit);
							if(insideQuad == true && renderInMap == true) {
								quadCache.visibleQuadUnitBuildList.push_back(pendingUnit);
							}
							else {
								//unit->setVisible(false);
								// Currently don't need this list
								//quadCache.inVisibleUnitList.push_back(unit);
							}

							//printf("#3 Unit build added? insideQuad = %d, renderInMap = %d\n",insideQuad,renderInMap);
						}

						//printf("#4 quadCache.visibleQuadUnitBuildList.size() = %d\n",quadCache.visibleQuadUnitBuildList.size());
					}
				}
			}

			if(forceNew == true || visibleQuad != quadCache.lastVisibleQuad) {
				// Object calculations
				const Map *map= world->getMap();
				// clear visibility of old objects
				for(int visibleIndex = 0;
					visibleIndex < quadCache.visibleObjectList.size(); ++visibleIndex){
					quadCache.visibleObjectList[visibleIndex]->setVisible(false);
				}
				quadCache.clearNonVolatileCacheData();

				//int loops1=0;
				PosQuadIterator pqi(map,visibleQuad, Map::cellScale);
				while(pqi.next()) {
					const Vec2i &pos= pqi.getPos();
					if(map->isInside(pos)) {
						//loops1++;
						const Vec2i &mapPos = Map::toSurfCoords(pos);

						//quadCache.visibleCellList.push_back(mapPos);

						SurfaceCell *sc = map->getSurfaceCell(mapPos);
						Object *o = sc->getObject();

						if(VisibleQuadContainerCache::enableFrustumCalcs == true) {
							if(o != NULL) {
								//bool insideQuad 	= PointInFrustum(quadCache.frustumData, o->getPos().x, o->getPos().y, o->getPos().z );
								bool insideQuad 	= CubeInFrustum(quadCache.frustumData, o->getPos().x, o->getPos().y, o->getPos().z, 1);
								if(insideQuad == false) {
									o->setVisible(false);
									continue;
								}
							}
						}

                        bool cellExplored = world->showWorldForPlayer(world->getThisFactionIndex());
                        if(cellExplored == false) {
                            cellExplored = sc->isExplored(world->getThisTeamIndex());
                        }

						bool isExplored = (cellExplored == true && o != NULL);
						//bool isVisible = (sc->isVisible(world->getThisTeamIndex()) && o != NULL);
						bool isVisible = true;

						if(isExplored == true && isVisible == true) {
							quadCache.visibleObjectList.push_back(o);
							o->setVisible(true);
						}
					}
				}

				//printf("Frame # = %d loops1 = %d\n",world->getFrameCount(),loops1);

				//int loops2=0;

				std::map<Vec2i, MarkedCell> markedCells = game->getMapMarkedCellList();

				const Rect2i mapBounds(0, 0, map->getSurfaceW()-1, map->getSurfaceH()-1);
				Quad2i scaledQuad = visibleQuad / Map::cellScale;
				PosQuadIterator pqis(map,scaledQuad);
				while(pqis.next()) {
					const Vec2i &pos= pqis.getPos();
					if(mapBounds.isInside(pos)) {
						//loops2++;
						if(VisibleQuadContainerCache::enableFrustumCalcs == false) {
							quadCache.visibleScaledCellList.push_back(pos);

							if(markedCells.empty() == false) {
								if(markedCells.find(pos) != markedCells.end()) {
									//printf("#1 ******** VISIBLE SCALED CELL FOUND in marked list pos [%s] markedCells.size() = %lu\n",pos.getString().c_str(),markedCells.size());
								//if(markedCells.empty() == false) {
									//SurfaceCell *sc = map->getSurfaceCell(pos);
									//quadCache.visibleScaledCellToScreenPosList[pos]=computeScreenPosition(sc->getVertex());
									updateMarkedCellScreenPosQuadCache(pos);
								}
								else {
									//printf("#1 VISIBLE SCALED CELL NOT FOUND in marked list pos [%s] markedCells.size() = %lu\n",pos.getString().c_str(),markedCells.size());
								}
							}
						}
						else {
							SurfaceCell *sc = map->getSurfaceCell(pos);

							// 2 as last param for CubeInFrustum to get rid of annoying black squares
							bool insideQuad = CubeInFrustum(quadCache.frustumData, sc->getVertex().x, sc->getVertex().y, sc->getVertex().z, 2);

							if(insideQuad == true) {
								quadCache.visibleScaledCellList.push_back(pos);

								if(markedCells.empty() == false) {
									if(markedCells.find(pos) != markedCells.end()) {
										//printf("#2 ******** VISIBLE SCALED CELL FOUND in marked list pos [%s] markedCells.size() = %lu\n",pos.getString().c_str(),markedCells.size());
									//if(markedCells.empty() == false) {
										//quadCache.visibleScaledCellToScreenPosList[pos]=computeScreenPosition(sc->getVertex());
										updateMarkedCellScreenPosQuadCache(pos);
									}
									else {
										//printf("#2 VISIBLE SCALED CELL NOT FOUND in marked list pos [%s] markedCells.size() = %lu\n",pos.getString().c_str(),markedCells.size());
									}
								}
							}
						}
					}
				}
				//printf("Frame # = %d loops2 = %d\n",world->getFrameCount(),loops2);
			}
			quadCache.cacheFrame = world->getFrameCount();
			quadCache.lastVisibleQuad = visibleQuad;
		}
	}

	return quadCache;
}

void Renderer::updateMarkedCellScreenPosQuadCache(Vec2i pos) {
	const World *world= game->getWorld();
	const Map *map= world->getMap();

	SurfaceCell *sc = map->getSurfaceCell(pos);
	quadCache.visibleScaledCellToScreenPosList[pos]=computeScreenPosition(sc->getVertex());
}

void Renderer::forceQuadCacheUpdate() {
	quadCache.cacheFrame = -1;

	Vec2i clearPos(-1,-1);
	quadCache.lastVisibleQuad.p[0] = clearPos;
	quadCache.lastVisibleQuad.p[1] = clearPos;
	quadCache.lastVisibleQuad.p[2] = clearPos;
	quadCache.lastVisibleQuad.p[3] = clearPos;
}

std::pair<bool,Vec3f> Renderer::posInCellQuadCache(Vec2i pos) {
	std::pair<bool,Vec3f> result = make_pair(false,Vec3f());
	if(std::find(
			quadCache.visibleScaledCellList.begin(),
			quadCache.visibleScaledCellList.end(),
			pos) != quadCache.visibleScaledCellList.end()) {
		result.first = true;
		result.second = quadCache.visibleScaledCellToScreenPosList[pos];
	}
	return result;
}

Vec3f Renderer::getMarkedCellScreenPosQuadCache(Vec2i pos) {
	Vec3f result(-1,-1,-1);
	if(std::find(
			quadCache.visibleScaledCellList.begin(),
			quadCache.visibleScaledCellList.end(),
			pos) != quadCache.visibleScaledCellList.end()) {
		result = quadCache.visibleScaledCellToScreenPosList[pos];
	}
	return result;
}

void Renderer::beginRenderToTexture(Texture2D **renderToTexture) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	static bool supportFBOs = Texture2DGl().supports_FBO_RBO();

	if(supportFBOs == true && renderToTexture != NULL) {
		Config &config= Config::getInstance();
		Texture2D::Filter textureFilter = strToTextureFilter(config.getString("Filter"));
		int maxAnisotropy				= config.getInt("FilterMaxAnisotropy");

		const Metrics &metrics	= Metrics::getInstance();

		*renderToTexture = GraphicsInterface::getInstance().getFactory()->newTexture2D();
		Texture2DGl *texture = static_cast<Texture2DGl *>(*renderToTexture);
		texture->setMipmap(false);
		Pixmap2D *pixmapScreenShot = texture->getPixmap();
		pixmapScreenShot->init(metrics.getScreenW(), metrics.getScreenH(), 4);
		texture->setForceCompressionDisabled(true);
		texture->init(textureFilter,maxAnisotropy);
		texture->setup_FBO_RBO();

		assertGl();

		if(texture->checkFrameBufferStatus() == false) {
			//printf("******************** WARNING CANNOT Attach to FBO!\n");
			texture->end();
			delete texture;
			*renderToTexture=NULL;
		}
	}
}

void Renderer::endRenderToTexture(Texture2D **renderToTexture) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	static bool supportFBOs = Texture2DGl().supports_FBO_RBO();

	if(supportFBOs == true && renderToTexture != NULL && *renderToTexture != NULL) {
		Texture2DGl *texture = static_cast<Texture2DGl *>(*renderToTexture);
		if(texture != NULL) {
			texture->dettachFrameBufferFromTexture();
		}

		assertGl();
	}
}

void Renderer::renderMapPreview( const MapPreview *map, bool renderAll,
								 int screenPosX, int screenPosY,
								 Texture2D **renderToTexture) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	static bool supportFBOs = Texture2DGl().supports_FBO_RBO();
	if(Config::getInstance().getBool("LegacyMapPreviewRendering","false") == true) {
		supportFBOs = false;
	}

	//static bool supportFBOs = false;
	const Metrics &metrics= Metrics::getInstance();

	float alt				= 0;
	float showWater			= 0;
	int renderMapHeight		= 64;
	int renderMapWidth		= 64;
	float cellSize			= 2;
	float playerCrossSize	= 2;
	float clientW			= renderMapWidth * cellSize;
	float clientH			= renderMapHeight * cellSize;;
	float minDimension 		= std::min(metrics.getVirtualW(), metrics.getVirtualH());

	// stretch small maps to 128x128
	if(map->getW() < map->getH()) {
		cellSize = cellSize * renderMapHeight / map->getH();
	}
	else {
		cellSize = cellSize * renderMapWidth / map->getW();
	}

	assertGl();

	if(supportFBOs == true && renderToTexture != NULL) {
		Config &config= Config::getInstance();
		Texture2D::Filter textureFilter = strToTextureFilter(config.getString("Filter"));
		int maxAnisotropy				= config.getInt("FilterMaxAnisotropy");

		*renderToTexture = GraphicsInterface::getInstance().getFactory()->newTexture2D();
		Texture2DGl *texture = static_cast<Texture2DGl *>(*renderToTexture);
		texture->setMipmap(false);
		Pixmap2D *pixmapScreenShot = texture->getPixmap();
		pixmapScreenShot->init(minDimension, minDimension, 4);
		texture->setForceCompressionDisabled(true);
		texture->init(textureFilter,maxAnisotropy);
		texture->setup_FBO_RBO();

		assertGl();

		if(texture->checkFrameBufferStatus() == false) {
			//printf("******************** WARNING CANNOT Attach to FBO!\n");
			texture->end();
			delete texture;
			*renderToTexture=NULL;
		}
	}

	if(supportFBOs == true && renderToTexture != NULL && *renderToTexture != NULL) {
		cellSize  =1;
		clientW = minDimension;
		clientH = minDimension;
		int mapMaxDimensionSize = std::max(map->getW(),map->getH());
		switch(mapMaxDimensionSize) {
			case 8:
				cellSize = 96;
				break;
			case 16:
				cellSize = 48;
				break;
			case 32:
				cellSize = 24;
				break;
			case 64:
				cellSize = 12;
				break;
			case 128:
				cellSize = 6;
				break;
			case 256:
				cellSize = 3;
				break;
			case 512:
				cellSize = 2;
				break;
		}
	}

	glFrontFace(GL_CW);
	glEnable(GL_CULL_FACE);

	glMatrixMode(GL_PROJECTION);
    glPushMatrix();
	glLoadIdentity();

	assertGl();

	GLint viewport[4];	// Where The original Viewport Values Will Be Stored

	if(supportFBOs == true && renderToTexture != NULL && *renderToTexture != NULL) {
		glGetIntegerv(GL_VIEWPORT, viewport);
		glOrtho(0, clientW, 0, clientH, 0, 1);
		glViewport(0, 0, clientW, clientH);
	}
	else {
		glOrtho(0, metrics.getVirtualW(), 0, metrics.getVirtualH(), 0, 1);
	}

	glMatrixMode(GL_MODELVIEW);

	glPushMatrix();
	glLoadIdentity();

	if(supportFBOs == false || renderToTexture == NULL ||  *renderToTexture == NULL) {
		glTranslatef(static_cast<float>(screenPosX),static_cast<float>(screenPosY)-clientH,0.0f);
	}

	assertGl();

	glPushAttrib(GL_CURRENT_BIT);
	glLineWidth(1);
	glColor3f(0, 0, 0);

	for (int j = 0; j < map->getH(); j++) {
		for (int i = 0; i < map->getW(); i++) {
			//surface
			alt = map->getHeight(i, j) / 20.f;
			showWater = map->getWaterLevel()/ 20.f - alt;
			showWater = (showWater > 0)? showWater:0;
			Vec3f surfColor;
			switch (map->getSurface(i, j)) {
				case st_Grass:
					surfColor = Vec3f(0.0, 0.8f * alt, 0.f + showWater);
					break;
				case st_Secondary_Grass:
					surfColor = Vec3f(0.4f * alt, 0.6f * alt, 0.f + showWater);
					break;
				case st_Road:
					surfColor = Vec3f(0.6f * alt, 0.3f * alt, 0.f + showWater);
					break;
				case st_Stone:
					surfColor = Vec3f(0.7f * alt, 0.7f * alt, 0.7f * alt + showWater);
					break;
				case st_Ground:
					surfColor = Vec3f(0.7f * alt, 0.5f * alt, 0.3f * alt + showWater);
					break;
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
					case 0:
						glColor3f(0.f, 0.f, 0.f);
						break;
					case 1:
						glColor3f(1.f, 0.f, 0.f);
						break;
					case 2:
						glColor3f(1.f, 1.f, 1.f);
						break;
					case 3:
						glColor3f(0.5f, 0.5f, 1.f);
						break;
					case 4:
						glColor3f(0.f, 0.f, 1.f);
						break;
					case 5:
						glColor3f(0.5f, 0.5f, 0.5f);
						break;
					case 6:
						glColor3f(1.f, 0.8f, 0.5f);
						break;
					case 7:
						glColor3f(0.f, 1.f, 1.f);
						break;
					case 8:
						glColor3f(0.7f, 0.1f, 0.3f);
						break;
					case 9:
						glColor3f(0.5f, 1.f, 0.1f);
						break;
					case 10:
						glColor3f(1.f, 0.2f, 0.8f);
						break;// we don't render unvisible blocking objects
				}

				if ( renderAll && (map->getObject(i, j) != 0) && (map->getObject(i, j) != 10) ) {
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

	assertGl();

	if(supportFBOs == true && renderToTexture != NULL && *renderToTexture != NULL) {
		glLineWidth(14);
		playerCrossSize = 24;
	}
	else {
		// force playerCrossSize to be at least of size 4
		if(cellSize < 4) {
			playerCrossSize = 4;
		}
		else {
			playerCrossSize = cellSize;
		}
	}

	assertGl();

	for (int i = 0; i < map->getMaxFactions(); i++) {
		switch (i) {
			case 0:
				glColor3f(1.f, 0.f, 0.f);
				break;
			case 1:
				glColor3f(0.f, 0.f, 1.f);
				break;
			case 2:
				glColor3f(0.f, 1.f, 0.f);
				break;
			case 3:
				glColor3f(1.f, 1.f, 0.f);
				break;
			case 4:
				glColor3f(1.f, 1.f, 1.f);
				break;
			case 5:
				glColor3f(0.f, 1.f, 0.8f);
				break;
			case 6:
				glColor3f(1.f, 0.5f, 0.f);
				break;
			case 7:
				glColor3f(1.f, 0.5f, 1.f);
				break;
   		}
		glBegin(GL_LINES);
		glVertex2f((map->getStartLocationX(i) - 1) * cellSize, clientH - (map->getStartLocationY(i) - 1) * cellSize);
		glVertex2f((map->getStartLocationX(i) + 1) * cellSize + playerCrossSize, clientH - (map->getStartLocationY(i) + 1) * cellSize - playerCrossSize);
		glVertex2f((map->getStartLocationX(i) - 1) * cellSize, clientH - (map->getStartLocationY(i) + 1) * cellSize - playerCrossSize);
		glVertex2f((map->getStartLocationX(i) + 1) * cellSize + playerCrossSize, clientH - (map->getStartLocationY(i) - 1) * cellSize);
		glEnd();
	}

	assertGl();

	glLineWidth(1);

	glPopMatrix();
	glPopAttrib();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	if(supportFBOs == true && renderToTexture != NULL && *renderToTexture != NULL) {
		Texture2DGl *texture = static_cast<Texture2DGl *>(*renderToTexture);
		if(texture != NULL) {
			texture->dettachFrameBufferFromTexture();
		}

		glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

		assertGl();
	}

	assertGl();
}

// setLastRenderFps and calculate shadowsOffDueToMinRender
void Renderer::setLastRenderFps(int value) {
	 	lastRenderFps = value;
	 	smoothedRenderFps=(MIN_FPS_NORMAL_RENDERING*smoothedRenderFps+lastRenderFps)/(MIN_FPS_NORMAL_RENDERING+1.0f);

		if(smoothedRenderFps>=MIN_FPS_NORMAL_RENDERING_TOP_THRESHOLD){
			shadowsOffDueToMinRender=false;
		}
		if(smoothedRenderFps<=MIN_FPS_NORMAL_RENDERING){
			 shadowsOffDueToMinRender=true;
		}
}

uint64 Renderer::getCurrentPixelByteCount(ResourceScope rs) const {
	uint64 result = 0;
	for(int i = (rs == rsCount ? 0 : rs); i < rsCount; ++i) {
		if(textureManager[i] != NULL) {
			const Shared::Graphics::TextureContainer &textures = textureManager[i]->getTextures();
			for(int j = 0; j < textures.size(); ++j) {
				const Texture *texture = textures[j];
				result += texture->getPixelByteCount();
			}
			if(rs != rsCount) {
				break;
			}
		}
	}

	return result;
}

Texture2D * Renderer::preloadTexture(string logoFilename) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] logoFilename [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,logoFilename.c_str());

	Texture2D *result = NULL;
	if(logoFilename != "") {
		// Cache faction preview textures
		string data_path = getGameReadWritePath(GameConstants::path_data_CacheLookupKey);
		std::map<string,Texture2D *> &crcFactionPreviewTextureCache = CacheManager::getCachedItem< std::map<string,Texture2D *> >(GameConstants::factionPreviewTextureCacheLookupKey);

		if(crcFactionPreviewTextureCache.find(logoFilename) != crcFactionPreviewTextureCache.end()) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] logoFilename [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,logoFilename.c_str());

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] load texture from cache [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,logoFilename.c_str());

			result = crcFactionPreviewTextureCache[logoFilename];
		}
		else {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] logoFilename [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,logoFilename.c_str());
			Renderer &renderer= Renderer::getInstance();
			result = renderer.newTexture2D(rsGlobal);
			if(result) {
				result->setMipmap(true);
				result->load(logoFilename);
				//renderer.initTexture(rsGlobal,result);
			}

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] add texture to manager and cache [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,logoFilename.c_str());

			crcFactionPreviewTextureCache[logoFilename] = result;
		}
	}

	return result;
}

Texture2D * Renderer::findFactionLogoTexture(string logoFilename) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] logoFilename [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,logoFilename.c_str());

	Texture2D *result = preloadTexture(logoFilename);
	if(result != NULL && result->getInited() == false) {
		Renderer &renderer= Renderer::getInstance();
		renderer.initTexture(rsGlobal,result);
	}

	return result;
}

void Renderer::cycleShowDebugUILevel() {
	//printf("#1 showDebugUILevel = %d, debugui_fps = %d, debugui_unit_titles = %d\n",showDebugUILevel,debugui_fps,debugui_unit_titles);

	if((showDebugUILevel & debugui_fps) != debugui_fps ||
		(showDebugUILevel & debugui_unit_titles) != debugui_unit_titles) {
		showDebugUILevel  |= debugui_fps;
		showDebugUILevel  |= debugui_unit_titles;
	}
	else {
		showDebugUILevel  = debugui_fps;
	}

	//printf("#2 showDebugUILevel = %d, debugui_fps = %d, debugui_unit_titles = %d\n",showDebugUILevel,debugui_fps,debugui_unit_titles);
}

void Renderer::renderFPSWhenEnabled(int lastFps) {
	if(getShowDebugUI() == true) {
		CoreData &coreData= CoreData::getInstance();
		if(Renderer::renderText3DEnabled) {
			renderText3D(
				"FPS: " + intToStr(lastFps),
				coreData.getMenuFontNormal3D(), Vec3f(1.f), 10, 10, false);
		}
		else {
			renderText(
				"FPS: " + intToStr(lastFps),
				coreData.getMenuFontNormal(), Vec3f(1.f), 10, 10, false);
		}
	}
}

void Renderer::renderPopupMenu(PopupMenu *menu) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	if(menu->getVisible() == false || menu->getEnabled() == false) {
		return;
	}

	//background
	glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
	glEnable(GL_BLEND);

	glColor4f(0.0f, 0.0f, 0.0f, 0.8f) ;
	glBegin(GL_TRIANGLE_STRIP);
		glVertex2i(menu->getX(), menu->getY() + 9 * menu->getH() / 10);
		glVertex2i(menu->getX(), menu->getY());
		glVertex2i(menu->getX() + menu->getW(), menu->getY() + 9 * menu->getH() / 10);
		glVertex2i(menu->getX() + menu->getW(), menu->getY());
	glEnd();

	glColor4f(0.0f, 0.0f, 0.0f, 0.8f) ;
	glBegin(GL_TRIANGLE_STRIP);
		glVertex2i(menu->getX(), menu->getY() + menu->getH());
		glVertex2i(menu->getX(), menu->getY() + 9 * menu->getH() / 10);
		glVertex2i(menu->getX() + menu->getW(), menu->getY() + menu->getH());
		glVertex2i(menu->getX() + menu->getW(), menu->getY() + 9 * menu->getH() / 10);
	glEnd();

	glBegin(GL_LINE_LOOP);
		glColor4f(0.5f, 0.5f, 0.5f, 0.25f) ;
		glVertex2i(menu->getX(), menu->getY());

		glColor4f(0.0f, 0.0f, 0.0f, 0.25f) ;
		glVertex2i(menu->getX() + menu->getW(), menu->getY());

		glColor4f(0.5f, 0.5f, 0.5f, 0.25f) ;
		glVertex2i(menu->getX() + menu->getW(), menu->getY() + menu->getH());

		glColor4f(0.25f, 0.25f, 0.25f, 0.25f) ;
		glVertex2i(menu->getX(), menu->getY() + menu->getH());
	glEnd();

	glBegin(GL_LINE_STRIP);
		glColor4f(1.0f, 1.0f, 1.0f, 0.25f) ;
		glVertex2i(menu->getX(), menu->getY() + 90*menu->getH()/100);

		glColor4f(0.5f, 0.5f, 0.5f, 0.25f) ;
		glVertex2i(menu->getX()+ menu->getW(), menu->getY() + 90*menu->getH()/100);
	glEnd();

	glPopAttrib();

	Vec4f fontColor;
	//if(game!=NULL){
	//	fontColor=game->getGui()->getDisplay()->getColor();
	//}
	//else {
		// white shadowed is default ( in the menu for example )
		fontColor=Vec4f(1.f, 1.f, 1.f, 1.0f);
	//}

	if(renderText3DEnabled == true) {
		//text
		renderTextBoundingBox3D(
				menu->getHeader(), menu->getFont3D(),fontColor,
				menu->getX(), menu->getY()+93*menu->getH()/100,menu->getW(),0,
			true,false, false,-1 );

	}
	else {
		//text
		int renderX = (menu->getX() + (menu->getW() / 2));
		//int renderY = (menu->getY() + (menu->getH() / 2));
		FontMetrics *fontMetrics= menu->getFont()->getMetrics();
		int renderY = menu->getY() + menu->getH() - fontMetrics->getHeight(menu->getHeader());
		renderTextShadow(
				menu->getHeader(), menu->getFont(),fontColor,
				renderX, renderY,
			true);

		//renderText(button->getText(), button->getFont(), color,x + (w / 2), y + (h / 2), true);
	}

	//buttons
//	int maxButtonWidth = -1;
	std::vector<GraphicButton> &menuItems = menu->getMenuItems();
//	for(unsigned int i = 0; i < menuItems.size(); ++i) {
//		GraphicButton *button = &menuItems[i];
//		int currentButtonWidth = -1;
//		if(renderText3DEnabled == true) {
//			FontMetrics *fontMetrics= menu->getFont3D()->getMetrics();
//			currentButtonWidth = fontMetrics->getTextWidth(button->getText());
//		}
//		else {
//			FontMetrics *fontMetrics= menu->getFont()->getMetrics();
//			currentButtonWidth = fontMetrics->getTextWidth(button->getText());
//		}
//
//		if(maxButtonWidth < 0 || currentButtonWidth > maxButtonWidth) {
//			maxButtonWidth = currentButtonWidth + 5;
//		}
//	}

	for(unsigned int i = 0; i < menuItems.size(); ++i) {
		GraphicButton *button = &menuItems[i];

		//button->setW(maxButtonWidth);
		renderButton(button);
	}

}

void Renderer::setupRenderForVideo() {
	clearBuffers();
	//3d
	reset3dMenu();
	clearZBuffer();
	//2d
	reset2d();
	glClearColor(0.f, 0.f, 0.f, 1.f);
}

void Renderer::renderVideoLoading(int progressPercent) {
	//printf("Rendering progress progressPercent = %d\n",progressPercent);
	setupRenderForVideo();

	Lang &lang= Lang::getInstance();
	string textToRender = lang.get("PleaseWait");
	const Metrics &metrics= Metrics::getInstance();

	static Chrono cycle(true);
	static float anim = 0.0f;
	static bool animCycleUp = true;

	if(CoreData::getInstance().getMenuFontBig3D() != NULL) {

		int renderX = 0;
		int renderY = 0;
		int w= metrics.getVirtualW();
		renderX = (w / 2) - (CoreData::getInstance().getMenuFontBig3D()->getMetrics()->getTextWidth(textToRender) / 2);
		int h= metrics.getVirtualH();
		renderY = (h / 2) + (CoreData::getInstance().getMenuFontBig3D()->getMetrics()->getHeight(textToRender) / 2);

		renderText3D(
				textToRender,
				CoreData::getInstance().getMenuFontBig3D(),
				Vec4f(1.f, 1.f, 0.f,anim),
				renderX, renderY, false);
	}
	else {
		renderText(
				textToRender,
				CoreData::getInstance().getMenuFontBig(),
				Vec4f(1.f, 1.f, 0.f,anim), (metrics.getScreenW() / 2),
				(metrics.getScreenH() / 2), true);
	}
    swapBuffers();

    if(cycle.getCurMillis() % 50 == 0) {
    	if(animCycleUp == true) {
			anim += 0.1;
			if(anim > 1.f) {
				anim= 1.f;
				cycle.reset();
				animCycleUp = false;
			}
    	}
    	else {
			anim -= 0.1;
			if(anim < 0.f) {
				anim= 0.f;
				cycle.reset();
				animCycleUp = true;
			}
    	}
    }
}

}}//end namespace
