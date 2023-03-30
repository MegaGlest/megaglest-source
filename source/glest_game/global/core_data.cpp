// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "core_data.h"

#include "byte_order.h"
#include "config.h"
#include "game_constants.h"
#include "game_settings.h"
#include "game_util.h"
#include "graphics_interface.h"
#include "lang.h"
#include "leak_dumper.h"
#include "logger.h"
#include "platform_util.h"
#include "renderer.h"
#include "util.h"
#include "video_player.h"

using namespace Shared::Sound;
using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest {
namespace Game {

// =====================================================
// 	class CoreData
// =====================================================

static string tempDataLocation = getUserHome();
// ===================== PUBLIC ========================

static const string CORE_PATH = "data/core/";
static const string CORE_MISC_TEXTURES_PATH = CORE_PATH + "misc_textures/";

static const string CORE_MENU_PATH = CORE_PATH + "menu/";
static const string CORE_MENU_TEXTURES_PATH = CORE_MENU_PATH + "textures/";
static const string CORE_MENU_SOUND_PATH = CORE_MENU_PATH + "sound/";
static const string CORE_MENU_MUSIC_PATH = CORE_MENU_PATH + "music/";
static const string CORE_MENU_VIDEOS_PATH = CORE_MENU_PATH + "videos/";

static const string CORE_WATER_SOUNDS_PATH = CORE_PATH + "/water_sounds/";

CoreData &CoreData::getInstance() {
  static CoreData coreData;
  return coreData;
}

CoreData::CoreData() {
  logoTexture = NULL;
  logoTextureList.clear();
  backgroundTexture = NULL;
  fireTexture = NULL;
  teamColorTexture = NULL;
  snowTexture = NULL;
  waterSplashTexture = NULL;
  customTexture = NULL;
  buttonSmallTexture = NULL;
  buttonBigTexture = NULL;
  horizontalLineTexture = NULL;
  verticalLineTexture = NULL;
  checkBoxTexture = NULL;
  checkedCheckBoxTexture = NULL;
  gameWinnerTexture = NULL;
  notOnServerTexture = NULL;
  onServerDifferentTexture = NULL;
  onServerTexture = NULL;
  onServerInstalledTexture = NULL;
  statusReadyTexture = NULL;
  statusNotReadyTexture = NULL;
  statusBRBTexture = NULL;

  healthbarTexture = NULL;
  healthbarBackgroundTexture = NULL;

  miscTextureList.clear();

  displayFont = NULL;
  menuFontNormal = NULL;
  displayFontSmall = NULL;
  menuFontBig = NULL;
  menuFontVeryBig = NULL;
  consoleFont = NULL;

  displayFont3D = NULL;
  menuFontNormal3D = NULL;
  displayFontSmall3D = NULL;
  menuFontBig3D = NULL;
  menuFontVeryBig3D = NULL;
  consoleFont3D = NULL;

  introVideoFilename = "";
  mainMenuVideoFilename = "";

  battleEndWinVideoFilename = "";
  battleEndWinVideoFilenameFallback = "";
  battleEndWinMusicFilename = "";
  battleEndLoseVideoFilename = "";
  battleEndLoseVideoFilenameFallback = "";
  battleEndLoseMusicFilename = "";
}

CoreData::~CoreData() { cleanup(); }

void CoreData::cleanup() {
  deleteValues(waterSounds.getSoundsPtr()->begin(),
               waterSounds.getSoundsPtr()->end());
  waterSounds.getSoundsPtr()->clear();
}

Texture2D *CoreData::getTextureBySystemId(TextureSystemType type) {
  Texture2D *result = NULL;
  switch (type) {
  case tsyst_logoTexture:
    result = getLogoTexture();
    break;
  // std::vector<Texture2D *> logoTextureList;
  case tsyst_backgroundTexture:
    result = getBackgroundTexture();
    break;
  case tsyst_fireTexture:
    result = getFireTexture();
    break;
  case tsyst_teamColorTexture:
    result = getTeamColorTexture();
    break;
  case tsyst_snowTexture:
    result = getSnowTexture();
    break;
  case tsyst_waterSplashTexture:
    result = getWaterSplashTexture();
    break;
  case tsyst_customTexture:
    result = getCustomTexture();
    break;
  case tsyst_buttonSmallTexture:
    result = buttonSmallTexture;
    break;
  case tsyst_buttonBigTexture:
    result = buttonBigTexture;
    break;
  case tsyst_horizontalLineTexture:
    result = horizontalLineTexture;
    break;
  case tsyst_verticalLineTexture:
    result = verticalLineTexture;
    break;
  case tsyst_checkBoxTexture:
    result = checkBoxTexture;
    break;
  case tsyst_checkedCheckBoxTexture:
    result = checkedCheckBoxTexture;
    break;
  case tsyst_gameWinnerTexture:
    result = gameWinnerTexture;
    break;
  case tsyst_notOnServerTexture:
    result = notOnServerTexture;
    break;
  case tsyst_onServerDifferentTexture:
    result = onServerDifferentTexture;
    break;
  case tsyst_onServerTexture:
    result = onServerTexture;
    break;
  case tsyst_onServerInstalledTexture:
    result = onServerInstalledTexture;
    break;
  case tsyst_statusReadyTexture:
    result = statusReadyTexture;
    break;
  case tsyst_statusNotReadyTexture:
    result = statusNotReadyTexture;
    break;
  case tsyst_statusBRBTexture:
    result = statusBRBTexture;
    break;
  case tsyst_healthbarTexture:
    result = healthbarTexture;
    break;
  case tsyst_healthbarBackgroundTexture:
    result = healthbarBackgroundTexture;
    break;

    // std::vector<Texture2D *> miscTextureList;
  }
  return result;
}

void CoreData::cleanupTexture(Texture2D **texture) {
  Renderer &renderer = Renderer::getInstance();
  renderer.endTexture(rsGlobal, *texture);
  *texture = NULL;
}

void CoreData::loadTextureIfRequired(Texture2D **tex, string data_path,
                                     string uniqueFilePath, int texSystemId,
                                     bool setMipMap, bool setAlpha,
                                     bool loadUniqueFilePath,
                                     bool compressionDisabled) {
  if (*tex == NULL) {
    bool attemptToLoadTexture = (texSystemId == tsyst_NONE);
    if (attemptToLoadTexture == false &&
        itemLoadAttempted.find(texSystemId) == itemLoadAttempted.end()) {

      attemptToLoadTexture = true;
      itemLoadAttempted[texSystemId] = true;
    }

    if (attemptToLoadTexture == true) {
      Renderer &renderer = Renderer::getInstance();
      *tex = renderer.newTexture2D(rsGlobal);
      if (*tex) {

        (*tex)->setForceCompressionDisabled(compressionDisabled);
        (*tex)->setMipmap(setMipMap);
        if (setAlpha == true) {

          (*tex)->setFormat(Texture::fAlpha);
          (*tex)->getPixmap()->init(1);
        }

        try {
          string fileToLoad = uniqueFilePath;
          if (loadUniqueFilePath == false) {

            fileToLoad = getGameCustomCoreDataPath(data_path, uniqueFilePath);
          }
          (*tex)->getPixmap()->load(fileToLoad);
          (*tex)->setTextureSystemId(texSystemId);

          renderer.initTexture(rsGlobal, *tex);
        } catch (const megaglest_runtime_error &ex) {
          message(ex.what(), GlobalStaticFlags::getIsNonGraphicalModeEnabled(),
                  tempDataLocation);
          cleanupTexture(tex);
        }
      }
    }
  }
}

string CoreData::getDataPath() {
  string data_path =
      getGameReadWritePath(GameConstants::path_data_CacheLookupKey);
  if (data_path != "") {
    endPathWithSlash(data_path);
  }
  return data_path;
}

Texture2D *CoreData::getBackgroundTexture() {
  string data_path = getDataPath();
  loadTextureIfRequired(&backgroundTexture, getDataPath(),
                        CORE_MENU_TEXTURES_PATH + "back.tga",
                        tsyst_backgroundTexture, false, false, false);

  return backgroundTexture;
}
Texture2D *CoreData::getFireTexture() {
  string data_path = getDataPath();
  loadTextureIfRequired(&fireTexture, data_path,
                        CORE_MISC_TEXTURES_PATH + "fire_particle.tga",
                        tsyst_fireTexture, true, true, false);

  return fireTexture;
}
Texture2D *CoreData::getTeamColorTexture() {
  string data_path = getDataPath();
  loadTextureIfRequired(&teamColorTexture, data_path,
                        CORE_MISC_TEXTURES_PATH + "team_color_texture.tga",
                        tsyst_teamColorTexture, true, true, false);

  return teamColorTexture;
}
Texture2D *CoreData::getSnowTexture() {
  string data_path = getDataPath();
  loadTextureIfRequired(&snowTexture, data_path,
                        CORE_MISC_TEXTURES_PATH + "snow_particle.tga",
                        tsyst_snowTexture, false, true, false);

  return snowTexture;
}
Texture2D *CoreData::getLogoTexture() {
  string data_path = getDataPath();
  loadTextureIfRequired(&logoTexture, data_path,
                        CORE_MENU_TEXTURES_PATH + "logo.tga", tsyst_logoTexture,
                        false, false, false);

  return logoTexture;
}
Texture2D *CoreData::getWaterSplashTexture() {
  string data_path = getDataPath();
  loadTextureIfRequired(&waterSplashTexture, data_path,
                        CORE_MISC_TEXTURES_PATH + "water_splash.tga",
                        tsyst_waterSplashTexture, true, true, false);

  return waterSplashTexture;
}
Texture2D *CoreData::getCustomTexture() {
  string data_path = getDataPath();
  loadTextureIfRequired(&customTexture, data_path,
                        CORE_MENU_TEXTURES_PATH + "custom_texture.tga",
                        tsyst_customTexture, true, false, false);

  return customTexture;
}
Texture2D *CoreData::getButtonSmallTexture() {
  string data_path = getDataPath();
  loadTextureIfRequired(&buttonSmallTexture, data_path,
                        CORE_MENU_TEXTURES_PATH + "button_small.tga",
                        tsyst_buttonSmallTexture, true, false, false, true);

  return buttonSmallTexture;
}
Texture2D *CoreData::getButtonBigTexture() {
  string data_path = getDataPath();
  loadTextureIfRequired(&buttonBigTexture, data_path,
                        CORE_MENU_TEXTURES_PATH + "button_big.tga",
                        tsyst_buttonBigTexture, true, false, false, true);

  return buttonBigTexture;
}
Texture2D *CoreData::getHorizontalLineTexture() {
  string data_path = getDataPath();
  loadTextureIfRequired(&horizontalLineTexture, data_path,
                        CORE_MENU_TEXTURES_PATH + "line_horizontal.tga",
                        tsyst_horizontalLineTexture, true, false, false, true);

  return horizontalLineTexture;
}
Texture2D *CoreData::getVerticalLineTexture() {
  string data_path = getDataPath();
  loadTextureIfRequired(&verticalLineTexture, data_path,
                        CORE_MENU_TEXTURES_PATH + "line_vertical.tga",
                        tsyst_verticalLineTexture, true, false, false, true);

  return verticalLineTexture;
}
Texture2D *CoreData::getCheckBoxTexture() {
  string data_path = getDataPath();
  loadTextureIfRequired(&checkBoxTexture, data_path,
                        CORE_MENU_TEXTURES_PATH + "checkbox.tga",
                        tsyst_checkBoxTexture, true, false, false, true);

  return checkBoxTexture;
}
Texture2D *CoreData::getCheckedCheckBoxTexture() {
  string data_path = getDataPath();
  loadTextureIfRequired(&checkedCheckBoxTexture, data_path,
                        CORE_MENU_TEXTURES_PATH + "checkbox_checked.tga",
                        tsyst_checkedCheckBoxTexture, true, false, false, true);

  return checkedCheckBoxTexture;
}
Texture2D *CoreData::getNotOnServerTexture() {
  string data_path = getDataPath();
  loadTextureIfRequired(&notOnServerTexture, data_path,
                        CORE_MENU_TEXTURES_PATH + "not_on_server.tga",
                        tsyst_notOnServerTexture, true, false, false);

  return notOnServerTexture;
}
Texture2D *CoreData::getOnServerDifferentTexture() {
  string data_path = getDataPath();
  loadTextureIfRequired(&onServerDifferentTexture, data_path,
                        CORE_MENU_TEXTURES_PATH + "on_server_different.tga",
                        tsyst_onServerDifferentTexture, true, false, false);

  return onServerDifferentTexture;
}
Texture2D *CoreData::getOnServerTexture() {
  string data_path = getDataPath();
  loadTextureIfRequired(&onServerTexture, data_path,
                        CORE_MENU_TEXTURES_PATH + "on_server.tga",
                        tsyst_onServerTexture, true, false, false);

  return onServerTexture;
}
Texture2D *CoreData::getOnServerInstalledTexture() {
  string data_path = getDataPath();
  loadTextureIfRequired(&onServerInstalledTexture, data_path,
                        CORE_MENU_TEXTURES_PATH + "on_server_installed.tga",
                        tsyst_onServerInstalledTexture, true, false, false);

  return onServerInstalledTexture;
}
Texture2D *CoreData::getStatusReadyTexture() {
  string data_path = getDataPath();
  loadTextureIfRequired(&statusReadyTexture, data_path,
                        CORE_MENU_TEXTURES_PATH + "status_ready.png",
                        tsyst_statusReadyTexture, true, false, false);

  return statusReadyTexture;
}
Texture2D *CoreData::getStatusNotReadyTexture() {
  string data_path = getDataPath();
  loadTextureIfRequired(&statusNotReadyTexture, data_path,
                        CORE_MENU_TEXTURES_PATH + "status_notready.png",
                        tsyst_statusNotReadyTexture, true, false, false);

  return statusNotReadyTexture;
}
Texture2D *CoreData::getStatusBRBTexture() {
  string data_path = getDataPath();
  loadTextureIfRequired(&statusBRBTexture, data_path,
                        CORE_MENU_TEXTURES_PATH + "status_brb.png",
                        tsyst_statusBRBTexture, true, false, false);

  return statusBRBTexture;
}
Texture2D *CoreData::getGameWinnerTexture() {
  string data_path = getDataPath();
  loadTextureIfRequired(&gameWinnerTexture, data_path,
                        CORE_MISC_TEXTURES_PATH + "game_winner.png",
                        tsyst_gameWinnerTexture, true, false, false, true);

  return gameWinnerTexture;
}

Texture2D *CoreData::getHealthbarTexture() {
  string data_path = getDataPath();
  loadTextureIfRequired(&healthbarTexture, data_path,
                        CORE_MISC_TEXTURES_PATH + "healthbar.png",
                        tsyst_healthbarTexture, true, false, false, true);

  return healthbarTexture;
}

Texture2D *CoreData::getHealthbarBackgroundTexture() {
  string data_path = getDataPath();
  loadTextureIfRequired(&healthbarBackgroundTexture, data_path,
                        CORE_MISC_TEXTURES_PATH + "healthbarBackground.png",
                        tsyst_healthbarBackgroundTexture, true, false, false,
                        true);

  return healthbarBackgroundTexture;
}

void CoreData::loadLogoTextureExtraIfRequired() {
  int loadAttemptLookupKey = tsyst_COUNT + 1;
  if (itemLoadAttempted.find(loadAttemptLookupKey) == itemLoadAttempted.end()) {

    itemLoadAttempted[loadAttemptLookupKey] = true;

    string data_path = getDataPath();
    logoTextureList.clear();
    string logosPath = getGameCustomCoreDataPath(data_path, "") +
                       CORE_MENU_TEXTURES_PATH + "logo*.*";
    vector<string> logoFilenames;
    findAll(logosPath, logoFilenames, false, false);
    for (int index = 0; index < (int)logoFilenames.size(); ++index) {

      string logo = logoFilenames[index];
      if (strcmp("logo.tga", logo.c_str()) != 0) {

        Texture2D *logoTextureExtra = NULL;
        loadTextureIfRequired(&logoTextureExtra, data_path,
                              getGameCustomCoreDataPath(data_path, "") +
                                  CORE_MENU_TEXTURES_PATH + logo,
                              tsyst_NONE, true, false, true);
        logoTextureList.push_back(logoTextureExtra);
      }
    }
    if (logoTextureList.empty() == true) {

      logosPath = data_path + CORE_MENU_TEXTURES_PATH + "logo*.*";
      vector<string> logoFilenames;
      findAll(logosPath, logoFilenames, false, false);
      for (int index = 0; index < (int)logoFilenames.size(); ++index) {

        string logo = logoFilenames[index];
        if (strcmp("logo.tga", logo.c_str()) != 0) {

          Texture2D *logoTextureExtra = NULL;
          loadTextureIfRequired(&logoTextureExtra, data_path,
                                data_path + CORE_MENU_TEXTURES_PATH + logo,
                                tsyst_NONE, true, false, true);
          logoTextureList.push_back(logoTextureExtra);
        }
      }
    }
  }
}
size_t CoreData::getLogoTextureExtraCount() {
  loadLogoTextureExtraIfRequired();
  return logoTextureList.size();
}
Texture2D *CoreData::getLogoTextureExtra(int idx) {
  loadLogoTextureExtraIfRequired();
  return logoTextureList[idx];
}

void CoreData::loadMiscTextureListIfRequired() {
  int loadAttemptLookupKey = tsyst_COUNT + 2;
  if (itemLoadAttempted.find(loadAttemptLookupKey) == itemLoadAttempted.end()) {

    itemLoadAttempted[loadAttemptLookupKey] = true;

    string data_path = getDataPath();

    miscTextureList.clear();
    string introPath = getGameCustomCoreDataPath(data_path, "") +
                       CORE_MENU_TEXTURES_PATH + "intro*.*";
    vector<string> introFilenames;
    findAll(introPath, introFilenames, false, false);
    for (int i = 0; i < (int)introFilenames.size(); ++i) {
      string logo = introFilenames[i];

      Texture2D *logoTextureExtra = NULL;
      loadTextureIfRequired(&logoTextureExtra, data_path,
                            getGameCustomCoreDataPath(data_path, "") +
                                CORE_MENU_TEXTURES_PATH + logo,
                            tsyst_NONE, true, false, true);
      miscTextureList.push_back(logoTextureExtra);
    }
    if (miscTextureList.empty() == true) {
      introPath = data_path + CORE_MENU_TEXTURES_PATH + "intro*.*";
      vector<string> introFilenames;
      findAll(introPath, introFilenames, false, false);
      for (int i = 0; i < (int)introFilenames.size(); ++i) {
        string logo = introFilenames[i];

        Texture2D *logoTextureExtra = NULL;
        loadTextureIfRequired(&logoTextureExtra, data_path,
                              data_path + CORE_MENU_TEXTURES_PATH + logo,
                              tsyst_NONE, true, false, true);
        miscTextureList.push_back(logoTextureExtra);
      }
    }
  }
}

std::vector<Texture2D *> &CoreData::getMiscTextureList() {
  loadMiscTextureListIfRequired();
  return miscTextureList;
}

void CoreData::loadTextures(string data_path) {
  // Required to be loaded at program startup as they may be accessed in
  // threads or some other dangerous way so lazy loading is not an option
  getCustomTexture();
}

StaticSound *CoreData::getClickSoundA() {
  int loadAttemptLookupKey = tsyst_COUNT + 3;
  if (itemLoadAttempted.find(loadAttemptLookupKey) == itemLoadAttempted.end()) {

    itemLoadAttempted[loadAttemptLookupKey] = true;

    try {
      string data_path = getDataPath();
      clickSoundA.load(getGameCustomCoreDataPath(
          data_path, CORE_MENU_SOUND_PATH + "click_a.wav"));
    } catch (const megaglest_runtime_error &ex) {
      message(ex.what(), GlobalStaticFlags::getIsNonGraphicalModeEnabled(),
              tempDataLocation);
    }
  }
  return &clickSoundA;
}

StaticSound *CoreData::getClickSoundB() {
  int loadAttemptLookupKey = tsyst_COUNT + 4;
  if (itemLoadAttempted.find(loadAttemptLookupKey) == itemLoadAttempted.end()) {

    itemLoadAttempted[loadAttemptLookupKey] = true;

    try {
      string data_path = getDataPath();
      clickSoundB.load(getGameCustomCoreDataPath(
          data_path, CORE_MENU_SOUND_PATH + "click_b.wav"));
    } catch (const megaglest_runtime_error &ex) {
      message(ex.what(), GlobalStaticFlags::getIsNonGraphicalModeEnabled(),
              tempDataLocation);
    }
  }

  return &clickSoundB;
}
StaticSound *CoreData::getClickSoundC() {
  int loadAttemptLookupKey = tsyst_COUNT + 5;
  if (itemLoadAttempted.find(loadAttemptLookupKey) == itemLoadAttempted.end()) {

    itemLoadAttempted[loadAttemptLookupKey] = true;

    try {
      string data_path = getDataPath();
      clickSoundC.load(getGameCustomCoreDataPath(
          data_path, CORE_MENU_SOUND_PATH + "click_c.wav"));
    } catch (const megaglest_runtime_error &ex) {
      message(ex.what(), GlobalStaticFlags::getIsNonGraphicalModeEnabled(),
              tempDataLocation);
    }
  }

  return &clickSoundC;
}
StaticSound *CoreData::getAttentionSound() {
  int loadAttemptLookupKey = tsyst_COUNT + 6;
  if (itemLoadAttempted.find(loadAttemptLookupKey) == itemLoadAttempted.end()) {

    itemLoadAttempted[loadAttemptLookupKey] = true;

    try {
      string data_path = getDataPath();
      attentionSound.load(getGameCustomCoreDataPath(
          data_path, CORE_MENU_SOUND_PATH + "attention.wav"));
    } catch (const megaglest_runtime_error &ex) {
      message(ex.what(), GlobalStaticFlags::getIsNonGraphicalModeEnabled(),
              tempDataLocation);
    }
  }

  return &attentionSound;
}
StaticSound *CoreData::getHighlightSound() {
  int loadAttemptLookupKey = tsyst_COUNT + 7;
  if (itemLoadAttempted.find(loadAttemptLookupKey) == itemLoadAttempted.end()) {

    itemLoadAttempted[loadAttemptLookupKey] = true;

    try {
      string data_path = getDataPath();
      highlightSound.load(getGameCustomCoreDataPath(
          data_path, CORE_MENU_SOUND_PATH + "highlight.wav"));
    } catch (const megaglest_runtime_error &ex) {
      message(ex.what(), GlobalStaticFlags::getIsNonGraphicalModeEnabled(),
              tempDataLocation);
    }
  }

  return &highlightSound;
}
StaticSound *CoreData::getMarkerSound() {
  int loadAttemptLookupKey = tsyst_COUNT + 8;
  if (itemLoadAttempted.find(loadAttemptLookupKey) == itemLoadAttempted.end()) {

    itemLoadAttempted[loadAttemptLookupKey] = true;

    try {
      string data_path = getDataPath();
      markerSound.load(getGameCustomCoreDataPath(
          data_path, CORE_MENU_SOUND_PATH + "sonar.wav"));
    } catch (const megaglest_runtime_error &ex) {
      message(ex.what(), GlobalStaticFlags::getIsNonGraphicalModeEnabled(),
              tempDataLocation);
    }
  }

  return &markerSound;
}

void CoreData::loadWaterSoundsIfRequired() {
  int loadAttemptLookupKey = tsyst_COUNT + 9;
  if (itemLoadAttempted.find(loadAttemptLookupKey) == itemLoadAttempted.end()) {

    itemLoadAttempted[loadAttemptLookupKey] = true;

    string data_path = getDataPath();
    cleanup();
    waterSounds.resize(6);

    for (int index = 0; index < 6; ++index) {
      waterSounds[index] = new StaticSound();
      if (waterSounds[index] != NULL) {
        try {
          waterSounds[index]->load(getGameCustomCoreDataPath(
              data_path,
              CORE_WATER_SOUNDS_PATH + "water" + intToStr(index) + ".wav"));
        } catch (const megaglest_runtime_error &ex) {
          message(ex.what(), GlobalStaticFlags::getIsNonGraphicalModeEnabled(),
                  tempDataLocation);
        }
      }
    }
  }
}

StaticSound *CoreData::getWaterSound() {
  loadWaterSoundsIfRequired();
  return waterSounds.getRandSound();
}

void CoreData::loadSounds(string data_path) {
  // sounds
  //	try {
  //		clickSoundA.load(
  //				getGameCustomCoreDataPath(data_path,
  //						CORE_MENU_SOUND_PATH +
  //"click_a.wav")); 		clickSoundB.load(
  //getGameCustomCoreDataPath(data_path, 						CORE_MENU_SOUND_PATH + "click_b.wav"));
  //clickSoundC.load( getGameCustomCoreDataPath(data_path, 						CORE_MENU_SOUND_PATH
  //+ "click_c.wav")); 		attentionSound.load(
  //getGameCustomCoreDataPath(data_path, 						CORE_MENU_SOUND_PATH +
  //"attention.wav")); 		highlightSound.load(
  //getGameCustomCoreDataPath(data_path, 						CORE_MENU_SOUND_PATH +
  //"highlight.wav")); 		markerSound.load(
  //getGameCustomCoreDataPath(data_path, 						CORE_MENU_SOUND_PATH + "sonar.wav"));
  //	}
  //	catch (const megaglest_runtime_error& ex) {
  //		message(ex.what(),
  // GlobalStaticFlags::getIsNonGraphicalModeEnabled(),
  // tempDataLocation);
  //	}

  //	cleanup();
  //	waterSounds.resize(6);
  //
  //	for (int i = 0; i < 6; ++i) {
  //		waterSounds[i] = new StaticSound();
  //		if (waterSounds[i]) {
  //			try {
  //				waterSounds[i]->load(
  //						getGameCustomCoreDataPath(data_path,
  //								CORE_WATER_SOUNDS_PATH + "water"
  //+ intToStr(i)
  //										+
  //".wav")); 			} catch (const megaglest_runtime_error& ex) {
  //message(ex.what(), 						GlobalStaticFlags::getIsNonGraphicalModeEnabled(),
  //						tempDataLocation);
  //			}
  //		}
  //	}
}

void CoreData::loadMusicIfRequired() {
  int loadAttemptLookupKey = tsyst_COUNT + 10;
  if (itemLoadAttempted.find(loadAttemptLookupKey) == itemLoadAttempted.end()) {

    itemLoadAttempted[loadAttemptLookupKey] = true;

    string data_path = getDataPath();

    XmlTree xmlTree;
    xmlTree.load(
        getGameCustomCoreDataPath(data_path, CORE_MENU_PATH + "menu.xml"),
        Properties::getTagReplacementValues());
    const XmlNode *menuNode = xmlTree.getRootNode();
    string menuMusicPath = "/menu/music/";
    string menuIntroMusicFile = "intro_music.ogg";
    string menuMusicFile = "menu_music.ogg";
    if (menuNode->hasChild("intro") == true) {
      const XmlNode *introNode = menuNode->getChild("intro");
      // intro info
      const XmlNode *menuPathNode = introNode->getChild("menu-music-path");
      menuMusicPath = menuPathNode->getAttribute("value")->getRestrictedValue();
      const XmlNode *menuIntroMusicNode =
          introNode->getChild("menu-intro-music");
      menuIntroMusicFile =
          menuIntroMusicNode->getAttribute("value")->getRestrictedValue();
      const XmlNode *menuMusicNode = introNode->getChild("menu-music");
      menuMusicFile =
          menuMusicNode->getAttribute("value")->getRestrictedValue();
    }
    try {
      introMusic.open(getGameCustomCoreDataPath(
          data_path, CORE_PATH + menuMusicPath + menuIntroMusicFile));
      introMusic.setNext(&menuMusic);
      menuMusic.open(getGameCustomCoreDataPath(
          data_path, CORE_PATH + menuMusicPath + menuMusicFile));
      menuMusic.setNext(&menuMusic);
    } catch (const megaglest_runtime_error &ex) {
      message(ex.what(), GlobalStaticFlags::getIsNonGraphicalModeEnabled(),
              tempDataLocation);
    }
  }
}

StrSound *CoreData::getIntroMusic() {
  loadMusicIfRequired();
  return &introMusic;
}

StrSound *CoreData::getMenuMusic() {
  loadMusicIfRequired();
  return &menuMusic;
}

void CoreData::loadMusic(string data_path) {}

void CoreData::loadIntroMedia(string data_path) {
  Config &config = Config::getInstance();

  introVideoFilename = config.getString("IntroVideoURL", "");
  introVideoFilenameFallback = config.getString("IntroVideoURLFallback", "");
  if (introVideoFilename == "") {
    string introVideoPath = getGameCustomCoreDataPath(data_path, "") +
                            CORE_MENU_VIDEOS_PATH + "intro.*";
    vector<string> introVideos;
    findAll(introVideoPath, introVideos, false, false);
    for (int i = 0; i < (int)introVideos.size(); ++i) {
      string video = getGameCustomCoreDataPath(data_path, "") +
                     CORE_MENU_VIDEOS_PATH + introVideos[i];
      if (SystemFlags::VERBOSE_MODE_ENABLED)
        printf("Checking if intro video [%s] exists\n", video.c_str());

      if (fileExists(video)) {
        introVideoFilename = video;
        if (SystemFlags::VERBOSE_MODE_ENABLED)
          printf("FOUND intro video [%s] will use this file\n", video.c_str());

        break;
      }
    }
    if (introVideoFilename == "") {
      introVideoPath = data_path + CORE_MENU_VIDEOS_PATH + "intro.*";
      introVideos.clear();
      findAll(introVideoPath, introVideos, false, false);
      for (int i = 0; i < (int)introVideos.size(); ++i) {
        string video = data_path + CORE_MENU_VIDEOS_PATH + introVideos[i];
        if (SystemFlags::VERBOSE_MODE_ENABLED)
          printf("Checking if intro video [%s] exists\n", video.c_str());

        if (fileExists(video)) {
          introVideoFilename = video;
          if (SystemFlags::VERBOSE_MODE_ENABLED)
            printf("FOUND intro video [%s] will use this file\n",
                   video.c_str());

          break;
        }
      }
    }
  }
}

void CoreData::loadMainMenuMedia(string data_path) {
  Config &config = Config::getInstance();

  mainMenuVideoFilename = config.getString("MainMenuVideoURL", "");
  mainMenuVideoFilenameFallback =
      config.getString("MainMenuVideoURLFallback", "");
  if (mainMenuVideoFilename == "") {
    string mainVideoPath = getGameCustomCoreDataPath(data_path, "") +
                           CORE_MENU_VIDEOS_PATH + "main.*";
    vector<string> mainVideos;
    findAll(mainVideoPath, mainVideos, false, false);
    for (int i = 0; i < (int)mainVideos.size(); ++i) {
      string video = getGameCustomCoreDataPath(data_path, "") +
                     CORE_MENU_VIDEOS_PATH + mainVideos[i];
      if (SystemFlags::VERBOSE_MODE_ENABLED)
        printf("Checking if mainmenu video [%s] exists\n", video.c_str());

      if (fileExists(video)) {
        mainMenuVideoFilename = video;
        if (SystemFlags::VERBOSE_MODE_ENABLED)
          printf("FOUND mainmenu video [%s] will use this file\n",
                 video.c_str());

        break;
      }
    }
    if (mainMenuVideoFilename == "") {
      mainVideoPath = data_path + CORE_MENU_VIDEOS_PATH + "main.*";
      mainVideos.clear();
      findAll(mainVideoPath, mainVideos, false, false);
      for (int i = 0; i < (int)mainVideos.size(); ++i) {
        string video = data_path + CORE_MENU_VIDEOS_PATH + mainVideos[i];
        if (SystemFlags::VERBOSE_MODE_ENABLED)
          printf("Checking if mainmenu video [%s] exists\n", video.c_str());

        if (fileExists(video)) {
          mainMenuVideoFilename = video;
          if (SystemFlags::VERBOSE_MODE_ENABLED)
            printf("FOUND mainmenu video [%s] will use this file\n",
                   video.c_str());

          break;
        }
      }
    }
  }
}

void CoreData::loadBattleEndMedia(string data_path) {
  Config &config = Config::getInstance();

  battleEndWinVideoFilename = config.getString("BattleEndWinVideoURL", "");
  battleEndWinVideoFilenameFallback =
      config.getString("BattleEndWinVideoURLFallback", "");
  if (battleEndWinVideoFilename == "") {
    string battleEndWinVideoPath = getGameCustomCoreDataPath(data_path, "") +
                                   CORE_MENU_VIDEOS_PATH + "battle_end_win.*";
    vector<string> battleEndWinVideos;
    findAll(battleEndWinVideoPath, battleEndWinVideos, false, false);
    for (int i = 0; i < (int)battleEndWinVideos.size(); ++i) {
      string video = getGameCustomCoreDataPath(data_path, "") +
                     CORE_MENU_VIDEOS_PATH + battleEndWinVideos[i];
      if (SystemFlags::VERBOSE_MODE_ENABLED)
        printf("Checking if battle end win video [%s] exists\n", video.c_str());

      if (fileExists(video)) {
        battleEndWinVideoFilename = video;
        if (SystemFlags::VERBOSE_MODE_ENABLED)
          printf("FOUND battle end win video [%s] will use this file\n",
                 video.c_str());

        break;
      }
    }
    if (battleEndWinVideoFilename == "") {
      battleEndWinVideoPath =
          data_path + CORE_MENU_VIDEOS_PATH + "battle_end_win.*";
      battleEndWinVideos.clear();
      findAll(battleEndWinVideoPath, battleEndWinVideos, false, false);
      for (int i = 0; i < (int)battleEndWinVideos.size(); ++i) {
        string video =
            data_path + CORE_MENU_VIDEOS_PATH + battleEndWinVideos[i];
        if (SystemFlags::VERBOSE_MODE_ENABLED)
          printf("Checking if battle end win video [%s] exists\n",
                 video.c_str());

        if (fileExists(video)) {
          battleEndWinVideoFilename = video;
          if (SystemFlags::VERBOSE_MODE_ENABLED)
            printf("FOUND battle end video win [%s] will use this file\n",
                   video.c_str());

          break;
        }
      }
    }
  }
  battleEndWinMusicFilename = config.getString("BattleEndWinMusicFilename", "");
  if (battleEndWinMusicFilename == "") {
    string battleEndWinPath = getGameCustomCoreDataPath(data_path, "") +
                              CORE_MENU_MUSIC_PATH + "battle_end_win.*";
    vector<string> battleEndWinMusic;
    findAll(battleEndWinPath, battleEndWinMusic, false, false);
    for (int i = 0; i < (int)battleEndWinMusic.size(); ++i) {
      string music = getGameCustomCoreDataPath(data_path, "") +
                     CORE_MENU_MUSIC_PATH + battleEndWinMusic[i];
      if (SystemFlags::VERBOSE_MODE_ENABLED)
        printf("Checking if battle end win music [%s] exists\n", music.c_str());

      if (fileExists(music)) {
        battleEndWinMusicFilename = music;
        if (SystemFlags::VERBOSE_MODE_ENABLED)
          printf("FOUND battle end win music [%s] will use this file\n",
                 music.c_str());

        break;
      }
    }
    if (battleEndWinMusicFilename == "") {
      battleEndWinPath = data_path + CORE_MENU_MUSIC_PATH + "battle_end_win.*";
      battleEndWinMusic.clear();
      findAll(battleEndWinPath, battleEndWinMusic, false, false);
      for (int i = 0; i < (int)battleEndWinMusic.size(); ++i) {
        string music = data_path + CORE_MENU_MUSIC_PATH + battleEndWinMusic[i];
        if (SystemFlags::VERBOSE_MODE_ENABLED)
          printf("Checking if battle end win music [%s] exists\n",
                 music.c_str());

        if (fileExists(music)) {
          battleEndWinMusicFilename = music;
          if (SystemFlags::VERBOSE_MODE_ENABLED)
            printf("FOUND battle end music win [%s] will use this file\n",
                   music.c_str());

          break;
        }
      }
    }
  }
  battleEndLoseVideoFilename = config.getString("BattleEndLoseVideoURL", "");
  battleEndLoseVideoFilenameFallback =
      config.getString("BattleEndLoseVideoURLFallback", "");
  if (battleEndLoseVideoFilename == "") {
    string battleEndLoseVideoPath = getGameCustomCoreDataPath(data_path, "") +
                                    CORE_MENU_VIDEOS_PATH + "battle_end_lose.*";
    vector<string> battleEndLoseVideos;
    findAll(battleEndLoseVideoPath, battleEndLoseVideos, false, false);
    for (int i = 0; i < (int)battleEndLoseVideos.size(); ++i) {
      string video = getGameCustomCoreDataPath(data_path, "") +
                     CORE_MENU_VIDEOS_PATH + battleEndLoseVideos[i];
      if (SystemFlags::VERBOSE_MODE_ENABLED)
        printf("Checking if battle end lose video [%s] exists\n",
               video.c_str());

      if (fileExists(video)) {
        battleEndLoseVideoFilename = video;
        if (SystemFlags::VERBOSE_MODE_ENABLED)
          printf("FOUND battle end lose video [%s] will use this file\n",
                 video.c_str());

        break;
      }
    }
    if (battleEndLoseVideoFilename == "") {
      battleEndLoseVideoPath =
          data_path + CORE_MENU_VIDEOS_PATH + "battle_end_lose.*";
      battleEndLoseVideos.clear();
      findAll(battleEndLoseVideoPath, battleEndLoseVideos, false, false);
      for (int i = 0; i < (int)battleEndLoseVideos.size(); ++i) {
        string video =
            data_path + CORE_MENU_VIDEOS_PATH + battleEndLoseVideos[i];
        if (SystemFlags::VERBOSE_MODE_ENABLED)
          printf("Checking if battle end lose video [%s] exists\n",
                 video.c_str());

        if (fileExists(video)) {
          battleEndLoseVideoFilename = video;
          if (SystemFlags::VERBOSE_MODE_ENABLED)
            printf("FOUND battle end video lose [%s] will use this file\n",
                   video.c_str());

          break;
        }
      }
    }
  }
  battleEndLoseMusicFilename =
      config.getString("BattleEndLoseMusicFilename", "");
  if (battleEndLoseMusicFilename == "") {
    string battleEndLosePath = getGameCustomCoreDataPath(data_path, "") +
                               CORE_MENU_MUSIC_PATH + "battle_end_lose.*";
    vector<string> battleEndLoseMusic;
    findAll(battleEndLosePath, battleEndLoseMusic, false, false);
    for (int i = 0; i < (int)battleEndLoseMusic.size(); ++i) {
      string music = getGameCustomCoreDataPath(data_path, "") +
                     CORE_MENU_MUSIC_PATH + battleEndLoseMusic[i];
      if (SystemFlags::VERBOSE_MODE_ENABLED)
        printf("Checking if battle end lose music [%s] exists\n",
               music.c_str());

      if (fileExists(music)) {
        battleEndLoseMusicFilename = music;
        if (SystemFlags::VERBOSE_MODE_ENABLED)
          printf("FOUND battle end lose music [%s] will use this file\n",
                 music.c_str());

        break;
      }
    }
    if (battleEndLoseMusicFilename == "") {
      battleEndLosePath =
          data_path + CORE_MENU_MUSIC_PATH + "battle_end_lose.*";
      battleEndLoseMusic.clear();
      findAll(battleEndLosePath, battleEndLoseMusic, false, false);
      for (int i = 0; i < (int)battleEndLoseMusic.size(); ++i) {
        string music = data_path + CORE_MENU_MUSIC_PATH + battleEndLoseMusic[i];
        if (SystemFlags::VERBOSE_MODE_ENABLED)
          printf("Checking if battle end lose music [%s] exists\n",
                 music.c_str());

        if (fileExists(music)) {
          battleEndLoseMusicFilename = music;
          if (SystemFlags::VERBOSE_MODE_ENABLED)
            printf("FOUND battle end music lose [%s] will use this file\n",
                   music.c_str());

          break;
        }
      }
    }
  }
}

void CoreData::load() {
  string data_path = CoreData::getDataPath();

  Logger::getInstance().add(
      Lang::getInstance().getString("LogScreenCoreDataLoading", "", true));

  // textures
  loadTextures(data_path);

  // fonts
  loadFonts();

  // sounds
  loadSounds(data_path);

  // music
  loadMusic(data_path);

  if (GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false &&
      Shared::Graphics::VideoPlayer::hasBackEndVideoPlayer() == true) {

    loadIntroMedia(data_path);

    loadMainMenuMedia(data_path);

    loadBattleEndMedia(data_path);
  }
}

bool CoreData::hasIntroVideoFilename() const {
  bool result = (introVideoFilename != "");
  return result;
}

bool CoreData::hasMainMenuVideoFilename() const {
  bool result = (mainMenuVideoFilename != "");
  return result;
}

// bool CoreData::hasBattleEndVideoFilename(bool won) const {
//	bool result = false;
//	if(won == true) {
//		result =(battleEndWinVideoFilename != "");
//	}
//	else {
//		result =(battleEndLoseVideoFilename != "");
//	}
//	return result;
// }

void CoreData::registerFontChangedCallback(std::string entityName,
                                           FontChangedCallbackInterface *cb) {
  if (entityName == "") {
    printf("Register Font Callback detected a blank entityName!\n");
    throw megaglest_runtime_error(
        "Register Font Callback detected a blank entityName!");
  }
  if (entityName != "") {
    registeredFontChangedCallbacks[entityName].push_back(cb);
  }
}
void CoreData::unRegisterFontChangedCallback(std::string entityName) {
  if (entityName == "") {
    printf("UnRegister Font Callback detected a blank entityName!\n");
    throw megaglest_runtime_error(
        "UnRegister Font Callback detected a blank entityName!");
  }
  if (entityName != "") {
    registeredFontChangedCallbacks.erase(entityName);
  }
}
void CoreData::triggerFontChangedCallbacks(std::string fontUniqueId,
                                           Font *font) {
  for (std::map<std::string,
                std::vector<FontChangedCallbackInterface *>>::const_iterator
           iterMap = registeredFontChangedCallbacks.begin();
       iterMap != registeredFontChangedCallbacks.end(); ++iterMap) {
    for (unsigned int index = 0; index < iterMap->second.size(); ++index) {
      // printf("Font Callback detected calling: Control [%s] for Font: [%s]
      // value [%p]\n",iterMap->first.c_str(),fontUniqueId.c_str(),font);
      FontChangedCallbackInterface *cb = iterMap->second[index];
      cb->FontChangedCallback(fontUniqueId, font);
    }
  }
}
void CoreData::loadFonts() {
  Lang &lang = Lang::getInstance();

  // display font
  Config &config = Config::getInstance();

  string displayFontNamePrefix = config.getString("FontDisplayPrefix");
  string displayFontNamePostfix = config.getString("FontDisplayPostfix");
  int displayFontSize = computeFontSize(config.getInt("FontDisplayBaseSize"));

  if (lang.hasString("FontDisplayPrefix") == true) {
    displayFontNamePrefix = lang.getString("FontDisplayPrefix");
  }
  if (lang.hasString("FontDisplayPostfix") == true) {
    displayFontNamePostfix = lang.getString("FontDisplayPostfix");
  }
  if (lang.hasString("FontDisplayBaseSize") == true) {
    displayFontSize =
        computeFontSize(strToInt(lang.getString("FontDisplayBaseSize")));
  }
  string displayFontName = displayFontNamePrefix + intToStr(displayFontSize) +
                           displayFontNamePostfix;

  displayFont =
      loadFont<Font2D>(displayFont, displayFontName, displayFontSize,
                       "FontDisplay", "FontDisplayFamily", "displayFont");
  displayFont3D =
      loadFont<Font3D>(displayFont3D, displayFontName, displayFontSize,
                       "FontDisplay", "FontDisplayFamily", "displayFont3D");

  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(
        SystemFlags::debugSystem,
        "In [%s::%s Line: %d] displayFontName = [%s] displayFontSize = %d\n",
        __FILE__, __FUNCTION__, __LINE__, displayFontName.c_str(),
        displayFontSize);

  // menu fonts
  string displayFontNameSmallPrefix = config.getString("FontDisplayPrefix");
  string displayFontNameSmallPostfix = config.getString("FontDisplayPostfix");
  int displayFontNameSmallSize =
      computeFontSize(config.getInt("FontDisplaySmallBaseSize"));

  if (lang.hasString("FontDisplayPrefix") == true) {
    displayFontNameSmallPrefix = lang.getString("FontDisplayPrefix");
  }
  if (lang.hasString("FontDisplayPostfix") == true) {
    displayFontNameSmallPostfix = lang.getString("FontDisplayPostfix");
  }
  if (lang.hasString("FontDisplaySmallBaseSize") == true) {
    displayFontNameSmallSize =
        computeFontSize(strToInt(lang.getString("FontDisplaySmallBaseSize")));
  }
  string displayFontNameSmall = displayFontNameSmallPrefix +
                                intToStr(displayFontNameSmallSize) +
                                displayFontNameSmallPostfix;

  displayFontSmall = loadFont<Font2D>(
      displayFontSmall, displayFontNameSmall, displayFontNameSmallSize,
      "FontSmallDisplay", "FontSmallDisplayFamily", "displayFontSmall");
  displayFontSmall3D = loadFont<Font3D>(
      displayFontSmall3D, displayFontNameSmall, displayFontNameSmallSize,
      "FontSmallDisplay", "FontSmallDisplayFamily", "displayFontSmall3D");

  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem,
                             "In [%s::%s Line: %d] displayFontSmallName = [%s] "
                             "displayFontSmallNameSize = %d\n",
                             __FILE__, __FUNCTION__, __LINE__,
                             displayFontNameSmall.c_str(),
                             displayFontNameSmallSize);

  string menuFontNameNormalPrefix = config.getString("FontMenuNormalPrefix");
  string menuFontNameNormalPostfix = config.getString("FontMenuNormalPostfix");
  int menuFontNameNormalSize =
      computeFontSize(config.getInt("FontMenuNormalBaseSize"));
  if (lang.hasString("FontMenuNormalPrefix") == true) {
    menuFontNameNormalPrefix = lang.getString("FontMenuNormalPrefix");
  }
  if (lang.hasString("FontMenuNormalPostfix") == true) {
    menuFontNameNormalPostfix = lang.getString("FontMenuNormalPostfix");
  }
  if (lang.hasString("FontMenuNormalBaseSize") == true) {
    menuFontNameNormalSize =
        computeFontSize(strToInt(lang.getString("FontMenuNormalBaseSize")));
  }
  string menuFontNameNormal = menuFontNameNormalPrefix +
                              intToStr(menuFontNameNormalSize) +
                              menuFontNameNormalPostfix;

  menuFontNormal = loadFont<Font2D>(menuFontNormal, menuFontNameNormal,
                                    menuFontNameNormalSize, "FontMenuNormal",
                                    "FontMenuNormalFamily", "menuFontNormal");
  menuFontNormal3D = loadFont<Font3D>(
      menuFontNormal3D, menuFontNameNormal, menuFontNameNormalSize,
      "FontMenuNormal", "FontMenuNormalFamily", "menuFontNormal3D");

  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem,
                             "In [%s::%s Line: %d] menuFontNormalName = [%s] "
                             "menuFontNormalNameSize = %d\n",
                             __FILE__, __FUNCTION__, __LINE__,
                             menuFontNameNormal.c_str(),
                             menuFontNameNormalSize);

  string menuFontNameBigPrefix = config.getString("FontMenuBigPrefix");
  string menuFontNameBigPostfix = config.getString("FontMenuBigPostfix");
  int menuFontNameBigSize =
      computeFontSize(config.getInt("FontMenuBigBaseSize"));

  if (lang.hasString("FontMenuBigPrefix") == true) {
    menuFontNameBigPrefix = lang.getString("FontMenuBigPrefix");
  }
  if (lang.hasString("FontMenuBigPostfix") == true) {
    menuFontNameBigPostfix = lang.getString("FontMenuBigPostfix");
  }
  if (lang.hasString("FontMenuBigBaseSize") == true) {
    menuFontNameBigSize =
        computeFontSize(strToInt(lang.getString("FontMenuBigBaseSize")));
  }
  string menuFontNameBig = menuFontNameBigPrefix +
                           intToStr(menuFontNameBigSize) +
                           menuFontNameBigPostfix;

  menuFontBig =
      loadFont<Font2D>(menuFontBig, menuFontNameBig, menuFontNameBigSize,
                       "FontMenuBig", "FontMenuBigFamily", "menuFontBig");
  menuFontBig3D =
      loadFont<Font3D>(menuFontBig3D, menuFontNameBig, menuFontNameBigSize,
                       "FontMenuBig", "FontMenuBigFamily", "menuFontBig3D");

  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem,
                             "In [%s::%s Line: %d] menuFontNameBig = [%s] "
                             "menuFontNameBigSize = %d\n",
                             __FILE__, __FUNCTION__, __LINE__,
                             menuFontNameBig.c_str(), menuFontNameBigSize);

  string menuFontNameVeryBigPrefix = config.getString("FontMenuBigPrefix");
  string menuFontNameVeryBigPostfix = config.getString("FontMenuBigPostfix");
  int menuFontNameVeryBigSize =
      computeFontSize(config.getInt("FontMenuVeryBigBaseSize"));

  if (lang.hasString("FontMenuBigPrefix") == true) {
    menuFontNameVeryBigPrefix = lang.getString("FontMenuBigPrefix");
  }
  if (lang.hasString("FontMenuBigPostfix") == true) {
    menuFontNameVeryBigPostfix = lang.getString("FontMenuBigPostfix");
  }
  if (lang.hasString("FontMenuVeryBigBaseSize") == true) {
    menuFontNameVeryBigSize =
        computeFontSize(strToInt(lang.getString("FontMenuVeryBigBaseSize")));
  }
  string menuFontNameVeryBig = menuFontNameVeryBigPrefix +
                               intToStr(menuFontNameVeryBigSize) +
                               menuFontNameVeryBigPostfix;

  menuFontVeryBig = loadFont<Font2D>(
      menuFontVeryBig, menuFontNameVeryBig, menuFontNameVeryBigSize,
      "FontMenuVeryBig", "FontMenuVeryBigFamily", "menuFontVeryBig");
  menuFontVeryBig3D = loadFont<Font3D>(
      menuFontVeryBig3D, menuFontNameVeryBig, menuFontNameVeryBigSize,
      "FontMenuVeryBig", "FontMenuVeryBigFamily", "menuFontVeryBig3D");

  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem,
                             "In [%s::%s Line: %d] menuFontNameVeryBig = [%s] "
                             "menuFontNameVeryBigSize = %d\n",
                             __FILE__, __FUNCTION__, __LINE__,
                             menuFontNameVeryBig.c_str(),
                             menuFontNameVeryBigSize);

  // console font
  string consoleFontNamePrefix = config.getString("FontConsolePrefix");
  string consoleFontNamePostfix = config.getString("FontConsolePostfix");
  int consoleFontNameSize =
      computeFontSize(config.getInt("FontConsoleBaseSize"));

  if (lang.hasString("FontConsolePrefix") == true) {
    consoleFontNamePrefix = lang.getString("FontConsolePrefix");
  }
  if (lang.hasString("FontConsolePostfix") == true) {
    consoleFontNamePostfix = lang.getString("FontConsolePostfix");
  }
  if (lang.hasString("FontConsoleBaseSize") == true) {
    consoleFontNameSize =
        computeFontSize(strToInt(lang.getString("FontConsoleBaseSize")));
  }
  string consoleFontName = consoleFontNamePrefix +
                           intToStr(consoleFontNameSize) +
                           consoleFontNamePostfix;

  consoleFont =
      loadFont<Font2D>(consoleFont, consoleFontName, consoleFontNameSize,
                       "FontConsole", "FontConsoleFamily", "consoleFont");
  consoleFont3D =
      loadFont<Font3D>(consoleFont3D, consoleFontName, consoleFontNameSize,
                       "FontConsole", "FontConsoleFamily", "consoleFont3D");

  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem,
                             "In [%s::%s Line: %d] consoleFontName = [%s] "
                             "consoleFontNameSize = %d\n",
                             __FILE__, __FUNCTION__, __LINE__,
                             consoleFontName.c_str(), consoleFontNameSize);
}

