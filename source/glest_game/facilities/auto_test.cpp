// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2009 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "auto_test.h"

#include "program.h"
#include "main_menu.h"
#include "menu_state_new_game.h"
#include "menu_state_scenario.h"
#include "game.h"
#include "config.h"

#include "leak_dumper.h"


namespace Glest{ namespace Game{

// =====================================================
//	class AutoTest
// =====================================================

const time_t AutoTest::invalidTime = -1;
const time_t AutoTest::gameTime = 60*20;

// ===================== PUBLIC ========================

AutoTest::AutoTest(){
	gameStartTime = invalidTime;
	random.init(time(NULL));
}

AutoTest & AutoTest::getInstance(){
	static AutoTest autoTest;
	return autoTest;
}

void AutoTest::updateIntro(Program *program){
	program->setState(new MainMenu(program));
}

void AutoTest::updateRoot(Program *program, MainMenu *mainMenu){
	mainMenu->setState(new MenuStateNewGame(program, mainMenu));
}

void AutoTest::updateNewGame(Program *program, MainMenu *mainMenu){
	mainMenu->setState(new MenuStateScenario(program, mainMenu, Config::getInstance().getPathListForType(ptScenarios)));
}

void AutoTest::updateScenario(MenuStateScenario *menuStateScenario){
	gameStartTime = invalidTime;

	int scenarioIndex = random.randRange(0, menuStateScenario->getScenarioCount()-1);
	menuStateScenario->setScenario(scenarioIndex);

	menuStateScenario->launchGame();
}

void AutoTest::updateGame(Game *game){

	// record start time
	if(gameStartTime==invalidTime)
	{
		gameStartTime = time(NULL);
	}

	// quit if we've espend enough time in the game
	if(time(NULL)-gameStartTime>gameTime){
		game->quitGame();
	}
}

void AutoTest::updateBattleEnd(Program *program){
	program->setState(new MainMenu(program));
}

}}//end namespace
