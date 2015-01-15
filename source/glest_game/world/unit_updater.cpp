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

#include "unit_updater.h"

#include <algorithm>
#include <cassert>

#include "core_data.h"
#include "config.h"
#include "game.h"
#include "faction.h"
#include "network_manager.h"
#include "object.h"
#include "particle_type.h"
#include "projectile_type.h"
#include "path_finder.h"
#include "renderer.h"
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

UnitUpdater::UnitUpdater() : mutexAttackWarnings(new Mutex(CODE_AT_LINE)),
		mutexUnitRangeCellsLookupItemCache(new Mutex(CODE_AT_LINE)) {
    this->game= NULL;
	this->gui= NULL;
	this->gameCamera= NULL;
	this->world= NULL;
	this->map= NULL;
	this->console= NULL;
	this->scriptManager= NULL;
	this->pathFinder = NULL;
	//UnitRangeCellsLookupItemCacheTimerCount = 0;
	attackWarnRange=0;
}

void UnitUpdater::init(Game *game){

    this->game= game;
	this->gui= game->getGuiPtr();
	this->gameCamera= game->getGameCamera();
	this->world= game->getWorld();
	this->map= world->getMap();
	this->console= game->getConsole();
	this->scriptManager= game->getScriptManager();
	this->pathFinder = NULL;
	attackWarnRange=Config::getInstance().getFloat("AttackWarnRange","50.0");
	//UnitRangeCellsLookupItemCacheTimerCount = 0;

	switch(this->game->getGameSettings()->getPathFinderType()) {
		case pfBasic:
			pathFinder = new PathFinder();
			pathFinder->init(map);
			break;
		default:
			throw megaglest_runtime_error("detected unsupported pathfinder type!");
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

	MutexSafeWrapper safeMutex(mutexAttackWarnings,string(__FILE__) + "_" + intToStr(__LINE__));
	while(attackWarnings.empty() == false) {
		AttackWarningData* awd = attackWarnings.back();
		attackWarnings.pop_back();
		delete awd;
	}

	safeMutex.ReleaseLock();

	delete mutexAttackWarnings;
	mutexAttackWarnings = NULL;

	delete mutexUnitRangeCellsLookupItemCache;
	mutexUnitRangeCellsLookupItemCache = NULL;
}

// ==================== progress skills ====================

//skill dependent actions
bool UnitUpdater::updateUnit(Unit *unit) {
	bool processUnitCommand = false;

	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld --------------------------- [START OF METHOD] ---------------------------\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	SoundRenderer &soundRenderer= SoundRenderer::getInstance();

	//play skill sound
	const SkillType *currSkill= unit->getCurrSkill();

	for(SkillSoundList::const_iterator it= currSkill->getSkillSoundList()->begin(); it != currSkill->getSkillSoundList()->end(); ++it) {
		float soundStartTime= (*it)->getStartTime();
		if(soundStartTime >= unit->getLastAnimProgressAsFloat() && soundStartTime < unit->getAnimProgressAsFloat()) {
			if(map->getSurfaceCell(Map::toSurfCoords(unit->getPos()))->isVisible(world->getThisTeamIndex()) ||
				(game->getWorld()->showWorldForPlayer(game->getWorld()->getThisTeamIndex()) == true)) {
				soundRenderer.playFx((*it)->getSoundContainer()->getRandSound(), unit->getCurrMidHeightVector(), gameCamera->getPos());
			}
		}
	}


	if (currSkill->getShake()) {
		float shakeStartTime = currSkill->getShakeStartTime();
		if (shakeStartTime >= unit->getLastAnimProgressAsFloat()
				&& shakeStartTime < unit->getAnimProgressAsFloat()) {
			bool visibleAffected=false;
			bool cameraViewAffected=false;
			bool cameraDistanceAffected=false;
			bool enabled=false;
			if (game->getWorld()->getThisFactionIndex() == unit->getFactionIndex()) {
				visibleAffected=currSkill->getShakeSelfVisible();
				cameraViewAffected=currSkill->getShakeSelfInCameraView();
				cameraDistanceAffected=currSkill->getShakeSelfCameraAffected();
				enabled=currSkill->getShakeSelfEnabled();
			} else if (unit->getTeam() == world->getThisTeamIndex()) {
				visibleAffected=currSkill->getShakeTeamVisible();
				cameraViewAffected=currSkill->getShakeTeamInCameraView();
				cameraDistanceAffected=currSkill->getShakeTeamCameraAffected();
				enabled=currSkill->getShakeTeamEnabled();
			} else {
				visibleAffected=currSkill->getShakeEnemyVisible();
				cameraViewAffected=currSkill->getShakeEnemyInCameraView();
				cameraDistanceAffected=currSkill->getShakeEnemyCameraAffected();
				enabled=currSkill->getShakeEnemyEnabled();
			}

			bool visibility=(!visibleAffected)||(map->getSurfaceCell(Map::toSurfCoords(unit->getPos()))->isVisible(world->getThisTeamIndex()) ||
					(game->getWorld()->showWorldForPlayer(game->getWorld()->getThisTeamIndex()) == true));

			bool cameraAffected=(!cameraViewAffected) || unit->getVisible();

			if(visibility && cameraAffected && enabled) {
				game->getGameCameraPtr()->shake( currSkill->getShakeDuration(), currSkill->getShakeIntensity(),cameraDistanceAffected,unit->getCurrMidHeightVector());
			}
		}
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld [after playsound]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	unit->updateTimedParticles();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld [after playsound]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	//start attack particle system
	if(unit->getCurrSkill()->getClass() == scAttack) {
		const AttackSkillType *ast= static_cast<const AttackSkillType*>(unit->getCurrSkill());

		float attackStartTime = truncateDecimal<float>(ast->getAttackStartTime(),6);
		float lastAnimProgress = truncateDecimal<float>(unit->getLastAnimProgressAsFloat(),6);
		float animProgress = truncateDecimal<float>(unit->getAnimProgressAsFloat(),6);
		bool startAttackParticleSystemNow = false;
		if(ast->projectileTypes.empty() == true ){
			startAttackParticleSystemNow = (attackStartTime >= lastAnimProgress && attackStartTime < animProgress);
		}
		else {// start projectile attack
			for(ProjectileTypes::const_iterator it= ast->projectileTypes.begin(); it != ast->projectileTypes.end(); ++it) {
				attackStartTime= (*it)->getAttackStartTime();
				startAttackParticleSystemNow = (attackStartTime >= lastAnimProgress && attackStartTime < animProgress);
				if(startAttackParticleSystemNow==true) break;
			}
		}

		char szBuf[8096]="";
		snprintf(szBuf,8095,"attackStartTime = %f, lastAnimProgress = %f, animProgress = %f startAttackParticleSystemNow = %d",attackStartTime,lastAnimProgress,animProgress,startAttackParticleSystemNow);
		unit->setNetworkCRCParticleLogInfo(szBuf);

		if(startAttackParticleSystemNow == true) {
			startAttackParticleSystem(unit,lastAnimProgress,animProgress);
		}
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld [after attack particle system]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	bool update = unit->update();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld [after unit->update()]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	//printf("Update Unit [%d - %s] = %d\n",unit->getId(),unit->getType()->getName().c_str(),update);

	//update unit
	if(update == true) {
        //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		processUnitCommand = true;
		updateUnitCommand(unit,-1);

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld [after updateUnitCommand()]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

		//if unit is out of EP, it stops
		if(unit->computeEp() == true) {
			bool reQueueHoldPosition = false;
			string holdPositionName = "";
			if(unit->getCurrCommand() != NULL &&
				unit->getCurrCommand()->getCommandType()->getClass() == ccAttackStopped) {
				reQueueHoldPosition = true;
				holdPositionName = unit->getCurrCommand()->getCommandType()->getName(false);
			}

			unit->setCurrSkill(scStop);
			unit->cancelCommand();

			if(reQueueHoldPosition == true) {
				//Search for a command that can produce the unit
				const UnitType *ut = unit->getType();
				for(int i= 0; i < ut->getCommandTypeCount(); ++i) {
					const CommandType* ct= ut->getCommandType(i);
					if(ct != NULL && ct->getClass() == ccAttackStopped) {
						const AttackStoppedCommandType *act= static_cast<const AttackStoppedCommandType*>(ct);
						if(act != NULL && act->getName(false) == holdPositionName) {
							if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

							//printf("Re-Queing hold pos = %d, ep = %d skillep = %d skillname [%s]\n ",unit->getFaction()->reqsOk(act),unit->getEp(),act->getAttackSkillType()->getEpCost(),act->getName().c_str());
							if(unit->getFaction()->reqsOk(act) == true &&
								unit->getEp() >= act->getStopSkillType()->getEpCost()) {
								unit->giveCommand(new Command(act),true);
							}

							if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
							break;
						}
					}
				}
			}
		}
		if(unit->getCurrSkill() != NULL && unit->getCurrSkill()->getClass() != scAttack) {
			// !!! Is this causing choppy network play somehow?
			//unit->computeHp();
		}
		else if(unit->getCommandSize() > 0) {
			Command *command= unit->getCurrCommand();
			if(command != NULL) {

				const AttackCommandType *act= dynamic_cast<const AttackCommandType*>(command->getCommandType());
				if (act != NULL && act->getAttackSkillType() != NULL
						&& act->getAttackSkillType()->getSpawnUnit() != ""
						&& act->getAttackSkillType()->getSpawnUnitCount() > 0) {
					spawnAttack(unit,act->getAttackSkillType()->getSpawnUnit(),act->getAttackSkillType()->getSpawnUnitCount(),act->getAttackSkillType()->getSpawnUnitAtTarget());
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
						unit->getCurrMidHeightVector(),
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

	return processUnitCommand;
}

void UnitUpdater::spawnAttack(Unit *unit,string spawnUnit,int spawnUnitcount,bool spawnUnitAtTarget,Vec2i targetPos) {
	if(spawnUnit != "" && spawnUnitcount > 0) {

		const FactionType *ft= unit->getFaction()->getType();
		const UnitType *spawnUnitType = ft->getUnitType(spawnUnit);
		int spawnCount = spawnUnitcount;
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
				default:
					throw megaglest_runtime_error("detected unsupported pathfinder type!");
			}

			Unit *spawned= new Unit(world->getNextUnitId(unit->getFaction()), newpath,
					                Vec2i(0), spawnUnitType, unit->getFaction(),
					                world->getMap(), CardinalDir::NORTH);
			//SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] about to place unit for unit [%s]\n",__FILE__,__FUNCTION__,__LINE__,spawned->toString().c_str());
			bool placedSpawnUnit=false;
			if(targetPos==Vec2i(-10,-10)) {
				targetPos=unit->getTargetPos();
			}
			if(spawnUnitAtTarget) {
				placedSpawnUnit=world->placeUnit(targetPos, 10, spawned);
			} else {
				placedSpawnUnit=world->placeUnit(unit->getCenteredPos(), 10, spawned);
			}
			if(!placedSpawnUnit) {
				//SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] COULD NOT PLACE UNIT for unitID [%d]\n",__FILE__,__FUNCTION__,__LINE__,spawned->getId());

				// This will also cleanup newPath
				delete spawned;
				spawned = NULL;
			}
			else {
				spawned->create();
				spawned->born(NULL);
				world->getStats()->produce(unit->getFactionIndex(),spawned->getType()->getCountUnitProductionInStats());
				const CommandType *ct= spawned->getType()->getFirstAttackCommand(unit->getTargetField());
				if(ct == NULL){
					ct= spawned->computeCommandType(targetPos,map->getCell(targetPos)->getUnit(unit->getTargetField()));
				}
				if(ct != NULL){
					if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
					spawned->giveCommand(new Command(ct, targetPos));
				}
				scriptManager->onUnitCreated(spawned);
			}
		}
	}
}

// ==================== progress commands ====================

//VERY IMPORTANT: compute next state depending on the first order of the list
void UnitUpdater::updateUnitCommand(Unit *unit, int frameIndex) {
	try {
	bool minorDebugPerformance = false;
	Chrono chrono;
	if((minorDebugPerformance == true && frameIndex > 0) || SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

	//if unit has command process it
    bool hasCommand = (unit->anyCommand());

    if((minorDebugPerformance && frameIndex > 0) && chrono.getMillis() >= 1) printf("UnitUpdate [%d - %s] #1-unit threaded updates on frame: %d took [%lld] msecs\n",unit->getId(),unit->getType()->getName(false).c_str(),frameIndex,(long long int)chrono.getMillis());

	int64 elapsed1 = 0;
	if(minorDebugPerformance && frameIndex > 0) elapsed1 = chrono.getMillis();

    if(hasCommand == true) {
    	if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] unit [%s] has command [%s]\n",__FILE__,__FUNCTION__,__LINE__,unit->toString(false).c_str(), unit->getCurrCommand()->toString(false).c_str());

    	bool commandUsesPathFinder = (frameIndex < 0);
    	if(frameIndex > 0) {
			if(unit->getCurrCommand() != NULL && unit->getCurrCommand()->getCommandType() != NULL) {
    			commandUsesPathFinder = unit->getCurrCommand()->getCommandType()->usesPathfinder();

    			// Clear previous cached unit data
    			if(commandUsesPathFinder == true) {
    				clearUnitPrecache(unit);
    			}
			}
    	}
    	if(commandUsesPathFinder == true) {
			if(unit->getCurrCommand() != NULL && unit->getCurrCommand()->getCommandType() != NULL) {
    			unit->getCurrCommand()->getCommandType()->update(this, unit, frameIndex);
			}
    	}

    	if((minorDebugPerformance && frameIndex > 0) && (chrono.getMillis() - elapsed1) >= 1) {
    		//CommandClass cc = unit->getCurrCommand()->getCommandType()->commandTypeClass;
    		printf("UnitUpdate [%d - %s] #2-unit threaded updates on frame: %d commandUsesPathFinder = %d took [%lld] msecs\nCommand: %s\n",unit->getId(),unit->getType()->getName(false).c_str(),frameIndex,commandUsesPathFinder,(long long int)chrono.getMillis() - elapsed1,unit->getCurrCommand()->toString(false).c_str());
    	}
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
    if((minorDebugPerformance && frameIndex > 0) && chrono.getMillis() >= 1) printf("UnitUpdate [%d - %s] #3-unit threaded updates on frame: %d took [%lld] msecs\n",unit->getId(),unit->getType()->getName(false).c_str(),frameIndex,(long long int)chrono.getMillis());

	}
	catch(const exception &ex) {
		//setRunningStatus(false);

		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		throw megaglest_runtime_error(ex.what());
	}
	catch(...) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s %d] UNKNOWN error\n",__FILE__,__FUNCTION__,__LINE__);
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		throw megaglest_runtime_error(szBuf);
	}

}

// ==================== updateStop ====================

void UnitUpdater::updateStop(Unit *unit, int frameIndex) {
	try {
	// Nothing to do
	if(frameIndex >= 0) {
		clearUnitPrecache(unit);
		return;
	}

	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

	Command *command= unit->getCurrCommand();
	if(command == NULL) {
		throw megaglest_runtime_error("command == NULL");
	}
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
				if(attackableOnSight(unit, &sighted, ast, (frameIndex >= 0))) {
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

		if(attackerOnSight(unit, &sighted, (frameIndex >= 0))) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
			Vec2i escapePos = unit->getPos() * 2 - sighted->getPos();
			//SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			unit->giveCommand(new Command(unit->getType()->getFirstCtOfClass(ccMove), escapePos));
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
	}

   	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld --------------------------- [END OF METHOD] ---------------------------\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	}
	catch(const exception &ex) {
		//setRunningStatus(false);

		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		throw megaglest_runtime_error(ex.what());
	}
	catch(...) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s %d] UNKNOWN error\n",__FILE__,__FUNCTION__,__LINE__);
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		throw megaglest_runtime_error(szBuf);
	}

}

// ==================== updateMove ====================
void UnitUpdater::updateMove(Unit *unit, int frameIndex) {
	try {
	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

    Command *command= unit->getCurrCommand();
	if(command == NULL) {
		throw megaglest_runtime_error("command == NULL");
	}
    const MoveCommandType *mct= static_cast<const MoveCommandType*>(command->getCommandType());


	Vec2i pos= command->getUnit()!=NULL? command->getUnit()->getCenteredPos(): command->getPos();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"[updateMove] pos [%s] unit [%d - %s] cmd [%s]",pos.getString().c_str(),unit->getId(),unit->getFullName(false).c_str(),command->toString(false).c_str());
		unit->logSynchData(__FILE__,__LINE__,szBuf);
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());


	TravelState tsValue = tsImpossible;
	switch(this->game->getGameSettings()->getPathFinderType()) {
		case pfBasic:
			tsValue = pathFinder->findPath(unit, pos, NULL, frameIndex);
			break;
		default:
			throw megaglest_runtime_error("detected unsupported pathfinder type!");
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
			break;
		}
	}


	if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"[updateMove] tsValue [%d]",tsValue);
		unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld --------------------------- [END OF METHOD] ---------------------------\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	}
	catch(const exception &ex) {
		//setRunningStatus(false);

		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		throw megaglest_runtime_error(ex.what());
	}
	catch(...) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s %d] UNKNOWN error\n",__FILE__,__FUNCTION__,__LINE__);
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		throw megaglest_runtime_error(szBuf);
	}

}


