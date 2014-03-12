// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2011-  by Titus Tscharntke
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_MENUSTATEKEYSETUP_H_
#define _GLEST_GAME_MENUSTATEKEYSETUP_H_

#include "main_menu.h"
#include "server_line.h"
#include "megaglest_cegui_manager.h"
#include "leak_dumper.h"

namespace Glest{ namespace Game{

// ===============================
// 	class
// ===============================
typedef vector<GraphicButton*> UserButtons;
typedef vector<GraphicLabel*> GraphicLabels;

class MenuStateKeysetup: public MenuState, public MegaGlest_CEGUIManagerBackInterface {

private:

	int mainMessageBoxState;
	vector<pair<string,string> > mergedProperties;
	vector<pair<string,string> > masterProperties;
	vector<pair<string,string> > userProperties;

	SDLKey hotkeyChar;

	ProgramState **parentUI;

	typedef void(MenuStateKeysetup::*DelayCallbackFunction)(void);
	vector<DelayCallbackFunction> delayedCallbackList;

protected:
	virtual bool hasDelayedCallbacks() { return delayedCallbackList.empty() == false; }
	virtual void callDelayedCallbacks();

public:
	MenuStateKeysetup(Program *program, MainMenu *mainMenu, ProgramState **parentUI=NULL);
	virtual ~MenuStateKeysetup();

	void mouseClick(int x, int y, MouseButton mouseButton);
	void mouseUp(int x, int y, const MouseButton mouseButton);
	void mouseMove(int x, int y, const MouseState *mouseState);
	virtual void update();
	void render();

	virtual void keyDown(SDL_KeyboardEvent key);

    virtual void keyUp(SDL_KeyboardEvent key);

	virtual bool isInSpecialKeyCaptureEvent() { return true; }

	void reloadUI();

private:

	void showMessageBox(const string &text, const string &header, bool toggle);

    void delayedCallbackFunctionSelectAudioTab();
    void delayedCallbackFunctionSelectMiscTab();
    void delayedCallbackFunctionSelectNetworkTab();
    void delayedCallbackFunctionSelectVideoTab();
    void delayedCallbackFunctionOk();
    void delayedCallbackFunctionReturn();
    void delayedCallbackFunctionDefaults();

    void setupCEGUIWidgets();
    void setupCEGUIWidgetsText();

    virtual bool EventCallback(CEGUI::Window *ctl, std::string name);

};


}}//end namespace

#endif
