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

#ifndef _GLEST_GAME_MENUSTATECONNECTEDGAME_H_
#define _GLEST_GAME_MENUSTATECONNECTEDGAME_H_

#ifdef WIN32
    #include <winsock2.h>
    #include <winsock.h>
#endif

#include "main_menu.h"
#include "chat_manager.h"
#include "map_preview.h"
#include "miniftpclient.h"
#include "common_scoped_ptr.h"
#include "leak_dumper.h"

namespace Shared { namespace Graphics {
	class VideoPlayer;
}}

namespace Glest { namespace Game {

class TechTree;

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

class MenuStateConnectedGame: public MenuState, public FTPClientCallbackInterface, public SimpleTaskCallbackInterface {
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
	GraphicLabel labelWaitingForPlayers;
	GraphicButton buttonRestoreLastSettings;

	//GraphicLabel labelPathFinderType;
	//GraphicListBox listBoxPathFinderType;

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

	GraphicLabel labelMapFilter;
	GraphicListBox listBoxMapFilter;

	GraphicLabel labelAllowObservers;
	GraphicCheckBox checkBoxAllowObservers;

	GraphicLabel labelAllowNativeLanguageTechtree;
	GraphicCheckBox checkBoxAllowNativeLanguageTechtree;

	GraphicLabel *activeInputLabel;

    time_t timerLabelFlash;
    GraphicLabel labelDataSynchInfo;

	MapInfo mapInfo;
	Texture2D *mapPreviewTexture;
	bool zoomedMap;
	int render_mapPreviewTexture_X;
	int render_mapPreviewTexture_Y;
	int render_mapPreviewTexture_W;
	int render_mapPreviewTexture_H;

	bool needToSetChangedGameSettings;
	time_t lastSetChangedGameSettings;
	bool updateDataSynchDetailText;

	int soundConnectionCount;

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
	::Shared::Graphics::VideoPlayer *factionVideo;
	bool factionVideoSwitchedOffVolume;

	MapPreview mapPreview;

	GraphicMessageBox mainMessageBox;

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

    SimpleTaskThread *modHttpServerThread;
	std::vector<std::string> tilesetListRemote;
	std::map<string, ModInfo> tilesetCacheList;
	std::vector<std::string> techListRemote;
	std::map<string, ModInfo> techCacheList;
	std::vector<std::string> mapListRemote;
	std::map<string, ModInfo> mapCacheList;

	std::map<string,uint32> mapCRCUpdateList;



    string getMissingMapFromFTPServer;
    bool getMissingMapFromFTPServerInProgress;
    time_t getMissingMapFromFTPServerLastPrompted;

    string getMissingTilesetFromFTPServer;
    bool getMissingTilesetFromFTPServerInProgress;
    time_t getMissingTilesetFromFTPServerLastPrompted;

    string getMissingTechtreeFromFTPServer;
    bool getMissingTechtreeFromFTPServerInProgress;
    time_t getMissingTechtreeFromFTPServerLastPrompted;

    string getInProgressSavedGameFromFTPServer;
    bool getInProgressSavedGameFromFTPServerInProgress;
    bool readyToJoinInProgressGame;

    string lastCheckedCRCTilesetName;
    string lastCheckedCRCTechtreeName;
    string lastCheckedCRCMapName;
    uint32 lastCheckedCRCTilesetValue;
    uint32 lastCheckedCRCTechtreeValue;
    uint32 lastCheckedCRCMapValue;
    vector<pair<string,uint32> > factionCRCList;

    std::map<string,pair<int,string> > fileFTPProgressList;
    GraphicButton buttonCancelDownloads;

	GraphicLabel labelEnableSwitchTeamMode;
	GraphicCheckBox checkBoxEnableSwitchTeamMode;

	GraphicLabel labelAllowTeamUnitSharing;
	GraphicCheckBox checkBoxAllowTeamUnitSharing;

	GraphicLabel labelAllowTeamResourceSharing;
	GraphicCheckBox checkBoxAllowTeamResourceSharing;

	GraphicLabel labelAISwitchTeamAcceptPercent;
	GraphicListBox listBoxAISwitchTeamAcceptPercent;
	GraphicLabel labelFallbackCpuMultiplier;
	GraphicListBox listBoxFallbackCpuMultiplier;


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

	time_t noReceiveTimer;

	bool launchingNewGame;
	bool isfirstSwitchingMapMessage;
	auto_ptr<TechTree> techTree;

	GameSettings originalGamesettings;
	bool validOriginalGameSettings;
	GameSettings displayedGamesettings;
	bool validDisplayedGamesettings;


public:

	MenuStateConnectedGame(Program *program, MainMenu *mainMenu, JoinMenu joinMenuInfo=jmSimple, bool openNetworkSlots= false);
	virtual ~MenuStateConnectedGame();

	void mouseClick(int x, int y, MouseButton mouseButton);
	void mouseMove(int x, int y, const MouseState *mouseState);
	void render();
	void update();

	virtual bool textInput(std::string text);
    virtual void keyDown(SDL_KeyboardEvent key);
    virtual void keyPress(SDL_KeyboardEvent c);
    virtual void keyUp(SDL_KeyboardEvent key);

    virtual bool isInSpecialKeyCaptureEvent();

    virtual void reloadUI();

    virtual bool isVideoPlaying();

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

    void mouseClickAdmin(int x, int y, MouseButton mouseButton,string advanceToItemStartingWith);
    void switchToNextMapGroup(const int direction);
    void switchToMapGroup(int filterIndex);
    string getCurrentMapFile();
    void loadGameSettings(GameSettings *gameSettings);
    void reloadFactions(bool keepExistingSelectedItem,string scenario);
    void PlayNow(bool saveGame);
    bool isHeadlessAdmin();
    void broadCastGameSettingsToHeadlessServer(bool forceNow);
    void updateResourceMultiplier(const int index);

    void RestoreLastGameSettings();
    void setupUIFromGameSettings(GameSettings *gameSettings, bool errorOnMissingData);

	int setupMapList(string scenario);
	int setupTechList(string scenario, bool forceLoad=false);
	void setupTilesetList(string scenario);

	void loadScenarioInfo(string file, ScenarioInfo *scenarioInfo);
	void initFactionPreview(const GameSettings *gameSettings);

	virtual void simpleTask(BaseThread *callingThread,void *userdata);
	string refreshTilesetModInfo(string tilesetInfo);
	string refreshTechModInfo(string techInfo);
	string refreshMapModInfo(string mapInfo);
	string getMapCRC(string mapName);

	void disconnectFromServer();
};

}}//end namespace

#endif
