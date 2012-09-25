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

#include "logger.h"
#include "renderer.h"
#include "graphics_interface.h"
#include "config.h"
#include "util.h"
#include "platform_util.h"
#include "game_constants.h"
#include "game_util.h"
#include "lang.h"
#include "game_settings.h"
#include "video_player.h"
#include "leak_dumper.h"

using namespace Shared::Sound;
using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class CoreData
// =====================================================

// ===================== PUBLIC ========================

CoreData &CoreData::getInstance() {
	static CoreData coreData;
	return coreData;
}

CoreData::CoreData() {
	logoTexture=NULL;
	logoTextureList.clear();
    backgroundTexture=NULL;
    fireTexture=NULL;
    teamColorTexture=NULL;
    snowTexture=NULL;
	waterSplashTexture=NULL;
    customTexture=NULL;
	buttonSmallTexture=NULL;
	buttonBigTexture=NULL;
	horizontalLineTexture=NULL;
	verticalLineTexture=NULL;
	checkBoxTexture=NULL;
	checkedCheckBoxTexture=NULL;
	gameWinnerTexture=NULL;
    notOnServerTexture=NULL;
    onServerDifferentTexture=NULL;
    onServerTexture=NULL;
    onServerInstalledTexture=NULL;

    miscTextureList.clear();

    displayFont=NULL;
	menuFontNormal=NULL;
	displayFontSmall=NULL;
	menuFontBig=NULL;
	menuFontVeryBig=NULL;
	consoleFont=NULL;

    displayFont3D=NULL;
	menuFontNormal3D=NULL;
	displayFontSmall3D=NULL;
	menuFontBig3D=NULL;
	menuFontVeryBig3D=NULL;
	consoleFont3D=NULL;

	introVideoFilename="";
	mainMenuVideoFilename="";

	battleEndWinVideoFilename="";
	battleEndWinVideoFilenameFallback="";
	battleEndWinMusicFilename="";
	battleEndLoseVideoFilename="";
	battleEndLoseVideoFilenameFallback="";
	battleEndLoseMusicFilename="";
}

CoreData::~CoreData() {
	cleanup();
}

void CoreData::cleanup() {
	deleteValues(waterSounds.getSoundsPtr()->begin(), waterSounds.getSoundsPtr()->end());
	waterSounds.getSoundsPtr()->clear();
}

Texture2D *CoreData::getTextureBySystemId(TextureSystemType type) const {
	Texture2D *result = NULL;
	switch(type) {
	case tsyst_logoTexture:
		result = logoTexture;
		break;
	//std::vector<Texture2D *> logoTextureList;
	case tsyst_backgroundTexture:
		result = backgroundTexture;
		break;
	case tsyst_fireTexture:
		result = fireTexture;
		break;
	case tsyst_teamColorTexture:
		result = teamColorTexture;
		break;
	case tsyst_snowTexture:
		result = snowTexture;
		break;
	case tsyst_waterSplashTexture:
		result = waterSplashTexture;
		break;
	case tsyst_customTexture:
		result = customTexture;
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

    //std::vector<Texture2D *> miscTextureList;
	}
	return result;
}

