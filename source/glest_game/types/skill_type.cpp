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

#include "skill_type.h"

#include <cassert>

#include "sound.h"
#include "util.h"
#include "lang.h"
#include "renderer.h"
#include "particle_type.h"
#include "unit_particle_type.h"
#include "tech_tree.h"
#include "faction_type.h"
#include "leak_dumper.h"

using namespace Shared::Util;
using namespace Shared::Graphics;

namespace Glest{ namespace Game{


AttackBoost::AttackBoost() {
	enabled = false;
	allowMultipleBoosts = false;
	radius = 0;
	targetType = abtFaction;
	unitParticleSystemTypeForSourceUnit = NULL;
	unitParticleSystemTypeForAffectedUnit = NULL;
}

AttackBoost::~AttackBoost() {
	delete unitParticleSystemTypeForSourceUnit;
	unitParticleSystemTypeForSourceUnit = NULL;

	delete unitParticleSystemTypeForAffectedUnit;
	unitParticleSystemTypeForAffectedUnit = NULL;
}

bool AttackBoost::isAffected(const Unit *source, const Unit *dest) const {
	bool result = false;
	if(enabled == true && source != NULL && dest != NULL && source != dest) {
		bool destUnitMightApply = false;
		// All units are affected (including enemies)
		if(targetType == abtAll) {
			destUnitMightApply = true;
		}
		// Only same faction units are affected
		else if(targetType == abtFaction) {
			if(boostUnitList.size() == 0) {
				if(source->getFactionIndex() == dest->getFactionIndex()) {
					destUnitMightApply = true;
				}
			}
		}
		// Only ally units are affected
		else if(targetType == abtAlly) {
			if(boostUnitList.size() == 0) {
				if(source->isAlly(dest) == true) {
					destUnitMightApply = true;
				}
			}
		}
		// Only foe units are affected
		else if(targetType == abtFoe) {
			if(boostUnitList.size() == 0) {
				if(source->isAlly(dest) == false) {
					destUnitMightApply = true;
				}
			}
		}
		else if(targetType == abtUnitTypes) {
			// Specify which units are affected
			for(unsigned int i = 0; i < boostUnitList.size(); ++i) {
				const UnitType *ut = boostUnitList[i];
				if(dest->getType()->getId() == ut->getId()) {
					destUnitMightApply = true;
				}
			}
		}

		if(destUnitMightApply == true) {
			float distance = source->getCenteredPos().dist(dest->getCenteredPos());
			if(distance <= radius) {
				result = true;
			}
		}
	}

	return result;
}


// =====================================================
// 	class SkillType
// =====================================================

SkillType::~SkillType() {
	deleteValues(sounds.getSounds().begin(), sounds.getSounds().end());
	//remove unitParticleSystemTypes
	while(!unitParticleSystemTypes.empty()) {
		delete unitParticleSystemTypes.back();
		unitParticleSystemTypes.pop_back();
	}
}

void SkillType::load(const XmlNode *sn, const string &dir, const TechTree *tt,
		const FactionType *ft, std::map<string,vector<pair<string, string> > > &loadedFileList,
		string parentLoader) {
	//name
	name= sn->getChild("name")->getAttribute("value")->getRestrictedValue();

	//ep cost
	mpCost= sn->getChild("ep-cost")->getAttribute("value")->getIntValue();
	if (sn->hasChild("hp-cost")) {
		hpCost = sn->getChild("hp-cost")->getAttribute("value")->getIntValue();
	}
	else {
		hpCost = 0;
	}

	//speed
	speed= sn->getChild("speed")->getAttribute("value")->getIntValue();

	//anim speed
	animSpeed= sn->getChild("anim-speed")->getAttribute("value")->getIntValue();

	//model
	string currentPath = dir;
	endPathWithSlash(currentPath);

	animationRandomCycleMaxcount = -1;
	if(sn->hasChild("animation-random-cycle-maxcount") == true) {
		const XmlNode *randomCycleCountNode = sn->getChild("animation-random-cycle-maxcount");
		animationRandomCycleMaxcount = randomCycleCountNode->getAttribute("value")->getIntValue();
	}

	//string path= sn->getChild("animation")->getAttribute("path")->getRestrictedValue(currentPath);
	vector<XmlNode *> animationList = sn->getChildList("animation");
	for(unsigned int i = 0; i < animationList.size(); ++i) {
		string path= animationList[i]->getAttribute("path")->getRestrictedValue(currentPath);
		if(fileExists(path) == true) {
			Model *animation= Renderer::getInstance().newModel(rsGame);
			animation->load(path, false, &loadedFileList, &parentLoader);
			loadedFileList[path].push_back(make_pair(parentLoader,animationList[i]->getAttribute("path")->getRestrictedValue()));

			animations.push_back(animation);
			//printf("**FOUND ANIMATION [%s]\n",path.c_str());

			AnimationAttributes animationAttributeList;
			if(animationList[i]->getAttribute("minHp",false) != NULL && animationList[i]->getAttribute("maxHp",false) != NULL) {
				animationAttributeList.fromHp = animationList[i]->getAttribute("minHp")->getIntValue();
				animationAttributeList.toHp = animationList[i]->getAttribute("maxHp")->getIntValue();
			}
			animationAttributes.push_back(animationAttributeList);
		}
		else {
			SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line %d] WARNING CANNOT LOAD MODEL [%s] for parentLoader [%s]\n",__FILE__,__FUNCTION__,__LINE__,path.c_str(),parentLoader.c_str());
		}
	}
	if(animations.size() <= 0) {
		char szBuf[4096]="";
		sprintf(szBuf,"Error no animations found for skill [%s] for parentLoader [%s]",name.c_str(),parentLoader.c_str());
		throw runtime_error(szBuf);
	}

