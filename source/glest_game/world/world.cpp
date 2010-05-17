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
#include "leak_dumper.h"

using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class World
// =====================================================

const float World::airHeight= 5.f;

// ===================== PUBLIC ========================

World::World(){
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	Config &config= Config::getInstance();

	techTree = NULL;
	fogOfWarOverride = false;

	fogOfWarSmoothing= config.getBool("FogOfWarSmoothing");
	fogOfWarSmoothingFrameSkip= config.getInt("FogOfWarSmoothingFrameSkip");

	frameCount= 0;
	nextUnitId= 0;

	scriptManager= NULL;
	this->game = NULL;

	allowRotateUnits = Config::getInstance().getBool("AllowRotateUnits","0");
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

World::~World() {
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	delete techTree;
	techTree = NULL;
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void World::end(){
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
    Logger::getInstance().add("World", true);

	for(int i= 0; i<factions.size(); ++i){
		factions[i].end();
	}
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

    this->game = game;
	scriptManager= game->getScriptManager();

	unitUpdater.init(game);

	GameSettings *gs = game->getGameSettings();
	if(fogOfWarOverride == false) {
		fogOfWar = gs->getFogOfWar();
	}
	
	initFactionTypes(gs);
	initCells(fogOfWar); //must be done after knowing faction number and dimensions
	initMap();
	initSplattedTextures();

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
	timeFlow.init(&tileset);
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void World::loadTileset(const string &dir, Checksum *checksum){
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	tileset.load(dir, checksum);
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

void World::update(){

	++frameCount;

	//time
	timeFlow.update();

	//water effects
	waterEffects.update();

	//units
	for(int i=0; i<getFactionCount(); ++i){
		for(int j=0; j<getFaction(i)->getUnitCount(); ++j){
			unitUpdater.updateUnit(getFaction(i)->getUnit(j));
		}
	}

	//undertake the dead
	for(int i=0; i<getFactionCount(); ++i){
		int unitCount = getFaction(i)->getUnitCount();
		for(int j= unitCount - 1; j >= 0; j--){
			Unit *unit= getFaction(i)->getUnit(j);
			if(unit->getToBeUndertaken()) {
				unit->undertake();
				delete unit;
				//j--;
			}
		}
	}

	//food costs
	for(int i=0; i<techTree->getResourceTypeCount(); ++i){
		const ResourceType *rt= techTree->getResourceType(i);
		if(rt->getClass()==rcConsumable && frameCount % (rt->getInterval()*GameConstants::updateFps)==0){
			for(int i=0; i<getFactionCount(); ++i){
				getFaction(i)->applyCostsOnInterval();
			}
		}
	}

	//fow smoothing
	if(fogOfWarSmoothing && ((frameCount+1) % (fogOfWarSmoothingFrameSkip+1))==0){
		float fogFactor= static_cast<float>(frameCount%GameConstants::updateFps)/GameConstants::updateFps;
		minimap.updateFowTex(clamp(fogFactor, 0.f, 1.f));
	}

	//tick
	if(frameCount%GameConstants::updateFps==0){
		computeFow();
		tick();
	}
}

void World::tick(){
	if(!fogOfWarSmoothing){
		minimap.updateFowTex(1.f);
	}

	//increase hp
	for(int i=0; i<getFactionCount(); ++i){
		for(int j=0; j<getFaction(i)->getUnitCount(); ++j){
			getFaction(i)->getUnit(j)->tick();
		}
	}

	//compute resources balance
	for(int k=0; k<getFactionCount(); ++k){
		Faction *faction= getFaction(k);

		//for each resource
		for(int i=0; i<techTree->getResourceTypeCount(); ++i){
			const ResourceType *rt= techTree->getResourceType(i);

			//if consumable
			if(rt->getClass()==rcConsumable){
				int balance= 0;
				for(int j=0; j<faction->getUnitCount(); ++j){

					//if unit operative and has this cost
					const Unit *u=  faction->getUnit(j);
					if(u->isOperative()){
						const Resource *r= u->getType()->getCost(rt);
						if(r!=NULL){
							balance-= u->getType()->getCost(rt)->getAmount();
						}
					}
				}
				faction->setResourceBalance(rt, balance);
			}
		}
	}
}

Unit* World::findUnitById(int id){
	for(int i= 0; i<getFactionCount(); ++i){
		Faction* faction= getFaction(i);

		for(int j= 0; j<faction->getUnitCount(); ++j){
			Unit* unit= faction->getUnit(j);

			if(unit->getId()==id){
				return unit;
			}
		}
	}
	return NULL;
}

const UnitType* World::findUnitTypeById(const FactionType* factionType, int id){
	for(int i= 0; i<factionType->getUnitTypeCount(); ++i){
		const UnitType* unitType= factionType->getUnitType(i);

		if(unitType->getId()==id){
			return unitType;
		}
	}
	return NULL;
}

//looks for a place for a unit around a start lociacion, returns true if succeded
bool World::placeUnit(const Vec2i &startLoc, int radius, Unit *unit, bool spaciated){
    bool freeSpace;
	int size= unit->getType()->getSize();
	Field currField= unit->getCurrField();

    for(int r=1; r<radius; r++){
        for(int i=-r; i<r; ++i){
            for(int j=-r; j<r; ++j){
                Vec2i pos= Vec2i(i,j)+startLoc;
				if(spaciated){
                    const int spacing= 2;
					freeSpace= map.isFreeCells(pos-Vec2i(spacing), size+spacing*2, currField);
				}
				else{
                    freeSpace= map.isFreeCells(pos, size, currField);
				}

                if(freeSpace){
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
void World::moveUnitCells(Unit *unit){
	Vec2i newPos= unit->getTargetPos();

	//newPos must be free or the same pos as current
	assert(map.getCell(unit->getPos())->getUnit(unit->getCurrField())==unit || map.isFreeCell(newPos, unit->getCurrField()));
	map.clearUnitCells(unit, unit->getPos());
	map.putUnitCells(unit, newPos);

	//water splash
	if(tileset.getWaterEffects() && unit->getCurrField()==fLand){
		if(map.getSubmerged(map.getCell(unit->getLastPos()))){
			int unitSize=unit->getType()->getSize();
			for(int i=0; i<3; ++i){
				waterEffects.addWaterSplash(
					Vec2f(unit->getLastPos().x+(float)unitSize/2.0f+random.randRange(-0.9f, -0.1f), unit->getLastPos().y+random.randRange(-0.9f, -0.1f)+(float)unitSize/2.0f), unit->getType()->getSize());
			}
		}
	}
}

//returns the nearest unit that can store a type of resource given a position and a faction
Unit *World::nearestStore(const Vec2i &pos, int factionIndex, const ResourceType *rt){
    float currDist= infinity;
    Unit *currUnit= NULL;

    for(int i=0; i<getFaction(factionIndex)->getUnitCount(); ++i){
		Unit *u= getFaction(factionIndex)->getUnit(i);
		float tmpDist= u->getPos().dist(pos);
        if(tmpDist<currDist && u->getType()->getStore(rt)>0 && u->isOperative()){
            currDist= tmpDist;
            currUnit= u;
        }
    }
    return currUnit;
}

bool World::toRenderUnit(const Unit *unit, const Quad2i &visibleQuad) const{
    //a unit is rendered if it is in a visible cell or is attacking a unit in a visible cell
    return
		visibleQuad.isInside(unit->getPos()) &&
		toRenderUnit(unit);
}

bool World::toRenderUnit(const Unit *unit) const{

	return
        map.getSurfaceCell(Map::toSurfCoords(unit->getCenteredPos()))->isVisible(thisTeamIndex) ||
        (unit->getCurrSkill()->getClass()==scAttack &&
        map.getSurfaceCell(Map::toSurfCoords(unit->getTargetPos()))->isVisible(thisTeamIndex));
}

void World::createUnit(const string &unitName, int factionIndex, const Vec2i &pos){
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] unitName [%s] factionIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,unitName.c_str(),factionIndex);

	if(factionIndex<factions.size()){
		Faction* faction= &factions[factionIndex];
		const FactionType* ft= faction->getType();
		const UnitType* ut= ft->getUnitType(unitName);

		Unit* unit= new Unit(getNextUnitId(), pos, ut, faction, &map, CardinalDir::NORTH);

		if(placeUnit(pos, generationArea, unit, true)){
			unit->create(true);
			unit->born();
			scriptManager->onUnitCreated(unit);
		}
		else{
			throw runtime_error("Unit cant be placed");
		}
	}
	else
	{
		throw runtime_error("Invalid faction index in createUnitAtPosition: " + intToStr(factionIndex));
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void World::giveResource(const string &resourceName, int factionIndex, int amount){
	if(factionIndex<factions.size()){
		Faction* faction= &factions[factionIndex];
		const ResourceType* rt= techTree->getResourceType(resourceName);
		faction->incResourceAmount(rt, amount);
	}
	else
	{
		throw runtime_error("Invalid faction index in giveResource: " + intToStr(factionIndex));
	}
}

void World::givePositionCommand(int unitId, const string &commandName, const Vec2i &pos){
	Unit* unit= findUnitById(unitId);
	if(unit!=NULL){
		CommandClass cc;

		if(commandName=="move"){
			cc= ccMove;
		}
		else if(commandName=="attack"){
			cc= ccAttack;
		}
		else{
			throw runtime_error("Invalid position commmand: " + commandName);
		}

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] cc = %d Unit [%s]\n",__FILE__,__FUNCTION__,__LINE__,cc,unit->getFullName().c_str());
		unit->giveCommand(new Command( unit->getType()->getFirstCtOfClass(cc), pos ));
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}
}

void World::giveProductionCommand(int unitId, const string &producedName){
	Unit *unit= findUnitById(unitId);
	if(unit!=NULL){
		const UnitType *ut= unit->getType();

		//Search for a command that can produce the unit
		for(int i= 0; i<ut->getCommandTypeCount(); ++i){
			const CommandType* ct= ut->getCommandType(i);
			if(ct->getClass()==ccProduce){
				const ProduceCommandType *pct= static_cast<const ProduceCommandType*>(ct);
				if(pct->getProducedUnit()->getName()==producedName){
					SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
					unit->giveCommand(new Command(pct));
					SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
					break;
				}
			}
		}
	}
}

void World::giveUpgradeCommand(int unitId, const string &upgradeName){
	Unit *unit= findUnitById(unitId);
	if(unit!=NULL){
		const UnitType *ut= unit->getType();

		//Search for a command that can produce the unit
		for(int i= 0; i<ut->getCommandTypeCount(); ++i){
			const CommandType* ct= ut->getCommandType(i);
			if(ct->getClass()==ccUpgrade){
				const UpgradeCommandType *uct= static_cast<const UpgradeCommandType*>(ct);
				if(uct->getProducedUpgrade()->getName()==upgradeName){
					SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
					unit->giveCommand(new Command(uct));
					SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
					break;
				}
			}
		}
	}
}


int World::getResourceAmount(const string &resourceName, int factionIndex){
	if(factionIndex<factions.size()){
		Faction* faction= &factions[factionIndex];
		const ResourceType* rt= techTree->getResourceType(resourceName);
		return faction->getResource(rt)->getAmount();
	}
	else
	{
		throw runtime_error("Invalid faction index in giveResource: " + intToStr(factionIndex));
	}
}

Vec2i World::getStartLocation(int factionIndex){
	if(factionIndex<factions.size()){
		Faction* faction= &factions[factionIndex];
		return map.getStartLocation(faction->getStartLocationIndex());
	}
	else
	{
		throw runtime_error("Invalid faction index in getStartLocation: " + intToStr(factionIndex));
	}
}

Vec2i World::getUnitPosition(int unitId){
	Unit* unit= findUnitById(unitId);
	if(unit==NULL){
		throw runtime_error("Can not find unit to get position");
	}
	return unit->getPos();
}

int World::getUnitFactionIndex(int unitId){
	Unit* unit= findUnitById(unitId);
	if(unit==NULL){
		throw runtime_error("Can not find unit to get position");
	}
	return unit->getFactionIndex();
}

int World::getUnitCount(int factionIndex){
	if(factionIndex<factions.size()){
		Faction* faction= &factions[factionIndex];
		int count= 0;

		for(int i= 0; i<faction->getUnitCount(); ++i){
			const Unit* unit= faction->getUnit(i);
			if(unit->isAlive()){
				++count;
			}
		}
		return count;
	}
	else
	{
		throw runtime_error("Invalid faction index in getUnitCount: " + intToStr(factionIndex));
	}
}

int World::getUnitCountOfType(int factionIndex, const string &typeName){
	if(factionIndex<factions.size()){
		Faction* faction= &factions[factionIndex];
		int count= 0;

		for(int i= 0; i< faction->getUnitCount(); ++i){
			const Unit* unit= faction->getUnit(i);
			if(unit->isAlive() && unit->getType()->getName()==typeName){
				++count;
			}
		}
		return count;
	}
	else
	{
		throw runtime_error("Invalid faction index in getUnitCountOfType: " + intToStr(factionIndex));
	}
}

// ==================== PRIVATE ====================

// ==================== private init ====================

//init basic cell state
void World::initCells(bool fogOfWar){
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	Logger::getInstance().add("State cells", true);
    for(int i=0; i<map.getSurfaceW(); ++i){
        for(int j=0; j<map.getSurfaceH(); ++j){

			SurfaceCell *sc= map.getSurfaceCell(i, j);

			sc->setFowTexCoord(Vec2f(
				i/(next2Power(map.getSurfaceW())-1.f),
				j/(next2Power(map.getSurfaceH())-1.f)));

			for (int k = 0; k < GameConstants::maxPlayers; k++) {
				sc->setExplored(k, !fogOfWar);
				sc->setVisible(k, !fogOfWar);
			}
		}
    }
    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

//init surface textures
void World::initSplattedTextures(){
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	for(int i=0; i<map.getSurfaceW()-1; ++i){
        for(int j=0; j<map.getSurfaceH()-1; ++j){
			Vec2f coord;
			const Texture2D *texture;
			SurfaceCell *sc00= map.getSurfaceCell(i, j);
			SurfaceCell *sc10= map.getSurfaceCell(i+1, j);
			SurfaceCell *sc01= map.getSurfaceCell(i, j+1);
			SurfaceCell *sc11= map.getSurfaceCell(i+1, j+1);
			tileset.addSurfTex(
				sc00->getSurfaceType(),
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
void World::initFactionTypes(GameSettings *gs){
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	Logger::getInstance().add("Faction types", true);

	if(gs->getFactionCount() > map.getMaxPlayers()){
		throw runtime_error("This map only supports "+intToStr(map.getMaxPlayers())+" players");
	}

	//create stats
	stats.init(gs->getFactionCount(), gs->getThisFactionIndex(), gs->getDescription());

	//create factions
	this->thisFactionIndex= gs->getThisFactionIndex();
	factions.resize(gs->getFactionCount());
	for(int i=0; i<factions.size(); ++i){
		const FactionType *ft= techTree->getType(gs->getFactionTypeName(i));
		factions[i].init(
			ft, gs->getFactionControl(i), techTree, game, i, gs->getTeam(i),
			gs->getStartLocationIndex(i), i==thisFactionIndex, gs->getDefaultResources());

		stats.setTeam(i, gs->getTeam(i));
		stats.setFactionTypeName(i, formatString(gs->getFactionTypeName(i)));
		stats.setControl(i, gs->getFactionControl(i));
	}

	thisTeamIndex= getFaction(thisFactionIndex)->getTeam();
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void World::initMinimap(){
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
    minimap.init(map.getW(), map.getH(), this, game->getGameSettings()->getFogOfWar());
	Logger::getInstance().add("Compute minimap surface", true);
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

//place units randomly aroud start location
void World::initUnits(){
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	Logger::getInstance().add("Generate elements", true);

	//put starting units
	for(int i=0; i<getFactionCount(); ++i){
		Faction *f= &factions[i];
		const FactionType *ft= f->getType();
		for(int j=0; j<ft->getStartingUnitCount(); ++j){
			const UnitType *ut= ft->getStartingUnit(j);
			int initNumber= ft->getStartingUnitAmount(j);
			for(int l=0; l<initNumber; l++){
				Unit *unit= new Unit(getNextUnitId(), Vec2i(0), ut, f, &map, CardinalDir::NORTH);

				int startLocationIndex= f->getStartLocationIndex();

				if(placeUnit(map.getStartLocation(startLocationIndex), generationArea, unit, true)){
					unit->create(true);
					unit->born();
				}
				else{
					throw runtime_error("Unit cant be placed, this error is caused because there is no enough place to put the units near its start location, make a better map: "+unit->getType()->getName() + " Faction: "+intToStr(i));
				}
				if(unit->getType()->hasSkillClass(scBeBuilt)){
                    map.flatternTerrain(unit);
				}
            }
		}
	}
	map.computeNormals();
	map.computeInterpolatedHeights();
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void World::initMap(){
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	map.init();
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

// ==================== exploration ====================

void World::exploreCells(const Vec2i &newPos, int sightRange, int teamIndex){

	Vec2i newSurfPos= Map::toSurfCoords(newPos);
	int surfSightRange= sightRange/Map::cellScale+1;

	//explore
    for(int i=-surfSightRange-indirectSightRange-1; i<=surfSightRange+indirectSightRange+1; ++i){
        for(int j=-surfSightRange-indirectSightRange-1; j<=surfSightRange+indirectSightRange+1; ++j){
			Vec2i currRelPos= Vec2i(i, j);
            Vec2i currPos= newSurfPos + currRelPos;
            if(map.isInsideSurface(currPos)){

				SurfaceCell *sc= map.getSurfaceCell(currPos);

				//explore
				if(Vec2i(0).dist(currRelPos) < surfSightRange+indirectSightRange+1){
                    sc->setExplored(teamIndex, true);
				}

				//visible
				if(Vec2i(0).dist(currRelPos) < surfSightRange){
					sc->setVisible(teamIndex, true);
				}
            }
        }
    }
}

//computes the fog of war texture, contained in the minimap
void World::computeFow(){

	//reset texture
	minimap.resetFowTex();

	//reset cells
	for(int i=0; i<map.getSurfaceW(); ++i){
		for(int j=0; j<map.getSurfaceH(); ++j){
			for(int k=0; k<GameConstants::maxPlayers; ++k){
				if(fogOfWar || k!=thisTeamIndex){
					map.getSurfaceCell(i, j)->setVisible(k, false);
				}
			}
		}
	}

	//compute cells
	for(int i=0; i<getFactionCount(); ++i){
		for(int j=0; j<getFaction(i)->getUnitCount(); ++j){
			Unit *unit= getFaction(i)->getUnit(j);

			//exploration
			if(unit->isOperative()){
				exploreCells(unit->getCenteredPos(), unit->getType()->getSight(), unit->getTeam());
			}
		}
	}

	//fire
	for(int i=0; i<getFactionCount(); ++i){
		for(int j=0; j<getFaction(i)->getUnitCount(); ++j){
			Unit *unit= getFaction(i)->getUnit(j);

			//fire
			ParticleSystem *fire= unit->getFire();
			if(fire!=NULL){
				fire->setActive(map.getSurfaceCell(Map::toSurfCoords(unit->getPos()))->isVisible(thisTeamIndex));
			}
		}
	}

	//compute texture
	for(int i=0; i<getFactionCount(); ++i){
		Faction *faction= getFaction(i);
		if(faction->getTeam()==thisTeamIndex){
			for(int j=0; j<faction->getUnitCount(); ++j){
				const Unit *unit= faction->getUnit(j);
				if(unit->isOperative()){
					int sightRange= unit->getType()->getSight();

					//iterate through all cells
					PosCircularIterator pci(&map, unit->getPos(), sightRange+indirectSightRange);
					while(pci.next()){
						Vec2i pos= pci.getPos();
						Vec2i surfPos= Map::toSurfCoords(pos);


						//compute max alpha
						float maxAlpha;
						if(surfPos.x>1 && surfPos.y>1 && surfPos.x<map.getSurfaceW()-2 && surfPos.y<map.getSurfaceH()-2){
							maxAlpha= 1.f;
						}
						else if(surfPos.x>0 && surfPos.y>0 && surfPos.x<map.getSurfaceW()-1 && surfPos.y<map.getSurfaceH()-1){
							maxAlpha= 0.3f;
						}
						else{
							maxAlpha= 0.0f;
						}

						//compute alpha
						float alpha;
						float dist= unit->getPos().dist(pos);
						if(dist>sightRange){
							alpha= clamp(1.f-(dist-sightRange)/(indirectSightRange), 0.f, maxAlpha);
						}
						else{
							alpha= maxAlpha;
						}
						minimap.incFowTextureAlphaSurface(surfPos, alpha);
					}
				}
			}
		}
	}
}

}}//end namespace
