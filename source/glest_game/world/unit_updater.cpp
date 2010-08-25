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

#include "unit_updater.h"

#include <algorithm>
#include <cassert>

#include "cartographer.h"
#include "core_data.h"
#include "config.h"
#include "game.h"
#include "faction.h"
#include "network_manager.h"
#include "object.h"
#include "particle_type.h"
#include "path_finder.h"
#include "renderer.h"
#include "route_planner.h"
#include "sound.h"
#include "sound_renderer.h"
#include "upgrade.h"
#include "unit.h"

#include "leak_dumper.h"

using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class UnitUpdater
// =====================================================

// ===================== PUBLIC ========================

UnitUpdater::UnitUpdater() {
    this->game= NULL;
	this->gui= NULL;
	this->gameCamera= NULL;
	this->world= NULL;
	this->map= NULL;
	this->console= NULL;
	this->scriptManager= NULL;
	this->routePlanner = NULL;
	this->pathFinder = NULL;
	UnitRangeCellsLookupItemCacheTimerCount = 0;
}

void UnitUpdater::init(Game *game){

    this->game= game;
	this->gui= game->getGui();
	this->gameCamera= game->getGameCamera();
	this->world= game->getWorld();
	this->map= world->getMap();
	this->console= game->getConsole();
	this->scriptManager= game->getScriptManager();
	this->routePlanner = NULL;
	this->pathFinder = NULL;
	UnitRangeCellsLookupItemCacheTimerCount = 0;

	switch(this->game->getGameSettings()->getPathFinderType()) {
		case pfBasic:
			pathFinder = new PathFinder();
			pathFinder->init(map);
			break;
		case pfRoutePlanner:
			routePlanner = world->getRoutePlanner();
			break;
		default:
			throw runtime_error("detected unsupported pathfinder type!");
    }
}

UnitUpdater::~UnitUpdater() {
	UnitRangeCellsLookupItemCache.clear();

	delete pathFinder;
	pathFinder = NULL;
}

// ==================== progress skills ====================

//skill dependent actions
void UnitUpdater::updateUnit(Unit *unit) {
	Chrono chrono;
	chrono.start();

	SoundRenderer &soundRenderer= SoundRenderer::getInstance();

	//play skill sound
	const SkillType *currSkill= unit->getCurrSkill();
	if(currSkill->getSound()!=NULL){
		float soundStartTime= currSkill->getSoundStartTime();
		if(soundStartTime>=unit->getLastAnimProgress() && soundStartTime<unit->getAnimProgress()){
			if(map->getSurfaceCell(Map::toSurfCoords(unit->getPos()))->isVisible(world->getThisTeamIndex())){
				soundRenderer.playFx(currSkill->getSound(), unit->getCurrVector(), gameCamera->getPos());
			}
		}
	}

	if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [play skill sound]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
	if(chrono.getMillis() > 0) chrono.start();

	//start attack particle system
	if(unit->getCurrSkill()->getClass()==scAttack){
		const AttackSkillType *ast= static_cast<const AttackSkillType*>(unit->getCurrSkill());
		float attackStartTime= ast->getAttackStartTime();
		if(attackStartTime>=unit->getLastAnimProgress() && attackStartTime<unit->getAnimProgress()){
			startAttackParticleSystem(unit);
		}
	}

	if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [start attack particles]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
	if(chrono.getMillis() > 0) chrono.start();

	//update unit
	if(unit->update()){
        //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [update unit check]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
		if(chrono.getMillis() > 0) chrono.start();

		updateUnitCommand(unit);

		if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [update unit command]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
		if(chrono.getMillis() > 0) chrono.start();

		//if unit is out of EP, it stops
		if(unit->computeEp()){
			unit->setCurrSkill(scStop);
			unit->cancelCommand();
		}

		if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [compute ep]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
		if(chrono.getMillis() > 0) chrono.start();

		//move unit in cells
		if(unit->getCurrSkill()->getClass()==scMove){
			world->moveUnitCells(unit);

			if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [move unit cells]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
			if(chrono.getMillis() > 0) chrono.start();

			//play water sound
			if(map->getCell(unit->getPos())->getHeight()<map->getWaterLevel() && unit->getCurrField()==fLand){
				soundRenderer.playFx(
					CoreData::getInstance().getWaterSound(),
					unit->getCurrVector(),
					gameCamera->getPos()
				);

				if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [play water sound]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
				if(chrono.getMillis() > 0) chrono.start();
			}
		}
	}

	//unit death
	if(unit->isDead() && unit->getCurrSkill()->getClass()!=scDie){
		unit->kill();
		if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [kill unit]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
		if(chrono.getMillis() > 0) chrono.start();
	}
}


// ==================== progress commands ====================

//VERY IMPORTANT: compute next state depending on the first order of the list
void UnitUpdater::updateUnitCommand(Unit *unit){
	Chrono chrono;
	chrono.start();

	//if unit has command process it
    if(unit->anyCommand()) {
		unit->getCurrCommand()->getCommandType()->update(this, unit);
	}

	//if no commands stop and add stop command
	if(!unit->anyCommand() && unit->isOperative()){
	    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		unit->setCurrSkill(scStop);
		if(unit->getType()->hasCommandClass(ccStop)){
		    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			unit->giveCommand(new Command(unit->getType()->getFirstCtOfClass(ccStop)));
		}
	}

	if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
	if(chrono.getMillis() > 0) chrono.start();
}

