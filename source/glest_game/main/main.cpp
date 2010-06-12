//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martio Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "math_wrapper.h"
#include "main.h"

#include <string>
#include <cstdlib>

#include "game.h"
#include "main_menu.h"
#include "program.h"
#include "config.h"
#include "metrics.h"
#include "game_util.h"
#include "platform_util.h"
#include "platform_main.h"
#include "network_interface.h"
#include "sound_renderer.h"
#include "ImageReaders.h"
#include "renderer.h"
#include "simple_threads.h"
#include <memory>
#include "font.h"
#include <curl/curl.h>
#include "menu_state_masterserver.h"

#include "leak_dumper.h"

using namespace std;
using namespace Shared::Platform;
using namespace Shared::Util;
using namespace Shared::Graphics;

namespace Glest{ namespace Game{

bool gameInitialized = false;

// =====================================================
// 	class ExceptionHandler
// =====================================================

class ExceptionHandler: public PlatformExceptionHandler{
public:
	virtual void handle(){
		string msg = "#1 An error ocurred and Glest will close.\nPlease report this bug to "+mailString+", attaching the generated "+getCrashDumpFileName()+" file.";
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s\n",msg.c_str());

        Program *program = Program::getInstance();
        if(program && gameInitialized == true) {
			//SystemFlags::Close();
            program->showMessage(msg.c_str());
        }

        message(msg.c_str());
	}

	static void handleRuntimeError(const char *msg) {
		Program *program = Program::getInstance();

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] [%s] gameInitialized = %d, program = %p\n",__FILE__,__FUNCTION__,__LINE__,msg,gameInitialized,program);

        if(program && gameInitialized == true) {
			//SystemFlags::Close();
            program->showMessage(msg);
        }
        else {
            message("#2 An error ocurred and Glest will close.\nError msg = [" + (msg != NULL ? string(msg) : string("?")) + "]\n\nPlease report this bug to "+mailString+", attaching the generated "+getCrashDumpFileName()+" file.");
        }
        showCursor(true);
        restoreVideoMode(true);
		//SystemFlags::Close();
        exit(0);
	}

	static int DisplayMessage(const char *msg, bool exitApp) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] %s\n",__FILE__,__FUNCTION__,__LINE__,msg);

        Program *program = Program::getInstance();
        if(program && gameInitialized == true) {
            program->showMessage(msg);
        }
        else {
            message(msg);
        }

        if(exitApp == true) {
        	showCursor(true);
            restoreVideoMode(true);
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s\n",msg);
			//SystemFlags::Close();
            exit(0);
        }

	    return 0;
	}
};

// =====================================================
// 	class MainWindow
// =====================================================

MainWindow::MainWindow(Program *program){
	this->program= program;
}

MainWindow::~MainWindow(){
	delete program;
}

void MainWindow::eventMouseDown(int x, int y, MouseButton mouseButton){

    const Metrics &metrics = Metrics::getInstance();
    int vx = metrics.toVirtualX(x);
    int vy = metrics.toVirtualY(getH() - y);

    ProgramState *programState = program->getState();

    switch(mouseButton) {
    case mbLeft:
        programState->mouseDownLeft(vx, vy);
        break;
    case mbRight:
        programState->mouseDownRight(vx, vy);
        break;
    case mbCenter:
        programState->mouseDownCenter(vx, vy);
        break;
    default:
        break;
    }
}

void MainWindow::eventMouseUp(int x, int y, MouseButton mouseButton){

    const Metrics &metrics = Metrics::getInstance();
    int vx = metrics.toVirtualX(x);
    int vy = metrics.toVirtualY(getH() - y);

    ProgramState *programState = program->getState();

    switch(mouseButton) {
    case mbLeft:
        programState->mouseUpLeft(vx, vy);
        break;
    case mbRight:
        programState->mouseUpRight(vx, vy);
        break;
    case mbCenter:
        programState->mouseUpCenter(vx, vy);
        break;
    default:
        break;
    }
}

