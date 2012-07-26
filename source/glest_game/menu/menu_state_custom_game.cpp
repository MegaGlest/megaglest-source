//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================
#include "menu_state_custom_game.h"

#include "renderer.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "config.h"
#include "menu_state_new_game.h"
#include "menu_state_masterserver.h"
#include "menu_state_join_game.h"
#include "metrics.h"
#include "network_manager.h"
#include "network_message.h"
#include "client_interface.h"
#include "conversion.h"
#include "socket.h"
#include "game.h"
#include "util.h"
#include <algorithm>
#include <time.h>
#include <curl/curl.h>
#include "cache_manager.h"
#include <iterator>
#include "map_preview.h"
#include "string_utils.h"
#include "leak_dumper.h"

namespace Glest{ namespace Game{

using namespace Shared::Util;

const int MASTERSERVER_BROADCAST_MAX_WAIT_RESPONSE_SECONDS   	= 10;
const int MASTERSERVER_BROADCAST_PUBLISH_SECONDS   	= 6;
const int BROADCAST_MAP_DELAY_SECONDS 	= 5;
const int BROADCAST_SETTINGS_SECONDS  	= 4;
static const char *SAVED_GAME_FILENAME 				= "lastCustomGamSettings.mgg";

struct FormatString {
	void operator()(string &s) {
		s = formatString(s);
	}
};

// =====================================================
// 	class MenuStateCustomGame
// =====================================================

MenuStateCustomGame::MenuStateCustomGame(Program *program, MainMenu *mainMenu,
		bool openNetworkSlots,ParentMenuState parentMenuState, bool autostart,
		GameSettings *settings, bool masterserverMode,
		string autoloadScenarioName) :
		MenuState(program, mainMenu, "new-game") {
	try {

	this->headlessServerMode = masterserverMode;
	if(this->headlessServerMode == true) {
		printf("Waiting for players to join and start a game...\n");
	}

	this->lastMasterServerSettingsUpdateCount = 0;
	this->masterserverModeMinimalResources = true;
	this->parentMenuState=parentMenuState;
	this->factionVideo = NULL;
	factionVideoSwitchedOffVolume=false;

	//printf("this->masterserverMode = %d [%d]\n",this->masterserverMode,masterserverMode);

	forceWaitForShutdown = true;
	this->autostart = autostart;
	this->autoStartSettings = settings;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] autostart = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,autostart);

	containerName = "CustomGame";
	activeInputLabel=NULL;
	showGeneralError = false;
	generalErrorToShow = "---";
	currentFactionLogo = "";
	factionTexture=NULL;
	currentTechName_factionPreview="";
	currentFactionName_factionPreview="";
	mapPreviewTexture=NULL;
	hasCheckedForUPNP = false;
	needToPublishDelayed=false;
	mapPublishingDelayTimer=time(NULL);

    lastCheckedCRCTilesetName					= "";
    lastCheckedCRCTechtreeName					= "";
    lastCheckedCRCMapName						= "";
    lastCheckedCRCTilesetValue					= -1;
    lastCheckedCRCTechtreeValue					= -1;
    lastCheckedCRCMapValue						= -1;

	publishToMasterserverThread = NULL;
	Lang &lang= Lang::getInstance();
	NetworkManager &networkManager= NetworkManager::getInstance();
    Config &config = Config::getInstance();
    defaultPlayerName = config.getString("NetPlayerName",Socket::getHostName().c_str());
    enableFactionTexturePreview = config.getBool("FactionPreview","true");
    enableMapPreview = config.getBool("MapPreview","true");

    showFullConsole=false;

	enableScenarioTexturePreview = Config::getInstance().getBool("EnableScenarioTexturePreview","true");
	scenarioLogoTexture=NULL;
	previewLoadDelayTimer=time(NULL);
	needToLoadTextures=true;
	this->autoloadScenarioName = autoloadScenarioName;
	this->dirList = Config::getInstance().getPathListForType(ptScenarios);

    mainMessageBox.registerGraphicComponent(containerName,"mainMessageBox");
	mainMessageBox.init(lang.get("Ok"));
	mainMessageBox.setEnabled(false);
	mainMessageBoxState=0;

	//initialize network interface
	NetworkManager::getInstance().end();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	serverInitError = false;
	try {
		networkManager.init(nrServer,openNetworkSlots);
	}
	catch(const std::exception &ex) {
		serverInitError = true;
		char szBuf[8096]="";
		sprintf(szBuf,"In [%s::%s %d]\nNetwork init error:\n%s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);

		showGeneralError=true;
		generalErrorToShow = szBuf;
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	needToSetChangedGameSettings = false;
	needToRepublishToMasterserver = false;
	needToBroadcastServerSettings = false;
	showMasterserverError = false;
	tMasterserverErrorElapsed = 0;
	masterServererErrorToShow = "---";
	lastSetChangedGameSettings  = 0;
	lastMasterserverPublishing 	= 0;
	lastNetworkPing				= 0;
	soundConnectionCount=0;

	vector<string> teamItems, controlItems, results , rMultiplier;

	//create
	buttonReturn.registerGraphicComponent(containerName,"buttonReturn");
	buttonReturn.init(240, 180, 125);

	buttonClearBlockedPlayers.registerGraphicComponent(containerName,"buttonClearBlockedPlayers");
	buttonClearBlockedPlayers.init(427, 590, 125);

	buttonRestoreLastSettings.registerGraphicComponent(containerName,"buttonRestoreLastSettings");
	buttonRestoreLastSettings.init(250+130, 180, 220);

	buttonPlayNow.registerGraphicComponent(containerName,"buttonPlayNow");
	buttonPlayNow.init(250+130+235, 180, 125);

	int labelOffset=23;
	int setupPos=590;
	int mapHeadPos=330;
	int mapPos=mapHeadPos-labelOffset;
	//int aHeadPos=mapHeadPos-80;
	int aHeadPos=240;
	int aPos=aHeadPos-labelOffset;
	int networkHeadPos=700;
	int networkPos=networkHeadPos-labelOffset;
	int xoffset=10;

	labelLocalGameVersion.registerGraphicComponent(containerName,"labelLocalGameVersion");
	labelLocalGameVersion.init(10, networkHeadPos+labelOffset);

	labelLocalIP.registerGraphicComponent(containerName,"labelLocalIP");
	labelLocalIP.init(360, networkHeadPos+labelOffset);

	string ipText = "none";
	std::vector<std::string> ipList = Socket::getLocalIPAddressList();
	if(ipList.empty() == false) {
		ipText = "";
		for(int idx = 0; idx < ipList.size(); idx++) {
			string ip = ipList[idx];
			if(ipText != "") {
				ipText += ", ";
			}
			ipText += ip;
		}
	}
	string externalPort=config.getString("MasterServerExternalPort", intToStr(GameConstants::serverPort).c_str());
	string serverPort=config.getString("ServerPort", intToStr(GameConstants::serverPort).c_str());
	labelLocalIP.setText(lang.get("LanIP") + ipText + "  ( "+serverPort+" / "+externalPort+" )");
	ServerSocket::setExternalPort(strToInt(externalPort));

	if(EndsWith(glestVersionString, "-dev") == false){
		labelLocalGameVersion.setText(glestVersionString);
	}
	else {
		labelLocalGameVersion.setText(glestVersionString + " [" + getCompileDateTime() + ", " + getSVNRevisionString() + "]");
	}

	xoffset=70;
	// MapFilter
	labelMapFilter.registerGraphicComponent(containerName,"labelMapFilter");
	labelMapFilter.init(xoffset+310, mapHeadPos);
	labelMapFilter.setText(lang.get("MapFilter")+":");

	listBoxMapFilter.registerGraphicComponent(containerName,"listBoxMapFilter");
	listBoxMapFilter.init(xoffset+310, mapPos, 80);
	listBoxMapFilter.pushBackItem("-");
	for(int i=1; i<GameConstants::maxPlayers+1; ++i){
		listBoxMapFilter.pushBackItem(intToStr(i));
	}
	listBoxMapFilter.setSelectedItemIndex(0);

	// Map
	labelMap.registerGraphicComponent(containerName,"labelMap");
	labelMap.init(xoffset+100, mapHeadPos);
	labelMap.setText(lang.get("Map")+":");

	//map listBox
	listBoxMap.registerGraphicComponent(containerName,"listBoxMap");
	listBoxMap.init(xoffset+100, mapPos, 200);
	// put them all in a set, to weed out duplicates (gbm & mgm with same name)
	// will also ensure they are alphabetically listed (rather than how the OS provides them)
	int initialMapSelection = setupMapList("");
    listBoxMap.setItems(formattedPlayerSortedMaps[0]);
    listBoxMap.setSelectedItemIndex(initialMapSelection);

    labelMapInfo.registerGraphicComponent(containerName,"labelMapInfo");
	labelMapInfo.init(xoffset+100, mapPos-labelOffset-10, 200, 40);

    labelTileset.registerGraphicComponent(containerName,"labelTileset");
	labelTileset.init(xoffset+460, mapHeadPos);
	labelTileset.setText(lang.get("Tileset"));

	//tileset listBox
	listBoxTileset.registerGraphicComponent(containerName,"listBoxTileset");
	listBoxTileset.init(xoffset+460, mapPos, 150);
    //listBoxTileset.setItems(results);
	setupTilesetList("");
    srand(time(NULL));
	listBoxTileset.setSelectedItemIndex(rand() % listBoxTileset.getItemCount());

    //tech Tree listBox
    int initialTechSelection = setupTechList("");

	listBoxTechTree.registerGraphicComponent(containerName,"listBoxTechTree");
	listBoxTechTree.init(xoffset+650, mapPos, 150);
    //listBoxTechTree.setItems(results);
    listBoxTechTree.setSelectedItemIndex(initialTechSelection);

    labelTechTree.registerGraphicComponent(containerName,"labelTechTree");
	labelTechTree.init(xoffset+650, mapHeadPos);
	labelTechTree.setText(lang.get("TechTree"));

	// fog - o - war
	// @350 ? 300 ?
	labelFogOfWar.registerGraphicComponent(containerName,"labelFogOfWar");
	labelFogOfWar.init(xoffset+100, aHeadPos, 130);
	labelFogOfWar.setText(lang.get("FogOfWar"));

	listBoxFogOfWar.registerGraphicComponent(containerName,"listBoxFogOfWar");
	listBoxFogOfWar.init(xoffset+100, aPos, 130);
	listBoxFogOfWar.pushBackItem(lang.get("Enabled"));
	listBoxFogOfWar.pushBackItem(lang.get("Explored"));
	listBoxFogOfWar.pushBackItem(lang.get("Disabled"));
	listBoxFogOfWar.setSelectedItemIndex(0);

	// Allow Observers
	labelAllowObservers.registerGraphicComponent(containerName,"labelAllowObservers");
	labelAllowObservers.init(xoffset+310, aHeadPos, 80);
	labelAllowObservers.setText(lang.get("AllowObservers"));

	listBoxAllowObservers.registerGraphicComponent(containerName,"listBoxAllowObservers");
	listBoxAllowObservers.init(xoffset+310, aPos, 80);
	listBoxAllowObservers.pushBackItem(lang.get("No"));
	listBoxAllowObservers.pushBackItem(lang.get("Yes"));
	listBoxAllowObservers.setSelectedItemIndex(0);

	// View Map At End Of Game
	//labelEnableObserverMode.registerGraphicComponent(containerName,"labelEnableObserverMode");
	//labelEnableObserverMode.init(xoffset+460, aHeadPos, 80);

	//listBoxEnableObserverMode.registerGraphicComponent(containerName,"listBoxEnableObserverMode");
	//listBoxEnableObserverMode.init(xoffset+460, aPos, 80);
	//listBoxEnableObserverMode.pushBackItem(lang.get("Yes"));
	//listBoxEnableObserverMode.pushBackItem(lang.get("No"));
	//listBoxEnableObserverMode.setSelectedItemIndex(0);

	// Allow Switch Team Mode
	labelEnableSwitchTeamMode.registerGraphicComponent(containerName,"labelEnableSwitchTeamMode");
	labelEnableSwitchTeamMode.init(xoffset+310, aHeadPos+45, 80);
	labelEnableSwitchTeamMode.setText(lang.get("EnableSwitchTeamMode"));

	listBoxEnableSwitchTeamMode.registerGraphicComponent(containerName,"listBoxEnableSwitchTeamMode");
	listBoxEnableSwitchTeamMode.init(xoffset+310, aPos+45, 80);
	listBoxEnableSwitchTeamMode.pushBackItem(lang.get("Yes"));
	listBoxEnableSwitchTeamMode.pushBackItem(lang.get("No"));
	listBoxEnableSwitchTeamMode.setSelectedItemIndex(1);

	labelAISwitchTeamAcceptPercent.registerGraphicComponent(containerName,"labelAISwitchTeamAcceptPercent");
	labelAISwitchTeamAcceptPercent.init(xoffset+460, aHeadPos+45, 80);
	labelAISwitchTeamAcceptPercent.setText(lang.get("AISwitchTeamAcceptPercent"));

	listBoxAISwitchTeamAcceptPercent.registerGraphicComponent(containerName,"listBoxAISwitchTeamAcceptPercent");
	listBoxAISwitchTeamAcceptPercent.init(xoffset+460, aPos+45, 80);
	for(int i = 0; i <= 100; i = i + 10) {
		listBoxAISwitchTeamAcceptPercent.pushBackItem(intToStr(i));
	}
	listBoxAISwitchTeamAcceptPercent.setSelectedItem(intToStr(30));

	// Which Pathfinder
	labelPathFinderType.registerGraphicComponent(containerName,"labelPathFinderType");
	labelPathFinderType.init(xoffset+650, aHeadPos, 80);
	labelPathFinderType.setText(lang.get("PathFinderType"));

	listBoxPathFinderType.registerGraphicComponent(containerName,"listBoxPathFinderType");
	listBoxPathFinderType.init(xoffset+650, aPos, 150);
	listBoxPathFinderType.pushBackItem(lang.get("PathFinderTypeRegular"));
	if(config.getBool("EnableRoutePlannerPathfinder","false") == true) {
		listBoxPathFinderType.pushBackItem(lang.get("PathFinderTypeRoutePlanner"));
	}
	listBoxPathFinderType.setSelectedItemIndex(0);

	// Advanced Options
	labelAdvanced.registerGraphicComponent(containerName,"labelAdvanced");
	labelAdvanced.init(810, 80, 80);
	labelAdvanced.setText(lang.get("AdvancedGameOptions"));

	listBoxAdvanced.registerGraphicComponent(containerName,"listBoxAdvanced");
	listBoxAdvanced.init(810,  80-labelOffset, 80);
	listBoxAdvanced.pushBackItem(lang.get("No"));
	listBoxAdvanced.pushBackItem(lang.get("Yes"));
	listBoxAdvanced.setSelectedItemIndex(0);

	// network things
	// PublishServer
	xoffset=90;

	labelPublishServer.registerGraphicComponent(containerName,"labelPublishServer");
	labelPublishServer.init(50, networkHeadPos, 100);
	labelPublishServer.setText(lang.get("PublishServer"));

	listBoxPublishServer.registerGraphicComponent(containerName,"listBoxPublishServer");
	listBoxPublishServer.init(50, networkPos, 100);
	listBoxPublishServer.pushBackItem(lang.get("Yes"));
	listBoxPublishServer.pushBackItem(lang.get("No"));
	if(this->headlessServerMode == true ||
		(openNetworkSlots == true && parentMenuState != pLanGame)) {
		listBoxPublishServer.setSelectedItemIndex(0);
	}
	else {
		listBoxPublishServer.setSelectedItemIndex(1);
	}

	labelGameNameLabel.registerGraphicComponent(containerName,"labelGameNameLabel");
	labelGameNameLabel.init(50, networkHeadPos-2*labelOffset-3,100);
	labelGameNameLabel.setText(lang.get("MGGameTitle")+":");

	labelGameName.registerGraphicComponent(containerName,"labelGameName");
	labelGameName.init(150, networkHeadPos-2*labelOffset,100);
	labelGameName.setFont(CoreData::getInstance().getMenuFontBig());
	labelGameName.setFont3D(CoreData::getInstance().getMenuFontBig3D());
	if(this->headlessServerMode == false) {
		labelGameName.setText(defaultPlayerName+"'s game");
	}
	else {
		labelGameName.setText("headless ("+defaultPlayerName+")");
	}
	labelGameName.setMaxEditWidth(20);
	// Network Frame Period
	//labelNetworkFramePeriod.registerGraphicComponent(containerName,"labelNetworkFramePeriod");
	//labelNetworkFramePeriod.init(xoffset+230, networkHeadPos, 80);
	//labelNetworkFramePeriod.setText(lang.get("NetworkFramePeriod"));

	//listBoxNetworkFramePeriod.registerGraphicComponent(containerName,"listBoxNetworkFramePeriod");
	//listBoxNetworkFramePeriod.init(xoffset+230, networkPos, 80);
	//listBoxNetworkFramePeriod.pushBackItem("10");
	//listBoxNetworkFramePeriod.pushBackItem("20");
	//listBoxNetworkFramePeriod.pushBackItem("30");
	//listBoxNetworkFramePeriod.pushBackItem("40");
	//listBoxNetworkFramePeriod.setSelectedItem("20");

	// Network Pause for lagged clients
	labelNetworkPauseGameForLaggedClients.registerGraphicComponent(containerName,"labelNetworkPauseGameForLaggedClients");
	labelNetworkPauseGameForLaggedClients.init(xoffset+380, networkHeadPos, 80);
	labelNetworkPauseGameForLaggedClients.setText(lang.get("NetworkPauseGameForLaggedClients"));

	listBoxNetworkPauseGameForLaggedClients.registerGraphicComponent(containerName,"listBoxNetworkPauseGameForLaggedClients");
	listBoxNetworkPauseGameForLaggedClients.init(xoffset+380, networkPos, 80);
	listBoxNetworkPauseGameForLaggedClients.pushBackItem(lang.get("No"));
	listBoxNetworkPauseGameForLaggedClients.pushBackItem(lang.get("Yes"));
	listBoxNetworkPauseGameForLaggedClients.setSelectedItem(lang.get("Yes"));

	// Enable Server Controlled AI
	//labelEnableServerControlledAI.registerGraphicComponent(containerName,"labelEnableServerControlledAI");
	//labelEnableServerControlledAI.init(xoffset+550, networkHeadPos, 80);
	//labelEnableServerControlledAI.setText(lang.get("EnableServerControlledAI"));

	//listBoxEnableServerControlledAI.registerGraphicComponent(containerName,"listBoxEnableServerControlledAI");
	//listBoxEnableServerControlledAI.init(xoffset+550, networkPos, 80);
	//listBoxEnableServerControlledAI.pushBackItem(lang.get("Yes"));
	//listBoxEnableServerControlledAI.pushBackItem(lang.get("No"));
	//listBoxEnableServerControlledAI.setSelectedItemIndex(0);


	//list boxes
	xoffset=100;
	int rowHeight=27;
    for(int i=0; i<GameConstants::maxPlayers; ++i){
		labelPlayerStatus[i].registerGraphicComponent(containerName,"labelPlayerStatus" + intToStr(i));
		labelPlayerStatus[i].init(10, setupPos-30-i*rowHeight, 60);

    	labelPlayers[i].registerGraphicComponent(containerName,"labelPlayers" + intToStr(i));
		labelPlayers[i].init(xoffset, setupPos-30-i*rowHeight);

		labelPlayerNames[i].registerGraphicComponent(containerName,"labelPlayerNames" + intToStr(i));
		labelPlayerNames[i].init(xoffset+70,setupPos-30-i*rowHeight);

		listBoxControls[i].registerGraphicComponent(containerName,"listBoxControls" + intToStr(i));
        listBoxControls[i].init(xoffset+210, setupPos-30-i*rowHeight);

        buttonBlockPlayers[i].registerGraphicComponent(containerName,"buttonBlockPlayers" + intToStr(i));
        buttonBlockPlayers[i].init(xoffset+355, setupPos-30-i*rowHeight, 70);
        buttonBlockPlayers[i].setText(lang.get("BlockPlayer"));
        buttonBlockPlayers[i].setFont(CoreData::getInstance().getDisplayFontSmall());
        buttonBlockPlayers[i].setFont3D(CoreData::getInstance().getDisplayFontSmall3D());

        listBoxRMultiplier[i].registerGraphicComponent(containerName,"listBoxRMultiplier" + intToStr(i));
        listBoxRMultiplier[i].init(xoffset+350, setupPos-30-i*rowHeight,70);

        listBoxFactions[i].registerGraphicComponent(containerName,"listBoxFactions" + intToStr(i));
        listBoxFactions[i].init(xoffset+430, setupPos-30-i*rowHeight, 150);

        listBoxTeams[i].registerGraphicComponent(containerName,"listBoxTeams" + intToStr(i));
		listBoxTeams[i].init(xoffset+590, setupPos-30-i*rowHeight, 60);

		labelNetStatus[i].registerGraphicComponent(containerName,"labelNetStatus" + intToStr(i));
		labelNetStatus[i].init(xoffset+655, setupPos-30-i*rowHeight, 60);
		labelNetStatus[i].setFont(CoreData::getInstance().getDisplayFontSmall());
		labelNetStatus[i].setFont3D(CoreData::getInstance().getDisplayFontSmall3D());
    }

	labelControl.registerGraphicComponent(containerName,"labelControl");
	labelControl.init(xoffset+210, setupPos, GraphicListBox::defW, GraphicListBox::defH, true);
	labelControl.setText(lang.get("Control"));

    labelRMultiplier.registerGraphicComponent(containerName,"labelRMultiplier");
	labelRMultiplier.init(xoffset+350, setupPos, GraphicListBox::defW, GraphicListBox::defH, true);
	//labelRMultiplier.setText(lang.get("RMultiplier"));

	labelFaction.registerGraphicComponent(containerName,"labelFaction");
    labelFaction.init(xoffset+430, setupPos, GraphicListBox::defW, GraphicListBox::defH, true);
    labelFaction.setText(lang.get("Faction"));

    labelTeam.registerGraphicComponent(containerName,"labelTeam");
    labelTeam.init(xoffset+590, setupPos, 50, GraphicListBox::defH, true);
    labelTeam.setText(lang.get("Team"));

    labelControl.setFont(CoreData::getInstance().getMenuFontBig());
    labelControl.setFont3D(CoreData::getInstance().getMenuFontBig3D());
    labelRMultiplier.setFont(CoreData::getInstance().getMenuFontBig());
    labelRMultiplier.setFont3D(CoreData::getInstance().getMenuFontBig3D());
	labelFaction.setFont(CoreData::getInstance().getMenuFontBig());
	labelFaction.setFont3D(CoreData::getInstance().getMenuFontBig3D());
	labelTeam.setFont(CoreData::getInstance().getMenuFontBig());
	labelTeam.setFont3D(CoreData::getInstance().getMenuFontBig3D());

	//texts
	buttonClearBlockedPlayers.setText(lang.get("BlockPlayerClear"));
	buttonReturn.setText(lang.get("Return"));
	buttonPlayNow.setText(lang.get("PlayNow"));
	buttonRestoreLastSettings.setText(lang.get("ReloadLastGameSettings"));

    controlItems.push_back(lang.get("Closed"));
	controlItems.push_back(lang.get("CpuEasy"));
	controlItems.push_back(lang.get("Cpu"));
    controlItems.push_back(lang.get("CpuUltra"));
    controlItems.push_back(lang.get("CpuMega"));
	controlItems.push_back(lang.get("Network"));
	controlItems.push_back(lang.get("NetworkUnassigned"));
	controlItems.push_back(lang.get("Human"));

	for(int i=0; i<45; ++i){
		rMultiplier.push_back(floatToStr(0.5f+0.1f*i,1));
	}

	if(config.getBool("EnableNetworkCpu","false") == true) {
		controlItems.push_back(lang.get("NetworkCpuEasy"));
		controlItems.push_back(lang.get("NetworkCpu"));
	    controlItems.push_back(lang.get("NetworkCpuUltra"));
	    controlItems.push_back(lang.get("NetworkCpuMega"));
	}

	for(int i = 1; i <= GameConstants::maxPlayers; ++i) {
		teamItems.push_back(intToStr(i));
	}
	for(int i = GameConstants::maxPlayers + 1; i <= GameConstants::maxPlayers + GameConstants::specialFactions; ++i) {
		teamItems.push_back(intToStr(i));
	}

	reloadFactions(false,"");

//    vector<string> techPaths = config.getPathListForType(ptTechs);
//    for(int idx = 0; idx < techPaths.size(); idx++) {
//        string &techPath = techPaths[idx];
//        endPathWithSlash(techPath);
//        //findAll(techPath + techTreeFiles[listBoxTechTree.getSelectedItemIndex()] + "/factions/*.", results, false, false);
//        findDirs(techPath + techTreeFiles[listBoxTechTree.getSelectedItemIndex()] + "/factions/", results, false, false);
//
//        if(results.empty() == false) {
//            break;
//        }
//    }

//    if(results.empty() == true) {
	if(factionFiles.empty() == true) {
        //throw megaglest_runtime_error("(1)There are no factions for the tech tree [" + techTreeFiles[listBoxTechTree.getSelectedItemIndex()] + "]");
		showGeneralError=true;
		generalErrorToShow = "[#1] There are no factions for the tech tree [" + techTreeFiles[listBoxTechTree.getSelectedItemIndex()] + "]";
    }

	for(int i=0; i < GameConstants::maxPlayers; ++i) {
		labelPlayerStatus[i].setText("");

		labelPlayers[i].setText(lang.get("Player")+" "+intToStr(i));
		labelPlayerNames[i].setText("*");
		labelPlayerNames[i].setMaxEditWidth(16);

        listBoxTeams[i].setItems(teamItems);
		listBoxTeams[i].setSelectedItemIndex(i);
		lastSelectedTeamIndex[i] = listBoxTeams[i].getSelectedItemIndex();

		listBoxControls[i].setItems(controlItems);
		listBoxRMultiplier[i].setItems(rMultiplier);
		listBoxRMultiplier[i].setSelectedItemIndex(5);
		labelNetStatus[i].setText("");
    }


    //labelEnableObserverMode.setText(lang.get("EnableObserverMode"));


	loadMapInfo(Map::getMapPath(getCurrentMapFile()), &mapInfo, true);

	labelMapInfo.setText(mapInfo.desc);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	//init controllers
	if(serverInitError == false) {
		ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();
		if(this->headlessServerMode == true) {
			listBoxControls[0].setSelectedItemIndex(ctNetwork);
		}
		else {
			listBoxControls[0].setSelectedItemIndex(ctHuman);
		}
		labelPlayerNames[0].setText("");
		labelPlayerNames[0].setText(getHumanPlayerName());

		if(openNetworkSlots){
			for(int i= 1; i<mapInfo.players; ++i){
				listBoxControls[i].setSelectedItemIndex(ctNetwork);
			}
		}
		else{
			listBoxControls[1].setSelectedItemIndex(ctCpu);
		}
		updateControlers();
		updateNetworkSlots();

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		// Ensure we have set the gamesettings at least once
		GameSettings gameSettings;
		loadGameSettings(&gameSettings);

		serverInterface->setGameSettings(&gameSettings,false);
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	updateAllResourceMultiplier();

	listBoxPlayerStatus.registerGraphicComponent(containerName,"listBoxPlayerStatus");
	listBoxPlayerStatus.init(10, 600, 150);
	vector<string> playerStatuses;
	playerStatuses.push_back(lang.get("PlayerStatusSetup"));
	playerStatuses.push_back(lang.get("PlayerStatusBeRightBack"));
	playerStatuses.push_back(lang.get("PlayerStatusReady"));
	listBoxPlayerStatus.setItems(playerStatuses);
	listBoxPlayerStatus.setSelectedItemIndex(2,true);
	listBoxPlayerStatus.setTextColor(Vec3f(0.0f,1.0f,0.0f));
	listBoxPlayerStatus.setLighted(false);
	listBoxPlayerStatus.setVisible(true);

    labelScenario.registerGraphicComponent(containerName,"labelScenario");
    labelScenario.init(320, 670);
    labelScenario.setText(lang.get("Scenario"));
	listBoxScenario.registerGraphicComponent(containerName,"listBoxScenario");
    listBoxScenario.init(320, 645);
    checkBoxScenario.registerGraphicComponent(containerName,"checkBoxScenario");
    checkBoxScenario.init(410, 670);
    checkBoxScenario.setValue(false);

    //scenario listbox
    vector<string> resultsScenarios;
	findDirs(dirList, resultsScenarios);
	// Filter out only scenarios with no network slots
	for(int i= 0; i < resultsScenarios.size(); ++i) {
		string scenario = resultsScenarios[i];
		string file = Scenario::getScenarioPath(dirList, scenario);

		try {
			if(file != "") {
				Scenario::loadScenarioInfo(file, &scenarioInfo);

				bool isNetworkScenario = false;
				for(unsigned int j = 0; isNetworkScenario == false && j < GameConstants::maxPlayers; ++j) {
					if(scenarioInfo.factionControls[j] == ctNetwork) {
						isNetworkScenario = true;
					}
				}
				if(isNetworkScenario == true) {
					scenarioFiles.push_back(scenario);
				}
			}
		}
		catch(const std::exception &ex) {
		    char szBuf[8096]="";
		    sprintf(szBuf,"In [%s::%s %d]\nError loading scenario [%s]:\n%s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,scenario.c_str(),ex.what());
		    SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);

		    showGeneralError=true;
			generalErrorToShow = szBuf;
		    //throw megaglest_runtime_error(szBuf);
		}
	}
	resultsScenarios.clear();
	for(int i = 0; i < scenarioFiles.size(); ++i) {
		resultsScenarios.push_back(formatString(scenarioFiles[i]));
	}
    listBoxScenario.setItems(resultsScenarios);
    if(resultsScenarios.empty() == true) {
    	checkBoxScenario.setEnabled(false);
    }

	// write hint to console:
	Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));

	console.addLine(lang.get("To switch off music press") + " - \"" + configKeys.getString("ToggleMusic") + "\"");

	chatManager.init(&console, -1,true);

	GraphicComponent::applyAllCustomProperties(containerName);

	publishToMasterserverThread = new SimpleTaskThread(this,0,200);
	publishToMasterserverThread->setUniqueID(__FILE__);
	publishToMasterserverThread->start();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	}
	catch(const std::exception &ex) {
	    char szBuf[8096]="";
	    sprintf(szBuf,"In [%s::%s %d]\nError detected:\n%s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
	    SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
	    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);

	    throw megaglest_runtime_error(szBuf);
	}
}

void MenuStateCustomGame::reloadUI() {
	Lang &lang= Lang::getInstance();
    Config &config = Config::getInstance();

    console.resetFonts();
	mainMessageBox.init(lang.get("Ok"));


	if(EndsWith(glestVersionString, "-dev") == false){
		labelLocalGameVersion.setText(glestVersionString);
	}
	else {
		labelLocalGameVersion.setText(glestVersionString + " [" + getCompileDateTime() + ", " + getSVNRevisionString() + "]");
	}

	vector<string> teamItems, controlItems, results , rMultiplier;

	string ipText = "none";
	std::vector<std::string> ipList = Socket::getLocalIPAddressList();
	if(ipList.empty() == false) {
		ipText = "";
		for(int idx = 0; idx < ipList.size(); idx++) {
			string ip = ipList[idx];
			if(ipText != "") {
				ipText += ", ";
			}
			ipText += ip;
		}
	}
	string externalPort=config.getString("MasterServerExternalPort", intToStr(GameConstants::serverPort).c_str());
	string serverPort=config.getString("ServerPort", intToStr(GameConstants::serverPort).c_str());
	labelLocalIP.setText(lang.get("LanIP") + ipText + "  ( "+serverPort+" / "+externalPort+" )");

	labelMap.setText(lang.get("Map")+":");

	labelMapFilter.setText(lang.get("MapFilter")+":");

	labelTileset.setText(lang.get("Tileset"));

	labelTechTree.setText(lang.get("TechTree"));

	labelFogOfWar.setText(lang.get("FogOfWar"));

	std::vector<std::string> listBoxData;
	listBoxData.push_back(lang.get("Enabled"));
	listBoxData.push_back(lang.get("Explored"));
	listBoxData.push_back(lang.get("Disabled"));
	listBoxFogOfWar.setItems(listBoxData);

	// Allow Observers
	labelAllowObservers.setText(lang.get("AllowObservers"));

	listBoxData.clear();
	listBoxData.push_back(lang.get("No"));
	listBoxData.push_back(lang.get("Yes"));
	listBoxAllowObservers.setItems(listBoxData);

	// View Map At End Of Game
	//listBoxData.clear();
	//listBoxData.push_back(lang.get("Yes"));
	//listBoxData.push_back(lang.get("No"));
	//listBoxEnableObserverMode.setItems(listBoxData);

	// Allow Switch Team Mode
	labelEnableSwitchTeamMode.setText(lang.get("EnableSwitchTeamMode"));

	listBoxData.clear();
	listBoxData.push_back(lang.get("Yes"));
	listBoxData.push_back(lang.get("No"));
	listBoxEnableSwitchTeamMode.setItems(listBoxData);

	labelAISwitchTeamAcceptPercent.setText(lang.get("AISwitchTeamAcceptPercent"));

	labelPathFinderType.setText(lang.get("PathFinderType"));

	listBoxData.clear();
	listBoxData.push_back(lang.get("PathFinderTypeRegular"));
	if(config.getBool("EnableRoutePlannerPathfinder","false") == true) {
		listBoxData.push_back(lang.get("PathFinderTypeRoutePlanner"));
	}
	listBoxPathFinderType.setItems(listBoxData);

	// Advanced Options
	labelAdvanced.setText(lang.get("AdvancedGameOptions"));

	listBoxData.clear();
	listBoxData.push_back(lang.get("No"));
	listBoxData.push_back(lang.get("Yes"));
	listBoxAdvanced.setItems(listBoxData);

	labelPublishServer.setText(lang.get("PublishServer"));

	listBoxData.clear();
	listBoxData.push_back(lang.get("Yes"));
	listBoxData.push_back(lang.get("No"));
	listBoxPublishServer.setItems(listBoxData);

	labelGameNameLabel.setText(lang.get("MGGameTitle")+":");

	labelGameName.setFont(CoreData::getInstance().getMenuFontBig());
	labelGameName.setFont3D(CoreData::getInstance().getMenuFontBig3D());
	if(this->headlessServerMode == false) {
		labelGameName.setText(defaultPlayerName+"'s game");
	}
	else {
		labelGameName.setText("headless ("+defaultPlayerName+")");
	}

	labelNetworkPauseGameForLaggedClients.setText(lang.get("NetworkPauseGameForLaggedClients"));

	listBoxData.clear();
	listBoxData.push_back(lang.get("No"));
	listBoxData.push_back(lang.get("Yes"));
	listBoxNetworkPauseGameForLaggedClients.setItems(listBoxData);

    for(int i=0; i < GameConstants::maxPlayers; ++i) {
        buttonBlockPlayers[i].setText(lang.get("BlockPlayer"));
    }

	labelControl.setText(lang.get("Control"));

    labelFaction.setText(lang.get("Faction"));

    labelTeam.setText(lang.get("Team"));

    labelControl.setFont(CoreData::getInstance().getMenuFontBig());
    labelControl.setFont3D(CoreData::getInstance().getMenuFontBig3D());
    labelRMultiplier.setFont(CoreData::getInstance().getMenuFontBig());
    labelRMultiplier.setFont3D(CoreData::getInstance().getMenuFontBig3D());
	labelFaction.setFont(CoreData::getInstance().getMenuFontBig());
	labelFaction.setFont3D(CoreData::getInstance().getMenuFontBig3D());
	labelTeam.setFont(CoreData::getInstance().getMenuFontBig());
	labelTeam.setFont3D(CoreData::getInstance().getMenuFontBig3D());

	//texts
	buttonClearBlockedPlayers.setText(lang.get("BlockPlayerClear"));
	buttonReturn.setText(lang.get("Return"));
	buttonPlayNow.setText(lang.get("PlayNow"));
	buttonRestoreLastSettings.setText(lang.get("ReloadLastGameSettings"));

    controlItems.push_back(lang.get("Closed"));
	controlItems.push_back(lang.get("CpuEasy"));
	controlItems.push_back(lang.get("Cpu"));
    controlItems.push_back(lang.get("CpuUltra"));
    controlItems.push_back(lang.get("CpuMega"));
	controlItems.push_back(lang.get("Network"));
	controlItems.push_back(lang.get("NetworkUnassigned"));
	controlItems.push_back(lang.get("Human"));

	if(config.getBool("EnableNetworkCpu","false") == true) {
		controlItems.push_back(lang.get("NetworkCpuEasy"));
		controlItems.push_back(lang.get("NetworkCpu"));
	    controlItems.push_back(lang.get("NetworkCpuUltra"));
	    controlItems.push_back(lang.get("NetworkCpuMega"));
	}

	for(int i=0; i < GameConstants::maxPlayers; ++i) {
		labelPlayers[i].setText(lang.get("Player")+" "+intToStr(i));

		listBoxControls[i].setItems(controlItems);
    }

    //labelEnableObserverMode.setText(lang.get("EnableObserverMode"));

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	vector<string> playerStatuses;
	playerStatuses.push_back(lang.get("PlayerStatusSetup"));
	playerStatuses.push_back(lang.get("PlayerStatusBeRightBack"));
	playerStatuses.push_back(lang.get("PlayerStatusReady"));
	listBoxPlayerStatus.setItems(playerStatuses);

	labelScenario.setText(lang.get("Scenario"));

	// write hint to console:
	Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));

	console.addLine(lang.get("To switch off music press") + " - \"" + configKeys.getString("ToggleMusic") + "\"");

	chatManager.init(&console, -1,true);

	GraphicComponent::reloadFontsForRegisterGraphicComponents(containerName);
}