// ==================== updateAttack ====================

void UnitUpdater::updateAttack(Unit *unit, int frameIndex) {
	try {

	if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"[updateAttack]");
		unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
	}

	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

	Command *command= unit->getCurrCommand();
	if(command == NULL) {

		if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
			char szBuf[8096]="";
			snprintf(szBuf,8096,"[updateAttack]");
			unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
		}

		return;
	}
    const AttackCommandType *act= static_cast<const AttackCommandType*>(command->getCommandType());
	if(act == NULL) {

		if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
			char szBuf[8096]="";
			snprintf(szBuf,8096,"[updateAttack]");
			unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
		}

		return;
	}
	Unit *target= NULL;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	if( (command->getUnit() == NULL || !(command->getUnit()->isAlive()) ) && unit->getCommandSize() > 1) {

		if(frameIndex < 0) {
			unit->finishCommand(); // all queued "ground attacks" are skipped if somthing else is queued after them.

			if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
				char szBuf[8096]="";
				snprintf(szBuf,8096,"[updateAttack]");
				unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
			}
		}
		return;
	}

	//if found
	//if(frameIndex < 0) {
	{
		if(attackableOnRange(unit, &target, act->getAttackSkillType(),(frameIndex >= 0))) {
    		if(frameIndex < 0) {
				if(unit->getEp() >= act->getAttackSkillType()->getEpCost()) {
					unit->setCurrSkill(act->getAttackSkillType());
					unit->setTarget(target);
				}
				else {
					unit->setCurrSkill(scStop);
				}

				if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
					char szBuf[8096]="";
					snprintf(szBuf,8096,"[updateAttack]");
					unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
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
			else if(attackableOnSight(unit, &target, act->getAttackSkillType(), (frameIndex >= 0))) {
				pos= target->getPos();
			}
			else {
				pos= command->getPos();
			}

			if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
				char szBuf[8096]="";
				snprintf(szBuf,8096,"[updateAttack] pos [%s] unit->getPos() [%s]",pos.getString().c_str(),unit->getPos().getString().c_str());
				unit->logSynchData(__FILE__,__LINE__,szBuf);
			}

			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

			TravelState tsValue = tsImpossible;
			//if(frameIndex < 0) {
			{
				//printf("In [%s::%s Line: %d] START pathfind for attacker [%d - %s]\n",__FILE__,__FUNCTION__,__LINE__,unit->getId(), unit->getType()->getName().c_str());
				//fflush(stdout);
				switch(this->game->getGameSettings()->getPathFinderType()) {
					case pfBasic:
						tsValue = pathFinder->findPath(unit, pos, NULL, frameIndex);
						break;
					default:
						throw megaglest_runtime_error("detected unsupported pathfinder type!");
				}
				//printf("In [%s::%s Line: %d] END pathfind for attacker [%d - %s]\n",__FILE__,__FUNCTION__,__LINE__,unit->getId(), unit->getType()->getName().c_str());
				//fflush(stdout);
			}

			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

			if(frameIndex < 0) {
				if(command->getUnit() != NULL && !command->getUnit()->isAlive() && unit->getCommandSize() > 1) {
					// don't run over to dead body if there is still something to do in the queue
					unit->finishCommand();

					if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
						char szBuf[8096]="";
						snprintf(szBuf,8096,"[updateAttack]");
						unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
					}
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
							break;
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

					if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
						char szBuf[8096]="";
						snprintf(szBuf,8096,"[updateAttack]");
						unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
					}

				}
			}

			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
		}
    }

    if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld --------------------------- [END OF METHOD] ---------------------------\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	}
	catch(const exception &ex) {
		//setRunningStatus(false);

		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Loc [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		throw megaglest_runtime_error(ex.what());
	}
	catch(...) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s %d] UNKNOWN error\n",__FILE__,__FUNCTION__,__LINE__);
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		throw megaglest_runtime_error(szBuf);
	}

}