// ==================== updateStop ====================

void UnitUpdater::updateStop(Unit *unit){
	Chrono chrono;
	chrono.start();

    Command *command= unit->getCurrCommand();
    const StopCommandType *sct = static_cast<const StopCommandType*>(command->getCommandType());
    Unit *sighted;

    unit->setCurrSkill(sct->getStopSkillType());

	//we can attack any unit => attack it
   	if(unit->getType()->hasSkillClass(scAttack)){
		for(int i=0; i<unit->getType()->getCommandTypeCount(); ++i){
			const CommandType *ct= unit->getType()->getCommandType(i);

			//look for an attack skill
			const AttackSkillType *ast= NULL;
			if(ct->getClass()==ccAttack){
				ast= static_cast<const AttackCommandType*>(ct)->getAttackSkillType();
			}
			else if(ct->getClass()==ccAttackStopped){
				ast= static_cast<const AttackStoppedCommandType*>(ct)->getAttackSkillType();
			}

			//use it to attack
			if(ast!=NULL){
				if(attackableOnSight(unit, &sighted, ast)){
				    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
					unit->giveCommand(new Command(ct, sighted->getPos()));
					break;
				}
			}
		}
	}

	//see any unit and cant attack it => run
	else if(unit->getType()->hasCommandClass(ccMove)){
		if(attackerOnSight(unit, &sighted)){
			Vec2i escapePos= unit->getPos()*2-sighted->getPos();
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			unit->giveCommand(new Command(unit->getType()->getFirstCtOfClass(ccMove), escapePos));
		}
	}

	if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
	if(chrono.getMillis() > 0) chrono.start();
}


// ==================== updateMove ====================
void UnitUpdater::updateMove(Unit *unit){
	Chrono chrono;
	chrono.start();

    Command *command= unit->getCurrCommand();
    const MoveCommandType *mct= static_cast<const MoveCommandType*>(command->getCommandType());

	Vec2i pos= command->getUnit()!=NULL? command->getUnit()->getCenteredPos(): command->getPos();

	TravelState tsValue = tsImpossible;
	switch(this->game->getGameSettings()->getPathFinderType()) {
		case pfBasic:
			tsValue = pathFinder->findPath(unit, pos);
			break;
		case pfRoutePlanner:
			tsValue = routePlanner->findPath(unit, pos);
			break;
		default:
			throw runtime_error("detected unsupported pathfinder type!");
    }

	switch (tsValue) {
	case tsMoving:
		unit->setCurrSkill(mct->getMoveSkillType());
        break;

	case tsBlocked:
		unit->setCurrSkill(scStop);
		if(unit->getPath()->isBlocked()){
			unit->finishCommand();
		}
		break;

    default:
        unit->finishCommand();
    }

	if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
	if(chrono.getMillis() > 0) chrono.start();
}


// ==================== updateAttack ====================

void UnitUpdater::updateAttack(Unit *unit){
	Chrono chrono;
	chrono.start();

	Command *command= unit->getCurrCommand();
    const AttackCommandType *act= static_cast<const AttackCommandType*>(command->getCommandType());
	Unit *target= NULL;

	//if found
    if(attackableOnRange(unit, &target, act->getAttackSkillType())){
		if(unit->getEp()>=act->getAttackSkillType()->getEpCost()){
			unit->setCurrSkill(act->getAttackSkillType());
			unit->setTarget(target);
		}
		else{
			unit->setCurrSkill(scStop);
		}
    }
    else{
		//compute target pos
		Vec2i pos;
		if(command->getUnit()!=NULL){
			pos= command->getUnit()->getCenteredPos();
		}
		else if(attackableOnSight(unit, &target, act->getAttackSkillType())){
			pos= target->getPos();
		}
		else{
			pos= command->getPos();
		}

		TravelState tsValue = tsImpossible;
		switch(this->game->getGameSettings()->getPathFinderType()) {
			case pfBasic:
				tsValue = pathFinder->findPath(unit, pos);
				break;
			case pfRoutePlanner:
				tsValue = routePlanner->findPath(unit, pos);
				break;
			default:
				throw runtime_error("detected unsupported pathfinder type!");
	    }

		//if unit arrives destPos order has ended
        switch (tsValue){
        case tsMoving:
            unit->setCurrSkill(act->getMoveSkillType());
            break;
		case tsBlocked:
			if(unit->getPath()->isBlocked()){
				unit->finishCommand();
			}
			break;
		default:
			unit->finishCommand();
		}
    }

	if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
	if(chrono.getMillis() > 0) chrono.start();
}


// ==================== updateAttackStopped ====================

void UnitUpdater::updateAttackStopped(Unit *unit){
	Chrono chrono;
	chrono.start();

    Command *command= unit->getCurrCommand();
    const AttackStoppedCommandType *asct= static_cast<const AttackStoppedCommandType*>(command->getCommandType());
    Unit *enemy;

    if(attackableOnRange(unit, &enemy, asct->getAttackSkillType())){
        unit->setCurrSkill(asct->getAttackSkillType());
		unit->setTarget(enemy);
    }
    else{
        unit->setCurrSkill(asct->getStopSkillType());
    }

    if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
	if(chrono.getMillis() > 0) chrono.start();
}