void CoreData::load() {
	string data_path = getGameReadWritePath(GameConstants::path_data_CacheLookupKey);
	if(data_path != "") {
		endPathWithSlash(data_path);
	}

	//const string dir = data_path + "data/core";
	Logger::getInstance().add(Lang::getInstance().get("LogScreenCoreDataLoading","",true));

	Renderer &renderer= Renderer::getInstance();

	//textures
	backgroundTexture= renderer.newTexture2D(rsGlobal);
	if(backgroundTexture) {
		backgroundTexture->setMipmap(false);
		backgroundTexture->getPixmap()->load(getGameCustomCoreDataPath(data_path, "data/core/menu/textures/back.tga"));
		backgroundTexture->setTextureSystemId(tsyst_backgroundTexture);
	}

	fireTexture= renderer.newTexture2D(rsGlobal);
	if(fireTexture) {
		fireTexture->setFormat(Texture::fAlpha);
		fireTexture->getPixmap()->init(1);
		fireTexture->getPixmap()->load(getGameCustomCoreDataPath(data_path, "data/core/misc_textures/fire_particle.tga"));
		fireTexture->setTextureSystemId(tsyst_fireTexture);
	}

	teamColorTexture= renderer.newTexture2D(rsGlobal);
	if(teamColorTexture) {
		teamColorTexture->setFormat(Texture::fAlpha);
		teamColorTexture->getPixmap()->init(1);
		teamColorTexture->getPixmap()->load(getGameCustomCoreDataPath(data_path, "data/core/misc_textures/team_color_texture.tga"));
		teamColorTexture->setTextureSystemId(tsyst_teamColorTexture);
	}

	snowTexture= renderer.newTexture2D(rsGlobal);
	if(snowTexture) {
		snowTexture->setMipmap(false);
		snowTexture->setFormat(Texture::fAlpha);
		snowTexture->getPixmap()->init(1);
		snowTexture->getPixmap()->load(getGameCustomCoreDataPath(data_path, "data/core/misc_textures/snow_particle.tga"));
		snowTexture->setTextureSystemId(tsyst_snowTexture);
	}

	customTexture= renderer.newTexture2D(rsGlobal);
	if(customTexture) {
		customTexture->getPixmap()->load(getGameCustomCoreDataPath(data_path, "data/core/menu/textures/custom_texture.tga"));
		customTexture->setTextureSystemId(tsyst_customTexture);
	}

	notOnServerTexture= renderer.newTexture2D(rsGlobal);
	if(notOnServerTexture) {
		notOnServerTexture->getPixmap()->load(getGameCustomCoreDataPath(data_path, "data/core/menu/textures/not_on_server.tga"));
		notOnServerTexture->setTextureSystemId(tsyst_notOnServerTexture);
	}

	onServerDifferentTexture= renderer.newTexture2D(rsGlobal);
	if(onServerDifferentTexture) {
		onServerDifferentTexture->getPixmap()->load(getGameCustomCoreDataPath(data_path, "data/core/menu/textures/on_server_different.tga"));
		onServerDifferentTexture->setTextureSystemId(tsyst_onServerDifferentTexture);

		onServerTexture= renderer.newTexture2D(rsGlobal);
		onServerTexture->getPixmap()->load(getGameCustomCoreDataPath(data_path, "data/core/menu/textures/on_server.tga"));
		onServerTexture->setTextureSystemId(tsyst_onServerTexture);

		onServerInstalledTexture= renderer.newTexture2D(rsGlobal);
		onServerInstalledTexture->getPixmap()->load(getGameCustomCoreDataPath(data_path, "data/core/menu/textures/on_server_installed.tga"));
		onServerInstalledTexture->setTextureSystemId(tsyst_onServerInstalledTexture);
	}

	logoTexture= renderer.newTexture2D(rsGlobal);
	if(logoTexture) {
		logoTexture->setMipmap(false);
		logoTexture->getPixmap()->load(getGameCustomCoreDataPath(data_path, "data/core/menu/textures/logo.tga"));
		logoTexture->setTextureSystemId(tsyst_logoTexture);
	}

	logoTextureList.clear();


	string logosPath = getGameCustomCoreDataPath(data_path, "") + "data/core/menu/textures/logo*.*";
	vector<string> logoFilenames;
    findAll(logosPath, logoFilenames, false, false);
    for(int i = 0; i < logoFilenames.size(); ++i) {
    	string logo = logoFilenames[i];
    	if(strcmp("logo.tga",logo.c_str()) != 0) {
    		Texture2D *logoTextureExtra= renderer.newTexture2D(rsGlobal);
    		if(logoTextureExtra) {
				logoTextureExtra->setMipmap(true);
				logoTextureExtra->getPixmap()->load(getGameCustomCoreDataPath(data_path, "") + "data/core/menu/textures/" + logo);
				logoTextureList.push_back(logoTextureExtra);
    		}
    	}
    }

	if(logoTextureList.empty() == true) {
		logosPath= data_path + "data/core/menu/textures/logo*.*";
		vector<string> logoFilenames;
		findAll(logosPath, logoFilenames, false, false);
		for(int i = 0; i < logoFilenames.size(); ++i) {
			string logo = logoFilenames[i];
			if(strcmp("logo.tga",logo.c_str()) != 0) {
				Texture2D *logoTextureExtra= renderer.newTexture2D(rsGlobal);
				if(logoTextureExtra) {
					logoTextureExtra->setMipmap(true);
					logoTextureExtra->getPixmap()->load(data_path + "data/core/menu/textures/" + logo);
					logoTextureList.push_back(logoTextureExtra);
				}
			}
		}
	}

    miscTextureList.clear();

	string introPath = getGameCustomCoreDataPath(data_path, "") + "data/core/menu/textures/intro*.*";
	vector<string> introFilenames;
    findAll(introPath, introFilenames, false, false);
    for(int i = 0; i < introFilenames.size(); ++i) {
    	string logo = introFilenames[i];
    	//if(strcmp("logo.tga",logo.c_str()) != 0) {
    		Texture2D *logoTextureExtra= renderer.newTexture2D(rsGlobal);
    		if(logoTextureExtra) {
				logoTextureExtra->setMipmap(true);
				logoTextureExtra->getPixmap()->load(getGameCustomCoreDataPath(data_path, "") + "data/core/menu/textures/" + logo);
				miscTextureList.push_back(logoTextureExtra);
    		}
    	//}
    }
    if(miscTextureList.empty() == true) {
		introPath= data_path + "data/core/menu/textures/intro*.*";
		vector<string> introFilenames;
		findAll(introPath, introFilenames, false, false);
		for(int i = 0; i < introFilenames.size(); ++i) {
			string logo = introFilenames[i];
			//if(strcmp("logo.tga",logo.c_str()) != 0) {
				Texture2D *logoTextureExtra= renderer.newTexture2D(rsGlobal);
				if(logoTextureExtra) {
					logoTextureExtra->setMipmap(true);
					logoTextureExtra->getPixmap()->load(data_path + "data/core/menu/textures/" + logo);
					miscTextureList.push_back(logoTextureExtra);
				}
			//}
		}
    }

	waterSplashTexture= renderer.newTexture2D(rsGlobal);
	if(waterSplashTexture) {
		waterSplashTexture->setFormat(Texture::fAlpha);
		waterSplashTexture->getPixmap()->init(1);
		waterSplashTexture->getPixmap()->load(getGameCustomCoreDataPath(data_path, "data/core/misc_textures/water_splash.tga"));
		waterSplashTexture->setTextureSystemId(tsyst_waterSplashTexture);
	}

	buttonSmallTexture= renderer.newTexture2D(rsGlobal);
	if(buttonSmallTexture) {
		buttonSmallTexture->setForceCompressionDisabled(true);
		buttonSmallTexture->getPixmap()->load(getGameCustomCoreDataPath(data_path, "data/core/menu/textures/button_small.tga"));
		buttonSmallTexture->setTextureSystemId(tsyst_buttonSmallTexture);
	}

	buttonBigTexture= renderer.newTexture2D(rsGlobal);
	if(buttonBigTexture) {
		buttonBigTexture->setForceCompressionDisabled(true);
		buttonBigTexture->getPixmap()->load(getGameCustomCoreDataPath(data_path, "data/core/menu/textures/button_big.tga"));
		buttonBigTexture->setTextureSystemId(tsyst_buttonBigTexture);
	}

	horizontalLineTexture= renderer.newTexture2D(rsGlobal);
	if(horizontalLineTexture) {
		horizontalLineTexture->setForceCompressionDisabled(true);
		horizontalLineTexture->getPixmap()->load(getGameCustomCoreDataPath(data_path, "data/core/menu/textures/line_horizontal.tga"));
		horizontalLineTexture->setTextureSystemId(tsyst_horizontalLineTexture);
	}

	verticalLineTexture= renderer.newTexture2D(rsGlobal);
	if(verticalLineTexture) {
		verticalLineTexture->setForceCompressionDisabled(true);
		verticalLineTexture->getPixmap()->load(getGameCustomCoreDataPath(data_path, "data/core/menu/textures/line_vertical.tga"));
		verticalLineTexture->setTextureSystemId(tsyst_verticalLineTexture);
	}

	checkBoxTexture= renderer.newTexture2D(rsGlobal);
	if(checkBoxTexture) {
		checkBoxTexture->setForceCompressionDisabled(true);
		checkBoxTexture->getPixmap()->load(getGameCustomCoreDataPath(data_path, "data/core/menu/textures/checkbox.tga"));
		checkBoxTexture->setTextureSystemId(tsyst_checkBoxTexture);
	}

	checkedCheckBoxTexture= renderer.newTexture2D(rsGlobal);
	if(checkedCheckBoxTexture) {
		checkedCheckBoxTexture->setForceCompressionDisabled(true);
		checkedCheckBoxTexture->getPixmap()->load(getGameCustomCoreDataPath(data_path, "data/core/menu/textures/checkbox_checked.tga"));
		checkedCheckBoxTexture->setTextureSystemId(tsyst_checkedCheckBoxTexture);
	}

	gameWinnerTexture= renderer.newTexture2D(rsGlobal);
	if(gameWinnerTexture) {
		gameWinnerTexture->setForceCompressionDisabled(true);
		gameWinnerTexture->getPixmap()->load(getGameCustomCoreDataPath(data_path, "data/core/misc_textures/game_winner.png"));
		gameWinnerTexture->setTextureSystemId(tsyst_gameWinnerTexture);
	}

	loadFonts();

	//sounds
	clickSoundA.load(getGameCustomCoreDataPath(data_path, "data/core/menu/sound/click_a.wav"));
	clickSoundB.load(getGameCustomCoreDataPath(data_path, "data/core/menu/sound/click_b.wav"));
	clickSoundC.load(getGameCustomCoreDataPath(data_path, "data/core/menu/sound/click_c.wav"));
	attentionSound.load(getGameCustomCoreDataPath(data_path, "data/core/menu/sound/attention.wav"));
	highlightSound.load(getGameCustomCoreDataPath(data_path, "data/core/menu/sound/highlight.wav"));
	markerSound.load(getGameCustomCoreDataPath(data_path, "data/core/menu/sound/sonar.wav"));

	XmlTree xmlTree;
	//string data_path = getGameReadWritePath(GameConstants::path_data_CacheLookupKey);
	xmlTree.load(getGameCustomCoreDataPath(data_path, "data/core/menu/menu.xml"),Properties::getTagReplacementValues());
	const XmlNode *menuNode= xmlTree.getRootNode();

	string menuMusicPath 		= "/menu/music/";
	string menuIntroMusicFile 	= "intro_music.ogg";
	string menuMusicFile 		= "menu_music.ogg";

	if(menuNode->hasChild("intro") == true) {
		const XmlNode *introNode= menuNode->getChild("intro");

		// intro info
		const XmlNode *menuPathNode= introNode->getChild("menu-music-path");
		menuMusicPath = menuPathNode->getAttribute("value")->getRestrictedValue();
		const XmlNode *menuIntroMusicNode= introNode->getChild("menu-intro-music");
		menuIntroMusicFile = menuIntroMusicNode->getAttribute("value")->getRestrictedValue();
		const XmlNode *menuMusicNode= introNode->getChild("menu-music");
		menuMusicFile = menuMusicNode->getAttribute("value")->getRestrictedValue();
	}
	introMusic.open(getGameCustomCoreDataPath(data_path, "data/core/" + menuMusicPath + menuIntroMusicFile));
	introMusic.setNext(&menuMusic);
	menuMusic.open(getGameCustomCoreDataPath(data_path, "data/core/" + menuMusicPath + menuMusicFile));
	menuMusic.setNext(&menuMusic);

	cleanup();
	waterSounds.resize(6);

	for(int i=0; i<6; ++i){
		waterSounds[i]= new StaticSound();
		if(waterSounds[i]) {
			waterSounds[i]->load(getGameCustomCoreDataPath(data_path, "data/core/water_sounds/water"+intToStr(i)+".wav"));
		}
	}


	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false &&
			Shared::Graphics::VideoPlayer::hasBackEndVideoPlayer() == true) {
		string data_path = getGameReadWritePath(GameConstants::path_data_CacheLookupKey);
		Config &config= Config::getInstance();

		introVideoFilename = config.getString("IntroVideoURL","");
		introVideoFilenameFallback = config.getString("IntroVideoURLFallback","");
		if(introVideoFilename == "") {
			string introVideoPath = getGameCustomCoreDataPath(data_path, "") + "data/core/menu/videos/intro.*";
			vector<string> introVideos;
			findAll(introVideoPath, introVideos, false, false);
			for(int i = 0; i < introVideos.size(); ++i) {
				string video = getGameCustomCoreDataPath(data_path, "") + "data/core/menu/videos/" + introVideos[i];
				if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Checking if intro video [%s] exists\n",video.c_str());

				if(fileExists(video)) {
					introVideoFilename = video;
					if(SystemFlags::VERBOSE_MODE_ENABLED) printf("FOUND intro video [%s] will use this file\n",video.c_str());
					break;
				}
			}

			if(introVideoFilename == "") {
				introVideoPath = data_path + "data/core/menu/videos/intro.*";
				introVideos.clear();
				findAll(introVideoPath, introVideos, false, false);
				for(int i = 0; i < introVideos.size(); ++i) {
					string video = data_path + "data/core/menu/videos/" + introVideos[i];
					if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Checking if intro video [%s] exists\n",video.c_str());

					if(fileExists(video)) {
						introVideoFilename = video;
						if(SystemFlags::VERBOSE_MODE_ENABLED) printf("FOUND intro video [%s] will use this file\n",video.c_str());
						break;
					}
				}
			}
		}

		mainMenuVideoFilename = config.getString("MainMenuVideoURL","");
		mainMenuVideoFilenameFallback = config.getString("MainMenuVideoURLFallback","");
		if(mainMenuVideoFilename == "") {
			string mainVideoPath = getGameCustomCoreDataPath(data_path, "") + "data/core/menu/videos/main.*";
			vector<string> mainVideos;
			findAll(mainVideoPath, mainVideos, false, false);
			for(int i = 0; i < mainVideos.size(); ++i) {
				string video = getGameCustomCoreDataPath(data_path, "") + "data/core/menu/videos/" + mainVideos[i];
				if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Checking if mainmenu video [%s] exists\n",video.c_str());

				if(fileExists(video)) {
					mainMenuVideoFilename = video;
					if(SystemFlags::VERBOSE_MODE_ENABLED) printf("FOUND mainmenu video [%s] will use this file\n",video.c_str());
					break;
				}
			}

			if(mainMenuVideoFilename == "") {
				mainVideoPath = data_path + "data/core/menu/videos/main.*";
				mainVideos.clear();
				findAll(mainVideoPath, mainVideos, false, false);
				for(int i = 0; i < mainVideos.size(); ++i) {
					string video = data_path + "data/core/menu/videos/" + mainVideos[i];
					if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Checking if mainmenu video [%s] exists\n",video.c_str());

					if(fileExists(video)) {
						mainMenuVideoFilename = video;
						if(SystemFlags::VERBOSE_MODE_ENABLED) printf("FOUND mainmenu video [%s] will use this file\n",video.c_str());
						break;
					}
				}
			}
		}

		battleEndWinVideoFilename = config.getString("BattleEndWinVideoURL","");
		battleEndWinVideoFilenameFallback = config.getString("BattleEndWinVideoURLFallback","");

		if(battleEndWinVideoFilename == "") {
			string battleEndWinVideoPath = getGameCustomCoreDataPath(data_path, "") + "data/core/menu/videos/battle_end_win.*";
			vector<string> battleEndWinVideos;
			findAll(battleEndWinVideoPath, battleEndWinVideos, false, false);
			for(int i = 0; i < battleEndWinVideos.size(); ++i) {
				string video = getGameCustomCoreDataPath(data_path, "") + "data/core/menu/videos/" + battleEndWinVideos[i];
				if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Checking if battle end win video [%s] exists\n",video.c_str());

				if(fileExists(video)) {
					battleEndWinVideoFilename = video;
					if(SystemFlags::VERBOSE_MODE_ENABLED) printf("FOUND battle end win video [%s] will use this file\n",video.c_str());
					break;
				}
			}

			if(battleEndWinVideoFilename == "") {
				battleEndWinVideoPath = data_path + "data/core/menu/videos/battle_end_win.*";
				battleEndWinVideos.clear();
				findAll(battleEndWinVideoPath, battleEndWinVideos, false, false);
				for(int i = 0; i < battleEndWinVideos.size(); ++i) {
					string video = data_path + "data/core/menu/videos/" + battleEndWinVideos[i];
					if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Checking if battle end win video [%s] exists\n",video.c_str());

					if(fileExists(video)) {
						battleEndWinVideoFilename = video;
						if(SystemFlags::VERBOSE_MODE_ENABLED) printf("FOUND battle end video win [%s] will use this file\n",video.c_str());
						break;
					}
				}
			}
		}

		battleEndWinMusicFilename = config.getString("BattleEndWinMusicFilename","");
		if(battleEndWinMusicFilename == "") {
			string battleEndWinPath = getGameCustomCoreDataPath(data_path, "") + "data/core/menu/music/battle_end_win.*";
			vector<string> battleEndWinMusic;
			findAll(battleEndWinPath, battleEndWinMusic, false, false);
			for(int i = 0; i < battleEndWinMusic.size(); ++i) {
				string music = getGameCustomCoreDataPath(data_path, "") + "data/core/menu/music/" + battleEndWinMusic[i];
				if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Checking if battle end win music [%s] exists\n",music.c_str());

				if(fileExists(music)) {
					battleEndWinMusicFilename = music;
					if(SystemFlags::VERBOSE_MODE_ENABLED) printf("FOUND battle end win music [%s] will use this file\n",music.c_str());
					break;
				}
			}

			if(battleEndWinMusicFilename == "") {
				battleEndWinPath = data_path + "data/core/menu/music/battle_end_win.*";
				battleEndWinMusic.clear();
				findAll(battleEndWinPath, battleEndWinMusic, false, false);
				for(int i = 0; i < battleEndWinMusic.size(); ++i) {
					string music = data_path + "data/core/menu/music/" + battleEndWinMusic[i];
					if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Checking if battle end win music [%s] exists\n",music.c_str());

					if(fileExists(music)) {
						battleEndWinMusicFilename = music;
						if(SystemFlags::VERBOSE_MODE_ENABLED) printf("FOUND battle end music win [%s] will use this file\n",music.c_str());
						break;
					}
				}
			}
		}

		battleEndLoseVideoFilename = config.getString("BattleEndLoseVideoURL","");
		battleEndLoseVideoFilenameFallback = config.getString("BattleEndLoseVideoURLFallback","");

		if(battleEndLoseVideoFilename == "") {
			string battleEndLoseVideoPath = getGameCustomCoreDataPath(data_path, "") + "data/core/menu/videos/battle_end_lose.*";
			vector<string> battleEndLoseVideos;
			findAll(battleEndLoseVideoPath, battleEndLoseVideos, false, false);
			for(int i = 0; i < battleEndLoseVideos.size(); ++i) {
				string video = getGameCustomCoreDataPath(data_path, "") + "data/core/menu/videos/" + battleEndLoseVideos[i];
				if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Checking if battle end lose video [%s] exists\n",video.c_str());

				if(fileExists(video)) {
					battleEndLoseVideoFilename = video;
					if(SystemFlags::VERBOSE_MODE_ENABLED) printf("FOUND battle end lose video [%s] will use this file\n",video.c_str());
					break;
				}
			}

			if(battleEndLoseVideoFilename == "") {
				battleEndLoseVideoPath = data_path + "data/core/menu/videos/battle_end_lose.*";
				battleEndLoseVideos.clear();
				findAll(battleEndLoseVideoPath, battleEndLoseVideos, false, false);
				for(int i = 0; i < battleEndLoseVideos.size(); ++i) {
					string video = data_path + "data/core/menu/videos/" + battleEndLoseVideos[i];
					if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Checking if battle end lose video [%s] exists\n",video.c_str());

					if(fileExists(video)) {
						battleEndLoseVideoFilename = video;
						if(SystemFlags::VERBOSE_MODE_ENABLED) printf("FOUND battle end video lose [%s] will use this file\n",video.c_str());
						break;
					}
				}
			}
		}

		battleEndLoseMusicFilename = config.getString("BattleEndLoseMusicFilename","");
		if(battleEndLoseMusicFilename == "") {
			string battleEndLosePath = getGameCustomCoreDataPath(data_path, "") + "data/core/menu/music/battle_end_lose.*";
			vector<string> battleEndLoseMusic;
			findAll(battleEndLosePath, battleEndLoseMusic, false, false);
			for(int i = 0; i < battleEndLoseMusic.size(); ++i) {
				string music = getGameCustomCoreDataPath(data_path, "") + "data/core/menu/music/" + battleEndLoseMusic[i];
				if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Checking if battle end lose music [%s] exists\n",music.c_str());

				if(fileExists(music)) {
					battleEndLoseMusicFilename = music;
					if(SystemFlags::VERBOSE_MODE_ENABLED) printf("FOUND battle end lose music [%s] will use this file\n",music.c_str());
					break;
				}
			}

			if(battleEndLoseMusicFilename == "") {
				battleEndLosePath = data_path + "data/core/menu/music/battle_end_lose.*";
				battleEndLoseMusic.clear();
				findAll(battleEndLosePath, battleEndLoseMusic, false, false);
				for(int i = 0; i < battleEndLoseMusic.size(); ++i) {
					string music = data_path + "data/core/menu/music/" + battleEndLoseMusic[i];
					if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Checking if battle end lose music [%s] exists\n",music.c_str());

					if(fileExists(music)) {
						battleEndLoseMusicFilename = music;
						if(SystemFlags::VERBOSE_MODE_ENABLED) printf("FOUND battle end music lose [%s] will use this file\n",music.c_str());
						break;
					}
				}
			}
		}
	}
}

