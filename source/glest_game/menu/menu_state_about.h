// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_MENUSTATEABOUT_H_
#define _GLEST_GAME_MENUSTATEABOUT_H_

#include "main_menu.h"

namespace Glest{ namespace Game{

// ===============================
// 	class MenuStateAbout  
// ===============================

class MenuStateAbout: public MenuState{
public:
	static const int aboutStringCount1= 3;
	static const int aboutStringCount2= 3;
	static const int teammateCount= 7;

private:
	GraphicButton buttonReturn;
	GraphicLabel labelAbout1[aboutStringCount1];
	GraphicLabel labelAbout2[aboutStringCount2];
	GraphicLabel labelTeammateName[teammateCount];
	GraphicLabel labelTeammateRole[teammateCount];

public:
	MenuStateAbout(Program *program, MainMenu *mainMenu);

	void mouseClick(int x, int y, MouseButton mouseButton);
	void mouseMove(int x, int y, const MouseState *mouseState);
	void render();
};

}}//end namespace

#endif
