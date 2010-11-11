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

#include "ai.h" 
#include "ai_interface.h"
#include "ai_rule.h"
#include "unit_type.h"
#include "unit.h"
#include "leak_dumper.h"

using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest { namespace Game {

// =====================================================
// 	class ProduceTask
// =====================================================

ProduceTask::ProduceTask(UnitClass unitClass){
	taskClass= tcProduce;
	this->unitClass= unitClass;
	unitType= NULL;
	resourceType= NULL;
}

ProduceTask::ProduceTask(const UnitType *unitType){
	taskClass= tcProduce;
	this->unitType= unitType;
	resourceType= NULL;
}

ProduceTask::ProduceTask(const ResourceType *resourceType){
	taskClass= tcProduce;
	unitType= NULL;
	this->resourceType= resourceType;
}

string ProduceTask::toString() const{
	string str= "Produce ";
	if(unitType!=NULL){
		str+= unitType->getName();
	}
	return str;
}

// =====================================================
// 	class BuildTask
// =====================================================

BuildTask::BuildTask(const UnitType *unitType){
	taskClass= tcBuild;
	this->unitType= unitType;
	resourceType= NULL;
	forcePos= false;
}

BuildTask::BuildTask(const ResourceType *resourceType){
	taskClass= tcBuild;
	unitType= NULL;
	this->resourceType= resourceType;
	forcePos= false;
}

BuildTask::BuildTask(const UnitType *unitType, const Vec2i &pos){
	taskClass= tcBuild;
	this->unitType= unitType;
	resourceType= NULL;
	forcePos= true;
	this->pos= pos;
}

string BuildTask::toString() const{
	string str= "Build ";
	if(unitType!=NULL){
		str+= unitType->getName();
	}
	return str;
}
	
// =====================================================
// 	class UpgradeTask
// =====================================================

UpgradeTask::UpgradeTask(const UpgradeType *upgradeType){
	taskClass= tcUpgrade;
	this->upgradeType= upgradeType;
}

string UpgradeTask::toString() const{
	string str= "Build ";
	if(upgradeType!=NULL){
		str+= upgradeType->getName();
	}
	return str;
}

// =====================================================
// 	class Ai
// =====================================================

void Ai::init(AiInterface *aiInterface, int useStartLocation) {
	this->aiInterface= aiInterface; 
	if(useStartLocation == -1) {
		startLoc = random.randRange(0, aiInterface->getMapMaxPlayers()-1);
	}
	else {
		startLoc = useStartLocation;
	}
	minWarriors= minMinWarriors;
	randomMinWarriorsReached= false;
	//add ai rules
	aiRules.clear();
	aiRules.push_back(new AiRuleWorkerHarvest(this));
	aiRules.push_back(new AiRuleRefreshHarvester(this));
	aiRules.push_back(new AiRuleScoutPatrol(this));
	aiRules.push_back(new AiRuleReturnBase(this));
	aiRules.push_back(new AiRuleMassiveAttack(this));
	aiRules.push_back(new AiRuleAddTasks(this));
	aiRules.push_back(new AiRuleProduceResourceProducer(this));
	aiRules.push_back(new AiRuleBuildOneFarm(this));
	aiRules.push_back(new AiRuleProduce(this));
	aiRules.push_back(new AiRuleBuild(this));
	aiRules.push_back(new AiRuleUpgrade(this));
	aiRules.push_back(new AiRuleExpand(this));
	aiRules.push_back(new AiRuleRepair(this));
	aiRules.push_back(new AiRuleRepair(this));
}

Ai::~Ai() {
	deleteValues(tasks.begin(), tasks.end());
	deleteValues(aiRules.begin(), aiRules.end());
}

void Ai::update() {

	Chrono chrono;
	chrono.start();

	if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [START]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	//process ai rules
	for(int ruleIdx = 0; ruleIdx < aiRules.size(); ++ruleIdx) {
		AiRule *rule = aiRules[ruleIdx];
		if(rule == NULL) {
			throw runtime_error("rule == NULL");
		}

		if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [ruleIdx = %d]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis(),ruleIdx);

		if((aiInterface->getTimer() % (rule->getTestInterval() * GameConstants::updateFps / 1000)) == 0) {

			if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [ruleIdx = %d, before rule->test()]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis(),ruleIdx);

			if(rule->test()) {
				aiInterface->printLog(3, intToStr(1000 * aiInterface->getTimer() / GameConstants::updateFps) + ": Executing rule: " + rule->getName() + '\n');

				if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [ruleIdx = %d, before rule->execute() [%s]]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis(),ruleIdx,rule->getName().c_str());

				rule->execute();

				if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [ruleIdx = %d, after rule->execute() [%s]]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis(),ruleIdx,rule->getName().c_str());
			}
		}
	}

	if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [END]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
}


