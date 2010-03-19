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

#include "sound.h"
#include "upgrade.h"
#include "unit.h"
#include "particle_type.h"
#include "core_data.h"
#include "config.h"
#include "renderer.h"
#include "sound_renderer.h"
#include "game.h"
#include "path_finder.h"
#include "object.h"
#include "faction.h"
#include "network_manager.h"
#include "leak_dumper.h"

using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class UnitUpdater
// =====================================================

// ===================== PUBLIC ========================

void UnitUpdater::init(Game *game){

    this->game= game;
	this->gui= game->getGui();
	this->gameCamera= game->getGameCamera();
	this->world= game->getWorld();
	this->map= world->getMap();
	this->console= game->getConsole();
	this->scriptManager= game->getScriptManager();
	pathFinder.init(map);

	allowRotateUnits = Config::getInstance().getBool(reinterpret_cast<const char *>("AllowRotateUnits"),"0");
}


// ==================== progress skills ====================

//skill dependent actions
void UnitUpdater::updateUnit(Unit *unit){

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

	//start attack particle system
	if(unit->getCurrSkill()->getClass()==scAttack){
		const AttackSkillType *ast= static_cast<const AttackSkillType*>(unit->getCurrSkill());
		float attackStartTime= ast->getAttackStartTime();
		if(attackStartTime>=unit->getLastAnimProgress() && attackStartTime<unit->getAnimProgress()){
			startAttackParticleSystem(unit);
		}
	}

	//update unit
	if(unit->update()){
        //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		updateUnitCommand(unit);

		//if unit is out of EP, it stops
		if(unit->computeEp()){
			unit->setCurrSkill(scStop);
			unit->cancelCommand();
		}

		//move unit in cells
		if(unit->getCurrSkill()->getClass()==scMove){
			world->moveUnitCells(unit);

			//play water sound
			if(map->getCell(unit->getPos())->getHeight()<map->getWaterLevel() && unit->getCurrField()==fLand){
				soundRenderer.playFx(CoreData::getInstance().getWaterSound());
			}
		}
	}

	//unit death
	if(unit->isDead() && unit->getCurrSkill()->getClass()!=scDie){
		unit->kill();
	}
}


// ==================== progress commands ====================

//VERY IMPORTANT: compute next state depending on the first order of the list
void UnitUpdater::updateUnitCommand(Unit *unit){

	//if unis has command process it
    if(unit->anyCommand()) {
		unit->getCurrCommand()->getCommandType()->update(this, unit);
	}

	//if no commands stop and add stop command
	if(!unit->anyCommand() && unit->isOperative()){
	    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		unit->setCurrSkill(scStop);
		if(unit->getType()->hasCommandClass(ccStop)){
		    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			unit->giveCommand(new Command(unit->getType()->getFirstCtOfClass(ccStop)));
		}
	}
}

// ==================== updateStop ====================

void UnitUpdater::updateStop(Unit *unit){

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
}


// ==================== updateMove ====================
void UnitUpdater::updateMove(Unit *unit){

    Command *command= unit->getCurrCommand();
    const MoveCommandType *mct= static_cast<const MoveCommandType*>(command->getCommandType());

	Vec2i pos= command->getUnit()!=NULL? command->getUnit()->getCenteredPos(): command->getPos();

	switch(pathFinder.findPath(unit, pos)){
	case PathFinder::tsOnTheWay:
		unit->setCurrSkill(mct->getMoveSkillType());
        break;

	case PathFinder::tsBlocked:
		if(unit->getPath()->isBlocked()){
			unit->finishCommand();
		}
		break;

    default:
        unit->finishCommand();
    }
}


// ==================== updateAttack ====================

void UnitUpdater::updateAttack(Unit *unit){

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

		//if unit arrives destPos order has ended
        switch (pathFinder.findPath(unit, pos)){
        case PathFinder::tsOnTheWay:
            unit->setCurrSkill(act->getMoveSkillType());
            break;
		case PathFinder::tsBlocked:
			if(unit->getPath()->isBlocked()){
				unit->finishCommand();
			}
			break;
		default:
			unit->finishCommand();
		}
    }
}


// ==================== updateAttackStopped ====================

void UnitUpdater::updateAttackStopped(Unit *unit){

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
}


// ==================== updateBuild ====================

