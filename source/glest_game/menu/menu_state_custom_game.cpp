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

#include "cache_manager.h"
#include "client_interface.h"
#include "config.h"
#include "conversion.h"
#include "core_data.h"
#include "game.h"
#include "gen_uuid.h"
#include "leak_dumper.h"
#include "map_preview.h"
#include "menu_state_join_game.h"
#include "menu_state_masterserver.h"
#include "menu_state_new_game.h"
#include "metrics.h"
#include "network_manager.h"
#include "network_message.h"
#include "renderer.h"
#include "socket.h"
#include "sound_renderer.h"
#include "util.h"
#include <algorithm>
#include <curl/curl.h>
#include <iterator>
#include <time.h>

namespace Glest {
namespace Game {

using namespace ::Shared::Util;

const int MASTERSERVER_BROADCAST_MAX_WAIT_RESPONSE_SECONDS = 15;
const int MASTERSERVER_BROADCAST_PUBLISH_SECONDS = 6;
const int BROADCAST_MAP_DELAY_SECONDS = 5;
const int BROADCAST_SETTINGS_SECONDS = 4;
static const char *SAVED_SETUP_FILENAME = "lastCustomGameSettings.mgg";
static const char *DEFAULT_SETUP_FILENAME = "data/defaultGameSetup.mgg";
static const char *DEFAULT_NETWORK_SETUP_FILENAME =
    "data/defaultNetworkGameSetup.mgg";
static const char *LAST_SETUP_STRING = "LastSetup";
static const char *SETUPS_DIR = GameConstants::folder_path_setups;

const int mapPreviewTexture_X = 5;
const int mapPreviewTexture_Y = 260;
const int mapPreviewTexture_W = 150;
const int mapPreviewTexture_H = 150;

struct FormatString {
  void operator()(string &s) { s = formatString(s); }
};

// =====================================================
// 	class MenuStateCustomGame
// =====================================================
enum THREAD_NOTIFIER_TYPE { tnt_MASTERSERVER = 1, tnt_CLIENTS = 2 };

MenuStateCustomGame::MenuStateCustomGame(Program *program, MainMenu *mainMenu,
                                         bool openNetworkSlots,
                                         ParentMenuState parentMenuState,
                                         bool autostart, GameSettings *settings,
                                         bool masterserverMode,
                                         string autoloadScenarioName)
    : MenuState(program, mainMenu, "new-game") {
  try {

    this->headlessServerMode = masterserverMode;
    if (this->headlessServerMode == true) {
      printf("Waiting for players to join and start a game...\n");
    }

    this->gameUUID = getUUIDAsString();

    this->zoomedMap = false;
    this->render_mapPreviewTexture_X = mapPreviewTexture_X;
    this->render_mapPreviewTexture_Y = mapPreviewTexture_Y;
    this->render_mapPreviewTexture_W = mapPreviewTexture_W;
    this->render_mapPreviewTexture_H = mapPreviewTexture_H;

    this->lastMasterServerSettingsUpdateCount = 0;
    this->masterserverModeMinimalResources = true;
    this->parentMenuState = parentMenuState;
    this->factionVideo = NULL;
    factionVideoSwitchedOffVolume = false;

    // printf("this->masterserverMode = %d
    // [%d]\n",this->masterserverMode,masterserverMode);

    forceWaitForShutdown = true;
    this->autostart = autostart;
    this->autoStartSettings = settings;

    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem,
                               "In [%s::%s Line %d] autostart = %d\n",
                               extractFileFromDirectoryPath(__FILE__).c_str(),
                               __FUNCTION__, __LINE__, autostart);

    containerName = "CustomGame";
    activeInputLabel = NULL;
    showGeneralError = false;
    generalErrorToShow = "---";
    currentFactionLogo = "";
    factionTexture = NULL;
    currentTechName_factionPreview = "";
    currentFactionName_factionPreview = "";
    mapPreviewTexture = NULL;
    hasCheckedForUPNP = false;
    needToPublishDelayed = false;
    mapPublishingDelayTimer = time(NULL);
    headlessHasConnectedPlayer = false;
    lastPreviewedMapFile = "";

    lastCheckedCRCTilesetName = "";
    lastCheckedCRCTechtreeName = "";
    lastCheckedCRCMapName = "";

    lastRecalculatedCRCTilesetName = "";
    lastRecalculatedCRCTechtreeName = "";

    initTime = time(NULL);           // now
    lastTechtreeChange = time(NULL); // now

    lastCheckedCRCTilesetValue = 0;
    lastCheckedCRCTechtreeValue = 0;
    lastCheckedCRCMapValue = 0;

    publishToMasterserverThread = NULL;
    publishToClientsThread = NULL;

    Lang &lang = Lang::getInstance();
    NetworkManager &networkManager = NetworkManager::getInstance();
    Config &config = Config::getInstance();
    defaultPlayerName =
        config.getString("NetPlayerName", Socket::getHostName().c_str());
    enableFactionTexturePreview = config.getBool("FactionPreview", "true");
    enableMapPreview = config.getBool("MapPreview", "true");

    showFullConsole = false;

    enableScenarioTexturePreview =
        Config::getInstance().getBool("EnableScenarioTexturePreview", "true");
    scenarioLogoTexture = NULL;
    previewLoadDelayTimer = time(NULL);
    needToLoadTextures = true;
    this->autoloadScenarioName = autoloadScenarioName;
    this->dirList = Config::getInstance().getPathListForType(ptScenarios);

    string userData = Config::getInstance().getString("UserData_Root", "");
    if (getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) != "") {
      userData = getGameReadWritePath(GameConstants::path_logs_CacheLookupKey);
    }
    if (userData != "") {
      endPathWithSlash(userData);
    }
    savedSetupsDir = userData + SETUPS_DIR;

    mainMessageBox.registerGraphicComponent(containerName, "mainMessageBox");
    mainMessageBox.init(lang.getString("Ok"), 500, 300);
    mainMessageBox.setEnabled(false);
    mainMessageBoxState = 0;

    // initialize network interface
    NetworkManager::getInstance().end();

    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem,
                               "In [%s::%s Line %d]\n",
                               extractFileFromDirectoryPath(__FILE__).c_str(),
                               __FUNCTION__, __LINE__);

    serverInitError = false;
    try {
      networkManager.init(nrServer, openNetworkSlots);
    } catch (const std::exception &ex) {
      serverInitError = true;
      char szBuf[8096] = "";
      snprintf(szBuf, 8096, "In [%s::%s %d]\nNetwork init error:\n%s\n",
               extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
               __LINE__, ex.what());
      SystemFlags::OutputDebug(SystemFlags::debugError, szBuf);
      if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
        SystemFlags::OutputDebug(SystemFlags::debugSystem, "%s", szBuf);

      showGeneralError = true;
      generalErrorToShow = szBuf;
    }

    string serverPort = config.getString(
        "PortServer", intToStr(GameConstants::serverPort).c_str());
    string externalPort = config.getString("PortExternal", serverPort.c_str());
    ServerSocket::setExternalPort(strToInt(externalPort));

    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem,
                               "In [%s::%s Line %d]\n",
                               extractFileFromDirectoryPath(__FILE__).c_str(),
                               __FUNCTION__, __LINE__);

    needToSetChangedGameSettings = false;
    needToRepublishToMasterserver = false;
    needToBroadcastServerSettings = false;
    lastGameSettingsreceivedCount = -1;
    showMasterserverError = false;
    tMasterserverErrorElapsed = 0;
    masterServererErrorToShow = "---";
    lastSetChangedGameSettings = 0;
    lastMasterserverPublishing = 0;
    lastNetworkPing = 0;
    soundConnectionCount = 0;

    techTree.reset(new TechTree(config.getPathListForType(ptTechs)));

    int labelOffset = 22;
    int setupPos = 650;
    int mapHeadPos = mapPreviewTexture_Y + mapPreviewTexture_H;
    int mapPos = mapHeadPos - labelOffset;
    int aHeadPos = 280;
    int aPos = aHeadPos - labelOffset;
    int networkHeadPos = 750 - labelOffset;
    int xoffset = 10;
    int currX = 0;
    int currY = 750;
    int currXLabel = currX + 20;
    int lineHeightSmall = 18;

    int buttonx = 195;
    int buttony = 180;

    // player status
    listBoxPlayerStatus.registerGraphicComponent(containerName,
                                                 "listBoxPlayerStatus");
    listBoxPlayerStatus.init(buttonx, buttony, 165);
    vector<string> playerStatuses;
    playerStatuses.push_back(lang.getString("PlayerStatusSetup"));
    playerStatuses.push_back(lang.getString("PlayerStatusBeRightBack"));
    playerStatuses.push_back(lang.getString("PlayerStatusReady"));
    listBoxPlayerStatus.setItems(playerStatuses);
    listBoxPlayerStatus.setSelectedItemIndex(2, true);
    listBoxPlayerStatus.setTextColor(Vec3f(0.0f, 1.0f, 0.0f));
    listBoxPlayerStatus.setLighted(false);
    listBoxPlayerStatus.setVisible(true);
    buttonx += 175;

    buttonReturn.registerGraphicComponent(containerName, "buttonReturn");
    buttonReturn.init(buttonx, buttony, 125);
    buttonReturn.setText(lang.getString("Return"));
    buttonx += 135;

    buttonPlayNow.registerGraphicComponent(containerName, "buttonPlayNow");
    buttonPlayNow.init(buttonx, buttony, 125);
    buttonPlayNow.setText(lang.getString("PlayNow"));

    // network options
    currY = networkHeadPos;
    currX = 10;
    currXLabel = currX + 20;

    labelPublishServer.registerGraphicComponent(containerName,
                                                "labelPublishServer");
    labelPublishServer.init(currX, currY, 100);
    labelPublishServer.setText(lang.getString("PublishServer"));

    currY = currY - labelOffset;

    checkBoxPublishServer.registerGraphicComponent(containerName,
                                                   "checkBoxPublishServer");
    checkBoxPublishServer.init(currX, currY);
    checkBoxPublishServer.setValue(false);
    if ((this->headlessServerMode == true ||
         (openNetworkSlots == true && parentMenuState != pLanGame)) &&
        GlobalStaticFlags::isFlagSet(gsft_lan_mode) == false) {
      checkBoxPublishServer.setValue(true);
    }

    labelGameName.registerGraphicComponent(containerName, "labelGameName");
    labelGameName.init(currX + checkBoxPublishServer.getW() + 5, currY, 200);
    // labelGameName.setFont(CoreData::getInstance().getMenuFontBig());
    // labelGameName.setFont3D(CoreData::getInstance().getMenuFontBig3D());
    labelGameName.setText(createGameName());
    labelGameName.setEditable(true);
    labelGameName.setMaxEditWidth(20);
    labelGameName.setMaxEditRenderWidth(200);

    currY = 730;
    currX = 390;
    currXLabel = currX + 20;

    checkBoxNetworkPauseGameForLaggedClients.registerGraphicComponent(
        containerName, "checkBoxNetworkPauseGameForLaggedClients");
    checkBoxNetworkPauseGameForLaggedClients.init(currX, currY + 2, 16, 16);
    checkBoxNetworkPauseGameForLaggedClients.setValue(true);

    labelNetworkPauseGameForLaggedClients.registerGraphicComponent(
        containerName, "labelNetworkPauseGameForLaggedClients");
    labelNetworkPauseGameForLaggedClients.init(currXLabel, currY, 80);
    labelNetworkPauseGameForLaggedClients.setText(
        lang.getString("NetworkPauseGameForLaggedClients"));
    currY = currY - lineHeightSmall;

    bool allowInProgressJoin =
        Config::getInstance().getBool("EnableJoinInProgressGame", "false");
    labelAllowInGameJoinPlayer.registerGraphicComponent(
        containerName, "labelAllowInGameJoinPlayer");
    labelAllowInGameJoinPlayer.init(currXLabel, currY, 80);
    labelAllowInGameJoinPlayer.setText(lang.getString("AllowInGameJoinPlayer"));
    labelAllowInGameJoinPlayer.setVisible(allowInProgressJoin);

    checkBoxAllowInGameJoinPlayer.registerGraphicComponent(
        containerName, "checkBoxAllowInGameJoinPlayer");
    checkBoxAllowInGameJoinPlayer.init(currX, currX, 16, 16);
    checkBoxAllowInGameJoinPlayer.setValue(false);
    checkBoxAllowInGameJoinPlayer.setVisible(allowInProgressJoin);

    currY = 680;
    vector<string> rMultiplier;
    for (int i = 0; i < 45; ++i) {
      rMultiplier.push_back(floatToStr(0.5f + 0.1f * i, 1));
    }
    listBoxFallbackCpuMultiplier.registerGraphicComponent(
        containerName, "listBoxFallbackCpuMultiplier");
    listBoxFallbackCpuMultiplier.init(currX - 44, currY + 2, 60, 16);
    listBoxFallbackCpuMultiplier.setItems(rMultiplier);
    listBoxFallbackCpuMultiplier.setSelectedItem("1.5");
    labelFallbackCpuMultiplier.registerGraphicComponent(
        containerName, "labelFallbackCpuMultiplier");
    labelFallbackCpuMultiplier.init(currXLabel, currY, 80);
    labelFallbackCpuMultiplier.setText(lang.getString("FallbackCpuMultiplier"));
    setSmallFont(labelAllowNativeLanguageTechtree);
    currY = currY - lineHeightSmall;

    xoffset = 65;
    // MapFilter
    labelMapFilter.registerGraphicComponent(containerName, "labelMapFilter");
    labelMapFilter.init(xoffset + 525, mapHeadPos);
    labelMapFilter.setText(lang.getString("MapFilter"));
    labelMapFilter.setVisible(false);

    labelMap.registerGraphicComponent(containerName, "labelMap");
    labelMap.init(xoffset + 100, mapPos + 20);
    labelMap.setText(lang.getString("Map"));

    // Map Filter
    listBoxMapFilter.registerGraphicComponent(containerName,
                                              "listBoxMapFilter");
    listBoxMapFilter.init(xoffset + 260, mapPos + labelOffset, 60);
    listBoxMapFilter.pushBackItem("-");
    for (int i = 1; i < GameConstants::maxPlayers + 1; ++i) {
      listBoxMapFilter.pushBackItem(intToStr(i));
    }
    listBoxMapFilter.setSelectedItemIndex(0);

    // map listBox
    comboBoxMap.registerGraphicComponent(containerName, "comboBoxMap");
    comboBoxMap.init(xoffset + 100, mapPos, 220);
    // put them all in a set, to weed out duplicates (gbm & mgm with same name)
    // will also ensure they are alphabetically listed (rather than how the OS
    // provides them)
    int initialMapSelection = setupMapList("");
    comboBoxMap.setItems(formattedPlayerSortedMaps[0]);
    comboBoxMap.setSelectedItemIndex(initialMapSelection);

    labelMapInfo.registerGraphicComponent(containerName, "labelMapInfo");
    labelMapInfo.init(xoffset + 100, mapPos - labelOffset - 10, 200,
                      40); // position is set by update() !
    setSmallFont(labelMapInfo);

    // fog - o - war
    // @350 ? 300 ?
    labelFogOfWar.registerGraphicComponent(containerName, "labelFogOfWar");
    labelFogOfWar.init(xoffset + 100, aHeadPos, 165);
    labelFogOfWar.setText(lang.getString("FogOfWar"));

    listBoxFogOfWar.registerGraphicComponent(containerName, "listBoxFogOfWar");
    listBoxFogOfWar.init(xoffset + 100, aPos, 165);
    listBoxFogOfWar.pushBackItem(lang.getString("Enabled"));
    listBoxFogOfWar.pushBackItem(lang.getString("Explored"));
    listBoxFogOfWar.pushBackItem(lang.getString("Disabled"));
    listBoxFogOfWar.setSelectedItemIndex(0);

    // tech Tree listBox
    labelTechTree.registerGraphicComponent(containerName, "labelTechTree");
    labelTechTree.init(xoffset + 325, mapHeadPos);
    labelTechTree.setText(lang.getString("TechTree"));

    int initialTechSelection = setupTechList("", true);
    listBoxTechTree.registerGraphicComponent(containerName, "listBoxTechTree");
    listBoxTechTree.init(xoffset + 325, mapPos, 180);
    if (listBoxTechTree.getItemCount() > 0) {
      listBoxTechTree.setSelectedItemIndex(initialTechSelection);
    }

    labelTileset.registerGraphicComponent(containerName, "labelTileset");
    labelTileset.init(xoffset + 325, mapHeadPos - 44);
    labelTileset.setText(lang.getString("Tileset"));

    // tileset listBox
    listBoxTileset.registerGraphicComponent(containerName, "listBoxTileset");
    listBoxTileset.init(xoffset + 325, mapPos - 44, 180);

    setupTilesetList("");
    Chrono seed(true);
    srand((unsigned int)seed.getCurTicks());

    listBoxTileset.setSelectedItemIndex(rand() % listBoxTileset.getItemCount());

    // Save Setup
    currY = mapHeadPos - 100;
    currX = xoffset + 325;

    comboBoxLoadSetup.registerGraphicComponent(containerName,
                                               "comboBoxLoadSetup");
    comboBoxLoadSetup.init(currX, currY, 220);
    loadSavedSetupNames();
    comboBoxLoadSetup.setItems(savedSetupFilenames);

    currY = currY - labelOffset;

    buttonDeleteSetup.registerGraphicComponent(containerName,
                                               "buttonDeleteSetup");
    buttonDeleteSetup.init(currX, currY, 110);
    buttonDeleteSetup.setText(lang.getString("Delete"));
    buttonLoadSetup.registerGraphicComponent(containerName, "buttonLoadSetup");
    buttonLoadSetup.init(currX + 110, currY, 110);
    buttonLoadSetup.setText(lang.getString("Load"));

    currY = currY - labelOffset;

    labelSaveSetupName.registerGraphicComponent(containerName,
                                                "labelSaveSetupName");
    labelSaveSetupName.init(currX, currY, 110);
    labelSaveSetupName.setText("");
    labelSaveSetupName.setEditable(true);
    labelSaveSetupName.setMaxEditWidth(16);
    labelSaveSetupName.setMaxEditRenderWidth(labelSaveSetupName.getW());
    labelSaveSetupName.setBackgroundColor(Vec4f(230, 230, 230, 0.4));
    labelSaveSetupName.setRenderBackground(true);

    buttonSaveSetup.registerGraphicComponent(containerName, "buttonSaveSetup");
    buttonSaveSetup.init(currX + 110, currY, 110);
    buttonSaveSetup.setText(lang.getString("Save"));

    // Toy Block
    currY = mapHeadPos;
    currX = 750;
    currXLabel = currX + 20;

    checkBoxAllowTeamUnitSharing.registerGraphicComponent(
        containerName, "checkBoxAllowTeamUnitSharing");
    checkBoxAllowTeamUnitSharing.init(currX, currY + 2, 16, 16);
    checkBoxAllowTeamUnitSharing.setValue(false);
    checkBoxAllowTeamUnitSharing.setVisible(true);

    labelAllowTeamUnitSharing.registerGraphicComponent(
        containerName, "labelAllowTeamUnitSharing");
    labelAllowTeamUnitSharing.init(currXLabel, currY, 80);
    labelAllowTeamUnitSharing.setText(lang.getString("AllowTeamUnitSharing"));
    labelAllowTeamUnitSharing.setVisible(true);
    setSmallFont(labelAllowTeamUnitSharing);
    currY = currY - lineHeightSmall;

    checkBoxAllowTeamResourceSharing.registerGraphicComponent(
        containerName, "checkBoxAllowTeamResourceSharing");
    checkBoxAllowTeamResourceSharing.init(currX, currY + 2, 16, 16);
    checkBoxAllowTeamResourceSharing.setValue(false);
    checkBoxAllowTeamResourceSharing.setVisible(true);
    labelAllowTeamResourceSharing.registerGraphicComponent(
        containerName, "labelAllowTeamResourceSharing");
    labelAllowTeamResourceSharing.init(currXLabel, currY, 80);
    labelAllowTeamResourceSharing.setText(
        lang.getString("AllowTeamResourceSharing"));
    labelAllowTeamResourceSharing.setVisible(true);
    setSmallFont(labelAllowTeamResourceSharing);
    currY = currY - lineHeightSmall;

    checkBoxAllowNativeLanguageTechtree.registerGraphicComponent(
        containerName, "checkBoxAllowNativeLanguageTechtree");
    checkBoxAllowNativeLanguageTechtree.init(currX, currY + 2, 16, 16);
    checkBoxAllowNativeLanguageTechtree.setValue(false);

    labelAllowNativeLanguageTechtree.registerGraphicComponent(
        containerName, "labelAllowNativeLanguageTechtree");
    labelAllowNativeLanguageTechtree.init(currXLabel, currY, 80);
    labelAllowNativeLanguageTechtree.setText(
        lang.getString("AllowNativeLanguageTechtree"));
    setSmallFont(labelAllowNativeLanguageTechtree);
    currY = currY - lineHeightSmall;

    // Allow Observers
    checkBoxAllowObservers.registerGraphicComponent(containerName,
                                                    "checkBoxAllowObservers");
    checkBoxAllowObservers.init(currX, currY + 2, 16, 16);
    checkBoxAllowObservers.setValue(true);

    labelAllowObservers.registerGraphicComponent(containerName,
                                                 "labelAllowObservers");
    labelAllowObservers.init(currXLabel, currY, 80);
    labelAllowObservers.setText(lang.getString("AllowObservers"));
    setSmallFont(labelAllowNativeLanguageTechtree);
    currY = currY - lineHeightSmall;

    checkBoxEnableSwitchTeamMode.registerGraphicComponent(
        containerName, "checkBoxEnableSwitchTeamMode");
    checkBoxEnableSwitchTeamMode.init(currX, currY + 2, 16, 16);
    checkBoxEnableSwitchTeamMode.setValue(false);
    labelEnableSwitchTeamMode.registerGraphicComponent(
        containerName, "labelEnableSwitchTeamMode");
    labelEnableSwitchTeamMode.init(currXLabel, currY);
    labelEnableSwitchTeamMode.setText(lang.getString("EnableSwitchTeamMode"));
    setSmallFont(labelEnableSwitchTeamMode);
    currY = currY - lineHeightSmall;

    listBoxAISwitchTeamAcceptPercent.registerGraphicComponent(
        containerName, "listBoxAISwitchTeamAcceptPercent");
    listBoxAISwitchTeamAcceptPercent.init(currX - 44, currY + 2, 60, 16);
    for (int i = 0; i <= 100; i = i + 10) {
      listBoxAISwitchTeamAcceptPercent.pushBackItem(intToStr(i));
    }
    listBoxAISwitchTeamAcceptPercent.setSelectedItem(intToStr(30));
    listBoxAISwitchTeamAcceptPercent.setVisible(false);
    labelAISwitchTeamAcceptPercent.registerGraphicComponent(
        containerName, "labelAISwitchTeamAcceptPercent");
    labelAISwitchTeamAcceptPercent.init(currXLabel, currY, 80);
    labelAISwitchTeamAcceptPercent.setText(
        lang.getString("AISwitchTeamAcceptPercent"));
    labelAISwitchTeamAcceptPercent.setVisible(false);
    setSmallFont(labelEnableSwitchTeamMode);
    currY = currY - lineHeightSmall;

    // Network Scenario
    checkBoxScenario.registerGraphicComponent(containerName,
                                              "checkBoxScenario");
    checkBoxScenario.init(currX, currY + 2, 16, 16);
    checkBoxScenario.setValue(false);
    labelScenario.registerGraphicComponent(containerName, "labelScenario");
    labelScenario.init(currXLabel, currY);
    labelScenario.setText(lang.getString("NetworkScenarios"));
    setSmallFont(labelAllowNativeLanguageTechtree);
    currY = currY - lineHeightSmall;
    listBoxScenario.registerGraphicComponent(containerName, "listBoxScenario");
    listBoxScenario.init(currX, currY, 190, 16);

    // scenario listbox
    vector<string> resultsScenarios;
    findDirs(dirList, resultsScenarios);
    // Filter out only scenarios with no network slots
    for (int i = 0; i < (int)resultsScenarios.size(); ++i) {
      string scenario = resultsScenarios[i];
      string file = Scenario::getScenarioPath(dirList, scenario);

      try {
        if (file != "") {
          bool isTutorial = Scenario::isGameTutorial(file);
          Scenario::loadScenarioInfo(file, &scenarioInfo, isTutorial);

          bool isNetworkScenario = false;
          for (unsigned int j = 0; isNetworkScenario == false &&
                                   j < (unsigned int)GameConstants::maxPlayers;
               ++j) {
            if (scenarioInfo.factionControls[j] == ctNetwork) {
              isNetworkScenario = true;
            }
          }
          if (isNetworkScenario == true) {
            scenarioFiles.push_back(scenario);
          }
        }
      } catch (const std::exception &ex) {
        char szBuf[8096] = "";
        snprintf(szBuf, 8096,
                 "In [%s::%s %d]\nError loading scenario [%s]:\n%s\n",
                 extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                 __LINE__, scenario.c_str(), ex.what());
        SystemFlags::OutputDebug(SystemFlags::debugError, szBuf);
        if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
          SystemFlags::OutputDebug(SystemFlags::debugSystem, "%s", szBuf);

        showGeneralError = true;
        generalErrorToShow = szBuf;
        // throw megaglest_runtime_error(szBuf);
      }
    }
    resultsScenarios.clear();
    for (int i = 0; i < (int)scenarioFiles.size(); ++i) {
      resultsScenarios.push_back(formatString(scenarioFiles[i]));
    }
    listBoxScenario.setItems(resultsScenarios);
    if (resultsScenarios.empty() == true) {
      checkBoxScenario.setEnabled(false);
    }

    currY = currY - lineHeightSmall;
    buttonShowLanInfo.registerGraphicComponent(containerName,
                                               "buttonShowLanInfo");
    buttonShowLanInfo.init(currX, currY, 200, 16);
    buttonShowLanInfo.setText(lang.getString("ShowIP"));
    currY = currY - lineHeightSmall;

    // Advanced Options
    labelAdvanced.registerGraphicComponent(containerName, "labelAdvanced");
    labelAdvanced.init(750, 80, 80);
    labelAdvanced.setText(lang.getString("AdvancedGameOptions"));

    checkBoxAdvanced.registerGraphicComponent(containerName,
                                              "checkBoxAdvanced");
    checkBoxAdvanced.init(750, 80 - labelOffset);
    checkBoxAdvanced.setValue(false);

    // list boxes
    xoffset = 5;
    int rowHeight = 22;
    for (int i = 0; i < GameConstants::maxPlayers; ++i) {

      labelPlayers[i].registerGraphicComponent(containerName,
                                               "labelPlayers" + intToStr(i));
      labelPlayers[i].init(xoffset - 1, setupPos - 30 - i * rowHeight + 2);
      labelPlayers[i].setFont(CoreData::getInstance().getMenuFontVeryBig());
      labelPlayers[i].setFont3D(CoreData::getInstance().getMenuFontVeryBig3D());

      labelPlayerStatus[i].registerGraphicComponent(
          containerName, "labelPlayerStatus" + intToStr(i));
      labelPlayerStatus[i].init(xoffset + 14,
                                setupPos - 30 - i * rowHeight + 2);
      labelPlayerNames[i].registerGraphicComponent(
          containerName, "labelPlayerNames" + intToStr(i));
      labelPlayerNames[i].init(xoffset + 30, setupPos - 30 - i * rowHeight);

      listBoxControls[i].registerGraphicComponent(
          containerName, "listBoxControls" + intToStr(i));
      listBoxControls[i].init(xoffset + 160, setupPos - 30 - i * rowHeight,
                              174);

      buttonBlockPlayers[i].registerGraphicComponent(
          containerName, "buttonBlockPlayers" + intToStr(i));
      // buttonBlockPlayers[i].init(xoffset+355, setupPos-30-i*rowHeight, 70);
      buttonBlockPlayers[i].init(xoffset + 185, setupPos - 30 - i * rowHeight,
                                 124);
      buttonBlockPlayers[i].setText(lang.getString("BlockPlayer"));
      buttonBlockPlayers[i].setFont(
          CoreData::getInstance().getDisplayFontSmall());
      buttonBlockPlayers[i].setFont3D(
          CoreData::getInstance().getDisplayFontSmall3D());

      listBoxRMultiplier[i].registerGraphicComponent(
          containerName, "listBoxRMultiplier" + intToStr(i));
      listBoxRMultiplier[i].init(xoffset + 336, setupPos - 30 - i * rowHeight,
                                 70);

      listBoxFactions[i].registerGraphicComponent(
          containerName, "listBoxFactions" + intToStr(i));
      listBoxFactions[i].init(xoffset + 411, setupPos - 30 - i * rowHeight,
                              147);
      listBoxFactions[i].setLeftControlled(true);

      listBoxTeams[i].registerGraphicComponent(containerName,
                                               "listBoxTeams" + intToStr(i));
      listBoxTeams[i].init(xoffset + 560, setupPos - 30 - i * rowHeight, 60);
      listBoxTeams[i].setLighted(true);

      labelNetStatus[i].registerGraphicComponent(
          containerName, "labelNetStatus" + intToStr(i));
      labelNetStatus[i].init(xoffset + 626, setupPos - 30 - i * rowHeight, 60);
      labelNetStatus[i].setFont(CoreData::getInstance().getDisplayFontSmall());
      labelNetStatus[i].setFont3D(
          CoreData::getInstance().getDisplayFontSmall3D());
    }

    buttonClearBlockedPlayers.registerGraphicComponent(
        containerName, "buttonClearBlockedPlayers");
    buttonClearBlockedPlayers.init(xoffset + 160, setupPos - 30 - 8 * rowHeight,
                                   174 + 2 + 70);
    buttonClearBlockedPlayers.setText(lang.getString("BlockPlayerClear"));

    labelControl.registerGraphicComponent(containerName, "labelControl");
    labelControl.init(xoffset + 160, setupPos, 50, GraphicListBox::defH, true);
    labelControl.setText(lang.getString("Control"));

    labelRMultiplier.registerGraphicComponent(containerName,
                                              "labelRMultiplier");
    labelRMultiplier.init(xoffset + 310, setupPos, 50, GraphicListBox::defH,
                          true);

    labelFaction.registerGraphicComponent(containerName, "labelFaction");
    labelFaction.init(xoffset + 411, setupPos, 50, GraphicListBox::defH, true);
    labelFaction.setText(lang.getString("Faction"));

    labelTeam.registerGraphicComponent(containerName, "labelTeam");
    labelTeam.init(xoffset + 560, setupPos, 50, GraphicListBox::defH, true);
    labelTeam.setText(lang.getString("Team"));

    labelControl.setFont(CoreData::getInstance().getMenuFontBig());
    labelControl.setFont3D(CoreData::getInstance().getMenuFontBig3D());
    labelRMultiplier.setFont(CoreData::getInstance().getMenuFontBig());
    labelRMultiplier.setFont3D(CoreData::getInstance().getMenuFontBig3D());
    labelFaction.setFont(CoreData::getInstance().getMenuFontBig());
    labelFaction.setFont3D(CoreData::getInstance().getMenuFontBig3D());
    labelTeam.setFont(CoreData::getInstance().getMenuFontBig());
    labelTeam.setFont3D(CoreData::getInstance().getMenuFontBig3D());

    // xoffset=100;

    vector<string> controlItems;
    controlItems.push_back(lang.getString("Closed"));
    controlItems.push_back(lang.getString("CpuEasy"));
    controlItems.push_back(lang.getString("Cpu"));
    controlItems.push_back(lang.getString("CpuUltra"));
    controlItems.push_back(lang.getString("CpuMega"));
    controlItems.push_back(lang.getString("Network"));
    controlItems.push_back(lang.getString("NetworkUnassigned"));
    controlItems.push_back(lang.getString("Human"));

    if (config.getBool("EnableNetworkCpu", "false") == true) {
      controlItems.push_back(lang.getString("NetworkCpuEasy"));
      controlItems.push_back(lang.getString("NetworkCpu"));
      controlItems.push_back(lang.getString("NetworkCpuUltra"));
      controlItems.push_back(lang.getString("NetworkCpuMega"));
    }

    vector<string> teamItems;
    for (int i = 1; i <= GameConstants::maxPlayers; ++i) {
      teamItems.push_back(intToStr(i));
    }
    for (int i = GameConstants::maxPlayers + 1;
         i <= GameConstants::maxPlayers + GameConstants::specialFactions; ++i) {
      teamItems.push_back(intToStr(i));
    }

    reloadFactions(false, "");

    if (factionFiles.empty() == true) {
      showGeneralError = true;
      generalErrorToShow =
          "[#1] There are no factions for the tech tree [" +
          techTreeFiles[listBoxTechTree.getSelectedItemIndex()] + "]";
    }

    for (int i = 0; i < GameConstants::maxPlayers; ++i) {
      labelPlayerStatus[i].setText(" ");
      labelPlayerStatus[i].setTexture(
          CoreData::getInstance().getStatusReadyTexture());
      labelPlayerStatus[i].setH(16);
      labelPlayerStatus[i].setW(12);

      // labelPlayers[i].setText(lang.getString("Player")+" "+intToStr(i));
      labelPlayers[i].setText(intToStr(i + 1));
      labelPlayerNames[i].setText("*");
      labelPlayerNames[i].setMaxEditWidth(16);
      labelPlayerNames[i].setMaxEditRenderWidth(127);

      listBoxTeams[i].setItems(teamItems);
      listBoxTeams[i].setSelectedItemIndex(i);
      lastSelectedTeamIndex[i] = listBoxTeams[i].getSelectedItemIndex();

      listBoxControls[i].setItems(controlItems);
      listBoxRMultiplier[i].setItems(rMultiplier);
      listBoxRMultiplier[i].setSelectedItem("1.0");
      labelNetStatus[i].setText("");
    }

    loadMapInfo(Config::getMapPath(getCurrentMapFile()), &mapInfo, true, true);
    labelMapInfo.setText(mapInfo.desc);

    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem,
                               "In [%s::%s Line %d]\n",
                               extractFileFromDirectoryPath(__FILE__).c_str(),
                               __FUNCTION__, __LINE__);

    // init controllers
    if (serverInitError == false) {
      ServerInterface *serverInterface =
          NetworkManager::getInstance().getServerInterface();
      if (serverInterface == NULL) {
        throw megaglest_runtime_error("serverInterface == NULL");
      }
      if (this->headlessServerMode == true) {
        listBoxControls[0].setSelectedItemIndex(ctNetwork);
        updateResourceMultiplier(0);
      } else {
        setSlotHuman(0);
        updateResourceMultiplier(0);
      }
      labelPlayerNames[0].setText("");
      labelPlayerNames[0].setText(getHumanPlayerName());

      if (openNetworkSlots == true) {
        for (int i = 1; i < mapInfo.players; ++i) {
          listBoxControls[i].setSelectedItemIndex(ctNetwork);
        }
      } else {
        listBoxControls[1].setSelectedItemIndex(ctCpu);
      }
      updateControlers();
      updateNetworkSlots();

      if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
        SystemFlags::OutputDebug(SystemFlags::debugSystem,
                                 "In [%s::%s Line %d]\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__);

      // Ensure we have set the gamesettings at least once
      GameSettings gameSettings;
      copyToGameSettings(&gameSettings);

      serverInterface->setGameSettings(&gameSettings, false);
    }
    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem,
                               "In [%s::%s Line %d]\n",
                               extractFileFromDirectoryPath(__FILE__).c_str(),
                               __FUNCTION__, __LINE__);

    updateAllResourceMultiplier();

    // write hint to console:
    Config &configKeys = Config::getInstance(
        std::pair<ConfigType, ConfigType>(cfgMainKeys, cfgUserKeys));

    console.addLine(lang.getString("ToSwitchOffMusicPress") + " - \"" +
                    configKeys.getString("ToggleMusic") + "\"");

    chatManager.init(&console, -1, true);

    GraphicComponent::applyAllCustomProperties(containerName);

    static string mutexOwnerId =
        string(extractFileFromDirectoryPath(__FILE__).c_str()) + string("_") +
        intToStr(__LINE__);
    publishToMasterserverThread =
        new SimpleTaskThread(this, 0, 300, false, (void *)tnt_MASTERSERVER);
    publishToMasterserverThread->setUniqueID(mutexOwnerId);

    static string mutexOwnerId2 =
        string(extractFileFromDirectoryPath(__FILE__).c_str()) + string("_") +
        intToStr(__LINE__);
    publishToClientsThread =
        new SimpleTaskThread(this, 0, 200, false, (void *)tnt_CLIENTS, false);
    publishToClientsThread->setUniqueID(mutexOwnerId2);

    publishToMasterserverThread->start();
    publishToClientsThread->start();

    if (openNetworkSlots == true) {
      string data_path =
          getGameReadWritePath(GameConstants::path_data_CacheLookupKey);

      if (fileExists(data_path + DEFAULT_NETWORK_SETUP_FILENAME) == true)
        loadGameSettings(data_path + DEFAULT_NETWORK_SETUP_FILENAME);
    } else {
      string data_path =
          getGameReadWritePath(GameConstants::path_data_CacheLookupKey);

      if (fileExists(data_path + DEFAULT_SETUP_FILENAME) == true)
        loadGameSettings(data_path + DEFAULT_SETUP_FILENAME);
    }
    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem,
                               "In [%s::%s Line %d]\n",
                               extractFileFromDirectoryPath(__FILE__).c_str(),
                               __FUNCTION__, __LINE__);

  } catch (const std::exception &ex) {
    char szBuf[8096] = "";
    snprintf(szBuf, 8096, "In [%s::%s %d]\nError detected:\n%s\n",
             extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
             __LINE__, ex.what());
    SystemFlags::OutputDebug(SystemFlags::debugError, szBuf);
    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem, "%s", szBuf);

    throw megaglest_runtime_error(szBuf);
  }
}

