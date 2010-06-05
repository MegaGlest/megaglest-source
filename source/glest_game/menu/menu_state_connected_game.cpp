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

#include "menu_state_connected_game.h"

#include "menu_state_join_game.h"
#include "menu_state_masterserver.h"
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
// 	class MenuStateConnectedGame
// =====================================================

MenuStateConnectedGame::MenuStateConnectedGame(Program *program, MainMenu *mainMenu,JoinMenu joinMenuInfo, bool openNetworkSlots):
	MenuState(program, mainMenu, "connected-game") //← set on connected-game 
{
	returnMenuInfo=joinMenuInfo;
	Lang &lang= Lang::getInstance();
	NetworkManager &networkManager= NetworkManager::getInstance();
    Config &config = Config::getInstance();
	needToSetChangedGameSettings = false;
	lastSetChangedGameSettings   = time(NULL);
	

	currentFactionName="";
	currentMap="";
	settingsReceivedFromServer=false;


	vector<string> teamItems, controlItems, results;
	//state
	labelStatus.init(330, 700);
	labelStatus.setText("");

	labelInfo.init(30, 700);
	labelInfo.setText("");
	
	
	
	//create
	buttonDisconnect.init(350, 180, 125);
	buttonPlayNow.init(525, 180, 125);

    //map listBox
	// put them all in a set, to weed out duplicates (gbm & mgm with same name)
	// will also ensure they are alphabetically listed (rather than how the OS provides them)
	listBoxMap.init(100, 260, 200);
	listBoxMap.setEditable(false);
	
    //listBoxMap.setItems(results);
	labelMap.init(100, 290);
	labelMapInfo.init(100, 230, 200, 40);
	

	// fog - o - war
	// @350 ? 300 ?
	labelFogOfWar.init(350, 290, 100);
	listBoxFogOfWar.init(350, 260, 100);
	listBoxFogOfWar.pushBackItem(lang.get("Yes"));
	listBoxFogOfWar.pushBackItem(lang.get("No"));
	listBoxFogOfWar.setSelectedItemIndex(0);
	listBoxFogOfWar.setEditable(false);

    //tileset listBox
	listBoxTileset.init(500, 260, 150);
	listBoxTileset.setEditable(false);
    //listBoxTileset.setItems(results);
	labelTileset.init(500, 290);

    //tech Tree listBox
	listBoxTechTree.init(700, 260, 150);
	listBoxTechTree.setEditable(false);
    //listBoxTechTree.setItems(results);
	labelTechTree.init(700, 290);
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
	//list boxes
    for(int i=0; i<GameConstants::maxPlayers; ++i){
		labelPlayers[i].init(100, 550-i*30);
		labelPlayers[i].setEditable(false);
        listBoxControls[i].init(200, 550-i*30);
        listBoxControls[i].setEditable(false);
        listBoxFactions[i].init(400, 550-i*30);
        listBoxFactions[i].setEditable(false);
		listBoxTeams[i].init(600, 550-i*30, 60);
		listBoxTeams[i].setEditable(false);
		labelNetStatus[i].init(700, 550-i*30, 60);
		grabSlotButton[i].init(700, 550-i*30, 30);
		grabSlotButton[i].setText(">");
    }

	labelControl.init(200, 600, GraphicListBox::defW, GraphicListBox::defH, true);
    labelFaction.init(400, 600, GraphicListBox::defW, GraphicListBox::defH, true);
    labelTeam.init(600, 600, 60, GraphicListBox::defH, true);

	//texts
	buttonDisconnect.setText(lang.get("Return"));
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
	for(int i=0; i<GameConstants::maxPlayers; ++i){
		labelPlayers[i].setText(lang.get("Player")+" "+intToStr(i));
        listBoxTeams[i].setItems(teamItems);
		listBoxTeams[i].setSelectedItemIndex(i);
		listBoxControls[i].setItems(controlItems);
		labelNetStatus[i].setText("V");
    }
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
	labelMap.setText(lang.get("Map"));
	labelFogOfWar.setText(lang.get("FogOfWar"));
	labelTileset.setText(lang.get("Tileset"));
	labelTechTree.setText(lang.get("TechTree"));
	labelControl.setText(lang.get("Control"));
    labelFaction.setText(lang.get("Faction"));
    labelTeam.setText(lang.get("Team"));

SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
	labelMapInfo.setText(mapInfo.desc);

	//init controllers
	listBoxControls[0].setSelectedItemIndex(ctHuman);
	chatManager.init(&console, -1);
}

