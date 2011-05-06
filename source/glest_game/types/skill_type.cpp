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

	string path= sn->getChild("animation")->getAttribute("path")->getRestrictedValue(currentPath);
	animation= Renderer::getInstance().newModel(rsGame);
	animation->load(path, false, &loadedFileList, &parentLoader);
	loadedFileList[path].push_back(make_pair(parentLoader,sn->getChild("animation")->getAttribute("path")->getRestrictedValue()));

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
						loadedFileList,parentLoader);
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
	return speed + totalUpgrade->getMoveSpeed();
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
					&Renderer::getInstance(), loadedFileList, parentLoader);
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
					&Renderer::getInstance(),loadedFileList, parentLoader);
		}
	}
}

string AttackSkillType::toString() const{
	return Lang::getInstance().get("Attack");
}

//get totals
int AttackSkillType::getTotalAttackStrength(const TotalUpgrade *totalUpgrade) const{
	return attackStrength + totalUpgrade->getAttackStrength();
}

int AttackSkillType::getTotalAttackRange(const TotalUpgrade *totalUpgrade) const{
	return attackRange + totalUpgrade->getAttackRange();
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
	return speed + totalUpgrade->getProdSpeed();
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
	return speed + totalUpgrade->getProdSpeed();
}

// =====================================================
// 	class BeBuiltSkillType
// =====================================================

BeBuiltSkillType::BeBuiltSkillType(){
    skillClass= scBeBuilt;
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
	return speed + totalUpgrade->getProdSpeed();
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
