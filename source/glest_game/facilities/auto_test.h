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

#ifndef _SHARED_UTIL_AUTO_TEST_H_
#define _SHARED_UTIL_AUTO_TEST_H_

#include <ctime>

#include "random.h"

using Shared::Util::Random;

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
	Random random;

private:
	static const time_t invalidTime;
	static const time_t gameTime;

public:
	static AutoTest & getInstance();
	AutoTest();

	void updateIntro(Program *program);
	void updateRoot(Program *program, MainMenu *mainMenu);
	void updateNewGame(Program *program, MainMenu *mainMenu);
	void updateScenario(MenuStateScenario *menuStateScenario);
	void updateGame(Game *game);
	void updateBattleEnd(Program *program);
};

}}//end namespace

#endif