void MenuStateConnectedGame::mouseClick(int x, int y, MouseButton mouseButton){

	CoreData &coreData= CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();
	NetworkManager &networkManager= NetworkManager::getInstance();
	ClientInterface* clientInterface= networkManager.getClientInterface();

	if (!settingsReceivedFromServer) return;
	
	if(buttonDisconnect.mouseClick(x,y)){
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		soundRenderer.playFx(coreData.getClickSoundA());
		if(clientInterface->getSocket() != NULL)
		{
		    if(clientInterface->isConnected() == true)
		    {
                string sQuitText = Config::getInstance().getString("NetPlayerName",Socket::getHostName().c_str()) + " has chosen to leave the game!";
                clientInterface->sendTextMessage(sQuitText,-1);
		    }
		    clientInterface->close();
		}
		clientInterface->reset();
		networkManager.end();
		currentFactionName="";
		currentMap="";
		returnToJoinMenu();
    }
	else if(buttonPlayNow.mouseClick(x,y) && buttonPlayNow.getEnabled()) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
		soundRenderer.playFx(coreData.getClickSoundC());	
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}
	else
	{
		int myCurrentIndex=-1;
		for(int i=0; i<GameConstants::maxPlayers; ++i)
		{// find my current index by looking at editable listBoxes
			if(listBoxFactions[i].getEditable()){
				myCurrentIndex=i;
			}
		}
		if (myCurrentIndex!=-1)
		for(int i=0; i<GameConstants::maxPlayers; ++i)
		{
			if(listBoxFactions[i].getEditable()){
				if(listBoxFactions[i].mouseClick(x, y)){
					soundRenderer.playFx(coreData.getClickSoundA());
					ClientInterface* clientInterface= NetworkManager::getInstance().getClientInterface();
					if(clientInterface->isConnected()){
						clientInterface->setGameSettingsReceived(false);
						clientInterface->sendSwitchSetupRequest(listBoxFactions[i].getSelectedItem(),i,-1,listBoxTeams[i].getSelectedItemIndex());
					}
					break;
				}
			}
			if(listBoxTeams[i].getEditable()){
				if(listBoxTeams[i].mouseClick(x, y)){
					soundRenderer.playFx(coreData.getClickSoundA());
					if(clientInterface->isConnected()){
						clientInterface->setGameSettingsReceived(false);
						clientInterface->sendSwitchSetupRequest(listBoxFactions[i].getSelectedItem(),i,-1,listBoxTeams[i].getSelectedItemIndex());
					}
					break;
				}
			}
			if((listBoxControls[i].getSelectedItemIndex()==ctNetwork) && (labelNetStatus[i].getText()=="???")){
				if(grabSlotButton[i].mouseClick(x, y) )
				{
					soundRenderer.playFx(coreData.getClickSoundA());
					clientInterface->setGameSettingsReceived(false);
					settingsReceivedFromServer=false;
					SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] sending a switchSlot request from %d to %d\n",__FILE__,__FUNCTION__,__LINE__,clientInterface->getGameSettings()->getThisFactionIndex(),i);
					clientInterface->sendSwitchSetupRequest(listBoxFactions[myCurrentIndex].getSelectedItem(),myCurrentIndex,i,listBoxTeams[myCurrentIndex].getSelectedItemIndex());
					break;
				}
			}
		}


//		for(int i=0; i<mapInfo.players; ++i)
//		{
//			//ensure thet only 1 human player is present
//			if(listBoxControls[i].mouseClick(x, y))
//			{
//				//look for human players
//				int humanIndex1= -1;
//				int humanIndex2= -1;
//				for(int j=0; j<GameConstants::maxPlayers; ++j){
//					ControlType ct= static_cast<ControlType>(listBoxControls[j].getSelectedItemIndex());
//					if(ct==ctHuman){
//						if(humanIndex1==-1){
//							humanIndex1= j;
//						}
//						else{
//							humanIndex2= j;
//						}
//					}
//				}
//
//				//no human
//				if(humanIndex1==-1 && humanIndex2==-1){
//					listBoxControls[i].setSelectedItemIndex(ctHuman);
//				}
//
//				//2 humans
//				if(humanIndex1!=-1 && humanIndex2!=-1){
//					listBoxControls[humanIndex1==i? humanIndex2: humanIndex1].setSelectedItemIndex(ctClosed);
//				}
//
//                if(hasNetworkGameSettings() == true)
//                {
//                    needToSetChangedGameSettings = true;
//                    lastSetChangedGameSettings   = time(NULL);;
//                }
//			}
//			else if(listBoxFactions[i].mouseClick(x, y)){
//
//                if(hasNetworkGameSettings() == true)
//                {
//                    needToSetChangedGameSettings = true;
//                    lastSetChangedGameSettings   = time(NULL);;
//                }
//			}
//			else if(listBoxTeams[i].mouseClick(x, y))
//			{
//                if(hasNetworkGameSettings() == true)
//                {
//                    needToSetChangedGameSettings = true;
//                    lastSetChangedGameSettings   = time(NULL);;
//                }
//			}
//		}
	}
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void MenuStateConnectedGame::returnToJoinMenu(){
	if(returnMenuInfo==jmSimple)
	{
		mainMenu->setState(new MenuStateJoinGame(program, mainMenu));
	}
	else
	{
		mainMenu->setState(new MenuStateMasterserver(program, mainMenu));
	}
}


