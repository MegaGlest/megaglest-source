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
#include "cache_manager.h"
#include "leak_dumper.h"
#include "map_preview.h"


namespace Glest{ namespace Game{

static const string ITEM_MISSING = "***missing***";

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
	MenuState(program, mainMenu, "connected-game")
{
	containerName = "ClientConnectedGame";
	switchSetupRequestFlagType |= ssrft_NetworkPlayerName;
	updateDataSynchDetailText = false;

	currentTechName_factionPreview="";
	currentFactionName_factionPreview="";

	ftpClientThread                             = NULL;
    ftpMissingDataType                          = ftpmsg_MissingNone;
	getMissingMapFromFTPServer                  = "";
	getMissingMapFromFTPServerInProgress        = false;
	getMissingTilesetFromFTPServer              = "";
	getMissingTilesetFromFTPServerInProgress    = false;
    getMissingTechtreeFromFTPServer				= "";
    getMissingTechtreeFromFTPServerInProgress	= false;
    lastCheckedCRCTilesetName					= "";
    lastCheckedCRCTechtreeName					= "";
    lastCheckedCRCMapName						= "";
    lastCheckedCRCTilesetValue					= -1;
    lastCheckedCRCTechtreeValue					= -1;
    lastCheckedCRCMapValue						= -1;

	currentFactionLogo = "";
	factionTexture=NULL;
	lastMissingMap="";
	lastMissingTechtree ="";
	lastMissingTileSet = "";

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

    mainMessageBox.registerGraphicComponent(containerName,"mainMessageBox");
	mainMessageBox.init(lang.get("Ok"));
	mainMessageBox.setEnabled(false);
	mainMessageBoxState=0;

    ftpMessageBox.registerGraphicComponent(containerName,"ftpMessageBox");
	ftpMessageBox.init(lang.get("Yes"),lang.get("No"));
	ftpMessageBox.setEnabled(false);

	NetworkManager &networkManager= NetworkManager::getInstance();
    Config &config = Config::getInstance();
    defaultPlayerName = config.getString("NetPlayerName",Socket::getHostName().c_str());
    enableFactionTexturePreview = config.getBool("FactionPreview","true");
    enableMapPreview = config.getBool("MapPreview","true");

	vector<string> teamItems, controlItems, results, rMultiplier, playerStatuses;
	int setupPos=590;
	int mapHeadPos=330;
	int mapPos=mapHeadPos-30;
	int aHeadPos=245;
	int aPos=aHeadPos-30;
	int networkHeadPos=700;
	int networkPos=networkHeadPos-30;
	int xoffset=0;

	//state
	labelStatus.registerGraphicComponent(containerName,"labelStatus");
	labelStatus.init(350, networkHeadPos+30);
	labelStatus.setText("");

	labelInfo.registerGraphicComponent(containerName,"labelInfo");
	labelInfo.init(30, networkHeadPos+30);
	labelInfo.setText("");
	labelInfo.setFont(CoreData::getInstance().getMenuFontBig());

    timerLabelFlash = time(NULL);
    labelDataSynchInfo.registerGraphicComponent(containerName,"labelDataSynchInfo");
	labelDataSynchInfo.init(30, networkHeadPos-60);
	labelDataSynchInfo.setText("");
	labelDataSynchInfo.setFont(CoreData::getInstance().getMenuFontBig());

	//create
	buttonDisconnect.registerGraphicComponent(containerName,"buttonDisconnect");
	buttonDisconnect.init(350, 180, 125);




	xoffset=170;
	// fog - o - war
	// @350 ? 300 ?

	labelFogOfWar.registerGraphicComponent(containerName,"labelFogOfWar");
	labelFogOfWar.init(xoffset, aHeadPos, 130);
	labelFogOfWar.setText(lang.get("FogOfWar"));

	listBoxFogOfWar.registerGraphicComponent(containerName,"listBoxFogOfWar");
	listBoxFogOfWar.init(xoffset, aPos, 130);
	listBoxFogOfWar.pushBackItem(lang.get("Enabled"));
	listBoxFogOfWar.pushBackItem(lang.get("Explored"));
	listBoxFogOfWar.pushBackItem(lang.get("Disabled"));
	listBoxFogOfWar.setSelectedItemIndex(0);
	listBoxFogOfWar.setEditable(false);


	labelAllowObservers.registerGraphicComponent(containerName,"labelAllowObservers");
	labelAllowObservers.init(xoffset+150, aHeadPos, 80);
	labelAllowObservers.setText(lang.get("AllowObservers"));

	listBoxAllowObservers.registerGraphicComponent(containerName,"listBoxAllowObservers");
	listBoxAllowObservers.init(xoffset+150, aPos, 80);
	listBoxAllowObservers.pushBackItem(lang.get("No"));
	listBoxAllowObservers.pushBackItem(lang.get("Yes"));
	listBoxAllowObservers.setSelectedItemIndex(0);
	listBoxAllowObservers.setEditable(false);


	// Enable Observer Mode
	labelEnableObserverMode.registerGraphicComponent(containerName,"labelEnableObserverMode");
	labelEnableObserverMode.init(xoffset+250, aHeadPos, 80);

	listBoxEnableObserverMode.registerGraphicComponent(containerName,"listBoxEnableObserverMode");
	listBoxEnableObserverMode.init(xoffset+250, aPos, 110);
	listBoxEnableObserverMode.pushBackItem(lang.get("Yes"));
	listBoxEnableObserverMode.pushBackItem(lang.get("No"));
	listBoxEnableObserverMode.setSelectedItemIndex(0);
	listBoxEnableObserverMode.setEditable(false);
	labelEnableObserverMode.setText(lang.get("EnableObserverMode"));

	labelPathFinderType.registerGraphicComponent(containerName,"labelPathFinderType");
	labelPathFinderType.init(xoffset+450, aHeadPos, 80);
	labelPathFinderType.setText(lang.get("PathFinderType"));

	listBoxPathFinderType.registerGraphicComponent(containerName,"listBoxPathFinderType");
	listBoxPathFinderType.init(xoffset+450, aPos, 150);
	listBoxPathFinderType.pushBackItem(lang.get("PathFinderTypeRegular"));
	if(config.getBool("EnableRoutePlannerPathfinder","false") == true) {
		listBoxPathFinderType.pushBackItem(lang.get("PathFinderTypeRoutePlanner"));
	}

	listBoxPathFinderType.setSelectedItemIndex(0);
	listBoxPathFinderType.setEditable(false);

	// Network Frame Period
	xoffset=0;
	labelNetworkFramePeriod.registerGraphicComponent(containerName,"labelNetworkFramePeriod");
	labelNetworkFramePeriod.init(xoffset+170, networkHeadPos, 80);
	labelNetworkFramePeriod.setText(lang.get("NetworkFramePeriod"));

	listBoxNetworkFramePeriod.registerGraphicComponent(containerName,"listBoxNetworkFramePeriod");
	listBoxNetworkFramePeriod.init(xoffset+170, networkPos, 80);
	listBoxNetworkFramePeriod.pushBackItem("10");
	listBoxNetworkFramePeriod.pushBackItem("20");
	listBoxNetworkFramePeriod.pushBackItem("30");
	listBoxNetworkFramePeriod.pushBackItem("40");
	listBoxNetworkFramePeriod.setSelectedItem("20");
	listBoxNetworkFramePeriod.setEditable(false);

	// Network Frame Period
	labelNetworkPauseGameForLaggedClients.registerGraphicComponent(containerName,"labelNetworkPauseGameForLaggedClients");
	labelNetworkPauseGameForLaggedClients.init(xoffset+420, networkHeadPos, 80);
	labelNetworkPauseGameForLaggedClients.setText(lang.get("NetworkPauseGameForLaggedClients"));

	listBoxNetworkPauseGameForLaggedClients.registerGraphicComponent(containerName,"listBoxNetworkPauseGameForLaggedClients");
	listBoxNetworkPauseGameForLaggedClients.init(xoffset+420, networkPos, 80);
	listBoxNetworkPauseGameForLaggedClients.pushBackItem(lang.get("No"));
	listBoxNetworkPauseGameForLaggedClients.pushBackItem(lang.get("Yes"));
	listBoxNetworkPauseGameForLaggedClients.setSelectedItem(lang.get("No"));
	listBoxNetworkPauseGameForLaggedClients.setEditable(false);


	// Enable Server Controlled AI
	labelEnableServerControlledAI.registerGraphicComponent(containerName,"labelEnableServerControlledAI");
	labelEnableServerControlledAI.init(xoffset+640, networkHeadPos, 80);
	labelEnableServerControlledAI.setText(lang.get("EnableServerControlledAI"));

	listBoxEnableServerControlledAI.registerGraphicComponent(containerName,"listBoxEnableServerControlledAI");
	listBoxEnableServerControlledAI.init(xoffset+640, networkPos, 80);
	listBoxEnableServerControlledAI.pushBackItem(lang.get("Yes"));
	listBoxEnableServerControlledAI.pushBackItem(lang.get("No"));
	listBoxEnableServerControlledAI.setSelectedItemIndex(0);
	listBoxEnableServerControlledAI.setEditable(false);

	xoffset=70;
	int labelOffset=23;
    //map listBox
	// put them all in a set, to weed out duplicates (gbm & mgm with same name)
	// will also ensure they are alphabetically listed (rather than how the OS provides them)
	listBoxMap.registerGraphicComponent(containerName,"listBoxMap");
	listBoxMap.init(xoffset+100, mapPos, 200);
	listBoxMap.setEditable(false);

    labelMapInfo.registerGraphicComponent(containerName,"labelMapInfo");
	labelMapInfo.init(xoffset+100, mapPos-labelOffset, 200, 40);
    labelMapInfo.setText("?");

	labelMap.registerGraphicComponent(containerName,"labelMap");
	labelMap.init(xoffset+100, mapHeadPos);
	labelMap.setText(lang.get("Map"));

    //tileset listBox
	//listBoxTileset.init(500, 260, 150);
	listBoxTileset.registerGraphicComponent(containerName,"listBoxTileset");
	listBoxTileset.init(xoffset+350, mapPos, 150);
	listBoxTileset.setEditable(false);
    //listBoxTileset.setItems(results);
	//labelTileset.init(500, 290);

	labelTileset.registerGraphicComponent(containerName,"labelTileset");
	labelTileset.init(xoffset+350, mapHeadPos);
	labelTileset.setText(lang.get("Tileset"));


    //tech Tree listBox
	//listBoxTechTree.init(700, 260, 150);
	listBoxTechTree.setEditable(false);
    //listBoxTechTree.setItems(results);
	//labelTechTree.init(700, 290);

	listBoxTechTree.registerGraphicComponent(containerName,"listBoxTechTree");
	listBoxTechTree.init(xoffset+550, mapPos, 150);

	labelTechTree.registerGraphicComponent(containerName,"labelTechTree");
	labelTechTree.init(xoffset+550, mapHeadPos);
	labelTechTree.setText(lang.get("TechTree"));


	listBoxPlayerStatus.registerGraphicComponent(containerName,"listBoxPlayerStatus");
	//listBoxPlayerStatus.init(10, 600, 150);
	listBoxPlayerStatus.init(525, 180, 125);
	listBoxPlayerStatus.setTextColor(Vec3f(1.0f,0.f,0.f));
	listBoxPlayerStatus.setLighted(true);
	playerStatuses.push_back(lang.get("PlayerStatusSetup"));
	playerStatuses.push_back(lang.get("PlayerStatusBeRightBack"));
	playerStatuses.push_back(lang.get("PlayerStatusReady"));
	listBoxPlayerStatus.setItems(playerStatuses);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
	//list boxes
	xoffset=100;
	int rowHeight=27;
    for(int i=0; i<GameConstants::maxPlayers; ++i){
		labelPlayerStatus[i].registerGraphicComponent(containerName,"labelPlayerStatus" + intToStr(i));
		labelPlayerStatus[i].init(10, setupPos-30-i*rowHeight, 60);

    	labelPlayers[i].registerGraphicComponent(containerName,"labelPlayers" + intToStr(i));
		labelPlayers[i].init(xoffset+0, setupPos-30-i*rowHeight);
		labelPlayers[i].setEditable(false);

		labelPlayerNames[i].registerGraphicComponent(containerName,"labelPlayerNames" + intToStr(i));
		labelPlayerNames[i].init(xoffset+50,setupPos-30-i*rowHeight);

		listBoxControls[i].registerGraphicComponent(containerName,"listBoxControls" + intToStr(i));
        listBoxControls[i].init(xoffset+210, setupPos-30-i*rowHeight);
        listBoxControls[i].setEditable(false);

        listBoxRMultiplier[i].registerGraphicComponent(containerName,"listBoxRMultiplier" + intToStr(i));
        listBoxRMultiplier[i].init(xoffset+350, setupPos-30-i*rowHeight,70);
		listBoxRMultiplier[i].setEditable(false);

        listBoxFactions[i].registerGraphicComponent(containerName,"listBoxFactions" + intToStr(i));
        listBoxFactions[i].init(xoffset+430, setupPos-30-i*rowHeight);
        listBoxFactions[i].setEditable(false);

        listBoxTeams[i].registerGraphicComponent(containerName,"listBoxTeams" + intToStr(i));
		listBoxTeams[i].init(xoffset+590, setupPos-30-i*rowHeight, 60);
		listBoxTeams[i].setEditable(false);

		labelNetStatus[i].registerGraphicComponent(containerName,"labelNetStatus" + intToStr(i));
		labelNetStatus[i].init(xoffset+670, setupPos-30-i*rowHeight, 60);

		grabSlotButton[i].registerGraphicComponent(containerName,"grabSlotButton" + intToStr(i));
		grabSlotButton[i].init(xoffset+670, setupPos-30-i*rowHeight, 30);
		grabSlotButton[i].setText(">");
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
    labelTeam.init(xoffset+590, setupPos, 60, GraphicListBox::defH, true);
	labelTeam.setText(lang.get("Team"));

    labelControl.setFont(CoreData::getInstance().getMenuFontBig());
	labelFaction.setFont(CoreData::getInstance().getMenuFontBig());
	labelTeam.setFont(CoreData::getInstance().getMenuFontBig());

	//texts
	buttonDisconnect.setText(lang.get("Return"));

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

	for(int i=0; i<GameConstants::maxPlayers; ++i){
		labelPlayerStatus[i].setText("");

		labelPlayers[i].setText(lang.get("Player")+" "+intToStr(i));
		labelPlayerNames[i].setText("");

        listBoxTeams[i].setItems(teamItems);
		listBoxTeams[i].setSelectedItemIndex(i);
		listBoxControls[i].setItems(controlItems);
		listBoxRMultiplier[i].setItems(rMultiplier);
		listBoxRMultiplier[i].setSelectedItemIndex(5);

		labelNetStatus[i].setText("V");
    }
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//init controllers
	listBoxControls[0].setSelectedItemIndex(ctHuman);

	chatManager.init(&console, -1,true);

	GraphicComponent::applyAllCustomProperties(containerName);

	//tileset listBox
    findDirs(config.getPathListForType(ptTilesets), tilesetFiles);

    findDirs(config.getPathListForType(ptTechs), techTreeFiles);


    if(config.getBool("EnableFTPXfer","true") == true) {
        ClientInterface *clientInterface = networkManager.getClientInterface();
        string serverUrl = clientInterface->getServerIpAddress();
        //int portNumber   = config.getInt("FTPServerPort",intToStr(ServerSocket::getFTPServerPort()).c_str());
        int portNumber   = clientInterface->getServerFTPPort();

        if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] Using FTP port #: %d\n",__FILE__,__FUNCTION__,__LINE__,portNumber);

        vector<string> mapPathList = config.getPathListForType(ptMaps);
        std::pair<string,string> mapsPath;
        if(mapPathList.size() > 0) {
            mapsPath.first = mapPathList[0];
        }
        if(mapPathList.size() > 1) {
            mapsPath.second = mapPathList[1];
        }
        std::pair<string,string> tilesetsPath;
        vector<string> tilesetsList = Config::getInstance().getPathListForType(ptTilesets);
        if(tilesetsList.size() > 0) {
            tilesetsPath.first = tilesetsList[0];
            if(tilesetsList.size() > 1) {
                tilesetsPath.second = tilesetsList[1];
            }
        }

        std::pair<string,string> techtreesPath;
        vector<string> techtreesList = Config::getInstance().getPathListForType(ptTechs);
        if(techtreesList.size() > 0) {
        	techtreesPath.first = techtreesList[0];
            if(techtreesList.size() > 1) {
            	techtreesPath.second = techtreesList[1];
            }
        }

        std::pair<string,string> scenariosPath;
        vector<string> scenariosList = Config::getInstance().getPathListForType(ptScenarios);
        if(scenariosList.size() > 0) {
        	scenariosPath.first = scenariosList[0];
            if(scenariosList.size() > 1) {
            	scenariosPath.second = scenariosList[1];
            }
        }

        string fileArchiveExtension = config.getString("FileArchiveExtension","");
        string fileArchiveExtractCommand = config.getString("FileArchiveExtractCommand","");
        string fileArchiveExtractCommandParameters = config.getString("FileArchiveExtractCommandParameters","");
        int32 fileArchiveExtractCommandSuccessResult = config.getInt("FileArchiveExtractCommandSuccessResult","0");

        ftpClientThread = new FTPClientThread(portNumber,serverUrl,
        		mapsPath,tilesetsPath,techtreesPath,scenariosPath,
        		this,fileArchiveExtension,fileArchiveExtractCommand,
        		fileArchiveExtractCommandParameters,fileArchiveExtractCommandSuccessResult);
        ftpClientThread->start();
    }

    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

MenuStateConnectedGame::~MenuStateConnectedGame() {
	if(ftpClientThread != NULL) {
	    ftpClientThread->setCallBackObject(NULL);
	    if(ftpClientThread->shutdownAndWait() == true) {
            delete ftpClientThread;
            ftpClientThread = NULL;
	    }
	}
}

void MenuStateConnectedGame::mouseClick(int x, int y, MouseButton mouseButton){

	CoreData &coreData= CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();
	NetworkManager &networkManager= NetworkManager::getInstance();
	ClientInterface* clientInterface= networkManager.getClientInterface();
	Lang &lang= Lang::getInstance();

	if(mainMessageBox.getEnabled()) {
		int button= 1;
		if(mainMessageBox.mouseClick(x, y, button)) {
			soundRenderer.playFx(coreData.getClickSoundA());
			if(button == 1) {
				mainMessageBox.setEnabled(false);
			}
		}
	}
	else if(ftpMessageBox.getEnabled()) {
		int button= 1;
		if(ftpMessageBox.mouseClick(x, y, button)) {
			soundRenderer.playFx(coreData.getClickSoundA());
			ftpMessageBox.setEnabled(false);
			if(button == 1) {
			    if(ftpMissingDataType == ftpmsg_MissingMap) {
                    getMissingMapFromFTPServerInProgress = true;

    		    	Lang &lang= Lang::getInstance();
    		    	const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
    		    	for(unsigned int i = 0; i < languageList.size(); ++i) {
						char szMsg[1024]="";
						if(lang.hasString("DataMissingMapNowDownloading",languageList[i]) == true) {
							sprintf(szMsg,lang.get("DataMissingMapNowDownloading",languageList[i]).c_str(),getHumanPlayerName().c_str(),getMissingMapFromFTPServer.c_str());
						}
						else {
							sprintf(szMsg,"Player: %s is attempting to download the map: %s",getHumanPlayerName().c_str(),getMissingMapFromFTPServer.c_str());
						}
						bool localEcho = lang.isLanguageLocal(languageList[i]);
						clientInterface->sendTextMessage(szMsg,-1, localEcho,languageList[i]);
    		    	}

                    if(ftpClientThread != NULL) {
                        ftpClientThread->addMapToRequests(getMissingMapFromFTPServer);
                        MutexSafeWrapper safeMutexFTPProgress((ftpClientThread != NULL ? ftpClientThread->getProgressMutex() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
                        fileFTPProgressList[getMissingMapFromFTPServer] = pair<int,string>(0,"");
                        safeMutexFTPProgress.ReleaseLock();
                    }
			    }
			    else if(ftpMissingDataType == ftpmsg_MissingTileset) {
                    getMissingTilesetFromFTPServerInProgress = true;

    		    	Lang &lang= Lang::getInstance();
    		    	const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
    		    	for(unsigned int i = 0; i < languageList.size(); ++i) {
						char szMsg[1024]="";
						if(lang.hasString("DataMissingTilesetNowDownloading",languageList[i]) == true) {
							sprintf(szMsg,lang.get("DataMissingTilesetNowDownloading",languageList[i]).c_str(),getHumanPlayerName().c_str(),getMissingTilesetFromFTPServer.c_str());
						}
						else {
							sprintf(szMsg,"Player: %s is attempting to download the tileset: %s",getHumanPlayerName().c_str(),getMissingTilesetFromFTPServer.c_str());
						}
						bool localEcho = lang.isLanguageLocal(languageList[i]);
						clientInterface->sendTextMessage(szMsg,-1, localEcho,languageList[i]);
    		    	}

                    if(ftpClientThread != NULL) {
                        ftpClientThread->addTilesetToRequests(getMissingTilesetFromFTPServer);
                        MutexSafeWrapper safeMutexFTPProgress((ftpClientThread != NULL ? ftpClientThread->getProgressMutex() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
                        fileFTPProgressList[getMissingTilesetFromFTPServer] = pair<int,string>(0,"");
                        safeMutexFTPProgress.ReleaseLock();
                    }
			    }
			    else if(ftpMissingDataType == ftpmsg_MissingTechtree) {
                    getMissingTechtreeFromFTPServerInProgress = true;

    		    	Lang &lang= Lang::getInstance();
    		    	const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
    		    	for(unsigned int i = 0; i < languageList.size(); ++i) {
						char szMsg[1024]="";
						if(lang.hasString("DataMissingTechtreeNowDownloading",languageList[i]) == true) {
							sprintf(szMsg,lang.get("DataMissingTechtreeNowDownloading",languageList[i]).c_str(),getHumanPlayerName().c_str(),getMissingTechtreeFromFTPServer.c_str());
						}
						else {
							sprintf(szMsg,"Player: %s is attempting to download the techtree: %s",getHumanPlayerName().c_str(),getMissingTechtreeFromFTPServer.c_str());
						}
						bool localEcho = lang.isLanguageLocal(languageList[i]);
						clientInterface->sendTextMessage(szMsg,-1, localEcho,languageList[i]);
    		    	}

                    if(ftpClientThread != NULL) {
                        ftpClientThread->addTechtreeToRequests(getMissingTechtreeFromFTPServer);
                        MutexSafeWrapper safeMutexFTPProgress((ftpClientThread != NULL ? ftpClientThread->getProgressMutex() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
                        fileFTPProgressList[getMissingTechtreeFromFTPServer] = pair<int,string>(0,"");
                        safeMutexFTPProgress.ReleaseLock();
                    }
			    }
			}
		}
	}
	else if(buttonDisconnect.mouseClick(x,y)){
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		soundRenderer.playFx(coreData.getClickSoundA());
		if(clientInterface->getSocket() != NULL) {
		    if(clientInterface->isConnected() == true) {
		    	Lang &lang= Lang::getInstance();
		    	const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
		    	for(unsigned int i = 0; i < languageList.size(); ++i) {
		    		string sQuitText = lang.get("QuitGame",languageList[i]);
		    		clientInterface->sendTextMessage(sQuitText,-1,false,languageList[i]);

		    		//printf("~~~ langList[%s] localLang[%s] msg[%s]\n",languageList[i].c_str(),lang.getLanguage().c_str(), sQuitText.c_str());
		    	}
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

	if (initialSettingsReceivedFromServer == false) return;

	// Only allow changes after we get game settings from the server
	if(clientInterface->isConnected() == true){
		int myCurrentIndex= -1;
		for(int i= 0; i < GameConstants::maxPlayers; ++i){// find my current index by looking at editable listBoxes
			if(listBoxFactions[i].getEditable()){
				myCurrentIndex= i;
			}
		}
		if(myCurrentIndex != -1)
			for(int i= 0; i < GameConstants::maxPlayers; ++i){
				if(listBoxFactions[i].getEditable()){
					if(listBoxFactions[i].mouseClick(x, y)){
						soundRenderer.playFx(coreData.getClickSoundA());
						ClientInterface* clientInterface= NetworkManager::getInstance().getClientInterface();
						if(clientInterface->isConnected()){
							clientInterface->setGameSettingsReceived(false);
							clientInterface->sendSwitchSetupRequest(
									listBoxFactions[i].getSelectedItem(),
									i,
									-1,
							        listBoxTeams[i].getSelectedItemIndex(),
							        getHumanPlayerName(),
							        getNetworkPlayerStatus(),
							        switchSetupRequestFlagType,
							        lang.getLanguage());
							switchSetupRequestFlagType= ssrft_None;
						}
						break;
					}
				}
				if(listBoxTeams[i].getEditable()){
					if(listBoxTeams[i].mouseClick(x, y)){
						soundRenderer.playFx(coreData.getClickSoundA());
						if(clientInterface->isConnected()){
							clientInterface->setGameSettingsReceived(false);
							clientInterface->sendSwitchSetupRequest(
									listBoxFactions[i].getSelectedItem(),
									i,
									-1,
							        listBoxTeams[i].getSelectedItemIndex(),
							        getHumanPlayerName(),
							        getNetworkPlayerStatus(),
							        switchSetupRequestFlagType,
							        lang.getLanguage());
							switchSetupRequestFlagType= ssrft_None;
						}
						break;
					}
				}
				if((listBoxControls[i].getSelectedItemIndex() == ctNetwork) && (labelNetStatus[i].getText()
				        == GameConstants::NETWORK_SLOT_UNCONNECTED_SLOTNAME)){
					if(grabSlotButton[i].mouseClick(x, y)){
						soundRenderer.playFx(coreData.getClickSoundA());
						clientInterface->setGameSettingsReceived(false);
						settingsReceivedFromServer= false;

						if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] sending a switchSlot request from %d to %d\n",__FILE__,__FUNCTION__,__LINE__,clientInterface->getGameSettings()->getThisFactionIndex(),i);

						clientInterface->sendSwitchSetupRequest(
								listBoxFactions[myCurrentIndex].getSelectedItem(),
						        myCurrentIndex,
						        i,
						        listBoxTeams[myCurrentIndex].getSelectedItemIndex(),
						        //labelPlayerNames[myCurrentIndex].getText(),
						        getHumanPlayerName(),
						        getNetworkPlayerStatus(),
						        switchSetupRequestFlagType,
						        lang.getLanguage());
						labelPlayerNames[myCurrentIndex].setText("");
						labelPlayerNames[i].setText("");
						switchSetupRequestFlagType= ssrft_None;
						break;
					}
				}

				if(labelPlayerNames[i].mouseClick(x, y) && (activeInputLabel != &labelPlayerNames[i])){
					if(i == clientInterface->getPlayerIndex()){
						setActiveInputLabel(&labelPlayerNames[i]);
					}
				}
			}

		if(listBoxPlayerStatus.mouseClick(x,y)) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
			soundRenderer.playFx(coreData.getClickSoundC());
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

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

			ClientInterface* clientInterface= NetworkManager::getInstance().getClientInterface();
			if(clientInterface->isConnected()) {
				clientInterface->setGameSettingsReceived(false);
				clientInterface->sendSwitchSetupRequest(
						listBoxFactions[clientInterface->getPlayerIndex()].getSelectedItem(),
						clientInterface->getPlayerIndex(),-1,
						listBoxTeams[clientInterface->getPlayerIndex()].getSelectedItemIndex(),
						getHumanPlayerName(),
						getNetworkPlayerStatus(),
						switchSetupRequestFlagType,
						lang.getLanguage());
				switchSetupRequestFlagType=ssrft_None;
			}
		}
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void MenuStateConnectedGame::returnToJoinMenu() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(ftpClientThread != NULL) {
	    ftpClientThread->setCallBackObject(NULL);
	    if(ftpClientThread->shutdownAndWait() == true) {
            delete ftpClientThread;
            ftpClientThread = NULL;
	    }
	}

	if(returnMenuInfo == jmSimple) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
		mainMenu->setState(new MenuStateJoinGame(program, mainMenu));
	}
	else {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
		mainMenu->setState(new MenuStateMasterserver(program, mainMenu));
	}
}

void MenuStateConnectedGame::mouseMove(int x, int y, const MouseState *ms) {
	if (mainMessageBox.getEnabled()) {
		mainMessageBox.mouseMove(x, y);
	}

	if (ftpMessageBox.getEnabled()) {
		ftpMessageBox.mouseMove(x, y);
	}

	buttonDisconnect.mouseMove(x, y);

	bool editingPlayerName = false;
	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
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
	listBoxAllowObservers.mouseMove(x, y);
	listBoxTileset.mouseMove(x, y);
	listBoxTechTree.mouseMove(x, y);
	listBoxPlayerStatus.mouseMove(x,y);
}

void MenuStateConnectedGame::render() {
	try {
		Renderer &renderer= Renderer::getInstance();

		if(mainMessageBox.getEnabled()) {
			renderer.renderMessageBox(&mainMessageBox);
		}

		if (!initialSettingsReceivedFromServer) return;

		if(factionTexture != NULL) {
			//renderer.renderTextureQuad(60+575+80,365,200,225,factionTexture,1);
			renderer.renderTextureQuad(800,600,200,150,factionTexture,1);
		}

		renderer.renderButton(&buttonDisconnect);

		// Get a reference to the player texture cache
		std::map<int,Texture2D *> &crcPlayerTextureCache = CacheManager::getCachedItem< std::map<int,Texture2D *> >(GameConstants::playerTextureCacheLookupKey);

		// START - this code ensure player title and player names don't overlap
		int offsetPosition=0;
	    for(int i=0; i < GameConstants::maxPlayers; ++i) {
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

	    renderer.renderListBox(&listBoxPlayerStatus);

		for(int i = 0; i < GameConstants::maxPlayers; ++i) {
			if(listBoxControls[i].getSelectedItemIndex() != ctClosed) {
				renderer.renderLabel(&labelPlayerStatus[i]);
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

			renderer.renderListBox(&listBoxControls[i]);
			if(listBoxControls[i].getSelectedItemIndex()!=ctClosed){
				renderer.renderListBox(&listBoxRMultiplier[i]);
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
		renderer.renderLabel(&labelStatus);
		renderer.renderLabel(&labelInfo);

		if(difftime(time(NULL),timerLabelFlash) < 1) {
		    renderer.renderLabel(&labelDataSynchInfo,&RED);
		}
		else {
            renderer.renderLabel(&labelDataSynchInfo,&WHITE);
            if(difftime(time(NULL),timerLabelFlash) > 2) {
                timerLabelFlash = time(NULL);
            }
		}

		renderer.renderLabel(&labelMap);
		renderer.renderLabel(&labelFogOfWar);
		renderer.renderLabel(&labelAllowObservers);
		renderer.renderLabel(&labelTileset);
		renderer.renderLabel(&labelTechTree);
		renderer.renderLabel(&labelControl);
		//renderer.renderLabel(&labelRMultiplier);
		renderer.renderLabel(&labelFaction);
		renderer.renderLabel(&labelTeam);
		renderer.renderLabel(&labelMapInfo);

		renderer.renderListBox(&listBoxMap);
		renderer.renderListBox(&listBoxFogOfWar);
		renderer.renderListBox(&listBoxAllowObservers);
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

        MutexSafeWrapper safeMutexFTPProgress((ftpClientThread != NULL ? ftpClientThread->getProgressMutex() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
        if(fileFTPProgressList.size() > 0) {
        	Lang &lang= Lang::getInstance();
            int yLocation = buttonDisconnect.getY();
            for(std::map<string,pair<int,string> >::iterator iterMap = fileFTPProgressList.begin();
                iterMap != fileFTPProgressList.end(); ++iterMap) {
                string progressLabelPrefix = lang.get("ModDownloading") + " " + iterMap->first + " ";
                //if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\nRendering file progress with the following prefix [%s]\n",progressLabelPrefix.c_str());

                renderer.renderProgressBar(
                    iterMap->second.first,
                    10,
                    yLocation,
                    CoreData::getInstance().getDisplayFontSmall(),
                    350,progressLabelPrefix);

                yLocation -= 10;
            }
        }
        safeMutexFTPProgress.ReleaseLock();

		if(mainMessageBox.getEnabled()) {
			renderer.renderMessageBox(&mainMessageBox);
		}
		if(ftpMessageBox.getEnabled()) {
			renderer.renderMessageBox(&ftpMessageBox);
		}

		if(program != NULL) program->renderProgramMsgBox();

		if(enableMapPreview && (mapPreview.hasFileLoaded() == true)) {

			int mouseX = mainMenu->getMouseX();
			int mouseY = mainMenu->getMouseY();
			int mouse2dAnim = mainMenu->getMouse2dAnim();

		    renderer.renderMouse2d(mouseX, mouseY, mouse2dAnim);
		    bool renderAll = (listBoxFogOfWar.getSelectedItemIndex() == 2);
		    renderer.renderMapPreview(&mapPreview, renderAll, 10, 350);
		}
		renderer.renderChatManager(&chatManager);
		renderer.renderConsole(&console,showFullConsole,true);
	}
	catch(const std::exception &ex) {
		char szBuf[1024]="";
		sprintf(szBuf,"In [%s::%s %d] error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		throw runtime_error(szBuf);
	}
}

void MenuStateConnectedGame::update() {
	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

	ClientInterface* clientInterface= NetworkManager::getInstance().getClientInterface();
	Lang &lang= Lang::getInstance();

	// Test progress bar
    //MutexSafeWrapper safeMutexFTPProgress((ftpClientThread != NULL ? ftpClientThread->getProgressMutex() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
    //fileFTPProgressList["test"] = pair<int,string>(difftime(time(NULL),lastNetworkSendPing) * 20,"test file 123");
    //safeMutexFTPProgress.ReleaseLock();
    //

	if(clientInterface != NULL && clientInterface->isConnected()) {
		if(difftime(time(NULL),lastNetworkSendPing) >= GameConstants::networkPingInterval) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] about to sendPingMessage...\n",__FILE__,__FUNCTION__,__LINE__);

			lastNetworkSendPing = time(NULL);
			clientInterface->sendPingMessage(GameConstants::networkPingInterval, time(NULL));

			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] pingCount = %d, clientInterface->getLastPingLag() = %f, GameConstants::networkPingInterval = %d\n",__FILE__,__FUNCTION__,__LINE__,pingCount, clientInterface->getLastPingLag(),GameConstants::networkPingInterval);

			// Starting checking timeout after sending at least 3 pings to server
			if(clientInterface != NULL && clientInterface->isConnected() &&
				pingCount >= 3 && clientInterface->getLastPingLag() >= (GameConstants::networkPingInterval * 3)) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		    	Lang &lang= Lang::getInstance();
		    	const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
		    	for(unsigned int i = 0; i < languageList.size(); ++i) {
					//string playerNameStr = getHumanPlayerName();
					clientInterface->sendTextMessage(lang.get("ConnectionTimedOut",languageList[i]),-1,false,languageList[i]);
					sleep(1);
					clientInterface->close();
		    	}
			}

			pingCount++;
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		}
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

	//update status label
	if(clientInterface != NULL && clientInterface->isConnected()) {
		buttonDisconnect.setText(lang.get("Disconnect"));

		if(clientInterface->getAllowDownloadDataSynch() == false) {
		    string label = lang.get("ConnectedToServer");

            if(clientInterface->getServerName().empty() == false) {
                label = label + " " + clientInterface->getServerName();
            }

            label = label + ", " + clientInterface->getVersionString();

            const GameSettings *gameSettings = clientInterface->getGameSettings();
            if(clientInterface->getAllowGameDataSynchCheck() == false &&
            	gameSettings->getTileset() != "" &&
            	gameSettings->getTech() != "" &&
            	gameSettings->getMap() != "") {
                Config &config = Config::getInstance();

                MutexSafeWrapper safeMutexFTPProgress(ftpClientThread != NULL ? ftpClientThread->getProgressMutex() : NULL,string(__FILE__) + "_" + intToStr(__LINE__));

                int32 tilesetCRC = lastCheckedCRCTilesetValue;
                if(lastCheckedCRCTilesetName != gameSettings->getTileset() &&
                	gameSettings->getTileset() != "") {
					//console.addLine("Checking tileset CRC [" + gameSettings->getTileset() + "]");
					tilesetCRC = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTilesets,""), string("/") + gameSettings->getTileset() + string("/*"), ".xml", NULL, true);
					// Test data synch
					//tilesetCRC++;
					lastCheckedCRCTilesetValue = tilesetCRC;
					lastCheckedCRCTilesetName = gameSettings->getTileset();
                }

                int32 techCRC = lastCheckedCRCTechtreeValue;
                if(lastCheckedCRCTechtreeName != gameSettings->getTech() &&
                	gameSettings->getTech() != "") {
					//console.addLine("Checking techtree CRC [" + gameSettings->getTech() + "]");
					techCRC = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTechs,""), string("/") + gameSettings->getTech() + string("/*"), ".xml", NULL);
					// Test data synch
					//techCRC++;
					lastCheckedCRCTechtreeValue = techCRC;
					lastCheckedCRCTechtreeName = gameSettings->getTech();

					loadFactions(gameSettings,false);
	    			factionCRCList.clear();
	    			for(unsigned int factionIdx = 0; factionIdx < factionFiles.size(); ++factionIdx) {
	    				string factionName = factionFiles[factionIdx];
	    				int32 factionCRC   = 0;
	    				if(factionName != GameConstants::RANDOMFACTION_SLOTNAME &&
	    					factionName != GameConstants::OBSERVER_SLOTNAME &&
	    					factionName != ITEM_MISSING) {
	    					//factionCRC   = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTechs,""), "/" + gameSettings->getTech() + "/factions/" + factionName + "/*", ".xml", NULL, true);
	    					factionCRC   = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTechs,""), "/" + gameSettings->getTech() + "/factions/" + factionName + "/*", ".xml", NULL);
	    				}
	    				factionCRCList.push_back(make_pair(factionName,factionCRC));
	    			}
	    			//console.addLine("Found factions: " + intToStr(factionCRCList.size()));
                }

                int32 mapCRC = lastCheckedCRCMapValue;
                if(lastCheckedCRCMapName != gameSettings->getMap() &&
                	gameSettings->getMap() != "") {
					Checksum checksum;
					string file = Map::getMapPath(gameSettings->getMap(),"",false);
					//console.addLine("Checking map CRC [" + file + "]");
					checksum.addFile(file);
					mapCRC = checksum.getSum();
					// Test data synch
					//mapCRC++;

					lastCheckedCRCMapValue = mapCRC;
					lastCheckedCRCMapName = gameSettings->getMap();
                }
                safeMutexFTPProgress.ReleaseLock();

                bool dataSynchMismatch = ((mapCRC != 0 && mapCRC != gameSettings->getMapCRC()) ||
                						  (tilesetCRC != 0 && tilesetCRC != gameSettings->getTilesetCRC()) ||
                						  (techCRC != 0 && techCRC != gameSettings->getTechCRC()));

                //if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\nmapCRC [%d] gameSettings->getMapCRC() [%d]\ntilesetCRC [%d] gameSettings->getTilesetCRC() [%d]\ntechCRC [%d] gameSettings->getTechCRC() [%d]\n",mapCRC,gameSettings->getMapCRC(),tilesetCRC,gameSettings->getTilesetCRC(),techCRC,gameSettings->getTechCRC());

                if(dataSynchMismatch == true) {
                    string labelSynch = lang.get("DataNotSynchedTitle");

                    if(mapCRC != 0 && mapCRC != gameSettings->getMapCRC() &&
                    		listBoxMap.getSelectedItemIndex() >= 0 &&
                    		listBoxMap.getSelectedItem() != ITEM_MISSING) {
                        labelSynch = labelSynch + " " + lang.get("Map");

                        if(updateDataSynchDetailText == true &&
                            lastMapDataSynchError != lang.get("DataNotSynchedMap") + " " + listBoxMap.getSelectedItem()) {
                            lastMapDataSynchError = lang.get("DataNotSynchedMap") + " " + listBoxMap.getSelectedItem();

            		    	Lang &lang= Lang::getInstance();
            		    	const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
            		    	for(unsigned int i = 0; i < languageList.size(); ++i) {
            		    		string msg = lang.get("DataNotSynchedMap",languageList[i]) + " " + listBoxMap.getSelectedItem();
            		    		bool localEcho = lang.isLanguageLocal(languageList[i]);
            		    		clientInterface->sendTextMessage(msg,-1,localEcho,languageList[i]);
            		    	}
                        }
                    }

                    if(tilesetCRC != 0 && tilesetCRC != gameSettings->getTilesetCRC() &&
                    		listBoxTileset.getSelectedItemIndex() >= 0 &&
                    		listBoxTileset.getSelectedItem() != ITEM_MISSING) {
                        labelSynch = labelSynch + " " + lang.get("Tileset");
                        if(updateDataSynchDetailText == true &&
                            lastTileDataSynchError != lang.get("DataNotSynchedTileset") + " " + listBoxTileset.getSelectedItem()) {
                            lastTileDataSynchError = lang.get("DataNotSynchedTileset") + " " + listBoxTileset.getSelectedItem();

            		    	Lang &lang= Lang::getInstance();
            		    	const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
            		    	for(unsigned int i = 0; i < languageList.size(); ++i) {
            		    		string msg = lang.get("DataNotSynchedTileset",languageList[i]) + " " + listBoxTileset.getSelectedItem();
            		    		bool localEcho = lang.isLanguageLocal(languageList[i]);
            		    		clientInterface->sendTextMessage(msg,-1,localEcho,languageList[i]);
            		    	}
                        }
                    }

                    if(techCRC != 0 && techCRC != gameSettings->getTechCRC() &&
                    		listBoxTechTree.getSelectedItemIndex() >= 0 &&
                    		listBoxTechTree.getSelectedItem() != ITEM_MISSING) {
                        labelSynch = labelSynch + " " + lang.get("TechTree");
                        if(updateDataSynchDetailText == true &&
                        	lastTechtreeDataSynchError != lang.get("DataNotSynchedTechtree") + " " + listBoxTechTree.getSelectedItem()) {
                        	lastTechtreeDataSynchError = lang.get("DataNotSynchedTechtree") + " " + listBoxTechTree.getSelectedItem();

            		    	Lang &lang= Lang::getInstance();
            		    	const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
            		    	for(unsigned int i = 0; i < languageList.size(); ++i) {
            		    		string msg = lang.get("DataNotSynchedTechtree",languageList[i]) + " " + listBoxTechTree.getSelectedItem();
            		    		bool localEcho = lang.isLanguageLocal(languageList[i]);
            		    		clientInterface->sendTextMessage(msg,-1,localEcho,languageList[i]);
            		    	}

            		    	//const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
            		    	for(unsigned int i = 0; i < languageList.size(); ++i) {
            		    		bool localEcho = lang.isLanguageLocal(languageList[i]);

								string mismatchedFactionText = "";
								vector<pair<string,int32> > serverFactionCRCList = gameSettings->getFactionCRCList();

								for(unsigned int factionIdx = 0; factionIdx < serverFactionCRCList.size(); ++factionIdx) {
									pair<string,int32> &serverFaction = serverFactionCRCList[factionIdx];

									bool foundFaction = false;
									for(unsigned int clientFactionIdx = 0; clientFactionIdx < factionCRCList.size(); ++clientFactionIdx) {
										pair<string,int32> &clientFaction = factionCRCList[clientFactionIdx];

										if(serverFaction.first == clientFaction.first) {
											foundFaction = true;
											if(serverFaction.second != clientFaction.second) {
												if(mismatchedFactionText == "") {
													mismatchedFactionText = "The following factions are mismatched: ";
													if(lang.hasString("MismatchedFactions",languageList[i]) == true) {
														mismatchedFactionText = lang.get("MismatchedFactions",languageList[i]);
													}

													mismatchedFactionText += " ["+ intToStr(factionCRCList.size()) + "][" + intToStr(serverFactionCRCList.size()) + "] - ";
												}
												mismatchedFactionText += serverFaction.first + ", ";
											}
											break;
										}
									}

									if(foundFaction == false) {
										if(mismatchedFactionText == "") {
											mismatchedFactionText = "The following factions are mismatched: ";
											if(lang.hasString("MismatchedFactions",languageList[i]) == true) {
												mismatchedFactionText = lang.get("MismatchedFactions",languageList[i]);
											}

											mismatchedFactionText += " ["+ intToStr(factionCRCList.size()) + "][" + intToStr(serverFactionCRCList.size()) + "] - ";
										}
										if(lang.hasString("MismatchedFactionsMissing",languageList[i]) == true) {
											mismatchedFactionText += serverFaction.first + " " + lang.get("MismatchedFactionsMissing",languageList[i]) + ", ";
										}
										else {
											mismatchedFactionText += serverFaction.first + " (missing), ";
										}
									}
								}

								for(unsigned int clientFactionIdx = 0; clientFactionIdx < factionCRCList.size(); ++clientFactionIdx) {
									pair<string,int32> &clientFaction = factionCRCList[clientFactionIdx];

									bool foundFaction = false;
									for(unsigned int factionIdx = 0; factionIdx < serverFactionCRCList.size(); ++factionIdx) {
										pair<string,int32> &serverFaction = serverFactionCRCList[factionIdx];

										if(serverFaction.first == clientFaction.first) {
											foundFaction = true;
											break;
										}
									}

									if(foundFaction == false) {
										if(mismatchedFactionText == "") {
											mismatchedFactionText = "The following factions are mismatched: ";
											if(lang.hasString("MismatchedFactions",languageList[i]) == true) {
												mismatchedFactionText = lang.get("MismatchedFactions",languageList[i]);
											}

											mismatchedFactionText += " ["+ intToStr(factionCRCList.size()) + "][" + intToStr(serverFactionCRCList.size()) + "] - ";
										}
										if(lang.hasString("MismatchedFactionsExtra",languageList[i]) == true) {
											mismatchedFactionText += clientFaction.first + " " + lang.get("MismatchedFactionsExtra",languageList[i]) + ", ";
										}
										else {
											mismatchedFactionText += clientFaction.first + " (extra), ";
										}
									}
								}

								if(mismatchedFactionText != "") {
									clientInterface->sendTextMessage(mismatchedFactionText,-1,localEcho,languageList[i]);
								}
            		    	}
                        }
                    }

                    //if(clientInterface->getReceivedDataSynchCheck() == true) {
                    updateDataSynchDetailText = false;
                    //}

                    labelDataSynchInfo.setText(labelSynch);
                }
                else {
                    labelDataSynchInfo.setText("");
                }
            }

            if(clientInterface->getAllowGameDataSynchCheck() == true &&
               clientInterface->getNetworkGameDataSynchCheckOk() == false) {
                label = label + " -synch mismatch for:";

                if(clientInterface->getNetworkGameDataSynchCheckOkMap() == false) {
                    label = label + " map";

                    if(updateDataSynchDetailText == true &&
                    	clientInterface->getReceivedDataSynchCheck() &&
                    	lastMapDataSynchError != "map CRC mismatch, " + listBoxMap.getSelectedItem()) {
                    	lastMapDataSynchError = "map CRC mismatch, " + listBoxMap.getSelectedItem();
                    	clientInterface->sendTextMessage(lastMapDataSynchError,-1,true, "");
                    }
                }

                if(clientInterface->getNetworkGameDataSynchCheckOkTile() == false) {
                    label = label + " tile";
                    if(updateDataSynchDetailText == true &&
                    	clientInterface->getReceivedDataSynchCheck() &&
                    	lastTileDataSynchError != "tile CRC mismatch, " + listBoxTileset.getSelectedItem()) {
                    	lastTileDataSynchError = "tile CRC mismatch, " + listBoxTileset.getSelectedItem();
                    	clientInterface->sendTextMessage(lastTileDataSynchError,-1,true,"");
                    }
                }

                if(clientInterface->getNetworkGameDataSynchCheckOkTech() == false) {
                    label = label + " techtree";

                    if(updateDataSynchDetailText == true &&
                    	clientInterface->getReceivedDataSynchCheck()) {

                    	string report = clientInterface->getNetworkGameDataSynchCheckTechMismatchReport();
						if(lastTechtreeDataSynchError != "techtree CRC mismatch" + report) {
							lastTechtreeDataSynchError = "techtree CRC mismatch" + report;

							if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] report: %s\n",__FILE__,__FUNCTION__,__LINE__,report.c_str());

							clientInterface->sendTextMessage("techtree CRC mismatch",-1,true,"");
							vector<string> reportLineTokens;
							Tokenize(report,reportLineTokens,"\n");
							for(int reportLine = 0; reportLine < reportLineTokens.size(); ++reportLine) {
								clientInterface->sendTextMessage(reportLineTokens[reportLine],-1,true,"");
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
               clientInterface->getNetworkGameDataSynchCheckOk() == false) {
                label = label + " -waiting to synch:";
                if(clientInterface->getNetworkGameDataSynchCheckOkMap() == false) {
                    label = label + " map";
                }
                if(clientInterface->getNetworkGameDataSynchCheckOkTile() == false) {
                    label = label + " tile";
                }
                if(clientInterface->getNetworkGameDataSynchCheckOkTech() == false) {
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

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();
	}
	else {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		if(clientInterface != NULL && clientInterface->isConnected() == true) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		    clientInterface->close();
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		returnToJoinMenu();
		return;
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

	//process network messages
	if(clientInterface != NULL && clientInterface->isConnected()) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

		try {
			bool mustSwitchPlayerName = false;
			if(clientInterface->getGameSettingsReceived()) {
				updateDataSynchDetailText = true;
				bool errorOnMissingData = (clientInterface->getAllowGameDataSynchCheck() == false);
				vector<string> maps,tilesets,techtree;
				const GameSettings *gameSettings = clientInterface->getGameSettings();

				if(gameSettings == NULL) {
					throw runtime_error("gameSettings == NULL");
				}

				if(getMissingTilesetFromFTPServerInProgress == false &&
					gameSettings->getTileset() != "") {
                    // tileset
                    if(std::find(tilesetFiles.begin(),tilesetFiles.end(),gameSettings->getTileset()) != tilesetFiles.end()) {
                        lastMissingTileSet = "";
                        getMissingTilesetFromFTPServer = "";
                        tilesets.push_back(formatString(gameSettings->getTileset()));
                    }
                    else {
                        // try to get the tileset via ftp
                        if(ftpClientThread != NULL && getMissingTilesetFromFTPServer != gameSettings->getTileset()) {
                            if(ftpMessageBox.getEnabled() == false) {
                                getMissingTilesetFromFTPServer = gameSettings->getTileset();
                                Lang &lang= Lang::getInstance();

                                char szBuf[1024]="";
                                sprintf(szBuf,"%s %s ?",lang.get("DownloadMissingTilesetQuestion").c_str(),gameSettings->getTileset().c_str());

                                ftpMissingDataType = ftpmsg_MissingTileset;
                                showFTPMessageBox(szBuf, lang.get("Question"), false);
                            }
                        }

                        tilesets.push_back(ITEM_MISSING);

                        NetworkManager &networkManager= NetworkManager::getInstance();
                        ClientInterface* clientInterface= networkManager.getClientInterface();
                        const GameSettings *gameSettings = clientInterface->getGameSettings();

                        if(lastMissingTileSet != gameSettings->getTileset()) {
                            lastMissingTileSet = gameSettings->getTileset();

                            if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

            		    	Lang &lang= Lang::getInstance();
            		    	const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
            		    	for(unsigned int i = 0; i < languageList.size(); ++i) {

								char szMsg[1024]="";
								if(lang.hasString("DataMissingTileset",languageList[i]) == true) {
									sprintf(szMsg,lang.get("DataMissingTileset",languageList[i]).c_str(),getHumanPlayerName().c_str(),gameSettings->getTileset().c_str());
								}
								else {
									sprintf(szMsg,"Player: %s is missing the tileset: %s",getHumanPlayerName().c_str(),gameSettings->getTileset().c_str());
								}
								clientInterface->sendTextMessage(szMsg,-1, lang.isLanguageLocal(languageList[i]),languageList[i]);
            		    	}
                        }
                    }
                    listBoxTileset.setItems(tilesets);
				}

				if(getMissingTechtreeFromFTPServerInProgress == false &&
					gameSettings->getTech() != "") {
                    // techtree
                    if(std::find(techTreeFiles.begin(),techTreeFiles.end(),gameSettings->getTech()) != techTreeFiles.end()) {
                        lastMissingTechtree = "";
                        getMissingTechtreeFromFTPServer = "";
                        techtree.push_back(formatString(gameSettings->getTech()));
                    }
                    else {
                        // try to get the tileset via ftp
                        if(ftpClientThread != NULL && getMissingTechtreeFromFTPServer != gameSettings->getTech()) {
                            if(ftpMessageBox.getEnabled() == false) {
                                getMissingTechtreeFromFTPServer = gameSettings->getTech();
                                Lang &lang= Lang::getInstance();

                                char szBuf[1024]="";
                                sprintf(szBuf,"%s %s ?",lang.get("DownloadMissingTechtreeQuestion").c_str(),gameSettings->getTech().c_str());

                                ftpMissingDataType = ftpmsg_MissingTechtree;
                                showFTPMessageBox(szBuf, lang.get("Question"), false);
                            }
                        }

                        techtree.push_back(ITEM_MISSING);

                        NetworkManager &networkManager= NetworkManager::getInstance();
                        ClientInterface* clientInterface= networkManager.getClientInterface();
                        const GameSettings *gameSettings = clientInterface->getGameSettings();

                        if(lastMissingTechtree != gameSettings->getTech()) {
                            lastMissingTechtree = gameSettings->getTech();

                            if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

            		    	Lang &lang= Lang::getInstance();
            		    	const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
            		    	for(unsigned int i = 0; i < languageList.size(); ++i) {

								char szMsg[1024]="";
	                            if(lang.hasString("DataMissingTechtree",languageList[i]) == true) {
	                            	sprintf(szMsg,lang.get("DataMissingTechtree",languageList[i]).c_str(),getHumanPlayerName().c_str(),gameSettings->getTech().c_str());
	                            }
	                            else {
	                            	sprintf(szMsg,"Player: %s is missing the techtree: %s",getHumanPlayerName().c_str(),gameSettings->getTech().c_str());
	                            }
								clientInterface->sendTextMessage(szMsg,-1, lang.isLanguageLocal(languageList[i]),languageList[i]);
            		    	}
                        }
                    }
                    listBoxTechTree.setItems(techtree);

					// techtree
					//techtree.push_back(formatString(gameSettings->getTech()));
					//listBoxTechTree.setItems(techtree);
				}

				// factions
				bool hasFactions = true;
				if(currentFactionName != gameSettings->getTech()
					&& gameSettings->getTech() != "") {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] hasFactions = %d, currentFactionName [%s]\n",__FILE__,__FUNCTION__,__LINE__,hasFactions,currentFactionName.c_str());
					currentFactionName = gameSettings->getTech();
					hasFactions = loadFactions(gameSettings,false);
				}
				else {
					// do this to process special faction types like observers
					loadFactions(gameSettings,false);
				}

				if(getMissingMapFromFTPServerInProgress == false &&
					gameSettings->getMap() != "") {
                    // map
                    if(currentMap != gameSettings->getMap()) {// load the setup again
                        currentMap = gameSettings->getMap();
                    }
                    bool mapLoaded = loadMapInfo(Map::getMapPath(currentMap,"",false), &mapInfo, true);
                    if(mapLoaded == true) {
                        maps.push_back(formatString(gameSettings->getMap()));
                    }
                    else {
                        // try to get the map via ftp
                        if(ftpClientThread != NULL && getMissingMapFromFTPServer != currentMap) {
                            if(ftpMessageBox.getEnabled() == false) {
                                getMissingMapFromFTPServer = currentMap;
                                Lang &lang= Lang::getInstance();

                                char szBuf[1024]="";
                                sprintf(szBuf,"%s %s ?",lang.get("DownloadMissingMapQuestion").c_str(),currentMap.c_str());
                                ftpMissingDataType = ftpmsg_MissingMap;
                                showFTPMessageBox(szBuf, lang.get("Question"), false);
                            }
                        }
                        maps.push_back(ITEM_MISSING);
                    }
                    listBoxMap.setItems(maps);
                    labelMapInfo.setText(mapInfo.desc);
				}

				// FogOfWar
				listBoxFogOfWar.setSelectedItemIndex(0); // default is 0!
				if(gameSettings->getFogOfWar() == false){
					listBoxFogOfWar.setSelectedItemIndex(2);
				}
				if((gameSettings->getFlagTypes1() & ft1_show_map_resources) == ft1_show_map_resources){
        			if(gameSettings->getFogOfWar() == true){
        				listBoxFogOfWar.setSelectedItemIndex(1);
        			}
				}

				// Allow Observers
				if(gameSettings->getAllowObservers()) {
					listBoxAllowObservers.setSelectedItemIndex(1);
				}
				else
				{
					listBoxAllowObservers.setSelectedItemIndex(0);
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

				// Control
				for(int i=0; i<GameConstants::maxPlayers; ++i){
					listBoxControls[i].setSelectedItemIndex(ctClosed);
					listBoxFactions[i].setEditable(false);
					listBoxTeams[i].setEditable(false);

					labelPlayerStatus[i].setText("");
				}

				if(hasFactions == true) {
					for(int i=0; i<gameSettings->getFactionCount(); ++i){
						int slot = gameSettings->getStartLocationIndex(i);

						if(gameSettings != NULL) {
							if(	gameSettings->getFactionControl(i) == ctNetwork ||
								gameSettings->getFactionControl(i) == ctHuman) {
								switch(gameSettings->getNetworkPlayerStatuses(i)) {
									case npst_BeRightBack:
										labelPlayerStatus[slot].setText(lang.get("PlayerStatusBeRightBack"));
										labelPlayerStatus[slot].setTextColor(Vec3f(1.f, 0.8f, 0.f));
										break;
									case npst_Ready:
										labelPlayerStatus[slot].setText(lang.get("PlayerStatusReady"));
										labelPlayerStatus[slot].setTextColor(Vec3f(0.f, 1.f, 0.f));
										break;
									case npst_PickSettings:
										labelPlayerStatus[slot].setText(lang.get("PlayerStatusSetup"));
										labelPlayerStatus[slot].setTextColor(Vec3f(1.f, 0.f, 0.f));
										break;
									default:
										labelPlayerStatus[slot].setText("");
										break;
								}
							}
						}


						listBoxControls[slot].setSelectedItemIndex(gameSettings->getFactionControl(i),errorOnMissingData);
						listBoxRMultiplier[slot].setSelectedItemIndex((gameSettings->getResourceMultiplierIndex(i)));
						listBoxTeams[slot].setSelectedItemIndex(gameSettings->getTeam(i),errorOnMissingData);
						//listBoxFactions[slot].setSelectedItem(formatString(gameSettings->getFactionTypeName(i)),errorOnMissingData);
						listBoxFactions[slot].setSelectedItem(formatString(gameSettings->getFactionTypeName(i)),false);
						if(gameSettings->getFactionControl(i) == ctNetwork ){
							labelNetStatus[slot].setText(gameSettings->getNetworkPlayerName(i));
							if( gameSettings->getThisFactionIndex() != i &&
								gameSettings->getNetworkPlayerName(i) != "" &&
								gameSettings->getNetworkPlayerName(i) != GameConstants::NETWORK_SLOT_UNCONNECTED_SLOTNAME) {
								labelPlayerNames[slot].setText(gameSettings->getNetworkPlayerName(i));
							}
						}

						ControlType ct= gameSettings->getFactionControl(i);
						if (ct == ctHuman || ct == ctNetwork || ct == ctClosed) {
							listBoxRMultiplier[slot].setEnabled(false);
							listBoxRMultiplier[slot].setVisible(false);
						} else {
							listBoxRMultiplier[slot].setEnabled(true);
							listBoxRMultiplier[slot].setVisible(true);
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

						if( currentTechName_factionPreview != gameSettings->getTech() ||
							currentFactionName_factionPreview != gameSettings->getFactionTypeName(gameSettings->getThisFactionIndex())) {

							currentTechName_factionPreview=gameSettings->getTech();
							currentFactionName_factionPreview=gameSettings->getFactionTypeName(gameSettings->getThisFactionIndex());

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
				}

				if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
				if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();
			}

			//update lobby
			clientInterface= NetworkManager::getInstance().getClientInterface();
			if(clientInterface != NULL && clientInterface->isConnected()) {
				clientInterface->updateLobby();

				if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
				if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();
			}

			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

			clientInterface= NetworkManager::getInstance().getClientInterface();
			if(clientInterface != NULL && clientInterface->isConnected()) {
				if(	initialSettingsReceivedFromServer == true &&
					clientInterface->getIntroDone() == true &&
					(switchSetupRequestFlagType & ssrft_NetworkPlayerName) == ssrft_NetworkPlayerName) {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] getHumanPlayerName() = [%s], clientInterface->getGameSettings()->getThisFactionIndex() = %d\n",__FILE__,__FUNCTION__,__LINE__,getHumanPlayerName().c_str(),clientInterface->getGameSettings()->getThisFactionIndex());
					clientInterface->sendSwitchSetupRequest(
							"",
							clientInterface->getPlayerIndex(),
							-1,
							-1,
							getHumanPlayerName(),
							getNetworkPlayerStatus(),
							switchSetupRequestFlagType,
							lang.getLanguage());

					switchSetupRequestFlagType=ssrft_None;
				}

				if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
				if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

				//intro
				if(clientInterface->getIntroDone()) {
					labelInfo.setText(lang.get("WaitingHost"));
					//servers.setString(clientInterface->getServerName(), Ip(labelServerIp.getText()).getString());
				}

				//launch
				if(clientInterface->getLaunchGame()) {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

					assert(clientInterface != NULL);

					if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

                    if(ftpClientThread != NULL) {
                        ftpClientThread->setCallBackObject(NULL);
                        if(ftpClientThread->shutdownAndWait() == true) {
                            delete ftpClientThread;
                            ftpClientThread = NULL;
                        }
                    }

                    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

					program->setState(new Game(program, clientInterface->getGameSettings()));
					return;

					if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
				}
			}

			//call the chat manager
			chatManager.updateNetwork();

			//console732
            console.update();

		}
		catch(const runtime_error &ex) {
			char szBuf[1024]="";
			sprintf(szBuf,"Error [%s]",ex.what());
			SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] %s\n",__FILE__,__FUNCTION__,__LINE__,szBuf);
			//throw runtime_error(szBuf);
			showMessageBox( szBuf, "Error", false);
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();
	}
}

bool MenuStateConnectedGame::loadFactions(const GameSettings *gameSettings, bool errorOnNoFactions){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	bool foundFactions = false;
	vector<string> results;

	if(gameSettings->getTech() != "") {
		Config &config = Config::getInstance();
		Lang &lang= Lang::getInstance();

		vector<string> techPaths = config.getPathListForType(ptTechs);
		for(int idx = 0; idx < techPaths.size(); idx++) {
			string &techPath = techPaths[idx];
			endPathWithSlash(techPath);
			findAll(techPath + gameSettings->getTech() + "/factions/*.", results, false, false);
			if(results.size() > 0) {
				break;
			}
		}

		if(results.size() == 0) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			NetworkManager &networkManager= NetworkManager::getInstance();
			ClientInterface* clientInterface= networkManager.getClientInterface();
			if(clientInterface->getAllowGameDataSynchCheck() == false) {
				if(errorOnNoFactions == true) {
					throw runtime_error("(2)There are no factions for the tech tree [" + gameSettings->getTech() + "]");
				}
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] (2)There are no factions for the tech tree [%s]\n",__FILE__,__FUNCTION__,__LINE__,gameSettings->getTech().c_str());
			}
			results.push_back(ITEM_MISSING);
			factionFiles = results;
			for(int i=0; i<GameConstants::maxPlayers; ++i){
				listBoxFactions[i].setItems(results);
			}

			if(lastMissingTechtree != gameSettings->getTech() &&
				gameSettings->getTech() != "") {
				lastMissingTechtree = gameSettings->getTech();

		    	Lang &lang= Lang::getInstance();
		    	const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
		    	for(unsigned int i = 0; i < languageList.size(); ++i) {

					char szMsg[1024]="";
					if(lang.hasString("DataMissingTechtree",languageList[i]) == true) {
						sprintf(szMsg,lang.get("DataMissingTechtree",languageList[i]).c_str(),getHumanPlayerName().c_str(),gameSettings->getTech().c_str());
					}
					else {
						sprintf(szMsg,"Player: %s is missing the techtree: %s",getHumanPlayerName().c_str(),gameSettings->getTech().c_str());
					}
					clientInterface->sendTextMessage(szMsg,-1, lang.isLanguageLocal(languageList[i]),languageList[i]);
		    	}
			}

			foundFactions = false;
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		}
		else {
			lastMissingTechtree = "";
			getMissingTechtreeFromFTPServer = "";
			// Add special Observer Faction
			//Lang &lang= Lang::getInstance();
			if(gameSettings->getAllowObservers() == true) {
				results.push_back(formatString(GameConstants::OBSERVER_SLOTNAME));
			}
			results.push_back(formatString(GameConstants::RANDOMFACTION_SLOTNAME));

			factionFiles= results;
			for(int i= 0; i<results.size(); ++i){
				results[i]= formatString(results[i]);

				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"Tech [%s] has faction [%s]\n",gameSettings->getTech().c_str(),results[i].c_str());
			}
			for(int i=0; i<GameConstants::maxPlayers; ++i){
				listBoxFactions[i].setItems(results);
			}

			foundFactions = (results.size() > 0);
		}
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
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
		sprintf(szBuf,"Error [%s]",ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] %s\n",__FILE__,__FUNCTION__,__LINE__,szBuf);
		showMessageBox( szBuf, "Error", false);
	}

    return hasNetworkSlot;
}

void MenuStateConnectedGame::keyDown(char key) {
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

			switchSetupRequestFlagType |= ssrft_NetworkPlayerName;
            needToSetChangedGameSettings = true;
            lastSetChangedGameSettings   = time(NULL);
		}
	}
	else {
		//send key to the chat manager
		chatManager.keyDown(key);
		if(chatManager.getEditEnabled() == false) {
			Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));

			if(key == configKeys.getCharKey("ShowFullConsole")) {
				showFullConsole= true;
			}
			else if(key == configKeys.getCharKey("SaveGUILayout")) {
				bool saved = GraphicComponent::saveAllCustomProperties(containerName);
				Lang &lang= Lang::getInstance();
				console.addLine(lang.get("GUILayoutSaved") + " [" + (saved ? lang.get("Yes") : lang.get("No"))+ "]");
			}
		}
	}
}

void MenuStateConnectedGame::keyPress(char c) {
	if(activeInputLabel != NULL) {
		int maxTextSize= 16;
	    for(int i = 0; i < GameConstants::maxPlayers; ++i) {
			if(&labelPlayerNames[i] == activeInputLabel) {
				if((c>='0' && c<='9') || (c>='a' && c<='z') || (c>='A' && c<='Z') ||
				   (c=='-') || (c=='(') || (c==')')) {
					if(activeInputLabel->getText().size() < maxTextSize) {
						string text= activeInputLabel->getText();
						text.insert(text.end() -1, c);
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

		if(chatManager.getEditEnabled()) {
			//send key to the chat manager
			chatManager.keyUp(key);
		}
		else if(key== configKeys.getCharKey("ShowFullConsole")) {
			showFullConsole= false;
		}
	}
}

void MenuStateConnectedGame::setActiveInputLabel(GraphicLabel *newLable) {
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

string MenuStateConnectedGame::getHumanPlayerName() {
	string result = defaultPlayerName;


	NetworkManager &networkManager= NetworkManager::getInstance();
	ClientInterface* clientInterface= networkManager.getClientInterface();
	for(int j=0; j<GameConstants::maxPlayers; ++j) {
		if(	clientInterface != NULL &&
			j == clientInterface->getPlayerIndex() &&
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
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(enableFactionTexturePreview == true) {
		if(filepath == "") {
			factionTexture=NULL;
		}
		else {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] filepath = [%s]\n",__FILE__,__FUNCTION__,__LINE__,filepath.c_str());
			factionTexture = Renderer::findFactionLogoTexture(filepath);
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		}
	}
}

bool MenuStateConnectedGame::loadMapInfo(string file, MapInfo *mapInfo, bool loadMapPreview) {

	Lang &lang= Lang::getInstance();

	bool mapLoaded = false;
	try {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] map [%s]\n",__FILE__,__FUNCTION__,__LINE__,file.c_str());

		if(file != "") {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			lastMissingMap = file;

			FILE *f= fopen(file.c_str(), "rb");
			if(f==NULL) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
				throw runtime_error("[2]Can't open file");
			}
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			MapFileHeader header;
			size_t readBytes = fread(&header, sizeof(MapFileHeader), 1, f);

			mapInfo->size.x= header.width;
			mapInfo->size.y= header.height;
			mapInfo->players= header.maxFactions;

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
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	    		mapPreview.loadFromFile(file.c_str());
			}

			mapLoaded = true;
		}
		else {
			mapInfo->desc = ITEM_MISSING;

			NetworkManager &networkManager= NetworkManager::getInstance();
			ClientInterface* clientInterface= networkManager.getClientInterface();
			const GameSettings *gameSettings = clientInterface->getGameSettings();

			if(lastMissingMap != gameSettings->getMap()) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

				lastMissingMap = gameSettings->getMap();

		    	Lang &lang= Lang::getInstance();
		    	const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
		    	for(unsigned int i = 0; i < languageList.size(); ++i) {

					char szMsg[1024]="";
		            if(lang.hasString("DataMissingMap",languageList[i]) == true) {
		            	sprintf(szMsg,lang.get("DataMissingMap",languageList[i]).c_str(),getHumanPlayerName().c_str(),gameSettings->getMap().c_str());
		            }
		            else {
		            	sprintf(szMsg,"Player: %s is missing the map: %s",getHumanPlayerName().c_str(),gameSettings->getMap().c_str());
		            }
					clientInterface->sendTextMessage(szMsg,-1, lang.isLanguageLocal(languageList[i]),languageList[i]);
		    	}
			}
		}
	}
	catch(exception &e){
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] error [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());

		showMessageBox( "Error loading map file: "+file+'\n'+e.what(), "Error", false);
	}

	return mapLoaded;
}

void MenuStateConnectedGame::showMessageBox(const string &text, const string &header, bool toggle){
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

void MenuStateConnectedGame::showFTPMessageBox(const string &text, const string &header, bool toggle) {
	if(!toggle) {
		ftpMessageBox.setEnabled(false);
	}

	if(!ftpMessageBox.getEnabled()) {
		ftpMessageBox.setText(text);
		ftpMessageBox.setHeader(header);
		ftpMessageBox.setEnabled(true);
	}
	else {
		ftpMessageBox.setEnabled(false);
	}
}

int32 MenuStateConnectedGame::getNetworkPlayerStatus() {
	int32 result = npst_None;
	switch(listBoxPlayerStatus.getSelectedItemIndex()) {
		case 2:
			result = npst_Ready;
			break;
		case 1:
			result = npst_BeRightBack;
			break;
		case 0:
			result = npst_PickSettings;
			break;
	}

	return result;
}

void MenuStateConnectedGame::FTPClient_CallbackEvent(string itemName,
		FTP_Client_CallbackType type, pair<FTP_Client_ResultType,string> result, void *userdata) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

    Lang &lang= Lang::getInstance();
    if(type == ftp_cct_DownloadProgress) {
        FTPClientCallbackInterface::FtpProgressStats *stats = (FTPClientCallbackInterface::FtpProgressStats *)userdata;
        if(stats != NULL) {
            int fileProgress = 0;
            if(stats->download_total > 0) {
                fileProgress = ((stats->download_now / stats->download_total) * 100.0);
            }
            //if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Got FTP Callback for [%s] current file [%s] fileProgress = %d [now = %f, total = %f]\n",itemName.c_str(),stats->currentFilename.c_str(), fileProgress,stats->download_now,stats->download_total);

            MutexSafeWrapper safeMutexFTPProgress((ftpClientThread != NULL ? ftpClientThread->getProgressMutex() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
            pair<int,string> lastProgress = fileFTPProgressList[itemName];
            fileFTPProgressList[itemName] = pair<int,string>(fileProgress,stats->currentFilename);
            safeMutexFTPProgress.ReleaseLock();

            if(itemName != "" && (lastProgress.first / 25) < (fileProgress / 25)) {
		        NetworkManager &networkManager= NetworkManager::getInstance();
		        ClientInterface* clientInterface= networkManager.getClientInterface();

		    	Lang &lang= Lang::getInstance();
		    	const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
		    	for(unsigned int i = 0; i < languageList.size(); ++i) {
					char szMsg[1024]="";
		            if(lang.hasString("FileDownloadProgress",languageList[i]) == true) {
		            	sprintf(szMsg,lang.get("FileDownloadProgress",languageList[i]).c_str(),getHumanPlayerName().c_str(),itemName.c_str(),fileProgress);
		            }
		            else {
		            	sprintf(szMsg,"Player: %s download progress for [%s] is %d %%",getHumanPlayerName().c_str(),itemName.c_str(),fileProgress);
		            }
		            if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] szMsg [%s] lastProgress.first = %d, fileProgress = %d\n",__FILE__,__FUNCTION__,__LINE__,szMsg,lastProgress.first,fileProgress);
		            clientInterface->sendTextMessage(szMsg,-1, lang.isLanguageLocal(languageList[i]),languageList[i]);
		    	}
            }
        }
    }
    else if(type == ftp_cct_Map) {
        getMissingMapFromFTPServerInProgress = false;
        if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Got FTP Callback for [%s] result = %d [%s]\n",itemName.c_str(),result.first,result.second.c_str());

        MutexSafeWrapper safeMutexFTPProgress((ftpClientThread != NULL ? ftpClientThread->getProgressMutex() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
        fileFTPProgressList.erase(itemName);
        safeMutexFTPProgress.ReleaseLock();

        NetworkManager &networkManager= NetworkManager::getInstance();
        ClientInterface* clientInterface= networkManager.getClientInterface();
        const GameSettings *gameSettings = clientInterface->getGameSettings();

        if(result.first == ftp_crt_SUCCESS) {
            // Clear the CRC file Cache
            Checksum::clearFileCache();
            //lastCheckedCRCMapValue = -1;
    		Checksum checksum;
    		string file = Map::getMapPath(itemName,"",false);
    		//console.addLine("Checking map CRC [" + file + "]");
    		checksum.addFile(file);
    		lastCheckedCRCMapValue = checksum.getSum();

	    	Lang &lang= Lang::getInstance();
	    	const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
	    	for(unsigned int i = 0; i < languageList.size(); ++i) {
				char szMsg[1024]="";
	            if(lang.hasString("DataMissingMapSuccessDownload",languageList[i]) == true) {
	            	sprintf(szMsg,lang.get("DataMissingMapSuccessDownload",languageList[i]).c_str(),getHumanPlayerName().c_str(),itemName.c_str());
	            }
	            else {
	            	sprintf(szMsg,"Player: %s SUCCESSFULLY downloaded the map: %s",getHumanPlayerName().c_str(),itemName.c_str());
	            }
	            clientInterface->sendTextMessage(szMsg,-1, lang.isLanguageLocal(languageList[i]),languageList[i]);
	    	}
        }
        else {
            curl_version_info_data *curlVersion= curl_version_info(CURLVERSION_NOW);

	    	Lang &lang= Lang::getInstance();
	    	const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
	    	for(unsigned int i = 0; i < languageList.size(); ++i) {
				char szMsg[1024]="";
	            if(lang.hasString("DataMissingMapFailDownload",languageList[i]) == true) {
	            	sprintf(szMsg,lang.get("DataMissingMapFailDownload",languageList[i]).c_str(),getHumanPlayerName().c_str(),itemName.c_str(),curlVersion->version);
	            }
	            else {
	            	sprintf(szMsg,"Player: %s FAILED to download the map: [%s] using CURL version [%s]",getHumanPlayerName().c_str(),itemName.c_str(),curlVersion->version);
	            }
	            clientInterface->sendTextMessage(szMsg,-1, lang.isLanguageLocal(languageList[i]),languageList[i]);
	    	}

            console.addLine(result.second,true);
        }
    }
    else if(type == ftp_cct_Tileset) {
        getMissingTilesetFromFTPServerInProgress = false;
        if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Got FTP Callback for [%s] result = %d [%s]\n",itemName.c_str(),result.first,result.second.c_str());

        MutexSafeWrapper safeMutexFTPProgress((ftpClientThread != NULL ? ftpClientThread->getProgressMutex() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
        fileFTPProgressList.erase(itemName);
        safeMutexFTPProgress.ReleaseLock(true);

        NetworkManager &networkManager= NetworkManager::getInstance();
        ClientInterface* clientInterface= networkManager.getClientInterface();
        const GameSettings *gameSettings = clientInterface->getGameSettings();

        if(result.first == ftp_crt_SUCCESS) {

	    	Lang &lang= Lang::getInstance();
	    	const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
	    	for(unsigned int i = 0; i < languageList.size(); ++i) {
				char szMsg[1024]="";
	            if(lang.hasString("DataMissingTilesetSuccessDownload",languageList[i]) == true) {
	            	sprintf(szMsg,lang.get("DataMissingTilesetSuccessDownload",languageList[i]).c_str(),getHumanPlayerName().c_str(),itemName.c_str());
	            }
	            else {
	            	sprintf(szMsg,"Player: %s SUCCESSFULLY downloaded the tileset: %s",getHumanPlayerName().c_str(),itemName.c_str());
	            }
	            clientInterface->sendTextMessage(szMsg,-1, lang.isLanguageLocal(languageList[i]),languageList[i]);
	    	}

            // START
            // Clear the CRC Cache if it is populated
            //
            // Clear the CRC file Cache
            safeMutexFTPProgress.Lock();
            Checksum::clearFileCache();

            vector<string> paths        = Config::getInstance().getPathListForType(ptTilesets);
            string pathSearchString     = string("/") + itemName + string("/*");
            const string filterFileExt  = ".xml";
            clearFolderTreeContentsCheckSum(paths, pathSearchString, filterFileExt);
            clearFolderTreeContentsCheckSumList(paths, pathSearchString, filterFileExt);

            // Refresh CRC
            Config &config = Config::getInstance();
            lastCheckedCRCTilesetValue = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTilesets,""), string("/") + itemName + string("/*"), ".xml", NULL, true);

            safeMutexFTPProgress.ReleaseLock();
            // END

            // Reload tilesets for the UI
            findDirs(Config::getInstance().getPathListForType(ptTilesets), tilesetFiles);
        }
        else {
            curl_version_info_data *curlVersion= curl_version_info(CURLVERSION_NOW);

	    	Lang &lang= Lang::getInstance();
	    	const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
	    	for(unsigned int i = 0; i < languageList.size(); ++i) {
				char szMsg[1024]="";
	            if(lang.hasString("DataMissingTilesetFailDownload",languageList[i]) == true) {
	            	sprintf(szMsg,lang.get("DataMissingTilesetFailDownload",languageList[i]).c_str(),getHumanPlayerName().c_str(),itemName.c_str(),curlVersion->version);
	            }
	            else {
	            	sprintf(szMsg,"Player: %s FAILED to download the tileset: [%s] using CURL version [%s]",getHumanPlayerName().c_str(),itemName.c_str(),curlVersion->version);
	            }
	            clientInterface->sendTextMessage(szMsg,-1, lang.isLanguageLocal(languageList[i]),languageList[i]);
	    	}

            console.addLine(result.second,true);
        }
    }
    else if(type == ftp_cct_Techtree) {
        getMissingTechtreeFromFTPServerInProgress = false;
        if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Got FTP Callback for [%s] result = %d [%s]\n",itemName.c_str(),result.first,result.second.c_str());

        MutexSafeWrapper safeMutexFTPProgress((ftpClientThread != NULL ? ftpClientThread->getProgressMutex() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
        fileFTPProgressList.erase(itemName);
        safeMutexFTPProgress.ReleaseLock(true);

        NetworkManager &networkManager= NetworkManager::getInstance();
        ClientInterface* clientInterface= networkManager.getClientInterface();
        const GameSettings *gameSettings = clientInterface->getGameSettings();

        if(result.first == ftp_crt_SUCCESS) {

	    	Lang &lang= Lang::getInstance();
	    	const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
	    	for(unsigned int i = 0; i < languageList.size(); ++i) {
				char szMsg[1024]="";
	            if(lang.hasString("DataMissingTechtreeSuccessDownload",languageList[i]) == true) {
	            	sprintf(szMsg,lang.get("DataMissingTechtreeSuccessDownload",languageList[i]).c_str(),getHumanPlayerName().c_str(),itemName.c_str());
	            }
	            else {
	            	sprintf(szMsg,"Player: %s SUCCESSFULLY downloaded the techtree: %s",getHumanPlayerName().c_str(),itemName.c_str());
	            }
	            clientInterface->sendTextMessage(szMsg,-1, lang.isLanguageLocal(languageList[i]),languageList[i]);
	    	}

            // START
            // Clear the CRC Cache if it is populated
            //
            // Clear the CRC file Cache
            safeMutexFTPProgress.Lock();
            Checksum::clearFileCache();

            vector<string> paths        = Config::getInstance().getPathListForType(ptTechs);
            string pathSearchString     = string("/") + itemName + string("/*");
            const string filterFileExt  = ".xml";
            clearFolderTreeContentsCheckSum(paths, pathSearchString, filterFileExt);
            clearFolderTreeContentsCheckSumList(paths, pathSearchString, filterFileExt);

            // Refresh CRC
            Config &config = Config::getInstance();
            lastCheckedCRCTechtreeValue = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTechs,""), string("/") + itemName + string("/*"), ".xml", NULL, true);

            safeMutexFTPProgress.ReleaseLock();
            // END

            // Reload tilesets for the UI
            findDirs(Config::getInstance().getPathListForType(ptTechs), techTreeFiles);
        }
        else {
            curl_version_info_data *curlVersion= curl_version_info(CURLVERSION_NOW);

	    	Lang &lang= Lang::getInstance();
	    	const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
	    	for(unsigned int i = 0; i < languageList.size(); ++i) {
				char szMsg[1024]="";
	            if(lang.hasString("DataMissingTechtreeFailDownload",languageList[i]) == true) {
	            	sprintf(szMsg,lang.get("DataMissingTechtreeFailDownload",languageList[i]).c_str(),getHumanPlayerName().c_str(),itemName.c_str(),curlVersion->version);
	            }
	            else {
	            	sprintf(szMsg,"Player: %s FAILED to download the techtree: [%s] using CURL version [%s]",getHumanPlayerName().c_str(),itemName.c_str(),curlVersion->version);
	            }
	            clientInterface->sendTextMessage(szMsg,-1, lang.isLanguageLocal(languageList[i]),languageList[i]);
	    	}

            console.addLine(result.second,true);
        }
    }
}

}}//end namespace
