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

#ifndef _GLEST_GAME_UNITUPDATER_H_
#define _GLEST_GAME_UNITUPDATER_H_

#include "gui.h"
#include "particle.h"
#include "randomgen.h"
#include "command.h"
#include "leak_dumper.h"

using Shared::Graphics::ParticleObserver;
using Shared::Util::RandomGen;

namespace Glest{ namespace Game{

class Unit;
class Map;
class ScriptManager;
class PathFinder;
class RoutePlanner;

// =====================================================
//	class UnitUpdater
//
///	Updates all units in the game, even the player
///	controlled units, performs basic actions only
///	such as responding to an attack
// =====================================================

class ParticleDamager;
class Cell;

class UnitRangeCellsLookupItem {
public:

	int UnitRangeCellsLookupItemCacheTimerCountIndex;
	std::vector<Cell *> rangeCellList;

	static time_t lastDebug;
};

class AttackWarningData {
public:
	Vec2f attackPosition;
	int lastFrameCount;
};

class UnitUpdater {
private:
	friend class ParticleDamager;
	typedef vector<AttackWarningData*> AttackWarnings;

private:
	static const int maxResSearchRadius= 10;
	static const int harvestDistance= 5;
	static const int ultraResourceFactor= 3;
	static const int megaResourceFactor= 4;

private:
	const GameCamera *gameCamera;
	Gui *gui;
	Map *map;
	World *world;
	Console *console;
	ScriptManager *scriptManager;
	PathFinder *pathFinder;
	RoutePlanner *routePlanner;
	Game *game;
	RandomGen random;
	float attackWarnRange;
	AttackWarnings attackWarnings;

	std::map<Vec2i, std::map<int, std::map<int, UnitRangeCellsLookupItem > > > UnitRangeCellsLookupItemCache;
	//std::map<int,ExploredCellsLookupKey> ExploredCellsLookupItemCacheTimer;
	int UnitRangeCellsLookupItemCacheTimerCount;

	bool findCachedCellsEnemies(Vec2i center, int range,
								int size, vector<Unit*> &enemies,
								const AttackSkillType *ast, const Unit *unit,
								const Unit *commandTarget);
	void findEnemiesForCell(const AttackSkillType *ast, Cell *cell, const Unit *unit,
							const Unit *commandTarget,vector<Unit*> &enemies);

public:
	UnitUpdater();
    void init(Game *game);
    ~UnitUpdater();

	//update skills
    void updateUnit(Unit *unit);

    //update commands
    void updateUnitCommand(Unit *unit);
    void updateStop(Unit *unit);
    void updateMove(Unit *unit);
    void updateAttack(Unit *unit);
    void updateAttackStopped(Unit *unit);
    void updateBuild(Unit *unit);
    void updateHarvest(Unit *unit);
    void updateRepair(Unit *unit);
    void updateProduce(Unit *unit);
    void updateUpgrade(Unit *unit);
	void updateMorph(Unit *unit);

private:
    //attack
    void hit(Unit *attacker);
	void hit(Unit *attacker, const AttackSkillType* ast, const Vec2i &targetPos, Field targetField);
	void damage(Unit *attacker, const AttackSkillType* ast, Unit *attacked, float distance);
	void startAttackParticleSystem(Unit *unit);

	//misc
    bool searchForResource(Unit *unit, const HarvestCommandType *hct);
    bool attackerOnSight(const Unit *unit, Unit **enemyPtr);
    bool attackableOnSight(const Unit *unit, Unit **enemyPtr, const AttackSkillType *ast);
    bool attackableOnRange(const Unit *unit, Unit **enemyPtr, const AttackSkillType *ast);
	bool unitOnRange(const Unit *unit, int range, Unit **enemyPtr, const AttackSkillType *ast);
	void enemiesAtDistance(const Unit *unit, const Unit *priorityUnit, int distance, vector<Unit*> &enemies);

	Unit * findPeerUnitBuilder(Unit *unit);
	void SwapActiveCommand(Unit *unitSrc, Unit *unitDest);
	void SwapActiveCommandState(Unit *unit, CommandStateType commandStateType,
								const CommandType *commandType,
								int originalValue,int newValue);

};

// =====================================================
//	class ParticleDamager
// =====================================================

class ParticleDamager: public ParticleObserver{
public:
	UnitReference attackerRef;
	const AttackSkillType* ast;
	UnitUpdater *unitUpdater;
	const GameCamera *gameCamera;
	Vec2i targetPos;
	Field targetField;

public:
	ParticleDamager(Unit *attacker, UnitUpdater *unitUpdater, const GameCamera *gameCamera);
	virtual void update(ParticleSystem *particleSystem);
};

}}//end namespace

#endif
