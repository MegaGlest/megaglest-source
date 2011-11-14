// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
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
}

CoreData::~CoreData() {
	deleteValues(waterSounds.getSoundsPtr()->begin(), waterSounds.getSoundsPtr()->end());
	waterSounds.getSoundsPtr()->clear();
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
	}

	fireTexture= renderer.newTexture2D(rsGlobal);
	if(fireTexture) {
		fireTexture->setFormat(Texture::fAlpha);
		fireTexture->getPixmap()->init(1);
		fireTexture->getPixmap()->load(getGameCustomCoreDataPath(data_path, "data/core/misc_textures/fire_particle.tga"));
	}

	teamColorTexture= renderer.newTexture2D(rsGlobal);
	if(teamColorTexture) {
		teamColorTexture->setFormat(Texture::fAlpha);
		teamColorTexture->getPixmap()->init(1);
		teamColorTexture->getPixmap()->load(getGameCustomCoreDataPath(data_path, "data/core/misc_textures/team_color_texture.tga"));
	}

	snowTexture= renderer.newTexture2D(rsGlobal);
	if(snowTexture) {
		snowTexture->setMipmap(false);
		snowTexture->setFormat(Texture::fAlpha);
		snowTexture->getPixmap()->init(1);
		snowTexture->getPixmap()->load(getGameCustomCoreDataPath(data_path, "data/core/misc_textures/snow_particle.tga"));
	}

	customTexture= renderer.newTexture2D(rsGlobal);
	if(customTexture) {
		customTexture->getPixmap()->load(getGameCustomCoreDataPath(data_path, "data/core/menu/textures/custom_texture.tga"));
	}

	notOnServerTexture= renderer.newTexture2D(rsGlobal);
	if(notOnServerTexture) {
		notOnServerTexture->getPixmap()->load(getGameCustomCoreDataPath(data_path, "data/core/menu/textures/not_on_server.tga"));
	}

	onServerDifferentTexture= renderer.newTexture2D(rsGlobal);
	if(onServerDifferentTexture) {
		onServerDifferentTexture->getPixmap()->load(getGameCustomCoreDataPath(data_path, "data/core/menu/textures/on_server_different.tga"));

		onServerTexture= renderer.newTexture2D(rsGlobal);
		onServerTexture->getPixmap()->load(getGameCustomCoreDataPath(data_path, "data/core/menu/textures/on_server.tga"));

		onServerInstalledTexture= renderer.newTexture2D(rsGlobal);
		onServerInstalledTexture->getPixmap()->load(getGameCustomCoreDataPath(data_path, "data/core/menu/textures/on_server_installed.tga"));
	}

	logoTexture= renderer.newTexture2D(rsGlobal);
	if(logoTexture) {
		logoTexture->setMipmap(false);
		logoTexture->getPixmap()->load(getGameCustomCoreDataPath(data_path, "data/core/menu/textures/logo.tga"));
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

	if(logoTextureList.size() == 0) {
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
    if(miscTextureList.size() == 0) {
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
	}

	buttonSmallTexture= renderer.newTexture2D(rsGlobal);
	if(buttonSmallTexture) {
		buttonSmallTexture->setForceCompressionDisabled(true);
		buttonSmallTexture->getPixmap()->load(getGameCustomCoreDataPath(data_path, "data/core/menu/textures/button_small.tga"));
	}

	buttonBigTexture= renderer.newTexture2D(rsGlobal);
	if(buttonBigTexture) {
		buttonBigTexture->setForceCompressionDisabled(true);
		buttonBigTexture->getPixmap()->load(getGameCustomCoreDataPath(data_path, "data/core/menu/textures/button_big.tga"));
	}

	horizontalLineTexture= renderer.newTexture2D(rsGlobal);
	if(horizontalLineTexture) {
		horizontalLineTexture->setForceCompressionDisabled(true);
		horizontalLineTexture->getPixmap()->load(getGameCustomCoreDataPath(data_path, "data/core/menu/textures/line_horizontal.tga"));
	}

	verticalLineTexture= renderer.newTexture2D(rsGlobal);
	if(verticalLineTexture) {
		verticalLineTexture->setForceCompressionDisabled(true);
		verticalLineTexture->getPixmap()->load(getGameCustomCoreDataPath(data_path, "data/core/menu/textures/line_vertical.tga"));
	}

	checkBoxTexture= renderer.newTexture2D(rsGlobal);
	if(checkBoxTexture) {
		checkBoxTexture->setForceCompressionDisabled(true);
		checkBoxTexture->getPixmap()->load(getGameCustomCoreDataPath(data_path, "data/core/menu/textures/checkbox.tga"));
	}

	checkedCheckBoxTexture= renderer.newTexture2D(rsGlobal);
	if(checkedCheckBoxTexture) {
		checkedCheckBoxTexture->setForceCompressionDisabled(true);
		checkedCheckBoxTexture->getPixmap()->load(getGameCustomCoreDataPath(data_path, "data/core/menu/textures/checkbox_checked.tga"));
	}

	gameWinnerTexture= renderer.newTexture2D(rsGlobal);
	if(gameWinnerTexture) {
		gameWinnerTexture->setForceCompressionDisabled(true);
		gameWinnerTexture->getPixmap()->load(getGameCustomCoreDataPath(data_path, "data/core/misc_textures/game_winner.png"));
	}

	loadFonts();

	//sounds
	XmlTree xmlTree;
	//string data_path = getGameReadWritePath(GameConstants::path_data_CacheLookupKey);
	xmlTree.load(getGameCustomCoreDataPath(data_path, "data/core/menu/menu.xml"),Properties::getTagReplacementValues());
	const XmlNode *menuNode= xmlTree.getRootNode();
	const XmlNode *introNode= menuNode->getChild("intro");

    clickSoundA.load(getGameCustomCoreDataPath(data_path, "data/core/menu/sound/click_a.wav"));
    clickSoundB.load(getGameCustomCoreDataPath(data_path, "data/core/menu/sound/click_b.wav"));
    clickSoundC.load(getGameCustomCoreDataPath(data_path, "data/core/menu/sound/click_c.wav"));
    attentionSound.load(getGameCustomCoreDataPath(data_path, "data/core/menu/sound/attention.wav"));
    highlightSound.load(getGameCustomCoreDataPath(data_path, "data/core/menu/sound/highlight.wav"));

	// intro info
	const XmlNode *menuPathNode= introNode->getChild("menu-music-path");
	string menuMusicPath = menuPathNode->getAttribute("value")->getRestrictedValue();
	const XmlNode *menuIntroMusicNode= introNode->getChild("menu-intro-music");
	string menuIntroMusicFile = menuIntroMusicNode->getAttribute("value")->getRestrictedValue();
	const XmlNode *menuMusicNode= introNode->getChild("menu-music");
	string menuMusicFile = menuMusicNode->getAttribute("value")->getRestrictedValue();

	introMusic.open(getGameCustomCoreDataPath(data_path, "data/core/" + menuMusicPath + menuIntroMusicFile));
	introMusic.setNext(&menuMusic);
	menuMusic.open(getGameCustomCoreDataPath(data_path, "data/core/" + menuMusicPath + menuMusicFile));
	menuMusic.setNext(&menuMusic);

	waterSounds.resize(6);

	for(int i=0; i<6; ++i){
		waterSounds[i]= new StaticSound();
		if(waterSounds[i]) {
			waterSounds[i]->load(getGameCustomCoreDataPath(data_path, "data/core/water_sounds/water"+intToStr(i)+".wav"));
		}
	}

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
			displayFont->setType(displayFontName,config.getString("FontDisplay",""));
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
			displayFont3D->setType(displayFontName,config.getString("FontDisplay",""));
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
			displayFontSmall->setType(displayFontNameSmall,config.getString("FontSmallDisplay",""));
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
			displayFontSmall3D->setType(displayFontNameSmall,config.getString("FontSmallDisplay",""));
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
			menuFontNormal->setType(menuFontNameNormal,config.getString("FontMenuNormal",""));
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
			menuFontNormal3D->setType(menuFontNameNormal,config.getString("FontMenuNormal",""));
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
			menuFontBig->setType(menuFontNameBig,config.getString("FontMenuBig",""));
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
			menuFontBig3D->setType(menuFontNameBig,config.getString("FontMenuBig",""));
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
			menuFontVeryBig->setType(menuFontNameVeryBig,config.getString("FontMenuVeryBig",""));
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
			menuFontVeryBig3D->setType(menuFontNameVeryBig,config.getString("FontMenuVeryBig",""));
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
			consoleFont->setType(consoleFontName,config.getString("FontConsole",""));
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
			consoleFont3D->setType(consoleFontName,config.getString("FontConsole",""));
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

// ================== PRIVATE ========================

}}//end namespace
