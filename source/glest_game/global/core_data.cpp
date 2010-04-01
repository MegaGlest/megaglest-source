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
#include "leak_dumper.h"

using namespace Shared::Sound;
using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class CoreData
// =====================================================

// ===================== PUBLIC ========================

CoreData &CoreData::getInstance(){
	static CoreData coreData;
	return coreData;
}

CoreData::~CoreData(){
	deleteValues(waterSounds.getSounds().begin(), waterSounds.getSounds().end());
}

void CoreData::load(){
	const string dir="data/core";
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
	customTexture->getPixmap()->load("data/core/menu/textures/custom_texture.tga");

	logoTexture= renderer.newTexture2D(rsGlobal);
	logoTexture->setMipmap(false);
	logoTexture->getPixmap()->load(dir+"/menu/textures/logo.tga");

	waterSplashTexture= renderer.newTexture2D(rsGlobal);
	waterSplashTexture->setFormat(Texture::fAlpha);
	waterSplashTexture->getPixmap()->init(1);
	waterSplashTexture->getPixmap()->load(dir+"/misc_textures/water_splash.tga");

	buttonSmallTexture= renderer.newTexture2D(rsGlobal);
	buttonSmallTexture->getPixmap()->load(dir+"/menu/textures/button_small.tga");

	buttonBigTexture= renderer.newTexture2D(rsGlobal);
	buttonBigTexture->getPixmap()->load(dir+"/menu/textures/button_big.tga");

	//display font
	Config &config= Config::getInstance();
	string displayFontNamePrefix=config.getString("FontDisplayPrefix");
	string displayFontNamePostfix=config.getString("FontDisplayPostfix");
	int displayFontSize=computeFontSize(15);
	string displayFontName=displayFontNamePrefix+intToStr(displayFontSize)+displayFontNamePostfix;
	displayFont= renderer.newFont(rsGlobal);
	displayFont->setType(displayFontName);
	displayFont->setSize(displayFontSize);

	menuFontSmall=displayFont;
	menuFontNormal=displayFont;
	menuFontBig=displayFont;
	menuFontVeryBig=displayFont;
	consoleFont=displayFont;

	//menu fonts
	string menuFontNameSmallPrefix= config.getString("FontMenuNormalPrefix");
	string menuFontNameSmallPostfix= config.getString("FontMenuNormalPostfix");
	int menuFontNameSmallSize=computeFontSize(12);
	string menuFontNameSmall=menuFontNameSmallPrefix+intToStr(menuFontNameSmallSize)+menuFontNameSmallPostfix;
	menuFontSmall= renderer.newFont(rsGlobal);
	menuFontSmall->setType(menuFontNameSmall);
	menuFontSmall->setSize(menuFontNameSmallSize);
	
	
	string menuFontNameNormalPrefix= config.getString("FontMenuNormalPrefix");
	string menuFontNameNormalPostfix= config.getString("FontMenuNormalPostfix");
	int menuFontNameNormalSize=computeFontSize(13);
	string menuFontNameNormal= menuFontNameNormalPrefix+intToStr(menuFontNameNormalSize)+menuFontNameNormalPostfix;
	menuFontNormal= renderer.newFont(rsGlobal);
	menuFontNormal->setType(menuFontNameNormal);
	menuFontNormal->setSize(menuFontNameNormalSize);
	menuFontNormal->setWidth(Font::wBold);


	string menuFontNameBigPrefix= config.getString("FontMenuBigPrefix");
	string menuFontNameBigPostfix= config.getString("FontMenuBigPostfix");
	int menuFontNameBigSize=computeFontSize(20);
	string menuFontNameBig= menuFontNameBigPrefix+intToStr(menuFontNameBigSize)+menuFontNameBigPostfix;
	menuFontBig= renderer.newFont(rsGlobal);
	menuFontBig->setType(menuFontNameBig);
	menuFontBig->setSize(menuFontNameBigSize);

	string menuFontNameVeryBigPrefix= config.getString("FontMenuBigPrefix");
	string menuFontNameVeryBigPostfix= config.getString("FontMenuBigPostfix");
	int menuFontNameVeryBigSize=computeFontSize(25);
	string menuFontNameVeryBig= menuFontNameVeryBigPrefix+intToStr(menuFontNameVeryBigSize)+menuFontNameVeryBigPostfix;
	menuFontVeryBig= renderer.newFont(rsGlobal);
	menuFontVeryBig->setType(menuFontNameVeryBig);
	menuFontVeryBig->setSize(menuFontNameVeryBigSize);

	//console font
	string consoleFontNamePrefix= config.getString("FontConsolePrefix");
	string consoleFontNamePostfix= config.getString("FontConsolePostfix");
	int consoleFontNameSize=computeFontSize(16);
	string consoleFontName= consoleFontNamePrefix+intToStr(consoleFontNameSize)+consoleFontNamePostfix;
	consoleFont= renderer.newFont(rsGlobal);
	consoleFont->setType(consoleFontName);
	consoleFont->setSize(consoleFontNameSize);

	//sounds
    clickSoundA.load(dir+"/menu/sound/click_a.wav");
    clickSoundB.load(dir+"/menu/sound/click_b.wav");
    clickSoundC.load(dir+"/menu/sound/click_c.wav");
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
	int screenH= Config::getInstance().getInt("ScreenHeight");
	int rs= size*screenH/1024;
	if(rs<12){
		rs= 12;
	}
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] fontsize orginal %d      calculated:%d   \n",__FILE__,__FUNCTION__,__LINE__,size,rs);
	return rs;
}

// ================== PRIVATE ========================

}}//end namespace
