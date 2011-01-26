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
#include "menu_state_custom_game.h"

#include "renderer.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "config.h"
#include "menu_state_new_game.h"
#include "menu_state_masterserver.h"
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
#include "leak_dumper.h"


namespace Glest{ namespace Game{

using namespace Shared::Util;

struct FormatString {
	void operator()(string &s) {
		s = formatString(s);
	}
};

// =====================================================
// 	class MenuStateCustomGame
// =====================================================

MenuStateCustomGame::MenuStateCustomGame(Program *program, MainMenu *mainMenu, bool openNetworkSlots,bool parentMenuIsMasterserver, bool autostart) :
		MenuState(program, mainMenu, "new-game")
{
	this->autostart = autostart;
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] autostart = %d\n",__FILE__,__FUNCTION__,__LINE__,autostart);

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

	publishToMasterserverThread = NULL;
	Lang &lang= Lang::getInstance();
	NetworkManager &networkManager= NetworkManager::getInstance();
    Config &config = Config::getInstance();
    defaultPlayerName = config.getString("NetPlayerName",Socket::getHostName().c_str());
    enableFactionTexturePreview = config.getBool("FactionPreview","true");
    enableMapPreview = config.getBool("MapPreview","true");

    showFullConsole=false;

    mainMessageBox.registerGraphicComponent(containerName,"mainMessageBox");
	mainMessageBox.init(lang.get("Ok"));
	mainMessageBox.setEnabled(false);
	mainMessageBoxState=0;

	//initialize network interface
	NetworkManager::getInstance().end();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

