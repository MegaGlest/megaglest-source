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

#ifndef _GLEST_GAME_MENUSTATEOPTIONS_SOUND_H_
#define _GLEST_GAME_MENUSTATEOPTIONS_SOUND_H_

#include "main_menu.h"
#include "leak_dumper.h"

namespace Glest{ namespace Game{

// ===============================
// 	class MenuStateOptionsSound
// ===============================

class MenuStateOptionsSound: public MenuState{
private:

	GraphicButton buttonOk;
	GraphicButton buttonReturn;

	GraphicButton buttonKeyboardSetup; // configure the keyboard
	GraphicButton buttonVideoSection;
	GraphicButton buttonAudioSection;
	GraphicButton buttonMiscSection;
	GraphicButton buttonNetworkSettings;

	GraphicLabel labelSoundFactory;
	GraphicListBox listBoxSoundFactory;

	GraphicLabel labelVolumeFx;
	GraphicListBox listBoxVolumeFx;

	GraphicLabel labelVolumeAmbient;
	GraphicListBox listBoxVolumeAmbient;

	GraphicLabel labelVolumeMusic;
	GraphicListBox listBoxVolumeMusic;

	GraphicMessageBox mainMessageBox;
	int mainMessageBoxState;

	ProgramState **parentUI;

public:
	MenuStateOptionsSound(Program *program, MainMenu *mainMenu, ProgramState **parentUI=NULL);

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
};

}}//end namespace

#endif