// ==================== state requests ==================== 

int Ai::getCountOfType(const UnitType *ut){
    int count= 0;
	for(int i=0; i<aiInterface->getMyUnitCount(); ++i){
		if(ut == aiInterface->getMyUnit(i)->getType()){
			count++;
		}
	}
    return count;
}

int Ai::getCountOfClass(UnitClass uc){
    int count= 0;
    for(int i=0; i<aiInterface->getMyUnitCount(); ++i){
		if(aiInterface->getMyUnit(i)->getType()->isOfClass(uc)){
            ++count;
		}
    }
    return count;
}

float Ai::getRatioOfClass(UnitClass uc){
	if(aiInterface->getMyUnitCount()==0){
		return 0;
	}
	else{
		return static_cast<float>(getCountOfClass(uc))/aiInterface->getMyUnitCount();
	}
}

const ResourceType *Ai::getNeededResource(int unitIndex) {
	int amount = INT_MAX;
	const ResourceType *neededResource= NULL;
    const TechTree *tt= aiInterface->getTechTree();
    const Unit *unit = aiInterface->getMyUnit(unitIndex);

    for(int i = 0; i < tt->getResourceTypeCount(); ++i) {
        const ResourceType *rt= tt->getResourceType(i);
        const Resource *r= aiInterface->getResource(rt);

		if( rt->getClass() != rcStatic && rt->getClass() != rcConsumable &&
			r->getAmount() < amount) {

			// Now MAKE SURE the unit has a harvest command for this resource
			// AND that the resource is within eye-sight to avoid units
			// standing around doing nothing.
			const HarvestCommandType *hct= unit->getType()->getFirstHarvestCommand(rt,unit->getFaction());

			Vec2i resPos;
			if(hct != NULL && aiInterface->getNearestSightedResource(rt, aiInterface->getHomeLocation(), resPos, false)) {
				amount= r->getAmount();
				neededResource= rt;
			}
        }
    }
    return neededResource;
}

bool Ai::beingAttacked(Vec2i &pos, Field &field, int radius){   
    int count= aiInterface->onSightUnitCount(); 
    const Unit *unit;

    for(int i=0; i<count; ++i){
        unit= aiInterface->getOnSightUnit(i);
        if(!aiInterface->isAlly(unit) && unit->isAlive()){
            pos= unit->getPos();
			field= unit->getCurrField();
            if(pos.dist(aiInterface->getHomeLocation())<radius){
                aiInterface->printLog(2, "Being attacked at pos "+intToStr(pos.x)+","+intToStr(pos.y)+"\n");
                return true; 
            }
        }
    }
    return false;
}

bool Ai::isStableBase(){

    if(getCountOfClass(ucWarrior)>minWarriors){
        aiInterface->printLog(4, "Base is stable\n");
        return true;          
    }
    else{
        aiInterface->printLog(4, "Base is not stable\n");
        return false;                
    }
}

bool Ai::findAbleUnit(int *unitIndex, CommandClass ability, bool idleOnly){
	vector<int> units;

	*unitIndex= -1;
	for(int i=0; i<aiInterface->getMyUnitCount(); ++i){
		const Unit *unit= aiInterface->getMyUnit(i); 
		if(unit->getType()->hasCommandClass(ability)){
			if(!idleOnly || !unit->anyCommand() || unit->getCurrCommand()->getCommandType()->getClass()==ccStop){
				units.push_back(i);
			}
		}   
	}

	if(units.empty()){
		return false;
	}
	else{
		*unitIndex= units[random.randRange(0, units.size()-1)];
		return true;
	}
}

