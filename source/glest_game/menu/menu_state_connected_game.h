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

#ifndef _GLEST_GAME_MENUSTATECONNECTEDGAME_H_
#define _GLEST_GAME_MENUSTATECONNECTEDGAME_H_

#include "main_menu.h"
#include "chat_manager.h"

namespace Glest{ namespace Game{

enum JoinMenu{
	jmSimple,
	jmMasterserver,

	jmCount
};


// ===============================
// 	class MenuStateConnectedGame
// ===============================

class MenuStateConnectedGame: public MenuState{
private:
	GraphicButton buttonDisconnect;
	GraphicButton buttonPlayNow;
	GraphicLabel labelControl;
	GraphicLabel labelFaction;
	GraphicLabel labelTeam;
	GraphicLabel labelMap;
	GraphicLabel labelFogOfWar;
	GraphicLabel labelTechTree;
	GraphicLabel labelTileset;
	GraphicLabel labelMapInfo;
	GraphicLabel labelStatus;
	GraphicLabel labelInfo;


	GraphicLabel labelEnableObserverMode;
	GraphicListBox listBoxEnableObserverMode;
	GraphicLabel labelEnableServerControlledAI;
	GraphicListBox listBoxEnableServerControlledAI;
	GraphicLabel labelNetworkPauseGameForLaggedClients;
	GraphicListBox listBoxNetworkPauseGameForLaggedClients;

	GraphicListBox listBoxNetworkFramePeriod;
	GraphicLabel labelNetworkFramePeriod;

	GraphicLabel labelPathFinderType;
	GraphicListBox listBoxPathFinderType;
	
	GraphicLabel labelMapPlayerCount;
	GraphicListBox listBoxMapPlayerCount;
	
	GraphicLabel labelAdvanced;
	GraphicListBox listBoxAdvanced;
	


	GraphicListBox listBoxMap;
	GraphicListBox listBoxFogOfWar;
	GraphicListBox listBoxTechTree;
	GraphicListBox listBoxTileset;
	vector<string> mapFiles;
	vector<string> techTreeFiles;
	vector<string> tilesetFiles;
	vector<string> factionFiles;
	GraphicLabel labelPlayers[GameConstants::maxPlayers];
	GraphicLabel labelPlayerNames[GameConstants::maxPlayers];
	GraphicListBox listBoxControls[GameConstants::maxPlayers];
	GraphicListBox listBoxFactions[GameConstants::maxPlayers];
	GraphicListBox listBoxTeams[GameConstants::maxPlayers];
	GraphicLabel labelNetStatus[GameConstants::maxPlayers];
	GraphicButton grabSlotButton[GameConstants::maxPlayers];

	GraphicLabel *activeInputLabel;

	MapInfo mapInfo;

	bool needToSetChangedGameSettings;
	time_t lastSetChangedGameSettings;
	bool updateDataSynchDetailText;

	Console console;
	ChatManager chatManager;
	bool showFullConsole;
	
	string currentFactionName;
	string currentMap;
	JoinMenu returnMenuInfo;
	bool settingsReceivedFromServer;
	time_t lastNetworkSendPing;
	int pingCount;
	bool initialSettingsReceivedFromServer;
	
	string lastMapDataSynchError;
	string lastTileDataSynchError;
	string lastTechtreeDataSynchError;
	
	int8 switchSetupRequestFlagType;
	string defaultPlayerName;

	string currentFactionLogo;
	Texture2D *factionTexture;

public:
	MenuStateConnectedGame(Program *program, MainMenu *mainMenu, JoinMenu joinMenuInfo=jmSimple, bool openNetworkSlots= false);
	~MenuStateConnectedGame();

	void mouseClick(int x, int y, MouseButton mouseButton);
	void mouseMove(int x, int y, const MouseState *mouseState);
	void render();
	void update();

    virtual void keyDown(char key);
    virtual void keyPress(char c);
    virtual void keyUp(char key);

    virtual bool isInSpecialKeyCaptureEvent() { return chatManager.getEditEnabled(); }

private:

    bool hasNetworkGameSettings();
	void reloadFactions();
	bool loadFactions(const GameSettings *gameSettings,bool errorOnNoFactions);
	void returnToJoinMenu();
	string getHumanPlayerName();
	void setActiveInputLabel(GraphicLabel *newLable);

	void cleanupFactionTexture();
	void loadFactionTexture(string filepath);

};

}}//end namespace

#endif