// ==================== updateAttackStopped ====================

void UnitUpdater::updateAttackStopped(Unit *unit, int frameIndex) {
	try {

	// Nothing to do
	if(frameIndex >= 0) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex >= 0) {
			char szBuf[8096]="";
			snprintf(szBuf,8096,"[updateAttackStopped]");
			unit->logSynchDataThreaded(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
		}
		clearUnitPrecache(unit);
		return;
	}

	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

	Command *command= unit->getCurrCommand();
	if(command == NULL) {
		throw megaglest_runtime_error("command == NULL");
	}

    const AttackStoppedCommandType *asct= static_cast<const AttackStoppedCommandType*>(command->getCommandType());
    Unit *enemy=NULL;


    if(unit->getCommandSize() > 1) {
    	if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
    		char szBuf[8096]="";
    		snprintf(szBuf,8096,"[updateAttackStopped]");
    		unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
    	}

    	unit->finishCommand(); // attackStopped is skipped if somthing else is queued after this.
    	return;
    }


    float distToUnit=-1;
    std::pair<bool,Unit *> result = make_pair(false,(Unit *)NULL);
    unitBeingAttacked(result, unit, asct->getAttackSkillType(), &distToUnit);


	if(result.first == true) {
        unit->setCurrSkill(asct->getAttackSkillType());
		unit->setTarget(result.second);

    	if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
    		char szBuf[8096]="";
    		snprintf(szBuf,8096,"[updateAttackStopped]");
    		unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
    	}
	}
	else if(attackableOnRange(unit, &enemy, asct->getAttackSkillType(),(frameIndex >= 0))) {
        unit->setCurrSkill(asct->getAttackSkillType());
		unit->setTarget(enemy);

    	if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
    		char szBuf[8096]="";
    		snprintf(szBuf,8096,"[updateAttackStopped]");
    		unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
    	}
    }
    else {
        unit->setCurrSkill(asct->getStopSkillType());

    	if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
    		char szBuf[8096]="";
    		snprintf(szBuf,8096,"[updateAttackStopped]");
    		unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
    	}
    }

    if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld --------------------------- [END OF METHOD] ---------------------------\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	}
	catch(const exception &ex) {
		//setRunningStatus(false);

		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		throw megaglest_runtime_error(ex.what());
	}
	catch(...) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s %d] UNKNOWN error\n",__FILE__,__FUNCTION__,__LINE__);
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		throw megaglest_runtime_error(szBuf);
	}

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
	for(unsigned int i = 0; i < (unsigned int)unit->getType()->getSkillTypeCount(); ++i) {
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
	try {

	if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"[updateBuild]");
		unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
	}

	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] unit [%s] will build using command [%s]\n",__FILE__,__FUNCTION__,__LINE__,unit->toString(false).c_str(), unit->getCurrCommand()->toString(false).c_str());

	Command *command= unit->getCurrCommand();
	if(command == NULL) {
		throw megaglest_runtime_error("command == NULL");
	}

    const BuildCommandType *bct= static_cast<const BuildCommandType*>(command->getCommandType());

	if(unit->getCurrSkill() != NULL && unit->getCurrSkill()->getClass() != scBuild) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

        //if not building
        const UnitType *ut= command->getUnitType();

        if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

		TravelState tsValue = tsImpossible;
		switch(this->game->getGameSettings()->getPathFinderType()) {
			case pfBasic:
				{
				Vec2i buildPos = map->findBestBuildApproach(unit, command->getPos(), ut);

				if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
					char szBuf[8096]="";
					snprintf(szBuf,8096,"[updateBuild] unit->getPos() [%s] command->getPos() [%s] buildPos [%s]",
							unit->getPos().getString().c_str(),command->getPos().getString().c_str(),buildPos.getString().c_str());
					unit->logSynchData(__FILE__,__LINE__,szBuf);
				}

				tsValue = pathFinder->findPath(unit, buildPos, NULL, frameIndex);

				if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
					char szBuf[8096]="";
					snprintf(szBuf,8096,"[updateBuild] tsValue: %d",tsValue);
					unit->logSynchData(__FILE__,__LINE__,szBuf);
				}

				}
				break;
			default:
				throw megaglest_runtime_error("detected unsupported pathfinder type!");
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
					throw megaglest_runtime_error("ut == NULL");
				}

				bool canOccupyCell = false;
				switch(this->game->getGameSettings()->getPathFinderType()) {
					case pfBasic:
						if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] tsArrived about to call map->isFreeCells() for command->getPos() = %s, ut->getSize() = %d\n",__FILE__,__FUNCTION__,__LINE__,command->getPos().getString().c_str(),ut->getSize());
						canOccupyCell = map->isFreeCells(command->getPos(), ut->getSize(), fLand);
						break;
					default:
						throw megaglest_runtime_error("detected unsupported pathfinder type!");
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
						default:
							throw megaglest_runtime_error("detected unsupported pathfinder type!");
					}

					Vec2i buildPos = command->getPos();
					Unit *builtUnit= new Unit(world->getNextUnitId(unit->getFaction()), newpath, buildPos, builtUnitType, unit->getFaction(), world->getMap(), facing);

					if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

					builtUnit->create();

					if(builtUnitType->hasSkillClass(scBeBuilt) == false) {
						throw megaglest_runtime_error("Unit [" + builtUnitType->getName(false) + "] has no be_built skill, producer was [" + intToStr(unit->getId()) + " - " + unit->getType()->getName(false) + "].");
					}

					builtUnit->setCurrSkill(scBeBuilt);

					unit->setCurrSkill(bct->getBuildSkillType());
					unit->setTarget(builtUnit);


					map->prepareTerrain(builtUnit);

					if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

					switch(this->game->getGameSettings()->getPathFinderType()) {
						case pfBasic:
							break;
						default:
							throw megaglest_runtime_error("detected unsupported pathfinder type!");
					}

					command->setUnit(builtUnit);


					//play start sound
					if(unit->getFactionIndex() == world->getThisFactionIndex() ||
						(game->getWorld()->showWorldForPlayer(game->getWorld()->getThisTeamIndex()) == true)) {
						SoundRenderer::getInstance().playFx(
							bct->getStartSound(),
							unit->getCurrMidHeightVector(),
							gameCamera->getPos());
					}

					if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] unit created for unit [%s]\n",__FILE__,__FUNCTION__,__LINE__,builtUnit->toString(false).c_str());
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
    	if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] tsArrived unit = %s\n",__FILE__,__FUNCTION__,__LINE__,unit->toString(false).c_str());

    	if(frameIndex < 0) {
			//if building
			Unit *builtUnit = map->getCell(unit->getTargetPos())->getUnit(fLand);
			if(builtUnit == NULL) {
				builtUnit = map->getCell(unit->getTargetPos())->getUnitWithEmptyCellMap(fLand);
			}

			if(builtUnit != NULL) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] builtUnit = %s\n",__FILE__,__FUNCTION__,__LINE__,builtUnit->toString(false).c_str());
			}

			if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] builtUnit = [%p]\n",__FILE__,__FUNCTION__,__LINE__,builtUnit);

			//if unit is killed while building then u==NULL;
			if(builtUnit != NULL && builtUnit != command->getUnit()) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] builtUnit is not the command's unit!\n",__FILE__,__FUNCTION__,__LINE__);

				if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
					char szBuf[8096]="";
					snprintf(szBuf,8096,"[updateBuild]");
					unit->logSynchData(__FILE__,__LINE__,szBuf);
				}

				unit->setCurrSkill(scStop);
			}
			else if(builtUnit == NULL || builtUnit->isBuilt()) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] builtUnit is NULL or ALREADY built\n",__FILE__,__FUNCTION__,__LINE__);

				if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
					char szBuf[8096]="";
					snprintf(szBuf,8096,"[updateBuild]");
					unit->logSynchData(__FILE__,__LINE__,szBuf);
				}

				unit->finishCommand();
				unit->setCurrSkill(scStop);

			}
			else if(builtUnit == NULL || builtUnit->repair()) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

				if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
					char szBuf[8096]="";
					snprintf(szBuf,8096,"[updateBuild]");
					unit->logSynchData(__FILE__,__LINE__,szBuf);
				}

				const CommandType *ct = (command != NULL ? command->getCommandType() : NULL);
				//building finished
				unit->finishCommand();
				unit->setCurrSkill(scStop);

				builtUnit->born(ct);
				scriptManager->onUnitCreated(builtUnit);
				if(unit->getFactionIndex() == world->getThisFactionIndex() ||
					(game->getWorld()->showWorldForPlayer(game->getWorld()->getThisTeamIndex()) == true)) {
					SoundRenderer::getInstance().playFx(
						bct->getBuiltSound(),
						unit->getCurrMidHeightVector(),
						gameCamera->getPos());
				}
			}
    	}
    	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
    }

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld --------------------------- [END OF METHOD] ---------------------------\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	}
	catch(const exception &ex) {
		//setRunningStatus(false);

		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		throw megaglest_runtime_error(ex.what());
	}
	catch(...) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s %d] UNKNOWN error\n",__FILE__,__FUNCTION__,__LINE__);
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		throw megaglest_runtime_error(szBuf);
	}

}

