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
#include "map_preview.h"
#include "miniftpclient.h"
#include "leak_dumper.h"

namespace Glest { namespace Game {

enum JoinMenu {
	jmSimple,
	jmMasterserver,

	jmCount
};

enum FTPMessageType {
    ftpmsg_MissingNone,
	ftpmsg_MissingMap,
	ftpmsg_MissingTileset,
	ftpmsg_MissingTechtree
};

// ===============================
// 	class MenuStateConnectedGame
// ===============================

class MenuStateConnectedGame: public MenuState, public FTPClientCallbackInterface {
private:
	GraphicButton buttonDisconnect;
	GraphicLabel labelControl;
	GraphicLabel labelRMultiplier;
	GraphicLabel labelFaction;
	GraphicLabel labelTeam;
	GraphicLabel labelMap;
	GraphicLabel labelFogOfWar;
	GraphicLabel labelTechTree;
	GraphicLabel labelTileset;
	GraphicLabel labelMapInfo;
	GraphicLabel labelStatus;
	GraphicLabel labelInfo;
	GraphicButton buttonRestoreLastSettings;

	//GraphicLabel labelEnableObserverMode;
	//GraphicListBox listBoxEnableObserverMode;
	//GraphicLabel labelEnableServerControlledAI;
	//GraphicListBox listBoxEnableServerControlledAI;
	//GraphicLabel labelNetworkPauseGameForLaggedClients;
	//GraphicListBox listBoxNetworkPauseGameForLaggedClients;

	//GraphicListBox listBoxNetworkFramePeriod;
	//GraphicLabel labelNetworkFramePeriod;

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
	GraphicLabel labelPlayers[GameConstants::maxPlayers];
	GraphicLabel labelPlayerNames[GameConstants::maxPlayers];
	GraphicListBox listBoxControls[GameConstants::maxPlayers];
	GraphicListBox listBoxRMultiplier[GameConstants::maxPlayers];
	GraphicListBox listBoxFactions[GameConstants::maxPlayers];
	GraphicListBox listBoxTeams[GameConstants::maxPlayers];
	GraphicLabel labelNetStatus[GameConstants::maxPlayers];
	GraphicButton grabSlotButton[GameConstants::maxPlayers];

	GraphicListBox listBoxPlayerStatus;
	GraphicLabel labelPlayerStatus[GameConstants::maxPlayers];

	GraphicLabel labelAllowObservers;
	GraphicListBox listBoxAllowObservers;

	GraphicLabel *activeInputLabel;

    time_t timerLabelFlash;
    GraphicLabel labelDataSynchInfo;

	MapInfo mapInfo;
	Texture2D *mapPreviewTexture;

	bool needToSetChangedGameSettings;
	time_t lastSetChangedGameSettings;
	bool updateDataSynchDetailText;

	//Console console;
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

	bool enableFactionTexturePreview;
	bool enableMapPreview;

	string currentTechName_factionPreview;
	string currentFactionName_factionPreview;
	string currentFactionLogo;
	Texture2D *factionTexture;

	MapPreview mapPreview;

	GraphicMessageBox mainMessageBox;
	int mainMessageBoxState;

	std::string lastMissingMap;
	std::string lastMissingTechtree;
	std::string lastMissingTileSet;

	vector<string> mapFiles;
	vector<string> techTreeFiles;
	vector<string> tilesetFiles;
	vector<string> factionFiles;

	vector<string> playerSortedMaps[GameConstants::maxPlayers+1];
	vector<string> formattedPlayerSortedMaps[GameConstants::maxPlayers+1];
	vector<string> formattedMapFiles;

    GraphicMessageBox ftpMessageBox;
    FTPClientThread *ftpClientThread;
    FTPMessageType ftpMissingDataType;

    string getMissingMapFromFTPServer;
    bool getMissingMapFromFTPServerInProgress;

    string getMissingTilesetFromFTPServer;
    bool getMissingTilesetFromFTPServerInProgress;

    string getMissingTechtreeFromFTPServer;
    bool getMissingTechtreeFromFTPServerInProgress;

    string lastCheckedCRCTilesetName;
    string lastCheckedCRCTechtreeName;
    string lastCheckedCRCMapName;
    int32 lastCheckedCRCTilesetValue;
    int32 lastCheckedCRCTechtreeValue;
    int32 lastCheckedCRCMapValue;
    vector<pair<string,int32> > factionCRCList;

    std::map<string,pair<int,string> > fileFTPProgressList;
    GraphicButton buttonCancelDownloads;

	GraphicLabel labelEnableSwitchTeamMode;
	GraphicListBox listBoxEnableSwitchTeamMode;
	GraphicLabel labelAISwitchTeamAcceptPercent;
	GraphicListBox listBoxAISwitchTeamAcceptPercent;

	GraphicButton buttonPlayNow;

	GraphicCheckBox checkBoxScenario;
	GraphicLabel labelScenario;
	GraphicListBox listBoxScenario;
	vector<string> scenarioFiles;
    ScenarioInfo scenarioInfo;
	vector<string> dirList;
	string autoloadScenarioName;
	time_t previewLoadDelayTimer;
	bool needToLoadTextures;
	bool enableScenarioTexturePreview;
	Texture2D *scenarioLogoTexture;


	bool needToBroadcastServerSettings;
	time_t broadcastServerSettingsDelayTimer;
	int lastGameSettingsReceivedCount;

public:

	MenuStateConnectedGame(Program *program, MainMenu *mainMenu, JoinMenu joinMenuInfo=jmSimple, bool openNetworkSlots= false);
	~MenuStateConnectedGame();

	void mouseClick(int x, int y, MouseButton mouseButton);
	void mouseMove(int x, int y, const MouseState *mouseState);
	void render();
	void update();

    virtual void keyDown(SDL_KeyboardEvent key);
    virtual void keyPress(SDL_KeyboardEvent c);
    virtual void keyUp(SDL_KeyboardEvent key);

    virtual bool isInSpecialKeyCaptureEvent();

    virtual void reloadUI();

private:

    bool hasNetworkGameSettings();
	bool loadFactions(const GameSettings *gameSettings,bool errorOnNoFactions);
	void returnToJoinMenu();
	string getHumanPlayerName();
	void setActiveInputLabel(GraphicLabel *newLable);

	void loadFactionTexture(string filepath);
	bool loadMapInfo(string file, MapInfo *mapInfo, bool loadMapPreview);
	void showMessageBox(const string &text, const string &header, bool toggle);

    void showFTPMessageBox(const string &text, const string &header, bool toggle);
    virtual void FTPClient_CallbackEvent(string itemName,
    		FTP_Client_CallbackType type, pair<FTP_Client_ResultType,string> result,void *userdata);

    int32 getNetworkPlayerStatus();
    void cleanupMapPreviewTexture();

    void mouseClickAdmin(int x, int y, MouseButton mouseButton);
    string getCurrentMapFile();
    void loadGameSettings(GameSettings *gameSettings);
    void reloadFactions(bool keepExistingSelectedItem);
    void PlayNow(bool saveGame);
    bool isMasterserverAdmin();
    void broadCastGameSettingsToMasterserver(bool forceNow);
    void updateResourceMultiplier(const int index);

    void RestoreLastGameSettings();
    void setupUIFromGameSettings(GameSettings *gameSettings, bool errorOnMissingData);
};

}}//end namespace

#endif