template <typename T>
T *CoreData::loadFont(Font *menuFont, string menuFontName, int menuFontNameSize,
                      string fontType, string fontTypeFamily,
                      string fontUniqueKey) {
  Renderer &renderer = Renderer::getInstance();
  if (menuFont) {
    string fontUniqueId = menuFont->getFontUniqueId();
    renderer.endFont(menuFont, rsGlobal);
    menuFont = NULL;
    triggerFontChangedCallbacks(fontUniqueId, menuFont);
  }
  if (Renderer::renderText3DEnabled == false) {
    menuFont = renderer.newFont(rsGlobal);
  } else {
    menuFont = renderer.newFont3D(rsGlobal);
  }
  if (menuFont) {
    Config &config = Config::getInstance();
    menuFont->setType(menuFontName, config.getString(fontType, ""),
                      config.getString(fontTypeFamily, ""));
    menuFont->setSize(menuFontNameSize);
    menuFont->setWidth(Font::wBold);
    menuFont->setFontUniqueId(fontUniqueKey);
    triggerFontChangedCallbacks(menuFont->getFontUniqueId(), menuFont);
  }
  return (T *)menuFont;
}

int CoreData::computeFontSize(int size) {
  int rs = size;
  Config &config = Config::getInstance();
  if (Font::forceLegacyFonts == true) {
    int screenH = config.getInt("ScreenHeight");
    rs = size * screenH / 1024;
  }
  // FontSizeAdjustment
  rs += config.getInt("FontSizeAdjustment");
  if (Font::forceLegacyFonts == false) {
    rs += Font::baseSize; // basesize only for new font system
  }
  if (Font::forceLegacyFonts == true) {
    if (rs < 10) {
      rs = 10;
    }
  }
  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(
        SystemFlags::debugSystem,
        "In [%s::%s Line: %d] fontsize original %d      calculated:%d   \n",
        __FILE__, __FUNCTION__, __LINE__, size, rs);
  return rs;
}

