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

#include "program.h"

#include "sound.h"
#include "renderer.h"
#include "config.h"
#include "game.h"
#include "main_menu.h"
#include "intro.h"
#include "world.h"
#include "main.h"
#include "sound_renderer.h"
#include "logger.h"
#include "profiler.h"
#include "core_data.h"
#include "metrics.h"
#include "network_manager.h"
#include "menu_state_custom_game.h"
#include "menu_state_join_game.h"
#include "leak_dumper.h"

using namespace Shared::Util;
using namespace Shared::Graphics;
using namespace Shared::Graphics::Gl;

// =====================================================
// 	class Program
// =====================================================

namespace Glest{ namespace Game{

const int Program::maxTimes= 10;
Program *Program::singleton = NULL;

// =====================================================
// 	class Program::CrashProgramState
// =====================================================

Program::ShowMessageProgramState::ShowMessageProgramState(Program *program, const char *msg) :
		ProgramState(program) {

    //if(Socket::enableDebugText) printf("In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
    userWantsExit = false;
	msgBox.init("Ok");

	//if(Socket::enableDebugText) printf("In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(msg) {
		fprintf(stderr, "%s\n", msg);
		msgBox.setText(msg);
	} else {
		msgBox.setText("Mega-Glest has crashed.");
	}
	//if(Socket::enableDebugText) printf("In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	mouse2dAnim = mouseY = mouseX = 0;
	this->msg = (msg ? msg : "");

	//if(Socket::enableDebugText) printf("In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void Program::ShowMessageProgramState::render() {
	Renderer &renderer= Renderer::getInstance();
	renderer.clearBuffers();
	renderer.reset2d();
	renderer.renderMessageBox(&msgBox);
	renderer.renderMouse2d(mouseX, mouseY, mouse2dAnim);
	renderer.swapBuffers();
}

void Program::ShowMessageProgramState::mouseDownLeft(int x, int y) {
    //if(Socket::enableDebugText) printf("In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(msgBox.mouseClick(x,y)) {

	    //if(Socket::enableDebugText) printf("In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
		program->exit();
		userWantsExit = true;
	}

	//if(Socket::enableDebugText) printf("In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void Program::ShowMessageProgramState::mouseMove(int x, int y, const MouseState &mouseState) {
	mouseX = x;
	mouseY = y;
	msgBox.mouseMove(x, y);
}

void Program::ShowMessageProgramState::update() {
	mouse2dAnim = (mouse2dAnim +1) % Renderer::maxMouse2dAnim;
}

// ===================== PUBLIC ========================

Program::Program() {
	programState= NULL;
	singleton = this;
}

void Program::initNormal(WindowGl *window){
	init(window);
	setState(new Intro(this));
}

void Program::initServer(WindowGl *window){
	MainMenu* mainMenu= NULL;

	init(window);
	mainMenu= new MainMenu(this);
	setState(mainMenu);
	mainMenu->setState(new MenuStateCustomGame(this, mainMenu, true));
}

void Program::initClient(WindowGl *window, const Ip &serverIp){
	MainMenu* mainMenu= NULL;

	init(window);
	mainMenu= new MainMenu(this);
	setState(mainMenu);
	mainMenu->setState(new MenuStateJoinGame(this, mainMenu, true, serverIp));
}

Program::~Program(){
	delete programState;
	programState = NULL;

	Renderer::getInstance().end();

	//restore video mode
	restoreDisplaySettings();
	singleton = NULL;
}

void Program::mouseDownLeft(int x, int y){
	const Metrics &metrics= Metrics::getInstance();
	programState->mouseDownLeft(metrics.toVirtualX(x), metrics.toVirtualY(y));
}

void Program::mouseUpLeft(int x, int y){
	const Metrics &metrics= Metrics::getInstance();
	programState->mouseUpLeft(metrics.toVirtualX(x), metrics.toVirtualY(y));
}

void Program::mouseDownRight(int x, int y){
	const Metrics &metrics= Metrics::getInstance();
	programState->mouseDownRight(metrics.toVirtualX(x), metrics.toVirtualY(y));
}

void Program::mouseDoubleClickLeft(int x, int y){
	const Metrics &metrics= Metrics::getInstance();
	programState->mouseDoubleClickLeft(metrics.toVirtualX(x), metrics.toVirtualY(y));
}

void Program::mouseMove(int x, int y, const MouseState *ms){
	const Metrics &metrics= Metrics::getInstance();
	programState->mouseMove(metrics.toVirtualX(x), metrics.toVirtualY(y), ms);
}

void Program::keyDown(char key){
	//delegate event
	programState->keyDown(key);
}

void Program::keyUp(char key){
	programState->keyUp(key);
}

void Program::keyPress(char c){
	programState->keyPress(c);
}

void Program::loop(){

	//render
	programState->render();

	//update camera
	while(updateCameraTimer.isTime()){
		programState->updateCamera();
	}

	//update world
	while(updateTimer.isTime()){
		GraphicComponent::update();
		programState->update();
		SoundRenderer::getInstance().update();
		NetworkManager::getInstance().update();
	}

	//fps timer
	while(fpsTimer.isTime()){
		programState->tick();
	}
}

