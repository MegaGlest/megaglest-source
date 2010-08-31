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

#ifndef _GLEST_GAME_MENUSTATECUSTOMGAME_H_
#define _GLEST_GAME_MENUSTATECUSTOMGAME_H_

#include "main_menu.h"
#include "chat_manager.h"
#include "simple_threads.h"

namespace Glest{ namespace Game{
// ===============================
// 	class MenuStateCustomGame
// ===============================

class MenuStateCustomGame : public MenuState, public SimpleTaskCallbackInterface {
private:
	GraphicButton buttonReturn;
	GraphicButton buttonPlayNow;
	GraphicButton buttonRestoreLastSettings;
	GraphicLabel labelControl;
	GraphicLabel labelFaction;
	GraphicLabel labelTeam;
	GraphicLabel labelMap;
	GraphicLabel labelFogOfWar;
	GraphicLabel labelTechTree;
	GraphicLabel labelTileset;
	GraphicLabel labelMapInfo;
	GraphicLabel labelEnableObserverMode;
	GraphicLabel labelEnableServerControlledAI;
	GraphicLabel labelLocalIP;
	

	GraphicListBox listBoxMap;
	GraphicListBox listBoxFogOfWar;
	GraphicListBox listBoxTechTree;
	GraphicListBox listBoxTileset;
	GraphicListBox listBoxEnableObserverMode;
	GraphicListBox listBoxEnableServerControlledAI;

	vector<string> mapFiles;
	vector<string> playerSortedMaps[GameConstants::maxPlayers+1];
	vector<string> formattedPlayerSortedMaps[GameConstants::maxPlayers+1];
	vector<string> techTreeFiles;
	vector<string> tilesetFiles;
	vector<string> factionFiles;
	GraphicLabel labelPlayers[GameConstants::maxPlayers];
	GraphicLabel labelPlayerNames[GameConstants::maxPlayers];
	GraphicListBox listBoxControls[GameConstants::maxPlayers];
	GraphicListBox listBoxFactions[GameConstants::maxPlayers];
	GraphicListBox listBoxTeams[GameConstants::maxPlayers];
	GraphicLabel labelNetStatus[GameConstants::maxPlayers];
	MapInfo mapInfo;
	
	GraphicLabel labelPublishServer;
	GraphicListBox listBoxPublishServer;

	GraphicLabel labelPublishServerExternalPort;
	GraphicListBox listBoxPublishServerExternalPort;
	
	GraphicMessageBox mainMessageBox;
	int mainMessageBoxState;
	
	GraphicListBox listBoxNetworkFramePeriod;
	GraphicLabel labelNetworkFramePeriod;
	
	GraphicLabel labelNetworkPauseGameForLaggedClients;
	GraphicListBox listBoxNetworkPauseGameForLaggedClients;

	GraphicLabel labelPathFinderType;
	GraphicListBox listBoxPathFinderType;
	
	GraphicLabel labelMapFilter;
	GraphicListBox listBoxMapFilter;
	
	GraphicLabel labelAdvanced;
	GraphicListBox listBoxAdvanced;

	GraphicLabel *activeInputLabel;

	bool needToSetChangedGameSettings;
	time_t lastSetChangedGameSettings;
	time_t lastMasterserverPublishing;
	time_t lastNetworkPing;

	bool needToRepublishToMasterserver;
	bool needToBroadcastServerSettings;
	std::map<string,string> publishToServerInfo;
	SimpleTaskThread *publishToMasterserverThread;
	Mutex masterServerThreadAccessor;
	
	bool parentMenuIsMs;
	int soundConnectionCount;
	
	bool showMasterserverError;
	string masterServererErrorToShow;

	bool showGeneralError;
	string generalErrorToShow;
	bool serverInitError;
	
	Console console;
	ChatManager chatManager;
	bool showFullConsole;

	string lastMapDataSynchError;
	string lastTileDataSynchError;
	string lastTechtreeDataSynchError;

	string defaultPlayerName;
	int8 switchSetupRequestFlagType;

	bool enableFactionTexturePreview;
	string currentFactionLogo;
	Texture2D *factionTexture;

public:
	MenuStateCustomGame(Program *program, MainMenu *mainMenu ,bool openNetworkSlots= false, bool parentMenuIsMasterserver=false);
	~MenuStateCustomGame();

	void mouseClick(int x, int y, MouseButton mouseButton);
	void mouseMove(int x, int y, const MouseState *mouseState);
	void render();
	void update();

    virtual void keyDown(char key);
    virtual void keyPress(char c);
    virtual void keyUp(char key);
    

    virtual void simpleTask();
    virtual bool isInSpecialKeyCaptureEvent() { return chatManager.getEditEnabled(); }

private:

    bool hasNetworkGameSettings();
    void loadGameSettings(GameSettings *gameSettings);
	void loadMapInfo(string file, MapInfo *mapInfo);
	void reloadFactions();
	void updateControlers();
	void closeUnusedSlots();
	void updateNetworkSlots();
	void publishToMasterserver();
	void returnToParentMenu();
	void showMessageBox(const string &text, const string &header, bool toggle);

	void saveGameSettingsToFile(std::string fileName);
	void switchToNextMapGroup(const int direction);
	string getCurrentMapFile();
	GameSettings loadGameSettingsFromFile(std::string fileName);
	void setActiveInputLabel(GraphicLabel *newLable);
	string getHumanPlayerName(int index=-1);

	void cleanupFactionTexture();
	void loadFactionTexture(string filepath);
};

}}//end namespace

#endif
