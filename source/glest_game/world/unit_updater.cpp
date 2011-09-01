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
	//UnitRangeCellsLookupItemCacheTimerCount = 0;
	attackWarnRange=0;
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
	attackWarnRange=Config::getInstance().getFloat("AttackWarnRange","50.0");
	//UnitRangeCellsLookupItemCacheTimerCount = 0;

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

void UnitUpdater::clearUnitPrecache(Unit *unit) {
	if(pathFinder != NULL) {
		pathFinder->clearUnitPrecache(unit);
	}
}

void UnitUpdater::removeUnitPrecache(Unit *unit) {
	if(pathFinder != NULL) {
		pathFinder->removeUnitPrecache(unit);
	}
}

UnitUpdater::~UnitUpdater() {
	//UnitRangeCellsLookupItemCache.clear();

	delete pathFinder;
	pathFinder = NULL;

	while(attackWarnings.empty() == false) {
			AttackWarningData* awd=attackWarnings.back();
			attackWarnings.pop_back();
			delete awd;
		}
}

// ==================== progress skills ====================

//skill dependent actions
void UnitUpdater::updateUnit(Unit *unit) {
	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld --------------------------- [START OF METHOD] ---------------------------\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	SoundRenderer &soundRenderer= SoundRenderer::getInstance();

	//play skill sound
	const SkillType *currSkill= unit->getCurrSkill();
	if(currSkill->getSound() != NULL) {
		float soundStartTime= currSkill->getSoundStartTime();
		if(soundStartTime >= unit->getLastAnimProgress() && soundStartTime < unit->getAnimProgress()) {
			if(map->getSurfaceCell(Map::toSurfCoords(unit->getPos()))->isVisible(world->getThisTeamIndex()) ||
				(game->getWorld()->getThisTeamIndex() == GameConstants::maxPlayers -1 + fpt_Observer)) {
				soundRenderer.playFx(currSkill->getSound(), unit->getCurrVector(), gameCamera->getPos());
			}
		}
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld [after playsound]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	unit->updateTimedParticles();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld [after playsound]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	//start attack particle system
	if(unit->getCurrSkill()->getClass() == scAttack) {
		const AttackSkillType *ast= static_cast<const AttackSkillType*>(unit->getCurrSkill());
		float attackStartTime= ast->getAttackStartTime();
		if(attackStartTime>=unit->getLastAnimProgress() && attackStartTime<unit->getAnimProgress()){
			startAttackParticleSystem(unit);
		}
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld [after attack particle system]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	bool update = unit->update();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld [after unit->update()]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	//update unit
	if(update == true) {
        //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		updateUnitCommand(unit,-1);

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld [after updateUnitCommand()]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

		//if unit is out of EP, it stops
		if(unit->computeEp()) {
			unit->setCurrSkill(scStop);
			unit->cancelCommand();
		}
		if(unit->getCurrSkill() != NULL && unit->getCurrSkill()->getClass() != scAttack) {
			// !!! Is this causing choppy network play somehow?
			//unit->computeHp();
		}
		else if(unit->getCommandSize() > 0) {
			Command *command= unit->getCurrCommand();
			if(command != NULL) {
				const AttackCommandType *act= dynamic_cast<const AttackCommandType*>(command->getCommandType());
				if( act != NULL && act->getAttackSkillType() != NULL &&
					act->getAttackSkillType()->getSpawnUnit() != "" && act->getAttackSkillType()->getSpawnUnitCount() > 0) {

					const FactionType *ft= unit->getFaction()->getType();
					const UnitType *spawnUnitType = ft->getUnitType(act->getAttackSkillType()->getSpawnUnit());
					int spawnCount = act->getAttackSkillType()->getSpawnUnitCount();
					for (int y=0; y < spawnCount; ++y) {
						if(spawnUnitType->getMaxUnitCount() > 0) {
							if(spawnUnitType->getMaxUnitCount() <= unit->getFaction()->getCountForMaxUnitCount(spawnUnitType)) {
								break;
							}
						}
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

						Unit *spawned= new Unit(world->getNextUnitId(unit->getFaction()), newpath,
								                Vec2i(0), spawnUnitType, unit->getFaction(),
								                world->getMap(), CardinalDir::NORTH);
						//SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] about to place unit for unit [%s]\n",__FILE__,__FUNCTION__,__LINE__,spawned->toString().c_str());
						if(!world->placeUnit(unit->getCenteredPos(), 10, spawned)) {
							//SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] COULD NOT PLACE UNIT for unitID [%d]\n",__FILE__,__FUNCTION__,__LINE__,spawned->getId());

							// This will also cleanup newPath
							delete spawned;
							spawned = NULL;
						}
						else {
							spawned->create();
							spawned->born();
							world->getStats()->produce(unit->getFactionIndex());
							const CommandType *ct= spawned->computeCommandType(command->getPos(),command->getUnit());
							if(ct != NULL){
								if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
								spawned->giveCommand(new Command(ct, unit->getMeetingPos()));
							}
							scriptManager->onUnitCreated(spawned);

							if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
						}
					}
				}
			}
		}
		
		//move unit in cells
		if(unit->getCurrSkill()->getClass() == scMove) {
			world->moveUnitCells(unit);

			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld [after world->moveUnitCells()]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

			//play water sound
			if(map->getCell(unit->getPos())->getHeight() < map->getWaterLevel() && unit->getCurrField() == fLand) {
				if(Config::getInstance().getBool("DisableWaterSounds","false") == false) {
					soundRenderer.playFx(
						CoreData::getInstance().getWaterSound(),
						unit->getCurrVector(),
						gameCamera->getPos()
					);

					if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld [after soundFx()]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
				}
			}
		}
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	//unit death
	if(unit->isDead() && unit->getCurrSkill()->getClass() != scDie) {
		unit->kill();
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld --------------------------- [END OF METHOD] ---------------------------\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
}

// ==================== progress commands ====================

//VERY IMPORTANT: compute next state depending on the first order of the list
void UnitUpdater::updateUnitCommand(Unit *unit, int frameIndex) {
	// Clear previous cached unit data
	if(frameIndex >= 0) {
		clearUnitPrecache(unit);
	}

	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

	//if unit has command process it
    if(unit->anyCommand()) {
    	if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] unit [%s] has command [%s]\n",__FILE__,__FUNCTION__,__LINE__,unit->toString().c_str(), unit->getCurrCommand()->toString().c_str());

    	CommandClass cc = unit->getCurrCommand()->getCommandType()->commandTypeClass;
		unit->getCurrCommand()->getCommandType()->update(this, unit, frameIndex);
	}

    if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

    if(frameIndex < 0) {
		//if no commands stop and add stop command
		if(unit->anyCommand() == false && unit->isOperative()) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			if(unit->getType()->hasSkillClass(scStop)) {
				unit->setCurrSkill(scStop);
			}

			if(unit->getType()->hasCommandClass(ccStop)) {
				unit->giveCommand(new Command(unit->getType()->getFirstCtOfClass(ccStop)));
			}
		}
    }
    if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld --------------------------- [END OF METHOD] ---------------------------\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
}

// ==================== updateStop ====================

void UnitUpdater::updateStop(Unit *unit, int frameIndex) {
	// Nothing to do
	if(frameIndex >= 0) {
		clearUnitPrecache(unit);
		return;
	}

	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

	Command *command= unit->getCurrCommand();
    const StopCommandType *sct = static_cast<const StopCommandType*>(command->getCommandType());
    Unit *sighted=NULL;

    unit->setCurrSkill(sct->getStopSkillType());

    if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	//we can attack any unit => attack it
   	if(unit->getType()->hasSkillClass(scAttack)) {
   		int cmdTypeCount = unit->getType()->getCommandTypeCount();

		for(int i = 0; i < cmdTypeCount; ++i) {
			const CommandType *ct= unit->getType()->getCommandType(i);

			//look for an attack skill
			const AttackSkillType *ast= NULL;
			if(ct->getClass() == ccAttack) {
				ast= static_cast<const AttackCommandType*>(ct)->getAttackSkillType();
			}
			else if(ct->getClass() == ccAttackStopped) {
				ast= static_cast<const AttackStoppedCommandType*>(ct)->getAttackSkillType();
			}

			//use it to attack
			if(ast != NULL) {
				if(attackableOnSight(unit, &sighted, ast)) {
				    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
					unit->giveCommand(new Command(ct, sighted->getPos()));
					break;
				}
			}
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
	}
	//see any unit and cant attack it => run
	else if(unit->getType()->hasCommandClass(ccMove)) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

		if(attackerOnSight(unit, &sighted)) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
			Vec2i escapePos = unit->getPos() * 2 - sighted->getPos();
			//SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			unit->giveCommand(new Command(unit->getType()->getFirstCtOfClass(ccMove), escapePos));
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
	}

   	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld --------------------------- [END OF METHOD] ---------------------------\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
}


// ==================== updateMove ====================
void UnitUpdater::updateMove(Unit *unit, int frameIndex) {
	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

    Command *command= unit->getCurrCommand();
    const MoveCommandType *mct= static_cast<const MoveCommandType*>(command->getCommandType());

	Vec2i pos= command->getUnit()!=NULL? command->getUnit()->getCenteredPos(): command->getPos();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
		char szBuf[4096]="";
		sprintf(szBuf,"[updateMove] pos [%s] unit [%d - %s] cmd [%s]",pos.getString().c_str(),unit->getId(),unit->getFullName().c_str(),command->toString().c_str());
		unit->logSynchData(__FILE__,__LINE__,szBuf);
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	TravelState tsValue = tsImpossible;
	switch(this->game->getGameSettings()->getPathFinderType()) {
		case pfBasic:
			tsValue = pathFinder->findPath(unit, pos, NULL, frameIndex);
			break;
		case pfRoutePlanner:
			tsValue = routePlanner->findPath(unit, pos);
			break;
		default:
			throw runtime_error("detected unsupported pathfinder type!");
    }

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	if(frameIndex < 0) {
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
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld --------------------------- [END OF METHOD] ---------------------------\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
}


// ==================== updateAttack ====================

void UnitUpdater::updateAttack(Unit *unit, int frameIndex) {
	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

	Command *command= unit->getCurrCommand();
    const AttackCommandType *act= static_cast<const AttackCommandType*>(command->getCommandType());
	Unit *target= NULL;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	if( (command->getUnit() == NULL || !(command->getUnit()->isAlive()) ) && unit->getCommandSize() > 1) {

		if(frameIndex < 0) {
			unit->finishCommand(); // all queued "ground attacks" are skipped if somthing else is queued after them.
		}
		return;
	}

	//if found
    if(attackableOnRange(unit, &target, act->getAttackSkillType())) {
    	if(frameIndex < 0) {
			if(unit->getEp() >= act->getAttackSkillType()->getEpCost()) {
				unit->setCurrSkill(act->getAttackSkillType());
				unit->setTarget(target);
			}
			else {
				unit->setCurrSkill(scStop);
			}
    	}
    	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
    }
    else {
		//compute target pos
		Vec2i pos;
		if(command->getUnit() != NULL) {
			pos= command->getUnit()->getCenteredPos();
		}
		else if(attackableOnSight(unit, &target, act->getAttackSkillType())) {
			pos= target->getPos();
		}
		else {
			pos= command->getPos();
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
			char szBuf[4096]="";
			sprintf(szBuf,"[updateAttack] pos [%s] unit->getPos() [%s]",
					pos.getString().c_str(),unit->getPos().getString().c_str());
			unit->logSynchData(__FILE__,__LINE__,szBuf);
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

		TravelState tsValue = tsImpossible;
		switch(this->game->getGameSettings()->getPathFinderType()) {
			case pfBasic:
				tsValue = pathFinder->findPath(unit, pos, NULL, frameIndex);
				break;
			case pfRoutePlanner:
				tsValue = routePlanner->findPath(unit, pos);
				break;
			default:
				throw runtime_error("detected unsupported pathfinder type!");
	    }

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

		if(frameIndex < 0) {
			if(command->getUnit() != NULL && !command->getUnit()->isAlive() && unit->getCommandSize() > 1) {
				// don't run over to dead body if there is still something to do in the queue
				unit->finishCommand();
			}
			else {
				//if unit arrives destPos order has ended
				switch (tsValue) {
					case tsMoving:
						unit->setCurrSkill(act->getMoveSkillType());
						break;
					case tsBlocked:
						if(unit->getPath()->isBlocked()) {
							unit->finishCommand();
						}
						break;
					default:
						unit->finishCommand();
					}
/*
					case tsMoving:
						unit->setCurrSkill(act->getMoveSkillType());

						{
							std::pair<bool,Unit *> beingAttacked = unitBeingAttacked(unit);
							if(beingAttacked.first == true) {
								Unit *enemy = beingAttacked.second;
								const AttackCommandType *act_forenemy = unit->getType()->getFirstAttackCommand(enemy->getCurrField());
								if(act_forenemy != NULL) {
									if(unit->getEp() >= act_forenemy->getAttackSkillType()->getEpCost()) {
										unit->setCurrSkill(act_forenemy->getAttackSkillType());
										unit->setTarget(enemy);
									}
									//aiInterface->giveCommand(i, act_forenemy, beingAttacked.second->getPos());
								}
								else {
									const AttackStoppedCommandType *asct_forenemy = unit->getType()->getFirstAttackStoppedCommand(enemy->getCurrField());
									if(asct_forenemy != NULL) {
										//aiInterface->giveCommand(i, asct_forenemy, beingAttacked.second->getCenteredPos());
										if(unit->getEp() >= asct_forenemy->getAttackSkillType()->getEpCost()) {
											unit->setCurrSkill(asct_forenemy->getAttackSkillType());
											unit->setTarget(enemy);
										}
									}
								}
							}
						}

						break;

					case tsBlocked:
						if(unit->getPath()->isBlocked()){
							unit->finishCommand();
						}
						break;
					default:
						unit->finishCommand();
				}
*/
			}
		}
		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
    }

    if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld --------------------------- [END OF METHOD] ---------------------------\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
}


// ==================== updateAttackStopped ====================

void UnitUpdater::updateAttackStopped(Unit *unit, int frameIndex) {
	// Nothing to do
	if(frameIndex >= 0) {
		clearUnitPrecache(unit);
		return;
	}

	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

	Command *command= unit->getCurrCommand();
    const AttackStoppedCommandType *asct= static_cast<const AttackStoppedCommandType*>(command->getCommandType());
    Unit *enemy=NULL;

    if(unit->getCommandSize() > 1)
    {
    	unit->finishCommand(); // attackStopped is skipped if somthing else is queued after this.
    	return;
    }

    float distToUnit=-1;
    std::pair<bool,Unit *> result = make_pair(false,(Unit *)NULL);
    unitBeingAttacked(result, unit, asct->getAttackSkillType(), &distToUnit);
	if(result.first == true) {
        unit->setCurrSkill(asct->getAttackSkillType());
		unit->setTarget(result.second);
	}
	else if(attackableOnRange(unit, &enemy, asct->getAttackSkillType())) {
        unit->setCurrSkill(asct->getAttackSkillType());
		unit->setTarget(enemy);
    }
    else {
        unit->setCurrSkill(asct->getStopSkillType());
    }

    if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld --------------------------- [END OF METHOD] ---------------------------\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
}

void UnitUpdater::unitBeingAttacked(std::pair<bool,Unit *> &result, const Unit *unit, const AttackSkillType *ast, float *currentDistToUnit) {
	//std::pair<bool,Unit *> result = make_pair(false,(Unit *)NULL);

	float distToUnit = -1;
	if(currentDistToUnit != NULL) {
		distToUnit = *currentDistToUnit;
	}
	if(ast != NULL) {
		vector<Unit*> enemies = enemyUnitsOnRange(unit,ast);
		for(unsigned j = 0; j < enemies.size(); ++j) {
			Unit *enemy = enemies[j];

			//printf("~~~~~~~~ Unit [%s - %d] enemy # %d found enemy [%s - %d] distToUnit = %f\n",unit->getFullName().c_str(),unit->getId(),j,enemy->getFullName().c_str(),enemy->getId(),unit->getCenteredPos().dist(enemy->getCenteredPos()));

			if(distToUnit < 0 || unit->getCenteredPos().dist(enemy->getCenteredPos()) < distToUnit) {
				distToUnit = unit->getCenteredPos().dist(enemy->getCenteredPos());
				if(ast->getAttackRange() >= distToUnit){
					result.first= true;
					result.second= enemy;
					break;
				}
			}
		}
	}

	if(currentDistToUnit != NULL) {
		*currentDistToUnit = distToUnit;
	}

//	if(result.first == true) {
//		printf("~~~~~~~~ Unit [%s - %d] found enemy [%s - %d] distToUnit = %f\n",unit->getFullName().c_str(),unit->getId(),result.second->getFullName().c_str(),result.second->getId(),distToUnit);
//	}
    //return result;
}

std::pair<bool,Unit *> UnitUpdater::unitBeingAttacked(const Unit *unit) {
	std::pair<bool,Unit *> result = make_pair(false,(Unit *)NULL);

	float distToUnit = -1;
	for(unsigned int i = 0; i < unit->getType()->getSkillTypeCount(); ++i) {
		const SkillType *st = unit->getType()->getSkillType(i);
		const AttackSkillType *ast = dynamic_cast<const AttackSkillType *>(st);
		unitBeingAttacked(result, unit, ast, &distToUnit);
	}

//	if(result.first == true) {
//		printf("~~~~~~~~ Unit [%s - %d] found enemy [%s - %d] distToUnit = %f\n",unit->getFullName().c_str(),unit->getId(),result.second->getFullName().c_str(),result.second->getId(),distToUnit);
//	}
    return result;
}

// ==================== updateBuild ====================

void UnitUpdater::updateBuild(Unit *unit, int frameIndex) {
	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] unit [%s] will build using command [%s]\n",__FILE__,__FUNCTION__,__LINE__,unit->toString().c_str(), unit->getCurrCommand()->toString().c_str());

	Command *command= unit->getCurrCommand();
    const BuildCommandType *bct= static_cast<const BuildCommandType*>(command->getCommandType());

	//std::pair<float,Vec2i> distance = map->getUnitDistanceToPos(unit,command->getPos(),command->getUnitType());
	//unit->setCurrentUnitTitle("Distance: " + floatToStr(distance.first) + " build pos: " + distance.second.getString() + " current pos: " + unit->getPos().getString());

	if(unit->getCurrSkill()->getClass() != scBuild) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

        //if not building
        const UnitType *ut= command->getUnitType();

        if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

		TravelState tsValue = tsImpossible;
		switch(this->game->getGameSettings()->getPathFinderType()) {
			case pfBasic:
				{
				//Vec2i buildPos = (command->getPos()-Vec2i(1));
				Vec2i buildPos = map->findBestBuildApproach(unit, command->getPos(), ut);
				//Vec2i buildPos = (command->getPos() + Vec2i(ut->getSize() / 2));

				if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
					char szBuf[4096]="";
					sprintf(szBuf,"[updateBuild] unit->getPos() [%s] command->getPos() [%s] buildPos [%s]",
							unit->getPos().getString().c_str(),command->getPos().getString().c_str(),buildPos.getString().c_str());
					unit->logSynchData(__FILE__,__LINE__,szBuf);
				}

				tsValue = pathFinder->findPath(unit, buildPos, NULL, frameIndex);
				}
				break;
			case pfRoutePlanner:
				tsValue = routePlanner->findPathToBuildSite(unit, ut, command->getPos(), command->getFacing());
				break;
			default:
				throw runtime_error("detected unsupported pathfinder type!");
	    }

		if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] tsValue = %d\n",__FILE__,__FUNCTION__,__LINE__,tsValue);

		if(frameIndex < 0) {
			switch (tsValue) {
			case tsMoving:
				if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] tsMoving\n",__FILE__,__FUNCTION__,__LINE__);

				unit->setCurrSkill(bct->getMoveSkillType());
				break;

			case tsArrived:
				{
				if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] tsArrived:\n",__FILE__,__FUNCTION__,__LINE__);

				//if arrived destination
				assert(ut);
				if(ut == NULL) {
					throw runtime_error("ut == NULL");
				}

				bool canOccupyCell = false;
				switch(this->game->getGameSettings()->getPathFinderType()) {
					case pfBasic:
						if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] tsArrived about to call map->isFreeCells() for command->getPos() = %s, ut->getSize() = %d\n",__FILE__,__FUNCTION__,__LINE__,command->getPos().getString().c_str(),ut->getSize());
						canOccupyCell = map->isFreeCells(command->getPos(), ut->getSize(), fLand);
						break;
					case pfRoutePlanner:
						canOccupyCell = map->canOccupy(command->getPos(), ut->getField(), ut, command->getFacing());
						break;
					default:
						throw runtime_error("detected unsupported pathfinder type!");
				}

				if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] canOccupyCell = %d\n",__FILE__,__FUNCTION__,__LINE__,canOccupyCell);

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

					Vec2i buildPos = command->getPos();
					Unit *builtUnit= new Unit(world->getNextUnitId(unit->getFaction()), newpath, buildPos, builtUnitType, unit->getFaction(), world->getMap(), facing);

					if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

					builtUnit->create();

					if(builtUnitType->hasSkillClass(scBeBuilt) == false) {
						throw runtime_error("Unit [" + builtUnitType->getName() + "] has no be_built skill, producer was [" + intToStr(unit->getId()) + " - " + unit->getType()->getName() + "].");
					}

					builtUnit->setCurrSkill(scBeBuilt);

					unit->setCurrSkill(bct->getBuildSkillType());
					unit->setTarget(builtUnit);
					map->prepareTerrain(builtUnit);

					if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

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
					if(unit->getFactionIndex() == world->getThisFactionIndex() ||
						(game->getWorld()->getThisTeamIndex() == GameConstants::maxPlayers -1 + fpt_Observer)) {
						SoundRenderer::getInstance().playFx(
							bct->getStartSound(),
							unit->getCurrVector(),
							gameCamera->getPos());
					}

					if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] unit created for unit [%s]\n",__FILE__,__FUNCTION__,__LINE__,builtUnit->toString().c_str());
				}
				else {
					//if there are no free cells
					unit->cancelCommand();
					unit->setCurrSkill(scStop);

					if(unit->getFactionIndex() == world->getThisFactionIndex()) {
						 console->addStdMessage("BuildingNoPlace");
					}

					if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] got BuildingNoPlace\n",__FILE__,__FUNCTION__,__LINE__);
				}
				}
				break;

			case tsBlocked:
				if(unit->getPath()->isBlocked()) {
					unit->cancelCommand();

					if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] got tsBlocked\n",__FILE__,__FUNCTION__,__LINE__);
				}
				break;
			}
		}
		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
    }
    else {
    	if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] tsArrived unit = %s\n",__FILE__,__FUNCTION__,__LINE__,unit->toString().c_str());

    	if(frameIndex < 0) {
			//if building
			Unit *builtUnit = map->getCell(unit->getTargetPos())->getUnit(fLand);
			if(builtUnit == NULL) {
				builtUnit = map->getCell(unit->getTargetPos())->getUnitWithEmptyCellMap(fLand);
			}

			if(builtUnit != NULL) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] builtUnit = %s\n",__FILE__,__FUNCTION__,__LINE__,builtUnit->toString().c_str());
			}

			if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] builtUnit = [%p]\n",__FILE__,__FUNCTION__,__LINE__,builtUnit);

			//if unit is killed while building then u==NULL;
			if(builtUnit != NULL && builtUnit != command->getUnit()) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] builtUnit is not the command's unit!\n",__FILE__,__FUNCTION__,__LINE__);
				unit->setCurrSkill(scStop);
			}
			else if(builtUnit == NULL || builtUnit->isBuilt()) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] builtUnit is NULL or ALREADY built\n",__FILE__,__FUNCTION__,__LINE__);

				unit->finishCommand();
				unit->setCurrSkill(scStop);

			}
			else if(builtUnit == NULL || builtUnit->repair()) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

				//building finished
				unit->finishCommand();
				unit->setCurrSkill(scStop);

				builtUnit->born();
				scriptManager->onUnitCreated(builtUnit);
				if(unit->getFactionIndex() == world->getThisFactionIndex() ||
					(game->getWorld()->getThisTeamIndex() == GameConstants::maxPlayers -1 + fpt_Observer)) {
					SoundRenderer::getInstance().playFx(
						bct->getBuiltSound(),
						unit->getCurrVector(),
						gameCamera->getPos());
				}
			}
    	}
    	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
    }

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld --------------------------- [END OF METHOD] ---------------------------\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
}


