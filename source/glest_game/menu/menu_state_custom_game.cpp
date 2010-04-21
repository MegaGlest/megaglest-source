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
#include "metrics.h"
#include "network_manager.h"
#include "network_message.h"
#include "client_interface.h"
#include "conversion.h"
#include "socket.h"
#include "game.h"
#include <algorithm>
#include <time.h>

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

MenuStateCustomGame::MenuStateCustomGame(Program *program, MainMenu *mainMenu, bool openNetworkSlots):
	MenuState(program, mainMenu, "new-game")
{
	Lang &lang= Lang::getInstance();
	NetworkManager &networkManager= NetworkManager::getInstance();
    Config &config = Config::getInstance();

	needToSetChangedGameSettings = false;
	lastSetChangedGameSettings   = time(NULL);;

	vector<string> teamItems, controlItems, results;

	//create
	buttonReturn.init(350, 180, 125);
	buttonPlayNow.init(525, 180, 125);

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

	listBoxMap.init(100, 260, 200);
    listBoxMap.setItems(results);
	labelMap.init(100, 290);
	labelMapInfo.init(100, 230, 200, 40);
	
	// fog - o - war
	// @350 ? 300 ?
	labelFogOfWar.init(350, 290, 100);
	listBoxFogOfWar.init(350, 260, 100);
	listBoxFogOfWar.pushBackItem(lang.get("Yes"));
	listBoxFogOfWar.pushBackItem(lang.get("No"));
	listBoxFogOfWar.setSelectedItemIndex(0);

    //tileset listBox
    findDirs(config.getPathListForType(ptTilesets), results);
	if (results.empty()) {
        throw runtime_error("No tile-sets were found!");
	}
    tilesetFiles= results;
	std::for_each(results.begin(), results.end(), FormatString());
	listBoxTileset.init(500, 260, 150);
    listBoxTileset.setItems(results);
	labelTileset.init(500, 290);

    //tech Tree listBox
    findDirs(config.getPathListForType(ptTechs), results);
	if(results.empty()) {
        throw runtime_error("No tech-trees were found!");
	}
    techTreeFiles= results;
	std::for_each(results.begin(), results.end(), FormatString());
	listBoxTechTree.init(700, 260, 150);
    listBoxTechTree.setItems(results);
	labelTechTree.init(700, 290);

	//list boxes
    for(int i=0; i<GameConstants::maxPlayers; ++i){
		labelPlayers[i].init(100, 550-i*30);
        listBoxControls[i].init(200, 550-i*30);
        listBoxFactions[i].init(400, 550-i*30);
		listBoxTeams[i].init(600, 550-i*30, 60);
		labelNetStatus[i].init(700, 550-i*30, 60);
    }

	labelControl.init(200, 600, GraphicListBox::defW, GraphicListBox::defH, true);
    labelFaction.init(400, 600, GraphicListBox::defW, GraphicListBox::defH, true);
    labelTeam.init(600, 600, 60, GraphicListBox::defH, true);

	//texts
	buttonReturn.setText(lang.get("Return"));
	buttonPlayNow.setText(lang.get("PlayNow"));

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

	labelMap.setText(lang.get("Map"));
	labelFogOfWar.setText(lang.get("FogOfWar"));
	labelTileset.setText(lang.get("Tileset"));
	labelTechTree.setText(lang.get("TechTree"));
	labelControl.setText(lang.get("Control"));
    labelFaction.setText(lang.get("Faction"));
    labelTeam.setText(lang.get("Team"));

	loadMapInfo(Map::getMapPath(mapFiles[listBoxMap.getSelectedItemIndex()]), &mapInfo);

	labelMapInfo.setText(mapInfo.desc);

	//initialize network interface
	networkManager.init(nrServer);

	//init controllers
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

	//chatManager.init(&console, world.getThisTeamIndex());
	chatManager.init(&console, -1);
}

