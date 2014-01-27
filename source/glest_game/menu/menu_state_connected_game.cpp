// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Martiño Figueroa
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
#include "string_utils.h"
#include "map_preview.h"
#include <iterator>
#include "compression_utils.h"

#include "leak_dumper.h"

using namespace Shared::Util;
using namespace Shared::CompressionUtil;

namespace Glest{ namespace Game{

static const int MAX_PING_LAG_COUNT		= 6;
static const double REPROMPT_DOWNLOAD_SECONDS		= 7;
//static const string ITEM_MISSING 					= "***missing***";
// above replaced with Lang::getInstance().getString("DataMissing","",true)
const int HEADLESSSERVER_BROADCAST_SETTINGS_SECONDS  	= 4;
static const char *HEADLESS_SAVED_GAME_FILENAME 	= "lastHeadlessGameSettings.mgg";

const int mapPreviewTexture_X = 5;
const int mapPreviewTexture_Y = 185;
const int mapPreviewTexture_W = 150;
const int mapPreviewTexture_H = 150;

struct FormatString {
	void operator()(string &s) {
		s = formatString(s);
	}
};

// =====================================================
// 	class MenuStateConnectedGame
// =====================================================

MenuStateConnectedGame::MenuStateConnectedGame(Program *program, MainMenu *mainMenu,JoinMenu joinMenuInfo, bool openNetworkSlots) :
	MenuState(program, mainMenu, "connected-game"), modHttpServerThread(NULL)
{
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	containerName = "ClientConnectedGame";
	switchSetupRequestFlagType |= ssrft_NetworkPlayerName;
	updateDataSynchDetailText = false;
	launchingNewGame = false;

	this->zoomedMap = false;
	this->render_mapPreviewTexture_X = mapPreviewTexture_X;
	this->render_mapPreviewTexture_Y = mapPreviewTexture_Y;
	this->render_mapPreviewTexture_W = mapPreviewTexture_W;
	this->render_mapPreviewTexture_H = mapPreviewTexture_H;

	needToBroadcastServerSettings=false;
	broadcastServerSettingsDelayTimer=0;
	lastGameSettingsReceivedCount=0;

	soundConnectionCount=0;

	this->factionVideo = NULL;
	factionVideoSwitchedOffVolume=false;
	currentTechName_factionPreview="";
	currentFactionName_factionPreview="";

	ftpClientThread                             = NULL;
    ftpMissingDataType                          = ftpmsg_MissingNone;
	getMissingMapFromFTPServer                  = "";
	getMissingMapFromFTPServerLastPrompted		= 0;
	getMissingMapFromFTPServerInProgress        = false;
	getMissingTilesetFromFTPServer              = "";
	getMissingTilesetFromFTPServerLastPrompted	= 0;
	getMissingTilesetFromFTPServerInProgress    = false;
    getMissingTechtreeFromFTPServer				= "";
    getMissingTechtreeFromFTPServerLastPrompted	= 0;
    getMissingTechtreeFromFTPServerInProgress	= false;

    getInProgressSavedGameFromFTPServer			  = "";
    getInProgressSavedGameFromFTPServerInProgress = false;
    readyToJoinInProgressGame					  = false;

    lastCheckedCRCTilesetName					= "";
    lastCheckedCRCTechtreeName					= "";
    lastCheckedCRCMapName						= "";
    lastCheckedCRCTilesetValue					= 0;
    lastCheckedCRCTechtreeValue					= 0;
    lastCheckedCRCMapValue						= 0;

    mapPreviewTexture=NULL;
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
	mainMessageBox.init(lang.getString("Ok"));
	mainMessageBox.setEnabled(false);

    ftpMessageBox.registerGraphicComponent(containerName,"ftpMessageBox");
	ftpMessageBox.init(lang.getString("ModCenter"),lang.getString("GameHost"));
	ftpMessageBox.addButton(lang.getString("NoDownload"));
	ftpMessageBox.setEnabled(false);

	NetworkManager &networkManager= NetworkManager::getInstance();
    Config &config = Config::getInstance();
    defaultPlayerName = config.getString("NetPlayerName",Socket::getHostName().c_str());
    enableFactionTexturePreview = config.getBool("FactionPreview","true");
    enableMapPreview = config.getBool("MapPreview","true");

	enableScenarioTexturePreview = Config::getInstance().getBool("EnableScenarioTexturePreview","true");
	scenarioLogoTexture=NULL;
	previewLoadDelayTimer=time(NULL);
	needToLoadTextures=true;
	this->dirList = Config::getInstance().getPathListForType(ptScenarios);

    vector<string> techtreesList = Config::getInstance().getPathListForType(ptTechs);
    techTree.reset(new TechTree(techtreesList));

	vector<string> teamItems, controlItems, results, rMultiplier, playerStatuses;
	int labelOffset=23;
	int setupPos=590;
	int mapHeadPos=330;
	int mapPos=mapHeadPos-labelOffset;
	int aHeadPos=240;
	int aPos=aHeadPos-labelOffset;
	int networkHeadPos=700;

	//state
	labelStatus.registerGraphicComponent(containerName,"labelStatus");
	labelStatus.init(30, networkHeadPos);
	labelStatus.setText("");

	labelInfo.registerGraphicComponent(containerName,"labelInfo");
	labelInfo.init(30, networkHeadPos+30);
	labelInfo.setText("");
	labelInfo.setFont(CoreData::getInstance().getMenuFontBig());
	labelInfo.setFont3D(CoreData::getInstance().getMenuFontBig3D());

    timerLabelFlash = time(NULL);
    labelDataSynchInfo.registerGraphicComponent(containerName,"labelDataSynchInfo");
	labelDataSynchInfo.init(30, networkHeadPos-60);
	labelDataSynchInfo.setText("");
	labelDataSynchInfo.setFont(CoreData::getInstance().getMenuFontBig());
	labelDataSynchInfo.setFont3D(CoreData::getInstance().getMenuFontBig3D());

	// fog - o - war
	int xoffset=70;
	labelFogOfWar.registerGraphicComponent(containerName,"labelFogOfWar");
	labelFogOfWar.init(xoffset+100, aHeadPos, 130);
	labelFogOfWar.setText(lang.getString("FogOfWar"));

	listBoxFogOfWar.registerGraphicComponent(containerName,"listBoxFogOfWar");
	listBoxFogOfWar.init(xoffset+100, aPos, 130);
	listBoxFogOfWar.pushBackItem(lang.getString("Enabled"));
	listBoxFogOfWar.pushBackItem(lang.getString("Explored"));
	listBoxFogOfWar.pushBackItem(lang.getString("Disabled"));
	listBoxFogOfWar.setSelectedItemIndex(0);
	listBoxFogOfWar.setEditable(false);


	labelAllowObservers.registerGraphicComponent(containerName,"labelAllowObservers");
	labelAllowObservers.init(xoffset+310, aHeadPos, 80);
	labelAllowObservers.setText(lang.getString("AllowObservers"));

	checkBoxAllowObservers.registerGraphicComponent(containerName,"checkBoxAllowObservers");
	checkBoxAllowObservers.init(xoffset+310, aPos);
	checkBoxAllowObservers.setValue(false);
	checkBoxAllowObservers.setEditable(false);

	labelAllowTeamUnitSharing.registerGraphicComponent(containerName,"labelAllowTeamUnitSharing");
	labelAllowTeamUnitSharing.init(xoffset+410, 670, 80);
	labelAllowTeamUnitSharing.setText(lang.getString("AllowTeamUnitSharing"));
	labelAllowTeamUnitSharing.setVisible(true);

	checkBoxAllowTeamUnitSharing.registerGraphicComponent(containerName,"checkBoxAllowTeamUnitSharing");
	checkBoxAllowTeamUnitSharing.init(xoffset+600, 670);
	checkBoxAllowTeamUnitSharing.setValue(false);
	checkBoxAllowTeamUnitSharing.setVisible(true);
	checkBoxAllowTeamUnitSharing.setEditable(false);

	labelAllowTeamResourceSharing.registerGraphicComponent(containerName,"labelAllowTeamResourceSharing");
	labelAllowTeamResourceSharing.init(xoffset+410, 640, 80);
	labelAllowTeamResourceSharing.setText(lang.getString("AllowTeamResourceSharing"));
	labelAllowTeamResourceSharing.setVisible(true);

	checkBoxAllowTeamResourceSharing.registerGraphicComponent(containerName,"checkBoxAllowTeamResourceSharing");
	checkBoxAllowTeamResourceSharing.init(xoffset+600, 640);
	checkBoxAllowTeamResourceSharing.setValue(false);
	checkBoxAllowTeamResourceSharing.setVisible(true);
	checkBoxAllowTeamResourceSharing.setEditable(false);

	for(int i=0; i<45; ++i){
		rMultiplier.push_back(floatToStr(0.5f+0.1f*i,1));
	}

	labelFallbackCpuMultiplier.registerGraphicComponent(containerName,"labelFallbackCpuMultiplier");
	labelFallbackCpuMultiplier.init(xoffset+460, aHeadPos, 80);
	labelFallbackCpuMultiplier.setText(lang.getString("FallbackCpuMultiplier"));

	listBoxFallbackCpuMultiplier.registerGraphicComponent(containerName,"listBoxFallbackCpuMultiplier");
	listBoxFallbackCpuMultiplier.init(xoffset+460, aPos, 80);
	listBoxFallbackCpuMultiplier.setItems(rMultiplier);
	listBoxFallbackCpuMultiplier.setSelectedItem("1.0");


	// Allow Switch Team Mode
	labelEnableSwitchTeamMode.registerGraphicComponent(containerName,"labelEnableSwitchTeamMode");
	labelEnableSwitchTeamMode.init(xoffset+310, aHeadPos+45, 80);
	labelEnableSwitchTeamMode.setText(lang.getString("EnableSwitchTeamMode"));

	checkBoxEnableSwitchTeamMode.registerGraphicComponent(containerName,"checkBoxEnableSwitchTeamMode");
	checkBoxEnableSwitchTeamMode.init(xoffset+310, aPos+45);
	checkBoxEnableSwitchTeamMode.setValue(false);
	checkBoxEnableSwitchTeamMode.setEditable(false);

	labelAISwitchTeamAcceptPercent.registerGraphicComponent(containerName,"labelAISwitchTeamAcceptPercent");
	labelAISwitchTeamAcceptPercent.init(xoffset+460, aHeadPos+45, 80);
	labelAISwitchTeamAcceptPercent.setText(lang.getString("AISwitchTeamAcceptPercent"));

	listBoxAISwitchTeamAcceptPercent.registerGraphicComponent(containerName,"listBoxAISwitchTeamAcceptPercent");
	listBoxAISwitchTeamAcceptPercent.init(xoffset+460, aPos+45, 80);
	for(int i = 0; i <= 100; i = i + 10) {
		listBoxAISwitchTeamAcceptPercent.pushBackItem(intToStr(i));
	}
	listBoxAISwitchTeamAcceptPercent.setSelectedItem(intToStr(30));
	listBoxAISwitchTeamAcceptPercent.setEditable(false);

	//create
	buttonCancelDownloads.registerGraphicComponent(containerName,"buttonCancelDownloads");
	buttonCancelDownloads.init(xoffset+620, 180, 150);
	buttonCancelDownloads.setText(lang.getString("CancelDownloads"));

	listBoxPlayerStatus.registerGraphicComponent(containerName,"listBoxPlayerStatus");
	nonAdminPlayerStatusX = xoffset+460;
	listBoxPlayerStatus.init(nonAdminPlayerStatusX, 180, 150);
	listBoxPlayerStatus.setTextColor(Vec3f(1.0f,0.f,0.f));
	listBoxPlayerStatus.setLighted(true);
	playerStatuses.push_back(lang.getString("PlayerStatusSetup"));
	playerStatuses.push_back(lang.getString("PlayerStatusBeRightBack"));
	playerStatuses.push_back(lang.getString("PlayerStatusReady"));
	listBoxPlayerStatus.setItems(playerStatuses);

	// Network Frame Period
	xoffset=70;
    //map listBox
	// put them all in a set, to weed out duplicates (gbm & mgm with same name)
	// will also ensure they are alphabetically listed (rather than how the OS provides them)
	listBoxMap.registerGraphicComponent(containerName,"listBoxMap");
	listBoxMap.init(xoffset+100, mapPos, 200);
	listBoxMap.setEditable(false);

    labelMapInfo.registerGraphicComponent(containerName,"labelMapInfo");
	labelMapInfo.init(xoffset+100, mapPos-labelOffset-10, 200, 40);
    labelMapInfo.setText("?");

	labelMap.registerGraphicComponent(containerName,"labelMap");
	labelMap.init(xoffset+100, mapHeadPos);
	labelMap.setText(lang.getString("Map"));

    //tileset listBox
	listBoxTileset.registerGraphicComponent(containerName,"listBoxTileset");
	listBoxTileset.init(xoffset+460, mapPos, 150);
	listBoxTileset.setEditable(false);

	labelTileset.registerGraphicComponent(containerName,"labelTileset");
	labelTileset.init(xoffset+460, mapHeadPos);
	labelTileset.setText(lang.getString("Tileset"));


    //tech Tree listBox
	listBoxTechTree.setEditable(false);

	listBoxTechTree.registerGraphicComponent(containerName,"listBoxTechTree");
	listBoxTechTree.init(xoffset+620, mapPos, 150);

	labelTechTree.registerGraphicComponent(containerName,"labelTechTree");
	labelTechTree.init(xoffset+620, mapHeadPos);
	labelTechTree.setText(lang.getString("TechTree"));

	labelAllowNativeLanguageTechtree.registerGraphicComponent(containerName,"labelAllowNativeLanguageTechtree");
	labelAllowNativeLanguageTechtree.init(xoffset+620, mapHeadPos-45);
	labelAllowNativeLanguageTechtree.setText(lang.getString("AllowNativeLanguageTechtree"));

	checkBoxAllowNativeLanguageTechtree.registerGraphicComponent(containerName,"checkBoxAllowNativeLanguageTechtree");
	checkBoxAllowNativeLanguageTechtree.init(xoffset+620, mapHeadPos-65);
	checkBoxAllowNativeLanguageTechtree.setValue(false);
	checkBoxAllowNativeLanguageTechtree.setEditable(false);
	checkBoxAllowNativeLanguageTechtree.setEnabled(false);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	xoffset=30;
	int rowHeight=27;
    for(int i=0; i<GameConstants::maxPlayers; ++i){
    	labelPlayers[i].registerGraphicComponent(containerName,"labelPlayers" + intToStr(i));
		labelPlayers[i].init(xoffset, setupPos-30-i*rowHeight+2);
		labelPlayers[i].setFont(CoreData::getInstance().getMenuFontBig());
		labelPlayers[i].setFont3D(CoreData::getInstance().getMenuFontBig3D());
		labelPlayers[i].setEditable(false);

		labelPlayerStatus[i].registerGraphicComponent(containerName,"labelPlayerStatus" + intToStr(i));
		labelPlayerStatus[i].init(xoffset+15, setupPos-30-i*rowHeight+2, 60);
		labelPlayerNames[i].registerGraphicComponent(containerName,"labelPlayerNames" + intToStr(i));
		labelPlayerNames[i].init(xoffset+30,setupPos-30-i*rowHeight);

		listBoxControls[i].registerGraphicComponent(containerName,"listBoxControls" + intToStr(i));
        listBoxControls[i].init(xoffset+170, setupPos-30-i*rowHeight);
        listBoxControls[i].setEditable(false);

        listBoxRMultiplier[i].registerGraphicComponent(containerName,"listBoxRMultiplier" + intToStr(i));
        listBoxRMultiplier[i].init(xoffset+310, setupPos-30-i*rowHeight,70);
        listBoxRMultiplier[i].setEditable(false);

        listBoxFactions[i].registerGraphicComponent(containerName,"listBoxFactions" + intToStr(i));
        listBoxFactions[i].init(xoffset+390, setupPos-30-i*rowHeight, 250);
        listBoxFactions[i].setLeftControlled(true);
        listBoxFactions[i].setEditable(false);

        listBoxTeams[i].registerGraphicComponent(containerName,"listBoxTeams" + intToStr(i));
		listBoxTeams[i].init(xoffset+650, setupPos-30-i*rowHeight, 60);
		listBoxTeams[i].setEditable(false);

		labelNetStatus[i].registerGraphicComponent(containerName,"labelNetStatus" + intToStr(i));
		labelNetStatus[i].init(xoffset+715, setupPos-30-i*rowHeight, 60);
		labelNetStatus[i].setFont(CoreData::getInstance().getDisplayFontSmall());
		labelNetStatus[i].setFont3D(CoreData::getInstance().getDisplayFontSmall3D());

		grabSlotButton[i].registerGraphicComponent(containerName,"grabSlotButton" + intToStr(i));
		grabSlotButton[i].init(xoffset+720, setupPos-30-i*rowHeight, 30);
		grabSlotButton[i].setText(">");
    }

    labelControl.registerGraphicComponent(containerName,"labelControl");
	labelControl.init(xoffset+170, setupPos, GraphicListBox::defW, GraphicListBox::defH, true);
	labelControl.setText(lang.getString("Control"));

	labelRMultiplier.registerGraphicComponent(containerName,"labelRMultiplier");
	labelRMultiplier.init(xoffset+310, setupPos, GraphicListBox::defW, GraphicListBox::defH, true);

	labelFaction.registerGraphicComponent(containerName,"labelFaction");
    labelFaction.init(xoffset+390, setupPos, GraphicListBox::defW, GraphicListBox::defH, true);
    labelFaction.setText(lang.getString("Faction"));

    labelTeam.registerGraphicComponent(containerName,"labelTeam");
    labelTeam.init(xoffset+650, setupPos, 60, GraphicListBox::defH, true);
	labelTeam.setText(lang.getString("Team"));

    labelControl.setFont(CoreData::getInstance().getMenuFontBig());
    labelControl.setFont3D(CoreData::getInstance().getMenuFontBig3D());
	labelFaction.setFont(CoreData::getInstance().getMenuFontBig());
	labelFaction.setFont3D(CoreData::getInstance().getMenuFontBig3D());
	labelTeam.setFont(CoreData::getInstance().getMenuFontBig());
	labelTeam.setFont3D(CoreData::getInstance().getMenuFontBig3D());

	//texts
	buttonDisconnect.setText(lang.getString("Return"));

    controlItems.push_back(lang.getString("Closed"));
	controlItems.push_back(lang.getString("CpuEasy"));
	controlItems.push_back(lang.getString("Cpu"));
    controlItems.push_back(lang.getString("CpuUltra"));
    controlItems.push_back(lang.getString("CpuMega"));
	controlItems.push_back(lang.getString("Network"));
	controlItems.push_back(lang.getString("NetworkUnassigned"));
	controlItems.push_back(lang.getString("Human"));


	if(config.getBool("EnableNetworkCpu","false") == true) {
		controlItems.push_back(lang.getString("NetworkCpuEasy"));
		controlItems.push_back(lang.getString("NetworkCpu"));
	    controlItems.push_back(lang.getString("NetworkCpuUltra"));
	    controlItems.push_back(lang.getString("NetworkCpuMega"));
	}

	for(int i = 1; i <= GameConstants::maxPlayers; ++i) {
		teamItems.push_back(intToStr(i));
	}
	for(int i = GameConstants::maxPlayers + 1; i <= GameConstants::maxPlayers + GameConstants::specialFactions; ++i) {
		teamItems.push_back(intToStr(i));
	}

	for(int i=0; i<GameConstants::maxPlayers; ++i){
		labelPlayerStatus[i].setText("");
		labelPlayerStatus[i].setTexture(NULL);
		labelPlayerStatus[i].setH(16);
		labelPlayerStatus[i].setW(12);

		labelPlayers[i].setText(intToStr(i+1));
		labelPlayerNames[i].setText("");
		labelPlayerNames[i].setMaxEditWidth(16);
		labelPlayerNames[i].setMaxEditRenderWidth(135);

        listBoxTeams[i].setItems(teamItems);
		listBoxTeams[i].setSelectedItemIndex(i);
		listBoxControls[i].setItems(controlItems);
		listBoxRMultiplier[i].setItems(rMultiplier);
		listBoxRMultiplier[i].setSelectedItem("1.0");

		labelNetStatus[i].setText("V");
    }
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	//init controllers
	listBoxControls[0].setSelectedItemIndex(ctHuman);


    //map listBox
	// put them all in a set, to weed out duplicates (gbm & mgm with same name)
	// will also ensure they are alphabetically listed (rather than how the OS provides them)
	setupMapList("");
    listBoxMap.setItems(formattedPlayerSortedMaps[0]);

	buttonPlayNow.registerGraphicComponent(containerName,"buttonPlayNow");
	buttonPlayNow.init(220, 180, 125);
	buttonPlayNow.setText(lang.getString("PlayNow"));
	buttonPlayNow.setVisible(false);

	buttonDisconnect.registerGraphicComponent(containerName,"buttonDisconnect");
	buttonDisconnect.init(350, 180, 125);

	buttonRestoreLastSettings.registerGraphicComponent(containerName,"buttonRestoreLastSettings");
	buttonRestoreLastSettings.init(480, 180, 220);
	buttonRestoreLastSettings.setText(lang.getString("ReloadLastGameSettings"));

	// write hint to console:
	Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));

	console.addLine(lang.getString("ToSwitchOffMusicPress") + " - \"" + configKeys.getString("ToggleMusic") + "\"");
	chatManager.init(&console, -1,true);

	GraphicComponent::applyAllCustomProperties(containerName);

	//tileset listBox
	setupTilesetList("");

	int initialTechSelection = setupTechList("",true);
	listBoxTechTree.setSelectedItemIndex(initialTechSelection);

	int scenarioX=810;
	int scenarioY=140;
    labelScenario.registerGraphicComponent(containerName,"labelScenario");
    labelScenario.init(scenarioX, scenarioY);
    labelScenario.setText(lang.getString("Scenario"));
	listBoxScenario.registerGraphicComponent(containerName,"listBoxScenario");
    listBoxScenario.init(scenarioX, scenarioY-30,190);
    checkBoxScenario.registerGraphicComponent(containerName,"checkBoxScenario");
    checkBoxScenario.init(scenarioX+90, scenarioY);
    checkBoxScenario.setValue(false);

    //scenario listbox
    vector<string> resultsScenarios;
	findDirs(dirList, resultsScenarios);
	// Filter out only scenarios with no network slots
	for(int i= 0; i < (int)resultsScenarios.size(); ++i) {
		string scenario = resultsScenarios[i];
		string file = Scenario::getScenarioPath(dirList, scenario);

		try {
			if(file != "") {
				bool isTutorial = Scenario::isGameTutorial(file);
				Scenario::loadScenarioInfo(file, &scenarioInfo, isTutorial);

				bool isNetworkScenario = false;
				for(unsigned int j = 0; isNetworkScenario == false && j < (unsigned int)GameConstants::maxPlayers; ++j) {
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
		    snprintf(szBuf,8096,"In [%s::%s %d]\nError loading scenario [%s]:\n%s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,scenario.c_str(),ex.what());
		    SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);

		    showMessageBox( szBuf, "Error", false);
		    //throw megaglest_runtime_error(szBuf);
		}
	}
	resultsScenarios.clear();
	for(int i = 0; i < (int)scenarioFiles.size(); ++i) {
		resultsScenarios.push_back(formatString(scenarioFiles[i]));
	}
    listBoxScenario.setItems(resultsScenarios);
    checkBoxScenario.setEnabled(false);

    if(config.getBool("EnableFTPXfer","true") == true) {
        ClientInterface *clientInterface = networkManager.getClientInterface();
        string serverUrl = clientInterface->getServerIpAddress();
        int portNumber   = clientInterface->getServerFTPPort();

        if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] Using FTP port #: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,portNumber);

        vector<string> mapPathList = config.getPathListForType(ptMaps);
        std::pair<string,string> mapsPath;
        if(mapPathList.empty() == false) {
            mapsPath.first = mapPathList[0];
        }
        if(mapPathList.size() > 1) {
            mapsPath.second = mapPathList[1];
        }
        std::pair<string,string> tilesetsPath;
        vector<string> tilesetsList = Config::getInstance().getPathListForType(ptTilesets);
        if(tilesetsList.empty() == false) {
            tilesetsPath.first = tilesetsList[0];
            if(tilesetsList.size() > 1) {
                tilesetsPath.second = tilesetsList[1];
            }
        }

        std::pair<string,string> techtreesPath;
        if(techtreesList.empty() == false) {
        	techtreesPath.first = techtreesList[0];
            if(techtreesList.size() > 1) {
            	techtreesPath.second = techtreesList[1];
            }
        }

        std::pair<string,string> scenariosPath;
        vector<string> scenariosList = Config::getInstance().getPathListForType(ptScenarios);
        if(scenariosList.empty() == false) {
        	scenariosPath.first = scenariosList[0];
            if(scenariosList.size() > 1) {
            	scenariosPath.second = scenariosList[1];
            }
        }

        string fileArchiveExtension = config.getString("FileArchiveExtension","");
        string fileArchiveExtractCommand = config.getString("FileArchiveExtractCommand","");
        string fileArchiveExtractCommandParameters = config.getString("FileArchiveExtractCommandParameters","");
        int32 fileArchiveExtractCommandSuccessResult = config.getInt("FileArchiveExtractCommandSuccessResult","0");

    	// Get path to temp files
    	string tempFilePath = "temp/";
    	if(getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) != "") {
    		tempFilePath = getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) + tempFilePath;
    	}
    	else {
            string userData = config.getString("UserData_Root","");
            if(userData != "") {
            	endPathWithSlash(userData);
            }
            tempFilePath = userData + tempFilePath;
    	}
    	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Temp files path [%s]\n",tempFilePath.c_str());

