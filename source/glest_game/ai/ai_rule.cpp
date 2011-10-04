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
#include "ai_rule.h"

#include <algorithm>
#include <limits.h>

#include "ai.h"
#include "ai_interface.h"
#include "unit.h"
#include "leak_dumper.h"

using Shared::Graphics::Vec2i;

namespace Glest{ namespace Game{

// =====================================================
//	class AiRule
// =====================================================

AiRule::AiRule(Ai *ai){
	this->ai= ai;
}

// =====================================================
//	class AiRuleWorkerHarvest
// =====================================================

AiRuleWorkerHarvest::AiRuleWorkerHarvest(Ai *ai):
	AiRule(ai)
{
	stoppedWorkerIndex= -1;
}

bool AiRuleWorkerHarvest::test(){
	return ai->findAbleUnit(&stoppedWorkerIndex, ccHarvest, true);
}

void AiRuleWorkerHarvest::execute(){
	ai->harvest(stoppedWorkerIndex);
}

// =====================================================
//	class AiRuleRefreshHarvester
// =====================================================

AiRuleRefreshHarvester::AiRuleRefreshHarvester(Ai *ai):
	AiRule(ai)
{
	workerIndex= -1;
}

bool AiRuleRefreshHarvester::test(){
	return ai->findAbleUnit(&workerIndex, ccHarvest, ccHarvest);
}

void AiRuleRefreshHarvester::execute(){
	ai->harvest(workerIndex);
}

// =====================================================
//	class AiRuleScoutPatrol
// =====================================================

AiRuleScoutPatrol::AiRuleScoutPatrol(Ai *ai):
	AiRule(ai)
{
}

bool AiRuleScoutPatrol::test(){
	return ai->isStableBase();
}

void AiRuleScoutPatrol::execute(){
	ai->sendScoutPatrol();
}
// =====================================================
//	class AiRuleRepair
// =====================================================

AiRuleRepair::AiRuleRepair(Ai *ai):
	AiRule(ai)
{
	damagedUnitIndex = 0;
	damagedUnitIsCastle = false;
}

double AiRuleRepair::getMinCastleHpRatio() const {
	return 0.6;
}

int AiRuleRepair::getMinUnitsToRepairCastle() {
	int minUnitsRepairingCastle 	= 7;
	if(ai->getCountOfClass(ucWorker) <= 6){
		minUnitsRepairingCastle= 1;
	}
	else if(ai->getCountOfClass(ucWorker) <= 8){
		minUnitsRepairingCastle= 2;
	}
	else if(ai->getCountOfClass(ucWorker) <= 10){
		minUnitsRepairingCastle= 3;
	}
	else if(ai->getCountOfClass(ucWorker) <= 12){
		minUnitsRepairingCastle= 5;
	}
	return minUnitsRepairingCastle;
}

bool AiRuleRepair::test(){
	AiInterface *aiInterface= ai->getAiInterface();

	int minUnitsRepairingCastle 	= getMinUnitsToRepairCastle();
	const double minCastleHpRatio 	= getMinCastleHpRatio();

	// look for a damaged unit and give priority to the factions bases
	// (units that produce workers and store resources)
	for(int i = 0; i < aiInterface->getMyUnitCount(); ++i) {
		const Unit *u= aiInterface->getMyUnit(i);
		//printf("\n\n\n\n!!!!!! Is damaged unit [%d - %s] u->getHpRatio() = %f, hp = %d, mapHp = %d\n",u->getId(),u->getType()->getName().c_str(),u->getHpRatio(),u->getHp(),u->getType()->getTotalMaxHp(u->getTotalUpgrade()));
		if(u->getHpRatio() < 1.f) {
			//printf("\n\n\n\n!!!!!! Is damaged unit [%d - %s] u->getHpRatio() = %f, hp = %d, mapHp = %d\n",u->getId(),u->getType()->getName().c_str(),u->getHpRatio(),u->getHp(),u->getType()->getTotalMaxHp(u->getTotalUpgrade()));

			bool unitCanProduceWorker = false;
			for(int j = 0; unitCanProduceWorker == false &&
			               j < u->getType()->getCommandTypeCount(); ++j) {
				const CommandType *ct= u->getType()->getCommandType(j);

				//if the command is produce
				if(ct->getClass() == ccProduce || ct->getClass() == ccMorph) {
					const ProducibleType *pt = ct->getProduced();
					if(pt != NULL) {
						const UnitType *ut = dynamic_cast<const UnitType *>(pt);
						if( ut != NULL && ut->hasCommandClass(ccHarvest) == true &&
							u->getType()->getStoredResourceCount() > 0) {
							//printf("\n\n\n\n!!!!!! found candidate castle unit to repair [%d - %s]\n",u->getId(),u->getType()->getName().c_str());
							unitCanProduceWorker = true;
						}
					}
				}
			}

			if(unitCanProduceWorker == true) {
				int candidatedamagedUnitIndex=-1;
				int unitCountAlreadyRepairingDamagedUnit = 0;
				// Now check if any other unit is able to repair this unit
				for(int i1 = 0; i1 < aiInterface->getMyUnitCount(); ++i1) {
					const Unit *u1= aiInterface->getMyUnit(i1);
					const RepairCommandType *rct= static_cast<const RepairCommandType *>(u1->getType()->getFirstCtOfClass(ccRepair));
					//if(rct) printf("\n\n\n\n^^^^^^^^^^ possible repairer unit [%d - %s] current skill [%d] can reapir damaged unit [%d]\n",u1->getId(),u1->getType()->getName().c_str(),u->getCurrSkill()->getClass(),rct->isRepairableUnitType(u->getType()));

					if(rct != NULL) {
						//printf("\n\n\n\n^^^^^^^^^^ possible repairer unit [%d - %s] current skill [%d] can repair damaged unit [%d] Castles hp-ratio = %f\n",u1->getId(),u1->getType()->getName().c_str(),u1->getCurrSkill()->getClass(),rct->isRepairableUnitType(u->getType()),u->getHpRatio());

						if(u1->getCurrSkill()->getClass() == scStop || u1->getCurrSkill()->getClass() == scMove ||
							u->getHpRatio() <= minCastleHpRatio) {
							if(rct->isRepairableUnitType(u->getType())) {
								candidatedamagedUnitIndex= i;
								//return true;
							}
						}
						else if(u1->getCurrSkill()->getClass() == scRepair) {
							Command *cmd = u1->getCurrCommand();
							if(cmd != NULL && cmd->getCommandType()->getClass() == ccRepair) {
								if(cmd->getUnit() != NULL && cmd->getUnit()->getId() == u->getId()) {
									//printf("\n\n\n\n^^^^^^^^^^ unit is ALREADY repairer unit [%d - %s]\n",u1->getId(),u1->getType()->getName().c_str());
									unitCountAlreadyRepairingDamagedUnit++;
								}
							}
						}
					}
				}

				if(candidatedamagedUnitIndex >= 0 && unitCountAlreadyRepairingDamagedUnit < minUnitsRepairingCastle) {
					//printf("\n\n\n\n^^^^^^^^^^ AI test will repair damaged unit [%d - %s]\n",u->getId(),u->getType()->getName().c_str());
					damagedUnitIndex = candidatedamagedUnitIndex;
					damagedUnitIsCastle=true;
					return true;
				}
			}
		}
	}
	damagedUnitIsCastle=false;
	damagedUnitIndex=-1;
	// Normal Repair checking
	for(int i = 0; i < aiInterface->getMyUnitCount(); ++i) {
		const Unit *u= aiInterface->getMyUnit(i);
		//printf("\n\n\n\n!!!!!! Is damaged unit [%d - %s] u->getHpRatio() = %f, hp = %d, mapHp = %d\n",u->getId(),u->getType()->getName().c_str(),u->getHpRatio(),u->getHp(),u->getType()->getTotalMaxHp(u->getTotalUpgrade()));
		if(u->getHpRatio() < 1.f) {
			// Now check if any other unit is able to repair this unit
			for(int i1 = 0; i1 < aiInterface->getMyUnitCount(); ++i1) {
				const Unit *u1= aiInterface->getMyUnit(i1);
				const RepairCommandType *rct= static_cast<const RepairCommandType *>(u1->getType()->getFirstCtOfClass(ccRepair));
				//if(rct) printf("\n\n\n\n^^^^^^^^^^ possible repairer unit [%d - %s] current skill [%d] can reapir damaged unit [%d]\n",u1->getId(),u1->getType()->getName().c_str(),u->getCurrSkill()->getClass(),rct->isRepairableUnitType(u->getType()));

				if(rct != NULL && (u1->getCurrSkill()->getClass() == scStop || u1->getCurrSkill()->getClass() == scMove)) {
					if(rct->isRepairableUnitType(u->getType())) {
						damagedUnitIndex= i;
						//random if return or not so we get different targets from time to time
						if(ai->getRandom()->randRange(0, 1) == 0)
						return true;
					}
				}
			}
		}
	}
	if( damagedUnitIndex!=-1)
	{
		return true;
	}
	return false;
}

void AiRuleRepair::execute() {
	AiInterface *aiInterface= ai->getAiInterface();
	const Unit *damagedUnit= aiInterface->getMyUnit(damagedUnitIndex);
	//printf("\n\n\n\n###^^^^^^^^^^ Looking for repairer for damaged unit [%d - %s]\n",damagedUnit->getId(),damagedUnit->getType()->getName().c_str());

	int minUnitsRepairingCastle 	= getMinUnitsToRepairCastle();
	const double minCastleHpRatio 	= getMinCastleHpRatio();

	if(minUnitsRepairingCastle > 2){
		if(damagedUnit->getCurrSkill()->getClass() == scBeBuilt){// if build is still be build 2 helpers are enough
			minUnitsRepairingCastle= 2;
		}

		if(!damagedUnitIsCastle){
			minUnitsRepairingCastle= 2;
		}
	}
    if(	aiInterface->getControlType() == ctCpuEasy ||
    	aiInterface->getControlType() == ctNetworkCpuEasy) {
    	if(!damagedUnitIsCastle){
    		// cpu easy does not repair!
    		minUnitsRepairingCastle=0;
    	}
    }
    if(	aiInterface->getControlType() == ctCpu ||
    	aiInterface->getControlType() == ctNetworkCpu) {
    	if(!damagedUnitIsCastle){
    		// cpu does only repair with one unit!
    		minUnitsRepairingCastle=1;
    	}
    }
	int unitCountAlreadyRepairingDamagedUnit = 0;
	//printf("team %d has damaged unit\n", damagedUnit->getTeam());
	// Now check if any other unit is able to repair this unit
	for(int i1 = 0; i1 < aiInterface->getMyUnitCount(); ++i1) {
		const Unit *u1= aiInterface->getMyUnit(i1);
		const RepairCommandType *rct= static_cast<const RepairCommandType *>(u1->getType()->getFirstCtOfClass(ccRepair));
		//if(rct) printf("\n\n\n\n^^^^^^^^^^ possible repairer unit [%d - %s] current skill [%d] can reapir damaged unit [%d]\n",u1->getId(),u1->getType()->getName().c_str(),u1->getCurrSkill()->getClass(),rct->isRepairableUnitType(u1->getType()));
		Command *cmd= u1->getCurrCommand();
		if(cmd != NULL && cmd->getCommandType()->getClass() == ccRepair){
			//if(cmd->getUnit() != NULL && cmd->getUnit()->getId() == damagedUnit->getId())
			//if(cmd->getUnit() != NULL && cmd->getPos() == damagedUnit->getPosWithCellMapSet())

			if(rct != NULL){
				//printf("\n\n\n\n^^^^^^^^^^ possible repairer unit [%d - %s] current skill [%d] can repair damaged unit [%d] Castles hp-ratio = %f\n",u1->getId(),u1->getType()->getName().c_str(),u1->getCurrSkill()->getClass(),rct->isRepairableUnitType(u->getType()),u->getHpRatio());

				if(((RepairCommandType*) (cmd->getCommandType()))->isRepairableUnitType(damagedUnit->getType())){
					//printf("^^^^test^^^^^^ unit is ALREADY repairer unit [%d - %s]\nminUnitsRepairingCastle=%d\n",u1->getId(), u1->getType()->getName().c_str(), minUnitsRepairingCastle);
					unitCountAlreadyRepairingDamagedUnit++;
				}
			}
		}
	}

	if(unitCountAlreadyRepairingDamagedUnit >= minUnitsRepairingCastle){
		return;
	}

	int unitGroupCommandId = -1;

	//find a repairer and issue command
	for(int i = 0; i < aiInterface->getMyUnitCount(); ++i) {
		const Unit *u= aiInterface->getMyUnit(i);
		const RepairCommandType *rct= static_cast<const RepairCommandType *>(u->getType()->getFirstCtOfClass(ccRepair));
		//if(rct) printf("\n\n\n\n^^^^^^^^^^ possible repairer unit [%d - %s] current skill [%d] can reapir damaged unit [%d]\n",u->getId(),u->getType()->getName().c_str(),u->getCurrSkill()->getClass(),rct->isRepairableUnitType(damagedUnit->getType()));

		if(rct != NULL) {
			//printf("\n\n\n\n^^^^^^^^^^ possible excute repairer unit [%d - %s] current skill [%d] can repair damaged unit [%d] Castles hp-ratio = %f\n",u->getId(),u->getType()->getName().c_str(),u->getCurrSkill()->getClass(),rct->isRepairableUnitType(damagedUnit->getType()),damagedUnit->getHpRatio());

			if((u->getCurrSkill()->getClass() == scStop || u->getCurrSkill()->getClass() == scMove || damagedUnit->getHpRatio() <=  minCastleHpRatio)) {
				if((u->getCurrCommand() == NULL || (u->getCurrCommand()->getCommandType()->getClass() != ccBuild
				        && u->getCurrCommand()->getCommandType()->getClass() != ccProduce))
				        && rct->isRepairableUnitType(damagedUnit->getType())){
					//if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
					//printf("\n\n\n\n^^^^^^^^^^ AI execute will repair damaged unit [%d - %s] at pos [%s] cellmapPos [%s] using unit [%d -%s]\n",damagedUnit->getId(),damagedUnit->getType()->getName().c_str(),damagedUnit->getPos().getString().c_str(),damagedUnit->getPosWithCellMapSet().getString().c_str(),u->getId(),u->getType()->getName().c_str());

					/*
					Map *map= aiInterface->getWorld()->getMap();
					Cell *cell = map->getCell(damagedUnit->getPosWithCellMapSet());
					if(cell != NULL) {
						printf("\n\n\n\n^^^^^^^^^^ cell is ok\n");

						Unit *cellUnit = cell->getUnit(damagedUnit->getCurrField());
						if(cellUnit != NULL) {
							printf("\n\n\n\n^^^^^^^^^^ cell unit [%d - %s] at pos [%s]\n",cellUnit->getId(),cellUnit->getType()->getName().c_str(),cellUnit->getPos().getString().c_str());
						}
					}
					*/

					//aiInterface->giveCommand(i, rct, damagedUnit->getPos());
					if(unitCountAlreadyRepairingDamagedUnit < minUnitsRepairingCastle) {

						if(unitGroupCommandId == -1) {
							unitGroupCommandId = aiInterface->getWorld()->getNextCommandGroupId();
						}

						aiInterface->giveCommand(i, rct, damagedUnit->getPosWithCellMapSet(),unitGroupCommandId);
						aiInterface->printLog(3, "Repairing order issued");
						unitCountAlreadyRepairingDamagedUnit++;
						//						printf(
						//						        "^^^^^^^^^^adding one unit to repair ... unitCountAlreadyRepairingDamagedUnit/minUnitsRepairingCastle=%d/%d\n",
						//						        unitCountAlreadyRepairingDamagedUnit, minUnitsRepairingCastle);
					}

					if( !damagedUnitIsCastle || unitCountAlreadyRepairingDamagedUnit >= minUnitsRepairingCastle){
						return;
					}
				}
			}
		}
	}
}

// =====================================================
//	class AiRuleReturnBase
// =====================================================

AiRuleReturnBase::AiRuleReturnBase(Ai *ai):
	AiRule(ai)
{
	stoppedUnitIndex= -1;
}

bool AiRuleReturnBase::test(){
	return ai->findAbleUnit(&stoppedUnitIndex, ccMove, true);
}

void AiRuleReturnBase::execute(){
	ai->returnBase(stoppedUnitIndex);
}

// =====================================================
//	class AiRuleMassiveAttack
// =====================================================

AiRuleMassiveAttack::AiRuleMassiveAttack(Ai *ai):
	AiRule(ai)
{
	ultraAttack=false;
}

bool AiRuleMassiveAttack::test(){

	if(ai->isStableBase()){
		ultraAttack= false;
		return ai->beingAttacked(attackPos, field, INT_MAX);
	}
	else{
		ultraAttack= true;
		return ai->beingAttacked(attackPos, field, baseRadius);
	}
}

void AiRuleMassiveAttack::execute(){
	ai->massiveAttack(attackPos, field, ultraAttack);
}
// =====================================================
//	class AiRuleAddTasks
// =====================================================

AiRuleAddTasks::AiRuleAddTasks(Ai *ai):
	AiRule(ai)
{
}

bool AiRuleAddTasks::test(){
	return !ai->anyTask() || ai->getCountOfClass(ucWorker) < 4;
}

void AiRuleAddTasks::execute(){
	int buildingCount= ai->getCountOfClass(ucBuilding);
	UnitClass ucWorkerType = ucWorker;
	int warriorCount= ai->getCountOfClass(ucWarrior,&ucWorkerType);
	int workerCount= ai->getCountOfClass(ucWorker);
	int upgradeCount= ai->getAiInterface()->getMyUpgradeCount();

	float buildingRatio= ai->getRatioOfClass(ucBuilding);
	float warriorRatio= ai->getRatioOfClass(ucWarrior);
	float workerRatio= ai->getRatioOfClass(ucWorker);

	//standard tasks
	if(ai->outputAIBehaviourToConsole()) printf("Add a TASK - AiRuleAddTasks adding ProduceTask(ucWorker) workerCount = %d, RULE Name[%s]\n",workerCount,this->getName().c_str());

	//emergency workers
	if(workerCount < 4){
		if(ai->outputAIBehaviourToConsole()) printf("AAA AiRuleAddTasks adding ProduceTask(ucWorker) workerCount = %d, RULE Name[%s]\n",workerCount,this->getName().c_str());
		ai->addPriorityTask(new ProduceTask(ucWorker));
	}
	else{
		if(	ai->getAiInterface()->getControlType() == ctCpuMega ||
			ai->getAiInterface()->getControlType() == ctNetworkCpuMega)
		{
			if(ai->outputAIBehaviourToConsole()) printf("AAA AiRuleAddTasks adding #1 workerCount = %d[%.2f], buildingCount = %d[%.2f] warriorCount = %d[%.2f] upgradeCount = %d RULE Name[%s]\n",
					workerCount,workerRatio,buildingCount,buildingRatio,warriorCount,warriorRatio,upgradeCount,this->getName().c_str());

			//workers
			if(workerCount<5) ai->addTask(new ProduceTask(ucWorker));
			if(workerCount<10) ai->addTask(new ProduceTask(ucWorker));
			if(workerRatio<0.20) ai->addTask(new ProduceTask(ucWorker));
			if(workerRatio<0.30) ai->addTask(new ProduceTask(ucWorker));

			//warriors
			if(warriorCount<10) ai->addTask(new ProduceTask(ucWarrior));
			if(warriorRatio<0.20) ai->addTask(new ProduceTask(ucWarrior));
			if(warriorRatio<0.30) ai->addTask(new ProduceTask(ucWarrior));
			if(workerCount>=10) ai->addTask(new ProduceTask(ucWarrior));
			if(workerCount>=15) ai->addTask(new ProduceTask(ucWarrior));
			if(warriorCount<ai->minWarriors+2)
			{
				ai->addTask(new ProduceTask(ucWarrior));
				if( buildingCount>9 )
				{
					ai->addTask(new ProduceTask(ucWarrior));
					ai->addTask(new ProduceTask(ucWarrior));
				}
				if( buildingCount>12 )
				{
					ai->addTask(new ProduceTask(ucWarrior));
					ai->addTask(new ProduceTask(ucWarrior));
				}
			}

			//buildings
			if(buildingCount<6 || buildingRatio<0.20) ai->addTask(new BuildTask());
			if(buildingCount<10 && workerCount>12) ai->addTask(new BuildTask());
			//upgrades
			if(upgradeCount==0 && workerCount>5) ai->addTask(new UpgradeTask());
			if(upgradeCount==1 && workerCount>10) ai->addTask(new UpgradeTask());
			if(upgradeCount==2 && workerCount>15) ai->addTask(new UpgradeTask());
			if(ai->isStableBase()) ai->addTask(new UpgradeTask());
		}
		else if(ai->getAiInterface()->getControlType() == ctCpuEasy ||
				ai->getAiInterface()->getControlType() == ctNetworkCpuEasy)
		{// Easy CPU

			if(ai->outputAIBehaviourToConsole()) printf("AAA AiRuleAddTasks adding #2 workerCount = %d[%.2f], buildingCount = %d[%.2f] warriorCount = %d[%.2f] upgradeCount = %d RULE Name[%s]\n",
					workerCount,workerRatio,buildingCount,buildingRatio,warriorCount,warriorRatio,upgradeCount,this->getName().c_str());

			//workers
			if(workerCount<buildingCount+2) ai->addTask(new ProduceTask(ucWorker));
			if(workerCount>5 && workerRatio<0.20) ai->addTask(new ProduceTask(ucWorker));

			//warriors
			if(warriorCount<10) ai->addTask(new ProduceTask(ucWarrior));
			if(warriorRatio<0.20) ai->addTask(new ProduceTask(ucWarrior));
			if(warriorRatio<0.30) ai->addTask(new ProduceTask(ucWarrior));
			if(workerCount>=10) ai->addTask(new ProduceTask(ucWarrior));
			if(workerCount>=15) ai->addTask(new ProduceTask(ucWarrior));

			//buildings
			if(buildingCount<6 || buildingRatio<0.20) ai->addTask(new BuildTask());
			if(buildingCount<10 && ai->isStableBase()) ai->addTask(new BuildTask());

			//upgrades
			if(upgradeCount==0 && workerCount>6) ai->addTask(new UpgradeTask());
			if(upgradeCount==1 && workerCount>7) ai->addTask(new UpgradeTask());
			if(upgradeCount==2 && workerCount>9) ai->addTask(new UpgradeTask());
			//if(ai->isStableBase()) ai->addTask(new UpgradeTask());
		}
		else
		{// normal CPU / UltraCPU ...
			if(ai->outputAIBehaviourToConsole()) printf("AAA AiRuleAddTasks adding #3 workerCount = %d[%.2f], buildingCount = %d[%.2f] warriorCount = %d[%.2f] upgradeCount = %d RULE Name[%s]\n",
					workerCount,workerRatio,buildingCount,buildingRatio,warriorCount,warriorRatio,upgradeCount,this->getName().c_str());

			//workers
			if(workerCount<5) ai->addTask(new ProduceTask(ucWorker));
			if(workerCount<10) ai->addTask(new ProduceTask(ucWorker));
			if(workerRatio<0.20) ai->addTask(new ProduceTask(ucWorker));
			if(workerRatio<0.30) ai->addTask(new ProduceTask(ucWorker));

			//warriors
			if(warriorCount<10) ai->addTask(new ProduceTask(ucWarrior));
			if(warriorRatio<0.20) ai->addTask(new ProduceTask(ucWarrior));
			if(warriorRatio<0.30) ai->addTask(new ProduceTask(ucWarrior));
			if(workerCount>=10) ai->addTask(new ProduceTask(ucWarrior));
			if(workerCount>=15) ai->addTask(new ProduceTask(ucWarrior));

			//buildings
			if(buildingCount<6 || buildingRatio<0.20) ai->addTask(new BuildTask());
			if(buildingCount<10 && workerCount>12) ai->addTask(new BuildTask());

			//upgrades
			if(upgradeCount==0 && workerCount>5) ai->addTask(new UpgradeTask());
			if(upgradeCount==1 && workerCount>10) ai->addTask(new UpgradeTask());
			if(upgradeCount==2 && workerCount>15) ai->addTask(new UpgradeTask());
			if(ai->isStableBase()) ai->addTask(new UpgradeTask());
		}
	}
}

// =====================================================
//	class AiRuleBuildOneFarm
// =====================================================

AiRuleBuildOneFarm::AiRuleBuildOneFarm(Ai *ai):
	AiRule(ai)
{
	farm=NULL;
}

bool AiRuleBuildOneFarm::test(){
	AiInterface *aiInterface= ai->getAiInterface();

	//for all units
	for(int i=0; i<aiInterface->getMyFactionType()->getUnitTypeCount(); ++i){
		const UnitType *ut= aiInterface->getMyFactionType()->getUnitType(i);

		//for all production commands
		for(int j=0; j<ut->getCommandTypeCount(); ++j){
			const CommandType *ct= ut->getCommandType(j);
			if(ct->getClass()==ccProduce){
				const UnitType *producedType= static_cast<const ProduceCommandType*>(ct)->getProducedUnit();

				//for all resources
				for(int k=0; k<producedType->getCostCount(); ++k){
					const Resource *r= producedType->getCost(k);

					//find a food producer in the farm produced units
					if(r->getAmount() < 0 && r->getType()->getClass() == rcConsumable && ai->getCountOfType(ut) == 0) {
						if(aiInterface->reqsOk(ct) && aiInterface->getMyFaction()->canCreateUnit(ut, true, true, true) == true) {
							farm= ut;
							//printf("AiRuleBuildOneFarm returning true, RULE Name[%s] ut [%s] producedType [%s]\n",this->getName().c_str(),ut->getName().c_str(),producedType->getName().c_str());

							if(ai->outputAIBehaviourToConsole()) printf("AiRuleBuildOneFarm returning true, RULE Name[%s]\n",this->getName().c_str());
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}

void AiRuleBuildOneFarm::execute(){
	ai->addPriorityTask(new BuildTask(farm));
}

// =====================================================
//	class AiRuleProduceResourceProducer
// =====================================================

AiRuleProduceResourceProducer::AiRuleProduceResourceProducer(Ai *ai):
	AiRule(ai)
{
	interval= shortInterval;
	rt=NULL;
}

bool AiRuleProduceResourceProducer::test(){
	//emergency tasks: resource buildings
	AiInterface *aiInterface= ai->getAiInterface();

	//consumables first
	for(int i=0; i<aiInterface->getTechTree()->getResourceTypeCount(); ++i){
        rt= aiInterface->getTechTree()->getResourceType(i);
		const Resource *r= aiInterface->getResource(rt);

		if(ai->outputAIBehaviourToConsole()) printf("CONSUMABLE [%s][%d] Testing AI RULE Name[%s]\n",rt->getName().c_str(), r->getBalance(), this->getName().c_str());

		bool factionUsesResourceType = aiInterface->factionUsesResourceType(aiInterface->getMyFactionType(), rt);
		//if(rt->getClass()==rcConsumable && r->getBalance()<0){
		if(factionUsesResourceType == true && rt->getClass() == rcConsumable && r->getBalance() < 0) {
			interval= longInterval;
			return true;

        }
    }

	//statics second
	for(int i=0; i < aiInterface->getTechTree()->getResourceTypeCount(); ++i) {
        rt= aiInterface->getTechTree()->getResourceType(i);
		const Resource *r= aiInterface->getResource(rt);

		if(ai->outputAIBehaviourToConsole()) printf("STATIC [%s][%d] [min %d] Testing AI RULE Name[%s]\n",rt->getName().c_str(), r->getAmount(), minStaticResources, this->getName().c_str());

		if(rt->getClass() == rcStatic && r->getAmount() < minStaticResources) {
			bool factionUsesResourceType = aiInterface->factionUsesResourceType(aiInterface->getMyFactionType(), rt);
			if(factionUsesResourceType == true) {
				interval= longInterval;
				return true;
			}
        }
    }

	if(ai->outputAIBehaviourToConsole()) printf("STATIC returning FALSE\n");

	interval= shortInterval;
	return false;
}

void AiRuleProduceResourceProducer::execute(){
	ai->addPriorityTask(new ProduceTask(rt));
	ai->addTask(new BuildTask(rt));
}

// =====================================================
//	class AiRuleProduce
// =====================================================

AiRuleProduce::AiRuleProduce(Ai *ai):
	AiRule(ai)
{
	produceTask= NULL;
}

bool AiRuleProduce::test(){
	const Task *task= ai->getTask();

	if(task==NULL || task->getClass()!=tcProduce){
		return false;
	}

	produceTask= static_cast<const ProduceTask*>(task);
	return true;
}

void AiRuleProduce::execute(){
	if(produceTask!=NULL){

		if(ai->outputAIBehaviourToConsole()) printf("AiRuleProduce producing [%s]\n",(produceTask->getUnitType() != NULL ? produceTask->getUnitType()->getName().c_str() : "null"));

		//generic produce task, produce random unit that has the skill or produces the resource
		if(produceTask->getUnitType()==NULL){
			produceGeneric(produceTask);
		}

		//specific produce task, produce if possible, retry if not enough resources
		else{
			produceSpecific(produceTask);
		}

		//remove the task
		ai->removeTask(produceTask);
	}
}

void AiRuleProduce::produceGeneric(const ProduceTask *pt) {
	typedef vector<const UnitType*> UnitTypes;
	UnitTypes ableUnits;
	AiInterface *aiInterface= ai->getAiInterface();

	if(pt->getResourceType() != NULL) {
		if(aiInterface->getMyFactionType()->getAIBehaviorUnits(aibcResourceProducerUnits).size() > 0) {
			const std::vector<FactionType::PairPUnitTypeInt> &unitList = aiInterface->getMyFactionType()->getAIBehaviorUnits(aibcResourceProducerUnits);
			for(unsigned int i = 0; i < unitList.size(); ++i) {
				const FactionType::PairPUnitTypeInt &priorityUnit = unitList[i];
				if(ai->getCountOfType(priorityUnit.first) < priorityUnit.second &&
					aiInterface->getMyFaction()->canCreateUnit(priorityUnit.first, false, true, true) == true) {
					//if(ai->getRandom()->randRange(0, 1)==0) {
					ai->addTask(new ProduceTask(priorityUnit.first));
					return;
					//}
				}
			}
		}
	}
	else if(pt->getUnitClass() == ucWorker) {
		if(aiInterface->getMyFactionType()->getAIBehaviorUnits(aibcWorkerUnits).size() > 0) {
			const std::vector<FactionType::PairPUnitTypeInt> &unitList = aiInterface->getMyFactionType()->getAIBehaviorUnits(aibcWorkerUnits);
			for(unsigned int i = 0; i < unitList.size(); ++i) {
				const FactionType::PairPUnitTypeInt &priorityUnit = unitList[i];
				if(ai->getCountOfType(priorityUnit.first) < priorityUnit.second &&
					aiInterface->getMyFaction()->canCreateUnit(priorityUnit.first, false, true, true) == true) {
					//if(ai->getRandom()->randRange(0, 1)==0) {
					ai->addTask(new ProduceTask(priorityUnit.first));
					return;
					//}
				}
			}
		}
	}
	else if(pt->getUnitClass() == ucWarrior) {
		if(aiInterface->getMyFactionType()->getAIBehaviorUnits(aibcWarriorUnits).size() > 0) {
			const std::vector<FactionType::PairPUnitTypeInt> &unitList = aiInterface->getMyFactionType()->getAIBehaviorUnits(aibcWarriorUnits);
			for(unsigned int i = 0; i < unitList.size(); ++i) {
				const FactionType::PairPUnitTypeInt &priorityUnit = unitList[i];
				if(ai->getCountOfType(priorityUnit.first) < priorityUnit.second &&
					aiInterface->getMyFaction()->canCreateUnit(priorityUnit.first, false, true, true) == true) {
					//if(ai->getRandom()->randRange(0, 1)==0) {
					ai->addTask(new ProduceTask(priorityUnit.first));
					return;
					//}
				}
			}
		}
	}

	//for each unit, produce it if possible
	for(int i = 0; i < aiInterface->getMyUnitCount(); ++i) {

		//for each command
		const UnitType *ut= aiInterface->getMyUnit(i)->getType();
		for(int j = 0; j < ut->getCommandTypeCount(); ++j) {
			const CommandType *ct= ut->getCommandType(j);

			//if the command is produce
			if(ct->getClass() == ccProduce || ct->getClass()==ccMorph) {

				const UnitType *producedUnit= static_cast<const UnitType*>(ct->getProduced());
				bool produceIt= false;

				if(ai->outputAIBehaviourToConsole()) printf("produceGeneric [%p] Testing AI RULE Name[%s]\n",pt->getResourceType(), this->getName().c_str());

				//if the unit produces the resource
				if(pt->getResourceType() != NULL) {
					const Resource *r= producedUnit->getCost(pt->getResourceType());

					if(r != NULL) {
						if(ai->outputAIBehaviourToConsole()) printf("produceGeneric r = [%s][%d] Testing AI RULE Name[%s]\n",r->getDescription().c_str(),r->getAmount(), this->getName().c_str());
					}

					if(r != NULL && r->getAmount() < 0) {
						produceIt= true;
					}
				}

				else {
					//if the unit is from the right class
					if(ai->outputAIBehaviourToConsole()) printf("produceGeneric right class = [%d] Testing AI RULE Name[%s]\n",producedUnit->isOfClass(pt->getUnitClass()), this->getName().c_str());

					if(producedUnit->isOfClass(pt->getUnitClass())){
						if(aiInterface->reqsOk(ct) && aiInterface->reqsOk(producedUnit)){
							produceIt= true;
						}
					}
				}

				if(produceIt) {
					//if the unit is not already on the list
					if(find(ableUnits.begin(), ableUnits.end(), producedUnit)==ableUnits.end()){
						ableUnits.push_back(producedUnit);
					}
				}
			}
		}
	}

	//add specific produce task
	if(ableUnits.empty() == false) {

		if(ai->outputAIBehaviourToConsole()) printf("produceGeneric !ableUnits.empty(), ableUnits.size() = [%d] Testing AI RULE Name[%s]\n",(int)ableUnits.size(), this->getName().c_str());

		//priority for non produced units
		for(unsigned int i=0; i < ableUnits.size(); ++i) {
			if(ai->getCountOfType(ableUnits[i]) == 0) {
				if(ai->getRandom()->randRange(0, 1)==0){
					ai->addTask(new ProduceTask(ableUnits[i]));
					return;
				}
			}
		}

		//normal case
		ai->addTask(new ProduceTask(ableUnits[ai->getRandom()->randRange(0, ableUnits.size()-1)]));
	}
}

void AiRuleProduce::produceSpecific(const ProduceTask *pt){

	AiInterface *aiInterface= ai->getAiInterface();

	if(ai->outputAIBehaviourToConsole()) printf("produceSpecific aiInterface->reqsOk(pt->getUnitType()) = [%d] Testing AI RULE Name[%s]\n",aiInterface->reqsOk(pt->getUnitType()), this->getName().c_str());

	//if unit meets requirements
	if(aiInterface->reqsOk(pt->getUnitType())){

		if(ai->outputAIBehaviourToConsole()) printf("produceSpecific aiInterface->checkCosts(pt->getUnitType()) = [%d] Testing AI RULE Name[%s]\n",aiInterface->checkCosts(pt->getUnitType()), this->getName().c_str());

		//if unit doesnt meet resources retry
		if(!aiInterface->checkCosts(pt->getUnitType())){
			ai->retryTask(pt);
			return;
		}

		//produce specific unit
		vector<int> producers;
		// Hold a list of units which can produce or morph
		// then a list of commandtypes for each unit
		map<int,vector<const CommandType *> > producersDefaultCommandType;
		const CommandType *defCt= NULL;

		//for each unit
		for(int i=0; i<aiInterface->getMyUnitCount(); ++i){

			//for each command
			const UnitType *ut= aiInterface->getMyUnit(i)->getType();
			for(int j=0; j<ut->getCommandTypeCount(); ++j){
				const CommandType *ct= ut->getCommandType(j);

				//if the command is produce
				if(ct->getClass()==ccProduce || ct->getClass()==ccMorph){
					const UnitType *producedUnit= static_cast<const UnitType*>(ct->getProduced());

					//if units match
					if(producedUnit == pt->getUnitType()){

						if(ai->outputAIBehaviourToConsole()) printf("produceSpecific aiInterface->reqsOk(ct) = [%d] Testing AI RULE Name[%s]\n",aiInterface->reqsOk(ct), this->getName().c_str());

						if(aiInterface->reqsOk(ct)){
							defCt= ct;
							producers.push_back(i);
							producersDefaultCommandType[i].push_back(ct);
						}
					}
				}
			}
		}

		//produce from random producer
		if(!producers.empty()){

			if(ai->outputAIBehaviourToConsole()) printf("produceSpecific producers.empty() = [%d] Testing AI RULE Name[%s]\n",producers.empty(), this->getName().c_str());

			if(	aiInterface->getControlType() == ctCpuMega ||
				aiInterface->getControlType() == ctNetworkCpuMega)
			{// mega cpu trys to balance the commands to the producers
				int randomstart=ai->getRandom()->randRange(0, producers.size()-1);
				int lowestCommandCount=1000000;
				int currentProducerIndex=producers[randomstart];
				int bestIndex=-1;
				//int besti=0;
				int currentCommandCount=0;
				for(unsigned int i=randomstart; i<producers.size()+randomstart; i++) {
					int prIndex = i;
					if(i >= producers.size()) {
						prIndex = (i - producers.size());
					}
					currentProducerIndex=producers[prIndex];

					if(currentProducerIndex >= aiInterface->getMyUnitCount()) {
						char szBuf[1024]="";
						printf("In [%s::%s Line: %d] currentProducerIndex >= aiInterface->getMyUnitCount(), currentProducerIndex = %d, aiInterface->getMyUnitCount() = %d, i = %d,producers.size() = %lu\n",__FILE__,__FUNCTION__,__LINE__,currentProducerIndex,aiInterface->getMyUnitCount(),i,(unsigned long)producers.size());
						sprintf(szBuf,"In [%s::%s Line: %d] currentProducerIndex >= aiInterface->getMyUnitCount(), currentProducerIndex = %d, aiInterface->getMyUnitCount() = %d, i = %d,producers.size() = %lu",__FILE__,__FUNCTION__,__LINE__,currentProducerIndex,aiInterface->getMyUnitCount(),i,(unsigned long)producers.size());
						throw runtime_error(szBuf);
					}
					if(prIndex >= producers.size()) {
						char szBuf[1024]="";
						printf("In [%s::%s Line: %d] prIndex >= producers.size(), currentProducerIndex = %d, i = %d,producers.size() = %lu \n",__FILE__,__FUNCTION__,__LINE__,prIndex,i,(unsigned long)producers.size());
						sprintf(szBuf,"In [%s::%s Line: %d] currentProducerIndex >= producers.size(), currentProducerIndex = %d, i = %d,producers.size() = %lu",__FILE__,__FUNCTION__,__LINE__,currentProducerIndex,i,(unsigned long)producers.size());
						throw runtime_error(szBuf);
					}

					currentCommandCount=aiInterface->getMyUnit(currentProducerIndex)->getCommandSize();
					if( currentCommandCount==1 &&
						aiInterface->getMyUnit(currentProducerIndex)->getCurrCommand()->getCommandType()->getClass()==ccStop)
					{// special for non buildings
						currentCommandCount=0;
					}
				    if(lowestCommandCount>currentCommandCount)
					{
						lowestCommandCount=aiInterface->getMyUnit(currentProducerIndex)->getCommandSize();
						bestIndex=currentProducerIndex;
						//besti=i%(producers.size());
					}
				}
				if(	aiInterface->getMyUnit(bestIndex)->getCommandSize() > 2) {
					// maybe we need another producer of this kind if possible!
					if(aiInterface->reqsOk(aiInterface->getMyUnit(bestIndex)->getType())) {
						if(ai->getCountOfClass(ucBuilding) > 5)
							ai->addTask(new BuildTask(aiInterface->getMyUnit(bestIndex)->getType()));
					}
					// need to calculate another producer, maybe its better to produce another warrior with another producer
					vector<int> backupProducers;
					// find another producer unit which is free and produce any kind of warrior.
					//for each unit
					for(int i=0; i<aiInterface->getMyUnitCount(); ++i){
						const UnitType *ut= aiInterface->getMyUnit(i)->getType();
						//for each command
						for(int j=0; j<ut->getCommandTypeCount(); ++j){
							const CommandType *ct= ut->getCommandType(j);
							//if the command is produce
							if(ct->getClass() == ccProduce) {
								const UnitType *unitType= static_cast<const UnitType*>(ct->getProduced());
								if(unitType->hasSkillClass(scAttack) && !unitType->hasCommandClass(ccHarvest) && aiInterface->reqsOk(ct))
								{//this can produce a warrior
									backupProducers.push_back(i);
								}
							}
						}
					}
					if(!backupProducers.empty()) {
						int randomstart=ai->getRandom()->randRange(0, backupProducers.size()-1);
						int lowestCommandCount=1000000;
						int currentProducerIndex=backupProducers[randomstart];
						int bestIndex=-1;
						int currentCommandCount=0;
						for(unsigned int i=randomstart; i<backupProducers.size()+randomstart; i++) {
							int prIndex = i;
							if(i >= backupProducers.size()) {
								prIndex = (i - backupProducers.size());
							}

							currentProducerIndex=backupProducers[prIndex];

							if(currentProducerIndex >= aiInterface->getMyUnitCount()) {
								char szBuf[1024]="";
								printf("In [%s::%s Line: %d] currentProducerIndex >= aiInterface->getMyUnitCount(), currentProducerIndex = %d, aiInterface->getMyUnitCount() = %d, i = %d,backupProducers.size() = %lu\n",__FILE__,__FUNCTION__,__LINE__,currentProducerIndex,aiInterface->getMyUnitCount(),i,(unsigned long)backupProducers.size());
								sprintf(szBuf,"In [%s::%s Line: %d] currentProducerIndex >= aiInterface->getMyUnitCount(), currentProducerIndex = %d, aiInterface->getMyUnitCount() = %d, i = %d,backupProducers.size() = %lu",__FILE__,__FUNCTION__,__LINE__,currentProducerIndex,aiInterface->getMyUnitCount(),i,(unsigned long)backupProducers.size());
								throw runtime_error(szBuf);
							}
							if(prIndex >= backupProducers.size()) {
								char szBuf[1024]="";
								printf("In [%s::%s Line: %d] prIndex >= backupProducers.size(), currentProducerIndex = %d, i = %d,backupProducers.size() = %lu \n",__FILE__,__FUNCTION__,__LINE__,prIndex,i,(unsigned long)backupProducers.size());
								sprintf(szBuf,"In [%s::%s Line: %d] currentProducerIndex >= backupProducers.size(), currentProducerIndex = %d, i = %d,backupProducers.size() = %lu",__FILE__,__FUNCTION__,__LINE__,currentProducerIndex,i,(unsigned long)backupProducers.size());
								throw runtime_error(szBuf);
							}

							currentCommandCount=aiInterface->getMyUnit(currentProducerIndex)->getCommandSize();
							if( currentCommandCount==1 &&
								aiInterface->getMyUnit(currentProducerIndex)->getCurrCommand()->getCommandType()->getClass()==ccStop)
							{// special for non buildings
								currentCommandCount=0;
							}
						    if(lowestCommandCount>currentCommandCount) {
								lowestCommandCount=currentCommandCount;
								bestIndex=currentProducerIndex;
								if(lowestCommandCount==0) break;
							}
						}
						// a good producer is found, lets choose a warrior production
						vector<int> productionCommandIndexes;
						const UnitType *ut=aiInterface->getMyUnit(bestIndex)->getType();
						for(int j=0; j<ut->getCommandTypeCount(); ++j){
							const CommandType *ct= ut->getCommandType(j);

							//if the command is produce
							if(ct->getClass()==ccProduce) {
								const UnitType *unitType= static_cast<const UnitType*>(ct->getProduced());
								if(unitType->hasSkillClass(scAttack) && !unitType->hasCommandClass(ccHarvest) && aiInterface->reqsOk(ct))
								{//this can produce a warrior
									productionCommandIndexes.push_back(j);
								}
							}
						}
						int commandIndex=productionCommandIndexes[ai->getRandom()->randRange(0, productionCommandIndexes.size()-1)];
						if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

						if(ai->outputAIBehaviourToConsole()) printf("mega #1 produceSpecific giveCommand to unit [%s] commandType [%s]\n",aiInterface->getMyUnit(bestIndex)->getType()->getName().c_str(),ut->getCommandType(commandIndex)->getName().c_str());
						aiInterface->giveCommand(bestIndex, ut->getCommandType(commandIndex));
					}
					else
					{// do it like normal CPU
						if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
						defCt = NULL;
						if(producersDefaultCommandType.find(bestIndex) != producersDefaultCommandType.end()) {
							int bestCommandTypeCount = producersDefaultCommandType[bestIndex].size();
							int bestCommandTypeIndex = ai->getRandom()->randRange(0, bestCommandTypeCount-1);

							if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] bestCommandTypeIndex = %d, bestCommandTypeCount = %d\n",__FILE__,__FUNCTION__,__LINE__,bestCommandTypeIndex,bestCommandTypeCount);

							defCt = producersDefaultCommandType[bestIndex][bestCommandTypeIndex];
						}

						if(ai->outputAIBehaviourToConsole()) printf("mega #2 produceSpecific giveCommand to unit [%s] commandType [%s]\n",aiInterface->getMyUnit(bestIndex)->getType()->getName().c_str(),defCt->getName().c_str());
						aiInterface->giveCommand(bestIndex, defCt);
					}
				}
				else
				{
					if(currentCommandCount==0) {
						if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
						defCt = NULL;
						if(producersDefaultCommandType.find(bestIndex) != producersDefaultCommandType.end()) {
							//defCt = producersDefaultCommandType[bestIndex];
							int bestCommandTypeCount = producersDefaultCommandType[bestIndex].size();
							int bestCommandTypeIndex = ai->getRandom()->randRange(0, bestCommandTypeCount-1);

							if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] bestCommandTypeIndex = %d, bestCommandTypeCount = %d\n",__FILE__,__FUNCTION__,__LINE__,bestCommandTypeIndex,bestCommandTypeCount);

							defCt = producersDefaultCommandType[bestIndex][bestCommandTypeIndex];
						}

						if(ai->outputAIBehaviourToConsole()) printf("mega #3 produceSpecific giveCommand to unit [%s] commandType [%s]\n",aiInterface->getMyUnit(bestIndex)->getType()->getName().c_str(),defCt->getName().c_str());
						aiInterface->giveCommand(bestIndex, defCt);
					}
					if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
					defCt = NULL;
					if(producersDefaultCommandType.find(bestIndex) != producersDefaultCommandType.end()) {
						//defCt = producersDefaultCommandType[bestIndex];
						int bestCommandTypeCount = producersDefaultCommandType[bestIndex].size();
						int bestCommandTypeIndex = ai->getRandom()->randRange(0, bestCommandTypeCount-1);

						if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] bestCommandTypeIndex = %d, bestCommandTypeCount = %d\n",__FILE__,__FUNCTION__,__LINE__,bestCommandTypeIndex,bestCommandTypeCount);

						defCt = producersDefaultCommandType[bestIndex][bestCommandTypeIndex];
					}
					if(ai->outputAIBehaviourToConsole()) printf("mega #4 produceSpecific giveCommand to unit [%s] commandType [%s]\n",aiInterface->getMyUnit(bestIndex)->getType()->getName().c_str(),defCt->getName().c_str());
					aiInterface->giveCommand(bestIndex, defCt);
				}
			}
			else {
				int pIndex = ai->getRandom()->randRange(0, producers.size()-1);
				int producerIndex= producers[pIndex];
				defCt = NULL;
				if(producersDefaultCommandType.find(producerIndex) != producersDefaultCommandType.end()) {
					//defCt = producersDefaultCommandType[producerIndex];
					int bestCommandTypeCount = producersDefaultCommandType[producerIndex].size();
					int bestCommandTypeIndex = ai->getRandom()->randRange(0, bestCommandTypeCount-1);

					if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] bestCommandTypeIndex = %d, bestCommandTypeCount = %d\n",__FILE__,__FUNCTION__,__LINE__,bestCommandTypeIndex,bestCommandTypeCount);

					defCt = producersDefaultCommandType[producerIndex][bestCommandTypeIndex];
				}

				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] producers.size() = %d, producerIndex = %d, pIndex = %d, producersDefaultCommandType.size() = %d\n",__FILE__,__FUNCTION__,__LINE__,producers.size(),producerIndex,pIndex,producersDefaultCommandType.size());

				if(ai->outputAIBehaviourToConsole()) printf("produceSpecific giveCommand to unit [%s] commandType [%s]\n",aiInterface->getMyUnit(producerIndex)->getType()->getName().c_str(),defCt->getName().c_str());
				aiInterface->giveCommand(producerIndex, defCt);
			}
		}
	}
}

// ========================================
// 	class AiRuleBuild
// ========================================

AiRuleBuild::AiRuleBuild(Ai *ai):
	AiRule(ai)
{
	buildTask= NULL;
}

bool AiRuleBuild::test(){
	const Task *task= ai->getTask();

	if(task==NULL || task->getClass()!=tcBuild){
		return false;
	}

	buildTask= static_cast<const BuildTask*>(task);
	return true;
}


void AiRuleBuild::execute() {
	if(buildTask!=NULL) {
		if(ai->outputAIBehaviourToConsole()) printf("BUILD AiRuleBuild Unit Name[%s]\n",(buildTask->getUnitType() != NULL ? buildTask->getUnitType()->getName().c_str() : "null"));

		//generic build task, build random building that can be built
		if(buildTask->getUnitType() == NULL) {
			buildGeneric(buildTask);
		}
		//specific building task, build if possible, retry if not enough resources or not position
		else {
			buildSpecific(buildTask);
		}

		//remove the task
		ai->removeTask(buildTask);
	}
}

void AiRuleBuild::buildGeneric(const BuildTask *bt) {

	//find buildings that can be built
	AiInterface *aiInterface= ai->getAiInterface();
	typedef vector<const UnitType*> UnitTypes;
	UnitTypes buildings;

	if(bt->getResourceType() != NULL) {
		if(aiInterface->getMyFactionType()->getAIBehaviorUnits(aibcResourceProducerUnits).size() > 0) {
			const std::vector<FactionType::PairPUnitTypeInt> &unitList = aiInterface->getMyFactionType()->getAIBehaviorUnits(aibcResourceProducerUnits);
			for(unsigned int i = 0; i < unitList.size(); ++i) {
				const FactionType::PairPUnitTypeInt &priorityUnit = unitList[i];
				if(ai->getCountOfType(priorityUnit.first) < priorityUnit.second &&
					aiInterface->getMyFaction()->canCreateUnit(priorityUnit.first, true, false, false) == true) {
					//if(ai->getRandom()->randRange(0, 1)==0) {
					ai->addTask(new BuildTask(priorityUnit.first));
					return;
					//}
				}
			}
		}
	}
	else {
		if(aiInterface->getMyFactionType()->getAIBehaviorUnits(aibcBuildingUnits).size() > 0) {
			const std::vector<FactionType::PairPUnitTypeInt> &unitList = aiInterface->getMyFactionType()->getAIBehaviorUnits(aibcBuildingUnits);
			for(unsigned int i = 0; i < unitList.size(); ++i) {
				const FactionType::PairPUnitTypeInt &priorityUnit = unitList[i];
				if(ai->getCountOfType(priorityUnit.first) < priorityUnit.second &&
					aiInterface->getMyFaction()->canCreateUnit(priorityUnit.first, true, false, false) == true) {
					//if(ai->getRandom()->randRange(0, 1)==0) {
					ai->addTask(new BuildTask(priorityUnit.first));
					return;
					//}
				}
			}
		}
	}

	//for each unit
	for(int i=0; i<aiInterface->getMyUnitCount(); ++i){

		//for each command
		const UnitType *ut= aiInterface->getMyUnit(i)->getType();
		for(int j=0; j<ut->getCommandTypeCount(); ++j){
			const CommandType *ct= ut->getCommandType(j);

			//if the command is build
			if(ct->getClass()==ccBuild){
				const BuildCommandType *bct= static_cast<const BuildCommandType*>(ct);

				//for each building
				for(int k=0; k<bct->getBuildingCount(); ++k){
					const UnitType *building= bct->getBuilding(k);
					if(aiInterface->reqsOk(bct) && aiInterface->reqsOk(building)){

						//if any building, or produces resource
						const ResourceType *rt= bt->getResourceType();
						const Resource *cost= building->getCost(rt);
						if(rt==NULL || (cost!=NULL && cost->getAmount()<0)){
							buildings.push_back(building);
						}
					}
				}
			}
		}
	}

	//add specific build task
	buildBestBuilding(buildings);
}

void AiRuleBuild::buildBestBuilding(const vector<const UnitType*> &buildings){

	if(!buildings.empty()){

		//build the least built building
		bool buildingFound= false;
		for(int i=0; i<10 && !buildingFound; ++i){

			if(i>0){

				//Defensive buildings have priority
				for(int j=0; j<buildings.size() && !buildingFound; ++j){
					const UnitType *building= buildings[j];
					if(ai->getCountOfType(building)<=i+1 && isDefensive(building))
					{
						ai->addTask(new BuildTask(building));
						buildingFound= true;
					}
				}

				//Warrior producers next
				for(unsigned int j=0; j<buildings.size() && !buildingFound; ++j){
					const UnitType *building= buildings[j];
					if(ai->getCountOfType(building)<=i+1 && isWarriorProducer(building))
					{
						ai->addTask(new BuildTask(building));
						buildingFound= true;
					}
				}

				//Resource producers next
				for(unsigned int j=0; j<buildings.size() && !buildingFound; ++j){
					const UnitType *building= buildings[j];
					if(ai->getCountOfType(building)<=i+1 && isResourceProducer(building))
					{
						ai->addTask(new BuildTask(building));
						buildingFound= true;
					}
				}
			}

			//Any building
			for(unsigned int j=0; j<buildings.size() && !buildingFound; ++j){
				const UnitType *building= buildings[j];
				if(ai->getCountOfType(building)<=i)
				{
					ai->addTask(new BuildTask(building));
					buildingFound= true;
				}
			}
		}
	}
}

void AiRuleBuild::buildSpecific(const BuildTask *bt) {
	AiInterface *aiInterface= ai->getAiInterface();
	//if reqs ok
	if(aiInterface->reqsOk(bt->getUnitType())) {

		//retry if not enough resources
		if(aiInterface->checkCosts(bt->getUnitType()) == false) {
			ai->retryTask(bt);
			return;
		}

		vector<int> builders;
		// Hold a list of units which can build
		// then a list of build commandtypes for each unit
		map<int,vector<const BuildCommandType *> > buildersDefaultCommandType;
		const BuildCommandType *defBct= NULL;

		//for each unit
		for(int i = 0; i < aiInterface->getMyUnitCount(); ++i) {

			//if the unit is not going to build
			const Unit *u = aiInterface->getMyUnit(i);
			if(u->anyCommand() == false || u->getCurrCommand()->getCommandType()->getClass() != ccBuild) {

				//for each command
				const UnitType *ut= aiInterface->getMyUnit(i)->getType();
				for(int j = 0; j < ut->getCommandTypeCount(); ++j) {
					const CommandType *ct= ut->getCommandType(j);

					//if the command is build
					if(ct->getClass() == ccBuild) {
						const BuildCommandType *bct= static_cast<const BuildCommandType*>(ct);

						//for each building
						for(int k = 0; k < bct->getBuildingCount(); ++k) {
							const UnitType *building= bct->getBuilding(k);

							//if building match
							if(bt->getUnitType() == building) {
								if(aiInterface->reqsOk(bct)) {
									builders.push_back(i);
									buildersDefaultCommandType[i].push_back(bct);
									defBct= bct;
								}
							}
						}
					}
				}
			}
		}

		//use random builder to build
		if(builders.empty() == false) {
			int bIndex = ai->getRandom()->randRange(0, builders.size()-1);
			int builderIndex= builders[bIndex];
			Vec2i pos;
			Vec2i searchPos = bt->getForcePos()? bt->getPos(): ai->getRandomHomePosition();
			if(bt->getForcePos() == false) {
				const int enemySightDistanceToAvoid = 18;
				vector<Unit*> enemies;
				ai->getAiInterface()->getWorld()->getUnitUpdater()->findEnemiesForCell(searchPos,bt->getUnitType()->getSize(),enemySightDistanceToAvoid,ai->getAiInterface()->getMyFaction(),enemies,true);
				if(enemies.empty() == false) {
					for(int i1 = 0; i1 < 25 && enemies.empty() == false; ++i1) {
						for(int j1 = 0; j1 < 25 && enemies.empty() == false; ++j1) {
							Vec2i tryPos = searchPos + Vec2i(i1,j1);

							const int spacing = 1;
							if(ai->getAiInterface()->isFreeCells(tryPos - Vec2i(spacing), bt->getUnitType()->getSize() + spacing * 2, fLand)) {
								enemies.clear();
								ai->getAiInterface()->getWorld()->getUnitUpdater()->findEnemiesForCell(tryPos,bt->getUnitType()->getSize(),enemySightDistanceToAvoid,ai->getAiInterface()->getMyFaction(),enemies,true);
								if(enemies.size() <= 0) {
									searchPos = tryPos;
								}
							}
						}
					}
				}
				if(enemies.empty() == false) {
					for(int i1 = -1; i1 >= -25 && enemies.empty() == false; --i1) {
						for(int j1 = -1; j1 >= -25 && enemies.empty() == false; --j1) {
							Vec2i tryPos = searchPos + Vec2i(i1,j1);

							const int spacing = 1;
							if(ai->getAiInterface()->isFreeCells(tryPos - Vec2i(spacing), bt->getUnitType()->getSize() + spacing * 2, fLand)) {
								enemies.clear();
								ai->getAiInterface()->getWorld()->getUnitUpdater()->findEnemiesForCell(tryPos,bt->getUnitType()->getSize(),enemySightDistanceToAvoid,ai->getAiInterface()->getMyFaction(),enemies,true);
								if(enemies.size() <= 0) {
									searchPos = tryPos;
								}
							}
						}
					}
				}
			}

			//if free pos give command, else retry
			if(ai->findPosForBuilding(bt->getUnitType(), searchPos, pos)) {
				defBct = NULL;
				if(buildersDefaultCommandType.find(builderIndex) != buildersDefaultCommandType.end()) {
					//defBct = buildersDefaultCommandType[builderIndex];
					int bestCommandTypeCount = buildersDefaultCommandType[builderIndex].size();
					int bestCommandTypeIndex = ai->getRandom()->randRange(0, bestCommandTypeCount-1);

					if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] bestCommandTypeIndex = %d, bestCommandTypeCount = %d\n",__FILE__,__FUNCTION__,__LINE__,bestCommandTypeIndex,bestCommandTypeCount);

					defBct = buildersDefaultCommandType[builderIndex][bestCommandTypeIndex];
				}
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] builderIndex = %d, bIndex = %d, defBct = %p\n",__FILE__,__FUNCTION__,__LINE__,builderIndex,bIndex,defBct);

				aiInterface->giveCommand(builderIndex, defBct, pos, bt->getUnitType());
			}
			else {
				ai->retryTask(bt);
				return;
			}
		}
	}
}

bool AiRuleBuild::isDefensive(const UnitType *building){
	if(ai->outputAIBehaviourToConsole()) printf("BUILD isDefensive check for Unit Name[%s] result = %d\n",building->getName().c_str(),building->hasSkillClass(scAttack));

	return building->hasSkillClass(scAttack);
}

bool AiRuleBuild::isResourceProducer(const UnitType *building){
	for(int i= 0; i<building->getCostCount(); i++){
		if(building->getCost(i)->getAmount()<0){
			if(ai->outputAIBehaviourToConsole()) printf("BUILD isResourceProducer check for Unit Name[%s] result = true\n",building->getName().c_str());
			return true;
		}
	}
	if(ai->outputAIBehaviourToConsole()) printf("BUILD isResourceProducer check for Unit Name[%s] result = false\n",building->getName().c_str());
	return false;
}

bool AiRuleBuild::isWarriorProducer(const UnitType *building){
	for(int i= 0; i < building->getCommandTypeCount(); i++){
		const CommandType *ct= building->getCommandType(i);
		if(ct->getClass() == ccProduce){
			const UnitType *ut= static_cast<const ProduceCommandType*>(ct)->getProducedUnit();

			if(ut->isOfClass(ucWarrior)){
				if(ai->outputAIBehaviourToConsole()) printf("BUILD isWarriorProducer check for Unit Name[%s] result = true\n",building->getName().c_str());
				return true;
			}
		}
	}
	if(ai->outputAIBehaviourToConsole()) printf("BUILD isWarriorProducer check for Unit Name[%s] result = false\n",building->getName().c_str());
	return false;
}

// ========================================
// 	class AiRuleUpgrade
// ========================================

AiRuleUpgrade::AiRuleUpgrade(Ai *ai):
	AiRule(ai)
{
	upgradeTask= NULL;
}

bool AiRuleUpgrade::test(){
	const Task *task= ai->getTask();

	if(task==NULL || task->getClass()!=tcUpgrade){
		return false;
	}

	upgradeTask= static_cast<const UpgradeTask*>(task);
	return true;
}

void AiRuleUpgrade::execute(){

	//upgrade any upgrade
	if(upgradeTask->getUpgradeType()==NULL){
		upgradeGeneric(upgradeTask);
	}
	//upgrade specific upgrade
	else{
		upgradeSpecific(upgradeTask);
	}

	//remove the task
	ai->removeTask(upgradeTask);
}

void AiRuleUpgrade::upgradeGeneric(const UpgradeTask *upgt){

	typedef vector<const UpgradeType*> UpgradeTypes;
	AiInterface *aiInterface= ai->getAiInterface();

	//find upgrades that can be upgraded
	UpgradeTypes upgrades;

	if(aiInterface->getMyFactionType()->getAIBehaviorUpgrades().size() > 0) {
		const std::vector<const UpgradeType*> &upgradeList = aiInterface->getMyFactionType()->getAIBehaviorUpgrades();
		for(unsigned int i = 0; i < upgradeList.size(); ++i) {
			const UpgradeType* priorityUpgrade = upgradeList[i];

			//for each upgrade, upgrade it if possible
			for(int k = 0; k < aiInterface->getMyUnitCount(); ++k) {

				//for each command
				const UnitType *ut= aiInterface->getMyUnit(k)->getType();
				for(int j = 0; j < ut->getCommandTypeCount(); ++j) {
					const CommandType *ct= ut->getCommandType(j);

					//if the command is upgrade
					if(ct->getClass() == ccUpgrade) {
						const UpgradeCommandType *upgct= dynamic_cast<const UpgradeCommandType*>(ct);
						if(upgct != NULL) {
							const UpgradeType *upgrade= upgct->getProducedUpgrade();
							if(upgrade == priorityUpgrade) {
								if(aiInterface->reqsOk(upgct) == true &&
									aiInterface->getMyFaction()->getUpgradeManager()->isUpgradingOrUpgraded(priorityUpgrade) == false) {
									//if(ai->getRandom()->randRange(0, 1)==0) {
									ai->addTask(new UpgradeTask(priorityUpgrade));
									return;
									//}
								}
							}
						}
					}
				}
			}
		}
	}

	//for each upgrade, upgrade it if possible
	for(int i=0; i<aiInterface->getMyUnitCount(); ++i){

		//for each command
		const UnitType *ut= aiInterface->getMyUnit(i)->getType();
		for(int j=0; j<ut->getCommandTypeCount(); ++j){
			const CommandType *ct= ut->getCommandType(j);

			//if the command is upgrade
			if(ct->getClass()==ccUpgrade){
				const UpgradeCommandType *upgct= static_cast<const UpgradeCommandType*>(ct);
				const UpgradeType *upgrade= upgct->getProducedUpgrade();
				if(aiInterface->reqsOk(upgct)){
					upgrades.push_back(upgrade);
				}
			}
		}
	}

	//add specific upgrade task
	if(!upgrades.empty()){
		ai->addTask(new UpgradeTask(upgrades[ai->getRandom()->randRange(0, upgrades.size()-1)]));
	}
}

void AiRuleUpgrade::upgradeSpecific(const UpgradeTask *upgt){

	AiInterface *aiInterface= ai->getAiInterface();

	//if reqs ok
	if(aiInterface->reqsOk(upgt->getUpgradeType())){

		//if resources dont meet retry
		if(!aiInterface->checkCosts(upgt->getUpgradeType())){
			ai->retryTask(upgt);
			return;
		}

		//for each unit
		for(int i=0; i<aiInterface->getMyUnitCount(); ++i){

			//for each command
			const UnitType *ut= aiInterface->getMyUnit(i)->getType();
			for(int j=0; j<ut->getCommandTypeCount(); ++j){
				const CommandType *ct= ut->getCommandType(j);

				//if the command is upgrade
				if(ct->getClass()==ccUpgrade){
					const UpgradeCommandType *uct= static_cast<const UpgradeCommandType*>(ct);
					const UpgradeType *producedUpgrade= uct->getProducedUpgrade();

					//if upgrades match
					if(producedUpgrade == upgt->getUpgradeType()){
						if(aiInterface->reqsOk(uct)){
							if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
							aiInterface->giveCommand(i, uct);
						}
					}
				}
			}
		}

	}
}

// ========================================
// 	class AiRuleExpand
// ========================================

AiRuleExpand::AiRuleExpand(Ai *ai):
	AiRule(ai)
{
	storeType= NULL;
}

bool AiRuleExpand::test() {
	AiInterface *aiInterface = ai->getAiInterface();

	int unitCount = aiInterface->getMyUnitCount();
	for(int i = 0; i < aiInterface->getTechTree()->getResourceTypeCount(); ++i) {
		const ResourceType *rt = aiInterface->getTechTree()->getResourceType(i);
		if(rt->getClass() == rcTech) {
			bool factionUsesResourceType = aiInterface->factionUsesResourceType(aiInterface->getMyFactionType(), rt);
			if(factionUsesResourceType == true) {
				// If any resource sighted
				if(aiInterface->getNearestSightedResource(rt, aiInterface->getHomeLocation(), expandPos, true)) {
					int minDistance= INT_MAX;
					storeType= NULL;

					//If there is no close store
					for(int j=0; j < unitCount; ++j) {
						const Unit *u= aiInterface->getMyUnit(j);
						const UnitType *ut= u->getType();

						// If this building is a store
						if(ut->getStore(rt) > 0) {
							storeType = ut;
							int distance= static_cast<int> (u->getPos().dist(expandPos));
							if(distance < minDistance) {
								minDistance = distance;
							}
						}
					}

					if(minDistance > expandDistance) {
						return true;
					}
				}
				else {
					// send patrol to look for resource
					ai->sendScoutPatrol();
				}
			}
		}
	}

	return false;
}

void AiRuleExpand::execute(){
	ai->addExpansion(expandPos);
	ai->addPriorityTask(new BuildTask(storeType, expandPos));
}


// ========================================
// 	class AiRuleUnBlock
// ========================================

AiRuleUnBlock::AiRuleUnBlock(Ai *ai):
	AiRule(ai)
{

}

bool AiRuleUnBlock::test() {
	return ai->haveBlockedUnits();
}

void AiRuleUnBlock::execute(){
	ai->unblockUnits();
}

}}//end namespace
