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

#ifndef _GLEST_GAME_MENUSTATEMASTERSERVER_H_
#define _GLEST_GAME_MENUSTATEMASTERSERVER_H_

#include "main_menu.h"
#include "masterserver_info.h"
#include "simple_threads.h"
#include "network_interface.h"

namespace Glest{ namespace Game{


// ===============================
// 	ServerLine
// ===============================

class ServerLine {
private:

	MasterServerInfo *masterServerInfo;
	int index;
	GraphicButton selectButton;

	//general info:
	GraphicLabel glestVersionLabel;
	GraphicLabel platformLabel;
	GraphicLabel binaryCompileDateLabel;
	
	//game info:
	GraphicLabel serverTitleLabel;
	GraphicLabel ipAddressLabel;
	
	//game setup info:
	GraphicLabel techLabel;
	GraphicLabel mapLabel; 
	GraphicLabel tilesetLabel;
	GraphicLabel activeSlotsLabel;

public:
	ServerLine( MasterServerInfo *mServerInfo, int lineIndex);
	virtual ~ServerLine();
	MasterServerInfo *getMasterServerInfo() const	{return masterServerInfo;}
	const int getIndex() const	{return index;}
	bool buttonMouseClick(int x, int y);
	bool buttonMouseMove(int x, int y);
	//void setIndex(int value);
	void render();
};



// ===============================
// 	class MenuStateMasterserver  
// ===============================
typedef vector<ServerLine*> ServerLines;
typedef vector<MasterServerInfo*> MasterServerInfos;

class MenuStateMasterserver : public MenuState, public SimpleTaskCallbackInterface {
private:
	GraphicButton buttonRefresh;
	GraphicButton buttonReturn;
	GraphicButton buttonCreateGame;
	GraphicLabel labelAutoRefresh;
	GraphicListBox listBoxAutoRefresh;
	GraphicLabel labelTitle;
	ServerLines serverLines;
	
	GraphicMessageBox mainMessageBox;
	int mainMessageBoxState;
	
	bool needUpdateFromServer;
	int autoRefreshTime;
	time_t lastRefreshTimer;
	SimpleTaskThread *updateFromMasterserverThread;
	bool playServerFoundSound;

	static DisplayMessageFunction pCB_DisplayMessage;
	std::string threadedErrorMsg;

public:
	MenuStateMasterserver(Program *program, MainMenu *mainMenu);
	virtual ~MenuStateMasterserver();
	
	void mouseClick(int x, int y, MouseButton mouseButton);
	void mouseMove(int x, int y, const MouseState *mouseState);
	void update();
	void render();

	virtual void simpleTask();

	static void setDisplayMessageFunction(DisplayMessageFunction pDisplayMessage) { pCB_DisplayMessage = pDisplayMessage; }

private:
	void showMessageBox(const string &text, const string &header, bool toggle);
	void connectToServer(string ipString);
	void clearServerLines();
	void updateServerInfo();
	
};


}}//end namespace

#endif
