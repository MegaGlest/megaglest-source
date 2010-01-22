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

#include "leak_dumper.h"

namespace Glest{ namespace Game{

using namespace Shared::Util;

// =====================================================
// 	class MenuStateCustomGame
// =====================================================

MenuStateCustomGame::MenuStateCustomGame(Program *program, MainMenu *mainMenu, bool openNetworkSlots): 
	MenuState(program, mainMenu, "new-game")
{
	Lang &lang= Lang::getInstance();
	NetworkManager &networkManager= NetworkManager::getInstance();

	vector<string> results, teamItems, controlItems;

	//create
	buttonReturn.init(350, 200, 125);
	buttonPlayNow.init(525, 200, 125);

    //map listBox
    findAll("maps/*.gbm", results, true);
	if(results.size()==0){
        throw runtime_error("There is no maps");
	}
	mapFiles= results;
	for(int i= 0; i<results.size(); ++i){
		results[i]= formatString(results[i]);
	}
    listBoxMap.init(200, 320, 150);
    listBoxMap.setItems(results);
	labelMap.init(200, 350);
	labelMapInfo.init(200, 290, 200, 40);

    //tileset listBox
    findAll("tilesets/*.", results);
	if(results.size()==0){
        throw runtime_error("There is no tile set");
	}
    tilesetFiles= results;
	for(int i= 0; i<results.size(); ++i){
		results[i]= formatString(results[i]);
	}
	listBoxTileset.init(400, 320, 150);
    listBoxTileset.setItems(results);
	labelTileset.init(400, 350);

    //tech Tree listBox
    findAll("techs/*.", results);
	if(results.size()==0){
        throw runtime_error("There is no tech tree");
	}
    techTreeFiles= results;
	for(int i= 0; i<results.size(); ++i){
		results[i]= formatString(results[i]);
	}
	listBoxTechTree.init(600, 320, 150);
    listBoxTechTree.setItems(results);
	labelTechTree.init(600, 350);

	//list boxes
    for(int i=0; i<GameConstants::maxPlayers; ++i){
		labelPlayers[i].init(200, 500-i*30);
        listBoxControls[i].init(300, 500-i*30);
        listBoxFactions[i].init(500, 500-i*30);
		listBoxTeams[i].init(700, 500-i*30, 60);
		labelNetStatus[i].init(800, 500-i*30, 60);
    }

	labelControl.init(300, 550, GraphicListBox::defW, GraphicListBox::defH, true);
    labelFaction.init(500, 550, GraphicListBox::defW, GraphicListBox::defH, true);
    labelTeam.init(700, 550, 60, GraphicListBox::defH, true);

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

	reloadFactions();

	findAll("techs/"+techTreeFiles[listBoxTechTree.getSelectedItemIndex()]+"/factions/*.", results);
	if(results.size()==0){
        throw runtime_error("There is no factions for this tech tree");
	}
   
	for(int i=0; i<GameConstants::maxPlayers; ++i){
		labelPlayers[i].setText(lang.get("Player")+" "+intToStr(i));
        listBoxTeams[i].setItems(teamItems);
		listBoxTeams[i].setSelectedItemIndex(i);
		listBoxControls[i].setItems(controlItems); 
		labelNetStatus[i].setText("");
    }

	labelMap.setText(lang.get("Map"));
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
}

void MenuStateCustomGame::mouseClick(int x, int y, MouseButton mouseButton){

	CoreData &coreData= CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();
	ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();

	if(buttonReturn.mouseClick(x,y)){
		soundRenderer.playFx(coreData.getClickSoundA());
		mainMenu->setState(new MenuStateNewGame(program, mainMenu));
    }  
	else if(buttonPlayNow.mouseClick(x,y)){
		GameSettings gameSettings;
		
		closeUnusedSlots();
		soundRenderer.playFx(coreData.getClickSoundC());
		loadGameSettings(&gameSettings);
		serverInterface->launchGame(&gameSettings);
		program->setState(new Game(program, &gameSettings));
	}
	else if(listBoxMap.mouseClick(x, y)){
		loadMapInfo(Map::getMapPath(mapFiles[listBoxMap.getSelectedItemIndex()]), &mapInfo);
		labelMapInfo.setText(mapInfo.desc);
		updateControlers();
	}
	else if(listBoxTileset.mouseClick(x, y)){
	}
	else if(listBoxTechTree.mouseClick(x, y)){
		reloadFactions();
	}
	else{
		for(int i=0; i<mapInfo.players; ++i){
			//ensure thet only 1 human player is present
			if(listBoxControls[i].mouseClick(x, y)){

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
			}
			else if(listBoxFactions[i].mouseClick(x, y)){
			}
			else if(listBoxTeams[i].mouseClick(x, y)){
			}
		}
	}
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
	listBoxTileset.mouseMove(x, y);
	listBoxTechTree.mouseMove(x, y);
}

void MenuStateCustomGame::render(){

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
	renderer.renderLabel(&labelTileset);
	renderer.renderLabel(&labelTechTree);
	renderer.renderLabel(&labelControl);
	renderer.renderLabel(&labelFaction);
	renderer.renderLabel(&labelTeam);
	renderer.renderLabel(&labelMapInfo);

	renderer.renderListBox(&listBoxMap);
	renderer.renderListBox(&listBoxTileset);
	renderer.renderListBox(&listBoxTechTree);
}

void MenuStateCustomGame::update(){
	ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();
	Lang& lang= Lang::getInstance();

	for(int i= 0; i<mapInfo.players; ++i){
		if(listBoxControls[i].getSelectedItemIndex()==ctNetwork){
			ConnectionSlot* connectionSlot= serverInterface->getSlot(i);

			assert(connectionSlot!=NULL);
			
			if(connectionSlot->isConnected()){
				labelNetStatus[i].setText(connectionSlot->getName());
			}
			else
			{
				labelNetStatus[i].setText(lang.get("NotConnected"));
			}
		}
		else{
			labelNetStatus[i].setText("");
		}
	}
}

void MenuStateCustomGame::loadGameSettings(GameSettings *gameSettings){

	int factionCount= 0;

	gameSettings->setDescription(formatString(mapFiles[listBoxMap.getSelectedItemIndex()]));
	gameSettings->setMap(mapFiles[listBoxMap.getSelectedItemIndex()]);
    gameSettings->setTileset(tilesetFiles[listBoxTileset.getSelectedItemIndex()]);
    gameSettings->setTech(techTreeFiles[listBoxTechTree.getSelectedItemIndex()]);
	gameSettings->setDefaultUnits(true);
	gameSettings->setDefaultResources(true);
	gameSettings->setDefaultVictoryConditions(true);
   
    for(int i=0; i<mapInfo.players; ++i){
		ControlType ct= static_cast<ControlType>(listBoxControls[i].getSelectedItemIndex());
		if(ct!=ctClosed){
			if(ct==ctHuman){
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
}

// ============ PRIVATE ===========================

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
		fread(&header, sizeof(MapFileHeader), 1, f);

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

	findAll("techs/"+techTreeFiles[listBoxTechTree.getSelectedItemIndex()]+"/factions/*.", results);
	if(results.size()==0){
        throw runtime_error("There is no factions for this tech tree");
	}
	factionFiles= results;
   	for(int i= 0; i<results.size(); ++i){
		results[i]= formatString(results[i]);
	}
	for(int i=0; i<GameConstants::maxPlayers; ++i){
		listBoxFactions[i].setItems(results);
		listBoxFactions[i].setSelectedItemIndex(i % results.size());
    }
}

void MenuStateCustomGame::updateControlers(){
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

void MenuStateCustomGame::closeUnusedSlots(){
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

void MenuStateCustomGame::updateNetworkSlots(){
	ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();
	
	for(int i= 0; i<GameConstants::maxPlayers; ++i){
		if(serverInterface->getSlot(i)==NULL && listBoxControls[i].getSelectedItemIndex()==ctNetwork){
			serverInterface->addSlot(i);
		}
		if(serverInterface->getSlot(i) != NULL && listBoxControls[i].getSelectedItemIndex()!=ctNetwork){
			serverInterface->removeSlot(i);
		}
	}
}

}}//end namespace