// ==================== updateBuild ====================

void UnitUpdater::updateBuild(Unit *unit){
	Chrono chrono;
	chrono.start();

	Command *command= unit->getCurrCommand();
    const BuildCommandType *bct= static_cast<const BuildCommandType*>(command->getCommandType());

	if(unit->getCurrSkill()->getClass() != scBuild) {
        //if not building
        const UnitType *ut= command->getUnitType();

		TravelState tsValue = tsImpossible;
		switch(this->game->getGameSettings()->getPathFinderType()) {
			case pfBasic:
				tsValue = pathFinder->findPath(unit, command->getPos()-Vec2i(1));
				break;
			case pfRoutePlanner:
				tsValue = routePlanner->findPathToBuildSite(unit, ut, command->getPos(), command->getFacing());
				break;
			default:
				throw runtime_error("detected unsupported pathfinder type!");
	    }

		switch (tsValue) {
        case tsMoving:
            unit->setCurrSkill(bct->getMoveSkillType());
            break;

        case tsArrived:
        	{
        	//if arrived destination
            assert(ut);

            bool canOccupyCell = false;
    		switch(this->game->getGameSettings()->getPathFinderType()) {
    			case pfBasic:
    				canOccupyCell = map->isFreeCells(command->getPos(), ut->getSize(), fLand);
    				break;
    			case pfRoutePlanner:
    				canOccupyCell = map->canOccupy(command->getPos(), ut->getField(), ut, command->getFacing());
    				break;
    			default:
    				throw runtime_error("detected unsupported pathfinder type!");
    	    }

			if (canOccupyCell == true) {
				const UnitType *builtUnitType= command->getUnitType();
				CardinalDir facing = command->getFacing();

				UnitPathInterface *newpath = NULL;
				switch(this->game->getGameSettings()->getPathFinderType()) {
					case pfBasic:
						newpath = new UnitPathBasic();
						break;
					case pfRoutePlanner:
						newpath = new UnitPath();
						break;
					default:
						throw runtime_error("detected unsupported pathfinder type!");
			    }

				Unit *builtUnit= new Unit(world->getNextUnitId(unit->getFaction()), newpath, command->getPos(), builtUnitType, unit->getFaction(), world->getMap(), facing);
				builtUnit->create();

				if(!builtUnitType->hasSkillClass(scBeBuilt)){
					throw runtime_error("Unit " + builtUnitType->getName() + "has no be_built skill");
				}

				builtUnit->setCurrSkill(scBeBuilt);
				unit->setCurrSkill(bct->getBuildSkillType());
				unit->setTarget(builtUnit);
				map->prepareTerrain(builtUnit);

	    		switch(this->game->getGameSettings()->getPathFinderType()) {
	    			case pfBasic:
	    				break;
	    			case pfRoutePlanner:
	    				world->getCartographer()->updateMapMetrics(builtUnit->getPos(), builtUnit->getType()->getSight());
	    				break;
	    			default:
	    				throw runtime_error("detected unsupported pathfinder type!");
	    	    }

				command->setUnit(builtUnit);

				//play start sound
				if(unit->getFactionIndex()==world->getThisFactionIndex()){
					SoundRenderer::getInstance().playFx(
						bct->getStartSound(),
						unit->getCurrVector(),
						gameCamera->getPos());
				}

				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] unit created for unit [%s]\n",__FILE__,__FUNCTION__,__LINE__,builtUnit->toString().c_str());
			}
            else{
                //if there are no free cells
				unit->cancelCommand();
                unit->setCurrSkill(scStop);
				if(unit->getFactionIndex()==world->getThisFactionIndex()){
                     console->addStdMessage("BuildingNoPlace");
				}
            }
        	}
            break;

        case tsBlocked:
			if(unit->getPath()->isBlocked()){
				unit->cancelCommand();
			}
            break;
        }
    }
    else{
        //if building
        Unit *builtUnit= map->getCell(unit->getTargetPos())->getUnit(fLand);

        //if u is killed while building then u==NULL;
		if(builtUnit!=NULL && builtUnit!=command->getUnit()){
			unit->setCurrSkill(scStop);
		}
		else if(builtUnit==NULL || builtUnit->isBuilt()){
            unit->finishCommand();
            unit->setCurrSkill(scStop);
        }
        else if(builtUnit->repair()){
            //building finished
            unit->finishCommand();
            unit->setCurrSkill(scStop);
			builtUnit->born();
			scriptManager->onUnitCreated(builtUnit);
			if(unit->getFactionIndex()==world->getThisFactionIndex()){
				SoundRenderer::getInstance().playFx(
					bct->getBuiltSound(),
					unit->getCurrVector(),
					gameCamera->getPos());
			}
        }
    }

    if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
	if(chrono.getMillis() > 0) chrono.start();
}


// ==================== updateHarvest ====================

