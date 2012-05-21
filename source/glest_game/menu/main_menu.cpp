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

#include "main_menu.h"

#include "renderer.h"
#include "sound.h"
#include "config.h"
#include "program.h"
#include "game_util.h"
#include "game.h"
#include "platform_util.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "faction.h"
#include "metrics.h"
#include "network_manager.h"
#include "network_message.h"
#include "socket.h"
#include "menu_state_root.h"
#include "video_player.h"

#include "leak_dumper.h"

using namespace Shared::Sound;
using namespace Shared::Platform;
using namespace Shared::Util;
using namespace Shared::Graphics;
using namespace	Shared::Xml;

namespace Glest{ namespace Game{

// =====================================================
// 	class MainMenu
// =====================================================

// ===================== PUBLIC ========================
MenuState * MainMenu::oldstate=NULL;

MainMenu::MainMenu(Program *program) : ProgramState(program), menuBackgroundVideo(NULL) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	mouseX=100;
	mouseY=100;

	state= NULL;
	this->program= program;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	setState(new MenuStateRoot(program, this));

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void MainMenu::reloadUI() {
	if(state) {
		state->reloadUI();
	}
}

MainMenu::~MainMenu() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(menuBackgroundVideo != NULL) {
		menuBackgroundVideo->closePlayer();
		delete menuBackgroundVideo;
		menuBackgroundVideo = NULL;
	}
	delete state;
	state = NULL;
	delete oldstate;
	oldstate = NULL;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	Renderer::getInstance().endMenu();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//SoundRenderer &soundRenderer= SoundRenderer::getInstance();
	//soundRenderer.stopAllSounds();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void MainMenu::init() {
	Renderer::getInstance().initMenu(this);

	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false &&
		Shared::Graphics::VideoPlayer::hasBackEndVideoPlayer() == true &&
		CoreData::getInstance().hasMainMenuVideoFilename() == true) {
		string introVideoFile = CoreData::getInstance().getMainMenuVideoFilename();
		//if(introVideoFile != "" && fileExists(introVideoFile)) {
			Context *c= GraphicsInterface::getInstance().getCurrentContext();
			SDL_Surface *screen = static_cast<ContextGl*>(c)->getPlatformContextGlPtr()->getScreen();

			string vlcPluginsPath = Config::getInstance().getString("VideoPlayerPluginsPath","");
			//printf("screen->w = %d screen->h = %d screen->format->BitsPerPixel = %d\n",screen->w,screen->h,screen->format->BitsPerPixel);
			menuBackgroundVideo = new VideoPlayer(introVideoFile.c_str(),
								screen,
								0,0,
								screen->w,
								screen->h,
								screen->format->BitsPerPixel,
								vlcPluginsPath,
								SystemFlags::VERBOSE_MODE_ENABLED);
			menuBackgroundVideo->initPlayer();
		//}
	}
}

//asynchronus render update
void MainMenu::render() {
	Renderer &renderer= Renderer::getInstance();

	canRender();
	incrementFps();

	if(state->isMasterserverMode() == false) {
		if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false) {
			renderer.clearBuffers();

			//3d
			renderer.reset3dMenu();
			renderer.clearZBuffer();

			if(menuBackgroundVideo == NULL) {
				renderer.loadCameraMatrix(menuBackground.getCamera());
				renderer.renderMenuBackground(&menuBackground);
				renderer.renderParticleManager(rsMenu);
			}

			//2d
			renderer.reset2d();

			if(menuBackgroundVideo != NULL) {
				if(menuBackgroundVideo->isPlaying() == true) {
					menuBackgroundVideo->playFrame(false);
				}
			}
		}
		state->render();

		if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false) {
			renderer.renderMouse2d(mouseX, mouseY, mouse2dAnim);
			renderer.renderFPSWhenEnabled(lastFps);
			renderer.swapBuffers();
		}
	}
}