void CoreData::saveGameSettingsToFile(std::string fileName,
                                      GameSettings *gameSettings,
                                      int advancedIndex) {
  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem, "In [%s::%s] Line: %d\n",
                             __FILE__, __FUNCTION__, __LINE__);

  Config &config = Config::getInstance();
  string saveSetupDir = config.getString("UserData_Root", "");
  if (saveSetupDir != "") {
    endPathWithSlash(saveSetupDir);
  }
  fileName = saveSetupDir + fileName;
  // create path if non existant
  createDirectoryPaths(extractDirectoryPathFromFile(fileName));
  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem,
                             "In [%s::%s Line: %d] fileName = [%s]\n", __FILE__,
                             __FUNCTION__, __LINE__, fileName.c_str());

#if defined(WIN32) && !defined(__MINGW32__)
  FILE *fp = _wfopen(utf8_decode(fileName).c_str(), L"w");
  std::ofstream saveGameFile(fp);
#else
  std::ofstream saveGameFile;
  saveGameFile.open(fileName.c_str(), ios_base::out | ios_base::trunc);
#endif

  saveGameFile << "Description=" << gameSettings->getDescription() << std::endl;
  saveGameFile << "MapFilterIndex="
               << Shared::PlatformByteOrder::toCommonEndian(
                      gameSettings->getMapFilter())
               << std::endl;
  saveGameFile << "Map=" << gameSettings->getMap() << std::endl;
  saveGameFile << "Tileset=" << gameSettings->getTileset() << std::endl;
  saveGameFile << "TechTree=" << gameSettings->getTech() << std::endl;
  saveGameFile << "DefaultUnits="
               << Shared::PlatformByteOrder::toCommonEndian(
                      gameSettings->getDefaultUnits())
               << std::endl;
  saveGameFile << "DefaultResources="
               << Shared::PlatformByteOrder::toCommonEndian(
                      gameSettings->getDefaultResources())
               << std::endl;
  saveGameFile << "DefaultVictoryConditions="
               << Shared::PlatformByteOrder::toCommonEndian(
                      gameSettings->getDefaultVictoryConditions())
               << std::endl;
  saveGameFile << "FogOfWar="
               << Shared::PlatformByteOrder::toCommonEndian(
                      gameSettings->getFogOfWar())
               << std::endl;
  saveGameFile << "AdvancedIndex="
               << Shared::PlatformByteOrder::toCommonEndian(advancedIndex)
               << std::endl;
  saveGameFile << "AllowObservers="
               << Shared::PlatformByteOrder::toCommonEndian(
                      gameSettings->getAllowObservers())
               << std::endl;
  saveGameFile << "FlagTypes1="
               << Shared::PlatformByteOrder::toCommonEndian(
                      gameSettings->getFlagTypes1())
               << std::endl;
  saveGameFile << "EnableObserverModeAtEndGame="
               << Shared::PlatformByteOrder::toCommonEndian(
                      gameSettings->getEnableObserverModeAtEndGame())
               << std::endl;
  saveGameFile << "AiAcceptSwitchTeamPercentChance="
               << Shared::PlatformByteOrder::toCommonEndian(
                      gameSettings->getAiAcceptSwitchTeamPercentChance())
               << std::endl;
  saveGameFile << "FallbackCpuMultiplier="
               << Shared::PlatformByteOrder::toCommonEndian(
                      gameSettings->getFallbackCpuMultiplier())
               << std::endl;
  saveGameFile << "PathFinderType="
               << Shared::PlatformByteOrder::toCommonEndian(
                      gameSettings->getPathFinderType())
               << std::endl;
  saveGameFile << "EnableServerControlledAI="
               << Shared::PlatformByteOrder::toCommonEndian(
                      gameSettings->getEnableServerControlledAI())
               << std::endl;
  saveGameFile << "NetworkFramePeriod="
               << Shared::PlatformByteOrder::toCommonEndian(
                      gameSettings->getNetworkFramePeriod())
               << std::endl;
  saveGameFile << "NetworkPauseGameForLaggedClients="
               << Shared::PlatformByteOrder::toCommonEndian(
                      gameSettings->getNetworkPauseGameForLaggedClients())
               << std::endl;

  saveGameFile << "FactionThisFactionIndex="
               << Shared::PlatformByteOrder::toCommonEndian(
                      gameSettings->getThisFactionIndex())
               << std::endl;
  saveGameFile << "FactionCount="
               << Shared::PlatformByteOrder::toCommonEndian(
                      gameSettings->getFactionCount())
               << std::endl;

  saveGameFile << "NetworkAllowNativeLanguageTechtree="
               << Shared::PlatformByteOrder::toCommonEndian(
                      gameSettings->getNetworkAllowNativeLanguageTechtree())
               << std::endl;

  for (int i = 0; i < GameConstants::maxPlayers; ++i) {
    int slotIndex = gameSettings->getStartLocationIndex(i);

    saveGameFile << "FactionControlForIndex"
                 << Shared::PlatformByteOrder::toCommonEndian(slotIndex) << "="
                 << Shared::PlatformByteOrder::toCommonEndian(
                        gameSettings->getFactionControl(i))
                 << std::endl;
    saveGameFile << "ResourceMultiplierIndex"
                 << Shared::PlatformByteOrder::toCommonEndian(slotIndex) << "="
                 << Shared::PlatformByteOrder::toCommonEndian(
                        gameSettings->getResourceMultiplierIndex(i))
                 << std::endl;
    saveGameFile << "FactionTeamForIndex"
                 << Shared::PlatformByteOrder::toCommonEndian(slotIndex) << "="
                 << Shared::PlatformByteOrder::toCommonEndian(
                        gameSettings->getTeam(i))
                 << std::endl;
    saveGameFile << "FactionStartLocationForIndex"
                 << Shared::PlatformByteOrder::toCommonEndian(slotIndex) << "="
                 << Shared::PlatformByteOrder::toCommonEndian(
                        gameSettings->getStartLocationIndex(i))
                 << std::endl;
    saveGameFile << "FactionTypeNameForIndex"
                 << Shared::PlatformByteOrder::toCommonEndian(slotIndex) << "="
                 << gameSettings->getFactionTypeName(i) << std::endl;
    saveGameFile << "FactionPlayerNameForIndex"
                 << Shared::PlatformByteOrder::toCommonEndian(slotIndex) << "="
                 << gameSettings->getNetworkPlayerName(i) << std::endl;

    saveGameFile << "FactionPlayerUUIDForIndex"
                 << Shared::PlatformByteOrder::toCommonEndian(slotIndex) << "="
                 << gameSettings->getNetworkPlayerUUID(i) << std::endl;
  }