bool CoreData::hasIntroVideoFilename() const {
	//bool result = (introVideoFilename != "" && fileExists(introVideoFilename) == true);
	bool result = (introVideoFilename != "");
	return result;
}

bool CoreData::hasMainMenuVideoFilename() const {
	//bool result = (mainMenuVideoFilename != "" && fileExists(mainMenuVideoFilename) == true);
	bool result = (mainMenuVideoFilename != "");
	return result;
}

bool CoreData::hasBattleEndVideoFilename(bool won) const {
	//bool result = (mainMenuVideoFilename != "" && fileExists(mainMenuVideoFilename) == true);
	bool result = false;
	if(won == true) {
		result =(battleEndWinVideoFilename != "");
	}
	else {
		result =(battleEndLoseVideoFilename != "");
	}
	return result;
}

void CoreData::loadFonts() {
	Renderer &renderer= Renderer::getInstance();
	Lang &lang= Lang::getInstance();

	//display font
	Config &config= Config::getInstance();

	string displayFontNamePrefix	= config.getString("FontDisplayPrefix");
	string displayFontNamePostfix	= config.getString("FontDisplayPostfix");
	int displayFontSize				= computeFontSize(config.getInt("FontDisplayBaseSize"));

	//printf("Checking if langfile has custom FontDisplayPostfix\n");

	if(lang.hasString("FontDisplayPrefix") == true) {
		displayFontNamePrefix = lang.get("FontDisplayPrefix");
	}
	if(lang.hasString("FontDisplayPostfix") == true) {
		displayFontNamePostfix = lang.get("FontDisplayPostfix");
	}
	if(lang.hasString("FontDisplayBaseSize") == true) {
		displayFontSize = strToInt(lang.get("FontDisplayBaseSize"));
	}

	//printf("displayFontNamePostfix [%s]\n",displayFontNamePostfix.c_str());

	string displayFontName = displayFontNamePrefix + intToStr(displayFontSize) + displayFontNamePostfix;

	if(displayFont) {
		renderer.endFont(displayFont, rsGlobal);
		displayFont=NULL;
	}
	if(Renderer::renderText3DEnabled == false) {
		displayFont= renderer.newFont(rsGlobal);
		if(displayFont) {
			displayFont->setType(displayFontName,config.getString("FontDisplay",""),config.getString("FontDisplayFamily",""));
			displayFont->setSize(displayFontSize);
			//displayFont->setYOffsetFactor(config.getFloat("FontDisplayYOffsetFactor",floatToStr(FontMetrics::DEFAULT_Y_OFFSET_FACTOR).c_str()));
		}
	}

	if(displayFont3D) {
		renderer.endFont(displayFont3D, rsGlobal);
		displayFont3D=NULL;
	}
	if(Renderer::renderText3DEnabled == true) {
		displayFont3D= renderer.newFont3D(rsGlobal);
		if(displayFont3D) {
			displayFont3D->setType(displayFontName,config.getString("FontDisplay",""),config.getString("FontDisplayFamily",""));
			displayFont3D->setSize(displayFontSize);
			//displayFont3D->setYOffsetFactor(config.getFloat("FontDisplayYOffsetFactor",floatToStr(FontMetrics::DEFAULT_Y_OFFSET_FACTOR).c_str()));
		}
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] displayFontName = [%s] displayFontSize = %d\n",__FILE__,__FUNCTION__,__LINE__,displayFontName.c_str(),displayFontSize);

	//menu fonts
	string displayFontNameSmallPrefix	= config.getString("FontDisplayPrefix");
	string displayFontNameSmallPostfix	= config.getString("FontDisplayPostfix");
	int displayFontNameSmallSize		= computeFontSize(config.getInt("FontDisplaySmallBaseSize"));

	if(lang.hasString("FontDisplayPrefix") == true) {
		displayFontNameSmallPrefix = lang.get("FontDisplayPrefix");
	}
	if(lang.hasString("FontDisplayPostfix") == true) {
		displayFontNameSmallPostfix = lang.get("FontDisplayPostfix");
	}
	if(lang.hasString("FontDisplaySmallBaseSize") == true) {
		displayFontNameSmallSize = strToInt(lang.get("FontDisplaySmallBaseSize"));
	}

	string displayFontNameSmall = displayFontNameSmallPrefix + intToStr(displayFontNameSmallSize) + displayFontNameSmallPostfix;

	if(displayFontSmall) {
		renderer.endFont(displayFontSmall, rsGlobal);
		displayFontSmall=NULL;
	}
	if(Renderer::renderText3DEnabled == false) {
		displayFontSmall= renderer.newFont(rsGlobal);
		if(displayFontSmall) {
			displayFontSmall->setType(displayFontNameSmall,config.getString("FontSmallDisplay",""),config.getString("FontSmallDisplayFamily",""));
			displayFontSmall->setSize(displayFontNameSmallSize);
			//displayFontSmall->setYOffsetFactor(config.getFloat("FontSmallDisplayYOffsetFactor",floatToStr(FontMetrics::DEFAULT_Y_OFFSET_FACTOR).c_str()));
		}
	}

	if(displayFontSmall3D) {
		renderer.endFont(displayFontSmall3D, rsGlobal);
		displayFontSmall3D=NULL;
	}
	if(Renderer::renderText3DEnabled == true) {
		displayFontSmall3D= renderer.newFont3D(rsGlobal);
		if(displayFontSmall3D) {
			displayFontSmall3D->setType(displayFontNameSmall,config.getString("FontSmallDisplay",""),config.getString("FontSmallDisplayFamily",""));
			displayFontSmall3D->setSize(displayFontNameSmallSize);
			//displayFontSmall3D->setYOffsetFactor(config.getFloat("FontSmallDisplayYOffsetFactor",floatToStr(FontMetrics::DEFAULT_Y_OFFSET_FACTOR).c_str()));
		}
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] displayFontSmallName = [%s] displayFontSmallNameSize = %d\n",__FILE__,__FUNCTION__,__LINE__,displayFontNameSmall.c_str(),displayFontNameSmallSize);

	string menuFontNameNormalPrefix		= config.getString("FontMenuNormalPrefix");
	string menuFontNameNormalPostfix	= config.getString("FontMenuNormalPostfix");
	int menuFontNameNormalSize			= computeFontSize(config.getInt("FontMenuNormalBaseSize"));

	if(lang.hasString("FontMenuNormalPrefix") == true) {
		menuFontNameNormalPrefix = lang.get("FontMenuNormalPrefix");
	}
	if(lang.hasString("FontMenuNormalPostfix") == true) {
		menuFontNameNormalPostfix = lang.get("FontMenuNormalPostfix");
	}
	if(lang.hasString("FontMenuNormalBaseSize") == true) {
		menuFontNameNormalSize = strToInt(lang.get("FontMenuNormalBaseSize"));
	}

	string menuFontNameNormal= menuFontNameNormalPrefix + intToStr(menuFontNameNormalSize) + menuFontNameNormalPostfix;

	if(menuFontNormal) {
		renderer.endFont(menuFontNormal, rsGlobal);
		menuFontNormal=NULL;
	}
	if(Renderer::renderText3DEnabled == false) {
		menuFontNormal= renderer.newFont(rsGlobal);
		if(menuFontNormal) {
			menuFontNormal->setType(menuFontNameNormal,config.getString("FontMenuNormal",""),config.getString("FontMenuNormalFamily",""));
			menuFontNormal->setSize(menuFontNameNormalSize);
			menuFontNormal->setWidth(Font::wBold);
			//menuFontNormal->setYOffsetFactor(config.getFloat("FontMenuNormalYOffsetFactor",floatToStr(FontMetrics::DEFAULT_Y_OFFSET_FACTOR).c_str()));
		}
	}

	if(menuFontNormal3D) {
		renderer.endFont(menuFontNormal3D, rsGlobal);
		menuFontNormal3D=NULL;
	}
	if(Renderer::renderText3DEnabled == true) {
		menuFontNormal3D= renderer.newFont3D(rsGlobal);
		if(menuFontNormal3D) {
			menuFontNormal3D->setType(menuFontNameNormal,config.getString("FontMenuNormal",""),config.getString("FontMenuNormalFamily",""));
			menuFontNormal3D->setSize(menuFontNameNormalSize);
			menuFontNormal3D->setWidth(Font::wBold);
			//menuFontNormal3D->setYOffsetFactor(config.getFloat("FontMenuNormalYOffsetFactor",floatToStr(FontMetrics::DEFAULT_Y_OFFSET_FACTOR).c_str()));
		}
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] menuFontNormalName = [%s] menuFontNormalNameSize = %d\n",__FILE__,__FUNCTION__,__LINE__,menuFontNameNormal.c_str(),menuFontNameNormalSize);

	string menuFontNameBigPrefix	= config.getString("FontMenuBigPrefix");
	string menuFontNameBigPostfix	= config.getString("FontMenuBigPostfix");
	int menuFontNameBigSize			= computeFontSize(config.getInt("FontMenuBigBaseSize"));

	if(lang.hasString("FontMenuBigPrefix") == true) {
		menuFontNameBigPrefix = lang.get("FontMenuBigPrefix");
	}
	if(lang.hasString("FontMenuBigPostfix") == true) {
		menuFontNameBigPostfix = lang.get("FontMenuBigPostfix");
	}
	if(lang.hasString("FontMenuBigBaseSize") == true) {
		menuFontNameBigSize = strToInt(lang.get("FontMenuBigBaseSize"));
	}

	string menuFontNameBig= menuFontNameBigPrefix+intToStr(menuFontNameBigSize)+menuFontNameBigPostfix;

	if(menuFontBig) {
		renderer.endFont(menuFontBig, rsGlobal);
		menuFontBig=NULL;
	}
	if(Renderer::renderText3DEnabled == false) {
		menuFontBig= renderer.newFont(rsGlobal);
		if(menuFontBig) {
			menuFontBig->setType(menuFontNameBig,config.getString("FontMenuBig",""),config.getString("FontMenuBigFamily",""));
			menuFontBig->setSize(menuFontNameBigSize);
			//menuFontBig->setYOffsetFactor(config.getFloat("FontMenuBigYOffsetFactor",floatToStr(FontMetrics::DEFAULT_Y_OFFSET_FACTOR).c_str()));
		}
	}

	if(menuFontBig3D) {
		renderer.endFont(menuFontBig3D, rsGlobal);
		menuFontBig3D=NULL;
	}
	if(Renderer::renderText3DEnabled == true) {
		menuFontBig3D= renderer.newFont3D(rsGlobal);
		if(menuFontBig3D) {
			menuFontBig3D->setType(menuFontNameBig,config.getString("FontMenuBig",""),config.getString("FontMenuBigFamily",""));
			menuFontBig3D->setSize(menuFontNameBigSize);
			//menuFontBig3D->setYOffsetFactor(config.getFloat("FontMenuBigYOffsetFactor",floatToStr(FontMetrics::DEFAULT_Y_OFFSET_FACTOR).c_str()));
		}
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] menuFontNameBig = [%s] menuFontNameBigSize = %d\n",__FILE__,__FUNCTION__,__LINE__,menuFontNameBig.c_str(),menuFontNameBigSize);

	string menuFontNameVeryBigPrefix	= config.getString("FontMenuBigPrefix");
	string menuFontNameVeryBigPostfix	= config.getString("FontMenuBigPostfix");
	int menuFontNameVeryBigSize			= computeFontSize(config.getInt("FontMenuVeryBigBaseSize"));

	if(lang.hasString("FontMenuBigPrefix") == true) {
		menuFontNameVeryBigPrefix = lang.get("FontMenuBigPrefix");
	}
	if(lang.hasString("FontMenuBigPostfix") == true) {
		menuFontNameVeryBigPostfix = lang.get("FontMenuBigPostfix");
	}
	if(lang.hasString("FontMenuVeryBigBaseSize") == true) {
		menuFontNameVeryBigSize = strToInt(lang.get("FontMenuVeryBigBaseSize"));
	}

	string menuFontNameVeryBig= menuFontNameVeryBigPrefix + intToStr(menuFontNameVeryBigSize) + menuFontNameVeryBigPostfix;

	if(menuFontVeryBig) {
		renderer.endFont(menuFontVeryBig, rsGlobal);
		menuFontVeryBig=NULL;
	}
	if(Renderer::renderText3DEnabled == false) {
		menuFontVeryBig= renderer.newFont(rsGlobal);
		if(menuFontVeryBig) {
			menuFontVeryBig->setType(menuFontNameVeryBig,config.getString("FontMenuVeryBig",""),config.getString("FontMenuVeryBigFamily",""));
			menuFontVeryBig->setSize(menuFontNameVeryBigSize);
			//menuFontVeryBig->setYOffsetFactor(config.getFloat("FontMenuVeryBigYOffsetFactor",floatToStr(FontMetrics::DEFAULT_Y_OFFSET_FACTOR).c_str()));
		}
	}

	if(menuFontVeryBig3D) {
		renderer.endFont(menuFontVeryBig3D, rsGlobal);
		menuFontVeryBig3D=NULL;
	}
	if(Renderer::renderText3DEnabled == true) {
		menuFontVeryBig3D= renderer.newFont3D(rsGlobal);
		if(menuFontVeryBig3D) {
			menuFontVeryBig3D->setType(menuFontNameVeryBig,config.getString("FontMenuVeryBig",""),config.getString("FontMenuVeryBigFamily",""));
			menuFontVeryBig3D->setSize(menuFontNameVeryBigSize);
			//menuFontVeryBig3D->setYOffsetFactor(config.getFloat("FontMenuVeryBigYOffsetFactor",floatToStr(FontMetrics::DEFAULT_Y_OFFSET_FACTOR).c_str()));
		}
	}

	//printf("CoreData menuFontVeryBig3D [%d] menuFontVeryBig3D [%p]\n",menuFontVeryBig3D->getSize(),menuFontVeryBig3D);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] menuFontNameVeryBig = [%s] menuFontNameVeryBigSize = %d\n",__FILE__,__FUNCTION__,__LINE__,menuFontNameVeryBig.c_str(),menuFontNameVeryBigSize);

	//console font
	string consoleFontNamePrefix	= config.getString("FontConsolePrefix");
	string consoleFontNamePostfix	= config.getString("FontConsolePostfix");
	int consoleFontNameSize			= computeFontSize(config.getInt("FontConsoleBaseSize"));

	if(lang.hasString("FontConsolePrefix") == true) {
		consoleFontNamePrefix = lang.get("FontConsolePrefix");
	}
	if(lang.hasString("FontConsolePostfix") == true) {
		consoleFontNamePostfix = lang.get("FontConsolePostfix");
	}
	if(lang.hasString("FontConsoleBaseSize") == true) {
		consoleFontNameSize = strToInt(lang.get("FontConsoleBaseSize"));
	}

	string consoleFontName= consoleFontNamePrefix + intToStr(consoleFontNameSize) + consoleFontNamePostfix;

	if(consoleFont) {
		renderer.endFont(consoleFont, rsGlobal);
		consoleFont=NULL;
	}
	if(Renderer::renderText3DEnabled == false) {
		consoleFont= renderer.newFont(rsGlobal);
		if(consoleFont) {
			consoleFont->setType(consoleFontName,config.getString("FontConsole",""),config.getString("FontConsoleFamily",""));
			consoleFont->setSize(consoleFontNameSize);
			//consoleFont->setYOffsetFactor(config.getFloat("FontConsoleYOffsetFactor",floatToStr(FontMetrics::DEFAULT_Y_OFFSET_FACTOR).c_str()));
		}
	}

	if(consoleFont3D) {
		renderer.endFont(consoleFont3D, rsGlobal);
		consoleFont3D=NULL;
	}
	if(Renderer::renderText3DEnabled == true) {
		consoleFont3D= renderer.newFont3D(rsGlobal);
		if(consoleFont3D) {
			consoleFont3D->setType(consoleFontName,config.getString("FontConsole",""),config.getString("FontConsoleFamily",""));
			consoleFont3D->setSize(consoleFontNameSize);
			//consoleFont3D->setYOffsetFactor(config.getFloat("FontConsoleYOffsetFactor",floatToStr(FontMetrics::DEFAULT_Y_OFFSET_FACTOR).c_str()));
		}
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] consoleFontName = [%s] consoleFontNameSize = %d\n",__FILE__,__FUNCTION__,__LINE__,consoleFontName.c_str(),consoleFontNameSize);
}