// ==================== updateHarvest ====================

void UnitUpdater::updateHarvest(Unit *unit, int frameIndex) {
	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

	Command *command= unit->getCurrCommand();
    const HarvestCommandType *hct= static_cast<const HarvestCommandType*>(command->getCommandType());
	Vec2i targetPos(-1);

	TravelState tsValue = tsImpossible;
	//UnitPathInterface *path= unit->getPath();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	if(unit->getCurrSkill()->getClass() != scHarvest) {
		//if not working
		if(unit->getLoadCount() == 0) {
			//if not loaded go for resources
			Resource *r = map->getSurfaceCell(Map::toSurfCoords(command->getPos()))->getResource();
			if(r != NULL && hct->canHarvest(r->getType())) {
				//if can harvest dest. pos
				bool canHarvestDestPos = false;

				if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	    		switch(this->game->getGameSettings()->getPathFinderType()) {
	    			case pfBasic:
	    				{
	    					const bool newHarvestPath = false;
	    					bool isNearResource = false;
	    					Vec2i clickPos = command->getOriginalPos();
	    					if(newHarvestPath == true) {
	    						isNearResource = map->isResourceNear(unit->getPos(), r->getType(), targetPos,unit->getType()->getSize(),unit, false,&clickPos);
	    					}
	    					else {
	    						isNearResource = map->isResourceNear(unit->getPos(), r->getType(), targetPos,unit->getType()->getSize(),unit);
	    					}
	    					if(isNearResource == true) {
	    						if((unit->getPos().dist(command->getPos()) < harvestDistance || unit->getPos().dist(targetPos) < harvestDistance) && isNearResource == true) {
	    							canHarvestDestPos = true;
	    						}
	    					}
	    					else if(newHarvestPath == true) {
	    						if(clickPos != command->getOriginalPos()) {
	    							//printf("%%----------- unit [%s - %d] CHANGING RESOURCE POS from [%s] to [%s]\n",unit->getFullName().c_str(),unit->getId(),command->getOriginalPos().getString().c_str(),clickPos.getString().c_str());

	    							command->setPos(clickPos);
	    						}
	    					}
	    				}
	    				break;
	    			case pfRoutePlanner:
	    				canHarvestDestPos = map->isResourceNear(unit->getPos(), unit->getType()->getSize(), r->getType(), targetPos);
	    				break;
	    			default:
	    				throw runtime_error("detected unsupported pathfinder type!");
	    	    }

	    		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

				if (canHarvestDestPos == true ) {
					if(frameIndex < 0) {
						unit->setLastHarvestResourceTarget(NULL);
					}

					canHarvestDestPos = (map->getSurfaceCell(Map::toSurfCoords(targetPos)) != NULL && map->getSurfaceCell(Map::toSurfCoords(targetPos))->getResource() != NULL && map->getSurfaceCell(Map::toSurfCoords(targetPos))->getResource()->getType() != NULL);

					if(canHarvestDestPos == true) {
						if(frameIndex < 0) {
							//if it finds resources it starts harvesting
							unit->setCurrSkill(hct->getHarvestSkillType());
							unit->setTargetPos(targetPos);
							command->setPos(targetPos);
							unit->setLoadCount(0);
							unit->getFaction()->addResourceTargetToCache(targetPos);

							switch(this->game->getGameSettings()->getPathFinderType()) {
								case pfBasic:
									unit->setLoadType(r->getType());
									break;
								case pfRoutePlanner:
									unit->setLoadType(r->getType());
									break;
								default:
									throw runtime_error("detected unsupported pathfinder type!");
							}
						}
						if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
					}
				}
				if(canHarvestDestPos == false) {
					if(frameIndex < 0) {
						unit->setLastHarvestResourceTarget(&targetPos);
					}

					if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

					if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
						char szBuf[4096]="";
						sprintf(szBuf,"[updateHarvest] unit->getPos() [%s] command->getPos() [%s]",
								unit->getPos().getString().c_str(),command->getPos().getString().c_str());
						unit->logSynchData(__FILE__,__LINE__,szBuf);
					}

					//if not continue walking
					bool wasStuck = false;
					TravelState tsValue = tsImpossible;
		    		switch(this->game->getGameSettings()->getPathFinderType()) {
		    			case pfBasic:
							tsValue = pathFinder->findPath(unit, command->getPos(), &wasStuck, frameIndex);
							if (tsValue == tsMoving && frameIndex < 0) {
								unit->setCurrSkill(hct->getMoveSkillType());
							}
		    				break;
		    			case pfRoutePlanner:
							tsValue = routePlanner->findPathToResource(unit, command->getPos(), r->getType());
							if (tsValue == tsMoving && frameIndex < 0) {
								unit->setCurrSkill(hct->getMoveSkillType());
							}
		    				break;
		    			default:
		    				throw runtime_error("detected unsupported pathfinder type!");
		    	    }

		    		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

		    		// If the unit is blocked or Even worse 'stuck' then try to
		    		// find the same resource type elsewhere, but close by
		    		if((wasStuck == true || tsValue == tsBlocked) && unit->isAlive() == true) {
		    			switch(this->game->getGameSettings()->getPathFinderType()) {
							case pfBasic:
								{
									bool isNearResource = map->isResourceNear(unit->getPos(), r->getType(), targetPos,unit->getType()->getSize(),unit,true);
									if(isNearResource == true) {
										if((unit->getPos().dist(command->getPos()) < harvestDistance || unit->getPos().dist(targetPos) < harvestDistance) && isNearResource == true) {
											canHarvestDestPos = true;
										}
									}
								}
								break;
							case pfRoutePlanner:
								canHarvestDestPos = map->isResourceNear(unit->getPos(), unit->getType()->getSize(), r->getType(), targetPos);
								break;
							default:
								throw runtime_error("detected unsupported pathfinder type!");
						}

						if (canHarvestDestPos == true) {
							if(frameIndex < 0) {
								unit->setLastHarvestResourceTarget(NULL);
							}

							canHarvestDestPos = (map->getSurfaceCell(Map::toSurfCoords(targetPos)) != NULL && map->getSurfaceCell(Map::toSurfCoords(targetPos))->getResource() != NULL && map->getSurfaceCell(Map::toSurfCoords(targetPos))->getResource()->getType() != NULL);
							if(canHarvestDestPos == true) {
								if(frameIndex < 0) {
									//if it finds resources it starts harvesting
									unit->setCurrSkill(hct->getHarvestSkillType());
									unit->setTargetPos(targetPos);
									command->setPos(targetPos);
									unit->setLoadCount(0);
									unit->getFaction()->addResourceTargetToCache(targetPos);

									switch(this->game->getGameSettings()->getPathFinderType()) {
										case pfBasic:
											unit->setLoadType(r->getType());
											break;
										case pfRoutePlanner:
											unit->setLoadType(r->getType());
											break;
										default:
											throw runtime_error("detected unsupported pathfinder type!");
									}
								}
							}

							if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
						}

						if(canHarvestDestPos == false) {
							if(frameIndex < 0) {
								unit->setLastHarvestResourceTarget(&targetPos);
							}

							if(targetPos.x >= 0) {
								//if not continue walking

								if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
									char szBuf[4096]="";
									sprintf(szBuf,"[updateHarvest #2] unit->getPos() [%s] command->getPos() [%s] targetPos [%s]",
											unit->getPos().getString().c_str(),command->getPos().getString().c_str(),targetPos.getString().c_str());
									unit->logSynchData(__FILE__,__LINE__,szBuf);
								}

								wasStuck = false;
								TravelState tsValue = tsImpossible;
								switch(this->game->getGameSettings()->getPathFinderType()) {
									case pfBasic:
										tsValue = pathFinder->findPath(unit, targetPos, &wasStuck, frameIndex);
										if (tsValue == tsMoving && frameIndex < 0) {
											unit->setCurrSkill(hct->getMoveSkillType());
											command->setPos(targetPos);
										}
										break;
									case pfRoutePlanner:
										tsValue = routePlanner->findPathToResource(unit, targetPos, r->getType());
										if (tsValue == tsMoving && frameIndex < 0) {
											unit->setCurrSkill(hct->getMoveSkillType());
											command->setPos(targetPos);
										}
										break;
									default:
										throw runtime_error("detected unsupported pathfinder type!");
								}
							}

							if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

				    		if(wasStuck == true && frameIndex < 0) {
								//if can't harvest, search for another resource
								unit->setCurrSkill(scStop);
								if(searchForResource(unit, hct) == false) {
									unit->finishCommand();
								}
				    		}
						}
		    		}
				}
			}
			else {
				if(frameIndex < 0) {
					//if can't harvest, search for another resource
					unit->setCurrSkill(scStop);
					if(searchForResource(unit, hct) == false) {
						unit->finishCommand();
					}
				}
				if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
			}

			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
		}
		else {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

			//if loaded, return to store
			Unit *store= world->nearestStore(unit->getPos(), unit->getFaction()->getIndex(), unit->getLoadType());
			if(store != NULL) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
					char szBuf[4096]="";
					sprintf(szBuf,"[updateHarvest #3] unit->getPos() [%s] store->getCenteredPos() [%s]",
							unit->getPos().getString().c_str(),store->getCenteredPos().getString().c_str());
					unit->logSynchData(__FILE__,__LINE__,szBuf);
				}

				TravelState tsValue = tsImpossible;
	    		switch(this->game->getGameSettings()->getPathFinderType()) {
	    			case pfBasic:
	    				tsValue = pathFinder->findPath(unit, store->getCenteredPos(), NULL, frameIndex);
	    				break;
	    			case pfRoutePlanner:
	    				tsValue = routePlanner->findPathToStore(unit, store);
	    				break;
	    			default:
	    				throw runtime_error("detected unsupported pathfinder type!");
	    	    }

	    		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	    		if(frameIndex < 0) {
					switch(tsValue) {
					case tsMoving:
						{
						if (hct->canHarvest(unit->getLoadType()) == false) {
							// hct has changed to a different harvest command.
							const HarvestCommandType *previousHarvestCmd = unit->getType()->getFirstHarvestCommand(unit->getLoadType(),unit->getFaction());
							if(previousHarvestCmd != NULL) {
								//printf("\n\n#1\n\n");
								unit->setCurrSkill(previousHarvestCmd->getMoveLoadedSkillType()); // make sure we use the right harvest animation
							}
							else {
								//printf("\n\n#2\n\n");
								unit->setCurrSkill(hct->getMoveLoadedSkillType());
							}
						}
						else {
							unit->setCurrSkill(hct->getMoveLoadedSkillType());
						}
						}
						break;
					default:
						break;
					}

					//world->changePosCells(unit,unit->getPos()+unit->getDest());
					if(map->isNextTo(unit->getPos(), store)) {

						//update resources
						int resourceAmount= unit->getLoadCount();
						if(unit->getFaction()->getCpuControl())
						{
							int resourceMultiplierIndex=game->getGameSettings()->getResourceMultiplierIndex(unit->getFaction()->getIndex());
							resourceAmount=(resourceAmount* (resourceMultiplierIndex +5))/10;
						}
						unit->getFaction()->incResourceAmount(unit->getLoadType(), resourceAmount);
						world->getStats()->harvest(unit->getFactionIndex(), resourceAmount);
						scriptManager->onResourceHarvested();

						//if next to a store unload resources
						unit->getPath()->clear();
						unit->setCurrSkill(scStop);
						unit->setLoadCount(0);

						command->setPosToOriginalPos();
					}
	    		}
	    		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
			}
			else {
				if(frameIndex < 0) {
					unit->finishCommand();
				}
			}
		}
	}
	else {
		if(frameIndex < 0) {
			//if working
			//unit->setLastHarvestResourceTarget(NULL);

			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

			const Vec2i unitTargetPos = unit->getTargetPos();
			SurfaceCell *sc= map->getSurfaceCell(Map::toSurfCoords(unitTargetPos));
			Resource *r= sc->getResource();

			if (r != NULL) {
				if (hct->canHarvest(r->getType()) == false ||
					r->getType()->getName() != unit->getLoadType()->getName()) {
					// hct has changed to a different harvest command.
					if(r->getType()->getName() != unit->getLoadType()->getName()) {
						const HarvestCommandType *previousHarvestCmd = unit->getType()->getFirstHarvestCommand(unit->getLoadType(),unit->getFaction());
						if(previousHarvestCmd != NULL) {
							//printf("\n\n#1\n\n");
							unit->setCurrSkill(previousHarvestCmd->getStopLoadedSkillType()); // make sure we use the right harvest animation
						}
						else {
							//printf("\n\n#2\n\n");
							unit->setCurrSkill(hct->getStopLoadedSkillType());
						}
					}
					else if(hct->canHarvest(r->getType()) == false) {
						const HarvestCommandType *previousHarvestCmd = unit->getType()->getFirstHarvestCommand(unit->getLoadType(),unit->getFaction());
						if(previousHarvestCmd != NULL) {
							//printf("\n\n#3\n\n");
							unit->setCurrSkill(previousHarvestCmd->getStopLoadedSkillType()); // make sure we use the right harvest animation
						}
						else {
							//printf("\n\n#4\n\n");
							unit->setCurrSkill(hct->getStopLoadedSkillType());
						}
					}
					else {
						//printf("\n\n#5 [%s] [%s]\n\n",r->getType()->getName().c_str(),unit->getLoadType()->getName().c_str());
						unit->setCurrSkill(hct->getStopLoadedSkillType()); // this is actually the wrong animation
					}
					unit->getPath()->clear();

					if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
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
								unit->getFaction()->removeResourceTargetFromCache(unitTargetPos);

								switch(this->game->getGameSettings()->getPathFinderType()) {
									case pfBasic:
										break;
									case pfRoutePlanner:
										world->getCartographer()->onResourceDepleted(Map::toSurfCoords(unit->getTargetPos()), rt);
										break;
									default:
										throw runtime_error("detected unsupported pathfinder type!");
								}

								//printf("\n\n#6\n\n");
								unit->setCurrSkill(hct->getStopLoadedSkillType());
							}
						}

						if (unit->getLoadCount() >= hct->getMaxLoad()) {
							//printf("\n\n#7\n\n");
							unit->setCurrSkill(hct->getStopLoadedSkillType());
							unit->getPath()->clear();
						}
					}

					if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
				}
			}
			else {
				//if there is no resource, just stop
				//printf("\n\n#8\n\n");
				unit->setCurrSkill(hct->getStopLoadedSkillType());
			}
		}
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld --------------------------- [END OF METHOD] ---------------------------\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
}

