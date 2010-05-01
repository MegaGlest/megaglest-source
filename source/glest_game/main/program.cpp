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

    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
    userWantsExit = false;
	msgBox.init("Ok");

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(msg) {
		fprintf(stderr, "%s\n", msg);
		msgBox.setText(msg);
	} else {
		msgBox.setText("Mega-Glest has crashed.");
	}
	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	mouse2dAnim = mouseY = mouseX = 0;
	this->msg = (msg ? msg : "");

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
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
    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	int button= 1;
	if(msgBox.mouseClick(x,y,button)) {

	    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
		program->exit();
		userWantsExit = true;
	}

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void Program::ShowMessageProgramState::keyPress(char c){
    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d] c = [%d]\n",__FILE__,__FUNCTION__,__LINE__,c);

    // if user pressed return we exit
	if(c == 13) {
		program->exit();
		userWantsExit = true;
	}
	else {
        //msgBox.keyPress(c);
	}
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

	soundThreadManager = NULL;
}

void Program::initNormal(WindowGl *window){
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	init(window);

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	setState(new Intro(this));

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
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

	BaseThread::shutdownAndWait(soundThreadManager);
	delete soundThreadManager; 
	soundThreadManager = NULL;
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

    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//render
	programState->render();

	//update camera
	while(updateCameraTimer.isTime()){
		programState->updateCamera();
	}

    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//update world
	while(updateTimer.isTime()){
		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

		GraphicComponent::update();
		programState->update();

		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

		//!!!SoundRenderer::getInstance().update();

		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

		NetworkManager::getInstance().update();

		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}

    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//fps timer
	while(fpsTimer.isTime()){
		programState->tick();
	}

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
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

void Program::setState(ProgramState *programState, bool cleanupOldState)
{
	try {
		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] START\n",__FILE__,__FUNCTION__);

		if(cleanupOldState == true) {
			delete this->programState;
		}

		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] %d\n",__FILE__,__FUNCTION__,__LINE__);

		this->programState= programState;
		programState->load();

		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] %d\n",__FILE__,__FUNCTION__,__LINE__);

		programState->init();

		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] %d\n",__FILE__,__FUNCTION__,__LINE__);

		updateTimer.reset();
		updateCameraTimer.reset();
		fpsTimer.reset();

		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] END\n",__FILE__,__FUNCTION__);
	}
	catch(const exception &e){
		//exceptionMessage(e);
		//throw runtime_error(e.what());
		this->showMessage(e.what());
		setState(new Intro(this));
	}
}

void Program::exit() {

    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	window->destroy();

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

// ==================== PRIVATE ====================

void Program::init(WindowGl *window, bool initSound, bool toggleFullScreen){

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	this->window= window;
	Config &config= Config::getInstance();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    //set video mode
	if(toggleFullScreen == false) {
		setDisplaySettings();
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//window
	window->setText("Glest");
	window->setStyle(config.getBool("Windowed")? wsWindowedFixed: wsFullscreen);
	window->setPos(0, 0);
	window->setSize(config.getInt("ScreenWidth"), config.getInt("ScreenHeight"));
	window->create();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//timers
	fpsTimer.init(1, maxTimes);
	updateTimer.init(GameConstants::updateFps, maxTimes);
	updateCameraTimer.init(GameConstants::cameraFps, maxTimes);

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    //log start
	Logger &logger= Logger::getInstance();
	string logFile = "glest.log";
    if(getGameReadWritePath() != "") {
        logFile = getGameReadWritePath() + logFile;
    }
	logger.setFile(logFile);
	logger.clear();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//lang
	Lang &lang= Lang::getInstance();
	lang.loadStrings(config.getString("Lang"));

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//render
	Renderer &renderer= Renderer::getInstance();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	window->initGl(config.getInt("ColorBits"), config.getInt("DepthBits"), config.getInt("StencilBits"));

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	window->makeCurrentGl();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//coreData, needs renderer, but must load before renderer init
	CoreData &coreData= CoreData::getInstance();
    coreData.load();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//init renderer (load global textures)
	renderer.init();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//sound
	if(initSound == true && toggleFullScreen == false) {
        SoundRenderer &soundRenderer= SoundRenderer::getInstance();
        soundRenderer.init(window);

		BaseThread::shutdownAndWait(soundThreadManager);
		delete soundThreadManager; 
		soundThreadManager = new SimpleTaskThread(&SoundRenderer::getInstance(),0,50);
		soundThreadManager->start();
	}

	NetworkInterface::setAllowGameDataSynchCheck(Config::getInstance().getBool("AllowGameDataSynchCheck","0"));
	NetworkInterface::setAllowDownloadDataSynch(Config::getInstance().getBool("AllowDownloadDataSynch","0"));

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void Program::setDisplaySettings(){

	Config &config= Config::getInstance();

	if(!config.getBool("Windowed")){

		int freq= config.getInt("RefreshFrequency");
		int colorBits= config.getInt("ColorBits");
		int screenWidth= config.getInt("ScreenWidth");
		int screenHeight= config.getInt("ScreenHeight");

        if(config.getBool("AutoMaxFullScreen","false") == true) {
            getFullscreenVideoInfo(colorBits,screenWidth,screenHeight);
            config.setInt("ColorBits",colorBits);
            config.setInt("ScreenWidth",screenWidth);
            config.setInt("ScreenHeight",screenHeight);
        }

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
    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

    ProgramState *originalState = NULL;
    if(this->programState) {
        //delete programState;
        originalState = this->programState;
    }

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

    showCursor(true);

    //SDL_WM_ToggleFullScreen(SDL_GetVideoSurface());
    ShowMessageProgramState *showMsg = new ShowMessageProgramState(this, msg);
    //this->programState = showMsg;

    this->programState = NULL;
    setState(showMsg);

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

    while(Window::handleEvent() && showMsg->wantExit() == false) {
        loop();
    }

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

    delete this->programState;
	this->programState = NULL;

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//showCursor(Config::getInstance().getBool("Windowed"));
    showCursor(false);

    //MainWindow *mainWindow= new MainWindow(this);
	init(this->window,false);
    //setState(originalState);

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] START\n",__FILE__,__FUNCTION__);

	delete this->programState;

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] %d\n",__FILE__,__FUNCTION__,__LINE__);

	this->programState= originalState;
	//programState->load();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] %d\n",__FILE__,__FUNCTION__,__LINE__);

	//programState->init();

    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] %d\n",__FILE__,__FUNCTION__,__LINE__);

	//updateTimer.reset();
	//updateCameraTimer.reset();
	//fpsTimer.reset();


	//this->programState = originalState;

    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
}


}}//end namespace