	//particles
	if(sn->hasChild("particles")) {
		const XmlNode *particleNode= sn->getChild("particles");
		bool particleEnabled= particleNode->getAttribute("value")->getBoolValue();
		if(particleEnabled) {
			for(int i = 0; i < particleNode->getChildCount(); ++i) {
				const XmlNode *particleFileNode= particleNode->getChild("particle-file", i);
				string path= particleFileNode->getAttribute("path")->getRestrictedValue();
				UnitParticleSystemType *unitParticleSystemType= new UnitParticleSystemType();
				unitParticleSystemType->load(dir,  currentPath + path, &Renderer::getInstance(),
						loadedFileList,parentLoader,tt->getPath());
				loadedFileList[currentPath + path].push_back(make_pair(parentLoader,particleFileNode->getAttribute("path")->getRestrictedValue()));
				unitParticleSystemTypes.push_back(unitParticleSystemType);
			}
		}
	}

	//sound
	const XmlNode *soundNode= sn->getChild("sound");
	if(soundNode->getAttribute("enabled")->getBoolValue()) {
		soundStartTime= soundNode->getAttribute("start-time")->getFloatValue();
		sounds.resize(soundNode->getChildCount());
		for(int i = 0; i < soundNode->getChildCount(); ++i) {
			const XmlNode *soundFileNode= soundNode->getChild("sound-file", i);
			string path= soundFileNode->getAttribute("path")->getRestrictedValue(currentPath, true);

			StaticSound *sound= new StaticSound();
			sound->load(path);
			loadedFileList[path].push_back(make_pair(parentLoader,soundFileNode->getAttribute("path")->getRestrictedValue()));
			sounds[i]= sound;
		}
	}


