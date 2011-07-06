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

CoreData::~CoreData() {
	deleteValues(waterSounds.getSounds().begin(), waterSounds.getSounds().end());
}

void CoreData::load() {
	string data_path = getGameReadWritePath(GameConstants::path_data_CacheLookupKey);
	if(data_path != "") {
		endPathWithSlash(data_path);
	}

	const string dir = data_path + "data/core";
	Logger::getInstance().add("Core data");

	Renderer &renderer= Renderer::getInstance();

	//textures
	backgroundTexture= renderer.newTexture2D(rsGlobal);
	backgroundTexture->setMipmap(false);
	backgroundTexture->getPixmap()->load(dir+"/menu/textures/back.tga");

	fireTexture= renderer.newTexture2D(rsGlobal);
	fireTexture->setFormat(Texture::fAlpha);
	fireTexture->getPixmap()->init(1);
	fireTexture->getPixmap()->load(dir+"/misc_textures/fire_particle.tga");

	teamColorTexture= renderer.newTexture2D(rsGlobal);
	teamColorTexture->setFormat(Texture::fAlpha);
	teamColorTexture->getPixmap()->init(1);
	teamColorTexture->getPixmap()->load(dir+"/misc_textures/team_color_texture.tga");

	snowTexture= renderer.newTexture2D(rsGlobal);
	snowTexture->setMipmap(false);
	snowTexture->setFormat(Texture::fAlpha);
	snowTexture->getPixmap()->init(1);
	snowTexture->getPixmap()->load(dir+"/misc_textures/snow_particle.tga");

	customTexture= renderer.newTexture2D(rsGlobal);
	customTexture->getPixmap()->load(dir+"/menu/textures/custom_texture.tga");

	logoTexture= renderer.newTexture2D(rsGlobal);
	logoTexture->setMipmap(false);
	logoTexture->getPixmap()->load(dir+"/menu/textures/logo.tga");

	logoTextureList.clear();
	string logosPath= dir+"/menu/textures/logo*.*";
	vector<string> logoFilenames;
    findAll(logosPath, logoFilenames);
    for(int i = 0; i < logoFilenames.size(); ++i) {
    	string logo = logoFilenames[i];
    	if(strcmp("logo.tga",logo.c_str()) != 0) {
    		Texture2D *logoTextureExtra= renderer.newTexture2D(rsGlobal);
    		logoTextureExtra->setMipmap(true);
    		logoTextureExtra->getPixmap()->load(dir+"/menu/textures/" + logo);
    		logoTextureList.push_back(logoTextureExtra);
    	}
    }

	waterSplashTexture= renderer.newTexture2D(rsGlobal);
	waterSplashTexture->setFormat(Texture::fAlpha);
	waterSplashTexture->getPixmap()->init(1);
	waterSplashTexture->getPixmap()->load(dir+"/misc_textures/water_splash.tga");

	buttonSmallTexture= renderer.newTexture2D(rsGlobal);
	buttonSmallTexture->setForceCompressionDisabled(true);
	buttonSmallTexture->getPixmap()->load(dir+"/menu/textures/button_small.tga");

	buttonBigTexture= renderer.newTexture2D(rsGlobal);
	buttonBigTexture->setForceCompressionDisabled(true);
	buttonBigTexture->getPixmap()->load(dir+"/menu/textures/button_big.tga");

	horizontalLineTexture= renderer.newTexture2D(rsGlobal);
	horizontalLineTexture->setForceCompressionDisabled(true);
	horizontalLineTexture->getPixmap()->load(dir+"/menu/textures/line_horizontal.tga");

	verticalLineTexture= renderer.newTexture2D(rsGlobal);
	verticalLineTexture->setForceCompressionDisabled(true);
	verticalLineTexture->getPixmap()->load(dir+"/menu/textures/line_vertical.tga");

	checkBoxTexture= renderer.newTexture2D(rsGlobal);
	checkBoxTexture->setForceCompressionDisabled(true);
	checkBoxTexture->getPixmap()->load(dir+"/menu/textures/checkbox.tga");

	checkedCheckBoxTexture= renderer.newTexture2D(rsGlobal);
	checkedCheckBoxTexture->setForceCompressionDisabled(true);
	checkedCheckBoxTexture->getPixmap()->load(dir+"/menu/textures/checkbox_checked.tga");

	gameWinnerTexture= renderer.newTexture2D(rsGlobal);
	gameWinnerTexture->setForceCompressionDisabled(true);
	gameWinnerTexture->getPixmap()->load(dir+"/misc_textures/game_winner.png");

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

	displayFont= renderer.newFont(rsGlobal);
	displayFont->setType(displayFontName,config.getString("FontDisplay",""));
	displayFont->setSize(displayFontSize);
	//displayFont->setYOffsetFactor(config.getFloat("FontDisplayYOffsetFactor",floatToStr(FontMetrics::DEFAULT_Y_OFFSET_FACTOR).c_str()));

	displayFont3D= renderer.newFont3D(rsGlobal);
	displayFont3D->setType(displayFontName,config.getString("FontDisplay",""));
	displayFont3D->setSize(displayFontSize);
	//displayFont3D->setYOffsetFactor(config.getFloat("FontDisplayYOffsetFactor",floatToStr(FontMetrics::DEFAULT_Y_OFFSET_FACTOR).c_str()));

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

	displayFontSmall= renderer.newFont(rsGlobal);
	displayFontSmall->setType(displayFontNameSmall,config.getString("FontSmallDisplay",""));
	displayFontSmall->setSize(displayFontNameSmallSize);
	//displayFontSmall->setYOffsetFactor(config.getFloat("FontSmallDisplayYOffsetFactor",floatToStr(FontMetrics::DEFAULT_Y_OFFSET_FACTOR).c_str()));

	displayFontSmall3D= renderer.newFont3D(rsGlobal);
	displayFontSmall3D->setType(displayFontNameSmall,config.getString("FontSmallDisplay",""));
	displayFontSmall3D->setSize(displayFontNameSmallSize);
	//displayFontSmall3D->setYOffsetFactor(config.getFloat("FontSmallDisplayYOffsetFactor",floatToStr(FontMetrics::DEFAULT_Y_OFFSET_FACTOR).c_str()));

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

	menuFontNormal= renderer.newFont(rsGlobal);
	menuFontNormal->setType(menuFontNameNormal,config.getString("FontMenuNormal",""));
	menuFontNormal->setSize(menuFontNameNormalSize);
	menuFontNormal->setWidth(Font::wBold);
	//menuFontNormal->setYOffsetFactor(config.getFloat("FontMenuNormalYOffsetFactor",floatToStr(FontMetrics::DEFAULT_Y_OFFSET_FACTOR).c_str()));

	menuFontNormal3D= renderer.newFont3D(rsGlobal);
	menuFontNormal3D->setType(menuFontNameNormal,config.getString("FontMenuNormal",""));
	menuFontNormal3D->setSize(menuFontNameNormalSize);
	menuFontNormal3D->setWidth(Font::wBold);
	//menuFontNormal3D->setYOffsetFactor(config.getFloat("FontMenuNormalYOffsetFactor",floatToStr(FontMetrics::DEFAULT_Y_OFFSET_FACTOR).c_str()));

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

	menuFontBig= renderer.newFont(rsGlobal);
	menuFontBig->setType(menuFontNameBig,config.getString("FontMenuBig",""));
	menuFontBig->setSize(menuFontNameBigSize);
	//menuFontBig->setYOffsetFactor(config.getFloat("FontMenuBigYOffsetFactor",floatToStr(FontMetrics::DEFAULT_Y_OFFSET_FACTOR).c_str()));

	menuFontBig3D= renderer.newFont3D(rsGlobal);
	menuFontBig3D->setType(menuFontNameBig,config.getString("FontMenuBig",""));
	menuFontBig3D->setSize(menuFontNameBigSize);
	//menuFontBig3D->setYOffsetFactor(config.getFloat("FontMenuBigYOffsetFactor",floatToStr(FontMetrics::DEFAULT_Y_OFFSET_FACTOR).c_str()));

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

	menuFontVeryBig= renderer.newFont(rsGlobal);
	menuFontVeryBig->setType(menuFontNameVeryBig,config.getString("FontMenuVeryBig",""));
	menuFontVeryBig->setSize(menuFontNameVeryBigSize);
	//menuFontVeryBig->setYOffsetFactor(config.getFloat("FontMenuVeryBigYOffsetFactor",floatToStr(FontMetrics::DEFAULT_Y_OFFSET_FACTOR).c_str()));

	menuFontVeryBig3D= renderer.newFont3D(rsGlobal);
	menuFontVeryBig3D->setType(menuFontNameVeryBig,config.getString("FontMenuVeryBig",""));
	menuFontVeryBig3D->setSize(menuFontNameVeryBigSize);
	//menuFontVeryBig3D->setYOffsetFactor(config.getFloat("FontMenuVeryBigYOffsetFactor",floatToStr(FontMetrics::DEFAULT_Y_OFFSET_FACTOR).c_str()));

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

	consoleFont= renderer.newFont(rsGlobal);
	consoleFont->setType(consoleFontName,config.getString("FontConsole",""));
	consoleFont->setSize(consoleFontNameSize);
	//consoleFont->setYOffsetFactor(config.getFloat("FontConsoleYOffsetFactor",floatToStr(FontMetrics::DEFAULT_Y_OFFSET_FACTOR).c_str()));

	consoleFont3D= renderer.newFont3D(rsGlobal);
	consoleFont3D->setType(consoleFontName,config.getString("FontConsole",""));
	consoleFont3D->setSize(consoleFontNameSize);
	//consoleFont3D->setYOffsetFactor(config.getFloat("FontConsoleYOffsetFactor",floatToStr(FontMetrics::DEFAULT_Y_OFFSET_FACTOR).c_str()));

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] consoleFontName = [%s] consoleFontNameSize = %d\n",__FILE__,__FUNCTION__,__LINE__,consoleFontName.c_str(),consoleFontNameSize);

	//sounds
    clickSoundA.load(dir+"/menu/sound/click_a.wav");
    clickSoundB.load(dir+"/menu/sound/click_b.wav");
    clickSoundC.load(dir+"/menu/sound/click_c.wav");
    attentionSound.load(dir+"/menu/sound/attention.wav");
    highlightSound.load(dir+"/menu/sound/highlight.wav");
	introMusic.open(dir+"/menu/music/intro_music.ogg");
	introMusic.setNext(&menuMusic);
	menuMusic.open(dir+"/menu/music/menu_music.ogg");
	menuMusic.setNext(&menuMusic);
	waterSounds.resize(6);

	for(int i=0; i<6; ++i){
		waterSounds[i]= new StaticSound();
		waterSounds[i]->load(dir+"/water_sounds/water"+intToStr(i)+".wav");
	}

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