string MenuStateCustomGame::createGameName(string controllingPlayer) {
  Config &config = Config::getInstance();
  string serverTitle = config.getString("ServerTitle", "");

  if (serverTitle != "" && controllingPlayer == "") {
    return serverTitle;
  } else if (this->headlessServerMode == true) {
    if (controllingPlayer != "") {
      return controllingPlayer + " controls";
    } else {
      return "[H] " + defaultPlayerName;
    }
  } else {
    string defaultPlayerNameEnd =
        defaultPlayerName.substr(defaultPlayerName.size() - 1, 1);
    if (defaultPlayerNameEnd == "s") {
      return defaultPlayerName + "' game";
    } else {
      return defaultPlayerName + "'s game";
    }
  }
}

void MenuStateCustomGame::reloadUI() {
  Lang &lang = Lang::getInstance();
  Config &config = Config::getInstance();

  console.resetFonts();
  mainMessageBox.init(lang.getString("Ok"), 500, 300);

  labelMap.setText(lang.getString("Map"));

  labelMapFilter.setText(lang.getString("MapFilter"));

  labelTileset.setText(lang.getString("Tileset"));

  labelTechTree.setText(lang.getString("TechTree"));

  labelAllowNativeLanguageTechtree.setText(
      lang.getString("AllowNativeLanguageTechtree"));

  labelFogOfWar.setText(lang.getString("FogOfWar"));

  buttonSaveSetup.setText(lang.getString("Save"));
  buttonLoadSetup.setText(lang.getString("Load"));
  buttonDeleteSetup.setText(lang.getString("Delete"));

  std::vector<std::string> listBoxData;
  listBoxData.push_back(lang.getString("Enabled"));
  listBoxData.push_back(lang.getString("Explored"));
  listBoxData.push_back(lang.getString("Disabled"));
  listBoxFogOfWar.setItems(listBoxData);

  // Allow Observers
  labelAllowObservers.setText(lang.getString("AllowObservers"));

  // Allow Switch Team Mode
  labelEnableSwitchTeamMode.setText(lang.getString("EnableSwitchTeamMode"));

  labelAllowInGameJoinPlayer.setText(lang.getString("AllowInGameJoinPlayer"));

  labelAllowTeamUnitSharing.setText(lang.getString("AllowTeamUnitSharing"));
  labelAllowTeamResourceSharing.setText(
      lang.getString("AllowTeamResourceSharing"));

  labelAISwitchTeamAcceptPercent.setText(
      lang.getString("AISwitchTeamAcceptPercent"));

  listBoxData.clear();

  // Advanced Options
  labelAdvanced.setText(lang.getString("AdvancedGameOptions"));

  labelPublishServer.setText(lang.getString("PublishServer"));

  labelGameName.setFont(CoreData::getInstance().getMenuFontBig());
  labelGameName.setFont3D(CoreData::getInstance().getMenuFontBig3D());

  labelGameName.setText(createGameName());

  labelNetworkPauseGameForLaggedClients.setText(
      lang.getString("NetworkPauseGameForLaggedClients"));

  for (int i = 0; i < GameConstants::maxPlayers; ++i) {
    buttonBlockPlayers[i].setText(lang.getString("BlockPlayer"));
  }

  labelControl.setText(lang.getString("Control"));

  labelFaction.setText(lang.getString("Faction"));

  labelTeam.setText(lang.getString("Team"));

  labelControl.setFont(CoreData::getInstance().getMenuFontBig());
  labelControl.setFont3D(CoreData::getInstance().getMenuFontBig3D());
  labelRMultiplier.setFont(CoreData::getInstance().getMenuFontBig());
  labelRMultiplier.setFont3D(CoreData::getInstance().getMenuFontBig3D());
  labelFaction.setFont(CoreData::getInstance().getMenuFontBig());
  labelFaction.setFont3D(CoreData::getInstance().getMenuFontBig3D());
  labelTeam.setFont(CoreData::getInstance().getMenuFontBig());
  labelTeam.setFont3D(CoreData::getInstance().getMenuFontBig3D());

  // texts
  buttonClearBlockedPlayers.setText(lang.getString("BlockPlayerClear"));
  buttonReturn.setText(lang.getString("Return"));
  buttonPlayNow.setText(lang.getString("PlayNow"));

  vector<string> controlItems;
  controlItems.push_back(lang.getString("Closed"));
  controlItems.push_back(lang.getString("CpuEasy"));
  controlItems.push_back(lang.getString("Cpu"));
  controlItems.push_back(lang.getString("CpuUltra"));
  controlItems.push_back(lang.getString("CpuMega"));
  controlItems.push_back(lang.getString("Network"));
  controlItems.push_back(lang.getString("NetworkUnassigned"));
  controlItems.push_back(lang.getString("Human"));

  if (config.getBool("EnableNetworkCpu", "false") == true) {
    controlItems.push_back(lang.getString("NetworkCpuEasy"));
    controlItems.push_back(lang.getString("NetworkCpu"));
    controlItems.push_back(lang.getString("NetworkCpuUltra"));
    controlItems.push_back(lang.getString("NetworkCpuMega"));
  }

  for (int i = 0; i < GameConstants::maxPlayers; ++i) {
    labelPlayers[i].setText(intToStr(i + 1));

    listBoxControls[i].setItems(controlItems);
  }

  labelFallbackCpuMultiplier.setText(lang.getString("FallbackCpuMultiplier"));

  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem, "In [%s::%s Line %d]\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__);

  vector<string> playerStatuses;
  playerStatuses.push_back(lang.getString("PlayerStatusSetup"));
  playerStatuses.push_back(lang.getString("PlayerStatusBeRightBack"));
  playerStatuses.push_back(lang.getString("PlayerStatusReady"));
  listBoxPlayerStatus.setItems(playerStatuses);

  labelScenario.setText(lang.getString("NetworkScenarios"));
  buttonShowLanInfo.setText(lang.getString("LanIP"));

  reloadFactions(true,
                 (checkBoxScenario.getValue() == true
                      ? scenarioFiles[listBoxScenario.getSelectedItemIndex()]
                      : ""));

  // write hint to console:
  Config &configKeys = Config::getInstance(
      std::pair<ConfigType, ConfigType>(cfgMainKeys, cfgUserKeys));

  console.addLine(lang.getString("ToSwitchOffMusicPress") + " - \"" +
                  configKeys.getString("ToggleMusic") + "\"");

  chatManager.init(&console, -1, true);

  GraphicComponent::reloadFontsForRegisterGraphicComponents(containerName);
}

void MenuStateCustomGame::cleanupThread(SimpleTaskThread **thread) {
  // printf("LINE: %d *thread = %p\n",__LINE__,*thread);

  if (thread != NULL && *thread != NULL) {
    if (SystemFlags::VERBOSE_MODE_ENABLED)
      printf("\n\n#1 cleanupThread callingThread [%p]\n", *thread);

    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem,
                               "In [%s::%s Line %d]\n",
                               extractFileFromDirectoryPath(__FILE__).c_str(),
                               __FUNCTION__, __LINE__);

    SimpleTaskThread *threadPtr = *thread;
    int value = threadPtr->getUserdataAsInt();
    THREAD_NOTIFIER_TYPE threadType = (THREAD_NOTIFIER_TYPE)value;

    if (SystemFlags::VERBOSE_MODE_ENABLED)
      printf("\n\n#1. cleanupThread callingThread [%p] value = %d\n", *thread,
             value);

    needToBroadcastServerSettings = false;
    needToRepublishToMasterserver = false;
    lastNetworkPing = time(NULL);
    threadPtr->setThreadOwnerValid(false);

    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem,
                               "In [%s::%s Line %d]\n",
                               extractFileFromDirectoryPath(__FILE__).c_str(),
                               __FUNCTION__, __LINE__);

    if (SystemFlags::VERBOSE_MODE_ENABLED)
      printf("\n\n#1.. cleanupThread callingThread [%p] value = %d\n", *thread,
             value);

    if (forceWaitForShutdown == true) {
      time_t elapsed = time(NULL);
      threadPtr->signalQuit();

      if (SystemFlags::VERBOSE_MODE_ENABLED)
        printf("\n\n#1a cleanupThread callingThread [%p]\n", *thread);

      for (; (threadPtr->canShutdown(false) == false ||
              threadPtr->getRunningStatus() == true) &&
             difftime((long int)time(NULL), elapsed) <= 15;) {
        // sleep(150);
      }

      if (SystemFlags::VERBOSE_MODE_ENABLED)
        printf("\n\n#1b cleanupThread callingThread [%p]\n", *thread);

      if (threadPtr->canShutdown(true) == true &&
          threadPtr->getRunningStatus() == false) {
        if (SystemFlags::VERBOSE_MODE_ENABLED)
          printf("\n\n#1c cleanupThread callingThread [%p]\n", *thread);

        delete threadPtr;
        // printf("LINE: %d *thread = %p\n",__LINE__,*thread);
        if (SystemFlags::VERBOSE_MODE_ENABLED)
          printf("In [%s::%s Line: %d]\n",
                 extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                 __LINE__);
      } else {
        if (SystemFlags::VERBOSE_MODE_ENABLED)
          printf("\n\n#1d cleanupThread callingThread [%p]\n", *thread);

        char szBuf[8096] = "";
        snprintf(szBuf, 8096, "In [%s::%s %d] Error cannot shutdown thread\n",
                 extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                 __LINE__);
        // SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
        if (SystemFlags::VERBOSE_MODE_ENABLED)
          printf("%s", szBuf);
        if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
          SystemFlags::OutputDebug(SystemFlags::debugSystem, "%s", szBuf);

        if (threadType == tnt_MASTERSERVER) {
          threadPtr->setOverrideShutdownTask(shutdownTaskStatic);
        }
        threadPtr->setDeleteSelfOnExecutionDone(true);
        threadPtr->setDeleteAfterExecute(true);
        // printf("LINE: %d *thread = %p\n",__LINE__,*thread);
      }
      threadPtr = NULL;
      // if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\n\n#1e cleanupThread
      // callingThread [%p]\n",*thread);
    } else {
      if (SystemFlags::VERBOSE_MODE_ENABLED)
        printf("\n\n#1f cleanupThread callingThread [%p]\n", *thread);
      threadPtr->signalQuit();
      sleep(0);
      if (threadPtr->canShutdown(true) == true &&
          threadPtr->getRunningStatus() == false) {
        if (SystemFlags::VERBOSE_MODE_ENABLED)
          printf("\n\n#1g cleanupThread callingThread [%p]\n", *thread);
        delete threadPtr;
        // printf("LINE: %d *thread = %p\n",__LINE__,*thread);
      } else {
        if (SystemFlags::VERBOSE_MODE_ENABLED)
          printf("\n\n#1h cleanupThread callingThread [%p]\n", *thread);
        char szBuf[8096] = "";
        snprintf(szBuf, 8096, "In [%s::%s %d] Error cannot shutdown thread\n",
                 extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                 __LINE__);
        // SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
        if (SystemFlags::VERBOSE_MODE_ENABLED)
          printf("%s", szBuf);
        if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
          SystemFlags::OutputDebug(SystemFlags::debugSystem, "%s", szBuf);

        if (threadType == tnt_MASTERSERVER) {
          threadPtr->setOverrideShutdownTask(shutdownTaskStatic);
        }
        threadPtr->setDeleteSelfOnExecutionDone(true);
        threadPtr->setDeleteAfterExecute(true);
        // printf("LINE: %d *thread = %p\n",__LINE__,*thread);
      }
    }

    *thread = NULL;
    if (SystemFlags::VERBOSE_MODE_ENABLED)
      printf("\n\n#2 cleanupThread callingThread [%p]\n", *thread);
  }
  // printf("LINE: %d *thread = %p\n",__LINE__,*thread);
}

void MenuStateCustomGame::cleanup() {
  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem, "In [%s::%s Line %d]\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__);

  if (publishToMasterserverThread) {
    // printf("LINE: %d\n",__LINE__);
    cleanupThread(&publishToMasterserverThread);
  }
  if (publishToClientsThread) {
    // printf("LINE: %d\n",__LINE__);
    cleanupThread(&publishToClientsThread);
  }

  // printf("LINE: %d\n",__LINE__);

  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem, "In [%s::%s Line %d]\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__);

  cleanupMapPreviewTexture();

  if (factionVideo != NULL) {
    factionVideo->closePlayer();
    delete factionVideo;
    factionVideo = NULL;
  }

  if (forceWaitForShutdown == true) {
    NetworkManager::getInstance().end();
  }

  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem, "In [%s::%s Line %d]\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__);
}

MenuStateCustomGame::~MenuStateCustomGame() {
  if (SystemFlags::VERBOSE_MODE_ENABLED)
    printf("In [%s::%s Line: %d]\n",
           extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
           __LINE__);

  cleanup();

  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem, "In [%s::%s Line %d]\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__);
  if (SystemFlags::VERBOSE_MODE_ENABLED)
    printf("In [%s::%s Line: %d]\n",
           extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
           __LINE__);
}

void MenuStateCustomGame::returnToParentMenu() {
  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem, "In [%s::%s Line %d]\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__);

  needToBroadcastServerSettings = false;
  needToRepublishToMasterserver = false;
  lastNetworkPing = time(NULL);
  ParentMenuState parentMenuState = this->parentMenuState;

  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem, "In [%s::%s Line %d]\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__);

  forceWaitForShutdown = false;
  if (parentMenuState == pMasterServer) {
    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem,
                               "In [%s::%s Line %d]\n",
                               extractFileFromDirectoryPath(__FILE__).c_str(),
                               __FUNCTION__, __LINE__);
    cleanup();
    mainMenu->setState(new MenuStateMasterserver(program, mainMenu));
  } else if (parentMenuState == pLanGame) {
    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem,
                               "In [%s::%s Line %d]\n",
                               extractFileFromDirectoryPath(__FILE__).c_str(),
                               __FUNCTION__, __LINE__);
    cleanup();
    mainMenu->setState(new MenuStateJoinGame(program, mainMenu));
  } else {
    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem,
                               "In [%s::%s Line %d]\n",
                               extractFileFromDirectoryPath(__FILE__).c_str(),
                               __FUNCTION__, __LINE__);
    cleanup();
    mainMenu->setState(new MenuStateNewGame(program, mainMenu));
  }

  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem, "In [%s::%s Line %d]\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__);
}