int CoreData::computeFontSize(int size) {
	int rs = size;
	Config &config= Config::getInstance();
	if(Font::forceLegacyFonts == true) {
		int screenH = config.getInt("ScreenHeight");
		rs = size * screenH / 1024;
	}
	else {
		if(Renderer::renderText3DEnabled) {
			//rs = ((float)size * 0.85);
			//rs = 24;
		}
		//int screenH = config.getInt("ScreenHeight");
		//rs = size * screenH / 1024;

	}
	//FontSizeAdjustment
	rs += config.getInt("FontSizeAdjustment");
	if(Font::forceLegacyFonts == false) {
		rs += Font::baseSize; //basesize only for new font system
	}
	if(Font::forceLegacyFonts == true) {
		if(rs < 10) {
			rs= 10;
		}
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] fontsize orginal %d      calculated:%d   \n",__FILE__,__FUNCTION__,__LINE__,size,rs);
	return rs;
}

void CoreData::saveGameSettingsToFile(std::string fileName, GameSettings *gameSettings, int advancedIndex) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

    Config &config = Config::getInstance();
    string userData = config.getString("UserData_Root","");
    if(userData != "") {
    	endPathWithSlash(userData);
    }
    fileName = userData + fileName;

    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] fileName = [%s]\n",__FILE__,__FUNCTION__,__LINE__,fileName.c_str());