        ftpClientThread = new FTPClientThread(portNumber,serverUrl,
        		mapsPath,tilesetsPath,techtreesPath,scenariosPath,
        		this,fileArchiveExtension,fileArchiveExtractCommand,
        		fileArchiveExtractCommandParameters,
        		fileArchiveExtractCommandSuccessResult,
        		tempFilePath);
        ftpClientThread->start();
    }
	// Start http meta data thread
    static string mutexOwnerId = string(extractFileFromDirectoryPath(__FILE__).c_str()) + string("_") + intToStr(__LINE__);
	modHttpServerThread = new SimpleTaskThread(this,0,200);
	modHttpServerThread->setUniqueID(mutexOwnerId);
	modHttpServerThread->start();

	ClientInterface *clientInterface = networkManager.getClientInterface();
	if(clientInterface != NULL && clientInterface->getJoinGameInProgress() == true) {
		listBoxPlayerStatus.setVisible(false);
		Lang &lang= Lang::getInstance();
    	const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
    	for(unsigned int i = 0; i < languageList.size(); ++i) {
			char szMsg[8096]="";
			if(lang.hasString("JoinPlayerToCurrentGameWelcome",languageList[i]) == true) {
				snprintf(szMsg,8096,lang.getString("JoinPlayerToCurrentGameWelcome",languageList[i]).c_str(),getHumanPlayerName().c_str());
			}
			else {
				snprintf(szMsg,8096,"Player: %s has connected to the game and would like to join.",getHumanPlayerName().c_str());
			}
			bool localEcho = lang.isLanguageLocal(languageList[i]);
			clientInterface->sendTextMessage(szMsg,-1, localEcho,languageList[i]);
    	}
    	sleep(1);
	}

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

void MenuStateConnectedGame::reloadUI() {
	Config &config = Config::getInstance();
	Lang &lang= Lang::getInstance();

	console.resetFonts();
	mainMessageBox.init(lang.getString("Ok"));
	ftpMessageBox.init(lang.getString("ModCenter"),lang.getString("GameHost"));
	ftpMessageBox.addButton(lang.getString("NoDownload"));

	labelInfo.setFont(CoreData::getInstance().getMenuFontBig());
	labelInfo.setFont3D(CoreData::getInstance().getMenuFontBig3D());

	labelDataSynchInfo.setFont(CoreData::getInstance().getMenuFontBig());
	labelDataSynchInfo.setFont3D(CoreData::getInstance().getMenuFontBig3D());

	buttonCancelDownloads.setText(lang.getString("CancelDownloads"));

	labelFogOfWar.setText(lang.getString("FogOfWar"));

	vector<string> fowItems;
	fowItems.push_back(lang.getString("Enabled"));
	fowItems.push_back(lang.getString("Explored"));
	fowItems.push_back(lang.getString("Disabled"));
	listBoxFogOfWar.setItems(fowItems);

	labelAllowObservers.setText(lang.getString("AllowObservers"));
	labelFallbackCpuMultiplier.setText(lang.getString("FallbackCpuMultiplier"));

	labelEnableSwitchTeamMode.setText(lang.getString("EnableSwitchTeamMode"));

	labelAllowTeamUnitSharing.setText(lang.getString("AllowTeamUnitSharing"));
	labelAllowTeamResourceSharing.setText(lang.getString("AllowTeamResourceSharing"));

	labelAISwitchTeamAcceptPercent.setText(lang.getString("AISwitchTeamAcceptPercent"));

	vector<string> aiswitchteamModeItems;
	for(int i = 0; i <= 100; i = i + 10) {
		aiswitchteamModeItems.push_back(intToStr(i));
	}
	listBoxAISwitchTeamAcceptPercent.setItems(aiswitchteamModeItems);

	vector<string> rMultiplier;
	for(int i=0; i<45; ++i){
		rMultiplier.push_back(floatToStr(0.5f+0.1f*i,1));
	}
	listBoxFallbackCpuMultiplier.setItems(rMultiplier);

	labelMap.setText(lang.getString("Map"));

	labelTileset.setText(lang.getString("Tileset"));

	labelTechTree.setText(lang.getString("TechTree"));

	vector<string> playerstatusItems;
	playerstatusItems.push_back(lang.getString("PlayerStatusSetup"));
	playerstatusItems.push_back(lang.getString("PlayerStatusBeRightBack"));
	playerstatusItems.push_back(lang.getString("PlayerStatusReady"));
	listBoxPlayerStatus.setItems(playerstatusItems);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	labelControl.setText(lang.getString("Control"));

    labelFaction.setText(lang.getString("Faction"));

	labelTeam.setText(lang.getString("Team"));

    labelControl.setFont(CoreData::getInstance().getMenuFontBig());
    labelControl.setFont3D(CoreData::getInstance().getMenuFontBig3D());
	labelFaction.setFont(CoreData::getInstance().getMenuFontBig());
	labelFaction.setFont3D(CoreData::getInstance().getMenuFontBig3D());
	labelTeam.setFont(CoreData::getInstance().getMenuFontBig());
	labelTeam.setFont3D(CoreData::getInstance().getMenuFontBig3D());

	//texts
	buttonDisconnect.setText(lang.getString("Return"));

	vector<string> controlItems;
    controlItems.push_back(lang.getString("Closed"));
	controlItems.push_back(lang.getString("CpuEasy"));
	controlItems.push_back(lang.getString("Cpu"));
    controlItems.push_back(lang.getString("CpuUltra"));
    controlItems.push_back(lang.getString("CpuMega"));
	controlItems.push_back(lang.getString("Network"));
	controlItems.push_back(lang.getString("NetworkUnassigned"));
	controlItems.push_back(lang.getString("Human"));

	if(config.getBool("EnableNetworkCpu","false") == true) {
		controlItems.push_back(lang.getString("NetworkCpuEasy"));
		controlItems.push_back(lang.getString("NetworkCpu"));
	    controlItems.push_back(lang.getString("NetworkCpuUltra"));
	    controlItems.push_back(lang.getString("NetworkCpuMega"));
	}

	for(int i=0; i < GameConstants::maxPlayers; ++i) {
		labelPlayers[i].setText(intToStr(i+1));
		listBoxControls[i].setItems(controlItems);
    }
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	labelScenario.setText(lang.getString("Scenario"));

	labelAllowNativeLanguageTechtree.setText(lang.getString("AllowNativeLanguageTechtree"));

	buttonPlayNow.setText(lang.getString("PlayNow"));
	buttonRestoreLastSettings.setText(lang.getString("ReloadLastGameSettings"));

	chatManager.init(&console, -1,true);

	GraphicComponent::reloadFontsForRegisterGraphicComponents(containerName);
}

void MenuStateConnectedGame::disconnectFromServer() {
	NetworkManager &networkManager= NetworkManager::getInstance();
	ClientInterface* clientInterface= networkManager.getClientInterface(false);
	if(clientInterface != NULL) {
		CoreData &coreData= CoreData::getInstance();
		SoundRenderer &soundRenderer= SoundRenderer::getInstance();

		soundRenderer.playFx(coreData.getClickSoundA());
		if(clientInterface->getSocket() != NULL) {
			if(clientInterface->isConnected() == true) {
				Lang &lang= Lang::getInstance();
				const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
				for(unsigned int i = 0; i < languageList.size(); ++i) {
					string sQuitText = lang.getString("QuitGame",languageList[i]);
					clientInterface->sendTextMessage(sQuitText,-1,false,languageList[i]);
				}
				sleep(1);

			}
			clientInterface->close();
		}
		clientInterface->reset();
	}
	currentFactionName="";
	currentMap="";
}

MenuStateConnectedGame::~MenuStateConnectedGame() {
	if(launchingNewGame == false) {
		disconnectFromServer();
		NetworkManager &networkManager= NetworkManager::getInstance();
		networkManager.end();
	}

	if(modHttpServerThread != NULL) {
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		modHttpServerThread->setSimpleTaskInterfaceValid(false);
		modHttpServerThread->signalQuit();
		modHttpServerThread->setThreadOwnerValid(false);

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
		if( modHttpServerThread->canShutdown(true) == true &&
			modHttpServerThread->shutdownAndWait() == true) {
			delete modHttpServerThread;
		}
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
		modHttpServerThread = NULL;
	}

	if(ftpClientThread != NULL) {
		ftpClientThread->setCallBackObject(NULL);
		ftpClientThread->signalQuit();
    	sleep(0);
    	if(ftpClientThread->canShutdown(true) == true &&
    			ftpClientThread->shutdownAndWait() == true) {
    		delete ftpClientThread;
    	}
		else {
			char szBuf[8096]="";
			snprintf(szBuf,8096,"In [%s::%s %d] Error cannot shutdown ftpClientThread\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("%s",szBuf);
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);
		}

    	ftpClientThread = NULL;
	}

	cleanupMapPreviewTexture();

	if(factionVideo != NULL) {
		factionVideo->closePlayer();
		delete factionVideo;
		factionVideo = NULL;
	}
}

string MenuStateConnectedGame::refreshTilesetModInfo(string tilesetInfo) {
	std::vector<std::string> tilesetInfoList;
	Tokenize(tilesetInfo,tilesetInfoList,"|");
	if(tilesetInfoList.size() >= 5) {
		Config &config = Config::getInstance();
		ModInfo modinfo;
		modinfo.name = tilesetInfoList[0];
		modinfo.crc = tilesetInfoList[1];
		modinfo.description = tilesetInfoList[2];
		modinfo.url = tilesetInfoList[3];
		modinfo.imageUrl = tilesetInfoList[4];
		modinfo.type = mt_Tileset;

		string itemPath = config.getPathListForType(ptTilesets,"")[1] + "/" + modinfo.name + string("/*");
		if(itemPath.empty() == false) {
		   bool forceRefresh = (mapCRCUpdateList.find(itemPath) == mapCRCUpdateList.end());
		   uint32 crc = getFolderTreeContentsCheckSumRecursively(itemPath, ".xml", NULL,forceRefresh);
		   if(crc == 0) {
				itemPath = config.getPathListForType(ptTilesets,"")[0] + "/" + modinfo.name + string("/*");
				if(itemPath.empty() == false) {
				   forceRefresh = (mapCRCUpdateList.find(itemPath) == mapCRCUpdateList.end());
				   crc=getFolderTreeContentsCheckSumRecursively(itemPath, ".xml", NULL,forceRefresh);
				}
		   }
		   modinfo.localCRC=uIntToStr(crc);
		   //printf("itemPath='%s' remote crc:'%s'  local crc:'%s'   crc='%d' \n",itemPath.c_str(),modinfo.crc.c_str(),modinfo.localCRC.c_str(),crc);

		   //printf("#1 refreshTilesetModInfo name [%s] modInfo.crc [%s] modInfo.localCRC [%s]\n",modinfo.name.c_str(),modinfo.crc.c_str(),modinfo.localCRC.c_str());
		}
		else {
			modinfo.localCRC="";

			//printf("#2 refreshTilesetModInfo name [%s] modInfo.crc [%s] modInfo.localCRC [%s]\n",modinfo.name.c_str(),modinfo.crc.c_str(),modinfo.localCRC.c_str());
		}

		tilesetCacheList[modinfo.name] = modinfo;
		return modinfo.name;
	}
	return "";
}

string MenuStateConnectedGame::refreshTechModInfo(string techInfo) {
	std::vector<std::string> techInfoList;
	Tokenize(techInfo,techInfoList,"|");
	if(techInfoList.size() >= 6) {
		Config &config = Config::getInstance();
		ModInfo modinfo;
		modinfo.name = techInfoList[0];
		modinfo.count = techInfoList[1];
		modinfo.crc = techInfoList[2];
		modinfo.description = techInfoList[3];
		modinfo.url = techInfoList[4];
		modinfo.imageUrl = techInfoList[5];
		modinfo.type = mt_Techtree;

		string itemPath = config.getPathListForType(ptTechs,"")[1] + "/" + modinfo.name + string("/*");
		if(itemPath.empty() == false) {
		   bool forceRefresh = (mapCRCUpdateList.find(itemPath) == mapCRCUpdateList.end());
		   uint32 crc = getFolderTreeContentsCheckSumRecursively(itemPath, ".xml", NULL,forceRefresh);
		   if(crc == 0) {
				itemPath = config.getPathListForType(ptTechs,"")[0] + "/" + modinfo.name + string("/*");
				if(itemPath.empty() == false) {
				   forceRefresh = (mapCRCUpdateList.find(itemPath) == mapCRCUpdateList.end());
				   crc = getFolderTreeContentsCheckSumRecursively(itemPath, ".xml", NULL,forceRefresh);
				}
		   }
		   modinfo.localCRC=uIntToStr(crc);
		   //printf("itemPath='%s' remote crc:'%s'  local crc:'%s'   crc='%d' \n",itemPath.c_str(),modinfo.crc.c_str(),modinfo.localCRC.c_str(),crc);
		}
		else {
			modinfo.localCRC="";
		}
		techCacheList[modinfo.name] = modinfo;
		return modinfo.name;
	}
	return "";
}

string MenuStateConnectedGame::getMapCRC(string mapName) {
	Config &config = Config::getInstance();
	vector<string> mappaths=config.getPathListForType(ptMaps,"");
	string result="";
	if(mappaths.empty() == false) {
		Checksum checksum;
		string itemPath = mappaths[1] + "/" + mapName;
		if (fileExists(itemPath)){
			checksum.addFile(itemPath);
			uint32 crc=checksum.getSum();
			result=uIntToStr(crc);
			//printf("itemPath='%s' modinfo.name='%s' remote crc:'%s'  local crc:'%s'   crc='%d' \n",itemPath.c_str(),modinfo.name.c_str(),modinfo.crc.c_str(),modinfo.localCRC.c_str(),crc);
		}
		else {
			itemPath = mappaths[0] + "/" + mapName;
			if (fileExists(itemPath)){
				checksum.addFile(itemPath);
				uint32 crc=checksum.getSum();
				result=uIntToStr(crc);
				//printf("itemPath='%s' modinfo.name='%s' remote crc:'%s'  local crc:'%s'   crc='%d' \n",itemPath.c_str(),modinfo.name.c_str(),modinfo.crc.c_str(),modinfo.localCRC.c_str(),crc);
			}
			else {
				result="";
			}
		}
	}
	else {
		result="";
	}
	return result;
}

string MenuStateConnectedGame::refreshMapModInfo(string mapInfo) {
	std::vector<std::string> mapInfoList;
	Tokenize(mapInfo,mapInfoList,"|");
	if(mapInfoList.size() >= 6) {
		//Config &config = Config::getInstance();
		ModInfo modinfo;
		modinfo.name = mapInfoList[0];
		modinfo.count = mapInfoList[1];
		modinfo.crc = mapInfoList[2];
		modinfo.description = mapInfoList[3];
		modinfo.url = mapInfoList[4];
		modinfo.imageUrl = mapInfoList[5];
		modinfo.type = mt_Map;
		modinfo.localCRC=getMapCRC(modinfo.name);
		mapCacheList[modinfo.name] = modinfo;
		return modinfo.name;
	}
	return "";
}