void MenuStateCustomGame::mouseClick(int x, int y, MouseButton mouseButton) {
  if (isMasterserverMode() == true) {
    return;
  }
  Config &config = Config::getInstance();
  Lang &lang = Lang::getInstance();
  try {
    CoreData &coreData = CoreData::getInstance();
    SoundRenderer &soundRenderer = SoundRenderer::getInstance();
    int oldListBoxMapfilterIndex = listBoxMapFilter.getSelectedItemIndex();

    if (mainMessageBox.getEnabled()) {
      int button = 0;
      if (mainMessageBox.mouseClick(x, y, button)) {
        soundRenderer.playFx(coreData.getClickSoundA());
        if (button == 0) {
          mainMessageBox.setEnabled(false);
        }
      }
    } else if (comboBoxMap.mouseClick(x, y)) {
      if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
        SystemFlags::OutputDebug(SystemFlags::debugSystem, "%s\n",
                                 getCurrentMapFile().c_str());

      MutexSafeWrapper safeMutex(
          (publishToMasterserverThread != NULL
               ? publishToMasterserverThread->getMutexThreadObjectAccessor()
               : NULL),
          string(__FILE__) + "_" + intToStr(__LINE__));
      MutexSafeWrapper safeMutexCLI(
          (publishToClientsThread != NULL
               ? publishToClientsThread->getMutexThreadObjectAccessor()
               : NULL),
          string(__FILE__) + "_" + intToStr(__LINE__));

      loadMapInfo(Config::getMapPath(getCurrentMapFile(), "", false), &mapInfo,
                  true, true);
      labelMapInfo.setText(mapInfo.desc);
      updateControlers();
      updateNetworkSlots();

      if (checkBoxPublishServer.getValue() == true) {
        needToRepublishToMasterserver = true;
      }

      if (hasNetworkGameSettings() == true) {
        // delay publishing for 5 seconds
        needToPublishDelayed = true;
        mapPublishingDelayTimer = time(NULL);
      }
    } else if (comboBoxMap.isDropDownShowing()) {
      // do nothing
    } else if (comboBoxLoadSetup.mouseClick(x, y)) {
    } else if (comboBoxLoadSetup.isDropDownShowing()) {
      // do nothing
    } else {
      string advanceToItemStartingWith = "";
      if (::Shared::Platform::Window::isKeyStateModPressed(KMOD_SHIFT) ==
          true) {
        const wchar_t lastKey =
            ::Shared::Platform::Window::extractLastKeyPressed();
        //				string helpString = "";
        //				helpString = lastKey;
        //				printf("lastKey =
        //'%s'\n",helpString.c_str());
        advanceToItemStartingWith = lastKey;
      }

      if (mapPreviewTexture != NULL) {
        //        		printf("X: %d Y: %d      [%d, %d, %d, %d]\n",
        //        				x, y,
        //        				this->render_mapPreviewTexture_X,
        //        this->render_mapPreviewTexture_X +
        //        this->render_mapPreviewTexture_W,
        //        				this->render_mapPreviewTexture_Y,
        //        this->render_mapPreviewTexture_Y +
        //        this->render_mapPreviewTexture_H);

        if (x >= this->render_mapPreviewTexture_X &&
            x <= this->render_mapPreviewTexture_X +
                     this->render_mapPreviewTexture_W &&
            y >= this->render_mapPreviewTexture_Y &&
            y <= this->render_mapPreviewTexture_Y +
                     this->render_mapPreviewTexture_H) {

          if (this->render_mapPreviewTexture_X == mapPreviewTexture_X &&
              this->render_mapPreviewTexture_Y == mapPreviewTexture_Y &&
              this->render_mapPreviewTexture_W == mapPreviewTexture_W &&
              this->render_mapPreviewTexture_H == mapPreviewTexture_H) {

            const Metrics &metrics = Metrics::getInstance();

            this->render_mapPreviewTexture_X = 0;
            this->render_mapPreviewTexture_Y = 0;
            this->render_mapPreviewTexture_W = metrics.getVirtualW();
            this->render_mapPreviewTexture_H = metrics.getVirtualH();
            this->zoomedMap = true;

            cleanupMapPreviewTexture();
          } else {
            this->render_mapPreviewTexture_X = mapPreviewTexture_X;
            this->render_mapPreviewTexture_Y = mapPreviewTexture_Y;
            this->render_mapPreviewTexture_W = mapPreviewTexture_W;
            this->render_mapPreviewTexture_H = mapPreviewTexture_H;
            this->zoomedMap = false;

            cleanupMapPreviewTexture();
          }
          return;
        }
        if (this->zoomedMap == true) {
          return;
        }
      }

      if (activeInputLabel != NULL && !(activeInputLabel->mouseClick(x, y))) {
        setActiveInputLabel(NULL);
      }
      if (buttonReturn.mouseClick(x, y) || serverInitError == true) {
        if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
          SystemFlags::OutputDebug(
              SystemFlags::debugSystem, "In [%s::%s Line %d]\n",
              extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
              __LINE__);

        soundRenderer.playFx(coreData.getClickSoundA());

        MutexSafeWrapper safeMutex(
            (publishToMasterserverThread != NULL
                 ? publishToMasterserverThread->getMutexThreadObjectAccessor()
                 : NULL),
            string(__FILE__) + "_" + intToStr(__LINE__));
        MutexSafeWrapper safeMutexCLI(
            (publishToClientsThread != NULL
                 ? publishToClientsThread->getMutexThreadObjectAccessor()
                 : NULL),
            string(__FILE__) + "_" + intToStr(__LINE__));
        needToBroadcastServerSettings = false;
        needToRepublishToMasterserver = false;
        lastNetworkPing = time(NULL);
        safeMutex.ReleaseLock();
        safeMutexCLI.ReleaseLock();

        if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
          SystemFlags::OutputDebug(
              SystemFlags::debugSystem, "In [%s::%s Line %d]\n",
              extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
              __LINE__);

        returnToParentMenu();
        return;
      } else if (buttonPlayNow.mouseClick(x, y) && buttonPlayNow.getEnabled()) {
        if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
          SystemFlags::OutputDebug(
              SystemFlags::debugSystem, "In [%s::%s Line %d]\n",
              extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
              __LINE__);

        PlayNow(true);
        return;
      } else if (checkBoxAdvanced.getValue() == 1 &&
                 listBoxFogOfWar.mouseClick(x, y)) {
        MutexSafeWrapper safeMutex(
            (publishToMasterserverThread != NULL
                 ? publishToMasterserverThread->getMutexThreadObjectAccessor()
                 : NULL),
            string(__FILE__) + "_" + intToStr(__LINE__));
        MutexSafeWrapper safeMutexCLI(
            (publishToClientsThread != NULL
                 ? publishToClientsThread->getMutexThreadObjectAccessor()
                 : NULL),
            string(__FILE__) + "_" + intToStr(__LINE__));

        cleanupMapPreviewTexture();
        if (checkBoxPublishServer.getValue() == true) {
          needToRepublishToMasterserver = true;
        }

        if (hasNetworkGameSettings() == true) {
          needToSetChangedGameSettings = true;
          lastSetChangedGameSettings = time(NULL);
        }
      } else if (labelSaveSetupName.mouseClick(x, y)) {
        setActiveInputLabel(&labelSaveSetupName);
      } else if (buttonSaveSetup.mouseClick(x, y)) {
        GameSettings gameSettings;
        copyToGameSettings(&gameSettings);
        int humanSlots = 0;
        for (int i = 0; i < gameSettings.getFactionCount(); i++) {
          switch (gameSettings.getFactionControl(i)) {
          case ctNetwork:
          case ctHuman:
          case ctNetworkUnassigned:
            humanSlots++;
            break;
          default:
            // do nothing
            break;
          }
        }
        string setupName = intToStr(humanSlots) + "_" + gameSettings.getMap();
        labelSaveSetupName.setText(trim(labelSaveSetupName.getText()));
        if (labelSaveSetupName.getText() != "") {
          setupName = labelSaveSetupName.getText();
          setupName = replaceAll(setupName, "/", "_");
          setupName = replaceAll(setupName, "\\", "_");
          if (StartsWith(setupName, ".")) {
            setupName[0] = '_';
          }
        }
        if (setupName != lang.getString(LAST_SETUP_STRING)) {
          string filename = SETUPS_DIR + setupName + ".mgg";
          saveGameSettings(filename);
          console.addLine("--> " + filename);
          loadSavedSetupNames();
          comboBoxLoadSetup.setItems(savedSetupFilenames);
          comboBoxLoadSetup.setSelectedItem(setupName);
        }
      } else if (buttonLoadSetup.mouseClick(x, y)) {
        string setupName = comboBoxLoadSetup.getSelectedItem();
        if (setupName != "") {
          string fileNameToLoad = SETUPS_DIR + setupName + ".mgg";
          if (setupName == lang.getString(LAST_SETUP_STRING)) {
            fileNameToLoad = SAVED_SETUP_FILENAME;
          }
          if (loadGameSettings(fileNameToLoad))
            console.addLine("<-- " + fileNameToLoad);
        }
      } else if (buttonDeleteSetup.mouseClick(x, y)) {
        string setupName = comboBoxLoadSetup.getSelectedItem();
        if (setupName != "" && setupName != lang.getString(LAST_SETUP_STRING)) {
          int index = comboBoxLoadSetup.getSelectedItemIndex();
          removeFile(savedSetupsDir + setupName + ".mgg");
          loadSavedSetupNames();
          comboBoxLoadSetup.setItems(savedSetupFilenames);
          if (comboBoxLoadSetup.getItemCount() > index) {
            comboBoxLoadSetup.setSelectedItemIndex(index, false);
          } else {
            comboBoxLoadSetup.setSelectedItem(lang.getString(LAST_SETUP_STRING),
                                              false);
          }
          console.addLine("X " + setupName + ".mgg");
        }
      } else if (checkBoxAdvanced.getValue() == 1 &&
                 buttonShowLanInfo.mouseClick(x, y)) {
        // show to console
        string ipText = "none";
        std::vector<std::string> ipList = Socket::getLocalIPAddressList();
        if (ipList.empty() == false) {
          ipText = "";
          for (int idx = 0; idx < (int)ipList.size(); idx++) {
            string ip = ipList[idx];
            if (ipText != "") {
              ipText += ", ";
            }
            ipText += ip;
          }
        }

        string serverPort = config.getString(
            "PortServer", intToStr(GameConstants::serverPort).c_str());
        string externalPort =
            config.getString("PortExternal", serverPort.c_str());
        console.addLine(lang.getString("LanIP") + ipText + "  ( " + serverPort +
                        " / " + externalPort + " )");
      } else if (checkBoxAdvanced.getValue() == 1 &&
                 checkBoxAllowObservers.mouseClick(x, y)) {
        MutexSafeWrapper safeMutex(
            (publishToMasterserverThread != NULL
                 ? publishToMasterserverThread->getMutexThreadObjectAccessor()
                 : NULL),
            string(__FILE__) + "_" + intToStr(__LINE__));
        MutexSafeWrapper safeMutexCLI(
            (publishToClientsThread != NULL
                 ? publishToClientsThread->getMutexThreadObjectAccessor()
                 : NULL),
            string(__FILE__) + "_" + intToStr(__LINE__));

        if (checkBoxPublishServer.getValue() == true) {
          needToRepublishToMasterserver = true;
        }

        reloadFactions(
            true, (checkBoxScenario.getValue() == true
                       ? scenarioFiles[listBoxScenario.getSelectedItemIndex()]
                       : ""));

        if (hasNetworkGameSettings() == true) {
          needToSetChangedGameSettings = true;
          lastSetChangedGameSettings = time(NULL);
        }
      } else if (checkBoxAllowInGameJoinPlayer.mouseClick(x, y)) {
        MutexSafeWrapper safeMutex(
            (publishToMasterserverThread != NULL
                 ? publishToMasterserverThread->getMutexThreadObjectAccessor()
                 : NULL),
            string(__FILE__) + "_" + intToStr(__LINE__));
        MutexSafeWrapper safeMutexCLI(
            (publishToClientsThread != NULL
                 ? publishToClientsThread->getMutexThreadObjectAccessor()
                 : NULL),
            string(__FILE__) + "_" + intToStr(__LINE__));

        if (checkBoxPublishServer.getValue() == true) {
          needToRepublishToMasterserver = true;
        }

        if (hasNetworkGameSettings() == true) {
          needToSetChangedGameSettings = true;
          lastSetChangedGameSettings = time(NULL);
        }

        ServerInterface *serverInterface =
            NetworkManager::getInstance().getServerInterface();
        serverInterface->setAllowInGameConnections(
            checkBoxAllowInGameJoinPlayer.getValue() == true);
      } else if (checkBoxAdvanced.getValue() == 1 &&
                 checkBoxAllowTeamUnitSharing.mouseClick(x, y)) {
        MutexSafeWrapper safeMutex(
            (publishToMasterserverThread != NULL
                 ? publishToMasterserverThread->getMutexThreadObjectAccessor()
                 : NULL),
            string(__FILE__) + "_" + intToStr(__LINE__));
        MutexSafeWrapper safeMutexCLI(
            (publishToClientsThread != NULL
                 ? publishToClientsThread->getMutexThreadObjectAccessor()
                 : NULL),
            string(__FILE__) + "_" + intToStr(__LINE__));

        if (checkBoxPublishServer.getValue() == true) {
          needToRepublishToMasterserver = true;
        }

        if (hasNetworkGameSettings() == true) {
          needToSetChangedGameSettings = true;
          lastSetChangedGameSettings = time(NULL);
        }
      } else if (checkBoxAdvanced.getValue() == 1 &&
                 checkBoxAllowTeamResourceSharing.mouseClick(x, y)) {
        MutexSafeWrapper safeMutex(
            (publishToMasterserverThread != NULL
                 ? publishToMasterserverThread->getMutexThreadObjectAccessor()
                 : NULL),
            string(__FILE__) + "_" + intToStr(__LINE__));
        MutexSafeWrapper safeMutexCLI(
            (publishToClientsThread != NULL
                 ? publishToClientsThread->getMutexThreadObjectAccessor()
                 : NULL),
            string(__FILE__) + "_" + intToStr(__LINE__));

        if (checkBoxPublishServer.getValue() == true) {
          needToRepublishToMasterserver = true;
        }

        if (hasNetworkGameSettings() == true) {
          needToSetChangedGameSettings = true;
          lastSetChangedGameSettings = time(NULL);
        }
      } else if (checkBoxAllowNativeLanguageTechtree.mouseClick(x, y)) {
        MutexSafeWrapper safeMutex(
            (publishToMasterserverThread != NULL
                 ? publishToMasterserverThread->getMutexThreadObjectAccessor()
                 : NULL),
            string(__FILE__) + "_" + intToStr(__LINE__));
        MutexSafeWrapper safeMutexCLI(
            (publishToClientsThread != NULL
                 ? publishToClientsThread->getMutexThreadObjectAccessor()
                 : NULL),
            string(__FILE__) + "_" + intToStr(__LINE__));

        if (checkBoxPublishServer.getValue() == true) {
          needToRepublishToMasterserver = true;
        }

        if (hasNetworkGameSettings() == true) {
          needToSetChangedGameSettings = true;
          lastSetChangedGameSettings = time(NULL);
        }
      } else if (checkBoxAdvanced.getValue() == 1 &&
                 checkBoxEnableSwitchTeamMode.mouseClick(x, y)) {
        MutexSafeWrapper safeMutex(
            (publishToMasterserverThread != NULL
                 ? publishToMasterserverThread->getMutexThreadObjectAccessor()
                 : NULL),
            string(__FILE__) + "_" + intToStr(__LINE__));
        MutexSafeWrapper safeMutexCLI(
            (publishToClientsThread != NULL
                 ? publishToClientsThread->getMutexThreadObjectAccessor()
                 : NULL),
            string(__FILE__) + "_" + intToStr(__LINE__));

        if (checkBoxPublishServer.getValue() == true) {
          needToRepublishToMasterserver = true;
        }

        if (hasNetworkGameSettings() == true) {
          needToSetChangedGameSettings = true;
          lastSetChangedGameSettings = time(NULL);
        }
      } else if (checkBoxAdvanced.getValue() == 1 &&
                 listBoxAISwitchTeamAcceptPercent.getEnabled() &&
                 listBoxAISwitchTeamAcceptPercent.mouseClick(x, y)) {
        MutexSafeWrapper safeMutex(
            (publishToMasterserverThread != NULL
                 ? publishToMasterserverThread->getMutexThreadObjectAccessor()
                 : NULL),
            string(__FILE__) + "_" + intToStr(__LINE__));
        MutexSafeWrapper safeMutexCLI(
            (publishToClientsThread != NULL
                 ? publishToClientsThread->getMutexThreadObjectAccessor()
                 : NULL),
            string(__FILE__) + "_" + intToStr(__LINE__));

        if (checkBoxPublishServer.getValue() == true) {
          needToRepublishToMasterserver = true;
        }

        if (hasNetworkGameSettings() == true) {
          needToSetChangedGameSettings = true;
          lastSetChangedGameSettings = time(NULL);
        }
      } else if (checkBoxAdvanced.getValue() == 1 &&
                 listBoxFallbackCpuMultiplier.getEditable() == true &&
                 listBoxFallbackCpuMultiplier.mouseClick(x, y)) {
        MutexSafeWrapper safeMutex(
            (publishToMasterserverThread != NULL
                 ? publishToMasterserverThread->getMutexThreadObjectAccessor()
                 : NULL),
            string(__FILE__) + "_" + intToStr(__LINE__));
        MutexSafeWrapper safeMutexCLI(
            (publishToClientsThread != NULL
                 ? publishToClientsThread->getMutexThreadObjectAccessor()
                 : NULL),
            string(__FILE__) + "_" + intToStr(__LINE__));

        if (checkBoxPublishServer.getValue() == true) {
          needToRepublishToMasterserver = true;
        }

        if (hasNetworkGameSettings() == true) {
          needToSetChangedGameSettings = true;
          lastSetChangedGameSettings = time(NULL);
        }
      } else if (checkBoxAdvanced.mouseClick(x, y)) {
      } else if (listBoxTileset.mouseClick(x, y, advanceToItemStartingWith)) {
        MutexSafeWrapper safeMutex(
            (publishToMasterserverThread != NULL
                 ? publishToMasterserverThread->getMutexThreadObjectAccessor()
                 : NULL),
            string(__FILE__) + "_" + intToStr(__LINE__));
        MutexSafeWrapper safeMutexCLI(
            (publishToClientsThread != NULL
                 ? publishToClientsThread->getMutexThreadObjectAccessor()
                 : NULL),
            string(__FILE__) + "_" + intToStr(__LINE__));

        if (checkBoxPublishServer.getValue() == true) {
          needToRepublishToMasterserver = true;
        }
        if (hasNetworkGameSettings() == true) {

          // delay publishing for 5 seconds
          needToPublishDelayed = true;
          mapPublishingDelayTimer = time(NULL);
        }
      } else if (listBoxMapFilter.mouseClick(x, y)) {
        MutexSafeWrapper safeMutex(
            (publishToMasterserverThread != NULL
                 ? publishToMasterserverThread->getMutexThreadObjectAccessor()
                 : NULL),
            string(__FILE__) + "_" + intToStr(__LINE__));
        MutexSafeWrapper safeMutexCLI(
            (publishToClientsThread != NULL
                 ? publishToClientsThread->getMutexThreadObjectAccessor()
                 : NULL),
            string(__FILE__) + "_" + intToStr(__LINE__));

        switchToNextMapGroup(listBoxMapFilter.getSelectedItemIndex() -
                             oldListBoxMapfilterIndex);

        if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
          SystemFlags::OutputDebug(SystemFlags::debugSystem, "%s\n",
                                   getCurrentMapFile().c_str());

        loadMapInfo(Config::getMapPath(getCurrentMapFile()), &mapInfo, true,
                    true);
        labelMapInfo.setText(mapInfo.desc);
        updateControlers();
        updateNetworkSlots();

        if (checkBoxPublishServer.getValue() == true) {
          needToRepublishToMasterserver = true;
        }

        if (hasNetworkGameSettings() == true) {
          needToSetChangedGameSettings = true;
          lastSetChangedGameSettings = time(NULL);
        }
      } else if (listBoxTechTree.mouseClick(x, y, advanceToItemStartingWith)) {
        reloadFactions(
            listBoxTechTree.getItemCount() <= 1,
            (checkBoxScenario.getValue() == true
                 ? scenarioFiles[listBoxScenario.getSelectedItemIndex()]
                 : ""));

        MutexSafeWrapper safeMutex(
            (publishToMasterserverThread != NULL
                 ? publishToMasterserverThread->getMutexThreadObjectAccessor()
                 : NULL),
            string(__FILE__) + "_" + intToStr(__LINE__));
        MutexSafeWrapper safeMutexCLI(
            (publishToClientsThread != NULL
                 ? publishToClientsThread->getMutexThreadObjectAccessor()
                 : NULL),
            string(__FILE__) + "_" + intToStr(__LINE__));

        if (checkBoxPublishServer.getValue() == true) {
          needToRepublishToMasterserver = true;
        }

        if (hasNetworkGameSettings() == true) {
          needToSetChangedGameSettings = true;
          lastSetChangedGameSettings = time(NULL);
        }
      } else if (checkBoxPublishServer.mouseClick(x, y) &&
                 checkBoxPublishServer.getEditable()) {
        MutexSafeWrapper safeMutex(
            (publishToMasterserverThread != NULL
                 ? publishToMasterserverThread->getMutexThreadObjectAccessor()
                 : NULL),
            string(__FILE__) + "_" + intToStr(__LINE__));
        MutexSafeWrapper safeMutexCLI(
            (publishToClientsThread != NULL
                 ? publishToClientsThread->getMutexThreadObjectAccessor()
                 : NULL),
            string(__FILE__) + "_" + intToStr(__LINE__));

        needToRepublishToMasterserver = true;
        soundRenderer.playFx(coreData.getClickSoundC());

        ServerInterface *serverInterface =
            NetworkManager::getInstance().getServerInterface();
        serverInterface->setPublishEnabled(checkBoxPublishServer.getValue() ==
                                           true);
      } else if (labelGameName.mouseClick(x, y) &&
                 checkBoxPublishServer.getEditable()) {
        setActiveInputLabel(&labelGameName);
      } else if (checkBoxAdvanced.getValue() == 1 &&
                 checkBoxNetworkPauseGameForLaggedClients.mouseClick(x, y)) {
        MutexSafeWrapper safeMutex(
            (publishToMasterserverThread != NULL
                 ? publishToMasterserverThread->getMutexThreadObjectAccessor()
                 : NULL),
            string(__FILE__) + "_" + intToStr(__LINE__));
        MutexSafeWrapper safeMutexCLI(
            (publishToClientsThread != NULL
                 ? publishToClientsThread->getMutexThreadObjectAccessor()
                 : NULL),
            string(__FILE__) + "_" + intToStr(__LINE__));

        if (checkBoxPublishServer.getValue() == true) {
          needToRepublishToMasterserver = true;
        }
        if (hasNetworkGameSettings() == true) {
          needToSetChangedGameSettings = true;
          lastSetChangedGameSettings = time(NULL);
        }

        soundRenderer.playFx(coreData.getClickSoundC());
      } else if (listBoxScenario.mouseClick(x, y) ||
                 checkBoxScenario.mouseClick(x, y)) {
        processScenario();
      } else {
        for (int i = 0; i < mapInfo.players; ++i) {
          MutexSafeWrapper safeMutex(
              (publishToMasterserverThread != NULL
                   ? publishToMasterserverThread->getMutexThreadObjectAccessor()
                   : NULL),
              string(__FILE__) + "_" + intToStr(__LINE__));
          MutexSafeWrapper safeMutexCLI(
              (publishToClientsThread != NULL
                   ? publishToClientsThread->getMutexThreadObjectAccessor()
                   : NULL),
              string(__FILE__) + "_" + intToStr(__LINE__));

          // set multiplier
          if (listBoxRMultiplier[i].mouseClick(x, y)) {
            // printf("Line: %d multiplier index: %d i: %d itemcount:
            // %d\n",__LINE__,listBoxRMultiplier[i].getSelectedItemIndex(),i,listBoxRMultiplier[i].getItemCount());

            // for(int indexData = 0; indexData <
            // listBoxRMultiplier[i].getItemCount(); ++indexData) { string item
            // = listBoxRMultiplier[i].getItem(indexData);

            // printf("Item index: %d value: %s\n",indexData,item.c_str());
            //}
          }

          // ensure thet only 1 human player is present
          ServerInterface *serverInterface =
              NetworkManager::getInstance().getServerInterface();
          ConnectionSlot *slot = serverInterface->getSlot(i, true);

          bool checkControTypeClicked = false;
          int selectedControlItemIndex =
              listBoxControls[i].getSelectedItemIndex();
          if (selectedControlItemIndex != ctNetwork ||
              (selectedControlItemIndex == ctNetwork &&
               (slot == NULL || slot->isConnected() == false))) {
            checkControTypeClicked = true;
          }

          // printf("checkControTypeClicked = %d selectedControlItemIndex = %d i
          // = %d\n",checkControTypeClicked,selectedControlItemIndex,i);

          if (selectedControlItemIndex != ctHuman &&
              checkControTypeClicked == true &&
              listBoxControls[i].mouseClick(x, y)) {
            if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem)
                    .enabled)
              SystemFlags::OutputDebug(
                  SystemFlags::debugSystem, "In [%s::%s Line %d]\n",
                  extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                  __LINE__);

            ControlType currentControlType = static_cast<ControlType>(
                listBoxControls[i].getSelectedItemIndex());
            int slotsToChangeStart = i;
            int slotsToChangeEnd = i;
            // If control is pressed while changing player types then change all
            // other slots to same type
            if (::Shared::Platform::Window::isKeyStateModPressed(KMOD_CTRL) ==
                    true &&
                currentControlType != ctHuman) {
              slotsToChangeStart = 0;
              slotsToChangeEnd = mapInfo.players - 1;
            }

            for (int index = slotsToChangeStart; index <= slotsToChangeEnd;
                 ++index) {
              if (index != i &&
                  static_cast<ControlType>(
                      listBoxControls[index].getSelectedItemIndex()) !=
                      ctHuman) {
                listBoxControls[index].setSelectedItemIndex(
                    listBoxControls[i].getSelectedItemIndex());
              }
              // Skip over networkunassigned
              if (listBoxControls[index].getSelectedItemIndex() ==
                      ctNetworkUnassigned &&
                  selectedControlItemIndex != ctNetworkUnassigned) {
                listBoxControls[index].mouseClick(x, y);
              }

              // look for human players
              int humanIndex1 = -1;
              int humanIndex2 = -1;
              for (int j = 0; j < GameConstants::maxPlayers; ++j) {
                ControlType ct = static_cast<ControlType>(
                    listBoxControls[j].getSelectedItemIndex());
                if (ct == ctHuman) {
                  if (humanIndex1 == -1) {
                    humanIndex1 = j;
                  } else {
                    humanIndex2 = j;
                  }
                }
              }

              if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem)
                      .enabled)
                SystemFlags::OutputDebug(
                    SystemFlags::debugSystem,
                    "In [%s::%s Line %d] humanIndex1 = %d, humanIndex2 = %d\n",
                    extractFileFromDirectoryPath(__FILE__).c_str(),
                    __FUNCTION__, __LINE__, humanIndex1, humanIndex2);

              // no human
              if (humanIndex1 == -1 && humanIndex2 == -1) {
                setSlotHuman(index);
                if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem)
                        .enabled)
                  SystemFlags::OutputDebug(
                      SystemFlags::debugSystem,
                      "In [%s::%s Line %d] i = %d, "
                      "labelPlayerNames[i].getText() [%s]\n",
                      extractFileFromDirectoryPath(__FILE__).c_str(),
                      __FUNCTION__, __LINE__, index,
                      labelPlayerNames[index].getText().c_str());

                // printf("humanIndex1 = %d humanIndex2 = %d i = %d
                // listBoxControls[i].getSelectedItemIndex() =
                // %d\n",humanIndex1,humanIndex2,i,listBoxControls[i].getSelectedItemIndex());
              }
              // 2 humans
              else if (humanIndex1 != -1 && humanIndex2 != -1) {
                int closeSlotIndex =
                    (humanIndex1 == index ? humanIndex2 : humanIndex1);
                int humanSlotIndex =
                    (closeSlotIndex == humanIndex1 ? humanIndex2 : humanIndex1);

                string origPlayName =
                    labelPlayerNames[closeSlotIndex].getText();

                if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem)
                        .enabled)
                  SystemFlags::OutputDebug(
                      SystemFlags::debugSystem,
                      "In [%s::%s Line %d] closeSlotIndex = %d, origPlayName "
                      "[%s]\n",
                      extractFileFromDirectoryPath(__FILE__).c_str(),
                      __FUNCTION__, __LINE__, closeSlotIndex,
                      origPlayName.c_str());

                listBoxControls[closeSlotIndex].setSelectedItemIndex(ctClosed);
                setSlotHuman(humanSlotIndex);
                labelPlayerNames[humanSlotIndex].setText(
                    (origPlayName != "" ? origPlayName : getHumanPlayerName()));
              }
              updateNetworkSlots();

              if (checkBoxPublishServer.getValue() == true) {
                needToRepublishToMasterserver = true;
              }

              if (hasNetworkGameSettings() == true) {
                needToSetChangedGameSettings = true;
                lastSetChangedGameSettings = time(NULL);
              }
              updateResourceMultiplier(index);
            }
          } else if (buttonClearBlockedPlayers.mouseClick(x, y)) {
            soundRenderer.playFx(coreData.getClickSoundB());

            ServerInterface *serverInterface =
                NetworkManager::getInstance().getServerInterface();
            if (serverInterface != NULL) {
              ServerSocket *serverSocket = serverInterface->getServerSocket();
              if (serverSocket != NULL) {
                serverSocket->clearBlockedIPAddress();
              }
            }
          } else if (buttonBlockPlayers[i].mouseClick(x, y)) {
            soundRenderer.playFx(coreData.getClickSoundB());

            ServerInterface *serverInterface =
                NetworkManager::getInstance().getServerInterface();
            if (serverInterface != NULL) {
              if (serverInterface->getSlot(i, true) != NULL &&
                  serverInterface->getSlot(i, true)->isConnected()) {

                ServerSocket *serverSocket = serverInterface->getServerSocket();
                if (serverSocket != NULL) {
                  serverSocket->addIPAddressToBlockedList(
                      serverInterface->getSlot(i, true)->getIpAddress());

                  const vector<string> languageList =
                      serverInterface->getGameSettings()
                          ->getUniqueNetworkPlayerLanguages();
                  for (unsigned int j = 0; j < languageList.size(); ++j) {
                    char szMsg[8096] = "";
                    if (lang.hasString("BlockPlayerServerMsg",
                                       languageList[j]) == true) {
                      snprintf(szMsg, 8096,
                               lang.getString("BlockPlayerServerMsg",
                                              languageList[j])
                                   .c_str(),
                               serverInterface->getSlot(i, true)
                                   ->getIpAddress()
                                   .c_str());
                    } else {
                      snprintf(szMsg, 8096,
                               "The server has temporarily blocked IP Address "
                               "[%s] from this game.",
                               serverInterface->getSlot(i, true)
                                   ->getIpAddress()
                                   .c_str());
                    }

                    serverInterface->sendTextMessage(szMsg, -1, true,
                                                     languageList[j]);
                  }
                  sleep(1);
                  serverInterface->getSlot(i, true)->close();
                }
              }
            }
          } else if (listBoxFactions[i].mouseClick(x, y,
                                                   advanceToItemStartingWith)) {
            // Disallow CPU players to be observers
            if (factionFiles[listBoxFactions[i].getSelectedItemIndex()] ==
                    formatString(GameConstants::OBSERVER_SLOTNAME) &&
                (listBoxControls[i].getSelectedItemIndex() == ctCpuEasy ||
                 listBoxControls[i].getSelectedItemIndex() == ctCpu ||
                 listBoxControls[i].getSelectedItemIndex() == ctCpuUltra ||
                 listBoxControls[i].getSelectedItemIndex() == ctCpuMega)) {
              listBoxFactions[i].setSelectedItemIndex(0);
            }
            //

            if (checkBoxPublishServer.getValue() == true) {
              needToRepublishToMasterserver = true;
            }

            if (hasNetworkGameSettings() == true) {
              needToSetChangedGameSettings = true;
              lastSetChangedGameSettings = time(NULL);
            }
          } else if (listBoxTeams[i].mouseClick(x, y)) {
            if (factionFiles[listBoxFactions[i].getSelectedItemIndex()] !=
                formatString(GameConstants::OBSERVER_SLOTNAME)) {
              if (listBoxTeams[i].getSelectedItemIndex() + 1 !=
                  (GameConstants::maxPlayers + fpt_Observer)) {
                lastSelectedTeamIndex[i] =
                    listBoxTeams[i].getSelectedItemIndex();
              }
            } else {
              lastSelectedTeamIndex[i] = -1;
            }

            if (checkBoxPublishServer.getValue() == true) {
              needToRepublishToMasterserver = true;
            }

            if (hasNetworkGameSettings() == true) {
              needToSetChangedGameSettings = true;
              lastSetChangedGameSettings = time(NULL);
              ;
            }
          } else if (labelPlayerNames[i].mouseClick(x, y)) {
            ControlType ct = static_cast<ControlType>(
                listBoxControls[i].getSelectedItemIndex());
            if (ct == ctHuman) {
              setActiveInputLabel(&labelPlayerNames[i]);
              break;
            }
          }
        }
      }
    }

    if (hasNetworkGameSettings() == true &&
        listBoxPlayerStatus.mouseClick(x, y)) {
      if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
        SystemFlags::OutputDebug(SystemFlags::debugSystem,
                                 "In [%s::%s Line %d]\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__);

      soundRenderer.playFx(coreData.getClickSoundC());
      if (getNetworkPlayerStatus() == npst_PickSettings) {
        listBoxPlayerStatus.setTextColor(Vec3f(1.0f, 0.0f, 0.0f));
        listBoxPlayerStatus.setLighted(true);
      } else if (getNetworkPlayerStatus() == npst_BeRightBack) {
        listBoxPlayerStatus.setTextColor(Vec3f(1.0f, 1.0f, 0.0f));
        listBoxPlayerStatus.setLighted(true);
      } else if (getNetworkPlayerStatus() == npst_Ready) {
        listBoxPlayerStatus.setTextColor(Vec3f(0.0f, 1.0f, 0.0f));
        listBoxPlayerStatus.setLighted(false);
      }

      if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
        SystemFlags::OutputDebug(SystemFlags::debugSystem,
                                 "In [%s::%s Line %d]\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__);

      if (checkBoxPublishServer.getValue() == true) {
        needToRepublishToMasterserver = true;
      }

      if (hasNetworkGameSettings() == true) {
        needToSetChangedGameSettings = true;
        lastSetChangedGameSettings = time(NULL);
      }
    }
  } catch (const std::exception &ex) {
    char szBuf[8096] = "";
    snprintf(szBuf, 8096, "In [%s::%s %d]\nError detected:\n%s\n",
             extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
             __LINE__, ex.what());
    SystemFlags::OutputDebug(SystemFlags::debugError, szBuf);
    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem, "%s", szBuf);

    showGeneralError = true;
    generalErrorToShow = szBuf;
  }

  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem, "In [%s::%s Line %d]\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__);
}

void MenuStateCustomGame::updateAllResourceMultiplier() {
  for (int j = 0; j < GameConstants::maxPlayers; ++j) {
    updateResourceMultiplier(j);
  }
}

void MenuStateCustomGame::updateResourceMultiplier(const int index) {
  // printf("Line: %d multiplier index: %d index:
  // %d\n",__LINE__,listBoxRMultiplier[index].getSelectedItemIndex(),index);

  ControlType ct =
      static_cast<ControlType>(listBoxControls[index].getSelectedItemIndex());
  if (ct == ctCpuEasy || ct == ctNetworkCpuEasy) {
    listBoxRMultiplier[index].setSelectedItem(
        floatToStr(GameConstants::easyMultiplier, 1));
    listBoxRMultiplier[index].setEnabled(checkBoxScenario.getValue() == false);
  } else if (ct == ctCpu || ct == ctNetworkCpu) {
    listBoxRMultiplier[index].setSelectedItem(
        floatToStr(GameConstants::normalMultiplier, 1));
    listBoxRMultiplier[index].setEnabled(checkBoxScenario.getValue() == false);
  } else if (ct == ctCpuUltra || ct == ctNetworkCpuUltra) {
    listBoxRMultiplier[index].setSelectedItem(
        floatToStr(GameConstants::ultraMultiplier, 1));
    listBoxRMultiplier[index].setEnabled(checkBoxScenario.getValue() == false);
  } else if (ct == ctCpuMega || ct == ctNetworkCpuMega) {
    listBoxRMultiplier[index].setSelectedItem(
        floatToStr(GameConstants::megaMultiplier, 1));
    listBoxRMultiplier[index].setEnabled(checkBoxScenario.getValue() == false);
  }
  // if(ct == ctHuman || ct == ctNetwork || ct == ctClosed) {
  else {
    listBoxRMultiplier[index].setSelectedItem(
        floatToStr(GameConstants::normalMultiplier, 1));
    listBoxRMultiplier[index].setEnabled(false);
    //!!!listBoxRMultiplier[index].setEnabled(checkBoxScenario.getValue() ==
    //! false);
  }

  listBoxRMultiplier[index].setEditable(listBoxRMultiplier[index].getEnabled());
  listBoxRMultiplier[index].setVisible(ct != ctHuman && ct != ctNetwork &&
                                       ct != ctClosed);
  // listBoxRMultiplier[index].setVisible(ct != ctClosed);

  // printf("Line: %d multiplier index: %d index:
  // %d\n",__LINE__,listBoxRMultiplier[index].getSelectedItemIndex(),index);
}

bool MenuStateCustomGame::loadGameSettings(const std::string &fileName) {
  // Ensure we have set the gamesettings at least once
  GameSettings gameSettings;
  bool result = loadGameSettingsFromFile(&gameSettings, fileName);
  if (result == false)
    return false;
  if (gameSettings.getMap() == "") {
    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem,
                               "In [%s::%s Line %d]\n",
                               extractFileFromDirectoryPath(__FILE__).c_str(),
                               __FUNCTION__, __LINE__);

    copyToGameSettings(&gameSettings);
  }

  ServerInterface *serverInterface =
      NetworkManager::getInstance().getServerInterface();
  serverInterface->setGameSettings(&gameSettings, false);

  MutexSafeWrapper safeMutex(
      (publishToMasterserverThread != NULL
           ? publishToMasterserverThread->getMutexThreadObjectAccessor()
           : NULL),
      string(__FILE__) + "_" + intToStr(__LINE__));
  MutexSafeWrapper safeMutexCLI(
      (publishToClientsThread != NULL
           ? publishToClientsThread->getMutexThreadObjectAccessor()
           : NULL),
      string(__FILE__) + "_" + intToStr(__LINE__));

  if (checkBoxPublishServer.getValue() == true) {
    needToRepublishToMasterserver = true;
  }

  if (hasNetworkGameSettings() == true) {
    needToSetChangedGameSettings = true;
    lastSetChangedGameSettings = time(NULL);
  }
  return true;
}

void MenuStateCustomGame::RestoreLastGameSettings() {
  loadGameSettings(SAVED_SETUP_FILENAME);
}

bool MenuStateCustomGame::checkNetworkPlayerDataSynch(bool checkMapCRC,
                                                      bool checkTileSetCRC,
                                                      bool checkTechTreeCRC) {
  ServerInterface *serverInterface =
      NetworkManager::getInstance().getServerInterface();

  bool dataSynchCheckOk = true;
  for (int i = 0; i < mapInfo.players; ++i) {
    if (listBoxControls[i].getSelectedItemIndex() == ctNetwork) {

      MutexSafeWrapper safeMutex(serverInterface->getSlotMutex(i),
                                 CODE_AT_LINE);
      ConnectionSlot *slot = serverInterface->getSlot(i, false);
      if (slot != NULL && slot->isConnected() &&
          (slot->getAllowDownloadDataSynch() == true ||
           slot->getAllowGameDataSynchCheck() == true)) {

        if (checkMapCRC == true &&
            slot->getNetworkGameDataSynchCheckOkMap() == false) {
          dataSynchCheckOk = false;
          break;
        }
        if (checkTileSetCRC == true &&
            slot->getNetworkGameDataSynchCheckOkTile() == false) {
          dataSynchCheckOk = false;
          break;
        }
        if (checkTechTreeCRC == true &&
            slot->getNetworkGameDataSynchCheckOkTech() == false) {
          dataSynchCheckOk = false;
          break;
        }
      }
    }
  }

  return dataSynchCheckOk;
}