void UnitUpdater::updateHarvest(Unit *unit){
	Chrono chrono;
	chrono.start();

	Command *command= unit->getCurrCommand();
    const HarvestCommandType *hct= static_cast<const HarvestCommandType*>(command->getCommandType());
	Vec2i targetPos;

	TravelState tsValue = tsImpossible;
	UnitPathInterface *path= unit->getPath();

	if(unit->getCurrSkill()->getClass() != scHarvest) {
		//if not working
		if(unit->getLoadCount()==0){
			//if not loaded go for resources
			Resource *r= map->getSurfaceCell(Map::toSurfCoords(command->getPos()))->getResource();
			if(r!=NULL && hct->canHarvest(r->getType())){
				//if can harvest dest. pos
				bool canHarvestDestPos = false;

	    		switch(this->game->getGameSettings()->getPathFinderType()) {
	    			case pfBasic:
						canHarvestDestPos = (unit->getPos().dist(command->getPos())<harvestDistance &&
											  map->isResourceNear(unit->getPos(), r->getType(), targetPos,unit->getType()->getSize()));
	    				break;
	    			case pfRoutePlanner:
	    				canHarvestDestPos = map->isResourceNear(unit->getPos(), unit->getType()->getSize(), r->getType(), targetPos);
	    				break;
	    			default:
	    				throw runtime_error("detected unsupported pathfinder type!");
	    	    }

				if (canHarvestDestPos == true) {
						//if it finds resources it starts harvesting
						unit->setCurrSkill(hct->getHarvestSkillType());
						unit->setTargetPos(targetPos);
						command->setPos(targetPos);
						unit->setLoadCount(0);

			    		switch(this->game->getGameSettings()->getPathFinderType()) {
			    			case pfBasic:
			    				unit->setLoadType(map->getSurfaceCell(Map::toSurfCoords(unit->getTargetPos()))->getResource()->getType());
			    				break;
			    			case pfRoutePlanner:
			    				unit->setLoadType(r->getType());
			    				break;
			    			default:
			    				throw runtime_error("detected unsupported pathfinder type!");
			    	    }
				}
				else {

					//if not continue walking
					TravelState tsValue = tsImpossible;
		    		switch(this->game->getGameSettings()->getPathFinderType()) {
		    			case pfBasic:
							tsValue = pathFinder->findPath(unit, command->getPos());
							if (tsValue == tsMoving) {
								unit->setCurrSkill(hct->getMoveSkillType());
							}
		    				break;
		    			case pfRoutePlanner:
							tsValue = routePlanner->findPathToResource(unit, command->getPos(), r->getType());
							if (tsValue == tsMoving) {
								unit->setCurrSkill(hct->getMoveSkillType());
							}
		    				break;
		    			default:
		    				throw runtime_error("detected unsupported pathfinder type!");
		    	    }
				}
			}
			else{
				//if can't harvest, search for another resource
				unit->setCurrSkill(scStop);
				if(!searchForResource(unit, hct)){
					unit->finishCommand();
				}
			}
		}
		else{
			//if loaded, return to store
			Unit *store= world->nearestStore(unit->getPos(), unit->getFaction()->getIndex(), unit->getLoadType());
			if(store!=NULL) {
				TravelState tsValue = tsImpossible;
	    		switch(this->game->getGameSettings()->getPathFinderType()) {
	    			case pfBasic:
	    				tsValue = pathFinder->findPath(unit, store->getCenteredPos());
	    				break;
	    			case pfRoutePlanner:
	    				tsValue = routePlanner->findPathToStore(unit, store);
	    				break;
	    			default:
	    				throw runtime_error("detected unsupported pathfinder type!");
	    	    }

				switch(tsValue) {
				case tsMoving:
					unit->setCurrSkill(hct->getMoveLoadedSkillType());
					break;
				default:
					break;
				}

				//world->changePosCells(unit,unit->getPos()+unit->getDest());
				if(map->isNextTo(unit->getPos(), store)){

					//update resources
					int resourceAmount= unit->getLoadCount();
					if(unit->getFaction()->getCpuUltraControl()){
						resourceAmount*= ultraResourceFactor;
					}
					if(unit->getFaction()->getCpuMegaControl()){
						resourceAmount*= megaResourceFactor;
					}
					unit->getFaction()->incResourceAmount(unit->getLoadType(), resourceAmount);
					world->getStats()->harvest(unit->getFactionIndex(), resourceAmount);
					scriptManager->onResourceHarvested();

					//if next to a store unload resources
					unit->getPath()->clear();
					unit->setCurrSkill(scStop);
					unit->setLoadCount(0);
				}
			}
			else{
				unit->finishCommand();
			}
		}
	}
	else{
		//if working
		SurfaceCell *sc= map->getSurfaceCell(Map::toSurfCoords(unit->getTargetPos()));
		Resource *r= sc->getResource();

		if (r != NULL) {
			if (!hct->canHarvest(r->getType()) || r->getType() != unit->getLoadType()) {
				// hct has changed to a different harvest command.
				unit->setCurrSkill(hct->getStopLoadedSkillType()); // this is actually the wrong animation
				unit->getPath()->clear();
			}
			else {
				// if there is a resource, continue working, until loaded
				unit->update2();
				if (unit->getProgress2() >= hct->getHitsPerUnit()) {
					if (unit->getLoadCount() < hct->getMaxLoad()) {
						unit->setProgress2(0);
						unit->setLoadCount(unit->getLoadCount() + 1);

						//if resource exausted, then delete it and stop
						if (r->decAmount(1)) {
							const ResourceType *rt = r->getType();
							sc->deleteResource();

							switch(this->game->getGameSettings()->getPathFinderType()) {
								case pfBasic:
									break;
								case pfRoutePlanner:
									world->getCartographer()->onResourceDepleted(Map::toSurfCoords(unit->getTargetPos()), rt);
									break;
								default:
									throw runtime_error("detected unsupported pathfinder type!");
							}

							unit->setCurrSkill(hct->getStopLoadedSkillType());
						}
					}
					if (unit->getLoadCount() >= hct->getMaxLoad()) {
						unit->setCurrSkill(hct->getStopLoadedSkillType());
						unit->getPath()->clear();
					}
				}
			}
		}
		else{
			//if there is no resource, just stop
			unit->setCurrSkill(hct->getStopLoadedSkillType());
		}

	}

    if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
	if(chrono.getMillis() > 0) chrono.start();
}


