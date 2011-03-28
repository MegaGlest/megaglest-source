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

	//display font
	Config &config= Config::getInstance();
	string displayFontNamePrefix=config.getString("FontDisplayPrefix");
	string displayFontNamePostfix=config.getString("FontDisplayPostfix");
	int displayFontSize=computeFontSize(config.getInt("FontDisplayBaseSize"));
	string displayFontName=displayFontNamePrefix+intToStr(displayFontSize)+displayFontNamePostfix;
	displayFont= renderer.newFont(rsGlobal);
	displayFont->setType(displayFontName);
	displayFont->setSize(displayFontSize);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] displayFontName = [%s] displayFontSize = %d\n",__FILE__,__FUNCTION__,__LINE__,displayFontName.c_str(),displayFontSize);

	//menu fonts
	string displayFontNameSmallPrefix= config.getString("FontDisplayPrefix");
	string displayFontNameSmallPostfix= config.getString("FontDisplayPostfix");
	int displayFontNameSmallSize=computeFontSize(config.getInt("FontDisplaySmallBaseSize"));
	string displayFontNameSmall=displayFontNameSmallPrefix+intToStr(displayFontNameSmallSize)+displayFontNameSmallPostfix;
	displayFontSmall= renderer.newFont(rsGlobal);
	displayFontSmall->setType(displayFontNameSmall);
	displayFontSmall->setSize(displayFontNameSmallSize);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] displayFontSmallName = [%s] displayFontSmallNameSize = %d\n",__FILE__,__FUNCTION__,__LINE__,displayFontNameSmall.c_str(),displayFontNameSmallSize);

	string menuFontNameNormalPrefix= config.getString("FontMenuNormalPrefix");
	string menuFontNameNormalPostfix= config.getString("FontMenuNormalPostfix");
	int menuFontNameNormalSize=computeFontSize(config.getInt("FontMenuNormalBaseSize"));
	string menuFontNameNormal= menuFontNameNormalPrefix+intToStr(menuFontNameNormalSize)+menuFontNameNormalPostfix;
	menuFontNormal= renderer.newFont(rsGlobal);
	menuFontNormal->setType(menuFontNameNormal);
	menuFontNormal->setSize(menuFontNameNormalSize);
	menuFontNormal->setWidth(Font::wBold);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] menuFontNormalName = [%s] menuFontNormalNameSize = %d\n",__FILE__,__FUNCTION__,__LINE__,menuFontNameNormal.c_str(),menuFontNameNormalSize);

	string menuFontNameBigPrefix= config.getString("FontMenuBigPrefix");
	string menuFontNameBigPostfix= config.getString("FontMenuBigPostfix");
	int menuFontNameBigSize=computeFontSize(config.getInt("FontMenuBigBaseSize"));
	string menuFontNameBig= menuFontNameBigPrefix+intToStr(menuFontNameBigSize)+menuFontNameBigPostfix;
	menuFontBig= renderer.newFont(rsGlobal);
	menuFontBig->setType(menuFontNameBig);
	menuFontBig->setSize(menuFontNameBigSize);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] menuFontNameBig = [%s] menuFontNameBigSize = %d\n",__FILE__,__FUNCTION__,__LINE__,menuFontNameBig.c_str(),menuFontNameBigSize);

	string menuFontNameVeryBigPrefix= config.getString("FontMenuBigPrefix");
	string menuFontNameVeryBigPostfix= config.getString("FontMenuBigPostfix");
	int menuFontNameVeryBigSize=computeFontSize(config.getInt("FontMenuVeryBigBaseSize"));
	string menuFontNameVeryBig= menuFontNameVeryBigPrefix+intToStr(menuFontNameVeryBigSize)+menuFontNameVeryBigPostfix;
	menuFontVeryBig= renderer.newFont(rsGlobal);
	menuFontVeryBig->setType(menuFontNameVeryBig);
	menuFontVeryBig->setSize(menuFontNameVeryBigSize);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] menuFontNameVeryBig = [%s] menuFontNameVeryBigSize = %d\n",__FILE__,__FUNCTION__,__LINE__,menuFontNameVeryBig.c_str(),menuFontNameVeryBigSize);

	//console font
	string consoleFontNamePrefix= config.getString("FontConsolePrefix");
	string consoleFontNamePostfix= config.getString("FontConsolePostfix");
	int consoleFontNameSize=computeFontSize(config.getInt("FontConsoleBaseSize"));
	string consoleFontName= consoleFontNamePrefix+intToStr(consoleFontNameSize)+consoleFontNamePostfix;
	consoleFont= renderer.newFont(rsGlobal);
	consoleFont->setType(consoleFontName);
	consoleFont->setSize(consoleFontNameSize);

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

int CoreData::computeFontSize(int size){
	Config &config= Config::getInstance();
	int screenH= config.getInt("ScreenHeight");
	int rs= size*screenH/1024;
	//FontSizeAdjustment
	rs=rs+config.getInt("FontSizeAdjustment");
	if(rs<10){
		rs= 10;
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] fontsize orginal %d      calculated:%d   \n",__FILE__,__FUNCTION__,__LINE__,size,rs);
	return rs;
}

// ================== PRIVATE ========================

}}//end namespace