bool Ai::findAbleUnit(int *unitIndex, CommandClass ability, CommandClass currentCommand){
	vector<int> units;

	*unitIndex= -1;
	for(int i=0; i<aiInterface->getMyUnitCount(); ++i){
		const Unit *unit= aiInterface->getMyUnit(i); 
		if(unit->getType()->hasCommandClass(ability)){
			if(unit->anyCommand() && unit->getCurrCommand()->getCommandType()->getClass()==currentCommand){
				units.push_back(i);
			}
		}   
	}

	if(units.empty()){
		return false;
	}
	else{
		*unitIndex= units[random.randRange(0, units.size()-1)];
		return true;
	}
}

bool Ai::findPosForBuilding(const UnitType* building, const Vec2i &searchPos, Vec2i &outPos){
    
	const int spacing= 1;

    for(int currRadius=0; currRadius<maxBuildRadius; ++currRadius){
        for(int i=searchPos.x-currRadius; i<searchPos.x+currRadius; ++i){
            for(int j=searchPos.y-currRadius; j<searchPos.y+currRadius; ++j){
                outPos= Vec2i(i, j);
                if(aiInterface->isFreeCells(outPos-Vec2i(spacing), building->getSize()+spacing*2, fLand)){
                    return true;
                }
            }
        }
    }

    return false;

}


// ==================== tasks ==================== 

void Ai::addTask(const Task *task){
	tasks.push_back(task);
	aiInterface->printLog(2, "Task added: " + task->toString());
}

void Ai::addPriorityTask(const Task *task){
	deleteValues(tasks.begin(), tasks.end());
	tasks.clear();

	tasks.push_back(task);
	aiInterface->printLog(2, "Priority Task added: " + task->toString());
}

bool Ai::anyTask(){
	return !tasks.empty();
}

const Task *Ai::getTask() const{
	if(tasks.empty()){
		return NULL;
	}
	else{
		return tasks.front();
	}
}

void Ai::removeTask(const Task *task){
	aiInterface->printLog(2, "Task removed: " + task->toString());
	tasks.remove(task);
	delete task;
}

void Ai::retryTask(const Task *task){
	tasks.remove(task);
	tasks.push_back(task);
}
// ==================== expansions ==================== 

void Ai::addExpansion(const Vec2i &pos){

	//check if there is a nearby expansion
	for(Positions::iterator it= expansionPositions.begin(); it!=expansionPositions.end(); ++it){
		if((*it).dist(pos)<villageRadius){
			return;
		}
	}

	//add expansion
	expansionPositions.push_front(pos);

	//remove expansion if queue is list is full
	if(expansionPositions.size()>maxExpansions){
		expansionPositions.pop_back();
	}
}

Vec2i Ai::getRandomHomePosition(){
	
	if(expansionPositions.empty() || random.randRange(0, 1) == 0){
		return aiInterface->getHomeLocation();
	}
	
	return expansionPositions[random.randRange(0, expansionPositions.size()-1)];
}

// ==================== actions ==================== 

void Ai::sendScoutPatrol(){
    Vec2i pos;
    int unit;

	startLoc= (startLoc+1) % aiInterface->getMapMaxPlayers();
	pos= aiInterface->getStartLocation(startLoc);

	if(aiInterface->getFactionIndex()!=startLoc){
		if(findAbleUnit(&unit, ccAttack, false)){
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			aiInterface->giveCommand(unit, ccAttack, pos);
			aiInterface->printLog(2, "Scout patrol sent to: " + intToStr(pos.x)+","+intToStr(pos.y)+"\n"); 
		}
	}

}