void MenuStateCustomGame::PlayNow(bool saveGame) {
  if (listBoxTechTree.getItemCount() <= 0) {
    mainMessageBoxState = 1;

    char szMsg[8096] = "";
    strcpy(szMsg, "Cannot start game.\nThere are no tech-trees!\n");
    printf("%s", szMsg);

    showMessageBox(szMsg, "", false);
    return;
  }

  MutexSafeWrapper safeMutex(
      (publishToMasterserverThread != NULL
           ? publishToMasterserverThread->getMutexThreadObjectAccessor()
           : NULL),
      string(__FILE__) + "_" + intToStr(__LINE__));
  MutexSafeWrapper safeMutexCLI(
      (publishToClientsThread != NULL
           ? publishToClientsThread->getMutexThreadObjectAccessor()
           : NULL),
      string(__FILE__) + "_" + intToStr(__LINE__));

  if (saveGame == true) {
    saveGameSettings(SAVED_SETUP_FILENAME);
  }

  forceWaitForShutdown = false;
  closeUnusedSlots();
  CoreData &coreData = CoreData::getInstance();
  SoundRenderer &soundRenderer = SoundRenderer::getInstance();
  soundRenderer.playFx(coreData.getClickSoundC());

  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem, "In [%s::%s Line %d]\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__);

  std::vector<string> randomFactionSelectionList;
  int RandomCount = 0;
  for (int i = 0; i < mapInfo.players; ++i) {
    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem,
                               "In [%s::%s Line %d]\n",
                               extractFileFromDirectoryPath(__FILE__).c_str(),
                               __FUNCTION__, __LINE__);

    // Check for random faction selection and choose the faction now
    if (listBoxControls[i].getSelectedItemIndex() != ctClosed) {
      if (listBoxFactions[i].getSelectedItem() ==
              formatString(GameConstants::RANDOMFACTION_SLOTNAME) &&
          listBoxFactions[i].getItemCount() > 1) {
        if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
          SystemFlags::OutputDebug(
              SystemFlags::debugSystem, "In [%s::%s Line %d] i = %d\n",
              extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
              __LINE__, i);

        // Max 1000 tries to get a random, unused faction
        for (int findRandomFaction = 1; findRandomFaction < 1000;
             ++findRandomFaction) {
          Chrono seed(true);
          srand((unsigned int)seed.getCurTicks() + findRandomFaction);

          int selectedFactionIndex = rand() % listBoxFactions[i].getItemCount();
          string selectedFactionName =
              listBoxFactions[i].getItem(selectedFactionIndex);

          if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem)
                  .enabled)
            SystemFlags::OutputDebug(
                SystemFlags::debugSystem,
                "In [%s::%s Line %d] selectedFactionName [%s] "
                "selectedFactionIndex = %d, findRandomFaction = %d\n",
                extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                __LINE__, selectedFactionName.c_str(), selectedFactionIndex,
                findRandomFaction);

          if (selectedFactionName !=
                  formatString(GameConstants::RANDOMFACTION_SLOTNAME) &&
              selectedFactionName !=
                  formatString(GameConstants::OBSERVER_SLOTNAME) &&
              std::find(randomFactionSelectionList.begin(),
                        randomFactionSelectionList.end(),
                        selectedFactionName) ==
                  randomFactionSelectionList.end()) {

            if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem)
                    .enabled)
              SystemFlags::OutputDebug(
                  SystemFlags::debugSystem, "In [%s::%s Line %d]\n",
                  extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                  __LINE__);
            listBoxFactions[i].setSelectedItem(selectedFactionName);
            randomFactionSelectionList.push_back(selectedFactionName);
            break;
          }
        }

        if (listBoxFactions[i].getSelectedItem() ==
            formatString(GameConstants::RANDOMFACTION_SLOTNAME)) {
          if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem)
                  .enabled)
            SystemFlags::OutputDebug(
                SystemFlags::debugSystem,
                "In [%s::%s Line %d] RandomCount = %d\n",
                extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                __LINE__, RandomCount);

          // Find first real faction and use it
          int factionIndexToUse = RandomCount;
          for (int useIdx = 0; useIdx < listBoxFactions[i].getItemCount();
               useIdx++) {
            string selectedFactionName = listBoxFactions[i].getItem(useIdx);
            if (selectedFactionName !=
                    formatString(GameConstants::RANDOMFACTION_SLOTNAME) &&
                selectedFactionName !=
                    formatString(GameConstants::OBSERVER_SLOTNAME)) {
              factionIndexToUse = useIdx;
              break;
            }
          }
          listBoxFactions[i].setSelectedItemIndex(factionIndexToUse);
          randomFactionSelectionList.push_back(
              listBoxFactions[i].getItem(factionIndexToUse));
        }

        if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
          SystemFlags::OutputDebug(
              SystemFlags::debugSystem,
              "In [%s::%s Line %d] i = %d, "
              "listBoxFactions[i].getSelectedItem() [%s]\n",
              extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
              __LINE__, i, listBoxFactions[i].getSelectedItem().c_str());

        RandomCount++;
      }
    }
  }

  if (RandomCount > 0) {
    needToSetChangedGameSettings = true;
  }

  safeMutex.ReleaseLock(true);
  safeMutexCLI.ReleaseLock(true);
  GameSettings gameSettings;
  copyToGameSettings(&gameSettings, true);

  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem, "In [%s::%s Line %d]\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__);
  ServerInterface *serverInterface =
      NetworkManager::getInstance().getServerInterface();

  // Send the game settings to each client if we have at least one networked
  // client
  safeMutex.Lock();
  safeMutexCLI.Lock();

  bool dataSynchCheckOk = checkNetworkPlayerDataSynch(true, true, true);

  // Ensure we have no dangling network players
  for (int i = 0; i < GameConstants::maxPlayers; ++i) {
    if (listBoxControls[i].getSelectedItemIndex() == ctNetworkUnassigned) {
      mainMessageBoxState = 1;

      Lang &lang = Lang::getInstance();
      string sMsg = "";
      if (lang.hasString("NetworkSlotUnassignedErrorUI") == true) {
        sMsg = lang.getString("NetworkSlotUnassignedErrorUI");
      } else {
        sMsg = "Cannot start game.\nSome player(s) are not in a network game "
               "slot!";
      }

      showMessageBox(sMsg, "", false);

      const vector<string> languageList =
          serverInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
      for (unsigned int j = 0; j < languageList.size(); ++j) {
        char szMsg[8096] = "";
        if (lang.hasString("NetworkSlotUnassignedError", languageList[j]) ==
            true) {
          string msg_string = lang.getString("NetworkSlotUnassignedError");
#ifdef WIN32
          strncpy(szMsg, msg_string.c_str(),
                  min((int)msg_string.length(), 8095));
#else
          strncpy(szMsg, msg_string.c_str(),
                  std::min((int)msg_string.length(), 8095));
#endif
        } else {
          strcpy(szMsg, "Cannot start game, some player(s) are not in a "
                        "network game slot!");
        }

        serverInterface->sendTextMessage(szMsg, -1, true, languageList[j]);
      }

      safeMutex.ReleaseLock();
      safeMutexCLI.ReleaseLock();
      return;
    }
  }

  if (dataSynchCheckOk == false) {
    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem,
                               "In [%s::%s Line %d]\n",
                               extractFileFromDirectoryPath(__FILE__).c_str(),
                               __FUNCTION__, __LINE__);
    mainMessageBoxState = 1;
    showMessageBox("You cannot start the game because\none or more clients do "
                   "not have the same game data!",
                   "Data Mismatch Error", false);

    safeMutex.ReleaseLock();
    safeMutexCLI.ReleaseLock();
    return;
  } else {
    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem,
                               "In [%s::%s Line %d]\n",
                               extractFileFromDirectoryPath(__FILE__).c_str(),
                               __FUNCTION__, __LINE__);
    if ((hasNetworkGameSettings() == true &&
         needToSetChangedGameSettings == true) ||
        (RandomCount > 0)) {
      if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
        SystemFlags::OutputDebug(SystemFlags::debugSystem,
                                 "In [%s::%s Line %d]\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__);
      serverInterface->setGameSettings(&gameSettings, true);
      serverInterface->broadcastGameSetup(&gameSettings);

      needToSetChangedGameSettings = false;
      lastSetChangedGameSettings = time(NULL);
    }

    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem,
                               "In [%s::%s Line %d]\n",
                               extractFileFromDirectoryPath(__FILE__).c_str(),
                               __FUNCTION__, __LINE__);

    // Last check, stop human player from being in same slot as network
    if (isMasterserverMode() == false) {
      bool hasHuman = false;
      for (int i = 0; i < mapInfo.players; ++i) {
        if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
          SystemFlags::OutputDebug(
              SystemFlags::debugSystem, "In [%s::%s Line %d]\n",
              extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
              __LINE__);

        // Check for random faction selection and choose the faction now
        if (listBoxControls[i].getSelectedItemIndex() == ctHuman) {
          hasHuman = true;
          break;
        }
      }
      if (hasHuman == false) {
        mainMessageBoxState = 1;

        Lang &lang = Lang::getInstance();
        string sMsg = lang.getString("NetworkSlotNoHumanErrorUI", "", true);
        showMessageBox(sMsg, "", false);

        const vector<string> languageList =
            serverInterface->getGameSettings()
                ->getUniqueNetworkPlayerLanguages();
        for (unsigned int j = 0; j < languageList.size(); ++j) {
          sMsg = lang.getString("NetworkSlotNoHumanError", "", true);

          serverInterface->sendTextMessage(sMsg, -1, true, languageList[j]);
        }

        safeMutex.ReleaseLock();
        safeMutexCLI.ReleaseLock();
        return;
      }
    }

    // Tell the server Interface whether or not to publish game status updates
    // to masterserver
    serverInterface->setNeedToRepublishToMasterserver(
        checkBoxPublishServer.getValue() == true);

    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem,
                               "In [%s::%s Line %d]\n",
                               extractFileFromDirectoryPath(__FILE__).c_str(),
                               __FUNCTION__, __LINE__);
    bool bOkToStart = serverInterface->launchGame(&gameSettings);
    if (bOkToStart == true) {
      if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
        SystemFlags::OutputDebug(SystemFlags::debugSystem,
                                 "In [%s::%s Line %d]\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__);

      if (checkBoxPublishServer.getEditable() &&
          checkBoxPublishServer.getValue() == true) {

        if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
          SystemFlags::OutputDebug(
              SystemFlags::debugSystem, "In [%s::%s Line %d]\n",
              extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
              __LINE__);

        needToRepublishToMasterserver = true;
        lastMasterserverPublishing = 0;

        if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
          SystemFlags::OutputDebug(
              SystemFlags::debugSystem, "In [%s::%s Line %d]\n",
              extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
              __LINE__);
      }
      needToBroadcastServerSettings = false;
      needToRepublishToMasterserver = false;
      lastNetworkPing = time(NULL);
      safeMutex.ReleaseLock();
      safeMutexCLI.ReleaseLock();

      if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
        SystemFlags::OutputDebug(SystemFlags::debugSystem,
                                 "In [%s::%s Line %d]\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__);

      assert(program != NULL);

      if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
        SystemFlags::OutputDebug(SystemFlags::debugSystem,
                                 "In [%s::%s Line %d]\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__);
      cleanup();
      Game *newGame =
          new Game(program, &gameSettings, this->headlessServerMode);
      forceWaitForShutdown = false;
      program->setState(newGame);
      return;
    } else {
      safeMutex.ReleaseLock();
      safeMutexCLI.ReleaseLock();
    }
  }
}

void MenuStateCustomGame::eventMouseWheel(int x, int y, int zDelta) {
  if (isMasterserverMode() == true) {
    return;
  }
  comboBoxMap.eventMouseWheel(x, y, zDelta);
  comboBoxLoadSetup.eventMouseWheel(x, y, zDelta);
}

void MenuStateCustomGame::mouseMove(int x, int y, const MouseState *ms) {
  if (isMasterserverMode() == true) {
    return;
  }

  if (mainMessageBox.getEnabled()) {
    mainMessageBox.mouseMove(x, y);
  }

  comboBoxMap.mouseMove(x, y);
  if (comboBoxMap.isDropDownShowing()) {
    if (ms->get(mbLeft)) {
      comboBoxMap.mouseDown(x, y);
    }
    if (lastPreviewedMapFile != getPreselectedMapFile()) {
      loadMapInfo(Config::getMapPath(getPreselectedMapFile(), "", false),
                  &mapInfo, true, false);
      labelMapInfo.setText(mapInfo.desc);
      lastPreviewedMapFile = getPreselectedMapFile();
    }
  }

  comboBoxLoadSetup.mouseMove(x, y);
  if (comboBoxLoadSetup.isDropDownShowing()) {
    if (ms->get(mbLeft)) {
      comboBoxLoadSetup.mouseDown(x, y);
    }
  }
  buttonSaveSetup.mouseMove(x, y);
  buttonLoadSetup.mouseMove(x, y);
  buttonDeleteSetup.mouseMove(x, y);

  buttonReturn.mouseMove(x, y);
  buttonPlayNow.mouseMove(x, y);
  buttonClearBlockedPlayers.mouseMove(x, y);

  for (int i = 0; i < GameConstants::maxPlayers; ++i) {
    listBoxRMultiplier[i].mouseMove(x, y);
    listBoxControls[i].mouseMove(x, y);
    buttonBlockPlayers[i].mouseMove(x, y);
    listBoxFactions[i].mouseMove(x, y);
    listBoxTeams[i].mouseMove(x, y);
  }

  if (checkBoxAdvanced.getValue() == 1) {
    listBoxFogOfWar.mouseMove(x, y);
    checkBoxAllowObservers.mouseMove(x, y);

    buttonShowLanInfo.mouseMove(x, y);

    checkBoxEnableSwitchTeamMode.mouseMove(x, y);
    listBoxAISwitchTeamAcceptPercent.mouseMove(x, y);
    listBoxFallbackCpuMultiplier.mouseMove(x, y);

    labelNetworkPauseGameForLaggedClients.mouseMove(x, y);
    checkBoxNetworkPauseGameForLaggedClients.mouseMove(x, y);

    labelAllowTeamUnitSharing.mouseMove(x, y);
    checkBoxAllowTeamUnitSharing.mouseMove(x, y);
    labelAllowTeamResourceSharing.mouseMove(x, y);
    checkBoxAllowTeamResourceSharing.mouseMove(x, y);
  }
  checkBoxAllowInGameJoinPlayer.mouseMove(x, y);

  checkBoxAllowNativeLanguageTechtree.mouseMove(x, y);

  listBoxTileset.mouseMove(x, y);
  listBoxMapFilter.mouseMove(x, y);
  listBoxTechTree.mouseMove(x, y);
  checkBoxPublishServer.mouseMove(x, y);

  checkBoxAdvanced.mouseMove(x, y);

  checkBoxScenario.mouseMove(x, y);
  listBoxScenario.mouseMove(x, y);
}

bool MenuStateCustomGame::isMasterserverMode() const {
  return (this->headlessServerMode == true &&
          this->masterserverModeMinimalResources == true);
  // return false;
}

bool MenuStateCustomGame::isVideoPlaying() {
  bool result = false;
  if (factionVideo != NULL) {
    result = factionVideo->isPlaying();
  }
  return result;
}

void MenuStateCustomGame::render() {
  try {
    Renderer &renderer = Renderer::getInstance();

    if (mainMessageBox.getEnabled() == false) {
      if (factionTexture != NULL) {
        if (factionVideo == NULL || factionVideo->isPlaying() == false) {
          renderer.renderTextureQuad(800, 600, 200, 150, factionTexture, 1.0f);
        }
      }
    }
    if (factionVideo != NULL) {
      if (factionVideo->isPlaying() == true) {
        factionVideo->playFrame(false);
      } else {
        if (GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false &&
            ::Shared::Graphics::VideoPlayer::hasBackEndVideoPlayer() == true) {
          if (factionVideo != NULL) {
            factionVideo->closePlayer();
            delete factionVideo;
            factionVideo = NULL;

            ServerInterface *serverInterface =
                NetworkManager::getInstance().getServerInterface();
            if (serverInterface != NULL) {
              initFactionPreview(serverInterface->getGameSettings());
            }
          }
        }
      }
    }

    if (mainMessageBox.getEnabled()) {
      renderer.renderMessageBox(&mainMessageBox);

      renderer.renderButton(&buttonReturn);
    } else {
      if (mapPreviewTexture != NULL) {
        // renderer.renderTextureQuad(5,185,150,150,mapPreviewTexture,1.0f);
        renderer.renderTextureQuad(
            this->render_mapPreviewTexture_X, this->render_mapPreviewTexture_Y,
            this->render_mapPreviewTexture_W, this->render_mapPreviewTexture_H,
            mapPreviewTexture, 1.0f);
        if (this->zoomedMap == true) {
          return;
        }
        // printf("=================> Rendering map preview texture\n");
      }
      if (scenarioLogoTexture != NULL) {
        renderer.renderTextureQuad(300, 350, 400, 300, scenarioLogoTexture,
                                   1.0f);
        // renderer.renderBackground(scenarioLogoTexture);
      }

      renderer.renderButton(&buttonReturn);
      renderer.renderButton(&buttonPlayNow);

      renderer.renderLabel(&labelSaveSetupName);
      renderer.renderButton(&buttonSaveSetup);
      renderer.renderButton(&buttonLoadSetup);
      renderer.renderButton(&buttonDeleteSetup);

      // Get a reference to the player texture cache
      std::map<int, Texture2D *> &crcPlayerTextureCache =
          CacheManager::getCachedItem<std::map<int, Texture2D *>>(
              GameConstants::playerTextureCacheLookupKey);

      // START - this code ensure player title and player names don't overlap
      int offsetPosition = 0;
      for (int i = 0; i < GameConstants::maxPlayers; ++i) {
        FontMetrics *fontMetrics = NULL;
        if (Renderer::renderText3DEnabled == false) {
          fontMetrics = labelPlayers[i].getFont()->getMetrics();
        } else {
          fontMetrics = labelPlayers[i].getFont3D()->getMetrics();
        }
        if (fontMetrics == NULL) {
          throw megaglest_runtime_error("fontMetrics == NULL");
        }
        int curWidth = (fontMetrics->getTextWidth(labelPlayers[i].getText()));
        int newOffsetPosition = labelPlayers[i].getX() + curWidth + 2;

        // printf("labelPlayers[i].getX() = %d curWidth = %d
        // labelPlayerNames[i].getX() = %d offsetPosition = %d newOffsetPosition
        // = %d
        // [%s]\n",labelPlayers[i].getX(),curWidth,labelPlayerNames[i].getX(),offsetPosition,newOffsetPosition,labelPlayers[i].getText().c_str());

        if (labelPlayers[i].getX() + curWidth >= labelPlayerNames[i].getX()) {
          if (offsetPosition < newOffsetPosition) {
            offsetPosition = newOffsetPosition;
          }
        }
      }
      // END

      ServerInterface *serverInterface =
          NetworkManager::getInstance().getServerInterface();
      if (hasNetworkGameSettings() == true) {
        renderer.renderListBox(&listBoxPlayerStatus);
        if (serverInterface != NULL &&
            buttonClearBlockedPlayers.getEditable() &&
            serverInterface->getServerSocket() != NULL) {
          renderer.renderButton(&buttonClearBlockedPlayers);
        }
      }
      for (int i = 0; i < GameConstants::maxPlayers; ++i) {
        if (listBoxControls[i].getSelectedItemIndex() == ctNetworkUnassigned) {
          // printf("Player #%d [%s] control =
          // %d\n",i,labelPlayerNames[i].getText().c_str(),listBoxControls[i].getSelectedItemIndex());

          labelPlayers[i].setVisible(true);
          labelPlayerNames[i].setVisible(true);
          listBoxControls[i].setVisible(true);
          listBoxFactions[i].setVisible(true);
          listBoxTeams[i].setVisible(true);
          labelNetStatus[i].setVisible(true);
        }

        if (hasNetworkGameSettings() == true &&
            listBoxControls[i].getSelectedItemIndex() != ctClosed) {

          renderer.renderLabel(&labelPlayerStatus[i]);
        }

        if (crcPlayerTextureCache[i] != NULL) {
          // Render the player # label the player's color
          Vec3f playerColor =
              crcPlayerTextureCache[i]->getPixmap()->getPixel3f(0, 0);
          renderer.renderLabel(&labelPlayers[i], &playerColor);
        } else {
          renderer.renderLabel(&labelPlayers[i]);
        }

        if (offsetPosition > 0) {
          labelPlayerNames[i].setX(offsetPosition);
        }
        renderer.renderLabel(&labelPlayerNames[i]);

        renderer.renderListBox(&listBoxControls[i]);

        if (hasNetworkGameSettings() == true &&
            listBoxControls[i].getSelectedItemIndex() != ctClosed) {

          renderer.renderLabel(&labelPlayerStatus[i]);

          if (listBoxControls[i].getSelectedItemIndex() == ctNetwork ||
              listBoxControls[i].getSelectedItemIndex() ==
                  ctNetworkUnassigned) {
            ServerInterface *serverInterface =
                NetworkManager::getInstance().getServerInterface();
            if (serverInterface != NULL &&
                serverInterface->getSlot(i, true) != NULL &&
                serverInterface->getSlot(i, true)->isConnected()) {
              renderer.renderButton(&buttonBlockPlayers[i]);
            }
          }
        }

        if (listBoxControls[i].getSelectedItemIndex() != ctClosed) {
          renderer.renderListBox(&listBoxRMultiplier[i]);
          renderer.renderListBox(&listBoxFactions[i]);

          int teamnumber = listBoxTeams[i].getSelectedItemIndex();
          Vec3f teamcolor = Vec3f(1.0f, 1.0f, 1.0f);
          if (teamnumber >= 0 && teamnumber < 8) {
            teamcolor =
                crcPlayerTextureCache[teamnumber]->getPixmap()->getPixel3f(0,
                                                                           0);
          }
          listBoxTeams[i].setTextColor(teamcolor);

          renderer.renderListBox(&listBoxTeams[i]);
          renderer.renderLabel(&labelNetStatus[i]);
        }
      }
      renderer.renderLabel(&labelMap);

      if (checkBoxAdvanced.getValue() == 1) {
        renderer.renderLabel(&labelFogOfWar);
        renderer.renderListBox(&listBoxFogOfWar);

        renderer.renderLabel(&labelAllowTeamUnitSharing);
        renderer.renderCheckBox(&checkBoxAllowTeamUnitSharing);

        renderer.renderButton(&buttonShowLanInfo);

        renderer.renderLabel(&labelAllowTeamResourceSharing);
        renderer.renderCheckBox(&checkBoxAllowTeamResourceSharing);

        renderer.renderLabel(&labelAllowNativeLanguageTechtree);
        renderer.renderCheckBox(&checkBoxAllowNativeLanguageTechtree);

        renderer.renderLabel(&labelAllowObservers);
        renderer.renderCheckBox(&checkBoxAllowObservers);

        renderer.renderCheckBox(&checkBoxEnableSwitchTeamMode);
        renderer.renderLabel(&labelEnableSwitchTeamMode);
        renderer.renderListBox(&listBoxAISwitchTeamAcceptPercent);
        renderer.renderLabel(&labelAISwitchTeamAcceptPercent);

        renderer.renderCheckBox(&checkBoxScenario);
        renderer.renderLabel(&labelScenario);
        if (checkBoxScenario.getValue() == true) {
          renderer.renderListBox(&listBoxScenario);
        }
      }
      renderer.renderLabel(&labelAllowInGameJoinPlayer);
      renderer.renderCheckBox(&checkBoxAllowInGameJoinPlayer);

      renderer.renderLabel(&labelTileset);
      renderer.renderLabel(&labelMapFilter);
      renderer.renderLabel(&labelTechTree);
      renderer.renderLabel(&labelControl);
      renderer.renderLabel(&labelFaction);
      renderer.renderLabel(&labelTeam);
      renderer.renderLabel(&labelMapInfo);
      renderer.renderLabel(&labelAdvanced);

      renderer.renderListBox(&listBoxTileset);
      renderer.renderListBox(&listBoxMapFilter);
      renderer.renderListBox(&listBoxTechTree);
      renderer.renderCheckBox(&checkBoxAdvanced);

      if (checkBoxPublishServer.getEditable()) {
        renderer.renderCheckBox(&checkBoxPublishServer);
        renderer.renderLabel(&labelPublishServer);
        renderer.renderLabel(&labelGameName);
        if (checkBoxAdvanced.getValue() == 1) {
          renderer.renderLabel(&labelNetworkPauseGameForLaggedClients);
          renderer.renderCheckBox(&checkBoxNetworkPauseGameForLaggedClients);
          renderer.renderListBox(&listBoxFallbackCpuMultiplier);
          renderer.renderLabel(&labelFallbackCpuMultiplier);
        }
      }
    }

    renderer.renderConsole(&console, showFullConsole ? consoleFull
                                                     : consoleStoredAndNormal);

    if (program != NULL)
      program->renderProgramMsgBox();

    if (mainMessageBox.getEnabled() == false) {
      if (hasNetworkGameSettings() == true) {
        renderer.renderChatManager(&chatManager);
      }
    }
    renderer.renderComboBox(&comboBoxMap);
    renderer.renderComboBox(&comboBoxLoadSetup);

    if (enableMapPreview == true && mapPreview.hasFileLoaded() == true) {

      if (mapPreviewTexture == NULL) {
        bool renderAll = (listBoxFogOfWar.getSelectedItemIndex() == 2);
        // printf("=================> Rendering map preview into a texture
        // BEFORE (%p)\n", mapPreviewTexture);

        // renderer.renderMapPreview(&mapPreview, renderAll, 10,
        // 350,&mapPreviewTexture);
        renderer.renderMapPreview(
            &mapPreview, renderAll, this->render_mapPreviewTexture_X,
            this->render_mapPreviewTexture_Y, &mapPreviewTexture);

        // printf("=================> Rendering map preview into a texture AFTER
        // (%p)\n", mapPreviewTexture);
      }
    }

  } catch (const std::exception &ex) {
    char szBuf[8096] = "";
    snprintf(szBuf, 8096, "In [%s::%s %d]\nError detected:\n%s\n",
             extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
             __LINE__, ex.what());
    // throw megaglest_runtime_error(szBuf);

    SystemFlags::OutputDebug(SystemFlags::debugError, szBuf);
    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem, "%s", szBuf);
    showGeneralError = true;
    generalErrorToShow = szBuf;
  }
}

void MenuStateCustomGame::switchSetupForSlots(
    SwitchSetupRequest **switchSetupRequests, ServerInterface *&serverInterface,
    int startIndex, int endIndex, bool onlyNetworkUnassigned) {
  for (int i = startIndex; i < endIndex; ++i) {
    if (switchSetupRequests[i] != NULL) {
      // printf("Switch slot = %d control = %d newIndex = %d currentindex = %d
      // onlyNetworkUnassigned =
      // %d\n",i,listBoxControls[i].getSelectedItemIndex(),switchSetupRequests[i]->getToFactionIndex(),switchSetupRequests[i]->getCurrentFactionIndex(),onlyNetworkUnassigned);

      if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
        SystemFlags::OutputDebug(
            SystemFlags::debugSystem,
            "In [%s::%s Line: %d] switchSetupRequests[i]->getSwitchFlags() = "
            "%d\n",
            extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
            __LINE__, switchSetupRequests[i]->getSwitchFlags());

      if (onlyNetworkUnassigned == true &&
          listBoxControls[i].getSelectedItemIndex() != ctNetworkUnassigned) {
        if (i < mapInfo.players ||
            (i >= mapInfo.players &&
             listBoxControls[i].getSelectedItemIndex() != ctNetwork)) {
          continue;
        }
      }

      if (listBoxControls[i].getSelectedItemIndex() == ctNetwork ||
          listBoxControls[i].getSelectedItemIndex() == ctNetworkUnassigned) {
        if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
          SystemFlags::OutputDebug(
              SystemFlags::debugSystem,
              "In [%s::%s Line: %d] "
              "switchSetupRequests[i]->getToFactionIndex() = %d\n",
              extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
              __LINE__, switchSetupRequests[i]->getToSlotIndex());

        if (switchSetupRequests[i]->getToSlotIndex() != -1) {
          int newFactionIdx = switchSetupRequests[i]->getToSlotIndex();

          // printf("switchSlot request from %d to
          // %d\n",switchSetupRequests[i]->getCurrentFactionIndex(),switchSetupRequests[i]->getToFactionIndex());
          int switchFactionIdx = switchSetupRequests[i]->getCurrentSlotIndex();
          if (serverInterface->switchSlot(switchFactionIdx, newFactionIdx)) {
            try {
              ServerInterface *serverInterface =
                  NetworkManager::getInstance().getServerInterface();
              ConnectionSlot *slot =
                  serverInterface->getSlot(newFactionIdx, true);

              if (switchSetupRequests[i]->getSelectedFactionName() != "" &&
                  (slot != NULL &&
                   switchSetupRequests[i]->getSelectedFactionName() !=
                       Lang::getInstance().getString(
                           "DataMissing", slot->getNetworkPlayerLanguage(),
                           true) &&
                   switchSetupRequests[i]->getSelectedFactionName() !=
                       "???DataMissing???")) {
                listBoxFactions[newFactionIdx].setSelectedItem(
                    switchSetupRequests[i]->getSelectedFactionName());
              }
              if (switchSetupRequests[i]->getToTeam() != -1) {
                listBoxTeams[newFactionIdx].setSelectedItemIndex(
                    switchSetupRequests[i]->getToTeam());
              }
              if (switchSetupRequests[i]->getNetworkPlayerName() != "") {
                if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem)
                        .enabled)
                  SystemFlags::OutputDebug(
                      SystemFlags::debugSystem,
                      "In [%s::%s Line %d] i = %d, "
                      "labelPlayerNames[newFactionIdx].getText() [%s] "
                      "switchSetupRequests[i]->getNetworkPlayerName() [%s]\n",
                      extractFileFromDirectoryPath(__FILE__).c_str(),
                      __FUNCTION__, __LINE__, i,
                      labelPlayerNames[newFactionIdx].getText().c_str(),
                      switchSetupRequests[i]->getNetworkPlayerName().c_str());
                labelPlayerNames[newFactionIdx].setText(
                    switchSetupRequests[i]->getNetworkPlayerName());
              }

              if (listBoxControls[switchFactionIdx].getSelectedItemIndex() ==
                  ctNetworkUnassigned) {
                serverInterface->removeSlot(switchFactionIdx);
                listBoxControls[switchFactionIdx].setSelectedItemIndex(
                    ctClosed);

                labelPlayers[switchFactionIdx].setVisible(
                    switchFactionIdx + 1 <= mapInfo.players);
                labelPlayerNames[switchFactionIdx].setVisible(
                    switchFactionIdx + 1 <= mapInfo.players);
                listBoxControls[switchFactionIdx].setVisible(
                    switchFactionIdx + 1 <= mapInfo.players);
                listBoxFactions[switchFactionIdx].setVisible(
                    switchFactionIdx + 1 <= mapInfo.players);
                listBoxTeams[switchFactionIdx].setVisible(
                    switchFactionIdx + 1 <= mapInfo.players);
                labelNetStatus[switchFactionIdx].setVisible(
                    switchFactionIdx + 1 <= mapInfo.players);
              }
            } catch (const runtime_error &e) {
              SystemFlags::OutputDebug(
                  SystemFlags::debugError, "In [%s::%s Line: %d] Error [%s]\n",
                  extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                  __LINE__, e.what());
              if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem)
                      .enabled)
                SystemFlags::OutputDebug(
                    SystemFlags::debugSystem,
                    "In [%s::%s Line: %d] caught exception error = [%s]\n",
                    extractFileFromDirectoryPath(__FILE__).c_str(),
                    __FUNCTION__, __LINE__, e.what());
            }
          }
        } else {
          try {
            int factionIdx = switchSetupRequests[i]->getCurrentSlotIndex();
            ServerInterface *serverInterface =
                NetworkManager::getInstance().getServerInterface();
            ConnectionSlot *slot = serverInterface->getSlot(factionIdx, true);

            if (switchSetupRequests[i]->getSelectedFactionName() != "" &&
                (slot != NULL &&
                 switchSetupRequests[i]->getSelectedFactionName() !=
                     Lang::getInstance().getString(
                         "DataMissing", slot->getNetworkPlayerLanguage(),
                         true) &&
                 switchSetupRequests[i]->getSelectedFactionName() !=
                     "???DataMissing???")) {
              listBoxFactions[i].setSelectedItem(
                  switchSetupRequests[i]->getSelectedFactionName());
            }
            if (switchSetupRequests[i]->getToTeam() != -1) {
              listBoxTeams[i].setSelectedItemIndex(
                  switchSetupRequests[i]->getToTeam());
            }

            if ((switchSetupRequests[i]->getSwitchFlags() &
                 ssrft_NetworkPlayerName) == ssrft_NetworkPlayerName) {
              if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem)
                      .enabled)
                SystemFlags::OutputDebug(
                    SystemFlags::debugSystem,
                    "In [%s::%s Line: %d] i = %d, "
                    "switchSetupRequests[i]->getSwitchFlags() = %d, "
                    "switchSetupRequests[i]->getNetworkPlayerName() [%s], "
                    "labelPlayerNames[i].getText() [%s]\n",
                    extractFileFromDirectoryPath(__FILE__).c_str(),
                    __FUNCTION__, __LINE__, i,
                    switchSetupRequests[i]->getSwitchFlags(),
                    switchSetupRequests[i]->getNetworkPlayerName().c_str(),
                    labelPlayerNames[i].getText().c_str());

              if (switchSetupRequests[i]->getNetworkPlayerName() !=
                  GameConstants::NETWORK_SLOT_UNCONNECTED_SLOTNAME) {
                labelPlayerNames[i].setText(
                    switchSetupRequests[i]->getNetworkPlayerName());
              } else {
                labelPlayerNames[i].setText("");
              }
            }
          } catch (const runtime_error &e) {
            SystemFlags::OutputDebug(
                SystemFlags::debugError, "In [%s::%s Line: %d] Error [%s]\n",
                extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                __LINE__, e.what());
            if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem)
                    .enabled)
              SystemFlags::OutputDebug(
                  SystemFlags::debugSystem,
                  "In [%s::%s Line: %d] caught exception error = [%s]\n",
                  extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                  __LINE__, e.what());
          }
        }
      }

      delete switchSetupRequests[i];
      switchSetupRequests[i] = NULL;
    }
  }
}