// ==================== updateHarvest ====================
void UnitUpdater::updateHarvestEmergencyReturn(Unit *unit, int frameIndex) {
	try {

	if(frameIndex >= 0) {
		return;
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"[updateHarvestEmergencyReturn]");
		unit->logSynchDataThreaded(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
	}

	//printf("\n#1 updateHarvestEmergencyReturn\n");
	Command *command= unit->getCurrCommand();
	if(command != NULL) {
		//printf("\n#2 updateHarvestEmergencyReturn\n");

		//const HarvestCommandType *hct= dynamic_cast<const HarvestCommandType*>((command != NULL ? command->getCommandType() : NULL));
		//if(hct != NULL) {
		{
			//printf("\n#3 updateHarvestEmergencyReturn\n");

			const Vec2i unitTargetPos = command->getPos();
			Cell *cell= map->getCell(unitTargetPos);
			if(cell != NULL && cell->getUnit(unit->getCurrField()) != NULL) {
				//printf("\n#4 updateHarvestEmergencyReturn\n");

				Unit *targetUnit = cell->getUnit(unit->getCurrField());
				if(targetUnit != NULL) {
					//printf("\n#5 updateHarvestEmergencyReturn\n");
					// Check if we can return whatever resources we have
					if(targetUnit->getFactionIndex() == unit->getFactionIndex() &&
						targetUnit->isOperative() == true && unit->getLoadType() != NULL &&
						targetUnit->getType() != NULL && targetUnit->getType()->getStore(unit->getLoadType()) > 0) {
						//printf("\n#6 updateHarvestEmergencyReturn\n");

						const HarvestCommandType *previousHarvestCmd = unit->getType()->getFirstHarvestCommand(unit->getLoadType(),unit->getFaction());
						if(previousHarvestCmd != NULL) {
							//printf("\n\n#1a return harvested resources\n\n");
							NetworkCommand networkCommand(this->world,nctGiveCommand, unit->getId(), previousHarvestCmd->getId(), unit->getLastHarvestedResourcePos(),
															-1, Unit::invalidId, -1, false, cst_None, -1, -1);

							if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

							Command* new_command= this->game->getCommander()->buildCommand(&networkCommand);
							new_command->setStateType(cst_EmergencyReturnResource);
							new_command->setStateValue(1);
							std::pair<CommandResult,string> cr= unit->checkCommand(new_command);
							if(cr.first == crSuccess) {
								//printf("\n\n#1b return harvested resources\n\n");

								if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
								unit->replaceCurrCommand(new_command);

								unit->setCurrSkill(previousHarvestCmd->getStopLoadedSkillType()); // make sure we use the right harvest animation
							}
							else {
								//printf("\n\n#1c return harvested resources\n\n");

								if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
								delete new_command;

								unit->setCurrSkill(scStop);
								unit->finishCommand();
							}
						}
					}
				}
			}
		}
	}

	}
	catch(const exception &ex) {
		//setRunningStatus(false);

		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		throw megaglest_runtime_error(ex.what());
	}
	catch(...) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s %d] UNKNOWN error\n",__FILE__,__FUNCTION__,__LINE__);
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		throw megaglest_runtime_error(szBuf);
	}

}

void UnitUpdater::updateHarvest(Unit *unit, int frameIndex) {
	try {

	if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"[updateHarvest]");
		unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
	}

	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

	Command *command= unit->getCurrCommand();
	if(command == NULL) {
		throw megaglest_runtime_error("command == NULL");
	}

    const HarvestCommandType *hct= dynamic_cast<const HarvestCommandType*>(command->getCommandType());
    if(hct == NULL) {
    	throw megaglest_runtime_error("hct == NULL");
    }
	Vec2i targetPos(-1);

	//TravelState tsValue = tsImpossible;
	//UnitPathInterface *path= unit->getPath();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
	//printf("In UpdateHarvest [%d - %s] unit->getCurrSkill()->getClass() = %d\n",unit->getId(),unit->getType()->getName().c_str(),unit->getCurrSkill()->getClass());

	Resource *harvestResource = NULL;
	SurfaceCell *sc = map->getSurfaceCell(Map::toSurfCoords(command->getPos()));
	if(sc != NULL) {
		harvestResource = sc->getResource();
	}

	if(unit->getCurrSkill() != NULL && unit->getCurrSkill()->getClass() != scHarvest) {
		bool forceReturnToStore = (command != NULL &&
							command->getStateType() == cst_EmergencyReturnResource && command->getStateValue() == 1);

		//if not working
		if(unit->getLoadCount() == 0 ||
			(forceReturnToStore == false && unit->getLoadType() != NULL &&
			 harvestResource != NULL && (unit->getLoadCount() < hct->getMaxLoad()) &&
			 harvestResource->getType() != NULL && unit->getLoadType() == harvestResource->getType())) {
			//if not loaded go for resources
			SurfaceCell *sc = map->getSurfaceCell(Map::toSurfCoords(command->getPos()));
			if(sc != NULL) {
				Resource *r = sc->getResource();
				if(r != NULL && hct != NULL && hct->canHarvest(r->getType())) {
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
	    							isNearResource = map->isResourceNear(frameIndex,unit->getPos(), r->getType(), targetPos,unit->getType()->getSize(),unit, false,&clickPos);
	    						}
	    						else {
	    							isNearResource = map->isResourceNear(frameIndex,unit->getPos(), r->getType(), targetPos,unit->getType()->getSize(),unit);
	    						}
	    						if(isNearResource == true) {
	    							if((unit->getPos().dist(command->getPos()) < harvestDistance || unit->getPos().dist(targetPos) < harvestDistance) && isNearResource == true) {
	    								canHarvestDestPos = true;
	    							}
	    						}
	    						else if(newHarvestPath == true) {
	    							if(clickPos != command->getOriginalPos()) {
	    								//printf("%%----------- unit [%s - %d] CHANGING RESOURCE POS from [%s] to [%s]\n",unit->getFullName().c_str(),unit->getId(),command->getOriginalPos().getString().c_str(),clickPos.getString().c_str());

										if(frameIndex < 0) {
											if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
												char szBuf[8096]="";
												snprintf(szBuf,8096,"[updateHarvest] clickPos [%s]",clickPos.getString().c_str());
												unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
											}

	    									command->setPos(clickPos);
										}
	    							}
	    						}
	    					}
	    					break;
	    				default:
	    					throw megaglest_runtime_error("detected unsupported pathfinder type!");
	    			}

	    			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

					if (canHarvestDestPos == true ) {
						if(frameIndex < 0) {
							if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
								char szBuf[8096]="";
								snprintf(szBuf,8096,"[updateHarvest]");
								unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
							}

							unit->setLastHarvestResourceTarget(NULL);
						}

						canHarvestDestPos = (map->getSurfaceCell(Map::toSurfCoords(targetPos)) != NULL && map->getSurfaceCell(Map::toSurfCoords(targetPos))->getResource() != NULL && map->getSurfaceCell(Map::toSurfCoords(targetPos))->getResource()->getType() != NULL);

						if(canHarvestDestPos == true) {
							if(frameIndex < 0) {
								//if it finds resources it starts harvesting
								unit->setCurrSkill(hct->getHarvestSkillType());
								unit->setTargetPos(targetPos);
								command->setPos(targetPos);

								if(unit->getLoadType() == NULL || harvestResource == NULL ||
								   harvestResource->getType() == NULL || unit->getLoadType() != harvestResource->getType()) {
									unit->setLoadCount(0);
								}

								unit->getFaction()->addResourceTargetToCache(targetPos);

								switch(this->game->getGameSettings()->getPathFinderType()) {
									case pfBasic:
										unit->setLoadType(r->getType());
										break;
									default:
										throw megaglest_runtime_error("detected unsupported pathfinder type!");
								}

								if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
									char szBuf[8096]="";
									snprintf(szBuf,8096,"[updateHarvest] targetPos [%s]",targetPos.getString().c_str());
									unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
								}
							}
							if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
						}
					}
					if(canHarvestDestPos == false) {
						if(frameIndex < 0) {
							if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
								char szBuf[8096]="";
								snprintf(szBuf,8096,"[updateHarvest] targetPos [%s]",targetPos.getString().c_str());
								unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
							}

							unit->setLastHarvestResourceTarget(&targetPos);
						}

						if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

						if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
							char szBuf[8096]="";
							snprintf(szBuf,8096,"[updateHarvest] unit->getPos() [%s] command->getPos() [%s]",
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
		    				default:
		    					throw megaglest_runtime_error("detected unsupported pathfinder type!");
		    			}

		    			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

		    			// If the unit is blocked or Even worse 'stuck' then try to
		    			// find the same resource type elsewhere, but close by
		    			if((wasStuck == true || tsValue == tsBlocked) && unit->isAlive() == true) {
		    				switch(this->game->getGameSettings()->getPathFinderType()) {
								case pfBasic:
									{
										bool isNearResource = map->isResourceNear(frameIndex,unit->getPos(), r->getType(), targetPos,unit->getType()->getSize(),unit,true);
										if(isNearResource == true) {
											if((unit->getPos().dist(command->getPos()) < harvestDistance || unit->getPos().dist(targetPos) < harvestDistance) && isNearResource == true) {
												canHarvestDestPos = true;
											}
										}
									}
									break;
								default:
									throw megaglest_runtime_error("detected unsupported pathfinder type!");
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

										if(unit->getLoadType() == NULL || harvestResource == NULL ||
										   harvestResource->getType() == NULL || unit->getLoadType() != harvestResource->getType()) {
											unit->setLoadCount(0);
										}
										unit->getFaction()->addResourceTargetToCache(targetPos);

										switch(this->game->getGameSettings()->getPathFinderType()) {
											case pfBasic:
												unit->setLoadType(r->getType());
												break;
											default:
												throw megaglest_runtime_error("detected unsupported pathfinder type!");
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
										char szBuf[8096]="";
										snprintf(szBuf,8096,"[updateHarvest #2] unit->getPos() [%s] command->getPos() [%s] targetPos [%s]",
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
										default:
											throw megaglest_runtime_error("detected unsupported pathfinder type!");
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
			}

			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
		}
		else {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

			//if loaded, return to store
			Unit *store= world->nearestStore(unit->getPos(), unit->getFaction()->getIndex(), unit->getLoadType());
			if(store != NULL) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
					char szBuf[8096]="";
					snprintf(szBuf,8096,"[updateHarvest #3] unit->getPos() [%s] store->getCenteredPos() [%s]",
							unit->getPos().getString().c_str(),store->getCenteredPos().getString().c_str());
					unit->logSynchData(__FILE__,__LINE__,szBuf);
				}

				TravelState tsValue = tsImpossible;
	    		switch(this->game->getGameSettings()->getPathFinderType()) {
	    			case pfBasic:
	    				tsValue = pathFinder->findPath(unit, store->getCenteredPos(), NULL, frameIndex);
	    				break;
	    			default:
	    				throw megaglest_runtime_error("detected unsupported pathfinder type!");
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
					if(map->isNextTo(unit, store)) {
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

					unit->setLastHarvestedResourcePos(unitTargetPos);

					if (unit->getProgress2() >= hct->getHitsPerUnit()) {
						if (unit->getLoadCount() < hct->getMaxLoad()) {
							unit->setProgress2(0);
							unit->setLoadCount(unit->getLoadCount() + 1);

							//if resource exausted, then delete it and stop
							if (sc->decAmount(1)) {
								//const ResourceType *rt = r->getType();
								sc->deleteResource();
								world->removeResourceTargetFromCache(unitTargetPos);

								switch(this->game->getGameSettings()->getPathFinderType()) {
									case pfBasic:
										break;
									default:
										throw megaglest_runtime_error("detected unsupported pathfinder type!");
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
	catch(const exception &ex) {
		//setRunningStatus(false);

		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		throw megaglest_runtime_error(ex.what());
	}
	catch(...) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s %d] UNKNOWN error\n",__FILE__,__FUNCTION__,__LINE__);
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		throw megaglest_runtime_error(szBuf);
	}

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
								if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] **peer NOT building**, peerUnit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,peerUnit->toString(false).c_str());

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

    if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] returning foundUnitBuilder = [%s]\n",__FILE__,__FUNCTION__,__LINE__,(foundUnitBuilder != NULL ? foundUnitBuilder->toString(false).c_str() : "null"));

    return foundUnitBuilder;
}

// ==================== updateRepair ====================

void UnitUpdater::updateRepair(Unit *unit, int frameIndex) {
	try {

	// Nothing to do
	if(frameIndex >= 0) {
		clearUnitPrecache(unit);
		return;
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"[updateRepair]");
		unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
	}

	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] unit = %p\n",__FILE__,__FUNCTION__,__LINE__,unit);

	//if(unit != NULL) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] unit doing the repair [%s] - %d\n",__FILE__,__FUNCTION__,__LINE__,unit->getFullName(false).c_str(),unit->getId());
	//}
    Command *command= unit->getCurrCommand();
    if(command == NULL) {
    	throw megaglest_runtime_error("command == NULL");
    }

    const RepairCommandType *rct= static_cast<const RepairCommandType*>(command->getCommandType());
    const CommandType *ct = (command != NULL ? command->getCommandType() : NULL);

    if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] rct = %p\n",__FILE__,__FUNCTION__,__LINE__,rct);

	Unit *repaired = (command != NULL ? map->getCell(command->getPos())->getUnitWithEmptyCellMap(fLand) : NULL);
	if(repaired == NULL && command != NULL) {
		repaired = map->getCell(command->getPos())->getUnit(fLand);
	}

	if(repaired != NULL) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] unit to repair [%s] - %d\n",__FILE__,__FUNCTION__,__LINE__,repaired->getFullName(false).c_str(),repaired->getId());
	}

	if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	// Check if the 'repaired' unit is actually the peer unit in a multi-build?
	Unit *peerUnitBuilder = findPeerUnitBuilder(unit);

	if(peerUnitBuilder != NULL) {
		SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] unit peer [%s] - %d\n",__FILE__,__FUNCTION__,__LINE__,peerUnitBuilder->getFullName(false).c_str(),peerUnitBuilder->getId());
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

	bool nextToRepaired = repaired != NULL && map->isNextTo(unit, repaired);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	peerUnitBuilder = NULL;
	if(repaired == NULL) {
		peerUnitBuilder = findPeerUnitBuilder(unit);
		if(peerUnitBuilder != NULL) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] peerUnitBuilder = %p\n",__FILE__,__FUNCTION__,__LINE__,peerUnitBuilder);

			if(peerUnitBuilder->getCurrCommand()->getUnit() != NULL) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] peerbuilder's unitid = %d\n",__FILE__,__FUNCTION__,__LINE__,peerUnitBuilder->getCurrCommand()->getUnit()->getId());
				repaired = peerUnitBuilder->getCurrCommand()->getUnit();
				nextToRepaired = repaired != NULL && map->isNextTo(unit, repaired);
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
						std::pair<CommandResult,string> cr= unit->checkCommand(command);
						if(cr.first == crSuccess) {
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
		if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] unit to repair[%s]\n",__FILE__,__FUNCTION__,__LINE__,repaired->getFullName(false).c_str());
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] repaired = %p, nextToRepaired = %d, unit->getCurrSkill()->getClass() = %d\n",__FILE__,__FUNCTION__,__LINE__,repaired,nextToRepaired,unit->getCurrSkill()->getClass());

	//UnitPathInterface *path= unit->getPath();
	if(unit->getCurrSkill()->getClass() != scRepair ||
		(nextToRepaired == false && peerUnitBuilder == NULL)) {

		if(command == NULL) {
			throw megaglest_runtime_error("command == NULL");
		}
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
					char szBuf[8096]="";
					snprintf(szBuf,8096,"[updateRepair] unit->getPos() [%s] command->getPos()() [%s] repairPos [%s]",unit->getPos().getString().c_str(),command->getPos().getString().c_str(),repairPos.getString().c_str());
					unit->logSynchData(__FILE__,__LINE__,szBuf);
				}

				if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

				// If the repair command has no move skill and we are not next to
				// the unit we cannot repair it
				if(rct->getMoveSkillType() == NULL) {
					//printf("CANCEL REPAIR NOT NEXT TO REPAIR UNIT\n");
					//Vec2i repairPos = command->getPos();
					//bool startRepairing = (repaired != NULL && rct->isRepairableUnitType(repaired->getType()) && repaired->isDamaged());
					//bool nextToRepaired = repaired != NULL && map->isNextTo(unit, repaired);

					//printf("repairPos [%s] startRepairing = %d nextToRepaired = %d unit->getPos() [%s] repaired->getPos() [%s]\n",repairPos.getString().c_str(),startRepairing,nextToRepaired,unit->getPos().getString().c_str(),repaired->getPos().getString().c_str());

					console->addStdMessage("InvalidPosition");
					unit->setCurrSkill(scStop);
					unit->finishCommand();
				}
				else {
					TravelState ts;
					switch(this->game->getGameSettings()->getPathFinderType()) {
						case pfBasic:
							if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

							ts = pathFinder->findPath(unit, repairPos, NULL, frameIndex);
							break;
						default:
							throw megaglest_runtime_error("detected unsupported pathfinder type!");
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
        }
        else {
        	if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] about to call [scStop]\n",__FILE__,__FUNCTION__,__LINE__);

        	//console->addStdMessage("InvalidPosition");
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
				repaired->born(ct);
				scriptManager->onUnitCreated(repaired);
			}
			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
		}
    }
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld --------------------------- [END OF METHOD] ---------------------------\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	}
	catch(const exception &ex) {
		//setRunningStatus(false);

		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		throw megaglest_runtime_error(ex.what());
	}
	catch(...) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s %d] UNKNOWN error\n",__FILE__,__FUNCTION__,__LINE__);
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		throw megaglest_runtime_error(szBuf);
	}

}

