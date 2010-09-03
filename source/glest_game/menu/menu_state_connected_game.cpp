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
	switchSetupRequestFlagType |= ssrft_NetworkPlayerName;
	updateDataSynchDetailText = false;

	currentFactionLogo = "";
	factionTexture=NULL;

	activeInputLabel = NULL;
	lastNetworkSendPing = 0;
	pingCount = 0;
	needToSetChangedGameSettings = false;
	lastSetChangedGameSettings   = time(NULL);
	showFullConsole=false;

	currentFactionName="";
	currentMap="";
	settingsReceivedFromServer=false;
	initialSettingsReceivedFromServer=false;

	returnMenuInfo=joinMenuInfo;
	Lang &lang= Lang::getInstance();
	NetworkManager &networkManager= NetworkManager::getInstance();
    Config &config = Config::getInstance();
    defaultPlayerName = config.getString("NetPlayerName",Socket::getHostName().c_str());
    enableFactionTexturePreview = config.getBool("FactionPreview","false");

    labelMapInfo.setText("?");

	vector<string> teamItems, controlItems, results;
	int setupPos=590;
	int mapHeadPos=330;
	int mapPos=mapHeadPos-30;
	int aHeadPos=260;
	int aPos=aHeadPos-30;
	int networkHeadPos=700;
	int networkPos=networkHeadPos-30;
	int xoffset=0;
	
	//state
	labelStatus.init(350, networkHeadPos+30);
	labelStatus.setText("");

	labelInfo.init(30, networkHeadPos+30);
	labelInfo.setText("");
	labelInfo.setFont(CoreData::getInstance().getMenuFontBig());
	
	//create
	buttonDisconnect.init(450, 180, 125);
	buttonPlayNow.init(525, 180, 125);


	xoffset=170;
	// fog - o - war
	// @350 ? 300 ?
	labelFogOfWar.init(xoffset+100, aHeadPos, 80);
	labelFogOfWar.setText(lang.get("FogOfWar"));
	listBoxFogOfWar.init(xoffset+100, aPos, 80);
	listBoxFogOfWar.pushBackItem(lang.get("Yes"));
	listBoxFogOfWar.pushBackItem(lang.get("No"));
	listBoxFogOfWar.setSelectedItemIndex(0);
	listBoxFogOfWar.setEditable(false);

	// Enable Observer Mode
	labelEnableObserverMode.init(xoffset+250, aHeadPos, 80);
	listBoxEnableObserverMode.init(xoffset+250, aPos, 110);
	listBoxEnableObserverMode.pushBackItem(lang.get("Yes"));
	listBoxEnableObserverMode.pushBackItem(lang.get("No"));
	listBoxEnableObserverMode.setSelectedItemIndex(0);
	listBoxEnableObserverMode.setEditable(false);
	labelEnableObserverMode.setText(lang.get("EnableObserverMode"));

	labelPathFinderType.init(xoffset+450, aHeadPos, 80);
	labelPathFinderType.setText(lang.get("PathFinderType"));
	listBoxPathFinderType.init(xoffset+450, aPos, 150);
	listBoxPathFinderType.pushBackItem(lang.get("PathFinderTypeRegular"));
	listBoxPathFinderType.pushBackItem(lang.get("PathFinderTypeRoutePlanner"));
	listBoxPathFinderType.setSelectedItemIndex(0);
	listBoxPathFinderType.setEditable(false);

	// Network Frame Period
	xoffset=0;
	labelNetworkFramePeriod.init(xoffset+170, networkHeadPos, 80);
	labelNetworkFramePeriod.setText(lang.get("NetworkFramePeriod"));
	listBoxNetworkFramePeriod.init(xoffset+170, networkPos, 80);
	listBoxNetworkFramePeriod.pushBackItem("10");
	listBoxNetworkFramePeriod.pushBackItem("20");
	listBoxNetworkFramePeriod.pushBackItem("30");
	listBoxNetworkFramePeriod.pushBackItem("40");
	listBoxNetworkFramePeriod.setSelectedItem("20");
	listBoxNetworkFramePeriod.setEditable(false);

	// Network Frame Period
	labelNetworkPauseGameForLaggedClients.init(xoffset+420, networkHeadPos, 80);
	labelNetworkPauseGameForLaggedClients.setText(lang.get("NetworkPauseGameForLaggedClients"));
	listBoxNetworkPauseGameForLaggedClients.init(xoffset+420, networkPos, 80);
	listBoxNetworkPauseGameForLaggedClients.pushBackItem(lang.get("No"));
	listBoxNetworkPauseGameForLaggedClients.pushBackItem(lang.get("Yes"));
	listBoxNetworkPauseGameForLaggedClients.setSelectedItem(lang.get("No"));
	listBoxNetworkPauseGameForLaggedClients.setEditable(false);


	// Enable Server Controlled AI
	labelEnableServerControlledAI.init(xoffset+640, networkHeadPos, 80);
	labelEnableServerControlledAI.setText(lang.get("EnableServerControlledAI"));
	listBoxEnableServerControlledAI.init(xoffset+640, networkPos, 80);
	listBoxEnableServerControlledAI.pushBackItem(lang.get("Yes"));
	listBoxEnableServerControlledAI.pushBackItem(lang.get("No"));
	listBoxEnableServerControlledAI.setSelectedItemIndex(0);
	listBoxEnableServerControlledAI.setEditable(false);

	xoffset=70;
    //map listBox
	// put them all in a set, to weed out duplicates (gbm & mgm with same name)
	// will also ensure they are alphabetically listed (rather than how the OS provides them)
	listBoxMap.init(xoffset+100, mapPos, 200);
	listBoxMap.setEditable(false);
	labelMap.init(xoffset+100, mapHeadPos);
	labelMap.setText(lang.get("Map"));

    //tileset listBox
	//listBoxTileset.init(500, 260, 150);
	listBoxTileset.init(xoffset+350, mapPos, 150);
	listBoxTileset.setEditable(false);
    //listBoxTileset.setItems(results);
	//labelTileset.init(500, 290);
	labelTileset.init(xoffset+350, mapHeadPos);
	labelTileset.setText(lang.get("Tileset"));
	

    //tech Tree listBox
	//listBoxTechTree.init(700, 260, 150);
	listBoxTechTree.setEditable(false);
    //listBoxTechTree.setItems(results);
	//labelTechTree.init(700, 290);
	listBoxTechTree.init(xoffset+550, mapPos, 150);
	labelTechTree.init(xoffset+550, mapHeadPos);
	labelTechTree.setText(lang.get("TechTree"));
	

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
	//list boxes
	xoffset=120;
	int rowHeight=27;
    for(int i=0; i<GameConstants::maxPlayers; ++i){
		labelPlayers[i].init(xoffset+50, setupPos-30-i*rowHeight);
		labelPlayers[i].setEditable(false);
		labelPlayerNames[i].init(xoffset+100,setupPos-30-i*rowHeight);

        listBoxControls[i].init(xoffset+200, setupPos-30-i*rowHeight);
        listBoxControls[i].setEditable(false);
        listBoxFactions[i].init(xoffset+350, setupPos-30-i*rowHeight);
        listBoxFactions[i].setEditable(false);
		listBoxTeams[i].init(xoffset+520, setupPos-30-i*rowHeight, 60);
		listBoxTeams[i].setEditable(false);
		labelNetStatus[i].init(xoffset+600, setupPos-30-i*rowHeight, 60);
		grabSlotButton[i].init(xoffset+600, setupPos-30-i*rowHeight, 30);
		grabSlotButton[i].setText(">");
    }

	labelControl.init(xoffset+200, setupPos, GraphicListBox::defW, GraphicListBox::defH, true);
	labelControl.setText(lang.get("Control"));
	
    labelFaction.init(xoffset+350, setupPos, GraphicListBox::defW, GraphicListBox::defH, true);
    labelFaction.setText(lang.get("Faction"));
    
    labelTeam.init(xoffset+520, setupPos, 60, GraphicListBox::defH, true);
	labelTeam.setText(lang.get("Team"));

    labelControl.setFont(CoreData::getInstance().getMenuFontBig());
	labelFaction.setFont(CoreData::getInstance().getMenuFontBig());
	labelTeam.setFont(CoreData::getInstance().getMenuFontBig());

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
		labelPlayerNames[i].setText("");

        listBoxTeams[i].setItems(teamItems);
		listBoxTeams[i].setSelectedItemIndex(i);
		listBoxControls[i].setItems(controlItems);
		labelNetStatus[i].setText("V");
    }
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
	labelMapInfo.setText(mapInfo.desc);

	//init controllers
	listBoxControls[0].setSelectedItemIndex(ctHuman);

	chatManager.init(&console, -1,true);
}