void MenuStateCustomGame::cleanup() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
    if(publishToMasterserverThread != NULL) {
    	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

        needToBroadcastServerSettings = false;
        needToRepublishToMasterserver = false;
        lastNetworkPing               = time(NULL);
        publishToMasterserverThread->setThreadOwnerValid(false);

        if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

        if(forceWaitForShutdown == true) {
    		time_t elapsed = time(NULL);
    		publishToMasterserverThread->signalQuit();
    		for(;publishToMasterserverThread->canShutdown(false) == false &&
    			difftime(time(NULL),elapsed) <= 15;) {
    			//sleep(150);
    		}
    		if(publishToMasterserverThread->canShutdown(true)) {
    			delete publishToMasterserverThread;
    			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
    		}
    		else {
    			char szBuf[4096]="";
    			sprintf(szBuf,"In [%s::%s %d] Error cannot shutdown publishToMasterserverThread\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
    			//SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
    			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("%s",szBuf);
    			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);

    			publishToMasterserverThread->setOverrideShutdownTask(shutdownTaskStatic);
    			//publishToMasterserverThread->cleanup();
    		}
    		publishToMasterserverThread = NULL;
        }
        else {
        	publishToMasterserverThread->signalQuit();
        	sleep(0);
        	if(publishToMasterserverThread->canShutdown(true) == true &&
        		publishToMasterserverThread->shutdownAndWait() == true) {
        		delete publishToMasterserverThread;
        	}
    		else {
    			char szBuf[4096]="";
    			sprintf(szBuf,"In [%s::%s %d] Error cannot shutdown publishToMasterserverThread\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
    			//SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
    			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("%s",szBuf);
    			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);

    			publishToMasterserverThread->setOverrideShutdownTask(shutdownTaskStatic);
    			//publishToMasterserverThread->cleanup();
    		}
        }

        publishToMasterserverThread = NULL;
    }

    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	cleanupMapPreviewTexture();

	if(factionVideo != NULL) {
		factionVideo->closePlayer();
		delete factionVideo;
		factionVideo = NULL;
	}

	if(forceWaitForShutdown == true) {
		NetworkManager::getInstance().end();
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

MenuStateCustomGame::~MenuStateCustomGame() {
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

    cleanup();

    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

void MenuStateCustomGame::returnToParentMenu() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	needToBroadcastServerSettings = false;
	needToRepublishToMasterserver = false;
	lastNetworkPing               = time(NULL);
	ParentMenuState parentMenuState = this->parentMenuState;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	forceWaitForShutdown = false;
	if(parentMenuState==pMasterServer) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		cleanup();
		mainMenu->setState(new MenuStateMasterserver(program, mainMenu));
	}
	else if(parentMenuState==pLanGame) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		cleanup();
		mainMenu->setState(new MenuStateJoinGame(program, mainMenu));
	}
	else {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		cleanup();
		mainMenu->setState(new MenuStateNewGame(program, mainMenu));
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

void MenuStateCustomGame::mouseClick(int x, int y, MouseButton mouseButton) {
	if(isMasterserverMode() == true) {
		return;
	}

    try {
        CoreData &coreData= CoreData::getInstance();
        SoundRenderer &soundRenderer= SoundRenderer::getInstance();
        int oldListBoxMapfilterIndex=listBoxMapFilter.getSelectedItemIndex();

        if(mainMessageBox.getEnabled()){
            int button= 0;
            if(mainMessageBox.mouseClick(x, y, button))
            {
                soundRenderer.playFx(coreData.getClickSoundA());
                if(button==0)
                {
                    mainMessageBox.setEnabled(false);
                }
            }
        }
        else {
			if(activeInputLabel!=NULL && !(activeInputLabel->mouseClick(x,y))){
				setActiveInputLabel(NULL);
			}
			if(buttonReturn.mouseClick(x,y) || serverInitError == true) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

				soundRenderer.playFx(coreData.getClickSoundA());

				MutexSafeWrapper safeMutex((publishToMasterserverThread != NULL ? publishToMasterserverThread->getMutexThreadObjectAccessor() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
				needToBroadcastServerSettings = false;
				needToRepublishToMasterserver = false;
				lastNetworkPing               = time(NULL);
				safeMutex.ReleaseLock();

				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

				returnToParentMenu();
				return;
			}
			else if(buttonPlayNow.mouseClick(x,y) && buttonPlayNow.getEnabled()) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

				PlayNow(true);
				return;
			}
			else if(buttonRestoreLastSettings.mouseClick(x,y) && buttonRestoreLastSettings.getEnabled()) {
				RestoreLastGameSettings();
			}
			else if(listBoxMap.mouseClick(x, y)){
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s\n", getCurrentMapFile().c_str());

				MutexSafeWrapper safeMutex((publishToMasterserverThread != NULL ? publishToMasterserverThread->getMutexThreadObjectAccessor() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));

				loadMapInfo(Map::getMapPath(getCurrentMapFile(),"",false), &mapInfo, true);
				labelMapInfo.setText(mapInfo.desc);
				updateControlers();
				updateNetworkSlots();

				if(listBoxPublishServer.getSelectedItemIndex() == 0) {
					needToRepublishToMasterserver = true;
				}

				if(hasNetworkGameSettings() == true) {
					//delay publishing for 5 seconds
					needToPublishDelayed=true;
					mapPublishingDelayTimer=time(NULL);
				}
			}
			else if (listBoxAdvanced.getSelectedItemIndex() == 1 && listBoxFogOfWar.mouseClick(x, y)) {
				MutexSafeWrapper safeMutex((publishToMasterserverThread != NULL ? publishToMasterserverThread->getMutexThreadObjectAccessor() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));

				cleanupMapPreviewTexture();
				if(listBoxPublishServer.getSelectedItemIndex() == 0) {
					needToRepublishToMasterserver = true;
				}

				if(hasNetworkGameSettings() == true) {
					needToSetChangedGameSettings = true;
					lastSetChangedGameSettings   = time(NULL);
				}
			}
			else if (listBoxAdvanced.getSelectedItemIndex() == 1 && listBoxAllowObservers.mouseClick(x, y)) {
				MutexSafeWrapper safeMutex((publishToMasterserverThread != NULL ? publishToMasterserverThread->getMutexThreadObjectAccessor() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));

				if(listBoxPublishServer.getSelectedItemIndex() == 0) {
					needToRepublishToMasterserver = true;
				}

				reloadFactions(true,(checkBoxScenario.getValue() == true ? scenarioFiles[listBoxScenario.getSelectedItemIndex()] : ""));

				if(hasNetworkGameSettings() == true) {
					needToSetChangedGameSettings = true;
					lastSetChangedGameSettings   = time(NULL);
				}
			}
//			else if (listBoxAdvanced.getSelectedItemIndex() == 1 && listBoxEnableObserverMode.mouseClick(x, y)) {
//				MutexSafeWrapper safeMutex((publishToMasterserverThread != NULL ? publishToMasterserverThread->getMutexThreadObjectAccessor() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
//
//				if(listBoxPublishServer.getSelectedItemIndex() == 0) {
//					needToRepublishToMasterserver = true;
//				}
//
//				if(hasNetworkGameSettings() == true)
//				{
//					needToSetChangedGameSettings = true;
//					lastSetChangedGameSettings   = time(NULL);
//				}
//			}
			else if (listBoxAdvanced.getSelectedItemIndex() == 1 && listBoxEnableSwitchTeamMode.mouseClick(x, y)) {
				MutexSafeWrapper safeMutex((publishToMasterserverThread != NULL ? publishToMasterserverThread->getMutexThreadObjectAccessor() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));

				if(listBoxPublishServer.getSelectedItemIndex() == 0) {
					needToRepublishToMasterserver = true;
				}

				if(hasNetworkGameSettings() == true)
				{
					needToSetChangedGameSettings = true;
					lastSetChangedGameSettings   = time(NULL);
				}
			}
			else if (listBoxAdvanced.getSelectedItemIndex() == 1 && listBoxAISwitchTeamAcceptPercent.getEnabled() && listBoxAISwitchTeamAcceptPercent.mouseClick(x, y)) {
				MutexSafeWrapper safeMutex((publishToMasterserverThread != NULL ? publishToMasterserverThread->getMutexThreadObjectAccessor() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));

				if(listBoxPublishServer.getSelectedItemIndex() == 0) {
					needToRepublishToMasterserver = true;
				}

				if(hasNetworkGameSettings() == true)
				{
					needToSetChangedGameSettings = true;
					lastSetChangedGameSettings   = time(NULL);
				}
			}
			else if (listBoxAdvanced.getSelectedItemIndex() == 1 && listBoxPathFinderType.mouseClick(x, y)) {
				MutexSafeWrapper safeMutex((publishToMasterserverThread != NULL ? publishToMasterserverThread->getMutexThreadObjectAccessor() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));

				if(listBoxPublishServer.getSelectedItemIndex() == 0) {
					needToRepublishToMasterserver = true;
				}

				if(hasNetworkGameSettings() == true)
				{
					needToSetChangedGameSettings = true;
					lastSetChangedGameSettings   = time(NULL);
				}
			}
			else if (listBoxAdvanced.mouseClick(x, y)) {
				//TODO
			}
			else if(listBoxTileset.mouseClick(x, y)) {
				MutexSafeWrapper safeMutex((publishToMasterserverThread != NULL ? publishToMasterserverThread->getMutexThreadObjectAccessor() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));

				if(listBoxPublishServer.getSelectedItemIndex() == 0) {
					needToRepublishToMasterserver = true;
				}
				if(hasNetworkGameSettings() == true)
				{

					//delay publishing for 5 seconds
					needToPublishDelayed=true;
					mapPublishingDelayTimer=time(NULL);
				}
			}
			else if(listBoxMapFilter.mouseClick(x, y)){
				MutexSafeWrapper safeMutex((publishToMasterserverThread != NULL ? publishToMasterserverThread->getMutexThreadObjectAccessor() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
				switchToNextMapGroup(listBoxMapFilter.getSelectedItemIndex()-oldListBoxMapfilterIndex);

				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s\n", getCurrentMapFile().c_str());

				loadMapInfo(Map::getMapPath(getCurrentMapFile()), &mapInfo, true);
				labelMapInfo.setText(mapInfo.desc);
				updateControlers();
				updateNetworkSlots();

				if(listBoxPublishServer.getSelectedItemIndex() == 0) {
					needToRepublishToMasterserver = true;
				}

				if(hasNetworkGameSettings() == true)
				{
					needToSetChangedGameSettings = true;
					lastSetChangedGameSettings   = time(NULL);
				}
			}
			else if(listBoxTechTree.mouseClick(x, y)){
				reloadFactions(false,(checkBoxScenario.getValue() == true ? scenarioFiles[listBoxScenario.getSelectedItemIndex()] : ""));

				MutexSafeWrapper safeMutex((publishToMasterserverThread != NULL ? publishToMasterserverThread->getMutexThreadObjectAccessor() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));

				if(listBoxPublishServer.getSelectedItemIndex() == 0) {
					needToRepublishToMasterserver = true;
				}

				if(hasNetworkGameSettings() == true)
				{
					needToSetChangedGameSettings = true;
					lastSetChangedGameSettings   = time(NULL);
				}
			}
			else if(listBoxPublishServer.mouseClick(x, y) && listBoxPublishServer.getEditable()) {
				MutexSafeWrapper safeMutex((publishToMasterserverThread != NULL ? publishToMasterserverThread->getMutexThreadObjectAccessor() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
				needToRepublishToMasterserver = true;
				soundRenderer.playFx(coreData.getClickSoundC());

				ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();
				serverInterface->setPublishEnabled(listBoxPublishServer.getSelectedItemIndex() == 0);
			}
			else if(labelGameName.mouseClick(x, y) && listBoxPublishServer.getEditable()){
				setActiveInputLabel(&labelGameName);
			}
			else if(listBoxAdvanced.getSelectedItemIndex() == 1 && listBoxNetworkPauseGameForLaggedClients.mouseClick(x, y)){
				MutexSafeWrapper safeMutex((publishToMasterserverThread != NULL ? publishToMasterserverThread->getMutexThreadObjectAccessor() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));

				if(listBoxPublishServer.getSelectedItemIndex() == 0) {
					needToRepublishToMasterserver = true;
				}
				if(hasNetworkGameSettings() == true)
				{
					needToSetChangedGameSettings = true;
					lastSetChangedGameSettings   = time(NULL);
				}

				soundRenderer.playFx(coreData.getClickSoundC());
			}
		    else if(listBoxScenario.mouseClick(x, y) || checkBoxScenario.mouseClick(x,y)) {
		        processScenario();
			}
			else {
				for(int i = 0; i < mapInfo.players; ++i) {
					MutexSafeWrapper safeMutex((publishToMasterserverThread != NULL ? publishToMasterserverThread->getMutexThreadObjectAccessor() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));

					//if (listBoxAdvanced.getSelectedItemIndex() == 1) {
						// set multiplier
						if(listBoxRMultiplier[i].mouseClick(x, y)) {
						}
					//}

					//ensure thet only 1 human player is present
					ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();
					ConnectionSlot *slot = serverInterface->getSlot(i);

					bool checkControTypeClicked = false;
					int selectedControlItemIndex = listBoxControls[i].getSelectedItemIndex();
					if(selectedControlItemIndex != ctNetwork ||
						(selectedControlItemIndex == ctNetwork && (slot == NULL || slot->isConnected() == false))) {
						checkControTypeClicked = true;
					}

					//printf("checkControTypeClicked = %d selectedControlItemIndex = %d i = %d\n",checkControTypeClicked,selectedControlItemIndex,i);

					if(checkControTypeClicked == true && listBoxControls[i].mouseClick(x, y)) {
						if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
						//printf("listBoxControls[i].mouseClick(x, y) is TRUE i = %d newcontrol = %d\n",i,listBoxControls[i].getSelectedItemIndex());

						// Skip over networkunassigned
						if(listBoxControls[i].getSelectedItemIndex() == ctNetworkUnassigned &&
							selectedControlItemIndex != ctNetworkUnassigned) {
							listBoxControls[i].mouseClick(x, y);
						}

						//look for human players
						int humanIndex1= -1;
						int humanIndex2= -1;
						for(int j = 0; j < GameConstants::maxPlayers; ++j) {
							ControlType ct= static_cast<ControlType>(listBoxControls[j].getSelectedItemIndex());
							if(ct == ctHuman) {
								if(humanIndex1 == -1) {
									humanIndex1= j;
								}
								else {
									humanIndex2= j;
								}
							}
						}

						if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] humanIndex1 = %d, humanIndex2 = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,humanIndex1,humanIndex2);

						//no human
						if(humanIndex1 == -1 && humanIndex2 == -1) {
							listBoxControls[i].setSelectedItemIndex(ctHuman);
							if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] i = %d, labelPlayerNames[i].getText() [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,i,labelPlayerNames[i].getText().c_str());

							//printf("humanIndex1 = %d humanIndex2 = %d i = %d listBoxControls[i].getSelectedItemIndex() = %d\n",humanIndex1,humanIndex2,i,listBoxControls[i].getSelectedItemIndex());
						}
						//2 humans
						else if(humanIndex1 != -1 && humanIndex2 != -1) {
							int closeSlotIndex = (humanIndex1 == i ? humanIndex2: humanIndex1);
							int humanSlotIndex = (closeSlotIndex == humanIndex1 ? humanIndex2 : humanIndex1);

							string origPlayName = labelPlayerNames[closeSlotIndex].getText();

							if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] closeSlotIndex = %d, origPlayName [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,closeSlotIndex,origPlayName.c_str());
							//printf("humanIndex1 = %d humanIndex2 = %d i = %d closeSlotIndex = %d humanSlotIndex = %d\n",humanIndex1,humanIndex2,i,closeSlotIndex,humanSlotIndex);

							listBoxControls[closeSlotIndex].setSelectedItemIndex(ctClosed);
							labelPlayerNames[humanSlotIndex].setText((origPlayName != "" ? origPlayName : getHumanPlayerName()));
						}
						updateNetworkSlots();

						if(listBoxPublishServer.getSelectedItemIndex() == 0) {
							needToRepublishToMasterserver = true;
						}

						if(hasNetworkGameSettings() == true) {
							needToSetChangedGameSettings = true;
							lastSetChangedGameSettings   = time(NULL);
						}
						updateResourceMultiplier(i);
					}
					else if(buttonClearBlockedPlayers.mouseClick(x, y)) {
						ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();
						if(serverInterface != NULL) {
							ServerSocket *serverSocket = serverInterface->getServerSocket();
							if(serverSocket != NULL) {
								serverSocket->clearBlockedIPAddress();
							}
						}
					}
					else if(buttonBlockPlayers[i].mouseClick(x, y)) {
						ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();
						if(serverInterface != NULL) {
							if(serverInterface->getSlot(i) != NULL &&
							   serverInterface->getSlot(i)->isConnected()) {

								ServerSocket *serverSocket = serverInterface->getServerSocket();
								if(serverSocket != NULL) {
									serverSocket->addIPAddressToBlockedList(serverInterface->getSlot(i)->getIpAddress());

									Lang &lang= Lang::getInstance();
									const vector<string> languageList = serverInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
									for(unsigned int j = 0; j < languageList.size(); ++j) {
										char szMsg[1024]="";
										if(lang.hasString("BlockPlayerServerMsg",languageList[j]) == true) {
											sprintf(szMsg,lang.get("BlockPlayerServerMsg",languageList[j]).c_str(),serverInterface->getSlot(i)->getIpAddress().c_str());
										}
										else {
											sprintf(szMsg,"The server has temporarily blocked IP Address [%s] from this game.",serverInterface->getSlot(i)->getIpAddress().c_str());
										}

										serverInterface->sendTextMessage(szMsg,-1, true,languageList[j]);
									}
									sleep(1);
									serverInterface->getSlot(i)->close();
								}
							}
						}
					}
					else if(listBoxFactions[i].mouseClick(x, y)) {
						// Disallow CPU players to be observers
						if(factionFiles[listBoxFactions[i].getSelectedItemIndex()] == formatString(GameConstants::OBSERVER_SLOTNAME) &&
							(listBoxControls[i].getSelectedItemIndex() == ctCpuEasy || listBoxControls[i].getSelectedItemIndex() == ctCpu ||
							 listBoxControls[i].getSelectedItemIndex() == ctCpuUltra || listBoxControls[i].getSelectedItemIndex() == ctCpuMega)) {
							listBoxFactions[i].setSelectedItemIndex(0);
						}
						//

						if(listBoxPublishServer.getSelectedItemIndex() == 0) {
							needToRepublishToMasterserver = true;
						}

						if(hasNetworkGameSettings() == true)
						{
							needToSetChangedGameSettings = true;
							lastSetChangedGameSettings   = time(NULL);
						}
					}
					else if(listBoxTeams[i].mouseClick(x, y))
					{
						if(factionFiles[listBoxFactions[i].getSelectedItemIndex()] != formatString(GameConstants::OBSERVER_SLOTNAME)) {
							if(listBoxTeams[i].getSelectedItemIndex() + 1 != (GameConstants::maxPlayers + fpt_Observer)) {
								lastSelectedTeamIndex[i] = listBoxTeams[i].getSelectedItemIndex();
							}
						}
						else {
							lastSelectedTeamIndex[i] = -1;
						}

						if(listBoxPublishServer.getSelectedItemIndex() == 0) {
							needToRepublishToMasterserver = true;
						}

						if(hasNetworkGameSettings() == true)
						{
							needToSetChangedGameSettings = true;
							lastSetChangedGameSettings   = time(NULL);;
						}
					}
					else if(labelPlayerNames[i].mouseClick(x, y)) {
						ControlType ct= static_cast<ControlType>(listBoxControls[i].getSelectedItemIndex());
						if(ct == ctHuman) {
							setActiveInputLabel(&labelPlayerNames[i]);
							break;
						}
					}
				}
        }
        }

		if(hasNetworkGameSettings() == true && listBoxPlayerStatus.mouseClick(x,y)) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

			soundRenderer.playFx(coreData.getClickSoundC());
			if(getNetworkPlayerStatus()==npst_PickSettings)
			{
				listBoxPlayerStatus.setTextColor(Vec3f(1.0f,0.0f,0.0f));
				listBoxPlayerStatus.setLighted(true);
			}
			else if(getNetworkPlayerStatus()==npst_BeRightBack)
			{
				listBoxPlayerStatus.setTextColor(Vec3f(1.0f,1.0f,0.0f));
				listBoxPlayerStatus.setLighted(true);
			}
			else if(getNetworkPlayerStatus()==npst_Ready)
			{
				listBoxPlayerStatus.setTextColor(Vec3f(0.0f,1.0f,0.0f));
				listBoxPlayerStatus.setLighted(false);
			}

			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

            if(listBoxPublishServer.getSelectedItemIndex() == 0) {
                needToRepublishToMasterserver = true;
            }

            if(hasNetworkGameSettings() == true) {
                needToSetChangedGameSettings = true;
                lastSetChangedGameSettings   = time(NULL);
            }
		}
    }
	catch(const std::exception &ex) {
		char szBuf[8096]="";
		sprintf(szBuf,"In [%s::%s %d]\nError detected:\n%s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);

		showGeneralError=true;
		generalErrorToShow = szBuf;
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

void MenuStateCustomGame::updateAllResourceMultiplier() {
	for(int j=0; j<GameConstants::maxPlayers; ++j) {
		updateResourceMultiplier(j);
	}
}

void MenuStateCustomGame::updateResourceMultiplier(const int index) {
		ControlType ct= static_cast<ControlType>(listBoxControls[index].getSelectedItemIndex());
		if(ct == ctHuman || ct == ctNetwork || ct == ctClosed) {
			listBoxRMultiplier[index].setSelectedItemIndex((GameConstants::normalMultiplier-0.5f)*10);
			listBoxRMultiplier[index].setEnabled(false);
		}
		else if(ct == ctCpuEasy || ct == ctNetworkCpuEasy)
		{
			listBoxRMultiplier[index].setSelectedItemIndex((GameConstants::easyMultiplier-0.5f)*10);
			listBoxRMultiplier[index].setEnabled(checkBoxScenario.getValue() == false);
		}
		else if(ct == ctCpu || ct == ctNetworkCpu) {
			listBoxRMultiplier[index].setSelectedItemIndex((GameConstants::normalMultiplier-0.5f)*10);
			listBoxRMultiplier[index].setEnabled(checkBoxScenario.getValue() == false);
		}
		else if(ct == ctCpuUltra || ct == ctNetworkCpuUltra)
		{
			listBoxRMultiplier[index].setSelectedItemIndex((GameConstants::ultraMultiplier-0.5f)*10);
			listBoxRMultiplier[index].setEnabled(checkBoxScenario.getValue() == false);
		}
		else if(ct == ctCpuMega || ct == ctNetworkCpuMega)
		{
			listBoxRMultiplier[index].setSelectedItemIndex((GameConstants::megaMultiplier-0.5f)*10);
			listBoxRMultiplier[index].setEnabled(checkBoxScenario.getValue() == false);
		}
		listBoxRMultiplier[index].setEditable(listBoxRMultiplier[index].getEnabled());
		listBoxRMultiplier[index].setVisible(ct != ctHuman && ct != ctNetwork && ct != ctClosed);
}

void MenuStateCustomGame::RestoreLastGameSettings() {
	// Ensure we have set the gamesettings at least once
	GameSettings gameSettings = loadGameSettingsFromFile(SAVED_GAME_FILENAME);
	if(gameSettings.getMap() == "") {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		loadGameSettings(&gameSettings);
	}

	ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();
	serverInterface->setGameSettings(&gameSettings,false);

	MutexSafeWrapper safeMutex((publishToMasterserverThread != NULL ? publishToMasterserverThread->getMutexThreadObjectAccessor() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));

	if(listBoxPublishServer.getSelectedItemIndex() == 0) {
		needToRepublishToMasterserver = true;
	}

	if(hasNetworkGameSettings() == true) {
		needToSetChangedGameSettings = true;
		lastSetChangedGameSettings   = time(NULL);
	}
}

void MenuStateCustomGame::PlayNow(bool saveGame) {
	MutexSafeWrapper safeMutex((publishToMasterserverThread != NULL ? publishToMasterserverThread->getMutexThreadObjectAccessor() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
	if(saveGame == true) {
		saveGameSettingsToFile(SAVED_GAME_FILENAME);
	}

	forceWaitForShutdown = false;
	closeUnusedSlots();
	CoreData &coreData= CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();
	soundRenderer.playFx(coreData.getClickSoundC());

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	std::vector<string> randomFactionSelectionList;
	int RandomCount = 0;
	for(int i= 0; i < mapInfo.players; ++i) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		// Check for random faction selection and choose the faction now
		if(listBoxControls[i].getSelectedItemIndex() != ctClosed) {
			if(listBoxFactions[i].getSelectedItem() == formatString(GameConstants::RANDOMFACTION_SLOTNAME)) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] i = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,i);

				// Max 1000 tries to get a random, unused faction
				for(int findRandomFaction = 1; findRandomFaction < 1000; ++findRandomFaction) {
					srand(time(NULL) + findRandomFaction);
					int selectedFactionIndex = rand() % listBoxFactions[i].getItemCount();
					string selectedFactionName = listBoxFactions[i].getItem(selectedFactionIndex);

					if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] selectedFactionName [%s] selectedFactionIndex = %d, findRandomFaction = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,selectedFactionName.c_str(),selectedFactionIndex,findRandomFaction);

					if(	selectedFactionName != formatString(GameConstants::RANDOMFACTION_SLOTNAME) &&
						selectedFactionName != formatString(GameConstants::OBSERVER_SLOTNAME) &&
						std::find(randomFactionSelectionList.begin(),randomFactionSelectionList.end(),selectedFactionName) == randomFactionSelectionList.end()) {

						if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
						listBoxFactions[i].setSelectedItem(selectedFactionName);
						randomFactionSelectionList.push_back(selectedFactionName);
						break;
					}
				}

				if(listBoxFactions[i].getSelectedItem() == formatString(GameConstants::RANDOMFACTION_SLOTNAME)) {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] RandomCount = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,RandomCount);

					// Find first real faction and use it
					int factionIndexToUse = RandomCount;
					for(int useIdx = 0; useIdx < listBoxFactions[i].getItemCount(); useIdx++) {
						string selectedFactionName = listBoxFactions[i].getItem(useIdx);
						if(	selectedFactionName != formatString(GameConstants::RANDOMFACTION_SLOTNAME) &&
							selectedFactionName != formatString(GameConstants::OBSERVER_SLOTNAME)) {
							factionIndexToUse = useIdx;
							break;
						}
					}
					listBoxFactions[i].setSelectedItemIndex(factionIndexToUse);
					randomFactionSelectionList.push_back(listBoxFactions[i].getItem(factionIndexToUse));
				}

				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] i = %d, listBoxFactions[i].getSelectedItem() [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,i,listBoxFactions[i].getSelectedItem().c_str());

				RandomCount++;
			}
		}
	}

	if(RandomCount > 0) {
		needToSetChangedGameSettings = true;
	}

	safeMutex.ReleaseLock(true);
	GameSettings gameSettings;
	loadGameSettings(&gameSettings, true);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();

	// Send the game settings to each client if we have at least one networked client
	safeMutex.Lock();

	bool dataSynchCheckOk = true;
	for(int i= 0; i < mapInfo.players; ++i) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		if(listBoxControls[i].getSelectedItemIndex() == ctNetwork) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

			if(	serverInterface->getSlot(i) != NULL && serverInterface->getSlot(i)->isConnected() &&
				(serverInterface->getSlot(i)->getAllowDownloadDataSynch() == true || serverInterface->getSlot(i)->getAllowGameDataSynchCheck() == true) &&
				serverInterface->getSlot(i)->getNetworkGameDataSynchCheckOk() == false) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
				dataSynchCheckOk = false;
				break;
			}
		}
	}

	// Ensure we have no dangling network players
	for(int i= 0; i < GameConstants::maxPlayers; ++i) {
		if(listBoxControls[i].getSelectedItemIndex() == ctNetworkUnassigned) {
			mainMessageBoxState=1;

			Lang &lang= Lang::getInstance();
			char szMsg[1024]="";
			if(lang.hasString("NetworkSlotUnassignedErrorUI") == true) {
				//sprintf(szMsg,lang.get("NetworkSlotUnassignedErrorUI").c_str());
				strcpy(szMsg,lang.get("NetworkSlotUnassignedErrorUI").c_str());
			}
			else {
				//sprintf(szMsg,"Cannot start game.\nSome player(s) are not in a network game slot!");
				strcpy(szMsg,"Cannot start game.\nSome player(s) are not in a network game slot!");
			}

			showMessageBox(szMsg, "", false);

	    	const vector<string> languageList = serverInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
	    	for(unsigned int j = 0; j < languageList.size(); ++j) {
				char szMsg[1024]="";
				if(lang.hasString("NetworkSlotUnassignedError",languageList[j]) == true) {
					//sprintf(szMsg,lang.get("NetworkSlotUnassignedError").c_str());
					strcpy(szMsg,lang.get("NetworkSlotUnassignedError").c_str());
				}
				else {
					//sprintf(szMsg,"Cannot start game, some player(s) are not in a network game slot!");
					strcpy(szMsg,"Cannot start game, some player(s) are not in a network game slot!");
				}

				serverInterface->sendTextMessage(szMsg,-1, true,languageList[j]);
	    	}

			safeMutex.ReleaseLock();
			return;
		}
	}

	if(dataSynchCheckOk == false) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		mainMessageBoxState=1;
		showMessageBox("You cannot start the game because\none or more clients do not have the same game data!", "Data Mismatch Error", false);

		safeMutex.ReleaseLock();
		return;
	}
	else {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		if( (hasNetworkGameSettings() == true &&
			 needToSetChangedGameSettings == true) || (RandomCount > 0)) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
			serverInterface->setGameSettings(&gameSettings,true);
			serverInterface->broadcastGameSetup(&gameSettings);

			needToSetChangedGameSettings    = false;
			lastSetChangedGameSettings      = time(NULL);
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		// Last check, stop human player from being in same slot as network
		if(isMasterserverMode() == false) {
			bool hasHuman = false;
			for(int i= 0; i < mapInfo.players; ++i) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

				// Check for random faction selection and choose the faction now
				if(listBoxControls[i].getSelectedItemIndex() == ctHuman) {
					hasHuman = true;
					break;
				}
			}
			if(hasHuman == false) {
				mainMessageBoxState=1;

				Lang &lang= Lang::getInstance();
				char szMsg[1024]="";
				strcpy(szMsg,lang.get("NetworkSlotNoHumanErrorUI","",true).c_str());
				showMessageBox(szMsg, "", false);

		    	const vector<string> languageList = serverInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
		    	for(unsigned int j = 0; j < languageList.size(); ++j) {
					char szMsg[1024]="";
					strcpy(szMsg,lang.get("NetworkSlotNoHumanError","",true).c_str());

					serverInterface->sendTextMessage(szMsg,-1, true,languageList[j]);
		    	}

				safeMutex.ReleaseLock();
				return;
			}
		}

		// Tell the server Interface whether or not to publish game status updates to masterserver
		serverInterface->setNeedToRepublishToMasterserver(listBoxPublishServer.getSelectedItemIndex() == 0);

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		bool bOkToStart = serverInterface->launchGame(&gameSettings);
		if(bOkToStart == true) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

			if( listBoxPublishServer.getEditable() &&
				listBoxPublishServer.getSelectedItemIndex() == 0) {

				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

				needToRepublishToMasterserver = true;
				lastMasterserverPublishing = 0;

				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
			}
			needToBroadcastServerSettings = false;
			needToRepublishToMasterserver = false;
			lastNetworkPing               = time(NULL);
			safeMutex.ReleaseLock();

			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

			assert(program != NULL);

			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
			cleanup();
			Game *newGame = new Game(program, &gameSettings, this->headlessServerMode);
			forceWaitForShutdown = false;
			program->setState(newGame);
			return;
		}
		else {
			safeMutex.ReleaseLock();
		}
	}
}

