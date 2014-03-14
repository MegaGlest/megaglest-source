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

#ifndef _GLEST_GAME_MENUSTATENEWGAME_H_
#define _GLEST_GAME_MENUSTATENEWGAME_H_

#include "main_menu.h"
#include "megaglest_cegui_manager.h"
#include "leak_dumper.h"

namespace Glest{ namespace Game{

// ===============================
// 	class MenuStateNewGame  
// ===============================

class MenuStateNewGame: public MenuState, public MegaGlest_CEGUIManagerBackInterface {
private:

	typedef void(MenuStateNewGame::*DelayCallbackFunction)(void);
	vector<DelayCallbackFunction> delayedCallbackList;

protected:
	virtual bool hasDelayedCallbacks() { return delayedCallbackList.empty() == false; }
	virtual void callDelayedCallbacks();

public:
	MenuStateNewGame(Program *program, MainMenu *mainMenu);

	void mouseClick(int x, int y, MouseButton mouseButton);
	void mouseMove(int x, int y, const MouseState *mouseState);
	void update();
	void render();
	virtual void keyDown(SDL_KeyboardEvent key);

	void reloadUI();

private:

	void delayedCallbackFunctionTutorial();
	void delayedCallbackFunctionScenario();
	void delayedCallbackFunctionCustomGame();
	void delayedCallbackFunctionInternetGame();
	void delayedCallbackFunctionLANGame();
	void delayedCallbackFunctionReturn();

	void setupCEGUIWidgetsText();
	void setupCEGUIWidgets();
	virtual bool EventCallback(CEGUI::Window *ctl, std::string name);

};


}}//end namespace

#endif
