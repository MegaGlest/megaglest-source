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

MenuStateCustomGame::MenuStateCustomGame(Program *program, MainMenu *mainMenu, bool openNetworkSlots,bool parentMenuIsMasterserver):
	MenuState(program, mainMenu, "new-game")
{
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

	activeInputLabel=NULL;
	showGeneralError = false;
	generalErrorToShow = "---";

	publishToMasterserverThread = NULL;
	Lang &lang= Lang::getInstance();
	NetworkManager &networkManager= NetworkManager::getInstance();
    Config &config = Config::getInstance();
    defaultPlayerName = config.getString("NetPlayerName",Socket::getHostName().c_str());
    
    showFullConsole=false;

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
		char szBuf[1024]="";
		sprintf(szBuf,"In [%s::%s %d] error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
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

	vector<string> teamItems, controlItems, results;

	//create
	buttonReturn.init(250, 180, 125);
	buttonRestoreLastSettings.init(250+130, 180, 200);
	buttonPlayNow.init(250+130+205, 180, 125);

	int setupPos=590;
	int mapHeadPos=330;
	int mapPos=mapHeadPos-30;
	int aHeadPos=260;
	int aPos=aHeadPos-30;
	int networkHeadPos=700;
	int networkPos=networkHeadPos-30;
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
	std::for_each(results.begin(), results.end(), FormatString());

	labelLocalIP.init(410, networkHeadPos+30);

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
	labelLocalIP.setText(lang.get("LanIP") + ipText);

	// Map
	xoffset=70;
	labelMap.init(xoffset+100, mapHeadPos);
	labelMap.setText(lang.get("Map")+":");
	listBoxMap.init(xoffset+100, mapPos, 200);
    listBoxMap.setItems(results);
	labelMapInfo.init(xoffset+100, mapPos-30, 200, 40);
	
	// MapFilter
	labelMapFilter.init(xoffset+310, mapHeadPos);
	labelMapFilter.setText(lang.get("MapFilter")+":");
	listBoxMapFilter.init(xoffset+310, mapPos, 80);
	listBoxMapFilter.pushBackItem(lang.get("all"));
	listBoxMapFilter.pushBackItem("1");
	listBoxMapFilter.pushBackItem("2");
	listBoxMapFilter.pushBackItem("3");
	listBoxMapFilter.pushBackItem("4");
	listBoxMapFilter.pushBackItem("5");
	listBoxMapFilter.pushBackItem("6");
	listBoxMapFilter.pushBackItem("7");
	listBoxMapFilter.pushBackItem("8");
	listBoxMapFilter.setSelectedItemIndex(4);
	
	//tileset listBox
    findDirs(config.getPathListForType(ptTilesets), results);
	if (results.empty()) {
        throw runtime_error("No tile-sets were found!");
	}
    tilesetFiles= results;
	std::for_each(results.begin(), results.end(), FormatString());
	listBoxTileset.init(xoffset+460, mapPos, 150);
    listBoxTileset.setItems(results);
	labelTileset.init(xoffset+460, mapHeadPos);
	labelTileset.setText(lang.get("Tileset"));
	

    //tech Tree listBox
    findDirs(config.getPathListForType(ptTechs), results);
	if(results.empty()) {
        throw runtime_error("No tech-trees were found!");
	}
    techTreeFiles= results;
	std::for_each(results.begin(), results.end(), FormatString());
	listBoxTechTree.init(xoffset+650, mapPos, 150);
    listBoxTechTree.setItems(results);
	labelTechTree.init(xoffset+650, mapHeadPos);
	labelTechTree.setText(lang.get("TechTree"));
	
		
	// fog - o - war
	// @350 ? 300 ?
	
	labelFogOfWar.init(xoffset+310, aHeadPos, 80);
	labelFogOfWar.setText(lang.get("FogOfWar"));
	listBoxFogOfWar.init(xoffset+310, aPos, 80);
	listBoxFogOfWar.pushBackItem(lang.get("Yes"));
	listBoxFogOfWar.pushBackItem(lang.get("No"));
	listBoxFogOfWar.setSelectedItemIndex(0);

	// Enable Observer Mode
	labelEnableObserverMode.init(xoffset+460, aHeadPos, 80);
	listBoxEnableObserverMode.init(xoffset+460, aPos, 150);
	listBoxEnableObserverMode.pushBackItem(lang.get("Yes"));
	listBoxEnableObserverMode.pushBackItem(lang.get("No"));
	listBoxEnableObserverMode.setSelectedItemIndex(0);

	// Witch Pathfinder
	labelPathFinderType.init(xoffset+650, aHeadPos, 80);
	labelPathFinderType.setText(lang.get("PathFinderType"));
	listBoxPathFinderType.init(xoffset+650, aPos, 150);
	listBoxPathFinderType.pushBackItem(lang.get("PathFinderTypeRegular"));
	listBoxPathFinderType.pushBackItem(lang.get("PathFinderTypeRoutePlanner"));
	listBoxPathFinderType.setSelectedItemIndex(0);


	// Advanced Options
	labelAdvanced.init(790, 80, 80);
	labelAdvanced.setText(lang.get("AdvancedGameOptions"));
	listBoxAdvanced.init(810, 80-30, 80);
	listBoxAdvanced.pushBackItem(lang.get("Yes"));
	listBoxAdvanced.pushBackItem(lang.get("No"));
	listBoxAdvanced.setSelectedItemIndex(0);

	// network things
	// PublishServer
	xoffset=50;
	labelPublishServer.init(xoffset+50, networkHeadPos, 100);
	labelPublishServer.setText(lang.get("PublishServer"));
	listBoxPublishServer.init(xoffset+60, networkPos, 100);
	listBoxPublishServer.pushBackItem(lang.get("Yes"));
	listBoxPublishServer.pushBackItem(lang.get("No"));
	if(openNetworkSlots)
		listBoxPublishServer.setSelectedItemIndex(0);
	else
		listBoxPublishServer.setSelectedItemIndex(1);

	// Port
	labelPublishServerExternalPort.init(xoffset+220, networkHeadPos, 150);
	labelPublishServerExternalPort.setText(lang.get("PublishServerExternalPort"));

	listBoxPublishServerExternalPort.init(xoffset+230, networkPos, 100);
	string supportExternalPortList = config.getString("MasterServerExternalPortList",intToStr(GameConstants::serverPort).c_str());
	std::vector<std::string> externalPortList;
	Tokenize(supportExternalPortList,externalPortList,",");

	for(int idx = 0; idx < externalPortList.size(); idx++) {
		if(externalPortList[idx] != "" && IsNumeric(externalPortList[idx].c_str(),false)) {
			listBoxPublishServerExternalPort.pushBackItem(externalPortList[idx]);
		}
	}
	
	//listBoxPublishServer.setSelectedItemIndex(0);

	// Network Frame Period
	labelNetworkFramePeriod.init(xoffset+370, networkHeadPos, 80);
	labelNetworkFramePeriod.setText(lang.get("NetworkFramePeriod"));
	listBoxNetworkFramePeriod.init(xoffset+380, networkPos, 80);
	listBoxNetworkFramePeriod.pushBackItem("10");
	listBoxNetworkFramePeriod.pushBackItem("20");
	listBoxNetworkFramePeriod.pushBackItem("30");
	listBoxNetworkFramePeriod.pushBackItem("40");
	listBoxNetworkFramePeriod.setSelectedItem("20");

	// Network Frame Period
	labelNetworkPauseGameForLaggedClients.init(xoffset+530, networkHeadPos, 80);
	labelNetworkPauseGameForLaggedClients.setText(lang.get("NetworkPauseGameForLaggedClients"));
	listBoxNetworkPauseGameForLaggedClients.init(xoffset+540, networkPos, 80);
	listBoxNetworkPauseGameForLaggedClients.pushBackItem(lang.get("No"));
	listBoxNetworkPauseGameForLaggedClients.pushBackItem(lang.get("Yes"));
	listBoxNetworkPauseGameForLaggedClients.setSelectedItem(lang.get("Yes"));


	// Enable Server Controlled AI
	labelEnableServerControlledAI.init(xoffset+670, networkHeadPos, 80);
	labelEnableServerControlledAI.setText(lang.get("EnableServerControlledAI"));
	listBoxEnableServerControlledAI.init(xoffset+680, networkPos, 80);
	listBoxEnableServerControlledAI.pushBackItem(lang.get("Yes"));
	listBoxEnableServerControlledAI.pushBackItem(lang.get("No"));
	listBoxEnableServerControlledAI.setSelectedItemIndex(0);

	//list boxes
	xoffset=120;
	int rowHeight=27;
    for(int i=0; i<GameConstants::maxPlayers; ++i){
		labelPlayers[i].init(xoffset+50, setupPos-30-i*rowHeight);
		labelPlayerNames[i].init(xoffset+100,setupPos-30-i*rowHeight);

        listBoxControls[i].init(xoffset+200, setupPos-30-i*rowHeight);
        listBoxFactions[i].init(xoffset+350, setupPos-30-i*rowHeight, 150);
		listBoxTeams[i].init(xoffset+520, setupPos-30-i*rowHeight, 60);
		labelNetStatus[i].init(xoffset+600, setupPos-30-i*rowHeight, 60);
    }

	labelControl.init(xoffset+200, setupPos, GraphicListBox::defW, GraphicListBox::defH, true);
	labelControl.setText(lang.get("Control"));
	
    labelFaction.init(xoffset+350, setupPos, GraphicListBox::defW, GraphicListBox::defH, true);
    labelFaction.setText(lang.get("Faction"));
    labelTeam.init(xoffset+520, setupPos, 50, GraphicListBox::defH, true);
    labelTeam.setText(lang.get("Team"));
    
    labelControl.setFont(CoreData::getInstance().getMenuFontBig());
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
	teamItems.push_back("1");
	teamItems.push_back("2");
	teamItems.push_back("3");
	teamItems.push_back("4");
	teamItems.push_back("5");
	teamItems.push_back("6");
	teamItems.push_back("7");
	teamItems.push_back("8");

	reloadFactions();

    vector<string> techPaths = config.getPathListForType(ptTechs);
    for(int idx = 0; idx < techPaths.size(); idx++) {
        string &techPath = techPaths[idx];
        findAll(techPath + "/" + techTreeFiles[listBoxTechTree.getSelectedItemIndex()] + "/factions/*.", results, false, false);

        if(results.size() > 0) {
            break;
        }
    }

    if(results.size() == 0) {
        throw runtime_error("(1)There are no factions for the tech tree [" + techTreeFiles[listBoxTechTree.getSelectedItemIndex()] + "]");
    }

	for(int i=0; i<GameConstants::maxPlayers; ++i){
		labelPlayers[i].setText(lang.get("Player")+" "+intToStr(i));
		labelPlayerNames[i].setText("*");

        listBoxTeams[i].setItems(teamItems);
		listBoxTeams[i].setSelectedItemIndex(i);
		listBoxControls[i].setItems(controlItems);
		labelNetStatus[i].setText("");
    }


    labelEnableObserverMode.setText(lang.get("EnableObserverMode"));
    

	loadMapInfo(Map::getMapPath(mapFiles[listBoxMap.getSelectedItemIndex()]), &mapInfo);

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

	//chatManager.init(&console, world.getThisTeamIndex());
	chatManager.init(&console, -1,true);

	publishToMasterserverThread = new SimpleTaskThread(this,0,25);
	publishToMasterserverThread->setUniqueID(__FILE__);
	publishToMasterserverThread->start();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

MenuStateCustomGame::~MenuStateCustomGame() {
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

	needToBroadcastServerSettings = false;
	needToRepublishToMasterserver = false;

	//BaseThread::shutdownAndWait(publishToMasterserverThread);
	delete publishToMasterserverThread;
	publishToMasterserverThread = NULL;

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void MenuStateCustomGame::returnToParentMenu(){
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

	needToBroadcastServerSettings = false;
	needToRepublishToMasterserver = false;
	bool returnToMasterServerMenu = parentMenuIsMs;

	//BaseThread::shutdownAndWait(publishToMasterserverThread);
	delete publishToMasterserverThread;
	publishToMasterserverThread = NULL;

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(returnToMasterServerMenu) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
		mainMenu->setState(new MenuStateMasterserver(program, mainMenu));
	}
	else {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
		mainMenu->setState(new MenuStateNewGame(program, mainMenu));
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void MenuStateCustomGame::mouseClick(int x, int y, MouseButton mouseButton){

	CoreData &coreData= CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();

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

		MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
		needToBroadcastServerSettings = false;
		needToRepublishToMasterserver = false;
		safeMutex.ReleaseLock();

		//BaseThread::shutdownAndWait(publishToMasterserverThread);
		delete publishToMasterserverThread;
		publishToMasterserverThread = NULL;

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		returnToParentMenu();
    }
	else if(buttonPlayNow.mouseClick(x,y) && buttonPlayNow.getEnabled()) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
		saveGameSettingsToFile("lastCustomGamSettings.mgg");

		closeUnusedSlots();
		soundRenderer.playFx(coreData.getClickSoundC());

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		safeMutex.ReleaseLock(true);
		GameSettings gameSettings;
		loadGameSettings(&gameSettings);

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
		ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();

        // Send the game settings to each client if we have at least one networked client
		safeMutex.Lock();
        if( hasNetworkGameSettings() == true &&
            needToSetChangedGameSettings == true)
        {
        	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
            serverInterface->setGameSettings(&gameSettings,true);

            needToSetChangedGameSettings    = false;
            lastSetChangedGameSettings      = time(NULL);
        }

        SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
		bool bOkToStart = serverInterface->launchGame(&gameSettings);
		if(bOkToStart == true) {
			if( listBoxPublishServer.getEditable() &&
				listBoxPublishServer.getSelectedItemIndex() == 0) {

				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

				needToRepublishToMasterserver = true;
				lastMasterserverPublishing = 0;

				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
			}
			needToBroadcastServerSettings = false;
			needToRepublishToMasterserver = false;
			safeMutex.ReleaseLock();

			delete publishToMasterserverThread;
			publishToMasterserverThread = NULL;

			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

			assert(program != NULL);

			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
            program->setState(new Game(program, &gameSettings));
		}
		else {
			safeMutex.ReleaseLock();
		}

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}
	else if(buttonRestoreLastSettings.mouseClick(x,y) && buttonRestoreLastSettings.getEnabled()) {
		// Ensure we have set the gamesettings at least once
		GameSettings gameSettings = loadGameSettingsFromFile("lastCustomGamSettings.mgg");
		if(gameSettings.getMap() == "") {
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

			loadGameSettings(&gameSettings);
		}

		ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();
		serverInterface->setGameSettings(&gameSettings,false);

		MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
		needToRepublishToMasterserver = true;

        if(hasNetworkGameSettings() == true)
        {
            needToSetChangedGameSettings = true;
            lastSetChangedGameSettings   = time(NULL);
        }
	}
	else if(listBoxMap.mouseClick(x, y)){
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s\n", mapFiles[listBoxMap.getSelectedItemIndex()].c_str());

		MutexSafeWrapper safeMutex(&masterServerThreadAccessor);

		loadMapInfo(Map::getMapPath(mapFiles[listBoxMap.getSelectedItemIndex()]), &mapInfo);
		labelMapInfo.setText(mapInfo.desc);
		updateControlers();
		updateNetworkSlots();
		needToRepublishToMasterserver = true;

        if(hasNetworkGameSettings() == true)
        {
            needToSetChangedGameSettings = true;
            lastSetChangedGameSettings   = time(NULL);
        }
	}
	else if (listBoxFogOfWar.mouseClick(x, y)) {
		MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
		needToRepublishToMasterserver = true;

        if(hasNetworkGameSettings() == true)
        {
            needToSetChangedGameSettings = true;
            lastSetChangedGameSettings   = time(NULL);
        }
	}
	else if (listBoxEnableObserverMode.mouseClick(x, y)) {
		MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
		needToRepublishToMasterserver = true;

        if(hasNetworkGameSettings() == true)
        {
            needToSetChangedGameSettings = true;
            lastSetChangedGameSettings   = time(NULL);
        }
	}
	else if (listBoxPathFinderType.mouseClick(x, y)) {
		MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
		needToRepublishToMasterserver = true;

        if(hasNetworkGameSettings() == true)
        {
            needToSetChangedGameSettings = true;
            lastSetChangedGameSettings   = time(NULL);
        }
	}
	else if (listBoxAdvanced.mouseClick(x, y)) {
		//TODO
	}
	else if (listBoxEnableServerControlledAI.mouseClick(x, y)&&listBoxEnableServerControlledAI.getEditable()) {
		MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
		needToRepublishToMasterserver = true;

        if(hasNetworkGameSettings() == true)
        {
            needToSetChangedGameSettings = true;
            lastSetChangedGameSettings   = time(NULL);
        }
	}
	else if(listBoxTileset.mouseClick(x, y)){
		MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
		// TODO: set a map that fits the condition
		
		needToRepublishToMasterserver = true;
        if(hasNetworkGameSettings() == true)
        {
            needToSetChangedGameSettings = true;
            lastSetChangedGameSettings   = time(NULL);
        }
	}
	else if(listBoxMapFilter.mouseClick(x, y)){
		MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
		needToRepublishToMasterserver = true;

        if(hasNetworkGameSettings() == true)
        {
            needToSetChangedGameSettings = true;
            lastSetChangedGameSettings   = time(NULL);
        }
	}
	else if(listBoxTechTree.mouseClick(x, y)){
		reloadFactions();

		MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
		needToRepublishToMasterserver = true;

        if(hasNetworkGameSettings() == true)
        {
            needToSetChangedGameSettings = true;
            lastSetChangedGameSettings   = time(NULL);
        }
	}
	else if(listBoxPublishServer.mouseClick(x, y) && listBoxPublishServer.getEditable()) {
		MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
		needToRepublishToMasterserver = true;
		soundRenderer.playFx(coreData.getClickSoundC());
	}
	else if(listBoxPublishServerExternalPort.mouseClick(x, y) && listBoxPublishServerExternalPort.getEditable()) {
		MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
		needToRepublishToMasterserver = true;
		soundRenderer.playFx(coreData.getClickSoundC());
	}
	else if(listBoxNetworkFramePeriod.mouseClick(x, y)){
		MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
		needToRepublishToMasterserver = true;
        if(hasNetworkGameSettings() == true)
        {
            needToSetChangedGameSettings = true;
            lastSetChangedGameSettings   = time(NULL);
        }

		soundRenderer.playFx(coreData.getClickSoundC());
	}
	else if(listBoxNetworkPauseGameForLaggedClients.mouseClick(x, y)){
		MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
		needToRepublishToMasterserver = true;
        if(hasNetworkGameSettings() == true)
        {
            needToSetChangedGameSettings = true;
            lastSetChangedGameSettings   = time(NULL);
        }

		soundRenderer.playFx(coreData.getClickSoundC());
	}
	else {
		for(int i=0; i<mapInfo.players; ++i) {
			MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
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

				needToRepublishToMasterserver = true;
                if(hasNetworkGameSettings() == true) {
                    needToSetChangedGameSettings = true;
                    lastSetChangedGameSettings   = time(NULL);
                }
			}
			else if(listBoxFactions[i].mouseClick(x, y)){
				needToRepublishToMasterserver = true;

                if(hasNetworkGameSettings() == true)
                {
                    needToSetChangedGameSettings = true;
                    lastSetChangedGameSettings   = time(NULL);
                }
			}
			else if(listBoxTeams[i].mouseClick(x, y))
			{
				needToRepublishToMasterserver = true;

                if(hasNetworkGameSettings() == true)
                {
                    needToSetChangedGameSettings = true;
                    lastSetChangedGameSettings   = time(NULL);;
                }
			}
			else if(labelPlayerNames[i].mouseClick(x, y) && ( activeInputLabel != &labelPlayerNames[i] )){
				ControlType ct= static_cast<ControlType>(listBoxControls[i].getSelectedItemIndex());
				if(ct == ctHuman) {
					setActiveInputLabel(&labelPlayerNames[i]);
				}
			}
		}
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void MenuStateCustomGame::mouseMove(int x, int y, const MouseState *ms){
	if (mainMessageBox.getEnabled()) {
		mainMessageBox.mouseMove(x, y);
	}
	buttonReturn.mouseMove(x, y);
	buttonPlayNow.mouseMove(x, y);
	buttonRestoreLastSettings.mouseMove(x, y);

	bool editingPlayerName = false;
	for(int i=0; i<GameConstants::maxPlayers; ++i){
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
	listBoxFogOfWar.mouseMove(x, y);
	listBoxTileset.mouseMove(x, y);
	listBoxMapFilter.mouseMove(x, y);
	listBoxTechTree.mouseMove(x, y);
	listBoxPublishServer.mouseMove(x, y);
	listBoxPublishServerExternalPort.mouseMove(x, y);

	listBoxEnableObserverMode.mouseMove(x, y);
	listBoxEnableServerControlledAI.mouseMove(x, y);
	labelNetworkFramePeriod.mouseMove(x, y);
	listBoxNetworkFramePeriod.mouseMove(x, y);

	labelNetworkPauseGameForLaggedClients.mouseMove(x, y);
	listBoxNetworkPauseGameForLaggedClients.mouseMove(x, y);

	labelPathFinderType.mouseMove(x, y);
	listBoxPathFinderType.mouseMove(x, y);
	listBoxAdvanced.mouseMove(x, y);
}

void MenuStateCustomGame::render(){
	try {
		Renderer &renderer= Renderer::getInstance();

		if(mainMessageBox.getEnabled()){
			renderer.renderMessageBox(&mainMessageBox);
		}
		else
		{
			int i;
			renderer.renderButton(&buttonReturn);
			renderer.renderButton(&buttonPlayNow);
			renderer.renderButton(&buttonRestoreLastSettings);
	
			for(i=0; i<GameConstants::maxPlayers; ++i){
				renderer.renderLabel(&labelPlayers[i]);
				renderer.renderLabel(&labelPlayerNames[i]);

				renderer.renderListBox(&listBoxControls[i]);
				if(listBoxControls[i].getSelectedItemIndex()!=ctClosed){
					renderer.renderListBox(&listBoxFactions[i]);
					renderer.renderListBox(&listBoxTeams[i]);
					renderer.renderLabel(&labelNetStatus[i]);
				}
			}
			renderer.renderLabel(&labelLocalIP);
			renderer.renderLabel(&labelMap);
			renderer.renderLabel(&labelFogOfWar);
			renderer.renderLabel(&labelTileset);
			renderer.renderLabel(&labelMapFilter);
			renderer.renderLabel(&labelTechTree);
			renderer.renderLabel(&labelControl);
			renderer.renderLabel(&labelFaction);
			renderer.renderLabel(&labelTeam);
			renderer.renderLabel(&labelMapInfo);
			renderer.renderLabel(&labelEnableObserverMode);
			renderer.renderLabel(&labelPathFinderType);
			renderer.renderLabel(&labelAdvanced);
			
			renderer.renderListBox(&listBoxMap);
			renderer.renderListBox(&listBoxFogOfWar);
			renderer.renderListBox(&listBoxTileset);
			renderer.renderListBox(&listBoxMapFilter);
			renderer.renderListBox(&listBoxTechTree);
			renderer.renderListBox(&listBoxEnableObserverMode);
			renderer.renderListBox(&listBoxPathFinderType);
			renderer.renderListBox(&listBoxAdvanced);
			
			renderer.renderChatManager(&chatManager);
			renderer.renderConsole(&console,showFullConsole,true);
			if(listBoxPublishServer.getEditable())
			{
				renderer.renderListBox(&listBoxPublishServer);
				renderer.renderLabel(&labelPublishServer);
				renderer.renderListBox(&listBoxPublishServerExternalPort);
				renderer.renderLabel(&labelPublishServerExternalPort);
				renderer.renderListBox(&listBoxEnableServerControlledAI);
				renderer.renderLabel(&labelEnableServerControlledAI);
				renderer.renderLabel(&labelNetworkFramePeriod);
				renderer.renderListBox(&listBoxNetworkFramePeriod);
				renderer.renderLabel(&labelNetworkPauseGameForLaggedClients);
				renderer.renderListBox(&listBoxNetworkPauseGameForLaggedClients);
			}
		}

		if(program != NULL) program->renderProgramMsgBox();
	}
	catch(const std::exception &ex) {
		char szBuf[1024]="";
		sprintf(szBuf,"In [%s::%s %d] error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		throw runtime_error(szBuf);
	}
}

void MenuStateCustomGame::update() {
	MutexSafeWrapper safeMutex(&masterServerThreadAccessor);

	try {
		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

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

		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		if(masterServerErr)
		{
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

			if(EndsWith(masterServererErrorToShow, "wrong router setup") == true)
			{
				masterServererErrorToShow=lang.get("WrongRouterSetup");
			}
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
		
		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		// handle setting changes from clients
		SwitchSetupRequest ** switchSetupRequests = serverInterface->getSwitchSetupRequests();
		for(int i= 0; i< mapInfo.players; ++i) {
			if(switchSetupRequests[i] != NULL) {
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] switchSetupRequests[i]->getSwitchFlags() = %d\n",__FILE__,__FUNCTION__,__LINE__,switchSetupRequests[i]->getSwitchFlags());

				if(listBoxControls[i].getSelectedItemIndex() == ctNetwork) {
					SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
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
								if(switchSetupRequests[i]->getSelectedFactionName() != ""){
									listBoxFactions[newFactionIdx].setSelectedItem(switchSetupRequests[i]->getSelectedFactionName());
								}
								if(switchSetupRequests[i]->getToTeam() != -1) {
									listBoxTeams[newFactionIdx].setSelectedItemIndex(switchSetupRequests[i]->getToTeam());
								}
								if(switchSetupRequests[i]->getNetworkPlayerName() != "") {
									SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] i = %d, labelPlayerNames[newFactionIdx].getText() [%s] switchSetupRequests[i]->getNetworkPlayerName() [%s]\n",__FILE__,__FUNCTION__,__LINE__,i,labelPlayerNames[newFactionIdx].getText().c_str(),switchSetupRequests[i]->getNetworkPlayerName().c_str());
									labelPlayerNames[newFactionIdx].setText(switchSetupRequests[i]->getNetworkPlayerName());
								}
							}
							catch(const runtime_error &e) {
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
								//switchSetupRequests[i]->clearSwitchFlag(ssrft_NetworkPlayerName);
							}
						}
						catch(const runtime_error &e) {
							SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] caught exception error = [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
						}
					}
				}

				delete switchSetupRequests[i];
				switchSetupRequests[i]=NULL;
			}
		}
		
		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] mapInfo.players = %d\n",__FILE__,__FUNCTION__,__LINE__,mapInfo.players);

		for(int i= 0; i<mapInfo.players; ++i) {
			//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

			if(listBoxControls[i].getSelectedItemIndex() == ctNetwork) {
				//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

				ConnectionSlot* connectionSlot= serverInterface->getSlot(i);
				assert(connectionSlot!=NULL);
				
				//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

				hasOneNetworkSlotOpen=true;

				//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] A - ctNetwork\n",__FILE__,__FUNCTION__);

				if(connectionSlot != NULL && connectionSlot->isConnected()) {
					connectionSlot->setName(labelPlayerNames[i].getText());

					//printf("FYI we have at least 1 client connected, slot = %d'\n",i);

					haveAtLeastOneNetworkClientConnected = true;
					if(connectionSlot->getConnectHasHandshaked())
						currentConnectionCount++;
					//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] B - ctNetwork\n",__FILE__,__FUNCTION__);

					string label = connectionSlot->getName() + ", " + connectionSlot->getVersionString();
					if(connectionSlot != NULL &&
					   connectionSlot->getAllowDownloadDataSynch() == true &&
					   connectionSlot->getAllowGameDataSynchCheck() == true)
					{
						if(connectionSlot->getNetworkGameDataSynchCheckOk() == false)
						{
							label += " -waiting to synch:";
							if(connectionSlot->getNetworkGameDataSynchCheckOkMap() == false)
							{
								label = label + " map";
							}
							if(connectionSlot->getNetworkGameDataSynchCheckOkTile() == false)
							{
								label = label + " tile";
							}
							if(connectionSlot->getNetworkGameDataSynchCheckOkTech() == false)
							{
								label = label + " techtree";
							}
						}
						else
						{
							label += " - data synch is ok";
						}
					}
					else
					{
						//label = connectionSlot->getName();

						connectionSlot= serverInterface->getSlot(i);
						if(connectionSlot != NULL &&
						   connectionSlot->getAllowGameDataSynchCheck() == true &&
						   connectionSlot->getNetworkGameDataSynchCheckOk() == false)
						{
							label += " -synch mismatch:";
							connectionSlot= serverInterface->getSlot(i);
							if(connectionSlot != NULL && connectionSlot->getNetworkGameDataSynchCheckOkMap() == false) {
								label = label + " map";

								if(connectionSlot->getReceivedDataSynchCheck() == true &&
									lastMapDataSynchError != "map CRC mismatch, " + listBoxMap.getSelectedItem()) {
									lastMapDataSynchError = "map CRC mismatch, " + listBoxMap.getSelectedItem();
									ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();
									serverInterface->sendTextMessage(lastMapDataSynchError,-1, true);
								}
							}

							connectionSlot= serverInterface->getSlot(i);
							if(connectionSlot != NULL && connectionSlot->getNetworkGameDataSynchCheckOkTile() == false) {
								label = label + " tile";

								if(connectionSlot->getReceivedDataSynchCheck() == true &&
									lastTileDataSynchError != "tile CRC mismatch, " + listBoxTileset.getSelectedItem()) {
									lastTileDataSynchError = "tile CRC mismatch, " + listBoxTileset.getSelectedItem();
									ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();
									serverInterface->sendTextMessage(lastTileDataSynchError,-1,true);
								}
							}

							connectionSlot= serverInterface->getSlot(i);
							if(connectionSlot != NULL && connectionSlot->getNetworkGameDataSynchCheckOkTech() == false)
							{
								label = label + " techtree";

								if(connectionSlot->getReceivedDataSynchCheck() == true) {
									ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();
									string report = connectionSlot->getNetworkGameDataSynchCheckTechMismatchReport();

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

							connectionSlot= serverInterface->getSlot(i);
							if(connectionSlot != NULL) {
								connectionSlot->setReceivedDataSynchCheck(false);
							}
						}
					}

					//float pingTime = connectionSlot->getThreadedPingMS(connectionSlot->getIpAddress().c_str());
					char szBuf[1024]="";
					//sprintf(szBuf,"%s, ping = %.2fms",label.c_str(),pingTime);
					sprintf(szBuf,"%s",label.c_str());

					labelNetStatus[i].setText(szBuf);
				}
				else
				{
					//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] C - ctNetwork\n",__FILE__,__FUNCTION__);
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

		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		bool checkDataSynch = (serverInterface->getAllowGameDataSynchCheck() == true &&
					//haveAtLeastOneNetworkClientConnected == true &&
					needToSetChangedGameSettings == true &&
					difftime(time(NULL),lastSetChangedGameSettings) >= 2);

		// Send the game settings to each client if we have at least one networked client
		if(checkDataSynch == true)
		{
			GameSettings gameSettings;
			loadGameSettings(&gameSettings);
			serverInterface->setGameSettings(&gameSettings,false);
			needToSetChangedGameSettings    = false;
		}
		
		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		if(hasOneNetworkSlotOpen) {
			//listBoxPublishServer.setSelectedItemIndex(0);
			listBoxPublishServer.setEditable(true);
			listBoxPublishServerExternalPort.setEditable(true);
			listBoxEnableServerControlledAI.setEditable(true);
		}
		else
		{
			listBoxPublishServer.setSelectedItemIndex(1);
			listBoxPublishServer.setEditable(false);
			listBoxPublishServerExternalPort.setEditable(false);
			listBoxEnableServerControlledAI.setEditable(false);
		}
		
		bool republishToMaster = (difftime(time(NULL),lastMasterserverPublishing) >= 5);

		if(republishToMaster == true) {
			needToRepublishToMasterserver = true;
			lastMasterserverPublishing = time(NULL);
		}

		bool callPublishNow = (listBoxPublishServer.getEditable() &&
							   listBoxPublishServer.getSelectedItemIndex() == 0 &&
							   needToRepublishToMasterserver == true);

		if(callPublishNow == true) {
			// give it to me baby, aha aha ...
			publishToMasterserver();
		}

		bool broadCastSettings = (difftime(time(NULL),lastSetChangedGameSettings) >= 2);
		
		if(broadCastSettings == true) {
			needToBroadcastServerSettings=true;
		}

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

		if(currentConnectionCount > soundConnectionCount){
			soundConnectionCount = currentConnectionCount;
			SoundRenderer::getInstance().playFx(CoreData::getInstance().getAttentionSound());
		}
		soundConnectionCount = currentConnectionCount;

		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}
	catch(const std::exception &ex) {
		char szBuf[1024]="";
		sprintf(szBuf,"In [%s::%s %d] error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
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
			ConnectionSlot* connectionSlot= serverInterface->getSlot(i);
			if((connectionSlot!=NULL) && (connectionSlot->isConnected()))
			{
				slotCountConnectedPlayers++;
			}	
		}
		else if(listBoxControls[i].getSelectedItemIndex() == ctHuman)
		{
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
	//string externalport = intToStr(Config::getInstance().getInt("ExternalServerPort",intToStr(Config::getInstance().getInt("ServerPort")).c_str()));
	string externalport = listBoxPublishServerExternalPort.getSelectedItem();
	publishToServerInfo["externalconnectport"] = externalport;

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void MenuStateCustomGame::simpleTask() {

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

	MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
	bool republish = (needToRepublishToMasterserver == true  && publishToServerInfo.size() != 0);
	needToRepublishToMasterserver = false;
	std::map<string,string> newPublishToServerInfo = publishToServerInfo;
	publishToServerInfo.clear();

	bool broadCastSettings = needToBroadcastServerSettings;
	needToBroadcastServerSettings=false;

	bool hasClientConnection = false;
	if(broadCastSettings) {
		ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();
		hasClientConnection = serverInterface->hasClientConnection();
	}

	bool needPing = (difftime(time(NULL),lastNetworkPing) >= GameConstants::networkPingInterval);
	safeMutex.ReleaseLock();

	if(republish == true) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		//string request = Config::getInstance().getString("Masterserver") + "addServerInfo.php?" + newPublishToServerInfo;
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

		std::string serverInfo = SystemFlags::getHTTP(request,handle);
		SystemFlags::cleanupHTTP(&handle);
		
		//printf("the result is:\n'%s'\n",serverInfo.c_str());
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] the result is:\n'%s'\n",__FILE__,__FUNCTION__,__LINE__,serverInfo.c_str());

		// uncomment to enable router setup check of this server
		if(EndsWith(serverInfo, "OK") == false) {
			showMasterserverError=true;
			masterServererErrorToShow = (serverInfo != "" ? serverInfo : "No Reply");
		}
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(broadCastSettings) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		MutexSafeWrapper safeMutex2(&masterServerThreadAccessor);
		GameSettings gameSettings;
		loadGameSettings(&gameSettings);

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
		serverInterface->setGameSettings(&gameSettings,false);
		safeMutex2.ReleaseLock();

		if(hasClientConnection == true) {
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

			MutexSafeWrapper safeMutex3(&masterServerThreadAccessor);

			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

			serverInterface->broadcastGameSetup(&gameSettings);
			safeMutex3.ReleaseLock();
		}
	}

	if(needPing == true) {
		lastNetworkPing = time(NULL);

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] Sending nmtPing to clients\n",__FILE__,__FUNCTION__,__LINE__);
		ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();
		NetworkMessagePing msg(GameConstants::networkPingInterval,time(NULL));

		MutexSafeWrapper safeMutex2(&masterServerThreadAccessor);
		serverInterface->broadcastPing(&msg);
		safeMutex2.ReleaseLock();
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void MenuStateCustomGame::loadGameSettings(GameSettings *gameSettings) {
    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

	int factionCount= 0;
	ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

	gameSettings->setDescription(formatString(mapFiles[listBoxMap.getSelectedItemIndex()]));
	gameSettings->setMap(mapFiles[listBoxMap.getSelectedItemIndex()]);
    gameSettings->setTileset(tilesetFiles[listBoxTileset.getSelectedItemIndex()]);
    gameSettings->setTech(techTreeFiles[listBoxTechTree.getSelectedItemIndex()]);
	gameSettings->setDefaultUnits(true);
	gameSettings->setDefaultResources(true);
	gameSettings->setDefaultVictoryConditions(true);
	gameSettings->setFogOfWar(listBoxFogOfWar.getSelectedItemIndex() == 0);
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
				gameSettings->setNetworkPlayerName(slotIndex, getHumanPlayerName(i));
				labelPlayerNames[i].setText(getHumanPlayerName(i));
			}
			gameSettings->setTeam(slotIndex, listBoxTeams[i].getSelectedItemIndex());
			gameSettings->setStartLocationIndex(slotIndex, i);
			gameSettings->setFactionTypeName(slotIndex, factionFiles[listBoxFactions[i].getSelectedItemIndex()]);

			if(listBoxControls[i].getSelectedItemIndex() == ctNetwork) {
				ConnectionSlot* connectionSlot= serverInterface->getSlot(i);
				if(connectionSlot != NULL && connectionSlot->isConnected()) {
					SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] i = %d, connectionSlot->getName() [%s]\n",__FILE__,__FUNCTION__,__LINE__,i,connectionSlot->getName().c_str());

					gameSettings->setNetworkPlayerName(slotIndex, connectionSlot->getName());
					labelPlayerNames[i].setText(connectionSlot->getName());
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
			gameSettings->setFactionTypeName(slotIndex, factionFiles[listBoxFactions[i].getSelectedItemIndex()]);
			gameSettings->setNetworkPlayerName(slotIndex, "Closed");

			closedCount++;
		}
    }

	gameSettings->setFactionCount(factionCount);
	gameSettings->setEnableServerControlledAI(listBoxEnableServerControlledAI.getSelectedItemIndex() == 0);
	gameSettings->setNetworkFramePeriod((listBoxNetworkFramePeriod.getSelectedItemIndex()+1)*10);
	gameSettings->setNetworkPauseGameForLaggedClients((listBoxNetworkPauseGameForLaggedClients.getSelectedItemIndex()));
	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] gameSettings->getTileset() = [%s]\n",__FILE__,__FUNCTION__,gameSettings->getTileset().c_str());
	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] gameSettings->getTech() = [%s]\n",__FILE__,__FUNCTION__,gameSettings->getTech().c_str());
	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] gameSettings->getMap() = [%s]\n",__FILE__,__FUNCTION__,gameSettings->getMap().c_str());

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
}

void MenuStateCustomGame::saveGameSettingsToFile(std::string fileName) {
    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

    if(getGameReadWritePath() != "") {
    	fileName = getGameReadWritePath() + fileName;
    }

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] fileName = [%s]\n",__FILE__,__FUNCTION__,__LINE__,fileName.c_str());

	GameSettings gameSettings;
	loadGameSettings(&gameSettings);

    std::ofstream saveGameFile;
    saveGameFile.open(fileName.c_str(), ios_base::out | ios_base::trunc);

	//int factionCount= 0;
	//ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();

	saveGameFile << "Description=" << gameSettings.getDescription() << std::endl;

	saveGameFile << "Map=" << gameSettings.getMap() << std::endl;
	saveGameFile << "Tileset=" << gameSettings.getTileset() << std::endl;
	saveGameFile << "TechTree=" << gameSettings.getTech() << std::endl;
	saveGameFile << "DefaultUnits=" << gameSettings.getDefaultUnits() << std::endl;
	saveGameFile << "DefaultResources=" << gameSettings.getDefaultResources() << std::endl;
	saveGameFile << "DefaultVictoryConditions=" << gameSettings.getDefaultVictoryConditions() << std::endl;
	saveGameFile << "FogOfWar=" << gameSettings.getFogOfWar() << std::endl;
	saveGameFile << "EnableObserverModeAtEndGame=" << gameSettings.getEnableObserverModeAtEndGame() << std::endl;
	saveGameFile << "PathFinderType=" << gameSettings.getPathFinderType() << std::endl;
	saveGameFile << "EnableServerControlledAI=" << gameSettings.getEnableServerControlledAI() << std::endl;
	saveGameFile << "NetworkFramePeriod=" << gameSettings.getNetworkFramePeriod() << std::endl;
	saveGameFile << "NetworkPauseGameForLaggedClients=" << gameSettings.getNetworkPauseGameForLaggedClients() << std::endl;
	saveGameFile << "ExternalPortNumber=" << listBoxPublishServerExternalPort.getSelectedItem() << std::endl;

	saveGameFile << "FactionThisFactionIndex=" << gameSettings.getThisFactionIndex() << std::endl;
	saveGameFile << "FactionCount=" << gameSettings.getFactionCount() << std::endl;

    //for(int i = 0; i < gameSettings.getFactionCount(); ++i) {
	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		int slotIndex = gameSettings.getStartLocationIndex(i);

		saveGameFile << "FactionControlForIndex" 		<< slotIndex << "=" << gameSettings.getFactionControl(i) << std::endl;
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

    if(getGameReadWritePath() != "") {
    	fileName = getGameReadWritePath() + fileName;
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

		gameSettings.setDescription(properties.getString("Description"));
		gameSettings.setMap(properties.getString("Map"));
		gameSettings.setTileset(properties.getString("Tileset"));
		gameSettings.setTech(properties.getString("TechTree"));
		gameSettings.setDefaultUnits(properties.getBool("DefaultUnits"));
		gameSettings.setDefaultResources(properties.getBool("DefaultResources"));
		gameSettings.setDefaultVictoryConditions(properties.getBool("DefaultVictoryConditions"));
		gameSettings.setFogOfWar(properties.getBool("FogOfWar"));
		gameSettings.setEnableObserverModeAtEndGame(properties.getBool("EnableObserverModeAtEndGame"));
		gameSettings.setPathFinderType(static_cast<PathFinderType>(properties.getInt("PathFinderType",intToStr(pfBasic).c_str())));
		gameSettings.setEnableServerControlledAI(properties.getBool("EnableServerControlledAI","false"));
		gameSettings.setNetworkFramePeriod(properties.getInt("NetworkFramePeriod",intToStr(GameConstants::networkFramePeriod).c_str())/10*10);
		gameSettings.setNetworkPauseGameForLaggedClients(properties.getBool("NetworkPauseGameForLaggedClients","false"));

		gameSettings.setThisFactionIndex(properties.getInt("FactionThisFactionIndex"));
		gameSettings.setFactionCount(properties.getInt("FactionCount"));

		//for(int i = 0; i < gameSettings.getFactionCount(); ++i) {
		for(int i = 0; i < GameConstants::maxPlayers; ++i) {
			gameSettings.setFactionControl(i,(ControlType)properties.getInt(string("FactionControlForIndex") + intToStr(i),intToStr(ctClosed).c_str()) );
			gameSettings.setTeam(i,properties.getInt(string("FactionTeamForIndex") + intToStr(i),"0") );
			gameSettings.setStartLocationIndex(i,properties.getInt(string("FactionStartLocationForIndex") + intToStr(i),intToStr(i).c_str()) );
			gameSettings.setFactionTypeName(i,properties.getString(string("FactionTypeNameForIndex") + intToStr(i),"?") );
			gameSettings.setNetworkPlayerName(i,properties.getString(string("FactionPlayerNameForIndex") + intToStr(i),"") );
		}

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

		string mapFile = gameSettings.getMap();
		mapFile = formatString(mapFile);
		listBoxMap.setSelectedItem(mapFile);

		loadMapInfo(Map::getMapPath(mapFiles[listBoxMap.getSelectedItemIndex()]), &mapInfo);
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
		listBoxFogOfWar.setSelectedItem(gameSettings.getFogOfWar() == true ? lang.get("Yes") : lang.get("No"));
		listBoxEnableObserverMode.setSelectedItem(gameSettings.getEnableObserverModeAtEndGame() == true ? lang.get("Yes") : lang.get("No"));
		listBoxPathFinderType.setSelectedItemIndex(gameSettings.getPathFinderType());

		listBoxEnableServerControlledAI.setSelectedItem(gameSettings.getEnableServerControlledAI() == true ? lang.get("Yes") : lang.get("No"));

		labelNetworkFramePeriod.setText(lang.get("NetworkFramePeriod"));

		listBoxPublishServerExternalPort.setSelectedItem(intToStr(properties.getInt("ExternalPortNumber",listBoxPublishServerExternalPort.getSelectedItem().c_str())));

		listBoxNetworkFramePeriod.setSelectedItem(intToStr(gameSettings.getNetworkFramePeriod()/10*10));

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

		listBoxNetworkPauseGameForLaggedClients.setSelectedItemIndex(gameSettings.getNetworkPauseGameForLaggedClients());

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

		reloadFactions();

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d] gameSettings.getFactionCount() = %d\n",__FILE__,__FUNCTION__,__LINE__,gameSettings.getFactionCount());

		//for(int i = 0; i < gameSettings.getFactionCount(); ++i) {
		for(int i = 0; i < GameConstants::maxPlayers; ++i) {
			listBoxControls[i].setSelectedItemIndex(gameSettings.getFactionControl(i));
			listBoxTeams[i].setSelectedItemIndex(gameSettings.getTeam(i));

			string factionName = gameSettings.getFactionTypeName(i);
			factionName = formatString(factionName);

			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] factionName = [%s]\n",__FILE__,__FUNCTION__,__LINE__,factionName.c_str());

			listBoxFactions[i].setSelectedItem(factionName);

			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] i = %d, gameSettings.getNetworkPlayerName(i) [%s]\n",__FILE__,__FUNCTION__,__LINE__,i,gameSettings.getNetworkPlayerName(i).c_str());

			labelPlayerNames[i].setText(gameSettings.getNetworkPlayerName(i));
		}

		updateControlers();
		updateNetworkSlots();
		needToRepublishToMasterserver = true;

        if(hasNetworkGameSettings() == true)
        {
            needToSetChangedGameSettings = true;
            lastSetChangedGameSettings   = time(NULL);
        }
    }
    catch(const exception &ex) {
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
		char szBuf[1024]="";
		sprintf(szBuf,"In [%s::%s %d] error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		//throw runtime_error(szBuf);
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);
		showGeneralError=true;
		generalErrorToShow = ex.what();
	}

    return hasNetworkSlot;
}