#if defined(WIN32) && !defined(__MINGW32__)
	FILE *fp = _wfopen(utf8_decode(fileName).c_str(), L"w");
	std::ofstream saveGameFile(fp);
#else
    std::ofstream saveGameFile;
    saveGameFile.open(fileName.c_str(), ios_base::out | ios_base::trunc);
#endif

	saveGameFile << "Description=" << gameSettings->getDescription() << std::endl;
	saveGameFile << "MapFilterIndex=" << gameSettings->getMapFilterIndex() << std::endl;
	saveGameFile << "Map=" << gameSettings->getMap() << std::endl;
	saveGameFile << "Tileset=" << gameSettings->getTileset() << std::endl;
	saveGameFile << "TechTree=" << gameSettings->getTech() << std::endl;
	saveGameFile << "DefaultUnits=" << gameSettings->getDefaultUnits() << std::endl;
	saveGameFile << "DefaultResources=" << gameSettings->getDefaultResources() << std::endl;
	saveGameFile << "DefaultVictoryConditions=" << gameSettings->getDefaultVictoryConditions() << std::endl;
	saveGameFile << "FogOfWar=" << gameSettings->getFogOfWar() << std::endl;
	saveGameFile << "AdvancedIndex=" << advancedIndex << std::endl;
	saveGameFile << "AllowObservers=" << gameSettings->getAllowObservers() << std::endl;
	saveGameFile << "FlagTypes1=" << gameSettings->getFlagTypes1() << std::endl;
	saveGameFile << "EnableObserverModeAtEndGame=" << gameSettings->getEnableObserverModeAtEndGame() << std::endl;
	saveGameFile << "AiAcceptSwitchTeamPercentChance=" << gameSettings->getAiAcceptSwitchTeamPercentChance() << std::endl;
	saveGameFile << "FallbackCpuMultiplier=" << gameSettings->getFallbackCpuMultiplier() << std::endl;
	saveGameFile << "PathFinderType=" << gameSettings->getPathFinderType() << std::endl;
	saveGameFile << "EnableServerControlledAI=" << gameSettings->getEnableServerControlledAI() << std::endl;
	saveGameFile << "NetworkFramePeriod=" << gameSettings->getNetworkFramePeriod() << std::endl;
	saveGameFile << "NetworkPauseGameForLaggedClients=" << gameSettings->getNetworkPauseGameForLaggedClients() << std::endl;

	saveGameFile << "FactionThisFactionIndex=" << gameSettings->getThisFactionIndex() << std::endl;
	saveGameFile << "FactionCount=" << gameSettings->getFactionCount() << std::endl;

	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		int slotIndex = gameSettings->getStartLocationIndex(i);

		saveGameFile << "FactionControlForIndex" 		<< slotIndex << "=" << gameSettings->getFactionControl(i) << std::endl;
		saveGameFile << "ResourceMultiplierIndex" 		<< slotIndex << "=" << gameSettings->getResourceMultiplierIndex(i) << std::endl;
		saveGameFile << "FactionTeamForIndex" 			<< slotIndex << "=" << gameSettings->getTeam(i) << std::endl;
		saveGameFile << "FactionStartLocationForIndex" 	<< slotIndex << "=" << gameSettings->getStartLocationIndex(i) << std::endl;
		saveGameFile << "FactionTypeNameForIndex" 		<< slotIndex << "=" << gameSettings->getFactionTypeName(i) << std::endl;
		saveGameFile << "FactionPlayerNameForIndex" 	<< slotIndex << "=" << gameSettings->getNetworkPlayerName(i) << std::endl;
    }