void UnitUpdater::SwapActiveCommand(Unit *unitSrc, Unit *unitDest) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(unitSrc->getCommandSize() > 0 && unitDest->getCommandSize() > 0) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		Command *cmd1 = unitSrc->getCurrCommand();
		Command *cmd2 = unitDest->getCurrCommand();
		unitSrc->replaceCurrCommand(cmd2);
		unitDest->replaceCurrCommand(cmd1);
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void UnitUpdater::SwapActiveCommandState(Unit *unit, CommandStateType commandStateType,
										const CommandType *commandType,
										int originalValue,int newValue) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(commandStateType == cst_linkedUnit) {
		if(dynamic_cast<const BuildCommandType *>(commandType) != NULL) {

			for(int i = 0; i < unit->getFaction()->getUnitCount(); ++i) {
				Unit *peerUnit = unit->getFaction()->getUnit(i);
				if(peerUnit != NULL) {
					if(peerUnit->getCommandSize() > 0 ) {
						if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

						Command *peerCommand = peerUnit->getCurrCommand();
						//const BuildCommandType *bct = dynamic_cast<const BuildCommandType*>(peerCommand->getCommandType());
						//if(bct != NULL) {
						if(peerCommand != NULL) {
							if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

							//if(command->getPos() == peerCommand->getPos()) {
							if( peerCommand->getStateType() == commandStateType &&
									peerCommand->getStateValue() == originalValue) {
								if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

								peerCommand->setStateValue(newValue);
							}
						}
					}
				}
			}

		}
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

Unit * UnitUpdater::findPeerUnitBuilder(Unit *unit) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    Unit *foundUnitBuilder = NULL;
    if(unit->getCommandSize() > 0 ) {
		Command *command = unit->getCurrCommand();
		if(command != NULL) {
			const RepairCommandType *rct= dynamic_cast<const RepairCommandType*>(command->getCommandType());
			if(rct != NULL && command->getStateType() == cst_linkedUnit) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] looking for command->getStateValue() = %d\n",__FILE__,__FUNCTION__,__LINE__,command->getStateValue());

                Unit *firstLinkedPeerRepairer = NULL;

				for(int i = 0; i < unit->getFaction()->getUnitCount(); ++i) {
					Unit *peerUnit = unit->getFaction()->getUnit(i);
					if(peerUnit != NULL) {
						if(peerUnit->getCommandSize() > 0 ) {
							if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

							Command *peerCommand = peerUnit->getCurrCommand();
							const BuildCommandType *bct = dynamic_cast<const BuildCommandType*>(peerCommand->getCommandType());
							if(bct != NULL) {
								if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

								if(command->getStateValue() == peerUnit->getId()) {
									if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

									foundUnitBuilder = peerUnit;
									break;
								}
							}
							else {
								if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] **peer NOT building**, peerUnit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,peerUnit->toString().c_str());

							    if(firstLinkedPeerRepairer == NULL) {
                                    const RepairCommandType *prct = dynamic_cast<const RepairCommandType*>(peerCommand->getCommandType());
                                    if(prct != NULL) {
                                    	if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

                                        if(unit->getId() != peerUnit->getId() && command->getStateValue() == peerUnit->getId()) {
                                        	if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

                                            firstLinkedPeerRepairer = peerUnit;
                                        }
                                    }
							    }
							}
						}
					}
				}

				if(foundUnitBuilder == NULL && firstLinkedPeerRepairer != NULL) {
				    foundUnitBuilder = firstLinkedPeerRepairer;
				}
			}
		}
    }

    if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] returning foundUnitBuilder = [%s]\n",__FILE__,__FUNCTION__,__LINE__,(foundUnitBuilder != NULL ? foundUnitBuilder->toString().c_str() : "null"));

    return foundUnitBuilder;
}