	serverInitError = false;
	try {
		networkManager.init(nrServer);
	}
	catch(const std::exception &ex) {
		serverInitError = true;
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s %d] Error detected:\n%s\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);
		//throw runtime_error(szBuf);!!!
		showGeneralError=true;
		generalErrorToShow = ex.what();

	}

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

    parentMenuIsMs=parentMenuIsMasterserver;

	needToSetChangedGameSettings = false;
	needToRepublishToMasterserver = false;
	needToBroadcastServerSettings = false;
	showMasterserverError = false;
	masterServererErrorToShow = "---";
	lastSetChangedGameSettings  = 0;
	lastMasterserverPublishing 	= 0;
	lastNetworkPing				= 0;
	soundConnectionCount=0;

	vector<string> teamItems, controlItems, results , rMultiplier;

	//create
	buttonReturn.registerGraphicComponent(containerName,"buttonReturn");
	buttonReturn.init(250, 180, 125);

	buttonRestoreLastSettings.registerGraphicComponent(containerName,"buttonRestoreLastSettings");
	buttonRestoreLastSettings.init(250+130, 180, 200);

	buttonPlayNow.registerGraphicComponent(containerName,"buttonPlayNow");
	buttonPlayNow.init(250+130+205, 180, 125);

	int labelOffset=23;
	int setupPos=590;
	int mapHeadPos=330;
	int mapPos=mapHeadPos-labelOffset;
	int aHeadPos=mapHeadPos-80;
	int aPos=aHeadPos-labelOffset;
	int networkHeadPos=700;
	int networkPos=networkHeadPos-labelOffset;
	int xoffset=10;

    //map listBox
	// put them all in a set, to weed out duplicates (gbm & mgm with same name)
	// will also ensure they are alphabetically listed (rather than how the OS provides them)
	set<string> allMaps;
    findAll(config.getPathListForType(ptMaps), "*.gbm", results, true, false);
	copy(results.begin(), results.end(), std::inserter(allMaps, allMaps.begin()));
	results.clear();
    findAll(config.getPathListForType(ptMaps), "*.mgm", results, true, false);
	copy(results.begin(), results.end(), std::inserter(allMaps, allMaps.begin()));
	results.clear();

	if (allMaps.empty()) {
        throw runtime_error("No maps were found!");
	}
	copy(allMaps.begin(), allMaps.end(), std::back_inserter(results));
	mapFiles = results;

	copy(mapFiles.begin(), mapFiles.end(), std::back_inserter(playerSortedMaps[0]));
	copy(playerSortedMaps[0].begin(), playerSortedMaps[0].end(), std::back_inserter(formattedPlayerSortedMaps[0]));
	std::for_each(formattedPlayerSortedMaps[0].begin(), formattedPlayerSortedMaps[0].end(), FormatString());

	for ( int i = 0 ; i<mapFiles.size(); i++ )
	{// fetch info and put map in right list
		loadMapInfo(Map::getMapPath(mapFiles.at(i),"",false), &mapInfo, false);
		playerSortedMaps[mapInfo.players].push_back(mapFiles.at(i));
		formattedPlayerSortedMaps[mapInfo.players].push_back(formatString(mapFiles.at(i)));
	}

	labelLocalIP.registerGraphicComponent(containerName,"labelLocalIP");
	labelLocalIP.init(210, networkHeadPos+labelOffset);

	string ipText = "none";
	std::vector<std::string> ipList = Socket::getLocalIPAddressList();
	if(ipList.size() > 0) {
		ipText = "";
		for(int idx = 0; idx < ipList.size(); idx++) {
			string ip = ipList[idx];
			if(ipText != "") {
				ipText += ", ";
			}
			ipText += ip;
		}
	}
	string externalPort=config.getString("MasterServerExternalPort", "61357");
	string serverPort=config.getString("ServerPort", "61357");
	labelLocalIP.setText(lang.get("LanIP") + ipText + "  ( "+serverPort+" / "+externalPort+" )");
	ServerSocket::setExternalPort(strToInt(externalPort));

	// Map
	xoffset=70;
	labelMap.registerGraphicComponent(containerName,"labelMap");
	labelMap.init(xoffset+100, mapHeadPos);
	labelMap.setText(lang.get("Map")+":");

	listBoxMap.registerGraphicComponent(containerName,"listBoxMap");
	listBoxMap.init(xoffset+100, mapPos, 200);
    listBoxMap.setItems(formattedPlayerSortedMaps[0]);

    labelMapInfo.registerGraphicComponent(containerName,"labelMapInfo");
	labelMapInfo.init(xoffset+100, mapPos-labelOffset, 200, 40);

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

	//tileset listBox
    findDirs(config.getPathListForType(ptTilesets), results);
	if (results.empty()) {
        throw runtime_error("No tile-sets were found!");
	}
    tilesetFiles= results;
	std::for_each(results.begin(), results.end(), FormatString());

	listBoxTileset.registerGraphicComponent(containerName,"listBoxTileset");
	listBoxTileset.init(xoffset+460, mapPos, 150);
    listBoxTileset.setItems(results);
    srand(time(NULL));
	listBoxTileset.setSelectedItemIndex(rand() % listBoxTileset.getItemCount());

    labelTileset.registerGraphicComponent(containerName,"labelTileset");
	labelTileset.init(xoffset+460, mapHeadPos);
	labelTileset.setText(lang.get("Tileset"));


    //tech Tree listBox
    findDirs(config.getPathListForType(ptTechs), results);
	if(results.empty()) {
        throw runtime_error("No tech-trees were found!");
	}
    techTreeFiles= results;
	std::for_each(results.begin(), results.end(), FormatString());

	listBoxTechTree.registerGraphicComponent(containerName,"listBoxTechTree");
	listBoxTechTree.init(xoffset+650, mapPos, 150);
    listBoxTechTree.setItems(results);

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
	labelEnableObserverMode.registerGraphicComponent(containerName,"labelEnableObserverMode");
	labelEnableObserverMode.init(xoffset+460, aHeadPos, 80);

	listBoxEnableObserverMode.registerGraphicComponent(containerName,"listBoxEnableObserverMode");
	listBoxEnableObserverMode.init(xoffset+460, aPos, 80);
	listBoxEnableObserverMode.pushBackItem(lang.get("Yes"));
	listBoxEnableObserverMode.pushBackItem(lang.get("No"));
	listBoxEnableObserverMode.setSelectedItemIndex(0);

	// Which Pathfinder
	labelPathFinderType.registerGraphicComponent(containerName,"labelPathFinderType");
	labelPathFinderType.init(xoffset+650, aHeadPos, 80);
	labelPathFinderType.setText(lang.get("PathFinderType"));

	listBoxPathFinderType.registerGraphicComponent(containerName,"listBoxPathFinderType");
	listBoxPathFinderType.init(xoffset+650, aPos, 150);
	listBoxPathFinderType.pushBackItem(lang.get("PathFinderTypeRegular"));
	listBoxPathFinderType.pushBackItem(lang.get("PathFinderTypeRoutePlanner"));
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
	if(openNetworkSlots)
		listBoxPublishServer.setSelectedItemIndex(0);
	else
		listBoxPublishServer.setSelectedItemIndex(1);

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
    	labelPlayers[i].registerGraphicComponent(containerName,"labelPlayers" + intToStr(i));
		labelPlayers[i].init(xoffset+0, setupPos-30-i*rowHeight);

		labelPlayerNames[i].registerGraphicComponent(containerName,"labelPlayerNames" + intToStr(i));
		labelPlayerNames[i].init(xoffset+50,setupPos-30-i*rowHeight);

		listBoxControls[i].registerGraphicComponent(containerName,"listBoxControls" + intToStr(i));
        listBoxControls[i].init(xoffset+210, setupPos-30-i*rowHeight);

        listBoxRMultiplier[i].registerGraphicComponent(containerName,"listBoxRMultiplier" + intToStr(i));
        listBoxRMultiplier[i].init(xoffset+350, setupPos-30-i*rowHeight,70);

        listBoxFactions[i].registerGraphicComponent(containerName,"listBoxFactions" + intToStr(i));
        listBoxFactions[i].init(xoffset+430, setupPos-30-i*rowHeight, 150);

        listBoxTeams[i].registerGraphicComponent(containerName,"listBoxTeams" + intToStr(i));
		listBoxTeams[i].init(xoffset+590, setupPos-30-i*rowHeight, 60);

		labelNetStatus[i].registerGraphicComponent(containerName,"labelNetStatus" + intToStr(i));
		labelNetStatus[i].init(xoffset+670, setupPos-30-i*rowHeight, 60);
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
    labelRMultiplier.setFont(CoreData::getInstance().getMenuFontBig());
	labelFaction.setFont(CoreData::getInstance().getMenuFontBig());
	labelTeam.setFont(CoreData::getInstance().getMenuFontBig());

	//texts
	buttonReturn.setText(lang.get("Return"));
	buttonPlayNow.setText(lang.get("PlayNow"));
	buttonRestoreLastSettings.setText(lang.get("ReloadLastGameSettings"));

    controlItems.push_back(lang.get("Closed"));
	controlItems.push_back(lang.get("CpuEasy"));
	controlItems.push_back(lang.get("Cpu"));
    controlItems.push_back(lang.get("CpuUltra"));
    controlItems.push_back(lang.get("CpuMega"));
	controlItems.push_back(lang.get("Network"));
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

	reloadFactions(false);

    vector<string> techPaths = config.getPathListForType(ptTechs);
    for(int idx = 0; idx < techPaths.size(); idx++) {
        string &techPath = techPaths[idx];
        findAll(techPath + "/" + techTreeFiles[listBoxTechTree.getSelectedItemIndex()] + "/factions/*.", results, false, false);

        if(results.size() > 0) {
            break;
        }
    }

    if(results.size() == 0) {
        //throw runtime_error("(1)There are no factions for the tech tree [" + techTreeFiles[listBoxTechTree.getSelectedItemIndex()] + "]");
		showGeneralError=true;
		generalErrorToShow = "[#1] There are no factions for the tech tree [" + techTreeFiles[listBoxTechTree.getSelectedItemIndex()] + "]";
    }

	for(int i=0; i<GameConstants::maxPlayers; ++i){
		labelPlayers[i].setText(lang.get("Player")+" "+intToStr(i));
		labelPlayerNames[i].setText("*");

        listBoxTeams[i].setItems(teamItems);
		listBoxTeams[i].setSelectedItemIndex(i);
		lastSelectedTeamIndex[i] = listBoxTeams[i].getSelectedItemIndex();

		listBoxControls[i].setItems(controlItems);
		listBoxRMultiplier[i].setItems(rMultiplier);
		listBoxRMultiplier[i].setSelectedItemIndex(5);
		labelNetStatus[i].setText("");
    }


    labelEnableObserverMode.setText(lang.get("EnableObserverMode"));


	loadMapInfo(Map::getMapPath(getCurrentMapFile()), &mapInfo, true);

	labelMapInfo.setText(mapInfo.desc);

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//init controllers
	if(serverInitError == false) {
		listBoxControls[0].setSelectedItemIndex(ctHuman);
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

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		// Ensure we have set the gamesettings at least once
		GameSettings gameSettings;
		loadGameSettings(&gameSettings);
		ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();
		serverInterface->setGameSettings(&gameSettings,false);
	}
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

	updateAllResourceMultiplier();

	// write hint to console:
	Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));

	console.addLine(lang.get("To switch off music press")+" - \""+configKeys.getCharKey("ToggleMusic")+"\"");

	chatManager.init(&console, -1,true);

	GraphicComponent::applyAllCustomProperties(containerName);

	publishToMasterserverThread = new SimpleTaskThread(this,0,25);
	publishToMasterserverThread->setUniqueID(__FILE__);
	publishToMasterserverThread->start();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void MenuStateCustomGame::cleanup() {
    //printf("IN MenuStateCustomGame cleanup - A\n");

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
    if(publishToMasterserverThread != NULL) {
        SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

        //printf("IN MenuStateCustomGame cleanup - B\n");

        needToBroadcastServerSettings = false;
        needToRepublishToMasterserver = false;
        lastNetworkPing               = time(NULL);
        publishToMasterserverThread->setThreadOwnerValid(false);

        SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
        if(publishToMasterserverThread->canShutdown(true) == true &&
           publishToMasterserverThread->shutdownAndWait() == true) {
            //printf("IN MenuStateCustomGame cleanup - C\n");
            delete publishToMasterserverThread;
        }

        //printf("IN MenuStateCustomGame cleanup - D\n");
        publishToMasterserverThread = NULL;
    }

    //printf("IN MenuStateCustomGame cleanup - E\n");

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

	cleanupMapPreviewTexture();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

MenuStateCustomGame::~MenuStateCustomGame() {
    //printf("IN MenuStateCustomGame Destructor - A\n");

    cleanup();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void MenuStateCustomGame::returnToParentMenu() {
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

	needToBroadcastServerSettings = false;
	needToRepublishToMasterserver = false;
	lastNetworkPing               = time(NULL);
	bool returnToMasterServerMenu = parentMenuIsMs;

/*
	if(publishToMasterserverThread != NULL &&
        publishToMasterserverThread->canShutdown() == true &&
		publishToMasterserverThread->shutdownAndWait() == true) {
        publishToMasterserverThread->setThreadOwnerValid(false);
		delete publishToMasterserverThread;
		publishToMasterserverThread = NULL;
	}
*/

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(returnToMasterServerMenu) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
		cleanup();
		mainMenu->setState(new MenuStateMasterserver(program, mainMenu));
	}
	else {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
		cleanup();
		mainMenu->setState(new MenuStateNewGame(program, mainMenu));
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void MenuStateCustomGame::mouseClick(int x, int y, MouseButton mouseButton){

    try {
        CoreData &coreData= CoreData::getInstance();
        SoundRenderer &soundRenderer= SoundRenderer::getInstance();
        int oldListBoxMapfilterIndex=listBoxMapFilter.getSelectedItemIndex();

        if(mainMessageBox.getEnabled()){
            int button= 1;
            if(mainMessageBox.mouseClick(x, y, button))
            {
                soundRenderer.playFx(coreData.getClickSoundA());
                if(button==1)
                {
                    mainMessageBox.setEnabled(false);
                }
            }
        }
        else if(buttonReturn.mouseClick(x,y) || serverInitError == true) {
            SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

            soundRenderer.playFx(coreData.getClickSoundA());

            MutexSafeWrapper safeMutex((publishToMasterserverThread != NULL ? publishToMasterserverThread->getMutexThreadObjectAccessor() : NULL));
            needToBroadcastServerSettings = false;
            needToRepublishToMasterserver = false;
            lastNetworkPing               = time(NULL);
            safeMutex.ReleaseLock();

/*
            if(publishToMasterserverThread != NULL &&
                publishToMasterserverThread->canShutdown() == true &&
                publishToMasterserverThread->shutdownAndWait() == true) {
                publishToMasterserverThread->setThreadOwnerValid(false);
                delete publishToMasterserverThread;
                publishToMasterserverThread = NULL;
            }
*/

            SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

            returnToParentMenu();
        }
        else if(buttonPlayNow.mouseClick(x,y) && buttonPlayNow.getEnabled()) {
            SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

            PlayNow();

            SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
        }
        else if(buttonRestoreLastSettings.mouseClick(x,y) && buttonRestoreLastSettings.getEnabled()) {
            RestoreLastGameSettings();
        }
        else if(listBoxMap.mouseClick(x, y)){
            SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s\n", getCurrentMapFile().c_str());

            MutexSafeWrapper safeMutex((publishToMasterserverThread != NULL ? publishToMasterserverThread->getMutexThreadObjectAccessor() : NULL));

            loadMapInfo(Map::getMapPath(getCurrentMapFile(),"",false), &mapInfo, true);
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
        else if (listBoxAdvanced.getSelectedItemIndex() == 1 && listBoxFogOfWar.mouseClick(x, y)) {
            MutexSafeWrapper safeMutex((publishToMasterserverThread != NULL ? publishToMasterserverThread->getMutexThreadObjectAccessor() : NULL));

            if(listBoxPublishServer.getSelectedItemIndex() == 0) {
                needToRepublishToMasterserver = true;
            }

            if(hasNetworkGameSettings() == true)
            {
                needToSetChangedGameSettings = true;
                lastSetChangedGameSettings   = time(NULL);
            }
        }
        else if (listBoxAdvanced.getSelectedItemIndex() == 1 && listBoxAllowObservers.mouseClick(x, y)) {
            MutexSafeWrapper safeMutex((publishToMasterserverThread != NULL ? publishToMasterserverThread->getMutexThreadObjectAccessor() : NULL));

            if(listBoxPublishServer.getSelectedItemIndex() == 0) {
                needToRepublishToMasterserver = true;
            }

            reloadFactions(true);

            if(hasNetworkGameSettings() == true) {
                needToSetChangedGameSettings = true;
                lastSetChangedGameSettings   = time(NULL);
            }
        }
        else if (listBoxAdvanced.getSelectedItemIndex() == 1 && listBoxEnableObserverMode.mouseClick(x, y)) {
            MutexSafeWrapper safeMutex((publishToMasterserverThread != NULL ? publishToMasterserverThread->getMutexThreadObjectAccessor() : NULL));

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
            MutexSafeWrapper safeMutex((publishToMasterserverThread != NULL ? publishToMasterserverThread->getMutexThreadObjectAccessor() : NULL));

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
        else if(listBoxTileset.mouseClick(x, y)){
            MutexSafeWrapper safeMutex((publishToMasterserverThread != NULL ? publishToMasterserverThread->getMutexThreadObjectAccessor() : NULL));

            if(listBoxPublishServer.getSelectedItemIndex() == 0) {
                needToRepublishToMasterserver = true;
            }
            if(hasNetworkGameSettings() == true)
            {
                needToSetChangedGameSettings = true;
                lastSetChangedGameSettings   = time(NULL);
            }
        }
        else if(listBoxMapFilter.mouseClick(x, y)){
            MutexSafeWrapper safeMutex((publishToMasterserverThread != NULL ? publishToMasterserverThread->getMutexThreadObjectAccessor() : NULL));
            switchToNextMapGroup(listBoxMapFilter.getSelectedItemIndex()-oldListBoxMapfilterIndex);
            SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s\n", getCurrentMapFile().c_str());

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
            reloadFactions(false);

            MutexSafeWrapper safeMutex((publishToMasterserverThread != NULL ? publishToMasterserverThread->getMutexThreadObjectAccessor() : NULL));

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
            MutexSafeWrapper safeMutex((publishToMasterserverThread != NULL ? publishToMasterserverThread->getMutexThreadObjectAccessor() : NULL));
            needToRepublishToMasterserver = true;
            soundRenderer.playFx(coreData.getClickSoundC());
        }
        else if(listBoxAdvanced.getSelectedItemIndex() == 1 && listBoxNetworkPauseGameForLaggedClients.mouseClick(x, y)){
            MutexSafeWrapper safeMutex((publishToMasterserverThread != NULL ? publishToMasterserverThread->getMutexThreadObjectAccessor() : NULL));

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
        else {
            for(int i=0; i<mapInfo.players; ++i) {
                MutexSafeWrapper safeMutex((publishToMasterserverThread != NULL ? publishToMasterserverThread->getMutexThreadObjectAccessor() : NULL));

                if (listBoxAdvanced.getSelectedItemIndex() == 1) {
                    // set multiplier
                    if(listBoxRMultiplier[i].mouseClick(x, y)) {
                    }
                }

                //ensure thet only 1 human player is present
                if(listBoxControls[i].mouseClick(x, y)) {
                    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

                    //look for human players
                    int humanIndex1= -1;
                    int humanIndex2= -1;
                    for(int j=0; j<GameConstants::maxPlayers; ++j) {
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

                    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] humanIndex1 = %d, humanIndex2 = %d\n",__FILE__,__FUNCTION__,__LINE__,humanIndex1,humanIndex2);

                    //no human
                    if(humanIndex1 == -1 && humanIndex2 == -1) {
                        listBoxControls[i].setSelectedItemIndex(ctHuman);
                        //labelPlayerNames[i].setText("");
                        //labelPlayerNames[i].setText(getHumanPlayerName());

                        SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] i = %d, labelPlayerNames[i].getText() [%s]\n",__FILE__,__FUNCTION__,__LINE__,i,labelPlayerNames[i].getText().c_str());
                    }
                    //2 humans
                    else if(humanIndex1 != -1 && humanIndex2 != -1) {
                        int closeSlotIndex = (humanIndex1 == i ? humanIndex2: humanIndex1);
                        int humanSlotIndex = (closeSlotIndex == humanIndex1 ? humanIndex2 : humanIndex1);

                        string origPlayName = labelPlayerNames[closeSlotIndex].getText();

                        SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] closeSlotIndex = %d, origPlayName [%s]\n",__FILE__,__FUNCTION__,__LINE__,closeSlotIndex,origPlayName.c_str());

                        listBoxControls[closeSlotIndex].setSelectedItemIndex(ctClosed);
                        //labelPlayerNames[closeSlotIndex].setText("");

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
                else if(listBoxFactions[i].mouseClick(x, y)) {

                    //printf("factionFiles[listBoxFactions[i].getSelectedItemIndex()] [%s] i = %d selIndex = %d\n",factionFiles[listBoxFactions[i].getSelectedItemIndex()].c_str(),i,listBoxFactions[i].getSelectedItemIndex());

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
                        //printf("i = %d factionFiles[listBoxFactions[i].getSelectedItemIndex()] [%s] listBoxTeams[i].getSelectedItemIndex() = %d, lastSelectedTeamIndex[i] = %d\n",i,factionFiles[listBoxFactions[i].getSelectedItemIndex()].c_str(),listBoxTeams[i].getSelectedItemIndex(),lastSelectedTeamIndex[i]);
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
                    SetActivePlayerNameEditor();
                }
            }
        }
    }
	catch(const std::exception &ex) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s %d] Error detected:\n%s\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);
		//throw runtime_error(szBuf);
		showGeneralError=true;
		generalErrorToShow = szBuf;
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
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
			listBoxRMultiplier[index].setEnabled(true);
		}
		else if(ct == ctCpu || ct == ctNetworkCpu) {
			listBoxRMultiplier[index].setSelectedItemIndex((GameConstants::normalMultiplier-0.5f)*10);
			listBoxRMultiplier[index].setEnabled(true);
		}
		else if(ct == ctCpuUltra || ct == ctNetworkCpuUltra)
		{
			listBoxRMultiplier[index].setSelectedItemIndex((GameConstants::ultraMultiplier-0.5f)*10);
			listBoxRMultiplier[index].setEnabled(true);
		}
		else if(ct == ctCpuMega || ct == ctNetworkCpuMega)
		{
			listBoxRMultiplier[index].setSelectedItemIndex((GameConstants::megaMultiplier-0.5f)*10);
			listBoxRMultiplier[index].setEnabled(true);
		}
		listBoxRMultiplier[index].setEditable(listBoxRMultiplier[index].getEnabled());
		listBoxRMultiplier[index].setVisible(listBoxRMultiplier[index].getEnabled());
}




void MenuStateCustomGame::SetActivePlayerNameEditor() {
	for(int i = 0; i < mapInfo.players; ++i) {
		ControlType ct= static_cast<ControlType>(listBoxControls[i].getSelectedItemIndex());
		if(ct == ctHuman) {
			setActiveInputLabel(&labelPlayerNames[i]);
			break;
		}
	}
}

void MenuStateCustomGame::RestoreLastGameSettings() {
	// Ensure we have set the gamesettings at least once
	GameSettings gameSettings = loadGameSettingsFromFile("lastCustomGamSettings.mgg");
	if(gameSettings.getMap() == "") {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		loadGameSettings(&gameSettings);
	}

	ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();
	serverInterface->setGameSettings(&gameSettings,false);

	MutexSafeWrapper safeMutex((publishToMasterserverThread != NULL ? publishToMasterserverThread->getMutexThreadObjectAccessor() : NULL));

	if(listBoxPublishServer.getSelectedItemIndex() == 0) {
		needToRepublishToMasterserver = true;
	}

	if(hasNetworkGameSettings() == true)
	{
		needToSetChangedGameSettings = true;
		lastSetChangedGameSettings   = time(NULL);
	}
}

void MenuStateCustomGame::PlayNow() {
	MutexSafeWrapper safeMutex((publishToMasterserverThread != NULL ? publishToMasterserverThread->getMutexThreadObjectAccessor() : NULL));
	saveGameSettingsToFile("lastCustomGamSettings.mgg");

	closeUnusedSlots();
	CoreData &coreData= CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();
	soundRenderer.playFx(coreData.getClickSoundC());

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

	std::vector<string> randomFactionSelectionList;
	int RandomCount = 0;
	for(int i= 0; i < mapInfo.players; ++i) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		// Check for random faction selection and choose the faction now
		if(listBoxControls[i].getSelectedItemIndex() != ctClosed) {
			if(listBoxFactions[i].getSelectedItem() == formatString(GameConstants::RANDOMFACTION_SLOTNAME)) {
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] i = %d\n",__FILE__,__FUNCTION__,__LINE__,i);

				// Max 1000 tries to get a random, unused faction
				for(int findRandomFaction = 1; findRandomFaction < 1000; ++findRandomFaction) {
					srand(time(NULL) + findRandomFaction);
					int selectedFactionIndex = rand() % listBoxFactions[i].getItemCount();
					string selectedFactionName = listBoxFactions[i].getItem(selectedFactionIndex);

					SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] selectedFactionName [%s] selectedFactionIndex = %d, findRandomFaction = %d\n",__FILE__,__FUNCTION__,__LINE__,selectedFactionName.c_str(),selectedFactionIndex,findRandomFaction);

					if(	selectedFactionName != formatString(GameConstants::RANDOMFACTION_SLOTNAME) &&
						selectedFactionName != formatString(GameConstants::OBSERVER_SLOTNAME) &&
						std::find(randomFactionSelectionList.begin(),randomFactionSelectionList.end(),selectedFactionName) == randomFactionSelectionList.end()) {

						SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
						listBoxFactions[i].setSelectedItem(selectedFactionName);
						randomFactionSelectionList.push_back(selectedFactionName);
						break;
					}
				}

				if(listBoxFactions[i].getSelectedItem() == formatString(GameConstants::RANDOMFACTION_SLOTNAME)) {
					SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] RandomCount = %d\n",__FILE__,__FUNCTION__,__LINE__,RandomCount);

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

				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] i = %d, listBoxFactions[i].getSelectedItem() [%s]\n",__FILE__,__FUNCTION__,__LINE__,i,listBoxFactions[i].getSelectedItem().c_str());

				RandomCount++;
			}
		}
	}

	if(RandomCount > 0) {
		needToSetChangedGameSettings = true;
	}

	safeMutex.ReleaseLock(true);
	GameSettings gameSettings;
	loadGameSettings(&gameSettings);

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
	ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();

	// Send the game settings to each client if we have at least one networked client
	safeMutex.Lock();

	bool dataSynchCheckOk = true;
	for(int i= 0; i < mapInfo.players; ++i) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		if(listBoxControls[i].getSelectedItemIndex() == ctNetwork) {
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

			if(	serverInterface->getSlot(i) != NULL && serverInterface->getSlot(i)->isConnected() &&
				serverInterface->getSlot(i)->getAllowGameDataSynchCheck() == true &&
				serverInterface->getSlot(i)->getNetworkGameDataSynchCheckOk() == false) {
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
				dataSynchCheckOk = false;
				break;
			}
		}
	}

	if(dataSynchCheckOk == false) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
		mainMessageBoxState=1;
		showMessageBox( "You cannot start the game because\none or more clients do not have the same game data!", "Data Mismatch Error", false);

		safeMutex.ReleaseLock();
		return;
	}
	else {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
		if( (hasNetworkGameSettings() == true &&
			 needToSetChangedGameSettings == true) || (RandomCount > 0)) {
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
			serverInterface->setGameSettings(&gameSettings,true);
			serverInterface->broadcastGameSetup(&gameSettings);

			needToSetChangedGameSettings    = false;
			lastSetChangedGameSettings      = time(NULL);
		}

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		// Tell the server Interface whether or not to publish game status updates to masterserver
		serverInterface->setNeedToRepublishToMasterserver(listBoxPublishServer.getSelectedItemIndex() == 0);

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
		bool bOkToStart = serverInterface->launchGame(&gameSettings);
		if(bOkToStart == true) {
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

			if( listBoxPublishServer.getEditable() &&
				listBoxPublishServer.getSelectedItemIndex() == 0) {

				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

				needToRepublishToMasterserver = true;
				lastMasterserverPublishing = 0;

				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
			}
			needToBroadcastServerSettings = false;
			needToRepublishToMasterserver = false;
			lastNetworkPing               = time(NULL);
			safeMutex.ReleaseLock();

/*
			if(publishToMasterserverThread != NULL &&
                publishToMasterserverThread->canShutdown() == true &&
				publishToMasterserverThread->shutdownAndWait() == true) {
                publishToMasterserverThread->setThreadOwnerValid(false);
				delete publishToMasterserverThread;
				publishToMasterserverThread = NULL;
			}
*/
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

			assert(program != NULL);

			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
			cleanup();
			program->setState(new Game(program, &gameSettings));
			return;
		}
		else {
			safeMutex.ReleaseLock();
		}
	}
}

