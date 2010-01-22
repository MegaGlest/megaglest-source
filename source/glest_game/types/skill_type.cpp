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

#include "skill_type.h"

#include <cassert>

#include "sound.h"
#include "util.h"
#include "lang.h"
#include "renderer.h"
#include "particle_type.h"
#include "tech_tree.h"
#include "faction_type.h"
#include "leak_dumper.h"

using namespace Shared::Util;
using namespace Shared::Graphics;

namespace Glest{ namespace Game{

// =====================================================
// 	class SkillType
// =====================================================

SkillType::~SkillType(){
	deleteValues(sounds.getSounds().begin(), sounds.getSounds().end());
}

void SkillType::load(const XmlNode *sn, const string &dir, const TechTree *tt, const FactionType *ft){
	//name
	name= sn->getChild("name")->getAttribute("value")->getRestrictedValue();
	
	//ep cost
	mpCost= sn->getChild("ep-cost")->getAttribute("value")->getIntValue();
	
	//speed
	speed= sn->getChild("speed")->getAttribute("value")->getIntValue();
	
	//anim speed
	animSpeed= sn->getChild("anim-speed")->getAttribute("value")->getIntValue();
	
	//model
	string path= sn->getChild("animation")->getAttribute("path")->getRestrictedValue();
	animation= Renderer::getInstance().newModel(rsGame);
	animation->load(dir + "/" + path);
	
	//sound
	const XmlNode *soundNode= sn->getChild("sound");
	if(soundNode->getAttribute("enabled")->getBoolValue()){
		
		soundStartTime= soundNode->getAttribute("start-time")->getFloatValue();
	
		sounds.resize(soundNode->getChildCount());
		for(int i=0; i<soundNode->getChildCount(); ++i){
			const XmlNode *soundFileNode= soundNode->getChild("sound-file", i);
			string path= soundFileNode->getAttribute("path")->getRestrictedValue();
			StaticSound *sound= new StaticSound();
			sound->load(dir + "/" + path);
			sounds[i]= sound;
		}
	}
}

string SkillType::skillClassToStr(SkillClass skillClass){
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
AttackSkillType::AttackSkillType(){
    skillClass= scAttack;
	attackType= NULL;
    projectile= false;
    splash= false;
    splashRadius= 0;
	projectileParticleSystemType= NULL;
	splashParticleSystemType= NULL;
	for(int i=0; i<fieldCount; ++i){
		attackFields[i]= false;
	}
}

AttackSkillType::~AttackSkillType(){
	delete projectileParticleSystemType;
	delete splashParticleSystemType;
	deleteValues(projSounds.getSounds().begin(), projSounds.getSounds().end());
}

void AttackSkillType::load(const XmlNode *sn, const string &dir, const TechTree *tt, const FactionType *ft){
    
	SkillType::load(sn, dir, tt, ft);

	//misc
	attackStrength= sn->getChild("attack-strenght")->getAttribute("value")->getIntValue();
    attackVar= sn->getChild("attack-var")->getAttribute("value")->getIntValue();
    attackRange= sn->getChild("attack-range")->getAttribute("value")->getIntValue();
	string attackTypeName= sn->getChild("attack-type")->getAttribute("value")->getRestrictedValue();
	attackType= tt->getAttackType(attackTypeName);
	attackStartTime= sn->getChild("attack-start-time")->getAttribute("value")->getFloatValue();

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
			projectileParticleSystemType->load(dir,  dir + "/" + path);
		}

		//proj sounds
		const XmlNode *soundNode= projectileNode->getChild("sound");
		if(soundNode->getAttribute("enabled")->getBoolValue()){
			
			projSounds.resize(soundNode->getChildCount());
			for(int i=0; i<soundNode->getChildCount(); ++i){
				const XmlNode *soundFileNode= soundNode->getChild("sound-file", i);
				string path= soundFileNode->getAttribute("path")->getRestrictedValue();
				StaticSound *sound= new StaticSound();
				sound->load(dir + "/" + path);
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
			splashParticleSystemType->load(dir,  dir + "/" + path);
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

void DieSkillType::load(const XmlNode *sn, const string &dir, const TechTree *tt, const FactionType *ft){
	SkillType::load(sn, dir, tt, ft);

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
