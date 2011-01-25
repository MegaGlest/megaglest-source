// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Marti�o Figueroa
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
	GraphicListBox listBoxSoundFactory;
	GraphicListBox listBoxVolumeFx;
	GraphicListBox listBoxVolumeAmbient;
	GraphicListBox listBoxVolumeMusic;
	GraphicLabel labelPlayerName;
	GraphicLabel labelPlayerNameLabel;
	GraphicLabel *activeInputLabel;
	GraphicLabel labelServerPort;
	GraphicLabel labelServerPortLabel;


	GraphicLabel labelScreenModes;
	GraphicListBox listBoxScreenModes;
	list<ModeInfo> modeInfos;

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
	GraphicListBox listBoxPublishServerExternalPort;


	GraphicLabel labelEnableFTP;
	GraphicCheckBox checkBoxEnableFTP;

	GraphicLabel labelEnableFTPServer;
	GraphicCheckBox checkBoxEnableFTPServer;

	GraphicLabel labelFTPServerPortLabel;
	GraphicLabel labelFTPServerPort;

	GraphicLabel labelFTPServerDataPortsLabel;
	GraphicLabel labelFTPServerDataPorts;

	GraphicLabel labelEnablePrivacy;
	GraphicCheckBox checkBoxEnablePrivacy;

public:
	MenuStateOptions(Program *program, MainMenu *mainMenu);

	void mouseClick(int x, int y, MouseButton mouseButton);
	void mouseMove(int x, int y, const MouseState *mouseState);
	void render();
	virtual void keyDown(char key);
    virtual void keyPress(char c);


private:
	void saveConfig();
	void setActiveInputLable(GraphicLabel* newLable);
	void showMessageBox(const string &text, const string &header, bool toggle);
};

}}//end namespace

#endif
