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
#include "menu_state_scenario.h"
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
const int SOUND_THREAD_UPDATE_MILLISECONDS = 25;

bool Program::wantShutdownApplicationAfterGame = false;

// =====================================================
// 	class Program::CrashProgramState
// =====================================================

ProgramState::ProgramState(Program *program) {
	this->program= program;
	this->forceMouseRender = false;
	this->mouseX = 0;
	this->mouseY = 0;
	this->mouse2dAnim = 0;
	this->fps= 0;
	this->lastFps= 0;
	this->startX=0;
	this->startY=0;
}

void ProgramState::incrementFps() {
	fps++;
}

void ProgramState::tick() {
	lastFps= fps;
	fps= 0;
}

bool ProgramState::canRender(bool sleepIfCannotRender) {
	if(lastFps > 800) {
		if(sleepIfCannotRender == true) {
			sleep(1);
		}
		return false;
	}
	return true;
}

void ProgramState::render() {
	Renderer &renderer= Renderer::getInstance();

	canRender();
	incrementFps();

	if(renderer.isMasterserverMode() == false) {
		renderer.clearBuffers();
		renderer.reset2d();
		renderer.renderMessageBox(program->getMsgBox());
		renderer.renderMouse2d(mouseX, mouseY, mouse2dAnim);
		renderer.swapBuffers();
	}
}

void ProgramState::update() {
	mouse2dAnim = (mouse2dAnim +1) % Renderer::maxMouse2dAnim;
}

void ProgramState::mouseMove(int x, int y, const MouseState *mouseState) {
	mouseX = x;
	mouseY = y;
	program->getMsgBox()->mouseMove(x, y);
}

Program::ShowMessageProgramState::ShowMessageProgramState(Program *program, const char *msg) :
		ProgramState(program) {
    userWantsExit = false;
	msgBox.init("Ok");

	if(msg) {
		fprintf(stderr, "%s\n", msg);
		msgBox.setText(msg);
	} else {
		msgBox.setText("Mega-Glest has crashed.");
	}

	mouse2dAnim = mouseY = mouseX = 0;
	this->msg = (msg ? msg : "");
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
	int button= 1;
	if(msgBox.mouseClick(x,y,button)) {
		program->exit();
		userWantsExit = true;
	}
}

void Program::ShowMessageProgramState::keyPress(SDL_KeyboardEvent c) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d] c = [%d]\n",__FILE__,__FUNCTION__,__LINE__,c.keysym.sym);

    // if user pressed return we exit
	//if(c == 13) {
	if(isKeyPressed(SDLK_RETURN,c) == true) {
		program->exit();
		userWantsExit = true;
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
	this->masterserverMode = false;
	this->shutdownApplicationEnabled = false;
	this->skipRenderFrameCount = 0;
	this->programState= NULL;
	this->singleton = this;
	this->soundThreadManager = NULL;

	//mesage box
	Lang &lang= Lang::getInstance();
	msgBox.init(lang.get("Ok"));
	msgBox.setEnabled(false);
}

bool Program::isMasterserverMode() const {
	return this->masterserverMode;
}