MenuStateConnectedGame::~MenuStateConnectedGame() {
	cleanupFactionTexture();
}

void MenuStateConnectedGame::mouseClick(int x, int y, MouseButton mouseButton){

	CoreData &coreData= CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();
	NetworkManager &networkManager= NetworkManager::getInstance();
	ClientInterface* clientInterface= networkManager.getClientInterface();

	if(buttonDisconnect.mouseClick(x,y)){
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		soundRenderer.playFx(coreData.getClickSoundA());
		if(clientInterface->getSocket() != NULL)
		{
		    if(clientInterface->isConnected() == true)
		    {
                string sQuitText = "chose to leave the game!";
                clientInterface->sendTextMessage(sQuitText,-1);
                sleep(1);
		    }
		    clientInterface->close();
		}
		clientInterface->reset();
		networkManager.end();
		currentFactionName="";
		currentMap="";
		returnToJoinMenu();
		return;
    }

	if (!initialSettingsReceivedFromServer) return;

	// Only allow changes after we get game settings from the server
	//if(	clientInterface->isConnected() == true &&
	//	clientInterface->getGameSettingsReceived() == true) {
	if(	clientInterface->isConnected() == true) {
		if(buttonPlayNow.mouseClick(x,y) && buttonPlayNow.getEnabled()) {
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
				if(listBoxFactions[i].getEditable()) {
					if(listBoxFactions[i].mouseClick(x, y)) {
						soundRenderer.playFx(coreData.getClickSoundA());
						ClientInterface* clientInterface= NetworkManager::getInstance().getClientInterface();
						if(clientInterface->isConnected()) {
							clientInterface->setGameSettingsReceived(false);
							clientInterface->sendSwitchSetupRequest(listBoxFactions[i].getSelectedItem(),i,-1,listBoxTeams[i].getSelectedItemIndex(),getHumanPlayerName(),switchSetupRequestFlagType);
							switchSetupRequestFlagType=ssrft_None;
						}
						break;
					}
				}
				if(listBoxTeams[i].getEditable()) {
					if(listBoxTeams[i].mouseClick(x, y)) {
						soundRenderer.playFx(coreData.getClickSoundA());
						if(clientInterface->isConnected()) {
							clientInterface->setGameSettingsReceived(false);
							clientInterface->sendSwitchSetupRequest(listBoxFactions[i].getSelectedItem(),i,-1,listBoxTeams[i].getSelectedItemIndex(),getHumanPlayerName(),switchSetupRequestFlagType);
							switchSetupRequestFlagType=ssrft_None;
						}
						break;
					}
				}
				if((listBoxControls[i].getSelectedItemIndex() == ctNetwork) &&
				   (labelNetStatus[i].getText() == GameConstants::NETWORK_SLOT_UNCONNECTED_SLOTNAME)) {
					if(grabSlotButton[i].mouseClick(x, y) )
					{
						soundRenderer.playFx(coreData.getClickSoundA());
						clientInterface->setGameSettingsReceived(false);
						settingsReceivedFromServer=false;
						SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] sending a switchSlot request from %d to %d\n",__FILE__,__FUNCTION__,__LINE__,clientInterface->getGameSettings()->getThisFactionIndex(),i);
						clientInterface->sendSwitchSetupRequest(listBoxFactions[myCurrentIndex].getSelectedItem(),myCurrentIndex,i,listBoxTeams[myCurrentIndex].getSelectedItemIndex(),labelPlayerNames[myCurrentIndex].getText(),switchSetupRequestFlagType);
						switchSetupRequestFlagType=ssrft_None;
						break;
					}
				}

				if(labelPlayerNames[i].mouseClick(x, y) && ( activeInputLabel != &labelPlayerNames[i] )){
					if(clientInterface->getGameSettings() != NULL &&
						i == clientInterface->getGameSettings()->getThisFactionIndex()) {
						setActiveInputLabel(&labelPlayerNames[i]);
					}
				}
			}
		}
	}
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void MenuStateConnectedGame::returnToJoinMenu() {
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(returnMenuInfo == jmSimple) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
		mainMenu->setState(new MenuStateJoinGame(program, mainMenu));
	}
	else {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
		mainMenu->setState(new MenuStateMasterserver(program, mainMenu));
	}
}