void MenuStateCustomGame::loadMapInfo(string file, MapInfo *mapInfo){

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
	}
	catch(exception e){
		throw runtime_error("Error loading map file: "+file+'\n'+e.what());
	}

}

void MenuStateCustomGame::reloadFactions(){

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
        throw runtime_error("(2)There are no factions for the tech tree [" + techTreeFiles[listBoxTechTree.getSelectedItemIndex()] + "]");
    }
    factionFiles= results;
    for(int i= 0; i<results.size(); ++i){
        results[i]= formatString(results[i]);

        SystemFlags::OutputDebug(SystemFlags::debugSystem,"Tech [%s] has faction [%s]\n",techTreeFiles[listBoxTechTree.getSelectedItemIndex()].c_str(),results[i].c_str());
    }
    for(int i=0; i<GameConstants::maxPlayers; ++i){
        listBoxFactions[i].setItems(results);
        listBoxFactions[i].setSelectedItemIndex(i % results.size());
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
		char szBuf[1024]="";
		sprintf(szBuf,"In [%s::%s %d] error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		throw runtime_error(szBuf);
	}
}

void MenuStateCustomGame::closeUnusedSlots(){
	try {
		ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();
		for(int i= 0; i<mapInfo.players; ++i){
			if(listBoxControls[i].getSelectedItemIndex()==ctNetwork){
				if(!serverInterface->getSlot(i)->isConnected()){
					listBoxControls[i].setSelectedItemIndex(ctClosed);
				}
			}
		}
		updateNetworkSlots();
	}
	catch(const std::exception &ex) {
		char szBuf[1024]="";
		sprintf(szBuf,"In [%s::%s %d] error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		throw runtime_error(szBuf);
	}
}

void MenuStateCustomGame::updateNetworkSlots() {
	try {
		ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();

		for(int i= 0; i<GameConstants::maxPlayers; ++i) {
			if(serverInterface->getSlot(i) == NULL &&
				listBoxControls[i].getSelectedItemIndex() == ctNetwork)	{
				try {
					serverInterface->addSlot(i);
				}
				catch(const std::exception &ex) {
					char szBuf[1024]="";
					sprintf(szBuf,"In [%s::%s %d] error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
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
		char szBuf[1024]="";
		sprintf(szBuf,"In [%s::%s %d] error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);
		//throw runtime_error(szBuf);!!!
		showGeneralError=true;
		generalErrorToShow = ex.what();

	}
}

void MenuStateCustomGame::keyDown(char key) {
	if(activeInputLabel!=NULL) {
		if(key==vkBack) {
			string text= activeInputLabel->getText();
			if(text.size()>1){
				text.erase(text.end()-2);
			}
			activeInputLabel->setText(text);

			MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
	        if(hasNetworkGameSettings() == true) {
	            needToSetChangedGameSettings = true;
	            lastSetChangedGameSettings   = time(NULL);
	        }
		}
	}
	else {
		//send key to the chat manager
		chatManager.keyDown(key);
		if(!chatManager.getEditEnabled()) {
			if(key=='M') {
				showFullConsole= true;
			}
		}
	}
}

void MenuStateCustomGame::keyPress(char c) {
	if(activeInputLabel!=NULL) {
		int maxTextSize= 16;
	    for(int i=0; i<GameConstants::maxPlayers; ++i) {
			if(&labelPlayerNames[i] == activeInputLabel) {
				if((c>='0' && c<='9')||(c>='a' && c<='z')||(c>='A' && c<='Z')||
					(c=='-')||(c=='(')||(c==')')){
					if(activeInputLabel->getText().size()<maxTextSize) {
						string text= activeInputLabel->getText();
						text.insert(text.end()-1, c);
						activeInputLabel->setText(text);

						MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
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
		chatManager.keyPress(c);
	}
}

void MenuStateCustomGame::keyUp(char key) {
	if(activeInputLabel==NULL) {
		chatManager.keyUp(key);
		if(chatManager.getEditEnabled()){
			//send key to the chat manager
			chatManager.keyUp(key);
		}
		else if(key== 'M'){
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

void MenuStateCustomGame::setActiveInputLabel(GraphicLabel *newLable) {
	if(newLable!=NULL) {
		string text= newLable->getText();
		size_t found;
		found=text.find_last_of("_");
		if (found==string::npos)
		{
			text=text+"_";
		}
		newLable->setText(text);
	}
	if(activeInputLabel!=NULL && !activeInputLabel->getText().empty()){
		string text= activeInputLabel->getText();
		size_t found;
		found=text.find_last_of("_");
		if (found!=string::npos)
		{
			text=text.substr(0,found);
		}
		activeInputLabel->setText(text);
	}
	activeInputLabel=newLable;
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

}}//end namespace