void MenuStateCustomGame::mouseMove(int x, int y, const MouseState *ms) {
	if(isMasterserverMode() == true) {
		return;
	}

	if (mainMessageBox.getEnabled()) {
		mainMessageBox.mouseMove(x, y);
	}
	buttonReturn.mouseMove(x, y);
	buttonPlayNow.mouseMove(x, y);
	buttonRestoreLastSettings.mouseMove(x, y);
	buttonClearBlockedPlayers.mouseMove(x, y);

	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		listBoxRMultiplier[i].mouseMove(x, y);
        listBoxControls[i].mouseMove(x, y);
        buttonBlockPlayers[i].mouseMove(x, y);
        listBoxFactions[i].mouseMove(x, y);
		listBoxTeams[i].mouseMove(x, y);
    }

	listBoxMap.mouseMove(x, y);
	if(listBoxAdvanced.getSelectedItemIndex() == 1) {
		listBoxFogOfWar.mouseMove(x, y);
		listBoxAllowObservers.mouseMove(x, y);
		//listBoxEnableObserverMode.mouseMove(x, y);
		//listBoxEnableServerControlledAI.mouseMove(x, y);
		//labelNetworkFramePeriod.mouseMove(x, y);
		//listBoxNetworkFramePeriod.mouseMove(x, y);

		listBoxEnableSwitchTeamMode.mouseMove(x, y);
		listBoxAISwitchTeamAcceptPercent.mouseMove(x, y);

		labelNetworkPauseGameForLaggedClients.mouseMove(x, y);
		listBoxNetworkPauseGameForLaggedClients.mouseMove(x, y);

		labelPathFinderType.mouseMove(x, y);
		listBoxPathFinderType.mouseMove(x, y);
	}
	listBoxTileset.mouseMove(x, y);
	listBoxMapFilter.mouseMove(x, y);
	listBoxTechTree.mouseMove(x, y);
	listBoxPublishServer.mouseMove(x, y);

	listBoxAdvanced.mouseMove(x, y);

	checkBoxScenario.mouseMove(x, y);
	listBoxScenario.mouseMove(x, y);
}