void MenuStateConnectedGame::simpleTask(BaseThread *callingThread,void *userdata) {
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

	static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
    MutexSafeWrapper safeMutexThreadOwner(callingThread->getMutexThreadOwnerValid(),mutexOwnerId);
    if(callingThread->getQuitStatus() == true || safeMutexThreadOwner.isValidMutex() == false) {
    	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
        return;
    }

    callingThread->getMutexThreadOwnerValid()->setOwnerId(mutexOwnerId);
    if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

    Lang &lang= Lang::getInstance();
    Config &config = Config::getInstance();

	std::string techsMetaData = "";
	std::string tilesetsMetaData = "";
	std::string mapsMetaData = "";
	std::string scenariosMetaData = "";

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(config.getString("Masterserver","") != "") {
		string baseURL = config.getString("Masterserver");
		if(baseURL != "") {
			endPathWithSlash(baseURL,false);
		}
		string phpVersionParam = config.getString("phpVersionParam","?version=0.1");
		string gameVersion = "&glestVersion=" + SystemFlags::escapeURL(glestVersionString);
		string playerUUID = "&uuid=" + SystemFlags::escapeURL(config.getString("PlayerId",""));

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d] About to call first http url, base [%s]..\n",__FILE__,__FUNCTION__,__LINE__,baseURL.c_str());

		CURL *handle = SystemFlags::initHTTP();
		CURLcode curlResult = CURLE_OK;
		techsMetaData = SystemFlags::getHTTP(baseURL + "showTechsForGlest.php"+phpVersionParam+gameVersion+playerUUID,handle,-1,&curlResult);

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("techsMetaData [%s] curlResult = %d\n",techsMetaData.c_str(),curlResult);

	    if(callingThread->getQuitStatus() == true || safeMutexThreadOwner.isValidMutex() == false) {
	    	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
	        return;
	    }

	    if(curlResult != CURLE_OK) {
			string curlError = curl_easy_strerror(curlResult);
			char szBuf[8096]="";
			snprintf(szBuf,8096,lang.getString("ModErrorGettingServerData").c_str(),curlError.c_str());
			console.addLine(string("#1 ") + szBuf,true);
	    }

		if(curlResult == CURLE_OK ||
			(curlResult != CURLE_COULDNT_RESOLVE_HOST &&
			 curlResult != CURLE_COULDNT_CONNECT)) {

			tilesetsMetaData = SystemFlags::getHTTP(baseURL + "showTilesetsForGlest.php"+phpVersionParam+gameVersion,handle,-1,&curlResult);
			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("tilesetsMetaData [%s]\n",tilesetsMetaData.c_str());

		    if(callingThread->getQuitStatus() == true || safeMutexThreadOwner.isValidMutex() == false) {
		    	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
		        return;
		    }

		    if(curlResult != CURLE_OK) {
				string curlError = curl_easy_strerror(curlResult);
				char szBuf[8096]="";
				snprintf(szBuf,8096,lang.getString("ModErrorGettingServerData").c_str(),curlError.c_str());
				console.addLine(string("#2 ") + szBuf,true);
		    }

			mapsMetaData = SystemFlags::getHTTP(baseURL + "showMapsForGlest.php"+phpVersionParam+gameVersion,handle,-1,&curlResult);
			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("mapsMetaData [%s]\n",mapsMetaData.c_str());

		    if(callingThread->getQuitStatus() == true || safeMutexThreadOwner.isValidMutex() == false) {
		    	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
		        return;
		    }

		    if(curlResult != CURLE_OK) {
				string curlError = curl_easy_strerror(curlResult);
				char szBuf[8096]="";
				snprintf(szBuf,8096,lang.getString("ModErrorGettingServerData").c_str(),curlError.c_str());
				console.addLine(string("#3 ") + szBuf,true);
		    }

			scenariosMetaData = SystemFlags::getHTTP(baseURL + "showScenariosForGlest.php"+phpVersionParam+gameVersion,handle,-1,&curlResult);
			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("scenariosMetaData [%s]\n",scenariosMetaData.c_str());

		    if(curlResult != CURLE_OK) {
				string curlError = curl_easy_strerror(curlResult);
				char szBuf[8096]="";
				snprintf(szBuf,8096,lang.getString("ModErrorGettingServerData").c_str(),curlError.c_str());
				console.addLine(string("#4 ") + szBuf,true);
		    }
		}
		SystemFlags::cleanupHTTP(&handle);
	}
	else {
        console.addLine(lang.getString("MasterServerMissing"),true);
	}

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

    if(callingThread->getQuitStatus() == true || safeMutexThreadOwner.isValidMutex() == false) {
    	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
        return;
    }

    if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

    MutexSafeWrapper safeMutex(callingThread->getMutexThreadObjectAccessor(),string(__FILE__) + "_" + intToStr(__LINE__));
	tilesetListRemote.clear();
	Tokenize(tilesetsMetaData,tilesetListRemote,"\n");
	safeMutex.ReleaseLock(true);

	for(unsigned int i=0; i < tilesetListRemote.size(); i++) {

	    if(callingThread->getQuitStatus() == true || safeMutexThreadOwner.isValidMutex() == false) {
	    	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
	        return;
	    }

	    safeMutex.Lock();
		string result=refreshTilesetModInfo(tilesetListRemote[i]);
		safeMutex.ReleaseLock(true);
	}

    if(callingThread->getQuitStatus() == true || safeMutexThreadOwner.isValidMutex() == false) {
    	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
        return;
    }

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

    if(callingThread->getQuitStatus() == true || safeMutexThreadOwner.isValidMutex() == false) {
    	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
        return;
    }

    if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

    safeMutex.Lock();
	techListRemote.clear();
	Tokenize(techsMetaData,techListRemote,"\n");
	safeMutex.ReleaseLock(true);

    if(callingThread->getQuitStatus() == true || safeMutexThreadOwner.isValidMutex() == false) {
    	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
        return;
    }

	for(unsigned int i=0; i < techListRemote.size(); i++) {

	    if(callingThread->getQuitStatus() == true || safeMutexThreadOwner.isValidMutex() == false) {
	    	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
	        return;
	    }

	    safeMutex.Lock();
		string result=refreshTechModInfo(techListRemote[i]);
		safeMutex.ReleaseLock(true);
	}

    if(callingThread->getQuitStatus() == true || safeMutexThreadOwner.isValidMutex() == false) {
    	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
        return;
    }

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

    if(callingThread->getQuitStatus() == true || safeMutexThreadOwner.isValidMutex() == false) {
    	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
        return;
    }

    if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

    safeMutex.Lock();
	mapListRemote.clear();
	Tokenize(mapsMetaData,mapListRemote,"\n");
	safeMutex.ReleaseLock(true);

	for(unsigned int i=0; i < mapListRemote.size(); i++) {

	    if(callingThread->getQuitStatus() == true || safeMutexThreadOwner.isValidMutex() == false) {
	    	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
	        return;
	    }

	    safeMutex.Lock();
		string result=refreshMapModInfo(mapListRemote[i]);
		safeMutex.ReleaseLock(true);
	}

    if(callingThread->getQuitStatus() == true || safeMutexThreadOwner.isValidMutex() == false) {
    	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
        return;
    }

    if(callingThread->getQuitStatus() == true || safeMutexThreadOwner.isValidMutex() == false) {
    	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
        return;
    }

    if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(modHttpServerThread != NULL) {
		modHttpServerThread->signalQuit();
	}

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void MenuStateConnectedGame::mouseClick(int x, int y, MouseButton mouseButton){

	CoreData &coreData= CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();
	NetworkManager &networkManager= NetworkManager::getInstance();
	ClientInterface* clientInterface= networkManager.getClientInterface();
	Lang &lang= Lang::getInstance();

	string advanceToItemStartingWith = "";
	if(mainMessageBox.getEnabled() == false) {
    	if(::Shared::Platform::Window::isKeyStateModPressed(KMOD_SHIFT) == true) {
    		wchar_t lastKey = ::Shared::Platform::Window::extractLastKeyPressed();
    		//printf("lastKey = %d [%c]\n",lastKey,lastKey);
    		advanceToItemStartingWith = lastKey;
    	}
	}

	if(mapPreviewTexture != NULL) {
//        		printf("X: %d Y: %d      [%d, %d, %d, %d]\n",
//        				x, y,
//        				this->render_mapPreviewTexture_X, this->render_mapPreviewTexture_X + this->render_mapPreviewTexture_W,
//        				this->render_mapPreviewTexture_Y, this->render_mapPreviewTexture_Y + this->render_mapPreviewTexture_H);

		if( x >= this->render_mapPreviewTexture_X && x <= this->render_mapPreviewTexture_X + this->render_mapPreviewTexture_W &&
			y >= this->render_mapPreviewTexture_Y && y <= this->render_mapPreviewTexture_Y + this->render_mapPreviewTexture_H) {

			if( this->render_mapPreviewTexture_X == mapPreviewTexture_X &&
				this->render_mapPreviewTexture_Y == mapPreviewTexture_Y &&
				this->render_mapPreviewTexture_W == mapPreviewTexture_W &&
				this->render_mapPreviewTexture_H == mapPreviewTexture_H) {

				const Metrics &metrics= Metrics::getInstance();

				this->render_mapPreviewTexture_X = 0;
				this->render_mapPreviewTexture_Y = 0;
				this->render_mapPreviewTexture_W = metrics.getVirtualW();
				this->render_mapPreviewTexture_H = metrics.getVirtualH();
				this->zoomedMap = true;

				cleanupMapPreviewTexture();
			}
			else {
				this->render_mapPreviewTexture_X = mapPreviewTexture_X;
				this->render_mapPreviewTexture_Y = mapPreviewTexture_Y;
				this->render_mapPreviewTexture_W = mapPreviewTexture_W;
				this->render_mapPreviewTexture_H = mapPreviewTexture_H;
				this->zoomedMap = false;

				cleanupMapPreviewTexture();
			}
			return;
		}
    	if(this->zoomedMap==true){
    		return;
    	}
	}

	if(mainMessageBox.getEnabled()) {
		int button= 0;
		if(mainMessageBox.mouseClick(x, y, button)) {
			soundRenderer.playFx(coreData.getClickSoundA());
			if(button == 0) {
				mainMessageBox.setEnabled(false);
			}
		}
	}
	else if(ftpMessageBox.getEnabled()) {
		int button= 0;
		if(ftpMessageBox.mouseClick(x, y, button)) {
			soundRenderer.playFx(coreData.getClickSoundA());
			ftpMessageBox.setEnabled(false);

			if(button == 0 || (button == 1 && ftpMessageBox.getButtonCount() == 3)) {
			    if(ftpMissingDataType == ftpmsg_MissingMap) {
                    getMissingMapFromFTPServerInProgress = true;

    		    	Lang &lang= Lang::getInstance();
    		    	const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
    		    	for(unsigned int i = 0; i < languageList.size(); ++i) {
						char szMsg[8096]="";
						if(lang.hasString("DataMissingMapNowDownloading",languageList[i]) == true) {
							snprintf(szMsg,8096,lang.getString("DataMissingMapNowDownloading",languageList[i]).c_str(),getHumanPlayerName().c_str(),getMissingMapFromFTPServer.c_str());
						}
						else {
							snprintf(szMsg,8096,"Player: %s is attempting to download the map: %s",getHumanPlayerName().c_str(),getMissingMapFromFTPServer.c_str());
						}
						bool localEcho = lang.isLanguageLocal(languageList[i]);
						clientInterface->sendTextMessage(szMsg,-1, localEcho,languageList[i]);
    		    	}

                    if(ftpClientThread != NULL) {
                    	if(button == 0 && ftpMessageBox.getButtonCount() == 3) {
							string mapName = getMissingMapFromFTPServer;

							MutexSafeWrapper safeMutexThread((modHttpServerThread != NULL ? modHttpServerThread->getMutexThreadObjectAccessor() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
							string mapURL = mapCacheList[mapName].url;
							safeMutexThread.ReleaseLock();

							if(ftpClientThread != NULL) ftpClientThread->addMapToRequests(mapName,mapURL);
                    		MutexSafeWrapper safeMutexFTPProgress((ftpClientThread != NULL ? ftpClientThread->getProgressMutex() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
                    		fileFTPProgressList[getMissingMapFromFTPServer] = pair<int,string>(0,"");
                    		safeMutexFTPProgress.ReleaseLock();
                    	}
                    	else {
                    		ftpClientThread->addMapToRequests(getMissingMapFromFTPServer);
                    		MutexSafeWrapper safeMutexFTPProgress((ftpClientThread != NULL ? ftpClientThread->getProgressMutex() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
                    		fileFTPProgressList[getMissingMapFromFTPServer] = pair<int,string>(0,"");
                    		safeMutexFTPProgress.ReleaseLock();
                    	}
                    }
			    }
			    else if(ftpMissingDataType == ftpmsg_MissingTileset) {
                    getMissingTilesetFromFTPServerInProgress = true;

    		    	Lang &lang= Lang::getInstance();
    		    	const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
    		    	for(unsigned int i = 0; i < languageList.size(); ++i) {
						char szMsg[8096]="";
						if(lang.hasString("DataMissingTilesetNowDownloading",languageList[i]) == true) {
							snprintf(szMsg,8096,lang.getString("DataMissingTilesetNowDownloading",languageList[i]).c_str(),getHumanPlayerName().c_str(),getMissingTilesetFromFTPServer.c_str());
						}
						else {
							snprintf(szMsg,8096,"Player: %s is attempting to download the tileset: %s",getHumanPlayerName().c_str(),getMissingTilesetFromFTPServer.c_str());
						}
						bool localEcho = lang.isLanguageLocal(languageList[i]);
						clientInterface->sendTextMessage(szMsg,-1, localEcho,languageList[i]);
    		    	}

                    if(ftpClientThread != NULL) {
                    	if(button == 0 && ftpMessageBox.getButtonCount() == 3) {
    						string tilesetName = getMissingTilesetFromFTPServer;

    						MutexSafeWrapper safeMutexThread((modHttpServerThread != NULL ? modHttpServerThread->getMutexThreadObjectAccessor() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
    						string tilesetURL = tilesetCacheList[tilesetName].url;
    						safeMutexThread.ReleaseLock();

    						if(ftpClientThread != NULL) ftpClientThread->addTilesetToRequests(tilesetName,tilesetURL);
                    		MutexSafeWrapper safeMutexFTPProgress((ftpClientThread != NULL ? ftpClientThread->getProgressMutex() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
                    		fileFTPProgressList[getMissingTilesetFromFTPServer] = pair<int,string>(0,"");
                    		safeMutexFTPProgress.ReleaseLock();
                    	}
                    	else {
							ftpClientThread->addTilesetToRequests(getMissingTilesetFromFTPServer);
							MutexSafeWrapper safeMutexFTPProgress((ftpClientThread != NULL ? ftpClientThread->getProgressMutex() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
							fileFTPProgressList[getMissingTilesetFromFTPServer] = pair<int,string>(0,"");
							safeMutexFTPProgress.ReleaseLock();
                    	}
                    }
			    }
			    else if(ftpMissingDataType == ftpmsg_MissingTechtree) {
                    getMissingTechtreeFromFTPServerInProgress = true;

    		    	Lang &lang= Lang::getInstance();
    		    	const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
    		    	for(unsigned int i = 0; i < languageList.size(); ++i) {
						char szMsg[8096]="";
						if(lang.hasString("DataMissingTechtreeNowDownloading",languageList[i]) == true) {
							snprintf(szMsg,8096,lang.getString("DataMissingTechtreeNowDownloading",languageList[i]).c_str(),getHumanPlayerName().c_str(),getMissingTechtreeFromFTPServer.c_str());
						}
						else {
							snprintf(szMsg,8096,"Player: %s is attempting to download the techtree: %s",getHumanPlayerName().c_str(),getMissingTechtreeFromFTPServer.c_str());
						}
						bool localEcho = lang.isLanguageLocal(languageList[i]);
						clientInterface->sendTextMessage(szMsg,-1, localEcho,languageList[i]);
    		    	}

                    if(ftpClientThread != NULL) {
                    	if(button == 0 && ftpMessageBox.getButtonCount() == 3) {
    						string techName = getMissingTechtreeFromFTPServer;

    						MutexSafeWrapper safeMutexThread((modHttpServerThread != NULL ? modHttpServerThread->getMutexThreadObjectAccessor() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
    						string techURL = techCacheList[techName].url;
    						safeMutexThread.ReleaseLock();

    						if(ftpClientThread != NULL) ftpClientThread->addTechtreeToRequests(techName,techURL);
                    		MutexSafeWrapper safeMutexFTPProgress((ftpClientThread != NULL ? ftpClientThread->getProgressMutex() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
                    		fileFTPProgressList[getMissingTechtreeFromFTPServer] = pair<int,string>(0,"");
                    		safeMutexFTPProgress.ReleaseLock();
                    	}
                    	else {
							ftpClientThread->addTechtreeToRequests(getMissingTechtreeFromFTPServer);
							MutexSafeWrapper safeMutexFTPProgress((ftpClientThread != NULL ? ftpClientThread->getProgressMutex() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
							fileFTPProgressList[getMissingTechtreeFromFTPServer] = pair<int,string>(0,"");
							safeMutexFTPProgress.ReleaseLock();
                    	}
                    }
			    }
			}
		}
	}
	else if(buttonCancelDownloads.mouseClick(x,y)) {
		soundRenderer.playFx(coreData.getClickSoundA());

        if(ftpClientThread != NULL && fileFTPProgressList.empty() == false) {

			ftpClientThread->setCallBackObject(NULL);
			ftpClientThread->signalQuit();
			sleep(0);
			if(ftpClientThread->canShutdown(true) == true &&
					ftpClientThread->shutdownAndWait() == true) {
				delete ftpClientThread;
			}
			else {
				char szBuf[8096]="";
				snprintf(szBuf,8096,"In [%s::%s %d] Error cannot shutdown ftpClientThread\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
				if(SystemFlags::VERBOSE_MODE_ENABLED) printf("%s",szBuf);
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);
			}
			ftpClientThread = NULL;

            fileFTPProgressList.clear();
            getMissingMapFromFTPServerInProgress 		= false;
            getMissingTilesetFromFTPServerInProgress 	= false;
            getMissingTechtreeFromFTPServerInProgress 	= false;
            getMissingMapFromFTPServer					= "";
            getMissingTilesetFromFTPServer				= "";
            getMissingTechtreeFromFTPServer				= "";
            getMissingMapFromFTPServerLastPrompted		= 0;
            getMissingTilesetFromFTPServerLastPrompted	= 0;
            getMissingTechtreeFromFTPServerLastPrompted	= 0;

            ClientInterface *clientInterface = networkManager.getClientInterface();
            if(clientInterface == NULL) {
            	throw megaglest_runtime_error("clientInterface == NULL");
            }
            if(getInProgressSavedGameFromFTPServerInProgress == true) {
            	if(clientInterface != NULL) {
            		clientInterface->close();
            		return;
            	}
            }

        	getInProgressSavedGameFromFTPServer = "";
        	getInProgressSavedGameFromFTPServerInProgress = false;

            string serverUrl = clientInterface->getServerIpAddress();
            int portNumber   = clientInterface->getServerFTPPort();

            if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] Using FTP port #: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,portNumber);

            Config &config = Config::getInstance();
            vector<string> mapPathList = config.getPathListForType(ptMaps);
            std::pair<string,string> mapsPath;
            if(mapPathList.empty() == false) {
                mapsPath.first = mapPathList[0];
            }
            if(mapPathList.size() > 1) {
                mapsPath.second = mapPathList[1];
            }
            std::pair<string,string> tilesetsPath;
            vector<string> tilesetsList = Config::getInstance().getPathListForType(ptTilesets);
            if(tilesetsList.empty() == false) {
                tilesetsPath.first = tilesetsList[0];
                if(tilesetsList.size() > 1) {
                    tilesetsPath.second = tilesetsList[1];
                }
            }

            std::pair<string,string> techtreesPath;
            vector<string> techtreesList = Config::getInstance().getPathListForType(ptTechs);
            if(techtreesList.empty() == false) {
            	techtreesPath.first = techtreesList[0];
                if(techtreesList.size() > 1) {
                	techtreesPath.second = techtreesList[1];
                }
            }

            std::pair<string,string> scenariosPath;
            vector<string> scenariosList = Config::getInstance().getPathListForType(ptScenarios);
            if(scenariosList.empty() == false) {
            	scenariosPath.first = scenariosList[0];
                if(scenariosList.size() > 1) {
                	scenariosPath.second = scenariosList[1];
                }
            }

            string fileArchiveExtension = config.getString("FileArchiveExtension","");
            string fileArchiveExtractCommand = config.getString("FileArchiveExtractCommand","");
            string fileArchiveExtractCommandParameters = config.getString("FileArchiveExtractCommandParameters","");
            int32 fileArchiveExtractCommandSuccessResult = config.getInt("FileArchiveExtractCommandSuccessResult","0");

        	// Get path to temp files
        	string tempFilePath = "temp/";
        	if(getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) != "") {
        		tempFilePath = getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) + tempFilePath;
        	}
        	else {
                string userData = config.getString("UserData_Root","");
                if(userData != "") {
                	endPathWithSlash(userData);
                }
                tempFilePath = userData + tempFilePath;
        	}
        	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Temp files path [%s]\n",tempFilePath.c_str());

            ftpClientThread = new FTPClientThread(portNumber,serverUrl,
            		mapsPath,tilesetsPath,techtreesPath,scenariosPath,
            		this,fileArchiveExtension,fileArchiveExtractCommand,
            		fileArchiveExtractCommandParameters,
            		fileArchiveExtractCommandSuccessResult,
            		tempFilePath);
            ftpClientThread->start();

	    	Lang &lang= Lang::getInstance();
	    	const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
	    	for(unsigned int i = 0; i < languageList.size(); ++i) {
				char szMsg[8096]="";
	            if(lang.hasString("CancelDownloadsMsg",languageList[i]) == true) {
	            	snprintf(szMsg,8096,lang.getString("CancelDownloadsMsg",languageList[i]).c_str(),getHumanPlayerName().c_str());
	            }
	            else {
	            	snprintf(szMsg,8096,"Player: %s cancelled all file downloads.",getHumanPlayerName().c_str());
	            }
	            clientInterface->sendTextMessage(szMsg,-1, lang.isLanguageLocal(languageList[i]),languageList[i]);
	    	}
        }
	}
	else if(buttonDisconnect.mouseClick(x,y)){
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		disconnectFromServer();
		networkManager.end();
		returnToJoinMenu();
		return;
    }

	if (initialSettingsReceivedFromServer == false) {
		return;
	}

	if(activeInputLabel != NULL && activeInputLabel->mouseClick(x,y) == false){
		setActiveInputLabel(NULL);
	}

	// Only allow changes after we get game settings from the server
	if(clientInterface != NULL && clientInterface->isConnected() == true) {
		int myCurrentIndex= -1;
		for(int i= 0; i < GameConstants::maxPlayers; ++i) {// find my current index by looking at editable listBoxes
			if(//listBoxFactions[i].getEditable() &&
				clientInterface->getGameSettings()->getStartLocationIndex(clientInterface->getGameSettings()->getThisFactionIndex()) == i) {
				myCurrentIndex= i;
			}
		}

		//printf("myCurrentIndex = %d thisFactionIndex = %d\n",myCurrentIndex,clientInterface->getGameSettings()->getThisFactionIndex());

		if(myCurrentIndex != -1)
			for(int i= 0; i < GameConstants::maxPlayers; ++i) {
				if(listBoxFactions[i].getEditable() &&
					clientInterface->getGameSettings()->getStartLocationIndex(clientInterface->getGameSettings()->getThisFactionIndex()) == i) {
					if(listBoxFactions[i].mouseClick(x, y,advanceToItemStartingWith)) {
						soundRenderer.playFx(coreData.getClickSoundA());
						ClientInterface* clientInterface= NetworkManager::getInstance().getClientInterface();
						if(clientInterface->isConnected()) {
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
				if(listBoxTeams[i].getEditable() &&
						clientInterface->getGameSettings()->getStartLocationIndex(clientInterface->getGameSettings()->getThisFactionIndex()) == i) {
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

				bool canGrabSlot = false;
				ClientInterface *clientInterface = networkManager.getClientInterface();
				if(clientInterface != NULL && clientInterface->getJoinGameInProgress() == true) {
					canGrabSlot = ((listBoxControls[i].getSelectedItemIndex() == ctNetwork &&
							labelNetStatus[i].getText() == GameConstants::NETWORK_SLOT_UNCONNECTED_SLOTNAME) ||
							(listBoxControls[i].getSelectedItemIndex() != ctHuman &&
							 listBoxControls[i].getSelectedItemIndex() != ctClosed &&
							 listBoxControls[i].getSelectedItemIndex() != ctNetwork));
				}
				else {
					canGrabSlot = (listBoxControls[i].getSelectedItemIndex() == ctNetwork &&
										labelNetStatus[i].getText() == GameConstants::NETWORK_SLOT_UNCONNECTED_SLOTNAME);
				}

				if(canGrabSlot == true) {
					if(i < mapInfo.players && grabSlotButton[i].mouseClick(x, y)) {
						//printf("Send slot switch request for slot = %d, myCurrentIndex = %d\n",i,myCurrentIndex);

						soundRenderer.playFx(coreData.getClickSoundB());
						clientInterface->setGameSettingsReceived(false);
						settingsReceivedFromServer= false;

						if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] sending a switchSlot request from %d to %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,clientInterface->getGameSettings()->getThisFactionIndex(),i);

						//printf("Switch slot from %d to %d\n",myCurrentIndex,i);

						string desiredFactionName = listBoxFactions[myCurrentIndex].getSelectedItem();
						int desiredTeamIndex = listBoxTeams[myCurrentIndex].getSelectedItemIndex();
						if(checkBoxScenario.getValue() == true) {
							desiredFactionName = listBoxFactions[i].getSelectedItem();
							desiredTeamIndex = listBoxTeams[i].getSelectedItemIndex();
						}

						//printf("Sending switch slot request to server...\n");

						clientInterface->sendSwitchSetupRequest(
								desiredFactionName,
						        myCurrentIndex,
						        i,
						        desiredTeamIndex,
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
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
			soundRenderer.playFx(coreData.getClickSoundC());
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

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
			if(clientInterface != NULL && clientInterface->isConnected()) {
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

		if(isHeadlessAdmin() == true) {
			//printf("#1 admin key [%d] client key [%d]\n",settings->getMasterserver_admin(),clientInterface->getSessionKey());
			mouseClickAdmin(x, y, mouseButton,advanceToItemStartingWith);
		}
		else if(clientInterface != NULL && clientInterface->getJoinGameInProgress() == true) {
	        if(buttonPlayNow.mouseClick(x,y) && buttonPlayNow.getEnabled()) {
	        	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	        	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

                uint32 tilesetCRC = lastCheckedCRCTilesetValue;
                uint32 techCRC = lastCheckedCRCTechtreeValue;
                uint32 mapCRC = lastCheckedCRCMapValue;
                const GameSettings *gameSettings = clientInterface->getGameSettings();

                bool dataSynchMismatch = ((mapCRC != 0 && mapCRC != gameSettings->getMapCRC()) ||
                						  (tilesetCRC != 0 && tilesetCRC != gameSettings->getTilesetCRC()) ||
                						  (techCRC != 0 && techCRC != gameSettings->getTechCRC()));
	        	if(dataSynchMismatch == false) {
	        		PlayNow(true);
	        		return;
	        	}
	        	else {
	        		showMessageBox("You cannot start the game because\none or more clients do not have the same game data!", "Data Mismatch Error", false);
	        	}
	        }
		}
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

bool MenuStateConnectedGame::isHeadlessAdmin() {
	bool result = false;

	ClientInterface* clientInterface= NetworkManager::getInstance().getClientInterface();
	if(clientInterface != NULL && clientInterface->isConnected()) {
		const GameSettings *settings = clientInterface->getGameSettings();
		if(settings != NULL) {
			//printf("#1 admin key [%d] client key [%d]\n",settings->getMasterserver_admin(),clientInterface->getSessionKey());

			if(settings->getMasterserver_admin() == clientInterface->getSessionKey()) {
				result = true;
			}
		}
	}

	return result;
}

void MenuStateConnectedGame::broadCastGameSettingsToHeadlessServer(bool forceNow) {
	if(isHeadlessAdmin() == false) {
		return;
	}

	if(forceNow == true ||
		((needToBroadcastServerSettings == true ) && ( difftime((long int)time(NULL),broadcastServerSettingsDelayTimer) >= HEADLESSSERVER_BROADCAST_SETTINGS_SECONDS))) {
		//printf("In [%s:%s] Line: %d forceNow = %d broadcastServerSettingsDelayTimer = " MG_SIZE_T_SPECIFIER ", now =" MG_SIZE_T_SPECIFIER "\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,forceNow,broadcastServerSettingsDelayTimer,time(NULL));

		needToBroadcastServerSettings = false;
		broadcastServerSettingsDelayTimer = time(NULL);

		NetworkManager &networkManager= NetworkManager::getInstance();
		ClientInterface *clientInterface = networkManager.getClientInterface();

		for(int i = 0; i < mapInfo.players; ++i) {
			if(listBoxControls[i].getSelectedItemIndex() == ctNetworkUnassigned) {
				listBoxControls[i].setSelectedItemIndex(ctNetwork);
			}
		}
		for(int i = mapInfo.players; i < GameConstants::maxPlayers; ++i) {
			if(listBoxControls[i].getSelectedItemIndex() == ctNetwork) {
				listBoxControls[i].setSelectedItemIndex(ctNetworkUnassigned);
			}
		}

		GameSettings gameSettings = *clientInterface->getGameSettings();
		loadGameSettings(&gameSettings);

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("broadcast settings:\n%s\n",gameSettings.toString().c_str());

		//printf("Client sending map [%s] admin key [%d]\n",gameSettings.getMap().c_str(),gameSettings.getMasterserver_admin());

		clientInterface->broadcastGameSetup(&gameSettings);
	}
}

void MenuStateConnectedGame::updateResourceMultiplier(const int index) {
		ControlType ct= static_cast<ControlType>(listBoxControls[index].getSelectedItemIndex());
		if(ct == ctCpuEasy || ct == ctNetworkCpuEasy)
		{
			listBoxRMultiplier[index].setSelectedItem(floatToStr(GameConstants::easyMultiplier,1));
			listBoxRMultiplier[index].setEnabled(true);
		}
		else if(ct == ctCpu || ct == ctNetworkCpu) {
			listBoxRMultiplier[index].setSelectedItem(floatToStr(GameConstants::normalMultiplier,1));
			listBoxRMultiplier[index].setEnabled(true);
		}
		else if(ct == ctCpuUltra || ct == ctNetworkCpuUltra)
		{
			listBoxRMultiplier[index].setSelectedItem(floatToStr(GameConstants::ultraMultiplier,1));
			listBoxRMultiplier[index].setEnabled(true);
		}
		else if(ct == ctCpuMega || ct == ctNetworkCpuMega)
		{
			listBoxRMultiplier[index].setSelectedItem(floatToStr(GameConstants::megaMultiplier,1));
			listBoxRMultiplier[index].setEnabled(true);
		}
		else {
			listBoxRMultiplier[index].setSelectedItem(floatToStr(GameConstants::normalMultiplier,1));
			listBoxRMultiplier[index].setEnabled(false);
		}

		listBoxRMultiplier[index].setEditable(listBoxRMultiplier[index].getEnabled());
		listBoxRMultiplier[index].setVisible(listBoxRMultiplier[index].getEnabled());
}

void MenuStateConnectedGame::mouseClickAdmin(int x, int y, MouseButton mouseButton,string advanceToItemStartingWith) {

    try {
        if(buttonPlayNow.mouseClick(x,y) && buttonPlayNow.getEnabled()) {
        	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
        	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

            PlayNow(true);
            return;
        }
        else if(buttonRestoreLastSettings.mouseClick(x,y) && buttonRestoreLastSettings.getEnabled()) {
        	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

        	CoreData &coreData= CoreData::getInstance();
        	SoundRenderer &soundRenderer= SoundRenderer::getInstance();
        	soundRenderer.playFx(coreData.getClickSoundB());

        	RestoreLastGameSettings();
        }
        else if (checkBoxAllowNativeLanguageTechtree.mouseClick(x, y)) {
        	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
        	needToBroadcastServerSettings=true;
        	broadcastServerSettingsDelayTimer=time(NULL);
        }
        else if(listBoxMap.mouseClick(x, y,advanceToItemStartingWith)) {
        	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
        	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s\n", getCurrentMapFile().c_str());

            if(loadMapInfo(Config::getMapPath(getCurrentMapFile(),"",false), &mapInfo, true) == true) {
            	labelMapInfo.setText(mapInfo.desc);
            }
            else {
            	labelMapInfo.setText("???");
            }

        	needToBroadcastServerSettings=true;
        	broadcastServerSettingsDelayTimer=time(NULL);
        }
        else if(listBoxFogOfWar.mouseClick(x, y)) {
        	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
        	needToBroadcastServerSettings=true;
        	broadcastServerSettingsDelayTimer=time(NULL);
        }
        else if(checkBoxAllowObservers.mouseClick(x, y)) {
        	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
        	needToBroadcastServerSettings=true;
        	broadcastServerSettingsDelayTimer=time(NULL);
        }
        else if (checkBoxEnableSwitchTeamMode.mouseClick(x, y)) {
        	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
        	needToBroadcastServerSettings=true;
        	broadcastServerSettingsDelayTimer=time(NULL);
        }
        else if(listBoxAISwitchTeamAcceptPercent.getEnabled() && listBoxAISwitchTeamAcceptPercent.mouseClick(x, y)) {
        	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
        	needToBroadcastServerSettings=true;
        	broadcastServerSettingsDelayTimer=time(NULL);
        }
        else if(listBoxFallbackCpuMultiplier.getEnabled() && listBoxFallbackCpuMultiplier.mouseClick(x, y)) {
        	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
        	needToBroadcastServerSettings=true;
        	broadcastServerSettingsDelayTimer=time(NULL);
        }
        else if(listBoxTileset.mouseClick(x, y,advanceToItemStartingWith)) {
        	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
        	needToBroadcastServerSettings=true;
        	broadcastServerSettingsDelayTimer=time(NULL);
        }
        else if(listBoxTechTree.mouseClick(x, y,advanceToItemStartingWith)) {
        	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
            reloadFactions(false,"");
        	needToBroadcastServerSettings=true;
        	broadcastServerSettingsDelayTimer=time(NULL);
        }

		else if (checkBoxAllowTeamUnitSharing.mouseClick(x, y)) {
        	needToBroadcastServerSettings=true;
        	broadcastServerSettingsDelayTimer=time(NULL);
		}
		else if (checkBoxAllowTeamResourceSharing.mouseClick(x, y)) {
        	needToBroadcastServerSettings=true;
        	broadcastServerSettingsDelayTimer=time(NULL);
		}
        else {
        	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

        	NetworkManager &networkManager= NetworkManager::getInstance();
        	ClientInterface* clientInterface= networkManager.getClientInterface();

            for(int i=0; i<mapInfo.players; ++i) {
				// set multiplier
				if(listBoxRMultiplier[i].mouseClick(x, y)) {
					if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
					//broadCastGameSettingsToMasterserver();
					needToBroadcastServerSettings=true;
					broadcastServerSettingsDelayTimer=time(NULL);
				}

                //ensure that only 1 human player is present
                if(clientInterface != NULL && clientInterface->getGameSettings() != NULL &&
                		clientInterface->getGameSettings()->getStartLocationIndex(clientInterface->getGameSettings()->getThisFactionIndex()) != i &&
                		listBoxControls[i].mouseClick(x, y)) {
                	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
                	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

                	//!! this must be done two times!""
                	if(listBoxControls[i].getSelectedItemIndex() == ctNetworkUnassigned) {
                		listBoxControls[i].mouseClick(x, y);
                	}
                	if( (isHeadlessAdmin()==true) && (listBoxControls[i].getSelectedItemIndex() == ctHuman)){
                		listBoxControls[i].mouseClick(x, y);
                	}
                	//!! this must be done two times!""
                	if(listBoxControls[i].getSelectedItemIndex() == ctNetworkUnassigned) {
                		listBoxControls[i].mouseClick(x, y);
                	}

                    updateResourceMultiplier(i);

                	needToBroadcastServerSettings=true;
                	broadcastServerSettingsDelayTimer=time(NULL);
                }
                else if(clientInterface != NULL && clientInterface->getGameSettings()->getStartLocationIndex(clientInterface->getGameSettings()->getThisFactionIndex()) != i &&
                		listBoxFactions[i].mouseClick(x, y,advanceToItemStartingWith)) {
                	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
                    // Disallow CPU players to be observers
        			if(factionFiles[listBoxFactions[i].getSelectedItemIndex()] == formatString(GameConstants::OBSERVER_SLOTNAME) &&
        				(listBoxControls[i].getSelectedItemIndex() == ctCpuEasy || listBoxControls[i].getSelectedItemIndex() == ctCpu ||
        				 listBoxControls[i].getSelectedItemIndex() == ctCpuUltra || listBoxControls[i].getSelectedItemIndex() == ctCpuMega)) {
        				if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
        				listBoxFactions[i].setSelectedItemIndex(0);
        			}

                	needToBroadcastServerSettings=true;
                	broadcastServerSettingsDelayTimer=time(NULL);
                }
                else if(clientInterface != NULL && clientInterface->getGameSettings()->getStartLocationIndex(clientInterface->getGameSettings()->getThisFactionIndex()) != i &&
                		listBoxTeams[i].mouseClick(x, y)) {
                	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
                    if(factionFiles[listBoxFactions[i].getSelectedItemIndex()] != formatString(GameConstants::OBSERVER_SLOTNAME)) {
                        if(listBoxTeams[i].getSelectedItemIndex() + 1 != (GameConstants::maxPlayers + fpt_Observer)) {
                            //lastSelectedTeamIndex[i] = listBoxTeams[i].getSelectedItemIndex();
                        }
                    }

                	needToBroadcastServerSettings=true;
                	broadcastServerSettingsDelayTimer=time(NULL);
                }
            }
        }
    }
	catch(const std::exception &ex) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s %d]\nError detected:\n%s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);

		showMessageBox( szBuf, "Error", false);
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

void MenuStateConnectedGame::PlayNow(bool saveGame) {
	NetworkManager &networkManager= NetworkManager::getInstance();
	ClientInterface *clientInterface = networkManager.getClientInterface();

	GameSettings gameSettings = *clientInterface->getGameSettings();
	loadGameSettings(&gameSettings);

	if(saveGame == true) {
		CoreData::getInstance().saveGameSettingsToFile(HEADLESS_SAVED_GAME_FILENAME,&gameSettings,true);
	}

	CoreData &coreData= CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();
	soundRenderer.playFx(coreData.getClickSoundC());

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	//printf("Client sending map [%s] admin key [%d]\n",gameSettings.getMap().c_str(),gameSettings.getMasterserver_admin());

	if(clientInterface->getJoinGameInProgress() == true) {
		if(readyToJoinInProgressGame == false && launchingNewGame == false) {
			Lang &lang= Lang::getInstance();
			const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
			for(unsigned int i = 0; i < languageList.size(); ++i) {
				char szMsg[8096]="";
				if(lang.hasString("JoinPlayerToCurrentGameLaunch",languageList[i]) == true) {
					snprintf(szMsg,8096,lang.getString("JoinPlayerToCurrentGameLaunch",languageList[i]).c_str(),getHumanPlayerName().c_str());
				}
				else {
					snprintf(szMsg,8096,"Player: %s is about to join the game, please wait...",getHumanPlayerName().c_str());
				}
				bool localEcho = lang.isLanguageLocal(languageList[i]);
				clientInterface->sendTextMessage(szMsg,-1, localEcho,languageList[i]);
			}

			sleep(1);
			launchingNewGame = true;
			clientInterface->broadcastGameStart(&gameSettings);
		}
		return;
	}
	else {
		launchingNewGame = true;
		broadCastGameSettingsToHeadlessServer(needToBroadcastServerSettings);
		clientInterface->broadcastGameStart(&gameSettings);
	}
}

string MenuStateConnectedGame::getCurrentMapFile() {
	int mapIndex=listBoxMap.getSelectedItemIndex();
	if(mapIndex < 0) {
		return "";
	}
	return mapFiles[mapIndex];
}

void MenuStateConnectedGame::reloadFactions(bool keepExistingSelectedItem, string scenario) {
	vector<string> results;
    Config &config = Config::getInstance();
	Lang &lang= Lang::getInstance();

    string scenarioDir = Scenario::getScenarioDir(dirList, scenario);
    vector<string> techPaths = config.getPathListForType(ptTechs,scenarioDir);
    for(int idx = 0; idx < (int)techPaths.size(); idx++) {
        string &techPath = techPaths[idx];
        endPathWithSlash(techPath);

        if(listBoxTechTree.getSelectedItemIndex() >= 0 && listBoxTechTree.getSelectedItemIndex() < (int)techTreeFiles.size()) {
			findDirs(techPath + techTreeFiles[listBoxTechTree.getSelectedItemIndex()] + "/factions/", results, false, false);
        }
        if(results.empty() == false) {
            break;
        }
    }

    if(results.empty() == true) {
        //throw megaglest_runtime_error("(2)There are no factions for the tech tree [" + techTreeFiles[listBoxTechTree.getSelectedItemIndex()] + "]");
		//showGeneralError=true;
		//generalErrorToShow = "[#2] There are no factions for the tech tree [" + techTreeFiles[listBoxTechTree.getSelectedItemIndex()] + "]";
    }

	vector<string> translatedFactionNames;
	factionFiles= results;
	for(int i = 0; i < (int)results.size(); ++i) {
		results[i]= formatString(results[i]);
		string translatedString=techTree->getTranslatedFactionName(techTreeFiles[listBoxTechTree.getSelectedItemIndex()],factionFiles[i]);
		if(toLower(translatedString)==toLower(results[i])){
			translatedFactionNames.push_back(results[i]);
		}
		else {
			translatedFactionNames.push_back(results[i]+" ("+translatedString+")");
		}
		//printf("FACTIONS i = %d results [%s]\n",i,results[i].c_str());

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"Tech [%s] has faction [%s]\n",techTreeFiles[listBoxTechTree.getSelectedItemIndex()].c_str(),results[i].c_str());
	}

	results.push_back(formatString(GameConstants::RANDOMFACTION_SLOTNAME));
	factionFiles.push_back(formatString(GameConstants::RANDOMFACTION_SLOTNAME));
	translatedFactionNames.push_back("*"+lang.getString("Random","",true)+"*");

	// Add special Observer Faction
	if(checkBoxAllowObservers.getValue() == 1) {
		results.push_back(formatString(GameConstants::OBSERVER_SLOTNAME));
		factionFiles.push_back(formatString(GameConstants::OBSERVER_SLOTNAME));
		translatedFactionNames.push_back("*"+lang.getString("Observer","",true)+"*");
	}

    for(int i=0; i<GameConstants::maxPlayers; ++i){
		int originalIndex = listBoxFactions[i].getSelectedItemIndex();
		string originalValue = (listBoxFactions[i].getItemCount() > 0 ? listBoxFactions[i].getSelectedItem() : "");

    	listBoxFactions[i].setItems(results,translatedFactionNames);
        if( keepExistingSelectedItem == false ||
        	(checkBoxAllowObservers.getValue() == true &&
        			originalValue == formatString(GameConstants::OBSERVER_SLOTNAME)) ) {
        	listBoxFactions[i].setSelectedItemIndex(i % results.size());

        	if( originalValue == formatString(GameConstants::OBSERVER_SLOTNAME) &&
        		listBoxFactions[i].getSelectedItem() != formatString(GameConstants::OBSERVER_SLOTNAME)) {
    			if(listBoxTeams[i].getSelectedItem() == intToStr(GameConstants::maxPlayers + fpt_Observer)) {
    				listBoxTeams[i].setSelectedItem(intToStr(1));
    			}
        	}
        }
        else if(originalIndex < (int)results.size()) {
        	listBoxFactions[i].setSelectedItemIndex(originalIndex);
        }
    }
}

void MenuStateConnectedGame::loadGameSettings(GameSettings *gameSettings) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	int factionCount= 0;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

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

	gameSettings->setNetworkAllowNativeLanguageTechtree(checkBoxAllowNativeLanguageTechtree.getValue());

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d] listBoxMap.getSelectedItemIndex() = %d, mapFiles.size() = " MG_SIZE_T_SPECIFIER ", getCurrentMapFile() [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,listBoxMap.getSelectedItemIndex(),mapFiles.size(),getCurrentMapFile().c_str());

	if(listBoxMap.getSelectedItemIndex() >= 0 && listBoxMap.getSelectedItemIndex() < (int)mapFiles.size()) {
		gameSettings->setDescription(formatString(getCurrentMapFile()));
		gameSettings->setMap(getCurrentMapFile());
	}
	else {
    	Lang &lang= Lang::getInstance();
    	NetworkManager &networkManager= NetworkManager::getInstance();
    	ClientInterface *clientInterface = networkManager.getClientInterface();
    	const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
    	for(unsigned int i = 0; i < languageList.size(); ++i) {
			char szMsg[8096]="";
			if(lang.hasString("DataMissingMap=Player",languageList[i]) == true) {
				snprintf(szMsg,8096,lang.getString("DataMissingMap=Player",languageList[i]).c_str(),getHumanPlayerName().c_str(),listBoxMap.getSelectedItem().c_str());
			}
			else {
				snprintf(szMsg,8096,"Player: %s is missing the map: %s",getHumanPlayerName().c_str(),listBoxMap.getSelectedItem().c_str());
			}
			bool localEcho = lang.isLanguageLocal(languageList[i]);
			clientInterface->sendTextMessage(szMsg,-1, localEcho,languageList[i]);
    	}
	}

	if(listBoxTileset.getSelectedItemIndex() >= 0 && listBoxTileset.getSelectedItemIndex() < (int)tilesetFiles.size()) {
		gameSettings->setTileset(tilesetFiles[listBoxTileset.getSelectedItemIndex()]);
	}
	else {
		//printf("A loadGameSettings listBoxTileset.getSelectedItemIndex() = %d tilesetFiles.size() = %d\n",listBoxTileset.getSelectedItemIndex(),tilesetFiles.size());

    	Lang &lang= Lang::getInstance();
    	NetworkManager &networkManager= NetworkManager::getInstance();
    	ClientInterface *clientInterface = networkManager.getClientInterface();
    	const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
    	for(unsigned int i = 0; i < languageList.size(); ++i) {
			char szMsg[8096]="";
			if(lang.hasString("DataMissingTileset=Player",languageList[i]) == true) {
				snprintf(szMsg,8096,lang.getString("DataMissingTileset=Player",languageList[i]).c_str(),getHumanPlayerName().c_str(),listBoxTileset.getSelectedItem().c_str());
			}
			else {
				snprintf(szMsg,8096,"Player: %s is missing the tileset: %s",getHumanPlayerName().c_str(),listBoxTileset.getSelectedItem().c_str());
			}
			bool localEcho = lang.isLanguageLocal(languageList[i]);
			clientInterface->sendTextMessage(szMsg,-1, localEcho,languageList[i]);
    	}
	}
	if(listBoxTechTree.getSelectedItemIndex() >= 0 && listBoxTechTree.getSelectedItemIndex() < (int)techTreeFiles.size()) {
		gameSettings->setTech(techTreeFiles[listBoxTechTree.getSelectedItemIndex()]);
	}
	else {
    	Lang &lang= Lang::getInstance();
    	NetworkManager &networkManager= NetworkManager::getInstance();
    	ClientInterface *clientInterface = networkManager.getClientInterface();
    	const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
    	for(unsigned int i = 0; i < languageList.size(); ++i) {
			char szMsg[8096]="";
			if(lang.hasString("DataMissingTechtree=Player",languageList[i]) == true) {
				snprintf(szMsg,8096,lang.getString("DataMissingTechtree=Player",languageList[i]).c_str(),getHumanPlayerName().c_str(),listBoxTechTree.getSelectedItem().c_str());
			}
			else {
				snprintf(szMsg,8096,"Player: %s is missing the techtree: %s",getHumanPlayerName().c_str(),listBoxTechTree.getSelectedItem().c_str());
			}
			bool localEcho = lang.isLanguageLocal(languageList[i]);
			clientInterface->sendTextMessage(szMsg,-1, localEcho,languageList[i]);
    	}
	}

	if(checkBoxScenario.getValue() == false) {
		gameSettings->setDefaultUnits(true);
		gameSettings->setDefaultResources(true);
		gameSettings->setDefaultVictoryConditions(true);
	}

	gameSettings->setFogOfWar(listBoxFogOfWar.getSelectedItemIndex() == 0 ||
								listBoxFogOfWar.getSelectedItemIndex() == 1 );

	gameSettings->setAllowObservers(checkBoxAllowObservers.getValue() == true);

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
	//gameSettings->setPathFinderType(static_cast<PathFinderType>(listBoxPathFinderType.getSelectedItemIndex()));

	valueFlags1 = gameSettings->getFlagTypes1();
	if(checkBoxEnableSwitchTeamMode.getValue() == true) {
        valueFlags1 |= ft1_allow_team_switching;
        gameSettings->setFlagTypes1(valueFlags1);
	}
	else {
        valueFlags1 &= ~ft1_allow_team_switching;
        gameSettings->setFlagTypes1(valueFlags1);
	}
	gameSettings->setAiAcceptSwitchTeamPercentChance(strToInt(listBoxAISwitchTeamAcceptPercent.getSelectedItem()));
	gameSettings->setFallbackCpuMultiplier(listBoxFallbackCpuMultiplier.getSelectedItemIndex());

	valueFlags1 = gameSettings->getFlagTypes1();
	if(checkBoxAllowTeamUnitSharing.getValue() == true) {
        valueFlags1 |= ft1_allow_shared_team_units;
        gameSettings->setFlagTypes1(valueFlags1);
	}
	else {
        valueFlags1 &= ~ft1_allow_shared_team_units;
        gameSettings->setFlagTypes1(valueFlags1);
	}

	valueFlags1 = gameSettings->getFlagTypes1();
	if(checkBoxAllowTeamResourceSharing.getValue() == true) {
        valueFlags1 |= ft1_allow_shared_team_resources;
        gameSettings->setFlagTypes1(valueFlags1);
	}
	else {
        valueFlags1 &= ~ft1_allow_shared_team_resources;
        gameSettings->setFlagTypes1(valueFlags1);
	}

	// First save Used slots
    //for(int i=0; i<mapInfo.players; ++i)
	int AIPlayerCount = 0;
	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		ControlType ct= static_cast<ControlType>(listBoxControls[i].getSelectedItemIndex());

		if(ct != ctClosed) {
			int slotIndex = factionCount;
			ControlType oldControlType=gameSettings->getFactionControl(slotIndex);
			gameSettings->setFactionControl(slotIndex, ct);
			if(ct == ctHuman) {
				//if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] i = %d, slotIndex = %d, getHumanPlayerName(i) [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,i,slotIndex,getHumanPlayerName(i).c_str());

				gameSettings->setThisFactionIndex(slotIndex);
				gameSettings->setNetworkPlayerName(slotIndex, getHumanPlayerName());
				gameSettings->setNetworkPlayerUUID(slotIndex,Config::getInstance().getString("PlayerId",""));
				gameSettings->setNetworkPlayerPlatform(slotIndex,getPlatformNameString());
				gameSettings->setNetworkPlayerStatuses(slotIndex, getNetworkPlayerStatus());
				Lang &lang= Lang::getInstance();
				gameSettings->setNetworkPlayerLanguages(slotIndex, lang.getLanguage());

				gameSettings->setResourceMultiplierIndex(slotIndex, 5);
			}
			else {
				gameSettings->setResourceMultiplierIndex(slotIndex, listBoxRMultiplier[i].getSelectedItemIndex());
			}

			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] i = %d, factionFiles[listBoxFactions[i].getSelectedItemIndex()] [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,i,factionFiles[listBoxFactions[i].getSelectedItemIndex()].c_str());

			gameSettings->setFactionTypeName(slotIndex, factionFiles[listBoxFactions[i].getSelectedItemIndex()]);
			if(factionFiles[listBoxFactions[i].getSelectedItemIndex()] == formatString(GameConstants::OBSERVER_SLOTNAME)) {
				listBoxTeams[i].setSelectedItem(intToStr(GameConstants::maxPlayers + fpt_Observer));
			}

			gameSettings->setTeam(slotIndex, listBoxTeams[i].getSelectedItemIndex());
			gameSettings->setStartLocationIndex(slotIndex, i);
			//printf("!!! setStartLocationIndex #1 slotIndex = %d, i = %d\n",slotIndex, i);

			if(listBoxControls[i].getSelectedItemIndex() == ctNetwork || listBoxControls[i].getSelectedItemIndex() == ctNetworkUnassigned) {
				if(oldControlType!=ctNetwork && oldControlType!=ctNetworkUnassigned ){
					gameSettings->setNetworkPlayerName(slotIndex,"");
				}
			}
			else if (listBoxControls[i].getSelectedItemIndex() != ctHuman) {
				AIPlayerCount++;
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] i = %d, playername is AI (blank)\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,i);

				Lang &lang= Lang::getInstance();
				gameSettings->setNetworkPlayerName(slotIndex, lang.getString("AI") + intToStr(AIPlayerCount));
				labelPlayerNames[i].setText("");
			}

			factionCount++;
		}
		else {
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
			//printf("!!! setStartLocationIndex #2 slotIndex = %d, i = %d\n",slotIndex, i);

			gameSettings->setResourceMultiplierIndex(slotIndex, 5);

			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] i = %d, factionFiles[listBoxFactions[i].getSelectedItemIndex()] [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,i,factionFiles[listBoxFactions[i].getSelectedItemIndex()].c_str());

			gameSettings->setFactionTypeName(slotIndex, factionFiles[listBoxFactions[i].getSelectedItemIndex()]);
			gameSettings->setNetworkPlayerStatuses(slotIndex, npst_None);
			gameSettings->setNetworkPlayerName(slotIndex, "Closed");

			closedCount++;
		}
    }

	gameSettings->setFactionCount(factionCount);

	Config &config = Config::getInstance();
	gameSettings->setEnableServerControlledAI(config.getBool("ServerControlledAI","true"));
	gameSettings->setNetworkFramePeriod(config.getInt("NetworkSendFrameCount","20"));

	if(hasNetworkGameSettings() == true) {
		if( gameSettings->getTileset() != "") {
			if(lastCheckedCRCTilesetName != gameSettings->getTileset()) {
				//console.addLine("Checking tileset CRC [" + gameSettings->getTileset() + "]");
				lastCheckedCRCTilesetValue = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTilesets,""), string("/") + gameSettings->getTileset() + string("/*"), ".xml", NULL);
				if(lastCheckedCRCTilesetValue == 0 || lastCheckedCRCTilesetValue != gameSettings->getTilesetCRC()) {
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
					if(lastCheckedCRCTechtreeValue == 0 || lastCheckedCRCTechtreeValue != gameSettings->getTechCRC()) {
						lastCheckedCRCTechtreeValue = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTechs,""), "/" + gameSettings->getTech() + "/*", ".xml", NULL, true);
					}

					reloadFactions(true,gameSettings->getScenario());
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
				string file = Config::getMapPath(gameSettings->getMap(),"",false);
				//console.addLine("Checking map CRC [" + file + "]");
				checksum.addFile(file);
				lastCheckedCRCMapValue = checksum.getSum();
				lastCheckedCRCMapName = gameSettings->getMap();
			}
			gameSettings->setMapCRC(lastCheckedCRCMapValue);
		}
	}

    //replace server player by network
    for(int i= 0; i< gameSettings->getFactionCount(); ++i) {
        //replace by network
        if(gameSettings->getFactionControl(i)==ctHuman) {
            gameSettings->setFactionControl(i, ctNetwork);
        }
    }

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

void MenuStateConnectedGame::returnToJoinMenu() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	if(modHttpServerThread != NULL) {
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		modHttpServerThread->setSimpleTaskInterfaceValid(false);
		modHttpServerThread->signalQuit();
		//modHttpServerThread->setThreadOwnerValid(false);

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
		if( modHttpServerThread->canShutdown(true) == true &&
			modHttpServerThread->shutdownAndWait() == true) {
			delete modHttpServerThread;
		}
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
		modHttpServerThread = NULL;
	}

	if(ftpClientThread != NULL) {

		ftpClientThread->setCallBackObject(NULL);
		ftpClientThread->signalQuit();
		sleep(0);
		if(ftpClientThread->canShutdown(true) == true &&
				ftpClientThread->shutdownAndWait() == true) {
			delete ftpClientThread;
		}
		else {
			char szBuf[8096]="";
			snprintf(szBuf,8096,"In [%s::%s %d] Error cannot shutdown ftpClientThread\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("%s",szBuf);
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);
		}
		ftpClientThread = NULL;
	}

	if(returnMenuInfo == jmSimple) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

        launchingNewGame = true;
        disconnectFromServer();
		mainMenu->setState(new MenuStateJoinGame(program, mainMenu));
	}
	else {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

        launchingNewGame = true;
        disconnectFromServer();
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

	buttonCancelDownloads.mouseMove(x, y);
	buttonDisconnect.mouseMove(x, y);

	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
        listBoxControls[i].mouseMove(x, y);
        listBoxFactions[i].mouseMove(x, y);
		listBoxTeams[i].mouseMove(x, y);
		grabSlotButton[i].mouseMove(x, y);
    }

	listBoxMap.mouseMove(x, y);
	listBoxFogOfWar.mouseMove(x, y);
	checkBoxAllowObservers.mouseMove(x, y);
	listBoxTileset.mouseMove(x, y);
	listBoxTechTree.mouseMove(x, y);
	listBoxPlayerStatus.mouseMove(x,y);

	checkBoxScenario.mouseMove(x, y);
	listBoxScenario.mouseMove(x, y);

	labelAllowTeamUnitSharing.mouseMove(x,y);
	checkBoxAllowTeamUnitSharing.mouseMove(x,y);
	labelAllowTeamResourceSharing.mouseMove(x,y);
	checkBoxAllowTeamResourceSharing.mouseMove(x,y);

	checkBoxAllowNativeLanguageTechtree.mouseMove(x, y);

	buttonPlayNow.mouseMove(x, y);
	buttonRestoreLastSettings.mouseMove(x, y);
}

bool MenuStateConnectedGame::isVideoPlaying() {
	bool result = false;
	if(factionVideo != NULL) {
		result = factionVideo->isPlaying();
	}
	return result;
}

void MenuStateConnectedGame::render() {
	try {
		Renderer &renderer= Renderer::getInstance();

		if(isHeadlessAdmin() == true) {
			listBoxPlayerStatus.setX(buttonRestoreLastSettings.getX() +
									 buttonRestoreLastSettings.getW() + 20);
		}
		else {
			listBoxPlayerStatus.setX(nonAdminPlayerStatusX);
		}

		if(mainMessageBox.getEnabled()) {
			renderer.renderMessageBox(&mainMessageBox);
		}

		renderer.renderButton(&buttonDisconnect);

		if (initialSettingsReceivedFromServer == false) {
			return;
		}

		if(factionTexture != NULL) {
			if(factionVideo == NULL || factionVideo->isPlaying() == false) {
				renderer.renderTextureQuad(800,600,200,150,factionTexture,1);
			}
		}
		if(factionVideo != NULL) {
			if(factionVideo->isPlaying() == true) {
				factionVideo->playFrame(false);
			}
			else {
				if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false &&
					::Shared::Graphics::VideoPlayer::hasBackEndVideoPlayer() == true) {
					if(factionVideo != NULL) {
						factionVideo->closePlayer();
						delete factionVideo;
						factionVideo = NULL;

						NetworkManager &networkManager= NetworkManager::getInstance();
						ClientInterface* clientInterface= networkManager.getClientInterface();
						if(clientInterface != NULL) {
							initFactionPreview(clientInterface->getGameSettings());
						}
					}
				}
			}
		}

		if(mapPreviewTexture != NULL) {
			//renderer.renderTextureQuad(5,185,150,150,mapPreviewTexture,1.0f);
			renderer.renderTextureQuad(	this->render_mapPreviewTexture_X,
										this->render_mapPreviewTexture_Y,
										this->render_mapPreviewTexture_W,
										this->render_mapPreviewTexture_H,
										mapPreviewTexture,1.0f);
			if(this->zoomedMap==true) {
				return;
			}
			//printf("=================> Rendering map preview texture\n");
		}

		if(scenarioLogoTexture != NULL) {
			renderer.renderTextureQuad(300,350,400,300,scenarioLogoTexture,1.0f);
			//renderer.renderBackground(scenarioLogoTexture);
		}

		renderer.renderButton(&buttonDisconnect);

		// Get a reference to the player texture cache
		std::map<int,Texture2D *> &crcPlayerTextureCache = CacheManager::getCachedItem< std::map<int,Texture2D *> >(GameConstants::playerTextureCacheLookupKey);

		// START - this code ensure player title and player names don't overlap
		int offsetPosition=0;
	    for(int i=0; i < GameConstants::maxPlayers; ++i) {
			const Metrics &metrics= Metrics::getInstance();
			FontMetrics *fontMetrics= NULL;
			if(Renderer::renderText3DEnabled == false) {
				fontMetrics = CoreData::getInstance().getMenuFontNormal()->getMetrics();
			}
			else {
				fontMetrics = CoreData::getInstance().getMenuFontNormal3D()->getMetrics();
			}

			if(fontMetrics == NULL) {
				throw megaglest_runtime_error("fontMetrics == NULL");
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

	    NetworkManager &networkManager= NetworkManager::getInstance();
	    ClientInterface *clientInterface = networkManager.getClientInterface();
		for(int i = 0; i < GameConstants::maxPlayers; ++i) {
	    	if(listBoxControls[i].getSelectedItemIndex() == ctNetworkUnassigned) {
	    		//printf("Player #%d [%s] control = %d\n",i,labelPlayerNames[i].getText().c_str(),listBoxControls[i].getSelectedItemIndex());

				labelPlayers[i].setVisible(true);
				labelPlayerNames[i].setVisible(true);
				listBoxControls[i].setVisible(true);
				listBoxRMultiplier[i].setVisible(true);
				listBoxFactions[i].setVisible(true);
				listBoxTeams[i].setVisible(true);
				labelNetStatus[i].setVisible(true);
	    	}

			if(listBoxControls[i].getSelectedItemIndex() != ctClosed) {
				renderer.renderLabel(&labelPlayerStatus[i]);
			}

			if(crcPlayerTextureCache[i] != NULL) {
				// Render the player # label the player's color

				Vec3f playerColor = crcPlayerTextureCache[i]->getPixmap()->getPixel3f(0, 0);
				if(clientInterface != NULL &&
					clientInterface->getGameSettings() != NULL &&
					clientInterface->getGameSettings()->getMasterserver_admin() > 0 &&
					clientInterface->getGameSettings()->getMasterserver_admin_faction_index() == i) {

					if(difftime((long int)time(NULL),timerLabelFlash) < 1) {
						renderer.renderLabel(&labelPlayers[i],&playerColor);
					}
					else {
						Vec4f flashColor=Vec4f(playerColor.x, playerColor.y, playerColor.z, 0.45f);
			            renderer.renderLabel(&labelPlayers[i],&flashColor);
					}

				}
				else {
					renderer.renderLabel(&labelPlayers[i],&playerColor);
				}

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
			if(listBoxControls[i].getSelectedItemIndex() != ctClosed) {
				renderer.renderListBox(&listBoxRMultiplier[i]);
				renderer.renderListBox(&listBoxFactions[i]);
				renderer.renderListBox(&listBoxTeams[i]);

				bool canGrabSlot = false;
				ClientInterface *clientInterface = networkManager.getClientInterface();
				if(clientInterface != NULL && clientInterface->getJoinGameInProgress() == true) {
					canGrabSlot = ((listBoxControls[i].getSelectedItemIndex() == ctNetwork &&
							labelNetStatus[i].getText() == GameConstants::NETWORK_SLOT_UNCONNECTED_SLOTNAME) ||
							(listBoxControls[i].getSelectedItemIndex() != ctHuman &&
							 listBoxControls[i].getSelectedItemIndex() != ctClosed &&
							 listBoxControls[i].getSelectedItemIndex() != ctNetwork));
				}
				else {
					canGrabSlot = (listBoxControls[i].getSelectedItemIndex() == ctNetwork &&
										labelNetStatus[i].getText() == GameConstants::NETWORK_SLOT_UNCONNECTED_SLOTNAME);
				}

				if(canGrabSlot == true) {
					if(i < mapInfo.players) {
						renderer.renderButton(&grabSlotButton[i]);
					}
				}
				else if(listBoxControls[i].getSelectedItemIndex() == ctNetwork ||
						listBoxControls[i].getSelectedItemIndex() == ctNetworkUnassigned ||
					    listBoxControls[i].getSelectedItemIndex() == ctHuman){
					renderer.renderLabel(&labelNetStatus[i]);
				}

				if(listBoxControls[i].getSelectedItemIndex() == ctNetwork ||
				   listBoxControls[i].getSelectedItemIndex() == ctNetworkUnassigned ||
				   listBoxControls[i].getSelectedItemIndex() == ctHuman){
					if(labelNetStatus[i].getText() != GameConstants::NETWORK_SLOT_UNCONNECTED_SLOTNAME) {
						renderer.renderLabel(&labelPlayerNames[i]);
					}
				}
			}
		}
		renderer.renderLabel(&labelStatus);
		renderer.renderLabel(&labelInfo);

		if(difftime((long int)time(NULL),timerLabelFlash) < 1) {
		    renderer.renderLabel(&labelDataSynchInfo,&RED);
		}
		else {
            renderer.renderLabel(&labelDataSynchInfo,&WHITE);
		}

		renderer.renderLabel(&labelMap);
		renderer.renderLabel(&labelFogOfWar);
		renderer.renderLabel(&labelAllowObservers);
		renderer.renderLabel(&labelFallbackCpuMultiplier);
		renderer.renderLabel(&labelTileset);
		renderer.renderLabel(&labelTechTree);
		renderer.renderLabel(&labelControl);
		renderer.renderLabel(&labelFaction);
		renderer.renderLabel(&labelTeam);
		renderer.renderLabel(&labelMapInfo);

		renderer.renderListBox(&listBoxMap);
		renderer.renderListBox(&listBoxFogOfWar);
		renderer.renderCheckBox(&checkBoxAllowObservers);
		renderer.renderListBox(&listBoxTileset);
		renderer.renderListBox(&listBoxTechTree);

		renderer.renderLabel(&labelEnableSwitchTeamMode);
		renderer.renderLabel(&labelAISwitchTeamAcceptPercent);

		renderer.renderCheckBox(&checkBoxEnableSwitchTeamMode);
		renderer.renderListBox(&listBoxAISwitchTeamAcceptPercent);
		renderer.renderListBox(&listBoxFallbackCpuMultiplier);

		renderer.renderLabel(&labelAllowTeamUnitSharing);
		renderer.renderCheckBox(&checkBoxAllowTeamUnitSharing);

		renderer.renderLabel(&labelAllowTeamResourceSharing);
		renderer.renderCheckBox(&checkBoxAllowTeamResourceSharing);

		renderer.renderButton(&buttonPlayNow);
		renderer.renderButton(&buttonRestoreLastSettings);

		renderer.renderCheckBox(&checkBoxScenario);
		renderer.renderLabel(&labelScenario);
		if(checkBoxScenario.getValue() == true) {
			renderer.renderListBox(&listBoxScenario);
		}

		renderer.renderLabel(&labelAllowNativeLanguageTechtree);
		renderer.renderCheckBox(&checkBoxAllowNativeLanguageTechtree);

        MutexSafeWrapper safeMutexFTPProgress((ftpClientThread != NULL ? ftpClientThread->getProgressMutex() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));

        // !!! START TEMP MV
        //renderer.renderButton(&buttonCancelDownloads);
        //fileFTPProgressList.clear();
        //fileFTPProgressList["test1a dsa asd asda sdasd asd ad ad"] = make_pair(1,"testa");
        //fileFTPProgressList["test2 asdasdasdadas dasdasdasda"] = make_pair(1,"testb");
        //fileFTPProgressList["test3 asdasdad asd ada dasdadasdada"] = make_pair(1,"testc");
        // !!! END TEMP MV

        if(fileFTPProgressList.empty() == false) {
        	Lang &lang= Lang::getInstance();
        	renderer.renderButton(&buttonCancelDownloads);
        	int xLocation = buttonCancelDownloads.getX();
            int yLocation = buttonCancelDownloads.getY() - 20;
            for(std::map<string,pair<int,string> >::iterator iterMap = fileFTPProgressList.begin();
                iterMap != fileFTPProgressList.end(); ++iterMap) {
                string progressLabelPrefix = lang.getString("ModDownloading") + " " + iterMap->first + " ";
                //if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\nRendering file progress with the following prefix [%s]\n",progressLabelPrefix.c_str());

                if(Renderer::renderText3DEnabled) {
					renderer.renderProgressBar3D(
						iterMap->second.first,
						xLocation,
						//10,
						yLocation,
						CoreData::getInstance().getDisplayFontSmall3D(),
						//350,progressLabelPrefix);
						300,progressLabelPrefix);
                }
                else {
					renderer.renderProgressBar(
						iterMap->second.first,
						//10,
						xLocation,
						yLocation,
						CoreData::getInstance().getDisplayFontSmall(),
						//350,progressLabelPrefix);
						300,progressLabelPrefix);
                }

                yLocation -= 20;
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

			if(mapPreviewTexture == NULL) {
				renderer.renderMouse2d(mouseX, mouseY, mouse2dAnim);

				bool renderAll = (listBoxFogOfWar.getSelectedItemIndex() == 2);
				//renderer.renderMapPreview(&mapPreview, renderAll, 10, 350, &mapPreviewTexture);
		    	renderer.renderMapPreview(&mapPreview, renderAll,
		    			this->render_mapPreviewTexture_X,
		    			this->render_mapPreviewTexture_Y,
		    			&mapPreviewTexture);
			}
		}
		renderer.renderChatManager(&chatManager);
		renderer.renderConsole(&console,showFullConsole,true);

        if(difftime((long int)time(NULL),timerLabelFlash) > 2) {
            timerLabelFlash = time(NULL);
        }
	}
	catch(const std::exception &ex) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s %d]\nError detected:\n%s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		throw megaglest_runtime_error(szBuf);
	}
}

void MenuStateConnectedGame::update() {
	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

	Lang &lang= Lang::getInstance();
	ClientInterface *clientInterface= NetworkManager::getInstance().getClientInterface();

	string newLabelConnectionInfo = lang.getString("WaitingHost");
	if(clientInterface != NULL && clientInterface->getJoinGameInProgress() == true) {
		newLabelConnectionInfo = lang.getString("MGGameStatus2");
	}
	// Test progress bar
    //MutexSafeWrapper safeMutexFTPProgress((ftpClientThread != NULL ? ftpClientThread->getProgressMutex() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
    //fileFTPProgressList["test"] = pair<int,string>(difftime(time(NULL),lastNetworkSendPing) * 20,"test file 123");
    //safeMutexFTPProgress.ReleaseLock();
    //

	if(clientInterface != NULL && clientInterface->isConnected()) {
		//printf("#2 admin key [%d] client key [%d]\n",settings->getMasterserver_admin(),clientInterface->getSessionKey());
		broadCastGameSettingsToHeadlessServer(false);

		checkBoxAllowNativeLanguageTechtree.setEditable(isHeadlessAdmin());
		checkBoxAllowNativeLanguageTechtree.setEnabled(isHeadlessAdmin());

		listBoxMap.setEditable(isHeadlessAdmin());
		buttonPlayNow.setVisible(isHeadlessAdmin() ||
				clientInterface->getJoinGameInProgress() == true);
		buttonRestoreLastSettings.setVisible(isHeadlessAdmin());
		listBoxTechTree.setEditable(isHeadlessAdmin());
		listBoxTileset.setEditable(isHeadlessAdmin());
		checkBoxEnableSwitchTeamMode.setEditable(isHeadlessAdmin());
		listBoxAISwitchTeamAcceptPercent.setEditable(isHeadlessAdmin());
		listBoxFallbackCpuMultiplier.setEditable(isHeadlessAdmin());
		listBoxFogOfWar.setEditable(isHeadlessAdmin());
		checkBoxAllowObservers.setEditable(isHeadlessAdmin());

		checkBoxAllowTeamUnitSharing.setEditable(isHeadlessAdmin());
		checkBoxAllowTeamResourceSharing.setEditable(isHeadlessAdmin());

		if(isHeadlessAdmin() == true) {
			for(unsigned int i = 0; i < (unsigned int)GameConstants::maxPlayers; ++i) {
				listBoxControls[i].setEditable(isHeadlessAdmin());
				listBoxRMultiplier[i].setEditable(isHeadlessAdmin());
				listBoxFactions[i].setEditable(isHeadlessAdmin());
				listBoxTeams[i].setEditable(isHeadlessAdmin());
			}
		}

		if(difftime((long int)time(NULL),lastNetworkSendPing) >= GameConstants::networkPingInterval) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] about to sendPingMessage...\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

			lastNetworkSendPing = time(NULL);
			clientInterface->sendPingMessage(GameConstants::networkPingInterval, (int64)time(NULL));

			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] pingCount = %d, clientInterface->getLastPingLag() = %f, GameConstants::networkPingInterval = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,pingCount, clientInterface->getLastPingLag(),GameConstants::networkPingInterval);

			// Starting checking timeout after sending at least 3 pings to server
			if(clientInterface->isConnected() &&
				pingCount >= MAX_PING_LAG_COUNT && clientInterface->getLastPingLag() >= (GameConstants::networkPingInterval * MAX_PING_LAG_COUNT)) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

				clientInterface->updateLobby();

				if(clientInterface->isConnected() && clientInterface->getJoinGameInProgress() == false &&
					pingCount >= MAX_PING_LAG_COUNT && clientInterface->getLastPingLag() >= (GameConstants::networkPingInterval * MAX_PING_LAG_COUNT)) {
					MutexSafeWrapper safeMutexFTPProgress((ftpClientThread != NULL ? ftpClientThread->getProgressMutex() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
					if(fileFTPProgressList.empty() == true) {
						Lang &lang= Lang::getInstance();
						const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
						for(unsigned int i = 0; i < languageList.size(); ++i) {
							clientInterface->sendTextMessage(lang.getString("ConnectionTimedOut",languageList[i]) + " : " + doubleToStr(clientInterface->getLastPingLag(),2),-1,false,languageList[i]);
							sleep(1);
							clientInterface->close();
						}
					}
				}
			}

			pingCount++;
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		}
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

	//update status label
	if(clientInterface != NULL && clientInterface->isConnected()) {
		buttonDisconnect.setText(lang.getString("Disconnect"));

		if(clientInterface->getAllowDownloadDataSynch() == false) {
		    string label = lang.getString("ConnectedToServer");

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

                uint32 tilesetCRC = lastCheckedCRCTilesetValue;
                if(lastCheckedCRCTilesetName != gameSettings->getTileset() &&
                	gameSettings->getTileset() != "") {
					//console.addLine("Checking tileset CRC [" + gameSettings->getTileset() + "]");
                	tilesetCRC = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTilesets,""), string("/") + gameSettings->getTileset() + string("/*"), ".xml", NULL);
                	if(tilesetCRC == 0 || tilesetCRC != gameSettings->getTilesetCRC()) {
                		tilesetCRC = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTilesets,""), string("/") + gameSettings->getTileset() + string("/*"), ".xml", NULL, true);
                	}

					// Test data synch
					//tilesetCRC++;
					lastCheckedCRCTilesetValue = tilesetCRC;
					lastCheckedCRCTilesetName = gameSettings->getTileset();
                }

                uint32 techCRC = lastCheckedCRCTechtreeValue;
                if(lastCheckedCRCTechtreeName != gameSettings->getTech() &&
                	gameSettings->getTech() != "") {
					//console.addLine("Checking techtree CRC [" + gameSettings->getTech() + "]");
					techCRC = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTechs,""), string("/") + gameSettings->getTech() + string("/*"), ".xml", NULL);
					//clientInterface->sendTextMessage("#1 TechCRC = " + intToStr(techCRC) + " remoteCRC = " + intToStr(gameSettings->getTechCRC()),-1, true, "");

					if(techCRC == 0 || techCRC != gameSettings->getTechCRC()) {
						techCRC = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTechs,""), string("/") + gameSettings->getTech() + string("/*"), ".xml", NULL, true);
						//clientInterface->sendTextMessage("#2 TechCRC = " + intToStr(techCRC) + " remoteCRC = " + intToStr(gameSettings->getTechCRC()),-1, true, "");
					}


                    if(techCRC != 0 && techCRC != gameSettings->getTechCRC() &&
                    	listBoxTechTree.getSelectedItemIndex() >= 0 &&
                    	listBoxTechTree.getSelectedItem() != Lang::getInstance().getString("DataMissing","",true)) {

                    	//time_t now = time(NULL);
                    	time_t lastUpdateDate = getFolderTreeContentsCheckSumRecursivelyLastGenerated(config.getPathListForType(ptTechs,""), string("/") + gameSettings->getTech() + string("/*"), ".xml");

                    	const time_t REFRESH_CRC_DAY_SECONDS = 60 * 60 * 1;
            			if(	lastUpdateDate <= 0 ||
            				difftime((long int)time(NULL),lastUpdateDate) >= REFRESH_CRC_DAY_SECONDS) {
            				techCRC = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTechs,""), string("/") + gameSettings->getTech() + string("/*"), ".xml", NULL, true);
            				//clientInterface->sendTextMessage("#3 TechCRC = " + intToStr(techCRC) + " remoteCRC = " + intToStr(gameSettings->getTechCRC()),-1, true, "");
            			}
                    }

					// Test data synch
					//techCRC++;
					lastCheckedCRCTechtreeValue = techCRC;
					lastCheckedCRCTechtreeName = gameSettings->getTech();

					loadFactions(gameSettings,false);
	    			factionCRCList.clear();
	    			for(unsigned int factionIdx = 0; factionIdx < factionFiles.size(); ++factionIdx) {
	    				string factionName = factionFiles[factionIdx];
	    				if(factionName != GameConstants::RANDOMFACTION_SLOTNAME &&
	    					factionName != GameConstants::OBSERVER_SLOTNAME &&
	    					factionName != Lang::getInstance().getString("DataMissing","",true)) {

	    					uint32 factionCRC   = 0;
	                    	//time_t now = time(NULL);
	                    	time_t lastUpdateDate = getFolderTreeContentsCheckSumRecursivelyLastGenerated(config.getPathListForType(ptTechs,""), "/" + gameSettings->getTech() + "/factions/" + factionName + "/*", ".xml");

	                    	const time_t REFRESH_CRC_DAY_SECONDS = 60 * 60 * 24;
	            			if(	lastUpdateDate <= 0 ||
	            				difftime((long int)time(NULL),lastUpdateDate) >= REFRESH_CRC_DAY_SECONDS ||
	            				(techCRC != 0 && techCRC != gameSettings->getTechCRC())) {
	            				factionCRC   = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTechs,""), "/" + gameSettings->getTech() + "/factions/" + factionName + "/*", ".xml", NULL, true);
	            			}
	            			else {
	            				factionCRC   = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTechs,""), "/" + gameSettings->getTech() + "/factions/" + factionName + "/*", ".xml", NULL);
	            			}
	            			if(factionCRC == 0) {
	            				factionCRC   = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTechs,""), "/" + gameSettings->getTech() + "/factions/" + factionName + "/*", ".xml", NULL, true);
	            			}

	            			if(factionCRC != 0) {
								vector<pair<string,uint32> > serverFactionCRCList = gameSettings->getFactionCRCList();
								for(unsigned int factionIdx1 = 0; factionIdx1 < serverFactionCRCList.size(); ++factionIdx1) {
									pair<string,uint32> &serverFaction = serverFactionCRCList[factionIdx1];
									if(serverFaction.first == factionName) {
										if(serverFaction.second != factionCRC) {
											factionCRC   = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTechs,""), "/" + gameSettings->getTech() + "/factions/" + factionName + "/*", ".xml", NULL, true);
										}
										break;
									}
								}
	            			}
	            			factionCRCList.push_back(make_pair(factionName,factionCRC));
	    				}
	    			}
	    			//console.addLine("Found factions: " + intToStr(factionCRCList.size()));
                }

                uint32 mapCRC = lastCheckedCRCMapValue;
                if(lastCheckedCRCMapName != gameSettings->getMap() &&
                	gameSettings->getMap() != "") {
					Checksum checksum;
					string file = Config::getMapPath(gameSettings->getMap(),"",false);
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
                    //printf("Data not synched: lmap %u rmap: %u ltile: %d rtile: %u ltech: %u rtech: %u\n",mapCRC,gameSettings->getMapCRC(),tilesetCRC,gameSettings->getTilesetCRC(),techCRC,gameSettings->getTechCRC());

                    string labelSynch = lang.getString("DataNotSynchedTitle");

                    if(mapCRC != 0 && mapCRC != gameSettings->getMapCRC() &&
                    		listBoxMap.getSelectedItemIndex() >= 0 &&
                    		listBoxMap.getSelectedItem() != Lang::getInstance().getString("DataMissing","",true)) {
                        labelSynch = labelSynch + " " + lang.getString("Map");

                        if(updateDataSynchDetailText == true &&
                            lastMapDataSynchError != lang.getString("DataNotSynchedMap") + " " + listBoxMap.getSelectedItem()) {
                            lastMapDataSynchError = lang.getString("DataNotSynchedMap") + " " + listBoxMap.getSelectedItem();

            		    	Lang &lang= Lang::getInstance();
            		    	const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
            		    	for(unsigned int i = 0; i < languageList.size(); ++i) {
            		    		string msg = lang.getString("DataNotSynchedMap",languageList[i]) + " " + listBoxMap.getSelectedItem();
            		    		bool localEcho = lang.isLanguageLocal(languageList[i]);
            		    		clientInterface->sendTextMessage(msg,-1,localEcho,languageList[i]);
            		    	}
                        }
                    }

                    if(tilesetCRC != 0 && tilesetCRC != gameSettings->getTilesetCRC() &&
                    		listBoxTileset.getSelectedItemIndex() >= 0 &&
                    		listBoxTileset.getSelectedItem() != Lang::getInstance().getString("DataMissing","",true)) {
                        labelSynch = labelSynch + " " + lang.getString("Tileset");
                        if(updateDataSynchDetailText == true &&
                            lastTileDataSynchError != lang.getString("DataNotSynchedTileset") + " " + listBoxTileset.getSelectedItem()) {
                            lastTileDataSynchError = lang.getString("DataNotSynchedTileset") + " " + listBoxTileset.getSelectedItem();

            		    	Lang &lang= Lang::getInstance();
            		    	const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
            		    	for(unsigned int i = 0; i < languageList.size(); ++i) {
            		    		string msg = lang.getString("DataNotSynchedTileset",languageList[i]) + " " + listBoxTileset.getSelectedItem();
            		    		bool localEcho = lang.isLanguageLocal(languageList[i]);
            		    		clientInterface->sendTextMessage(msg,-1,localEcho,languageList[i]);
            		    	}
                        }
                    }

                    if(techCRC != 0 && techCRC != gameSettings->getTechCRC() &&
                    		listBoxTechTree.getSelectedItemIndex() >= 0 &&
                    		listBoxTechTree.getSelectedItem() != Lang::getInstance().getString("DataMissing","",true)) {
                        labelSynch = labelSynch + " " + lang.getString("TechTree");
                        if(updateDataSynchDetailText == true &&
                        	lastTechtreeDataSynchError != lang.getString("DataNotSynchedTechtree") + " " + listBoxTechTree.getSelectedItem()) {
                        	lastTechtreeDataSynchError = lang.getString("DataNotSynchedTechtree") + " " + listBoxTechTree.getSelectedItem();

            		    	Lang &lang= Lang::getInstance();
            		    	const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
            		    	for(unsigned int i = 0; i < languageList.size(); ++i) {
            		    		string msg = lang.getString("DataNotSynchedTechtree",languageList[i]) + " " + listBoxTechTree.getSelectedItem();
            		    		bool localEcho = lang.isLanguageLocal(languageList[i]);
            		    		clientInterface->sendTextMessage(msg,-1,localEcho,languageList[i]);
            		    	}

            		    	const int MAX_CHAT_TEXT_LINE_LENGTH = 110;
            		    	//const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
            		    	for(unsigned int i = 0; i < languageList.size(); ++i) {
            		    		bool localEcho = lang.isLanguageLocal(languageList[i]);

								string mismatchedFactionText = "";
								vector<string> mismatchedFactionTextList;
								vector<pair<string,uint32> > serverFactionCRCList = gameSettings->getFactionCRCList();

								for(unsigned int factionIdx = 0; factionIdx < serverFactionCRCList.size(); ++factionIdx) {
									pair<string,uint32> &serverFaction = serverFactionCRCList[factionIdx];

									bool foundFaction = false;
									for(unsigned int clientFactionIdx = 0; clientFactionIdx < factionCRCList.size(); ++clientFactionIdx) {
										pair<string,uint32> &clientFaction = factionCRCList[clientFactionIdx];

										if(serverFaction.first == clientFaction.first) {
											foundFaction = true;
											if(serverFaction.second != clientFaction.second) {
												if(mismatchedFactionText.length() >= 10) {
													mismatchedFactionTextList.push_back(mismatchedFactionText);
													mismatchedFactionText = "";
												}
												if(mismatchedFactionText == "") {
													mismatchedFactionText = "The following factions are mismatched: ";
													if(lang.hasString("MismatchedFactions",languageList[i]) == true) {
														mismatchedFactionText = lang.getString("MismatchedFactions",languageList[i]);
													}

													mismatchedFactionText += " ["+ intToStr(factionCRCList.size()) + "][" + intToStr(serverFactionCRCList.size()) + "] - ";
												}
												else {
													mismatchedFactionText += ", ";
												}
												mismatchedFactionText += serverFaction.first;
											}
											break;
										}
									}

									if(foundFaction == false) {
										if((int)mismatchedFactionText.length() > MAX_CHAT_TEXT_LINE_LENGTH) {
											mismatchedFactionTextList.push_back(mismatchedFactionText);
											mismatchedFactionText = "";
										}

										if(mismatchedFactionText == "") {
											mismatchedFactionText = "The following factions are mismatched: ";
											if(lang.hasString("MismatchedFactions",languageList[i]) == true) {
												mismatchedFactionText = lang.getString("MismatchedFactions",languageList[i]);
											}

											mismatchedFactionText += " ["+ intToStr(factionCRCList.size()) + "][" + intToStr(serverFactionCRCList.size()) + "] - ";
										}
										else {
											mismatchedFactionText += ", ";
										}

										if(lang.hasString("MismatchedFactionsMissing",languageList[i]) == true) {
											mismatchedFactionText += serverFaction.first + " " + lang.getString("MismatchedFactionsMissing",languageList[i]);
										}
										else {
											mismatchedFactionText += serverFaction.first + " (missing)";
										}
									}
								}

								for(unsigned int clientFactionIdx = 0; clientFactionIdx < factionCRCList.size(); ++clientFactionIdx) {
									pair<string,uint32> &clientFaction = factionCRCList[clientFactionIdx];

									bool foundFaction = false;
									for(unsigned int factionIdx = 0; factionIdx < serverFactionCRCList.size(); ++factionIdx) {
										pair<string,uint32> &serverFaction = serverFactionCRCList[factionIdx];

										if(serverFaction.first == clientFaction.first) {
											foundFaction = true;
											break;
										}
									}

									if(foundFaction == false) {
										if((int)mismatchedFactionText.length() > MAX_CHAT_TEXT_LINE_LENGTH) {
											mismatchedFactionTextList.push_back(mismatchedFactionText);
											mismatchedFactionText = "";
										}

										if(mismatchedFactionText == "") {
											mismatchedFactionText = "The following factions are mismatched: ";
											if(lang.hasString("MismatchedFactions",languageList[i]) == true) {
												mismatchedFactionText = lang.getString("MismatchedFactions",languageList[i]);
											}

											mismatchedFactionText += " ["+ intToStr(factionCRCList.size()) + "][" + intToStr(serverFactionCRCList.size()) + "] - ";
										}
										else {
											mismatchedFactionText += ", ";
										}

										if(lang.hasString("MismatchedFactionsExtra",languageList[i]) == true) {
											mismatchedFactionText += clientFaction.first + " " + lang.getString("MismatchedFactionsExtra",languageList[i]);
										}
										else {
											mismatchedFactionText += clientFaction.first + " (extra)";
										}
									}
								}

								if(mismatchedFactionText != "") {
									if(mismatchedFactionTextList.empty() == false) {
										if(mismatchedFactionText != "") {
											mismatchedFactionText += ".";
											mismatchedFactionTextList.push_back(mismatchedFactionText);
										}
										for(unsigned int splitIdx = 0; splitIdx < mismatchedFactionTextList.size(); ++splitIdx) {
											clientInterface->sendTextMessage(mismatchedFactionTextList[splitIdx],-1,localEcho,languageList[i]);
										}
									}
									else {
										mismatchedFactionText += ".";
										clientInterface->sendTextMessage(mismatchedFactionText,-1,localEcho,languageList[i]);
									}
								}
            		    	}
                        }
                    }

                    updateDataSynchDetailText = false;
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

							if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] report: %s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,report.c_str());

							clientInterface->sendTextMessage("techtree CRC mismatch",-1,true,"");
							vector<string> reportLineTokens;
							Tokenize(report,reportLineTokens,"\n");
							for(int reportLine = 0; reportLine < (int)reportLineTokens.size(); ++reportLine) {
								clientInterface->sendTextMessage(reportLineTokens[reportLine],-1,true,"");
							}
						}
                    }
                }

                if(clientInterface->getReceivedDataSynchCheck() == true) {
                	updateDataSynchDetailText = false;
                }
            }
            else if(clientInterface->getAllowGameDataSynchCheck() == true) {
                label += " - data synch is ok";
            }

            labelStatus.setText(label);
		}
		else {
		    string label = lang.getString("ConnectedToServer");

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
            }
            else if(clientInterface->getAllowGameDataSynchCheck() == true)
            {
                label += " - data synch is ok";
            }

            labelStatus.setText(label);
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();
	}
	else {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		if(clientInterface != NULL && clientInterface->isConnected() == true) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		    clientInterface->close();
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		returnToJoinMenu();
		return;
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

	//process network messages
	if(clientInterface != NULL && clientInterface->isConnected()) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

		try {
			if(clientInterface->getGameSettingsReceived() &&
					lastGameSettingsReceivedCount != clientInterface->getGameSettingsReceivedCount()) {
				broadCastGameSettingsToHeadlessServer(needToBroadcastServerSettings);

				lastGameSettingsReceivedCount = clientInterface->getGameSettingsReceivedCount();
				bool errorOnMissingData = (clientInterface->getAllowGameDataSynchCheck() == false);
				GameSettings *gameSettings = clientInterface->getGameSettingsPtr();

				//printf("Menu got new settings thisfactionindex = %d startlocation: %d control = %d\n",gameSettings->getThisFactionIndex(),clientInterface->getGameSettings()->getStartLocationIndex(clientInterface->getGameSettings()->getThisFactionIndex()),gameSettings->getFactionControl(clientInterface->getGameSettings()->getThisFactionIndex()));

				setupUIFromGameSettings(gameSettings, errorOnMissingData);
			}

			// check if we are joining an in progress game
			if( clientInterface->getJoinGameInProgress() == true &&
				clientInterface->getJoinGameInProgressLaunch() == true &&
			    clientInterface->getReadyForInGameJoin() == true &&
			   ftpClientThread != NULL) {

				MutexSafeWrapper safeMutexFTPProgress((ftpClientThread != NULL ? ftpClientThread->getProgressMutex() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
				if(readyToJoinInProgressGame == false) {
					if(getInProgressSavedGameFromFTPServer == "") {

						getInProgressSavedGameFromFTPServerInProgress = true;
						ftpClientThread->addTempFileToRequests(
								GameConstants::saveNetworkGameFileClientCompressed,
								GameConstants::saveNetworkGameFileServerCompressed);

						getInProgressSavedGameFromFTPServer = GameConstants::saveNetworkGameFileServerCompressed;
						fileFTPProgressList[getInProgressSavedGameFromFTPServer] = pair<int,string>(0,"");
					}
					safeMutexFTPProgress.ReleaseLock();
				}
				else {
					safeMutexFTPProgress.ReleaseLock();

					string saveGameFile = "temp/" + string(GameConstants::saveNetworkGameFileClient);
					if(getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) != "") {
						saveGameFile = getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) + saveGameFile;
					}
					else {
						string userData = Config::getInstance().getString("UserData_Root","");
						if(userData != "") {
							endPathWithSlash(userData);
						}
						saveGameFile = userData + saveGameFile;
					}

					//printf("Loading saved game file [%s]\n",saveGameFile.c_str());

					GameSettings gameSettings = *clientInterface->getGameSettings();
					loadGameSettings(&gameSettings);

					Game::loadGame(saveGameFile,program,false,&gameSettings);
					return;
				}
			}

			//update lobby
			clientInterface= NetworkManager::getInstance().getClientInterface();
			if(clientInterface != NULL && clientInterface->isConnected()) {
				clientInterface->updateLobby();

				if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());
				if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();
			}

			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());
			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

			clientInterface= NetworkManager::getInstance().getClientInterface();
			if(clientInterface != NULL && clientInterface->isConnected()) {
				if(	initialSettingsReceivedFromServer == true &&
					clientInterface->getIntroDone() == true &&
					(switchSetupRequestFlagType & ssrft_NetworkPlayerName) == ssrft_NetworkPlayerName) {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] getHumanPlayerName() = [%s], clientInterface->getGameSettings()->getThisFactionIndex() = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,getHumanPlayerName().c_str(),clientInterface->getGameSettings()->getThisFactionIndex());
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

				if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());
				if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

				//intro
				if(clientInterface->getIntroDone()) {
					if(newLabelConnectionInfo != labelInfo.getText()) {
						if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
						labelInfo.setText(newLabelConnectionInfo);
					}
				}

				//launch
				if(clientInterface->getLaunchGame()) {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

					assert(clientInterface != NULL);

					if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

					if(modHttpServerThread != NULL) {
						if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
						if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
						modHttpServerThread->setSimpleTaskInterfaceValid(false);
						modHttpServerThread->signalQuit();

						if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
						if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
						if( modHttpServerThread->canShutdown(true) == true &&
							modHttpServerThread->shutdownAndWait() == true) {
							delete modHttpServerThread;
						}
						if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
						modHttpServerThread = NULL;
					}

                    if(ftpClientThread != NULL) {
						ftpClientThread->setCallBackObject(NULL);
						ftpClientThread->signalQuit();
						sleep(0);
						if(ftpClientThread->canShutdown(true) == true &&
								ftpClientThread->shutdownAndWait() == true) {
							delete ftpClientThread;
						}
						else {
							char szBuf[8096]="";
							snprintf(szBuf,8096,"In [%s::%s %d] Error cannot shutdown ftpClientThread\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
							if(SystemFlags::VERBOSE_MODE_ENABLED) printf("%s",szBuf);
							if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);
						}
						ftpClientThread = NULL;
                    }

                    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

                    launchingNewGame = true;
					program->setState(new Game(program, clientInterface->getGameSettings(),false));
					return;
				}
			}

			//call the chat manager
			chatManager.updateNetwork();

			//console732
            console.update();

            // check for need to switch music on again
			if(clientInterface != NULL) {
				GameSettings *gameSettings = clientInterface->getGameSettingsPtr();
				int currentConnectionCount=0;
				for(int i=0; i < GameConstants::maxPlayers; ++i) {
					if(gameSettings->getFactionControl(i)==ctNetwork &&
							gameSettings->getNetworkPlayerName(i) != "" &&
							gameSettings->getNetworkPlayerName(i) != GameConstants::NETWORK_SLOT_UNCONNECTED_SLOTNAME)
					{
						currentConnectionCount++;
					}
				}
    			if(currentConnectionCount > soundConnectionCount){
    				soundConnectionCount = currentConnectionCount;
    				SoundRenderer::getInstance().playFx(CoreData::getInstance().getAttentionSound());
    				//switch on music again!!
    				Config &config = Config::getInstance();
    				float configVolume = (config.getInt("SoundVolumeMusic") / 100.f);
    				CoreData::getInstance().getMenuMusic()->setVolume(configVolume);
    			}
    			soundConnectionCount = currentConnectionCount;
			}


		}
		catch(const runtime_error &ex) {
			char szBuf[8096]="";
			snprintf(szBuf,8096,"In [%s::%s %d]\nError detected:\n%s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
			SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] %s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,szBuf);
			showMessageBox( szBuf, "Error", false);
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();
	}
}