void MenuStateCustomGame::update() {
  Chrono chrono;
  if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled)
    chrono.start();

  // Test openal buffer underrun issue
  // sleep(200);
  // END

  MutexSafeWrapper safeMutex(
      (publishToMasterserverThread != NULL
           ? publishToMasterserverThread->getMutexThreadObjectAccessor()
           : NULL),
      string(__FILE__) + "_" + intToStr(__LINE__));
  MutexSafeWrapper safeMutexCLI(
      (publishToClientsThread != NULL
           ? publishToClientsThread->getMutexThreadObjectAccessor()
           : NULL),
      string(__FILE__) + "_" + intToStr(__LINE__));

  try {
    if (serverInitError == true) {
      if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
        SystemFlags::OutputDebug(SystemFlags::debugSystem,
                                 "In [%s::%s Line %d]\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__);

      if (showGeneralError) {
        if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
          SystemFlags::OutputDebug(
              SystemFlags::debugSystem, "In [%s::%s Line %d]\n",
              extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
              __LINE__);

        showGeneralError = false;
        mainMessageBoxState = 1;
        showMessageBox(generalErrorToShow, "Error", false);
      }

      if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
        SystemFlags::OutputDebug(SystemFlags::debugSystem,
                                 "In [%s::%s Line %d]\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__);

      if (this->headlessServerMode == false) {
        return;
      }
    }
    ServerInterface *serverInterface =
        NetworkManager::getInstance().getServerInterface();
    Lang &lang = Lang::getInstance();

    if (serverInterface != NULL && serverInterface->getServerSocket() != NULL) {
      buttonClearBlockedPlayers.setEditable(
          serverInterface->getServerSocket()->hasBlockedIPAddresses());
    }

    if (comboBoxMap.isDropDownShowing()) {
      labelMapInfo.setX(0);
      labelMapInfo.setY(mapPreviewTexture_Y - 40);
    } else {
      labelMapInfo.setX(165);
      labelMapInfo.setY(mapPreviewTexture_Y + mapPreviewTexture_H - 60);
    }
    if (this->autoloadScenarioName != "") {
      listBoxScenario.setSelectedItem(formatString(this->autoloadScenarioName),
                                      false);
      lastSetChangedGameSettings = time(NULL);
      if (serverInterface != NULL) {
        lastGameSettingsreceivedCount =
            serverInterface->getGameSettingsUpdateCount();
      }
      if (listBoxScenario.getSelectedItem() !=
          formatString(this->autoloadScenarioName)) {
        mainMessageBoxState = 1;
        showMessageBox("Could not find scenario name: " +
                           formatString(this->autoloadScenarioName),
                       "Scenario Missing", false);
        this->autoloadScenarioName = "";
      } else {
        loadScenarioInfo(
            Scenario::getScenarioPath(
                dirList, scenarioFiles[listBoxScenario.getSelectedItemIndex()]),
            &scenarioInfo);
        // labelInfo.setText(scenarioInfo.desc);

        SoundRenderer &soundRenderer = SoundRenderer::getInstance();
        CoreData &coreData = CoreData::getInstance();
        soundRenderer.playFx(coreData.getClickSoundC());
        // launchGame();
        PlayNow(true);
        return;
      }
    }

    if (needToLoadTextures) {
      // this delay is done to make it possible to switch faster
      if (difftime((long int)time(NULL), previewLoadDelayTimer) >= 2) {
        // loadScenarioPreviewTexture();
        needToLoadTextures = false;
      }
    }

    // bool haveAtLeastOneNetworkClientConnected = false;
    bool hasOneNetworkSlotOpen = false;
    int currentConnectionCount = 0;
    Config &config = Config::getInstance();

    bool masterServerErr = showMasterserverError;

    if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
            .enabled &&
        chrono.getMillis() > 0)
      SystemFlags::OutputDebug(SystemFlags::debugPerformance,
                               "In [%s::%s Line: %d] took msecs: %lld\n",
                               extractFileFromDirectoryPath(__FILE__).c_str(),
                               __FUNCTION__, __LINE__, chrono.getMillis());
    if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
            .enabled &&
        chrono.getMillis() > 0)
      chrono.start();

    if (masterServerErr) {
      if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
        SystemFlags::OutputDebug(SystemFlags::debugSystem,
                                 "In [%s::%s Line %d]\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__);

      if (EndsWith(masterServererErrorToShow, "wrong router setup") == true) {
        masterServererErrorToShow = lang.getString("WrongRouterSetup");
      }

      Lang &lang = Lang::getInstance();
      string publishText = " (disabling publish)";
      if (lang.hasString("PublishDisabled") == true) {
        publishText = lang.getString("PublishDisabled");
      }

      masterServererErrorToShow += "\n\n" + publishText;
      showMasterserverError = false;
      mainMessageBoxState = 1;
      showMessageBox(masterServererErrorToShow,
                     lang.getString("ErrorFromMasterserver"), false);

      if (this->headlessServerMode == false) {
        checkBoxPublishServer.setValue(false);
      }

      ServerInterface *serverInterface =
          NetworkManager::getInstance().getServerInterface();
      serverInterface->setPublishEnabled(checkBoxPublishServer.getValue() ==
                                         true);
    } else if (showGeneralError) {
      if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
        SystemFlags::OutputDebug(SystemFlags::debugSystem,
                                 "In [%s::%s Line %d]\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__);

      showGeneralError = false;
      mainMessageBoxState = 1;
      showMessageBox(generalErrorToShow, "Error", false);
    }

    if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
            .enabled &&
        chrono.getMillis() > 0)
      SystemFlags::OutputDebug(SystemFlags::debugPerformance,
                               "In [%s::%s Line: %d] took msecs: %lld\n",
                               extractFileFromDirectoryPath(__FILE__).c_str(),
                               __FUNCTION__, __LINE__, chrono.getMillis());
    if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
            .enabled &&
        chrono.getMillis() > 0)
      chrono.start();

    if (this->headlessServerMode == true && serverInterface == NULL) {
      throw megaglest_runtime_error("serverInterface == NULL");
    }
    if (this->headlessServerMode == true &&
        serverInterface->getGameSettingsUpdateCount() >
            lastMasterServerSettingsUpdateCount &&
        serverInterface->getGameSettings() != NULL) {
      const GameSettings *settings = serverInterface->getGameSettings();
      // printf("\n\n\n\n=====#1 got settings [%d]
      // [%d]:\n%s\n",lastMasterServerSettingsUpdateCount,serverInterface->getGameSettingsUpdateCount(),settings->toString().c_str());

      lastMasterServerSettingsUpdateCount =
          serverInterface->getGameSettingsUpdateCount();
      // printf("#2 custom menu got map [%s]\n",settings->getMap().c_str());

      setupUIFromGameSettings(*settings);
      printf("received Settings map filter=%d\n", settings->getMapFilter());

      GameSettings gameSettings;
      copyToGameSettings(&gameSettings);

      // printf("\n\n\n\n=====#1.1 got settings [%d]
      // [%d]:\n%s\n",lastMasterServerSettingsUpdateCount,serverInterface->getGameSettingsUpdateCount(),gameSettings.toString().c_str());
    }
    if (this->headlessServerMode == true &&
        serverInterface->getMasterserverAdminRequestLaunch() == true) {
      serverInterface->setMasterserverAdminRequestLaunch(false);
      safeMutex.ReleaseLock();
      safeMutexCLI.ReleaseLock();

      PlayNow(false);
      return;
    }

    // handle setting changes from clients
    SwitchSetupRequest **switchSetupRequests =
        serverInterface->getSwitchSetupRequests();
    //!!!
    switchSetupForSlots(switchSetupRequests, serverInterface, 0,
                        mapInfo.players, false);
    switchSetupForSlots(switchSetupRequests, serverInterface, mapInfo.players,
                        GameConstants::maxPlayers, true);

    if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
            .enabled &&
        chrono.getMillis() > 0)
      SystemFlags::OutputDebug(SystemFlags::debugPerformance,
                               "In [%s::%s Line: %d] took msecs: %lld\n",
                               extractFileFromDirectoryPath(__FILE__).c_str(),
                               __FUNCTION__, __LINE__, chrono.getMillis());
    if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
            .enabled &&
        chrono.getMillis() > 0)
      chrono.start();

    GameSettings gameSettings;
    copyToGameSettings(&gameSettings);

    listBoxAISwitchTeamAcceptPercent.setVisible(
        checkBoxEnableSwitchTeamMode.getValue());
    labelAISwitchTeamAcceptPercent.setVisible(
        checkBoxEnableSwitchTeamMode.getValue());

    int factionCount = 0;
    for (int i = 0; i < mapInfo.players; ++i) {
      if (hasNetworkGameSettings() == true) {
        if (listBoxControls[i].getSelectedItemIndex() != ctClosed) {
          int slotIndex = factionCount;
          if (listBoxControls[i].getSelectedItemIndex() == ctHuman) {
            switch (gameSettings.getNetworkPlayerStatuses(slotIndex)) {
            case npst_BeRightBack:
              labelPlayerStatus[i].setTexture(
                  CoreData::getInstance().getStatusBRBTexture());
              break;
            case npst_Ready:
              labelPlayerStatus[i].setTexture(
                  CoreData::getInstance().getStatusReadyTexture());
              break;
            case npst_PickSettings:
              labelPlayerStatus[i].setTexture(
                  CoreData::getInstance().getStatusNotReadyTexture());
              break;
            case npst_Disconnected:
              labelPlayerStatus[i].setTexture(NULL);
              break;

            default:
              labelPlayerStatus[i].setTexture(NULL);
              break;
            }
          } else {
            labelPlayerStatus[i].setTexture(NULL);
          }

          factionCount++;
        } else {
          labelPlayerStatus[i].setTexture(NULL);
        }
      }

      if (listBoxControls[i].getSelectedItemIndex() == ctNetwork ||
          listBoxControls[i].getSelectedItemIndex() == ctNetworkUnassigned) {
        hasOneNetworkSlotOpen = true;

        if (serverInterface->getSlot(i, true) != NULL &&
            serverInterface->getSlot(i, true)->isConnected()) {

          if (hasNetworkGameSettings() == true) {
            switch (
                serverInterface->getSlot(i, true)->getNetworkPlayerStatus()) {
            case npst_BeRightBack:
              labelPlayerStatus[i].setTexture(
                  CoreData::getInstance().getStatusBRBTexture());
              break;
            case npst_Ready:
              labelPlayerStatus[i].setTexture(
                  CoreData::getInstance().getStatusReadyTexture());
              break;
            case npst_PickSettings:
            default:
              labelPlayerStatus[i].setTexture(
                  CoreData::getInstance().getStatusNotReadyTexture());
              break;
            }
          }

          serverInterface->getSlot(i, true)->setName(
              labelPlayerNames[i].getText());

          // printf("FYI we have at least 1 client connected, slot = %d'\n",i);

          // haveAtLeastOneNetworkClientConnected = true;
          if (serverInterface->getSlot(i, true) != NULL &&
              serverInterface->getSlot(i, true)->getConnectHasHandshaked()) {
            currentConnectionCount++;
          }
          string label =
              (serverInterface->getSlot(i, true) != NULL
                   ? serverInterface->getSlot(i, true)->getVersionString()
                   : "");

          if (serverInterface->getSlot(i, true) != NULL &&
              serverInterface->getSlot(i, true)->getAllowDownloadDataSynch() ==
                  true &&
              serverInterface->getSlot(i, true)->getAllowGameDataSynchCheck() ==
                  true) {
            if (serverInterface->getSlot(i, true)
                    ->getNetworkGameDataSynchCheckOk() == false) {
              label += " -waiting to synch:";
              if (serverInterface->getSlot(i, true)
                      ->getNetworkGameDataSynchCheckOkMap() == false) {
                label = label + " map";
              }
              if (serverInterface->getSlot(i, true)
                      ->getNetworkGameDataSynchCheckOkTile() == false) {
                label = label + " tile";
              }
              if (serverInterface->getSlot(i, true)
                      ->getNetworkGameDataSynchCheckOkTech() == false) {
                label = label + " techtree";
              }
            } else {
              label += " - data synch is ok";
            }
          } else {
            if (serverInterface->getSlot(i, true) != NULL &&
                serverInterface->getSlot(i, true)
                        ->getAllowGameDataSynchCheck() == true &&
                serverInterface->getSlot(i, true)
                        ->getNetworkGameDataSynchCheckOk() == false) {
              label += " -synch mismatch:";

              if (serverInterface->getSlot(i, true) != NULL &&
                  serverInterface->getSlot(i, true)
                          ->getNetworkGameDataSynchCheckOkMap() == false) {
                label = label + " map";

                if (serverInterface->getSlot(i, true)
                            ->getReceivedDataSynchCheck() == true &&
                    lastMapDataSynchError !=
                        "map CRC mismatch, " + comboBoxMap.getSelectedItem()) {
                  lastMapDataSynchError =
                      "map CRC mismatch, " + comboBoxMap.getSelectedItem();
                  ServerInterface *serverInterface =
                      NetworkManager::getInstance().getServerInterface();
                  serverInterface->sendTextMessage(lastMapDataSynchError, -1,
                                                   true, "");
                }
              }

              if (serverInterface->getSlot(i, true) != NULL &&
                  serverInterface->getSlot(i, true)
                          ->getNetworkGameDataSynchCheckOkTile() == false) {
                label = label + " tile";

                if (serverInterface->getSlot(i, true)
                            ->getReceivedDataSynchCheck() == true &&
                    lastTileDataSynchError !=
                        "tile CRC mismatch, " +
                            listBoxTileset.getSelectedItem()) {
                  lastTileDataSynchError =
                      "tile CRC mismatch, " + listBoxTileset.getSelectedItem();
                  ServerInterface *serverInterface =
                      NetworkManager::getInstance().getServerInterface();
                  serverInterface->sendTextMessage(lastTileDataSynchError, -1,
                                                   true, "");
                }
              }

              if (serverInterface->getSlot(i, true) != NULL &&
                  serverInterface->getSlot(i, true)
                          ->getNetworkGameDataSynchCheckOkTech() == false) {
                {
                  // Ensure local CRC cache is correct
                  setRefreshedCrcToGameSettings(
                      serverInterface->getGameSettingsPtr());
                }
                label = label + " techtree";

                if (serverInterface->getSlot(i, true)
                        ->getReceivedDataSynchCheck() == true) {
                  ServerInterface *serverInterface =
                      NetworkManager::getInstance().getServerInterface();
                  string report =
                      serverInterface->getSlot(i, true)
                          ->getNetworkGameDataSynchCheckTechMismatchReport();

                  if (lastTechtreeDataSynchError !=
                      "techtree CRC mismatch" + report) {
                    lastTechtreeDataSynchError =
                        "techtree CRC mismatch" + report;

                    if (SystemFlags::getSystemSettingType(
                            SystemFlags::debugSystem)
                            .enabled)
                      SystemFlags::OutputDebug(
                          SystemFlags::debugSystem,
                          "In [%s::%s Line: %d] report: %s\n",
                          extractFileFromDirectoryPath(__FILE__).c_str(),
                          __FUNCTION__, __LINE__, report.c_str());

                    serverInterface->sendTextMessage("techtree CRC mismatch",
                                                     -1, true, "");
                    vector<string> reportLineTokens;
                    Tokenize(report, reportLineTokens, "\n");
                    for (int reportLine = 0;
                         reportLine < (int)reportLineTokens.size();
                         ++reportLine) {
                      serverInterface->sendTextMessage(
                          reportLineTokens[reportLine], -1, true, "");
                    }
                  }
                }
              }

              if (serverInterface->getSlot(i, true) != NULL) {
                serverInterface->getSlot(i, true)->setReceivedDataSynchCheck(
                    false);
              }
            }
          }

          // float pingTime =
          // serverInterface->getSlot(i)->getThreadedPingMS(serverInterface->getSlot(i)->getIpAddress().c_str());
          char szBuf[8096] = "";
          snprintf(szBuf, 8096, "%s", label.c_str());

          labelNetStatus[i].setText(szBuf);
        } else {
          string port = "(" + intToStr(config.getInt("PortServer")) + ")";
          labelNetStatus[i].setText("--- " + port);
        }
      } else if (listBoxControls[i].getSelectedItemIndex() == ctHuman) {
        if (EndsWith(glestVersionString, "-dev") == false) {
          labelNetStatus[i].setText(glestVersionString);
        } else {
          // labelLocalGameVersion.setText(glestVersionString + " [" +
          // getCompileDateTime() + ", " + getGITRevisionString() + "]");
          labelNetStatus[i].setText(glestVersionString + " [" +
                                    getGITRevisionString() + "]");
        }
      } else {
        labelNetStatus[i].setText("");
      }
    }

    if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
            .enabled &&
        chrono.getMillis() > 0)
      SystemFlags::OutputDebug(SystemFlags::debugPerformance,
                               "In [%s::%s Line: %d] took msecs: %lld\n",
                               extractFileFromDirectoryPath(__FILE__).c_str(),
                               __FUNCTION__, __LINE__, chrono.getMillis());
    if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
            .enabled &&
        chrono.getMillis() > 0)
      chrono.start();

    // ServerInterface* serverInterface=
    // NetworkManager::getInstance().getServerInterface();

    if (checkBoxScenario.getValue() == false) {
      for (int i = 0; i < GameConstants::maxPlayers; ++i) {
        if (i >= mapInfo.players) {
          listBoxControls[i].setEditable(false);
          listBoxControls[i].setEnabled(false);

          // printf("In [%s::%s] Line: %d i = %d mapInfo.players =
          // %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,i,mapInfo.players);
        } else if (listBoxControls[i].getSelectedItemIndex() !=
                   ctNetworkUnassigned) {
          ConnectionSlot *slot = serverInterface->getSlot(i, true);
          if ((listBoxControls[i].getSelectedItemIndex() != ctNetwork) ||
              (listBoxControls[i].getSelectedItemIndex() == ctNetwork &&
               (slot == NULL || slot->isConnected() == false))) {
            listBoxControls[i].setEditable(true);
            listBoxControls[i].setEnabled(true);
          } else {
            listBoxControls[i].setEditable(false);
            listBoxControls[i].setEnabled(false);

            // printf("In [%s::%s] Line: %d i = %d mapInfo.players =
            // %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,i,mapInfo.players);
          }
        } else {
          listBoxControls[i].setEditable(false);
          listBoxControls[i].setEnabled(false);

          // printf("In [%s::%s] Line: %d i = %d mapInfo.players =
          // %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,i,mapInfo.players);
        }
      }
    }

    bool checkDataSynch =
        (serverInterface->getAllowGameDataSynchCheck() == true &&
         needToSetChangedGameSettings == true &&
         ((difftime((long int)time(NULL), lastSetChangedGameSettings) >=
           BROADCAST_SETTINGS_SECONDS) ||
          (this->headlessServerMode == true)));

    if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
            .enabled &&
        chrono.getMillis() > 0)
      SystemFlags::OutputDebug(SystemFlags::debugPerformance,
                               "In [%s::%s Line: %d] took msecs: %lld\n",
                               extractFileFromDirectoryPath(__FILE__).c_str(),
                               __FUNCTION__, __LINE__, chrono.getMillis());
    if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
            .enabled &&
        chrono.getMillis() > 0)
      chrono.start();

    // Send the game settings to each client if we have at least one networked
    // client
    if (checkDataSynch == true) {
      serverInterface->setGameSettings(&gameSettings, false);
      needToSetChangedGameSettings = false;
    }

    if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
            .enabled &&
        chrono.getMillis() > 0)
      SystemFlags::OutputDebug(SystemFlags::debugPerformance,
                               "In [%s::%s Line: %d] took msecs: %lld\n",
                               extractFileFromDirectoryPath(__FILE__).c_str(),
                               __FUNCTION__, __LINE__, chrono.getMillis());
    if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
            .enabled &&
        chrono.getMillis() > 0)
      chrono.start();

    if (this->headlessServerMode == true || hasOneNetworkSlotOpen == true ||
        checkBoxAllowInGameJoinPlayer.getValue() == true) {
      if (this->headlessServerMode == true &&
          GlobalStaticFlags::isFlagSet(gsft_lan_mode) == false) {
        checkBoxPublishServer.setValue(true);
      }
      listBoxFallbackCpuMultiplier.setEditable(true);
      checkBoxPublishServer.setEditable(true);

      // Masterserver always needs one network slot
      if (this->headlessServerMode == true && hasOneNetworkSlotOpen == false) {
        bool anyoneConnected = false;
        for (int i = 0; i < mapInfo.players; ++i) {
          MutexSafeWrapper safeMutex(
              (publishToMasterserverThread != NULL
                   ? publishToMasterserverThread->getMutexThreadObjectAccessor()
                   : NULL),
              string(__FILE__) + "_" + intToStr(__LINE__));

          ServerInterface *serverInterface =
              NetworkManager::getInstance().getServerInterface();
          ConnectionSlot *slot = serverInterface->getSlot(i, true);
          if (slot != NULL && slot->isConnected() == true) {
            anyoneConnected = true;
            break;
          }
        }

        for (int i = 0; i < mapInfo.players; ++i) {
          if (anyoneConnected == false &&
              listBoxControls[i].getSelectedItemIndex() != ctNetwork) {
            listBoxControls[i].setSelectedItemIndex(ctNetwork);
          }
        }

        updateNetworkSlots();
      }
    } else {
      checkBoxPublishServer.setValue(false);
      checkBoxPublishServer.setEditable(false);
      listBoxFallbackCpuMultiplier.setEditable(false);

      ServerInterface *serverInterface =
          NetworkManager::getInstance().getServerInterface();
      serverInterface->setPublishEnabled(checkBoxPublishServer.getValue() ==
                                         true);
    }

    bool republishToMaster =
        (difftime((long int)time(NULL), lastMasterserverPublishing) >=
         MASTERSERVER_BROADCAST_PUBLISH_SECONDS);

    if (republishToMaster == true) {
      if (checkBoxPublishServer.getValue() == true) {
        needToRepublishToMasterserver = true;
        lastMasterserverPublishing = time(NULL);
      }
    }

    bool callPublishNow = (checkBoxPublishServer.getEditable() &&
                           checkBoxPublishServer.getValue() == true &&
                           needToRepublishToMasterserver == true);

    if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
            .enabled &&
        chrono.getMillis() > 0)
      SystemFlags::OutputDebug(SystemFlags::debugPerformance,
                               "In [%s::%s Line: %d] took msecs: %lld\n",
                               extractFileFromDirectoryPath(__FILE__).c_str(),
                               __FUNCTION__, __LINE__, chrono.getMillis());
    if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
            .enabled &&
        chrono.getMillis() > 0)
      chrono.start();

    if (callPublishNow == true) {
      // give it to me baby, aha aha ...
      publishToMasterserver();
    }
    if (needToPublishDelayed) {
      // this delay is done to make it possible to switch over maps which are
      // not meant to be distributed
      if ((difftime((long int)time(NULL), mapPublishingDelayTimer) >=
           BROADCAST_MAP_DELAY_SECONDS) ||
          (this->headlessServerMode == true)) {
        // after 5 seconds we are allowed to publish again!
        needToSetChangedGameSettings = true;
        lastSetChangedGameSettings = time(NULL);
        // set to normal....
        needToPublishDelayed = false;
      }
    }
    if (needToPublishDelayed == false || headlessServerMode == true) {
      bool broadCastSettings =
          (difftime((long int)time(NULL), lastSetChangedGameSettings) >=
           BROADCAST_SETTINGS_SECONDS);

      if (headlessServerMode == true) {
        // publish settings directly when we receive them
        ServerInterface *serverInterface =
            NetworkManager::getInstance().getServerInterface();
        if (lastGameSettingsreceivedCount <
            serverInterface->getGameSettingsUpdateCount()) {
          needToBroadcastServerSettings = true;
          lastSetChangedGameSettings = time(NULL);
          lastGameSettingsreceivedCount =
              serverInterface->getGameSettingsUpdateCount();
        }
      }

      if (broadCastSettings == true) {
        needToBroadcastServerSettings = true;
        lastSetChangedGameSettings = time(NULL);
      }

      if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
              .enabled &&
          chrono.getMillis() > 0)
        SystemFlags::OutputDebug(SystemFlags::debugPerformance,
                                 "In [%s::%s Line: %d] took msecs: %lld\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__, chrono.getMillis());
      if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
              .enabled &&
          chrono.getMillis() > 0)
        chrono.start();

      // broadCastSettings = (difftime(time(NULL),lastSetChangedGameSettings) >=
      // 2); if (broadCastSettings == true) {// reset timer here on bottom
      // becasue used for different things 	lastSetChangedGameSettings =
      // time(NULL);
      // }
    }

    if (this->headlessServerMode == true) {
      lastPlayerDisconnected();
    }

    // call the chat manager
    chatManager.updateNetwork();

    // console
    console.update();

    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem,
                               "In [%s::%s Line %d]\n",
                               extractFileFromDirectoryPath(__FILE__).c_str(),
                               __FUNCTION__, __LINE__);

    if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
            .enabled &&
        chrono.getMillis() > 0)
      SystemFlags::OutputDebug(SystemFlags::debugPerformance,
                               "In [%s::%s Line: %d] took msecs: %lld\n",
                               extractFileFromDirectoryPath(__FILE__).c_str(),
                               __FUNCTION__, __LINE__, chrono.getMillis());
    if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
            .enabled &&
        chrono.getMillis() > 0)
      chrono.start();

    if (currentConnectionCount > soundConnectionCount) {
      soundConnectionCount = currentConnectionCount;
      SoundRenderer::getInstance().playFx(
          CoreData::getInstance().getAttentionSound());
      // switch on music again!!
      Config &config = Config::getInstance();
      float configVolume = (config.getInt("SoundVolumeMusic") / 100.f);
      CoreData::getInstance().getMenuMusic()->setVolume(configVolume);
    }
    soundConnectionCount = currentConnectionCount;

    if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
            .enabled &&
        chrono.getMillis() > 0)
      SystemFlags::OutputDebug(SystemFlags::debugPerformance,
                               "In [%s::%s Line: %d] took msecs: %lld\n",
                               extractFileFromDirectoryPath(__FILE__).c_str(),
                               __FUNCTION__, __LINE__, chrono.getMillis());
    if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
            .enabled &&
        chrono.getMillis() > 0)
      chrono.start();

    if (enableFactionTexturePreview == true) {
      if (currentTechName_factionPreview != gameSettings.getTech() ||
          currentFactionName_factionPreview !=
              gameSettings.getFactionTypeName(
                  gameSettings.getThisFactionIndex())) {

        currentTechName_factionPreview = gameSettings.getTech();
        currentFactionName_factionPreview =
            gameSettings.getFactionTypeName(gameSettings.getThisFactionIndex());

        initFactionPreview(&gameSettings);
      }
    }

    if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
            .enabled &&
        chrono.getMillis() > 0)
      SystemFlags::OutputDebug(SystemFlags::debugPerformance,
                               "In [%s::%s Line: %d] took msecs: %lld\n",
                               extractFileFromDirectoryPath(__FILE__).c_str(),
                               __FUNCTION__, __LINE__, chrono.getMillis());
    if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
            .enabled &&
        chrono.getMillis() > 0)
      chrono.start();

    if (autostart == true) {
      autostart = false;
      safeMutex.ReleaseLock();
      safeMutexCLI.ReleaseLock();
      if (autoStartSettings != NULL) {

        setupUIFromGameSettings(*autoStartSettings);
        ServerInterface *serverInterface =
            NetworkManager::getInstance().getServerInterface();
        serverInterface->setGameSettings(autoStartSettings, false);
      } else {
        RestoreLastGameSettings();
      }
      PlayNow((autoStartSettings == NULL));
      return;
    }
  } catch (megaglest_runtime_error &ex) {
    // abort();
    // printf("1111111bbbb ex.wantStackTrace() = %d\n",ex.wantStackTrace());
    char szBuf[8096] = "";
    snprintf(szBuf, 8096, "In [%s::%s %d]\nError detected:\n%s\n",
             extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
             __LINE__, ex.what());
    SystemFlags::OutputDebug(SystemFlags::debugError, szBuf);
    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem, "%s", szBuf);

    // printf("2222222bbbb ex.wantStackTrace() = %d\n",ex.wantStackTrace());

    showGeneralError = true;
    generalErrorToShow = szBuf;
  } catch (const std::exception &ex) {
    char szBuf[8096] = "";
    snprintf(szBuf, 8096, "In [%s::%s %d]\nError detected:\n%s\n",
             extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
             __LINE__, ex.what());
    SystemFlags::OutputDebug(SystemFlags::debugError, szBuf);
    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem, "%s", szBuf);

    showGeneralError = true;
    generalErrorToShow = szBuf;
  }
}

void MenuStateCustomGame::initFactionPreview(const GameSettings *gameSettings) {
  string factionVideoUrl = "";
  string factionVideoUrlFallback = "";

  string factionDefinitionXML = Game::findFactionLogoFile(
      gameSettings, NULL, currentFactionName_factionPreview + ".xml");
  if (factionDefinitionXML != "" &&
      currentFactionName_factionPreview !=
          GameConstants::RANDOMFACTION_SLOTNAME &&
      currentFactionName_factionPreview != GameConstants::OBSERVER_SLOTNAME &&
      fileExists(factionDefinitionXML) == true) {
    XmlTree xmlTree;
    std::map<string, string> mapExtraTagReplacementValues;
    xmlTree.load(factionDefinitionXML, Properties::getTagReplacementValues(
                                           &mapExtraTagReplacementValues));
    const XmlNode *factionNode = xmlTree.getRootNode();
    if (factionNode->hasAttribute("faction-preview-video") == true) {
      factionVideoUrl =
          factionNode->getAttribute("faction-preview-video")->getValue();
    }

    factionVideoUrlFallback =
        Game::findFactionLogoFile(gameSettings, NULL, "preview_video.*");
    if (factionVideoUrl == "") {
      factionVideoUrl = factionVideoUrlFallback;
      factionVideoUrlFallback = "";
    }
  }
  // printf("currentFactionName_factionPreview [%s] random [%s] observer [%s]
  // factionVideoUrl
  // [%s]\n",currentFactionName_factionPreview.c_str(),GameConstants::RANDOMFACTION_SLOTNAME,GameConstants::OBSERVER_SLOTNAME,factionVideoUrl.c_str());

  if (factionVideoUrl != "") {
    // SoundRenderer &soundRenderer= SoundRenderer::getInstance();
    if (CoreData::getInstance().getMenuMusic()->getVolume() != 0) {
      CoreData::getInstance().getMenuMusic()->setVolume(0);
      factionVideoSwitchedOffVolume = true;
    }

    if (currentFactionLogo != factionVideoUrl) {
      currentFactionLogo = factionVideoUrl;
      if (GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false &&
          ::Shared::Graphics::VideoPlayer::hasBackEndVideoPlayer() == true) {

        if (factionVideo != NULL) {
          factionVideo->closePlayer();
          delete factionVideo;
          factionVideo = NULL;
        }
        string introVideoFile = factionVideoUrl;
        string introVideoFileFallback = factionVideoUrlFallback;

        Context *c = GraphicsInterface::getInstance().getCurrentContext();
        SDL_Window *window = static_cast<ContextGl *>(c)
                                 ->getPlatformContextGlPtr()
                                 ->getScreenWindow();
        SDL_Surface *screen = static_cast<ContextGl *>(c)
                                  ->getPlatformContextGlPtr()
                                  ->getScreenSurface();

        string vlcPluginsPath =
            Config::getInstance().getString("VideoPlayerPluginsPath", "");
        // printf("screen->w = %d screen->h = %d screen->format->BitsPerPixel =
        // %d\n",screen->w,screen->h,screen->format->BitsPerPixel);
        factionVideo = new VideoPlayer(
            &Renderer::getInstance(), introVideoFile, introVideoFileFallback,
            window, 0, 0, screen->w, screen->h, screen->format->BitsPerPixel,
            true, vlcPluginsPath, SystemFlags::VERBOSE_MODE_ENABLED);
        factionVideo->initPlayer();
      }
    }
  } else {
    // SoundRenderer &soundRenderer= SoundRenderer::getInstance();
    // switch on music again!!
    Config &config = Config::getInstance();
    float configVolume = (config.getInt("SoundVolumeMusic") / 100.f);
    if (factionVideoSwitchedOffVolume) {
      if (CoreData::getInstance().getMenuMusic()->getVolume() != configVolume) {
        CoreData::getInstance().getMenuMusic()->setVolume(configVolume);
      }
      factionVideoSwitchedOffVolume = false;
    }

    if (factionVideo != NULL) {
      factionVideo->closePlayer();
      delete factionVideo;
      factionVideo = NULL;
    }
  }

  if (factionVideo == NULL) {
    string factionLogo = Game::findFactionLogoFile(
        gameSettings, NULL, GameConstants::PREVIEW_SCREEN_FILE_FILTER);
    if (factionLogo == "") {
      factionLogo = Game::findFactionLogoFile(gameSettings, NULL);
    }
    if (currentFactionLogo != factionLogo) {
      currentFactionLogo = factionLogo;
      loadFactionTexture(currentFactionLogo);
    }
  }
}

void MenuStateCustomGame::publishToMasterserver() {
  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem, "In [%s::%s Line %d]\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__);

  int slotCountUsed = 0;
  int slotCountHumans = 0;
  int slotCountConnectedPlayers = 0;
  ServerInterface *serverInterface =
      NetworkManager::getInstance().getServerInterface();
  GameSettings gameSettings;
  copyToGameSettings(&gameSettings);
  Config &config = Config::getInstance();
  // string serverinfo="";

  MutexSafeWrapper safeMutex(
      (publishToMasterserverThread != NULL
           ? publishToMasterserverThread->getMutexThreadObjectAccessor()
           : NULL),
      string(__FILE__) + "_" + intToStr(__LINE__));

  publishToServerInfo.clear();

  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem, "In [%s::%s Line %d]\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__);

  for (int i = 0; i < mapInfo.players; ++i) {
    if (listBoxControls[i].getSelectedItemIndex() != ctClosed) {
      slotCountUsed++;
    }

    if (listBoxControls[i].getSelectedItemIndex() == ctNetwork ||
        listBoxControls[i].getSelectedItemIndex() == ctNetworkUnassigned) {
      slotCountHumans++;
      if (serverInterface->getSlot(i, true) != NULL &&
          serverInterface->getSlot(i, true)->isConnected()) {
        slotCountConnectedPlayers++;
      }
    } else if (listBoxControls[i].getSelectedItemIndex() == ctHuman) {
      slotCountHumans++;
      slotCountConnectedPlayers++;
    }
  }

  publishToServerInfo["uuid"] = Config::getInstance().getString("PlayerId", "");

  //?status=waiting&system=linux&info=titus
  publishToServerInfo["glestVersion"] = glestVersionString;
  publishToServerInfo["platform"] =
      getPlatformNameString() + "-" + getGITRevisionString();
  publishToServerInfo["binaryCompileDate"] = getCompileDateTime();

  // game info:
  publishToServerInfo["serverTitle"] = gameSettings.getGameName();
  // ip is automatically set

  // game setup info:

  // publishToServerInfo["tech"] = listBoxTechTree.getSelectedItem();
  publishToServerInfo["tech"] =
      techTreeFiles[listBoxTechTree.getSelectedItemIndex()];
  // publishToServerInfo["map"] = listBoxMap.getSelectedItem();
  publishToServerInfo["map"] = getCurrentMapFile();
  // publishToServerInfo["tileset"] = listBoxTileset.getSelectedItem();
  publishToServerInfo["tileset"] =
      tilesetFiles[listBoxTileset.getSelectedItemIndex()];

  publishToServerInfo["activeSlots"] = intToStr(slotCountUsed);
  publishToServerInfo["networkSlots"] = intToStr(slotCountHumans);
  publishToServerInfo["connectedClients"] = intToStr(slotCountConnectedPlayers);

  string serverPort = config.getString(
      "PortServer", intToStr(GameConstants::serverPort).c_str());
  string externalPort = config.getString("PortExternal", serverPort.c_str());
  publishToServerInfo["externalconnectport"] = externalPort;
  publishToServerInfo["privacyPlease"] =
      intToStr(config.getBool("PrivacyPlease", "false"));

  publishToServerInfo["gameStatus"] = intToStr(game_status_waiting_for_players);
  if (slotCountHumans <= slotCountConnectedPlayers) {
    publishToServerInfo["gameStatus"] = intToStr(game_status_waiting_for_start);
  }

  publishToServerInfo["gameUUID"] = gameSettings.getGameUUID();
  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem, "In [%s::%s Line %d]\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__);
}

void MenuStateCustomGame::setupTask(BaseThread *callingThread, void *userdata) {
  if (SystemFlags::VERBOSE_MODE_ENABLED)
    printf("\n\nsetupTask callingThread [%p] userdata [%p]\n", callingThread,
           userdata);
  if (userdata != NULL) {
    int value = *((int *)&userdata);
    THREAD_NOTIFIER_TYPE threadType = (THREAD_NOTIFIER_TYPE)value;
    // printf("\n\nsetupTask callingThread [%p] userdata
    // [%p]\n",callingThread,userdata);
    if (threadType == tnt_MASTERSERVER) {
      MenuStateCustomGame::setupTaskStatic(callingThread);
    }
  }
}
void MenuStateCustomGame::shutdownTask(BaseThread *callingThread,
                                       void *userdata) {
  // printf("\n\nshutdownTask callingThread [%p] userdata
  // [%p]\n",callingThread,userdata);
  if (SystemFlags::VERBOSE_MODE_ENABLED)
    printf("\n\nshutdownTask callingThread [%p] userdata [%p]\n", callingThread,
           userdata);
  if (userdata != NULL) {
    int value = *((int *)&userdata);
    THREAD_NOTIFIER_TYPE threadType = (THREAD_NOTIFIER_TYPE)value;
    // printf("\n\nshutdownTask callingThread [%p] userdata
    // [%p]\n",callingThread,userdata);
    if (threadType == tnt_MASTERSERVER) {
      MenuStateCustomGame::shutdownTaskStatic(callingThread);
    }
  }
}
void MenuStateCustomGame::setupTaskStatic(BaseThread *callingThread) {
  if (SystemFlags::VERBOSE_MODE_ENABLED)
    printf("In [%s::%s Line %d]\n",
           extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
           __LINE__);

  CURL *handle = SystemFlags::initHTTP();
  callingThread->setGenericData<CURL>(handle);

  if (SystemFlags::VERBOSE_MODE_ENABLED)
    printf("In [%s::%s Line %d]\n",
           extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
           __LINE__);
}
void MenuStateCustomGame::shutdownTaskStatic(BaseThread *callingThread) {
  if (SystemFlags::VERBOSE_MODE_ENABLED)
    printf("In [%s::%s Line %d]\n",
           extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
           __LINE__);

  // printf("LINE: %d\n",__LINE__);
  CURL *handle = callingThread->getGenericData<CURL>();
  SystemFlags::cleanupHTTP(&handle);

  if (SystemFlags::VERBOSE_MODE_ENABLED)
    printf("In [%s::%s Line %d]\n",
           extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
           __LINE__);
}

void MenuStateCustomGame::simpleTask(BaseThread *callingThread,
                                     void *userdata) {
  // printf("\n\nSimple Task callingThread [%p] userdata
  // [%p]\n",callingThread,userdata);
  int value = *((int *)&userdata);
  // printf("\n\nSimple Task callingThread [%p] userdata [%p] value =
  // %d\n",callingThread,userdata,value);

  THREAD_NOTIFIER_TYPE threadType = (THREAD_NOTIFIER_TYPE)value;
  if (threadType == tnt_MASTERSERVER) {
    simpleTaskForMasterServer(callingThread);
  } else if (threadType == tnt_CLIENTS) {
    simpleTaskForClients(callingThread);
  }
}

