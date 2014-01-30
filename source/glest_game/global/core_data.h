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

#ifndef _GLEST_GAME_COREDATA_H_
#define _GLEST_GAME_COREDATA_H_

#ifdef WIN32
    #include <winsock2.h>
    #include <winsock.h>
#endif

#include <string>
#include "sound.h"
#include "font.h"
#include "texture.h"
#include "sound_container.h"
#include "leak_dumper.h"

namespace Glest{ namespace Game{

using ::Shared::Graphics::Texture2D;
using ::Shared::Graphics::Texture3D;
using ::Shared::Graphics::Font2D;
using ::Shared::Graphics::Font3D;
using ::Shared::Sound::StrSound;
using ::Shared::Sound::StaticSound;

// =====================================================
// 	class CoreData  
//
/// Data shared among all the ProgramStates
// =====================================================

class GameSettings;

class CoreData {
private:
	std::map<int,bool> itemLoadAttempted;

    StrSound introMusic;
    StrSound menuMusic;
	StaticSound clickSoundA;
    StaticSound clickSoundB;
    StaticSound clickSoundC;
    StaticSound attentionSound;
    StaticSound highlightSound;
    StaticSound markerSound;
	SoundContainer waterSounds;
	
	Texture2D *logoTexture;
	std::vector<Texture2D *> logoTextureList;
    Texture2D *backgroundTexture;
    Texture2D *fireTexture;
    Texture2D *teamColorTexture;
    Texture2D *snowTexture;
	Texture2D *waterSplashTexture;
    Texture2D *customTexture;
	Texture2D *buttonSmallTexture;
	Texture2D *buttonBigTexture;
	Texture2D *horizontalLineTexture;
	Texture2D *verticalLineTexture;
	Texture2D *checkBoxTexture;
	Texture2D *checkedCheckBoxTexture;
	Texture2D *gameWinnerTexture;
    Texture2D *notOnServerTexture;
    Texture2D *onServerDifferentTexture;
    Texture2D *onServerTexture;
    Texture2D *onServerInstalledTexture;
    Texture2D *statusReadyTexture;
    Texture2D *statusNotReadyTexture;
    Texture2D *statusBRBTexture;

    std::vector<Texture2D *> miscTextureList;

    Font2D *displayFont;
	Font2D *menuFontNormal;
	Font2D *displayFontSmall;
	Font2D *menuFontBig;
	Font2D *menuFontVeryBig;
	Font2D *consoleFont;

    Font3D *displayFont3D;
	Font3D *menuFontNormal3D;
	Font3D *displayFontSmall3D;
	Font3D *menuFontBig3D;
	Font3D *menuFontVeryBig3D;
	Font3D *consoleFont3D;

	string introVideoFilename;
	string introVideoFilenameFallback;

	string mainMenuVideoFilename;
	string mainMenuVideoFilenameFallback;

	string battleEndWinVideoFilename;
	string battleEndWinVideoFilenameFallback;
	string battleEndWinMusicFilename;

	string battleEndLoseVideoFilename;
	string battleEndLoseVideoFilenameFallback;
	string battleEndLoseMusicFilename;

public:

	enum TextureSystemType {
		tsyst_NONE,
		tsyst_logoTexture,
		tsyst_backgroundTexture,
		tsyst_fireTexture,
		tsyst_teamColorTexture,
		tsyst_snowTexture,
		tsyst_waterSplashTexture,
		tsyst_customTexture,
		tsyst_buttonSmallTexture,
		tsyst_buttonBigTexture,
		tsyst_horizontalLineTexture,
		tsyst_verticalLineTexture,
		tsyst_checkBoxTexture,
		tsyst_checkedCheckBoxTexture,
		tsyst_gameWinnerTexture,
		tsyst_notOnServerTexture,
		tsyst_onServerDifferentTexture,
		tsyst_onServerTexture,
		tsyst_onServerInstalledTexture,
		tsyst_statusReadyTexture,
		tsyst_statusNotReadyTexture,
		tsyst_statusBRBTexture,

		tsyst_COUNT
	};

public:

	~CoreData();
	static CoreData &getInstance();

    void load();
    void cleanup();
    void loadFonts();

    // Textures
    Texture2D *getTextureBySystemId(TextureSystemType type) const;