#if defined(WIN32) && !defined(__MINGW32__)
  if (fp)
    fclose(fp);
#endif
  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem, "In [%s::%s] Line: %d\n",
                             __FILE__, __FUNCTION__, __LINE__);
}

bool CoreData::loadGameSettingsFromFile(std::string fileName,
                                        GameSettings *gameSettings) {
  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem, "In [%s::%s] Line: %d\n",
                             __FILE__, __FUNCTION__, __LINE__);

  bool fileWasFound = false;
  Config &config = Config::getInstance();
  string saveSetupDir = config.getString("UserData_Root", "");
  if (saveSetupDir != "") {
    endPathWithSlash(saveSetupDir);
  }

  if (fileExists(saveSetupDir + fileName) == true) {
    fileName = saveSetupDir + fileName;
    fileWasFound = true;
  }

  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem,
                             "In [%s::%s Line: %d] fileName = [%s]\n", __FILE__,
                             __FUNCTION__, __LINE__, fileName.c_str());

  if (fileExists(fileName) == false) {
    return false;
  }

  fileWasFound = true;
  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem,
                             "In [%s::%s Line: %d] fileName = [%s]\n", __FILE__,
                             __FUNCTION__, __LINE__, fileName.c_str());

  Properties properties;
  properties.load(fileName);

  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem,
                             "In [%s::%s Line: %d] fileName = [%s]\n", __FILE__,
                             __FUNCTION__, __LINE__, fileName.c_str());

  gameSettings->setMapFilter(properties.getInt("MapFilterIndex", "0"));
  gameSettings->setDescription(properties.getString("Description"));
  gameSettings->setMap(properties.getString("Map"));
  gameSettings->setTileset(properties.getString("Tileset"));
  gameSettings->setTech(properties.getString("TechTree"));
  gameSettings->setDefaultUnits(properties.getBool("DefaultUnits"));
  gameSettings->setDefaultResources(properties.getBool("DefaultResources"));
  gameSettings->setDefaultVictoryConditions(
      properties.getBool("DefaultVictoryConditions"));
  gameSettings->setFogOfWar(properties.getBool("FogOfWar"));
  // listBoxAdvanced.setSelectedItemIndex(properties.getInt("AdvancedIndex","0"));

  gameSettings->setAllowObservers(
      properties.getBool("AllowObservers", "false"));
  gameSettings->setFlagTypes1(properties.getInt("FlagTypes1", "0"));

  uint32 valueFlags1 = gameSettings->getFlagTypes1();
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

  gameSettings->setEnableObserverModeAtEndGame(
      properties.getBool("EnableObserverModeAtEndGame"));
  gameSettings->setAiAcceptSwitchTeamPercentChance(
      properties.getInt("AiAcceptSwitchTeamPercentChance", "30"));
  gameSettings->setFallbackCpuMultiplier(
      properties.getInt("FallbackCpuMultiplier", "5"));

  gameSettings->setPathFinderType(static_cast<PathFinderType>(
      properties.getInt("PathFinderType", intToStr(pfBasic).c_str())));
  gameSettings->setEnableServerControlledAI(
      properties.getBool("EnableServerControlledAI", "true"));
  gameSettings->setNetworkFramePeriod(
      properties.getInt("NetworkFramePeriod",
                        intToStr(GameConstants::networkFramePeriod).c_str()));
  gameSettings->setNetworkPauseGameForLaggedClients(
      properties.getBool("NetworkPauseGameForLaggedClients", "false"));

  gameSettings->setThisFactionIndex(
      properties.getInt("FactionThisFactionIndex"));
  gameSettings->setFactionCount(properties.getInt("FactionCount"));

  if (properties.hasString("NetworkAllowNativeLanguageTechtree") == true) {
    gameSettings->setNetworkAllowNativeLanguageTechtree(
        properties.getBool("NetworkAllowNativeLanguageTechtree"));
  } else {
    gameSettings->setNetworkAllowNativeLanguageTechtree(false);
  }

  for (int i = 0; i < GameConstants::maxPlayers; ++i) {
    gameSettings->setFactionControl(
        i, (ControlType)properties.getInt(string("FactionControlForIndex") +
                                              intToStr(i),
                                          intToStr(ctClosed).c_str()));

    if (gameSettings->getFactionControl(i) == ctNetworkUnassigned) {
      gameSettings->setFactionControl(i, ctNetwork);
    }

    gameSettings->setResourceMultiplierIndex(
        i, properties.getInt(string("ResourceMultiplierIndex") + intToStr(i),
                             "5"));
    gameSettings->setTeam(
        i, properties.getInt(string("FactionTeamForIndex") + intToStr(i), "0"));
    gameSettings->setStartLocationIndex(
        i,
        properties.getInt(string("FactionStartLocationForIndex") + intToStr(i),
                          intToStr(i).c_str()));
    gameSettings->setFactionTypeName(
        i, properties.getString(string("FactionTypeNameForIndex") + intToStr(i),
                                "?"));

    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(
          SystemFlags::debugSystem,
          "In [%s::%s Line: %d] i = %d, factionTypeName [%s]\n", __FILE__,
          __FUNCTION__, __LINE__, i,
          gameSettings->getFactionTypeName(i).c_str());

    if (gameSettings->getFactionControl(i) == ctHuman) {
      gameSettings->setNetworkPlayerName(
          i, properties.getString(
                 string("FactionPlayerNameForIndex") + intToStr(i), ""));
    } else {
      gameSettings->setNetworkPlayerName(i, "");
    }

    gameSettings->setNetworkPlayerUUID(
        i, properties.getString(
               string("FactionPlayerUUIDForIndex") + intToStr(i), ""));
  }

  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem, "In [%s::%s] Line: %d\n",
                             __FILE__, __FUNCTION__, __LINE__);

  return fileWasFound;
}

// ================== PRIVATE ========================

} // namespace Game
} // namespace Glest