bool MenuStateCustomGame::isMasterserverMode() const {
	return (this->headlessServerMode == true && this->masterserverModeMinimalResources == true);
	//return false;
}

bool MenuStateCustomGame::isVideoPlaying() {
	bool result = false;
	if(factionVideo != NULL) {
		result = factionVideo->isPlaying();
	}
	return result;
}

void MenuStateCustomGame::render() {
	try {
		Renderer &renderer= Renderer::getInstance();

		if(factionTexture != NULL) {
			renderer.renderTextureQuad(800,600,200,150,factionTexture,0.7f);
		}
		if(factionVideo != NULL) {
			if(factionVideo->isPlaying() == true) {
				factionVideo->playFrame(false);
			}
		}
		if(mapPreviewTexture != NULL) {
			renderer.renderTextureQuad(5,185,150,150,mapPreviewTexture,1.0f);
			//printf("=================> Rendering map preview texture\n");
		}

		if(scenarioLogoTexture != NULL) {
			renderer.renderTextureQuad(300,350,400,300,scenarioLogoTexture,1.0f);
			//renderer.renderBackground(scenarioLogoTexture);
		}

		if(mainMessageBox.getEnabled()) {
			renderer.renderMessageBox(&mainMessageBox);
		}
		else {
			renderer.renderButton(&buttonReturn);
			renderer.renderButton(&buttonPlayNow);
			renderer.renderButton(&buttonRestoreLastSettings);

			// Get a reference to the player texture cache
			std::map<int,Texture2D *> &crcPlayerTextureCache = CacheManager::getCachedItem< std::map<int,Texture2D *> >(GameConstants::playerTextureCacheLookupKey);

			// START - this code ensure player title and player names don't overlap
			int offsetPosition=0;
		    for(int i=0; i < GameConstants::maxPlayers; ++i) {
				//const Metrics &metrics= Metrics::getInstance();
				FontMetrics *fontMetrics= NULL;
				if(Renderer::renderText3DEnabled == false) {
					fontMetrics = labelPlayers[i].getFont()->getMetrics();
				}
				else {
					fontMetrics = labelPlayers[i].getFont3D()->getMetrics();
				}
				if(fontMetrics == NULL) {
					throw megaglest_runtime_error("fontMetrics == NULL");
				}
				//int curWidth = (metrics.toVirtualX(fontMetrics->getTextWidth(labelPlayers[i].getText())));
				int curWidth = (fontMetrics->getTextWidth(labelPlayers[i].getText()));
				int newOffsetPosition = labelPlayers[i].getX() + curWidth + 2;

				//printf("labelPlayers[i].getX() = %d curWidth = %d labelPlayerNames[i].getX() = %d offsetPosition = %d newOffsetPosition = %d [%s]\n",labelPlayers[i].getX(),curWidth,labelPlayerNames[i].getX(),offsetPosition,newOffsetPosition,labelPlayers[i].getText().c_str());

				if(labelPlayers[i].getX() + curWidth >= labelPlayerNames[i].getX()) {
					if(offsetPosition < newOffsetPosition) {
						offsetPosition = newOffsetPosition;
					}
				}
		    }
		    // END

		    if(hasNetworkGameSettings() == true) {
		    	renderer.renderListBox(&listBoxPlayerStatus);
		    }

			ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();
			if( serverInterface != NULL &&
				serverInterface->getServerSocket() != NULL &&
				serverInterface->getServerSocket()->hasBlockedIPAddresses() == true) {
				renderer.renderButton(&buttonClearBlockedPlayers);
			}
			for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		    	if(listBoxControls[i].getSelectedItemIndex() == ctNetworkUnassigned) {
		    		//printf("Player #%d [%s] control = %d\n",i,labelPlayerNames[i].getText().c_str(),listBoxControls[i].getSelectedItemIndex());

					labelPlayers[i].setVisible(true);
					labelPlayerNames[i].setVisible(true);
					listBoxControls[i].setVisible(true);
					listBoxFactions[i].setVisible(true);
					listBoxTeams[i].setVisible(true);
					labelNetStatus[i].setVisible(true);
		    	}

				if( hasNetworkGameSettings() == true &&
					listBoxControls[i].getSelectedItemIndex() != ctClosed) {

					renderer.renderLabel(&labelPlayerStatus[i]);

					if(listBoxControls[i].getSelectedItemIndex() == ctNetwork || listBoxControls[i].getSelectedItemIndex() == ctNetworkUnassigned) {
						ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();
						if( serverInterface != NULL &&
							serverInterface->getSlot(i) != NULL &&
		                    serverInterface->getSlot(i)->isConnected()) {
							renderer.renderButton(&buttonBlockPlayers[i]);
						}
					}
				}

				if(crcPlayerTextureCache[i] != NULL) {
					// Render the player # label the player's color
					Vec3f playerColor = crcPlayerTextureCache[i]->getPixmap()->getPixel3f(0, 0);
					renderer.renderLabel(&labelPlayers[i],&playerColor);

					// Blend the color with white so make it more readable
					//Vec4f newColor(1.f, 1.f, 1.f, 0.57f);
					//renderer.renderLabel(&labelPlayers[i],&newColor);

					//int quadWidth = labelPlayerNames[i].getX() - labelPlayers[i].getX() - 5;
					//renderer.renderTextureQuad(labelPlayers[i].getX(), labelPlayers[i].getY(), quadWidth, labelPlayers[i].getH(), crcPlayerTextureCache[i],1.0f,&playerColor);
				}
				else {
					renderer.renderLabel(&labelPlayers[i]);
				}

				if(offsetPosition > 0) {
					labelPlayerNames[i].setX(offsetPosition);
				}
				renderer.renderLabel(&labelPlayerNames[i]);

				renderer.renderListBox(&listBoxControls[i]);

				if(listBoxControls[i].getSelectedItemIndex()!=ctClosed){
					//if(listBoxAdvanced.getSelectedItemIndex() == 1){
						renderer.renderListBox(&listBoxRMultiplier[i]);
					//}
					renderer.renderListBox(&listBoxFactions[i]);
					renderer.renderListBox(&listBoxTeams[i]);
					renderer.renderLabel(&labelNetStatus[i]);
				}
			}

			renderer.renderLabel(&labelLocalGameVersion);
			renderer.renderLabel(&labelLocalIP);
			renderer.renderLabel(&labelMap);

			if(listBoxAdvanced.getSelectedItemIndex() == 1) {
				renderer.renderLabel(&labelFogOfWar);
				renderer.renderLabel(&labelAllowObservers);
				//renderer.renderLabel(&labelEnableObserverMode);
				renderer.renderLabel(&labelPathFinderType);

				renderer.renderLabel(&labelEnableSwitchTeamMode);
				renderer.renderLabel(&labelAISwitchTeamAcceptPercent);

				renderer.renderListBox(&listBoxFogOfWar);
				renderer.renderListBox(&listBoxAllowObservers);
				//renderer.renderListBox(&listBoxEnableObserverMode);
				renderer.renderListBox(&listBoxPathFinderType);

				renderer.renderListBox(&listBoxEnableSwitchTeamMode);
				renderer.renderListBox(&listBoxAISwitchTeamAcceptPercent);
			}
			renderer.renderLabel(&labelTileset);
			renderer.renderLabel(&labelMapFilter);
			renderer.renderLabel(&labelTechTree);
			renderer.renderLabel(&labelControl);
			//renderer.renderLabel(&labelRMultiplier);
			renderer.renderLabel(&labelFaction);
			renderer.renderLabel(&labelTeam);
			renderer.renderLabel(&labelMapInfo);
			renderer.renderLabel(&labelAdvanced);

			renderer.renderListBox(&listBoxMap);
			renderer.renderListBox(&listBoxTileset);
			renderer.renderListBox(&listBoxMapFilter);
			renderer.renderListBox(&listBoxTechTree);
			renderer.renderListBox(&listBoxAdvanced);

			if(listBoxPublishServer.getEditable())
			{
				renderer.renderListBox(&listBoxPublishServer);
				renderer.renderLabel(&labelPublishServer);
				renderer.renderLabel(&labelGameName);
				renderer.renderLabel(&labelGameNameLabel);
				if(listBoxAdvanced.getSelectedItemIndex() == 1) {
					//renderer.renderListBox(&listBoxEnableServerControlledAI);
					//renderer.renderLabel(&labelEnableServerControlledAI);
					//renderer.renderLabel(&labelNetworkFramePeriod);
					//renderer.renderListBox(&listBoxNetworkFramePeriod);
					renderer.renderLabel(&labelNetworkPauseGameForLaggedClients);
					renderer.renderListBox(&listBoxNetworkPauseGameForLaggedClients);
				}
			}
		}

		//renderer.renderLabel(&labelInfo);
		renderer.renderCheckBox(&checkBoxScenario);
		renderer.renderLabel(&labelScenario);
		if(checkBoxScenario.getValue() == true) {
			renderer.renderListBox(&listBoxScenario);
		}

		if(program != NULL) program->renderProgramMsgBox();

		if( enableMapPreview == true &&
			mapPreview.hasFileLoaded() == true) {

		    if(mapPreviewTexture == NULL) {
		    	bool renderAll = (listBoxFogOfWar.getSelectedItemIndex() == 2);
		    	//printf("=================> Rendering map preview into a texture BEFORE (%p)\n", mapPreviewTexture);
		    	renderer.renderMapPreview(&mapPreview, renderAll, 10, 350,&mapPreviewTexture);
		    	//printf("=================> Rendering map preview into a texture AFTER (%p)\n", mapPreviewTexture);
		    }
		}

		if(hasNetworkGameSettings() == true) {
			renderer.renderChatManager(&chatManager);
		}
		renderer.renderConsole(&console,showFullConsole,true);
	}
	catch(const std::exception &ex) {
		char szBuf[8096]="";
		sprintf(szBuf,"In [%s::%s %d]\nError detected:\n%s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
		//throw megaglest_runtime_error(szBuf);

		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);
		showGeneralError=true;
		generalErrorToShow = szBuf;
	}
}

void MenuStateCustomGame::switchSetupForSlots(SwitchSetupRequest **switchSetupRequests,
		ServerInterface *& serverInterface, int startIndex, int endIndex, bool onlyNetworkUnassigned) {
    for(int i= startIndex; i < endIndex; ++i) {
		if(switchSetupRequests[i] != NULL) {
			//printf("Switch slot = %d control = %d newIndex = %d currentindex = %d onlyNetworkUnassigned = %d\n",i,listBoxControls[i].getSelectedItemIndex(),switchSetupRequests[i]->getToFactionIndex(),switchSetupRequests[i]->getCurrentFactionIndex(),onlyNetworkUnassigned);

			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] switchSetupRequests[i]->getSwitchFlags() = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,switchSetupRequests[i]->getSwitchFlags());

			if(onlyNetworkUnassigned == true && listBoxControls[i].getSelectedItemIndex() != ctNetworkUnassigned) {
				if(i < mapInfo.players || (i >= mapInfo.players && listBoxControls[i].getSelectedItemIndex() != ctNetwork)) {
					continue;
				}
			}

			if(listBoxControls[i].getSelectedItemIndex() == ctNetwork || listBoxControls[i].getSelectedItemIndex() == ctNetworkUnassigned) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] switchSetupRequests[i]->getToFactionIndex() = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,switchSetupRequests[i]->getToFactionIndex());

				if(switchSetupRequests[i]->getToFactionIndex() != -1) {
					int newFactionIdx = switchSetupRequests[i]->getToFactionIndex();

					//printf("switchSlot request from %d to %d\n",switchSetupRequests[i]->getCurrentFactionIndex(),switchSetupRequests[i]->getToFactionIndex());
					int switchFactionIdx = switchSetupRequests[i]->getCurrentFactionIndex();
					if(serverInterface->switchSlot(switchFactionIdx,newFactionIdx)) {
						try {
							if(switchSetupRequests[i]->getSelectedFactionName() != "") {
								listBoxFactions[newFactionIdx].setSelectedItem(switchSetupRequests[i]->getSelectedFactionName());
							}
							if(switchSetupRequests[i]->getToTeam() != -1) {
								listBoxTeams[newFactionIdx].setSelectedItemIndex(switchSetupRequests[i]->getToTeam());
							}
							if(switchSetupRequests[i]->getNetworkPlayerName() != "") {
								if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] i = %d, labelPlayerNames[newFactionIdx].getText() [%s] switchSetupRequests[i]->getNetworkPlayerName() [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,i,labelPlayerNames[newFactionIdx].getText().c_str(),switchSetupRequests[i]->getNetworkPlayerName().c_str());
								labelPlayerNames[newFactionIdx].setText(switchSetupRequests[i]->getNetworkPlayerName());
							}

							if(listBoxControls[switchFactionIdx].getSelectedItemIndex() == ctNetworkUnassigned) {
								serverInterface->removeSlot(switchFactionIdx);
								listBoxControls[switchFactionIdx].setSelectedItemIndex(ctClosed);

								labelPlayers[switchFactionIdx].setVisible(switchFactionIdx+1 <= mapInfo.players);
								labelPlayerNames[switchFactionIdx].setVisible(switchFactionIdx+1 <= mapInfo.players);
						        listBoxControls[switchFactionIdx].setVisible(switchFactionIdx+1 <= mapInfo.players);
						        listBoxFactions[switchFactionIdx].setVisible(switchFactionIdx+1 <= mapInfo.players);
								listBoxTeams[switchFactionIdx].setVisible(switchFactionIdx+1 <= mapInfo.players);
								labelNetStatus[switchFactionIdx].setVisible(switchFactionIdx+1 <= mapInfo.players);
							}
						}
						catch(const runtime_error &e) {
							SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,e.what());
							if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] caught exception error = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,e.what());
						}
					}
				}
				else {
					try {
						if(switchSetupRequests[i]->getSelectedFactionName() != "") {
							listBoxFactions[i].setSelectedItem(switchSetupRequests[i]->getSelectedFactionName());
						}
						if(switchSetupRequests[i]->getToTeam() != -1) {
							listBoxTeams[i].setSelectedItemIndex(switchSetupRequests[i]->getToTeam());
						}

						if((switchSetupRequests[i]->getSwitchFlags() & ssrft_NetworkPlayerName) == ssrft_NetworkPlayerName) {
							if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] i = %d, switchSetupRequests[i]->getSwitchFlags() = %d, switchSetupRequests[i]->getNetworkPlayerName() [%s], labelPlayerNames[i].getText() [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,i,switchSetupRequests[i]->getSwitchFlags(),switchSetupRequests[i]->getNetworkPlayerName().c_str(),labelPlayerNames[i].getText().c_str());

							if(switchSetupRequests[i]->getNetworkPlayerName() != GameConstants::NETWORK_SLOT_UNCONNECTED_SLOTNAME) {
								labelPlayerNames[i].setText(switchSetupRequests[i]->getNetworkPlayerName());
							}
							else {
								labelPlayerNames[i].setText("");
							}
							//SetActivePlayerNameEditor();
							//switchSetupRequests[i]->clearSwitchFlag(ssrft_NetworkPlayerName);
						}
					}
					catch(const runtime_error &e) {
						SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,e.what());
						if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] caught exception error = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,e.what());
					}
				}
			}

			delete switchSetupRequests[i];
			switchSetupRequests[i]=NULL;
		}
	}
}

