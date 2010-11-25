// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "world.h"

#include <algorithm>
#include <cassert>

#include "config.h"
#include "faction.h"
#include "unit.h"
#include "game.h"
#include "logger.h"
#include "sound_renderer.h"
#include "game_settings.h"
#include "cache_manager.h"
#include "route_planner.h"

#include <iostream>

#include "leak_dumper.h"

using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class World
// =====================================================

const float World::airHeight= 5.f;
// This limit is to keep RAM use under control while offering better performance.
int MaxExploredCellsLookupItemCache = 9500;
time_t ExploredCellsLookupItem::lastDebug = 0;

// ===================== PUBLIC ========================

World::World(){
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	Config &config= Config::getInstance();

	staggeredFactionUpdates = config.getBool("StaggeredFactionUpdates","false");

	ExploredCellsLookupItemCache.clear();
	ExploredCellsLookupItemCacheTimer.clear();
	ExploredCellsLookupItemCacheTimerCount = 0;
	FowAlphaCellsLookupItemCache.clear();

	techTree = NULL;
	fogOfWarOverride = false;

	fogOfWarSmoothing= config.getBool("FogOfWarSmoothing");
	fogOfWarSmoothingFrameSkip= config.getInt("FogOfWarSmoothingFrameSkip");

	MaxExploredCellsLookupItemCache= config.getInt("MaxExploredCellsLookupItemCache",intToStr(MaxExploredCellsLookupItemCache).c_str());

	routePlanner = 0;
	cartographer = 0;

	frameCount= 0;
	//nextUnitId= 0;

	scriptManager= NULL;
	this->game = NULL;

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

World::~World() {
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	ExploredCellsLookupItemCache.clear();
	ExploredCellsLookupItemCacheTimer.clear();
	FowAlphaCellsLookupItemCache.clear();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	for(int i= 0; i<factions.size(); ++i){
		factions[i].end();
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	factions.clear();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	delete techTree;
	techTree = NULL;

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	delete routePlanner;
	routePlanner = 0;

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	delete cartographer;
	cartographer = 0;
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void World::end(){
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
    Logger::getInstance().add("World", true);

    ExploredCellsLookupItemCache.clear();
    ExploredCellsLookupItemCacheTimer.clear();
    FowAlphaCellsLookupItemCache.clear();

	for(int i= 0; i<factions.size(); ++i){
		factions[i].end();
	}
	factions.clear();
	fogOfWarOverride = false;
	//stats will be deleted by BattleEnd
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

// ========================== init ===============================================

void World::setFogOfWar(bool value) {
	fogOfWar 		 = value;
	fogOfWarOverride = true;

	if(game != NULL && game->getGameSettings() != NULL) {
		game->getGameSettings()->setFogOfWar(fogOfWar);
		initCells(fogOfWar); //must be done after knowing faction number and dimensions
		//initMinimap();
		minimap.setFogOfWar(fogOfWar);
		computeFow();
	}
}

void World::init(Game *game, bool createUnits){

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	ExploredCellsLookupItemCache.clear();
	ExploredCellsLookupItemCacheTimer.clear();
	FowAlphaCellsLookupItemCache.clear();

	this->game = game;
	scriptManager= game->getScriptManager();

	GameSettings *gs = game->getGameSettings();
	if(fogOfWarOverride == false) {
		fogOfWar = gs->getFogOfWar();
	}
	
	initFactionTypes(gs);
	initCells(fogOfWar); //must be done after knowing faction number and dimensions
	initMap();
	initSplattedTextures();

	// must be done after initMap()
	if(gs->getPathFinderType() != pfBasic) {
		routePlanner = new RoutePlanner(this);
		cartographer = new Cartographer(this);
	}

	unitUpdater.init(game);

	//minimap must be init after sum computation
	initMinimap();
	if(createUnits){
		initUnits();
	}
	//initExplorationState(); ... was only for !fog-of-war, now handled in initCells()
	computeFow();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

//load tileset
void World::loadTileset(const vector<string> pathList, const string &tilesetName, Checksum* checksum) {
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	tileset.loadTileset(pathList, tilesetName, checksum);
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	timeFlow.init(&tileset);
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void World::loadTileset(const string &dir, Checksum *checksum){
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	tileset.load(dir, checksum);
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	timeFlow.init(&tileset);
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

//load tech
void World::loadTech(const vector<string> pathList, const string &techName, set<string> &factions, Checksum *checksum){
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	/*
	std::map<string,TechTree *> &techCache = Shared::PlatformCommon::CacheManager::getCachedItem< std::map<string,TechTree *> >("techCache");
	if(techCache.find(techName) != techCache.end()) {
		techTree = new TechTree();
		*techTree =	*techCache[techName];
		return;
	}
	*/

	techTree = new TechTree();
	techTree->loadTech(pathList, techName, factions, checksum);

	//techCache[techName] = techTree;
}

std::vector<std::string> World::validateFactionTypes() {
	return techTree->validateFactionTypes();
}

std::vector<std::string> World::validateResourceTypes() {
	return techTree->validateResourceTypes();
}

//load map
void World::loadMap(const string &path, Checksum *checksum){
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	checksum->addFile(path);
	map.load(path, techTree, &tileset);
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

//load map
void World::loadScenario(const string &path, Checksum *checksum){
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	checksum->addFile(path);
	scenario.load(path);
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

// ==================== misc ====================

void World::updateAllFactionUnits() {
	scriptManager->onTimerTriggerEvent();
	//units
	int factionCount = getFactionCount();
	for(int i = 0; i < factionCount; ++i) {
		Faction *faction = getFaction(i);
		if(faction == NULL) {
			throw runtime_error("faction == NULL");
		}

		int unitCount = faction->getUnitCount();

		for(int j = 0; j < unitCount; ++j) {
			Unit *unit = faction->getUnit(j);
			if(unit == NULL) {
				throw runtime_error("unit == NULL");
			}

			unitUpdater.updateUnit(unit);
		}
	}
}

void World::underTakeDeadFactionUnits() {
	int factionIdxToTick = -1;
	if(staggeredFactionUpdates == true) {
		factionIdxToTick = tickFactionIndex();
		if(factionIdxToTick < 0) {
			return;
		}
	}

	int factionCount = getFactionCount();
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] factionIdxToTick = %d, factionCount = %d\n",__FILE__,__FUNCTION__,__LINE__,factionIdxToTick,factionCount);

	//undertake the dead
	for(int i = 0; i< factionCount; ++i) {
		if(factionIdxToTick == -1 || factionIdxToTick == i) {
			Faction *faction = getFaction(i);
			if(faction == NULL) {
				throw runtime_error("faction == NULL");
			}
			int unitCount = faction->getUnitCount();
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] factionIdxToTick = %d, i = %d, unitCount = %d\n",__FILE__,__FUNCTION__,__LINE__,factionIdxToTick,i,unitCount);

			for(int j= unitCount - 1; j >= 0; j--) {
				Unit *unit= faction->getUnit(j);

				if(unit == NULL) {
					throw runtime_error("unit == NULL");
				}

				if(unit->getToBeUndertaken() == true) {
					unit->undertake();
					delete unit;
					//j--;
				}
			}
		}
	}
}

void World::updateAllFactionConsumableCosts() {
	//food costs
	int resourceTypeCount = techTree->getResourceTypeCount();
	int factionCount = getFactionCount();
	for(int i = 0; i < resourceTypeCount; ++i) {
		const ResourceType *rt = techTree->getResourceType(i);
		if(rt != NULL && rt->getClass() == rcConsumable && frameCount % (rt->getInterval() * GameConstants::updateFps) == 0) {
			for(int j = 0; j < factionCount; ++j) {
				Faction *faction = getFaction(j);
				if(faction == NULL) {
					throw runtime_error("faction == NULL");
				}

				faction->applyCostsOnInterval(rt);
			}
		}
	}
}

void World::update(){
	++frameCount;

	//time
	timeFlow.update();

	//water effects
	waterEffects.update();

	//bool needToUpdateUnits = true;
	//if(staggeredFactionUpdates == true) {
	//	needToUpdateUnits = (frameCount % (GameConstants::updateFps / GameConstants::maxPlayers) == 0);
	//}

	//if(needToUpdateUnits == true) {
	//	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] needToUpdateUnits = %d, frameCount = %d\n",__FILE__,__FUNCTION__,__LINE__,needToUpdateUnits,frameCount);

	//units
	updateAllFactionUnits();

	//undertake the dead
	underTakeDeadFactionUnits();
	//}

	//food costs
	updateAllFactionConsumableCosts();

	//fow smoothing
	if(fogOfWarSmoothing && ((frameCount+1) % (fogOfWarSmoothingFrameSkip+1))==0) {
		float fogFactor= static_cast<float>(frameCount % GameConstants::updateFps) / GameConstants::updateFps;
		minimap.updateFowTex(clamp(fogFactor, 0.f, 1.f));
	}

	//tick
	bool needToTick = canTickWorld();
	if(needToTick == true) {
		tick();
	}
}

bool World::canTickWorld() const {
	//tick
	bool needToTick = (frameCount % GameConstants::updateFps == 0);
	if(staggeredFactionUpdates == true) {
		needToTick = (frameCount % (GameConstants::updateFps / GameConstants::maxPlayers) == 0);
	}

	return needToTick;
}

int World::getUpdateFps(int factionIndex) const {
	int result = GameConstants::updateFps;
	//if(factionIndex != -1 && staggeredFactionUpdates == true) {
	//	result = (GameConstants::updateFps / GameConstants::maxPlayers);
	//}
	return result;
}

bool World::canTickFaction(int factionIdx) {
	int factionUpdateInterval = (GameConstants::updateFps / GameConstants::maxPlayers);
	int expectedFactionIdx = (frameCount / factionUpdateInterval) -1;
	if(expectedFactionIdx >= GameConstants::maxPlayers) {
		expectedFactionIdx = expectedFactionIdx % GameConstants::maxPlayers;
	}
	bool result = (expectedFactionIdx == factionIdx);

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] factionIdx = %d, frameCount = %d, GameConstants::updateFps = %d, expectedFactionIdx = %d, factionUpdateInterval = %d, result = %d\n",__FILE__,__FUNCTION__,__LINE__,factionIdx,frameCount,GameConstants::updateFps,expectedFactionIdx,factionUpdateInterval,result);

	return result;
}

int World::tickFactionIndex() {
	int factionIdxToTick = -1;
	for(int i=0; i<getFactionCount(); ++i) {
		if(canTickFaction(i) == true) {
			factionIdxToTick = i;
			break;
		}
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] factionIdxToTick = %d\n",__FILE__,__FUNCTION__,__LINE__,factionIdxToTick);

	return factionIdxToTick;
}

void World::tick() {
	int factionIdxToTick = -1;
	if(staggeredFactionUpdates == true) {
		factionIdxToTick = tickFactionIndex();
		if(factionIdxToTick < 0) {
			return;
		}
	}
	computeFow(factionIdxToTick);

	if(factionIdxToTick == -1 || factionIdxToTick == 0) {
		if(fogOfWarSmoothing == false) {
			minimap.updateFowTex(1.f);
		}
	}

	//increase hp
	//int i = factionIdxToTick;
	int factionCount = getFactionCount();
	for(int i = 0; i < factionCount; ++i) {
		if(factionIdxToTick == -1 || i == factionIdxToTick) {
			Faction *faction = getFaction(i);
			if(faction == NULL) {
				throw runtime_error("faction == NULL");
			}
			int unitCount = faction->getUnitCount();

			for(int j = 0; j < unitCount; ++j) {
				Unit *unit = faction->getUnit(j);
				if(unit == NULL) {
					throw runtime_error("unit == NULL");
				}

				unit->tick();
			}
		}
	}

	//compute resources balance
	//int k = factionIdxToTick;
	factionCount = getFactionCount();
	for(int k = 0; k < factionCount; ++k) {
		if(factionIdxToTick == -1 || k == factionIdxToTick) {
			Faction *faction= getFaction(k);
			if(faction == NULL) {
				throw runtime_error("faction == NULL");
			}

			//for each resource
			for(int i = 0; i < techTree->getResourceTypeCount(); ++i) {
				const ResourceType *rt= techTree->getResourceType(i);

				//if consumable
				if(rt != NULL && rt->getClass()==rcConsumable) {
					int balance= 0;
					for(int j = 0; j < faction->getUnitCount(); ++j) {

						//if unit operative and has this cost
						const Unit *u=  faction->getUnit(j);
						if(u != NULL && u->isOperative()) {
							const Resource *r= u->getType()->getCost(rt);
							if(r != NULL) {
								balance -= u->getType()->getCost(rt)->getAmount();
							}
						}
					}
					faction->setResourceBalance(rt, balance);
				}
			}
		}
	}

	if(cartographer != NULL) {
		cartographer->tick();
	}
}

Unit* World::findUnitById(int id) const {
	for(int i= 0; i<getFactionCount(); ++i) {
		const Faction* faction= getFaction(i);
		Unit* unit = faction->findUnit(id);
		if(unit != NULL) {
			return unit;
		}
	}
	return NULL;
}

const UnitType* World::findUnitTypeById(const FactionType* factionType, int id) {
	if(factionType == NULL) {
		throw runtime_error("factionType == NULL");
	}
	for(int i= 0; i < factionType->getUnitTypeCount(); ++i) {
		const UnitType *unitType = factionType->getUnitType(i);
		if(unitType != NULL && unitType->getId() == id) {
			return unitType;
		}
	}
	return NULL;
}

//looks for a place for a unit around a start location, returns true if succeded
bool World::placeUnit(const Vec2i &startLoc, int radius, Unit *unit, bool spaciated) {
    if(unit == NULL) {
    	throw runtime_error("unit == NULL");
    }

	bool freeSpace=false;
	int size= unit->getType()->getSize();
	Field currField= unit->getCurrField();

    for(int r = 1; r < radius; r++) {
        for(int i = -r; i < r; ++i) {
            for(int j = -r; j < r; ++j) {
                Vec2i pos= Vec2i(i,j) + startLoc;
				if(spaciated) {
                    const int spacing = 2;
					freeSpace= map.isFreeCells(pos-Vec2i(spacing), size+spacing*2, currField);
				}
				else {
                    freeSpace= map.isFreeCells(pos, size, currField);
				}

                if(freeSpace) {
                    unit->setPos(pos);
					unit->setMeetingPos(pos-Vec2i(1));
                    return true;
                }
            }
        }
    }
    return false;
}

//clears a unit old position from map and places new position
void World::moveUnitCells(Unit *unit) {
	if(unit == NULL) {
    	throw runtime_error("unit == NULL");
    }

	Vec2i newPos= unit->getTargetPos();

	//newPos must be free or the same pos as current
	assert(map.getCell(unit->getPos())->getUnit(unit->getCurrField())==unit || map.isFreeCell(newPos, unit->getCurrField()));

	// Only change cell placement in map if the new position is different
	// from the old one
	if(newPos != unit->getPos()) {
		map.clearUnitCells(unit, unit->getPos());
		map.putUnitCells(unit, newPos);
	}
	// Add resources close by to the faction's cache
	unit->getFaction()->addCloseResourceTargetToCache(newPos);

	//water splash
	if(tileset.getWaterEffects() && unit->getCurrField() == fLand) {
		if(map.getSubmerged(map.getCell(unit->getLastPos()))) {
			int unitSize= unit->getType()->getSize();
			for(int i = 0; i < 3; ++i) {
				waterEffects.addWaterSplash(
					Vec2f(unit->getLastPos().x+(float)unitSize/2.0f+random.randRange(-0.9f, -0.1f), unit->getLastPos().y+random.randRange(-0.9f, -0.1f)+(float)unitSize/2.0f), unit->getType()->getSize());
			}
		}
	}

	scriptManager->onCellTriggerEvent(unit);
}

//returns the nearest unit that can store a type of resource given a position and a faction
Unit *World::nearestStore(const Vec2i &pos, int factionIndex, const ResourceType *rt) {
    float currDist= infinity;
    Unit *currUnit= NULL;

    if(factionIndex >= getFactionCount()) {
    	throw runtime_error("factionIndex >= getFactionCount()");
    }

    for(int i=0; i < getFaction(factionIndex)->getUnitCount(); ++i) {
		Unit *u= getFaction(factionIndex)->getUnit(i);
		if(u != NULL) {
			float tmpDist= u->getPos().dist(pos);
			if(tmpDist < currDist && u->getType()->getStore(rt) > 0 && u->isOperative()) {
				currDist= tmpDist;
				currUnit= u;
			}
		}
    }
    return currUnit;
}

bool World::toRenderUnit(const Unit *unit, const Quad2i &visibleQuad) const {
    if(unit == NULL) {
    	throw runtime_error("unit == NULL");
    }

	//a unit is rendered if it is in a visible cell or is attacking a unit in a visible cell
    return visibleQuad.isInside(unit->getPos()) && toRenderUnit(unit);
}

bool World::toRenderUnit(const Unit *unit) const {
    if(unit == NULL) {
    	throw runtime_error("unit == NULL");
    }

	return
        (map.getSurfaceCell(Map::toSurfCoords(unit->getCenteredPos()))->isVisible(thisTeamIndex) &&
         map.getSurfaceCell(Map::toSurfCoords(unit->getCenteredPos()))->isExplored(thisTeamIndex) ) ||
        (unit->getCurrSkill()->getClass()==scAttack &&
         map.getSurfaceCell(Map::toSurfCoords(unit->getTargetPos()))->isVisible(thisTeamIndex) &&
         map.getSurfaceCell(Map::toSurfCoords(unit->getTargetPos()))->isExplored(thisTeamIndex));
}

void World::createUnit(const string &unitName, int factionIndex, const Vec2i &pos) {
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] unitName [%s] factionIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,unitName.c_str(),factionIndex);

	if(factionIndex < factions.size()) {
		Faction* faction= &factions[factionIndex];

		if(faction->getIndex() != factionIndex) {
			throw runtime_error("faction->getIndex() != factionIndex");
		}

		const FactionType* ft= faction->getType();
		const UnitType* ut= ft->getUnitType(unitName);

		UnitPathInterface *newpath = NULL;
		switch(game->getGameSettings()->getPathFinderType()) {
			case pfBasic:
				newpath = new UnitPathBasic();
				break;
			case pfRoutePlanner:
				newpath = new UnitPath();
				break;
			default:
				throw runtime_error("detected unsupported pathfinder type!");
	    }

		Unit* unit= new Unit(getNextUnitId(faction), newpath, pos, ut, faction, &map, CardinalDir::NORTH);

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] unit created for unit [%s]\n",__FILE__,__FUNCTION__,__LINE__,unit->toString().c_str());

		if(placeUnit(pos, generationArea, unit, true)) {
			unit->create(true);
			unit->born();
			scriptManager->onUnitCreated(unit);
		}
		else {
			throw runtime_error("Unit cant be placed");
		}

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] unit created for unit [%s]\n",__FILE__,__FUNCTION__,__LINE__,unit->toString().c_str());
	}
	else {
		throw runtime_error("Invalid faction index in createUnitAtPosition: " + intToStr(factionIndex));
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void World::giveResource(const string &resourceName, int factionIndex, int amount) {
	if(factionIndex < factions.size()) {
		Faction* faction= &factions[factionIndex];
		const ResourceType* rt= techTree->getResourceType(resourceName);
		faction->incResourceAmount(rt, amount);
	}
	else {
		throw runtime_error("Invalid faction index in giveResource: " + intToStr(factionIndex));
	}
}

void World::givePositionCommand(int unitId, const string &commandName, const Vec2i &pos) {
	Unit* unit= findUnitById(unitId);
	if(unit != NULL) {
		CommandClass cc;

		if(commandName=="move") {
			cc= ccMove;
		}
		else if(commandName=="attack") {
			cc= ccAttack;
		}
		else {
			throw runtime_error("Invalid position commmand: " + commandName);
		}

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] cc = %d Unit [%s]\n",__FILE__,__FUNCTION__,__LINE__,cc,unit->getFullName().c_str());
		unit->giveCommand(new Command( unit->getType()->getFirstCtOfClass(cc), pos ));
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}
	else {
		throw runtime_error("Invalid unitId index in givePositionCommand: " + intToStr(unitId) + " commandName = " + commandName);
	}
}


void World::giveAttackCommand(int unitId, int unitToAttackId) {
	Unit* unit= findUnitById(unitId);
	if(unit != NULL) {
		Unit* targetUnit = findUnitById(unitToAttackId);
		if(targetUnit != NULL) {
			const CommandType *ct = unit->getType()->getFirstAttackCommand(targetUnit->getCurrField());
			if(ct != NULL) {
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] Unit [%s] is attacking [%s]\n",__FILE__,__FUNCTION__,__LINE__,unit->getFullName().c_str(),targetUnit->getFullName().c_str());
				unit->giveCommand(new Command(ct,targetUnit));
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			}
			else {
				throw runtime_error("Invalid ct in giveAttackCommand: " + intToStr(unitId) + " unitToAttackId = " + intToStr(unitToAttackId));
			}
		}
		else {
			throw runtime_error("Invalid unitToAttackId index in giveAttackCommand: " + intToStr(unitId) + " unitToAttackId = " + intToStr(unitToAttackId));
		}
	}
	else {
		throw runtime_error("Invalid unitId index in giveAttackCommand: " + intToStr(unitId) + " unitToAttackId = " + intToStr(unitToAttackId));
	}
}

void World::giveProductionCommand(int unitId, const string &producedName) {
	Unit *unit= findUnitById(unitId);
	if(unit != NULL) {
		const UnitType *ut= unit->getType();

		//Search for a command that can produce the unit
		for(int i= 0; i< ut->getCommandTypeCount(); ++i) {
			const CommandType* ct= ut->getCommandType(i);
			if(ct != NULL && ct->getClass() == ccProduce) {
				const ProduceCommandType *pct= static_cast<const ProduceCommandType*>(ct);
				if(pct != NULL && pct->getProducedUnit()->getName() == producedName) {
					SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

					unit->giveCommand(new Command(pct));

					SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
					break;
				}
			}
		}
	}
	else {
		throw runtime_error("Invalid unitId index in giveProductionCommand: " + intToStr(unitId) + " producedName = " + producedName);
	}
}

void World::giveUpgradeCommand(int unitId, const string &upgradeName) {
	Unit *unit= findUnitById(unitId);
	if(unit != NULL) {
		const UnitType *ut= unit->getType();

		//Search for a command that can produce the unit
		for(int i= 0; i < ut->getCommandTypeCount(); ++i) {
			const CommandType* ct= ut->getCommandType(i);
			if(ct != NULL && ct->getClass() == ccUpgrade) {
				const UpgradeCommandType *uct= static_cast<const UpgradeCommandType*>(ct);
				if(uct != NULL && uct->getProducedUpgrade()->getName() == upgradeName) {
					SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

					unit->giveCommand(new Command(uct));

					SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
					break;
				}
			}
		}
	}
	else {
		throw runtime_error("Invalid unitId index in giveUpgradeCommand: " + intToStr(unitId) + " upgradeName = " + upgradeName);
	}
}


int World::getResourceAmount(const string &resourceName, int factionIndex) {
	if(factionIndex < factions.size()) {
		Faction* faction= &factions[factionIndex];
		const ResourceType* rt= techTree->getResourceType(resourceName);
		return faction->getResource(rt)->getAmount();
	}
	else {
		throw runtime_error("Invalid faction index in giveResource: " + intToStr(factionIndex) + " resourceName = " + resourceName);
	}
}

Vec2i World::getStartLocation(int factionIndex) {
	if(factionIndex < factions.size()) {
		Faction* faction= &factions[factionIndex];
		return map.getStartLocation(faction->getStartLocationIndex());
	}
	else {
		throw runtime_error("Invalid faction index in getStartLocation: " + intToStr(factionIndex));
	}
}

Vec2i World::getUnitPosition(int unitId) {
	Unit* unit= findUnitById(unitId);
	if(unit == NULL) {
		throw runtime_error("Can not find unit to get position unitId = " + intToStr(unitId));
	}
	return unit->getPos();
}

int World::getUnitFactionIndex(int unitId) {
	Unit* unit= findUnitById(unitId);
	if(unit == NULL) {
		throw runtime_error("Can not find Faction unit to get position unitId = " + intToStr(unitId));
	}
	return unit->getFactionIndex();
}

int World::getUnitCount(int factionIndex) {
	if(factionIndex < factions.size()) {
		Faction* faction= &factions[factionIndex];
		int count= 0;

		for(int i= 0; i < faction->getUnitCount(); ++i) {
			const Unit* unit= faction->getUnit(i);
			if(unit != NULL && unit->isAlive()) {
				++count;
			}
		}
		return count;
	}
	else {
		throw runtime_error("Invalid faction index in getUnitCount: " + intToStr(factionIndex));
	}
}

int World::getUnitCountOfType(int factionIndex, const string &typeName) {
	if(factionIndex < factions.size()) {
		Faction* faction= &factions[factionIndex];
		int count= 0;

		for(int i= 0; i< faction->getUnitCount(); ++i) {
			const Unit* unit= faction->getUnit(i);
			if(unit != NULL && unit->isAlive() && unit->getType()->getName() == typeName) {
				++count;
			}
		}
		return count;
	}
	else {
		throw runtime_error("Invalid faction index in getUnitCountOfType: " + intToStr(factionIndex));
	}
}

// ==================== PRIVATE ====================

// ==================== private init ====================

//init basic cell state
void World::initCells(bool fogOfWar) {
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	Logger::getInstance().add("State cells", true);
    for(int i=0; i< map.getSurfaceW(); ++i) {
        for(int j=0; j< map.getSurfaceH(); ++j) {

			SurfaceCell *sc= map.getSurfaceCell(i, j);
			if(sc == NULL) {
				throw runtime_error("sc == NULL");
			}

			sc->setFowTexCoord(Vec2f(
				i/(next2Power(map.getSurfaceW())-1.f),
				j/(next2Power(map.getSurfaceH())-1.f)));

			for (int k = 0; k < GameConstants::maxPlayers; k++) {
				sc->setExplored(k, !fogOfWar);
				sc->setVisible(k, !fogOfWar);
			}
			for (int k = GameConstants::maxPlayers; k < GameConstants::maxPlayers + GameConstants::specialFactions; k++) {
				sc->setExplored(k, true);
				sc->setVisible(k, true);
			}
		}
    }
    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

//init surface textures
void World::initSplattedTextures() {
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	for(int i = 0; i < map.getSurfaceW() - 1; ++i) {
        for(int j = 0; j < map.getSurfaceH() - 1; ++j) {
			Vec2f coord;
			const Texture2D *texture=NULL;
			SurfaceCell *sc00= map.getSurfaceCell(i, j);
			SurfaceCell *sc10= map.getSurfaceCell(i+1, j);
			SurfaceCell *sc01= map.getSurfaceCell(i, j+1);
			SurfaceCell *sc11= map.getSurfaceCell(i+1, j+1);

			if(sc00 == NULL) {
				throw runtime_error("sc00 == NULL");
			}
			if(sc10 == NULL) {
				throw runtime_error("sc10 == NULL");
			}
			if(sc01 == NULL) {
				throw runtime_error("sc01 == NULL");
			}
			if(sc11 == NULL) {
				throw runtime_error("sc11 == NULL");
			}

			tileset.addSurfTex( sc00->getSurfaceType(),
								sc10->getSurfaceType(),
								sc01->getSurfaceType(),
								sc11->getSurfaceType(),
								coord, texture);
			sc00->setSurfTexCoord(coord);
			sc00->setSurfaceTexture(texture);
		}
	}
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

//creates each faction looking at each faction name contained in GameSettings
void World::initFactionTypes(GameSettings *gs) {
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	Logger::getInstance().add("Faction types", true);

	if(gs == NULL) {
		throw runtime_error("gs == NULL");
	}

	if(gs->getFactionCount() > map.getMaxPlayers()) {
		throw runtime_error("This map only supports "+intToStr(map.getMaxPlayers())+" players");
	}

	//create stats
	stats.init(gs->getFactionCount(), gs->getThisFactionIndex(), gs->getDescription());

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//create factions
	this->thisFactionIndex= gs->getThisFactionIndex();
	factions.resize(gs->getFactionCount());

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] factions.size() = %d\n",__FILE__,__FUNCTION__,__LINE__,factions.size());

	for(int i=0; i < factions.size(); ++i) {
		FactionType *ft= techTree->getTypeByName(gs->getFactionTypeName(i));
		if(ft == NULL) {
			throw runtime_error("ft == NULL");
		}
		factions[i].init(ft, gs->getFactionControl(i), techTree, game, i, gs->getTeam(i),
						 gs->getStartLocationIndex(i), i==thisFactionIndex, gs->getDefaultResources());

		stats.setTeam(i, gs->getTeam(i));
		stats.setFactionTypeName(i, formatString(gs->getFactionTypeName(i)));
		stats.setPersonalityType(i, getFaction(i)->getType()->getPersonalityType());
		stats.setControl(i, gs->getFactionControl(i));
		stats.setResourceMultiplier(i,(gs->getResourceMultiplierIndex(i)+5)*0.1f);
		stats.setPlayerName(i,gs->getNetworkPlayerName(i));
		stats.setPlayerColor(i,getFaction(i)->getTexture()->getPixmapConst()->getPixel3f(0, 0));
	}
	
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(factions.size() > 0) {
		thisTeamIndex= getFaction(thisFactionIndex)->getTeam();
	}
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void World::initMinimap() {
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	minimap.init(map.getW(), map.getH(), this, game->getGameSettings()->getFogOfWar());
	Logger::getInstance().add("Compute minimap surface", true);

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

//place units randomly aroud start location
void World::initUnits() {
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	Logger::getInstance().add("Generate elements", true);

	//put starting units
	for(int i = 0; i < getFactionCount(); ++i) {
		Faction *f= &factions[i];
		const FactionType *ft= f->getType();

		for(int j = 0; j < ft->getStartingUnitCount(); ++j) {
			const UnitType *ut= ft->getStartingUnit(j);
			int initNumber= ft->getStartingUnitAmount(j);

			for(int l = 0; l < initNumber; l++) {

				UnitPathInterface *newpath = NULL;
				switch(game->getGameSettings()->getPathFinderType()) {
					case pfBasic:
						newpath = new UnitPathBasic();
						break;
					case pfRoutePlanner:
						newpath = new UnitPath();
						break;
					default:
						throw runtime_error("detected unsupported pathfinder type!");
			    }

				Unit *unit= new Unit(getNextUnitId(f), newpath, Vec2i(0), ut, f, &map, CardinalDir::NORTH);

				int startLocationIndex= f->getStartLocationIndex();

				if(placeUnit(map.getStartLocation(startLocationIndex), generationArea, unit, true)) {
					unit->create(true);
					unit->born();
				}
				else {
					throw runtime_error("Unit cant be placed, this error is caused because there is no enough place to put the units near its start location, make a better map: "+unit->getType()->getName() + " Faction: "+intToStr(i));
				}
				if (unit->getType()->hasSkillClass(scBeBuilt)) {
					map.flatternTerrain(unit);
					if(cartographer != NULL) {
						cartographer->updateMapMetrics(unit->getPos(), unit->getType()->getSize());
					}
				}
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] unit created for unit [%s]\n",__FILE__,__FUNCTION__,__LINE__,unit->toString().c_str());
            }
		}
	}
	map.computeNormals();
	map.computeInterpolatedHeights();
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void World::initMap() {
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	map.init();
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

// ==================== exploration ====================

void World::exploreCells(const Vec2i &newPos, int sightRange, int teamIndex) {
	bool cacheLookupPosResult 	= false;
	bool cacheLookupSightResult = false;

	// Experimental cache lookup of previously calculated cells + sight range
	if(MaxExploredCellsLookupItemCache > 0) {
		if(difftime(time(NULL),ExploredCellsLookupItem::lastDebug) >= 10) {
			ExploredCellsLookupItem::lastDebug = time(NULL);
			//printf("In [%s::%s Line: %d] ExploredCellsLookupItemCache.size() = %d\n",__FILE__,__FUNCTION__,__LINE__,ExploredCellsLookupItemCache.size());
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] ExploredCellsLookupItemCache.size() = %d\n",__FILE__,__FUNCTION__,__LINE__,ExploredCellsLookupItemCache.size());
			SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] ExploredCellsLookupItemCache.size() = %d\n",__FILE__,__FUNCTION__,__LINE__,ExploredCellsLookupItemCache.size());
		}

		// Ok we limit the cache size due to possible RAM constraints when
		// the threshold is met
		bool MaxExploredCellsLookupItemCacheTriggered = false;
		if(ExploredCellsLookupItemCache.size() >= MaxExploredCellsLookupItemCache) {
			MaxExploredCellsLookupItemCacheTriggered = true;
			// Snag the oldest entry in the list
			std::map<int,ExploredCellsLookupKey>::reverse_iterator purgeItem = ExploredCellsLookupItemCacheTimer.rbegin();

			ExploredCellsLookupItemCache[purgeItem->second.pos].erase(purgeItem->second.sightRange);

			if(ExploredCellsLookupItemCache[purgeItem->second.pos].size() == 0) {
				ExploredCellsLookupItemCache.erase(purgeItem->second.pos);
			}

			ExploredCellsLookupItemCacheTimer.erase(purgeItem->first);
		}

		// Check the cache for the pos, sightrange and use from
		// cache if already found
		std::map<Vec2i, std::map<int, ExploredCellsLookupItem> >::iterator iterFind = ExploredCellsLookupItemCache.find(newPos);
		if(iterFind != ExploredCellsLookupItemCache.end()) {
			cacheLookupPosResult = true;

			std::map<int, ExploredCellsLookupItem>::iterator iterFind2 = iterFind->second.find(sightRange);
			if(iterFind2 != iterFind->second.end()) {
				cacheLookupSightResult = true;

				std::vector<SurfaceCell *> &cellList = iterFind2->second.exploredCellList;
				for(int idx2 = 0; idx2 < cellList.size(); ++idx2) {
					SurfaceCell *sc = cellList[idx2];
					sc->setExplored(teamIndex, true);
				}
				cellList = iterFind2->second.visibleCellList;
				for(int idx2 = 0; idx2 < cellList.size(); ++idx2) {
					SurfaceCell *sc = cellList[idx2];
					sc->setVisible(teamIndex, true);
				}

				// Only start worrying about updating the cache timer if we
				// have hit the threshold
				if(MaxExploredCellsLookupItemCacheTriggered == true) {
					// Remove the old timer entry since the search key id is stale
					ExploredCellsLookupItemCacheTimer.erase(iterFind2->second.ExploredCellsLookupItemCacheTimerCountIndex);
					iterFind2->second.ExploredCellsLookupItemCacheTimerCountIndex = ExploredCellsLookupItemCacheTimerCount++;

					ExploredCellsLookupKey lookupKey;
					lookupKey.pos = newPos;
					lookupKey.sightRange = sightRange;
					lookupKey.teamIndex = teamIndex;

					// Add a new search key since we just used this item
					ExploredCellsLookupItemCacheTimer[iterFind2->second.ExploredCellsLookupItemCacheTimerCountIndex] = lookupKey;
				}

				return;
			}
		}
	}

	Vec2i newSurfPos= Map::toSurfCoords(newPos);
	int surfSightRange= sightRange/Map::cellScale+1;

	// Explore, this code is quite expensive when we have lots of units
	ExploredCellsLookupItem item;

	int loopCount = 0;
    for(int i = -surfSightRange - indirectSightRange -1; i <= surfSightRange + indirectSightRange +1; ++i) {
        for(int j = -surfSightRange - indirectSightRange -1; j <= surfSightRange + indirectSightRange +1; ++j) {
        	loopCount++;
        	Vec2i currRelPos= Vec2i(i, j);
            Vec2i currPos= newSurfPos + currRelPos;
            if(map.isInsideSurface(currPos)){
				SurfaceCell *sc= map.getSurfaceCell(currPos);

				//explore
				//if(Vec2i(0).dist(currRelPos) < surfSightRange + indirectSightRange + 1) {
				if(currRelPos.length() < surfSightRange + indirectSightRange + 1) {
                    sc->setExplored(teamIndex, true);
                    item.exploredCellList.push_back(sc);
				}

				//visible
				//if(Vec2i(0).dist(currRelPos) < surfSightRange) {
				if(currRelPos.length() < surfSightRange) {
					sc->setVisible(teamIndex, true);
					item.visibleCellList.push_back(sc);
				}
            }
        }
    }

    // Ok update our caches with the latest info for this position, sight and team
    if(MaxExploredCellsLookupItemCache > 0) {
		if(item.exploredCellList.size() > 0 || item.visibleCellList.size() > 0) {
			//ExploredCellsLookupItemCache.push_back(item);
			item.ExploredCellsLookupItemCacheTimerCountIndex = ExploredCellsLookupItemCacheTimerCount++;
			ExploredCellsLookupItemCache[newPos][sightRange] = item;

			ExploredCellsLookupKey lookupKey;
			lookupKey.pos 			= newPos;
			lookupKey.sightRange 	= sightRange;
			lookupKey.teamIndex 	= teamIndex;
			ExploredCellsLookupItemCacheTimer[item.ExploredCellsLookupItemCacheTimerCountIndex] = lookupKey;
		}
    }
}

//computes the fog of war texture, contained in the minimap
void World::computeFow(int factionIdxToTick) {
	//reset texture
	//if(factionIdxToTick == -1 || factionIdxToTick == 0) {
	//if(factionIdxToTick == -1 || factionIdxToTick == this->thisFactionIndex) {
	//if(frameCount % (GameConstants::updateFps) == 0) {
	minimap.resetFowTex();
	//}

	//reset cells
	if(factionIdxToTick == -1 || factionIdxToTick == this->thisFactionIndex) {
		for(int i = 0; i < map.getSurfaceW(); ++i) {
			for(int j = 0; j < map.getSurfaceH(); ++j) {
				for(int k = 0; k < GameConstants::maxPlayers + GameConstants::specialFactions; ++k) {
					if(fogOfWar || k != thisTeamIndex) {
						if(	k == thisTeamIndex &&
							(thisTeamIndex == GameConstants::maxPlayers -1 + fpt_Observer ||
							 (game->getGameOver() == true && (game->getGameSettings()->isNetworkGame() == false ||
							  game->getGameSettings()->getEnableObserverModeAtEndGame() == true)))) {
							map.getSurfaceCell(i, j)->setVisible(k, true);
							map.getSurfaceCell(i, j)->setExplored(k, true);

							const Vec2i pos(i,j);
							//Vec2i surfPos= Map::toSurfCoords(pos);
							Vec2i surfPos= pos;

							//compute max alpha
							float maxAlpha= 0.0f;
							if(surfPos.x>1 && surfPos.y>1 && surfPos.x<map.getSurfaceW()-2 && surfPos.y<map.getSurfaceH()-2){
								maxAlpha= 1.f;
							}
							else if(surfPos.x>0 && surfPos.y>0 && surfPos.x<map.getSurfaceW()-1 && surfPos.y<map.getSurfaceH()-1){
								maxAlpha= 0.3f;
							}

							//compute alpha
							float alpha=maxAlpha;
							minimap.incFowTextureAlphaSurface(surfPos, alpha);
						}
						else {
							map.getSurfaceCell(i, j)->setVisible(k, false);
						}
					}
				}
			}
		}
	}
	else {
		// Deal with observers
		for(int i = 0; i < map.getSurfaceW(); ++i) {
			for(int j = 0; j < map.getSurfaceH(); ++j) {
				for(int k = 0; k < GameConstants::maxPlayers + GameConstants::specialFactions; ++k) {
					if(fogOfWar || k != thisTeamIndex) {
						if(k == thisTeamIndex && thisTeamIndex == GameConstants::maxPlayers -1 + fpt_Observer) {
							map.getSurfaceCell(i, j)->setVisible(k, true);
							map.getSurfaceCell(i, j)->setExplored(k, true);

							const Vec2i pos(i,j);
							//Vec2i surfPos= Map::toSurfCoords(pos);
							Vec2i surfPos= pos;

							//compute max alpha
							float maxAlpha= 0.0f;
							if(surfPos.x>1 && surfPos.y>1 && surfPos.x<map.getSurfaceW()-2 && surfPos.y<map.getSurfaceH()-2){
								maxAlpha= 1.f;
							}
							else if(surfPos.x>0 && surfPos.y>0 && surfPos.x<map.getSurfaceW()-1 && surfPos.y<map.getSurfaceH()-1){
								maxAlpha= 0.3f;
							}

							//compute alpha
							float alpha=maxAlpha;
							minimap.incFowTextureAlphaSurface(surfPos, alpha);
						}
					}
				}
			}
		}
	}

	//compute cells
	for(int i=0; i<getFactionCount(); ++i) {
		if(factionIdxToTick == -1 || factionIdxToTick == this->thisFactionIndex) {
			for(int j=0; j<getFaction(i)->getUnitCount(); ++j) {
				Unit *unit= getFaction(i)->getUnit(j);

				//exploration
				unit->exploreCells();
			}
		}
	}

	//fire
	for(int i=0; i<getFactionCount(); ++i) {
		if(factionIdxToTick == -1 || factionIdxToTick == this->thisFactionIndex) {
			for(int j=0; j<getFaction(i)->getUnitCount(); ++j){
				Unit *unit= getFaction(i)->getUnit(j);

				//fire
				ParticleSystem *fire= unit->getFire();
				if(fire!=NULL){
					fire->setActive(map.getSurfaceCell(Map::toSurfCoords(unit->getPos()))->isVisible(thisTeamIndex));
				}
			}
		}
	}

	//compute texture
	if(fogOfWar) {
		for(int i=0; i<getFactionCount(); ++i) {
			Faction *faction= getFaction(i);
			if(faction->getTeam() == thisTeamIndex) {
				//if(thisTeamIndex == GameConstants::maxPlayers + fpt_Observer) {
				for(int j=0; j<faction->getUnitCount(); ++j){
					const Unit *unit= faction->getUnit(j);
					if(unit->isOperative()){
						int sightRange= unit->getType()->getSight();

						bool foundInCache = false;
						std::map<Vec2i, std::map<int, FowAlphaCellsLookupItem > >::iterator iterMap = FowAlphaCellsLookupItemCache.find(unit->getPos());
						if(iterMap != FowAlphaCellsLookupItemCache.end()) {
							std::map<int, FowAlphaCellsLookupItem>::iterator iterMap2 = iterMap->second.find(sightRange);
							if(iterMap2 != iterMap->second.end()) {
								foundInCache = true;

								FowAlphaCellsLookupItem &cellList = iterMap2->second;
								for(int k = 0; k < cellList.surfPosList.size(); ++k) {
									Vec2i &surfPos = cellList.surfPosList[k];
									float &alpha = cellList.alphaList[k];

									minimap.incFowTextureAlphaSurface(surfPos, alpha);
								}
							}
						}

						if(foundInCache == false) {
							FowAlphaCellsLookupItem itemCache;

							//iterate through all cells
							PosCircularIterator pci(&map, unit->getPos(), sightRange+indirectSightRange);
							while(pci.next()){
								const Vec2i pos= pci.getPos();
								Vec2i surfPos= Map::toSurfCoords(pos);

								//compute max alpha
								float maxAlpha= 0.0f;
								if(surfPos.x>1 && surfPos.y>1 && surfPos.x<map.getSurfaceW()-2 && surfPos.y<map.getSurfaceH()-2){
									maxAlpha= 1.f;
								}
								else if(surfPos.x>0 && surfPos.y>0 && surfPos.x<map.getSurfaceW()-1 && surfPos.y<map.getSurfaceH()-1){
									maxAlpha= 0.3f;
								}

								//compute alpha
								float alpha=maxAlpha;
								float dist= unit->getPos().dist(pos);
								if(dist>sightRange){
									alpha= clamp(1.f-(dist-sightRange)/(indirectSightRange), 0.f, maxAlpha);
								}
								minimap.incFowTextureAlphaSurface(surfPos, alpha);

								itemCache.surfPosList.push_back(surfPos);
								itemCache.alphaList.push_back(alpha);
							}

							if(itemCache.surfPosList.size() > 0) {
								FowAlphaCellsLookupItemCache[unit->getPos()][sightRange] = itemCache;
							}
						}
					}
				}
			}
		}
	}
}

// WARNING! This id is critical! Make sure it fits inside the network packet
// (currently cannot be larger than 2,147,483,647)
// Make sure each faction has their own unique section of id #'s for proper
// multi-platform play
// Calculates the unit unit ID for each faction
//
int World::getNextUnitId(Faction *faction)	{
	if(mapFactionNextUnitId.find(faction->getIndex()) == mapFactionNextUnitId.end()) {
		mapFactionNextUnitId[faction->getIndex()] = faction->getIndex() * 100000;
	}
	return mapFactionNextUnitId[faction->getIndex()]++;
}

std::string World::DumpWorldToLog(bool consoleBasicInfoOnly) const {
	
	string debugWorldLogFile = Config::getInstance().getString("DebugWorldLogFile","debugWorld.log");
	if(getGameReadWritePath() != "") {
		debugWorldLogFile = getGameReadWritePath() + debugWorldLogFile;
	}

	if(consoleBasicInfoOnly == true) {
		std::cout << "World debug information:"  << std::endl;
		std::cout << "========================"  << std::endl;

		//factions (and their related info)
		for(int i = 0; i < getFactionCount(); ++i) {
			std::cout << "Faction detail for index: " << i << std::endl;
			std::cout << "--------------------------" << std::endl;
			
			std::cout << "FactionName = " << getFaction(i)->getType()->getName() << std::endl;
			std::cout << "FactionIndex = " << intToStr(getFaction(i)->getIndex()) << std::endl;
			std::cout << "teamIndex = " << intToStr(getFaction(i)->getTeam()) << std::endl;
			std::cout << "startLocationIndex = " << intToStr(getFaction(i)->getStartLocationIndex()) << std::endl;
			std::cout << "thisFaction = " << intToStr(getFaction(i)->getThisFaction()) << std::endl;
			std::cout << "control = " << intToStr(getFaction(i)->getControlType()) << std::endl;
		}
	}
	else {

		std::ofstream logFile;
		logFile.open(debugWorldLogFile.c_str(), ios_base::out | ios_base::trunc);

		logFile << "World debug information:"  << std::endl;
		logFile << "========================"  << std::endl;

		//factions (and their related info)
		for(int i = 0; i < getFactionCount(); ++i) {
			logFile << "Faction detail for index: " << i << std::endl;
			logFile << "--------------------------" << std::endl;
			logFile << getFaction(i)->toString() << std::endl;
		}

		//undertake the dead
		logFile << "Undertake stats:" << std::endl;
		for(int i = 0; i < getFactionCount(); ++i){
			logFile << "Faction: " << getFaction(i)->getType()->getName() << std::endl;
			int unitCount = getFaction(i)->getUnitCount();
			for(int j= unitCount - 1; j >= 0; j--){
				Unit *unit= getFaction(i)->getUnit(j);
				if(unit->getToBeUndertaken()) {
					logFile << "Undertake unit index = " << j << unit->getFullName() << std::endl;
				}
			}
		}

		logFile.close();
	}
    return debugWorldLogFile;
}

}}//end namespace
