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
#include "leak_dumper.h"
#include "network_interface.h"

using namespace std;
using namespace Shared::Platform;
using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class ExceptionHandler
// =====================================================

class ExceptionHandler: public PlatformExceptionHandler{
public:
	virtual void handle(){

        string msg = "An error ocurred and Glest will close.\nPlease report this bug to "+mailString+", attaching the generated "+getCrashDumpFileName()+" file.";
        Program *program = Program::getInstance();
        if(program) {
            program->showMessage(msg.c_str());
        }

        message(msg.c_str());
	}

	static void handleRuntimeError(const char *msg) {
        Program *program = Program::getInstance();
        if(program) {
            program->showMessage(msg);
        }
        else {
            message("An error ocurred and Glest will close.\nError msg = [" + (msg != NULL ? string(msg) : string("?")) + "]\n\nPlease report this bug to "+mailString+", attaching the generated "+getCrashDumpFileName()+" file.");
        }

        exit(0);
	}

	static int DisplayMessage(const char *msg, bool exitApp) {

        Program *program = Program::getInstance();
        if(program) {
            program->showMessage(msg);
        }
        else {
            message(msg);
        }

        if(exitApp == true) {
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
	switch(mouseButton){
	case mbLeft:
		program->mouseDownLeft(x, getH() - y);
		break;
	case mbRight:
		program->mouseDownRight(x, getH() - y);
		break;
	default:
		break;
	}
}

void MainWindow::eventMouseUp(int x, int y, MouseButton mouseButton){
	if(mouseButton==mbLeft){
		program->mouseUpLeft(x, getH() - y);
	}
}

void MainWindow::eventMouseDoubleClick(int x, int y, MouseButton mouseButton){
	if(mouseButton == mbLeft){
		program->mouseDoubleClickLeft(x,  getH() - y);
	}
}

void MainWindow::eventMouseMove(int x, int y, const MouseState *ms){
	program->mouseMove(x, getH() - y, ms);
}

void MainWindow::eventKeyDown(char key){
	program->keyDown(key);
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

// =====================================================
// Main
// =====================================================

int glestMain(int argc, char** argv){

	SystemFlags::enableNetworkDebugInfo = true;
    SystemFlags::enableDebugText = true;

	MainWindow *mainWindow= NULL;
	Program *program= NULL;
	ExceptionHandler exceptionHandler;
	exceptionHandler.install( getCrashDumpFileName() );

	try{
		Config &config = Config::getInstance();

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		SystemFlags::enableNetworkDebugInfo = config.getBool("DebugNetwork","0");
		SystemFlags::enableDebugText = config.getBool("DebugMode","0");

		NetworkInterface::setDisplayMessageFunction(ExceptionHandler::DisplayMessage);

		showCursor(config.getBool("Windowed"));

		program= new Program();
		mainWindow= new MainWindow(program);

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

        // test
        //Shared::Platform::MessageBox(NULL,"Mark's test.","Test",0);
        //throw runtime_error("test!");
        //ExceptionHandler::DisplayMessage("test!", false);

		//main loop
		while(Window::handleEvent()){
			program->loop();
		}
	}
	catch(const exception &e){
		restoreVideoMode();
		//exceptionMessage(e);
		ExceptionHandler::handleRuntimeError(e.what());
	}

	delete mainWindow;

	return 0;
}

}}//end namespace

MAIN_FUNCTION(Glest::Game::glestMain)