// ==================== updateRepair ====================

void UnitUpdater::updateRepair(Unit *unit){
	Chrono chrono;
	chrono.start();

    Command *command= unit->getCurrCommand();
    const RepairCommandType *rct= static_cast<const RepairCommandType*>(command->getCommandType());

	Unit *repaired= map->getCell(command->getPos())->getUnit(fLand);
	bool nextToRepaired= repaired!=NULL && map->isNextTo(unit->getPos(), repaired);

	UnitPathInterface *path= unit->getPath();

	if(unit->getCurrSkill()->getClass()!=scRepair || !nextToRepaired){
        //if not repairing
        if(repaired!=NULL && rct->isRepairableUnitType(repaired->getType()) && repaired->isDamaged()){

			if(nextToRepaired){
				unit->setTarget(repaired);
                unit->setCurrSkill(rct->getRepairSkillType());
			}
			else {
				TravelState ts;
	    		switch(this->game->getGameSettings()->getPathFinderType()) {
	    			case pfBasic:
	    				ts = pathFinder->findPath(unit, command->getPos());
	    				break;
	    			case pfRoutePlanner:
						if (repaired && !repaired->getType()->isMobile()) {
							ts = routePlanner->findPathToBuildSite(unit, repaired->getType(), repaired->getPos(), repaired->getModelFacing());
						}
						else {
							ts = routePlanner->findPath(unit, command->getPos());
						}
	    				break;
	    			default:
	    				throw runtime_error("detected unsupported pathfinder type!");
	    	    }

				switch(ts) {
				case tsMoving:
					unit->setCurrSkill(rct->getMoveSkillType());
					break;
				case tsBlocked:
					if(unit->getPath()->isBlocked()){
						unit->finishCommand();
					}
					break;
				default:
					break;
				}
			}
        }
        else{
            unit->setCurrSkill(scStop);
            unit->finishCommand();
        }
    }
    else{
        //if repairing
		if(repaired!=NULL){
			unit->setTarget(repaired);
		}
		if(repaired==NULL || repaired->repair()){
            unit->setCurrSkill(scStop);
            unit->finishCommand();
			if(repaired!=NULL && !repaired->isBuilt()){
				repaired->born();
				scriptManager->onUnitCreated(repaired);
			}
        }
    }

    if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
	if(chrono.getMillis() > 0) chrono.start();
}


// ==================== updateProduce ====================

void UnitUpdater::updateProduce(Unit *unit){
	Chrono chrono;
	chrono.start();

    Command *command= unit->getCurrCommand();
    const ProduceCommandType *pct= static_cast<const ProduceCommandType*>(command->getCommandType());
    Unit *produced;

    if(unit->getCurrSkill()->getClass()!=scProduce){
        //if not producing
        unit->setCurrSkill(pct->getProduceSkillType());
    }
    else{
		unit->update2();
        if(unit->getProgress2()>pct->getProduced()->getProductionTime()){
            unit->finishCommand();
            unit->setCurrSkill(scStop);

			UnitPathInterface *newpath = NULL;
			switch(this->game->getGameSettings()->getPathFinderType()) {
				case pfBasic:
					newpath = new UnitPathBasic();
					break;
				case pfRoutePlanner:
					newpath = new UnitPath();
					break;
				default:
					throw runtime_error("detected unsupported pathfinder type!");
		    }

			produced= new Unit(world->getNextUnitId(unit->getFaction()), newpath, Vec2i(0), pct->getProducedUnit(), unit->getFaction(), world->getMap(), CardinalDir::NORTH);

			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] about to place unit for unit [%s]\n",__FILE__,__FUNCTION__,__LINE__,produced->toString().c_str());

			//place unit creates the unit
			if(!world->placeUnit(unit->getCenteredPos(), 10, produced)) {
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] COULD NOT PLACE UNIT for unitID [%d]\n",__FILE__,__FUNCTION__,__LINE__,produced->getId());

				delete produced;
			}
			else{
				produced->create();
				produced->born();
				world->getStats()->produce(unit->getFactionIndex());
				const CommandType *ct= produced->computeCommandType(unit->getMeetingPos());
				if(ct!=NULL){
				    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
					produced->giveCommand(new Command(ct, unit->getMeetingPos()));
				}
				scriptManager->onUnitCreated(produced);
			}
        }
    }

    if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
	if(chrono.getMillis() > 0) chrono.start();
}


