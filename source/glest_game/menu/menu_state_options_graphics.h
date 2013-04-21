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

#ifndef _GLEST_GAME_MENUSTATEOPTIONS_GRAPHICS_H_
#define _GLEST_GAME_MENUSTATEOPTIONS_GRAPHICS_H_

#include "main_menu.h"
#include "leak_dumper.h"

namespace Glest{ namespace Game{

// ===============================
// 	class MenuStateOptionsGraphics
// ===============================

class MenuStateOptionsGraphics: public MenuState{
private:

	GraphicButton buttonOk;
	GraphicButton buttonReturn;
	GraphicButton buttonAutoConfig;
	GraphicButton buttonVideoInfo;

	GraphicButton buttonKeyboardSetup; // configure the keyboard
	GraphicButton buttonVideoSection;
	GraphicButton buttonAudioSection;
	GraphicButton buttonMiscSection;
	GraphicButton buttonNetworkSettings;

	GraphicLabel labelShadows;
	GraphicListBox listBoxShadows;
	GraphicLabel labelFilter;
	GraphicListBox listBoxFilter;
	GraphicLabel labelTextures3D;
	GraphicCheckBox checkBoxTextures3D;
	GraphicLabel labelLights;
	GraphicListBox listBoxLights;
	GraphicLabel labelUnitParticles;
	GraphicCheckBox checkBoxUnitParticles;
	GraphicLabel labelTilesetParticles;
	GraphicCheckBox checkBoxTilesetParticles;

	GraphicLabel labelScreenModes;
	GraphicListBox listBoxScreenModes;
	vector<ModeInfo> modeInfos;

	GraphicLabel labelFullscreenWindowed;
	GraphicCheckBox checkBoxFullscreenWindowed;


	GraphicLabel labelMapPreview;
	GraphicCheckBox checkBoxMapPreview;

	GraphicMessageBox mainMessageBox;
	int mainMessageBoxState;

	GraphicLabel labelEnableTextureCompression;
	GraphicCheckBox checkBoxEnableTextureCompression;

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

	GraphicLabel labelSelectionType;
	GraphicListBox listBoxSelectionType;

	ProgramState **parentUI;

public:
	MenuStateOptionsGraphics(Program *program, MainMenu *mainMenu, ProgramState **parentUI=NULL);

	void mouseClick(int x, int y, MouseButton mouseButton);
	void mouseMove(int x, int y, const MouseState *mouseState);
	void render();
	//virtual void keyDown(SDL_KeyboardEvent key);
    virtual void keyPress(SDL_KeyboardEvent c);
    //virtual bool isInSpecialKeyCaptureEvent();

    virtual void reloadUI();


private:
	void saveConfig();
	void setActiveInputLable(GraphicLabel* newLable);
	void showMessageBox(const string &text, const string &header, bool toggle);

	void setupTransifexUI();
};

}}//end namespace

#endif