bool MenuStateConnectedGame::loadFactions(const GameSettings *gameSettings, bool errorOnNoFactions) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	Lang &lang= Lang::getInstance();
	bool foundFactions = false;
	vector<string> results;

	string scenarioDir = Scenario::getScenarioDir(dirList, gameSettings->getScenario());
	if(gameSettings->getTech() != "") {
		Config &config = Config::getInstance();

		vector<string> techPaths = config.getPathListForType(ptTechs,scenarioDir);
		for(int idx = 0; idx < (int)techPaths.size(); idx++) {
			string &techPath = techPaths[idx];
			endPathWithSlash(techPath);
			//findAll(techPath + gameSettings->getTech() + "/factions/*.", results, false, false);
			findDirs(techPath + gameSettings->getTech() + "/factions/", results, false, false);
			if(results.empty() == false) {
				break;
			}
		}

		if(results.empty() == true) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

			NetworkManager &networkManager= NetworkManager::getInstance();
			ClientInterface* clientInterface= networkManager.getClientInterface();
			if(clientInterface->getAllowGameDataSynchCheck() == false) {
				if(errorOnNoFactions == true) {
					throw megaglest_runtime_error("(2)There are no factions for the tech tree [" + gameSettings->getTech() + "]");
				}
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] (2)There are no factions for the tech tree [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,gameSettings->getTech().c_str());
			}
			results.push_back(Lang::getInstance().getString("DataMissing","",true));
			factionFiles = results;
		    vector<string> translatedFactionNames;
		    for(int i= 0; i < (int)factionFiles.size(); ++i) {
		    	results[i]= formatString(results[i]);
				string translatedString=techTree->getTranslatedFactionName(gameSettings->getTech(),factionFiles[i]);
				if(toLower(translatedString)==toLower(results[i])){
					translatedFactionNames.push_back(results[i]);
				}
				else {
					translatedFactionNames.push_back(results[i]+" ("+translatedString+")");
				}
		    }

			for(int i=0; i<GameConstants::maxPlayers; ++i){
				listBoxFactions[i].setItems(results,translatedFactionNames);
			}

			if(lastMissingTechtree != gameSettings->getTech() &&
				gameSettings->getTech() != "") {
				lastMissingTechtree = gameSettings->getTech();

		    	Lang &lang= Lang::getInstance();
		    	const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
		    	for(unsigned int i = 0; i < languageList.size(); ++i) {

					char szMsg[8096]="";
					if(lang.hasString("DataMissingTechtree",languageList[i]) == true) {
						snprintf(szMsg,8096,lang.getString("DataMissingTechtree",languageList[i]).c_str(),getHumanPlayerName().c_str(),gameSettings->getTech().c_str());
					}
					else {
						snprintf(szMsg,8096,"Player: %s is missing the techtree: %s",getHumanPlayerName().c_str(),gameSettings->getTech().c_str());
					}
					clientInterface->sendTextMessage(szMsg,-1, lang.isLanguageLocal(languageList[i]),languageList[i]);
		    	}
			}

			foundFactions = false;
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		}
		else {
			lastMissingTechtree = "";
			getMissingTechtreeFromFTPServer = "";

			factionFiles= results;
		    vector<string> translatedFactionNames;
		    for(int i= 0; i < (int)factionFiles.size(); ++i) {
		    	results[i]= formatString(results[i]);
		    	string translatedString=techTree->getTranslatedFactionName(gameSettings->getTech(),factionFiles[i]);
		    	if(toLower(translatedString)==toLower(results[i])){
		    		translatedFactionNames.push_back(results[i]);
		    	}
		    	else {
		    		translatedFactionNames.push_back(results[i]+" ("+translatedString+")");
		    	}
		    }

			results.push_back(formatString(GameConstants::RANDOMFACTION_SLOTNAME));
			factionFiles.push_back(formatString(GameConstants::RANDOMFACTION_SLOTNAME));
			translatedFactionNames.push_back("*"+lang.getString("Random","",true)+"*");

			// Add special Observer Faction
			if(checkBoxAllowObservers.getValue() == 1) {
				results.push_back(formatString(GameConstants::OBSERVER_SLOTNAME));
				factionFiles.push_back(formatString(GameConstants::OBSERVER_SLOTNAME));
				translatedFactionNames.push_back("*"+lang.getString("Observer","",true)+"*");
			}


			for(int i=0; i<GameConstants::maxPlayers; ++i){
				listBoxFactions[i].setItems(results,translatedFactionNames);
			}

			foundFactions = (results.empty() == false);
		}
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	return foundFactions;
}