void MenuStateConnectedGame::mouseMove(int x, int y, const MouseState *ms){

	buttonDisconnect.mouseMove(x, y);
	buttonPlayNow.mouseMove(x, y);

	bool editingPlayerName = false;
	for(int i=0; i<GameConstants::maxPlayers; ++i){
        listBoxControls[i].mouseMove(x, y);
        listBoxFactions[i].mouseMove(x, y);
		listBoxTeams[i].mouseMove(x, y);
		grabSlotButton[i].mouseMove(x, y);

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
	listBoxTechTree.mouseMove(x, y);


}

void MenuStateConnectedGame::render() {
	try {
		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		if (!initialSettingsReceivedFromServer) return;

		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		Renderer &renderer= Renderer::getInstance();

		if(factionTexture != NULL) {
			//renderer.renderTextureQuad(60+575+80,365,200,225,factionTexture,1);
			renderer.renderTextureQuad(800,600,200,150,factionTexture,1);
		}

		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		int i;

		renderer.renderButton(&buttonDisconnect);
		//renderer.renderButton(&buttonPlayNow);

		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		for(i=0; i<GameConstants::maxPlayers; ++i){
			renderer.renderLabel(&labelPlayers[i]);
			renderer.renderListBox(&listBoxControls[i]);
			if(listBoxControls[i].getSelectedItemIndex()!=ctClosed){
				renderer.renderListBox(&listBoxFactions[i]);
				renderer.renderListBox(&listBoxTeams[i]);
				//renderer.renderLabel(&labelNetStatus[i]);
				
				if((listBoxControls[i].getSelectedItemIndex() == ctNetwork) &&
					(labelNetStatus[i].getText() == GameConstants::NETWORK_SLOT_UNCONNECTED_SLOTNAME)) {
					renderer.renderButton(&grabSlotButton[i]);
				}
				else if((listBoxControls[i].getSelectedItemIndex()==ctNetwork) ||
					(listBoxControls[i].getSelectedItemIndex()==ctHuman)){
					renderer.renderLabel(&labelNetStatus[i]);
				}

				if((listBoxControls[i].getSelectedItemIndex()==ctNetwork) ||
					(listBoxControls[i].getSelectedItemIndex()==ctHuman)){
					if(labelNetStatus[i].getText() != GameConstants::NETWORK_SLOT_UNCONNECTED_SLOTNAME) {
						renderer.renderLabel(&labelPlayerNames[i]);
					}
				}
			}
		}
		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		renderer.renderLabel(&labelStatus);
		renderer.renderLabel(&labelInfo);
		renderer.renderLabel(&labelMap);
		renderer.renderLabel(&labelFogOfWar);
		renderer.renderLabel(&labelTileset);
		renderer.renderLabel(&labelTechTree);
		renderer.renderLabel(&labelControl);
		renderer.renderLabel(&labelFaction);
		renderer.renderLabel(&labelTeam);
		//renderer.renderLabel(&labelMapInfo);

		renderer.renderListBox(&listBoxMap);
		renderer.renderListBox(&listBoxFogOfWar);
		renderer.renderListBox(&listBoxTileset);
		renderer.renderListBox(&listBoxTechTree);

		renderer.renderLabel(&labelEnableObserverMode);
		renderer.renderLabel(&labelPathFinderType);

		renderer.renderListBox(&listBoxEnableObserverMode);
		renderer.renderListBox(&listBoxPathFinderType);

		renderer.renderListBox(&listBoxEnableServerControlledAI);
		renderer.renderLabel(&labelEnableServerControlledAI);
		renderer.renderLabel(&labelNetworkFramePeriod);
		renderer.renderListBox(&listBoxNetworkFramePeriod);
		renderer.renderLabel(&labelNetworkPauseGameForLaggedClients);
		renderer.renderListBox(&listBoxNetworkPauseGameForLaggedClients);

		if(program != NULL) program->renderProgramMsgBox();

		renderer.renderChatManager(&chatManager);
		renderer.renderConsole(&console,showFullConsole,true);
	}
	catch(const std::exception &ex) {
		char szBuf[1024]="";
		sprintf(szBuf,"In [%s::%s %d] error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		throw runtime_error(szBuf);
	}
}

void MenuStateConnectedGame::update() {
	Chrono chrono;
	chrono.start();

	ClientInterface* clientInterface= NetworkManager::getInstance().getClientInterface();
	Lang &lang= Lang::getInstance();

	if(clientInterface != NULL && clientInterface->isConnected()) {
		if(difftime(time(NULL),lastNetworkSendPing) >= GameConstants::networkPingInterval) {
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] about to sendPingMessage...\n",__FILE__,__FUNCTION__,__LINE__);

			lastNetworkSendPing = time(NULL);
			clientInterface->sendPingMessage(GameConstants::networkPingInterval, time(NULL));

			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] pingCount = %d, clientInterface->getLastPingLag() = %f, GameConstants::networkPingInterval = %d\n",__FILE__,__FUNCTION__,__LINE__,pingCount, clientInterface->getLastPingLag(),GameConstants::networkPingInterval);

			// Starting checking timeout after sending at least 3 pings to server
			if(clientInterface != NULL && clientInterface->isConnected() &&
				pingCount >= 3 && clientInterface->getLastPingLag() >= (GameConstants::networkPingInterval * 3)) {
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

				string playerNameStr = getHumanPlayerName();
				clientInterface->sendTextMessage("connection timed out communicating with server.",-1);
				clientInterface->close();
			}

			pingCount++;
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		}
	}

	if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
	if(chrono.getMillis() > 0) chrono.start();

	//update status label
	if(clientInterface != NULL && clientInterface->isConnected()) {
		buttonDisconnect.setText(lang.get("Disconnect"));

		if(clientInterface->getAllowDownloadDataSynch() == false) {
		    string label = lang.get("ConnectedToServer");

            if(!clientInterface->getServerName().empty()) {
                label = label + " " + clientInterface->getServerName();
            }

            label = label + ", " + clientInterface->getVersionString();

            if(clientInterface->getAllowGameDataSynchCheck() == true &&
               clientInterface->getNetworkGameDataSynchCheckOk() == false) {
                label = label + " -synch mismatch for:";

                if(clientInterface->getNetworkGameDataSynchCheckOkMap() == false) {
                    label = label + " map";

                    if(updateDataSynchDetailText == true &&
                    	clientInterface->getReceivedDataSynchCheck() &&
                    	lastMapDataSynchError != "map CRC mismatch, " + listBoxMap.getSelectedItem()) {
                    	lastMapDataSynchError = "map CRC mismatch, " + listBoxMap.getSelectedItem();
                    	clientInterface->sendTextMessage(lastMapDataSynchError,-1,true);
                    }
                }

                if(clientInterface->getNetworkGameDataSynchCheckOkTile() == false) {
                    label = label + " tile";
                    if(updateDataSynchDetailText == true &&
                    	clientInterface->getReceivedDataSynchCheck() &&
                    	lastTileDataSynchError != "tile CRC mismatch, " + listBoxTileset.getSelectedItem()) {
                    	lastTileDataSynchError = "tile CRC mismatch, " + listBoxTileset.getSelectedItem();
                    	clientInterface->sendTextMessage(lastTileDataSynchError,-1,true);
                    }
                }

                if(clientInterface->getNetworkGameDataSynchCheckOkTech() == false) {
                    label = label + " techtree";

                    if(updateDataSynchDetailText == true &&
                    	clientInterface->getReceivedDataSynchCheck()) {

                    	string report = clientInterface->getNetworkGameDataSynchCheckTechMismatchReport();
						if(lastTechtreeDataSynchError != "techtree CRC mismatch" + report) {
							lastTechtreeDataSynchError = "techtree CRC mismatch" + report;

							SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] report: %s\n",__FILE__,__FUNCTION__,__LINE__,report.c_str());

							clientInterface->sendTextMessage("techtree CRC mismatch",-1,true);
							vector<string> reportLineTokens;
							Tokenize(report,reportLineTokens,"\n");
							for(int reportLine = 0; reportLine < reportLineTokens.size(); ++reportLine) {
								clientInterface->sendTextMessage(reportLineTokens[reportLine],-1,true);
							}
						}
                    }
                }

                if(clientInterface->getReceivedDataSynchCheck() == true) {
                	updateDataSynchDetailText = false;
                }

                //if(clientInterface->getNetworkGameDataSynchCheckOkFogOfWar() == false)
                //{
                //    label = label + " FogOfWar == false";
                //}
            }
            else if(clientInterface->getAllowGameDataSynchCheck() == true) {
                label += " - data synch is ok";
            }

            //std::string networkFrameString = lang.get("NetworkFramePeriod") + " " + intToStr(clientInterface->getGameSettings()->getNetworkFramePeriod());
			//float pingTime = clientInterface->getThreadedPingMS(clientInterface->getServerIpAddress().c_str());
			//char szBuf[1024]="";
			//sprintf(szBuf,"%s, ping = %.2fms, %s",label.c_str(),pingTime,networkFrameString.c_str());
			//sprintf(szBuf,"%s, %s",label.c_str(),networkFrameString.c_str());


            //labelStatus.setText(szBuf);
            labelStatus.setText(label);
		}
		else {
		    string label = lang.get("ConnectedToServer");

            if(!clientInterface->getServerName().empty()) {
                label = label + " " + clientInterface->getServerName();
            }

            if(clientInterface->getAllowGameDataSynchCheck() == true &&
               clientInterface->getNetworkGameDataSynchCheckOk() == false)
            {
                label = label + " -waiting to synch:";
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

            //std::string networkFrameString = lang.get("NetworkFramePeriod") + " " + intToStr(clientInterface->getGameSettings()->getNetworkFramePeriod());
			//float pingTime = clientInterface->getThreadedPingMS(clientInterface->getServerIpAddress().c_str());
			//char szBuf[1024]="";
			//sprintf(szBuf,"%s, ping = %.2fms, %s",label.c_str(),pingTime,networkFrameString.c_str());
			//sprintf(szBuf,"%s, %s",label.c_str(),networkFrameString.c_str());

            //labelStatus.setText(szBuf);
            labelStatus.setText(label);
		}

		if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
		if(chrono.getMillis() > 0) chrono.start();
	}
	else {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		if(clientInterface != NULL && clientInterface->isConnected() == true) {
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		    clientInterface->close();
		}

		if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
		if(chrono.getMillis() > 0) chrono.start();

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		returnToJoinMenu();
		return;
	}

	if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
	if(chrono.getMillis() > 0) chrono.start();

	//process network messages
	if(clientInterface != NULL && clientInterface->isConnected()) {
		
		if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
		if(chrono.getMillis() > 0) chrono.start();

		bool mustSwitchPlayerName = false;
		if(clientInterface->getGameSettingsReceived()) {
			updateDataSynchDetailText = true;
			bool errorOnMissingData = (clientInterface->getAllowGameDataSynchCheck() == false);
			//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
			vector<string> maps,tilesets,techtree;
			const GameSettings *gameSettings = clientInterface->getGameSettings();
		
			if(gameSettings == NULL) {
				throw runtime_error("gameSettings == NULL");
			}
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
			
			//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] hasFactions = %d, currentFactionName [%s]\n",__FILE__,__FUNCTION__,__LINE__,hasFactions,currentFactionName.c_str());

			// map
			maps.push_back(formatString(gameSettings->getMap()));
			listBoxMap.setItems(maps);
			if(currentMap != gameSettings->getMap())
			{// load the setup again
				currentMap = gameSettings->getMap();
			}
			
			//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			// FogOfWar
			if(gameSettings->getFogOfWar()){
				listBoxFogOfWar.setSelectedItemIndex(0); 
			}
			else
			{
				listBoxFogOfWar.setSelectedItemIndex(1);
			}
			
			if(gameSettings->getEnableObserverModeAtEndGame()) {
				listBoxEnableObserverMode.setSelectedItemIndex(0);
			}
			else {
				listBoxEnableObserverMode.setSelectedItemIndex(1);
			}
			if(gameSettings->getEnableServerControlledAI()) {
					listBoxEnableServerControlledAI.setSelectedItemIndex(0);
			}
			else {
				listBoxEnableServerControlledAI.setSelectedItemIndex(1);
			}
			if(gameSettings->getNetworkPauseGameForLaggedClients()) {
				listBoxNetworkPauseGameForLaggedClients.setSelectedItemIndex(1);
			}
			else {
				listBoxNetworkPauseGameForLaggedClients.setSelectedItemIndex(0);
			}
			if(gameSettings->getPathFinderType() == pfBasic) {
				listBoxPathFinderType.setSelectedItemIndex(0);
			}
			else {
				listBoxPathFinderType.setSelectedItemIndex(1);
			}

			listBoxNetworkFramePeriod.setSelectedItem(intToStr(gameSettings->getNetworkFramePeriod()),false);

			//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			// Control
			for(int i=0; i<GameConstants::maxPlayers; ++i){
				listBoxControls[i].setSelectedItemIndex(ctClosed);
				listBoxFactions[i].setEditable(false);
				listBoxTeams[i].setEditable(false);
			}
			
			//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			if(hasFactions == true) {
				//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] errorOnMissingData = %d\n",__FILE__,__FUNCTION__,__LINE__,errorOnMissingData);

				for(int i=0; i<gameSettings->getFactionCount(); ++i){
					int slot=gameSettings->getStartLocationIndex(i);
					listBoxControls[slot].setSelectedItemIndex(gameSettings->getFactionControl(i),errorOnMissingData);
					listBoxTeams[slot].setSelectedItemIndex(gameSettings->getTeam(i),errorOnMissingData);
					//listBoxFactions[slot].setSelectedItem(formatString(gameSettings->getFactionTypeName(i)),errorOnMissingData);
					listBoxFactions[slot].setSelectedItem(formatString(gameSettings->getFactionTypeName(i)),false);

					if(gameSettings->getFactionControl(i) == ctNetwork ){
						labelNetStatus[slot].setText(gameSettings->getNetworkPlayerName(i));
						if(gameSettings->getThisFactionIndex() != i &&
								gameSettings->getNetworkPlayerName(i) != "" &&
								gameSettings->getNetworkPlayerName(i) != GameConstants::NETWORK_SLOT_UNCONNECTED_SLOTNAME) {
							labelPlayerNames[slot].setText(gameSettings->getNetworkPlayerName(i));
						}
					}
					
					if(gameSettings->getFactionControl(i) == ctNetwork &&
						gameSettings->getThisFactionIndex() == i){
						// set my current slot to ctHuman
						listBoxControls[slot].setSelectedItemIndex(ctHuman);
						listBoxFactions[slot].setEditable(true);
						listBoxTeams[slot].setEditable(true);

						if(labelPlayerNames[slot].getText() == "" &&
							gameSettings->getNetworkPlayerName(i) != "" &&
							gameSettings->getNetworkPlayerName(i) != GameConstants::NETWORK_SLOT_UNCONNECTED_SLOTNAME) {
							labelPlayerNames[slot].setText(gameSettings->getNetworkPlayerName(i));
						}
					}
					
					settingsReceivedFromServer=true;
					initialSettingsReceivedFromServer=true;

		            needToSetChangedGameSettings = true;
		            lastSetChangedGameSettings   = time(NULL);

		            mustSwitchPlayerName = true;
				}
			}

			if(enableFactionTexturePreview == true) {
				if( clientInterface != NULL && clientInterface->isConnected() &&
					gameSettings != NULL) {

					string factionLogo = Game::findFactionLogoFile(gameSettings, NULL,"preview_screen.*");
					if(factionLogo == "") {
						factionLogo = Game::findFactionLogoFile(gameSettings, NULL);
					}
					if(currentFactionLogo != factionLogo) {
						currentFactionLogo = factionLogo;
						loadFactionTexture(currentFactionLogo);
					}
				}
			}

			if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
			if(chrono.getMillis() > 0) chrono.start();
		}

		//update lobby
		clientInterface= NetworkManager::getInstance().getClientInterface();
		if(clientInterface != NULL && clientInterface->isConnected()) {
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] clientInterface = %p\n",__FILE__,__FUNCTION__,__LINE__,clientInterface);
			clientInterface->updateLobby();

			if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
			if(chrono.getMillis() > 0) chrono.start();
		}

		if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
		if(chrono.getMillis() > 0) chrono.start();

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		clientInterface= NetworkManager::getInstance().getClientInterface();
		if(clientInterface != NULL && clientInterface->isConnected()) {
			if(clientInterface->getIntroDone() == true &&
				(switchSetupRequestFlagType & ssrft_NetworkPlayerName) == ssrft_NetworkPlayerName) {
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
				//needToSetChangedGameSettings = false;
				//lastSetChangedGameSettings = time(NULL);
				clientInterface->sendSwitchSetupRequest("",clientInterface->getPlayerIndex(),-1,-1,getHumanPlayerName(),switchSetupRequestFlagType);
				switchSetupRequestFlagType=ssrft_None;
			}

			if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
			if(chrono.getMillis() > 0) chrono.start();

			//call the chat manager
			chatManager.updateNetwork();

			//console732

			console.update();

			//intro
			if(clientInterface->getIntroDone()) {
				labelInfo.setText(lang.get("WaitingHost"));
				//servers.setString(clientInterface->getServerName(), Ip(labelServerIp.getText()).getString());
			}

			//launch
			if(clientInterface->getLaunchGame()) {
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

				//servers.save(serversSavedFile);

				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

				assert(clientInterface != NULL);

				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

				program->setState(new Game(program, clientInterface->getGameSettings()));
				return;

				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			}
		}

		if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
		if(chrono.getMillis() > 0) chrono.start();

		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}

    //if(clientInterface != NULL && clientInterface->getLaunchGame()) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] clientInterface->getLaunchGame() - D\n",__FILE__,__FUNCTION__);
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
		sprintf(szMsg,"Player: %s is missing the techtree: %s",getHumanPlayerName().c_str(),gameSettings->getTech().c_str());
		clientInterface->sendTextMessage(szMsg,-1, true);

		foundFactions = false;
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
    }
	else {
	    // Add special Observer Faction
	    Lang &lang= Lang::getInstance();
	    results.push_back(formatString(lang.get("ObserverOnly")));

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

void MenuStateConnectedGame::reloadFactions() {

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




void MenuStateConnectedGame::keyDown(char key) {
	if(activeInputLabel!=NULL) {
		if(key==vkBack){
			string text= activeInputLabel->getText();
			if(text.size()>1){
				text.erase(text.end()-2);
			}
			activeInputLabel->setText(text);

			switchSetupRequestFlagType |= ssrft_NetworkPlayerName;
            needToSetChangedGameSettings = true;
            lastSetChangedGameSettings   = time(NULL);
		}
	}
	else {
		//send key to the chat manager
		chatManager.keyDown(key);
		if(!chatManager.getEditEnabled()){
			Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));

			if(key == configKeys.getCharKey("ShowFullConsole")) {
				showFullConsole= true;
			}
		}
	}
}