// ==================== updateRepair ====================

void UnitUpdater::updateRepair(Unit *unit, int frameIndex) {
	// Nothing to do
	if(frameIndex >= 0) {
		clearUnitPrecache(unit);
		return;
	}

	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] unit = %p\n",__FILE__,__FUNCTION__,__LINE__,unit);

	//if(unit != NULL) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] unit doing the repair [%s] - %d\n",__FILE__,__FUNCTION__,__LINE__,unit->getFullName().c_str(),unit->getId());
	//}

    Command *command= unit->getCurrCommand();
    const RepairCommandType *rct= static_cast<const RepairCommandType*>(command->getCommandType());

    if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] rct = %p\n",__FILE__,__FUNCTION__,__LINE__,rct);

	Unit *repaired = map->getCell(command->getPos())->getUnitWithEmptyCellMap(fLand);
	if(repaired == NULL) {
		repaired = map->getCell(command->getPos())->getUnit(fLand);
	}

	if(repaired != NULL) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] unit to repair [%s] - %d\n",__FILE__,__FUNCTION__,__LINE__,repaired->getFullName().c_str(),repaired->getId());
	}

	if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	// Check if the 'repaired' unit is actually the peer unit in a multi-build?
	Unit *peerUnitBuilder = findPeerUnitBuilder(unit);

	if(peerUnitBuilder != NULL) {
		SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] unit peer [%s] - %d\n",__FILE__,__FUNCTION__,__LINE__,peerUnitBuilder->getFullName().c_str(),peerUnitBuilder->getId());
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	// Ensure we have the right unit to repair
	if(peerUnitBuilder != NULL) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] peerUnitBuilder = %p\n",__FILE__,__FUNCTION__,__LINE__,peerUnitBuilder);

		if(peerUnitBuilder->getCurrCommand()->getUnit() != NULL) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] peerbuilder's unitid = %d\n",__FILE__,__FUNCTION__,__LINE__,peerUnitBuilder->getCurrCommand()->getUnit()->getId());

			repaired = peerUnitBuilder->getCurrCommand()->getUnit();
		}
	}

	bool nextToRepaired = repaired != NULL && map->isNextTo(unit->getPos(), repaired);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	peerUnitBuilder = NULL;
	if(repaired == NULL) {
		peerUnitBuilder = findPeerUnitBuilder(unit);
		if(peerUnitBuilder != NULL) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] peerUnitBuilder = %p\n",__FILE__,__FUNCTION__,__LINE__,peerUnitBuilder);

			if(peerUnitBuilder->getCurrCommand()->getUnit() != NULL) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] peerbuilder's unitid = %d\n",__FILE__,__FUNCTION__,__LINE__,peerUnitBuilder->getCurrCommand()->getUnit()->getId());

				repaired = peerUnitBuilder->getCurrCommand()->getUnit();
				nextToRepaired = repaired != NULL && map->isNextTo(unit->getPos(), repaired);
			}
			else {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

				Vec2i buildPos = map->findBestBuildApproach(unit, command->getPos(), peerUnitBuilder->getCurrCommand()->getUnitType());

				//nextToRepaired= (unit->getPos() == (command->getPos()-Vec2i(1)));
				nextToRepaired = (unit->getPos() == buildPos);

				if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] peerUnitBuilder = %p, nextToRepaired = %d\n",__FILE__,__FUNCTION__,__LINE__,peerUnitBuilder,nextToRepaired);

				if(nextToRepaired == true) {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

					Command *peerCommand = peerUnitBuilder->getCurrCommand();
					const RepairCommandType *rct = dynamic_cast<const RepairCommandType*>(peerCommand->getCommandType());
					// If the peer is also scheduled to do a repair we CANNOT swap their commands or
					// it will result in a stack overflow as each swaps the others repair command.
					// We must convert this unit's repair into a build right now!
					if(rct != NULL) {
						if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

						const CommandType *ctbuild = unit->getType()->getFirstCtOfClass(ccBuild);
						NetworkCommand networkCommand(this->world,nctGiveCommand, unit->getId(), ctbuild->getId(), command->getPos(),
														command->getUnitType()->getId(), -1, CardinalDir::NORTH, true, command->getStateType(),
														command->getStateValue());

						if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

						Command* command= this->game->getCommander()->buildCommand(&networkCommand);
						CommandResult cr= unit->checkCommand(command);
						if(cr == crSuccess) {
							if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
							unit->replaceCurrCommand(command);
						}
						else {
							if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
							delete command;

							unit->setCurrSkill(scStop);
							unit->finishCommand();
						}
					}
					else {
						CommandStateType commandStateType = unit->getCurrCommand()->getStateType();
						SwapActiveCommand(unit,peerUnitBuilder);
						int oldPeerUnitId = peerUnitBuilder->getId();
						int newPeerUnitId = unit->getId();
						SwapActiveCommandState(unit,commandStateType,unit->getCurrCommand()->getCommandType(),oldPeerUnitId,newPeerUnitId);

						// Give the swapped unit a fresh chance to help build in case they
						// were or are about to be blocked
						peerUnitBuilder->getPath()->clear();
						peerUnitBuilder->setRetryCurrCommandCount(1);
						updateUnitCommand(unit,-1);
					}
					return;
				}
			}

			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
		}
	}
	else {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] unit to repair[%s]\n",__FILE__,__FUNCTION__,__LINE__,repaired->getFullName().c_str());
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] repaired = %p, nextToRepaired = %d, unit->getCurrSkill()->getClass() = %d\n",__FILE__,__FUNCTION__,__LINE__,repaired,nextToRepaired,unit->getCurrSkill()->getClass());

	//UnitPathInterface *path= unit->getPath();

	if(unit->getCurrSkill()->getClass() != scRepair ||
		(nextToRepaired == false && peerUnitBuilder == NULL)) {

		Vec2i repairPos = command->getPos();
		bool startRepairing = (repaired != NULL && rct->isRepairableUnitType(repaired->getType()) && repaired->isDamaged());

		if(startRepairing == true) {
			//printf("STARTING REPAIR, unit [%s - %d] for unit [%s - %d]\n",unit->getType()->getName().c_str(),unit->getId(),repaired->getType()->getName().c_str(),repaired->getId());
//			for(unsigned int i = 0; i < rct->getRepairCount(); ++i) {
//				const UnitType *rUnit = rct->getRepair(i);
//				printf("Can repair unittype [%s]\n",rUnit->getName().c_str());
//			}
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] repairPos = %s, startRepairing = %d\n",__FILE__,__FUNCTION__,__LINE__,repairPos.getString().c_str(),startRepairing);

		if(startRepairing == false && peerUnitBuilder != NULL) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			startRepairing = true;
			// Since the unit to be built is not yet existing we need to tell the
			// other units to move to the build position or else they get in the way

			// No need to adjust repair pos since we already did this above via: Vec2i buildPos = map->findBestBuildApproach(unit->getPos(), command->getPos(), peerUnitBuilder->getCurrCommand()->getUnitType());
			//repairPos = command->getPos()-Vec2i(1);
		}

        //if not repairing
        if(startRepairing == true) {
        	if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			if(nextToRepaired == true) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

				unit->setTarget(repaired);
				unit->setCurrSkill(rct->getRepairSkillType());
			}
			else {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

				if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
					char szBuf[4096]="";
					sprintf(szBuf,"[updateRepair] unit->getPos() [%s] command->getPos()() [%s] repairPos [%s]",unit->getPos().getString().c_str(),command->getPos().getString().c_str(),repairPos.getString().c_str());
					unit->logSynchData(__FILE__,__LINE__,szBuf);
				}

				if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

				TravelState ts;
	    		switch(this->game->getGameSettings()->getPathFinderType()) {
	    			case pfBasic:
	    				if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	    				ts = pathFinder->findPath(unit, repairPos, NULL, frameIndex);
	    				break;
	    			case pfRoutePlanner:
	    				if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

						if (repaired && !repaired->getType()->isMobile()) {
							ts = routePlanner->findPathToBuildSite(unit, repaired->getType(), repaired->getPos(), repaired->getModelFacing());
						}
						else {
							ts = routePlanner->findPath(unit, repairPos);
						}
	    				break;
	    			default:
	    				throw runtime_error("detected unsupported pathfinder type!");
	    	    }

	    		if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] ts = %d\n",__FILE__,__FUNCTION__,__LINE__,ts);

	    		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

				switch(ts) {
				case tsMoving:
					if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] tsMoving\n",__FILE__,__FUNCTION__,__LINE__);
					unit->setCurrSkill(rct->getMoveSkillType());
					break;
				case tsBlocked:
					if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] tsBlocked\n",__FILE__,__FUNCTION__,__LINE__);

					if(unit->getPath()->isBlocked()) {
						if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] about to call [scStop]\n",__FILE__,__FUNCTION__,__LINE__);

						if(unit->getRetryCurrCommandCount() > 0) {
							if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] will retry command, unit->getRetryCurrCommandCount() = %d\n",__FILE__,__FUNCTION__,__LINE__,unit->getRetryCurrCommandCount());

							unit->setRetryCurrCommandCount(0);
							unit->getPath()->clear();
							updateUnitCommand(unit,-1);
						}
						else {
							unit->finishCommand();
						}
					}
					break;
				default:
					break;
				}
			}
        }
        else {
        	if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] about to call [scStop]\n",__FILE__,__FUNCTION__,__LINE__);

       		unit->setCurrSkill(scStop);
       		unit->finishCommand();
       		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
        }
    }
    else {
    	if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		bool cancelRepair = false;
    	//if repairing
		if(repaired != NULL) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			// Check if we can still repair the unit (may have morphed, etc)
			bool canStillRepair = rct->isRepairableUnitType(repaired->getType());
			if(canStillRepair == true) {
				unit->setTarget(repaired);

			}
			else {
				//printf("CANCELLING CURRENT REPAIR, unit [%s - %d] for unit [%s - %d]\n",unit->getType()->getName().c_str(),unit->getId(),repaired->getType()->getName().c_str(),repaired->getId());
//				for(unsigned int i = 0; i < rct->getRepairCount(); ++i) {
//					const UnitType *rUnit = rct->getRepair(i);
//					printf("Can repair unittype [%s]\n",rUnit->getName().c_str());
//				}

				unit->setCurrSkill(scStop);
				unit->finishCommand();
				cancelRepair = true;
			}
		}
		else if(peerUnitBuilder != NULL) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			unit->setTargetPos(command->getPos());
		}

		if(cancelRepair == false && (repaired == NULL || repaired->repair()) &&
			peerUnitBuilder == NULL) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] about to call [scStop]\n",__FILE__,__FUNCTION__,__LINE__);

			unit->setCurrSkill(scStop);
			unit->finishCommand();

			if(repaired != NULL && repaired->isBuilt() == false) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

				repaired->born();
				scriptManager->onUnitCreated(repaired);
			}

			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
		}
    }
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld --------------------------- [END OF METHOD] ---------------------------\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
}