// ============ PRIVATE ===========================

bool MenuStateConnectedGame::hasNetworkGameSettings() {
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
		snprintf(szBuf,8096,"In [%s::%s %d]\nError detected:\n%s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] %s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,szBuf);
		showMessageBox( szBuf, "Error", false);
	}

    return hasNetworkSlot;
}

void MenuStateConnectedGame::keyDown(SDL_KeyboardEvent key) {
	if(activeInputLabel != NULL) {
		bool handled = keyDownEditLabel(key, &activeInputLabel);
		if(handled == true) {
			switchSetupRequestFlagType |= ssrft_NetworkPlayerName;
            needToSetChangedGameSettings = true;
            lastSetChangedGameSettings   = time(NULL);
		}
	}
	else {
		//send key to the chat manager
		NetworkManager &networkManager= NetworkManager::getInstance();
		ClientInterface *clientInterface = networkManager.getClientInterface();
		if(clientInterface != NULL &&
				clientInterface->isConnected() == true &&
				clientInterface->getIntroDone() == true) {
			chatManager.keyDown(key);
		}
		if(chatManager.getEditEnabled() == false &&
				(::Shared::Platform::Window::isKeyStateModPressed(KMOD_SHIFT) == false)) {
			Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));

			if(isKeyPressed(configKeys.getSDLKey("ShowFullConsole"),key) == true) {
				showFullConsole= true;
			}
			else if(isKeyPressed(configKeys.getSDLKey("ToggleMusic"),key) == true) {
				Config &config = Config::getInstance();
				Lang &lang= Lang::getInstance();

				float configVolume = (config.getInt("SoundVolumeMusic") / 100.f);
				float currentVolume = CoreData::getInstance().getMenuMusic()->getVolume();
				if(currentVolume > 0) {
					CoreData::getInstance().getMenuMusic()->setVolume(0.f);
					console.addLine(lang.getString("GameMusic") + " " + lang.getString("Off"));
				}
				else {
					CoreData::getInstance().getMenuMusic()->setVolume(configVolume);
					//If the config says zero, use the default music volume
					//gameMusic->setVolume(configVolume ? configVolume : 0.9);
					console.addLine(lang.getString("GameMusic"));
				}
			}
			//else if(key == configKeys.getCharKey("SaveGUILayout")) {
			else if(isKeyPressed(configKeys.getSDLKey("SaveGUILayout"),key) == true) {
				bool saved = GraphicComponent::saveAllCustomProperties(containerName);
				Lang &lang= Lang::getInstance();
				console.addLine(lang.getString("GUILayoutSaved") + " [" + (saved ? lang.getString("Yes") : lang.getString("No"))+ "]");
			}
		}
	}
}

