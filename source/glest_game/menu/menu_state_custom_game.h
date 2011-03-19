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
#include "map_preview.h"
#include "leak_dumper.h"

using namespace Shared::Map;

namespace Glest { namespace Game {

// ===============================
// 	class MenuStateCustomGame
// ===============================

class MenuStateCustomGame : public MenuState, public SimpleTaskCallbackInterface {
private:
	GraphicButton buttonReturn;
	GraphicButton buttonPlayNow;
	GraphicButton buttonRestoreLastSettings;
	GraphicLabel labelControl;
	GraphicLabel labelRMultiplier;
	GraphicLabel labelFaction;
	GraphicLabel labelTeam;
	GraphicLabel labelMap;
	GraphicLabel labelFogOfWar;
	GraphicLabel labelTechTree;
	GraphicLabel labelTileset;
	GraphicLabel labelMapInfo;
	GraphicLabel labelEnableObserverMode;
	//GraphicLabel labelEnableServerControlledAI;
	GraphicLabel labelLocalIP;


	GraphicListBox listBoxMap;
	GraphicListBox listBoxFogOfWar;
	GraphicListBox listBoxTechTree;
	GraphicListBox listBoxTileset;
	GraphicListBox listBoxEnableObserverMode;
	//GraphicListBox listBoxEnableServerControlledAI;

	vector<string> mapFiles;
	vector<string> playerSortedMaps[GameConstants::maxPlayers+1];
	vector<string> formattedPlayerSortedMaps[GameConstants::maxPlayers+1];
	vector<string> techTreeFiles;
	vector<string> tilesetFiles;
	vector<string> factionFiles;
	GraphicLabel labelPlayers[GameConstants::maxPlayers];
	GraphicLabel labelPlayerNames[GameConstants::maxPlayers];
	GraphicListBox listBoxControls[GameConstants::maxPlayers];
	GraphicListBox listBoxRMultiplier[GameConstants::maxPlayers];
	GraphicListBox listBoxFactions[GameConstants::maxPlayers];
	GraphicListBox listBoxTeams[GameConstants::maxPlayers];
	GraphicLabel labelNetStatus[GameConstants::maxPlayers];
	MapInfo mapInfo;

	GraphicLabel labelPublishServer;
	GraphicListBox listBoxPublishServer;

	GraphicMessageBox mainMessageBox;
	int mainMessageBoxState;

	//GraphicListBox listBoxNetworkFramePeriod;
	//GraphicLabel labelNetworkFramePeriod;

	GraphicLabel labelNetworkPauseGameForLaggedClients;
	GraphicListBox listBoxNetworkPauseGameForLaggedClients;

	GraphicLabel labelPathFinderType;
	GraphicListBox listBoxPathFinderType;

	GraphicLabel labelMapFilter;
	GraphicListBox listBoxMapFilter;

	GraphicLabel labelAdvanced;
	GraphicListBox listBoxAdvanced;

	GraphicLabel labelAllowObservers;
	GraphicListBox listBoxAllowObservers;

	GraphicLabel *activeInputLabel;

	GraphicLabel labelPlayerStatus[GameConstants::maxPlayers];
	GraphicListBox listBoxPlayerStatus;

	bool needToSetChangedGameSettings;
	time_t lastSetChangedGameSettings;
	time_t lastMasterserverPublishing;
	time_t lastNetworkPing;
	time_t mapPublishingDelayTimer;
	bool needToPublishDelayed;

	bool needToRepublishToMasterserver;
	bool needToBroadcastServerSettings;
	std::map<string,string> publishToServerInfo;
	SimpleTaskThread *publishToMasterserverThread;
	//Mutex masterServerThreadAccessor;
	//Mutex publishToMasterserverThreadPtrChangeAccessor;
	//bool publishToMasterserverThreadInDeletion;

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
	bool enableMapPreview;

	string currentTechName_factionPreview;
	string currentFactionName_factionPreview;
	string currentFactionLogo;
	Texture2D *factionTexture;

	MapPreview mapPreview;
	Texture2D *mapPreviewTexture;

	bool autostart;
	std::map<int,int> lastSelectedTeamIndex;
	float rMultiplierOffset;
	bool hasCheckedForUPNP;

    string lastCheckedCRCTilesetName;
    string lastCheckedCRCTechtreeName;
    string lastCheckedCRCMapName;
    int32 lastCheckedCRCTilesetValue;
    int32 lastCheckedCRCTechtreeValue;
    int32 lastCheckedCRCMapValue;

public:
	MenuStateCustomGame(Program *program, MainMenu *mainMenu ,bool openNetworkSlots= false, bool parentMenuIsMasterserver=false, bool autostart=false);
	virtual ~MenuStateCustomGame();

	void mouseClick(int x, int y, MouseButton mouseButton);
	void mouseMove(int x, int y, const MouseState *mouseState);
	void render();
	void update();

    virtual void keyDown(char key);
    virtual void keyPress(char c);
    virtual void keyUp(char key);


    virtual void simpleTask(BaseThread *callingThread);
    virtual bool isInSpecialKeyCaptureEvent() { return chatManager.getEditEnabled(); }

private:

    bool hasNetworkGameSettings();
    void loadGameSettings(GameSettings *gameSettings);
	void loadMapInfo(string file, MapInfo *mapInfo,bool loadMapPreview);
	void cleanupMapPreviewTexture();

	void reloadFactions(bool keepExistingSelectedItem);
	void updateControlers();
	void closeUnusedSlots();
	void updateNetworkSlots();
	void publishToMasterserver();
	void returnToParentMenu();
	void showMessageBox(const string &text, const string &header, bool toggle);

	void saveGameSettingsToFile(std::string fileName);
	void switchToNextMapGroup(const int direction);
	void updateAllResourceMultiplier();
	void updateResourceMultiplier(const int index);
	string getCurrentMapFile();
	GameSettings loadGameSettingsFromFile(std::string fileName);
	void setActiveInputLabel(GraphicLabel *newLable);
	string getHumanPlayerName(int index=-1);

	void loadFactionTexture(string filepath);

	void RestoreLastGameSettings();
	void PlayNow();

	void SetActivePlayerNameEditor();
	void cleanup();

	int32 getNetworkPlayerStatus();
};

}}//end namespace

#endif