void MenuStateConnectedGame::mouseMove(int x, int y, const MouseState *ms){

	buttonDisconnect.mouseMove(x, y);
	buttonPlayNow.mouseMove(x, y);

	for(int i=0; i<GameConstants::maxPlayers; ++i){
        listBoxControls[i].mouseMove(x, y);
        listBoxFactions[i].mouseMove(x, y);
		listBoxTeams[i].mouseMove(x, y);
		grabSlotButton[i].mouseMove(x, y);
    }
	listBoxMap.mouseMove(x, y);
	listBoxFogOfWar.mouseMove(x, y);
	listBoxTileset.mouseMove(x, y);
	listBoxTechTree.mouseMove(x, y);
}

void MenuStateConnectedGame::render(){

	try {
		if (!settingsReceivedFromServer) return;
		Renderer &renderer= Renderer::getInstance();

		int i;

		renderer.renderButton(&buttonDisconnect);
		//renderer.renderButton(&buttonPlayNow);

		for(i=0; i<GameConstants::maxPlayers; ++i){
			renderer.renderLabel(&labelPlayers[i]);
			renderer.renderListBox(&listBoxControls[i]);
			if(listBoxControls[i].getSelectedItemIndex()!=ctClosed){
				renderer.renderListBox(&listBoxFactions[i]);
				renderer.renderListBox(&listBoxTeams[i]);
				//renderer.renderLabel(&labelNetStatus[i]);
				
				if((listBoxControls[i].getSelectedItemIndex()==ctNetwork) && (labelNetStatus[i].getText()=="???")){
					renderer.renderButton(&grabSlotButton[i]);
				}
				else if((listBoxControls[i].getSelectedItemIndex()==ctNetwork) ||
					(listBoxControls[i].getSelectedItemIndex()==ctHuman)){
					renderer.renderLabel(&labelNetStatus[i]);
				}
			}
		}
		renderer.renderLabel(&labelStatus);
		renderer.renderLabel(&labelInfo);
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

void MenuStateConnectedGame::update()
{
	ClientInterface* clientInterface= NetworkManager::getInstance().getClientInterface();
	Lang &lang= Lang::getInstance();

	//update status label
	if(clientInterface->isConnected())
	{
		buttonDisconnect.setText(lang.get("Disconnect"));

		if(clientInterface->getAllowDownloadDataSynch() == false)
		{
		    string label = lang.get("ConnectedToServer");

            if(!clientInterface->getServerName().empty())
            {
                label = label + " " + clientInterface->getServerName();
            }

            if(clientInterface->getAllowGameDataSynchCheck() == true &&
               clientInterface->getNetworkGameDataSynchCheckOk() == false)
            {
                label = label + " - warning synch mismatch for:";
                if(clientInterface->getNetworkGameDataSynchCheckOkMap() == false)
                {
                    label = label + " map";
                }
                if(clientInterface->getNetworkGameDataSynchCheckOkTile() == false)
                {
                    label = label + " tile";
                }
                if(clientInterface->getNetworkGameDataSynchCheckOkTech() == false)
                {
                    label = label + " techtree";
                }
                //if(clientInterface->getNetworkGameDataSynchCheckOkFogOfWar() == false)
                //{
                //    label = label + " FogOfWar == false";
                //}
            }
            else if(clientInterface->getAllowGameDataSynchCheck() == true)
            {
                label += " - data synch is ok";
            }

            std::string networkFrameString = lang.get("NetworkFramePeriod") + " " + intToStr(clientInterface->getGameSettings()->getNetworkFramePeriod());
			float pingTime = clientInterface->getThreadedPingMS(clientInterface->getServerIpAddress().c_str());
			char szBuf[1024]="";
			sprintf(szBuf,"%s, ping = %.2fms, %s",label.c_str(),pingTime,networkFrameString.c_str());

            labelStatus.setText(szBuf);
		}
		else
		{
		    string label = lang.get("ConnectedToServer");

            if(!clientInterface->getServerName().empty())
            {
                label = label + " " + clientInterface->getServerName();
            }

            if(clientInterface->getAllowGameDataSynchCheck() == true &&
               clientInterface->getNetworkGameDataSynchCheckOk() == false)
            {
                label = label + " - waiting to synch:";
                if(clientInterface->getNetworkGameDataSynchCheckOkMap() == false)
                {
                    label = label + " map";
                }
                if(clientInterface->getNetworkGameDataSynchCheckOkTile() == false)
                {
                    label = label + " tile";
                }
                if(clientInterface->getNetworkGameDataSynchCheckOkTech() == false)
                {
                    label = label + " techtree";
                }
                //if(clientInterface->getNetworkGameDataSynchCheckOkFogOfWar() == false)
                //{
                //    label = label + " FogOfWar == false";
                //}
            }
            else if(clientInterface->getAllowGameDataSynchCheck() == true)
            {
                label += " - data synch is ok";
            }

            std::string networkFrameString = lang.get("NetworkFramePeriod") + " " + intToStr(clientInterface->getGameSettings()->getNetworkFramePeriod());
			float pingTime = clientInterface->getThreadedPingMS(clientInterface->getServerIpAddress().c_str());
			char szBuf[1024]="";
			sprintf(szBuf,"%s, ping = %.2fms, %s",label.c_str(),pingTime,networkFrameString.c_str());

            labelStatus.setText(szBuf);
		}
	}
	else
	{
		if(clientInterface->getSocket() != NULL)
		{
		    clientInterface->close();
		}
		returnToJoinMenu();
		return;
	}

	//process network messages
	if(clientInterface->isConnected())
	{
		if(clientInterface->getGameSettingsReceived()){
			bool errorOnMissingData = (clientInterface->getAllowGameDataSynchCheck() == false);
			//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
			vector<string> maps,tilesets,techtree;
			const GameSettings *gameSettings=clientInterface->getGameSettings();
			
			// tileset
			tilesets.push_back(formatString(gameSettings->getTileset()));
			listBoxTileset.setItems(tilesets);
	
			// techtree
			techtree.push_back(formatString(gameSettings->getTech()));
			listBoxTechTree.setItems(techtree);
			
			// factions
			bool hasFactions = true;
			if(currentFactionName != gameSettings->getTech())
			{
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] hasFactions = %d, currentFactionName [%s]\n",__FILE__,__FUNCTION__,__LINE__,hasFactions,currentFactionName.c_str());
				currentFactionName = gameSettings->getTech();
				hasFactions = loadFactions(gameSettings,false);
			}
			
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] hasFactions = %d, currentFactionName [%s]\n",__FILE__,__FUNCTION__,__LINE__,hasFactions,currentFactionName.c_str());

			// map
			maps.push_back(formatString(gameSettings->getMap()));
			listBoxMap.setItems(maps);
			if(currentMap != gameSettings->getMap())
			{// load the setup again
				currentMap = gameSettings->getMap();
			}
			
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			// FogOfWar
			if(gameSettings->getFogOfWar()){
				listBoxFogOfWar.setSelectedItemIndex(0); 
			}
			else
			{
				listBoxFogOfWar.setSelectedItemIndex(1);
			}
			
			//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			// Control
			for(int i=0; i<GameConstants::maxPlayers; ++i){
				listBoxControls[i].setSelectedItemIndex(ctClosed);
				listBoxFactions[i].setEditable(false);
				listBoxTeams[i].setEditable(false);
			}
			
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			if(hasFactions == true) {
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] errorOnMissingData = %d\n",__FILE__,__FUNCTION__,__LINE__,errorOnMissingData);

				for(int i=0; i<gameSettings->getFactionCount(); ++i){
					int slot=gameSettings->getStartLocationIndex(i);
					listBoxControls[slot].setSelectedItemIndex(gameSettings->getFactionControl(i),errorOnMissingData);
					listBoxTeams[slot].setSelectedItemIndex(gameSettings->getTeam(i),errorOnMissingData);
					//listBoxFactions[slot].setSelectedItem(formatString(gameSettings->getFactionTypeName(i)),errorOnMissingData);
					listBoxFactions[slot].setSelectedItem(formatString(gameSettings->getFactionTypeName(i)),false);

					if(gameSettings->getFactionControl(i) == ctNetwork ){
						labelNetStatus[slot].setText(gameSettings->getNetworkPlayerName(i));
					}
					
					if(gameSettings->getFactionControl(i) == ctNetwork && gameSettings->getThisFactionIndex() == i){
						// set my current slot to ctHuman
						listBoxControls[slot].setSelectedItemIndex(ctHuman);
						listBoxFactions[slot].setEditable(true);
						listBoxTeams[slot].setEditable(true);
					}
					
					settingsReceivedFromServer=true;
				}
			}
		}
		//update lobby
		clientInterface->updateLobby();

        //call the chat manager
        chatManager.updateNetwork();

        //console
        console.update();

		//intro
		if(clientInterface->getIntroDone())
		{
			labelInfo.setText(lang.get("WaitingHost"));
			//servers.setString(clientInterface->getServerName(), Ip(labelServerIp.getText()).getString());
		}

		//launch
		if(clientInterface->getLaunchGame())
		{
		    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			//servers.save(serversSavedFile);

		    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		    assert(clientInterface != NULL);

		    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			program->setState(new Game(program, clientInterface->getGameSettings()));

			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		}

		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}

    if(clientInterface->getLaunchGame()) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] clientInterface->getLaunchGame() - D\n",__FILE__,__FUNCTION__);

}

