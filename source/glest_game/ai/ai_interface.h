// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martio Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_AIINTERFACE_H_
#define _GLEST_GAME_AIINTERFACE_H_

#include "world.h"
#include "commander.h"
#include "command.h"
#include "conversion.h"
#include "ai.h"
#include "game_settings.h"
#include <map>
#include "leak_dumper.h"

using Shared::Util::intToStr;

namespace Glest{ namespace Game{

// =====================================================
// 	class AiInterface
//
///	The AI will interact with the game through this interface
// =====================================================

class AiInterface {
private:
    World *world;
    Commander *commander;
    Console *console;
    GameSettings *gameSettings;

    Ai ai;

    int timer;
    int factionIndex;
    int teamIndex;

	//config
	bool redir;
    int logLevel;
    std::map<const ResourceType *,int> cacheUnitHarvestResourceLookup;

public:
    AiInterface(Game &game, int factionIndex, int teamIndex, int useStartLocation=-1);
    ~AiInterface();

	//main
    void update();

	//get
	int getTimer() const		{return timer;}
	int getFactionIndex() const	{return factionIndex;}

    //misc
    void printLog(int logLevel, const string &s);

    //interact
    CommandResult giveCommand(int unitIndex, CommandClass commandClass, const Vec2i &pos=Vec2i(0));
    CommandResult giveCommand(int unitIndex, const CommandType *commandType, const Vec2i &pos, const UnitType* unitType);
    CommandResult giveCommand(int unitIndex, const CommandType *commandType, const Vec2i &pos);
    CommandResult giveCommand(int unitIndex, const CommandType *commandType, Unit *u= NULL);
    CommandResult giveCommand(const Unit *unit, const CommandType *commandType, const Vec2i &pos);

    //get data
    const ControlType getControlType();
    int getMapMaxPlayers();
    Vec2i getHomeLocation();
    Vec2i getStartLocation(int locationIndex);
    int getFactionCount();
    int getMyUnitCount() const;
	int getMyUpgradeCount() const;
    int onSightUnitCount();
    const Resource *getResource(const ResourceType *rt);
    const Unit *getMyUnit(int unitIndex);
    const Unit *getOnSightUnit(int unitIndex);
    const FactionType *getMyFactionType();
    Faction *getMyFaction();
    const TechTree *getTechTree();
    bool isResourceNear(const Vec2i &pos, const ResourceType *rt, Vec2i &resourcePos, Faction *faction, bool fallbackToPeersHarvestingSameResource) const;
    bool getNearestSightedResource(const ResourceType *rt, const Vec2i &pos, Vec2i &resultPos, bool usableResourceTypeOnly);
    bool isAlly(const Unit *unit) const;
	bool isAlly(int factionIndex) const;
	bool reqsOk(const RequirableType *rt);
	bool reqsOk(const CommandType *ct);
    bool checkCosts(const ProducibleType *pt);
	bool isFreeCells(const Vec2i &pos, int size, Field field);
	const Unit *getFirstOnSightEnemyUnit(Vec2i &pos, Field &field, int radius);
	Map * getMap();

private:
	string getLogFilename() const	{return "ai"+intToStr(factionIndex)+".log";}
	bool executeCommandOverNetwork();
};

}}//end namespace

#endif
