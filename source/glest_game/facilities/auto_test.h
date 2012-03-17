// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2009 Martio Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_UTIL_AUTO_TEST_H_
#define _SHARED_UTIL_AUTO_TEST_H_

#include <ctime>
#include "randomgen.h"
#include <string>
#include "game_settings.h"
#include "leak_dumper.h"

using namespace std;
using Shared::Util::RandomGen;

namespace Glest{ namespace Game{

class Program;
class MainMenu;
class MenuStateScenario;
class Game;

// =====================================================
//	class AutoTest  
//
/// Interface to write log files
// =====================================================

class AutoTest{
private:
	int gameStartTime;
	RandomGen random;
	bool exitGame;
	static bool wantExitGame;

	static GameSettings gameSettings;
	static string loadGameSettingsFile;

	static const time_t invalidTime;
	static time_t gameTime;

public:
	static AutoTest & getInstance();
	AutoTest();

	static void setMaxGameTime(time_t value) { gameTime = value; }
	static void setWantExitGameWhenDone(bool value) { wantExitGame = value; }
	static string getLoadGameSettingsFile() { return loadGameSettingsFile; }
	static void setLoadGameSettingsFile(string filename) { loadGameSettingsFile = filename; }

	bool mustExitGame() const { return exitGame; }

	void updateIntro(Program *program);
	void updateRoot(Program *program, MainMenu *mainMenu);
	void updateNewGame(Program *program, MainMenu *mainMenu);
	void updateScenario(MenuStateScenario *menuStateScenario);
	bool updateGame(Game *game);
	void updateBattleEnd(Program *program);
};

}}//end namespace

#endif
