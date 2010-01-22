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

#ifndef _GLEST_GAME_COMMANDER_H_
#define _GLEST_GAME_COMMANDER_H_

#include <vector>

#include "vec.h"
#include "selection.h"
#include "command_type.h"

using std::vector;

namespace Glest{ namespace Game{

using Shared::Graphics::Vec2i;

class World;
class Unit;
class Command;
class CommandType;
class NetworkCommand;

// =====================================================
// 	class Commander
//
///	Gives commands to the units 
// =====================================================

class Commander{
private:
	typedef vector<CommandResult> CommandResultContainer;

private:
    World *world;

public:
    void init(World *world); 
	void updateNetwork();
    
	CommandResult tryGiveCommand(const Unit* unit, const CommandType *commandType, const Vec2i &pos, const UnitType* unitType) const;
	CommandResult tryGiveCommand(const Selection *selection, CommandClass commandClass, const Vec2i &pos= Vec2i(0), const Unit *targetUnit= NULL) const; 
    CommandResult tryGiveCommand(const Selection *selection, const CommandType *commandType, const Vec2i &pos= Vec2i(0), const Unit *targetUnit= NULL) const;
    CommandResult tryGiveCommand(const Selection *selection, const Vec2i &pos, const Unit *targetUnit= NULL) const;
	CommandResult tryCancelCommand(const Selection *selection) const;
	void trySetMeetingPoint(const Unit* unit, const Vec2i &pos) const;
	CommandResult pushNetworkCommand(const NetworkCommand* networkCommand) const;
	
private: 
    Vec2i computeRefPos(const Selection *selection) const;
    Vec2i computeDestPos(const Vec2i &refUnitPos, const Vec2i &unitPos, const Vec2i &commandPos) const;
    CommandResult computeResult(const CommandResultContainer &results) const;
	void giveNetworkCommand(const NetworkCommand* networkCommand) const;
	Command* buildCommand(const NetworkCommand* networkCommand) const;
};

}} //end namespace

#endif