void MenuStateCustomGame::mouseMove(int x, int y, const MouseState *ms){
	if (mainMessageBox.getEnabled()) {
		mainMessageBox.mouseMove(x, y);
	}
	buttonReturn.mouseMove(x, y);
	buttonPlayNow.mouseMove(x, y);
	buttonRestoreLastSettings.mouseMove(x, y);

	bool editingPlayerName = false;
	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		listBoxRMultiplier[i].mouseMove(x, y);
        listBoxControls[i].mouseMove(x, y);
        listBoxFactions[i].mouseMove(x, y);
		listBoxTeams[i].mouseMove(x, y);

		if(labelPlayerNames[i].mouseMove(x, y) == true) {
			editingPlayerName = true;
		}
    }
	if(editingPlayerName == false) {
		setActiveInputLabel(NULL);
	}
	listBoxMap.mouseMove(x, y);
	if(listBoxAdvanced.getSelectedItemIndex() == 1) {
		listBoxFogOfWar.mouseMove(x, y);
		listBoxAllowObservers.mouseMove(x, y);
		listBoxEnableObserverMode.mouseMove(x, y);
		//listBoxEnableServerControlledAI.mouseMove(x, y);
		//labelNetworkFramePeriod.mouseMove(x, y);
		//listBoxNetworkFramePeriod.mouseMove(x, y);

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
}