bool MenuStateConnectedGame::loadFactions(const GameSettings *gameSettings, bool errorOnNoFactions){
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	bool foundFactions = false;
	vector<string> results;
    Config &config = Config::getInstance();

    vector<string> techPaths = config.getPathListForType(ptTechs);
    for(int idx = 0; idx < techPaths.size(); idx++) {
        string &techPath = techPaths[idx];

        findAll(techPath + "/" + gameSettings->getTech() + "/factions/*.", results, false, false);
        if(results.size() > 0) {
            break;
        }
    }

    if(results.size() == 0) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		NetworkManager &networkManager= NetworkManager::getInstance();
		ClientInterface* clientInterface= networkManager.getClientInterface();
		if(clientInterface->getAllowGameDataSynchCheck() == false) {
			if(errorOnNoFactions == true) {
				throw runtime_error("(2)There are no factions for the tech tree [" + gameSettings->getTech() + "]");
			}
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] (2)There are no factions for the tech tree [%s]\n",__FILE__,__FUNCTION__,__LINE__,gameSettings->getTech().c_str());
		}
		results.push_back("***missing***");
		factionFiles = results;
		for(int i=0; i<GameConstants::maxPlayers; ++i){
			listBoxFactions[i].setItems(results);
		}

		char szMsg[1024]="";
		sprintf(szMsg,"Player: %s is missing the techtree: %s",Config::getInstance().getString("NetPlayerName",Socket::getHostName().c_str()).c_str(),gameSettings->getTech().c_str());
		clientInterface->sendTextMessage(szMsg,-1);

		foundFactions = false;
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
    }
	else {
		factionFiles= results;
		for(int i= 0; i<results.size(); ++i){
			results[i]= formatString(results[i]);

			SystemFlags::OutputDebug(SystemFlags::debugSystem,"Tech [%s] has faction [%s]\n",gameSettings->getTech().c_str(),results[i].c_str());
		}
		for(int i=0; i<GameConstants::maxPlayers; ++i){
			listBoxFactions[i].setItems(results);
		}

		foundFactions = (results.size() > 0);
	}
    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	return foundFactions;
}