// ==================== updateProduce ====================

void UnitUpdater::updateProduce(Unit *unit, int frameIndex) {
	try {

	// Nothing to do
	if(frameIndex >= 0) {
		clearUnitPrecache(unit);
		return;
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"[updateProduce]");
		unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
	}

	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

    Command *command= unit->getCurrCommand();
	if(command == NULL) {
		throw megaglest_runtime_error("command == NULL");
	}

    const ProduceCommandType *pct= static_cast<const ProduceCommandType*>(command->getCommandType());
    Unit *produced;

    if(unit->getCurrSkill()->getClass() != scProduce) {
        //if not producing
        unit->setCurrSkill(pct->getProduceSkillType());
    }
    else {
    	const CommandType *ct = (command != NULL ? command->getCommandType() : NULL);

		unit->update2();

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

        if(unit->getProgress2() > pct->getProduced()->getProductionTime()){
            unit->finishCommand();
            unit->setCurrSkill(scStop);

			UnitPathInterface *newpath = NULL;
			switch(this->game->getGameSettings()->getPathFinderType()) {
				case pfBasic:
					newpath = new UnitPathBasic();
					break;
				default:
					throw megaglest_runtime_error("detected unsupported pathfinder type!");
		    }

			produced= new Unit(world->getNextUnitId(unit->getFaction()), newpath, Vec2i(0), pct->getProducedUnit(), unit->getFaction(), world->getMap(), CardinalDir::NORTH);

			if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] about to place unit for unit [%s]\n",__FILE__,__FUNCTION__,__LINE__,produced->toString(false).c_str());

			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

			//place unit creates the unit
			if(!world->placeUnit(unit->getCenteredPos(), 10, produced)) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] COULD NOT PLACE UNIT for unitID [%d]\n",__FILE__,__FUNCTION__,__LINE__,produced->getId());
				delete produced;
			}
			else{
				produced->create();
				produced->born(ct);

				world->getStats()->produce(unit->getFactionIndex(),produced->getType()->getCountUnitProductionInStats());
				const CommandType *ct= produced->computeCommandType(unit->getMeetingPos());
				if(ct != NULL) {
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
	catch(const exception &ex) {
		//setRunningStatus(false);

		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		throw megaglest_runtime_error(ex.what());
	}
	catch(...) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s %d] UNKNOWN\n",__FILE__,__FUNCTION__,__LINE__);
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		throw megaglest_runtime_error(szBuf);
	}

}


// ==================== updateUpgrade ====================

void UnitUpdater::updateUpgrade(Unit *unit, int frameIndex) {
	try {

	// Nothing to do
	if(frameIndex >= 0) {
		clearUnitPrecache(unit);
		return;
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"[updateUpgrade]");
		unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
	}

	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

    Command *command= unit->getCurrCommand();
	if(command == NULL) {
		throw megaglest_runtime_error("command == NULL");
	}

    const UpgradeCommandType *uct= static_cast<const UpgradeCommandType*>(command->getCommandType());

	if(unit->getCurrSkill()->getClass() != scUpgrade) {
		//if not producing
		unit->setCurrSkill(uct->getUpgradeSkillType());
    }
	else {
		//if producing
		unit->update2();
        if(unit->getProgress2() > uct->getProduced()->getProductionTime()){
            unit->finishCommand();
            unit->setCurrSkill(scStop);
			unit->getFaction()->finishUpgrade(uct->getProducedUpgrade());
        }
    }

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld --------------------------- [END OF METHOD] ---------------------------\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	}
	catch(const exception &ex) {
		//setRunningStatus(false);

		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		throw megaglest_runtime_error(ex.what());
	}
	catch(...) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s %d] UNKNOWN error\n",__FILE__,__FUNCTION__,__LINE__);
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		throw megaglest_runtime_error(szBuf);
	}

}