void MenuStateCustomGame::simpleTaskForMasterServer(BaseThread *callingThread) {
  try {
    // printf("-=-=-=-=- IN MenuStateCustomGame simpleTask - A\n");

    MutexSafeWrapper safeMutexThreadOwner(
        callingThread->getMutexThreadOwnerValid(),
        string(__FILE__) + "_" + intToStr(__LINE__));
    if (callingThread->getQuitStatus() == true ||
        safeMutexThreadOwner.isValidMutex() == false) {
      return;
    }

    // printf("-=-=-=-=- IN MenuStateCustomGame simpleTask - B\n");

    MutexSafeWrapper safeMutex(callingThread->getMutexThreadObjectAccessor(),
                               string(__FILE__) + "_" + intToStr(__LINE__));
    bool republish = (needToRepublishToMasterserver == true &&
                      publishToServerInfo.empty() == false);
    needToRepublishToMasterserver = false;
    std::map<string, string> newPublishToServerInfo = publishToServerInfo;
    publishToServerInfo.clear();

    // printf("simpleTask broadCastSettings = %d\n",broadCastSettings);

    if (callingThread->getQuitStatus() == true) {
      return;
    }

    // printf("-=-=-=-=- IN MenuStateCustomGame simpleTask - C\n");

    if (republish == true) {
      if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
        SystemFlags::OutputDebug(SystemFlags::debugSystem,
                                 "In [%s::%s Line %d]\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__);

      string request = Config::getInstance().getString("Masterserver");
      if (request != "") {
        endPathWithSlash(request, false);
      }
      request += "addServerInfo.php?";

      // CURL *handle = SystemFlags::initHTTP();
      CURL *handle = callingThread->getGenericData<CURL>();

      int paramIndex = 0;
      for (std::map<string, string>::const_iterator iterMap =
               newPublishToServerInfo.begin();
           iterMap != newPublishToServerInfo.end(); ++iterMap) {

        request += iterMap->first;
        request += "=";
        request += SystemFlags::escapeURL(iterMap->second, handle);

        paramIndex++;
        if (paramIndex < (int)newPublishToServerInfo.size()) {
          request += "&";
        }
      }

      if (SystemFlags::VERBOSE_MODE_ENABLED)
        printf("The Lobby request is:\n%s\n", request.c_str());

      if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
        SystemFlags::OutputDebug(SystemFlags::debugSystem,
                                 "In [%s::%s Line %d] the request is:\n%s\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__, request.c_str());
      safeMutex.ReleaseLock(true);
      safeMutexThreadOwner.ReleaseLock();

      std::string serverInfo = SystemFlags::getHTTP(request, handle);
      // SystemFlags::cleanupHTTP(&handle);

      MutexSafeWrapper safeMutexThreadOwner2(
          callingThread->getMutexThreadOwnerValid(),
          string(__FILE__) + "_" + intToStr(__LINE__));
      if (callingThread->getQuitStatus() == true ||
          safeMutexThreadOwner2.isValidMutex() == false) {
        return;
      }
      safeMutex.Lock();

      // printf("the result is:\n'%s'\n",serverInfo.c_str());
      if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
        SystemFlags::OutputDebug(SystemFlags::debugSystem,
                                 "In [%s::%s Line %d] the result is:\n'%s'\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__, serverInfo.c_str());

      // uncomment to enable router setup check of this server
      if (EndsWith(serverInfo, "OK") == false) {
        if (callingThread->getQuitStatus() == true) {
          return;
        }

        // Give things another chance to see if we can get a connection from the
        // master server
        if (tMasterserverErrorElapsed > 0 &&
            difftime((long int)time(NULL), tMasterserverErrorElapsed) >
                MASTERSERVER_BROADCAST_MAX_WAIT_RESPONSE_SECONDS) {
          showMasterserverError = true;
          masterServererErrorToShow =
              (serverInfo != "" ? serverInfo : "No Reply");
        } else {
          if (tMasterserverErrorElapsed == 0) {
            tMasterserverErrorElapsed = time(NULL);
          }

          SystemFlags::OutputDebug(
              SystemFlags::debugError,
              "In [%s::%s Line %d] error checking response from masterserver "
              "elapsed seconds = %.2f / %d\nResponse:\n%s\n",
              extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
              __LINE__,
              difftime((long int)time(NULL), tMasterserverErrorElapsed),
              MASTERSERVER_BROADCAST_MAX_WAIT_RESPONSE_SECONDS,
              serverInfo.c_str());

          needToRepublishToMasterserver = true;
        }
      }
    } else {
      safeMutexThreadOwner.ReleaseLock();
    }

    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem,
                               "In [%s::%s Line %d]\n",
                               extractFileFromDirectoryPath(__FILE__).c_str(),
                               __FUNCTION__, __LINE__);

    // printf("-=-=-=-=- IN MenuStateCustomGame simpleTask - D\n");

    safeMutex.ReleaseLock();

    // printf("-=-=-=-=- IN MenuStateCustomGame simpleTask - F\n");
  } catch (const std::exception &ex) {
    char szBuf[8096] = "";
    snprintf(szBuf, 8096, "In [%s::%s %d]\nError detected:\n%s\n",
             extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
             __LINE__, ex.what());
    SystemFlags::OutputDebug(SystemFlags::debugError, szBuf);
    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem, "%s", szBuf);

    if (callingThread->getQuitStatus() == false) {
      // throw megaglest_runtime_error(szBuf);
      showGeneralError = true;
      generalErrorToShow = ex.what();
    }
  }

  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem, "In [%s::%s Line %d]\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__);
}

void MenuStateCustomGame::simpleTaskForClients(BaseThread *callingThread) {
  try {
    // printf("-=-=-=-=- IN MenuStateCustomGame simpleTask - A\n");

    MutexSafeWrapper safeMutexThreadOwner(
        callingThread->getMutexThreadOwnerValid(),
        string(__FILE__) + "_" + intToStr(__LINE__));
    if (callingThread->getQuitStatus() == true ||
        safeMutexThreadOwner.isValidMutex() == false) {
      return;
    }

    // printf("-=-=-=-=- IN MenuStateCustomGame simpleTask - B\n");

    MutexSafeWrapper safeMutex(callingThread->getMutexThreadObjectAccessor(),
                               string(__FILE__) + "_" + intToStr(__LINE__));
    bool broadCastSettings = needToBroadcastServerSettings;

    // printf("simpleTask broadCastSettings = %d\n",broadCastSettings);

    needToBroadcastServerSettings = false;
    bool hasClientConnection = false;

    if (broadCastSettings == true) {
      ServerInterface *serverInterface =
          NetworkManager::getInstance().getServerInterface(false);
      if (serverInterface != NULL) {
        hasClientConnection = serverInterface->hasClientConnection();
      }
    }
    bool needPing = (difftime((long int)time(NULL), lastNetworkPing) >=
                     GameConstants::networkPingInterval);

    if (callingThread->getQuitStatus() == true) {
      return;
    }

    // printf("-=-=-=-=- IN MenuStateCustomGame simpleTask - C\n");

    safeMutexThreadOwner.ReleaseLock();

    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem,
                               "In [%s::%s Line %d]\n",
                               extractFileFromDirectoryPath(__FILE__).c_str(),
                               __FUNCTION__, __LINE__);

    // printf("-=-=-=-=- IN MenuStateCustomGame simpleTask - D\n");

    if (broadCastSettings == true) {
      MutexSafeWrapper safeMutexThreadOwner2(
          callingThread->getMutexThreadOwnerValid(),
          string(__FILE__) + "_" + intToStr(__LINE__));
      if (callingThread->getQuitStatus() == true ||
          safeMutexThreadOwner2.isValidMutex() == false) {
        return;
      }

      if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
        SystemFlags::OutputDebug(SystemFlags::debugSystem,
                                 "In [%s::%s Line %d]\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__);

      // printf("simpleTask broadCastSettings = %d hasClientConnection =
      // %d\n",broadCastSettings,hasClientConnection);

      if (callingThread->getQuitStatus() == true) {
        return;
      }
      ServerInterface *serverInterface =
          NetworkManager::getInstance().getServerInterface(false);
      if (serverInterface != NULL) {
        lastGameSettingsreceivedCount++;
        if (this->headlessServerMode == false ||
            (serverInterface->getGameSettingsUpdateCount() <=
             lastMasterServerSettingsUpdateCount)) {
          GameSettings gameSettings;
          copyToGameSettings(&gameSettings);

          // printf("\n\n\n\n=====#2 got settings [%d]
          // [%d]:\n%s\n",lastMasterServerSettingsUpdateCount,serverInterface->getGameSettingsUpdateCount(),gameSettings.toString().c_str());

          if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem)
                  .enabled)
            SystemFlags::OutputDebug(
                SystemFlags::debugSystem, "In [%s::%s Line %d]\n",
                extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                __LINE__);

          serverInterface->setGameSettings(&gameSettings, false);
          lastMasterServerSettingsUpdateCount =
              serverInterface->getGameSettingsUpdateCount();

          if (hasClientConnection == true) {
            if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem)
                    .enabled)
              SystemFlags::OutputDebug(
                  SystemFlags::debugSystem, "In [%s::%s Line %d]\n",
                  extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                  __LINE__);
            serverInterface->broadcastGameSetup(&gameSettings);
          }
        }
      }
    }

    // printf("-=-=-=-=- IN MenuStateCustomGame simpleTask - E\n");

    if (needPing == true) {
      MutexSafeWrapper safeMutexThreadOwner2(
          callingThread->getMutexThreadOwnerValid(),
          string(__FILE__) + "_" + intToStr(__LINE__));
      if (callingThread->getQuitStatus() == true ||
          safeMutexThreadOwner2.isValidMutex() == false) {
        return;
      }

      lastNetworkPing = time(NULL);

      if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
        SystemFlags::OutputDebug(
            SystemFlags::debugSystem,
            "In [%s::%s Line %d] Sending nmtPing to clients\n",
            extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
            __LINE__);

      ServerInterface *serverInterface =
          NetworkManager::getInstance().getServerInterface(false);
      if (serverInterface != NULL) {
        if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
          SystemFlags::OutputDebug(
              SystemFlags::debugSystem, "In [%s::%s Line %d]\n",
              extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
              __LINE__);
        NetworkMessagePing *msg = new NetworkMessagePing(
            GameConstants::networkPingInterval, time(NULL));
        // serverInterface->broadcastPing(&msg);
        serverInterface->queueBroadcastMessage(msg);
        if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
          SystemFlags::OutputDebug(
              SystemFlags::debugSystem, "In [%s::%s Line %d]\n",
              extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
              __LINE__);
      }
    }
    safeMutex.ReleaseLock();

    // printf("-=-=-=-=- IN MenuStateCustomGame simpleTask - F\n");
  } catch (const std::exception &ex) {
    char szBuf[8096] = "";
    snprintf(szBuf, 8096, "In [%s::%s %d]\nError detected:\n%s\n",
             extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
             __LINE__, ex.what());
    SystemFlags::OutputDebug(SystemFlags::debugError, szBuf);
    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem, "%s", szBuf);

    if (callingThread->getQuitStatus() == false) {
      // throw megaglest_runtime_error(szBuf);
      showGeneralError = true;
      generalErrorToShow = ex.what();
    }
  }

  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem, "In [%s::%s Line %d]\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__);
}

void MenuStateCustomGame::copyToGameSettings(GameSettings *gameSettings,
                                             bool forceCloseUnusedSlots) {
  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem, "In [%s::%s Line: %d\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__);

  int factionCount = 0;
  ServerInterface *serverInterface =
      NetworkManager::getInstance().getServerInterface();

  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem, "In [%s::%s] Line: %d\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__);

  if (this->headlessServerMode == true &&
      serverInterface->getGameSettingsUpdateCount() >
          lastMasterServerSettingsUpdateCount &&
      serverInterface->getGameSettings() != NULL) {
    const GameSettings *settings = serverInterface->getGameSettings();
    // printf("\n\n\n\n=====#3 got settings [%d]
    // [%d]:\n%s\n",lastMasterServerSettingsUpdateCount,serverInterface->getGameSettingsUpdateCount(),settings->toString().c_str());

    lastMasterServerSettingsUpdateCount =
        serverInterface->getGameSettingsUpdateCount();
    // printf("#1 custom menu got map [%s]\n",settings->getMap().c_str());

    setupUIFromGameSettings(*settings);
  }

  gameSettings->setGameName(labelGameName.getText());

  // Test flags values
  // gameSettings->setFlagTypes1(ft1_show_map_resources);
  //

  if (checkBoxScenario.getValue() == true) {
    gameSettings->setScenario(scenarioInfo.name);
    gameSettings->setScenarioDir(
        Scenario::getScenarioPath(dirList, scenarioInfo.name));

    gameSettings->setDefaultResources(scenarioInfo.defaultResources);
    gameSettings->setDefaultUnits(scenarioInfo.defaultUnits);
    gameSettings->setDefaultVictoryConditions(
        scenarioInfo.defaultVictoryConditions);
  } else {
    gameSettings->setScenario("");
    gameSettings->setScenarioDir("");
  }

  gameSettings->setGameUUID(this->gameUUID);

  // printf("scenarioInfo.name [%s] [%s]
  // [%s]\n",scenarioInfo.name.c_str(),listBoxMap.getSelectedItem().c_str(),getCurrentMapFile().c_str());

  gameSettings->setMapFilter(listBoxMapFilter.getSelectedItemIndex());
  gameSettings->setDescription(formatString(getCurrentMapFile()));
  gameSettings->setMap(getCurrentMapFile());
  if (tilesetFiles.empty() == false) {
    gameSettings->setTileset(
        tilesetFiles[listBoxTileset.getSelectedItemIndex()]);
  }
  if (techTreeFiles.empty() == false) {
    gameSettings->setTech(
        techTreeFiles[listBoxTechTree.getSelectedItemIndex()]);
  }

  if (autoStartSettings != NULL) {
    gameSettings->setDefaultUnits(autoStartSettings->getDefaultUnits());
    gameSettings->setDefaultResources(autoStartSettings->getDefaultResources());
    gameSettings->setDefaultVictoryConditions(
        autoStartSettings->getDefaultVictoryConditions());
  } else if (checkBoxScenario.getValue() == false) {
    gameSettings->setDefaultUnits(true);
    gameSettings->setDefaultResources(true);
    gameSettings->setDefaultVictoryConditions(true);
  }

  gameSettings->setFogOfWar(listBoxFogOfWar.getSelectedItemIndex() == 0 ||
                            listBoxFogOfWar.getSelectedItemIndex() == 1);

  gameSettings->setAllowObservers(checkBoxAllowObservers.getValue() == 1);

  uint32 valueFlags1 = gameSettings->getFlagTypes1();
  if (listBoxFogOfWar.getSelectedItemIndex() == 1 ||
      listBoxFogOfWar.getSelectedItemIndex() == 2) {
    valueFlags1 |= ft1_show_map_resources;
    gameSettings->setFlagTypes1(valueFlags1);
  } else {
    valueFlags1 &= ~ft1_show_map_resources;
    gameSettings->setFlagTypes1(valueFlags1);
  }

  // gameSettings->setEnableObserverModeAtEndGame(listBoxEnableObserverMode.getSelectedItemIndex()
  // == 0);
  gameSettings->setEnableObserverModeAtEndGame(true);
  // gameSettings->setPathFinderType(static_cast<PathFinderType>(listBoxPathFinderType.getSelectedItemIndex()));

  valueFlags1 = gameSettings->getFlagTypes1();
  if (checkBoxEnableSwitchTeamMode.getValue() == true) {
    valueFlags1 |= ft1_allow_team_switching;
    gameSettings->setFlagTypes1(valueFlags1);
  } else {
    valueFlags1 &= ~ft1_allow_team_switching;
    gameSettings->setFlagTypes1(valueFlags1);
  }
  gameSettings->setAiAcceptSwitchTeamPercentChance(
      strToInt(listBoxAISwitchTeamAcceptPercent.getSelectedItem()));
  gameSettings->setFallbackCpuMultiplier(
      listBoxFallbackCpuMultiplier.getSelectedItemIndex());

  if (checkBoxAllowInGameJoinPlayer.getValue() == true) {
    valueFlags1 |= ft1_allow_in_game_joining;
    gameSettings->setFlagTypes1(valueFlags1);
  } else {
    valueFlags1 &= ~ft1_allow_in_game_joining;
    gameSettings->setFlagTypes1(valueFlags1);
  }

  if (checkBoxAllowTeamUnitSharing.getValue() == true) {
    valueFlags1 |= ft1_allow_shared_team_units;
    gameSettings->setFlagTypes1(valueFlags1);
  } else {
    valueFlags1 &= ~ft1_allow_shared_team_units;
    gameSettings->setFlagTypes1(valueFlags1);
  }

  if (checkBoxAllowTeamResourceSharing.getValue() == true) {
    valueFlags1 |= ft1_allow_shared_team_resources;
    gameSettings->setFlagTypes1(valueFlags1);
  } else {
    valueFlags1 &= ~ft1_allow_shared_team_resources;
    gameSettings->setFlagTypes1(valueFlags1);
  }

  if (Config::getInstance().getBool("EnableNetworkGameSynchChecks", "false") ==
      true) {
    // printf("*WARNING* - EnableNetworkGameSynchChecks is enabled\n");

    valueFlags1 |= ft1_network_synch_checks_verbose;
    gameSettings->setFlagTypes1(valueFlags1);

  } else {
    valueFlags1 &= ~ft1_network_synch_checks_verbose;
    gameSettings->setFlagTypes1(valueFlags1);
  }
  if (Config::getInstance().getBool("EnableNetworkGameSynchMonitor", "false") ==
      true) {
    // printf("*WARNING* - EnableNetworkGameSynchChecks is enabled\n");

    valueFlags1 |= ft1_network_synch_checks;
    gameSettings->setFlagTypes1(valueFlags1);

  } else {
    valueFlags1 &= ~ft1_network_synch_checks;
    gameSettings->setFlagTypes1(valueFlags1);
  }

  gameSettings->setNetworkAllowNativeLanguageTechtree(
      checkBoxAllowNativeLanguageTechtree.getValue());

  // First save Used slots
  // for(int i=0; i<mapInfo.players; ++i)
  int AIPlayerCount = 0;
  for (int i = 0; i < GameConstants::maxPlayers; ++i) {
    if (listBoxControls[i].getSelectedItemIndex() == ctHuman &&
        this->headlessServerMode == true) {
      // switch slot to network, because no human in headless mode
      listBoxControls[i].setSelectedItemIndex(ctNetwork);
      updateResourceMultiplier(i);
    }

    ControlType ct =
        static_cast<ControlType>(listBoxControls[i].getSelectedItemIndex());

    if (forceCloseUnusedSlots == true &&
        (ct == ctNetworkUnassigned || ct == ctNetwork)) {
      if (serverInterface != NULL &&
          (serverInterface->getSlot(i, true) == NULL ||
           serverInterface->getSlot(i, true)->isConnected() == false)) {
        if (checkBoxScenario.getValue() == false) {
          // printf("Closed A [%d]
          // [%s]\n",i,labelPlayerNames[i].getText().c_str());

          listBoxControls[i].setSelectedItemIndex(ctClosed);
          ct = ctClosed;
        }
      }
    } else if (ct == ctNetworkUnassigned && i < mapInfo.players) {
      listBoxControls[i].setSelectedItemIndex(ctNetwork);
      ct = ctNetwork;
    }

    if (ct != ctClosed) {
      int slotIndex = factionCount;
      gameSettings->setFactionControl(slotIndex, ct);
      if (ct == ctHuman) {
        if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
          SystemFlags::OutputDebug(
              SystemFlags::debugSystem,
              "In [%s::%s Line: %d] i = %d, slotIndex = %d, "
              "getHumanPlayerName(i) [%s]\n",
              extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
              __LINE__, i, slotIndex, getHumanPlayerName(i).c_str());

        gameSettings->setThisFactionIndex(slotIndex);
        gameSettings->setNetworkPlayerName(slotIndex, getHumanPlayerName(i));
        gameSettings->setNetworkPlayerUUID(
            slotIndex, Config::getInstance().getString("PlayerId", ""));
        gameSettings->setNetworkPlayerPlatform(slotIndex,
                                               getPlatformNameString());
        gameSettings->setNetworkPlayerStatuses(slotIndex,
                                               getNetworkPlayerStatus());
        Lang &lang = Lang::getInstance();
        gameSettings->setNetworkPlayerLanguages(slotIndex, lang.getLanguage());
      } else if (serverInterface != NULL &&
                 serverInterface->getSlot(i, true) != NULL) {
        gameSettings->setNetworkPlayerLanguages(
            slotIndex,
            serverInterface->getSlot(i, true)->getNetworkPlayerLanguage());
      }

      // if(slotIndex == 0) printf("slotIndex = %d, i = %d, multiplier =
      // %d\n",slotIndex,i,listBoxRMultiplier[i].getSelectedItemIndex());

      // printf("Line: %d multiplier index: %d slotIndex:
      // %d\n",__LINE__,listBoxRMultiplier[i].getSelectedItemIndex(),slotIndex);
      gameSettings->setResourceMultiplierIndex(
          slotIndex, listBoxRMultiplier[i].getSelectedItemIndex());
      // printf("Line: %d multiplier index: %d slotIndex:
      // %d\n",__LINE__,gameSettings->getResourceMultiplierIndex(slotIndex),slotIndex);

      if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
        SystemFlags::OutputDebug(
            SystemFlags::debugSystem,
            "In [%s::%s Line: %d] i = %d, "
            "factionFiles[listBoxFactions[i].getSelectedItemIndex()] [%s]\n",
            extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
            __LINE__, i,
            factionFiles[listBoxFactions[i].getSelectedItemIndex()].c_str());

      gameSettings->setFactionTypeName(
          slotIndex, factionFiles[listBoxFactions[i].getSelectedItemIndex()]);
      if (factionFiles[listBoxFactions[i].getSelectedItemIndex()] ==
          formatString(GameConstants::OBSERVER_SLOTNAME)) {
        listBoxTeams[i].setSelectedItem(
            intToStr(GameConstants::maxPlayers + fpt_Observer));
      } else if (listBoxTeams[i].getSelectedItem() ==
                 intToStr(GameConstants::maxPlayers + fpt_Observer)) {

        // printf("Line: %d lastSelectedTeamIndex[i] = %d
        // \n",__LINE__,lastSelectedTeamIndex[i]);

        if ((listBoxControls[i].getSelectedItemIndex() == ctCpuEasy ||
             listBoxControls[i].getSelectedItemIndex() == ctCpu ||
             listBoxControls[i].getSelectedItemIndex() == ctCpuUltra ||
             listBoxControls[i].getSelectedItemIndex() == ctCpuMega) &&
            checkBoxScenario.getValue() == true) {

        } else {
          if (lastSelectedTeamIndex[i] >= 0 &&
              lastSelectedTeamIndex[i] + 1 !=
                  (GameConstants::maxPlayers + fpt_Observer)) {
            if (lastSelectedTeamIndex[i] == 0) {
              lastSelectedTeamIndex[i] = GameConstants::maxPlayers - 1;
            } else if (lastSelectedTeamIndex[i] ==
                       GameConstants::maxPlayers - 1) {
              lastSelectedTeamIndex[i] = 0;
            }

            listBoxTeams[i].setSelectedItemIndex(lastSelectedTeamIndex[i]);
          } else {
            listBoxTeams[i].setSelectedItem(intToStr(1));
          }
        }
      }

      gameSettings->setTeam(slotIndex, listBoxTeams[i].getSelectedItemIndex());
      gameSettings->setStartLocationIndex(slotIndex, i);

      if (listBoxControls[i].getSelectedItemIndex() == ctNetwork ||
          listBoxControls[i].getSelectedItemIndex() == ctNetworkUnassigned) {
        if (serverInterface != NULL &&
            serverInterface->getSlot(i, true) != NULL &&
            serverInterface->getSlot(i, true)->isConnected()) {

          gameSettings->setNetworkPlayerStatuses(
              slotIndex,
              serverInterface->getSlot(i, true)->getNetworkPlayerStatus());

          if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem)
                  .enabled)
            SystemFlags::OutputDebug(
                SystemFlags::debugSystem,
                "In [%s::%s Line: %d] i = %d, connectionSlot->getName() [%s]\n",
                extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                __LINE__, i,
                serverInterface->getSlot(i, true)->getName().c_str());

          gameSettings->setNetworkPlayerName(
              slotIndex, serverInterface->getSlot(i, true)->getName());
          gameSettings->setNetworkPlayerUUID(
              i, serverInterface->getSlot(i, true)->getUUID());
          gameSettings->setNetworkPlayerPlatform(
              i, serverInterface->getSlot(i, true)->getPlatform());
          labelPlayerNames[i].setText(
              serverInterface->getSlot(i, true)->getName());
        } else {
          if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem)
                  .enabled)
            SystemFlags::OutputDebug(
                SystemFlags::debugSystem,
                "In [%s::%s Line: %d] i = %d, playername unconnected\n",
                extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                __LINE__, i);

          gameSettings->setNetworkPlayerName(
              slotIndex, GameConstants::NETWORK_SLOT_UNCONNECTED_SLOTNAME);
          labelPlayerNames[i].setText("");
        }
      } else if (listBoxControls[i].getSelectedItemIndex() != ctHuman) {
        AIPlayerCount++;
        if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
          SystemFlags::OutputDebug(
              SystemFlags::debugSystem,
              "In [%s::%s Line: %d] i = %d, playername is AI (blank)\n",
              extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
              __LINE__, i);

        Lang &lang = Lang::getInstance();
        gameSettings->setNetworkPlayerName(
            slotIndex, lang.getString("AI") + intToStr(AIPlayerCount));
        labelPlayerNames[i].setText("");
      }
      if (listBoxControls[i].getSelectedItemIndex() == ctHuman) {
        setSlotHuman(i);
      }
      if (serverInterface != NULL &&
          serverInterface->getSlot(i, true) != NULL) {
        gameSettings->setNetworkPlayerUUID(
            slotIndex, serverInterface->getSlot(i, true)->getUUID());
        gameSettings->setNetworkPlayerPlatform(
            slotIndex, serverInterface->getSlot(i, true)->getPlatform());
      }

      factionCount++;
    } else {
      // gameSettings->setNetworkPlayerName("");
      gameSettings->setNetworkPlayerStatuses(factionCount, npst_None);
      labelPlayerNames[i].setText("");
    }
  }

  // Next save closed slots
  int closedCount = 0;
  for (int i = 0; i < GameConstants::maxPlayers; ++i) {
    ControlType ct =
        static_cast<ControlType>(listBoxControls[i].getSelectedItemIndex());
    if (ct == ctClosed) {
      int slotIndex = factionCount + closedCount;

      gameSettings->setFactionControl(slotIndex, ct);
      gameSettings->setTeam(slotIndex, listBoxTeams[i].getSelectedItemIndex());
      gameSettings->setStartLocationIndex(slotIndex, i);
      // gameSettings->setResourceMultiplierIndex(slotIndex, 10);
      listBoxRMultiplier[i].setSelectedItem("1.0");
      gameSettings->setResourceMultiplierIndex(
          slotIndex, listBoxRMultiplier[i].getSelectedItemIndex());
      // printf("Test multiplier =
      // %s\n",listBoxRMultiplier[i].getSelectedItem().c_str());

      if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
        SystemFlags::OutputDebug(
            SystemFlags::debugSystem,
            "In [%s::%s Line: %d] i = %d, "
            "factionFiles[listBoxFactions[i].getSelectedItemIndex()] [%s]\n",
            extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
            __LINE__, i,
            factionFiles[listBoxFactions[i].getSelectedItemIndex()].c_str());

      gameSettings->setFactionTypeName(
          slotIndex, factionFiles[listBoxFactions[i].getSelectedItemIndex()]);
      gameSettings->setNetworkPlayerName(
          slotIndex, GameConstants::NETWORK_SLOT_CLOSED_SLOTNAME);
      gameSettings->setNetworkPlayerUUID(slotIndex, "");
      gameSettings->setNetworkPlayerPlatform(slotIndex, "");

      closedCount++;
    }
  }

  gameSettings->setFactionCount(factionCount);

  Config &config = Config::getInstance();
  gameSettings->setEnableServerControlledAI(
      config.getBool("ServerControlledAI", "true"));
  gameSettings->setNetworkFramePeriod(
      config.getInt("NetworkSendFrameCount", "20"));
  gameSettings->setNetworkPauseGameForLaggedClients(
      ((checkBoxNetworkPauseGameForLaggedClients.getValue() == true)));

  setCRCsToGameSettings(gameSettings);

  if (this->headlessServerMode == true) {
    time_t clientConnectedTime = 0;
    bool masterserver_admin_found = false;
    // printf("mapInfo.players [%d]\n",mapInfo.players);

    for (int i = 0; i < mapInfo.players; ++i) {
      if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
        SystemFlags::OutputDebug(SystemFlags::debugSystem,
                                 "In [%s::%s Line %d]\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__);

      if (listBoxControls[i].getSelectedItemIndex() == ctNetwork ||
          listBoxControls[i].getSelectedItemIndex() == ctNetworkUnassigned) {
        if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
          SystemFlags::OutputDebug(
              SystemFlags::debugSystem, "In [%s::%s Line %d]\n",
              extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
              __LINE__);

        if (serverInterface->getSlot(i, true) != NULL &&
            serverInterface->getSlot(i, true)->isConnected()) {
          if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem)
                  .enabled)
            SystemFlags::OutputDebug(
                SystemFlags::debugSystem, "In [%s::%s Line %d]\n",
                extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                __LINE__);

          // printf("slot = %d serverInterface->getSlot(i)->getConnectedTime() =
          // %d session key
          // [%d]\n",i,serverInterface->getSlot(i)->getConnectedTime(),serverInterface->getSlot(i)->getSessionKey());

          if (clientConnectedTime == 0 ||
              (serverInterface->getSlot(i, true)->getConnectedTime() > 0 &&
               serverInterface->getSlot(i, true)->getConnectedTime() <
                   clientConnectedTime)) {
            clientConnectedTime =
                serverInterface->getSlot(i, true)->getConnectedTime();
            gameSettings->setMasterserver_admin(
                serverInterface->getSlot(i, true)->getSessionKey());
            gameSettings->setMasterserver_admin_faction_index(
                serverInterface->getSlot(i, true)->getPlayerIndex());
            labelGameName.setText(
                createGameName(serverInterface->getSlot(i, true)->getName()));
            // printf("slot = %d, admin key [%d] slot connected time["
            // MG_SIZE_T_SPECIFIER "] clientConnectedTime [" MG_SIZE_T_SPECIFIER
            // "]\n",i,gameSettings->getMasterserver_admin(),serverInterface->getSlot(i)->getConnectedTime(),clientConnectedTime);
          }
          if (serverInterface->getSlot(i, true)->getSessionKey() ==
              gameSettings->getMasterserver_admin()) {
            masterserver_admin_found = true;
          }
        }
      }
    }
    if (masterserver_admin_found == false) {
      for (int i = mapInfo.players; i < GameConstants::maxPlayers; ++i) {
        ServerInterface *serverInterface =
            NetworkManager::getInstance().getServerInterface();
        // ConnectionSlot *slot = serverInterface->getSlot(i);

        if (serverInterface->getSlot(i, true) != NULL &&
            serverInterface->getSlot(i, true)->isConnected()) {
          if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem)
                  .enabled)
            SystemFlags::OutputDebug(
                SystemFlags::debugSystem, "In [%s::%s Line %d]\n",
                extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                __LINE__);

          // printf("slot = %d serverInterface->getSlot(i)->getConnectedTime() =
          // %d session key
          // [%d]\n",i,serverInterface->getSlot(i)->getConnectedTime(),serverInterface->getSlot(i)->getSessionKey());

          if (clientConnectedTime == 0 ||
              (serverInterface->getSlot(i, true)->getConnectedTime() > 0 &&
               serverInterface->getSlot(i, true)->getConnectedTime() <
                   clientConnectedTime)) {
            clientConnectedTime =
                serverInterface->getSlot(i, true)->getConnectedTime();
            gameSettings->setMasterserver_admin(
                serverInterface->getSlot(i, true)->getSessionKey());
            gameSettings->setMasterserver_admin_faction_index(
                serverInterface->getSlot(i, true)->getPlayerIndex());
            labelGameName.setText(
                createGameName(serverInterface->getSlot(i, true)->getName()));
            // printf("slot = %d, admin key [%d] slot connected time["
            // MG_SIZE_T_SPECIFIER "] clientConnectedTime [" MG_SIZE_T_SPECIFIER
            // "]\n",i,gameSettings->getMasterserver_admin(),serverInterface->getSlot(i)->getConnectedTime(),clientConnectedTime);
          }
          if (serverInterface->getSlot(i, true)->getSessionKey() ==
              gameSettings->getMasterserver_admin()) {
            masterserver_admin_found = true;
          }
        }
      }
    }

    if (masterserver_admin_found == false) {
      labelGameName.setText(createGameName());
    }
  }

  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem, "In [%s::%s] Line: %d\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__);
}

void MenuStateCustomGame::KeepCurrentHumanPlayerSlots(
    GameSettings &gameSettings) {
  // look for human players
  bool foundValidHumanControlTypeInFile = false;
  for (int index2 = 0; index2 < GameConstants::maxPlayers; ++index2) {
    ControlType ctFile =
        static_cast<ControlType>(gameSettings.getFactionControl(index2));
    if (ctFile == ctHuman) {
      ControlType ctUI = static_cast<ControlType>(
          listBoxControls[index2].getSelectedItemIndex());
      if (ctUI != ctNetwork && ctUI != ctNetworkUnassigned) {
        foundValidHumanControlTypeInFile = true;
        // printf("Human found in file [%d]\n",index2);
      } else if (labelPlayerNames[index2].getText() == "") {
        foundValidHumanControlTypeInFile = true;
      }
    }
  }

  for (int index = 0; index < GameConstants::maxPlayers; ++index) {
    ControlType ct =
        static_cast<ControlType>(listBoxControls[index].getSelectedItemIndex());
    if (ct == ctHuman) {
      // printf("Human found in UI [%d] and file
      // [%d]\n",index,foundControlType);

      if (foundValidHumanControlTypeInFile == false) {
        gameSettings.setFactionControl(index, ctHuman);
        gameSettings.setNetworkPlayerName(index, getHumanPlayerName());
      }
    }

    ControlType ctFile =
        static_cast<ControlType>(gameSettings.getFactionControl(index));
    if (ctFile == ctHuman) {
      gameSettings.setFactionControl(index, ctHuman);
    }
  }
}

void MenuStateCustomGame::saveGameSettings(std::string fileName) {
  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem, "In [%s::%s] Line: %d\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__);

  GameSettings gameSettings;
  copyToGameSettings(&gameSettings);
  CoreData::getInstance().saveGameSettingsToFile(fileName, &gameSettings,
                                                 checkBoxAdvanced.getValue());

  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem, "In [%s::%s] Line: %d\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__);
}