// ============ PRIVATE ===========================

bool MenuStateConnectedGame::hasNetworkGameSettings()
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



void MenuStateConnectedGame::reloadFactions(){

	vector<string> results;

    Config &config = Config::getInstance();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    vector<string> techPaths = config.getPathListForType(ptTechs);
    for(int idx = 0; idx < techPaths.size(); idx++) {
        string &techPath = techPaths[idx];

        findAll(techPath + "/" + techTreeFiles[listBoxTechTree.getSelectedItemIndex()] + "/factions/*.", results, false, false);
        if(results.size() > 0) {
            break;
        }
    }

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    if(results.size() == 0) {
        throw runtime_error("(2)There are no factions for the tech tree [" + techTreeFiles[listBoxTechTree.getSelectedItemIndex()] + "]");
    }
    factionFiles= results;
    for(int i= 0; i<results.size(); ++i){
        results[i]= formatString(results[i]);

        SystemFlags::OutputDebug(SystemFlags::debugSystem,"Tech [%s] has faction [%s]\n",techTreeFiles[listBoxTechTree.getSelectedItemIndex()].c_str(),results[i].c_str());
    }

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    for(int i=0; i<GameConstants::maxPlayers; ++i){
        listBoxFactions[i].setItems(results);
        listBoxFactions[i].setSelectedItemIndex(i % results.size());
    }

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}




void MenuStateConnectedGame::keyDown(char key)
{
	//send key to the chat manager
	chatManager.keyDown(key);
}

void MenuStateConnectedGame::keyPress(char c)
{
	chatManager.keyPress(c);
}

}}//end namespace