	// attack-boost
	if(sn->hasChild("attack-boost") == true) {
		//printf("$$FOUND ATTACK BOOST FOR [%s]\n",parentLoader.c_str());

		attackBoost.enabled = true;

		const XmlNode *attackBoostNode = sn->getChild("attack-boost");
		attackBoost.allowMultipleBoosts = attackBoostNode->getChild("allow-multiple-boosts")->getAttribute("value")->getBoolValue();
		attackBoost.radius = attackBoostNode->getChild("radius")->getAttribute("value")->getIntValue();

		string targetType = attackBoostNode->getChild("target")->getAttribute("value")->getValue();
		if(targetType == "ally") {
			attackBoost.targetType = abtAlly;
		}
		else if(targetType == "foe") {
			attackBoost.targetType = abtFoe;
		}
		else if(targetType == "faction") {
			attackBoost.targetType = abtFaction;
		}
		else if(targetType == "unit-types") {
			attackBoost.targetType = abtUnitTypes;

			for(int i = 0; i < attackBoostNode->getChild("target")->getChildCount(); ++i) {
				const XmlNode *boostUnitNode= attackBoostNode->getChild("target")->getChild("unit-type", i);
				attackBoost.boostUnitList.push_back(ft->getUnitType(boostUnitNode->getAttribute("name")->getRestrictedValue()));
			}
		}
		else if(targetType == "all") {
			attackBoost.targetType = abtAll;
		}
		else {
			char szBuf[4096]="";
			sprintf(szBuf,"Unsupported target [%s] specified for attack boost for skill [%s] in [%s]",targetType.c_str(),name.c_str(),parentLoader.c_str());
			throw runtime_error(szBuf);
		}

		attackBoost.boostUpgrade.load(attackBoostNode);

		//particles
		if(attackBoostNode->hasChild("particles") == true) {
			const XmlNode *particleNode= attackBoostNode->getChild("particles");
			bool particleEnabled= particleNode->getAttribute("value")->getBoolValue();
			if(particleEnabled == true) {
				if(particleNode->hasChild("originator-particle-file") == true) {
					const XmlNode *particleFileNode= particleNode->getChild("originator-particle-file");
					string path= particleFileNode->getAttribute("path")->getRestrictedValue();
					attackBoost.unitParticleSystemTypeForSourceUnit = new UnitParticleSystemType();
					attackBoost.unitParticleSystemTypeForSourceUnit->load(dir,  currentPath + path, &Renderer::getInstance(),
							loadedFileList,parentLoader,tt->getPath());
					loadedFileList[currentPath + path].push_back(make_pair(parentLoader,particleFileNode->getAttribute("path")->getRestrictedValue()));

				}
				if(particleNode->hasChild("affected-particle-file") == true) {
					const XmlNode *particleFileNode= particleNode->getChild("affected-particle-file");
					string path= particleFileNode->getAttribute("path")->getRestrictedValue();
					attackBoost.unitParticleSystemTypeForAffectedUnit = new UnitParticleSystemType();
					attackBoost.unitParticleSystemTypeForAffectedUnit->load(dir,  currentPath + path, &Renderer::getInstance(),
							loadedFileList,parentLoader,tt->getPath());
					loadedFileList[currentPath + path].push_back(make_pair(parentLoader,particleFileNode->getAttribute("path")->getRestrictedValue()));
				}
			}
		}
	}
}

bool SkillType::CanCycleNextRandomAnimation(const int *animationRandomCycleCount) const {
	bool result = true;
	if(animations.size() > 1) {
		if( animationRandomCycleMaxcount >= 0 &&
				animationRandomCycleCount != NULL &&
			*animationRandomCycleCount >= animationRandomCycleMaxcount) {
			result = false;
		}
	}
	return result;
}

Model *SkillType::getAnimation(float animProgress, const Unit *unit,
		int *lastAnimationIndex, int *animationRandomCycleCount) const {
	int modelIndex = 0;
	if(animations.size() > 1) {
		//printf("animProgress = [%f] for skill [%s]\n",animProgress,name.c_str());

		if(lastAnimationIndex) {
			modelIndex = *lastAnimationIndex;
		}
		if(modelIndex < 0 || animProgress > 1.0f) {
			bool canCycle = CanCycleNextRandomAnimation(animationRandomCycleCount);
			if(canCycle == true) {
				vector<int> filteredAnimations;
				bool foundSpecificAnimation = false;
				if(unit != NULL) {
					for(unsigned int i = 0; i < animationAttributes.size(); ++i) {
						const AnimationAttributes &attributes = animationAttributes[i];
						if(attributes.fromHp != 0 || attributes.toHp != 0) {
							if(unit->getHp() >= attributes.fromHp && unit->getHp() <= attributes.toHp) {
								modelIndex = i;
								foundSpecificAnimation = true;
								filteredAnimations.push_back(i);
								//printf("SELECTING Model index = %d [%s] model attributes [%d to %d] for unit [%s - %d] with HP = %d\n",i,animations[modelIndex]->getFileName().c_str(),attributes.fromHp,attributes.toHp,unit->getType()->getName().c_str(),unit->getId(),unit->getHp());
								//break;
							}
						}
					}
				}

				if(foundSpecificAnimation == false) {
					//int modelIndex = random.randRange(0,animations.size()-1);
					srand(time(NULL) + unit->getId());
					modelIndex = rand() % animations.size();

					//const AnimationAttributes &attributes = animationAttributes[modelIndex];
					//printf("SELECTING RANDOM Model index = %d [%s] model attributes [%d to %d] for unit [%s - %d] with HP = %d\n",modelIndex,animations[modelIndex]->getFileName().c_str(),attributes.fromHp,attributes.toHp,unit->getType()->getName().c_str(),unit->getId(),unit->getHp());
				}
				else {
					srand(time(NULL) + unit->getId());
					int filteredModelIndex = rand() % filteredAnimations.size();
					modelIndex = filteredAnimations[filteredModelIndex];
				}
			}
		}
	}
	if(lastAnimationIndex) {
		*lastAnimationIndex = modelIndex;
	}

	//printf("!!RETURN ANIMATION [%d / %d]\n",modelIndex,animations.size()-1);
	return animations[modelIndex];
}

string SkillType::skillClassToStr(SkillClass skillClass) {
	switch(skillClass){
	case scStop: return "Stop";
	case scMove: return "Move";
	case scAttack: return "Attack";
	case scHarvest: return "Harvest";
	case scRepair: return "Repair";
	case scBuild: return "Build";
	case scDie: return "Die";
	case scBeBuilt: return "Be Built";
	case scProduce: return "Produce";
	case scUpgrade: return "Upgrade";
	default:
		assert(false);
		return "";
	};
}

string SkillType::fieldToStr(Field field){
	switch(field){
	case fLand: return "Land";
	case fAir: return "Air";
	default:
		assert(false);
		return "";
	};
}


// =====================================================
// 	class StopSkillType
// =====================================================

StopSkillType::StopSkillType(){
    skillClass= scStop;
}

string StopSkillType::toString() const{
	return Lang::getInstance().get("Stop");
}

// =====================================================
// 	class MoveSkillType
// =====================================================

MoveSkillType::MoveSkillType(){
	skillClass= scMove;
}

string MoveSkillType::toString() const{
	return Lang::getInstance().get("Move");
}

int MoveSkillType::getTotalSpeed(const TotalUpgrade *totalUpgrade) const{
	int result = speed + totalUpgrade->getMoveSpeed(this);
	result = max(0,result);
	return result;
}

// =====================================================
// 	class AttackSkillType
// =====================================================

//varios
AttackSkillType::AttackSkillType() {
    skillClass= scAttack;
	attackType= NULL;
    projectile= false;
    splash= false;
    splashRadius= 0;
    spawnUnit="";
    spawnUnitcount=0;
	projectileParticleSystemType= NULL;
	splashParticleSystemType= NULL;

	for(int i = 0; i < fieldCount; ++i) {
		attackFields[i]= false;
	}
}

AttackSkillType::~AttackSkillType() {
	delete projectileParticleSystemType;
	projectileParticleSystemType = NULL;

	delete splashParticleSystemType;
	splashParticleSystemType = NULL;
	deleteValues(projSounds.getSounds().begin(), projSounds.getSounds().end());
}

void AttackSkillType::load(const XmlNode *sn, const string &dir, const TechTree *tt,
		const FactionType *ft, std::map<string,vector<pair<string, string> > > &loadedFileList,
		string parentLoader) {
    SkillType::load(sn, dir, tt, ft, loadedFileList, parentLoader);

	string currentPath = dir;
	endPathWithSlash(currentPath);

	//misc
	attackStrength= sn->getChild("attack-strenght")->getAttribute("value")->getIntValue();
    attackVar= sn->getChild("attack-var")->getAttribute("value")->getIntValue();

    if(attackVar < 0) {
        char szBuf[4096]="";
        sprintf(szBuf,"The attack skill has an INVALID attack var value which is < 0 [%d] in file [%s]!",attackVar,dir.c_str());
        throw runtime_error(szBuf);
    }

    attackRange= sn->getChild("attack-range")->getAttribute("value")->getIntValue();
	string attackTypeName= sn->getChild("attack-type")->getAttribute("value")->getRestrictedValue();
	attackType= tt->getAttackType(attackTypeName);
	attackStartTime= sn->getChild("attack-start-time")->getAttribute("value")->getFloatValue();

	if (sn->hasChild("unit")) {
		spawnUnit = sn->getChild("unit")->getAttribute("value")->getValue();
		spawnUnitcount
				= sn->getChild("unit")->getAttribute("amount")->getIntValue();
	} else {
		spawnUnit = "";
		spawnUnitcount = 0;
	}
	//attack fields
	const XmlNode *attackFieldsNode= sn->getChild("attack-fields");
	for(int i=0; i<attackFieldsNode->getChildCount(); ++i){
		const XmlNode *fieldNode= attackFieldsNode->getChild("field", i);
		string fieldName= fieldNode->getAttribute("value")->getRestrictedValue();
		if(fieldName=="land"){
			attackFields[fLand]= true;
		}
		else if(fieldName=="air"){
			attackFields[fAir]= true;
		}
		else{
			throw runtime_error("Not a valid field: "+fieldName+": "+ dir);
		}
	}

	//projectile
	const XmlNode *projectileNode= sn->getChild("projectile");
	projectile= projectileNode->getAttribute("value")->getBoolValue();
	if(projectile){

		//proj particle
		const XmlNode *particleNode= projectileNode->getChild("particle");
		bool particleEnabled= particleNode->getAttribute("value")->getBoolValue();
		if(particleEnabled){
			string path= particleNode->getAttribute("path")->getRestrictedValue();
			projectileParticleSystemType= new ParticleSystemTypeProjectile();
			projectileParticleSystemType->load(dir,  currentPath + path,
					&Renderer::getInstance(), loadedFileList, parentLoader,
					tt->getPath());
		}

		//proj sounds
		const XmlNode *soundNode= projectileNode->getChild("sound");
		if(soundNode->getAttribute("enabled")->getBoolValue()){

			projSounds.resize(soundNode->getChildCount());
			for(int i=0; i<soundNode->getChildCount(); ++i){
				const XmlNode *soundFileNode= soundNode->getChild("sound-file", i);
				string path= soundFileNode->getAttribute("path")->getRestrictedValue(currentPath, true);
				//printf("\n\n\n\n!@#$ ---> parentLoader [%s] path [%s] nodeValue [%s] i = %d",parentLoader.c_str(),path.c_str(),soundFileNode->getAttribute("path")->getRestrictedValue().c_str(),i);

				StaticSound *sound= new StaticSound();
				sound->load(path);
				loadedFileList[path].push_back(make_pair(parentLoader,soundFileNode->getAttribute("path")->getRestrictedValue()));
				projSounds[i]= sound;
			}
		}
	}

	//splash
	const XmlNode *splashNode= sn->getChild("splash");
	splash= splashNode->getAttribute("value")->getBoolValue();
	if(splash){
		splashRadius= splashNode->getChild("radius")->getAttribute("value")->getIntValue();
		splashDamageAll= splashNode->getChild("damage-all")->getAttribute("value")->getBoolValue();

		//splash particle
		const XmlNode *particleNode= splashNode->getChild("particle");
		bool particleEnabled= particleNode->getAttribute("value")->getBoolValue();
		if(particleEnabled){
			string path= particleNode->getAttribute("path")->getRestrictedValue();
			splashParticleSystemType= new ParticleSystemTypeSplash();
			splashParticleSystemType->load(dir,  currentPath + path,
					&Renderer::getInstance(),loadedFileList, parentLoader,
					tt->getPath());
		}
	}
}

string AttackSkillType::toString() const{
	return Lang::getInstance().get("Attack");
}

//get totals
int AttackSkillType::getTotalAttackStrength(const TotalUpgrade *totalUpgrade) const{
	int result = attackStrength + totalUpgrade->getAttackStrength(this);
	result = max(0,result);
	return result;
}

int AttackSkillType::getTotalAttackRange(const TotalUpgrade *totalUpgrade) const{
	int result = attackRange + totalUpgrade->getAttackRange(this);
	result = max(0,result);
	return result;
}

// =====================================================
// 	class BuildSkillType
// =====================================================

BuildSkillType::BuildSkillType(){
    skillClass= scBuild;
}

string BuildSkillType::toString() const{
	return Lang::getInstance().get("Build");
}

// =====================================================
// 	class HarvestSkillType
// =====================================================

HarvestSkillType::HarvestSkillType(){
    skillClass= scHarvest;
}

string HarvestSkillType::toString() const{
	return Lang::getInstance().get("Harvest");
}

// =====================================================
// 	class RepairSkillType
// =====================================================

RepairSkillType::RepairSkillType(){
    skillClass= scRepair;
}

string RepairSkillType::toString() const{
	return Lang::getInstance().get("Repair");
}

// =====================================================
// 	class ProduceSkillType
// =====================================================

ProduceSkillType::ProduceSkillType(){
    skillClass= scProduce;
}

string ProduceSkillType::toString() const{
	return Lang::getInstance().get("Produce");
}

int ProduceSkillType::getTotalSpeed(const TotalUpgrade *totalUpgrade) const{
	int result = speed + totalUpgrade->getProdSpeed(this);
	result = max(0,result);
	return result;
}

// =====================================================
// 	class UpgradeSkillType
// =====================================================

UpgradeSkillType::UpgradeSkillType(){
    skillClass= scUpgrade;
}

string UpgradeSkillType::toString() const{
	return Lang::getInstance().get("Upgrade");
}

int UpgradeSkillType::getTotalSpeed(const TotalUpgrade *totalUpgrade) const{
	int result = speed + totalUpgrade->getProdSpeed(this);
	result = max(0,result);
	return result;
}

// =====================================================
// 	class BeBuiltSkillType
// =====================================================

BeBuiltSkillType::BeBuiltSkillType(){
    skillClass= scBeBuilt;
}

void BeBuiltSkillType::load(const XmlNode *sn, const string &dir, const TechTree *tt,
		const FactionType *ft, std::map<string,vector<pair<string, string> > > &loadedFileList,
		string parentLoader) {
	SkillType::load(sn, dir, tt, ft, loadedFileList, parentLoader);

	if(sn->hasChild("anim-hp-bound")){
		animHpBound= sn->getChild("anim-hp-bound")->getAttribute("value")->getBoolValue();
	}
	else {
		animHpBound=false;
	}
}


string BeBuiltSkillType::toString() const{
	return "Be built";
}

// =====================================================
// 	class MorphSkillType
// =====================================================

MorphSkillType::MorphSkillType(){
    skillClass= scMorph;
}

string MorphSkillType::toString() const{
	return "Morph";
}

int MorphSkillType::getTotalSpeed(const TotalUpgrade *totalUpgrade) const{
	int result = speed + totalUpgrade->getProdSpeed(this);
	result = max(0,result);
	return result;
}

// =====================================================
// 	class DieSkillType
// =====================================================

DieSkillType::DieSkillType(){
    skillClass= scDie;
}

void DieSkillType::load(const XmlNode *sn, const string &dir, const TechTree *tt,
		const FactionType *ft, std::map<string,vector<pair<string, string> > > &loadedFileList,
		string parentLoader) {
	SkillType::load(sn, dir, tt, ft, loadedFileList, parentLoader);

	fade= sn->getChild("fade")->getAttribute("value")->getBoolValue();
}

string DieSkillType::toString() const{
	return "Die";
}

// =====================================================
// 	class SkillTypeFactory
// =====================================================

SkillTypeFactory::SkillTypeFactory(){
	registerClass<StopSkillType>("stop");
	registerClass<MoveSkillType>("move");
	registerClass<AttackSkillType>("attack");
	registerClass<BuildSkillType>("build");
	registerClass<BeBuiltSkillType>("be_built");
	registerClass<HarvestSkillType>("harvest");
	registerClass<RepairSkillType>("repair");
	registerClass<ProduceSkillType>("produce");
	registerClass<UpgradeSkillType>("upgrade");
	registerClass<MorphSkillType>("morph");
	registerClass<DieSkillType>("die");
}

SkillTypeFactory &SkillTypeFactory::getInstance(){
	static SkillTypeFactory ctf;
	return ctf;
}

}} //end namespace
