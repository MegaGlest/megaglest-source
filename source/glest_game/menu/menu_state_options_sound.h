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
#include "megaglest_cegui_manager.h"
#include "leak_dumper.h"

namespace Glest{ namespace Game{

// ===============================
// 	class MenuStateOptionsSound
// ===============================

class MenuStateOptionsSound : public MenuState, public MegaGlest_CEGUIManagerBackInterface {
private:

	ProgramState **parentUI;

	typedef void(MenuStateOptionsSound::*DelayCallbackFunction)(void);
	vector<DelayCallbackFunction> delayedCallbackList;

protected:
	virtual bool hasDelayedCallbacks() { return delayedCallbackList.empty() == false; }
	virtual void callDelayedCallbacks();

public:

	MenuStateOptionsSound(Program *program, MainMenu *mainMenu, ProgramState **parentUI=NULL);

	void mouseClick(int x, int y, MouseButton mouseButton);
	void mouseMove(int x, int y, const MouseState *mouseState);
	void render();

    virtual void keyPress(SDL_KeyboardEvent c);
    virtual void reloadUI();

private:

    void saveConfig();

    void delayedCallbackFunctionSelectKeyboardTab();
    void delayedCallbackFunctionSelectMiscTab();
    void delayedCallbackFunctionSelectNetworkTab();
    void delayedCallbackFunctionSelectVideoTab();
    void delayedCallbackFunctionOk();
    void delayedCallbackFunctionReturn();

    void setupCEGUIWidgets();
    void setupCEGUIWidgetsText();

    virtual bool EventCallback(CEGUI::Window *ctl, std::string name);
};

}}//end namespace

#endif