void MenuStateCustomGame::update() {
	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

	// Test openal buffer underrun issue
	//sleep(200);
	// END

	MutexSafeWrapper safeMutex((publishToMasterserverThread != NULL ? publishToMasterserverThread->getMutexThreadObjectAccessor() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));

	try {
		if(serverInitError == true) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

			if(showGeneralError) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);


				showGeneralError=false;
				mainMessageBoxState=1;
				showMessageBox( generalErrorToShow, "Error", false);
			}

			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

			if(this->headlessServerMode == false) {
				return;
			}
		}
		ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();
		Lang& lang= Lang::getInstance();

		if(this->autoloadScenarioName != "") {
			listBoxScenario.setSelectedItem(formatString(this->autoloadScenarioName),false);

			if(listBoxScenario.getSelectedItem() != formatString(this->autoloadScenarioName)) {
				mainMessageBoxState=1;
				showMessageBox( "Could not find scenario name: " + formatString(this->autoloadScenarioName), "Scenario Missing", false);
				this->autoloadScenarioName = "";
			}
			else {
				loadScenarioInfo(Scenario::getScenarioPath(dirList, scenarioFiles[listBoxScenario.getSelectedItemIndex()]), &scenarioInfo);
				//labelInfo.setText(scenarioInfo.desc);

				SoundRenderer &soundRenderer= SoundRenderer::getInstance();
				CoreData &coreData= CoreData::getInstance();
				soundRenderer.playFx(coreData.getClickSoundC());
				//launchGame();
				PlayNow(true);
				return;
			}
		}

		if(needToLoadTextures) {
			// this delay is done to make it possible to switch faster
			if(difftime(time(NULL), previewLoadDelayTimer) >= 2){
				//loadScenarioPreviewTexture();
				needToLoadTextures= false;
			}
		}

		//bool haveAtLeastOneNetworkClientConnected = false;
		bool hasOneNetworkSlotOpen = false;
		int currentConnectionCount=0;
		Config &config = Config::getInstance();

		bool masterServerErr = showMasterserverError;

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

		if(masterServerErr) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

			if(EndsWith(masterServererErrorToShow, "wrong router setup") == true) {
				masterServererErrorToShow=lang.get("WrongRouterSetup");
			}

			Lang &lang= Lang::getInstance();
			string publishText = " (disabling publish)";
			if(lang.hasString("PublishDisabled") == true) {
				publishText = lang.get("PublishDisabled");
			}

            masterServererErrorToShow += publishText;
			showMasterserverError=false;
			mainMessageBoxState=1;
			showMessageBox( masterServererErrorToShow, lang.get("ErrorFromMasterserver"), false);

			if(this->headlessServerMode == false) {
				listBoxPublishServer.setSelectedItemIndex(1);
			}

            ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();
            serverInterface->setPublishEnabled(listBoxPublishServer.getSelectedItemIndex() == 0);
		}
		else if(showGeneralError) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

			showGeneralError=false;
			mainMessageBoxState=1;
			showMessageBox( generalErrorToShow, "Error", false);
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

		if(this->headlessServerMode == true && serverInterface->getGameSettingsUpdateCount() > lastMasterServerSettingsUpdateCount &&
				serverInterface->getGameSettings() != NULL) {
			const GameSettings *settings = serverInterface->getGameSettings();
			//printf("\n\n\n\n=====#1 got settings [%d] [%d]:\n%s\n",lastMasterServerSettingsUpdateCount,serverInterface->getGameSettingsUpdateCount(),settings->toString().c_str());

			lastMasterServerSettingsUpdateCount = serverInterface->getGameSettingsUpdateCount();
			//printf("#2 custom menu got map [%s]\n",settings->getMap().c_str());

			setupUIFromGameSettings(*settings);

            GameSettings gameSettings;
            loadGameSettings(&gameSettings);

            //printf("\n\n\n\n=====#1.1 got settings [%d] [%d]:\n%s\n",lastMasterServerSettingsUpdateCount,serverInterface->getGameSettingsUpdateCount(),gameSettings.toString().c_str());

		}
		if(this->headlessServerMode == true && serverInterface->getMasterserverAdminRequestLaunch() == true) {
			serverInterface->setMasterserverAdminRequestLaunch(false);
			safeMutex.ReleaseLock();

			PlayNow(false);
			return;
		}

		// handle setting changes from clients
		SwitchSetupRequest ** switchSetupRequests = serverInterface->getSwitchSetupRequests();
		//!!!
		switchSetupForSlots(switchSetupRequests, serverInterface, 0, mapInfo.players, false);
		switchSetupForSlots(switchSetupRequests, serverInterface, mapInfo.players, GameConstants::maxPlayers, true);

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

		GameSettings gameSettings;
		loadGameSettings(&gameSettings);

		listBoxAISwitchTeamAcceptPercent.setEnabled(listBoxEnableSwitchTeamMode.getSelectedItemIndex() == 0);

		int factionCount = 0;
		for(int i= 0; i< mapInfo.players; ++i) {
			if(hasNetworkGameSettings() == true) {
				if(listBoxControls[i].getSelectedItemIndex() != ctClosed) {
					int slotIndex = factionCount;
					if(listBoxControls[i].getSelectedItemIndex() == ctHuman) {
						switch(gameSettings.getNetworkPlayerStatuses(slotIndex)) {
							case npst_BeRightBack:
								labelPlayerStatus[i].setText(lang.get("PlayerStatusBeRightBack"));
								labelPlayerStatus[i].setTextColor(Vec3f(1.f, 0.8f, 0.f));
								break;
							case npst_Ready:
								labelPlayerStatus[i].setText(lang.get("PlayerStatusReady"));
								labelPlayerStatus[i].setTextColor(Vec3f(0.f, 1.f, 0.f));
								break;
							case npst_PickSettings:
								labelPlayerStatus[i].setText(lang.get("PlayerStatusSetup"));
								labelPlayerStatus[i].setTextColor(Vec3f(1.f, 0.f, 0.f));
								break;
							default:
								labelPlayerStatus[i].setText("");
								break;
						}
					}
					else {
						labelPlayerStatus[i].setText("");
					}

					factionCount++;
				}
				else {
					labelPlayerStatus[i].setText("");
				}
			}

			if(listBoxControls[i].getSelectedItemIndex() == ctNetwork ||
				listBoxControls[i].getSelectedItemIndex() == ctNetworkUnassigned) {
				hasOneNetworkSlotOpen=true;

				if(serverInterface->getSlot(i) != NULL &&
                   serverInterface->getSlot(i)->isConnected()) {

					if(hasNetworkGameSettings() == true) {
						switch(serverInterface->getSlot(i)->getNetworkPlayerStatus()) {
							case npst_BeRightBack:
								labelPlayerStatus[i].setText(lang.get("PlayerStatusBeRightBack"));
								labelPlayerStatus[i].setTextColor(Vec3f(1.f, 0.8f, 0.f));
								break;
							case npst_Ready:
								labelPlayerStatus[i].setText(lang.get("PlayerStatusReady"));
								labelPlayerStatus[i].setTextColor(Vec3f(0.f, 1.f, 0.f));
								break;
							case npst_PickSettings:
							default:
								labelPlayerStatus[i].setText(lang.get("PlayerStatusSetup"));
								labelPlayerStatus[i].setTextColor(Vec3f(1.f, 0.f, 0.f));
								break;
						}
					}

					serverInterface->getSlot(i)->setName(labelPlayerNames[i].getText());

					//printf("FYI we have at least 1 client connected, slot = %d'\n",i);

					//haveAtLeastOneNetworkClientConnected = true;
					if(serverInterface->getSlot(i) != NULL &&
                       serverInterface->getSlot(i)->getConnectHasHandshaked()) {
						currentConnectionCount++;
                    }
					string label = (serverInterface->getSlot(i) != NULL ? serverInterface->getSlot(i)->getVersionString() : "");

					if(serverInterface->getSlot(i) != NULL &&
					   serverInterface->getSlot(i)->getAllowDownloadDataSynch() == true &&
					   serverInterface->getSlot(i)->getAllowGameDataSynchCheck() == true) {
						if(serverInterface->getSlot(i)->getNetworkGameDataSynchCheckOk() == false) {
							label += " -waiting to synch:";
							if(serverInterface->getSlot(i)->getNetworkGameDataSynchCheckOkMap() == false) {
								label = label + " map";
							}
							if(serverInterface->getSlot(i)->getNetworkGameDataSynchCheckOkTile() == false) {
								label = label + " tile";
							}
							if(serverInterface->getSlot(i)->getNetworkGameDataSynchCheckOkTech() == false) {
								label = label + " techtree";
							}
						}
						else {
							label += " - data synch is ok";
						}
					}
					else {
						if(serverInterface->getSlot(i) != NULL &&
						   serverInterface->getSlot(i)->getAllowGameDataSynchCheck() == true &&
						   serverInterface->getSlot(i)->getNetworkGameDataSynchCheckOk() == false) {
							label += " -synch mismatch:";

							if(serverInterface->getSlot(i) != NULL && serverInterface->getSlot(i)->getNetworkGameDataSynchCheckOkMap() == false) {
								label = label + " map";

								if(serverInterface->getSlot(i)->getReceivedDataSynchCheck() == true &&
									lastMapDataSynchError != "map CRC mismatch, " + listBoxMap.getSelectedItem()) {
									lastMapDataSynchError = "map CRC mismatch, " + listBoxMap.getSelectedItem();
									ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();
									serverInterface->sendTextMessage(lastMapDataSynchError,-1, true,"");
								}
							}

							if(serverInterface->getSlot(i) != NULL &&
                               serverInterface->getSlot(i)->getNetworkGameDataSynchCheckOkTile() == false) {
								label = label + " tile";

								if(serverInterface->getSlot(i)->getReceivedDataSynchCheck() == true &&
									lastTileDataSynchError != "tile CRC mismatch, " + listBoxTileset.getSelectedItem()) {
									lastTileDataSynchError = "tile CRC mismatch, " + listBoxTileset.getSelectedItem();
									ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();
									serverInterface->sendTextMessage(lastTileDataSynchError,-1,true,"");
								}
							}

							if(serverInterface->getSlot(i) != NULL &&
                               serverInterface->getSlot(i)->getNetworkGameDataSynchCheckOkTech() == false) {
								label = label + " techtree";

								if(serverInterface->getSlot(i)->getReceivedDataSynchCheck() == true) {
									ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();
									string report = serverInterface->getSlot(i)->getNetworkGameDataSynchCheckTechMismatchReport();

									if(lastTechtreeDataSynchError != "techtree CRC mismatch" + report) {
										lastTechtreeDataSynchError = "techtree CRC mismatch" + report;

										if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] report: %s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,report.c_str());

										serverInterface->sendTextMessage("techtree CRC mismatch",-1,true,"");
										vector<string> reportLineTokens;
										Tokenize(report,reportLineTokens,"\n");
										for(int reportLine = 0; reportLine < reportLineTokens.size(); ++reportLine) {
											serverInterface->sendTextMessage(reportLineTokens[reportLine],-1,true,"");
										}
									}
								}
							}

							if(serverInterface->getSlot(i) != NULL) {
								serverInterface->getSlot(i)->setReceivedDataSynchCheck(false);
							}
						}
					}

					//float pingTime = serverInterface->getSlot(i)->getThreadedPingMS(serverInterface->getSlot(i)->getIpAddress().c_str());
					char szBuf[1024]="";
					//sprintf(szBuf,"%s, ping = %.2fms",label.c_str(),pingTime);
					sprintf(szBuf,"%s",label.c_str());

					labelNetStatus[i].setText(szBuf);
				}
				else {
					string port = intToStr(config.getInt("ServerPort"));
					if(port != intToStr(GameConstants::serverPort)){
						port = port + " " + lang.get("NonStandardPort") + "!)";
					}
					else
					{
						port = port + ")";
					}
					port = "(" + port;
					labelNetStatus[i].setText("--- " + port);
				}
			}
			else{
				labelNetStatus[i].setText("");
			}
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

		//ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();

		if(checkBoxScenario.getValue() == false) {
			for(int i= 0; i< GameConstants::maxPlayers; ++i) {
				if(i >= mapInfo.players) {
					listBoxControls[i].setEditable(false);
					listBoxControls[i].setEnabled(false);

					//printf("In [%s::%s] Line: %d i = %d mapInfo.players = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,i,mapInfo.players);
				}
				else if(listBoxControls[i].getSelectedItemIndex() != ctNetworkUnassigned) {
					ConnectionSlot *slot = serverInterface->getSlot(i);
					if((listBoxControls[i].getSelectedItemIndex() != ctNetwork) ||
						(listBoxControls[i].getSelectedItemIndex() == ctNetwork && (slot == NULL || slot->isConnected() == false))) {
						listBoxControls[i].setEditable(true);
						listBoxControls[i].setEnabled(true);
					}
					else {
						listBoxControls[i].setEditable(false);
						listBoxControls[i].setEnabled(false);

						//printf("In [%s::%s] Line: %d i = %d mapInfo.players = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,i,mapInfo.players);
					}
				}
				else {
					listBoxControls[i].setEditable(false);
					listBoxControls[i].setEnabled(false);

					//printf("In [%s::%s] Line: %d i = %d mapInfo.players = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,i,mapInfo.players);
				}
			}
		}

		bool checkDataSynch = (serverInterface->getAllowGameDataSynchCheck() == true &&
					needToSetChangedGameSettings == true &&
					(( difftime(time(NULL),lastSetChangedGameSettings) >= BROADCAST_SETTINGS_SECONDS)||
					(this->headlessServerMode == true)));

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

		// Send the game settings to each client if we have at least one networked client
		if(checkDataSynch == true) {
			serverInterface->setGameSettings(&gameSettings,false);
			needToSetChangedGameSettings    = false;
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

		if(this->headlessServerMode == true || hasOneNetworkSlotOpen == true) {
			if(this->headlessServerMode == true) {
				listBoxPublishServer.setSelectedItemIndex(0);
			}
			listBoxPublishServer.setEditable(true);
			//listBoxEnableServerControlledAI.setEditable(true);

			// Masterserver always needs one network slot
			if(this->headlessServerMode == true && hasOneNetworkSlotOpen == false) {
				bool anyoneConnected = false;
				for(int i= 0; i < mapInfo.players; ++i) {
					MutexSafeWrapper safeMutex((publishToMasterserverThread != NULL ? publishToMasterserverThread->getMutexThreadObjectAccessor() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));

					ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();
					ConnectionSlot *slot = serverInterface->getSlot(i);
					if(slot != NULL && slot->isConnected() == true) {
						anyoneConnected = true;
						break;
					}
				}

				for(int i= 0; i < mapInfo.players; ++i) {
					if(anyoneConnected == false && listBoxControls[i].getSelectedItemIndex() != ctNetwork) {
						listBoxControls[i].setSelectedItemIndex(ctNetwork);
					}
				}

				updateNetworkSlots();
			}
		}
		else {
			listBoxPublishServer.setSelectedItemIndex(1);
			listBoxPublishServer.setEditable(false);

            ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();
            serverInterface->setPublishEnabled(listBoxPublishServer.getSelectedItemIndex() == 0);
			//listBoxEnableServerControlledAI.setEditable(false);
		}

		bool republishToMaster = (difftime(time(NULL),lastMasterserverPublishing) >= MASTERSERVER_BROADCAST_PUBLISH_SECONDS);

		if(republishToMaster == true) {
			if(listBoxPublishServer.getSelectedItemIndex() == 0) {
				needToRepublishToMasterserver = true;
				lastMasterserverPublishing = time(NULL);
			}
		}

		bool callPublishNow = (listBoxPublishServer.getEditable() &&
							   listBoxPublishServer.getSelectedItemIndex() == 0 &&
							   needToRepublishToMasterserver == true);

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

		if(callPublishNow == true) {
			// give it to me baby, aha aha ...
			publishToMasterserver();
		}
		if(needToPublishDelayed) {
			// this delay is done to make it possible to switch over maps which are not meant to be distributed
			if((difftime(time(NULL), mapPublishingDelayTimer) >= BROADCAST_MAP_DELAY_SECONDS) ||
					(this->headlessServerMode == true)	){
				// after 5 seconds we are allowed to publish again!
				needToSetChangedGameSettings = true;
				lastSetChangedGameSettings   = time(NULL);
				// set to normal....
				needToPublishDelayed=false;
			}
		}
		if(needToPublishDelayed == false || headlessServerMode == true) {
			bool broadCastSettings = (difftime(time(NULL),lastSetChangedGameSettings) >= BROADCAST_SETTINGS_SECONDS);

			//printf("broadCastSettings = %d\n",broadCastSettings);

			if(broadCastSettings == true) {
				needToBroadcastServerSettings=true;
				lastSetChangedGameSettings = time(NULL);
			}

			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());
			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

			//broadCastSettings = (difftime(time(NULL),lastSetChangedGameSettings) >= 2);
			//if (broadCastSettings == true) {// reset timer here on bottom becasue used for different things
			//	lastSetChangedGameSettings = time(NULL);
			//}
		}

		//call the chat manager
		chatManager.updateNetwork();

		//console
		console.update();

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

		if(currentConnectionCount > soundConnectionCount){
			soundConnectionCount = currentConnectionCount;
			SoundRenderer::getInstance().playFx(CoreData::getInstance().getAttentionSound());
			//switch on music again!!
			Config &config = Config::getInstance();
			float configVolume = (config.getInt("SoundVolumeMusic") / 100.f);
			CoreData::getInstance().getMenuMusic()->setVolume(configVolume);
		}
		soundConnectionCount = currentConnectionCount;

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

		if(enableFactionTexturePreview == true) {
			if( currentTechName_factionPreview != gameSettings.getTech() ||
				currentFactionName_factionPreview != gameSettings.getFactionTypeName(gameSettings.getThisFactionIndex())) {

				currentTechName_factionPreview=gameSettings.getTech();
				currentFactionName_factionPreview=gameSettings.getFactionTypeName(gameSettings.getThisFactionIndex());

				string factionVideoUrl = "";
				string factionVideoUrlFallback = "";

				string factionDefinitionXML = Game::findFactionLogoFile(&gameSettings, NULL,currentFactionName_factionPreview + ".xml");
				if(factionDefinitionXML != "" && currentFactionName_factionPreview != GameConstants::RANDOMFACTION_SLOTNAME &&
						currentFactionName_factionPreview != GameConstants::OBSERVER_SLOTNAME && fileExists(factionDefinitionXML) == true) {
					XmlTree	xmlTree;
					std::map<string,string> mapExtraTagReplacementValues;
					xmlTree.load(factionDefinitionXML, Properties::getTagReplacementValues(&mapExtraTagReplacementValues));
					const XmlNode *factionNode= xmlTree.getRootNode();
					if(factionNode->hasAttribute("faction-preview-video") == true) {
						factionVideoUrl = factionNode->getAttribute("faction-preview-video")->getValue();
					}

					factionVideoUrlFallback = Game::findFactionLogoFile(&gameSettings, NULL,"preview_video.*");
					if(factionVideoUrl == "") {
						factionVideoUrl = factionVideoUrlFallback;
						factionVideoUrlFallback = "";
					}
				}
				//printf("currentFactionName_factionPreview [%s] random [%s] observer [%s] factionVideoUrl [%s]\n",currentFactionName_factionPreview.c_str(),GameConstants::RANDOMFACTION_SLOTNAME,GameConstants::OBSERVER_SLOTNAME,factionVideoUrl.c_str());

				if(factionVideoUrl != "") {
					SoundRenderer &soundRenderer= SoundRenderer::getInstance();
					if(CoreData::getInstance().getMenuMusic()->getVolume() != 0) {
						CoreData::getInstance().getMenuMusic()->setVolume(0);
						factionVideoSwitchedOffVolume=true;
					}

					if(currentFactionLogo != factionVideoUrl) {
						currentFactionLogo = factionVideoUrl;
						if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false &&
							Shared::Graphics::VideoPlayer::hasBackEndVideoPlayer() == true) {

							if(factionVideo != NULL) {
								factionVideo->closePlayer();
								delete factionVideo;
								factionVideo = NULL;
							}
							string introVideoFile = factionVideoUrl;
							string introVideoFileFallback = factionVideoUrlFallback;

							Context *c= GraphicsInterface::getInstance().getCurrentContext();
							SDL_Surface *screen = static_cast<ContextGl*>(c)->getPlatformContextGlPtr()->getScreen();

							string vlcPluginsPath = Config::getInstance().getString("VideoPlayerPluginsPath","");
							//printf("screen->w = %d screen->h = %d screen->format->BitsPerPixel = %d\n",screen->w,screen->h,screen->format->BitsPerPixel);
							factionVideo = new VideoPlayer(
									&Renderer::getInstance(),
									introVideoFile,
									introVideoFileFallback,
									screen,
									0,0,
									screen->w,
									screen->h,
									screen->format->BitsPerPixel,
									vlcPluginsPath,
									SystemFlags::VERBOSE_MODE_ENABLED);
							factionVideo->initPlayer();
						}
					}
				}
				else {
					SoundRenderer &soundRenderer= SoundRenderer::getInstance();
					//switch on music again!!
					Config &config = Config::getInstance();
					float configVolume = (config.getInt("SoundVolumeMusic") / 100.f);
					if(factionVideoSwitchedOffVolume){
						if(CoreData::getInstance().getMenuMusic()->getVolume() != configVolume) {
							CoreData::getInstance().getMenuMusic()->setVolume(configVolume);
						}
						factionVideoSwitchedOffVolume=false;
					}

					if(factionVideo != NULL) {
						factionVideo->closePlayer();
						delete factionVideo;
						factionVideo = NULL;
					}
				}

				if(factionVideo == NULL) {
					string factionLogo = Game::findFactionLogoFile(&gameSettings, NULL,"preview_screen.*");
					if(factionLogo == "") {
						factionLogo = Game::findFactionLogoFile(&gameSettings, NULL);
					}
					if(currentFactionLogo != factionLogo) {
						currentFactionLogo = factionLogo;
						loadFactionTexture(currentFactionLogo);
					}
				}
			}
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

		if(autostart == true) {
			autostart = false;
			safeMutex.ReleaseLock();
			if(autoStartSettings != NULL) {

				setupUIFromGameSettings(*autoStartSettings);
				ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();
				serverInterface->setGameSettings(autoStartSettings,false);
			}
			else {
				RestoreLastGameSettings();
			}
			PlayNow((autoStartSettings == NULL));
			return;
		}
	}
	catch(const std::exception &ex) {
		char szBuf[8096]="";
		sprintf(szBuf,"In [%s::%s %d]\nError detected:\n%s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);

		showGeneralError=true;
		generalErrorToShow = szBuf;
	}
}

