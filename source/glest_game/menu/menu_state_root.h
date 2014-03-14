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

#ifndef _GLEST_GAME_MENUSTATEROOT_H_
#define _GLEST_GAME_MENUSTATEROOT_H_

#include "main_menu.h"
#include "megaglest_cegui_manager.h"

#include "leak_dumper.h"

namespace Glest{ namespace Game{

// ===============================
// 	class MenuStateRoot  
// ===============================

class MenuStateRoot: public MenuState, public MegaGlest_CEGUIManagerBackInterface {

public:

	MenuStateRoot(Program *program, MainMenu *mainMenu);

	virtual void mouseClick(int x, int y, MouseButton mouseButton) {};
	virtual void mouseMove(int x, int y, const MouseState *mouseState) {};

	void render();
	virtual void update();

	virtual void keyDown(SDL_KeyboardEvent key);
	void showMessageBox(const string &text, const string &header);
	//void showErrorMessageBox(const string &text, const string &header);

	virtual bool isMasterserverMode() const;
	virtual void reloadUI();

private:

	void setupCEGUIWidgetsText();
	void setupCEGUIWidgets();
	virtual bool EventCallback(CEGUI::Window *ctl, std::string name);
};


}}//end namespace

#endif