// ==================== updateUpgrade ====================

void UnitUpdater::updateUpgrade(Unit *unit){
	Chrono chrono;
	chrono.start();

    Command *command= unit->getCurrCommand();
    const UpgradeCommandType *uct= static_cast<const UpgradeCommandType*>(command->getCommandType());

	if(unit->getCurrSkill()->getClass()!=scUpgrade){
		//if not producing
		unit->setCurrSkill(uct->getUpgradeSkillType());
    }
	else{
		//if producing
		unit->update2();
        if(unit->getProgress2()>uct->getProduced()->getProductionTime()){
            unit->finishCommand();
            unit->setCurrSkill(scStop);
			unit->getFaction()->finishUpgrade(uct->getProducedUpgrade());
        }
    }

    if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
	if(chrono.getMillis() > 0) chrono.start();
}

// ==================== updateMorph ====================

void UnitUpdater::updateMorph(Unit *unit){
	Chrono chrono;
	chrono.start();

    Command *command= unit->getCurrCommand();
    const MorphCommandType *mct= static_cast<const MorphCommandType*>(command->getCommandType());

    if(unit->getCurrSkill()->getClass()!=scMorph){
		//if not morphing, check space
		if(map->isFreeCellsOrHasUnit(unit->getPos(), mct->getMorphUnit()->getSize(), unit->getCurrField(), unit)){
			unit->setCurrSkill(mct->getMorphSkillType());
		}
		else{
			if(unit->getFactionIndex()==world->getThisFactionIndex()){
				console->addStdMessage("InvalidPosition");
			}
			unit->cancelCommand();
		}
    }
    else{
		unit->update2();
        if(unit->getProgress2()>mct->getProduced()->getProductionTime()){
			int oldSize = 0;
			bool needMapUpdate = false;

    		switch(this->game->getGameSettings()->getPathFinderType()) {
    			case pfBasic:
    				break;
    			case pfRoutePlanner:
    				oldSize = unit->getType()->getSize();
    				needMapUpdate = unit->getType()->isMobile() != mct->getMorphUnit()->isMobile();
    				break;
    			default:
    				throw runtime_error("detected unsupported pathfinder type!");
    	    }

			//finish the command
			if(unit->morph(mct)){
				unit->finishCommand();
				if(gui->isSelected(unit)){
					gui->onSelectionChanged();
				}
	    		switch(this->game->getGameSettings()->getPathFinderType()) {
	    			case pfBasic:
	    				break;
	    			case pfRoutePlanner:
						if (needMapUpdate) {
							int size = std::max(oldSize, unit->getType()->getSize());
							world->getCartographer()->updateMapMetrics(unit->getPos(), size);
						}
	    				break;
	    			default:
	    				throw runtime_error("detected unsupported pathfinder type!");
	    	    }

				scriptManager->onUnitCreated(unit);
			}
			else{
				unit->cancelCommand();
				if(unit->getFactionIndex()==world->getThisFactionIndex()){
					console->addStdMessage("InvalidPosition");
				}
			}
			unit->setCurrSkill(scStop);

        }
    }

    if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
	if(chrono.getMillis() > 0) chrono.start();
}

// ==================== PRIVATE ====================

// ==================== attack ====================

void UnitUpdater::hit(Unit *attacker){
	hit(attacker, static_cast<const AttackSkillType*>(attacker->getCurrSkill()), attacker->getTargetPos(), attacker->getTargetField());
}

void UnitUpdater::hit(Unit *attacker, const AttackSkillType* ast, const Vec2i &targetPos, Field targetField){
	Chrono chrono;
	chrono.start();

	//hit attack positions
	if(ast->getSplash()){
		PosCircularIterator pci(map, targetPos, ast->getSplashRadius());
		while(pci.next()){
			Unit *attacked= map->getCell(pci.getPos())->getUnit(targetField);
			if(attacked!=NULL){
				if(ast->getSplashDamageAll()
					|| !attacker->isAlly(attacked)
					|| ( targetPos.x==pci.getPos().x && targetPos.y==pci.getPos().y )){
					damage(attacker, ast, attacked, pci.getPos().dist(attacker->getTargetPos()));
			  	}
			}
		}
	}
	else{
		Unit *attacked= map->getCell(targetPos)->getUnit(targetField);
		if(attacked!=NULL){
			damage(attacker, ast, attacked, 0.f);
		}
	}

    if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
	if(chrono.getMillis() > 0) chrono.start();
}