void MenuStateCustomGame::publishToMasterserver() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	int slotCountUsed=0;
	int slotCountHumans=0;
	int slotCountConnectedPlayers=0;
	ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();
	GameSettings gameSettings;
	loadGameSettings(&gameSettings);
	Config &config= Config::getInstance();
	//string serverinfo="";

	MutexSafeWrapper safeMutex((publishToMasterserverThread != NULL ? publishToMasterserverThread->getMutexThreadObjectAccessor() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
	publishToServerInfo.clear();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	for(int i= 0; i < mapInfo.players; ++i) {
		if(listBoxControls[i].getSelectedItemIndex() != ctClosed) {
			slotCountUsed++;
		}

		if(listBoxControls[i].getSelectedItemIndex() == ctNetwork || listBoxControls[i].getSelectedItemIndex() == ctNetworkUnassigned)
		{
			slotCountHumans++;
			if(serverInterface->getSlot(i) != NULL &&
               serverInterface->getSlot(i)->isConnected()) {
				slotCountConnectedPlayers++;
			}
		}
		else if(listBoxControls[i].getSelectedItemIndex() == ctHuman) {
			slotCountHumans++;
			slotCountConnectedPlayers++;
		}
	}

	//?status=waiting&system=linux&info=titus
	publishToServerInfo["glestVersion"] = glestVersionString;
	publishToServerInfo["platform"] = getPlatformNameString() + "-" + getSVNRevisionString();
    publishToServerInfo["binaryCompileDate"] = getCompileDateTime();

	//game info:
	publishToServerInfo["serverTitle"] = getHumanPlayerName() + "'s game";
	publishToServerInfo["serverTitle"] = labelGameName.getText();
	//ip is automatically set

	//game setup info:

	//publishToServerInfo["tech"] = listBoxTechTree.getSelectedItem();
    publishToServerInfo["tech"] = techTreeFiles[listBoxTechTree.getSelectedItemIndex()];
	//publishToServerInfo["map"] = listBoxMap.getSelectedItem();
    publishToServerInfo["map"] = getCurrentMapFile();
	//publishToServerInfo["tileset"] = listBoxTileset.getSelectedItem();
    publishToServerInfo["tileset"] = tilesetFiles[listBoxTileset.getSelectedItemIndex()];

	publishToServerInfo["activeSlots"] = intToStr(slotCountUsed);
	publishToServerInfo["networkSlots"] = intToStr(slotCountHumans);
	publishToServerInfo["connectedClients"] = intToStr(slotCountConnectedPlayers);

	string externalport = config.getString("MasterServerExternalPort", intToStr(GameConstants::serverPort).c_str());
	publishToServerInfo["externalconnectport"] = externalport;
	publishToServerInfo["privacyPlease"] = intToStr(config.getBool("PrivacyPlease","false"));

	publishToServerInfo["gameStatus"] = intToStr(game_status_waiting_for_players);
	if(slotCountHumans <= slotCountConnectedPlayers) {
		publishToServerInfo["gameStatus"] = intToStr(game_status_waiting_for_start);
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

void MenuStateCustomGame::setupTask(BaseThread *callingThread) {
	MenuStateCustomGame::setupTaskStatic(callingThread);
}
void MenuStateCustomGame::shutdownTask(BaseThread *callingThread) {
	MenuStateCustomGame::shutdownTaskStatic(callingThread);
}
void MenuStateCustomGame::setupTaskStatic(BaseThread *callingThread) {
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	CURL *handle = SystemFlags::initHTTP();
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	callingThread->setGenericData<CURL>(handle);
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}
void MenuStateCustomGame::shutdownTaskStatic(BaseThread *callingThread) {
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	CURL *handle = callingThread->getGenericData<CURL>();
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	SystemFlags::cleanupHTTP(&handle);
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

void MenuStateCustomGame::simpleTask(BaseThread *callingThread) {

    try {
        //printf("-=-=-=-=- IN MenuStateCustomGame simpleTask - A\n");

        MutexSafeWrapper safeMutexThreadOwner(callingThread->getMutexThreadOwnerValid(),string(__FILE__) + "_" + intToStr(__LINE__));
        if(callingThread->getQuitStatus() == true || safeMutexThreadOwner.isValidMutex() == false) {
            return;
        }

        //printf("-=-=-=-=- IN MenuStateCustomGame simpleTask - B\n");

        MutexSafeWrapper safeMutex(callingThread->getMutexThreadObjectAccessor(),string(__FILE__) + "_" + intToStr(__LINE__));
        bool republish                                  = (needToRepublishToMasterserver == true  && publishToServerInfo.empty() == false);
        needToRepublishToMasterserver                   = false;
        std::map<string,string> newPublishToServerInfo  = publishToServerInfo;
        publishToServerInfo.clear();
        bool broadCastSettings                          = needToBroadcastServerSettings;

        //printf("simpleTask broadCastSettings = %d\n",broadCastSettings);

        needToBroadcastServerSettings                   = false;
        bool hasClientConnection                        = false;

        if(broadCastSettings == true) {
            ServerInterface *serverInterface = NetworkManager::getInstance().getServerInterface(false);
            if(serverInterface != NULL) {
                hasClientConnection = serverInterface->hasClientConnection();
            }
        }
        bool needPing = (difftime(time(NULL),lastNetworkPing) >= GameConstants::networkPingInterval);

        if(callingThread->getQuitStatus() == true) {
            return;
        }

        //printf("-=-=-=-=- IN MenuStateCustomGame simpleTask - C\n");

        if(republish == true) {
        	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

            string request = Config::getInstance().getString("Masterserver") + "addServerInfo.php?";

            //CURL *handle = SystemFlags::initHTTP();
            CURL *handle = callingThread->getGenericData<CURL>();

            int paramIndex = 0;
            for(std::map<string,string>::const_iterator iterMap = newPublishToServerInfo.begin();
                iterMap != newPublishToServerInfo.end(); ++iterMap) {

                request += iterMap->first;
                request += "=";
                request += SystemFlags::escapeURL(iterMap->second,handle);

                paramIndex++;
                if(paramIndex < newPublishToServerInfo.size()) {
                	request += "&";
                }
            }

            //printf("the request is:\n%s\n",request.c_str());
            if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] the request is:\n%s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,request.c_str());
            safeMutex.ReleaseLock(true);
            safeMutexThreadOwner.ReleaseLock();

            std::string serverInfo = SystemFlags::getHTTP(request,handle);
            //SystemFlags::cleanupHTTP(&handle);

            MutexSafeWrapper safeMutexThreadOwner2(callingThread->getMutexThreadOwnerValid(),string(__FILE__) + "_" + intToStr(__LINE__));
            if(callingThread->getQuitStatus() == true || safeMutexThreadOwner2.isValidMutex() == false) {
                return;
            }
            safeMutex.Lock();

            //printf("the result is:\n'%s'\n",serverInfo.c_str());
            if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] the result is:\n'%s'\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,serverInfo.c_str());

            // uncomment to enable router setup check of this server
            if(EndsWith(serverInfo, "OK") == false) {
                if(callingThread->getQuitStatus() == true) {
                    return;
                }

                // Give things another chance to see if we can get a connection from the master server
                if(tMasterserverErrorElapsed > 0 &&
                		difftime(time(NULL),tMasterserverErrorElapsed) > MASTERSERVER_BROADCAST_MAX_WAIT_RESPONSE_SECONDS) {
                	showMasterserverError=true;
                	masterServererErrorToShow = (serverInfo != "" ? serverInfo : "No Reply");
                }
                else {
                	if(tMasterserverErrorElapsed == 0) {
                		tMasterserverErrorElapsed = time(NULL);
                	}

                	SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line %d] error checking response from masterserver elapsed seconds = %.2f / %d\nResponse:\n%s\n",
                			extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,difftime(time(NULL),tMasterserverErrorElapsed),MASTERSERVER_BROADCAST_MAX_WAIT_RESPONSE_SECONDS,serverInfo.c_str());

                	needToRepublishToMasterserver = true;
                }
            }
        }
        else {
            safeMutexThreadOwner.ReleaseLock();
        }

        if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

        //printf("-=-=-=-=- IN MenuStateCustomGame simpleTask - D\n");

        if(broadCastSettings == true) {
            MutexSafeWrapper safeMutexThreadOwner2(callingThread->getMutexThreadOwnerValid(),string(__FILE__) + "_" + intToStr(__LINE__));
            if(callingThread->getQuitStatus() == true || safeMutexThreadOwner2.isValidMutex() == false) {
                return;
            }

            if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

            //printf("simpleTask broadCastSettings = %d hasClientConnection = %d\n",broadCastSettings,hasClientConnection);

            if(callingThread->getQuitStatus() == true) {
                return;
            }
            ServerInterface *serverInterface= NetworkManager::getInstance().getServerInterface(false);
            if(serverInterface != NULL) {

            	if(this->headlessServerMode == false || (serverInterface->getGameSettingsUpdateCount() <= lastMasterServerSettingsUpdateCount)) {
                    GameSettings gameSettings;
                    loadGameSettings(&gameSettings);

                    //printf("\n\n\n\n=====#2 got settings [%d] [%d]:\n%s\n",lastMasterServerSettingsUpdateCount,serverInterface->getGameSettingsUpdateCount(),gameSettings.toString().c_str());

                    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

					serverInterface->setGameSettings(&gameSettings,false);
					lastMasterServerSettingsUpdateCount = serverInterface->getGameSettingsUpdateCount();

					if(hasClientConnection == true) {
						if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
						serverInterface->broadcastGameSetup(&gameSettings);
					}
            	}
            }
        }

        //printf("-=-=-=-=- IN MenuStateCustomGame simpleTask - E\n");

        if(needPing == true) {
            MutexSafeWrapper safeMutexThreadOwner2(callingThread->getMutexThreadOwnerValid(),string(__FILE__) + "_" + intToStr(__LINE__));
            if(callingThread->getQuitStatus() == true || safeMutexThreadOwner2.isValidMutex() == false) {
                return;
            }

            lastNetworkPing = time(NULL);

            if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] Sending nmtPing to clients\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

            ServerInterface *serverInterface= NetworkManager::getInstance().getServerInterface(false);
            if(serverInterface != NULL) {
            	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
                NetworkMessagePing *msg = new NetworkMessagePing(GameConstants::networkPingInterval,time(NULL));
                //serverInterface->broadcastPing(&msg);
                serverInterface->queueBroadcastMessage(msg);
                if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
            }
        }
        safeMutex.ReleaseLock();

        //printf("-=-=-=-=- IN MenuStateCustomGame simpleTask - F\n");
    }
	catch(const std::exception &ex) {
		char szBuf[8096]="";
		sprintf(szBuf,"In [%s::%s %d]\nError detected:\n%s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);

		if(callingThread->getQuitStatus() == false) {
            //throw megaglest_runtime_error(szBuf);
            showGeneralError=true;
            generalErrorToShow = ex.what();
		}
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

void MenuStateCustomGame::loadGameSettings(GameSettings *gameSettings,bool forceCloseUnusedSlots) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	int factionCount= 0;
	ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	if(this->headlessServerMode == true && serverInterface->getGameSettingsUpdateCount() > lastMasterServerSettingsUpdateCount &&
			serverInterface->getGameSettings() != NULL) {
		const GameSettings *settings = serverInterface->getGameSettings();
		//printf("\n\n\n\n=====#3 got settings [%d] [%d]:\n%s\n",lastMasterServerSettingsUpdateCount,serverInterface->getGameSettingsUpdateCount(),settings->toString().c_str());

		lastMasterServerSettingsUpdateCount = serverInterface->getGameSettingsUpdateCount();
		//printf("#1 custom menu got map [%s]\n",settings->getMap().c_str());

		setupUIFromGameSettings(*settings);
	}

    // Test flags values
    //gameSettings->setFlagTypes1(ft1_show_map_resources);
    //

	if(checkBoxScenario.getValue() == true) {
		gameSettings->setScenario(scenarioInfo.name);
		gameSettings->setScenarioDir(Scenario::getScenarioPath(dirList, scenarioInfo.name));

		gameSettings->setDefaultResources(scenarioInfo.defaultResources);
		gameSettings->setDefaultUnits(scenarioInfo.defaultUnits);
		gameSettings->setDefaultVictoryConditions(scenarioInfo.defaultVictoryConditions);
	}
	else {
		gameSettings->setScenario("");
		gameSettings->setScenarioDir("");
	}

	//printf("scenarioInfo.name [%s] [%s] [%s]\n",scenarioInfo.name.c_str(),listBoxMap.getSelectedItem().c_str(),getCurrentMapFile().c_str());

	gameSettings->setMapFilterIndex(listBoxMapFilter.getSelectedItemIndex());
	gameSettings->setDescription(formatString(getCurrentMapFile()));
	gameSettings->setMap(getCurrentMapFile());
    gameSettings->setTileset(tilesetFiles[listBoxTileset.getSelectedItemIndex()]);
    gameSettings->setTech(techTreeFiles[listBoxTechTree.getSelectedItemIndex()]);

    if(autoStartSettings != NULL) {
    	gameSettings->setDefaultUnits(autoStartSettings->getDefaultUnits());
    	gameSettings->setDefaultResources(autoStartSettings->getDefaultResources());
    	gameSettings->setDefaultVictoryConditions(autoStartSettings->getDefaultVictoryConditions());
    }
    else if(checkBoxScenario.getValue() == false) {
		gameSettings->setDefaultUnits(true);
		gameSettings->setDefaultResources(true);
		gameSettings->setDefaultVictoryConditions(true);
    }

   	gameSettings->setFogOfWar(listBoxFogOfWar.getSelectedItemIndex() == 0 ||
								listBoxFogOfWar.getSelectedItemIndex() == 1 );

	gameSettings->setAllowObservers(listBoxAllowObservers.getSelectedItemIndex() == 1);

	uint32 valueFlags1 = gameSettings->getFlagTypes1();
	if(listBoxFogOfWar.getSelectedItemIndex() == 1 ||
		listBoxFogOfWar.getSelectedItemIndex() == 2 ) {
        valueFlags1 |= ft1_show_map_resources;
        gameSettings->setFlagTypes1(valueFlags1);
	}
	else {
        valueFlags1 &= ~ft1_show_map_resources;
        gameSettings->setFlagTypes1(valueFlags1);
	}

	//gameSettings->setEnableObserverModeAtEndGame(listBoxEnableObserverMode.getSelectedItemIndex() == 0);
	gameSettings->setEnableObserverModeAtEndGame(true);
	gameSettings->setPathFinderType(static_cast<PathFinderType>(listBoxPathFinderType.getSelectedItemIndex()));

	valueFlags1 = gameSettings->getFlagTypes1();
	if(listBoxEnableSwitchTeamMode.getSelectedItemIndex() == 0) {
        valueFlags1 |= ft1_allow_team_switching;
        gameSettings->setFlagTypes1(valueFlags1);
	}
	else {
        valueFlags1 &= ~ft1_allow_team_switching;
        gameSettings->setFlagTypes1(valueFlags1);
	}
	gameSettings->setAiAcceptSwitchTeamPercentChance(strToInt(listBoxAISwitchTeamAcceptPercent.getSelectedItem()));

	// First save Used slots
    //for(int i=0; i<mapInfo.players; ++i)
	int AIPlayerCount = 0;
	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		ControlType ct= static_cast<ControlType>(listBoxControls[i].getSelectedItemIndex());

		if(forceCloseUnusedSlots == true && (ct == ctNetworkUnassigned || ct == ctNetwork)) {
			if(serverInterface->getSlot(i) == NULL ||
               serverInterface->getSlot(i)->isConnected() == false) {
				if(checkBoxScenario.getValue() == false) {
				//printf("Closed A [%d] [%s]\n",i,labelPlayerNames[i].getText().c_str());

					listBoxControls[i].setSelectedItemIndex(ctClosed);
					ct = ctClosed;
				}
			}
		}
		else if(ct == ctNetworkUnassigned && i < mapInfo.players) {
			listBoxControls[i].setSelectedItemIndex(ctNetwork);
			ct = ctNetwork;
		}

		if(ct != ctClosed) {
			int slotIndex = factionCount;

			gameSettings->setFactionControl(slotIndex, ct);
			if(ct == ctHuman) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] i = %d, slotIndex = %d, getHumanPlayerName(i) [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,i,slotIndex,getHumanPlayerName(i).c_str());

				gameSettings->setThisFactionIndex(slotIndex);
				gameSettings->setNetworkPlayerName(slotIndex, getHumanPlayerName(i));
				gameSettings->setNetworkPlayerStatuses(slotIndex, getNetworkPlayerStatus());
				Lang &lang= Lang::getInstance();
				gameSettings->setNetworkPlayerLanguages(slotIndex, lang.getLanguage());
			}
			else if(serverInterface->getSlot(i) != NULL) {
				gameSettings->setNetworkPlayerLanguages(slotIndex, serverInterface->getSlot(i)->getNetworkPlayerLanguage());
			}
			gameSettings->setResourceMultiplierIndex(slotIndex, listBoxRMultiplier[i].getSelectedItemIndex());

			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] i = %d, factionFiles[listBoxFactions[i].getSelectedItemIndex()] [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,i,factionFiles[listBoxFactions[i].getSelectedItemIndex()].c_str());

			gameSettings->setFactionTypeName(slotIndex, factionFiles[listBoxFactions[i].getSelectedItemIndex()]);
			if(factionFiles[listBoxFactions[i].getSelectedItemIndex()] == formatString(GameConstants::OBSERVER_SLOTNAME)) {
				listBoxTeams[i].setSelectedItem(intToStr(GameConstants::maxPlayers + fpt_Observer));
			}
			else if(listBoxTeams[i].getSelectedItem() == intToStr(GameConstants::maxPlayers + fpt_Observer)) {
				if(lastSelectedTeamIndex[i] >= 0 && lastSelectedTeamIndex[i] + 1 != (GameConstants::maxPlayers + fpt_Observer)) {
					if(lastSelectedTeamIndex[i] == 0) {
						lastSelectedTeamIndex[i] = GameConstants::maxPlayers-1;
					}
					else if(lastSelectedTeamIndex[i] == GameConstants::maxPlayers-1) {
						lastSelectedTeamIndex[i] = 0;
					}

					listBoxTeams[i].setSelectedItemIndex(lastSelectedTeamIndex[i]);
				}
				else {
					listBoxTeams[i].setSelectedItem(intToStr(1));
				}
			}

			gameSettings->setTeam(slotIndex, listBoxTeams[i].getSelectedItemIndex());
			gameSettings->setStartLocationIndex(slotIndex, i);


			if(listBoxControls[i].getSelectedItemIndex() == ctNetwork || listBoxControls[i].getSelectedItemIndex() == ctNetworkUnassigned) {
				if(serverInterface->getSlot(i) != NULL &&
                   serverInterface->getSlot(i)->isConnected()) {

					gameSettings->setNetworkPlayerStatuses(slotIndex,serverInterface->getSlot(i)->getNetworkPlayerStatus());

					if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] i = %d, connectionSlot->getName() [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,i,serverInterface->getSlot(i)->getName().c_str());

					gameSettings->setNetworkPlayerName(slotIndex, serverInterface->getSlot(i)->getName());
					labelPlayerNames[i].setText(serverInterface->getSlot(i)->getName());
				}
				else {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] i = %d, playername unconnected\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,i);

					gameSettings->setNetworkPlayerName(slotIndex, GameConstants::NETWORK_SLOT_UNCONNECTED_SLOTNAME);
					labelPlayerNames[i].setText("");
				}
			}
			else if (listBoxControls[i].getSelectedItemIndex() != ctHuman) {
				AIPlayerCount++;
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] i = %d, playername is AI (blank)\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,i);

				Lang &lang= Lang::getInstance();
				gameSettings->setNetworkPlayerName(slotIndex, lang.get("AI") + intToStr(AIPlayerCount));
				labelPlayerNames[i].setText("");
			}

			factionCount++;
		}
		else {
			//gameSettings->setNetworkPlayerName("");
			gameSettings->setNetworkPlayerStatuses(factionCount, 0);
			labelPlayerNames[i].setText("");
		}
    }

	// Next save closed slots
	int closedCount = 0;
	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		ControlType ct= static_cast<ControlType>(listBoxControls[i].getSelectedItemIndex());
		if(ct == ctClosed) {
			int slotIndex = factionCount + closedCount;

			gameSettings->setFactionControl(slotIndex, ct);
			gameSettings->setTeam(slotIndex, listBoxTeams[i].getSelectedItemIndex());
			gameSettings->setStartLocationIndex(slotIndex, i);
			gameSettings->setResourceMultiplierIndex(slotIndex, 10);

			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] i = %d, factionFiles[listBoxFactions[i].getSelectedItemIndex()] [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,i,factionFiles[listBoxFactions[i].getSelectedItemIndex()].c_str());

			gameSettings->setFactionTypeName(slotIndex, factionFiles[listBoxFactions[i].getSelectedItemIndex()]);
			gameSettings->setNetworkPlayerName(slotIndex, "Closed");

			closedCount++;
		}
    }

	gameSettings->setFactionCount(factionCount);

	Config &config = Config::getInstance();
	gameSettings->setEnableServerControlledAI(config.getBool("ServerControlledAI","true"));
	gameSettings->setNetworkFramePeriod(config.getInt("NetworkSendFrameCount","20"));
	gameSettings->setNetworkPauseGameForLaggedClients(((listBoxNetworkPauseGameForLaggedClients.getSelectedItemIndex() != 0)));

	if(hasNetworkGameSettings() == true) {
		if( gameSettings->getTileset() != "") {
			if(lastCheckedCRCTilesetName != gameSettings->getTileset()) {
				//console.addLine("Checking tileset CRC [" + gameSettings->getTileset() + "]");
				lastCheckedCRCTilesetValue = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTilesets,""), string("/") + gameSettings->getTileset() + string("/*"), ".xml", NULL);
				if(lastCheckedCRCTilesetValue == 0) {
					lastCheckedCRCTilesetValue = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTilesets,""), string("/") + gameSettings->getTileset() + string("/*"), ".xml", NULL, true);
				}
				lastCheckedCRCTilesetName = gameSettings->getTileset();
			}
			gameSettings->setTilesetCRC(lastCheckedCRCTilesetValue);
		}

		if(config.getBool("DisableServerLobbyTechtreeCRCCheck","false") == false) {
			if(gameSettings->getTech() != "") {
				if(lastCheckedCRCTechtreeName != gameSettings->getTech()) {
					//console.addLine("Checking techtree CRC [" + gameSettings->getTech() + "]");
					lastCheckedCRCTechtreeValue = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTechs,""), "/" + gameSettings->getTech() + "/*", ".xml", NULL);
					if(lastCheckedCRCTechtreeValue == 0) {
						lastCheckedCRCTechtreeValue = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTechs,""), "/" + gameSettings->getTech() + "/*", ".xml", NULL, true);
					}

					reloadFactions(true,(checkBoxScenario.getValue() == true ? scenarioFiles[listBoxScenario.getSelectedItemIndex()] : ""));
					factionCRCList.clear();
					for(unsigned int factionIdx = 0; factionIdx < factionFiles.size(); ++factionIdx) {
						string factionName = factionFiles[factionIdx];
						if(factionName != GameConstants::RANDOMFACTION_SLOTNAME &&
							factionName != GameConstants::OBSERVER_SLOTNAME) {
							//factionCRC   = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTechs,""), "/" + gameSettings->getTech() + "/factions/" + factionName + "/*", ".xml", NULL, true);
							uint32 factionCRC   = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTechs,""), "/" + gameSettings->getTech() + "/factions/" + factionName + "/*", ".xml", NULL);
							if(factionCRC == 0) {
								factionCRC   = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTechs,""), "/" + gameSettings->getTech() + "/factions/" + factionName + "/*", ".xml", NULL, true);
							}
							factionCRCList.push_back(make_pair(factionName,factionCRC));
						}
					}
					//console.addLine("Found factions: " + intToStr(factionCRCList.size()));
					lastCheckedCRCTechtreeName = gameSettings->getTech();
				}

				gameSettings->setFactionCRCList(factionCRCList);
				gameSettings->setTechCRC(lastCheckedCRCTechtreeValue);
			}
		}

		if(gameSettings->getMap() != "") {
			if(lastCheckedCRCMapName != gameSettings->getMap()) {
				Checksum checksum;
				string file = Map::getMapPath(gameSettings->getMap(),"",false);
				//console.addLine("Checking map CRC [" + file + "]");
				checksum.addFile(file);
				lastCheckedCRCMapValue = checksum.getSum();
				lastCheckedCRCMapName = gameSettings->getMap();
			}
			gameSettings->setMapCRC(lastCheckedCRCMapValue);
		}
	}

	//printf("this->masterserverMode = %d\n",this->masterserverMode);

	if(this->headlessServerMode == true) {
		time_t clientConnectedTime = 0;
		bool masterserver_admin_found=false;
		//printf("mapInfo.players [%d]\n",mapInfo.players);

		for(int i= 0; i < mapInfo.players; ++i) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

			if(listBoxControls[i].getSelectedItemIndex() == ctNetwork || listBoxControls[i].getSelectedItemIndex() == ctNetworkUnassigned) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

				if(	serverInterface->getSlot(i) != NULL && serverInterface->getSlot(i)->isConnected()) {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

					//printf("slot = %d serverInterface->getSlot(i)->getConnectedTime() = %d session key [%d]\n",i,serverInterface->getSlot(i)->getConnectedTime(),serverInterface->getSlot(i)->getSessionKey());

					if(clientConnectedTime == 0 ||
							(serverInterface->getSlot(i)->getConnectedTime() > 0 && serverInterface->getSlot(i)->getConnectedTime() < clientConnectedTime)) {
						clientConnectedTime = serverInterface->getSlot(i)->getConnectedTime();
						gameSettings->setMasterserver_admin(serverInterface->getSlot(i)->getSessionKey());
						gameSettings->setMasterserver_admin_faction_index(serverInterface->getSlot(i)->getPlayerIndex());
						labelGameName.setText(serverInterface->getSlot(i)->getName()+" controls");
						//printf("slot = %d, admin key [%d] slot connected time[%lu] clientConnectedTime [%lu]\n",i,gameSettings->getMasterserver_admin(),serverInterface->getSlot(i)->getConnectedTime(),clientConnectedTime);
					}
					if(serverInterface->getSlot(i)->getSessionKey() == gameSettings->getMasterserver_admin()){
						masterserver_admin_found=true;
					}
				}
			}
		}
		if(masterserver_admin_found == false ) {
			for(int i=mapInfo.players; i < GameConstants::maxPlayers; ++i) {
				ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();
				ConnectionSlot *slot = serverInterface->getSlot(i);

				if(	serverInterface->getSlot(i) != NULL && serverInterface->getSlot(i)->isConnected()) {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

					//printf("slot = %d serverInterface->getSlot(i)->getConnectedTime() = %d session key [%d]\n",i,serverInterface->getSlot(i)->getConnectedTime(),serverInterface->getSlot(i)->getSessionKey());

					if(clientConnectedTime == 0 ||
							(serverInterface->getSlot(i)->getConnectedTime() > 0 && serverInterface->getSlot(i)->getConnectedTime() < clientConnectedTime)) {
						clientConnectedTime = serverInterface->getSlot(i)->getConnectedTime();
						gameSettings->setMasterserver_admin(serverInterface->getSlot(i)->getSessionKey());
						gameSettings->setMasterserver_admin_faction_index(serverInterface->getSlot(i)->getPlayerIndex());
						labelGameName.setText(serverInterface->getSlot(i)->getName()+" controls");
						//printf("slot = %d, admin key [%d] slot connected time[%lu] clientConnectedTime [%lu]\n",i,gameSettings->getMasterserver_admin(),serverInterface->getSlot(i)->getConnectedTime(),clientConnectedTime);
					}
					if(serverInterface->getSlot(i)->getSessionKey() == gameSettings->getMasterserver_admin()){
						masterserver_admin_found=true;
					}
				}
			}
		}

		if(masterserver_admin_found == false)
		{
			labelGameName.setText("headless("+defaultPlayerName+")");
		}
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

void MenuStateCustomGame::saveGameSettingsToFile(std::string fileName) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	GameSettings gameSettings;
	loadGameSettings(&gameSettings);
	CoreData::getInstance().saveGameSettingsToFile(fileName, &gameSettings,listBoxAdvanced.getSelectedItemIndex());