	Texture2D *getBackgroundTexture();
	Texture2D *getFireTexture();
	Texture2D *getTeamColorTexture();
	Texture2D *getSnowTexture();
	Texture2D *getLogoTexture();
	Texture2D *getWaterSplashTexture();
	Texture2D *getCustomTexture();
	Texture2D *getButtonSmallTexture();
	Texture2D *getButtonBigTexture();
	Texture2D *getHorizontalLineTexture();
	Texture2D *getVerticalLineTexture();
	Texture2D *getCheckBoxTexture();
	Texture2D *getCheckedCheckBoxTexture();
	Texture2D *getNotOnServerTexture();
	Texture2D *getOnServerDifferentTexture();
	Texture2D *getOnServerTexture();
	Texture2D *getOnServerInstalledTexture();
    Texture2D *getStatusReadyTexture();
    Texture2D *getStatusNotReadyTexture();
    Texture2D *getStatusBRBTexture();
	Texture2D *getGameWinnerTexture();

	size_t getLogoTextureExtraCount();
	Texture2D *getLogoTextureExtra(int idx);

	std::vector<Texture2D *> & getMiscTextureList();

	// Sounds and Music
	StrSound *getIntroMusic();
	StrSound *getMenuMusic();

    StaticSound *getClickSoundA();
    StaticSound *getClickSoundB();
    StaticSound *getClickSoundC();
    StaticSound *getAttentionSound();
    StaticSound *getHighlightSound();
    StaticSound *getMarkerSound();
	StaticSound *getWaterSound();

	// Fonts
	Font2D *getDisplayFont() const			{return displayFont;}
    Font2D *getDisplayFontSmall() const		{return displayFontSmall;}
    Font2D *getMenuFontNormal() const		{return menuFontNormal;}
    Font2D *getMenuFontBig() const			{return menuFontBig;}
	Font2D *getMenuFontVeryBig() const		{return menuFontVeryBig;}
    Font2D *getConsoleFont() const			{return consoleFont;}

	Font3D *getDisplayFont3D() const			{return displayFont3D;}
    Font3D *getDisplayFontSmall3D() const		{return displayFontSmall3D;}
    Font3D *getMenuFontNormal3D() const			{return menuFontNormal3D;}
    Font3D *getMenuFontBig3D() const			{return menuFontBig3D;}
	Font3D *getMenuFontVeryBig3D() const		{return menuFontVeryBig3D;}
    Font3D *getConsoleFont3D() const			{return consoleFont3D;}

    // Helper functions
    string getMainMenuVideoFilename() const 					{ return mainMenuVideoFilename; }
    bool hasMainMenuVideoFilename() const;
    string getMainMenuVideoFilenameFallback() const 			{ return mainMenuVideoFilenameFallback; }
    bool hasMainMenuVideoFilenameFallback() const;

    string getIntroVideoFilename() const 						{ return introVideoFilename; }
    bool hasIntroVideoFilename() const;
    string getIntroVideoFilenameFallback() const 				{ return introVideoFilenameFallback; }
    bool hasIntroVideoFilenameFallback() const;

    string getBattleEndVideoFilename(bool won) const 			{ return won == true ? battleEndWinVideoFilename : battleEndLoseVideoFilename; }
    bool hasBattleEndVideoFilename(bool won) const;
    string getBattleEndVideoFilenameFallback(bool won) const 	{ return won == true ? battleEndWinVideoFilenameFallback : battleEndLoseVideoFilenameFallback; }
    bool hasBattleEndVideoFilenameFallback(bool won) const;

    string getBattleEndMusicFilename(bool won) const 			{ return won == true ? battleEndWinMusicFilename : battleEndLoseMusicFilename; }

    void saveGameSettingsToFile(std::string fileName, GameSettings *gameSettings,int advancedIndex=0);
    bool loadGameSettingsFromFile(std::string fileName, GameSettings *gameSettings);

private:

    CoreData();

	int computeFontSize(int size);
	void cleanupTexture(Texture2D **texture);
	void loadTextures(string data_path);
	void loadSounds(string data_path);
	void loadMusic(string data_path);
	void loadIntroMedia(string data_path);
	void loadMainMenuMedia(string data_path);
	void loadBattleEndMedia(string data_path);

	string getDataPath();
	void loadTextureIfRequired(Texture2D **tex,string data_path,
						string uniqueFilePath, int texSystemId, bool setMipMap,
						bool setAlpha, bool loadUniqueFilePath,
						bool compressionDisabled = false);

	void loadLogoTextureExtraIfRequired();
	void loadMiscTextureListIfRequired();

	void loadWaterSoundsIfRequired();
	void loadMusicIfRequired();
};

}} //end namespace

#endif