void UnitUpdater::updateBuild(Unit *unit){

	Command *command= unit->getCurrCommand();
    const BuildCommandType *bct= static_cast<const BuildCommandType*>(command->getCommandType());

	if(unit->getCurrSkill()->getClass() != scBuild) {
        //if not building
        const UnitType *ut= command->getUnitType();

		switch (pathFinder.findPath(unit, command->getPos()-Vec2i(1))){
        case PathFinder::tsOnTheWay:
            unit->setCurrSkill(bct->getMoveSkillType());
            break;

        case PathFinder::tsArrived:
            //if arrived destination
            assert(command->getUnitType()!=NULL);
            if(map->isFreeCells(command->getPos(), ut->getSize(), fLand)){
				const UnitType *builtUnitType= command->getUnitType();

                //!!!
                float unitRotation = -1;
                if(allowRotateUnits == true) {
                    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

                    char unitKey[50]="";
                    sprintf(unitKey,"%d_%d",builtUnitType->getId(),unit->getFaction()->getIndex());
                    unitRotation = gui->getUnitTypeBuildRotation(unitKey);

                    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] builtUnitType->getId() = %d unitRotation = %f\n",__FILE__,__FUNCTION__,__LINE__,builtUnitType->getId(),unitRotation);
                }
				Unit *builtUnit= new Unit(world->getNextUnitId(), command->getPos(), builtUnitType, unit->getFaction(), world->getMap(),unitRotation);
				builtUnit->create();

				if(!builtUnitType->hasSkillClass(scBeBuilt)){
					throw runtime_error("Unit " + builtUnitType->getName() + "has no be_built skill");
				}

				builtUnit->setCurrSkill(scBeBuilt);
				unit->setCurrSkill(bct->getBuildSkillType());
				unit->setTarget(builtUnit);
				map->prepareTerrain(builtUnit);
				command->setUnit(builtUnit);

				//play start sound
				if(unit->getFactionIndex()==world->getThisFactionIndex()){
					SoundRenderer::getInstance().playFx(
						bct->getStartSound(),
						unit->getCurrVector(),
						gameCamera->getPos());
				}
			}
            else{
                //if there are no free cells
				unit->cancelCommand();
                unit->setCurrSkill(scStop);
				if(unit->getFactionIndex()==world->getThisFactionIndex()){
                     console->addStdMessage("BuildingNoPlace");
				}
            }
            break;

        case PathFinder::tsBlocked:
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
}


// ==================== updateHarvest ====================

void UnitUpdater::updateHarvest(Unit *unit){

	Command *command= unit->getCurrCommand();
    const HarvestCommandType *hct= static_cast<const HarvestCommandType*>(command->getCommandType());
	Vec2i targetPos;

	if(unit->getCurrSkill()->getClass() != scHarvest) {
		//if not working
		if(unit->getLoadCount()==0){
			//if not loaded go for resources
			Resource *r= map->getSurfaceCell(Map::toSurfCoords(command->getPos()))->getResource();
			if(r!=NULL && hct->canHarvest(r->getType())){
				//if can harvest dest. pos
				if(unit->getPos().dist(command->getPos())<harvestDistance &&
					map->isResourceNear(unit->getPos(), r->getType(), targetPos)) {
						//if it finds resources it starts harvesting
						unit->setCurrSkill(hct->getHarvestSkillType());
						unit->setTargetPos(targetPos);
						unit->setLoadCount(0);
						unit->setLoadType(map->getSurfaceCell(Map::toSurfCoords(unit->getTargetPos()))->getResource()->getType());
				}
				else{
					//if not continue walking
					switch(pathFinder.findPath(unit, command->getPos())){
					case PathFinder::tsOnTheWay:
						unit->setCurrSkill(hct->getMoveSkillType());
						break;
					default:
						break;
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
			if(store!=NULL){
				switch(pathFinder.findPath(unit, store->getCenteredPos())){
				case PathFinder::tsOnTheWay:
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
		if(r!=NULL){
			//if there is a resource, continue working, until loaded
			unit->update2();
			if(unit->getProgress2()>=hct->getHitsPerUnit()){
				unit->setProgress2(0);
				unit->setLoadCount(unit->getLoadCount()+1);

				//if resource exausted, then delete it and stop
				if(r->decAmount(1)){
					sc->deleteResource();
					unit->setCurrSkill(hct->getStopLoadedSkillType());
				}

				if(unit->getLoadCount()==hct->getMaxLoad()){
					unit->setCurrSkill(hct->getStopLoadedSkillType());
					unit->getPath()->clear();
				}

			}
		}
		else{
			//if there is no resource, just stop
			unit->setCurrSkill(hct->getStopLoadedSkillType());
		}
	}
}


// ==================== updateRepair ====================

void UnitUpdater::updateRepair(Unit *unit){

    Command *command= unit->getCurrCommand();
    const RepairCommandType *rct= static_cast<const RepairCommandType*>(command->getCommandType());

	Unit *repaired= map->getCell(command->getPos())->getUnit(fLand);
	bool nextToRepaired= repaired!=NULL && map->isNextTo(unit->getPos(), repaired);

	if(unit->getCurrSkill()->getClass()!=scRepair || !nextToRepaired){
        //if not repairing
        if(repaired!=NULL && rct->isRepairableUnitType(repaired->getType()) && repaired->isDamaged()){

			if(nextToRepaired){
				unit->setTarget(repaired);
                unit->setCurrSkill(rct->getRepairSkillType());
			}
			else{
				switch(pathFinder.findPath(unit, command->getPos())){
				case PathFinder::tsOnTheWay:
					unit->setCurrSkill(rct->getMoveSkillType());
					break;
				case PathFinder::tsBlocked:
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
}


// ==================== updateProduce ====================

void UnitUpdater::updateProduce(Unit *unit){

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

            //!!!
            float unitRotation = -1;
            if(allowRotateUnits == true) {
                char unitKey[50]="";
                sprintf(unitKey,"%d_%d",pct->getId(),unit->getFaction()->getIndex());
                unitRotation = gui->getUnitTypeBuildRotation(unitKey);
            }

			produced= new Unit(world->getNextUnitId(), Vec2i(0), pct->getProducedUnit(), unit->getFaction(), world->getMap(),unitRotation);

			//place unit creates the unit
			if(!world->placeUnit(unit->getCenteredPos(), 10, produced)){
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
}


// ==================== updateUpgrade ====================

void UnitUpdater::updateUpgrade(Unit *unit){

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
}

// ==================== updateMorph ====================

void UnitUpdater::updateMorph(Unit *unit){

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

			//finish the command
			if(unit->morph(mct)){
				unit->finishCommand();
				if(gui->isSelected(unit)){
					gui->onSelectionChanged();
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
		scriptManager->onUnitDied(attacked);
	}
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

    Vec2i pos= unit->getCurrCommand()->getPos();

    for(int radius= 0; radius<maxResSearchRadius; radius++){
        for(int i=pos.x-radius; i<=pos.x+radius; ++i){
            for(int j=pos.y-radius; j<=pos.y+radius; ++j){
				if(map->isInside(i, j)){
					Resource *r= map->getSurfaceCell(Map::toSurfCoords(Vec2i(i, j)))->getResource();
                    if(r!=NULL){
						if(hct->canHarvest(r->getType())){
							unit->getCurrCommand()->setPos(Vec2i(i, j));
							return true;
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

//if the unit has any enemy on range
bool UnitUpdater::unitOnRange(const Unit *unit, int range, Unit **rangedPtr, const AttackSkillType *ast){

    vector<Unit*> enemies;

	//we check command target
	const Unit *commandTarget= NULL;
	if(unit->anyCommand()){
		commandTarget= static_cast<const Unit*>(unit->getCurrCommand()->getUnit());
	}
	if(commandTarget!=NULL && commandTarget->isDead()){
		commandTarget= NULL;
	}

	//aux vars
	int size= unit->getType()->getSize();
	Vec2i center= unit->getPos();
	Vec2f floatCenter= unit->getFloatCenteredPos();

	//nearby cells
	for(int i=center.x-range; i<center.x+range+size; ++i){
		for(int j=center.y-range; j<center.y+range+size; ++j){

			//cells insede map and in range
			if(map->isInside(i, j) && floor(floatCenter.dist(Vec2f(i, j))) <= (range+1)){

				//all fields
				for(int k=0; k<fieldCount; k++){
					Field f= static_cast<Field>(k);

					//check field
					if((ast==NULL || ast->getAttackField(f))){
						Unit *possibleEnemy= map->getCell(i, j)->getUnit(f);

						//check enemy
						if(possibleEnemy!=NULL && possibleEnemy->isAlive()){
							if((!unit->isAlly(possibleEnemy) && commandTarget==NULL) || commandTarget==possibleEnemy){
								enemies.push_back(possibleEnemy);
							}
						}
					}
				}
			}
		}
	}

	//attack enemies that can attack first
    for(int i=0; i<enemies.size(); ++i){
		if(enemies[i]->getType()->hasSkillClass(scAttack)){
            *rangedPtr= enemies[i];
            return true;
        }
    }

	//any enemy
    if(enemies.size()>0){
        *rangedPtr= enemies.front();
        return true;
    }

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