void MenuStateCustomGame::render() {
	try {
		Renderer &renderer= Renderer::getInstance();

		if(factionTexture != NULL) {
			renderer.renderTextureQuad(800,600,200,150,factionTexture,0.7f);
		}
		if(mapPreviewTexture != NULL) {
			//renderer.renderTextureQuad(10,350,-1,-1,mapPreviewTexture,0.7);
			renderer.renderTextureQuad(800,300,200,150,mapPreviewTexture,0.7f);
			//printf("=================> Rendering map preview texture\n");
		}

		if(mainMessageBox.getEnabled()){
			renderer.renderMessageBox(&mainMessageBox);
		}
		else
		{
			renderer.renderButton(&buttonReturn);
			renderer.renderButton(&buttonPlayNow);
			renderer.renderButton(&buttonRestoreLastSettings);

			// Get a reference to the player texture cache
			std::map<int,Texture2D *> &crcPlayerTextureCache = CacheManager::getCachedItem< std::map<int,Texture2D *> >(GameConstants::playerTextureCacheLookupKey);

			// START - this code ensure player title and player names don't overlap
			int offsetPosition=0;
		    for(int i=0; i < GameConstants::maxPlayers; ++i) {
		    	//labelPlayers[i].registerGraphicComponent(containerName,"labelPlayers" + intToStr(i));
				//labelPlayers[i].init(xoffset+50, setupPos-30-i*rowHeight);

				const Metrics &metrics= Metrics::getInstance();
				const FontMetrics *fontMetrics= CoreData::getInstance().getMenuFontNormal()->getMetrics();
				if(fontMetrics == NULL) {
					throw runtime_error("fontMetrics == NULL");
				}
				int curWidth = (metrics.toVirtualX(fontMetrics->getTextWidth(labelPlayers[i].getText())));

				if(labelPlayers[i].getX() + curWidth >= labelPlayerNames[i].getX()) {
					int newOffsetPosition = labelPlayers[i].getX() + curWidth + 2;
					if(offsetPosition < newOffsetPosition) {
						offsetPosition = newOffsetPosition;
					}
				}
		    }
		    // END

			for(int i = 0; i < GameConstants::maxPlayers; ++i) {
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
					if(listBoxAdvanced.getSelectedItemIndex() == 1){
						renderer.renderListBox(&listBoxRMultiplier[i]);
					}
					renderer.renderListBox(&listBoxFactions[i]);
					renderer.renderListBox(&listBoxTeams[i]);
					renderer.renderLabel(&labelNetStatus[i]);
				}
			}
			renderer.renderLabel(&labelLocalIP);
			renderer.renderLabel(&labelMap);

			if(listBoxAdvanced.getSelectedItemIndex() == 1) {
				renderer.renderLabel(&labelFogOfWar);
				renderer.renderLabel(&labelAllowObservers);
				renderer.renderLabel(&labelEnableObserverMode);
				renderer.renderLabel(&labelPathFinderType);

				renderer.renderListBox(&listBoxFogOfWar);
				renderer.renderListBox(&listBoxAllowObservers);
				renderer.renderListBox(&listBoxEnableObserverMode);
				renderer.renderListBox(&listBoxPathFinderType);
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

		if(program != NULL) program->renderProgramMsgBox();

		if( enableMapPreview == true &&
			mapPreview.hasFileLoaded() == true) {

			int mouseX = mainMenu->getMouseX();
			int mouseY = mainMenu->getMouseY();
			int mouse2dAnim = mainMenu->getMouse2dAnim();

		    renderer.renderMouse2d(mouseX, mouseY, mouse2dAnim);
		    bool renderAll = (listBoxFogOfWar.getSelectedItemIndex() == 2);

		    if(mapPreviewTexture == NULL) {
		    	//printf("=================> Rendering map preview into a texture BEFORE (%p)\n", mapPreviewTexture);
		    	renderer.renderMapPreview(&mapPreview, renderAll, 10, 350,&mapPreviewTexture);
		    	//printf("=================> Rendering map preview into a texture AFTER (%p)\n", mapPreviewTexture);
		    }
		    else {
		    	renderer.renderMapPreview(&mapPreview, renderAll, 10, 350);
		    }
		}

		if(hasNetworkGameSettings() == true) {
			renderer.renderChatManager(&chatManager);
		}
		renderer.renderConsole(&console,showFullConsole,true);
	}
	catch(const std::exception &ex) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s %d] Error detected:\n%s\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		//throw runtime_error(szBuf);

		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);
		//throw runtime_error(szBuf);
		showGeneralError=true;
		generalErrorToShow = ex.what();
	}
}

