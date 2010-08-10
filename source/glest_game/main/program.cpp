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

	//mesage box
	Lang &lang= Lang::getInstance();
	msgBox.init(lang.get("Ok"));
	msgBox.setEnabled(false);
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

	if(msgBox.getEnabled()) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		if(key == vkEscape || key == vkReturn) {
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			msgBox.setEnabled(false);

		}
	}
	//delegate event
	programState->keyDown(key);
}

void Program::keyUp(char key){
	programState->keyUp(key);
}

void Program::keyPress(char c){
	programState->keyPress(c);
}

void Program::mouseDownLeft(int x, int y) {
	if(msgBox.getEnabled()) {
		int button= 1;
		if(msgBox.mouseClick(x, y, button)) {
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			//close message box
			msgBox.setEnabled(false);
		}
	}
}

void Program::eventMouseMove(int x, int y, const MouseState *ms) {
	if (msgBox.getEnabled()) {
		msgBox.mouseMove(x, y);
	}
}

void Program::simpleTask() {
	loopWorker();
}

void Program::loop() {
	loopWorker();
}

void Program::loopWorker() {
	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] programState = %p\n",__FILE__,__FUNCTION__,__LINE__,programState);

	Chrono chrono;
	chrono.start();

	//getWindow()->makeCurrentGl();
	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//render
    assert(programState != NULL);

    if(this->programState->quitTriggered() == true) {
    	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    	Stats endStats = this->programState->quitAndToggleState();

    	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    	Game::exitGameState(this, endStats);

    	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    	return;
    }
    ProgramState *prevState = this->programState;

    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	programState->render();

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d programState->render took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	//update camera
	chrono.start();
	while(updateCameraTimer.isTime()){
		programState->updateCamera();
	}

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d programState->updateCamera took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	//update world
	chrono.start();

	while(prevState == this->programState && updateTimer.isTime()) {
		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

		GraphicComponent::update();
		programState->update();
		if(prevState == this->programState) {
			//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

			if(soundThreadManager == NULL) {
				SoundRenderer::getInstance().update();
			}

			//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

			NetworkManager::getInstance().update();

			//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
		}
	}

	if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(prevState == this->programState) {
		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		//fps timer
		chrono.start();
		while(fpsTimer.isTime()) {
			programState->tick();
		}

		//if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
	}

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
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

void Program::renderProgramMsgBox() {
	if(msgBox.getEnabled()) {
		Renderer &renderer= Renderer::getInstance();
		renderer.renderMessageBox(&msgBox);
	}
}

void Program::setState(ProgramState *programState, bool cleanupOldState)
{
	try {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		bool msgBoxEnabled = msgBox.getEnabled();

		if(dynamic_cast<Game *>(programState) != NULL) {
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			int X = 0;
			int Y = 0;
			SDL_GetMouseState(&X,&Y);
			programState->setStartXY(X,Y);

			SDL_PumpEvents();
			
			showCursor(true);
			SDL_PumpEvents();
			sleep(0);
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		}

		if(cleanupOldState == true) {
			if(this->programState != programState) {
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

				delete this->programState;

				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

				this->programState = NULL;

				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			}
		}

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		//mesage box
		Lang &lang= Lang::getInstance();
		msgBox.init(lang.get("Ok"));
		msgBox.setEnabled(msgBoxEnabled);

		this->programState= programState;
		programState->load();

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		programState->init();

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		updateTimer.reset();
		updateCameraTimer.reset();
		fpsTimer.reset();

		Config &config = Config::getInstance();
		if(config.getBool("No2DMouseRendering","false") == false) {
			showCursor(false);
		}
		sleep(0);

		if(dynamic_cast<Intro *>(programState) != NULL && msgBoxEnabled == true) {
			showCursor(true);
		}

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}
	catch(const exception &e){
		//exceptionMessage(e);
		//throw runtime_error(e.what());
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
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

	window->initGl(config.getInt("ColorBits"), config.getInt("DepthBits"), config.getInt("StencilBits"),config.getBool("HardwareAcceleration","false"),config.getBool("FullScreenAntiAliasing","false"));

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
        bool initOk = soundRenderer.init(window);

        SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] initOk = %d\n",__FILE__,__FUNCTION__,__LINE__,initOk);

        // Test sound system failed
        //initOk = false;
        // END

        if(initOk == false) {
        	string sError = "Sound System could not be initialzed!";
        	this->showMessage(sError.c_str());
        }

		// Run sound streaming in a background thread if enabled
		if(config.getBool("ThreadedSoundStream","false") == true) {
			BaseThread::shutdownAndWait(soundThreadManager);
			delete soundThreadManager;
			soundThreadManager = new SimpleTaskThread(&SoundRenderer::getInstance(),0,50);
			soundThreadManager->setUniqueID(__FILE__);
			soundThreadManager->start();
		}
	}

	NetworkInterface::setAllowGameDataSynchCheck(Config::getInstance().getBool("AllowGameDataSynchCheck","false"));
	NetworkInterface::setAllowDownloadDataSynch(Config::getInstance().getBool("AllowDownloadDataSynch","false"));

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
    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d] msg [%s]\n",__FILE__,__FUNCTION__,__LINE__,msg);

	msgBox.setText(msg);
	//msgBox.setHeader(header);
	msgBox.setEnabled(true);


/*
    int showMouseState = SDL_ShowCursor(SDL_QUERY);

    ProgramState *originalState = NULL;
    if(this->programState) {
        //delete programState;
        originalState = this->programState;
    }

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

    ShowMessageProgramState *showMsg = new ShowMessageProgramState(this, msg);

    this->programState = NULL;
    setState(showMsg);

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

    showCursor(true);

    while(Window::handleEvent() && showMsg->wantExit() == false) {
        loop();
    }

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

    delete this->programState;
	this->programState = NULL;

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	Config &config = Config::getInstance();
	//if(config.getBool("No2DMouseRendering","false") == false) {
	showCursor((showMouseState == SDL_ENABLE));
	//}

	init(this->window,false);

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] START\n",__FILE__,__FUNCTION__);

	delete this->programState;

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] %d\n",__FILE__,__FUNCTION__,__LINE__);

	this->programState= originalState;

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] %d\n",__FILE__,__FUNCTION__,__LINE__);
*/

    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
}


}}//end namespace
