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

#ifndef _GLEST_GAME_MAINMENU_H_
#define _GLEST_GAME_MAINMENU_H_

#ifdef WIN32
    #include <winsock2.h>
    #include <winsock.h>
#endif

#include "lang.h"
#include "console.h"
#include "vec.h" 
#include "world.h"
#include "program.h"
#include "components.h"
#include "menu_background.h"
#include "game_settings.h"
#include "leak_dumper.h"

namespace Shared { namespace Graphics {
	class VideoPlayer;
}}

namespace Glest{ namespace Game{

class MenuState;

// =====================================================
// 	class MainMenu
//
///	Main menu ProgramState
// =====================================================

class MainMenu: public ProgramState {
	
private:
	static MenuState *oldstate;
	//up
	Program *program;
	
	//shared
	GameSettings gameSettings;
	MenuBackground menuBackground;
	Shared::Graphics::VideoPlayer *menuBackgroundVideo;

	MenuState *state;

	//shared
    int mouseX, mouseY;
    int mouse2dAnim;

    void initBackgroundVideo();

public:
	MainMenu(Program *program);
    ~MainMenu();

	MenuBackground *getMenuBackground()	{return &menuBackground;}
	const MenuBackground *getConstMenuBackground() const	{return &menuBackground;}

    virtual void render();
    virtual void update();
	virtual void init();
    virtual void mouseMove(int x, int y, const MouseState *mouseState);
    virtual void mouseDownLeft(int x, int y);
    virtual void mouseDownRight(int x, int y);
	virtual void keyDown(SDL_KeyboardEvent key);
	virtual void keyUp(SDL_KeyboardEvent key);
	virtual void keyPress(SDL_KeyboardEvent key);
	
	void setState(MenuState *state);
	virtual bool isInSpecialKeyCaptureEvent();

    int getMouseX() const {return mouseX;}
    int getMouseY() const {return mouseY;}
    int getMouse2dAnim() const {return mouse2dAnim;}
    virtual void consoleAddLine(string line);
    virtual void reloadUI();
};


// ===============================
// 	class MenuState  
// ===============================

class MenuState {
protected:
	Program *program;

	MainMenu *mainMenu;
	Camera camera;

	const char *containerName;
	Console console;

public:
	MenuState(Program *program, MainMenu *mainMenu, const string &stateName);
	virtual ~MenuState();
	virtual void mouseClick(int x, int y, MouseButton mouseButton)=0;
	virtual void mouseMove(int x, int y, const MouseState *mouseState)=0;
	virtual void render()=0;
	virtual void update(){};
	virtual void keyDown(SDL_KeyboardEvent key){};
	virtual void keyPress(SDL_KeyboardEvent c){};
	virtual void keyUp(SDL_KeyboardEvent key){};

	virtual bool isMasterserverMode() const {return false;}
	const Camera *getCamera() const			{return &camera;}

	virtual bool isInSpecialKeyCaptureEvent() { return false; }
	virtual void consoleAddLine(string line);
	virtual void reloadUI();

	virtual bool isVideoPlaying() { return false; };
};

}}//end namespace

#endif