void UnitUpdater::damage(Unit *attacker, const AttackSkillType* ast, Unit *attacked, float distance){
	Chrono chrono;
	chrono.start();

	//get vars
	float damage= ast->getTotalAttackStrength(attacker->getTotalUpgrade());
	int var= ast->getAttackVar();
	int armor= attacked->getType()->getTotalArmor(attacked->getTotalUpgrade());
	float damageMultiplier= world->getTechTree()->getDamageMultiplier(ast->getAttackType(), attacked->getType()->getArmorType());

	//compute damage
	damage+= random.randRange(-var, var);
	damage/= distance+1;
	damage-= armor;
	damage*= damageMultiplier;
	if(damage<1){
		damage= 1;
	}

	//damage the unit
	if(attacked->decHp(static_cast<int>(damage))){
		world->getStats()->kill(attacker->getFactionIndex(), attacked->getFactionIndex());
		attacker->incKills();

		switch(this->game->getGameSettings()->getPathFinderType()) {
			case pfBasic:
				break;
			case pfRoutePlanner:
				if (!attacked->getType()->isMobile()) {
					world->getCartographer()->updateMapMetrics(attacked->getPos(), attacked->getType()->getSize());
				}
				break;
			default:
				throw runtime_error("detected unsupported pathfinder type!");
	    }
		scriptManager->onUnitDied(attacked);
	}

    if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
	if(chrono.getMillis() > 0) chrono.start();
}

void UnitUpdater::startAttackParticleSystem(Unit *unit){
	Renderer &renderer= Renderer::getInstance();

	ProjectileParticleSystem *psProj = 0;
	SplashParticleSystem *psSplash;

	const AttackSkillType *ast= static_cast<const AttackSkillType*>(unit->getCurrSkill());
	ParticleSystemTypeProjectile *pstProj= ast->getProjParticleType();
	ParticleSystemTypeSplash *pstSplash= ast->getSplashParticleType();

	Vec3f startPos= unit->getCurrVector();
	Vec3f endPos= unit->getTargetVec();

	//make particle system
	const SurfaceCell *sc= map->getSurfaceCell(Map::toSurfCoords(unit->getPos()));
	const SurfaceCell *tsc= map->getSurfaceCell(Map::toSurfCoords(unit->getTargetPos()));
	bool visible= sc->isVisible(world->getThisTeamIndex()) || tsc->isVisible(world->getThisTeamIndex());

	//projectile
	if(pstProj!=NULL){
		psProj= pstProj->create();
		psProj->setPath(startPos, endPos);
		psProj->setObserver(new ParticleDamager(unit, this, gameCamera));
		psProj->setVisible(visible);
		psProj->setFactionColor(unit->getFaction()->getTexture()->getPixmap()->getPixel3f(0,0));
		renderer.manageParticleSystem(psProj, rsGame);
	}
	else{
		hit(unit);
	}

	//splash
	if(pstSplash!=NULL){
		psSplash= pstSplash->create();
		psSplash->setPos(endPos);
		psSplash->setVisible(visible);
		psSplash->setFactionColor(unit->getFaction()->getTexture()->getPixmap()->getPixel3f(0,0));
		renderer.manageParticleSystem(psSplash, rsGame);
		if(pstProj!=NULL){
			psProj->link(psSplash);
		}
	}
}

// ==================== misc ====================

//looks for a resource of type rt, if rt==NULL looks for any
//resource the unit can harvest
bool UnitUpdater::searchForResource(Unit *unit, const HarvestCommandType *hct){
	Chrono chrono;
	chrono.start();

    Vec2i pos= unit->getCurrCommand()->getPos();

    for(int radius= 0; radius<maxResSearchRadius; radius++){
        for(int i=pos.x-radius; i<=pos.x+radius; ++i){
            for(int j=pos.y-radius; j<=pos.y+radius; ++j){
				if(map->isInside(i, j)){
					Resource *r= map->getSurfaceCell(Map::toSurfCoords(Vec2i(i, j)))->getResource();
                    if(r!=NULL){
						if(hct->canHarvest(r->getType())){
							unit->getCurrCommand()->setPos(Vec2i(i, j));

						    if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
							if(chrono.getMillis() > 0) chrono.start();

							return true;
                        }
                    }
                }
            }
        }
    }

    if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
	if(chrono.getMillis() > 0) chrono.start();

    return false;
}

bool UnitUpdater::attackerOnSight(const Unit *unit, Unit **rangedPtr){
	int range= unit->getType()->getSight();
	return unitOnRange(unit, range, rangedPtr, NULL);
}

bool UnitUpdater::attackableOnSight(const Unit *unit, Unit **rangedPtr, const AttackSkillType *ast){
	int range= unit->getType()->getSight();
	return unitOnRange(unit, range, rangedPtr, ast);
}

bool UnitUpdater::attackableOnRange(const Unit *unit, Unit **rangedPtr, const AttackSkillType *ast){
	int range= ast->getTotalAttackRange(unit->getTotalUpgrade());
	return unitOnRange(unit, range, rangedPtr, ast);
}

bool UnitUpdater::findCachedCellsEnemies(Vec2i center, int range, int size, vector<Unit*> &enemies,
										 const AttackSkillType *ast, const Unit *unit,
										 const Unit *commandTarget) {
	bool result = false;
	//return result;

	std::map<Vec2i, std::map<int, std::map<int, UnitRangeCellsLookupItem > > >::iterator iterFind = UnitRangeCellsLookupItemCache.find(center);
	if(iterFind != UnitRangeCellsLookupItemCache.end()) {
		std::map<int, std::map<int, UnitRangeCellsLookupItem > >::iterator iterFind3 = iterFind->second.find(size);
		if(iterFind3 != iterFind->second.end()) {
			std::map<int, UnitRangeCellsLookupItem>::iterator iterFind4 = iterFind3->second.find(range);
			if(iterFind4 != iterFind3->second.end()) {
				result = true;

				std::vector<Cell *> &cellList = iterFind4->second.rangeCellList;
				for(int idx = 0; idx < cellList.size(); ++idx) {
					Cell *cell = cellList[idx];

					findEnemiesForCell(ast,cell,unit,commandTarget,enemies);
				}
			}
		}
	}

	return result;
}