void MenuStateCustomGame::mouseClick(int x, int y, MouseButton mouseButton){

	CoreData &coreData= CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();
	ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();

	if(buttonReturn.mouseClick(x,y)){
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		soundRenderer.playFx(coreData.getClickSoundA());
		mainMenu->setState(new MenuStateNewGame(program, mainMenu));
    }
	else if(buttonPlayNow.mouseClick(x,y) && buttonPlayNow.getEnabled()) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		closeUnusedSlots();
		soundRenderer.playFx(coreData.getClickSoundC());

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		GameSettings gameSettings;
		loadGameSettings(&gameSettings);

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

        // Send the game settings to each client if we have at least one networked client
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
		if(bOkToStart == true)
		{
            program->setState(new Game(program, &gameSettings));
		}
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}
	else if(listBoxMap.mouseClick(x, y)){
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s\n", mapFiles[listBoxMap.getSelectedItemIndex()].c_str());

		loadMapInfo(Map::getMapPath(mapFiles[listBoxMap.getSelectedItemIndex()]), &mapInfo);
		labelMapInfo.setText(mapInfo.desc);
		updateControlers();

        if(hasNetworkGameSettings() == true)
        {
            needToSetChangedGameSettings = true;
            lastSetChangedGameSettings   = time(NULL);;
        }
	}
	else if (listBoxFogOfWar.mouseClick(x, y)) {
        if(hasNetworkGameSettings() == true)
        {
            needToSetChangedGameSettings = true;
            lastSetChangedGameSettings   = time(NULL);;
        }
	}
	else if(listBoxTileset.mouseClick(x, y)){

        if(hasNetworkGameSettings() == true)
        {
            needToSetChangedGameSettings = true;
            lastSetChangedGameSettings   = time(NULL);;
        }
	}
	else if(listBoxTechTree.mouseClick(x, y)){
		reloadFactions();

        if(hasNetworkGameSettings() == true)
        {
            needToSetChangedGameSettings = true;
            lastSetChangedGameSettings   = time(NULL);;
        }
	}
	else
	{
		for(int i=0; i<mapInfo.players; ++i)
		{
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

                if(hasNetworkGameSettings() == true)
                {
                    needToSetChangedGameSettings = true;
                    lastSetChangedGameSettings   = time(NULL);;
                }
			}
			else if(listBoxFactions[i].mouseClick(x, y)){

                if(hasNetworkGameSettings() == true)
                {
                    needToSetChangedGameSettings = true;
                    lastSetChangedGameSettings   = time(NULL);;
                }
			}
			else if(listBoxTeams[i].mouseClick(x, y))
			{
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

	buttonReturn.mouseMove(x, y);
	buttonPlayNow.mouseMove(x, y);

	for(int i=0; i<GameConstants::maxPlayers; ++i){
        listBoxControls[i].mouseMove(x, y);
        listBoxFactions[i].mouseMove(x, y);
		listBoxTeams[i].mouseMove(x, y);
    }
	listBoxMap.mouseMove(x, y);
	listBoxFogOfWar.mouseMove(x, y);
	listBoxTileset.mouseMove(x, y);
	listBoxTechTree.mouseMove(x, y);
}

void MenuStateCustomGame::render(){

	try {
		Renderer &renderer= Renderer::getInstance();

		int i;

		renderer.renderButton(&buttonReturn);
		renderer.renderButton(&buttonPlayNow);

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

		renderer.renderListBox(&listBoxMap);
		renderer.renderListBox(&listBoxFogOfWar);
		renderer.renderListBox(&listBoxTileset);
		renderer.renderListBox(&listBoxTechTree);

		renderer.renderChatManager(&chatManager);
		renderer.renderConsole(&console);
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
		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();
		Lang& lang= Lang::getInstance();

		bool haveAtLeastOneNetworkClientConnected = false;
		Config &config = Config::getInstance();
		for(int i= 0; i<mapInfo.players; ++i)
		{
			if(listBoxControls[i].getSelectedItemIndex() == ctNetwork)
			{
				//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] START - ctNetwork\n",__FILE__,__FUNCTION__);

				ConnectionSlot* connectionSlot= serverInterface->getSlot(i);

				assert(connectionSlot!=NULL);

				//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] A - ctNetwork\n",__FILE__,__FUNCTION__);

				if(connectionSlot->isConnected())
				{
					haveAtLeastOneNetworkClientConnected = true;
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
							//if(connectionSlot->getNetworkGameDataSynchCheckOkFogOfWar() == false)
							//{
							//    label = label + " FogOfWar == false";
							//}

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
							//if(connectionSlot->getNetworkGameDataSynchCheckOkFogOfWar() == false)
							//{
							//    label = label + " FogOfWar == false";
							//}
						}
					}

					labelNetStatus[i].setText(label);
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

		// Send the game settings to each client if we have at least one networked client
		if( serverInterface->getAllowGameDataSynchCheck() == true &&
			haveAtLeastOneNetworkClientConnected == true &&
			needToSetChangedGameSettings == true &&
			difftime(time(NULL),lastSetChangedGameSettings) >= 2)
		{
			GameSettings gameSettings;
			loadGameSettings(&gameSettings);
			serverInterface->setGameSettings(&gameSettings);

			needToSetChangedGameSettings    = false;
		}
		
		if(difftime(time(NULL),lastSetChangedGameSettings) >= 2)
		{
			GameSettings gameSettings;
			loadGameSettings(&gameSettings);
			serverInterface->setGameSettings(&gameSettings);
			serverInterface->broadcastGameSetup(&gameSettings);
		}

		//call the chat manager
		chatManager.updateNetwork();

		//console
		console.update();

		if(difftime(time(NULL),lastSetChangedGameSettings) >= 2)
		{// reset timer here on bottom becasue used for different things
			lastSetChangedGameSettings      = time(NULL);
		}
		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}
	catch(const std::exception &ex) {
		char szBuf[1024]="";
		sprintf(szBuf,"In [%s::%s %d] error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		throw runtime_error(szBuf);
	}
}

void MenuStateCustomGame::loadGameSettings(GameSettings *gameSettings)
{
    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

	int factionCount= 0;

	gameSettings->setDescription(formatString(mapFiles[listBoxMap.getSelectedItemIndex()]));
	gameSettings->setMap(mapFiles[listBoxMap.getSelectedItemIndex()]);
    gameSettings->setTileset(tilesetFiles[listBoxTileset.getSelectedItemIndex()]);
    gameSettings->setTech(techTreeFiles[listBoxTechTree.getSelectedItemIndex()]);
	gameSettings->setDefaultUnits(true);
	gameSettings->setDefaultResources(true);
	gameSettings->setDefaultVictoryConditions(true);
	gameSettings->setFogOfWar(listBoxFogOfWar.getSelectedItemIndex() == 0);

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
			factionCount++;
		}
    }
	gameSettings->setFactionCount(factionCount);

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] gameSettings->getTileset() = [%s]\n",__FILE__,__FUNCTION__,gameSettings->getTileset().c_str());
	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] gameSettings->getTech() = [%s]\n",__FILE__,__FUNCTION__,gameSettings->getTech().c_str());
	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] gameSettings->getMap() = [%s]\n",__FILE__,__FUNCTION__,gameSettings->getMap().c_str());

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
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
		throw runtime_error(szBuf);
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
		throw runtime_error(szBuf);
	}
}

void MenuStateCustomGame::keyDown(char key)
{
	//send key to the chat manager
	chatManager.keyDown(key);
}

void MenuStateCustomGame::keyPress(char c)
{
	chatManager.keyPress(c);
}

}}//end namespace