// ==================== updateProduce ====================

void UnitUpdater::updateProduce(Unit *unit, int frameIndex) {
	// Nothing to do
	if(frameIndex >= 0) {
		clearUnitPrecache(unit);
		return;
	}
	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

    Command *command= unit->getCurrCommand();
    const ProduceCommandType *pct= static_cast<const ProduceCommandType*>(command->getCommandType());
    Unit *produced;

    if(unit->getCurrSkill()->getClass() != scProduce) {
        //if not producing
        unit->setCurrSkill(pct->getProduceSkillType());
    }
    else {
		unit->update2();

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

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

			if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] about to place unit for unit [%s]\n",__FILE__,__FUNCTION__,__LINE__,produced->toString().c_str());

			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

			//place unit creates the unit
			if(!world->placeUnit(unit->getCenteredPos(), 10, produced)) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] COULD NOT PLACE UNIT for unitID [%d]\n",__FILE__,__FUNCTION__,__LINE__,produced->getId());

				delete produced;
			}
			else{
				produced->create();
				produced->born();
				world->getStats()->produce(unit->getFactionIndex());
				const CommandType *ct= produced->computeCommandType(unit->getMeetingPos());
				if(ct!=NULL){
					if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
					produced->giveCommand(new Command(ct, unit->getMeetingPos()));
				}
				scriptManager->onUnitCreated(produced);

				if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
			}
        }
    }

    if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld --------------------------- [END OF METHOD] ---------------------------\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
}