void MenuStateCustomGame::update() {
	Chrono chrono;
	chrono.start();

	// Test openal buffer underrun issue
	//sleep(200);
	// END

	MutexSafeWrapper safeMutex((publishToMasterserverThread != NULL ? publishToMasterserverThread->getMutexThreadObjectAccessor() : NULL));

	try {
		if(serverInitError == true) {
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

			if(showGeneralError) {
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);


				showGeneralError=false;
				mainMessageBoxState=1;
				showMessageBox( generalErrorToShow, "Error", false);
			}

			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
			return;
		}
		ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();
		Lang& lang= Lang::getInstance();

		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		bool haveAtLeastOneNetworkClientConnected = false;
		bool hasOneNetworkSlotOpen = false;
		int currentConnectionCount=0;
		Config &config = Config::getInstance();

		bool masterServerErr = showMasterserverError;

		if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
		if(chrono.getMillis() > 0) chrono.start();

		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		if(masterServerErr) {
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

			if(EndsWith(masterServererErrorToShow, "wrong router setup") == true) {
				masterServererErrorToShow=lang.get("WrongRouterSetup");
			}
            masterServererErrorToShow += " (disabling publish)";
			showMasterserverError=false;
			mainMessageBoxState=1;
			showMessageBox( masterServererErrorToShow, lang.get("ErrorFromMasterserver"), false);

			listBoxPublishServer.setSelectedItemIndex(1);
		}
		else if(showGeneralError) {
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

			showGeneralError=false;
			mainMessageBoxState=1;
			showMessageBox( generalErrorToShow, "Error", false);
		}

		if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
		if(chrono.getMillis() > 0) chrono.start();

		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		// handle setting changes from clients
		SwitchSetupRequest ** switchSetupRequests = serverInterface->getSwitchSetupRequests();
		for(int i= 0; i< mapInfo.players; ++i) {
			if(switchSetupRequests[i] != NULL) {
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] switchSetupRequests[i]->getSwitchFlags() = %d\n",__FILE__,__FUNCTION__,__LINE__,switchSetupRequests[i]->getSwitchFlags());

				if(listBoxControls[i].getSelectedItemIndex() == ctNetwork) {
					SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] switchSetupRequests[i]->getToFactionIndex() = %d\n",__FILE__,__FUNCTION__,__LINE__,switchSetupRequests[i]->getToFactionIndex());
					//printf("switchSetupRequests[i]->getSelectedFactionName()=%s\n",switchSetupRequests[i]->getSelectedFactionName().c_str());
					//printf("switchSetupRequests[i]->getToTeam()=%d\n",switchSetupRequests[i]->getToTeam());

					if(switchSetupRequests[i]->getToFactionIndex() != -1) {
						int newFactionIdx = switchSetupRequests[i]->getToFactionIndex();
						/*
						if(switchSetupRequests[i]->getNetworkPlayerName() != GameConstants::NETWORK_SLOT_UNCONNECTED_SLOTNAME) {
							labelPlayerNames[k].setText(switchSetupRequests[i]->getNetworkPlayerName());
						}
						else {
							labelPlayerNames[k].setText("");
						}
						*/

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
									SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] i = %d, labelPlayerNames[newFactionIdx].getText() [%s] switchSetupRequests[i]->getNetworkPlayerName() [%s]\n",__FILE__,__FUNCTION__,__LINE__,i,labelPlayerNames[newFactionIdx].getText().c_str(),switchSetupRequests[i]->getNetworkPlayerName().c_str());
									labelPlayerNames[newFactionIdx].setText(switchSetupRequests[i]->getNetworkPlayerName());
									//SetActivePlayerNameEditor();
								}
							}
							catch(const runtime_error &e) {
								SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
								SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] caught exception error = [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
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
								SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] i = %d, switchSetupRequests[i]->getSwitchFlags() = %d, switchSetupRequests[i]->getNetworkPlayerName() [%s], labelPlayerNames[i].getText() [%s]\n",__FILE__,__FUNCTION__,__LINE__,i,switchSetupRequests[i]->getSwitchFlags(),switchSetupRequests[i]->getNetworkPlayerName().c_str(),labelPlayerNames[i].getText().c_str());
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
							SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
							SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] caught exception error = [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
						}
					}
				}

				delete switchSetupRequests[i];
				switchSetupRequests[i]=NULL;
			}
		}

		if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
		if(chrono.getMillis() > 0) chrono.start();

		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] mapInfo.players = %d\n",__FILE__,__FUNCTION__,__LINE__,mapInfo.players);

		for(int i= 0; i< mapInfo.players; ++i) {
			//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

			if(listBoxControls[i].getSelectedItemIndex() == ctNetwork) {
				//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

				hasOneNetworkSlotOpen=true;

				//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] A - ctNetwork\n",__FILE__,__FUNCTION__);

				if(serverInterface->getSlot(i) != NULL &&
                   serverInterface->getSlot(i)->isConnected()) {
					serverInterface->getSlot(i)->setName(labelPlayerNames[i].getText());

					//printf("FYI we have at least 1 client connected, slot = %d'\n",i);

					haveAtLeastOneNetworkClientConnected = true;
					if(serverInterface->getSlot(i) != NULL &&
                       serverInterface->getSlot(i)->getConnectHasHandshaked()) {
						currentConnectionCount++;
                    }
					//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] B - ctNetwork\n",__FILE__,__FUNCTION__);
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
									serverInterface->sendTextMessage(lastMapDataSynchError,-1, true);
								}
							}

							if(serverInterface->getSlot(i) != NULL &&
                               serverInterface->getSlot(i)->getNetworkGameDataSynchCheckOkTile() == false) {
								label = label + " tile";

								if(serverInterface->getSlot(i)->getReceivedDataSynchCheck() == true &&
									lastTileDataSynchError != "tile CRC mismatch, " + listBoxTileset.getSelectedItem()) {
									lastTileDataSynchError = "tile CRC mismatch, " + listBoxTileset.getSelectedItem();
									ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();
									serverInterface->sendTextMessage(lastTileDataSynchError,-1,true);
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

										SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] report: %s\n",__FILE__,__FUNCTION__,__LINE__,report.c_str());

										serverInterface->sendTextMessage("techtree CRC mismatch",-1,true);
										vector<string> reportLineTokens;
										Tokenize(report,reportLineTokens,"\n");
										for(int reportLine = 0; reportLine < reportLineTokens.size(); ++reportLine) {
											serverInterface->sendTextMessage(reportLineTokens[reportLine],-1,true);
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

				//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] END - ctNetwork\n",__FILE__,__FUNCTION__);
			}
			else{
				labelNetStatus[i].setText("");
			}
		}

		if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
		if(chrono.getMillis() > 0) chrono.start();

		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		bool checkDataSynch = (serverInterface->getAllowGameDataSynchCheck() == true &&
					//haveAtLeastOneNetworkClientConnected == true &&
					needToSetChangedGameSettings == true &&
					difftime(time(NULL),lastSetChangedGameSettings) >= 2);

		GameSettings gameSettings;
		loadGameSettings(&gameSettings);

		if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
		if(chrono.getMillis() > 0) chrono.start();

		// Send the game settings to each client if we have at least one networked client
		if(checkDataSynch == true) {
			serverInterface->setGameSettings(&gameSettings,false);
			needToSetChangedGameSettings    = false;
		}

		if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
		if(chrono.getMillis() > 0) chrono.start();

		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		if(hasOneNetworkSlotOpen) {
			//listBoxPublishServer.setSelectedItemIndex(0);
			listBoxPublishServer.setEditable(true);
			//listBoxEnableServerControlledAI.setEditable(true);
		}
		else
		{
			listBoxPublishServer.setSelectedItemIndex(1);
			listBoxPublishServer.setEditable(false);
			//listBoxEnableServerControlledAI.setEditable(false);
		}

		bool republishToMaster = (difftime(time(NULL),lastMasterserverPublishing) >= 5);

		if(republishToMaster == true) {
			if(listBoxPublishServer.getSelectedItemIndex() == 0) {
				needToRepublishToMasterserver = true;
				lastMasterserverPublishing = time(NULL);
			}
		}

		bool callPublishNow = (listBoxPublishServer.getEditable() &&
							   listBoxPublishServer.getSelectedItemIndex() == 0 &&
							   needToRepublishToMasterserver == true);

		if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
		if(chrono.getMillis() > 0) chrono.start();

		if(callPublishNow == true) {
			// give it to me baby, aha aha ...
			publishToMasterserver();
		}

		bool broadCastSettings = (difftime(time(NULL),lastSetChangedGameSettings) >= 2);

		if(broadCastSettings == true) {
			needToBroadcastServerSettings=true;
		}

		if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
		if(chrono.getMillis() > 0) chrono.start();

		//call the chat manager
		chatManager.updateNetwork();

		//console
		console.update();

		broadCastSettings = (difftime(time(NULL),lastSetChangedGameSettings) >= 2);

		if(broadCastSettings == true)
		{// reset timer here on bottom becasue used for different things
			lastSetChangedGameSettings      = time(NULL);
		}
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
		if(chrono.getMillis() > 0) chrono.start();

		if(currentConnectionCount > soundConnectionCount){
			soundConnectionCount = currentConnectionCount;
			SoundRenderer::getInstance().playFx(CoreData::getInstance().getAttentionSound());
			//switch on music again!!
			Config &config = Config::getInstance();
			float configVolume = (config.getInt("SoundVolumeMusic") / 100.f);
			CoreData::getInstance().getMenuMusic()->setVolume(configVolume);
		}
		soundConnectionCount = currentConnectionCount;

		if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
		if(chrono.getMillis() > 0) chrono.start();

		if(enableFactionTexturePreview == true) {

			if( currentTechName_factionPreview != gameSettings.getTech() ||
				currentFactionName_factionPreview != gameSettings.getFactionTypeName(gameSettings.getThisFactionIndex())) {

				currentTechName_factionPreview=gameSettings.getTech();
				currentFactionName_factionPreview=gameSettings.getFactionTypeName(gameSettings.getThisFactionIndex());

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

		if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
		if(chrono.getMillis() > 0) chrono.start();

		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		if(autostart == true) {
			safeMutex.ReleaseLock();
			RestoreLastGameSettings();
			PlayNow();
		}
	}
	catch(const std::exception &ex) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s %d] Error detected:\n%s\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);
		//throw runtime_error(szBuf);
		showGeneralError=true;
		generalErrorToShow = szBuf;
	}
	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void MenuStateCustomGame::publishToMasterserver()
{
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

	int slotCountUsed=0;
	int slotCountHumans=0;
	int slotCountConnectedPlayers=0;
	ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();
	GameSettings gameSettings;
	loadGameSettings(&gameSettings);
	Config &config= Config::getInstance();
	//string serverinfo="";
	publishToServerInfo.clear();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

	for(int i= 0; i < mapInfo.players; ++i) {
		if(listBoxControls[i].getSelectedItemIndex() != ctClosed) {
			slotCountUsed++;
		}

		if(listBoxControls[i].getSelectedItemIndex() == ctNetwork)
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
	publishToServerInfo["platform"] = getPlatformNameString();
    publishToServerInfo["binaryCompileDate"] = getCompileDateTime();

	//game info:
	publishToServerInfo["serverTitle"] = getHumanPlayerName() + "'s game";
	//ip is automatically set

	//game setup info:
	publishToServerInfo["tech"] = listBoxTechTree.getSelectedItem();
	publishToServerInfo["map"] = listBoxMap.getSelectedItem();
	publishToServerInfo["tileset"] = listBoxTileset.getSelectedItem();
	publishToServerInfo["activeSlots"] = intToStr(slotCountUsed);
	publishToServerInfo["networkSlots"] = intToStr(slotCountHumans);
	publishToServerInfo["connectedClients"] = intToStr(slotCountConnectedPlayers);

	string externalport = config.getString("MasterServerExternalPort", "61357");
	publishToServerInfo["externalconnectport"] = externalport;
	publishToServerInfo["privacyPlease"] = config.getBool("PrivacyPlease","false");

	publishToServerInfo["gameStatus"] = intToStr(game_status_waiting_for_players);
	if(slotCountHumans <= slotCountConnectedPlayers) {
		publishToServerInfo["gameStatus"] = intToStr(game_status_waiting_for_start);
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void MenuStateCustomGame::simpleTask(BaseThread *callingThread) {

    try {
        //printf("-=-=-=-=- IN MenuStateCustomGame simpleTask - A\n");

        MutexSafeWrapper safeMutexThreadOwner(callingThread->getMutexThreadOwnerValid());
        if(callingThread->getQuitStatus() == true || safeMutexThreadOwner.isValidMutex() == false) {
            return;
        }

        //printf("-=-=-=-=- IN MenuStateCustomGame simpleTask - B\n");

        MutexSafeWrapper safeMutex(callingThread->getMutexThreadObjectAccessor());
        bool republish                                  = (needToRepublishToMasterserver == true  && publishToServerInfo.size() != 0);
        needToRepublishToMasterserver                   = false;
        std::map<string,string> newPublishToServerInfo  = publishToServerInfo;
        publishToServerInfo.clear();
        bool broadCastSettings                          = needToBroadcastServerSettings;
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
            SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

            string request = Config::getInstance().getString("Masterserver") + "addServerInfo.php?";

            CURL *handle = SystemFlags::initHTTP();
            for(std::map<string,string>::const_iterator iterMap = newPublishToServerInfo.begin();
                iterMap != newPublishToServerInfo.end(); iterMap++) {

                request += iterMap->first;
                request += "=";
                request += SystemFlags::escapeURL(iterMap->second,handle);
                request += "&";
            }

            //printf("the request is:\n%s\n",request.c_str());
            SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] the request is:\n%s\n",__FILE__,__FUNCTION__,__LINE__,request.c_str());
            safeMutex.ReleaseLock(true);
            safeMutexThreadOwner.ReleaseLock();

            std::string serverInfo = SystemFlags::getHTTP(request,handle);
            SystemFlags::cleanupHTTP(&handle);

            MutexSafeWrapper safeMutexThreadOwner2(callingThread->getMutexThreadOwnerValid());
            if(callingThread->getQuitStatus() == true || safeMutexThreadOwner2.isValidMutex() == false) {
                return;
            }
            safeMutex.Lock();

            //printf("the result is:\n'%s'\n",serverInfo.c_str());
            SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] the result is:\n'%s'\n",__FILE__,__FUNCTION__,__LINE__,serverInfo.c_str());

            // uncomment to enable router setup check of this server
            if(EndsWith(serverInfo, "OK") == false) {
                if(callingThread->getQuitStatus() == true) {
                    return;
                }

                showMasterserverError=true;
                masterServererErrorToShow = (serverInfo != "" ? serverInfo : "No Reply");
            }
        }
        else {
            safeMutexThreadOwner.ReleaseLock();
        }

        SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

        //printf("-=-=-=-=- IN MenuStateCustomGame simpleTask - D\n");

        if(broadCastSettings == true) {
            MutexSafeWrapper safeMutexThreadOwner2(callingThread->getMutexThreadOwnerValid());
            if(callingThread->getQuitStatus() == true || safeMutexThreadOwner2.isValidMutex() == false) {
                return;
            }

            SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
            GameSettings gameSettings;
            loadGameSettings(&gameSettings);

            SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

            if(callingThread->getQuitStatus() == true) {
                return;
            }
            ServerInterface *serverInterface= NetworkManager::getInstance().getServerInterface(false);
            if(serverInterface != NULL) {
                serverInterface->setGameSettings(&gameSettings,false);
            }

            if(hasClientConnection == true && serverInterface != NULL) {
                SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
                serverInterface->broadcastGameSetup(&gameSettings);
            }
        }

        //printf("-=-=-=-=- IN MenuStateCustomGame simpleTask - E\n");

        if(needPing == true) {
            MutexSafeWrapper safeMutexThreadOwner2(callingThread->getMutexThreadOwnerValid());
            if(callingThread->getQuitStatus() == true || safeMutexThreadOwner2.isValidMutex() == false) {
                return;
            }

            lastNetworkPing = time(NULL);

            SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] Sending nmtPing to clients\n",__FILE__,__FUNCTION__,__LINE__);

            ServerInterface *serverInterface= NetworkManager::getInstance().getServerInterface(false);
            if(serverInterface != NULL) {
                SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
                NetworkMessagePing *msg = new NetworkMessagePing(GameConstants::networkPingInterval,time(NULL));
                //serverInterface->broadcastPing(&msg);
                serverInterface->queueBroadcastMessage(msg);
                SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
            }
        }
        safeMutex.ReleaseLock();

        //printf("-=-=-=-=- IN MenuStateCustomGame simpleTask - F\n");
    }
	catch(const std::exception &ex) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s %d] Error detected:\n%s\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);

		if(callingThread->getQuitStatus() == false) {
            //throw runtime_error(szBuf);
            showGeneralError=true;
            generalErrorToShow = ex.what();
		}
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void MenuStateCustomGame::loadGameSettings(GameSettings *gameSettings) {
    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

	int factionCount= 0;
	ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

    // Test flags values
    //gameSettings->setFlagTypes1(ft1_show_map_resources);
    //

	gameSettings->setMapFilterIndex(listBoxMapFilter.getSelectedItemIndex());
	gameSettings->setDescription(formatString(getCurrentMapFile()));
	gameSettings->setMap(getCurrentMapFile());
    gameSettings->setTileset(tilesetFiles[listBoxTileset.getSelectedItemIndex()]);
    gameSettings->setTech(techTreeFiles[listBoxTechTree.getSelectedItemIndex()]);
	gameSettings->setDefaultUnits(true);
	gameSettings->setDefaultResources(true);
	gameSettings->setDefaultVictoryConditions(true);
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

	gameSettings->setEnableObserverModeAtEndGame(listBoxEnableObserverMode.getSelectedItemIndex() == 0);
	gameSettings->setPathFinderType(static_cast<PathFinderType>(listBoxPathFinderType.getSelectedItemIndex()));

	// First save Used slots
    //for(int i=0; i<mapInfo.players; ++i)
	int AIPlayerCount = 0;
	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		ControlType ct= static_cast<ControlType>(listBoxControls[i].getSelectedItemIndex());
		if(ct != ctClosed) {
			int slotIndex = factionCount;

			gameSettings->setFactionControl(slotIndex, ct);
			if(ct == ctHuman) {
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] i = %d, slotIndex = %d, getHumanPlayerName(i) [%s]\n",__FILE__,__FUNCTION__,__LINE__,i,slotIndex,getHumanPlayerName(i).c_str());

				gameSettings->setThisFactionIndex(slotIndex);
				//gameSettings->setNetworkPlayerName(slotIndex, getHumanPlayerName(i));
				gameSettings->setNetworkPlayerName(slotIndex, getHumanPlayerName(i));
				//labelPlayerNames[i].setText(getHumanPlayerName(i));
				//SetActivePlayerNameEditor();
			}
			gameSettings->setResourceMultiplierIndex(slotIndex, listBoxRMultiplier[i].getSelectedItemIndex());
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] i = %d, factionFiles[listBoxFactions[i].getSelectedItemIndex()] [%s]\n",__FILE__,__FUNCTION__,__LINE__,i,factionFiles[listBoxFactions[i].getSelectedItemIndex()].c_str());
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


			if(listBoxControls[i].getSelectedItemIndex() == ctNetwork) {
				if(serverInterface->getSlot(i) != NULL &&
                   serverInterface->getSlot(i)->isConnected()) {
					SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] i = %d, connectionSlot->getName() [%s]\n",__FILE__,__FUNCTION__,__LINE__,i,serverInterface->getSlot(i)->getName().c_str());

					gameSettings->setNetworkPlayerName(slotIndex, serverInterface->getSlot(i)->getName());
					labelPlayerNames[i].setText(serverInterface->getSlot(i)->getName());
				}
				else {
					SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] i = %d, playername unconnected\n",__FILE__,__FUNCTION__,__LINE__,i);

					gameSettings->setNetworkPlayerName(slotIndex, GameConstants::NETWORK_SLOT_UNCONNECTED_SLOTNAME);
					labelPlayerNames[i].setText("");
				}
			}
			else if (listBoxControls[i].getSelectedItemIndex() != ctHuman) {
				AIPlayerCount++;
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] i = %d, playername is AI (blank)\n",__FILE__,__FUNCTION__,__LINE__,i);

				gameSettings->setNetworkPlayerName(slotIndex, string("AI") + intToStr(AIPlayerCount));
				labelPlayerNames[i].setText("");
			}

			factionCount++;
		}
		else {
			//gameSettings->setNetworkPlayerName("");
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

			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] i = %d, factionFiles[listBoxFactions[i].getSelectedItemIndex()] [%s]\n",__FILE__,__FUNCTION__,__LINE__,i,factionFiles[listBoxFactions[i].getSelectedItemIndex()].c_str());
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

    int32 tilesetCRC = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTilesets,""), string("/") + gameSettings->getTileset() + string("/*"), ".xml", NULL);
    gameSettings->setTilesetCRC(tilesetCRC);

    //int32 techCRC    = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTechs,""), "/" + gameSettings->getTech() + "/*", ".xml", NULL);
    Checksum checksum;
    string file = Map::getMapPath(gameSettings->getMap(),"",false);
    checksum.addFile(file);
    int32 mapCRC = checksum.getSum();
    gameSettings->setMapCRC(mapCRC);

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
}