void MenuStateConnectedGame::keyPress(SDL_KeyboardEvent c) {
	if(activeInputLabel != NULL) {
		bool handled = keyPressEditLabel(c, &activeInputLabel);
		if(handled == true) {
			switchSetupRequestFlagType |= ssrft_NetworkPlayerName;
            needToSetChangedGameSettings = true;
            lastSetChangedGameSettings   = time(NULL);
		}
	}
	else {
		NetworkManager &networkManager= NetworkManager::getInstance();
		ClientInterface *clientInterface = networkManager.getClientInterface();
		if(clientInterface != NULL &&
				clientInterface->isConnected() == true &&
				clientInterface->getIntroDone() == true) {
			chatManager.keyPress(c);
		}
	}
}

void MenuStateConnectedGame::keyUp(SDL_KeyboardEvent key) {
	if(activeInputLabel==NULL) {
		NetworkManager &networkManager= NetworkManager::getInstance();
		ClientInterface *clientInterface = networkManager.getClientInterface();
		if(clientInterface != NULL &&
				clientInterface->isConnected() == true &&
				clientInterface->getIntroDone() == true) {
			chatManager.keyUp(key);
		}

		Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));

		if(chatManager.getEditEnabled()) {
			//send key to the chat manager
			chatManager.keyUp(key);
		}
		else if(isKeyPressed(configKeys.getSDLKey("ShowFullConsole"),key) == true) {
			showFullConsole= false;
		}
	}
}

