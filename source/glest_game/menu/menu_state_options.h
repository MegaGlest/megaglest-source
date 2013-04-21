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
	GraphicButton buttonReturn;

	GraphicLabel labelLang;
	GraphicListBox listBoxLang;
	GraphicLabel labelPlayerName;
	GraphicLabel labelPlayerNameLabel;
	GraphicLabel *activeInputLabel;


	GraphicButton buttonKeyboardSetup; // configure the keyboard
	GraphicButton buttonVideoSection;
	GraphicButton buttonAudioSection;
	GraphicButton buttonMiscSection;
	GraphicButton buttonNetworkSettings;

	GraphicLabel labelFontSizeAdjustment;
	GraphicListBox listFontSizeAdjustment;


	GraphicMessageBox mainMessageBox;
	int mainMessageBoxState;



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

	ProgramState **parentUI;

public:
	MenuStateOptions(Program *program, MainMenu *mainMenu, ProgramState **parentUI=NULL);

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