void MainWindow::eventMouseDoubleClick(int x, int y, MouseButton mouseButton){

    const Metrics &metrics= Metrics::getInstance();
    int vx = metrics.toVirtualX(x);
    int vy = metrics.toVirtualY(getH() - y);

    ProgramState *programState = program->getState();

    switch(mouseButton){
    case mbLeft:
        programState->mouseDoubleClickLeft(vx, vy);
        break;
    case mbRight:
        programState->mouseDoubleClickRight(vx, vy);
        break;
    case mbCenter:
        programState->mouseDoubleClickCenter(vx, vy);
        break;
    default:
        break;
    }
}

void MainWindow::eventMouseMove(int x, int y, const MouseState *ms){

    const Metrics &metrics= Metrics::getInstance();
    int vx = metrics.toVirtualX(x);
    int vy = metrics.toVirtualY(getH() - y);

    ProgramState *programState = program->getState();
    programState->mouseMove(vx, vy, ms);
}

void MainWindow::eventMouseWheel(int x, int y, int zDelta) {

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	const Metrics &metrics= Metrics::getInstance();
	int vx = metrics.toVirtualX(x);
	int vy = metrics.toVirtualY(getH() - y);

    ProgramState *programState = program->getState();
	programState->eventMouseWheel(vx, vy, zDelta);

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void MainWindow::eventKeyDown(char key){
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] key = [%c][%d]\n",__FILE__,__FUNCTION__,__LINE__,key,key);
	program->keyDown(key);

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(key == vkReturn) {
		SDL_keysym keystate = Window::getKeystate();
		if(keystate.mod & (KMOD_LALT | KMOD_RALT)) {
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] ALT-ENTER pressed\n",__FILE__,__FUNCTION__,__LINE__);

			// This stupidity only required in win32.
			// We reload the textures so that 
#ifdef WIN32
			//Renderer &renderer= Renderer::getInstance();
			//renderer.reinitAll();
#endif

			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		}
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void MainWindow::eventKeyUp(char key){
	program->keyUp(key);
}

void MainWindow::eventKeyPress(char c){
	program->keyPress(c);
}

void MainWindow::eventActivate(bool active){
	if(!active){
		//minimize();
	}
}

void MainWindow::eventResize(SizeState sizeState){
	program->resize(sizeState);
}

void MainWindow::eventClose(){
	delete program;
	program= NULL;
}

void MainWindow::setProgram(Program *program) {
	this->program= program;
}

// =====================================================
// Main
// =====================================================
SystemFlags debugger;

int glestMain(int argc, char** argv){

#ifdef SL_LEAK_DUMP
	AllocRegistry memoryLeaks = AllocRegistry::getInstance();
#endif

#ifdef USE_STREFLOP

	streflop_init<streflop::Simple>();
	printf("%s, STREFLOP enabled.\n",getNetworkVersionString().c_str());
#else
	printf("%s, STREFLOP NOT enabled.\n",getNetworkVersionString().c_str());
#endif

	SystemFlags::init();
	SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled  = true;
	SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled = true;

	MainWindow *mainWindow= NULL;
	Program *program= NULL;
	ExceptionHandler exceptionHandler;
	exceptionHandler.install( getCrashDumpFileName() );

	try{
		std::auto_ptr<FileCRCPreCacheThread> preCacheThread;
		Config &config = Config::getInstance();

		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled      = config.getBool("DebugMode","false");
		SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled     = config.getBool("DebugNetwork","false");
		SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled = config.getBool("DebugPerformance","false");
		SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled  = config.getBool("DebugWorldSynch","false");
		SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled  = config.getBool("DebugUnitCommands","false");

		string debugLogFile 			= config.getString("DebugLogFile","");
        if(getGameReadWritePath() != "") {
            debugLogFile = getGameReadWritePath() + debugLogFile;
        }
		string debugWorldSynchLogFile 	= config.getString("DebugLogFileWorldSynch","");
        if(debugWorldSynchLogFile == "") {
        	debugWorldSynchLogFile = debugLogFile;
        }
		string debugPerformanceLogFile = config.getString("DebugLogFilePerformance","");
        if(debugPerformanceLogFile == "") {
        	debugPerformanceLogFile = debugLogFile;
        }
		string debugNetworkLogFile = config.getString("DebugLogFileNetwork","");
        if(debugNetworkLogFile == "") {
        	debugNetworkLogFile = debugLogFile;
        }
		string debugUnitCommandsLogFile = config.getString("DebugLogFileUnitCommands","");
        if(debugUnitCommandsLogFile == "") {
        	debugUnitCommandsLogFile = debugLogFile;
        }

        SystemFlags::getSystemSettingType(SystemFlags::debugSystem).debugLogFileName      = debugLogFile;
        SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).debugLogFileName     = debugNetworkLogFile;
        SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).debugLogFileName = debugPerformanceLogFile;
        SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).debugLogFileName  = debugWorldSynchLogFile;
        SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).debugLogFileName  = debugUnitCommandsLogFile;

		printf("Startup settings are: debugSystem [%d], debugNetwork [%d], debugPerformance [%d], debugWorldSynch [%d], debugUnitCommands[%d]\n",
			SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled,
			SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled,
			SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled,
			SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled,
			SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled);

		NetworkInterface::setDisplayMessageFunction(ExceptionHandler::DisplayMessage);
		MenuStateMasterserver::setDisplayMessageFunction(ExceptionHandler::DisplayMessage);
		
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"START\n");

		// 256 for English
		// 30000 for Chinese
		Font::charCount    = config.getInt("FONT_CHARCOUNT",intToStr(Font::charCount).c_str());
		Font::fontTypeName = config.getString("FONT_TYPENAME",Font::fontTypeName.c_str());
		// Example values:
		// DEFAULT_CHARSET (English) = 1
		// GB2312_CHARSET (Chinese)  = 134
		Shared::Platform::charSet = config.getInt("FONT_CHARSET",intToStr(Shared::Platform::charSet).c_str());

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] Font::charCount = %d, Font::fontTypeName [%s] Shared::Platform::charSet = %d\n",__FILE__,__FUNCTION__,__LINE__,Font::charCount,Font::fontTypeName.c_str(),Shared::Platform::charSet);

		Config &configKeys = Config::getInstance(
				std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys),
				std::pair<string,string>("glestkeys.ini","glestuserkeys.ini"),
				std::pair<bool,bool>(true,false));

		//showCursor(config.getBool("Windowed"));
		showCursor(false);

		if(config.getBool("noTeamColors","false") == true) {
			MeshCallbackTeamColor::noTeamColors = true;
		}

		// Over-ride default network command framecount
		GameConstants::networkFramePeriod = config.getInt("NetworkFramePeriod",intToStr(GameConstants::networkFramePeriod).c_str());

		//float pingTime = Socket::getAveragePingMS("soft-haus.com");
		//printf("Ping time = %f\n",pingTime);

		program= new Program();

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		mainWindow= new MainWindow(program);

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		//parse command line
		if(argc==2 && string(argv[1])=="-server"){
			program->initServer(mainWindow);
		}
		else if(argc==3 && string(argv[1])=="-client"){
			program->initClient(mainWindow, Ip(argv[2]));
		}
		else{
			program->initNormal(mainWindow);
		}

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		gameInitialized = true;

		if(config.getBool("AllowGameDataSynchCheck","false") == true) {
			vector<string> techDataPaths = config.getPathListForType(ptTechs);
			preCacheThread.reset(new FileCRCPreCacheThread());
			preCacheThread->setTechDataPaths(techDataPaths);
			preCacheThread->start();
		}

        // test
        //Shared::Platform::MessageBox(NULL,"Mark's test.","Test",0);
        //throw runtime_error("test!");
        //ExceptionHandler::DisplayMessage("test!", false);

		//main loop
		while(Window::handleEvent()){
			program->loop();
		}

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}
	catch(const exception &e){
		ExceptionHandler::handleRuntimeError(e.what());
	}
	catch(const char *e){
		ExceptionHandler::handleRuntimeError(e);
	}
	catch(const string &ex){
		ExceptionHandler::handleRuntimeError(ex.c_str());
	}
	catch(...){
		ExceptionHandler::handleRuntimeError("Unknown error!");
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//SoundRenderer &soundRenderer= SoundRenderer::getInstance();
	//soundRenderer.stopAllSounds();

	delete mainWindow;

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//SystemFlags::Close();

	return 0;
}

}}//end namespace

MAIN_FUNCTION(Glest::Game::glestMain)