void Program::initNormal(WindowGl *window){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	init(window);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	setState(new Intro(this));

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void Program::initServer(WindowGl *window, bool autostart,bool openNetworkSlots,
		bool masterserverMode) {
	MainMenu* mainMenu= NULL;

	this->masterserverMode = masterserverMode;
	init(window);
	mainMenu= new MainMenu(this);
	setState(mainMenu);
	mainMenu->setState(new MenuStateCustomGame(this, mainMenu, openNetworkSlots, false, autostart, NULL, masterserverMode));
}

void Program::initServer(WindowGl *window, GameSettings *settings) {
	MainMenu* mainMenu= NULL;

	init(window);
	mainMenu= new MainMenu(this);
	setState(mainMenu);
	mainMenu->setState(new MenuStateCustomGame(this, mainMenu, false, false, true, settings));
}

void Program::initClient(WindowGl *window, const Ip &serverIp) {
	MainMenu* mainMenu= NULL;

	init(window);
	mainMenu= new MainMenu(this);
	setState(mainMenu);
	mainMenu->setState(new MenuStateJoinGame(this, mainMenu, true, serverIp));
}

void Program::initScenario(WindowGl *window, string autoloadScenarioName) {
	MainMenu* mainMenu= NULL;

	init(window);
	mainMenu= new MainMenu(this);
	setState(mainMenu);
	mainMenu->setState(new MenuStateScenario(this, mainMenu, Config::getInstance().getPathListForType(ptScenarios),autoloadScenarioName));
}

Program::~Program(){
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	delete programState;
	programState = NULL;

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	Renderer::getInstance().end();

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//restore video mode
	restoreDisplaySettings();
	singleton = NULL;

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(soundThreadManager != NULL) {
		BaseThread::shutdownAndWait(soundThreadManager);
		delete soundThreadManager;
		soundThreadManager = NULL;
	}

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void Program::keyDown(SDL_KeyboardEvent key) {
	if(msgBox.getEnabled()) {
		//SDL_keysym keystate = Window::getKeystate();
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		//if(key == vkEscape  || key == SDLK_ESCAPE ||
		//	((key == vkReturn || key == SDLK_RETURN || key == SDLK_KP_ENTER) && !(keystate.mod & (KMOD_LALT | KMOD_RALT)))) {
		if(isKeyPressed(SDLK_ESCAPE,key) == true ||	((isKeyPressed(SDLK_RETURN,key) == true) && !(key.keysym.mod & (KMOD_LALT | KMOD_RALT)))) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			//printf("---> keystate [%d]\n",keystate);
			msgBox.setEnabled(false);

		}
	}
	//delegate event
	programState->keyDown(key);
}

void Program::keyUp(SDL_KeyboardEvent key) {
	programState->keyUp(key);
}

void Program::keyPress(SDL_KeyboardEvent c) {
	programState->keyPress(c);
}

void Program::mouseDownLeft(int x, int y) {
	if(msgBox.getEnabled()) {
		int button= 1;
		if(msgBox.mouseClick(x, y, button)) {
			//if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
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

void Program::simpleTask(BaseThread *callingThread) {
	loopWorker();
}

void Program::loop() {
	loopWorker();
}

void Program::loopWorker() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] ================================= MAIN LOOP START ================================= \n",__FILE__,__FUNCTION__,__LINE__);

	Chrono chronoLoop;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chronoLoop.start();

	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

	//render
    assert(programState != NULL);

    if(this->programState->quitTriggered() == true) {
    	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    	Stats endStats = this->programState->quitAndToggleState();

    	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    	Game::exitGameState(this, endStats);

    	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    	return;
    }
    ProgramState *prevState = this->programState;

    assert(programState != NULL);
    programState->render();

    if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d programState->render took msecs: %lld ==============> MAIN LOOP RENDERING\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
    if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

	//update camera
    if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();
	while(updateCameraTimer.isTime()){
		programState->updateCamera();
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d programState->render took msecs: %lld ==============> MAIN LOOP CAMERA UPDATING\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

	//update world
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();
	int updateCount = 0;
	while(prevState == this->programState && updateTimer.isTime()) {
		Chrono chronoUpdateLoop;
		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chronoUpdateLoop.start();

		GraphicComponent::update();
		programState->update();
		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chronoUpdateLoop.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] programState->update took msecs: %lld, updateCount = %d\n",__FILE__,__FUNCTION__,__LINE__,chronoUpdateLoop.getMillis(),updateCount);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chronoUpdateLoop.getMillis() > 0) chronoUpdateLoop.start();

		if(prevState == this->programState) {
			if(soundThreadManager == NULL || soundThreadManager->isThreadExecutionLagging()) {
				if(soundThreadManager != NULL) {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d] ERROR / WARNING soundThreadManager->isThreadExecutionLagging is TRUE\n",__FILE__,__FUNCTION__,__LINE__);
					SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s %d] ERROR / WARNING soundThreadManager->isThreadExecutionLagging is TRUE\n",__FILE__,__FUNCTION__,__LINE__);
				}
				SoundRenderer::getInstance().update();
				if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chronoUpdateLoop.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] SoundRenderer::getInstance().update() took msecs: %lld, updateCount = %d\n",__FILE__,__FUNCTION__,__LINE__,chronoUpdateLoop.getMillis(),updateCount);
				if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chronoUpdateLoop.getMillis() > 0) chronoUpdateLoop.start();
			}

			NetworkManager::getInstance().update();
			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chronoUpdateLoop.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] NetworkManager::getInstance().update() took msecs: %lld, updateCount = %d\n",__FILE__,__FUNCTION__,__LINE__,chronoUpdateLoop.getMillis(),updateCount);
			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chronoUpdateLoop.getMillis() > 0) chronoUpdateLoop.start();
		}
		updateCount++;
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d AFTER programState->update took msecs: %lld ==============> MAIN LOOP BODY LOGIC, updateCount = %d\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis(),updateCount);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

	if(prevState == this->programState) {
		//fps timer
		chrono.start();
		while(fpsTimer.isTime()) {
			programState->tick();
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d programState->render took msecs: %lld ==============> MAIN LOOP TICKING\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

		//if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d programState->render took msecs: %lld ==============> MAIN LOOP TICKING\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] ------------------------------- MAIN LOOP END, stats: loop took msecs: %lld -------------------------------\n",__FILE__,__FUNCTION__,__LINE__,chronoLoop.getMillis());
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

void Program::setState(ProgramState *programStateNew, bool cleanupOldState)
{
	try {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		bool msgBoxEnabled = msgBox.getEnabled();

		bool showingOSCursor = isCursorShowing();
		if(dynamic_cast<Game *>(programStateNew) != NULL) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			int X = 0;
			int Y = 0;
			SDL_GetMouseState(&X,&Y);
			programStateNew->setStartXY(X,Y);
			Logger::getInstance().setProgress(0);
			Logger::getInstance().setState("");


			SDL_PumpEvents();

			showCursor(true);
			SDL_PumpEvents();
			sleep(0);
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		}

		if(cleanupOldState == true) {
			if(this->programState != programStateNew) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

				delete this->programState;

				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

				this->programState = NULL;

				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			}
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		//mesage box
		Lang &lang= Lang::getInstance();
		msgBox.init(lang.get("Ok"));
		msgBox.setEnabled(msgBoxEnabled);

		fpsTimer.init(1, maxTimes);
		updateTimer.init(GameConstants::updateFps, maxTimes);
		updateCameraTimer.init(GameConstants::cameraFps, maxTimes);

		this->programState= programStateNew;
		assert(programStateNew != NULL);
		programStateNew->load();

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		programStateNew->init();

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		updateTimer.reset();
		updateCameraTimer.reset();
		fpsTimer.reset();

		if(showingOSCursor == false) {
			Config &config = Config::getInstance();
			if(config.getBool("No2DMouseRendering","false") == false) {
				showCursor(false);
			}
			sleep(0);

			if(dynamic_cast<Intro *>(programStateNew) != NULL && msgBoxEnabled == true) {
				showCursor(true);
			}
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}
	catch(const exception &e){
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		this->showMessage(e.what());
		setState(new Intro(this));
	}
}

void Program::exit() {
	window->destroy();
}

// ==================== PRIVATE ====================

void Program::init(WindowGl *window, bool initSound, bool toggleFullScreen){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	this->window= window;
	Config &config= Config::getInstance();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    //set video mode
	if(toggleFullScreen == false) {
		setDisplaySettings();
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//window
	window->setText("MegaGlest");
	window->setStyle(config.getBool("Windowed")? wsWindowedFixed: wsFullscreen);
	window->setPos(0, 0);
	window->setSize(config.getInt("ScreenWidth"), config.getInt("ScreenHeight"));
	window->create();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//timers
	fpsTimer.init(1, maxTimes);
	updateTimer.init(GameConstants::updateFps, maxTimes);
	updateCameraTimer.init(GameConstants::cameraFps, maxTimes);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    //log start
	Logger &logger= Logger::getInstance();
	string logFile = "glest.log";
    if(getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) != "") {
        logFile = getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) + logFile;
    }
    else {
        string userData = config.getString("UserData_Root","");
        if(userData != "") {
        	endPathWithSlash(userData);
        }

        logFile = userData + logFile;
    }
	logger.setFile(logFile);
	logger.clear();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//lang
	//Lang &lang= Lang::getInstance();
	Lang::getInstance();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//render
	Renderer &renderer= Renderer::getInstance();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	window->initGl(config.getInt("ColorBits"), config.getInt("DepthBits"), config.getInt("StencilBits"),config.getBool("HardwareAcceleration","false"),config.getBool("FullScreenAntiAliasing","false"));

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	window->makeCurrentGl();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//coreData, needs renderer, but must load before renderer init
	CoreData &coreData= CoreData::getInstance();
    coreData.load();

    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//init renderer (load global textures)
	renderer.init();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//sound
	if(initSound == true && toggleFullScreen == false) {
        SoundRenderer &soundRenderer= SoundRenderer::getInstance();
        bool initOk = soundRenderer.init(window);

        if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] initOk = %d\n",__FILE__,__FUNCTION__,__LINE__,initOk);

        // Test sound system failed
        //initOk = false;
        // END

        if(initOk == false) {
        	string sError = "Sound System could not be initialzed!";
        	this->showMessage(sError.c_str());
        }

		// Run sound streaming in a background thread if enabled
        if(SoundRenderer::getInstance().runningThreaded() == true) {
			if(BaseThread::shutdownAndWait(soundThreadManager) == true) {
				delete soundThreadManager;
			}
			soundThreadManager = new SimpleTaskThread(&SoundRenderer::getInstance(),0,SOUND_THREAD_UPDATE_MILLISECONDS);
			soundThreadManager->setUniqueID(__FILE__);
			soundThreadManager->start();
		}
	}

	NetworkInterface::setAllowGameDataSynchCheck(Config::getInstance().getBool("AllowGameDataSynchCheck","false"));
	NetworkInterface::setAllowDownloadDataSynch(Config::getInstance().getBool("AllowDownloadDataSynch","false"));

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void Program::setDisplaySettings(){

	Config &config= Config::getInstance();

	if(!config.getBool("Windowed")) {

		int freq= config.getInt("RefreshFrequency");
		int colorBits= config.getInt("ColorBits");
		int screenWidth= config.getInt("ScreenWidth");
		int screenHeight= config.getInt("ScreenHeight");

        if(config.getBool("AutoMaxFullScreen","false") == true) {
            getFullscreenVideoInfo(colorBits,screenWidth,screenHeight,!config.getBool("Windowed"));
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

bool Program::isMessageShowing() {
    return msgBox.getEnabled();
}

void Program::showMessage(const char *msg) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d] msg [%s]\n",__FILE__,__FUNCTION__,__LINE__,msg);

	msgBox.setText(msg);
	msgBox.setEnabled(true);
}

void Program::stopSoundSystem() {
	if(soundThreadManager != NULL) {
		BaseThread::shutdownAndWait(soundThreadManager);
		delete soundThreadManager;
		soundThreadManager = NULL;
	}
}

void Program::startSoundSystem() {
	stopSoundSystem();
	if(SoundRenderer::getInstance().runningThreaded() == true) {
		soundThreadManager = new SimpleTaskThread(&SoundRenderer::getInstance(),0,SOUND_THREAD_UPDATE_MILLISECONDS);
		soundThreadManager->setUniqueID(__FILE__);
		soundThreadManager->start();
	}
}

void Program::resetSoundSystem() {
	startSoundSystem();
}

void Program::reInitGl() {
	if(window != NULL) {
		Config &config= Config::getInstance();
		window->initGl(config.getInt("ColorBits"), config.getInt("DepthBits"), config.getInt("StencilBits"),config.getBool("HardwareAcceleration","false"),config.getBool("FullScreenAntiAliasing","false"));
	}
}

void Program::consoleAddLine(string line) {
	if(programState != NULL) {
		programState->consoleAddLine(line);
	}
}

SimpleTaskThread * Program::getSoundThreadManager(bool takeOwnership) {
	SimpleTaskThread *result = soundThreadManager;
	if(takeOwnership) {
		soundThreadManager = NULL;
	}
	return result;
}

}}//end namespace