void Ai::massiveAttack(const Vec2i &pos, Field field, bool ultraAttack){
	int producerWarriorCount=0;
	int maxProducerWarriors=random.randRange(1,11);
	int unitCount = aiInterface->getMyUnitCount();

    for(int i = 0; i < unitCount; ++i) {
    	bool isWarrior=false;
        const Unit *unit= aiInterface->getMyUnit(i);
		const AttackCommandType *act= unit->getType()->getFirstAttackCommand(field);

		if(act != NULL && unit->getType()->hasCommandClass(ccProduce)) {
			producerWarriorCount++;
		}
		
		if(	aiInterface->getControlType() == ctCpuMega ||
			aiInterface->getControlType() == ctNetworkCpuMega) {
			if(producerWarriorCount > maxProducerWarriors) {
				if(
					unit->getCommandSize()>0 &&
					unit->getCurrCommand()->getCommandType()!=NULL && (
				    unit->getCurrCommand()->getCommandType()->getClass()==ccBuild || 
					unit->getCurrCommand()->getCommandType()->getClass()==ccMorph || 
					unit->getCurrCommand()->getCommandType()->getClass()==ccProduce
					)
				 ) {
					isWarrior=false;
				}
				else {
					isWarrior =! unit->getType()->hasCommandClass(ccHarvest);
				}

			}
			else {
				isWarrior= !unit->getType()->hasCommandClass(ccHarvest) && !unit->getType()->hasCommandClass(ccProduce);
			}
		}
		else {
			isWarrior= !unit->getType()->hasCommandClass(ccHarvest) && !unit->getType()->hasCommandClass(ccProduce);
		}
		
		bool alreadyAttacking= (unit->getCurrSkill()->getClass() == scAttack);
		if(!alreadyAttacking && act!=NULL && (ultraAttack || isWarrior)) {
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			aiInterface->giveCommand(i, act, pos);
		}
    }

    if(	aiInterface->getControlType() == ctCpuEasy ||
    	aiInterface->getControlType() == ctNetworkCpuEasy) {
		minWarriors+= 1;
	}
	else if(aiInterface->getControlType() == ctCpuMega ||
			aiInterface->getControlType() == ctNetworkCpuMega) {
		minWarriors+= 3;
		if(minWarriors>maxMinWarriors-1 || randomMinWarriorsReached) {
			randomMinWarriorsReached=true;
			minWarriors=random.randRange(maxMinWarriors-10, maxMinWarriors*2);
		}
	}
	else if(minWarriors<maxMinWarriors) {
		minWarriors+= 3;
	}
	aiInterface->printLog(2, "Massive attack to pos: "+ intToStr(pos.x)+", "+intToStr(pos.y)+"\n");
}

void Ai::returnBase(int unitIndex) {
    Vec2i pos;
    CommandResult r;
    int fi;

    fi= aiInterface->getFactionIndex();
    pos= Vec2i(
		random.randRange(-villageRadius, villageRadius), random.randRange(-villageRadius, villageRadius)) + 
		getRandomHomePosition();

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
    r= aiInterface->giveCommand(unitIndex, ccMove, pos);

    //aiInterface->printLog(1, "Order return to base pos:" + intToStr(pos.x)+", "+intToStr(pos.y)+": "+rrToStr(r)+"\n");
}

void Ai::harvest(int unitIndex) {
	const ResourceType *rt= getNeededResource(unitIndex);
	if(rt != NULL) {
		const HarvestCommandType *hct= aiInterface->getMyUnit(unitIndex)->getType()->getFirstHarvestCommand(rt,aiInterface->getMyUnit(unitIndex)->getFaction());

		Vec2i resPos;
		if(hct != NULL && aiInterface->getNearestSightedResource(rt, aiInterface->getHomeLocation(), resPos, false)) {
			resPos= resPos+Vec2i(random.randRange(-2, 2), random.randRange(-2, 2)); 
			aiInterface->giveCommand(unitIndex, hct, resPos);
			//aiInterface->printLog(4, "Order harvest pos:" + intToStr(resPos.x)+", "+intToStr(resPos.y)+": "+rrToStr(r)+"\n");
		}
	}
}

}}//end namespace