void MenuStateConnectedGame::keyPress(char c) {
	if(activeInputLabel!=NULL) {
		int maxTextSize= 16;
	    for(int i=0; i<GameConstants::maxPlayers; ++i){
			if(&labelPlayerNames[i] == activeInputLabel){
				if((c>='0' && c<='9')||(c>='a' && c<='z')||(c>='A' && c<='Z')||
					(c=='-')||(c=='(')||(c==')')){
					if(activeInputLabel->getText().size()<maxTextSize) {
						string text= activeInputLabel->getText();
						text.insert(text.end()-1, c);
						activeInputLabel->setText(text);

						switchSetupRequestFlagType |= ssrft_NetworkPlayerName;
			            needToSetChangedGameSettings = true;
			            lastSetChangedGameSettings   = time(NULL);
					}
				}
			}
	    }
	}
	else {
		chatManager.keyPress(c);
	}
}

void MenuStateConnectedGame::keyUp(char key) {
	if(activeInputLabel==NULL) {
		chatManager.keyUp(key);

		Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));

		if(chatManager.getEditEnabled()){
			//send key to the chat manager
			chatManager.keyUp(key);
		}
		else if(key== configKeys.getCharKey("ShowFullConsole")) {
			showFullConsole= false;
		}
	}
}

void MenuStateConnectedGame::setActiveInputLabel(GraphicLabel *newLable) {
	if(newLable!=NULL) {
		string text= newLable->getText();
		size_t found;
		found=text.find_last_of("_");
		if (found==string::npos) {
			text=text+"_";
		}
		newLable->setText(text);
	}
	if(activeInputLabel!=NULL && !activeInputLabel->getText().empty()) {
		string text= activeInputLabel->getText();
		size_t found;
		found=text.find_last_of("_");
		if (found!=string::npos) {
			text=text.substr(0,found);
		}
		activeInputLabel->setText(text);
	}
	activeInputLabel=newLable;
}

