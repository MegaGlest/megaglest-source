// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_MENUSTATEROOT_H_
#define _GLEST_GAME_MENUSTATEROOT_H_

#include "main_menu.h"

namespace Glest{ namespace Game{

// ===============================
// 	class MenuStateRoot  
// ===============================

class MenuStateRoot: public MenuState{
private:
	GraphicButton buttonNewGame;
	GraphicButton buttonJoinGame;
	GraphicButton buttonMasterserverGame;
	GraphicButton buttonOptions;
	GraphicButton buttonAbout;
	GraphicButton buttonExit;
	GraphicLabel labelVersion;

public:
	MenuStateRoot(Program *program, MainMenu *mainMenu);

	void mouseClick(int x, int y, MouseButton mouseButton);
	void mouseMove(int x, int y, const MouseState *mouseState);
	void render();
	void update();
};


}}//end namespace

#endif
