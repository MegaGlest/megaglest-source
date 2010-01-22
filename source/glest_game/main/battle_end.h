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

#ifndef _GLEST_GAME_BATTLEEND_H_
#define _GLEST_GAME_BATTLEEND_H_

#include "program.h"
#include "stats.h"

namespace Glest{ namespace Game{

// =====================================================
// 	class BattleEnd  
//
///	ProgramState representing the end of the game
// =====================================================

class BattleEnd: public ProgramState{
private:
	Stats stats;

public:
	BattleEnd(Program *program, const Stats *stats);
	~BattleEnd();
	virtual void update();
	virtual void render();
	virtual void keyDown(char key);
	virtual void mouseDownLeft(int x, int y);
};

}}//end namespace

#endif