string MenuStateConnectedGame::getHumanPlayerName() {
	string result = defaultPlayerName;

	NetworkManager &networkManager= NetworkManager::getInstance();
	ClientInterface* clientInterface= networkManager.getClientInterface();
	for(int j=0; j<GameConstants::maxPlayers; ++j) {
		if(	clientInterface != NULL &&
			clientInterface->getGameSettings() != NULL &&
			j == clientInterface->getGameSettings()->getThisFactionIndex() &&
			labelPlayerNames[j].getText() != "") {
			result = labelPlayerNames[j].getText();

			if(activeInputLabel != NULL) {
				size_t found = result.find_last_of("_");
				if (found != string::npos) {
					result = result.substr(0,found);
				}
			}

			break;
		}
	}

	return result;
}

void MenuStateConnectedGame::loadFactionTexture(string filepath) {

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	cleanupFactionTexture();

	if(enableFactionTexturePreview == true) {
		if(filepath == "") {
			factionTexture=NULL;
		}
		else {
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] filepath = [%s]\n",__FILE__,__FUNCTION__,__LINE__,filepath.c_str());

			factionTexture = GraphicsInterface::getInstance().getFactory()->newTexture2D();
			//loadingTexture = renderer.newTexture2D(rsGlobal);
			factionTexture->setMipmap(true);
			//loadingTexture->getPixmap()->load(filepath);
			factionTexture->load(filepath);

			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			Renderer &renderer= Renderer::getInstance();
			renderer.initTexture(rsGlobal,factionTexture);

			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		}
	}
}

void MenuStateConnectedGame::cleanupFactionTexture() {
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(factionTexture!=NULL) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		factionTexture->end();
		delete factionTexture;

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		//delete loadingTexture;
		factionTexture=NULL;
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

}}//end namespace