// ==================== updateUpgrade ====================

void UnitUpdater::updateUpgrade(Unit *unit, int frameIndex) {
	// Nothing to do
	if(frameIndex >= 0) {
		clearUnitPrecache(unit);
		return;
	}

	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

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

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld --------------------------- [END OF METHOD] ---------------------------\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
}

// ==================== updateMorph ====================

void UnitUpdater::updateMorph(Unit *unit, int frameIndex) {
	// Nothing to do
	if(frameIndex >= 0) {
		clearUnitPrecache(unit);
		return;
	}

	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

	Command *command= unit->getCurrCommand();
    const MorphCommandType *mct= static_cast<const MorphCommandType*>(command->getCommandType());

    if(unit->getCurrSkill()->getClass()!=scMorph){
		//if not morphing, check space
		if(map->isFreeCellsOrHasUnit(unit->getPos(), mct->getMorphUnit()->getSize(), unit->getCurrField(), unit, mct->getMorphUnit())){
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
							int size = max(oldSize, unit->getType()->getSize());
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

    if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld --------------------------- [END OF METHOD] ---------------------------\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
}

// ==================== PRIVATE ====================

// ==================== attack ====================

void UnitUpdater::hit(Unit *attacker){
	hit(attacker, static_cast<const AttackSkillType*>(attacker->getCurrSkill()), attacker->getTargetPos(), attacker->getTargetField());
}

void UnitUpdater::hit(Unit *attacker, const AttackSkillType* ast, const Vec2i &targetPos, Field targetField){
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
}

void UnitUpdater::damage(Unit *attacker, const AttackSkillType* ast, Unit *attacked, float distance){
	if(attacker == NULL) {
		throw runtime_error("attacker == NULL");
	}
	if(ast == NULL) {
		throw runtime_error("ast == NULL");
	}

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
		world->getStats()->kill(attacker->getFactionIndex(), attacked->getFactionIndex(), attacker->getTeam() != attacked->getTeam());
		attacker->incKills(attacked->getTeam());

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
	// !!! Is this causing choppy network play somehow?
	//attacker->computeHp();
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
	if(visible == false && world->showWorldForPlayer(world->getThisFactionIndex()) == true) {
		visible = true;
	}

	//projectile
	if(pstProj!=NULL){
		psProj= pstProj->create();
		psProj->setPath(startPos, endPos);
		psProj->setObserver(new ParticleDamager(unit, this, gameCamera));
		psProj->setVisible(visible);
		psProj->setFactionColor(unit->getFaction()->getTexture()->getPixmapConst()->getPixel3f(0,0));
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
		psSplash->setFactionColor(unit->getFaction()->getTexture()->getPixmapConst()->getPixel3f(0,0));
		renderer.manageParticleSystem(psSplash, rsGame);
		if(pstProj!=NULL){
			psProj->link(psSplash);
		}
	}
}

// ==================== misc ====================

//looks for a resource of type rt, if rt==NULL looks for any
//resource the unit can harvest
bool UnitUpdater::searchForResource(Unit *unit, const HarvestCommandType *hct) {
    Vec2i pos= unit->getCurrCommand()->getPos();

    for(int radius= 0; radius < maxResSearchRadius; radius++) {
        for(int i = pos.x - radius; i <= pos.x + radius; ++i) {
            for(int j=pos.y - radius; j <= pos.y + radius; ++j) {
				if(map->isInside(i, j)) {
					Resource *r= map->getSurfaceCell(Map::toSurfCoords(Vec2i(i, j)))->getResource();
                    if(r != NULL) {
						if(hct->canHarvest(r->getType())) {
							const Vec2i newPos = Vec2i(i, j);
							if(unit->isBadHarvestPos(newPos) == false) {
								unit->getCurrCommand()->setPos(newPos);

								return true;
							}
                        }
                    }
                }
            }
        }
    }

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
	for(int k = 0; k < fieldCount; k++) {
		Field f= static_cast<Field>(k);

		//check field
		if((ast == NULL || ast->getAttackField(f))) {
			Unit *possibleEnemy = cell->getUnit(f);

			//check enemy
			if(possibleEnemy != NULL && possibleEnemy->isAlive()) {
				if((unit->isAlly(possibleEnemy) == false && commandTarget == NULL) ||
					commandTarget == possibleEnemy) {

					enemies.push_back(possibleEnemy);
				}
			}
		}
	}
}

void UnitUpdater::findEnemiesForCell(const Vec2i pos, int size, int sightRange, const Faction *faction, vector<Unit*> &enemies, bool attackersOnly) const {
	//all fields
	for(int k = 0; k < fieldCount; k++) {
		Field f= static_cast<Field>(k);

		for(int i = pos.x - sightRange; i < pos.x + size + sightRange; ++i) {
			for(int j = pos.y - sightRange; j < pos.y + size + sightRange; ++j) {
				Vec2i testPos(i,j);
				if( map->isInside(testPos) &&
						map->isInsideSurface(map->toSurfCoords(testPos))) {
					Cell *cell = map->getCell(testPos);
					//check field
					Unit *possibleEnemy = cell->getUnit(f);

					//check enemy
					if(possibleEnemy != NULL && possibleEnemy->isAlive()) {
						if(faction->getTeam() != possibleEnemy->getTeam()) {
							if(attackersOnly == true) {
								if(possibleEnemy->getType()->hasCommandClass(ccAttack) || possibleEnemy->getType()->hasCommandClass(ccAttackStopped)) {
									enemies.push_back(possibleEnemy);
								}
							}
							else {
								enemies.push_back(possibleEnemy);
							}
						}
					}
				}
			}
		}
	}
}

//if the unit has any enemy on range
bool UnitUpdater::unitOnRange(const Unit *unit, int range, Unit **rangedPtr,
							  const AttackSkillType *ast) {
    vector<Unit*> enemies;
	bool result=false;
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

	//bool foundInCache = true;
	if(findCachedCellsEnemies(center,range,size,enemies,ast,
							  unit,commandTarget) == false) {
		//foundInCache = false;
		//nearby cells
		UnitRangeCellsLookupItem cacheItem;
		for(int i = center.x - range; i < center.x + range + size; ++i) {
			for(int j = center.y - range; j < center.y + range + size; ++j) {
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
			//cacheItem.UnitRangeCellsLookupItemCacheTimerCountIndex = UnitRangeCellsLookupItemCacheTimerCount++;
			UnitRangeCellsLookupItemCache[center][size][range] = cacheItem;
		}
	}

	//attack enemies that can attack first
	float distToUnit=-1;
	Unit* enemySeen = NULL;
    for(int i = 0; i< enemies.size(); ++i) {
    	Unit *enemy = enemies[i];

    	if(enemy->isAlive() == true) {
    		// Here we default to first enemy if no attackers found
    		if(enemySeen == NULL) {
                *rangedPtr 	= enemy;
    			enemySeen 	= enemy;
                result		= true;
    		}
    		// Attackers get first priority
    		if(enemy->getType()->hasSkillClass(scAttack) == true) {
    			// Select closest attacking unit
    			if(distToUnit < 0 || unit->getCenteredPos().dist(enemy->getCenteredPos()) < distToUnit) {
    				distToUnit = unit->getCenteredPos().dist(enemy->getCenteredPos());
					*rangedPtr	= enemies[i];
					enemySeen	= enemies[i];
					result		= true;
    			}
    			//break;
    		}
    	}
    }


/*
		if(enemies[i]->getType()->hasSkillClass(scAttack) &&
			enemies[i]->isAlive() == true ) {
            *rangedPtr= enemies[i];
			enemySeen=enemies[i];
            result=true;
            break;
        }
    }

/*
    if(result == false) {
		//any enemy
		for(int i= 0; i < enemies.size(); ++i) {
			if(enemies[i]->isAlive() == true) {
				*rangedPtr= enemies[i];
				enemySeen= enemies[i];
				result= true;
				break;
			}
		}
	}
*/

	if(result == true) {
		//const Unit* teamUnit	= NULL;
		const Unit* enemyUnit	= NULL;
		bool onlyEnemyUnits		= true;

		if(unit->getTeam() == world->getThisTeamIndex()) {
			//teamUnit		= unit;
			enemyUnit		= enemySeen;
			onlyEnemyUnits	= false;
		}
		else if(enemySeen->getTeam() == world->getThisTeamIndex()) {
			//teamUnit		= enemySeen;
			enemyUnit		= unit;
			onlyEnemyUnits	= false;
		}

		if(onlyEnemyUnits == false &&
			enemyUnit->getTeam() != world->getThisTeamIndex()) {
			Vec2f enemyFloatCenter	= enemyUnit->getFloatCenteredPos();
			// find nearest Attack and cleanup old dates
			AttackWarningData *nearest	= NULL;
			float currentDistance		= 0.f;
			float nearestDistance		= 0.f;

			for(int i = attackWarnings.size() - 1; i >= 0; --i) {
				if(world->getFrameCount() - attackWarnings[i]->lastFrameCount > 200) { //after 200 frames attack break we warn again
					AttackWarningData *toDelete =attackWarnings[i];
					attackWarnings.erase(attackWarnings.begin()+i);
					delete toDelete; // old one
				}
				else {
#ifdef USE_STREFLOP
					currentDistance = streflop::floor(enemyFloatCenter.dist(attackWarnings[i]->attackPosition)); // no need for streflops here!
#else
					currentDistance = floor(enemyFloatCenter.dist(attackWarnings[i]->attackPosition)); // no need for streflops here!
#endif

					if(nearest == NULL) {
						nearest = attackWarnings[i];
						nearestDistance = currentDistance;
					}
					else {
						if(currentDistance < nearestDistance) {
							nearest = attackWarnings[i];
							nearestDistance = currentDistance;
						}
					}
				}
	    	}

	    	if(nearest != NULL) {
	    		// does it fit?
	    		if(nearestDistance < attackWarnRange) {
	    			// update entry with current values
					nearest->lastFrameCount=world->getFrameCount();
					nearest->attackPosition.x=enemyFloatCenter.x;
					nearest->attackPosition.y=enemyFloatCenter.y;
	    		}
	    		else {
	    			//Must be a different Attack!
	    			nearest=NULL;  //set to null to force a new entry in next step
	    		}
	    	}
	    	// add new attack
	    	if(nearest == NULL) {
	    		// no else!
	    		AttackWarningData* awd= new AttackWarningData();
	    		awd->lastFrameCount=world->getFrameCount();
	    		awd->attackPosition.x=enemyFloatCenter.x;
	    		awd->attackPosition.y=enemyFloatCenter.y;
	    		attackWarnings.push_back(awd);
	    		SoundRenderer::getInstance().playFx(CoreData::getInstance().getAttentionSound());
	    		world->addAttackEffects(enemyUnit);
	    	}
		}
	}
    return result;
}


//if the unit has any enemy on range
vector<Unit*> UnitUpdater::enemyUnitsOnRange(const Unit *unit,const AttackSkillType *ast) {
	int range = unit->getType()->getSight();
	if(ast != NULL) {
		range = ast->getTotalAttackRange(unit->getTotalUpgrade());
	}
	vector<Unit*> enemies;
	//we check command target
	const Unit *commandTarget = NULL;
//	if(unit->anyCommand()) {
//		commandTarget = static_cast<const Unit*>(unit->getCurrCommand()->getUnit());
//	}
//	if(commandTarget != NULL && commandTarget->isDead()) {
//		commandTarget = NULL;
//	}

	//aux vars
	int size 			= unit->getType()->getSize();
	Vec2i center 		= unit->getPos();
	Vec2f floatCenter	= unit->getFloatCenteredPos();

	//bool foundInCache = true;
	if(findCachedCellsEnemies(center,range,size,enemies,ast,
							  unit,commandTarget) == false) {
		//foundInCache = false;
		//nearby cells
		UnitRangeCellsLookupItem cacheItem;
		for(int i = center.x - range; i < center.x + range + size; ++i) {
			for(int j = center.y - range; j < center.y + range + size; ++j) {
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
			//cacheItem.UnitRangeCellsLookupItemCacheTimerCountIndex = UnitRangeCellsLookupItemCacheTimerCount++;
			UnitRangeCellsLookupItemCache[center][size][range] = cacheItem;
		}
	}

	return enemies;
}


void UnitUpdater::findUnitsForCell(Cell *cell, const Unit *unit,vector<Unit*> &units) {
	//all fields
	for(int k = 0; k < fieldCount; k++) {
		Field f= static_cast<Field>(k);

		//check field
		Unit *cellUnit = cell->getUnit(f);

		if(cellUnit != NULL && cellUnit->isAlive()) {
			units.push_back(cellUnit);
		}
	}
}

vector<Unit*> UnitUpdater::findUnitsInRange(const Unit *unit, int radius) {
	int range = radius;
	vector<Unit*> units;

	//aux vars
	int size 			= unit->getType()->getSize();
	Vec2i center 		= unit->getPos();
	Vec2f floatCenter	= unit->getFloatCenteredPos();

	//nearby cells
	UnitRangeCellsLookupItem cacheItem;
	for(int i = center.x - range; i < center.x + range + size; ++i) {
		for(int j = center.y - range; j < center.y + range + size; ++j) {
			//cells inside map and in range
#ifdef USE_STREFLOP
			if(map->isInside(i, j) && streflop::floor(floatCenter.dist(Vec2f((float)i, (float)j))) <= (range+1)){
#else
			if(map->isInside(i, j) && floor(floatCenter.dist(Vec2f((float)i, (float)j))) <= (range+1)){
#endif
				Cell *cell = map->getCell(i,j);
				findUnitsForCell(cell,unit,units);
			}
		}
	}

	return units;
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

void ParticleDamager::update(ParticleSystem *particleSystem) {
	Unit *attacker= attackerRef.getUnit();

	if(attacker != NULL) {
		unitUpdater->hit(attacker, ast, targetPos, targetField);

		//play sound
		StaticSound *projSound= ast->getProjSound();
		if(particleSystem->getVisible() && projSound != NULL) {
			SoundRenderer::getInstance().playFx(projSound, attacker->getCurrVector(), gameCamera->getPos());
		}
	}
	particleSystem->setObserver(NULL);
	delete this;
}

}}//end namespace