void MenuStateCustomGame::saveGameSettingsToFile(std::string fileName) {
    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

    if(getGameReadWritePath(GameConstants::path_ini_CacheLookupKey) != "") {
    	fileName = getGameReadWritePath(GameConstants::path_ini_CacheLookupKey) + fileName;
    }

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] fileName = [%s]\n",__FILE__,__FUNCTION__,__LINE__,fileName.c_str());

	GameSettings gameSettings;
	loadGameSettings(&gameSettings);

    std::ofstream saveGameFile;
    saveGameFile.open(fileName.c_str(), ios_base::out | ios_base::trunc);

	//int factionCount= 0;
	//ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();

	saveGameFile << "Description=" << gameSettings.getDescription() << std::endl;

	saveGameFile << "MapFilterIndex=" << gameSettings.getMapFilterIndex() << std::endl;
	saveGameFile << "Map=" << gameSettings.getMap() << std::endl;
	saveGameFile << "Tileset=" << gameSettings.getTileset() << std::endl;
	saveGameFile << "TechTree=" << gameSettings.getTech() << std::endl;
	saveGameFile << "DefaultUnits=" << gameSettings.getDefaultUnits() << std::endl;
	saveGameFile << "DefaultResources=" << gameSettings.getDefaultResources() << std::endl;
	saveGameFile << "DefaultVictoryConditions=" << gameSettings.getDefaultVictoryConditions() << std::endl;
	saveGameFile << "FogOfWar=" << gameSettings.getFogOfWar() << std::endl;
	saveGameFile << "AdvancedIndex=" << listBoxAdvanced.getSelectedItemIndex() << std::endl;

	saveGameFile << "AllowObservers=" << gameSettings.getAllowObservers() << std::endl;

	saveGameFile << "FlagTypes1=" << gameSettings.getFlagTypes1() << std::endl;

	saveGameFile << "EnableObserverModeAtEndGame=" << gameSettings.getEnableObserverModeAtEndGame() << std::endl;
	saveGameFile << "PathFinderType=" << gameSettings.getPathFinderType() << std::endl;
	saveGameFile << "EnableServerControlledAI=" << gameSettings.getEnableServerControlledAI() << std::endl;
	saveGameFile << "NetworkFramePeriod=" << gameSettings.getNetworkFramePeriod() << std::endl;
	saveGameFile << "NetworkPauseGameForLaggedClients=" << gameSettings.getNetworkPauseGameForLaggedClients() << std::endl;

	saveGameFile << "FactionThisFactionIndex=" << gameSettings.getThisFactionIndex() << std::endl;
	saveGameFile << "FactionCount=" << gameSettings.getFactionCount() << std::endl;

    //for(int i = 0; i < gameSettings.getFactionCount(); ++i) {
	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		int slotIndex = gameSettings.getStartLocationIndex(i);

		saveGameFile << "FactionControlForIndex" 		<< slotIndex << "=" << gameSettings.getFactionControl(i) << std::endl;
		saveGameFile << "ResourceMultiplierIndex" 			<< slotIndex << "=" << gameSettings.getResourceMultiplierIndex(i) << std::endl;
		saveGameFile << "FactionTeamForIndex" 			<< slotIndex << "=" << gameSettings.getTeam(i) << std::endl;
		saveGameFile << "FactionStartLocationForIndex" 	<< slotIndex << "=" << gameSettings.getStartLocationIndex(i) << std::endl;
		saveGameFile << "FactionTypeNameForIndex" 		<< slotIndex << "=" << gameSettings.getFactionTypeName(i) << std::endl;
		saveGameFile << "FactionPlayerNameForIndex" 	<< slotIndex << "=" << gameSettings.getNetworkPlayerName(i) << std::endl;
    }

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
}

