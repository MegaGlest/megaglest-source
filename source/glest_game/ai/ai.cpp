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
#include "map.h"
#include "faction_type.h"
#include "leak_dumper.h"

using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest { namespace Game {

Task::Task() {
	taskClass = tcProduce;
}

//void Task::saveGame(XmlNode *rootNode) const {
//	std::map<string,string> mapTagReplacements;
//	XmlNode *taskNode = rootNode->addChild("Task");
//}

// =====================================================
// 	class ProduceTask
// =====================================================
ProduceTask::ProduceTask() : Task() {
	taskClass= tcProduce;
	unitType= NULL;
	resourceType= NULL;
	unitClass = ucWarrior;
}

ProduceTask::ProduceTask(UnitClass unitClass) : Task() {
	taskClass= tcProduce;
	this->unitClass= unitClass;
	unitType= NULL;
	resourceType= NULL;
}

ProduceTask::ProduceTask(const UnitType *unitType) : Task() {
	taskClass= tcProduce;
	this->unitType= unitType;
	resourceType= NULL;
	unitClass = ucWarrior;
}

ProduceTask::ProduceTask(const ResourceType *resourceType) : Task() {
	taskClass= tcProduce;
	unitType= NULL;
	unitClass = ucWarrior;
	this->resourceType= resourceType;
}

string ProduceTask::toString() const{
	string str= "Produce ";
	if(unitType!=NULL){
		str+= unitType->getName(false);
	}
	return str;
}

void ProduceTask::saveGame(XmlNode *rootNode) const {
	std::map<string,string> mapTagReplacements;
	XmlNode *taskNode = rootNode->addChild("Task");
	taskNode->addAttribute("taskClass",intToStr(taskClass), mapTagReplacements);
	XmlNode *produceTaskNode = taskNode->addChild("ProduceTask");

//	UnitClass unitClass;
	produceTaskNode->addAttribute("unitClass",intToStr(unitClass), mapTagReplacements);
//	const UnitType *unitType;
	if(unitType != NULL) {
		produceTaskNode->addAttribute("unitType",unitType->getName(false), mapTagReplacements);
	}
//	const ResourceType *resourceType;
	if(resourceType != NULL) {
		produceTaskNode->addAttribute("resourceType",resourceType->getName(false), mapTagReplacements);
	}
}

ProduceTask * ProduceTask::loadGame(const XmlNode *rootNode, Faction *faction) {
	const XmlNode *produceTaskNode = rootNode->getChild("ProduceTask");

	ProduceTask *newTask = new ProduceTask();
	//	UnitClass unitClass;
	newTask->unitClass = static_cast<UnitClass>(produceTaskNode->getAttribute("unitClass")->getIntValue());
	//	const UnitType *unitType;
	if(produceTaskNode->hasAttribute("unitType")) {
		string unitTypeName = produceTaskNode->getAttribute("unitType")->getValue();
		newTask->unitType = faction->getType()->getUnitType(unitTypeName);
	}
	//	const ResourceType *resourceType;
	if(produceTaskNode->hasAttribute("resourceType")) {
		string resourceTypeName = produceTaskNode->getAttribute("resourceType")->getValue();
		newTask->resourceType = faction->getTechTree()->getResourceType(resourceTypeName);
	}

	return newTask;
}

// =====================================================
// 	class BuildTask
// =====================================================
BuildTask::BuildTask() {
	taskClass= tcBuild;
	this->unitType= NULL;
	resourceType= NULL;
	forcePos= false;
}

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
		str+= unitType->getName(false);
	}
	return str;
}

void BuildTask::saveGame(XmlNode *rootNode) const {
	std::map<string,string> mapTagReplacements;
	XmlNode *taskNode = rootNode->addChild("Task");
	taskNode->addAttribute("taskClass",intToStr(taskClass), mapTagReplacements);
	XmlNode *buildTaskNode = taskNode->addChild("BuildTask");

//	const UnitType *unitType;
	if(unitType != NULL) {
		buildTaskNode->addAttribute("unitType",unitType->getName(false), mapTagReplacements);
	}
//	const ResourceType *resourceType;
	if(resourceType != NULL) {
		buildTaskNode->addAttribute("resourceType",resourceType->getName(), mapTagReplacements);
	}
//	bool forcePos;
	buildTaskNode->addAttribute("forcePos",intToStr(forcePos), mapTagReplacements);
//	Vec2i pos;
	buildTaskNode->addAttribute("pos",pos.getString(), mapTagReplacements);
}

BuildTask * BuildTask::loadGame(const XmlNode *rootNode, Faction *faction) {
	const XmlNode *buildTaskNode = rootNode->getChild("BuildTask");

	BuildTask *newTask = new BuildTask();
	if(buildTaskNode->hasAttribute("unitType")) {
		string unitTypeName = buildTaskNode->getAttribute("unitType")->getValue();
		newTask->unitType = faction->getType()->getUnitType(unitTypeName);
	}
	if(buildTaskNode->hasAttribute("resourceType")) {
		string resourceTypeName = buildTaskNode->getAttribute("resourceType")->getValue();
		newTask->resourceType = faction->getTechTree()->getResourceType(resourceTypeName);
	}

	newTask->forcePos = buildTaskNode->getAttribute("forcePos")->getIntValue() != 0;
	newTask->pos = Vec2i::strToVec2(buildTaskNode->getAttribute("pos")->getValue());

	return newTask;
}

// =====================================================
// 	class UpgradeTask
// =====================================================
UpgradeTask::UpgradeTask() {
	taskClass= tcUpgrade;
	this->upgradeType= NULL;
}

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

void UpgradeTask::saveGame(XmlNode *rootNode) const {
	std::map<string,string> mapTagReplacements;
	XmlNode *taskNode = rootNode->addChild("Task");
	taskNode->addAttribute("taskClass",intToStr(taskClass), mapTagReplacements);
	XmlNode *upgradeTaskNode = taskNode->addChild("UpgradeTask");

	if(upgradeType != NULL) {
		//upgradeType->saveGame(upgradeTaskNode);
		upgradeTaskNode->addAttribute("upgradeType",upgradeType->getName(), mapTagReplacements);
	}
}