// ==================== updateMorph ====================

void UnitUpdater::updateMorph(Unit *unit, int frameIndex) {
	try {

	// Nothing to do
	if(frameIndex >= 0) {
		clearUnitPrecache(unit);
		return;
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"[updateMorph]");
		unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
	}

	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

	Command *command= unit->getCurrCommand();
	if(command == NULL) {
		throw megaglest_runtime_error("command == NULL");
	}

    const MorphCommandType *mct= static_cast<const MorphCommandType*>(command->getCommandType());

    if(unit->getCurrSkill()->getClass()!=scMorph){
		//if not morphing, check space
		if(map->canMorph(unit->getPos(),unit,mct->getMorphUnit())){
			unit->setCurrSkill(mct->getMorphSkillType());
			// block space for morphing units ( block space before and after morph ! )
			map->putUnitCells(unit, unit->getPos());
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
        if(unit->getProgress2() > mct->getProduced()->getProductionTime()){
			//int oldSize = 0;
			//bool needMapUpdate = false;

    		switch(this->game->getGameSettings()->getPathFinderType()) {
    			case pfBasic:
    				break;
    			default:
    				throw megaglest_runtime_error("detected unsupported pathfinder type!");
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
	    			default:
	    				throw megaglest_runtime_error("detected unsupported pathfinder type!");
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
	catch(const exception &ex) {
		//setRunningStatus(false);

		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		throw megaglest_runtime_error(ex.what());
	}
	catch(...) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s %d] UNKNOWN error\n",__FILE__,__FUNCTION__,__LINE__);
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		throw megaglest_runtime_error(szBuf);
	}

}


// ==================== updateMove ====================
void UnitUpdater::updateSwitchTeam(Unit *unit, int frameIndex) {
}

// ==================== updateAttack ====================

// ==================== PRIVATE ====================

// ==================== attack ====================

void UnitUpdater::hit(Unit *attacker){
	hit(attacker, dynamic_cast<const AttackSkillType*>(attacker->getCurrSkill()), attacker->getTargetPos(), attacker->getTargetField(),100);
}

void UnitUpdater::hit(Unit *attacker, const AttackSkillType* ast, const Vec2i &targetPos, Field targetField, int damagePercent){
	//hit attack positions
	if(ast != NULL && ast->getSplash()) {
		char szBuf[8096]="";
		snprintf(szBuf,8095,"Unit hitting [UnitUpdater::hit] hasSplash = %d radius = %d damageall = %d",ast->getSplash(),ast->getSplashRadius(),ast->getSplashDamageAll());
		attacker->addNetworkCRCDecHp(szBuf);

		PosCircularIterator pci(map, targetPos, ast->getSplashRadius());
		while(pci.next()) {
			Unit *attacked= map->getCell(pci.getPos())->getUnit(targetField);
			if(attacked != NULL) {
				if(ast->getSplashDamageAll() ||
						attacker->isAlly(attacked) == false ||
						( targetPos.x == pci.getPos().x && targetPos.y == pci.getPos().y )) {

					attacker->setLastAttackedUnitId(attacked->getId());
					scriptManager->onUnitAttacking(attacker);

					float distance = pci.getPos().dist(targetPos);
					distance = truncateDecimal<float>(distance,6);
					damage(attacker, ast, attacked, distance,damagePercent);
			  	}
			}
		}
	}
	else {
		Unit *attacked= map->getCell(targetPos)->getUnit(targetField);

		char szBuf[8096]="";
		snprintf(szBuf,8095,"Unit hitting [UnitUpdater::hit 2] attacked = %d",(attacked != NULL ? attacked->getId() : -1));
		attacker->addNetworkCRCDecHp(szBuf);

		if(attacked != NULL) {
			damage(attacker, ast, attacked, 0.f,damagePercent);
		}
	}
}

void UnitUpdater::damage(Unit *attacker, const AttackSkillType* ast, Unit *attacked, float distance, int damagePercent) {
	if(attacker == NULL) {
		throw megaglest_runtime_error("attacker == NULL");
	}
	if(ast == NULL) {
		throw megaglest_runtime_error("ast == NULL");
	}
	if(attacked == NULL) {
		throw megaglest_runtime_error("attacked == NULL");
	}

	//get vars
	float damage			= ast->getTotalAttackStrength(attacker->getTotalUpgrade());
	int var					= ast->getAttackVar();
	int armor				= attacked->getType()->getTotalArmor(attacked->getTotalUpgrade());
	float damageMultiplier	= world->getTechTree()->getDamageMultiplier(ast->getAttackType(), attacked->getType()->getArmorType());
	damageMultiplier = truncateDecimal<float>(damageMultiplier,6);

	//compute damage
	//damage += random.randRange(-var, var);
	damage += attacker->getRandom()->randRange(-var, var, extractFileFromDirectoryPath(__FILE__) + intToStr(__LINE__));
	damage /= distance+1;
	damage -= armor;
	damage *= damageMultiplier;
	damage = truncateDecimal<float>(damage,6);

	damage = (damage*damagePercent)/100;
	if(damage < 1) {
		damage= 1;
	}
	int damageVal = static_cast<int>(damage);

	attacked->setLastAttackerUnitId(attacker->getId());

	char szBuf[8096]="";
	snprintf(szBuf,8095,"Unit hitting [UnitUpdater::damage] damageVal = %d",damageVal);
	attacker->addNetworkCRCDecHp(szBuf);

	//damage the unit
	if(attacked->decHp(damageVal)) {
		world->getStats()->kill(attacker->getFactionIndex(), attacked->getFactionIndex(), attacker->getTeam() != attacked->getTeam(),attacked->getType()->getCountUnitDeathInStats(),attacked->getType()->getCountUnitKillInStats());
		if(attacked->getType()->getCountKillForUnitUpgrade() == true){
			attacker->incKills(attacked->getTeam());
		}

		// Perform resource looting iff the attack is from a different faction
		if(attacker->getFaction() != attacked->getFaction()) {
			int lootableResourceCount = attacked->getType()->getLootableResourceCount();
			for(int i = 0; i < lootableResourceCount; i++) {
				LootableResource resource = attacked->getType()->getLootableResource(i);
			
				// Figure out how much of the resource in question that the attacked unit's faction has
				int factionTotalResource = 0;
				for(int j = 0; j < attacked->getFaction()->getTechTree()->getResourceTypeCount(); j++) {
					if(attacked->getFaction()->getTechTree()->getResourceType(j) == resource.getResourceType()) {
						factionTotalResource = attacked->getFaction()->getResource(j)->getAmount();
						break;
					}
				}
			
				if(resource.isNegativeAllowed()) {
					attacked->getFaction()->incResourceAmount(resource.getResourceType(), -(factionTotalResource * resource.getLossFactionPercent() / 100));
					attacked->getFaction()->incResourceAmount(resource.getResourceType(), -resource.getLossValue());
					attacker->getFaction()->incResourceAmount(resource.getResourceType(), factionTotalResource * resource.getAmountFactionPercent() / 100);
					attacker->getFaction()->incResourceAmount(resource.getResourceType(), resource.getAmountValue());
				}
				// Can't take more resources than the faction has, otherwise we end up in the negatives
				else {
					attacked->getFaction()->incResourceAmount(resource.getResourceType(), -(std::min)(factionTotalResource * resource.getLossFactionPercent() / 100, factionTotalResource));
					attacked->getFaction()->incResourceAmount(resource.getResourceType(), -(std::min)(resource.getLossValue(), factionTotalResource));
					attacker->getFaction()->incResourceAmount(resource.getResourceType(), (std::min)(factionTotalResource * resource.getAmountFactionPercent() / 100, factionTotalResource));
					attacker->getFaction()->incResourceAmount(resource.getResourceType(), (std::min)(resource.getAmountValue(), factionTotalResource));
				}
			}
		}

		switch(this->game->getGameSettings()->getPathFinderType()) {
			case pfBasic:
				break;
			default:
				throw megaglest_runtime_error("detected unsupported pathfinder type!");
	    }

		attacked->setCauseOfDeath(ucodAttacked);
		scriptManager->onUnitDied(attacked);
	}

	if(attacked->isAlive() == true) {
		scriptManager->onUnitAttacked(attacked);
	}

	// !!! Is this causing choppy network play somehow?
	//attacker->computeHp();
}

void UnitUpdater::startAttackParticleSystem(Unit *unit, float lastAnimProgress, float animProgress){
	Renderer &renderer= Renderer::getInstance();

	//const AttackSkillType *ast= dynamic_cast<const AttackSkillType*>(unit->getCurrSkill());
	const AttackSkillType *ast= static_cast<const AttackSkillType*>(unit->getCurrSkill());

	if(ast == NULL) {
		throw megaglest_runtime_error("Start attack particle ast == NULL!");
	}

	ParticleSystemTypeSplash *pstSplash= ast->getSplashParticleType();
	bool hasProjectile = !ast->projectileTypes.empty();
	Vec3f startPos= unit->getCurrVectorForParticlesystems();
	Vec3f endPos= unit->getTargetVec();

	//make particle system
	const SurfaceCell *sc= map->getSurfaceCell(Map::toSurfCoords(unit->getPos()));
	const SurfaceCell *tsc= map->getSurfaceCell(Map::toSurfCoords(unit->getTargetPos()));
	bool visible= sc->isVisible(world->getThisTeamIndex()) || tsc->isVisible(world->getThisTeamIndex());
	if(visible == false && world->showWorldForPlayer(world->getThisFactionIndex()) == true) {
		visible = true;
	}

	//for(ProjectileParticleSystemTypes::const_iterator pit= unit->getCurrSkill()->projectileParticleSystemTypes.begin(); pit != unit->getCurrSkill()->projectileParticleSystemTypes.end(); ++pit) {
	for(ProjectileTypes::const_iterator pt= ast->projectileTypes.begin(); pt != ast->projectileTypes.end(); ++pt) {
		bool startAttackParticleSystemNow = ((*pt)->getAttackStartTime() >= lastAnimProgress && (*pt)->getAttackStartTime() < animProgress);
		if(startAttackParticleSystemNow){
			ProjectileParticleSystem *psProj= (*pt)->getProjectileParticleSystemType()->create(unit);
			psProj->setPath(startPos, endPos);
			psProj->setObserver(new ParticleDamager(unit,(*pt), this, gameCamera));
			psProj->setVisible(visible);
			if(unit->getFaction()->getTexture()) {
				psProj->setFactionColor(unit->getFaction()->getTexture()->getPixmapConst()->getPixel3f(0,0));
			}
			renderer.manageParticleSystem(psProj, rsGame);
			unit->addAttackParticleSystem(psProj);

			if(pstSplash!=NULL){
				SplashParticleSystem *psSplash= pstSplash->create(unit);
				psSplash->setPos(endPos);
				psSplash->setVisible(visible);
				if(unit->getFaction()->getTexture()) {
					psSplash->setFactionColor(unit->getFaction()->getTexture()->getPixmapConst()->getPixel3f(0,0));
				}
				renderer.manageParticleSystem(psSplash, rsGame);
				unit->addAttackParticleSystem(psSplash);
				psProj->link(psSplash);
			}
		}
	}

	// if no projectile, still deal damage..
	if(hasProjectile == false) {
		char szBuf[8096]="";
		snprintf(szBuf,8095,"Unit hitting [startAttackParticleSystem] no proj");
		unit->addNetworkCRCDecHp(szBuf);
		hit(unit);
		//splash
		if(pstSplash != NULL) {
			SplashParticleSystem *psSplash= pstSplash->create(unit);
			psSplash->setPos(endPos);
			psSplash->setVisible(visible);
			if(unit->getFaction()->getTexture()) {
				psSplash->setFactionColor(unit->getFaction()->getTexture()->getPixmapConst()->getPixel3f(0,0));
			}
			renderer.manageParticleSystem(psSplash, rsGame);
			unit->addAttackParticleSystem(psSplash);
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

bool UnitUpdater::attackerOnSight(Unit *unit, Unit **rangedPtr, bool evalMode){
	int range = unit->getType()->getTotalSight(unit->getTotalUpgrade());
	return unitOnRange(unit, range, rangedPtr, NULL,evalMode);
}

bool UnitUpdater::attackableOnSight(Unit *unit, Unit **rangedPtr, const AttackSkillType *ast, bool evalMode) {
	int range = unit->getType()->getTotalSight(unit->getTotalUpgrade());
	return unitOnRange(unit, range, rangedPtr, ast, evalMode);
}

bool UnitUpdater::attackableOnRange(Unit *unit, Unit **rangedPtr, const AttackSkillType *ast,bool evalMode) {
	int range= ast->getTotalAttackRange(unit->getTotalUpgrade());
	return unitOnRange(unit, range, rangedPtr, ast, evalMode);
}

bool UnitUpdater::findCachedCellsEnemies(Vec2i center, int range, int size, vector<Unit*> &enemies,
										 const AttackSkillType *ast, const Unit *unit,
										 const Unit *commandTarget) {
	bool result = false;
	MutexSafeWrapper safeMutex(mutexUnitRangeCellsLookupItemCache,string(__FILE__) + "_" + intToStr(__LINE__));
	std::map<Vec2i, std::map<int, std::map<int, UnitRangeCellsLookupItem > > >::iterator iterFind = UnitRangeCellsLookupItemCache.find(center);

	if(iterFind != UnitRangeCellsLookupItemCache.end()) {
		std::map<int, std::map<int, UnitRangeCellsLookupItem > >::iterator iterFind3 = iterFind->second.find(size);
		if(iterFind3 != iterFind->second.end()) {
			std::map<int, UnitRangeCellsLookupItem>::iterator iterFind4 = iterFind3->second.find(range);
			if(iterFind4 != iterFind3->second.end()) {
				result = true;

				std::vector<Cell *> &cellList = iterFind4->second.rangeCellList;
				for(int idx = 0; idx < (int)cellList.size(); ++idx) {
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
bool UnitUpdater::unitOnRange(Unit *unit, int range, Unit **rangedPtr,
							  const AttackSkillType *ast,bool evalMode) {
	bool result=false;

	try {
	vector<Unit*> enemies;
    enemies.reserve(100);

	//we check command target
	const Unit *commandTarget = NULL;
	if(unit->anyCommand() && unit->getCurrCommand() != NULL) {
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
				if(map->isInside(i, j) && streflop::floor(static_cast<streflop::Simple>(floatCenter.dist(Vec2f((float)i, (float)j)))) <= (range+1)){
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
		if(cacheItem.rangeCellList.empty() == false) {
			MutexSafeWrapper safeMutex(mutexUnitRangeCellsLookupItemCache,string(__FILE__) + "_" + intToStr(__LINE__));

			UnitRangeCellsLookupItemCache[center][size][range] = cacheItem;
		}
	}

	//attack enemies that can attack first
	float distToUnit= -1;
	Unit* enemySeen= NULL;

	float distToStandingUnit= -1;
	Unit* attackingEnemySeen= NULL;
	ControlType controlType= unit->getFaction()->getControlType();
	bool isUltra= controlType == ctCpuUltra || controlType == ctNetworkCpuUltra;
	bool isMega= controlType == ctCpuMega || controlType == ctNetworkCpuMega;


	string randomInfoData = "enemies.size() = " + intToStr(enemies.size());

	//printf("unit %d has control:%d\n",unit->getId(),controlType);
    for(int i = 0; i< (int)enemies.size(); ++i) {
    	Unit *enemy = enemies[i];


    	if(enemy != NULL && enemy->isAlive() == true) {

    		// Here we default to first enemy if no attackers found
    		if(enemySeen == NULL) {
                *rangedPtr 	= enemy;
    			enemySeen 	= enemy;
                result		= true;
    		}

    		//randomInfoData += " i = " + intToStr(i) + " alive = true result = " + intToStr(result);

    		// Attackers get first priority
    		if(enemy->getType()->hasSkillClass(scAttack) == true) {

    			float currentDist = unit->getCenteredPos().dist(enemy->getCenteredPos());

    			//randomInfoData += " currentDist = " + floatToStr(currentDist);

    			// Select closest attacking unit
    			if(distToUnit < 0 ||  currentDist< distToUnit) {
    				distToUnit = currentDist;
					*rangedPtr	= enemies[i];
					enemySeen	= enemies[i];
					result		= true;
    			}

    			if(isUltra || isMega) {

        			if(distToStandingUnit < 0 || currentDist< distToStandingUnit) {
        			    if(enemies[i]->getCurrSkill()!=NULL && enemies[i]->getCurrSkill()->getClass()==scAttack) {
        			    	distToStandingUnit = currentDist;
        			    	attackingEnemySeen=enemies[i];
        			    }
        			}
    			}
    		}
    	}
    }

    if(evalMode == false && (isUltra || isMega)) {

    	unit->getRandom()->addLastCaller(randomInfoData);

    	if( attackingEnemySeen!=NULL && unit->getRandom()->randRange(0,2,extractFileFromDirectoryPath(__FILE__) + intToStr(__LINE__)) != 2 ) {
    		*rangedPtr 	= attackingEnemySeen;
    		enemySeen 	= attackingEnemySeen;
    		//printf("Da hat er wen gefunden:%s\n",enemySeen->getType()->getName(false).c_str());
    	}
    }

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

		if(evalMode == false && onlyEnemyUnits == false &&
			enemyUnit->getTeam() != world->getThisTeamIndex()) {

			Vec2f enemyFloatCenter	= enemyUnit->getFloatCenteredPos();
			// find nearest Attack and cleanup old dates
			AttackWarningData *nearest	= NULL;
			float currentDistance		= 0.f;
			float nearestDistance		= 0.f;


			MutexSafeWrapper safeMutex(mutexAttackWarnings,string(__FILE__) + "_" + intToStr(__LINE__));
			for(int i = (int)attackWarnings.size() - 1; i >= 0; --i) {
				if(world->getFrameCount() - attackWarnings[i]->lastFrameCount > 200) { //after 200 frames attack break we warn again
					AttackWarningData *toDelete =attackWarnings[i];
					attackWarnings.erase(attackWarnings.begin()+i);
					delete toDelete; // old one
				}
				else {
#ifdef USE_STREFLOP
					currentDistance = streflop::floor(static_cast<streflop::Simple>(enemyFloatCenter.dist(attackWarnings[i]->attackPosition))); // no need for streflops here!
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

				MutexSafeWrapper safeMutex(mutexAttackWarnings,string(__FILE__) + "_" + intToStr(__LINE__));
	    		attackWarnings.push_back(awd);

	    		if(world->getAttackWarningsEnabled() == true) {

	    			SoundRenderer::getInstance().playFx(CoreData::getInstance().getAttentionSound());
	    			world->addAttackEffects(enemyUnit);
	    		}
	    	}
		}
	}

	}
	catch(const exception &ex) {
		//setRunningStatus(false);

		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		throw megaglest_runtime_error(ex.what());
	}
	catch(...) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s %d] UNKNOWN error\n",__FILE__,__FUNCTION__,__LINE__);
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		throw megaglest_runtime_error(szBuf);
	}

    return result;
}


//if the unit has any enemy on range
vector<Unit*> UnitUpdater::enemyUnitsOnRange(const Unit *unit,const AttackSkillType *ast) {
	vector<Unit*> enemies;
	enemies.reserve(100);

	try {


	int range = unit->getType()->getTotalSight(unit->getTotalUpgrade());
	if(ast != NULL) {

		range = ast->getTotalAttackRange(unit->getTotalUpgrade());
	}
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
	Vec2i center 		= unit->getPosNotThreadSafe();
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
				if(map->isInside(i, j) && streflop::floor(static_cast<streflop::Simple>(floatCenter.dist(Vec2f((float)i, (float)j)))) <= (range+1)){
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
		if(cacheItem.rangeCellList.empty() == false) {
			MutexSafeWrapper safeMutex(mutexUnitRangeCellsLookupItemCache,string(__FILE__) + "_" + intToStr(__LINE__));

			UnitRangeCellsLookupItemCache[center][size][range] = cacheItem;
		}
	}

	}
	catch(const exception &ex) {
		//setRunningStatus(false);

		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		throw megaglest_runtime_error(ex.what());
	}
	catch(...) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s %d] UNKNOWN error\n",__FILE__,__FUNCTION__,__LINE__);
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		throw megaglest_runtime_error(szBuf);
	}

	return enemies;
}


void UnitUpdater::findUnitsForCell(Cell *cell, vector<Unit*> &units) {
	//all fields
	if(cell != NULL) {
		for(int k = 0; k < fieldCount; k++) {
			Field f= static_cast<Field>(k);

			//check field
			Unit *cellUnit = cell->getUnit(f);

			if(cellUnit != NULL && cellUnit->isAlive()) {
				// check if unit already is in list
				bool found = false;
				//printf("---- search for cellUnit=%d\n",cellUnit->getId());
				for (unsigned int i = 0; i < units.size(); ++i) {
					Unit *unitInList = units[i];
					//printf("compare unitInList=%d cellUnit=%d\n",unitInList->getId(),cellUnit->getId());
					if (unitInList->getId() == cellUnit->getId()){
						found=true;
						break;
					}
				}
				if(found==false){
					//printf(">>> adding cellUnit=%d\n",cellUnit->getId());
					units.push_back(cellUnit);
				}
			}
		}
	}
}

vector<Unit*> UnitUpdater::findUnitsInRange(const Unit *unit, int radius) {
	int range = radius;
	vector<Unit*> units;

	//aux vars
	int size 			= unit->getType()->getSize();
	Vec2i center 		= unit->getPosNotThreadSafe();
	Vec2f floatCenter	= unit->getFloatCenteredPos();

	//nearby cells
	//UnitRangeCellsLookupItem cacheItem;
	for(int i = center.x - range; i < center.x + range + size; ++i) {
		for(int j = center.y - range; j < center.y + range + size; ++j) {
			//cells inside map and in range
#ifdef USE_STREFLOP
			if(map->isInside(i, j) && streflop::floor(static_cast<streflop::Simple>(floatCenter.dist(Vec2f((float)i, (float)j)))) <= (range+1)){
#else
			if(map->isInside(i, j) && floor(floatCenter.dist(Vec2f((float)i, (float)j))) <= (range+1)){
#endif
				Cell *cell = map->getCell(i,j);
				findUnitsForCell(cell,units);
			}
		}
	}

	return units;
}

string UnitUpdater::getUnitRangeCellsLookupItemCacheStats() {
	string result = "";

	int posCount = 0;
	int sizeCount = 0;
	int rangeCount = 0;
	int rangeCountCellCount = 0;

	MutexSafeWrapper safeMutex(mutexUnitRangeCellsLookupItemCache,string(__FILE__) + "_" + intToStr(__LINE__));
	for(std::map<Vec2i, std::map<int, std::map<int, UnitRangeCellsLookupItem > > >::iterator iterMap1 = UnitRangeCellsLookupItemCache.begin();
		iterMap1 != UnitRangeCellsLookupItemCache.end(); ++iterMap1) {
		posCount++;

		for(std::map<int, std::map<int, UnitRangeCellsLookupItem > >::iterator iterMap2 = iterMap1->second.begin();
			iterMap2 != iterMap1->second.end(); ++iterMap2) {
			sizeCount++;

			for(std::map<int, UnitRangeCellsLookupItem>::iterator iterMap3 = iterMap2->second.begin();
				iterMap3 != iterMap2->second.end(); ++iterMap3) {
				rangeCount++;

				rangeCountCellCount += (int)iterMap3->second.rangeCellList.size();
			}
		}
	}

	uint64 totalBytes = rangeCountCellCount * sizeof(Cell *);
	totalBytes /= 1000;

	char szBuf[8096]="";
	snprintf(szBuf,8096,"pos [%d] size [%d] range [%d][%d] total KB: %s",posCount,sizeCount,rangeCount,rangeCountCellCount,formatNumber(totalBytes).c_str());
	result = szBuf;
	return result;
}

void UnitUpdater::saveGame(XmlNode *rootNode) {
	std::map<string,string> mapTagReplacements;
	XmlNode *unitupdaterNode = rootNode->addChild("UnitUpdater");

//	const GameCamera *gameCamera;
//	Gui *gui;
//	Map *map;
//	World *world;
//	Console *console;
//	ScriptManager *scriptManager;
//	PathFinder *pathFinder;
	pathFinder->saveGame(unitupdaterNode);
//	Game *game;
//	RandomGen random;
	//unitupdaterNode->addAttribute("random",intToStr(random.getLastNumber()), mapTagReplacements);
//	float attackWarnRange;
	unitupdaterNode->addAttribute("attackWarnRange",floatToStr(attackWarnRange,6), mapTagReplacements);
//	AttackWarnings attackWarnings;
//
}

void UnitUpdater::clearCaches() {
	 if(pathFinder != NULL) {
		 pathFinder->clearCaches();
	 }
}

void UnitUpdater::loadGame(const XmlNode *rootNode) {
	const XmlNode *unitupdaterNode = rootNode->getChild("UnitUpdater");

	pathFinder->loadGame(unitupdaterNode);
	//random.setLastNumber(unitupdaterNode->getAttribute("random")->getIntValue());
//	float attackWarnRange;
	attackWarnRange = unitupdaterNode->getAttribute("attackWarnRange")->getFloatValue();
}
// =====================================================
//	class ParticleDamager
// =====================================================

ParticleDamager::ParticleDamager(Unit *attacker,const ProjectileType* projectileType, UnitUpdater *unitUpdater, const GameCamera *gameCamera){
	this->gameCamera= gameCamera;
	this->attackerRef= attacker;
	this->projectileType= projectileType;
	this->ast= static_cast<const AttackSkillType*>(attacker->getCurrSkill());
	this->targetPos= attacker->getTargetPos();
	this->targetField= attacker->getTargetField();
	this->unitUpdater= unitUpdater;
}

void ParticleDamager::update(ParticleSystem *particleSystem) {
	Unit *attacker= attackerRef.getUnit();

	if(attacker != NULL) {
		//string auditBeforeHit = particleSystem->toString();

		char szBuf[8096]="";
		snprintf(szBuf,8095,"Unit hitting [ParticleDamager::update] [%s] targetField = %d",targetPos.getString().c_str(),targetField);
		attacker->addNetworkCRCDecHp(szBuf);

		unitUpdater->hit(attacker, ast, targetPos, targetField, projectileType->getDamagePercentage());

		//char szBuf[8096]="";
		//snprintf(szBuf,8095,"ParticleDamager::update attacker particleSystem before: %s\nafter: %s",auditBeforeHit.c_str(),particleSystem->toString().c_str());
		//attacker->setNetworkCRCParticleObserverLogInfo(szBuf);

		//play sound
		// Try to use the sound form the projetileType
		StaticSound *projSound=projectileType->getHitSound();
		if(projSound == NULL){
			// use the sound from the skill
			projSound= ast->getProjSound();
		}
		if(particleSystem->getVisible() && projSound != NULL) {
			SoundRenderer::getInstance().playFx(projSound, attacker->getCurrMidHeightVector(), gameCamera->getPos());
		}

		//check for spawnattack
		if(projectileType->getSpawnUnit()!="" && projectileType->getSpawnUnitcount()>0 ){
			unitUpdater->spawnAttack(attacker,projectileType->getSpawnUnit(),projectileType->getSpawnUnitcount(),projectileType->getSpawnUnitAtTarget(),targetPos);
		}

		// check for shake and shake
		if(projectileType->isShake()==true){
			World *world=attacker->getFaction()->getWorld();
			Map* map=world->getMap();
			Game *game=world->getGame();

			//Unit *attacked= map->getCell(targetPos)->getUnit(targetField);
			Vec2i surfaceTargetPos=Map::toSurfCoords(targetPos);
			bool visibility=(!projectileType->isShakeVisible())||(map->getSurfaceCell(surfaceTargetPos)->isVisible(world->getThisTeamIndex()) ||
								(game->getWorld()->showWorldForPlayer(game->getWorld()->getThisTeamIndex()) == true));

			bool isInCameraView=(!projectileType->isShakeInCameraView()) || Renderer::getInstance().posInCellQuadCache(surfaceTargetPos).first;

			if(visibility && isInCameraView) {
				game->getGameCameraPtr()->shake( projectileType->getShakeDuration(), projectileType->getShakeIntensity(),projectileType->isShakeCameraDistanceAffected(),map->getSurfaceCell(surfaceTargetPos)->getVertex());
			}
		}
	}
	particleSystem->setObserver(NULL);
	delete this;
}

void ParticleDamager::saveGame(XmlNode *rootNode) {
	std::map<string,string> mapTagReplacements;
	XmlNode *particleDamagerNode = rootNode->addChild("ParticleDamager");

//	UnitReference attackerRef;
	attackerRef.saveGame(particleDamagerNode);

//	const AttackSkillType* ast;
	particleDamagerNode->addAttribute("astName",ast->getName(), mapTagReplacements);
	particleDamagerNode->addAttribute("astClass",intToStr(ast->getClass()), mapTagReplacements);
//	UnitUpdater *unitUpdater;
//	const GameCamera *gameCamera;
//	Vec2i targetPos;
	particleDamagerNode->addAttribute("targetPos",targetPos.getString(), mapTagReplacements);
//	Field targetField;
	particleDamagerNode->addAttribute("targetField",intToStr(targetField), mapTagReplacements);
}

void ParticleDamager::loadGame(const XmlNode *rootNode, void *genericData) {
	const XmlNode *particleDamagerNode = rootNode->getChild("ParticleDamager");

	std::pair<Game *,Unit *> *pairData = (std::pair<Game *,Unit *>*)genericData;
	//UnitType *ut, Game *game
	attackerRef.loadGame(particleDamagerNode,pairData->first->getWorld());

	//random.setLastNumber(particleSystemNode->getAttribute("random")->getIntValue());

	//	const AttackSkillType* ast;
	string astName = particleDamagerNode->getAttribute("astName")->getValue();
	SkillClass astClass = static_cast<SkillClass>(particleDamagerNode->getAttribute("astClass")->getIntValue());
	ast = dynamic_cast<const AttackSkillType*>(pairData->second->getType()->getSkillType(astName,astClass));
	//	UnitUpdater *unitUpdater;
	unitUpdater = pairData->first->getWorld()->getUnitUpdater();
	//	const GameCamera *gameCamera;
	gameCamera = pairData->first->getGameCamera();
	//	Vec2i targetPos;
	targetPos = Vec2i::strToVec2(particleDamagerNode->getAttribute("targetPos")->getValue());
	//	Field targetField;
	targetField = static_cast<Field>(particleDamagerNode->getAttribute("targetField")->getIntValue());
}

}}//end namespace