//    Config &config = Config::getInstance();
//    string userData = config.getString("UserData_Root","");
//    if(userData != "") {
//    	endPathWithSlash(userData);
//    }
//    fileName = userData + fileName;
//
//    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] fileName = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,fileName.c_str());
//
//	GameSettings gameSettings;
//	loadGameSettings(&gameSettings);
//
//#if defined(WIN32) && !defined(__MINGW32__)
//	FILE *fp = _wfopen(utf8_decode(fileName).c_str(), L"w");
//	std::ofstream saveGameFile(fp);
//#else
//    std::ofstream saveGameFile;
//    saveGameFile.open(fileName.c_str(), ios_base::out | ios_base::trunc);
//#endif
//
//	//int factionCount= 0;
//	//ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();
//
//	saveGameFile << "Description=" << gameSettings.getDescription() << std::endl;
//
//	saveGameFile << "MapFilterIndex=" << gameSettings.getMapFilterIndex() << std::endl;
//	saveGameFile << "Map=" << gameSettings.getMap() << std::endl;
//	saveGameFile << "Tileset=" << gameSettings.getTileset() << std::endl;
//	saveGameFile << "TechTree=" << gameSettings.getTech() << std::endl;
//	saveGameFile << "DefaultUnits=" << gameSettings.getDefaultUnits() << std::endl;
//	saveGameFile << "DefaultResources=" << gameSettings.getDefaultResources() << std::endl;
//	saveGameFile << "DefaultVictoryConditions=" << gameSettings.getDefaultVictoryConditions() << std::endl;
//	saveGameFile << "FogOfWar=" << gameSettings.getFogOfWar() << std::endl;
//	saveGameFile << "AdvancedIndex=" << listBoxAdvanced.getSelectedItemIndex() << std::endl;
//
//	saveGameFile << "AllowObservers=" << gameSettings.getAllowObservers() << std::endl;
//
//	saveGameFile << "FlagTypes1=" << gameSettings.getFlagTypes1() << std::endl;
//
//	saveGameFile << "EnableObserverModeAtEndGame=" << gameSettings.getEnableObserverModeAtEndGame() << std::endl;
//
//	saveGameFile << "AiAcceptSwitchTeamPercentChance=" << gameSettings.getAiAcceptSwitchTeamPercentChance() << std::endl;
//
//	saveGameFile << "PathFinderType=" << gameSettings.getPathFinderType() << std::endl;
//	saveGameFile << "EnableServerControlledAI=" << gameSettings.getEnableServerControlledAI() << std::endl;
//	saveGameFile << "NetworkFramePeriod=" << gameSettings.getNetworkFramePeriod() << std::endl;
//	saveGameFile << "NetworkPauseGameForLaggedClients=" << gameSettings.getNetworkPauseGameForLaggedClients() << std::endl;
//
//	saveGameFile << "FactionThisFactionIndex=" << gameSettings.getThisFactionIndex() << std::endl;
//	saveGameFile << "FactionCount=" << gameSettings.getFactionCount() << std::endl;
//
//    //for(int i = 0; i < gameSettings.getFactionCount(); ++i) {
//	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
//		int slotIndex = gameSettings.getStartLocationIndex(i);
//
//		saveGameFile << "FactionControlForIndex" 		<< slotIndex << "=" << gameSettings.getFactionControl(i) << std::endl;
//		saveGameFile << "ResourceMultiplierIndex" 			<< slotIndex << "=" << gameSettings.getResourceMultiplierIndex(i) << std::endl;
//		saveGameFile << "FactionTeamForIndex" 			<< slotIndex << "=" << gameSettings.getTeam(i) << std::endl;
//		saveGameFile << "FactionStartLocationForIndex" 	<< slotIndex << "=" << gameSettings.getStartLocationIndex(i) << std::endl;
//		saveGameFile << "FactionTypeNameForIndex" 		<< slotIndex << "=" << gameSettings.getFactionTypeName(i) << std::endl;
//		saveGameFile << "FactionPlayerNameForIndex" 	<< slotIndex << "=" << gameSettings.getNetworkPlayerName(i) << std::endl;
//    }
//
//#if defined(WIN32) && !defined(__MINGW32__)
//	fclose(fp);
//#endif
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

GameSettings MenuStateCustomGame::loadGameSettingsFromFile(std::string fileName) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

    GameSettings gameSettings;

    GameSettings originalGameSettings;
	loadGameSettings(&originalGameSettings);

    try {
    	CoreData::getInstance().loadGameSettingsFromFile(fileName, &gameSettings);
		setupUIFromGameSettings(gameSettings);
	}
    catch(const exception &ex) {
    	SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
    	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] ERROR = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());

    	showMessageBox( ex.what(), "Error", false);

    	setupUIFromGameSettings(originalGameSettings);
    	gameSettings = originalGameSettings;
    }

    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	return gameSettings;
}

void MenuStateCustomGame::setupUIFromGameSettings(const GameSettings &gameSettings) {
	string scenarioDir = "";
	checkBoxScenario.setValue((gameSettings.getScenario() != ""));
	if(checkBoxScenario.getValue() == true) {
		listBoxScenario.setSelectedItem(formatString(gameSettings.getScenario()));

		loadScenarioInfo(Scenario::getScenarioPath(dirList, scenarioFiles[listBoxScenario.getSelectedItemIndex()]), &scenarioInfo);
		scenarioDir = Scenario::getScenarioDir(dirList, gameSettings.getScenario());

		//printf("scenarioInfo.fogOfWar = %d scenarioInfo.fogOfWar_exploredFlag = %d\n",scenarioInfo.fogOfWar,scenarioInfo.fogOfWar_exploredFlag);
		if(scenarioInfo.fogOfWar == false && scenarioInfo.fogOfWar_exploredFlag == false) {
			listBoxFogOfWar.setSelectedItemIndex(2);
		}
		else if(scenarioInfo.fogOfWar_exploredFlag == true) {
			listBoxFogOfWar.setSelectedItemIndex(1);
		}
		else {
			listBoxFogOfWar.setSelectedItemIndex(0);
		}
	}
	setupMapList(gameSettings.getScenario());
	setupTechList(gameSettings.getScenario());
	setupTilesetList(gameSettings.getScenario());

	if(checkBoxScenario.getValue() == true) {
		//string file = Scenario::getScenarioPath(dirList, gameSettings.getScenario());
		//loadScenarioInfo(file, &scenarioInfo);

		//printf("#6.1 about to load map [%s]\n",scenarioInfo.mapName.c_str());
		//loadMapInfo(Map::getMapPath(scenarioInfo.mapName, scenarioDir, true), &mapInfo, false);
		//printf("#6.2\n");

		listBoxMapFilter.setSelectedItemIndex(0);
		listBoxMap.setItems(formattedPlayerSortedMaps[mapInfo.players]);
		listBoxMap.setSelectedItem(formatString(scenarioInfo.mapName));
	}
	else {
		if(gameSettings.getMapFilterIndex() == 0) {
			listBoxMapFilter.setSelectedItemIndex(0);
		}
		else {
			listBoxMapFilter.setSelectedItem(intToStr(gameSettings.getMapFilterIndex()));
		}
		listBoxMap.setItems(formattedPlayerSortedMaps[gameSettings.getMapFilterIndex()]);
	}

	//printf("gameSettings.getMap() [%s] [%s]\n",gameSettings.getMap().c_str(),listBoxMap.getSelectedItem().c_str());

	string mapFile = gameSettings.getMap();
	if(find(mapFiles.begin(),mapFiles.end(),mapFile) != mapFiles.end()) {
		mapFile = formatString(mapFile);
		listBoxMap.setSelectedItem(mapFile);

		loadMapInfo(Map::getMapPath(getCurrentMapFile(),scenarioDir,true), &mapInfo, true);
		labelMapInfo.setText(mapInfo.desc);
	}

	string tilesetFile = gameSettings.getTileset();
	if(find(tilesetFiles.begin(),tilesetFiles.end(),tilesetFile) != tilesetFiles.end()) {
		tilesetFile = formatString(tilesetFile);
		listBoxTileset.setSelectedItem(tilesetFile);
	}

	string techtreeFile = gameSettings.getTech();
	if(find(techTreeFiles.begin(),techTreeFiles.end(),techtreeFile) != techTreeFiles.end()) {
		techtreeFile = formatString(techtreeFile);
		listBoxTechTree.setSelectedItem(techtreeFile);
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	//gameSettings->setDefaultUnits(true);
	//gameSettings->setDefaultResources(true);
	//gameSettings->setDefaultVictoryConditions(true);

	Lang &lang= Lang::getInstance();

	//FogOfWar
	if(checkBoxScenario.getValue() == false) {
		listBoxFogOfWar.setSelectedItemIndex(0); // default is 0!
		if(gameSettings.getFogOfWar() == false){
			listBoxFogOfWar.setSelectedItemIndex(2);
		}

		if((gameSettings.getFlagTypes1() & ft1_show_map_resources) == ft1_show_map_resources){
			if(gameSettings.getFogOfWar() == true){
				listBoxFogOfWar.setSelectedItemIndex(1);
			}
		}
	}

	//printf("In [%s::%s line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	listBoxAllowObservers.setSelectedItem(gameSettings.getAllowObservers() == true ? lang.get("Yes") : lang.get("No"));
	//listBoxEnableObserverMode.setSelectedItem(gameSettings.getEnableObserverModeAtEndGame() == true ? lang.get("Yes") : lang.get("No"));

	listBoxEnableSwitchTeamMode.setSelectedItem((gameSettings.getFlagTypes1() & ft1_allow_team_switching) == ft1_allow_team_switching ? lang.get("Yes") : lang.get("No"));
	listBoxAISwitchTeamAcceptPercent.setSelectedItem(intToStr(gameSettings.getAiAcceptSwitchTeamPercentChance()));

	listBoxPathFinderType.setSelectedItemIndex(gameSettings.getPathFinderType());

	//listBoxEnableServerControlledAI.setSelectedItem(gameSettings.getEnableServerControlledAI() == true ? lang.get("Yes") : lang.get("No"));

	//labelNetworkFramePeriod.setText(lang.get("NetworkFramePeriod"));

	//listBoxNetworkFramePeriod.setSelectedItem(intToStr(gameSettings.getNetworkFramePeriod()/10*10));

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	listBoxNetworkPauseGameForLaggedClients.setSelectedItemIndex(gameSettings.getNetworkPauseGameForLaggedClients());

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	reloadFactions(false,(checkBoxScenario.getValue() == true ? scenarioFiles[listBoxScenario.getSelectedItemIndex()] : ""));
	//reloadFactions(true);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d] gameSettings.getFactionCount() = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,gameSettings.getFactionCount());

	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		int slotIndex = gameSettings.getStartLocationIndex(i);
		if(gameSettings.getFactionControl(i) < listBoxControls[slotIndex].getItemCount()) {
			listBoxControls[slotIndex].setSelectedItemIndex(gameSettings.getFactionControl(i));
		}

		updateResourceMultiplier(slotIndex);
		listBoxRMultiplier[slotIndex].setSelectedItemIndex(gameSettings.getResourceMultiplierIndex(i));

		listBoxTeams[slotIndex].setSelectedItemIndex(gameSettings.getTeam(i));

		lastSelectedTeamIndex[slotIndex] = listBoxTeams[slotIndex].getSelectedItemIndex();

		string factionName = gameSettings.getFactionTypeName(i);
		factionName = formatString(factionName);

		//printf("\n\n\n*** setupUIFromGameSettings A, i = %d, startLoc = %d, factioncontrol = %d, factionName [%s]\n",i,gameSettings.getStartLocationIndex(i),gameSettings.getFactionControl(i),factionName.c_str());

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] factionName = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,factionName.c_str());

		if(listBoxFactions[slotIndex].hasItem(factionName) == true) {
			listBoxFactions[slotIndex].setSelectedItem(factionName);
		}
		else {
			listBoxFactions[slotIndex].setSelectedItem(formatString(GameConstants::RANDOMFACTION_SLOTNAME));
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] i = %d, gameSettings.getNetworkPlayerName(i) [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,i,gameSettings.getNetworkPlayerName(i).c_str());

		labelPlayerNames[slotIndex].setText(gameSettings.getNetworkPlayerName(i));
	}

	//SetActivePlayerNameEditor();

	updateControlers();
	updateNetworkSlots();

	//if(listBoxPublishServer.getSelectedItemIndex() == 0) {
	//	needToRepublishToMasterserver = true;
	//}

	if(hasNetworkGameSettings() == true)
	{
		needToSetChangedGameSettings = true;
		lastSetChangedGameSettings   = time(NULL);
	}
}
// ============ PRIVATE ===========================

bool MenuStateCustomGame::hasNetworkGameSettings() {
    bool hasNetworkSlot = false;

    try {
		for(int i=0; i<mapInfo.players; ++i) {
			ControlType ct= static_cast<ControlType>(listBoxControls[i].getSelectedItemIndex());
			if(ct != ctClosed) {
				if(ct == ctNetwork || ct == ctNetworkUnassigned) {
					hasNetworkSlot = true;
					break;
				}
			}
		}
		if(hasNetworkSlot == false) {
			for(int i=0; i < GameConstants::maxPlayers; ++i) {
				ControlType ct= static_cast<ControlType>(listBoxControls[i].getSelectedItemIndex());
				if(ct != ctClosed) {
					if(ct == ctNetworkUnassigned) {
						hasNetworkSlot = true;
						break;
					}
				}
			}
		}
    }
	catch(const std::exception &ex) {
		char szBuf[8096]="";
		sprintf(szBuf,"In [%s::%s %d]\nError detected:\n%s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);

		showGeneralError=true;
		generalErrorToShow = szBuf;
	}

    return hasNetworkSlot;
}

void MenuStateCustomGame::loadMapInfo(string file, MapInfo *mapInfo, bool loadMapPreview) {
	try {
		Lang &lang= Lang::getInstance();
		if(MapPreview::loadMapInfo(file, mapInfo, lang.get("MaxPlayers"),lang.get("Size"),true) == true) {
			ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();
			for(int i = 0; i < GameConstants::maxPlayers; ++i) {
				if(serverInterface->getSlot(i) != NULL &&
					(listBoxControls[i].getSelectedItemIndex() == ctNetwork ||
					listBoxControls[i].getSelectedItemIndex() == ctNetworkUnassigned)) {
					if(serverInterface->getSlot(i)->isConnected() == true) {
						if(i+1 > mapInfo->players &&
							listBoxControls[i].getSelectedItemIndex() != ctNetworkUnassigned) {
							listBoxControls[i].setSelectedItemIndex(ctNetworkUnassigned);
						}
					}
				}

				labelPlayers[i].setVisible(i+1 <= mapInfo->players);
				labelPlayerNames[i].setVisible(i+1 <= mapInfo->players);
				listBoxControls[i].setVisible(i+1 <= mapInfo->players);
				listBoxFactions[i].setVisible(i+1 <= mapInfo->players);
				listBoxTeams[i].setVisible(i+1 <= mapInfo->players);
				labelNetStatus[i].setVisible(i+1 <= mapInfo->players);
			}

			// Not painting properly so this is on hold
			if(loadMapPreview == true) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

				mapPreview.loadFromFile(file.c_str());

				//printf("Loading map preview MAP\n");
				cleanupMapPreviewTexture();
			}
		}
	}
	catch(exception &e) {
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s] loading map [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,e.what(),file.c_str());
		throw megaglest_runtime_error("Error loading map file: [" + file + "] msg: " + e.what());
	}
}

void MenuStateCustomGame::updateControlers() {
	try {
		bool humanPlayer= false;

		for(int i = 0; i < mapInfo.players; ++i) {
			if(listBoxControls[i].getSelectedItemIndex() == ctHuman) {
				humanPlayer= true;
			}
		}

		if(humanPlayer == false) {
			if(this->headlessServerMode == false) {
				bool foundNewSlotForHuman = false;
				for(int i = 0; i < mapInfo.players; ++i) {
					if(listBoxControls[i].getSelectedItemIndex() == ctClosed) {
						listBoxControls[i].setSelectedItemIndex(ctHuman);
						labelPlayerNames[i].setText("");
						labelPlayerNames[i].setText(getHumanPlayerName());

						foundNewSlotForHuman = true;
						break;
					}
				}

				if(foundNewSlotForHuman == false) {
					for(int i = 0; i < mapInfo.players; ++i) {
						if(listBoxControls[i].getSelectedItemIndex() == ctClosed ||
							listBoxControls[i].getSelectedItemIndex() == ctCpuEasy ||
							listBoxControls[i].getSelectedItemIndex() == ctCpu ||
							listBoxControls[i].getSelectedItemIndex() == ctCpuUltra ||
							listBoxControls[i].getSelectedItemIndex() == ctCpuMega) {
							listBoxControls[i].setSelectedItemIndex(ctHuman);
							labelPlayerNames[i].setText("");
							labelPlayerNames[i].setText(getHumanPlayerName());

							foundNewSlotForHuman = true;
							break;
						}
					}
				}

				if(foundNewSlotForHuman == false) {
					ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();
					ConnectionSlot *slot = serverInterface->getSlot(0);
					if(slot != NULL && slot->isConnected() == true) {
						serverInterface->removeSlot(0);
					}
					listBoxControls[0].setSelectedItemIndex(ctHuman);
					labelPlayerNames[0].setText("");
					labelPlayerNames[0].setText(getHumanPlayerName());
				}
			}
		}

		for(int i= mapInfo.players; i < GameConstants::maxPlayers; ++i) {
			if( listBoxControls[i].getSelectedItemIndex() != ctNetwork &&
				listBoxControls[i].getSelectedItemIndex() != ctNetworkUnassigned) {
				//printf("Closed A [%d] [%s]\n",i,labelPlayerNames[i].getText().c_str());

				listBoxControls[i].setSelectedItemIndex(ctClosed);
			}
		}
	}
	catch(const std::exception &ex) {
		char szBuf[8096]="";
		sprintf(szBuf,"In [%s::%s %d]\nError detected:\n%s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		throw megaglest_runtime_error(szBuf);
	}
}

void MenuStateCustomGame::closeUnusedSlots(){
	try {
		if(checkBoxScenario.getValue() == false) {
			ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();
			for(int i= 0; i<mapInfo.players; ++i){
				if(listBoxControls[i].getSelectedItemIndex() == ctNetwork ||
						listBoxControls[i].getSelectedItemIndex() == ctNetworkUnassigned) {
					if(serverInterface->getSlot(i) == NULL ||
					   serverInterface->getSlot(i)->isConnected() == false) {
						//printf("Closed A [%d] [%s]\n",i,labelPlayerNames[i].getText().c_str());

						listBoxControls[i].setSelectedItemIndex(ctClosed);
					}
				}
			}
			updateNetworkSlots();
		}
	}
	catch(const std::exception &ex) {
		char szBuf[8096]="";
		sprintf(szBuf,"In [%s::%s %d]\nError detected:\n%s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		throw megaglest_runtime_error(szBuf);
	}
}

void MenuStateCustomGame::updateNetworkSlots() {
	try {
		ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();

        if(hasNetworkGameSettings() == true) {
            if(hasCheckedForUPNP == false) {
                hasCheckedForUPNP = true;

                serverInterface->getServerSocket()->NETdiscoverUPnPDevices();
            }
        }
        else {
            hasCheckedForUPNP = false;
        }

		for(int i= 0; i < GameConstants::maxPlayers; ++i) {
			if(serverInterface->getSlot(i) == NULL &&
				listBoxControls[i].getSelectedItemIndex() == ctNetwork)	{
				try {
					serverInterface->addSlot(i);
				}
				catch(const std::exception &ex) {
					char szBuf[4096]="";
					sprintf(szBuf,"In [%s::%s %d]\nError detected:\n%s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
					SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
					if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);
					showGeneralError=true;
					if(serverInterface->isPortBound() == false) {
						generalErrorToShow = Lang::getInstance().get("ErrorBindingPort") + " : " + intToStr(serverInterface->getBindPort());
					}
					else {
						generalErrorToShow = ex.what();
					}

					// Revert network to CPU
					listBoxControls[i].setSelectedItemIndex(ctCpu);
				}
			}
			ConnectionSlot *slot = serverInterface->getSlot(i);
			if(slot != NULL) {
				if(listBoxControls[i].getSelectedItemIndex() != ctNetwork) {
					if(slot->getCanAcceptConnections() == true) {
						slot->setCanAcceptConnections(false);
					}
					if(slot->isConnected() == true) {
						if(listBoxControls[i].getSelectedItemIndex() != ctNetworkUnassigned) {
							listBoxControls[i].setSelectedItemIndex(ctNetworkUnassigned);
						}
					}
					else {
						serverInterface->removeSlot(i);

						if(listBoxControls[i].getSelectedItemIndex() == ctNetworkUnassigned) {
							listBoxControls[i].setSelectedItemIndex(ctClosed);
						}
					}
				}
				else if(slot->getCanAcceptConnections() == false) {
					slot->setCanAcceptConnections(true);
				}
			}
		}
	}
	catch(const std::exception &ex) {
		char szBuf[8096]="";
		sprintf(szBuf,"In [%s::%s %d]\nError detected:\n%s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);
		//throw megaglest_runtime_error(szBuf);
		showGeneralError=true;
		generalErrorToShow = szBuf;

	}
}

void MenuStateCustomGame::keyDown(SDL_KeyboardEvent key) {
	if(isMasterserverMode() == true) {
		return;
	}

	if(activeInputLabel != NULL) {
		bool handled = keyDownEditLabel(key, &activeInputLabel);
		if(handled == true) {
			MutexSafeWrapper safeMutex((publishToMasterserverThread != NULL ? publishToMasterserverThread->getMutexThreadObjectAccessor() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
	        if(hasNetworkGameSettings() == true) {
	            needToSetChangedGameSettings = true;
	            lastSetChangedGameSettings   = time(NULL);
	        }
		}
	}
	else {
		//send key to the chat manager
		if(hasNetworkGameSettings() == true) {
			chatManager.keyDown(key);
		}
		if(chatManager.getEditEnabled() == false) {
			Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));

			//if(key == configKeys.getCharKey("ShowFullConsole")) {
			if(isKeyPressed(configKeys.getSDLKey("ShowFullConsole"),key) == true) {
				showFullConsole= true;
			}
			//Toggle music
			//else if(key == configKeys.getCharKey("ToggleMusic")) {
			else if(isKeyPressed(configKeys.getSDLKey("ToggleMusic"),key) == true) {
				Config &config = Config::getInstance();
				Lang &lang= Lang::getInstance();

				float configVolume = (config.getInt("SoundVolumeMusic") / 100.f);
				float currentVolume = CoreData::getInstance().getMenuMusic()->getVolume();
				if(currentVolume > 0) {
					CoreData::getInstance().getMenuMusic()->setVolume(0.f);
					console.addLine(lang.get("GameMusic") + " " + lang.get("Off"));
				}
				else {
					CoreData::getInstance().getMenuMusic()->setVolume(configVolume);
					//If the config says zero, use the default music volume
					//gameMusic->setVolume(configVolume ? configVolume : 0.9);
					console.addLine(lang.get("GameMusic"));
				}
			}
			//else if(key == configKeys.getCharKey("SaveGUILayout")) {
			else if(isKeyPressed(configKeys.getSDLKey("SaveGUILayout"),key) == true) {
				bool saved = GraphicComponent::saveAllCustomProperties(containerName);
				Lang &lang= Lang::getInstance();
				console.addLine(lang.get("GUILayoutSaved") + " [" + (saved ? lang.get("Yes") : lang.get("No"))+ "]");
			}
		}
	}
}

void MenuStateCustomGame::keyPress(SDL_KeyboardEvent c) {
	if(isMasterserverMode() == true) {
		return;
	}

	if(activeInputLabel != NULL) {
		bool handled = keyPressEditLabel(c, &activeInputLabel);
		if(handled == true && &labelGameName != activeInputLabel) {
			MutexSafeWrapper safeMutex((publishToMasterserverThread != NULL ? publishToMasterserverThread->getMutexThreadObjectAccessor() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
			if(hasNetworkGameSettings() == true) {
				needToSetChangedGameSettings = true;
				lastSetChangedGameSettings   = time(NULL);
			}
		}
	}
	else {
		if(hasNetworkGameSettings() == true) {
			chatManager.keyPress(c);
		}
	}
}

void MenuStateCustomGame::keyUp(SDL_KeyboardEvent key) {
	if(isMasterserverMode() == true) {
		return;
	}

	if(activeInputLabel==NULL) {
		if(hasNetworkGameSettings() == true) {
			chatManager.keyUp(key);
		}
		Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));

		if(chatManager.getEditEnabled()) {
			//send key to the chat manager
			if(hasNetworkGameSettings() == true) {
				chatManager.keyUp(key);
			}
		}
		//else if(key == configKeys.getCharKey("ShowFullConsole")) {
		else if(isKeyPressed(configKeys.getSDLKey("ShowFullConsole"),key) == true) {
			showFullConsole= false;
		}
	}
}

void MenuStateCustomGame::showMessageBox(const string &text, const string &header, bool toggle){
	if(!toggle){
		mainMessageBox.setEnabled(false);
	}

	if(!mainMessageBox.getEnabled()){
		mainMessageBox.setText(text);
		mainMessageBox.setHeader(header);
		mainMessageBox.setEnabled(true);
	}
	else{
		mainMessageBox.setEnabled(false);
	}
}

void MenuStateCustomGame::switchToNextMapGroup(const int direction){
	int i=listBoxMapFilter.getSelectedItemIndex();
	// if there are no maps for the current selection we switch to next selection
	while(formattedPlayerSortedMaps[i].empty()){
		i=i+direction;
		if(i>GameConstants::maxPlayers){
			i=0;
		}
		if(i<0){
			i=GameConstants::maxPlayers;
		}
	}
	listBoxMapFilter.setSelectedItemIndex(i);
	listBoxMap.setItems(formattedPlayerSortedMaps[i]);
}

string MenuStateCustomGame::getCurrentMapFile(){
	int i=listBoxMapFilter.getSelectedItemIndex();
	int mapIndex=listBoxMap.getSelectedItemIndex();
	return playerSortedMaps[i].at(mapIndex);
}

void MenuStateCustomGame::setActiveInputLabel(GraphicLabel *newLable) {
	MenuState::setActiveInputLabel(newLable,&activeInputLabel);
}

string MenuStateCustomGame::getHumanPlayerName(int index) {
	string  result = defaultPlayerName;
	if(index < 0) {
		for(int j = 0; j < GameConstants::maxPlayers; ++j) {
			if(listBoxControls[j].getSelectedItemIndex() >= 0) {
				ControlType ct = static_cast<ControlType>(listBoxControls[j].getSelectedItemIndex());
				if(ct == ctHuman) {
					index = j;
					break;
				}
			}
		}
	}

	if(index >= 0 && index < GameConstants::maxPlayers &&
		labelPlayerNames[index].getText() != "" &&
		labelPlayerNames[index].getText() !=  GameConstants::NETWORK_SLOT_UNCONNECTED_SLOTNAME) {
		result = labelPlayerNames[index].getText();

		if(activeInputLabel != NULL) {
			size_t found = result.find_last_of("_");
			if (found != string::npos) {
				result = result.substr(0,found);
			}
		}
	}

	return result;
}

void MenuStateCustomGame::loadFactionTexture(string filepath) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	if(enableFactionTexturePreview == true) {
		if(filepath == "") {
			factionTexture = NULL;
		}
		else {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] filepath = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,filepath.c_str());

			factionTexture = Renderer::findFactionLogoTexture(filepath);

			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		}
	}
}