UpgradeTask * UpgradeTask::loadGame(const XmlNode *rootNode, Faction *faction) {
	const XmlNode *upgradeTaskNode = rootNode->getChild("UpgradeTask");

	UpgradeTask *newTask = new UpgradeTask();
	if(upgradeTaskNode->hasAttribute("upgradeType")) {
		string upgradeTypeName = upgradeTaskNode->getAttribute("upgradeType")->getValue();
		newTask->upgradeType = faction->getType()->getUpgradeType(upgradeTypeName);
	}
	return newTask;
}

// =====================================================
// 	class Ai
// =====================================================

void Ai::init(AiInterface *aiInterface, int useStartLocation) {
	this->aiInterface= aiInterface;

	Faction *faction = this->aiInterface->getMyFaction();
	if(faction->getAIBehaviorStaticOverideValue(aibsvcMaxBuildRadius) != INT_MAX) {
		maxBuildRadius = faction->getAIBehaviorStaticOverideValue(aibsvcMaxBuildRadius);
		//printf("Discovered overriden static value for AI, maxBuildRadius = %d\n",maxBuildRadius);
	}
	if(faction->getAIBehaviorStaticOverideValue(aibsvcMinMinWarriors) != INT_MAX) {
		minMinWarriors = faction->getAIBehaviorStaticOverideValue(aibsvcMinMinWarriors);
		//printf("Discovered overriden static value for AI, minMinWarriors = %d\n",minMinWarriors);
	}
	if(faction->getAIBehaviorStaticOverideValue(aibsvcMinMinWarriorsExpandCpuEasy) != INT_MAX) {
		minMinWarriorsExpandCpuEasy = faction->getAIBehaviorStaticOverideValue(aibsvcMinMinWarriorsExpandCpuEasy);
		//printf("Discovered overriden static value for AI, minMinWarriorsExpandCpuEasy = %d\n",minMinWarriorsExpandCpuEasy);
	}
	if(faction->getAIBehaviorStaticOverideValue(aibsvcMinMinWarriorsExpandCpuMega) != INT_MAX) {
		minMinWarriorsExpandCpuMega = faction->getAIBehaviorStaticOverideValue(aibsvcMinMinWarriorsExpandCpuMega);
		//printf("Discovered overriden static value for AI, minMinWarriorsExpandCpuMega = %d\n",minMinWarriorsExpandCpuMega);
	}
	if(faction->getAIBehaviorStaticOverideValue(aibsvcMinMinWarriorsExpandCpuUltra) != INT_MAX) {
		minMinWarriorsExpandCpuUltra = faction->getAIBehaviorStaticOverideValue(aibsvcMinMinWarriorsExpandCpuUltra);
		//printf("Discovered overriden static value for AI, minMinWarriorsExpandCpuUltra = %d\n",minMinWarriorsExpandCpuUltra);
	}
	if(faction->getAIBehaviorStaticOverideValue(aibsvcMinMinWarriorsExpandCpuNormal) != INT_MAX) {
		minMinWarriorsExpandCpuNormal = faction->getAIBehaviorStaticOverideValue(aibsvcMinMinWarriorsExpandCpuNormal);
		//printf("Discovered overriden static value for AI, minMinWarriorsExpandCpuNormal = %d\n",minMinWarriorsExpandCpuNormal);
	}
	if(faction->getAIBehaviorStaticOverideValue(aibsvcMaxMinWarriors) != INT_MAX) {
		maxMinWarriors = faction->getAIBehaviorStaticOverideValue(aibsvcMaxMinWarriors);
		//printf("Discovered overriden static value for AI, maxMinWarriors = %d\n",maxMinWarriors);
	}
	if(faction->getAIBehaviorStaticOverideValue(aibsvcMaxExpansions) != INT_MAX) {
		maxExpansions = faction->getAIBehaviorStaticOverideValue(aibsvcMaxExpansions);
		//printf("Discovered overriden static value for AI, maxExpansions = %d\n",maxExpansions);
	}
	if(faction->getAIBehaviorStaticOverideValue(aibsvcVillageRadius) != INT_MAX) {
		villageRadius = faction->getAIBehaviorStaticOverideValue(aibsvcVillageRadius);
		//printf("Discovered overriden static value for AI, villageRadius = %d\n",villageRadius);
	}
	if(faction->getAIBehaviorStaticOverideValue(aibsvcScoutResourceRange) != INT_MAX) {
		scoutResourceRange = faction->getAIBehaviorStaticOverideValue(aibsvcScoutResourceRange);
		//printf("Discovered overriden static value for AI, scoutResourceRange = %d\n",scoutResourceRange);
	}
	if(faction->getAIBehaviorStaticOverideValue(aibsvcMinWorkerAttackersHarvesting) != INT_MAX) {
		minWorkerAttackersHarvesting = faction->getAIBehaviorStaticOverideValue(aibsvcMinWorkerAttackersHarvesting);
		//printf("Discovered overriden static value for AI, scoutResourceRange = %d\n",scoutResourceRange);
	}
	if(faction->getAIBehaviorStaticOverideValue(aibsvcMinBuildSpacing) != INT_MAX) {
		minBuildSpacing = faction->getAIBehaviorStaticOverideValue(aibsvcMinBuildSpacing);
		//printf("Discovered overriden static value for AI, scoutResourceRange = %d\n",scoutResourceRange);
	}

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
	aiRules.push_back(new AiRuleUnBlock(this));
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
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] deleting AI aiInterface [%p]\n",__FILE__,__FUNCTION__,__LINE__,aiInterface);
	deleteValues(tasks.begin(), tasks.end());
	tasks.clear();
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] deleting AI aiInterface [%p]\n",__FILE__,__FUNCTION__,__LINE__,aiInterface);

	deleteValues(aiRules.begin(), aiRules.end());
	aiRules.clear();
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] deleting AI aiInterface [%p]\n",__FILE__,__FUNCTION__,__LINE__,aiInterface);

	aiInterface = NULL;
}

RandomGen* Ai::getRandom() {
//	if(Thread::isCurrentThreadMainThread() == false) {
//		throw megaglest_runtime_error("Invalid access to AI random from outside main thread current id = " +
//				intToStr(Thread::getCurrentThreadId()) + " main = " + intToStr(Thread::getMainThreadId()));
//	}
	return &random;
}