void UnitUpdater::findEnemiesForCell(const AttackSkillType *ast, Cell *cell, const Unit *unit,
									 const Unit *commandTarget,vector<Unit*> &enemies) {
	//all fields
	for(int k=0; k<fieldCount; k++) {
		Field f= static_cast<Field>(k);

		//check field
		if((ast == NULL || ast->getAttackField(f))) {
			Unit *possibleEnemy = cell->getUnit(f);

			//check enemy
			if(possibleEnemy != NULL && possibleEnemy->isAlive()) {
				if((!unit->isAlly(possibleEnemy) && commandTarget == NULL) ||
					commandTarget == possibleEnemy) {
					enemies.push_back(possibleEnemy);
				}
			}
		}
	}
}

//if the unit has any enemy on range
bool UnitUpdater::unitOnRange(const Unit *unit, int range, Unit **rangedPtr,
							  const AttackSkillType *ast){
	Chrono chrono;
	chrono.start();

    vector<Unit*> enemies;

	//we check command target
	const Unit *commandTarget = NULL;
	if(unit->anyCommand()) {
		commandTarget = static_cast<const Unit*>(unit->getCurrCommand()->getUnit());
	}
	if(commandTarget != NULL && commandTarget->isDead()) {
		commandTarget = NULL;
	}

	//aux vars
	int size 			= unit->getType()->getSize();
	Vec2i center 		= unit->getPos();
	Vec2f floatCenter	= unit->getFloatCenteredPos();

	bool foundInCache = true;
	if(findCachedCellsEnemies(center,range,size,enemies,ast,
							  unit,commandTarget) == false) {
		foundInCache = false;

		//nearby cells
		UnitRangeCellsLookupItem cacheItem;
		for(int i=center.x-range; i<center.x+range+size; ++i){
			for(int j=center.y-range; j<center.y+range+size; ++j){

				//cells inside map and in range
#ifdef USE_STREFLOP
				if(map->isInside(i, j) && streflop::floor(floatCenter.dist(Vec2f((float)i, (float)j))) <= (range+1)){
#else
				if(map->isInside(i, j) && floor(floatCenter.dist(Vec2f((float)i, (float)j))) <= (range+1)){
#endif
					Cell *cell = map->getCell(i,j);
					findEnemiesForCell(ast,cell,unit,commandTarget,enemies);

					cacheItem.rangeCellList.push_back(cell);
				}
			}
		}

		// Ok update our caches with the latest info
		if(cacheItem.rangeCellList.size() > 0) {
			cacheItem.UnitRangeCellsLookupItemCacheTimerCountIndex = UnitRangeCellsLookupItemCacheTimerCount++;
			UnitRangeCellsLookupItemCache[center][size][range] = cacheItem;
		}
	}

	//attack enemies that can attack first
    for(int i=0; i<enemies.size(); ++i){
		if(enemies[i]->getType()->hasSkillClass(scAttack)){
            *rangedPtr= enemies[i];

            if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld, unit %s, range = %d, size = %d, foundInCache = %d, enemies.size() = %d\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis(),unit->getFullName().c_str(),range,size,foundInCache,enemies.size());
        	if(chrono.getMillis() > 0) chrono.start();

            return true;
        }
    }

	//any enemy
    if(enemies.size()>0){
        *rangedPtr= enemies.front();

        if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld, unit %s, range = %d, size = %d, foundInCache = %d, enemies.size() = %d\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis(),unit->getFullName().c_str(),range,size,foundInCache,enemies.size());
    	if(chrono.getMillis() > 0) chrono.start();

        return true;
    }

    if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld, unit %s, range = %d, size = %d, foundInCache = %d, enemies.size() = %d\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis(),unit->getFullName().c_str(),range,size,foundInCache,enemies.size());
	if(chrono.getMillis() > 0) chrono.start();

    return false;
}

// =====================================================
//	class ParticleDamager
// =====================================================

ParticleDamager::ParticleDamager(Unit *attacker, UnitUpdater *unitUpdater, const GameCamera *gameCamera){
	this->gameCamera= gameCamera;
	this->attackerRef= attacker;
	this->ast= static_cast<const AttackSkillType*>(attacker->getCurrSkill());
	this->targetPos= attacker->getTargetPos();
	this->targetField= attacker->getTargetField();
	this->unitUpdater= unitUpdater;
}

void ParticleDamager::update(ParticleSystem *particleSystem){
	Unit *attacker= attackerRef.getUnit();

	if(attacker!=NULL){

		unitUpdater->hit(attacker, ast, targetPos, targetField);

		//play sound
		StaticSound *projSound= ast->getProjSound();
		if(particleSystem->getVisible() && projSound!=NULL){
			SoundRenderer::getInstance().playFx(projSound, attacker->getCurrVector(), gameCamera->getPos());
		}
	}
	particleSystem->setObserver(NULL);
	delete this;
}

}}//end namespace
