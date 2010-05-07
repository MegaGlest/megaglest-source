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

#ifndef _GLEST_GAME_PROGRAM_H_
#define _GLEST_GAME_PROGRAM_H_

#include "context.h"
#include "platform_util.h"
#include "window_gl.h"
#include "socket.h"
#include "components.h"
#include "window.h"
#include "simple_threads.h"

using Shared::Platform::MouseButton;
using Shared::Graphics::Context;
using Shared::Platform::WindowGl;
using Shared::Platform::SizeState;
using Shared::Platform::MouseState;
using Shared::PlatformCommon::PerformanceTimer;
using Shared::Platform::Ip;
using namespace Shared::PlatformCommon;

namespace Glest{ namespace Game{

class Program;
class MainWindow;

// =====================================================
// 	class ProgramState
//
///	Base class for all program states:
/// Intro, MainMenu, Game, BattleEnd (State Design pattern)
// =====================================================

class ProgramState{
protected:
	Program *program;

public:
	ProgramState(Program *program)	{this->program= program;}
	virtual ~ProgramState(){};

	virtual void render()=0;
	virtual void update(){};
	virtual void updateCamera(){};
	virtual void tick(){};
	virtual void init(){};
	virtual void load(){};
	virtual void end(){};
	virtual void mouseDownLeft(int x, int y){};
	virtual void mouseUpLeft(int x, int y){};
	virtual void mouseUpRight(int x, int y){}
	virtual void mouseUpCenter(int x, int y){}
	virtual void mouseDownRight(int x, int y){};
	virtual void mouseDownCenter(int x, int y){}
	virtual void mouseDoubleClickLeft(int x, int y){};
	virtual void mouseDoubleClickRight(int x, int y){}
	virtual void mouseDoubleClickCenter(int x, int y){}
	virtual void eventMouseWheel(int x, int y, int zDelta){}
	virtual void mouseMove(int x, int y, const MouseState *mouseState) {};
	virtual void keyDown(char key){};
	virtual void keyUp(char key){};
	virtual void keyPress(char c){};
};

// ===============================
// 	class Program
// ===============================

class Program : public SimpleTaskCallbackInterface {
private:
	static const int maxTimes;
	SimpleTaskThread *soundThreadManager;

	class ShowMessageProgramState : public ProgramState {
		GraphicMessageBox msgBox;
		int mouseX;
		int mouseY;
		int mouse2dAnim;
		string msg;
		bool userWantsExit;

	public:
		ShowMessageProgramState(Program *program, const char *msg);

		virtual void render();
		virtual void mouseDownLeft(int x, int y);
		virtual void mouseMove(int x, int y, const MouseState &mouseState);
		virtual void keyPress(char c);
		virtual void update();
		virtual bool wantExit() { return userWantsExit; }
	};


private:
    ProgramState *programState;

	PerformanceTimer fpsTimer;
	PerformanceTimer updateTimer;
	PerformanceTimer updateCameraTimer;

    WindowGl *window;
    static Program *singleton;
    SimpleTaskThread *loopThreadManager;

public:
    Program();
    ~Program();

    static Program *getInstance()	{return singleton;}

	void initNormal(WindowGl *window);
	void initServer(WindowGl *window);
	void initClient(WindowGl *window, const Ip &serverIp);

	//main
    void keyDown(char key);
    void keyUp(char key);
    void keyPress(char c);

	void loop();
	void loopWorker();
	void resize(SizeState sizeState);
	void showMessage(const char *msg);

	//misc
	void setState(ProgramState *programState,bool cleanupOldState=true);
	ProgramState * getState() { return programState;}
	WindowGl * getWindow() { return window; }
	void init(WindowGl *window, bool initSound=true, bool toggleFullScreen=false);
	void exit();

	virtual void simpleTask();

private:
	
	void setDisplaySettings();
	void restoreDisplaySettings();
};

}} //end namespace

#endif