void Ai::update() {

	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [START]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	if(aiInterface->getMyFaction()->getFirstSwitchTeamVote() != NULL) {
		const SwitchTeamVote *vote = aiInterface->getMyFaction()->getFirstSwitchTeamVote();
		aiInterface->getMyFaction()->setCurrentSwitchTeamVoteFactionIndex(vote->factionIndex);

		factionSwitchTeamRequestCount[vote->factionIndex]++;
		int factionSwitchTeamRequestCountCurrent = factionSwitchTeamRequestCount[vote->factionIndex];

		//int allowJoinTeam = random.randRange(0, 100);
		//srand(time(NULL) + aiInterface->getMyFaction()->getIndex());
		Chrono seed(true);
		srand((unsigned int)seed.getCurTicks() + aiInterface->getMyFaction()->getIndex());

		int allowJoinTeam = rand() % 100;

		SwitchTeamVote *voteResult = aiInterface->getMyFaction()->getSwitchTeamVote(vote->factionIndex);
		voteResult->voted = true;
		voteResult->allowSwitchTeam = false;

		const GameSettings *settings =  aiInterface->getWorld()->getGameSettings();

		// If AI player already lost game they cannot vote
		if(aiInterface->getWorld()->factionLostGame(aiInterface->getFactionIndex()) == true) {
			voteResult->allowSwitchTeam = true;
		}
		else {
			// Can only ask the AI player 2 times max per game
			if(factionSwitchTeamRequestCountCurrent <= 2) {
				// x% chance the AI will answer yes
				if(settings->getAiAcceptSwitchTeamPercentChance() >= 100) {
					voteResult->allowSwitchTeam = true;
				}
				else if(settings->getAiAcceptSwitchTeamPercentChance() <= 0) {
					voteResult->allowSwitchTeam = false;
				}
				else {
					voteResult->allowSwitchTeam = (allowJoinTeam >= (100 - settings->getAiAcceptSwitchTeamPercentChance()));
				}
			}
		}

		char szBuf[8096]="";
		snprintf(szBuf,8096,"AI for faction# %d voted %s [%d] CountCurrent [%d] PercentChance [%d]",aiInterface->getMyFaction()->getIndex(),(voteResult->allowSwitchTeam ? "Yes" : "No"),allowJoinTeam,factionSwitchTeamRequestCountCurrent,settings->getAiAcceptSwitchTeamPercentChance());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] %s\n",__FILE__,__FUNCTION__,__LINE__,szBuf);

		aiInterface->printLog(3, szBuf);

		aiInterface->giveCommandSwitchTeamVote(aiInterface->getMyFaction(),voteResult);
	}

	//process ai rules
	for(unsigned int ruleIdx = 0; ruleIdx < aiRules.size(); ++ruleIdx) {
		AiRule *rule = aiRules[ruleIdx];
		if(rule == NULL) {
			throw megaglest_runtime_error("rule == NULL");
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [ruleIdx = %d]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis(),ruleIdx);

		if((aiInterface->getTimer() % (rule->getTestInterval() * GameConstants::updateFps / 1000)) == 0) {

			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [ruleIdx = %d, before rule->test()]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis(),ruleIdx);

			//printf("Testing AI Faction # %d RULE Name[%s]\n",aiInterface->getFactionIndex(),rule->getName().c_str());

			if(rule->test()) {
				if(outputAIBehaviourToConsole()) printf("\n\nYYYYY Executing AI Faction # %d RULE Name[%s]\n\n",aiInterface->getFactionIndex(),rule->getName().c_str());

				aiInterface->printLog(3, intToStr(1000 * aiInterface->getTimer() / GameConstants::updateFps) + ": Executing rule: " + rule->getName() + '\n');

				if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [ruleIdx = %d, before rule->execute() [%s]]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis(),ruleIdx,rule->getName().c_str());

				rule->execute();

				if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [ruleIdx = %d, after rule->execute() [%s]]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis(),ruleIdx,rule->getName().c_str());
			}
		}
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [END]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
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

int Ai::getCountOfClass(UnitClass uc,UnitClass *additionalUnitClassToExcludeFromCount) {
    int count= 0;
    for(int i = 0; i < aiInterface->getMyUnitCount(); ++i) {
		if(aiInterface->getMyUnit(i)->getType()->isOfClass(uc)) {
			// Skip unit if it ALSO contains the exclusion unit class type
			if(additionalUnitClassToExcludeFromCount != NULL) {
				if(aiInterface->getMyUnit(i)->getType()->isOfClass(*additionalUnitClassToExcludeFromCount)) {
					continue;
				}
			}
            ++count;
		}
    }
    return count;
}