bool MenuStateCustomGame::loadGameSettingsFromFile(GameSettings *gameSettings,
                                                   std::string fileName) {
  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem, "In [%s::%s] Line: %d\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__);

  GameSettings originalGameSettings;
  copyToGameSettings(&originalGameSettings);

  try {
    bool loadSuccessful = CoreData::getInstance().loadGameSettingsFromFile(
        fileName, gameSettings);
    if (!loadSuccessful) {
      console.addLine("Cannot load '" + fileName + "'");
      // do nothing on failure
      return false;
    }
    KeepCurrentHumanPlayerSlots(*gameSettings);

    // correct game settings for headless:
    if (this->headlessServerMode == true) {
      for (int i = 0; i < GameConstants::maxPlayers; ++i) {
        if (gameSettings->getFactionControl(i) == ctHuman) {
          gameSettings->setFactionControl(i, ctNetwork);
        }
      }
    }

    vector<string> mapsV = playerSortedMaps[0];
    if (std::find(mapsV.begin(), mapsV.end(), gameSettings->getMap()) ==
        mapsV.end()) {
      console.addLine("Cannot load '" + fileName + "', map unknown ('" +
                      gameSettings->getMap() + "')");
      return false; // map unknown
    }
    if (std::find(tilesetFiles.begin(), tilesetFiles.end(),
                  gameSettings->getTileset()) == tilesetFiles.end()) {
      console.addLine("Cannot load '" + fileName + "', tileset unknown ('" +
                      gameSettings->getTileset() + "')");
      return false; // tileset unknown
    }
    if (std::find(techTreeFiles.begin(), techTreeFiles.end(),
                  gameSettings->getTech()) == techTreeFiles.end()) {
      console.addLine("Cannot load '" + fileName + "', techtree unknown ('" +
                      gameSettings->getTech() + "')");
      return false; // techtree unknown
    }

    setupUIFromGameSettings(*gameSettings);
  } catch (const exception &ex) {
    SystemFlags::OutputDebug(SystemFlags::debugError,
                             "In [%s::%s Line: %d] Error [%s]\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__, ex.what());
    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem,
                               "In [%s::%s Line: %d] ERROR = [%s]\n",
                               extractFileFromDirectoryPath(__FILE__).c_str(),
                               __FUNCTION__, __LINE__, ex.what());

    showMessageBox(ex.what(), "Error", false);

    setupUIFromGameSettings(originalGameSettings);
    *gameSettings = originalGameSettings;
  }

  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem, "In [%s::%s] Line: %d\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__);

  return true;
}

void MenuStateCustomGame::setupUIFromGameSettings(
    const GameSettings &gameSettings) {
  string humanPlayerName = getHumanPlayerName();

  string scenarioDir = "";
  checkBoxScenario.setValue((gameSettings.getScenario() != ""));
  if (checkBoxScenario.getValue() == true) {
    listBoxScenario.setSelectedItem(formatString(gameSettings.getScenario()));

    loadScenarioInfo(
        Scenario::getScenarioPath(
            dirList, scenarioFiles[listBoxScenario.getSelectedItemIndex()]),
        &scenarioInfo);
    scenarioDir = Scenario::getScenarioDir(dirList, gameSettings.getScenario());

    // printf("scenarioInfo.fogOfWar = %d scenarioInfo.fogOfWar_exploredFlag =
    // %d\n",scenarioInfo.fogOfWar,scenarioInfo.fogOfWar_exploredFlag);
    if (scenarioInfo.fogOfWar == false &&
        scenarioInfo.fogOfWar_exploredFlag == false) {
      listBoxFogOfWar.setSelectedItemIndex(2);
    } else if (scenarioInfo.fogOfWar_exploredFlag == true) {
      listBoxFogOfWar.setSelectedItemIndex(1);
    } else {
      listBoxFogOfWar.setSelectedItemIndex(0);
    }
    checkBoxAllowTeamUnitSharing.setValue(scenarioInfo.allowTeamUnitSharing);
    checkBoxAllowTeamResourceSharing.setValue(
        scenarioInfo.allowTeamResourceSharing);
  }
  setupMapList(gameSettings.getScenario());
  setupTechList(gameSettings.getScenario(), false);
  setupTilesetList(gameSettings.getScenario());

  if (checkBoxScenario.getValue() == true) {
    // string file = Scenario::getScenarioPath(dirList,
    // gameSettings.getScenario()); loadScenarioInfo(file, &scenarioInfo);

    // printf("#6.1 about to load map [%s]\n",scenarioInfo.mapName.c_str());
    // loadMapInfo(Config::getMapPath(scenarioInfo.mapName, scenarioDir, true),
    // &mapInfo, false); printf("#6.2\n");

    listBoxMapFilter.setSelectedItemIndex(0);
    comboBoxMap.setItems(formattedPlayerSortedMaps[mapInfo.players]);
    comboBoxMap.setSelectedItem(formatString(scenarioInfo.mapName));
  } else {
    // printf("gameSettings.getMapFilter()=%d \n",gameSettings.getMapFilter());
    if (gameSettings.getMapFilter() == 0) {
      listBoxMapFilter.setSelectedItemIndex(0);
    } else {
      listBoxMapFilter.setSelectedItem(intToStr(gameSettings.getMapFilter()));
    }
    comboBoxMap.setItems(
        formattedPlayerSortedMaps[gameSettings.getMapFilter()]);
  }

  // printf("gameSettings.getMap() [%s]
  // [%s]\n",gameSettings.getMap().c_str(),listBoxMap.getSelectedItem().c_str());

  string mapFile = gameSettings.getMap();
  if (find(mapFiles.begin(), mapFiles.end(), mapFile) != mapFiles.end()) {
    mapFile = formatString(mapFile);
    comboBoxMap.setSelectedItem(mapFile);

    loadMapInfo(Config::getMapPath(getCurrentMapFile(), scenarioDir, true),
                &mapInfo, true, true);
    labelMapInfo.setText(mapInfo.desc);
  }

  string tilesetFile = gameSettings.getTileset();
  if (find(tilesetFiles.begin(), tilesetFiles.end(), tilesetFile) !=
      tilesetFiles.end()) {
    tilesetFile = formatString(tilesetFile);
    listBoxTileset.setSelectedItem(tilesetFile);
  }

  string techtreeFile = gameSettings.getTech();
  if (find(techTreeFiles.begin(), techTreeFiles.end(), techtreeFile) !=
      techTreeFiles.end()) {
    techtreeFile = formatString(techtreeFile);
    listBoxTechTree.setSelectedItem(techtreeFile);
  }

  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem, "In [%s::%s] Line: %d\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__);

  // gameSettings->setDefaultUnits(true);
  // gameSettings->setDefaultResources(true);
  // gameSettings->setDefaultVictoryConditions(true);

  // FogOfWar
  if (checkBoxScenario.getValue() == false) {
    listBoxFogOfWar.setSelectedItemIndex(0); // default is 0!
    if (gameSettings.getFogOfWar() == false) {
      listBoxFogOfWar.setSelectedItemIndex(2);
    }

    if ((gameSettings.getFlagTypes1() & ft1_show_map_resources) ==
        ft1_show_map_resources) {
      if (gameSettings.getFogOfWar() == true) {
        listBoxFogOfWar.setSelectedItemIndex(1);
      }
    }
  }

  // printf("In [%s::%s line
  // %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

  checkBoxAllowObservers.setValue(
      gameSettings.getAllowObservers() == true ? true : false);
  // listBoxEnableObserverMode.setSelectedItem(gameSettings.getEnableObserverModeAtEndGame()
  // == true ? lang.getString("Yes") : lang.getString("No"));

  checkBoxEnableSwitchTeamMode.setValue(
      (gameSettings.getFlagTypes1() & ft1_allow_team_switching) ==
              ft1_allow_team_switching
          ? true
          : false);
  listBoxAISwitchTeamAcceptPercent.setSelectedItem(
      intToStr(gameSettings.getAiAcceptSwitchTeamPercentChance()));
  listBoxFallbackCpuMultiplier.setSelectedItemIndex(
      gameSettings.getFallbackCpuMultiplier());

  checkBoxAllowInGameJoinPlayer.setValue(
      (gameSettings.getFlagTypes1() & ft1_allow_in_game_joining) ==
              ft1_allow_in_game_joining
          ? true
          : false);

  checkBoxAllowTeamUnitSharing.setValue(
      (gameSettings.getFlagTypes1() & ft1_allow_shared_team_units) ==
              ft1_allow_shared_team_units
          ? true
          : false);
  checkBoxAllowTeamResourceSharing.setValue(
      (gameSettings.getFlagTypes1() & ft1_allow_shared_team_resources) ==
              ft1_allow_shared_team_resources
          ? true
          : false);

  ServerInterface *serverInterface =
      NetworkManager::getInstance().getServerInterface();
  if (serverInterface != NULL) {
    serverInterface->setAllowInGameConnections(
        checkBoxAllowInGameJoinPlayer.getValue() == true);
  }

  checkBoxAllowNativeLanguageTechtree.setValue(
      gameSettings.getNetworkAllowNativeLanguageTechtree());

  // listBoxPathFinderType.setSelectedItemIndex(gameSettings.getPathFinderType());

  // listBoxEnableServerControlledAI.setSelectedItem(gameSettings.getEnableServerControlledAI()
  // == true ? lang.getString("Yes") : lang.getString("No"));

  // labelNetworkFramePeriod.setText(lang.getString("NetworkFramePeriod"));

  // listBoxNetworkFramePeriod.setSelectedItem(intToStr(gameSettings.getNetworkFramePeriod()/10*10));

  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem, "In [%s::%s] Line: %d\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__);

  checkBoxNetworkPauseGameForLaggedClients.setValue(
      gameSettings.getNetworkPauseGameForLaggedClients());

  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem, "In [%s::%s] Line: %d\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__);

  reloadFactions(false,
                 (checkBoxScenario.getValue() == true
                      ? scenarioFiles[listBoxScenario.getSelectedItemIndex()]
                      : ""));
  // reloadFactions(true);

  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(
        SystemFlags::debugSystem,
        "In [%s::%s] Line: %d] gameSettings.getFactionCount() = %d\n",
        extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__, __LINE__,
        gameSettings.getFactionCount());

  for (int i = 0; i < GameConstants::maxPlayers; ++i) {
    int slotIndex = gameSettings.getStartLocationIndex(i);
    if (gameSettings.getFactionControl(i) <
        listBoxControls[slotIndex].getItemCount()) {
      listBoxControls[slotIndex].setSelectedItemIndex(
          gameSettings.getFactionControl(i));
    }

    // if(slotIndex == 0) printf("#2 slotIndex = %d, i = %d, multiplier =
    // %d\n",slotIndex,i,listBoxRMultiplier[i].getSelectedItemIndex());

    updateResourceMultiplier(slotIndex);

    // if(slotIndex == 0) printf("#3 slotIndex = %d, i = %d, multiplier =
    // %d\n",slotIndex,i,listBoxRMultiplier[i].getSelectedItemIndex());

    listBoxRMultiplier[slotIndex].setSelectedItemIndex(
        gameSettings.getResourceMultiplierIndex(i));

    // if(slotIndex == 0) printf("#4 slotIndex = %d, i = %d, multiplier =
    // %d\n",slotIndex,i,listBoxRMultiplier[i].getSelectedItemIndex());

    listBoxTeams[slotIndex].setSelectedItemIndex(gameSettings.getTeam(i));

    lastSelectedTeamIndex[slotIndex] =
        listBoxTeams[slotIndex].getSelectedItemIndex();

    string factionName = gameSettings.getFactionTypeName(i);
    factionName = formatString(factionName);

    // printf("\n\n\n*** setupUIFromGameSettings A, i = %d, startLoc = %d,
    // factioncontrol = %d, factionName
    // [%s]\n",i,gameSettings.getStartLocationIndex(i),gameSettings.getFactionControl(i),factionName.c_str());

    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem,
                               "In [%s::%s Line: %d] factionName = [%s]\n",
                               extractFileFromDirectoryPath(__FILE__).c_str(),
                               __FUNCTION__, __LINE__, factionName.c_str());

    if (listBoxFactions[slotIndex].hasItem(factionName) == true) {
      listBoxFactions[slotIndex].setSelectedItem(factionName);
    } else {
      listBoxFactions[slotIndex].setSelectedItem(
          formatString(GameConstants::RANDOMFACTION_SLOTNAME));
    }

    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem,
                               "In [%s::%s Line: %d] i = %d, "
                               "gameSettings.getNetworkPlayerName(i) [%s]\n",
                               extractFileFromDirectoryPath(__FILE__).c_str(),
                               __FUNCTION__, __LINE__, i,
                               gameSettings.getNetworkPlayerName(i).c_str());

    // labelPlayerNames[slotIndex].setText(gameSettings.getNetworkPlayerName(i));
  }

  // SetActivePlayerNameEditor();

  updateControlers();
  updateNetworkSlots();

  if (this->headlessServerMode == false && humanPlayerName != "") {
    for (int index = 0; index < GameConstants::maxPlayers; ++index) {
      ControlType ct = static_cast<ControlType>(
          listBoxControls[index].getSelectedItemIndex());
      if (ct == ctHuman) {
        if (humanPlayerName != labelPlayerNames[index].getText()) {
          // printf("Player name changing from [%s] to
          // [%s]\n",labelPlayerNames[index].getText().c_str(),humanPlayerName.c_str());

          labelPlayerNames[index].setText("");
          labelPlayerNames[index].setText(humanPlayerName);
        }
      }
    }
  }

  if (hasNetworkGameSettings() == true) {
    needToSetChangedGameSettings = true;
    lastSetChangedGameSettings = time(NULL);
  }
}
// ============ PRIVATE ===========================

void MenuStateCustomGame::setRefreshedCrcToGameSettings(
    GameSettings *gameSettings) {
  Config &config = Config::getInstance();
  bool forceRefresh = false;

  if (gameSettings->getTileset() != "" &&
      lastRecalculatedCRCTilesetName != gameSettings->getTileset()) {
    lastRecalculatedCRCTilesetName = gameSettings->getTileset();
    // Check if we have calculated the crc since menu_state started
    time_t lastUpdateDate =
        getFolderTreeContentsCheckSumRecursivelyLastGenerated(
            config.getPathListForType(ptTilesets, ""),
            string("/") + gameSettings->getTileset() + string("/*"), ".xml");
    if (difftime(lastUpdateDate, initTime) < 0) {
      forceRefresh = true;
    }
  }

  if (gameSettings->getTech() != "" &&
      lastRecalculatedCRCTechtreeName != gameSettings->getTech()) {
    lastRecalculatedCRCTechtreeName = gameSettings->getTech();
    // Check if we have calculated the crc since menu_state started
    time_t lastUpdateDate =
        getFolderTreeContentsCheckSumRecursivelyLastGenerated(
            config.getPathListForType(ptTechs, ""),
            "/" + gameSettings->getTech() + "/*", ".xml");
    if (difftime(lastUpdateDate, initTime) < 0) {
      forceRefresh = true;
    }
  }
  // no need to deal with map CRC this is always calculated

  // if( forceRefresh) console.addLine("forced refresh CRCCache");
  setCRCsToSettingsInternal(gameSettings, forceRefresh);
}

void MenuStateCustomGame::setCRCsToGameSettings(GameSettings *gameSettings) {
  // printf("lastTechtreeChange=%f\n",difftime(time(NULL),lastTechtreeChange) );
  if (lastCheckedCRCTechtreeName != gameSettings->getTech()) {
    lastTechtreeChange = time(NULL); // now
  }

  if (this->headlessServerMode == false &&
      difftime(time(NULL), lastTechtreeChange) > 5 &&
      hasNetworkGameSettings()) {
    // try a forced CRC recalculation if game settings stay the same for 5
    // seconds. it will only recalculted CRC if it was not already done since
    // program start.
    setRefreshedCrcToGameSettings(gameSettings);
  } else {
    setCRCsToSettingsInternal(gameSettings, false);
  }
}

void MenuStateCustomGame::setCRCsToSettingsInternal(GameSettings *gameSettings,
                                                    bool forceRefresh) {
  Config &config = Config::getInstance();
  if (forceRefresh == true) {
    lastCheckedCRCTilesetName = "";
    lastCheckedCRCTechtreeName = "";
  }

  if (gameSettings->getTileset() != "") {
    if (lastCheckedCRCTilesetName != gameSettings->getTileset()) {
      // console.addLine("Checking tileset CRC [" + gameSettings->getTileset() +
      // "]");
      lastCheckedCRCTilesetValue = getFolderTreeContentsCheckSumRecursively(
          config.getPathListForType(ptTilesets, ""),
          string("/") + gameSettings->getTileset() + string("/*"), ".xml", NULL,
          forceRefresh);
      lastCheckedCRCTilesetName = gameSettings->getTileset();
    }
    gameSettings->setTilesetCRC(lastCheckedCRCTilesetValue);
  }

  if (config.getBool("DisableServerLobbyTechtreeCRCCheck", "false") == false) {
    if (gameSettings->getTech() != "") {
      if (lastCheckedCRCTechtreeName != gameSettings->getTech()) {
        // console.addLine("Checking techtree CRC [" + gameSettings->getTech() +
        // "]");
        uint32 oldCRC = 0;
        if (forceRefresh) {
          oldCRC = getFolderTreeContentsCheckSumRecursively(
              config.getPathListForType(ptTechs, ""),
              "/" + gameSettings->getTech() + "/*", ".xml", NULL, false);
        }
        lastCheckedCRCTechtreeValue = getFolderTreeContentsCheckSumRecursively(
            config.getPathListForType(ptTechs, ""),
            "/" + gameSettings->getTech() + "/*", ".xml", NULL, forceRefresh);
        if (forceRefresh && lastCheckedCRCTechtreeValue != oldCRC) {
          // we really had a bad CRC before!
          console.addLine("techtree CRC  fixed!");
        }
        reloadFactions(
            true, (checkBoxScenario.getValue() == true
                       ? scenarioFiles[listBoxScenario.getSelectedItemIndex()]
                       : ""));
        factionCRCList.clear();
        for (unsigned int factionIdx = 0; factionIdx < factionFiles.size();
             ++factionIdx) {
          string factionName = factionFiles[factionIdx];
          if (factionName != GameConstants::RANDOMFACTION_SLOTNAME &&
              factionName != GameConstants::OBSERVER_SLOTNAME) {
            // factionCRC   =
            // getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTechs,""),
            // "/" + gameSettings->getTech() + "/factions/" + factionName +
            // "/*", ".xml", NULL, true);
            uint32 factionCRC = getFolderTreeContentsCheckSumRecursively(
                config.getPathListForType(ptTechs, ""),
                "/" + gameSettings->getTech() + "/factions/" + factionName +
                    "/*",
                ".xml", NULL, forceRefresh);
            factionCRCList.push_back(make_pair(factionName, factionCRC));
          }
        }
        // console.addLine("Found factions: " +
        // intToStr(factionCRCList.size()));
        lastCheckedCRCTechtreeName = gameSettings->getTech();
      }

      gameSettings->setFactionCRCList(factionCRCList);
      gameSettings->setTechCRC(lastCheckedCRCTechtreeValue);
    }
  }

  if (gameSettings->getMap() != "") {
    if (lastCheckedCRCMapName != gameSettings->getMap()) {
      Checksum checksum;
      string file = Config::getMapPath(gameSettings->getMap(), "", false);
      // console.addLine("Checking map CRC [" + file + "]");
      checksum.addFile(file);
      lastCheckedCRCMapValue = checksum.getSum();
      lastCheckedCRCMapName = gameSettings->getMap();
    }
    gameSettings->setMapCRC(lastCheckedCRCMapValue);
  }
}

void MenuStateCustomGame::setSmallFont(GraphicLabel l) {
  l.setFont(CoreData::getInstance().getDisplayFontSmall());
  l.setFont3D(CoreData::getInstance().getDisplayFontSmall3D());
}

void MenuStateCustomGame::lastPlayerDisconnected() {
  // this is for headless mode only!
  // if last player disconnects we load the network defaults.
  if (this->headlessServerMode == false) {
    return;
  }

  ServerInterface *serverInterface =
      NetworkManager::getInstance().getServerInterface();
  bool foundConnectedPlayer = false;
  for (int i = 0; i < GameConstants::maxPlayers; ++i) {
    if (serverInterface->getSlot(i, true) != NULL &&
        (listBoxControls[i].getSelectedItemIndex() == ctNetwork ||
         listBoxControls[i].getSelectedItemIndex() == ctNetworkUnassigned)) {
      if (serverInterface->getSlot(i, true)->isConnected() == true) {
        foundConnectedPlayer = true;
      }
    }
  }

  if (!foundConnectedPlayer && headlessHasConnectedPlayer == true) {
    // load defaults
    string data_path =
        getGameReadWritePath(GameConstants::path_data_CacheLookupKey);
    if (fileExists(data_path + DEFAULT_NETWORK_SETUP_FILENAME) == true)
      loadGameSettings(data_path + DEFAULT_NETWORK_SETUP_FILENAME);
  }
  headlessHasConnectedPlayer = foundConnectedPlayer;
}

bool MenuStateCustomGame::hasNetworkGameSettings() {
  bool hasNetworkSlot = false;

  try {
    for (int i = 0; i < mapInfo.players; ++i) {
      ControlType ct =
          static_cast<ControlType>(listBoxControls[i].getSelectedItemIndex());
      if (ct != ctClosed) {
        if (ct == ctNetwork || ct == ctNetworkUnassigned) {
          hasNetworkSlot = true;
          break;
        }
      }
    }
    if (hasNetworkSlot == false) {
      for (int i = 0; i < GameConstants::maxPlayers; ++i) {
        ControlType ct =
            static_cast<ControlType>(listBoxControls[i].getSelectedItemIndex());
        if (ct != ctClosed) {
          if (ct == ctNetworkUnassigned) {
            hasNetworkSlot = true;
            break;
          }
        }
      }
    }
  } catch (const std::exception &ex) {
    char szBuf[8096] = "";
    snprintf(szBuf, 8096, "In [%s::%s %d]\nError detected:\n%s\n",
             extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
             __LINE__, ex.what());
    SystemFlags::OutputDebug(SystemFlags::debugError,
                             "In [%s::%s Line: %d] Error [%s]\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__, ex.what());
    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem, "%s", szBuf);

    showGeneralError = true;
    generalErrorToShow = szBuf;
  }

  return hasNetworkSlot;
}

void MenuStateCustomGame::loadMapInfo(string file, MapInfo *mapInfo,
                                      bool loadMapPreview, bool doPlayerSetup) {
  try {
    Lang &lang = Lang::getInstance();
    if (MapPreview::loadMapInfo(file, mapInfo, lang.getString("MaxPlayers"),
                                lang.getString("Size"), true) == true) {
      if (doPlayerSetup) {
        ServerInterface *serverInterface =
            NetworkManager::getInstance().getServerInterface();
        for (int i = 0; i < GameConstants::maxPlayers; ++i) {
          if (serverInterface->getSlot(i, true) != NULL &&
              (listBoxControls[i].getSelectedItemIndex() == ctNetwork ||
               listBoxControls[i].getSelectedItemIndex() ==
                   ctNetworkUnassigned)) {
            if (serverInterface->getSlot(i, true)->isConnected() == true) {
              if (i + 1 > mapInfo->players &&
                  listBoxControls[i].getSelectedItemIndex() !=
                      ctNetworkUnassigned) {
                listBoxControls[i].setSelectedItemIndex(ctNetworkUnassigned);
              }
            }
          }

          labelPlayers[i].setVisible(i + 1 <= mapInfo->players);
          labelPlayerNames[i].setVisible(i + 1 <= mapInfo->players);
          listBoxControls[i].setVisible(i + 1 <= mapInfo->players);
          listBoxFactions[i].setVisible(i + 1 <= mapInfo->players);
          listBoxTeams[i].setVisible(i + 1 <= mapInfo->players);
          labelNetStatus[i].setVisible(i + 1 <= mapInfo->players);
        }
      }

      // Not painting properly so this is on hold
      if (loadMapPreview == true) {
        if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
          SystemFlags::OutputDebug(
              SystemFlags::debugSystem, "In [%s::%s Line: %d]\n",
              extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
              __LINE__);

        mapPreview.loadFromFile(file.c_str());

        // printf("Loading map preview MAP\n");
        cleanupMapPreviewTexture();
      }
    }
  } catch (exception &e) {
    SystemFlags::OutputDebug(
        SystemFlags::debugError,
        "In [%s::%s Line: %d] Error [%s] loading map [%s]\n",
        extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__, __LINE__,
        e.what(), file.c_str());
    throw megaglest_runtime_error("Error loading map file: [" + file +
                                  "] msg: " + e.what());
  }
}

void MenuStateCustomGame::updateControlers() {
  try {
    bool humanPlayer = false;

    for (int i = 0; i < mapInfo.players; ++i) {
      if (listBoxControls[i].getSelectedItemIndex() == ctHuman) {
        humanPlayer = true;
      }
    }

    if (humanPlayer == false) {
      if (this->headlessServerMode == false) {
        bool foundNewSlotForHuman = false;
        for (int i = 0; i < mapInfo.players; ++i) {
          if (listBoxControls[i].getSelectedItemIndex() == ctClosed) {
            setSlotHuman(i);
            foundNewSlotForHuman = true;
            break;
          }
        }

        if (foundNewSlotForHuman == false) {
          for (int i = 0; i < mapInfo.players; ++i) {
            if (listBoxControls[i].getSelectedItemIndex() == ctClosed ||
                listBoxControls[i].getSelectedItemIndex() == ctCpuEasy ||
                listBoxControls[i].getSelectedItemIndex() == ctCpu ||
                listBoxControls[i].getSelectedItemIndex() == ctCpuUltra ||
                listBoxControls[i].getSelectedItemIndex() == ctCpuMega) {
              setSlotHuman(i);

              foundNewSlotForHuman = true;
              break;
            }
          }
        }

        if (foundNewSlotForHuman == false) {
          ServerInterface *serverInterface =
              NetworkManager::getInstance().getServerInterface();
          ConnectionSlot *slot = serverInterface->getSlot(0, true);
          if (slot != NULL && slot->isConnected() == true) {
            serverInterface->removeSlot(0);
          }
          setSlotHuman(0);
        }
      }
    }

    for (int i = mapInfo.players; i < GameConstants::maxPlayers; ++i) {
      if (listBoxControls[i].getSelectedItemIndex() != ctNetwork &&
          listBoxControls[i].getSelectedItemIndex() != ctNetworkUnassigned) {
        // printf("Closed A [%d]
        // [%s]\n",i,labelPlayerNames[i].getText().c_str());

        listBoxControls[i].setSelectedItemIndex(ctClosed);
      }
    }
  } catch (const std::exception &ex) {
    char szBuf[8096] = "";
    snprintf(szBuf, 8096, "In [%s::%s %d]\nError detected:\n%s\n",
             extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
             __LINE__, ex.what());
    SystemFlags::OutputDebug(SystemFlags::debugError, szBuf);
    throw megaglest_runtime_error(szBuf);
  }
}

void MenuStateCustomGame::closeUnusedSlots() {
  try {
    if (checkBoxScenario.getValue() == false) {
      ServerInterface *serverInterface =
          NetworkManager::getInstance().getServerInterface();
      // for(int i= 0; i<mapInfo.players; ++i){
      for (int i = 0; i < GameConstants::maxPlayers; ++i) {
        if (listBoxControls[i].getSelectedItemIndex() == ctNetwork ||
            listBoxControls[i].getSelectedItemIndex() == ctNetworkUnassigned) {
          if (serverInterface->getSlot(i, true) == NULL ||
              serverInterface->getSlot(i, true)->isConnected() == false ||
              serverInterface->getSlot(i, true)->getConnectHasHandshaked() ==
                  false) {
            // printf("Closed A [%d]
            // [%s]\n",i,labelPlayerNames[i].getText().c_str());

            listBoxControls[i].setSelectedItemIndex(ctClosed);
          }
        }
      }
      updateNetworkSlots();
    }
  } catch (const std::exception &ex) {
    char szBuf[8096] = "";
    snprintf(szBuf, 8096, "In [%s::%s %d]\nError detected:\n%s\n",
             extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
             __LINE__, ex.what());
    SystemFlags::OutputDebug(SystemFlags::debugError, szBuf);
    throw megaglest_runtime_error(szBuf);
  }
}

void MenuStateCustomGame::updateNetworkSlots() {
  try {
    ServerInterface *serverInterface =
        NetworkManager::getInstance().getServerInterface();

    if (hasNetworkGameSettings() == true) {
      if (hasCheckedForUPNP == false) {

        if (checkBoxPublishServer.getValue() == true ||
            this->headlessServerMode == true) {

          hasCheckedForUPNP = true;
          serverInterface->getServerSocket()->NETdiscoverUPnPDevices();
        }
      }
    } else {
      hasCheckedForUPNP = false;
    }

    for (int i = 0; i < GameConstants::maxPlayers; ++i) {
      ConnectionSlot *slot = serverInterface->getSlot(i, true);
      // printf("A i = %d control type = %d slot
      // [%p]\n",i,listBoxControls[i].getSelectedItemIndex(),slot);

      if (slot == NULL &&
          listBoxControls[i].getSelectedItemIndex() == ctNetwork) {
        try {
          serverInterface->addSlot(i);
        } catch (const std::exception &ex) {
          char szBuf[8096] = "";
          snprintf(szBuf, 8096, "In [%s::%s %d]\nError detected:\n%s\n",
                   extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                   __LINE__, ex.what());
          SystemFlags::OutputDebug(SystemFlags::debugError, szBuf);
          if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem)
                  .enabled)
            SystemFlags::OutputDebug(SystemFlags::debugSystem, "%s", szBuf);
          showGeneralError = true;
          if (serverInterface->isPortBound() == false) {
            generalErrorToShow =
                Lang::getInstance().getString("ErrorBindingPort") + " : " +
                intToStr(serverInterface->getBindPort());
          } else {
            generalErrorToShow = ex.what();
          }

          // Revert network to CPU
          listBoxControls[i].setSelectedItemIndex(ctCpu);
        }
      }
      slot = serverInterface->getSlot(i, true);
      if (slot != NULL) {
        if ((listBoxControls[i].getSelectedItemIndex() != ctNetwork) ||
            (listBoxControls[i].getSelectedItemIndex() == ctNetwork &&
             slot->isConnected() == false && i >= mapInfo.players)) {
          if (slot->getCanAcceptConnections() == true) {
            slot->setCanAcceptConnections(false);
          }
          if (slot->isConnected() == true) {
            if (listBoxControls[i].getSelectedItemIndex() !=
                ctNetworkUnassigned) {
              listBoxControls[i].setSelectedItemIndex(ctNetworkUnassigned);
            }
          } else {
            serverInterface->removeSlot(i);

            if (listBoxControls[i].getSelectedItemIndex() ==
                ctNetworkUnassigned) {
              listBoxControls[i].setSelectedItemIndex(ctClosed);
            }
          }
        } else if (slot->getCanAcceptConnections() == false) {
          slot->setCanAcceptConnections(true);
        }
      }
    }
  } catch (const std::exception &ex) {
    char szBuf[8096] = "";
    snprintf(szBuf, 8096, "In [%s::%s %d]\nError detected:\n%s\n",
             extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
             __LINE__, ex.what());
    SystemFlags::OutputDebug(SystemFlags::debugError, szBuf);
    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem, "%s", szBuf);
    // throw megaglest_runtime_error(szBuf);
    showGeneralError = true;
    generalErrorToShow = szBuf;
  }
}

bool MenuStateCustomGame::textInput(std::string text) {
  // printf("In [%s::%s Line: %d] text
  // [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,text.c_str());
  if (activeInputLabel != NULL) {
    bool handled = textInputEditLabel(text, &activeInputLabel);
    if (handled == true && &labelGameName != activeInputLabel) {
      MutexSafeWrapper safeMutex(
          (publishToMasterserverThread != NULL
               ? publishToMasterserverThread->getMutexThreadObjectAccessor()
               : NULL),
          string(__FILE__) + "_" + intToStr(__LINE__));
      MutexSafeWrapper safeMutexCLI(
          (publishToClientsThread != NULL
               ? publishToClientsThread->getMutexThreadObjectAccessor()
               : NULL),
          string(__FILE__) + "_" + intToStr(__LINE__));

      if (hasNetworkGameSettings() == true) {
        needToSetChangedGameSettings = true;
        lastSetChangedGameSettings = time(NULL);
      }
    }
  } else {
    if (hasNetworkGameSettings() == true) {
      chatManager.textInput(text);
    }
  }
  return false;
}

void MenuStateCustomGame::keyDown(SDL_KeyboardEvent key) {
  if (isMasterserverMode() == true) {
    return;
  }

  if (activeInputLabel != NULL) {
    bool handled = keyDownEditLabel(key, &activeInputLabel);
    if (handled == true) {
      MutexSafeWrapper safeMutex(
          (publishToMasterserverThread != NULL
               ? publishToMasterserverThread->getMutexThreadObjectAccessor()
               : NULL),
          string(__FILE__) + "_" + intToStr(__LINE__));
      MutexSafeWrapper safeMutexCLI(
          (publishToClientsThread != NULL
               ? publishToClientsThread->getMutexThreadObjectAccessor()
               : NULL),
          string(__FILE__) + "_" + intToStr(__LINE__));

      if (hasNetworkGameSettings() == true) {
        needToSetChangedGameSettings = true;
        lastSetChangedGameSettings = time(NULL);
      }
    }
  } else {
    // send key to the chat manager
    if (hasNetworkGameSettings() == true) {
      chatManager.keyDown(key);
    }
    if (chatManager.getEditEnabled() == false &&
        (::Shared::Platform::Window::isKeyStateModPressed(KMOD_SHIFT) ==
         false)) {
      Config &configKeys = Config::getInstance(
          std::pair<ConfigType, ConfigType>(cfgMainKeys, cfgUserKeys));

      // if(key == configKeys.getCharKey("ShowFullConsole")) {
      if (isKeyPressed(configKeys.getSDLKey("ShowFullConsole"), key) == true) {
        showFullConsole = true;
      }
      // Toggle music
      // else if(key == configKeys.getCharKey("ToggleMusic")) {
      else if (isKeyPressed(configKeys.getSDLKey("ToggleMusic"), key) == true) {
        Config &config = Config::getInstance();
        Lang &lang = Lang::getInstance();

        float configVolume = (config.getInt("SoundVolumeMusic") / 100.f);
        float currentVolume =
            CoreData::getInstance().getMenuMusic()->getVolume();
        if (currentVolume > 0) {
          CoreData::getInstance().getMenuMusic()->setVolume(0.f);
          console.addLine(lang.getString("GameMusic") + " " +
                          lang.getString("Off"));
        } else {
          CoreData::getInstance().getMenuMusic()->setVolume(configVolume);
          // If the config says zero, use the default music volume
          // gameMusic->setVolume(configVolume ? configVolume : 0.9);
          console.addLine(lang.getString("GameMusic"));
        }
      }
      // else if(key == configKeys.getCharKey("SaveGUILayout")) {
      else if (isKeyPressed(configKeys.getSDLKey("SaveGUILayout"), key) ==
               true) {
        bool saved = GraphicComponent::saveAllCustomProperties(containerName);
        Lang &lang = Lang::getInstance();
        console.addLine(lang.getString("GUILayoutSaved") + " [" +
                        (saved ? lang.getString("Yes") : lang.getString("No")) +
                        "]");
      }
    }
  }
}

