// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Martio Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_MENUSTATESCENARIO_H_
#define _GLEST_GAME_MENUSTATESCENARIO_H_

#include "main_menu.h"
#include "megaglest_cegui_manager.h"

#include "leak_dumper.h"

namespace Glest{ namespace Game{

// ===============================
// 	class MenuStateScenario
// ===============================

class MenuStateScenario: public MenuState, public MegaGlest_CEGUIManagerBackInterface {
private:

	vector<string> scenarioFiles;

    ScenarioInfo scenarioInfo;
	vector<string> dirList;

	int mainMessageBoxState;

	string autoloadScenarioName;

	time_t previewLoadDelayTimer;
	bool needToLoadTextures;

	bool enableScenarioTexturePreview;
	Texture2D *scenarioLogoTexture;

	bool isTutorialMode;

	typedef void(MenuStateScenario::*DelayCallbackFunction)(void);
	vector<DelayCallbackFunction> delayedCallbackList;

protected:
	virtual bool hasDelayedCallbacks() { return delayedCallbackList.empty() == false; }
	virtual void callDelayedCallbacks();

public:
	MenuStateScenario(Program *program, MainMenu *mainMenu, bool isTutorialMode, const vector<string> &dirList, string autoloadScenarioName="");
	virtual ~MenuStateScenario();

    void mouseClick(int x, int y, MouseButton mouseButton);
	void mouseMove(int x, int y, const MouseState *mouseState);
	void render();
	virtual void update();

	void launchGame();
	void setScenario(int i);
	int getScenarioCount() const { return scenarioFiles.size(); }

	virtual void keyDown(SDL_KeyboardEvent key);

	virtual void reloadUI();

private:

	void loadScenarioInfo(string file, ScenarioInfo *scenarioInfo);
    void loadGameSettings(const ScenarioInfo *scenarioInfo, GameSettings *gameSettings);
    void loadScenarioPreviewTexture();
	Difficulty computeDifficulty(const ScenarioInfo *scenarioInfo);
    void showMessageBox(const string &text, const string &header,bool okOnly = false);

    void cleanupPreviewTexture();

	void delayedCallbackFunctionPlay();
	void delayedCallbackFunctionReturn();

	void setupCEGUIWidgetsText();
	void setupCEGUIWidgets();
	virtual bool EventCallback(CEGUI::Window *ctl, std::string name);

};


}}//end namespace

#endif