void MenuStateCustomGame::cleanupMapPreviewTexture() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	//printf("CLEANUP map preview texture\n");

	if(mapPreviewTexture != NULL) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		mapPreviewTexture->end();

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		delete mapPreviewTexture;
		mapPreviewTexture = NULL;
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

int32 MenuStateCustomGame::getNetworkPlayerStatus() {
	int32 result = npst_None;
	switch(listBoxPlayerStatus.getSelectedItemIndex()) {
		case 2:
			result = npst_Ready;
			break;
		case 1:
			result = npst_BeRightBack;
			break;
		case 0:
		default:
			result = npst_PickSettings;
			break;
	}

	return result;
}

void MenuStateCustomGame::loadScenarioInfo(string file, ScenarioInfo *scenarioInfo) {
	//printf("Load scenario file [%s]\n",file.c_str());
	Scenario::loadScenarioInfo(file, scenarioInfo);

	//cleanupPreviewTexture();
	previewLoadDelayTimer=time(NULL);
	needToLoadTextures=true;
}

bool MenuStateCustomGame::isInSpecialKeyCaptureEvent() {
	bool result = (chatManager.getEditEnabled() || activeInputLabel != NULL);
	return result;
}

void MenuStateCustomGame::processScenario() {
	try {
		if(checkBoxScenario.getValue() == true) {
			//printf("listBoxScenario.getSelectedItemIndex() = %d [%s] scenarioFiles.size() = %d\n",listBoxScenario.getSelectedItemIndex(),listBoxScenario.getSelectedItem().c_str(),scenarioFiles.size());
			loadScenarioInfo(Scenario::getScenarioPath(dirList, scenarioFiles[listBoxScenario.getSelectedItemIndex()]), &scenarioInfo);
			string scenarioDir = Scenario::getScenarioDir(dirList, scenarioInfo.name);

			//printf("scenarioInfo.fogOfWar = %d scenarioInfo.fogOfWar_exploredFlag = %d\n",scenarioInfo.fogOfWar,scenarioInfo.fogOfWar_exploredFlag);
			if(scenarioInfo.fogOfWar == false && scenarioInfo.fogOfWar_exploredFlag == false) {
				listBoxFogOfWar.setSelectedItemIndex(2);
			}
			else if(scenarioInfo.fogOfWar_exploredFlag == true) {
				listBoxFogOfWar.setSelectedItemIndex(1);
			}
			else {
				listBoxFogOfWar.setSelectedItemIndex(0);
			}

			setupTechList(scenarioInfo.name);
			listBoxTechTree.setSelectedItem(formatString(scenarioInfo.techTreeName));
			reloadFactions(false,scenarioInfo.name);

			setupTilesetList(scenarioInfo.name);
			listBoxTileset.setSelectedItem(formatString(scenarioInfo.tilesetName));

			setupMapList(scenarioInfo.name);
			listBoxMap.setSelectedItem(formatString(scenarioInfo.mapName));
			loadMapInfo(Map::getMapPath(getCurrentMapFile(),scenarioDir,true), &mapInfo, true);
			labelMapInfo.setText(mapInfo.desc);

			//printf("scenarioInfo.name [%s] [%s]\n",scenarioInfo.name.c_str(),listBoxMap.getSelectedItem().c_str());

			// Loop twice to set the human slot or else it closes network slots in some cases
			for(int humanIndex = 0; humanIndex < 2; ++humanIndex) {
				for(int i = 0; i < mapInfo.players; ++i) {
					listBoxRMultiplier[i].setSelectedItem(floatToStr(scenarioInfo.resourceMultipliers[i],1));

					ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();
					ConnectionSlot *slot = serverInterface->getSlot(i);

					int selectedControlItemIndex = listBoxControls[i].getSelectedItemIndex();
					if(selectedControlItemIndex != ctNetwork ||
						(selectedControlItemIndex == ctNetwork && (slot == NULL || slot->isConnected() == false))) {
					}

					listBoxControls[i].setSelectedItemIndex(scenarioInfo.factionControls[i]);
					if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

					// Skip over networkunassigned
					//if(listBoxControls[i].getSelectedItemIndex() == ctNetworkUnassigned &&
					//	selectedControlItemIndex != ctNetworkUnassigned) {
					//	listBoxControls[i].mouseClick(x, y);
					//}

					//look for human players
					int humanIndex1= -1;
					int humanIndex2= -1;
					for(int j = 0; j < GameConstants::maxPlayers; ++j) {
						ControlType ct= static_cast<ControlType>(listBoxControls[j].getSelectedItemIndex());
						if(ct == ctHuman) {
							if(humanIndex1 == -1) {
								humanIndex1= j;
							}
							else {
								humanIndex2= j;
							}
						}
					}

					if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] humanIndex1 = %d, humanIndex2 = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,humanIndex1,humanIndex2);

					//no human
					if(humanIndex1 == -1 && humanIndex2 == -1) {
						listBoxControls[i].setSelectedItemIndex(ctHuman);
						if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] i = %d, labelPlayerNames[i].getText() [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,i,labelPlayerNames[i].getText().c_str());

						//printf("humanIndex1 = %d humanIndex2 = %d i = %d listBoxControls[i].getSelectedItemIndex() = %d\n",humanIndex1,humanIndex2,i,listBoxControls[i].getSelectedItemIndex());
					}
					//2 humans
					else if(humanIndex1 != -1 && humanIndex2 != -1) {
						int closeSlotIndex = (humanIndex1 == i ? humanIndex2: humanIndex1);
						int humanSlotIndex = (closeSlotIndex == humanIndex1 ? humanIndex2 : humanIndex1);

						string origPlayName = labelPlayerNames[closeSlotIndex].getText();

						if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] closeSlotIndex = %d, origPlayName [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,closeSlotIndex,origPlayName.c_str());
						//printf("humanIndex1 = %d humanIndex2 = %d i = %d closeSlotIndex = %d humanSlotIndex = %d\n",humanIndex1,humanIndex2,i,closeSlotIndex,humanSlotIndex);

						listBoxControls[closeSlotIndex].setSelectedItemIndex(ctClosed);
						labelPlayerNames[humanSlotIndex].setText((origPlayName != "" ? origPlayName : getHumanPlayerName()));
					}

					ControlType ct= static_cast<ControlType>(listBoxControls[i].getSelectedItemIndex());
					if(ct != ctClosed) {
						//updateNetworkSlots();
						//updateResourceMultiplier(i);
						updateResourceMultiplier(i);

						//printf("Setting scenario faction i = %d [ %s]\n",i,scenarioInfo.factionTypeNames[i].c_str());
						listBoxFactions[i].setSelectedItem(formatString(scenarioInfo.factionTypeNames[i]));
						//printf("DONE Setting scenario faction i = %d [ %s]\n",i,scenarioInfo.factionTypeNames[i].c_str());

						// Disallow CPU players to be observers
						if(factionFiles[listBoxFactions[i].getSelectedItemIndex()] == formatString(GameConstants::OBSERVER_SLOTNAME) &&
							(listBoxControls[i].getSelectedItemIndex() == ctCpuEasy || listBoxControls[i].getSelectedItemIndex() == ctCpu ||
							 listBoxControls[i].getSelectedItemIndex() == ctCpuUltra || listBoxControls[i].getSelectedItemIndex() == ctCpuMega)) {
							listBoxFactions[i].setSelectedItemIndex(0);
						}
						//

						listBoxTeams[i].setSelectedItem(intToStr(scenarioInfo.teams[i]));
						if(factionFiles[listBoxFactions[i].getSelectedItemIndex()] != formatString(GameConstants::OBSERVER_SLOTNAME)) {
							if(listBoxTeams[i].getSelectedItemIndex() + 1 != (GameConstants::maxPlayers + fpt_Observer)) {
								lastSelectedTeamIndex[i] = listBoxTeams[i].getSelectedItemIndex();
							}
						}
						else {
							lastSelectedTeamIndex[i] = -1;
						}
					}

					if(listBoxPublishServer.getSelectedItemIndex() == 0) {
						needToRepublishToMasterserver = true;
					}

					if(hasNetworkGameSettings() == true) {
						needToSetChangedGameSettings = true;
						lastSetChangedGameSettings   = time(NULL);;
					}
				}
			}

			updateControlers();
			updateNetworkSlots();

			MutexSafeWrapper safeMutex((publishToMasterserverThread != NULL ? publishToMasterserverThread->getMutexThreadObjectAccessor() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
			if(listBoxPublishServer.getSelectedItemIndex() == 0) {
				needToRepublishToMasterserver = true;
			}
			if(hasNetworkGameSettings() == true) {
				needToSetChangedGameSettings = true;
				lastSetChangedGameSettings   = time(NULL);
			}

			//labelInfo.setText(scenarioInfo.desc);
		}
		else {
			setupMapList("");
			listBoxMap.setSelectedItem(formatString(formattedPlayerSortedMaps[0][0]));
			loadMapInfo(Map::getMapPath(getCurrentMapFile(),"",true), &mapInfo, true);
			labelMapInfo.setText(mapInfo.desc);

			setupTechList("");
			reloadFactions(false,"");
			setupTilesetList("");
		}
		SetupUIForScenarios();
	}
	catch(const std::exception &ex) {
		char szBuf[8096]="";
		sprintf(szBuf,"In [%s::%s %d]\nError detected:\n%s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);

		mainMessageBoxState=1;
		showMessageBox( szBuf, "Error detected", false);
	}
}

void MenuStateCustomGame::SetupUIForScenarios() {
	try {
		if(checkBoxScenario.getValue() == true) {
			// START - Disable changes to controls while in Scenario mode
			for(int i = 0; i < GameConstants::maxPlayers; ++i) {
				listBoxControls[i].setEditable(false);
				listBoxFactions[i].setEditable(false);
				listBoxRMultiplier[i].setEditable(false);
				listBoxTeams[i].setEditable(false);
			}
			listBoxFogOfWar.setEditable(false);
			listBoxAllowObservers.setEditable(false);
			listBoxPathFinderType.setEditable(false);
			listBoxEnableSwitchTeamMode.setEditable(false);
			listBoxAISwitchTeamAcceptPercent.setEditable(false);
			listBoxMap.setEditable(false);
			listBoxTileset.setEditable(false);
			listBoxMapFilter.setEditable(false);
			listBoxTechTree.setEditable(false);
			// END - Disable changes to controls while in Scenario mode
		}
		else {
			// START - Disable changes to controls while in Scenario mode
			for(int i = 0; i < GameConstants::maxPlayers; ++i) {
				listBoxControls[i].setEditable(true);
				listBoxFactions[i].setEditable(true);
				listBoxRMultiplier[i].setEditable(true);
				listBoxTeams[i].setEditable(true);
			}
			listBoxFogOfWar.setEditable(true);
			listBoxAllowObservers.setEditable(true);
			listBoxPathFinderType.setEditable(true);
			listBoxEnableSwitchTeamMode.setEditable(true);
			listBoxAISwitchTeamAcceptPercent.setEditable(true);
			listBoxMap.setEditable(true);
			listBoxTileset.setEditable(true);
			listBoxMapFilter.setEditable(true);
			listBoxTechTree.setEditable(true);
			// END - Disable changes to controls while in Scenario mode
		}
	}
	catch(const std::exception &ex) {
		char szBuf[8096]="";
		sprintf(szBuf,"In [%s::%s %d]\nError detected:\n%s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);

		throw megaglest_runtime_error(szBuf);
	}

}

int MenuStateCustomGame::setupMapList(string scenario) {
	int initialMapSelection = 0;

	try {
		Config &config = Config::getInstance();
		vector<string> invalidMapList;
		string scenarioDir = Scenario::getScenarioDir(dirList, scenario);
		vector<string> pathList = config.getPathListForType(ptMaps,scenarioDir);
		vector<string> allMaps = MapPreview::findAllValidMaps(pathList,scenarioDir,false,true,&invalidMapList);

		if(scenario != "") {
			vector<string> allMaps2 = MapPreview::findAllValidMaps(config.getPathListForType(ptMaps,""),"",false,true,&invalidMapList);
			copy(allMaps2.begin(), allMaps2.end(), std::inserter(allMaps, allMaps.begin()));
			std::sort(allMaps.begin(),allMaps.end());
		}

		if (allMaps.empty()) {
			throw megaglest_runtime_error("No maps were found!");
		}
		vector<string> results;
		copy(allMaps.begin(), allMaps.end(), std::back_inserter(results));
		mapFiles = results;

		for(unsigned int i = 0; i < GameConstants::maxPlayers+1; ++i) {
			playerSortedMaps[i].clear();
			formattedPlayerSortedMaps[i].clear();
		}

		copy(mapFiles.begin(), mapFiles.end(), std::back_inserter(playerSortedMaps[0]));
		copy(playerSortedMaps[0].begin(), playerSortedMaps[0].end(), std::back_inserter(formattedPlayerSortedMaps[0]));
		std::for_each(formattedPlayerSortedMaps[0].begin(), formattedPlayerSortedMaps[0].end(), FormatString());
		//printf("#5\n");

		for(int i= 0; i < mapFiles.size(); i++){// fetch info and put map in right list
			loadMapInfo(Map::getMapPath(mapFiles.at(i), scenarioDir, false), &mapInfo, false);

			if(GameConstants::maxPlayers+1 <= mapInfo.players) {
				char szBuf[1024]="";
				sprintf(szBuf,"Sorted map list [%d] does not match\ncurrent map playercount [%d]\nfor file [%s]\nmap [%s]",GameConstants::maxPlayers+1,mapInfo.players,Map::getMapPath(mapFiles.at(i), "", false).c_str(),mapInfo.desc.c_str());
				throw megaglest_runtime_error(szBuf);
			}
			playerSortedMaps[mapInfo.players].push_back(mapFiles.at(i));
			formattedPlayerSortedMaps[mapInfo.players].push_back(formatString(mapFiles.at(i)));
			if(config.getString("InitialMap", "Conflict") == formattedPlayerSortedMaps[mapInfo.players].back()){
				initialMapSelection= i;
			}
		}

		//printf("#6 scenario [%s] [%s]\n",scenario.c_str(),scenarioDir.c_str());
		if(scenario != "") {
			string file = Scenario::getScenarioPath(dirList, scenario);
			loadScenarioInfo(file, &scenarioInfo);

			//printf("#6.1 about to load map [%s]\n",scenarioInfo.mapName.c_str());
			loadMapInfo(Map::getMapPath(scenarioInfo.mapName, scenarioDir, true), &mapInfo, false);
			//printf("#6.2\n");
			listBoxMapFilter.setSelectedItem(intToStr(mapInfo.players));
			listBoxMap.setItems(formattedPlayerSortedMaps[mapInfo.players]);
		}
		else {
			listBoxMapFilter.setSelectedItemIndex(0);
			listBoxMap.setItems(formattedPlayerSortedMaps[0]);
		}
		//printf("#7\n");
	}
	catch(const std::exception &ex) {
		char szBuf[8096]="";
		sprintf(szBuf,"In [%s::%s %d]\nError detected:\n%s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);

		throw megaglest_runtime_error(szBuf);
		//abort();
	}

	return initialMapSelection;
}

int MenuStateCustomGame::setupTechList(string scenario) {
	int initialTechSelection = 0;
	try {
		Config &config = Config::getInstance();

		string scenarioDir = Scenario::getScenarioDir(dirList, scenario);
		vector<string> results;
		vector<string> techPaths = config.getPathListForType(ptTechs,scenarioDir);
		findDirs(techPaths, results);

		if(results.empty()) {
			throw megaglest_runtime_error("No tech-trees were found!");
		}

		techTreeFiles= results;

		for(unsigned int i= 0; i < results.size(); i++) {
			//printf("TECHS i = %d results [%s] scenario [%s]\n",i,results[i].c_str(),scenario.c_str());

			results.at(i)= formatString(results.at(i));
			if(config.getString("InitialTechTree", "Megapack") == results.at(i)) {
				initialTechSelection= i;
			}
		}

		listBoxTechTree.setItems(results);
	}
	catch(const std::exception &ex) {
		char szBuf[8096]="";
		sprintf(szBuf,"In [%s::%s %d]\nError detected:\n%s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);

		throw megaglest_runtime_error(szBuf);
	}

	return initialTechSelection;
}

void MenuStateCustomGame::reloadFactions(bool keepExistingSelectedItem, string scenario) {
	try {
		Config &config = Config::getInstance();

		vector<string> results;
		string scenarioDir = Scenario::getScenarioDir(dirList, scenario);
		vector<string> techPaths = config.getPathListForType(ptTechs,scenarioDir);

		//printf("#1 techPaths.size() = %d scenarioDir [%s] [%s]\n",techPaths.size(),scenario.c_str(),scenarioDir.c_str());

		for(int idx = 0; idx < techPaths.size(); idx++) {
			string &techPath = techPaths[idx];
			endPathWithSlash(techPath);
			string factionPath = techPath + techTreeFiles[listBoxTechTree.getSelectedItemIndex()] + "/factions/";
			findDirs(factionPath, results, false, false);

			//printf("idx = %d factionPath [%s] results.size() = %d\n",idx,factionPath.c_str(),results.size());

			if(results.empty() == false) {
				break;
			}
		}

		if(results.empty() == true) {
			//throw megaglest_runtime_error("(2)There are no factions for the tech tree [" + techTreeFiles[listBoxTechTree.getSelectedItemIndex()] + "]");
			showGeneralError=true;
			generalErrorToShow = "[#2] There are no factions for the tech tree [" + techTreeFiles[listBoxTechTree.getSelectedItemIndex()] + "]";
		}

		results.push_back(formatString(GameConstants::RANDOMFACTION_SLOTNAME));

		// Add special Observer Faction
		if(listBoxAllowObservers.getSelectedItemIndex() == 1) {
			results.push_back(formatString(GameConstants::OBSERVER_SLOTNAME));
		}

		factionFiles= results;
		for(int i = 0; i < results.size(); ++i) {
			results[i]= formatString(results[i]);
			//printf("FACTIONS i = %d results [%s]\n",i,results[i].c_str());

			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"Tech [%s] has faction [%s]\n",techTreeFiles[listBoxTechTree.getSelectedItemIndex()].c_str(),results[i].c_str());
		}

		for(int i = 0; i < GameConstants::maxPlayers; ++i) {
			int originalIndex = listBoxFactions[i].getSelectedItemIndex();
			string originalValue = (listBoxFactions[i].getItemCount() > 0 ? listBoxFactions[i].getSelectedItem() : "");

			listBoxFactions[i].setItems(results);
			if( keepExistingSelectedItem == false ||
				(listBoxAllowObservers.getSelectedItemIndex() == 0 &&
						originalValue == formatString(GameConstants::OBSERVER_SLOTNAME)) ) {

				listBoxFactions[i].setSelectedItemIndex(i % results.size());

				if( originalValue == formatString(GameConstants::OBSERVER_SLOTNAME) &&
					listBoxFactions[i].getSelectedItem() != formatString(GameConstants::OBSERVER_SLOTNAME)) {
					if(listBoxTeams[i].getSelectedItem() == intToStr(GameConstants::maxPlayers + fpt_Observer)) {
						listBoxTeams[i].setSelectedItem(intToStr(1));
					}
				}
			}
			else if(originalIndex < results.size()) {
				listBoxFactions[i].setSelectedItemIndex(originalIndex);
			}
		}
	}
	catch(const std::exception &ex) {
		char szBuf[8096]="";
		sprintf(szBuf,"In [%s::%s %d]\nError detected:\n%s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);

		throw megaglest_runtime_error(szBuf);
	}
}

void MenuStateCustomGame::setupTilesetList(string scenario) {
	try {
		Config &config = Config::getInstance();

		string scenarioDir = Scenario::getScenarioDir(dirList, scenario);

		vector<string> results;
		findDirs(config.getPathListForType(ptTilesets,scenarioDir), results);
		if (results.empty()) {
			throw megaglest_runtime_error("No tile-sets were found!");
		}
		tilesetFiles= results;
		std::for_each(results.begin(), results.end(), FormatString());

		listBoxTileset.setItems(results);
	}
	catch(const std::exception &ex) {
		char szBuf[8096]="";
		sprintf(szBuf,"In [%s::%s %d]\nError detected:\n%s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);

		throw megaglest_runtime_error(szBuf);
	}

}

}}//end namespace
