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

#ifndef _GLEST_GAME_MENUSTATEJOINGAME_H_
#define _GLEST_GAME_MENUSTATEJOINGAME_H_

#include "properties.h"
#include "main_menu.h"

using Shared::Util::Properties;

namespace Glest{ namespace Game{

class NetworkMessageIntro;

// ===============================
// 	class MenuStateJoinGame  
// ===============================

class MenuStateJoinGame: public MenuState{
private:
	static const int newServerIndex;
	static const string serverFileName;

private:
	GraphicButton buttonReturn;
	GraphicButton buttonConnect;
	GraphicLabel labelServer;
	GraphicLabel labelServerType;
	GraphicLabel labelServerIp;
	GraphicLabel labelStatus;
	GraphicLabel labelInfo;
	GraphicListBox listBoxServerType;
	GraphicListBox listBoxServers;

	bool connected;
	int playerIndex;
	Properties servers;

public:
	MenuStateJoinGame(Program *program, MainMenu *mainMenu, bool connect= false, Ip serverIp= Ip());

	void mouseClick(int x, int y, MouseButton mouseButton);
	void mouseMove(int x, int y, const MouseState *mouseState);
	void render();
	void update();
	void keyDown(char key);
	void keyPress(char c);

private:
	void connectToServer();
};
}}//end namespace

#endif