float Ai::getRatioOfClass(UnitClass uc,UnitClass *additionalUnitClassToExcludeFromCount) {
	if(aiInterface->getMyUnitCount() == 0) {
		return 0;
	}
	else {
		//return static_cast<float>(getCountOfClass(uc,additionalUnitClassToExcludeFromCount)) / aiInterface->getMyUnitCount();
		return truncateDecimal<float>(static_cast<float>(getCountOfClass(uc,additionalUnitClassToExcludeFromCount)) / aiInterface->getMyUnitCount(),6);
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

		if( rt->getClass() != rcStatic && rt->getClass() != rcConsumable) {
			char szBuf[8096]="";
			snprintf(szBuf,8096,"Examining resource [%s] amount [%d] (previous amount [%d]",rt->getName().c_str(),r->getAmount(),amount);
			aiInterface->printLog(3, szBuf);
		}

		if( rt->getClass() != rcStatic && rt->getClass() != rcConsumable &&
			r->getAmount() < amount) {

			// Only have up to x units going for this resource so we can focus
			// on other needed resources for other units
			const int maxUnitsToHarvestResource = 5;

			vector<int> unitsGettingResource = findUnitsHarvestingResourceType(rt);
			if((int)unitsGettingResource.size() <= maxUnitsToHarvestResource) {
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
    }

    char szBuf[8096]="";
    snprintf(szBuf,8096,"Unit [%d - %s] looking for resources (not static or consumable)",unit->getId(),unit->getType()->getName(false).c_str());
    aiInterface->printLog(3, szBuf);
    snprintf(szBuf,8096,"[resource type count %d] Needed resource [%s].",tt->getResourceTypeCount(),(neededResource != NULL ? neededResource->getName().c_str() : "<none>"));
    aiInterface->printLog(3, szBuf);

    return neededResource;
}

bool Ai::beingAttacked(Vec2i &pos, Field &field, int radius){
	const Unit *enemy = aiInterface->getFirstOnSightEnemyUnit(pos, field, radius);
	return (enemy != NULL);
}

bool Ai::isStableBase() {
	UnitClass ucWorkerType = ucWorker;
    if(getCountOfClass(ucWarrior,&ucWorkerType) > minWarriors) {
        char szBuf[8096]="";
        snprintf(szBuf,8096,"Base is stable [minWarriors = %d found = %d]",minWarriors,ucWorkerType);
        aiInterface->printLog(4, szBuf);

        return true;
    }
    else{
        char szBuf[8096]="";
        snprintf(szBuf,8096,"Base is NOT stable [minWarriors = %d found = %d]",minWarriors,ucWorkerType);
        aiInterface->printLog(4, szBuf);

        return false;
    }
}

bool Ai::findAbleUnit(int *unitIndex, CommandClass ability, bool idleOnly){
	vector<int> units;

	*unitIndex= -1;
	for(int i=0; i<aiInterface->getMyUnitCount(); ++i){
		const Unit *unit= aiInterface->getMyUnit(i);
		if(unit->getType()->isCommandable() && unit->getType()->hasCommandClass(ability)){
			if(!idleOnly || !unit->anyCommand() || unit->getCurrCommand()->getCommandType()->getClass()==ccStop){
				units.push_back(i);
			}
		}
	}

	if(units.empty()){
		return false;
	}
	else{
		*unitIndex= units[random.randRange(0, (int)units.size()-1)];
		return true;
	}
}

vector<int> Ai::findUnitsHarvestingResourceType(const ResourceType *rt) {
	vector<int> units;

	Map *map= aiInterface->getMap();
	for(int i = 0; i < aiInterface->getMyUnitCount(); ++i) {
		const Unit *unit= aiInterface->getMyUnit(i);
		if(unit->getType()->isCommandable()) {
			if(unit->getType()->hasCommandClass(ccHarvest)) {
				if(unit->anyCommand() && unit->getCurrCommand()->getCommandType()->getClass() == ccHarvest) {
					Command *command= unit->getCurrCommand();
					const HarvestCommandType *hct= dynamic_cast<const HarvestCommandType*>(command->getCommandType());
					if(hct != NULL) {
						const Vec2i unitTargetPos = unit->getTargetPos();
						SurfaceCell *sc= map->getSurfaceCell(Map::toSurfCoords(unitTargetPos));
						Resource *r= sc->getResource();
						if (r != NULL && r->getType() == rt) {
							units.push_back(i);
						}
					}
				}
			}
			else if(unit->getType()->hasCommandClass(ccProduce)) {
				if(unit->anyCommand() && unit->getCurrCommand()->getCommandType()->getClass() == ccProduce) {
					Command *command= unit->getCurrCommand();
					const ProduceCommandType *pct= dynamic_cast<const ProduceCommandType*>(command->getCommandType());
					if(pct != NULL) {
						const UnitType *ut = pct->getProducedUnit();
						if(ut != NULL) {
							const Resource *r = ut->getCost(rt);
							if(r != NULL) {
								if (r != NULL && r->getAmount() < 0) {
									units.push_back(i);
								}
							}
						}
					}
				}
			}
			else if(unit->getType()->hasCommandClass(ccBuild)) {
				if(unit->anyCommand() && unit->getCurrCommand()->getCommandType()->getClass() == ccBuild) {
					Command *command= unit->getCurrCommand();
					const BuildCommandType *bct= dynamic_cast<const BuildCommandType*>(command->getCommandType());
					if(bct != NULL) {
						for(int j = 0; j < bct->getBuildingCount(); ++j) {
							const UnitType *ut = bct->getBuilding(j);
							if(ut != NULL) {
								const Resource *r = ut->getCost(rt);
								if(r != NULL) {
									if (r != NULL && r->getAmount() < 0) {
										units.push_back(i);
										break;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	return units;
}

//vector<int> Ai::findUnitsDoingCommand(CommandClass currentCommand) {
//	vector<int> units;
//
//	for(int i = 0; i < aiInterface->getMyUnitCount(); ++i) {
//		const Unit *unit= aiInterface->getMyUnit(i);
//		if(unit->getType()->isCommandable() && unit->getType()->hasCommandClass(currentCommand)) {
//			if(unit->anyCommand() && unit->getCurrCommand()->getCommandType()->getClass() == currentCommand) {
//				units.push_back(i);
//			}
//		}
//	}
//
//	return units;
//}

bool Ai::findAbleUnit(int *unitIndex, CommandClass ability, CommandClass currentCommand){
	vector<int> units;

	*unitIndex= -1;
	for(int i=0; i<aiInterface->getMyUnitCount(); ++i){
		const Unit *unit= aiInterface->getMyUnit(i);
		if(unit->getType()->isCommandable() && unit->getType()->hasCommandClass(ability)){
			if(unit->anyCommand() && unit->getCurrCommand()->getCommandType()->getClass()==currentCommand){
				units.push_back(i);
			}
		}
	}

	if(units.empty()){
		return false;
	}
	else{
		*unitIndex= units[random.randRange(0, (int)units.size()-1)];
		return true;
	}
}

bool Ai::findPosForBuilding(const UnitType* building, const Vec2i &searchPos, Vec2i &outPos){

    for(int currRadius = 0; currRadius < maxBuildRadius; ++currRadius) {
        for(int i=searchPos.x - currRadius; i < searchPos.x + currRadius; ++i) {
            for(int j=searchPos.y - currRadius; j < searchPos.y + currRadius; ++j) {
                outPos= Vec2i(i, j);
                if(aiInterface->isFreeCells(outPos - Vec2i(minBuildSpacing), building->getAiBuildSize() + minBuildSpacing * 2, fLand)) {
                	int aiBuildSizeDiff= building->getAiBuildSize()- building->getSize();
                	if( aiBuildSizeDiff>0){
                		int halfSize=aiBuildSizeDiff/2;
                		outPos.x+=halfSize;
                		outPos.y+=halfSize;
                	}
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

void Ai::addExpansion(const Vec2i &pos) {

	//check if there is a nearby expansion
	for(Positions::iterator it = expansionPositions.begin(); it != expansionPositions.end(); ++it) {
		if((*it).dist(pos) < villageRadius) {
			return;
		}
	}

	//add expansion
	expansionPositions.push_front(pos);

	//remove expansion if queue is list is full
	if((int)expansionPositions.size() > maxExpansions){
		expansionPositions.pop_back();
	}
}

Vec2i Ai::getRandomHomePosition() {

	if(expansionPositions.empty() || random.randRange(0, 1) == 0){
		return aiInterface->getHomeLocation();
	}

	return expansionPositions[random.randRange(0, (int)expansionPositions.size()-1)];
}

// ==================== actions ====================

void Ai::sendScoutPatrol(){

	Vec2i pos;
	int unit;
	bool possibleTargetFound= false;

	bool ultraResourceAttack= (aiInterface->getControlType() == ctCpuUltra || aiInterface->getControlType() == ctNetworkCpuUltra)
	        && random.randRange(0, 2) == 1;
	bool megaResourceAttack=(aiInterface->getControlType() == ctCpuMega || aiInterface->getControlType() == ctNetworkCpuMega)
			&& random.randRange(0, 1) == 1;

	if(megaResourceAttack || ultraResourceAttack) {
		Map *map= aiInterface->getMap();

		const TechTree *tt= aiInterface->getTechTree();
		const ResourceType *rt= tt->getResourceType(0);
		int tryCount= 0;
		int height= map->getH();
		int width= map->getW();

		for(int i= 0; i < tt->getResourceTypeCount(); ++i){
			const ResourceType *rt_= tt->getResourceType(i);
			//const Resource *r= aiInterface->getResource(rt);

			if(rt_->getClass() == rcTech){
				rt=rt_;
				break;
			}
		}
		//printf("looking for resource %s\n",rt->getName().c_str());
		while(possibleTargetFound == false){
			tryCount++;
			if(tryCount == 4){
				//printf("no target found\n");
				break;
			}
			pos= Vec2i(random.randRange(2, width - 2), random.randRange(2, height - 2));
			if(map->isInside(pos) && map->isInsideSurface(map->toSurfCoords(pos))){
				//printf("is inside map\n");
				// find first resource in this area
				Vec2i resPos;
				if(aiInterface->isResourceInRegion(pos, rt, resPos, scoutResourceRange)){
					// found a possible target.
					pos= resPos;
					//printf("lets try the new target\n");
					possibleTargetFound= true;
					break;
				}
			}
			//else printf("is outside map\n");
		}
	}

	std::vector<Vec2i> warningEnemyList = aiInterface->getEnemyWarningPositionList();
	if( (possibleTargetFound == false) && (warningEnemyList.empty() == false)) {
		//for(int i = (int)warningEnemyList.size() - 1; i <= 0; --i) {
			//Vec2i &checkPos = warningEnemyList[i];
			Vec2i &checkPos = warningEnemyList[0];
			if (random.randRange(0, 1) == 1 ) {
				pos = checkPos;
				possibleTargetFound = true;
				warningEnemyList.clear();
			} else {
				aiInterface->removeEnemyWarningPositionFromList(checkPos);
			}
			//break;
		//}
	}

	if(possibleTargetFound == false){
		startLoc= (startLoc + 1) % aiInterface->getMapMaxPlayers();
		pos= aiInterface->getStartLocation(startLoc);
		//printf("normal target used\n");
	}

	if(aiInterface->getHomeLocation() != pos){
		if(findAbleUnit(&unit, ccAttack, false)){
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			aiInterface->giveCommand(unit, ccAttack, pos);
			aiInterface->printLog(2, "Scout patrol sent to: " + intToStr(pos.x) + "," + intToStr(pos.y) + "\n");
		}
	}
}

void Ai::massiveAttack(const Vec2i &pos, Field field, bool ultraAttack){
	int producerWarriorCount=0;
	int maxProducerWarriors=random.randRange(1,11);
	int unitCount = aiInterface->getMyUnitCount();
	int unitGroupCommandId = -1;

	int attackerWorkersHarvestingCount = 0;
    for(int i = 0; i < unitCount; ++i) {
    	bool isWarrior=false;
    	bool productionInProgress=false;
        const Unit *unit= aiInterface->getMyUnit(i);
		const AttackCommandType *act= unit->getType()->getFirstAttackCommand(field);

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
					productionInProgress=true;
					isWarrior=false;
					producerWarriorCount++;
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

		bool unitSignalledToAttack = false;
		if(alreadyAttacking == false && unit->getType()->hasSkillClass(scAttack) && (aiInterface->getControlType()
		        == ctCpuUltra || aiInterface->getControlType() == ctCpuMega || aiInterface->getControlType()
		        == ctNetworkCpuUltra || aiInterface->getControlType() == ctNetworkCpuMega)){
			//printf("~~~~~~~~ Unit [%s - %d] checking if unit is being attacked\n",unit->getFullName().c_str(),unit->getId());

			std::pair<bool, Unit *> beingAttacked= aiInterface->getWorld()->getUnitUpdater()->unitBeingAttacked(unit);
			if(beingAttacked.first == true){
				Unit *enemy= beingAttacked.second;
				const AttackCommandType *act_forenemy= unit->getType()->getFirstAttackCommand(enemy->getCurrField());

				//printf("~~~~~~~~ Unit [%s - %d] attacked by enemy [%s - %d] act_forenemy [%p] enemy->getCurrField() = %d\n",unit->getFullName().c_str(),unit->getId(),enemy->getFullName().c_str(),enemy->getId(),act_forenemy,enemy->getCurrField());

				if(act_forenemy != NULL){
					bool shouldAttack= true;
					if(unit->getType()->hasSkillClass(scHarvest)){
						shouldAttack= (attackerWorkersHarvestingCount > minWorkerAttackersHarvesting);
						if(shouldAttack == false){
							attackerWorkersHarvestingCount++;
						}
					}
					if(shouldAttack) {
						if(unitGroupCommandId == -1) {
							unitGroupCommandId = aiInterface->getWorld()->getNextCommandGroupId();
						}

						aiInterface->giveCommand(i, act_forenemy, beingAttacked.second->getPos(), unitGroupCommandId);
						unitSignalledToAttack= true;
					}
				}
				else{
					const AttackStoppedCommandType *asct_forenemy= unit->getType()->getFirstAttackStoppedCommand(
					        enemy->getCurrField());
					//printf("~~~~~~~~ Unit [%s - %d] found enemy [%s - %d] asct_forenemy [%p] enemy->getCurrField() = %d\n",unit->getFullName().c_str(),unit->getId(),enemy->getFullName().c_str(),enemy->getId(),asct_forenemy,enemy->getCurrField());
					if(asct_forenemy != NULL){
						bool shouldAttack= true;
						if(unit->getType()->hasSkillClass(scHarvest)){
							shouldAttack= (attackerWorkersHarvestingCount > minWorkerAttackersHarvesting);
							if(shouldAttack == false){
								attackerWorkersHarvestingCount++;
							}
						}
						if(shouldAttack){
//								printf("~~~~~~~~ Unit [%s - %d] WILL AttackStoppedCommand [%s - %d]\n", unit->getFullName().c_str(),
//								        unit->getId(), enemy->getFullName().c_str(), enemy->getId());

							if(unitGroupCommandId == -1) {
								unitGroupCommandId = aiInterface->getWorld()->getNextCommandGroupId();
							}

							aiInterface->giveCommand(i, asct_forenemy, beingAttacked.second->getCenteredPos(), unitGroupCommandId);
							unitSignalledToAttack= true;
						}
					}
				}
			}
		}
		if(alreadyAttacking == false && act != NULL && (ultraAttack || isWarrior) &&
			unitSignalledToAttack == false) {
			bool shouldAttack = true;
			if(unit->getType()->hasSkillClass(scHarvest)) {
				shouldAttack = (attackerWorkersHarvestingCount > minWorkerAttackersHarvesting);
				if(shouldAttack == false) {
					attackerWorkersHarvestingCount++;
				}
			}

			// Mega CPU does not send ( far away ) units which are currently producing something
			if(aiInterface->getControlType() == ctCpuMega || aiInterface->getControlType() == ctNetworkCpuMega){
				if(!isWarrior ){
					if(!productionInProgress){
						shouldAttack= false;
						//printf("no attack \n ");
					}
				}
			}
			if(shouldAttack) {
				if(unitGroupCommandId == -1) {
					unitGroupCommandId = aiInterface->getWorld()->getNextCommandGroupId();
				}

				aiInterface->giveCommand(i, act, pos, unitGroupCommandId);
			}
		}
    }

    if(	aiInterface->getControlType() == ctCpuEasy ||
    	aiInterface->getControlType() == ctNetworkCpuEasy) {
		minWarriors += minMinWarriorsExpandCpuEasy;
	}
	else if(aiInterface->getControlType() == ctCpuMega ||
			aiInterface->getControlType() == ctNetworkCpuMega) {
		minWarriors += minMinWarriorsExpandCpuMega;
		if(minWarriors > maxMinWarriors-1 || randomMinWarriorsReached) {
			randomMinWarriorsReached=true;
			minWarriors=random.randRange(maxMinWarriors-10, maxMinWarriors*2);
		}
	}
	else if(minWarriors < maxMinWarriors) {
	    if(aiInterface->getControlType() == ctCpuUltra ||
	    	aiInterface->getControlType() == ctNetworkCpuUltra) {
    		minWarriors += minMinWarriorsExpandCpuUltra;
		}
	    else {
	    	minWarriors+= minMinWarriorsExpandCpuNormal;
	    }
	}
	aiInterface->printLog(2, "Massive attack to pos: "+ intToStr(pos.x)+", "+intToStr(pos.y)+"\n");
}

void Ai::returnBase(int unitIndex) {
    Vec2i pos;
    //std::pair<CommandResult,string> r(crFailUndefined,"");
    //aiInterface->getFactionIndex();
    pos= Vec2i(
		random.randRange(-villageRadius, villageRadius),
		random.randRange(-villageRadius, villageRadius)) +
		                 getRandomHomePosition();

    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
    //r= aiInterface->giveCommand(unitIndex, ccMove, pos);
    aiInterface->giveCommand(unitIndex, ccMove, pos);

    //aiInterface->printLog(1, "Order return to base pos:" + intToStr(pos.x)+", "+intToStr(pos.y)+": "+rrToStr(r)+"\n");
}

void Ai::harvest(int unitIndex) {
	const ResourceType *rt= getNeededResource(unitIndex);
	if(rt != NULL) {
		const HarvestCommandType *hct= aiInterface->getMyUnit(unitIndex)->getType()->getFirstHarvestCommand(rt,aiInterface->getMyUnit(unitIndex)->getFaction());

		Vec2i resPos;
		if(hct != NULL && aiInterface->getNearestSightedResource(rt, aiInterface->getHomeLocation(), resPos, false)) {
			resPos= resPos+Vec2i(random.randRange(-2, 2), random.randRange(-2, 2));
			aiInterface->giveCommand(unitIndex, hct, resPos, -1);
			//aiInterface->printLog(4, "Order harvest pos:" + intToStr(resPos.x)+", "+intToStr(resPos.y)+": "+rrToStr(r)+"\n");
		}
	}
}

bool Ai::haveBlockedUnits() {
	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [START]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	int unitCount = aiInterface->getMyUnitCount();
	Map *map = aiInterface->getMap();
	//If there is no close store
	for(int j=0; j < unitCount; ++j) {
		const Unit *u= aiInterface->getMyUnit(j);
		const UnitType *ut= u->getType();

		// If this building is a store
		if(u->isAlive() && ut->isMobile() && u->getPath() != NULL && (u->getPath()->isBlocked() || u->getPath()->getBlockCount())) {
			Vec2i unitPos = u->getPosNotThreadSafe();

			//printf("#1 AI found blocked unit [%d - %s]\n",u->getId(),u->getFullName().c_str());

			int failureCount = 0;
			int cellCount = 0;

			for(int i = -1; i <= 1; ++i) {
				for(int j = -1; j <= 1; ++j) {
					Vec2i pos = unitPos + Vec2i(i, j);
					if(map->isInside(pos) && map->isInsideSurface(map->toSurfCoords(pos))) {
						if(pos != unitPos) {
							bool canUnitMoveToCell = map->aproxCanMove(u, unitPos, pos);
							if(canUnitMoveToCell == false) {
								failureCount++;
							}
							cellCount++;
						}
					}
				}
			}
			bool unitImmediatelyBlocked = (failureCount == cellCount);
			//printf("#1 unitImmediatelyBlocked = %d, failureCount = %d, cellCount = %d\n",unitImmediatelyBlocked,failureCount,cellCount);

			if(unitImmediatelyBlocked) {
				//printf("#1 AI unit IS BLOCKED [%d - %s]\n",u->getId(),u->getFullName().c_str());
				if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [START]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
				return true;
			}
		}
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [START]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
	return false;
}

bool Ai::getAdjacentUnits(std::map<float, std::map<int, const Unit *> > &signalAdjacentUnits, const Unit *unit) {
	//printf("In getAdjacentUnits...\n");

	bool result = false;
	Map *map = aiInterface->getMap();
	Vec2i unitPos = unit->getPosNotThreadSafe();
	for(int i = -1; i <= 1; ++i) {
		for(int j = -1; j <= 1; ++j) {
			Vec2i pos = unitPos + Vec2i(i, j);
			if(map->isInside(pos) && map->isInsideSurface(map->toSurfCoords(pos))) {
				if(pos != unitPos) {
					Unit *adjacentUnit = map->getCell(pos)->getUnit(unit->getCurrField());
					if(adjacentUnit != NULL && adjacentUnit->getFactionIndex() == unit->getFactionIndex()) {
						if(adjacentUnit->getType()->isMobile() && adjacentUnit->getPath() != NULL) {
							//signalAdjacentUnits.push_back(adjacentUnit);
							float dist = unitPos.dist(adjacentUnit->getPos());

							std::map<float, std::map<int, const Unit *> >::iterator iterFind1 = signalAdjacentUnits.find(dist);
							if(iterFind1 == signalAdjacentUnits.end()) {
								signalAdjacentUnits[dist][adjacentUnit->getId()] = adjacentUnit;

								getAdjacentUnits(signalAdjacentUnits, adjacentUnit);
								result = true;
							}
							else {
								std::map<int, const Unit *>::iterator iterFind2 = iterFind1->second.find(adjacentUnit->getId());
								if(iterFind2 == iterFind1->second.end()) {
									signalAdjacentUnits[dist][adjacentUnit->getId()] = adjacentUnit;
									getAdjacentUnits(signalAdjacentUnits, adjacentUnit);
									result = true;
								}
							}
						}
					}
				}
			}
		}
	}
	return result;
}

void Ai::unblockUnits() {
	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [START]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	int unitCount = aiInterface->getMyUnitCount();
	Map *map = aiInterface->getMap();
	// Find blocked units and move surrounding units out of the way
	std::map<float, std::map<int, const Unit *> > signalAdjacentUnits;
	for(int idx=0; idx < unitCount; ++idx) {
		const Unit *u= aiInterface->getMyUnit(idx);
		const UnitType *ut= u->getType();

		// If this building is a store
		if(u->isAlive() && ut->isMobile() && u->getPath() != NULL && (u->getPath()->isBlocked() || u->getPath()->getBlockCount())) {
			Vec2i unitPos = u->getPosNotThreadSafe();

			//printf("#2 AI found blocked unit [%d - %s]\n",u->getId(),u->getFullName().c_str());

			//int failureCount = 0;
			//int cellCount = 0;

			for(int i = -1; i <= 1; ++i) {
				for(int j = -1; j <= 1; ++j) {
					Vec2i pos = unitPos + Vec2i(i, j);
					if(map->isInside(pos) && map->isInsideSurface(map->toSurfCoords(pos))) {
						if(pos != unitPos) {
							bool canUnitMoveToCell = map->aproxCanMove(u, unitPos, pos);
							if(canUnitMoveToCell == false) {
								//failureCount++;
								getAdjacentUnits(signalAdjacentUnits, u);
							}
							//cellCount++;
						}
					}
				}
			}
			//bool unitImmediatelyBlocked = (failureCount == cellCount);
			//printf("#2 unitImmediatelyBlocked = %d, failureCount = %d, cellCount = %d, signalAdjacentUnits.size() = %d\n",unitImmediatelyBlocked,failureCount,cellCount,signalAdjacentUnits.size());
		}
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [START]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	if(signalAdjacentUnits.empty() == false) {
		//printf("#2 AI units ARE BLOCKED about to unblock\n");

		int unitGroupCommandId = -1;

		for(std::map<float, std::map<int, const Unit *> >::reverse_iterator iterMap = signalAdjacentUnits.rbegin();
			iterMap != signalAdjacentUnits.rend(); ++iterMap) {

			for(std::map<int, const Unit *>::iterator iterMap2 = iterMap->second.begin();
				iterMap2 != iterMap->second.end(); ++iterMap2) {
				//int idx = iterMap2->first;
				const Unit *adjacentUnit = iterMap2->second;
				if(adjacentUnit != NULL && adjacentUnit->getType()->getFirstCtOfClass(ccMove) != NULL) {
					const CommandType *ct = adjacentUnit->getType()->getFirstCtOfClass(ccMove);

					for(int moveAttempt = 1; moveAttempt <= villageRadius; ++moveAttempt) {
						Vec2i pos= Vec2i(
							random.randRange(-villageRadius*2, villageRadius*2),
							random.randRange(-villageRadius*2, villageRadius*2)) +
											 adjacentUnit->getPosNotThreadSafe();

						bool canUnitMoveToCell = map->aproxCanMove(adjacentUnit, adjacentUnit->getPosNotThreadSafe(), pos);
						if(canUnitMoveToCell == true) {

							if(ct != NULL) {
								if(unitGroupCommandId == -1) {
									unitGroupCommandId = aiInterface->getWorld()->getNextCommandGroupId();
								}

								//std::pair<CommandResult,string> r = aiInterface->giveCommand(adjacentUnit,ct, pos, unitGroupCommandId);
								aiInterface->giveCommand(adjacentUnit,ct, pos, unitGroupCommandId);
							}
						}
					}
				}
			}
		}
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [START]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
}

bool Ai::outputAIBehaviourToConsole() const {
	return false;
}

void Ai::saveGame(XmlNode *rootNode) const {
	std::map<string,string> mapTagReplacements;
	XmlNode *aiNode = rootNode->addChild("Ai");

//    AiInterface *aiInterface;
//	AiRules aiRules;
//    int startLoc;
	aiNode->addAttribute("startLoc",intToStr(startLoc), mapTagReplacements);
//    bool randomMinWarriorsReached;
	aiNode->addAttribute("randomMinWarriorsReached",intToStr(randomMinWarriorsReached), mapTagReplacements);
//	Tasks tasks;
	for(Tasks::const_iterator it = tasks.begin(); it != tasks.end(); ++it) {
		(*it)->saveGame(aiNode);
	}
//	Positions expansionPositions;
	for(Positions::const_iterator it = expansionPositions.begin(); it != expansionPositions.end(); ++it) {
		XmlNode *expansionPositionsNode = aiNode->addChild("expansionPositions");
		expansionPositionsNode->addAttribute("pos",(*it).getString(), mapTagReplacements);
	}

//	RandomGen random;
	aiNode->addAttribute("random",intToStr(random.getLastNumber()), mapTagReplacements);
//	std::map<int,int> factionSwitchTeamRequestCount;

//	int maxBuildRadius;
	aiNode->addAttribute("maxBuildRadius",intToStr(maxBuildRadius), mapTagReplacements);
//	int minMinWarriors;
	aiNode->addAttribute("minMinWarriors",intToStr(minMinWarriors), mapTagReplacements);
//	int minMinWarriorsExpandCpuEasy;
	aiNode->addAttribute("minMinWarriorsExpandCpuEasy",intToStr(minMinWarriorsExpandCpuEasy), mapTagReplacements);
//	int minMinWarriorsExpandCpuMega;
	aiNode->addAttribute("minMinWarriorsExpandCpuMega",intToStr(minMinWarriorsExpandCpuMega), mapTagReplacements);
//	int minMinWarriorsExpandCpuUltra;
	aiNode->addAttribute("minMinWarriorsExpandCpuUltra",intToStr(minMinWarriorsExpandCpuUltra), mapTagReplacements);
//	int minMinWarriorsExpandCpuNormal;
	aiNode->addAttribute("minMinWarriorsExpandCpuNormal",intToStr(minMinWarriorsExpandCpuNormal), mapTagReplacements);
//	int maxMinWarriors;
	aiNode->addAttribute("maxMinWarriors",intToStr(maxMinWarriors), mapTagReplacements);
//	int maxExpansions;
	aiNode->addAttribute("maxExpansions",intToStr(maxExpansions), mapTagReplacements);
//	int villageRadius;
	aiNode->addAttribute("villageRadius",intToStr(villageRadius), mapTagReplacements);
//	int scoutResourceRange;
	aiNode->addAttribute("scoutResourceRange",intToStr(scoutResourceRange), mapTagReplacements);
//	int minWorkerAttackersHarvesting;
	aiNode->addAttribute("minWorkerAttackersHarvesting",intToStr(minWorkerAttackersHarvesting), mapTagReplacements);
}

void Ai::loadGame(const XmlNode *rootNode, Faction *faction) {
	const XmlNode *aiNode = rootNode->getChild("Ai");

	startLoc = aiNode->getAttribute("startLoc")->getIntValue();
	randomMinWarriorsReached = aiNode->getAttribute("randomMinWarriorsReached")->getIntValue() != 0;

	vector<XmlNode *> taskNodeList = aiNode->getChildList("Task");
	for(unsigned int i = 0; i < taskNodeList.size(); ++i) {
		XmlNode *taskNode = taskNodeList[i];
		TaskClass taskClass = static_cast<TaskClass>(taskNode->getAttribute("taskClass")->getIntValue());
		switch(taskClass) {
			case tcProduce:
				{
				ProduceTask *newTask = ProduceTask::loadGame(taskNode, faction);
				tasks.push_back(newTask);
				}
				break;
			case tcBuild:
				{
				BuildTask *newTask = BuildTask::loadGame(taskNode, faction);
				tasks.push_back(newTask);
				}
				break;
			case tcUpgrade:
				{
				UpgradeTask *newTask = UpgradeTask::loadGame(taskNode, faction);
				tasks.push_back(newTask);
				}
				break;
		}
	}

	vector<XmlNode *> expansionPositionsNodeList = aiNode->getChildList("expansionPositions");
	for(unsigned int i = 0; i < expansionPositionsNodeList.size(); ++i) {
		XmlNode *expansionPositionsNode = expansionPositionsNodeList[i];
		Vec2i pos = Vec2i::strToVec2(expansionPositionsNode->getAttribute("pos")->getValue());
		expansionPositions.push_back(pos);
	}

	//	RandomGen random;
	random.setLastNumber(aiNode->getAttribute("random")->getIntValue());
	//	std::map<int,int> factionSwitchTeamRequestCount;

	//	int maxBuildRadius;
	maxBuildRadius = aiNode->getAttribute("maxBuildRadius")->getIntValue();
	//	int minMinWarriors;
	minMinWarriors = aiNode->getAttribute("minMinWarriors")->getIntValue();
	//	int minMinWarriorsExpandCpuEasy;
	minMinWarriorsExpandCpuEasy = aiNode->getAttribute("minMinWarriorsExpandCpuEasy")->getIntValue();
	//	int minMinWarriorsExpandCpuMega;
	minMinWarriorsExpandCpuMega = aiNode->getAttribute("minMinWarriorsExpandCpuMega")->getIntValue();
	//	int minMinWarriorsExpandCpuUltra;
	minMinWarriorsExpandCpuUltra = aiNode->getAttribute("minMinWarriorsExpandCpuUltra")->getIntValue();
	//	int minMinWarriorsExpandCpuNormal;
	minMinWarriorsExpandCpuNormal = aiNode->getAttribute("minMinWarriorsExpandCpuNormal")->getIntValue();
	//	int maxMinWarriors;
	maxMinWarriors = aiNode->getAttribute("maxMinWarriors")->getIntValue();
	//	int maxExpansions;
	maxExpansions = aiNode->getAttribute("maxExpansions")->getIntValue();
	//	int villageRadius;
	villageRadius = aiNode->getAttribute("villageRadius")->getIntValue();
	//	int scoutResourceRange;
	scoutResourceRange = aiNode->getAttribute("scoutResourceRange")->getIntValue();
	//	int minWorkerAttackersHarvesting;
	minWorkerAttackersHarvesting = aiNode->getAttribute("minWorkerAttackersHarvesting")->getIntValue();
}

}}//end namespace
