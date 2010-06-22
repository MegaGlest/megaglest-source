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

	showGeneralError = false;
	generalErrorToShow = "---";

	publishToMasterserverThread = NULL;
	Lang &lang= Lang::getInstance();
	NetworkManager &networkManager= NetworkManager::getInstance();
    Config &config = Config::getInstance();
    
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
	soundConnectionCount=0;

	vector<string> teamItems, controlItems, results;

	//create
	buttonReturn.init(250, 180, 125);
	buttonRestoreLastSettings.init(250+130, 180, 200);
	buttonPlayNow.init(250+130+205, 180, 125);

	int setupPos=610;
	int mapHeadPos=330;
	int mapPos=mapHeadPos-30;
	int aHeadPos=260;
	int aPos=aHeadPos-30;
	int networkHeadPos=700;
	int networkPos=networkHeadPos-30;
	
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

	labelMap.init(100, mapHeadPos);
	listBoxMap.init(100, mapPos, 200);
    listBoxMap.setItems(results);
	labelMapInfo.init(100, mapPos-30, 200, 40);
		
	// fog - o - war
	// @350 ? 300 ?
	labelFogOfWar.init(400, aHeadPos, 80);
	listBoxFogOfWar.init(400, aPos, 80);
	listBoxFogOfWar.pushBackItem(lang.get("Yes"));
	listBoxFogOfWar.pushBackItem(lang.get("No"));
	listBoxFogOfWar.setSelectedItemIndex(0);

	// Enable Observer Mode
	labelEnableObserverMode.init(600, aHeadPos, 80);
	listBoxEnableObserverMode.init(600, aPos, 80);
	listBoxEnableObserverMode.pushBackItem(lang.get("Yes"));
	listBoxEnableObserverMode.pushBackItem(lang.get("No"));
	listBoxEnableObserverMode.setSelectedItemIndex(0);

    //tileset listBox
    findDirs(config.getPathListForType(ptTilesets), results);
	if (results.empty()) {
        throw runtime_error("No tile-sets were found!");
	}
    tilesetFiles= results;
	std::for_each(results.begin(), results.end(), FormatString());
	listBoxTileset.init(400, mapPos, 150);
    listBoxTileset.setItems(results);
	labelTileset.init(400, mapHeadPos);

    //tech Tree listBox
    findDirs(config.getPathListForType(ptTechs), results);
	if(results.empty()) {
        throw runtime_error("No tech-trees were found!");
	}
    techTreeFiles= results;
	std::for_each(results.begin(), results.end(), FormatString());
	listBoxTechTree.init(600, mapPos, 150);
    listBoxTechTree.setItems(results);
	labelTechTree.init(600, mapHeadPos);


	labelPublishServer.init(190, networkHeadPos, 100);
	labelPublishServer.setText(lang.get("PublishServer"));
	listBoxPublishServer.init(200, networkPos, 100);
	listBoxPublishServer.pushBackItem(lang.get("Yes"));
	listBoxPublishServer.pushBackItem(lang.get("No"));
	if(openNetworkSlots)
		listBoxPublishServer.setSelectedItemIndex(0);
	else
		listBoxPublishServer.setSelectedItemIndex(1);

	// Network Frame Period
	labelNetworkFramePeriod.init(440, networkHeadPos, 80);
	labelNetworkFramePeriod.setText(lang.get("NetworkFramePeriod"));
	listBoxNetworkFramePeriod.init(450, networkPos, 80);
	listBoxNetworkFramePeriod.pushBackItem("10");
	listBoxNetworkFramePeriod.pushBackItem("20");
	listBoxNetworkFramePeriod.pushBackItem("30");
	listBoxNetworkFramePeriod.pushBackItem("40");
	listBoxNetworkFramePeriod.setSelectedItem("20");


	// Enable Server Controlled AI
	labelEnableServerControlledAI.init(690, networkHeadPos, 80);
	labelEnableServerControlledAI.setText(lang.get("EnableServerControlledAI"));
	listBoxEnableServerControlledAI.init(700, networkPos, 80);
	listBoxEnableServerControlledAI.pushBackItem(lang.get("Yes"));
	listBoxEnableServerControlledAI.pushBackItem(lang.get("No"));
	listBoxEnableServerControlledAI.setSelectedItemIndex(1);

	

	//list boxes
    for(int i=0; i<GameConstants::maxPlayers; ++i){
		labelPlayers[i].init(100, setupPos-30-i*30);
        listBoxControls[i].init(200, setupPos-30-i*30);
        listBoxFactions[i].init(400, setupPos-30-i*30, 150);
		listBoxTeams[i].init(600, setupPos-30-i*30, 60);
		labelNetStatus[i].init(700, setupPos-30-i*30, 60);
    }


	labelControl.init(200, setupPos, GraphicListBox::defW, GraphicListBox::defH, true);
    labelFaction.init(400, setupPos, GraphicListBox::defW, GraphicListBox::defH, true);
    labelTeam.init(600, setupPos, 50, GraphicListBox::defH, true);
    
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
        listBoxTeams[i].setItems(teamItems);
		listBoxTeams[i].setSelectedItemIndex(i);
		listBoxControls[i].setItems(controlItems);
		labelNetStatus[i].setText("");
    }

	labelMap.setText(lang.get("Map")+":");
	labelFogOfWar.setText(lang.get("FogOfWar"));
	labelTileset.setText(lang.get("Tileset"));
	labelTechTree.setText(lang.get("TechTree"));
	labelControl.setText(lang.get("Control"));
    labelFaction.setText(lang.get("Faction"));
    labelTeam.setText(lang.get("Team"));

    labelEnableObserverMode.setText(lang.get("EnableObserverMode"));
    

	loadMapInfo(Map::getMapPath(mapFiles[listBoxMap.getSelectedItemIndex()]), &mapInfo);

	labelMapInfo.setText(mapInfo.desc);

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//init controllers
	if(serverInitError == false) {
		listBoxControls[0].setSelectedItemIndex(ctHuman);
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

	MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
	needToBroadcastServerSettings = false;
	needToRepublishToMasterserver = false;
	safeMutex.ReleaseLock();

	//BaseThread::shutdownAndWait(publishToMasterserverThread);
	delete publishToMasterserverThread;
	publishToMasterserverThread = NULL;

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void MenuStateCustomGame::returnToParentMenu(){
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

	MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
	needToBroadcastServerSettings = false;
	needToRepublishToMasterserver = false;
	bool returnToMasterServerMenu = parentMenuIsMs;
	safeMutex.ReleaseLock();

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
			//BaseThread::shutdownAndWait(publishToMasterserverThread);
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
	else if(listBoxPublishServer.mouseClick(x, y)&&listBoxPublishServer.getEditable()){
		MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
		needToRepublishToMasterserver = true;
		soundRenderer.playFx(coreData.getClickSoundC());
	}
	else if(listBoxNetworkFramePeriod.mouseClick(x, y)){
		MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
		soundRenderer.playFx(coreData.getClickSoundC());
	}
	else {
		for(int i=0; i<mapInfo.players; ++i) {
			MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
			//ensure thet only 1 human player is present
			if(listBoxControls[i].mouseClick(x, y))
			{
				//look for human players
				int humanIndex1= -1;
				int humanIndex2= -1;
				for(int j=0; j<GameConstants::maxPlayers; ++j){
					ControlType ct= static_cast<ControlType>(listBoxControls[j].getSelectedItemIndex());
					if(ct==ctHuman){
						if(humanIndex1==-1){
							humanIndex1= j;
						}
						else{
							humanIndex2= j;
						}
					}
				}

				//no human
				if(humanIndex1==-1 && humanIndex2==-1){
					listBoxControls[i].setSelectedItemIndex(ctHuman);
				}

				//2 humans
				if(humanIndex1!=-1 && humanIndex2!=-1){
					listBoxControls[humanIndex1==i? humanIndex2: humanIndex1].setSelectedItemIndex(ctClosed);
				}
				updateNetworkSlots();

				needToRepublishToMasterserver = true;
                if(hasNetworkGameSettings() == true)
                {
                    needToSetChangedGameSettings = true;
                    lastSetChangedGameSettings   = time(NULL);;
                }
			}
			else if(listBoxFactions[i].mouseClick(x, y)){
				needToRepublishToMasterserver = true;

                if(hasNetworkGameSettings() == true)
                {
                    needToSetChangedGameSettings = true;
                    lastSetChangedGameSettings   = time(NULL);;
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
		}
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void MenuStateCustomGame::mouseMove(int x, int y, const MouseState *ms){
	MutexSafeWrapper safeMutex(&masterServerThreadAccessor);

	if (mainMessageBox.getEnabled()) {
		mainMessageBox.mouseMove(x, y);
	}
	buttonReturn.mouseMove(x, y);
	buttonPlayNow.mouseMove(x, y);
	buttonRestoreLastSettings.mouseMove(x, y);

	for(int i=0; i<GameConstants::maxPlayers; ++i){
        listBoxControls[i].mouseMove(x, y);
        listBoxFactions[i].mouseMove(x, y);
		listBoxTeams[i].mouseMove(x, y);
    }
	listBoxMap.mouseMove(x, y);
	listBoxFogOfWar.mouseMove(x, y);
	listBoxTileset.mouseMove(x, y);
	listBoxTechTree.mouseMove(x, y);
	listBoxPublishServer.mouseMove(x, y);
	listBoxEnableObserverMode.mouseMove(x, y);
	listBoxEnableServerControlledAI.mouseMove(x, y);
	labelNetworkFramePeriod.mouseMove(x, y);
	listBoxNetworkFramePeriod.mouseMove(x, y);
}

void MenuStateCustomGame::render(){

	try {
		Renderer &renderer= Renderer::getInstance();

		MutexSafeWrapper safeMutex(&masterServerThreadAccessor);

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
				renderer.renderListBox(&listBoxControls[i]);
				if(listBoxControls[i].getSelectedItemIndex()!=ctClosed){
					renderer.renderListBox(&listBoxFactions[i]);
					renderer.renderListBox(&listBoxTeams[i]);
					renderer.renderLabel(&labelNetStatus[i]);
				}
			}
			renderer.renderLabel(&labelMap);
			renderer.renderLabel(&labelFogOfWar);
			renderer.renderLabel(&labelTileset);
			renderer.renderLabel(&labelTechTree);
			renderer.renderLabel(&labelControl);
			renderer.renderLabel(&labelFaction);
			renderer.renderLabel(&labelTeam);
			renderer.renderLabel(&labelMapInfo);
			renderer.renderLabel(&labelEnableObserverMode);
			
	
			renderer.renderListBox(&listBoxMap);
			renderer.renderListBox(&listBoxFogOfWar);
			renderer.renderListBox(&listBoxTileset);
			renderer.renderListBox(&listBoxTechTree);
			renderer.renderListBox(&listBoxEnableObserverMode);
	
			renderer.renderChatManager(&chatManager);
			renderer.renderConsole(&console,showFullConsole,true);
			if(listBoxPublishServer.getEditable())
			{
				renderer.renderListBox(&listBoxPublishServer);
				renderer.renderLabel(&labelPublishServer);
				renderer.renderListBox(&listBoxEnableServerControlledAI);
				renderer.renderLabel(&labelEnableServerControlledAI);
				renderer.renderLabel(&labelNetworkFramePeriod);
				renderer.renderListBox(&listBoxNetworkFramePeriod);
			}
		}
	}
	catch(const std::exception &ex) {
		char szBuf[1024]="";
		sprintf(szBuf,"In [%s::%s %d] error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		throw runtime_error(szBuf);
	}
}

void MenuStateCustomGame::update()
{
	try {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		if(serverInitError == true) {
			if(showGeneralError) {
				MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
				showGeneralError=false;
				mainMessageBoxState=1;
				showMessageBox( generalErrorToShow, "Error", false);
				safeMutex.ReleaseLock();
			}

			return;
		}
		ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();
		Lang& lang= Lang::getInstance();

		bool haveAtLeastOneNetworkClientConnected = false;
		bool hasOneNetworkSlotOpen = false;
		int currentConnectionCount=0;
		Config &config = Config::getInstance();
		
		MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
		bool masterServerErr = showMasterserverError;
		safeMutex.ReleaseLock(true);
		if(masterServerErr)
		{
			safeMutex.Lock();
			if(EndsWith(masterServererErrorToShow, "wrong router setup") == true)
			{
				masterServererErrorToShow=lang.get("wrong router setup");
			}
			showMasterserverError=false;
			listBoxPublishServer.setSelectedItemIndex(1);
			mainMessageBoxState=1;
			showMessageBox( masterServererErrorToShow, lang.get("ErrorFromMasterserver"), false);
			safeMutex.ReleaseLock(true);
		}
		else if(showGeneralError) {
			safeMutex.Lock();
			showGeneralError=false;
			mainMessageBoxState=1;
			showMessageBox( generalErrorToShow, "Error", false);
			safeMutex.ReleaseLock(true);
		}
		
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		// handle setting changes from clients
		SwitchSetupRequest** switchSetupRequests=serverInterface->getSwitchSetupRequests();
		safeMutex.Lock();
		for(int i= 0; i<mapInfo.players; ++i)
		{
			if(switchSetupRequests[i]!=NULL)
			{
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

				if(listBoxControls[i].getSelectedItemIndex() == ctNetwork)
				{
					SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
					//printf("switchSetupRequests[i]->getSelectedFactionName()=%s\n",switchSetupRequests[i]->getSelectedFactionName().c_str());
					//printf("switchSetupRequests[i]->getToTeam()=%d\n",switchSetupRequests[i]->getToTeam());
					
					if(switchSetupRequests[i]->getToFactionIndex()!=-1)
					{
						//printf("switchSlot request from %d to %d\n",switchSetupRequests[i]->getCurrentFactionIndex(),switchSetupRequests[i]->getToFactionIndex());
						if(serverInterface->switchSlot(switchSetupRequests[i]->getCurrentFactionIndex(),switchSetupRequests[i]->getToFactionIndex())){
							int k=switchSetupRequests[i]->getToFactionIndex();
							try {
								if(switchSetupRequests[i]->getSelectedFactionName()!=""){
									listBoxFactions[k].setSelectedItem(switchSetupRequests[i]->getSelectedFactionName());
								}
								if(switchSetupRequests[i]->getToTeam()!=-1)
									listBoxTeams[k].setSelectedItemIndex(switchSetupRequests[i]->getToTeam());					
							}
							catch(const runtime_error &e) {
								SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] caught exception error = [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
							}
							
						}
					}
					else
					{
						try {
							if(switchSetupRequests[i]->getSelectedFactionName()!=""){
								listBoxFactions[i].setSelectedItem(switchSetupRequests[i]->getSelectedFactionName());
							}
							if(switchSetupRequests[i]->getToTeam()!=-1)
								listBoxTeams[i].setSelectedItemIndex(switchSetupRequests[i]->getToTeam());					
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
		safeMutex.ReleaseLock(true);
		
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] mapInfo.players = %d\n",__FILE__,__FUNCTION__,__LINE__,mapInfo.players);

		safeMutex.Lock();
		for(int i= 0; i<mapInfo.players; ++i)
		{
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

			if(listBoxControls[i].getSelectedItemIndex() == ctNetwork)
			{
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

				ConnectionSlot* connectionSlot= serverInterface->getSlot(i);
				assert(connectionSlot!=NULL);
				
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

				hasOneNetworkSlotOpen=true;

				//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] A - ctNetwork\n",__FILE__,__FUNCTION__);

				if(connectionSlot->isConnected())
				{
					//printf("FYI we have at least 1 client connected, slot = %d'\n",i);

					haveAtLeastOneNetworkClientConnected = true;
					if(connectionSlot->getConnectHasHandshaked())
						currentConnectionCount++;
					//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] B - ctNetwork\n",__FILE__,__FUNCTION__);

					string label = connectionSlot->getName();
					if(connectionSlot->getAllowDownloadDataSynch() == true &&
					   connectionSlot->getAllowGameDataSynchCheck() == true)
					{
						if(connectionSlot->getNetworkGameDataSynchCheckOk() == false)
						{
							label = connectionSlot->getName() + " - waiting to synch:";
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
							label = connectionSlot->getName() + " - data synch is ok";
						}
					}
					else
					{
						label = connectionSlot->getName();

						if(connectionSlot->getAllowGameDataSynchCheck() == true &&
						   connectionSlot->getNetworkGameDataSynchCheckOk() == false)
						{
							label += " - warning synch mismatch for:";
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
					}

					float pingTime = connectionSlot->getThreadedPingMS(connectionSlot->getIpAddress().c_str());
					char szBuf[1024]="";
					sprintf(szBuf,"%s, ping = %.2fms",label.c_str(),pingTime);

					labelNetStatus[i].setText(szBuf);
				}
				else
				{
					//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] C - ctNetwork\n",__FILE__,__FUNCTION__);
					string port=intToStr(config.getInt("ServerPort"));
					if(port!="61357"){
						port=port +lang.get(" NonStandardPort")+"!";
					}
					else
					{
						port=port+")";
					}
					port="("+port;
					labelNetStatus[i].setText("--- "+port);
				}

				//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] END - ctNetwork\n",__FILE__,__FUNCTION__);
			}
			else{
				labelNetStatus[i].setText("");
			}
		}
		safeMutex.ReleaseLock(true);

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		safeMutex.Lock();
		bool checkDataSynch = (serverInterface->getAllowGameDataSynchCheck() == true &&
					//haveAtLeastOneNetworkClientConnected == true &&
					needToSetChangedGameSettings == true &&
					difftime(time(NULL),lastSetChangedGameSettings) >= 2);
		safeMutex.ReleaseLock(true);

		// Send the game settings to each client if we have at least one networked client
		if(checkDataSynch == true)
		{
			GameSettings gameSettings;
			loadGameSettings(&gameSettings);
			serverInterface->setGameSettings(&gameSettings);

			safeMutex.Lock();
			needToSetChangedGameSettings    = false;
			safeMutex.ReleaseLock(true);
		}
		
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		if(hasOneNetworkSlotOpen)
		{
			safeMutex.Lock();
			//listBoxPublishServer.setSelectedItemIndex(0);
			listBoxPublishServer.setEditable(true);
			listBoxEnableServerControlledAI.setEditable(true);
			safeMutex.ReleaseLock(true);
		}
		else
		{
			safeMutex.Lock();
			listBoxPublishServer.setSelectedItemIndex(1);
			listBoxPublishServer.setEditable(false);
			listBoxEnableServerControlledAI.setEditable(false);
			safeMutex.ReleaseLock(true);
		}
		
		safeMutex.Lock();
		bool republishToMaster = (difftime(time(NULL),lastMasterserverPublishing) >= 5);
		safeMutex.ReleaseLock(true);

		if(republishToMaster == true) {
			safeMutex.Lock();
			needToRepublishToMasterserver = true;
			lastMasterserverPublishing = time(NULL);
			safeMutex.ReleaseLock(true);
		}

		safeMutex.Lock();
		bool callPublishNow = (listBoxPublishServer.getEditable() &&
							   listBoxPublishServer.getSelectedItemIndex() == 0 &&
							   needToRepublishToMasterserver == true);
		safeMutex.ReleaseLock(true);

		if(callPublishNow == true) {
			// give it to me baby, aha aha ...
			publishToMasterserver();
		}

		safeMutex.Lock();
		bool broadCastSettings = (difftime(time(NULL),lastSetChangedGameSettings) >= 2);
		safeMutex.ReleaseLock(true);
		
		if(broadCastSettings == true) {
			safeMutex.Lock();
			needToBroadcastServerSettings=true;
			safeMutex.ReleaseLock(true);
		}

		//call the chat manager
		chatManager.updateNetwork();

		//console
		console.update();

		safeMutex.Lock();
		broadCastSettings = (difftime(time(NULL),lastSetChangedGameSettings) >= 2);
		safeMutex.ReleaseLock(true);

		if(broadCastSettings == true)
		{// reset timer here on bottom becasue used for different things
			safeMutex.Lock();
			lastSetChangedGameSettings      = time(NULL);
			safeMutex.ReleaseLock(true);
		}
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		safeMutex.Lock();
		if(currentConnectionCount > soundConnectionCount){
			soundConnectionCount = currentConnectionCount;
			SoundRenderer::getInstance().playFx(CoreData::getInstance().getAttentionSound());
		}
		soundConnectionCount = currentConnectionCount;
		safeMutex.ReleaseLock(true);

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}
	catch(const std::exception &ex) {
		char szBuf[1024]="";
		sprintf(szBuf,"In [%s::%s %d] error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);
		//throw runtime_error(szBuf);
		showGeneralError=true;
		generalErrorToShow = szBuf;
	}
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void MenuStateCustomGame::publishToMasterserver()
{
	int slotCountUsed=0;
	int slotCountHumans=0;
	int slotCountConnectedPlayers=0;
	ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();
	GameSettings gameSettings;
	loadGameSettings(&gameSettings);
	string serverinfo="";

	MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
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
	serverinfo += "glestVersion=" + SystemFlags::escapeURL(glestVersionString) + "&";
	serverinfo += "platform=" + SystemFlags::escapeURL(getPlatformNameString()) + "&";
	serverinfo += "binaryCompileDate=" + SystemFlags::escapeURL(getCompileDateTime()) + "&";

	//game info:
	serverinfo += "serverTitle=" + SystemFlags::escapeURL(Config::getInstance().getString("NetPlayerName") + "'s game") + "&";
	//ip is automatically set

	//game setup info:
	serverinfo += "tech=" + SystemFlags::escapeURL(listBoxTechTree.getSelectedItem()) + "&";
	serverinfo += "map=" + SystemFlags::escapeURL(listBoxMap.getSelectedItem()) + "&";
	serverinfo += "tileset=" + SystemFlags::escapeURL(listBoxTileset.getSelectedItem()) + "&";
	serverinfo += "activeSlots=" + intToStr(slotCountUsed) + "&";
	serverinfo += "networkSlots=" + intToStr(slotCountHumans) + "&";
	serverinfo += "connectedClients=" + intToStr(slotCountConnectedPlayers);

	publishToServerInfo = serverinfo;
	safeMutex.ReleaseLock();
}

void MenuStateCustomGame::simpleTask() {

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

	MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
	bool republish = (needToRepublishToMasterserver == true  && publishToServerInfo != "");
	if(publishToMasterserverThread == NULL || publishToMasterserverThread->getQuitStatus() == true || publishToMasterserverThread->getRunningStatus() == false) {
		return;
	}
	needToRepublishToMasterserver = false;
	string newPublishToServerInfo = publishToServerInfo;
	publishToServerInfo = "";
	//safeMutex.ReleaseLock(true);

	if(republish == true) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		string request = Config::getInstance().getString("Masterserver") + "addServerInfo.php?" + newPublishToServerInfo;

		//printf("the request is:\n%s\n",request.c_str());
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] the request is:\n%s\n",__FILE__,__FUNCTION__,__LINE__,request.c_str());

		if(publishToMasterserverThread == NULL || publishToMasterserverThread->getQuitStatus() == true || publishToMasterserverThread->getRunningStatus() == false) {
			return;
		}
		
		std::string serverInfo = SystemFlags::getHTTP(request);
		//printf("the result is:\n'%s'\n",serverInfo.c_str());
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] the result is:\n'%s'\n",__FILE__,__FUNCTION__,__LINE__,serverInfo.c_str());

		// uncomment to enable router setup check of this server
		//if(serverInfo!="OK")
		if(EndsWith(serverInfo, "OK") == false) {
			//safeMutex.Lock();
			showMasterserverError=true;
			masterServererErrorToShow=serverInfo;
			//safeMutex.ReleaseLock(true);
		}
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//safeMutex.Lock();
	bool broadCastSettings = needToBroadcastServerSettings;
	if(publishToMasterserverThread == NULL || publishToMasterserverThread->getQuitStatus() == true || publishToMasterserverThread->getRunningStatus() == false) {
		return;
	}
	needToBroadcastServerSettings=false;
	//safeMutex.ReleaseLock(true);

	if(broadCastSettings) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		if(serverInterface->hasClientConnection() == true) {
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

			if(publishToMasterserverThread == NULL || publishToMasterserverThread->getQuitStatus() == true || publishToMasterserverThread->getRunningStatus() == false) {
				return;
			}

			GameSettings gameSettings;
			loadGameSettings(&gameSettings);

			if(publishToMasterserverThread == NULL || publishToMasterserverThread->getQuitStatus() == true || publishToMasterserverThread->getRunningStatus() == false) {
				return;
			}

			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

			serverInterface->setGameSettings(&gameSettings);
			serverInterface->broadcastGameSetup(&gameSettings);
		}
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void MenuStateCustomGame::loadGameSettings(GameSettings *gameSettings) {
    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

    MutexSafeWrapper safeMutex(&masterServerThreadAccessor);

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

    for(int i=0; i<mapInfo.players; ++i)
    {
		ControlType ct= static_cast<ControlType>(listBoxControls[i].getSelectedItemIndex());
		if(ct != ctClosed)
		{
			if(ct == ctHuman)
			{
				gameSettings->setThisFactionIndex(factionCount);
			}
			gameSettings->setFactionControl(factionCount, ct);
			gameSettings->setTeam(factionCount, listBoxTeams[i].getSelectedItemIndex());
			gameSettings->setStartLocationIndex(factionCount, i);
			gameSettings->setFactionTypeName(factionCount, factionFiles[listBoxFactions[i].getSelectedItemIndex()]);
			if(listBoxControls[i].getSelectedItemIndex() == ctNetwork)
			{
				ConnectionSlot* connectionSlot= serverInterface->getSlot(i);
				if((connectionSlot!=NULL) && (connectionSlot->isConnected()))
				{
					gameSettings->setNetworkPlayerName(factionCount, connectionSlot->getName());
				}
				else
				{
					gameSettings->setNetworkPlayerName(factionCount, "???");
				}
			}
			else if (listBoxControls[i].getSelectedItemIndex() == ctHuman)
			{
				gameSettings->setNetworkPlayerName(factionCount, Config::getInstance().getString("NetPlayerName",Socket::getHostName().c_str()));
			}
			else
			{
				gameSettings->setNetworkPlayerName(factionCount, "");
			}
			factionCount++;
		}
    }
	gameSettings->setFactionCount(factionCount);
	gameSettings->setEnableServerControlledAI(listBoxEnableServerControlledAI.getSelectedItemIndex() == 0);
	gameSettings->setNetworkFramePeriod((listBoxNetworkFramePeriod.getSelectedItemIndex()+1)*10);
	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] gameSettings->getTileset() = [%s]\n",__FILE__,__FUNCTION__,gameSettings->getTileset().c_str());
	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] gameSettings->getTech() = [%s]\n",__FILE__,__FUNCTION__,gameSettings->getTech().c_str());
	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] gameSettings->getMap() = [%s]\n",__FILE__,__FUNCTION__,gameSettings->getMap().c_str());

	safeMutex.ReleaseLock();

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
	saveGameFile << "EnableServerControlledAI=" << gameSettings.getEnableServerControlledAI() << std::endl;
	saveGameFile << "NetworkFramePeriod=" << gameSettings.getNetworkFramePeriod() << std::endl;

	saveGameFile << "FactionThisFactionIndex=" << gameSettings.getThisFactionIndex() << std::endl;
	saveGameFile << "FactionCount=" << gameSettings.getFactionCount() << std::endl;

    for(int i = 0; i < gameSettings.getFactionCount(); ++i) {
		saveGameFile << "FactionControlForIndex" << i << "=" << gameSettings.getFactionControl(i) << std::endl;
		saveGameFile << "FactionTeamForIndex" << i << "=" << gameSettings.getTeam(i) << std::endl;
		saveGameFile << "FactionStartLocationForIndex" << i << "=" << gameSettings.getStartLocationIndex(i) << std::endl;
		saveGameFile << "FactionTypeNameForIndex" << i << "=" << gameSettings.getFactionTypeName(i) << std::endl;
		saveGameFile << "FactionPlayerNameForIndex" << i << "=" << gameSettings.getNetworkPlayerName(i) << std::endl;
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
		gameSettings.setEnableServerControlledAI(properties.getBool("EnableServerControlledAI","false"));
		gameSettings.setNetworkFramePeriod(properties.getInt("NetworkFramePeriod",intToStr(GameConstants::networkFramePeriod).c_str())/10*10);

		gameSettings.setThisFactionIndex(properties.getInt("FactionThisFactionIndex"));
		gameSettings.setFactionCount(properties.getInt("FactionCount"));

		for(int i = 0; i < gameSettings.getFactionCount(); ++i) {
			gameSettings.setFactionControl(i,(ControlType)properties.getInt(string("FactionControlForIndex") + intToStr(i)) );
			gameSettings.setTeam(i,properties.getInt(string("FactionTeamForIndex") + intToStr(i)) );
			gameSettings.setStartLocationIndex(i,properties.getInt(string("FactionStartLocationForIndex") + intToStr(i)) );
			gameSettings.setFactionTypeName(i,properties.getString(string("FactionTypeNameForIndex") + intToStr(i)) );
			gameSettings.setNetworkPlayerName(i,properties.getString(string("FactionPlayerNameForIndex") + intToStr(i)) );
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
		listBoxEnableServerControlledAI.setSelectedItem(gameSettings.getEnableServerControlledAI() == true ? lang.get("Yes") : lang.get("No"));

		labelNetworkFramePeriod.setText(lang.get("NetworkFramePeriod"));
		
		listBoxNetworkFramePeriod.setSelectedItem(intToStr(gameSettings.getNetworkFramePeriod()/10*10));

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

		reloadFactions();

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d] gameSettings.getFactionCount() = %d\n",__FILE__,__FUNCTION__,__LINE__,gameSettings.getFactionCount());

		for(int i = 0; i < gameSettings.getFactionCount(); ++i) {
			listBoxControls[i].setSelectedItemIndex(gameSettings.getFactionControl(i));
			listBoxTeams[i].setSelectedItemIndex(gameSettings.getTeam(i));

			string factionName = gameSettings.getFactionTypeName(i);
			factionName = formatString(factionName);

			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] factionName = [%s]\n",__FILE__,__FUNCTION__,__LINE__,factionName.c_str());

			listBoxFactions[i].setSelectedItem(factionName);
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

bool MenuStateCustomGame::hasNetworkGameSettings()
{
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
		MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
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

void MenuStateCustomGame::updateNetworkSlots()
{
	try {
		ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();

		for(int i= 0; i<GameConstants::maxPlayers; ++i)
		{
			if(serverInterface->getSlot(i) == NULL && listBoxControls[i].getSelectedItemIndex() == ctNetwork)
			{
				serverInterface->addSlot(i);
			}
			if(serverInterface->getSlot(i) != NULL && listBoxControls[i].getSelectedItemIndex() != ctNetwork)
			{
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

void MenuStateCustomGame::keyDown(char key)
{
	//send key to the chat manager
	chatManager.keyDown(key);
	if(!chatManager.getEditEnabled()){
		if(key=='M'){
			showFullConsole= true;
		}
	}
}

void MenuStateCustomGame::keyPress(char c)
{
	chatManager.keyPress(c);
}

void MenuStateCustomGame::keyUp(char key)
{
	chatManager.keyUp(key);
	if(chatManager.getEditEnabled()){
		//send key to the chat manager
		chatManager.keyUp(key);
	}
	else if(key== 'M'){
		showFullConsole= false;
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

}}//end namespace
