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

#ifndef _GLEST_GAME_MENUSTATEABOUT_H_
#define _GLEST_GAME_MENUSTATEABOUT_H_

#ifdef WIN32
    #include <winsock2.h>
    #include <winsock.h>
#endif

#include "main_menu.h"
#include "leak_dumper.h"

namespace Glest{ namespace Game{

// ===============================
// 	class MenuStateAbout  
// ===============================

class MenuStateAbout: public MenuState{
public:
	static const int aboutStringCount1= 3;
	static const int aboutStringCount2= 3;
	static const int teammateCount= 9;
	static const int teammateTopLineCount= 5;

private:
	GraphicButton buttonReturn;
	GraphicLabel labelAdditionalCredits;
	GraphicLabel labelAbout1[aboutStringCount1];
	GraphicLabel labelAbout2[aboutStringCount2];
	GraphicLabel labelTeammateName[teammateCount];
	GraphicLabel labelTeammateRole[teammateCount];

	bool adjustModelText;
	string loadAdditionalCredits();

public:
	MenuStateAbout(Program *program, MainMenu *mainMenu);

	void mouseClick(int x, int y, MouseButton mouseButton);
	void mouseMove(int x, int y, const MouseState *mouseState);
	void render();
	virtual void keyDown(SDL_KeyboardEvent key);

	virtual void reloadUI();
};

}}//end namespace

#endif