GameSettings MenuStateCustomGame::loadGameSettingsFromFile(std::string fileName) {
    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

    GameSettings gameSettings;

    if(getGameReadWritePath(GameConstants::path_ini_CacheLookupKey) != "") {
    	fileName = getGameReadWritePath(GameConstants::path_ini_CacheLookupKey) + fileName;
    }

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] fileName = [%s]\n",__FILE__,__FUNCTION__,__LINE__,fileName.c_str());

    if(fileExists(fileName) == false) {
    	return gameSettings;
    }

    try {
		Properties properties;
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] fileName = [%s]\n",__FILE__,__FUNCTION__,__LINE__,fileName.c_str());
		properties.load(fileName);
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] fileName = [%s]\n",__FILE__,__FUNCTION__,__LINE__,fileName.c_str());

		gameSettings.setMapFilterIndex(properties.getInt("MapFilterIndex","0"));
		gameSettings.setDescription(properties.getString("Description"));
		gameSettings.setMap(properties.getString("Map"));
		gameSettings.setTileset(properties.getString("Tileset"));
		gameSettings.setTech(properties.getString("TechTree"));
		gameSettings.setDefaultUnits(properties.getBool("DefaultUnits"));
		gameSettings.setDefaultResources(properties.getBool("DefaultResources"));
		gameSettings.setDefaultVictoryConditions(properties.getBool("DefaultVictoryConditions"));
		gameSettings.setFogOfWar(properties.getBool("FogOfWar"));
		listBoxAdvanced.setSelectedItemIndex(properties.getInt("AdvancedIndex","0"));

		gameSettings.setAllowObservers(properties.getBool("AllowObservers","false"));

        gameSettings.setFlagTypes1(properties.getInt("FlagTypes1","0"));

		gameSettings.setEnableObserverModeAtEndGame(properties.getBool("EnableObserverModeAtEndGame"));
		gameSettings.setPathFinderType(static_cast<PathFinderType>(properties.getInt("PathFinderType",intToStr(pfBasic).c_str())));
		gameSettings.setEnableServerControlledAI(properties.getBool("EnableServerControlledAI","true"));
		gameSettings.setNetworkFramePeriod(properties.getInt("NetworkFramePeriod",intToStr(GameConstants::networkFramePeriod).c_str()));
		gameSettings.setNetworkPauseGameForLaggedClients(properties.getBool("NetworkPauseGameForLaggedClients","false"));

		gameSettings.setThisFactionIndex(properties.getInt("FactionThisFactionIndex"));
		gameSettings.setFactionCount(properties.getInt("FactionCount"));

		//for(int i = 0; i < gameSettings.getFactionCount(); ++i) {
		for(int i = 0; i < GameConstants::maxPlayers; ++i) {
			gameSettings.setFactionControl(i,(ControlType)properties.getInt(string("FactionControlForIndex") + intToStr(i),intToStr(ctClosed).c_str()) );
			gameSettings.setResourceMultiplierIndex(i,properties.getInt(string("ResourceMultiplierIndex") + intToStr(i),"5"));
			gameSettings.setTeam(i,properties.getInt(string("FactionTeamForIndex") + intToStr(i),"0") );
			gameSettings.setStartLocationIndex(i,properties.getInt(string("FactionStartLocationForIndex") + intToStr(i),intToStr(i).c_str()) );
			gameSettings.setFactionTypeName(i,properties.getString(string("FactionTypeNameForIndex") + intToStr(i),"?") );

			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] i = %d, factionTypeName [%s]\n",__FILE__,__FUNCTION__,__LINE__,i,gameSettings.getFactionTypeName(i).c_str());

			if(gameSettings.getFactionControl(i) == ctHuman) {
				gameSettings.setNetworkPlayerName(i,properties.getString(string("FactionPlayerNameForIndex") + intToStr(i),"") );
			}
			else {
				gameSettings.setNetworkPlayerName(i,"");
			}
		}

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

		listBoxMapFilter.setSelectedItemIndex(gameSettings.getMapFilterIndex());
		listBoxMap.setItems(formattedPlayerSortedMaps[gameSettings.getMapFilterIndex()]);

		string mapFile = gameSettings.getMap();
		mapFile = formatString(mapFile);
		listBoxMap.setSelectedItem(mapFile);

		loadMapInfo(Map::getMapPath(getCurrentMapFile()), &mapInfo, true);
		labelMapInfo.setText(mapInfo.desc);

		string tilesetFile = gameSettings.getTileset();
		tilesetFile = formatString(tilesetFile);

		listBoxTileset.setSelectedItem(tilesetFile);

		string techtreeFile = gameSettings.getTech();
		techtreeFile = formatString(techtreeFile);

		listBoxTechTree.setSelectedItem(techtreeFile);

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

		//gameSettings->setDefaultUnits(true);
		//gameSettings->setDefaultResources(true);
		//gameSettings->setDefaultVictoryConditions(true);

		Lang &lang= Lang::getInstance();

		//FogOfWar
		listBoxFogOfWar.setSelectedItemIndex(0); // default is 0!
		if(gameSettings.getFogOfWar() == false){
			listBoxFogOfWar.setSelectedItemIndex(2);
		}
		if((gameSettings.getFlagTypes1() & ft1_show_map_resources) == ft1_show_map_resources){
			if(gameSettings.getFogOfWar() == true){
        		listBoxFogOfWar.setSelectedItemIndex(1);
			}
		}

		listBoxAllowObservers.setSelectedItem(gameSettings.getAllowObservers() == true ? lang.get("Yes") : lang.get("No"));
		listBoxEnableObserverMode.setSelectedItem(gameSettings.getEnableObserverModeAtEndGame() == true ? lang.get("Yes") : lang.get("No"));
		listBoxPathFinderType.setSelectedItemIndex(gameSettings.getPathFinderType());

		//listBoxEnableServerControlledAI.setSelectedItem(gameSettings.getEnableServerControlledAI() == true ? lang.get("Yes") : lang.get("No"));

		//labelNetworkFramePeriod.setText(lang.get("NetworkFramePeriod"));

		//listBoxNetworkFramePeriod.setSelectedItem(intToStr(gameSettings.getNetworkFramePeriod()/10*10));

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

		listBoxNetworkPauseGameForLaggedClients.setSelectedItemIndex(gameSettings.getNetworkPauseGameForLaggedClients());

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

		reloadFactions(false);

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d] gameSettings.getFactionCount() = %d\n",__FILE__,__FUNCTION__,__LINE__,gameSettings.getFactionCount());

		//for(int i = 0; i < gameSettings.getFactionCount(); ++i) {
		for(int i = 0; i < GameConstants::maxPlayers; ++i) {
			listBoxControls[i].setSelectedItemIndex(gameSettings.getFactionControl(i));
			updateResourceMultiplier(i);
			listBoxRMultiplier[i].setSelectedItemIndex(gameSettings.getResourceMultiplierIndex(i));
			listBoxTeams[i].setSelectedItemIndex(gameSettings.getTeam(i));
			lastSelectedTeamIndex[i] = listBoxTeams[i].getSelectedItemIndex();

			string factionName = gameSettings.getFactionTypeName(i);
			factionName = formatString(factionName);

			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] factionName = [%s]\n",__FILE__,__FUNCTION__,__LINE__,factionName.c_str());

			listBoxFactions[i].setSelectedItem(factionName);

			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] i = %d, gameSettings.getNetworkPlayerName(i) [%s]\n",__FILE__,__FUNCTION__,__LINE__,i,gameSettings.getNetworkPlayerName(i).c_str());

			labelPlayerNames[i].setText(gameSettings.getNetworkPlayerName(i));
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
    catch(const exception &ex) {
    	SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
    	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] ERROR = [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
    	showMessageBox( ex.what(), "Error", false);

    	gameSettings = GameSettings();
    }

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

	return gameSettings;
}

// ============ PRIVATE ===========================

bool MenuStateCustomGame::hasNetworkGameSettings() {
    bool hasNetworkSlot = false;

    try {
		for(int i=0; i<mapInfo.players; ++i)
		{
			ControlType ct= static_cast<ControlType>(listBoxControls[i].getSelectedItemIndex());
			if(ct != ctClosed)
			{
				if(ct == ctNetwork)
				{
					hasNetworkSlot = true;
					break;
				}
			}
		}
    }
	catch(const std::exception &ex) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s %d] Error detected:\n%s\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);
		showGeneralError=true;
		generalErrorToShow = ex.what();
	}

    return hasNetworkSlot;
}