void MenuStateCustomGame::keyPress(SDL_KeyboardEvent c) {
  if (isMasterserverMode() == true) {
    return;
  }

  if (activeInputLabel != NULL) {
    bool handled = keyPressEditLabel(c, &activeInputLabel);
    if (handled == true && &labelGameName != activeInputLabel) {
      MutexSafeWrapper safeMutex(
          (publishToMasterserverThread != NULL
               ? publishToMasterserverThread->getMutexThreadObjectAccessor()
               : NULL),
          string(__FILE__) + "_" + intToStr(__LINE__));
      MutexSafeWrapper safeMutexCLI(
          (publishToClientsThread != NULL
               ? publishToClientsThread->getMutexThreadObjectAccessor()
               : NULL),
          string(__FILE__) + "_" + intToStr(__LINE__));

      if (hasNetworkGameSettings() == true) {
        needToSetChangedGameSettings = true;
        lastSetChangedGameSettings = time(NULL);
      }
    }
  } else {
    if (hasNetworkGameSettings() == true) {
      chatManager.keyPress(c);
    }
  }
}

void MenuStateCustomGame::keyUp(SDL_KeyboardEvent key) {
  if (isMasterserverMode() == true) {
    return;
  }

  if (activeInputLabel == NULL) {
    if (hasNetworkGameSettings() == true) {
      chatManager.keyUp(key);
    }
    Config &configKeys = Config::getInstance(
        std::pair<ConfigType, ConfigType>(cfgMainKeys, cfgUserKeys));

    if (chatManager.getEditEnabled()) {
      // send key to the chat manager
      if (hasNetworkGameSettings() == true) {
        chatManager.keyUp(key);
      }
    }
    // else if(key == configKeys.getCharKey("ShowFullConsole")) {
    else if (isKeyPressed(configKeys.getSDLKey("ShowFullConsole"), key) ==
             true) {
      showFullConsole = false;
    }
  }
}

void MenuStateCustomGame::showMessageBox(const string &text,
                                         const string &header, bool toggle) {
  if (!toggle) {
    mainMessageBox.setEnabled(false);
  }

  if (!mainMessageBox.getEnabled()) {
    mainMessageBox.setText(text);
    mainMessageBox.setHeader(header);
    mainMessageBox.setEnabled(true);
  } else {
    mainMessageBox.setEnabled(false);
  }
}

void MenuStateCustomGame::switchToNextMapGroup(const int direction) {
  int i = listBoxMapFilter.getSelectedItemIndex();
  // if there are no maps for the current selection we switch to next selection
  while (formattedPlayerSortedMaps[i].empty()) {
    i = i + direction;
    if (i > GameConstants::maxPlayers) {
      i = 0;
    }
    if (i < 0) {
      i = GameConstants::maxPlayers;
    }
  }
  listBoxMapFilter.setSelectedItemIndex(i);
  comboBoxMap.setItems(formattedPlayerSortedMaps[i]);
}

string MenuStateCustomGame::getCurrentMapFile() {
  int i = listBoxMapFilter.getSelectedItemIndex();
  int mapIndex = comboBoxMap.getSelectedItemIndex();
  if (playerSortedMaps[i].empty() == false) {
    return playerSortedMaps[i].at(mapIndex);
  }
  return "";
}

string MenuStateCustomGame::getPreselectedMapFile() {
  int i = listBoxMapFilter.getSelectedItemIndex();
  int mapIndex = comboBoxMap.getPreselectedItemIndex();
  if (playerSortedMaps[i].empty() == false) {
    return playerSortedMaps[i].at(mapIndex);
  }
  return "";
}

void MenuStateCustomGame::setActiveInputLabel(GraphicLabel *newLable) {
  MenuState::setActiveInputLabel(newLable, &activeInputLabel);
}

string MenuStateCustomGame::getHumanPlayerName(int index) {
  string result = defaultPlayerName;
  if (index < 0) {
    for (int j = 0; j < GameConstants::maxPlayers; ++j) {
      if (listBoxControls[j].getSelectedItemIndex() >= 0) {
        ControlType ct =
            static_cast<ControlType>(listBoxControls[j].getSelectedItemIndex());
        if (ct == ctHuman) {
          index = j;
          break;
        }
      }
    }
  }

  if (index >= 0 && index < GameConstants::maxPlayers &&
      labelPlayerNames[index].getText() != "" &&
      labelPlayerNames[index].getText() !=
          GameConstants::NETWORK_SLOT_UNCONNECTED_SLOTNAME) {
    result = labelPlayerNames[index].getText();

    if (activeInputLabel != NULL) {
      size_t found = result.find_last_of("_");
      if (found != string::npos) {
        result = result.substr(0, found);
      }
    }
  }

  return result;
}

void MenuStateCustomGame::loadFactionTexture(string filepath) {
  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem, "In [%s::%s Line: %d]\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__);

  if (enableFactionTexturePreview == true) {
    if (filepath == "") {
      factionTexture = NULL;
    } else {
      if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
        SystemFlags::OutputDebug(SystemFlags::debugSystem,
                                 "In [%s::%s Line: %d] filepath = [%s]\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__, filepath.c_str());

      factionTexture = Renderer::findTexture(filepath);

      if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
        SystemFlags::OutputDebug(SystemFlags::debugSystem,
                                 "In [%s::%s Line: %d]\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__);
    }
  }
}

void MenuStateCustomGame::cleanupMapPreviewTexture() {
  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem, "In [%s::%s Line: %d]\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__);

  // printf("CLEANUP map preview texture\n");

  if (mapPreviewTexture != NULL) {
    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem,
                               "In [%s::%s Line: %d]\n",
                               extractFileFromDirectoryPath(__FILE__).c_str(),
                               __FUNCTION__, __LINE__);

    mapPreviewTexture->end();

    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem,
                               "In [%s::%s Line: %d]\n",
                               extractFileFromDirectoryPath(__FILE__).c_str(),
                               __FUNCTION__, __LINE__);
    delete mapPreviewTexture;
    mapPreviewTexture = NULL;
  }
  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem, "In [%s::%s Line: %d]\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__);
}

int32 MenuStateCustomGame::getNetworkPlayerStatus() {
  int32 result = npst_None;
  switch (listBoxPlayerStatus.getSelectedItemIndex()) {
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

void MenuStateCustomGame::loadScenarioInfo(string file,
                                           ScenarioInfo *scenarioInfo) {
  // printf("Load scenario file [%s]\n",file.c_str());
  bool isTutorial = Scenario::isGameTutorial(file);
  Scenario::loadScenarioInfo(file, scenarioInfo, isTutorial);

  // cleanupPreviewTexture();
  previewLoadDelayTimer = time(NULL);
  needToLoadTextures = true;
}

bool MenuStateCustomGame::isInSpecialKeyCaptureEvent() {
  bool result = (chatManager.getEditEnabled() || activeInputLabel != NULL);
  return result;
}

void MenuStateCustomGame::processScenario() {
  try {
    if (checkBoxScenario.getValue() == true) {
      // printf("listBoxScenario.getSelectedItemIndex() = %d [%s]
      // scenarioFiles.size() =
      // %d\n",listBoxScenario.getSelectedItemIndex(),listBoxScenario.getSelectedItem().c_str(),scenarioFiles.size());
      loadScenarioInfo(
          Scenario::getScenarioPath(
              dirList, scenarioFiles[listBoxScenario.getSelectedItemIndex()]),
          &scenarioInfo);
      string scenarioDir = Scenario::getScenarioDir(dirList, scenarioInfo.name);

      // printf("scenarioInfo.fogOfWar = %d scenarioInfo.fogOfWar_exploredFlag =
      // %d\n",scenarioInfo.fogOfWar,scenarioInfo.fogOfWar_exploredFlag);
      if (scenarioInfo.fogOfWar == false &&
          scenarioInfo.fogOfWar_exploredFlag == false) {
        listBoxFogOfWar.setSelectedItemIndex(2);
      } else if (scenarioInfo.fogOfWar_exploredFlag == true) {
        listBoxFogOfWar.setSelectedItemIndex(1);
      } else {
        listBoxFogOfWar.setSelectedItemIndex(0);
      }

      checkBoxAllowTeamUnitSharing.setValue(scenarioInfo.allowTeamUnitSharing);
      checkBoxAllowTeamResourceSharing.setValue(
          scenarioInfo.allowTeamResourceSharing);

      setupTechList(scenarioInfo.name, false);
      listBoxTechTree.setSelectedItem(formatString(scenarioInfo.techTreeName));
      reloadFactions(false, scenarioInfo.name);

      setupTilesetList(scenarioInfo.name);
      listBoxTileset.setSelectedItem(formatString(scenarioInfo.tilesetName));

      setupMapList(scenarioInfo.name);
      comboBoxMap.setSelectedItem(formatString(scenarioInfo.mapName));
      loadMapInfo(Config::getMapPath(getCurrentMapFile(), scenarioDir, true),
                  &mapInfo, true, true);
      labelMapInfo.setText(mapInfo.desc);

      // printf("scenarioInfo.name [%s]
      // [%s]\n",scenarioInfo.name.c_str(),listBoxMap.getSelectedItem().c_str());

      // Loop twice to set the human slot or else it closes network slots in
      // some cases
      for (int humanIndex = 0; humanIndex < 2; ++humanIndex) {
        for (int i = 0; i < mapInfo.players; ++i) {
          listBoxRMultiplier[i].setSelectedItem(
              floatToStr(scenarioInfo.resourceMultipliers[i], 1));

          ServerInterface *serverInterface =
              NetworkManager::getInstance().getServerInterface();
          ConnectionSlot *slot = serverInterface->getSlot(i, true);

          int selectedControlItemIndex =
              listBoxControls[i].getSelectedItemIndex();
          if (selectedControlItemIndex != ctNetwork ||
              (selectedControlItemIndex == ctNetwork &&
               (slot == NULL || slot->isConnected() == false))) {
          }

          listBoxControls[i].setSelectedItemIndex(
              scenarioInfo.factionControls[i]);
          if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem)
                  .enabled)
            SystemFlags::OutputDebug(
                SystemFlags::debugSystem, "In [%s::%s Line %d]\n",
                extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                __LINE__);

          // Skip over networkunassigned
          // if(listBoxControls[i].getSelectedItemIndex() == ctNetworkUnassigned
          // && 	selectedControlItemIndex != ctNetworkUnassigned) {
          //	listBoxControls[i].mouseClick(x, y);
          //}

          // look for human players
          int humanIndex1 = -1;
          int humanIndex2 = -1;
          for (int j = 0; j < GameConstants::maxPlayers; ++j) {
            ControlType ct = static_cast<ControlType>(
                listBoxControls[j].getSelectedItemIndex());
            if (ct == ctHuman) {
              if (humanIndex1 == -1) {
                humanIndex1 = j;
              } else {
                humanIndex2 = j;
              }
            }
          }

          if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem)
                  .enabled)
            SystemFlags::OutputDebug(
                SystemFlags::debugSystem,
                "In [%s::%s Line %d] humanIndex1 = %d, humanIndex2 = %d\n",
                extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                __LINE__, humanIndex1, humanIndex2);

          // no human
          if (humanIndex1 == -1 && humanIndex2 == -1) {
            setSlotHuman(i);
            if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem)
                    .enabled)
              SystemFlags::OutputDebug(
                  SystemFlags::debugSystem,
                  "In [%s::%s Line %d] i = %d, labelPlayerNames[i].getText() "
                  "[%s]\n",
                  extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                  __LINE__, i, labelPlayerNames[i].getText().c_str());

            // printf("humanIndex1 = %d humanIndex2 = %d i = %d
            // listBoxControls[i].getSelectedItemIndex() =
            // %d\n",humanIndex1,humanIndex2,i,listBoxControls[i].getSelectedItemIndex());
          }
          // 2 humans
          else if (humanIndex1 != -1 && humanIndex2 != -1) {
            int closeSlotIndex = (humanIndex1 == i ? humanIndex2 : humanIndex1);
            int humanSlotIndex =
                (closeSlotIndex == humanIndex1 ? humanIndex2 : humanIndex1);

            string origPlayName = labelPlayerNames[closeSlotIndex].getText();

            if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem)
                    .enabled)
              SystemFlags::OutputDebug(
                  SystemFlags::debugSystem,
                  "In [%s::%s Line %d] closeSlotIndex = %d, origPlayName "
                  "[%s]\n",
                  extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                  __LINE__, closeSlotIndex, origPlayName.c_str());
            // printf("humanIndex1 = %d humanIndex2 = %d i = %d closeSlotIndex =
            // %d humanSlotIndex =
            // %d\n",humanIndex1,humanIndex2,i,closeSlotIndex,humanSlotIndex);

            listBoxControls[closeSlotIndex].setSelectedItemIndex(ctClosed);
            labelPlayerNames[humanSlotIndex].setText(
                (origPlayName != "" ? origPlayName : getHumanPlayerName()));
          }

          ControlType ct = static_cast<ControlType>(
              listBoxControls[i].getSelectedItemIndex());
          if (ct != ctClosed) {
            // updateNetworkSlots();
            // updateResourceMultiplier(i);
            updateResourceMultiplier(i);

            // printf("Setting scenario faction i = %d [
            // %s]\n",i,scenarioInfo.factionTypeNames[i].c_str());
            listBoxFactions[i].setSelectedItem(
                formatString(scenarioInfo.factionTypeNames[i]));
            // printf("DONE Setting scenario faction i = %d [
            // %s]\n",i,scenarioInfo.factionTypeNames[i].c_str());

            // Disallow CPU players to be observers
            if (factionFiles[listBoxFactions[i].getSelectedItemIndex()] ==
                    formatString(GameConstants::OBSERVER_SLOTNAME) &&
                (listBoxControls[i].getSelectedItemIndex() == ctCpuEasy ||
                 listBoxControls[i].getSelectedItemIndex() == ctCpu ||
                 listBoxControls[i].getSelectedItemIndex() == ctCpuUltra ||
                 listBoxControls[i].getSelectedItemIndex() == ctCpuMega)) {
              listBoxFactions[i].setSelectedItemIndex(0);
            }
            //

            listBoxTeams[i].setSelectedItem(intToStr(scenarioInfo.teams[i]));
            if (factionFiles[listBoxFactions[i].getSelectedItemIndex()] !=
                formatString(GameConstants::OBSERVER_SLOTNAME)) {
              if (listBoxTeams[i].getSelectedItemIndex() + 1 !=
                  (GameConstants::maxPlayers + fpt_Observer)) {
                lastSelectedTeamIndex[i] =
                    listBoxTeams[i].getSelectedItemIndex();
              }
              // Alow Neutral cpu players
              else if (listBoxControls[i].getSelectedItemIndex() == ctCpuEasy ||
                       listBoxControls[i].getSelectedItemIndex() == ctCpu ||
                       listBoxControls[i].getSelectedItemIndex() ==
                           ctCpuUltra ||
                       listBoxControls[i].getSelectedItemIndex() == ctCpuMega) {
                lastSelectedTeamIndex[i] =
                    listBoxTeams[i].getSelectedItemIndex();
              }
            } else {
              lastSelectedTeamIndex[i] = -1;
            }
          }

          if (checkBoxPublishServer.getValue() == true) {
            needToRepublishToMasterserver = true;
          }

          if (hasNetworkGameSettings() == true) {
            needToSetChangedGameSettings = true;
            lastSetChangedGameSettings = time(NULL);
            ;
          }
        }
      }

      updateControlers();
      updateNetworkSlots();

      MutexSafeWrapper safeMutex(
          (publishToMasterserverThread != NULL
               ? publishToMasterserverThread->getMutexThreadObjectAccessor()
               : NULL),
          string(__FILE__) + "_" + intToStr(__LINE__));
      MutexSafeWrapper safeMutexCLI(
          (publishToClientsThread != NULL
               ? publishToClientsThread->getMutexThreadObjectAccessor()
               : NULL),
          string(__FILE__) + "_" + intToStr(__LINE__));

      if (checkBoxPublishServer.getValue() == true) {
        needToRepublishToMasterserver = true;
      }
      if (hasNetworkGameSettings() == true) {
        needToSetChangedGameSettings = true;
        lastSetChangedGameSettings = time(NULL);
      }

      // labelInfo.setText(scenarioInfo.desc);
    } else {
      setupMapList("");
      comboBoxMap.setSelectedItem(
          formatString(formattedPlayerSortedMaps[0][0]));
      loadMapInfo(Config::getMapPath(getCurrentMapFile(), "", true), &mapInfo,
                  true, true);
      labelMapInfo.setText(mapInfo.desc);

      int initialTechSelection = setupTechList("", false);
      if (listBoxTechTree.getItemCount() > 0) {
        listBoxTechTree.setSelectedItemIndex(initialTechSelection);
      }
      reloadFactions(false, "");
      setupTilesetList("");
      updateControlers();
    }
    SetupUIForScenarios();
  } catch (const std::exception &ex) {
    char szBuf[8096] = "";
    snprintf(szBuf, 8096, "In [%s::%s %d]\nError detected:\n%s\n",
             extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
             __LINE__, ex.what());
    SystemFlags::OutputDebug(SystemFlags::debugError, szBuf);
    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem, "%s", szBuf);

    mainMessageBoxState = 1;
    showMessageBox(szBuf, "Error detected", false);
  }
}

void MenuStateCustomGame::SetupUIForScenarios() {
  try {
    if (checkBoxScenario.getValue() == true) {
      // START - Disable changes to controls while in Scenario mode
      for (int i = 0; i < GameConstants::maxPlayers; ++i) {
        listBoxControls[i].setEditable(false);
        listBoxFactions[i].setEditable(false);
        listBoxRMultiplier[i].setEditable(false);
        listBoxTeams[i].setEditable(false);
      }
      listBoxFogOfWar.setEditable(false);
      checkBoxAllowObservers.setEditable(false);
      checkBoxAllowTeamUnitSharing.setEditable(false);
      checkBoxAllowTeamResourceSharing.setEditable(false);
      // listBoxPathFinderType.setEditable(false);
      checkBoxEnableSwitchTeamMode.setEditable(false);
      listBoxAISwitchTeamAcceptPercent.setEditable(false);
      listBoxFallbackCpuMultiplier.setEditable(false);
      comboBoxMap.setEditable(false);
      listBoxTileset.setEditable(false);
      listBoxMapFilter.setEditable(false);
      listBoxTechTree.setEditable(false);
      // END - Disable changes to controls while in Scenario mode
    } else {
      // START - Disable changes to controls while in Scenario mode
      for (int i = 0; i < GameConstants::maxPlayers; ++i) {
        listBoxControls[i].setEditable(true);
        listBoxFactions[i].setEditable(true);
        listBoxRMultiplier[i].setEditable(true);
        listBoxTeams[i].setEditable(true);
      }
      listBoxFogOfWar.setEditable(true);
      checkBoxAllowObservers.setEditable(true);
      checkBoxAllowTeamUnitSharing.setEditable(true);
      checkBoxAllowTeamResourceSharing.setEditable(true);
      // listBoxPathFinderType.setEditable(true);
      checkBoxEnableSwitchTeamMode.setEditable(true);
      listBoxAISwitchTeamAcceptPercent.setEditable(true);
      listBoxFallbackCpuMultiplier.setEditable(true);
      comboBoxMap.setEditable(true);
      listBoxTileset.setEditable(true);
      listBoxMapFilter.setEditable(true);
      listBoxTechTree.setEditable(true);
      // END - Disable changes to controls while in Scenario mode
    }
  } catch (const std::exception &ex) {
    char szBuf[8096] = "";
    snprintf(szBuf, 8096, "In [%s::%s %d]\nError detected:\n%s\n",
             extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
             __LINE__, ex.what());
    SystemFlags::OutputDebug(SystemFlags::debugError, szBuf);
    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem, "%s", szBuf);

    throw megaglest_runtime_error(szBuf);
  }
}

void MenuStateCustomGame::loadSavedSetupNames() {
  Config &config = Config::getInstance();
  Lang &lang = Lang::getInstance();
  vector<string> paths;
  string userData = config.getString("UserData_Root", "");
  if (userData != "") {
    endPathWithSlash(userData);
  }
  string saveSetupDir;
  saveSetupDir = userData + "setups";
  paths.push_back(saveSetupDir);
  savedSetupFilenames.clear();
  findAll(paths, "*.mgg", savedSetupFilenames, true, false, true);
  sort(savedSetupFilenames.begin(), savedSetupFilenames.end());
  savedSetupFilenames.insert(savedSetupFilenames.begin(), 1,
                             lang.getString(LAST_SETUP_STRING));
}

int MenuStateCustomGame::setupMapList(string scenario) {
  int initialMapSelection = 0;

  try {
    Config &config = Config::getInstance();
    vector<string> invalidMapList;
    string scenarioDir = Scenario::getScenarioDir(dirList, scenario);
    vector<string> pathList = config.getPathListForType(ptMaps, scenarioDir);
    vector<string> allMaps = MapPreview::findAllValidMaps(
        pathList, scenarioDir, false, true, &invalidMapList);
    if (scenario != "") {
      vector<string> allMaps2 =
          MapPreview::findAllValidMaps(config.getPathListForType(ptMaps, ""),
                                       "", false, true, &invalidMapList);
      copy(allMaps2.begin(), allMaps2.end(),
           std::inserter(allMaps, allMaps.begin()));
    }
    // sort map list non case sensitive
    std::sort(allMaps.begin(), allMaps.end(), compareNonCaseSensitive);

    if (allMaps.empty()) {
      throw megaglest_runtime_error("No maps were found!");
    }
    vector<string> results;
    copy(allMaps.begin(), allMaps.end(), std::back_inserter(results));
    mapFiles = results;

    for (unsigned int i = 0; i < GameConstants::maxPlayers + 1; ++i) {
      playerSortedMaps[i].clear();
      formattedPlayerSortedMaps[i].clear();
    }

    // at index=0 fill in the whole list
    copy(mapFiles.begin(), mapFiles.end(),
         std::back_inserter(playerSortedMaps[0]));
    copy(playerSortedMaps[0].begin(), playerSortedMaps[0].end(),
         std::back_inserter(formattedPlayerSortedMaps[0]));
    std::for_each(formattedPlayerSortedMaps[0].begin(),
                  formattedPlayerSortedMaps[0].end(), FormatString());

    // fill playerSortedMaps and formattedPlayerSortedMaps according to map
    // player count
    for (int i = 0; i < (int)mapFiles.size();
         i++) { // fetch info and put map in right list
      loadMapInfo(Config::getMapPath(mapFiles.at(i), scenarioDir, false),
                  &mapInfo, false, true);

      if (GameConstants::maxPlayers + 1 <= mapInfo.players) {
        char szBuf[8096] = "";
        snprintf(szBuf, 8096,
                 "Sorted map list [%d] does not match\ncurrent map playercount "
                 "[%d]\nfor file [%s]\nmap [%s]",
                 GameConstants::maxPlayers + 1, mapInfo.players,
                 Config::getMapPath(mapFiles.at(i), "", false).c_str(),
                 mapInfo.desc.c_str());
        throw megaglest_runtime_error(szBuf);
      }
      playerSortedMaps[mapInfo.players].push_back(mapFiles.at(i));
      formattedPlayerSortedMaps[mapInfo.players].push_back(
          formatString(mapFiles.at(i)));
      if (config.getString("InitialMap", "Conflict") ==
          formattedPlayerSortedMaps[mapInfo.players].back()) {
        initialMapSelection = i;
      }
    }

    // printf("#6 scenario [%s] [%s]\n",scenario.c_str(),scenarioDir.c_str());
    if (scenario != "") {
      string file = Scenario::getScenarioPath(dirList, scenario);
      loadScenarioInfo(file, &scenarioInfo);

      // printf("#6.1 about to load map [%s]\n",scenarioInfo.mapName.c_str());
      loadMapInfo(Config::getMapPath(scenarioInfo.mapName, scenarioDir, true),
                  &mapInfo, false, true);
      // printf("#6.2\n");
      listBoxMapFilter.setSelectedItem(intToStr(mapInfo.players));
      comboBoxMap.setItems(formattedPlayerSortedMaps[mapInfo.players]);
    } else {
      listBoxMapFilter.setSelectedItemIndex(0);
      comboBoxMap.setItems(formattedPlayerSortedMaps[0]);
    }
    // printf("#7\n");
  } catch (const std::exception &ex) {
    char szBuf[8096] = "";
    snprintf(szBuf, 8096, "In [%s::%s %d]\nError detected:\n%s\n",
             extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
             __LINE__, ex.what());
    SystemFlags::OutputDebug(SystemFlags::debugError, szBuf);
    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem, "%s", szBuf);

    throw megaglest_runtime_error(szBuf);
    // abort();
  }

  return initialMapSelection;
}

int MenuStateCustomGame::setupTechList(string scenario, bool forceLoad) {
  int initialTechSelection = 0;
  try {
    Config &config = Config::getInstance();

    string scenarioDir = Scenario::getScenarioDir(dirList, scenario);
    vector<string> results;
    vector<string> techPaths = config.getPathListForType(ptTechs, scenarioDir);
    findDirs(techPaths, results);

    if (results.empty()) {
      // throw megaglest_runtime_error("No tech-trees were found!");
      printf("No tech-trees were found (custom)!\n");
    }

    techTreeFiles = results;

    vector<string> translatedTechs;

    for (unsigned int i = 0; i < results.size(); i++) {
      // printf("TECHS i = %d results [%s] scenario
      // [%s]\n",i,results[i].c_str(),scenario.c_str());

      results.at(i) = formatString(results.at(i));
      if (config.getString("InitialTechTree", "Megapack") == results.at(i)) {
        initialTechSelection = i;
      }
      string txTech =
          techTree->getTranslatedName(techTreeFiles.at(i), forceLoad);
      translatedTechs.push_back(formatString(txTech));
    }

    listBoxTechTree.setItems(results, translatedTechs);
  } catch (const std::exception &ex) {
    char szBuf[8096] = "";
    snprintf(szBuf, 8096, "In [%s::%s %d]\nError detected:\n%s\n",
             extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
             __LINE__, ex.what());
    SystemFlags::OutputDebug(SystemFlags::debugError, szBuf);
    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem, "%s", szBuf);

    throw megaglest_runtime_error(szBuf);
  }

  return initialTechSelection;
}

void MenuStateCustomGame::reloadFactions(bool keepExistingSelectedItem,
                                         string scenario) {
  try {
    Config &config = Config::getInstance();
    Lang &lang = Lang::getInstance();

    vector<string> results;
    string scenarioDir = Scenario::getScenarioDir(dirList, scenario);
    vector<string> techPaths = config.getPathListForType(ptTechs, scenarioDir);

    // printf("#1 techPaths.size() = %d scenarioDir [%s]
    // [%s]\n",techPaths.size(),scenario.c_str(),scenarioDir.c_str());

    if (listBoxTechTree.getItemCount() > 0) {
      for (int idx = 0; idx < (int)techPaths.size(); idx++) {
        string &techPath = techPaths[idx];
        endPathWithSlash(techPath);
        string factionPath =
            techPath + techTreeFiles[listBoxTechTree.getSelectedItemIndex()] +
            "/factions/";
        findDirs(factionPath, results, false, false);

        // printf("idx = %d factionPath [%s] results.size() =
        // %d\n",idx,factionPath.c_str(),results.size());

        if (results.empty() == false) {
          break;
        }
      }
    }

    if (results.empty() == true) {
      // throw megaglest_runtime_error("(2)There are no factions for the tech
      // tree [" + techTreeFiles[listBoxTechTree.getSelectedItemIndex()] + "]");
      showGeneralError = true;
      if (listBoxTechTree.getItemCount() > 0) {
        generalErrorToShow =
            "[#2] There are no factions for the tech tree [" +
            techTreeFiles[listBoxTechTree.getSelectedItemIndex()] + "]";
      } else {
        generalErrorToShow =
            "[#2] There are no factions since there is no tech tree!";
      }
    }

    //		results.push_back(formatString(GameConstants::RANDOMFACTION_SLOTNAME));
    //
    //		// Add special Observer Faction
    //		if(checkBoxAllowObservers.getValue() == 1) {
    //			results.push_back(formatString(GameConstants::OBSERVER_SLOTNAME));
    //		}

    vector<string> translatedFactionNames;
    factionFiles = results;
    for (int i = 0; i < (int)results.size(); ++i) {
      results[i] = formatString(results[i]);

      string translatedString = "";
      if (listBoxTechTree.getItemCount() > 0) {
        translatedString = techTree->getTranslatedFactionName(
            techTreeFiles[listBoxTechTree.getSelectedItemIndex()],
            factionFiles[i]);
      }
      // printf("translatedString=%s  formatString(results[i])=%s
      // \n",translatedString.c_str(),formatString(results[i]).c_str() );
      if (toLower(translatedString) == toLower(results[i])) {
        translatedFactionNames.push_back(results[i]);
      } else {
        translatedFactionNames.push_back(results[i] + " (" + translatedString +
                                         ")");
      }
      // printf("FACTIONS i = %d results [%s]\n",i,results[i].c_str());

      if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
        SystemFlags::OutputDebug(
            SystemFlags::debugSystem, "Tech [%s] has faction [%s]\n",
            techTreeFiles[listBoxTechTree.getSelectedItemIndex()].c_str(),
            results[i].c_str());
    }
    results.push_back(formatString(GameConstants::RANDOMFACTION_SLOTNAME));
    factionFiles.push_back(formatString(GameConstants::RANDOMFACTION_SLOTNAME));
    translatedFactionNames.push_back("*" + lang.getString("Random", "", true) +
                                     "*");

    // Add special Observer Faction
    if (checkBoxAllowObservers.getValue() == 1) {
      results.push_back(formatString(GameConstants::OBSERVER_SLOTNAME));
      factionFiles.push_back(formatString(GameConstants::OBSERVER_SLOTNAME));
      translatedFactionNames.push_back(
          "*" + lang.getString("Observer", "", true) + "*");
    }

    for (int i = 0; i < GameConstants::maxPlayers; ++i) {
      int originalIndex = listBoxFactions[i].getSelectedItemIndex();
      string originalValue = (listBoxFactions[i].getItemCount() > 0
                                  ? listBoxFactions[i].getSelectedItem()
                                  : "");

      listBoxFactions[i].setItems(results, translatedFactionNames);
      if (keepExistingSelectedItem == false ||
          (checkBoxAllowObservers.getValue() == 0 &&
           originalValue == formatString(GameConstants::OBSERVER_SLOTNAME))) {

        listBoxFactions[i].setSelectedItemIndex(i % results.size());

        if (originalValue == formatString(GameConstants::OBSERVER_SLOTNAME) &&
            listBoxFactions[i].getSelectedItem() !=
                formatString(GameConstants::OBSERVER_SLOTNAME)) {
          if (listBoxTeams[i].getSelectedItem() ==
              intToStr(GameConstants::maxPlayers + fpt_Observer)) {
            listBoxTeams[i].setSelectedItem(intToStr(1));
          }
        }
      } else if (originalIndex < (int)results.size()) {
        listBoxFactions[i].setSelectedItemIndex(originalIndex);
      }
    }
  } catch (const std::exception &ex) {
    char szBuf[8096] = "";
    snprintf(szBuf, 8096, "In [%s::%s %d]\nError detected:\n%s\n",
             extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
             __LINE__, ex.what());
    SystemFlags::OutputDebug(SystemFlags::debugError, szBuf);
    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem, "%s", szBuf);

    throw megaglest_runtime_error(szBuf);
  }
}

void MenuStateCustomGame::setSlotHuman(int i) {
  if (labelPlayerNames[i].getEditable()) {
    return;
  }
  listBoxControls[i].setSelectedItemIndex(ctHuman);
  listBoxRMultiplier[i].setSelectedItem("1.0");

  labelPlayerNames[i].setText(getHumanPlayerName());
  for (int j = 0; j < GameConstants::maxPlayers; ++j) {
    labelPlayerNames[j].setEditable(false);
  }
  labelPlayerNames[i].setEditable(true);
}

void MenuStateCustomGame::setupTilesetList(string scenario) {
  try {
    Config &config = Config::getInstance();

    string scenarioDir = Scenario::getScenarioDir(dirList, scenario);

    vector<string> results;
    findDirs(config.getPathListForType(ptTilesets, scenarioDir), results);
    if (results.empty()) {
      throw megaglest_runtime_error("No tile-sets were found!");
    }
    tilesetFiles = results;
    std::for_each(results.begin(), results.end(), FormatString());

    listBoxTileset.setItems(results);
  } catch (const std::exception &ex) {
    char szBuf[8096] = "";
    snprintf(szBuf, 8096, "In [%s::%s %d]\nError detected:\n%s\n",
             extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
             __LINE__, ex.what());
    SystemFlags::OutputDebug(SystemFlags::debugError, szBuf);
    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem, "%s", szBuf);

    throw megaglest_runtime_error(szBuf);
  }
}

} // namespace Game
} // namespace Glest