void MenuStateConnectedGame::setActiveInputLabel(GraphicLabel *newLable) {
	MenuState::setActiveInputLabel(newLable,&activeInputLabel);
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
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	if(enableFactionTexturePreview == true) {
		if(filepath == "") {
			factionTexture=NULL;
		}
		else {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] filepath = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,filepath.c_str());
			factionTexture = Renderer::findTexture(filepath);
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		}
	}
}

bool MenuStateConnectedGame::loadMapInfo(string file, MapInfo *mapInfo, bool loadMapPreview) {
	bool mapLoaded = false;
	try {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] map [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,file.c_str());

		if(file != "") {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

			lastMissingMap = file;

			Lang &lang= Lang::getInstance();
			if(MapPreview::loadMapInfo(file, mapInfo, lang.getString("MaxPlayers"),lang.getString("Size"),true) == true) {
				for(int i = 0; i < GameConstants::maxPlayers; ++i) {
					labelPlayers[i].setVisible(i+1 <= mapInfo->players);
					labelPlayerNames[i].setVisible(i+1 <= mapInfo->players);
					listBoxControls[i].setVisible(i+1 <= mapInfo->players);
					listBoxRMultiplier[i].setVisible(i+1 <= mapInfo->players);
					listBoxFactions[i].setVisible(i+1 <= mapInfo->players);
					listBoxTeams[i].setVisible(i+1 <= mapInfo->players);
					labelNetStatus[i].setVisible(i+1 <= mapInfo->players);
				}

				// Not painting properly so this is on hold
				if(loadMapPreview == true) {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
					if(mapPreview.getMapFileLoaded() != file) {
						mapPreview.loadFromFile(file.c_str());
						cleanupMapPreviewTexture();
					}
				}

				mapLoaded = true;
			}
		}
		else {
			cleanupMapPreviewTexture();
			mapInfo->desc = Lang::getInstance().getString("DataMissing","",true);

			NetworkManager &networkManager= NetworkManager::getInstance();
			ClientInterface* clientInterface= networkManager.getClientInterface();
			const GameSettings *gameSettings = clientInterface->getGameSettings();

			if(lastMissingMap != gameSettings->getMap()) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

				lastMissingMap = gameSettings->getMap();

				Lang &lang= Lang::getInstance();
				const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
				for(unsigned int i = 0; i < languageList.size(); ++i) {

					char szMsg[8096]="";
					if(lang.hasString("DataMissingMap",languageList[i]) == true) {
						snprintf(szMsg,8096,lang.getString("DataMissingMap",languageList[i]).c_str(),getHumanPlayerName().c_str(),gameSettings->getMap().c_str());
					}
					else {
						snprintf(szMsg,8096,"Player: %s is missing the map: %s",getHumanPlayerName().c_str(),gameSettings->getMap().c_str());
					}
					clientInterface->sendTextMessage(szMsg,-1, lang.isLanguageLocal(languageList[i]),languageList[i]);
				}
			}
		}
	}
	catch(exception &e){
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,e.what());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,e.what());

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

void MenuStateConnectedGame::cleanupMapPreviewTexture() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	if(mapPreviewTexture != NULL) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		mapPreviewTexture->end();

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		delete mapPreviewTexture;
		mapPreviewTexture = NULL;
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

bool MenuStateConnectedGame::isInSpecialKeyCaptureEvent() {
	bool result = (chatManager.getEditEnabled() || activeInputLabel != NULL);
	return result;
}

void MenuStateConnectedGame::FTPClient_CallbackEvent(string itemName,
		FTP_Client_CallbackType type, pair<FTP_Client_ResultType,string> result, void *userdata) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

    if(type == ftp_cct_DownloadProgress) {
        FTPClientCallbackInterface::FtpProgressStats *stats = (FTPClientCallbackInterface::FtpProgressStats *)userdata;
        if(stats != NULL) {
            int fileProgress = 0;
            if(stats->download_total > 0) {
                fileProgress = ((stats->download_now / stats->download_total) * 100.0);
            }
            //if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Got FTP Callback for [%s] current file [%s] fileProgress = %d [now = %f, total = %f]\n",itemName.c_str(),stats->currentFilename.c_str(), fileProgress,stats->download_now,stats->download_total);

            MutexSafeWrapper safeMutexFTPProgress((ftpClientThread != NULL ? ftpClientThread->getProgressMutex() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
            pair<int,string> lastProgress;
            std::map<string,pair<int,string> >::iterator iterFind = fileFTPProgressList.find(itemName);
            if(iterFind == fileFTPProgressList.end()) {
            	iterFind = fileFTPProgressList.find(GameConstants::saveNetworkGameFileServerCompressed);
                if(iterFind == fileFTPProgressList.end()) {
                	iterFind = fileFTPProgressList.find(GameConstants::saveNetworkGameFileClientCompressed);
                }
            }
            if(iterFind != fileFTPProgressList.end()) {
            	lastProgress = iterFind->second;
            	fileFTPProgressList[iterFind->first] = pair<int,string>(fileProgress,stats->currentFilename);
            }
            safeMutexFTPProgress.ReleaseLock();

            if(itemName != "" && (lastProgress.first / 25) < (fileProgress / 25)) {
		        NetworkManager &networkManager= NetworkManager::getInstance();
		        ClientInterface* clientInterface= networkManager.getClientInterface();

		    	Lang &lang= Lang::getInstance();
		    	const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
		    	for(unsigned int i = 0; i < languageList.size(); ++i) {
					char szMsg[8096]="";
		            if(lang.hasString("FileDownloadProgress",languageList[i]) == true) {
		            	snprintf(szMsg,8096,lang.getString("FileDownloadProgress",languageList[i]).c_str(),getHumanPlayerName().c_str(),itemName.c_str(),fileProgress);
		            }
		            else {
		            	snprintf(szMsg,8096,"Player: %s download progress for [%s] is %d %%",getHumanPlayerName().c_str(),itemName.c_str(),fileProgress);
		            }
		            if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] szMsg [%s] lastProgress.first = %d, fileProgress = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,szMsg,lastProgress.first,fileProgress);
		            clientInterface->sendTextMessage(szMsg,-1, lang.isLanguageLocal(languageList[i]),languageList[i]);
		    	}
		    	sleep(1);
            }
        }
    }
    else if(type == ftp_cct_ExtractProgress) {
    	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Got FTP extract Callback for [%s] result = %d [%s]\n",itemName.c_str(),result.first,result.second.c_str());

    	if(userdata == NULL) {
			NetworkManager &networkManager= NetworkManager::getInstance();
			ClientInterface* clientInterface= networkManager.getClientInterface();

			Lang &lang= Lang::getInstance();
			const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
			for(unsigned int i = 0; i < languageList.size(); ++i) {
				char szMsg[8096]="";
				if(lang.hasString("DataMissingExtractDownload",languageList[i]) == true) {
					snprintf(szMsg,8096,lang.getString("DataMissingExtractDownload",languageList[i]).c_str(),getHumanPlayerName().c_str(),itemName.c_str());
				}
				else {
					snprintf(szMsg,8096,"Please wait, player: %s is extracting: %s",getHumanPlayerName().c_str(),itemName.c_str());
				}
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] szMsg [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,szMsg);
				clientInterface->sendTextMessage(szMsg,-1, lang.isLanguageLocal(languageList[i]),languageList[i]);
			}
			sleep(1);
    	}
    	else {
			char *szBuf = (char *)userdata;
			//printf("%s\n",szBuf);
			//console.addLine(szBuf);
			console.addLine(szBuf, false,"");
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
    		string file = Config::getMapPath(itemName,"",false);

    		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Got map itemName [%s] file [%s] lastCheckedCRCMapName [%s] gameSettings->getMap() [%s]\n",
    				itemName.c_str(),file.c_str(),lastCheckedCRCMapName.c_str(),gameSettings->getMap().c_str());

			if(gameSettings != NULL && itemName == gameSettings->getMap() &&
					lastCheckedCRCMapName == gameSettings->getMap() &&
					gameSettings->getMap() != "") {
	            Checksum::clearFileCache();
	    		Checksum checksum;

				checksum.addFile(file);
				lastCheckedCRCMapValue = checksum.getSum();
			}

	    	Lang &lang= Lang::getInstance();
	    	const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
	    	for(unsigned int i = 0; i < languageList.size(); ++i) {
				char szMsg[8096]="";
	            if(lang.hasString("DataMissingMapSuccessDownload",languageList[i]) == true) {
	            	snprintf(szMsg,8096,lang.getString("DataMissingMapSuccessDownload",languageList[i]).c_str(),getHumanPlayerName().c_str(),itemName.c_str());
	            }
	            else {
	            	snprintf(szMsg,8096,"Player: %s SUCCESSFULLY downloaded the map: %s",getHumanPlayerName().c_str(),itemName.c_str());
	            }
	            clientInterface->sendTextMessage(szMsg,-1, lang.isLanguageLocal(languageList[i]),languageList[i]);
	    	}
	    	sleep(1);
        }
        else {
    		printf("FAILED map itemName [%s] lastCheckedCRCMapName [%s] gameSettings->getMap() [%s]\n",
    				itemName.c_str(),lastCheckedCRCMapName.c_str(),gameSettings->getMap().c_str());

            curl_version_info_data *curlVersion= curl_version_info(CURLVERSION_NOW);

	    	Lang &lang= Lang::getInstance();
	    	const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
	    	for(unsigned int i = 0; i < languageList.size(); ++i) {
				char szMsg[8096]="";
	            if(lang.hasString("DataMissingMapFailDownload",languageList[i]) == true) {
	            	snprintf(szMsg,8096,lang.getString("DataMissingMapFailDownload",languageList[i]).c_str(),getHumanPlayerName().c_str(),itemName.c_str(),curlVersion->version);
	            }
	            else {
	            	snprintf(szMsg,8096,"Player: %s FAILED to download the map: [%s] using CURL version [%s]",getHumanPlayerName().c_str(),itemName.c_str(),curlVersion->version);
	            }
	            clientInterface->sendTextMessage(szMsg,-1, lang.isLanguageLocal(languageList[i]),languageList[i]);

	            if(result.first == ftp_crt_HOST_NOT_ACCEPTING) {
		            if(lang.hasString("HostNotAcceptingDataConnections",languageList[i]) == true) {
		            	clientInterface->sendTextMessage(lang.getString("HostNotAcceptingDataConnections",languageList[i]),-1, lang.isLanguageLocal(languageList[i]),languageList[i]);
		            }
		            else {
		            	clientInterface->sendTextMessage("*Warning* the host is not accepting data connections.",-1, lang.isLanguageLocal(languageList[i]),languageList[i]);
		            }
	            }
	    	}
	    	sleep(1);

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
				char szMsg[8096]="";
	            if(lang.hasString("DataMissingTilesetSuccessDownload",languageList[i]) == true) {
	            	snprintf(szMsg,8096,lang.getString("DataMissingTilesetSuccessDownload",languageList[i]).c_str(),getHumanPlayerName().c_str(),itemName.c_str());
	            }
	            else {
	            	snprintf(szMsg,8096,"Player: %s SUCCESSFULLY downloaded the tileset: %s",getHumanPlayerName().c_str(),itemName.c_str());
	            }
	            clientInterface->sendTextMessage(szMsg,-1, lang.isLanguageLocal(languageList[i]),languageList[i]);
	    	}
	    	sleep(1);

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

    		//printf("Got map itemName [%s] file [%s] lastCheckedCRCMapName [%s] gameSettings->getMap() [%s]\n",
    		//		itemName.c_str(),file.c_str(),lastCheckedCRCMapName.c_str(),gameSettings->getMap().c_str());

			if(gameSettings != NULL && itemName == gameSettings->getTileset() &&
					lastCheckedCRCTilesetName == gameSettings->getTileset() &&
					gameSettings->getTileset() != "") {
				Config &config = Config::getInstance();
				lastCheckedCRCTilesetValue = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTilesets,""), string("/") + itemName + string("/*"), ".xml", NULL, true);
			}

            safeMutexFTPProgress.ReleaseLock();
            // END

            // Reload tilesets for the UI
            string scenarioDir = Scenario::getScenarioDir(dirList, gameSettings->getScenario());
            findDirs(Config::getInstance().getPathListForType(ptTilesets,scenarioDir), tilesetFiles);

            std::vector<string> tilesetsFormatted = tilesetFiles;
        	std::for_each(tilesetsFormatted.begin(), tilesetsFormatted.end(), FormatString());
        	listBoxTileset.setItems(tilesetsFormatted);
        }
        else {
            curl_version_info_data *curlVersion= curl_version_info(CURLVERSION_NOW);

	    	Lang &lang= Lang::getInstance();
	    	const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
	    	for(unsigned int i = 0; i < languageList.size(); ++i) {
				char szMsg[8096]="";
	            if(lang.hasString("DataMissingTilesetFailDownload",languageList[i]) == true) {
	            	snprintf(szMsg,8096,lang.getString("DataMissingTilesetFailDownload",languageList[i]).c_str(),getHumanPlayerName().c_str(),(itemName+"(.7z)").c_str(),curlVersion->version);
	            }
	            else {
	            	snprintf(szMsg,8096,"Player: %s FAILED to download the tileset: [%s] using CURL version [%s]",getHumanPlayerName().c_str(),(itemName+"(.7z)").c_str(),curlVersion->version);
	            }
	            clientInterface->sendTextMessage(szMsg,-1, lang.isLanguageLocal(languageList[i]),languageList[i]);

	            if(result.first == ftp_crt_HOST_NOT_ACCEPTING) {
		            if(lang.hasString("HostNotAcceptingDataConnections",languageList[i]) == true) {
		            	clientInterface->sendTextMessage(lang.getString("HostNotAcceptingDataConnections",languageList[i]),-1, lang.isLanguageLocal(languageList[i]),languageList[i]);
		            }
		            else {
		            	clientInterface->sendTextMessage("*Warning* the host is not accepting data connections.",-1, lang.isLanguageLocal(languageList[i]),languageList[i]);
		            }
	            }
	    	}
	    	sleep(1);

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
				char szMsg[8096]="";
	            if(lang.hasString("DataMissingTechtreeSuccessDownload",languageList[i]) == true) {
	            	snprintf(szMsg,8096,lang.getString("DataMissingTechtreeSuccessDownload",languageList[i]).c_str(),getHumanPlayerName().c_str(),itemName.c_str());
	            }
	            else {
	            	snprintf(szMsg,8096,"Player: %s SUCCESSFULLY downloaded the techtree: %s",getHumanPlayerName().c_str(),itemName.c_str());
	            }
	            clientInterface->sendTextMessage(szMsg,-1, lang.isLanguageLocal(languageList[i]),languageList[i]);
	    	}
	    	sleep(1);

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
			if(gameSettings != NULL && itemName == gameSettings->getTech() &&
					lastCheckedCRCTechtreeName == gameSettings->getTech() &&
					gameSettings->getTech() != "") {
				Config &config = Config::getInstance();
				lastCheckedCRCTechtreeValue = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTechs,""), string("/") + itemName + string("/*"), ".xml", NULL, true);
			}
            safeMutexFTPProgress.ReleaseLock();
            // END

            // Reload techs for the UI
            string scenarioDir = Scenario::getScenarioDir(dirList, gameSettings->getScenario());
            findDirs(Config::getInstance().getPathListForType(ptTechs,scenarioDir), techTreeFiles);

            vector<string> translatedTechs;
            std::vector<string> techsFormatted = techTreeFiles;
        	for(int i= 0; i < (int)techsFormatted.size(); i++){
        		techsFormatted.at(i)= formatString(techsFormatted.at(i));

    			string txTech = techTree->getTranslatedName(techTreeFiles.at(i), true);
    			translatedTechs.push_back(formatString(txTech));
        	}
            listBoxTechTree.setItems(techsFormatted,translatedTechs);
        }
        else {
            curl_version_info_data *curlVersion= curl_version_info(CURLVERSION_NOW);

	    	Lang &lang= Lang::getInstance();
	    	const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
	    	for(unsigned int i = 0; i < (unsigned int)languageList.size(); ++i) {
				char szMsg[8096]="";
	            if(lang.hasString("DataMissingTechtreeFailDownload",languageList[i]) == true) {
	            	snprintf(szMsg,8096,lang.getString("DataMissingTechtreeFailDownload",languageList[i]).c_str(),getHumanPlayerName().c_str(),(itemName+"(.7z)").c_str(),curlVersion->version);
	            }
	            else {
	            	snprintf(szMsg,8096,"Player: %s FAILED to download the techtree: [%s] using CURL version [%s]",getHumanPlayerName().c_str(),(itemName+"(.7z)").c_str(),curlVersion->version);
	            }
	            clientInterface->sendTextMessage(szMsg,-1, lang.isLanguageLocal(languageList[i]),languageList[i]);

	            if(result.first == ftp_crt_HOST_NOT_ACCEPTING) {
		            if(lang.hasString("HostNotAcceptingDataConnections",languageList[i]) == true) {
		            	clientInterface->sendTextMessage(lang.getString("HostNotAcceptingDataConnections",languageList[i]),-1, lang.isLanguageLocal(languageList[i]),languageList[i]);
		            }
		            else {
		            	clientInterface->sendTextMessage("*Warning* the host is not accepting data connections.",-1, lang.isLanguageLocal(languageList[i]),languageList[i]);
		            }
	            }
	    	}
	    	sleep(1);

            console.addLine(result.second,true);
        }
    }
    else if(type == ftp_cct_TempFile) {
    	getInProgressSavedGameFromFTPServerInProgress = false;
        if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Got FTP Callback for [%s] result = %d [%s]\n",itemName.c_str(),result.first,result.second.c_str());

        MutexSafeWrapper safeMutexFTPProgress((ftpClientThread != NULL ? ftpClientThread->getProgressMutex() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
        //fileFTPProgressList.erase(itemName);
        std::map<string,pair<int,string> >::iterator iterFind = fileFTPProgressList.find(itemName);
        if(iterFind == fileFTPProgressList.end()) {
        	iterFind = fileFTPProgressList.find(GameConstants::saveNetworkGameFileServerCompressed);
            if(iterFind == fileFTPProgressList.end()) {
            	iterFind = fileFTPProgressList.find(GameConstants::saveNetworkGameFileClientCompressed);
            }
        }
        if(iterFind != fileFTPProgressList.end()) {
        	fileFTPProgressList.erase(iterFind->first);
        }
        safeMutexFTPProgress.ReleaseLock();

        //printf("Status update downloading saved game file: [%s]\n",itemName.c_str());

        NetworkManager &networkManager= NetworkManager::getInstance();
        ClientInterface* clientInterface= networkManager.getClientInterface();
        //const GameSettings *gameSettings = clientInterface->getGameSettings();

        if(result.first == ftp_crt_SUCCESS) {
	    	Lang &lang= Lang::getInstance();
	    	const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
	    	for(unsigned int i = 0; i < languageList.size(); ++i) {
				char szMsg[8096]="";
	            if(lang.hasString("JoinPlayerToCurrentGameSuccessDownload",languageList[i]) == true) {
	            	snprintf(szMsg,8096,lang.getString("JoinPlayerToCurrentGameSuccessDownload",languageList[i]).c_str(),getHumanPlayerName().c_str(),itemName.c_str());
	            }
	            else {
	            	snprintf(szMsg,8096,"Player: %s SUCCESSFULLY downloaded the saved game: %s",getHumanPlayerName().c_str(),itemName.c_str());
	            }
	            clientInterface->sendTextMessage(szMsg,-1, lang.isLanguageLocal(languageList[i]),languageList[i]);
	    	}
	    	sleep(1);

	    	if(itemName == GameConstants::saveNetworkGameFileClientCompressed) {
				string saveGameFilePath = "temp/";
				string saveGameFile = saveGameFilePath + string(GameConstants::saveNetworkGameFileClientCompressed);
				if(getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) != "") {
					saveGameFilePath = getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) + saveGameFilePath;
					saveGameFile = saveGameFilePath + string(GameConstants::saveNetworkGameFileClientCompressed);
				}
				else {
					string userData = Config::getInstance().getString("UserData_Root","");
					if(userData != "") {
						endPathWithSlash(userData);
					}
					saveGameFilePath = userData + saveGameFilePath;
					saveGameFile = saveGameFilePath + string(GameConstants::saveNetworkGameFileClientCompressed);
				}

				string extractedFileName = saveGameFilePath + string(GameConstants::saveNetworkGameFileClient);
				bool extract_result = extractFileFromZIPFile(saveGameFile,extractedFileName);

				if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Saved game [%s] compressed to [%s] returned: %d\n",saveGameFile.c_str(),extractedFileName.c_str(), extract_result);
	    	}
	    	readyToJoinInProgressGame = true;

	    	//printf("Success downloading saved game file: [%s]\n",itemName.c_str());
        }
        else {
            curl_version_info_data *curlVersion= curl_version_info(CURLVERSION_NOW);

	    	Lang &lang= Lang::getInstance();
	    	const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
	    	for(unsigned int i = 0; i < languageList.size(); ++i) {
				char szMsg[8096]="";
	            if(lang.hasString("JoinPlayerToCurrentGameFailDownload",languageList[i]) == true) {
	            	snprintf(szMsg,8096,lang.getString("JoinPlayerToCurrentGameFailDownload",languageList[i]).c_str(),getHumanPlayerName().c_str(),itemName.c_str(),curlVersion->version);
	            }
	            else {
	            	snprintf(szMsg,8096,"Player: %s FAILED to download the saved game: [%s] using CURL version [%s]",getHumanPlayerName().c_str(),itemName.c_str(),curlVersion->version);
	            }
	            clientInterface->sendTextMessage(szMsg,-1, lang.isLanguageLocal(languageList[i]),languageList[i]);

	            if(result.first == ftp_crt_HOST_NOT_ACCEPTING) {
		            if(lang.hasString("HostNotAcceptingDataConnections",languageList[i]) == true) {
		            	clientInterface->sendTextMessage(lang.getString("HostNotAcceptingDataConnections",languageList[i]),-1, lang.isLanguageLocal(languageList[i]),languageList[i]);
		            }
		            else {
		            	clientInterface->sendTextMessage("*Warning* the host is not accepting data connections.",-1, lang.isLanguageLocal(languageList[i]),languageList[i]);
		            }
	            }
	    	}
	    	sleep(1);

            console.addLine(result.second,true);
        }
    }
}