#if defined(WIN32) && !defined(__MINGW32__)
	fclose(fp);
#endif
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
}

bool CoreData::loadGameSettingsFromFile(std::string fileName, GameSettings *gameSettings) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

	bool fileWasFound = false;
    Config &config = Config::getInstance();
    string userData = config.getString("UserData_Root","");
    if(userData != "") {
    	endPathWithSlash(userData);
    }
    if(fileExists(userData + fileName) == true) {
    	fileName = userData + fileName;
    	fileWasFound = true;
    }

    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] fileName = [%s]\n",__FILE__,__FUNCTION__,__LINE__,fileName.c_str());

    if(fileExists(fileName) == false) {
    	return false;
    }

    fileWasFound = true;
    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] fileName = [%s]\n",__FILE__,__FUNCTION__,__LINE__,fileName.c_str());

	Properties properties;
	properties.load(fileName);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] fileName = [%s]\n",__FILE__,__FUNCTION__,__LINE__,fileName.c_str());

	gameSettings->setMapFilterIndex(properties.getInt("MapFilterIndex","0"));
	gameSettings->setDescription(properties.getString("Description"));
	gameSettings->setMap(properties.getString("Map"));
	gameSettings->setTileset(properties.getString("Tileset"));
	gameSettings->setTech(properties.getString("TechTree"));
	gameSettings->setDefaultUnits(properties.getBool("DefaultUnits"));
	gameSettings->setDefaultResources(properties.getBool("DefaultResources"));
	gameSettings->setDefaultVictoryConditions(properties.getBool("DefaultVictoryConditions"));
	gameSettings->setFogOfWar(properties.getBool("FogOfWar"));
	//listBoxAdvanced.setSelectedItemIndex(properties.getInt("AdvancedIndex","0"));

	gameSettings->setAllowObservers(properties.getBool("AllowObservers","false"));
	gameSettings->setFlagTypes1(properties.getInt("FlagTypes1","0"));
	gameSettings->setEnableObserverModeAtEndGame(properties.getBool("EnableObserverModeAtEndGame"));
	gameSettings->setAiAcceptSwitchTeamPercentChance(properties.getInt("AiAcceptSwitchTeamPercentChance","30"));
	gameSettings->setFallbackCpuMultiplier(properties.getInt("FallbackCpuMultiplier","1"));

	gameSettings->setPathFinderType(static_cast<PathFinderType>(properties.getInt("PathFinderType",intToStr(pfBasic).c_str())));
	gameSettings->setEnableServerControlledAI(properties.getBool("EnableServerControlledAI","true"));
	gameSettings->setNetworkFramePeriod(properties.getInt("NetworkFramePeriod",intToStr(GameConstants::networkFramePeriod).c_str()));
	gameSettings->setNetworkPauseGameForLaggedClients(properties.getBool("NetworkPauseGameForLaggedClients","false"));

	gameSettings->setThisFactionIndex(properties.getInt("FactionThisFactionIndex"));
	gameSettings->setFactionCount(properties.getInt("FactionCount"));

	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		gameSettings->setFactionControl(i,(ControlType)properties.getInt(string("FactionControlForIndex") + intToStr(i),intToStr(ctClosed).c_str()) );

		if(gameSettings->getFactionControl(i) == ctNetworkUnassigned) {
			gameSettings->setFactionControl(i,ctNetwork);
		}

		gameSettings->setResourceMultiplierIndex(i,properties.getInt(string("ResourceMultiplierIndex") + intToStr(i),"5"));
		gameSettings->setTeam(i,properties.getInt(string("FactionTeamForIndex") + intToStr(i),"0") );
		gameSettings->setStartLocationIndex(i,properties.getInt(string("FactionStartLocationForIndex") + intToStr(i),intToStr(i).c_str()) );
		gameSettings->setFactionTypeName(i,properties.getString(string("FactionTypeNameForIndex") + intToStr(i),"?") );

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] i = %d, factionTypeName [%s]\n",__FILE__,__FUNCTION__,__LINE__,i,gameSettings->getFactionTypeName(i).c_str());

		if(gameSettings->getFactionControl(i) == ctHuman) {
			gameSettings->setNetworkPlayerName(i,properties.getString(string("FactionPlayerNameForIndex") + intToStr(i),"") );
		}
		else {
			gameSettings->setNetworkPlayerName(i,"");
		}
	}

    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

    return fileWasFound;
}

// ================== PRIVATE ========================

}}//end namespace
