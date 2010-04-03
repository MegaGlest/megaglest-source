// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Martio Figueroa
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
#include "chat_manager.h"
#include <vector>
#include <string>

using Shared::Util::Properties;

namespace Glest{ namespace Game{

class NetworkMessageIntro;

// ===============================
// 	class MenuStateJoinGame
// ===============================

class MenuStateJoinGame: public MenuState, public DiscoveredServersInterface {
private:
	static const int newServerIndex;
	static const string serverFileName;

private:
	GraphicButton buttonReturn;
	GraphicButton buttonConnect;
	GraphicButton buttonAutoFindServers;
	GraphicLabel labelServer;
	GraphicLabel labelServerType;
	GraphicLabel labelServerIp;
	GraphicLabel labelStatus;
	GraphicLabel labelInfo;
	GraphicListBox listBoxServerType;
	GraphicListBox listBoxServers;
	GraphicLabel labelServerPort;
	GraphicLabel labelServerPortLabel;


	bool connected;
	int playerIndex;
	Properties servers;

	Console console;
	ChatManager chatManager;

	string serversSavedFile;

public:
	MenuStateJoinGame(Program *program, MainMenu *mainMenu, bool connect= false, Ip serverIp= Ip());
	virtual ~MenuStateJoinGame();

	void mouseClick(int x, int y, MouseButton mouseButton);
	void mouseMove(int x, int y, const MouseState *mouseState);
	void render();
	void update();
    virtual void keyDown(char key);
    virtual void keyPress(char c);

private:
	void connectToServer();
	virtual void DiscoveredServers(std::vector<string> serverList);
};
}}//end namespace

#endif
