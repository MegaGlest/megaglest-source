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

#ifndef _GLEST_GAME_MENUSTATEOPTIONS_H_
#define _GLEST_GAME_MENUSTATEOPTIONS_H_

#include "main_menu.h"
#include "leak_dumper.h"

namespace Glest{ namespace Game{

// ===============================
// 	class MenuStateOptions
// ===============================

class MenuStateOptions: public MenuState{
private:

	GraphicButton buttonOk;
	GraphicButton buttonAbort;
	GraphicButton buttonAutoConfig;
	GraphicButton buttonVideoInfo;
	GraphicButton buttonKeyboardSetup; // configure the keyboard

	GraphicLabel labelLang;
	GraphicLabel labelShadows;
	GraphicLabel labelFilter;
	GraphicLabel labelTextures3D;
	GraphicLabel labelLights;
	GraphicLabel labelUnitParticles;
	GraphicLabel labelTilesetParticles;
	GraphicLabel labelSoundFactory;
	GraphicLabel labelVolumeFx;
	GraphicLabel labelVolumeAmbient;
	GraphicLabel labelVolumeMusic;
	GraphicListBox listBoxLang;
	GraphicListBox listBoxShadows;
	GraphicListBox listBoxFilter;
	GraphicCheckBox checkBoxTextures3D;
	GraphicListBox listBoxLights;
	GraphicCheckBox checkBoxUnitParticles;
	GraphicCheckBox checkBoxTilesetParticles;
	GraphicListBox listBoxSoundFactory;
	GraphicListBox listBoxVolumeFx;
	GraphicListBox listBoxVolumeAmbient;
	GraphicListBox listBoxVolumeMusic;
	GraphicLabel labelPlayerName;
	GraphicLabel labelPlayerNameLabel;
	GraphicLabel *activeInputLabel;
	GraphicLabel labelExternalPort;
	GraphicLabel labelServerPortLabel;


	GraphicLabel labelScreenModes;
	GraphicListBox listBoxScreenModes;
	vector<ModeInfo> modeInfos;

	GraphicLabel labelFullscreenWindowed;
	GraphicCheckBox checkBoxFullscreenWindowed;

	GraphicLabel labelVideoSection;
	GraphicLabel labelAudioSection;
	GraphicLabel labelMiscSection;
	GraphicLabel labelNetworkSettings;

	GraphicLabel labelFontSizeAdjustment;
	GraphicListBox listFontSizeAdjustment;

	GraphicLabel labelMapPreview;
	GraphicCheckBox checkBoxMapPreview;

	GraphicMessageBox mainMessageBox;
	int mainMessageBoxState;

	GraphicLabel labelPublishServerExternalPort;
	GraphicListBox listBoxServerPort;


	GraphicLabel labelEnableFTP;
	GraphicCheckBox checkBoxEnableFTP;

	GraphicLabel labelEnableFTPServer;
	GraphicCheckBox checkBoxEnableFTPServer;

	GraphicLabel labelFTPServerPortLabel;
	GraphicLabel labelFTPServerPort;

	GraphicLabel labelFTPServerDataPortsLabel;
	GraphicLabel labelFTPServerDataPorts;

	GraphicLabel labelEnableFTPServerInternetTilesetXfer;
	GraphicCheckBox checkBoxEnableFTPServerInternetTilesetXfer;

	GraphicLabel labelEnableFTPServerInternetTechtreeXfer;
	GraphicCheckBox checkBoxEnableFTPServerInternetTechtreeXfer;

	GraphicLabel labelEnablePrivacy;
	GraphicCheckBox checkBoxEnablePrivacy;

	GraphicLabel labelEnableTextureCompression;
	GraphicCheckBox checkBoxEnableTextureCompression;

	GraphicLabel labelScreenShotType;
	GraphicListBox listBoxScreenShotType;

	GraphicLabel labelDisableScreenshotConsoleText;
	GraphicCheckBox checkBoxDisableScreenshotConsoleText;

	GraphicLabel labelMouseMoveScrollsWorld;
	GraphicCheckBox checkBoxMouseMoveScrollsWorld;

	GraphicLabel labelVisibleHud;
	GraphicCheckBox checkBoxVisibleHud;
	GraphicLabel labelTimeDisplay;
	GraphicCheckBox checkBoxTimeDisplay;
	GraphicLabel labelChatStaysActive;
	GraphicCheckBox checkBoxChatStaysActive;

	GraphicLabel labelRainEffect;
	GraphicLabel labelRainEffectSeparator;
	GraphicCheckBox checkBoxRainEffect;
	GraphicCheckBox checkBoxRainEffectMenu;

	GraphicLabel labelGammaCorrection;
	GraphicListBox listBoxGammaCorrection;

	GraphicLabel labelShadowTextureSize;
	GraphicListBox listBoxShadowTextureSize;

	GraphicLabel labelVideos;
	GraphicCheckBox checkBoxVideos;

	GraphicLabel labelLuaDisableSecuritySandbox;
	GraphicCheckBox checkBoxLuaDisableSecuritySandbox;

	GraphicMessageBox luaMessageBox;
	int luaMessageBoxState;

	map<string,string> languageList;

	GraphicLabel labelCustomTranslation;
	GraphicCheckBox checkBoxCustomTranslation;

	GraphicButton buttonGetNewLanguageFiles;
	GraphicButton buttonDeleteNewLanguageFiles;
	GraphicLabel labelTransifexUserLabel;
	GraphicLabel labelTransifexUser;
	GraphicLabel labelTransifexPwdLabel;
	GraphicLabel labelTransifexPwd;
	GraphicLabel labelTransifexI18NLabel;
	GraphicLabel labelTransifexI18N;

public:
	MenuStateOptions(Program *program, MainMenu *mainMenu);

	void mouseClick(int x, int y, MouseButton mouseButton);
	void mouseMove(int x, int y, const MouseState *mouseState);
	void render();
	virtual void keyDown(SDL_KeyboardEvent key);
    virtual void keyPress(SDL_KeyboardEvent c);
    virtual bool isInSpecialKeyCaptureEvent();

    virtual void reloadUI();


private:
	void saveConfig();
	void setActiveInputLable(GraphicLabel* newLable);
	void showMessageBox(const string &text, const string &header, bool toggle);
	void showLuaMessageBox(const string &text, const string &header, bool toggle);

	void setupTransifexUI();
};

}}//end namespace

#endif
