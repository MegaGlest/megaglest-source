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
#include "leak_dumper.h"

namespace Glest{ namespace Game{

// ===============================
// 	class MenuStateScenario
// ===============================

class MenuStateScenario: public MenuState {
private:

	GraphicButton buttonReturn;
	GraphicButton buttonPlayNow;

	GraphicLabel labelInfo;
	GraphicLabel labelScenario;
	GraphicListBox listBoxScenario;

	vector<string> scenarioFiles;

    ScenarioInfo scenarioInfo;
	vector<string> dirList;

	GraphicMessageBox mainMessageBox;
	int mainMessageBoxState;

	string autoloadScenarioName;

	time_t previewLoadDelayTimer;
	bool needToLoadTextures;

	bool enableScenarioTexturePreview;
	Texture2D *scenarioLogoTexture;

public:
	MenuStateScenario(Program *program, MainMenu *mainMenu, const vector<string> &dirList, string autoloadScenarioName="");
	virtual ~MenuStateScenario();

    void mouseClick(int x, int y, MouseButton mouseButton);
	void mouseMove(int x, int y, const MouseState *mouseState);
	void render();
	void update();

	void launchGame();
	void setScenario(int i);
	int getScenarioCount() const	{ return listBoxScenario.getItemCount(); }

	virtual void keyDown(SDL_KeyboardEvent key);

	virtual void reloadUI();

private:

	void loadScenarioInfo(string file, ScenarioInfo *scenarioInfo);
    void loadGameSettings(const ScenarioInfo *scenarioInfo, GameSettings *gameSettings);
    void loadScenarioPreviewTexture();
	Difficulty computeDifficulty(const ScenarioInfo *scenarioInfo);
    void showMessageBox(const string &text, const string &header, bool toggle);

    void cleanupPreviewTexture();
};


}}//end namespace

#endif