void MenuStateCustomGame::loadMapInfo(string file, MapInfo *mapInfo, bool loadMapPreview) {

	struct MapFileHeader{
		int32 version;
		int32 maxPlayers;
		int32 width;
		int32 height;
		int32 altFactor;
		int32 waterLevel;
		int8 title[128];
	};

	Lang &lang= Lang::getInstance();

	try{
		FILE *f= fopen(file.c_str(), "rb");
		if(f==NULL)
			throw runtime_error("Can't open file");

		MapFileHeader header;
		size_t readBytes = fread(&header, sizeof(MapFileHeader), 1, f);

		mapInfo->size.x= header.width;
		mapInfo->size.y= header.height;
		mapInfo->players= header.maxPlayers;

		mapInfo->desc= lang.get("MaxPlayers")+": "+intToStr(mapInfo->players)+"\n";
		mapInfo->desc+=lang.get("Size")+": "+intToStr(mapInfo->size.x) + " x " + intToStr(mapInfo->size.y);

		fclose(f);

	    for(int i = 0; i < GameConstants::maxPlayers; ++i) {
			labelPlayers[i].setVisible(i+1 <= mapInfo->players);
			labelPlayerNames[i].setVisible(i+1 <= mapInfo->players);
	        listBoxControls[i].setVisible(i+1 <= mapInfo->players);
	        listBoxFactions[i].setVisible(i+1 <= mapInfo->players);
			listBoxTeams[i].setVisible(i+1 <= mapInfo->players);
			labelNetStatus[i].setVisible(i+1 <= mapInfo->players);
	    }

	    // Not painting properly so this is on hold
	    if(loadMapPreview == true) {
	    	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	    	mapPreview.loadFromFile(file.c_str());

	    	//printf("Loading map preview MAP\n");
	    	cleanupMapPreviewTexture();
	    }
	}
	catch(exception &e) {
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s] loading map [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what(),file.c_str());
		throw runtime_error("Error loading map file: [" + file + "] msg: " + e.what());
	}

}

void MenuStateCustomGame::reloadFactions(bool keepExistingSelectedItem) {

	vector<string> results;

    Config &config = Config::getInstance();

    vector<string> techPaths = config.getPathListForType(ptTechs);
    for(int idx = 0; idx < techPaths.size(); idx++) {
        string &techPath = techPaths[idx];

        findAll(techPath + "/" + techTreeFiles[listBoxTechTree.getSelectedItemIndex()] + "/factions/*.", results, false, false);
        if(results.size() > 0) {
            break;
        }
    }

    if(results.size() == 0) {
        //throw runtime_error("(2)There are no factions for the tech tree [" + techTreeFiles[listBoxTechTree.getSelectedItemIndex()] + "]");
		showGeneralError=true;
		generalErrorToShow = "[#2] There are no factions for the tech tree [" + techTreeFiles[listBoxTechTree.getSelectedItemIndex()] + "]";
    }

    results.push_back(formatString(GameConstants::RANDOMFACTION_SLOTNAME));

    // Add special Observer Faction
    //Lang &lang= Lang::getInstance();
    if(listBoxAllowObservers.getSelectedItemIndex() == 1) {
    	results.push_back(formatString(GameConstants::OBSERVER_SLOTNAME));
    }

    factionFiles= results;
    for(int i= 0; i<results.size(); ++i){
        results[i]= formatString(results[i]);

        SystemFlags::OutputDebug(SystemFlags::debugSystem,"Tech [%s] has faction [%s]\n",techTreeFiles[listBoxTechTree.getSelectedItemIndex()].c_str(),results[i].c_str());
    }

    for(int i=0; i<GameConstants::maxPlayers; ++i){
    	int originalIndex = listBoxFactions[i].getSelectedItemIndex();
    	//printf("[a] i = %d, originalIndex = %d\n",i,originalIndex);

    	string originalValue = (listBoxFactions[i].getItemCount() > 0 ? listBoxFactions[i].getSelectedItem() : "");

    	//printf("[a1] i = %d, originalValue = [%s]\n",i,originalValue.c_str());

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
        	//printf("[b] i = %d, listBoxFactions[i].getSelectedItemIndex() = %d\n",i,listBoxFactions[i].getSelectedItemIndex());
        }
        else if(originalIndex < results.size()) {
        	listBoxFactions[i].setSelectedItemIndex(originalIndex);

        	//printf("[c] i = %d, originalIndex = %d\n",i,originalIndex);
        }
    }
}

void MenuStateCustomGame::updateControlers(){
	try {
		bool humanPlayer= false;

		for(int i= 0; i<mapInfo.players; ++i){
			if(listBoxControls[i].getSelectedItemIndex() == ctHuman){
				humanPlayer= true;
			}
		}

		if(!humanPlayer){
			listBoxControls[0].setSelectedItemIndex(ctHuman);
		}

		for(int i= mapInfo.players; i<GameConstants::maxPlayers; ++i){
			listBoxControls[i].setSelectedItemIndex(ctClosed);
		}
	}
	catch(const std::exception &ex) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s %d] Error detected:\n%s\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		throw runtime_error(szBuf);
	}
}

void MenuStateCustomGame::closeUnusedSlots(){
	try {
		ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();
		for(int i= 0; i<mapInfo.players; ++i){
			if(listBoxControls[i].getSelectedItemIndex() == ctNetwork){
				if(serverInterface->getSlot(i) == NULL ||
                   serverInterface->getSlot(i)->isConnected() == false) {
					listBoxControls[i].setSelectedItemIndex(ctClosed);
				}
			}
		}
		updateNetworkSlots();
	}
	catch(const std::exception &ex) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s %d] Error detected:\n%s\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		throw runtime_error(szBuf);
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
					sprintf(szBuf,"In [%s::%s %d] Error detected:\n%s\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
					SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
					SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);
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
			if(serverInterface->getSlot(i) != NULL &&
				listBoxControls[i].getSelectedItemIndex() != ctNetwork) {
				serverInterface->removeSlot(i);
			}
		}
	}
	catch(const std::exception &ex) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s %d] Error detected:\n%s\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);
		//throw runtime_error(szBuf);
		showGeneralError=true;
		generalErrorToShow = ex.what();

	}
}

void MenuStateCustomGame::keyDown(char key) {
	if(activeInputLabel != NULL) {
		string text = activeInputLabel->getText();
		if(key == vkBack && text.length() > 0) {
			size_t found = text.find_last_of("_");
			if (found == string::npos) {
				text.erase(text.end() - 1);
			}
			else {
				if(text.size() > 1) {
					text.erase(text.end() - 2);
				}
			}

			activeInputLabel->setText(text);

			MutexSafeWrapper safeMutex((publishToMasterserverThread != NULL ? publishToMasterserverThread->getMutexThreadObjectAccessor() : NULL));
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

			if(key == configKeys.getCharKey("ShowFullConsole")) {
				showFullConsole= true;
			}
			//Toggle music
			else if(key == configKeys.getCharKey("ToggleMusic")) {
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
			else if(key == configKeys.getCharKey("SaveGUILayout")) {
				bool saved = GraphicComponent::saveAllCustomProperties(containerName);
				Lang &lang= Lang::getInstance();
				console.addLine(lang.get("GUILayoutSaved") + " [" + (saved ? lang.get("Yes") : lang.get("No"))+ "]");
			}
		}
	}
}

void MenuStateCustomGame::keyPress(char c) {
	if(activeInputLabel != NULL) {
		int maxTextSize= 16;
	    for(int i = 0; i < GameConstants::maxPlayers; ++i) {
			if(&labelPlayerNames[i] == activeInputLabel) {
				if((c>='0' && c<='9') || (c>='a' && c<='z') || (c>='A' && c<='Z') ||
				   (c=='-') || (c=='(') || (c==')')) {
					if(activeInputLabel->getText().size() < maxTextSize) {
						string text= activeInputLabel->getText();
						text.insert(text.end()-1, c);
						activeInputLabel->setText(text);

						MutexSafeWrapper safeMutex((publishToMasterserverThread != NULL ? publishToMasterserverThread->getMutexThreadObjectAccessor() : NULL));
				        if(hasNetworkGameSettings() == true) {
				            needToSetChangedGameSettings = true;
				            lastSetChangedGameSettings   = time(NULL);
				        }
					}
				}
			}
	    }
	}
	else {
		if(hasNetworkGameSettings() == true) {
			chatManager.keyPress(c);
		}
	}
}

void MenuStateCustomGame::keyUp(char key) {
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
		else if(key == configKeys.getCharKey("ShowFullConsole")) {
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

	if(newLable != NULL) {
		string text= newLable->getText();
		size_t found = text.find_last_of("_");
		if (found == string::npos) {
			text += "_";
		}
		newLable->setText(text);
	}
	if(activeInputLabel != NULL && activeInputLabel->getText().empty() == false) {
		string text= activeInputLabel->getText();
		size_t found = text.find_last_of("_");
		if (found != string::npos) {
			text = text.substr(0,found);
		}
		activeInputLabel->setText(text);
	}
	activeInputLabel = newLable;

}

string MenuStateCustomGame::getHumanPlayerName(int index) {
	string  result = defaultPlayerName;

	//printf("\nIn [%s::%s Line: %d] index = %d\n",__FILE__,__FUNCTION__,__LINE__,index);
	//fflush(stdout);


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

	//printf("\nIn [%s::%s Line: %d] index = %d, labelPlayerNames[index].getText() = [%s]\n",__FILE__,__FUNCTION__,__LINE__,index,(index >= 0 ? labelPlayerNames[index].getText().c_str() : "?"));
	//fflush(stdout);

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

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(enableFactionTexturePreview == true) {
		if(filepath == "") {
			factionTexture = NULL;
		}
		else {
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] filepath = [%s]\n",__FILE__,__FUNCTION__,__LINE__,filepath.c_str());
			factionTexture = Renderer::findFactionLogoTexture(filepath);
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		}
	}
}

void MenuStateCustomGame::cleanupMapPreviewTexture() {
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//printf("CLEANUP map preview texture\n");

	if(mapPreviewTexture != NULL) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		mapPreviewTexture->end();

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		delete mapPreviewTexture;
		mapPreviewTexture = NULL;
	}
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

}}//end namespace