//syncronus update
void MainMenu::update(){
	if(menuBackgroundVideo == NULL) {
		Renderer::getInstance().updateParticleManager(rsMenu);
	}
	mouse2dAnim= (mouse2dAnim +1) % Renderer::maxMouse2dAnim;
	if(menuBackgroundVideo == NULL) {
		menuBackground.update();
	}
	state->update();
}

//event magangement: mouse click
void MainMenu::mouseMove(int x, int y, const MouseState *ms){
    mouseX= x; mouseY= y;
	state->mouseMove(x, y, ms);
}

//returns if exiting
void MainMenu::mouseDownLeft(int x, int y){
	state->mouseClick(x, y, mbLeft);
}

void MainMenu::mouseDownRight(int x, int y){
	state->mouseClick(x, y, mbRight);
}

void MainMenu::keyDown(SDL_KeyboardEvent key) {
	state->keyDown(key);
}

void MainMenu::keyUp(SDL_KeyboardEvent key) {
	state->keyUp(key);
}

void MainMenu::keyPress(SDL_KeyboardEvent c) {
	state->keyPress(c);
}

void MainMenu::setState(MenuState *newstate) {
    //printf("In [%s::%s Line: %d] oldstate [%p] newstate [%p] this->state [%p]\n",__FILE__,__FUNCTION__,__LINE__,oldstate,newstate,this->state);

    //delete this->state;
    //this->state = newstate;

	if(oldstate != NULL && oldstate != newstate) {
		delete oldstate;

        //printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		if(oldstate != this->state) {
			oldstate=this->state;
			//printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		}
		else {
			oldstate = NULL;
			//printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		}
	}
	else {
		oldstate=this->state;
		//printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}
	//printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	this->state= newstate;

	GraphicComponent::resetFade();
	menuBackground.setTargetCamera(newstate->getCamera());
}

bool MainMenu::isInSpecialKeyCaptureEvent() {
	return state->isInSpecialKeyCaptureEvent();
}

void MainMenu::consoleAddLine(string line) {
	if(state != NULL) {
		state->consoleAddLine(line);
	}
}

// =====================================================
// 	class MenuState
// =====================================================

MenuState::MenuState(Program *program, MainMenu *mainMenu, const string &stateName){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	this->containerName="";
	this->program= program;
	this->mainMenu= mainMenu;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
	//switch on menu music again, it might be muted
	Config &config = Config::getInstance();
	float configVolume = (config.getInt("SoundVolumeMusic") / 100.f);
	CoreData::getInstance().getMenuMusic()->setVolume(configVolume);

	string data_path = getGameReadWritePath(GameConstants::path_data_CacheLookupKey);

	//camera
	XmlTree xmlTree;
	xmlTree.load(getGameCustomCoreDataPath(data_path, "data/core/menu/menu.xml"),Properties::getTagReplacementValues());
	const XmlNode *menuNode= xmlTree.getRootNode();
	const XmlNode *cameraNode= menuNode->getChild("camera");

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//position
	const XmlNode *positionNode= cameraNode->getChild(stateName + "-position");
	Vec3f startPosition;
    startPosition.x= positionNode->getAttribute("x")->getFloatValue();
	startPosition.y= positionNode->getAttribute("y")->getFloatValue();
	startPosition.z= positionNode->getAttribute("z")->getFloatValue();
	camera.setPosition(startPosition);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//rotation
	const XmlNode *rotationNode= cameraNode->getChild(stateName + "-rotation");
	Vec3f startRotation;
    startRotation.x= rotationNode->getAttribute("x")->getFloatValue();
	startRotation.y= rotationNode->getAttribute("y")->getFloatValue();
	startRotation.z= rotationNode->getAttribute("z")->getFloatValue();
	camera.setOrientation(Quaternion(EulerAngles(
		degToRad(startRotation.x),
		degToRad(startRotation.y),
		degToRad(startRotation.z))));

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

MenuState::~MenuState() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
	GraphicComponent::clearRegisteredComponents(this->containerName);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void MenuState::consoleAddLine(string line) {
	console.addLine(line);
}

void MenuState::reloadUI() {
	console.resetFonts();
}

}}//end namespace