void MenuStateConnectedGame::setupUIFromGameSettings(GameSettings *gameSettings, bool errorOnMissingData) {
	NetworkManager &networkManager= NetworkManager::getInstance();
	ClientInterface *clientInterface = networkManager.getClientInterface();

	updateDataSynchDetailText = true;
	vector<string> maps,tilesets,techtree;

	if(gameSettings == NULL) {
		throw megaglest_runtime_error("gameSettings == NULL");
	}

	checkBoxScenario.setValue((gameSettings->getScenario() != ""));
	if(checkBoxScenario.getValue() == true) {
		int originalFOWValue = listBoxFogOfWar.getSelectedItemIndex();

		string scenario = gameSettings->getScenario();
		listBoxScenario.setSelectedItem(formatString(scenario));
		string file = Scenario::getScenarioPath(dirList, scenario);

		bool isTutorial = Scenario::isGameTutorial(file);
		Scenario::loadScenarioInfo(file, &scenarioInfo, isTutorial);

		gameSettings->setScenarioDir(Scenario::getScenarioPath(dirList, scenarioInfo.name));

		gameSettings->setDefaultResources(scenarioInfo.defaultResources);
		gameSettings->setDefaultUnits(scenarioInfo.defaultUnits);
		gameSettings->setDefaultVictoryConditions(scenarioInfo.defaultVictoryConditions);

		if(scenarioInfo.fogOfWar == false && scenarioInfo.fogOfWar_exploredFlag == false) {
			listBoxFogOfWar.setSelectedItemIndex(2);
		}
		else if(scenarioInfo.fogOfWar_exploredFlag == true) {
			listBoxFogOfWar.setSelectedItemIndex(1);
		}
		else {
			listBoxFogOfWar.setSelectedItemIndex(0);
		}

		if(originalFOWValue != listBoxFogOfWar.getSelectedItemIndex()) {
			cleanupMapPreviewTexture();
		}
	}

	string scenarioDir = Scenario::getScenarioDir(dirList, gameSettings->getScenario());
	setupMapList(gameSettings->getScenario());
	setupTechList(gameSettings->getScenario());
	setupTilesetList(gameSettings->getScenario());

	//listBoxMap.setItems(formattedPlayerSortedMaps[gameSettings.getMapFilterIndex()]);
	//printf("A gameSettings->getTileset() [%s]\n",gameSettings->getTileset().c_str());

	if(getMissingTilesetFromFTPServerInProgress == false &&
			gameSettings->getTileset() != "") {
		// tileset
		tilesets = tilesetFiles;
		std::for_each(tilesets.begin(), tilesets.end(), FormatString());

		if(std::find(tilesetFiles.begin(),tilesetFiles.end(),gameSettings->getTileset()) != tilesetFiles.end()) {
			lastMissingTileSet = "";
			getMissingTilesetFromFTPServer = "";
			listBoxTileset.setSelectedItem(formatString(gameSettings->getTileset()));
		}
		else {
			// try to get the tileset via ftp
			if(ftpClientThread != NULL &&
					(getMissingTilesetFromFTPServer != gameSettings->getTileset() ||
						difftime(time(NULL),getMissingTilesetFromFTPServerLastPrompted) > REPROMPT_DOWNLOAD_SECONDS)) {
				if(ftpMessageBox.getEnabled() == false) {
					getMissingTilesetFromFTPServerLastPrompted = time(NULL);
					getMissingTilesetFromFTPServer = gameSettings->getTileset();
					Lang &lang= Lang::getInstance();

					char szBuf[8096]="";
					snprintf(szBuf,8096,"%s %s ?",lang.getString("DownloadMissingTilesetQuestion").c_str(),gameSettings->getTileset().c_str());

					// Is the item in the mod center?
					MutexSafeWrapper safeMutexThread((modHttpServerThread != NULL ? modHttpServerThread->getMutexThreadObjectAccessor() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
					if(tilesetCacheList.find(getMissingMapFromFTPServer) == tilesetCacheList.end()) {
						ftpMessageBox.init(lang.getString("Yes"),lang.getString("NoDownload"));
					}
					else {
						ftpMessageBox.init(lang.getString("ModCenter"),lang.getString("GameHost"));
						ftpMessageBox.addButton(lang.getString("NoDownload"));
					}
					safeMutexThread.ReleaseLock();

					ftpMissingDataType = ftpmsg_MissingTileset;
					showFTPMessageBox(szBuf, lang.getString("Question"), false);
				}
			}

			tilesets.push_back(Lang::getInstance().getString("DataMissing","",true));

			NetworkManager &networkManager= NetworkManager::getInstance();
			ClientInterface* clientInterface= networkManager.getClientInterface();
			const GameSettings *gameSettings = clientInterface->getGameSettings();

			if(lastMissingTileSet != gameSettings->getTileset()) {
				lastMissingTileSet = gameSettings->getTileset();

				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

				Lang &lang= Lang::getInstance();
				const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
				for(unsigned int i = 0; i < languageList.size(); ++i) {

					char szMsg[8096]="";
					if(lang.hasString("DataMissingTileset",languageList[i]) == true) {
						snprintf(szMsg,8096,lang.getString("DataMissingTileset",languageList[i]).c_str(),getHumanPlayerName().c_str(),gameSettings->getTileset().c_str());
					}
					else {
						snprintf(szMsg,8096,"Player: %s is missing the tileset: %s",getHumanPlayerName().c_str(),gameSettings->getTileset().c_str());
					}
					clientInterface->sendTextMessage(szMsg,-1, lang.isLanguageLocal(languageList[i]),languageList[i]);
				}
			}

			listBoxTileset.setItems(tilesets);
			listBoxTileset.setSelectedItem(Lang::getInstance().getString("DataMissing","",true));
		}

	}

	if(getMissingTechtreeFromFTPServerInProgress == false &&
			gameSettings->getTech() != "") {
		// techtree
		techtree = techTreeFiles;
		std::for_each(techtree.begin(), techtree.end(), FormatString());

		if(std::find(techTreeFiles.begin(),techTreeFiles.end(),gameSettings->getTech()) != techTreeFiles.end()) {

			lastMissingTechtree = "";
			getMissingTechtreeFromFTPServer = "";
			reloadFactions(true,gameSettings->getScenario());
			listBoxTechTree.setSelectedItem(formatString(gameSettings->getTech()));
		}
		else {
			// try to get the tileset via ftp
			if(ftpClientThread != NULL && (getMissingTechtreeFromFTPServer != gameSettings->getTech() ||
					difftime(time(NULL),getMissingTechtreeFromFTPServerLastPrompted) > REPROMPT_DOWNLOAD_SECONDS)) {
				if(ftpMessageBox.getEnabled() == false) {
					getMissingTechtreeFromFTPServerLastPrompted = time(NULL);
					getMissingTechtreeFromFTPServer = gameSettings->getTech();
					Lang &lang= Lang::getInstance();

					char szBuf[8096]="";
					snprintf(szBuf,8096,"%s %s ?",lang.getString("DownloadMissingTechtreeQuestion").c_str(),gameSettings->getTech().c_str());

					// Is the item in the mod center?
					MutexSafeWrapper safeMutexThread((modHttpServerThread != NULL ? modHttpServerThread->getMutexThreadObjectAccessor() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
					if(techCacheList.find(getMissingTechtreeFromFTPServer) == techCacheList.end()) {
						ftpMessageBox.init(lang.getString("Yes"),lang.getString("NoDownload"));
					}
					else {
						ftpMessageBox.init(lang.getString("ModCenter"),lang.getString("GameHost"));
						ftpMessageBox.addButton(lang.getString("NoDownload"));
					}
					safeMutexThread.ReleaseLock();

					ftpMissingDataType = ftpmsg_MissingTechtree;
					showFTPMessageBox(szBuf, lang.getString("Question"), false);
				}
			}

			techtree.push_back(Lang::getInstance().getString("DataMissing","",true));

			NetworkManager &networkManager= NetworkManager::getInstance();
			ClientInterface* clientInterface= networkManager.getClientInterface();
			const GameSettings *gameSettings = clientInterface->getGameSettings();

			if(lastMissingTechtree != gameSettings->getTech()) {
				lastMissingTechtree = gameSettings->getTech();

				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

				Lang &lang= Lang::getInstance();
				const vector<string> languageList = clientInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
				for(unsigned int i = 0; i < languageList.size(); ++i) {

					char szMsg[8096]="";
					if(lang.hasString("DataMissingTechtree",languageList[i]) == true) {
						snprintf(szMsg,8096,lang.getString("DataMissingTechtree",languageList[i]).c_str(),getHumanPlayerName().c_str(),gameSettings->getTech().c_str());
					}
					else {
						snprintf(szMsg,8096,"Player: %s is missing the techtree: %s",getHumanPlayerName().c_str(),gameSettings->getTech().c_str());
					}
					clientInterface->sendTextMessage(szMsg,-1, lang.isLanguageLocal(languageList[i]),languageList[i]);
				}
			}

			vector<string> translatedTechs;
			for(unsigned int i= 0; i < techTreeFiles.size(); i++) {
				string txTech = techTree->getTranslatedName(techTreeFiles.at(i));
				translatedTechs.push_back(txTech);
			}
			listBoxTechTree.setItems(techtree,translatedTechs);
			listBoxTechTree.setSelectedItem(Lang::getInstance().getString("DataMissing","",true));
		}
	}

	// factions
	bool hasFactions = true;
	if(currentFactionName != gameSettings->getTech() && gameSettings->getTech() != "") {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] hasFactions = %d, currentFactionName [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,hasFactions,currentFactionName.c_str());
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
		string mapFile = gameSettings->getMap();
		mapFile = formatString(mapFile);

		maps = formattedMapFiles;

		if(currentMap != gameSettings->getMap()) {// load the setup again
			currentMap = gameSettings->getMap();
		}
		bool mapLoaded = loadMapInfo(Config::getMapPath(currentMap,scenarioDir,false), &mapInfo, true);
		if(mapLoaded == true) {
			if(find(maps.begin(),maps.end(),formatString(gameSettings->getMap())) == maps.end()) {
				maps.push_back(formatString(gameSettings->getMap()));
			}
		}
		else {
			// try to get the map via ftp
			if(ftpClientThread != NULL && (getMissingMapFromFTPServer != currentMap ||
					difftime(time(NULL),getMissingMapFromFTPServerLastPrompted) > REPROMPT_DOWNLOAD_SECONDS)) {
				if(ftpMessageBox.getEnabled() == false) {
					getMissingMapFromFTPServerLastPrompted = time(NULL);
					getMissingMapFromFTPServer = currentMap;
					Lang &lang= Lang::getInstance();

					char szBuf[8096]="";
					snprintf(szBuf,8096,"%s %s ?",lang.getString("DownloadMissingMapQuestion").c_str(),currentMap.c_str());

					// Is the item in the mod center?
					MutexSafeWrapper safeMutexThread((modHttpServerThread != NULL ? modHttpServerThread->getMutexThreadObjectAccessor() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
					if(mapCacheList.find(getMissingTechtreeFromFTPServer) == mapCacheList.end()) {
						ftpMessageBox.init(lang.getString("Yes"),lang.getString("NoDownload"));
					}
					else {
						ftpMessageBox.init(lang.getString("ModCenter"),lang.getString("GameHost"));
						ftpMessageBox.addButton(lang.getString("NoDownload"));
					}
					safeMutexThread.ReleaseLock();

					ftpMissingDataType = ftpmsg_MissingMap;
					showFTPMessageBox(szBuf, lang.getString("Question"), false);
				}
			}
			maps.push_back(Lang::getInstance().getString("DataMissing","",true));
			mapFile = Lang::getInstance().getString("DataMissing","",true);
		}

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d] listBoxMap.getSelectedItemIndex() = %d, mapFiles.size() = " MG_SIZE_T_SPECIFIER ", maps.size() = " MG_SIZE_T_SPECIFIER ", getCurrentMapFile() [%s] mapFile [%s]\n",
				extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,listBoxMap.getSelectedItemIndex(),mapFiles.size(),maps.size(),getCurrentMapFile().c_str(),mapFile.c_str());

		listBoxMap.setItems(maps);

		listBoxMap.setSelectedItem(mapFile);
		labelMapInfo.setText(mapInfo.desc);
	}

	// FogOfWar
	if(checkBoxScenario.getValue() == false) {
		int originalFOWValue = listBoxFogOfWar.getSelectedItemIndex();
		listBoxFogOfWar.setSelectedItemIndex(0); // default is 0!
		if(gameSettings->getFogOfWar() == false){
			listBoxFogOfWar.setSelectedItemIndex(2);
		}
		if((gameSettings->getFlagTypes1() & ft1_show_map_resources) == ft1_show_map_resources){
			if(gameSettings->getFogOfWar() == true){
				listBoxFogOfWar.setSelectedItemIndex(1);
			}
		}
		if(originalFOWValue != listBoxFogOfWar.getSelectedItemIndex()) {
			cleanupMapPreviewTexture();
		}
	}

	// Allow Observers
	if(gameSettings->getAllowObservers()) {
		checkBoxAllowObservers.setValue(true);
	}
	else
	{
		checkBoxAllowObservers.setValue(false);
	}

	if((gameSettings->getFlagTypes1() & ft1_allow_team_switching) == ft1_allow_team_switching){
		checkBoxEnableSwitchTeamMode.setValue(true);
	}
	else {
		checkBoxEnableSwitchTeamMode.setValue(false);
	}
	listBoxAISwitchTeamAcceptPercent.setSelectedItem(intToStr(gameSettings->getAiAcceptSwitchTeamPercentChance()));
	listBoxFallbackCpuMultiplier.setSelectedItemIndex(gameSettings->getFallbackCpuMultiplier());

	if((gameSettings->getFlagTypes1() & ft1_allow_shared_team_units) == ft1_allow_shared_team_units) {
		checkBoxAllowTeamUnitSharing.setValue(true);
	}
	else {
		checkBoxAllowTeamUnitSharing.setValue(false);
	}

	if((gameSettings->getFlagTypes1() & ft1_allow_shared_team_resources) == ft1_allow_shared_team_resources) {
		checkBoxAllowTeamResourceSharing.setValue(true);
	}
	else {
		checkBoxAllowTeamResourceSharing.setValue(false);
	}

	// Control
	for(int i=0; i<GameConstants::maxPlayers; ++i) {
		listBoxControls[i].setSelectedItemIndex(ctClosed);

		if(isHeadlessAdmin() == false) {
			if(clientInterface->getJoinGameInProgress() == false) {
				listBoxFactions[i].setEditable(false);
				listBoxTeams[i].setEditable(false);
			}
		}

		labelPlayerStatus[i].setTexture(NULL);;
	}

	if(hasFactions == true && gameSettings != NULL) {
		NetworkManager &networkManager= NetworkManager::getInstance();
		ClientInterface *clientInterface = networkManager.getClientInterface();

		//for(int i=0; i < gameSettings->getFactionCount(); ++i){
		for(int i=0; i < GameConstants::maxPlayers; ++i) {
			int slot = gameSettings->getStartLocationIndex(i);

			if(slot == clientInterface->getPlayerIndex()){
				labelPlayerNames[slot].setEditable(true);
			}
			else {
				labelPlayerNames[slot].setEditable(false);
			}

			if(i >= gameSettings->getFactionCount()) {
				if( gameSettings->getFactionControl(i) != ctNetworkUnassigned) {
					continue;
				}
				else if(clientInterface->getPlayerIndex() != slot) {
					continue;
				}
			}

			if(	gameSettings->getFactionControl(i) == ctNetwork ||
				gameSettings->getFactionControl(i) == ctNetworkUnassigned ||
				gameSettings->getFactionControl(i) == ctHuman) {
				switch(gameSettings->getNetworkPlayerStatuses(i)) {
					case npst_BeRightBack:
						labelPlayerStatus[slot].setTexture(CoreData::getInstance().getStatusBRBTexture());
						break;
					case npst_Ready:
						labelPlayerStatus[slot].setTexture(CoreData::getInstance().getStatusReadyTexture());
						break;
					case npst_PickSettings:
						labelPlayerStatus[slot].setTexture(CoreData::getInstance().getStatusNotReadyTexture());
						break;
					case npst_Disconnected:
						labelPlayerStatus[slot].setTexture(NULL);
						break;

					default:
						labelPlayerStatus[slot].setTexture(NULL);
						break;
				}
			}

			listBoxControls[slot].setSelectedItemIndex(gameSettings->getFactionControl(i),errorOnMissingData);
			listBoxRMultiplier[slot].setSelectedItemIndex(gameSettings->getResourceMultiplierIndex(i),errorOnMissingData);

			listBoxTeams[slot].setSelectedItemIndex(gameSettings->getTeam(i),errorOnMissingData);
			listBoxFactions[slot].setSelectedItem(formatString(gameSettings->getFactionTypeName(i)),false);
			if( gameSettings->getFactionControl(i) == ctNetwork ||
				gameSettings->getFactionControl(i) == ctNetworkUnassigned) {
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
			}
			else {
				listBoxRMultiplier[slot].setEnabled(true);
				listBoxRMultiplier[slot].setVisible(true);
			}

			if((gameSettings->getFactionControl(i) == ctNetwork ||
				gameSettings->getFactionControl(i) == ctNetworkUnassigned) &&
				gameSettings->getThisFactionIndex() == i) {

				// set my current slot to ctHuman
				if(gameSettings->getFactionControl(i) != ctNetworkUnassigned) {
					listBoxControls[slot].setSelectedItemIndex(ctHuman);
				}
				if(checkBoxScenario.getValue() == false) {
					if(clientInterface->getJoinGameInProgress() == false) {
						listBoxFactions[slot].setEditable(true);
						listBoxTeams[slot].setEditable(true);
					}
				}

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
		}
	}

	if(enableFactionTexturePreview == true) {
		if( clientInterface != NULL && clientInterface->isConnected() &&
			gameSettings != NULL) {

			if( currentTechName_factionPreview != gameSettings->getTech() ||
				currentFactionName_factionPreview != gameSettings->getFactionTypeName(gameSettings->getThisFactionIndex())) {

				currentTechName_factionPreview=gameSettings->getTech();
				currentFactionName_factionPreview=gameSettings->getFactionTypeName(gameSettings->getThisFactionIndex());

				initFactionPreview(gameSettings);
			}
		}
	}

	checkBoxAllowNativeLanguageTechtree.setValue(gameSettings->getNetworkAllowNativeLanguageTechtree());
}

void MenuStateConnectedGame::initFactionPreview(const GameSettings *gameSettings) {
	string factionVideoUrl = "";
	string factionVideoUrlFallback = "";

	string factionDefinitionXML = Game::findFactionLogoFile(gameSettings, NULL,currentFactionName_factionPreview + ".xml");
	if(factionDefinitionXML != "" && currentFactionName_factionPreview != GameConstants::RANDOMFACTION_SLOTNAME &&
			currentFactionName_factionPreview != GameConstants::OBSERVER_SLOTNAME && fileExists(factionDefinitionXML) == true) {
		XmlTree	xmlTree;
		std::map<string,string> mapExtraTagReplacementValues;
		xmlTree.load(factionDefinitionXML, Properties::getTagReplacementValues(&mapExtraTagReplacementValues));
		const XmlNode *factionNode= xmlTree.getRootNode();
		if(factionNode->hasAttribute("faction-preview-video") == true) {
			factionVideoUrl = factionNode->getAttribute("faction-preview-video")->getValue();
		}

		factionVideoUrlFallback = Game::findFactionLogoFile(gameSettings, NULL,"preview_video.*");
		if(factionVideoUrl == "") {
			factionVideoUrl = factionVideoUrlFallback;
			factionVideoUrlFallback = "";
		}
	}

	if(factionVideoUrl != "") {
		if(CoreData::getInstance().getMenuMusic()->getVolume() != 0) {
			CoreData::getInstance().getMenuMusic()->setVolume(0);
			factionVideoSwitchedOffVolume=true;
		}

		if(currentFactionLogo != factionVideoUrl) {
			currentFactionLogo = factionVideoUrl;
			if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false &&
				::Shared::Graphics::VideoPlayer::hasBackEndVideoPlayer() == true) {

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
						true,
						vlcPluginsPath,
						SystemFlags::VERBOSE_MODE_ENABLED);
				factionVideo->initPlayer();
			}
		}
	}
	else {
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
		string factionLogo = Game::findFactionLogoFile(gameSettings, NULL,GameConstants::PREVIEW_SCREEN_FILE_FILTER);
		if(factionLogo == "") {
			factionLogo = Game::findFactionLogoFile(gameSettings, NULL);
		}
		if(currentFactionLogo != factionLogo) {
			currentFactionLogo = factionLogo;
			loadFactionTexture(currentFactionLogo);
		}
	}
}

void MenuStateConnectedGame::RestoreLastGameSettings() {
	// Ensure we have set the gamesettings at least once
	NetworkManager &networkManager= NetworkManager::getInstance();
	ClientInterface* clientInterface= networkManager.getClientInterface();
	GameSettings gameSettings = *clientInterface->getGameSettings();
	CoreData::getInstance().loadGameSettingsFromFile(HEADLESS_SAVED_GAME_FILENAME,&gameSettings);
	if(gameSettings.getMap() == "") {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		loadGameSettings(&gameSettings);
	}

	setupUIFromGameSettings(&gameSettings, false);

	needToBroadcastServerSettings=true;
	broadcastServerSettingsDelayTimer=time(NULL);

}

int MenuStateConnectedGame::setupMapList(string scenario) {
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

		formattedMapFiles.clear();
		for(int i= 0; i < (int)mapFiles.size(); i++){// fetch info and put map in right list
			loadMapInfo(Config::getMapPath(mapFiles.at(i), scenarioDir, false), &mapInfo, false);

			if(GameConstants::maxPlayers+1 <= mapInfo.players) {
				char szBuf[8096]="";
				snprintf(szBuf,8096,"Sorted map list [%d] does not match\ncurrent map playercount [%d]\nfor file [%s]\nmap [%s]",GameConstants::maxPlayers+1,mapInfo.players,Config::getMapPath(mapFiles.at(i), "", false).c_str(),mapInfo.desc.c_str());
				throw megaglest_runtime_error(szBuf);
			}
			playerSortedMaps[mapInfo.players].push_back(mapFiles.at(i));
			formattedPlayerSortedMaps[mapInfo.players].push_back(formatString(mapFiles.at(i)));
			if(config.getString("InitialMap", "Conflict") == formattedPlayerSortedMaps[mapInfo.players].back()){
				initialMapSelection= i;
			}
			formattedMapFiles.push_back(formatString(mapFiles.at(i)));
		}

		if(scenario != "") {
			string file = Scenario::getScenarioPath(dirList, scenario);
			loadScenarioInfo(file, &scenarioInfo);

			loadMapInfo(Config::getMapPath(scenarioInfo.mapName, scenarioDir, true), &mapInfo, false);

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d] listBoxMap.getSelectedItemIndex() = %d, mapFiles.size() = " MG_SIZE_T_SPECIFIER ", mapInfo.players = %d, formattedPlayerSortedMaps[mapInfo.players].size() = " MG_SIZE_T_SPECIFIER ", scenarioInfo.mapName [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,listBoxMap.getSelectedItemIndex(),mapFiles.size(),mapInfo.players,formattedPlayerSortedMaps[mapInfo.players].size(),scenarioInfo.mapName.c_str());
			listBoxMap.setItems(formattedPlayerSortedMaps[mapInfo.players]);
		}
	}
	catch(const std::exception &ex) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s %d]\nError detected:\n%s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);

		throw megaglest_runtime_error(szBuf);
		//abort();
	}

	return initialMapSelection;
}

int MenuStateConnectedGame::setupTechList(string scenario, bool forceLoad) {
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

		vector<string> translatedTechs;

		for(unsigned int i= 0; i < results.size(); i++) {
			//printf("TECHS i = %d results [%s] scenario [%s]\n",i,results[i].c_str(),scenario.c_str());

			results.at(i)= formatString(results.at(i));
			if(config.getString("InitialTechTree", "Megapack") == results.at(i)) {
				initialTechSelection= i;
			}
			string txTech = techTree->getTranslatedName(techTreeFiles.at(i), forceLoad);
			translatedTechs.push_back(formatString(txTech));
		}


		listBoxTechTree.setItems(results,translatedTechs);
	}
	catch(const std::exception &ex) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s %d]\nError detected:\n%s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);

		throw megaglest_runtime_error(szBuf);
	}

	return initialTechSelection;
}

void MenuStateConnectedGame::setupTilesetList(string scenario) {
	try {
		Config &config = Config::getInstance();

		string scenarioDir = Scenario::getScenarioDir(dirList, scenario);

		vector<string> results;
		findDirs(config.getPathListForType(ptTilesets,scenarioDir), results);
		if (results.empty()) {
			//throw megaglest_runtime_error("No tile-sets were found!");
			showMessageBox( "No tile-sets were found!", "Error", false);
		}
		else {
			tilesetFiles= results;
			std::for_each(results.begin(), results.end(), FormatString());

			listBoxTileset.setItems(results);
		}
	}
	catch(const std::exception &ex) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s %d]\nError detected:\n%s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);

		throw megaglest_runtime_error(szBuf);
	}
}

void MenuStateConnectedGame::loadScenarioInfo(string file, ScenarioInfo *scenarioInfo) {
	bool isTutorial = Scenario::isGameTutorial(file);
	Scenario::loadScenarioInfo(file, scenarioInfo, isTutorial);

	previewLoadDelayTimer=time(NULL);
	needToLoadTextures=true;
}

}}//end namespace
