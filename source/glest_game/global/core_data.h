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

#ifndef _GLEST_GAME_COREDATA_H_
#define _GLEST_GAME_COREDATA_H_

#include <string>

#include "sound.h"
#include "font.h"
#include "texture.h"
#include "sound_container.h"
#include "leak_dumper.h"

namespace Glest{ namespace Game{

using Shared::Graphics::Texture2D;
using Shared::Graphics::Texture3D;
using Shared::Graphics::Font2D;
using Shared::Sound::StrSound;
using Shared::Sound::StaticSound;

// =====================================================
// 	class CoreData  
//
/// Data shared ammont all the ProgramStates
// =====================================================

class CoreData{
private:
    StrSound introMusic;
    StrSound menuMusic;
	StaticSound clickSoundA;
    StaticSound clickSoundB;
    StaticSound clickSoundC;    
    StaticSound attentionSound;
    StaticSound highlightSound;
	SoundContainer waterSounds;
	
	Texture2D *logoTexture;
	std::vector<Texture2D *> logoTextureList;
    Texture2D *backgroundTexture;
    Texture2D *fireTexture;
    Texture2D *snowTexture;
	Texture2D *waterSplashTexture;
    Texture2D *customTexture;
	Texture2D *buttonSmallTexture;
	Texture2D *buttonBigTexture;
	Texture2D *horizontalLineTexture;
	Texture2D *verticalLineTexture;
	Texture2D *checkBoxTexture;
	Texture2D *checkedCheckBoxTexture;

    Font2D *displayFont;
	Font2D *menuFontNormal;
	Font2D *displayFontSmall;
	Font2D *menuFontBig;
	Font2D *menuFontVeryBig;
	Font2D *consoleFont;

public:
	static CoreData &getInstance();
	~CoreData();

    void load();

	Texture2D *getBackgroundTexture() const		{return backgroundTexture;}
	Texture2D *getFireTexture() const			{return fireTexture;}
	Texture2D *getSnowTexture() const			{return snowTexture;}
	Texture2D *getLogoTexture() const			{return logoTexture;}
	Texture2D *getWaterSplashTexture() const	{return waterSplashTexture;}
	Texture2D *getCustomTexture() const			{return customTexture;}
	Texture2D *getButtonSmallTexture() const	{return buttonSmallTexture;}
	Texture2D *getButtonBigTexture() const		{return buttonBigTexture;}
	Texture2D *getHorizontalLineTexture() const	{return horizontalLineTexture;}
	Texture2D *getVerticalLineTexture() const	{return verticalLineTexture;}
	Texture2D *getCheckBoxTexture() const		{return checkBoxTexture;}
	Texture2D *getCheckedCheckBoxTexture() const	{return checkedCheckBoxTexture;}

	size_t getLogoTextureExtraCount() const {return logoTextureList.size();}
	Texture2D *getLogoTextureExtra(int idx) const {return logoTextureList[idx];}

	StrSound *getIntroMusic() 				{return &introMusic;}
	StrSound *getMenuMusic() 				{return &menuMusic;}
    StaticSound *getClickSoundA()			{return &clickSoundA;}
    StaticSound *getClickSoundB()			{return &clickSoundB;}
    StaticSound *getClickSoundC()			{return &clickSoundC;}
    StaticSound *getAttentionSound()		{return &attentionSound;}
    StaticSound *getHighlightSound()		{return &highlightSound;}
	StaticSound *getWaterSound()			{return waterSounds.getRandSound();}

	Font2D *getDisplayFont() const			{return displayFont;}
    Font2D *getDisplayFontSmall() const		{return displayFontSmall;}
    Font2D *getMenuFontNormal() const		{return menuFontNormal;}
    Font2D *getMenuFontBig() const			{return menuFontBig;}
	Font2D *getMenuFontVeryBig() const		{return menuFontVeryBig;}
    Font2D *getConsoleFont() const			{return consoleFont;}

private:
	CoreData(){};

	int computeFontSize(int size);
};

}} //end namespace

#endif
