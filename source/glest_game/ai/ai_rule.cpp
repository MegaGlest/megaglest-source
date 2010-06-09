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
}

bool AiRuleRepair::test(){
	AiInterface *aiInterface= ai->getAiInterface();

	//look for a damaged unit
	for(int i=0; i<aiInterface->getMyUnitCount(); ++i){
		const Unit *u= aiInterface->getMyUnit(i);
		if(u->getHpRatio()<1.f){
			damagedUnitIndex= i;
			return true;
		}
	}
	return false;
}

void AiRuleRepair::execute(){
	AiInterface *aiInterface= ai->getAiInterface();
	const Unit *damagedUnit= aiInterface->getMyUnit(damagedUnitIndex);
	
	//find a repairer and issue command
	for(int i=0; i<aiInterface->getMyUnitCount(); ++i){
		const Unit *u= aiInterface->getMyUnit(i);
		const RepairCommandType *rct= static_cast<const RepairCommandType *>(u->getType()->getFirstCtOfClass(ccRepair));
		if(rct!=NULL && (u->getCurrSkill()->getClass()==scStop || u->getCurrSkill()->getClass()==scMove)){
			if(rct->isRepairableUnitType(damagedUnit->getType())){
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

				aiInterface->giveCommand(i, rct, damagedUnit->getPos()); 
				aiInterface->printLog(3, "Repairing order issued");
				return;
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
	return !ai->anyTask() || ai->getCountOfClass(ucWorker)<4;
}

void AiRuleAddTasks::execute(){
	int buildingCount= ai->getCountOfClass(ucBuilding);
	int warriorCount= ai->getCountOfClass(ucWarrior);
	int workerCount= ai->getCountOfClass(ucWorker);
	int upgradeCount= ai->getAiInterface()->getMyUpgradeCount();

	float buildingRatio= ai->getRatioOfClass(ucBuilding);
	float warriorRatio= ai->getRatioOfClass(ucWarrior);
	float workerRatio= ai->getRatioOfClass(ucWorker);

	//standard tasks

	//emergency workers
	if(workerCount<4){
		ai->addPriorityTask(new ProduceTask(ucWorker));
	}
	else{
		if(ai->getAiInterface()->getControlType()==ctCpuMega)
		{
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
		else if(ai->getAiInterface()->getControlType()==ctCpuEasy)
		{// Easy CPU
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
					if(r->getAmount()<0 && r->getType()->getClass()==rcConsumable && ai->getCountOfType(ut)==0){
						farm= ut;
						return true;
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
}

bool AiRuleProduceResourceProducer::test(){
	//emergency tasks: resource buildings
	AiInterface *aiInterface= ai->getAiInterface();

	//consumables first
	for(int i=0; i<aiInterface->getTechTree()->getResourceTypeCount(); ++i){
        rt= aiInterface->getTechTree()->getResourceType(i);
		const Resource *r= aiInterface->getResource(rt);   
		if(rt->getClass()==rcConsumable && r->getBalance()<0){
			interval= longInterval;
			return true;
			
        }
    }

	//statics second
	for(int i=0; i<aiInterface->getTechTree()->getResourceTypeCount(); ++i){
        rt= aiInterface->getTechTree()->getResourceType(i);
		const Resource *r= aiInterface->getResource(rt);   
		if(rt->getClass()==rcStatic && r->getAmount()<minStaticResources){
			interval= longInterval;
			return true;
			
        }
    }

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

void AiRuleProduce::produceGeneric(const ProduceTask *pt){
	typedef vector<const UnitType*> UnitTypes; 
	UnitTypes ableUnits;
	AiInterface *aiInterface= ai->getAiInterface();

	//for each unit, produce it if possible
	for(int i=0; i<aiInterface->getMyUnitCount(); ++i){
		
		//for each command
		const UnitType *ut= aiInterface->getMyUnit(i)->getType();
		for(int j=0; j<ut->getCommandTypeCount(); ++j){
			const CommandType *ct= ut->getCommandType(j);
			
			//if the command is produce
			if(ct->getClass()==ccProduce || ct->getClass()==ccMorph){
				
				const UnitType *producedUnit= static_cast<const UnitType*>(ct->getProduced());
				bool produceIt= false;

				//if the unit produces the resource
				if(pt->getResourceType()!=NULL){
					const Resource *r= producedUnit->getCost(pt->getResourceType());
					if(r!=NULL && r->getAmount()<0){
						produceIt= true;
					}
				}

				else{
					//if the unit is from the right class
					if(producedUnit->isOfClass(pt->getUnitClass())){
						if(aiInterface->reqsOk(ct) && aiInterface->reqsOk(producedUnit)){
							produceIt= true;
						}
					}
				}

				if(produceIt){
					//if the unit is not already on the list
					if(find(ableUnits.begin(), ableUnits.end(), producedUnit)==ableUnits.end()){
						ableUnits.push_back(producedUnit);
					}
				}
			}
		}
	}

	//add specific produce task
	if(!ableUnits.empty()){

		//priority for non produced units
		for(unsigned int i=0; i<ableUnits.size(); ++i){
			if(ai->getCountOfType(ableUnits[i])==0){
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
	
	//if unit meets requirements
	if(aiInterface->reqsOk(pt->getUnitType())){
	
		//if unit doesnt meet resources retry
		if(!aiInterface->checkCosts(pt->getUnitType())){
			ai->retryTask(pt);
			return;
		}

		//produce specific unit
		vector<int> producers;
		vector<const CommandType *> producersDefaultCommandType;
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
						if(aiInterface->reqsOk(ct)){
							defCt= ct;
							producers.push_back(i);
							producersDefaultCommandType.push_back(ct);
						}
					} 
				}
			}
		}

		//produce from random producer
		if(!producers.empty()){
			if(aiInterface->getControlType()==ctCpuMega)
			{// mega cpu trys to balance the commands to the producers
				int randomstart=ai->getRandom()->randRange(0, producers.size()-1);
				int lowestCommandCount=1000000;
				int currentProducerIndex=producers[randomstart];
				int bestIndex=-1;
				int besti=0;
				int currentCommandCount=0;
				for(unsigned int i=randomstart; i<producers.size()+randomstart; i++)
				{
					currentProducerIndex=producers[i%(producers.size())];

					if(currentProducerIndex >= aiInterface->getMyUnitCount()) {
						char szBuf[1024]="";
						sprintf(szBuf,"In [%s::%s Line: %d] currentProducerIndex >= aiInterface->getMyUnitCount(), currentProducerIndex = %d, aiInterface->getMyUnitCount() = %d, i = %d,producers.size() = %d",__FILE__,__FUNCTION__,__LINE__,currentProducerIndex,aiInterface->getMyUnitCount(),i,producers.size());
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
						besti=i%(producers.size());
					}
				}
				if(	aiInterface->getMyUnit(bestIndex)->getCommandSize()>2)
				{
					// maybe we need another producer of this kind if possible!
					if(aiInterface->reqsOk(aiInterface->getMyUnit(bestIndex)->getType()))
					{ 
						if(ai->getCountOfClass(ucBuilding)>5)
							ai->addTask(new BuildTask(aiInterface->getMyUnit(bestIndex)->getType()));
					}
					// need to calculate another producer, maybe its better to produce another warrior with another producer
					vector<int> backupProducers;
					vector<const CommandType *> backupProducersDefaultCommandType;
					// find another producer unit which is free and produce any kind of warrior.
					//for each unit
					for(int i=0; i<aiInterface->getMyUnitCount(); ++i){
						const UnitType *ut= aiInterface->getMyUnit(i)->getType();
						//for each command
						for(int j=0; j<ut->getCommandTypeCount(); ++j){
							const CommandType *ct= ut->getCommandType(j);
							//if the command is produce
							if(ct->getClass()==ccProduce)
							{
								const UnitType *unitType= static_cast<const UnitType*>(ct->getProduced());								
								if(unitType->hasSkillClass(scAttack) && !unitType->hasCommandClass(ccHarvest) && aiInterface->reqsOk(ct))
								{//this can produce a warrior
									backupProducers.push_back(i);
									backupProducersDefaultCommandType.push_back(ct);
								} 
							}
						}	
					}
					if(!backupProducers.empty())
					{
						int randomstart=ai->getRandom()->randRange(0, backupProducers.size()-1);
						int lowestCommandCount=1000000;
						int currentProducerIndex=backupProducers[randomstart];
						int bestIndex=-1;
						int currentCommandCount=0;
						for(unsigned int i=randomstart; i<backupProducers.size()+randomstart; i++)
						{
							currentProducerIndex=backupProducers[i%(backupProducers.size())];
							currentCommandCount=aiInterface->getMyUnit(currentProducerIndex)->getCommandSize();
							if( currentCommandCount==1 &&
								aiInterface->getMyUnit(currentProducerIndex)->getCurrCommand()->getCommandType()->getClass()==ccStop)
							{// special for non buildings
								currentCommandCount=0;
							}
						    if(lowestCommandCount>currentCommandCount)
							{
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
							if(ct->getClass()==ccProduce)
							{
								const UnitType *unitType= static_cast<const UnitType*>(ct->getProduced());								
								if(unitType->hasSkillClass(scAttack) && !unitType->hasCommandClass(ccHarvest) && aiInterface->reqsOk(ct))
								{//this can produce a warrior
										productionCommandIndexes.push_back(j);
								} 
							}
						}
						int commandIndex=productionCommandIndexes[ai->getRandom()->randRange(0, productionCommandIndexes.size()-1)];
						SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
						aiInterface->giveCommand(bestIndex, ut->getCommandType(commandIndex));
					}
					else
					{// do it like normal CPU
						SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
						defCt = (bestIndex < producersDefaultCommandType.size() ? producersDefaultCommandType[bestIndex] : NULL);
						aiInterface->giveCommand(bestIndex, defCt);
					}
				}
				else
				{	
					if(currentCommandCount==0)
					{
						SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
						defCt = (bestIndex < producersDefaultCommandType.size() ? producersDefaultCommandType[bestIndex] : NULL);
						aiInterface->giveCommand(bestIndex, defCt);
					}
					SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
					defCt = (bestIndex < producersDefaultCommandType.size() ? producersDefaultCommandType[bestIndex] : NULL);
					aiInterface->giveCommand(bestIndex, defCt);
				}
			}
			else
			{
				int pIndex = ai->getRandom()->randRange(0, producers.size()-1);
				int producerIndex= producers[pIndex];
				defCt = (pIndex < producersDefaultCommandType.size() ? producersDefaultCommandType[pIndex] : NULL);
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] producers.size() = %d, producerIndex = %d, pIndex = %d, producersDefaultCommandType.size() = %d\n",__FILE__,__FUNCTION__,__LINE__,producers.size(),producerIndex,pIndex,producersDefaultCommandType.size());
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


void AiRuleBuild::execute(){
	
	if(buildTask!=NULL){

		//generic build task, build random building that can be built
		if(buildTask->getUnitType()==NULL){
			buildGeneric(buildTask);
		}
		//specific building task, build if possible, retry if not enough resources or not position
		else{
			buildSpecific(buildTask);
		}

		//remove the task
		ai->removeTask(buildTask);
	}
}

void AiRuleBuild::buildGeneric(const BuildTask *bt){
	
	//find buildings that can be built
	AiInterface *aiInterface= ai->getAiInterface();
	typedef vector<const UnitType*> UnitTypes; 
	UnitTypes buildings;
	
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

void AiRuleBuild::buildSpecific(const BuildTask *bt){
	AiInterface *aiInterface= ai->getAiInterface();
	//if reqs ok
	if(aiInterface->reqsOk(bt->getUnitType())){

		//retry if not enough resources
		if(!aiInterface->checkCosts(bt->getUnitType())){
			ai->retryTask(bt);
			return;
		}

		vector<int> builders;
		vector<const BuildCommandType *> buildersDefaultCommandType;
		const BuildCommandType *defBct= NULL;
						
		//for each unit
		for(int i=0; i<aiInterface->getMyUnitCount(); ++i){
			
			//if the unit is not going to build
			const Unit *u= aiInterface->getMyUnit(i);
			if(!u->anyCommand() || u->getCurrCommand()->getCommandType()->getClass()!=ccBuild){

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
							
							//if building match
							if(bt->getUnitType()==building){
								if(aiInterface->reqsOk(bct)){
									builders.push_back(i);
									buildersDefaultCommandType.push_back(bct);
									defBct= bct;
								}
							}
						}
					}
				}
			}
		}

		//use random builder to build
		if(!builders.empty()){
			int bIndex = ai->getRandom()->randRange(0, builders.size()-1);
			int builderIndex= builders[bIndex];
			Vec2i pos;		
			Vec2i searchPos= bt->getForcePos()? bt->getPos(): ai->getRandomHomePosition();

			//if free pos give command, else retry
			if(ai->findPosForBuilding(bt->getUnitType(), searchPos, pos)){
				defBct = (bIndex < buildersDefaultCommandType.size() ? buildersDefaultCommandType[bIndex] : NULL);
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] builderIndex = %d, bIndex = %d, defBct = %p\n",__FILE__,__FUNCTION__,__LINE__,builderIndex,bIndex,defBct);

				aiInterface->giveCommand(builderIndex, defBct, pos, bt->getUnitType());
			}
			else{
				ai->retryTask(bt);
				return;
			}
		}
	}
}

bool AiRuleBuild::isDefensive(const UnitType *building){
	return building->hasSkillClass(scAttack);
}

bool AiRuleBuild::isResourceProducer(const UnitType *building){
	for(int i= 0; i<building->getCostCount(); i++){
		if(building->getCost(i)->getAmount()<0){
			return true;
		}
	}
	return false;
}

bool AiRuleBuild::isWarriorProducer(const UnitType *building){
	for(int i= 0; i < building->getCommandTypeCount(); i++){
		const CommandType *ct= building->getCommandType(i);
		if(ct->getClass() == ccProduce){
			const UnitType *ut= static_cast<const ProduceCommandType*>(ct)->getProducedUnit();

			if(ut->isOfClass(ucWarrior)){
				return true;
			}
		}
	}
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
							SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
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

bool AiRuleExpand::test(){
	AiInterface *aiInterface = ai->getAiInterface();

	for(int i= 0; i<aiInterface->getTechTree()->getResourceTypeCount(); ++i){
		const ResourceType *rt = aiInterface->getTechTree()->getResourceType(i);

		if(rt->getClass()==rcTech){

			// If any resource sighted
			if(aiInterface->getNearestSightedResource(rt, aiInterface->getHomeLocation(), expandPos)){
			
				int minDistance= INT_MAX;
				storeType= NULL;

				//If there is no close store
				for(int j=0; j<aiInterface->getMyUnitCount(); ++j){
					const Unit *u= aiInterface->getMyUnit(j);
					const UnitType *ut= aiInterface->getMyUnit(j)->getType();

					// If this building is a store
					if(ut->getStore(rt)>0){
						storeType = ut;
						int distance= static_cast<int> (u->getPos().dist(expandPos));

						if(distance < minDistance){
							minDistance = distance;
						}
					}
				}

				if(minDistance>expandDistance)
				{
					return true;
				}
			}
			else{
				// send patrol to look for resource
				ai->sendScoutPatrol();
			}
		}
	}
	
	return false;
}

void AiRuleExpand::execute(){
	ai->addExpansion(expandPos);
	ai->addPriorityTask(new BuildTask(storeType, expandPos));
}

}}//end namespace 
