// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_MENUSTATESCENARIO_H_
#define _GLEST_GAME_MENUSTATESCENARIO_H_

#include "main_menu.h"

namespace Glest{ namespace Game{

// ===============================
// 	class MenuStateScenario
// ===============================

class MenuStateScenario: public MenuState{
private:
    enum Difficulty{
        dVeryEasy,
        dEasy,
        dMedium,
        dHard,
        dVeryHard,
        dInsane
    };

	GraphicButton buttonReturn;
	GraphicButton buttonPlayNow;

	GraphicLabel labelInfo;
	GraphicLabel labelScenario;
	GraphicListBox listBoxScenario;
    
	vector<string> scenarioFiles;

    ScenarioInfo scenarioInfo;
	string dir;

public:
	MenuStateScenario(Program *program, MainMenu *mainMenu, const string &dir);

    void mouseClick(int x, int y, MouseButton mouseButton);
	void mouseMove(int x, int y, const MouseState *mouseState);
	void render();
	void update();

	void launchGame();
	void setScenario(int i);
	int getScenarioCount() const	{ return listBoxScenario.getItemCount(); }
	
private:
    void loadScenarioInfo(string file, ScenarioInfo *scenarioInfo);
    void loadGameSettings(const ScenarioInfo *scenarioInfo, GameSettings *gameSettings);
	Difficulty computeDifficulty(const ScenarioInfo *scenarioInfo);
    ControlType strToControllerType(const string &str);

};


}}//end namespace

#endif