void Program::resize(SizeState sizeState){

	switch(sizeState){
	case ssMinimized:
		//restoreVideoMode();
		break;
	case ssMaximized:
	case ssRestored:
		//setDisplaySettings();
		//renderer.reloadResources();
		break;
	}
}

// ==================== misc ====================

void Program::setState(ProgramState *programState)
{

	//if(Socket::enableDebugText) printf("In [%s::%s] START\n",__FILE__,__FUNCTION__);

	delete this->programState;

    //if(Socket::enableDebugText) printf("In [%s::%s] %d\n",__FILE__,__FUNCTION__,__LINE__);

	this->programState= programState;
	programState->load();

	//if(Socket::enableDebugText) printf("In [%s::%s] %d\n",__FILE__,__FUNCTION__,__LINE__);

	programState->init();

    //if(Socket::enableDebugText) printf("In [%s::%s] %d\n",__FILE__,__FUNCTION__,__LINE__);

	updateTimer.reset();
	updateCameraTimer.reset();
	fpsTimer.reset();

	//if(Socket::enableDebugText) printf("In [%s::%s] END\n",__FILE__,__FUNCTION__);
}

void Program::exit() {

    //if(Socket::enableDebugText) printf("In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	window->destroy();

	//if(Socket::enableDebugText) printf("In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

// ==================== PRIVATE ====================

void Program::init(WindowGl *window){

	this->window= window;
	Config &config= Config::getInstance();

    //set video mode
	setDisplaySettings();

	//window
	window->setText("Glest");
	window->setStyle(config.getBool("Windowed")? wsWindowedFixed: wsFullscreen);
	window->setPos(0, 0);
	window->setSize(config.getInt("ScreenWidth"), config.getInt("ScreenHeight"));
	window->create();

	//timers
	fpsTimer.init(1, maxTimes);
	updateTimer.init(GameConstants::updateFps, maxTimes);
	updateCameraTimer.init(GameConstants::cameraFps, maxTimes);

    //log start
	Logger &logger= Logger::getInstance();
	logger.setFile("glest.log");
	logger.clear();

	//lang
	Lang &lang= Lang::getInstance();
	lang.loadStrings(config.getString("Lang"));

	//render
	Renderer &renderer= Renderer::getInstance();

	window->initGl(config.getInt("ColorBits"), config.getInt("DepthBits"), config.getInt("StencilBits"));
	window->makeCurrentGl();

	//coreData, needs renderer, but must load before renderer init
	CoreData &coreData= CoreData::getInstance();
    coreData.load();

	//init renderer (load global textures)
	renderer.init();

	//sound
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();
	soundRenderer.init(window);

	NetworkInterface::setAllowGameDataSynchCheck(Config::getInstance().getBool("AllowGameDataSynchCheck","0"));
	NetworkInterface::setAllowDownloadDataSynch(Config::getInstance().getBool("AllowDownloadDataSynch","0"));
}

void Program::setDisplaySettings(){

	Config &config= Config::getInstance();

	if(!config.getBool("Windowed")){

		int freq= config.getInt("RefreshFrequency");
		int colorBits= config.getInt("ColorBits");
		int screenWidth= config.getInt("ScreenWidth");
		int screenHeight= config.getInt("ScreenHeight");

		if(!(changeVideoMode(screenWidth, screenHeight, colorBits, freq) ||
			changeVideoMode(screenWidth, screenHeight, colorBits, 0)))
		{
			throw runtime_error(
				"Error setting video mode: " +
				intToStr(screenWidth) + "x" + intToStr(screenHeight) + "x" + intToStr(colorBits));
		}
	}
}

void Program::restoreDisplaySettings(){
	Config &config= Config::getInstance();

	if(!config.getBool("Windowed")){
		restoreVideoMode();
	}
}

void Program::showMessage(const char *msg) {
    //if(Socket::enableDebugText) printf("In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

    ProgramState *originalState = NULL;
    if(this->programState) {
        //delete programState;
        originalState = this->programState;
    }

    //if(Socket::enableDebugText) printf("In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	showCursor(true);

    //SDL_WM_ToggleFullScreen(SDL_GetVideoSurface());
    ShowMessageProgramState *showMsg = new ShowMessageProgramState(this, msg);
    this->programState = showMsg;

    //if(Socket::enableDebugText) printf("In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

    while(Window::handleEvent() && showMsg->wantExit() == false) {
        loop();
    }

    //if(Socket::enableDebugText) printf("In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

    delete this->programState;
	this->programState = NULL;

    //if(Socket::enableDebugText) printf("In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

    //MainWindow *mainWindow= new MainWindow(this);
	init(this->window);
    setState(originalState);
	//this->programState = originalState;

    //if(Socket::enableDebugText) printf("In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
}


}}//end namespace
