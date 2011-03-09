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
	GraphicButton buttonPlayNow;
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
	GraphicLabel labelPlayers[GameConstants::maxPlayers];
	GraphicLabel labelPlayerNames[GameConstants::maxPlayers];
	GraphicListBox listBoxControls[GameConstants::maxPlayers];
	GraphicListBox listBoxRMultiplier[GameConstants::maxPlayers];
	GraphicListBox listBoxFactions[GameConstants::maxPlayers];
	GraphicListBox listBoxTeams[GameConstants::maxPlayers];
	GraphicLabel labelNetStatus[GameConstants::maxPlayers];
	GraphicButton grabSlotButton[GameConstants::maxPlayers];

	GraphicLabel labelAllowObservers;
	GraphicListBox listBoxAllowObservers;

	GraphicLabel *activeInputLabel;

    time_t timerLabelFlash;
    GraphicLabel labelDataSynchInfo;

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

    GraphicMessageBox ftpMessageBox;
    FTPClientThread *ftpClientThread;
    FTPMessageType ftpMissingDataType;

    string getMissingMapFromFTPServer;
    bool getMissingMapFromFTPServerInProgress;

    string getMissingTilesetFromFTPServer;
    bool getMissingTilesetFromFTPServerInProgress;

    string getMissingTechtreeFromFTPServer;
    bool getMissingTechtreeFromFTPServerInProgress;

    std::map<string,pair<int,string> > fileFTPProgressList;

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
	bool loadFactions(const GameSettings *gameSettings,bool errorOnNoFactions);
	void returnToJoinMenu();
	string getHumanPlayerName();
	void setActiveInputLabel(GraphicLabel *newLable);

	void loadFactionTexture(string filepath);
	bool loadMapInfo(string file, MapInfo *mapInfo, bool loadMapPreview);
	void showMessageBox(const string &text, const string &header, bool toggle);

    void showFTPMessageBox(const string &text, const string &header, bool toggle);
    virtual void FTPClient_CallbackEvent(string itemName, FTP_Client_CallbackType type, FTP_Client_ResultType result,void *userdata);
};

}}//end namespace

#endif
