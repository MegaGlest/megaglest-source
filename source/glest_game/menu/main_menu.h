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

#include "lang.h"
#include "console.h"
#include "vec.h" 
#include "world.h"
#include "program.h"
#include "components.h"
#include "menu_background.h"
#include "game_settings.h"

namespace Glest{ namespace Game{

//misc consts
struct MapInfo{
	Vec2i size;
	int players;
	string desc;
};

struct ScenarioInfo
{
	int difficulty;
    ControlType factionControls[GameConstants::maxPlayers];
    int teams[GameConstants::maxPlayers];
    string factionTypeNames[GameConstants::maxPlayers];

    string mapName;
    string tilesetName;
    string techTreeName;

	bool defaultUnits;
	bool defaultResources;
	bool defaultVictoryConditions;

    string desc;
};

class MenuState;

// =====================================================
// 	class MainMenu
//
///	Main menu ProgramState
// =====================================================

class MainMenu: public ProgramState{
private:
	//up
	Program *program;
	
	//shared
	GameSettings gameSettings;
	MenuBackground menuBackground;

	MenuState *state;

	//shared
    int mouseX, mouseY;
    int mouse2dAnim;
	int fps, lastFps;

public:
	MainMenu(Program *program);
    ~MainMenu();

	MenuBackground *getMenuBackground()	{return &menuBackground;}

    virtual void render();
    virtual void update();
	virtual void tick();
	virtual void init();
    virtual void mouseMove(int x, int y, const MouseState *mouseState);
    virtual void mouseDownLeft(int x, int y);
    virtual void mouseDownRight(int x, int y);
	virtual void keyDown(char key);
	virtual void keyPress(char key);
    
	void setState(MenuState *state);
};


// ===============================
// 	class MenuState  
// ===============================

class MenuState{
protected:
	Program *program;

	MainMenu *mainMenu;
	Camera camera;

public:
	MenuState(Program *program, MainMenu *mainMenu, const string &stateName);
	virtual ~MenuState(){};
	virtual void mouseClick(int x, int y, MouseButton mouseButton)=0;
	virtual void mouseMove(int x, int y, const MouseState *mouseState)=0;
	virtual void render()=0;
	virtual void update(){};
	virtual void keyDown(char key){};
	virtual void keyPress(char c){};

	const Camera *getCamera() const			{return &camera;}
};

}}//end namespace

#endif
