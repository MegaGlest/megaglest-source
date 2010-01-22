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

#include "command_type.h"

#include <algorithm>
#include <cassert>

#include "upgrade_type.h"
#include "unit_type.h"
#include "sound.h"
#include "util.h"
#include "leak_dumper.h"
#include "graphics_interface.h"
#include "tech_tree.h"
#include "faction_type.h"
#include "unit_updater.h"
#include "renderer.h"
#include "leak_dumper.h"

using namespace Shared::Util;

namespace Glest{ namespace Game{


// =====================================================
// 	class CommandType
// =====================================================

//get
CommandClass CommandType::getClass() const{
    assert(this!=NULL);
    return commandTypeClass;
}

void CommandType::load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut){
	this->id= id;
	name= n->getChild("name")->getAttribute("value")->getRestrictedValue();

	//image
	const XmlNode *imageNode= n->getChild("image");
	image= Renderer::getInstance().newTexture2D(rsGame);
	image->load(dir+"/"+imageNode->getAttribute("path")->getRestrictedValue());

	//unit requirements
	const XmlNode *unitRequirementsNode= n->getChild("unit-requirements");
	for(int i=0; i<unitRequirementsNode->getChildCount(); ++i){
		const XmlNode *unitNode= 	unitRequirementsNode->getChild("unit", i);
		string name= unitNode->getAttribute("name")->getRestrictedValue();
		unitReqs.push_back(ft->getUnitType(name));
	}

	//upgrade requirements
	const XmlNode *upgradeRequirementsNode= n->getChild("upgrade-requirements");
	for(int i=0; i<upgradeRequirementsNode->getChildCount(); ++i){
		const XmlNode *upgradeReqNode= upgradeRequirementsNode->getChild("upgrade", i);
		string name= upgradeReqNode->getAttribute("name")->getRestrictedValue();
		upgradeReqs.push_back(ft->getUpgradeType(name));
	}
}

// =====================================================
// 	class StopCommandType
// =====================================================

//varios
StopCommandType::StopCommandType(){
    commandTypeClass= ccStop;
    clicks= cOne;
}

void StopCommandType::update(UnitUpdater *unitUpdater, Unit *unit) const{
	unitUpdater->updateStop(unit);
}

string StopCommandType::getDesc(const TotalUpgrade *totalUpgrade) const{
    string str;
	Lang &lang= Lang::getInstance();
	
    str= name+"\n";
	str+= lang.get("ReactionSpeed")+": "+ intToStr(stopSkillType->getSpeed())+"\n";
    if(stopSkillType->getEpCost()!=0)
        str+= lang.get("EpCost")+": "+intToStr(stopSkillType->getEpCost())+"\n";

    return str;
}

string StopCommandType::toString() const{
	Lang &lang= Lang::getInstance();
	return lang.get("Stop");
}

void StopCommandType::load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut){
	CommandType::load(id, n, dir, tt, ft, ut);

	//stop
   	string skillName= n->getChild("stop-skill")->getAttribute("value")->getRestrictedValue();
	stopSkillType= static_cast<const StopSkillType*>(ut.getSkillType(skillName, scStop));
}


// =====================================================
// 	class MoveCommandType
// =====================================================

//varios
MoveCommandType::MoveCommandType(){
    commandTypeClass= ccMove;
    clicks= cTwo;
}

void MoveCommandType::update(UnitUpdater *unitUpdater, Unit *unit) const{
	unitUpdater->updateMove(unit);
}

void MoveCommandType::load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut){
    CommandType::load(id, n, dir, tt, ft, ut);

	//move
   	string skillName= n->getChild("move-skill")->getAttribute("value")->getRestrictedValue();
	moveSkillType= static_cast<const MoveSkillType*>(ut.getSkillType(skillName, scMove));
}

string MoveCommandType::getDesc(const TotalUpgrade *totalUpgrade) const{
    string str;
	Lang &lang= Lang::getInstance();
	
    str= name+"\n";
    str+= lang.get("WalkSpeed")+": "+ intToStr(moveSkillType->getSpeed());
	if(totalUpgrade->getMoveSpeed()!=0){
        str+= "+" + intToStr(totalUpgrade->getMoveSpeed());
	}
    str+="\n";
	if(moveSkillType->getEpCost()!=0){
        str+= lang.get("EpCost")+": "+intToStr(moveSkillType->getEpCost())+"\n";
	}
    
    return str;
}

string MoveCommandType::toString() const{
	Lang &lang= Lang::getInstance();
	return lang.get("Move");
}

// =====================================================
// 	class AttackCommandType
// =====================================================

//varios
AttackCommandType::AttackCommandType(){
    commandTypeClass= ccAttack;
    clicks= cTwo;
}

void AttackCommandType::update(UnitUpdater *unitUpdater, Unit *unit) const{
	unitUpdater->updateAttack(unit);
}

void AttackCommandType::load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut){
    CommandType::load(id, n, dir, tt, ft, ut);
	
    //move
   	string skillName= n->getChild("move-skill")->getAttribute("value")->getRestrictedValue();
	moveSkillType= static_cast<const MoveSkillType*>(ut.getSkillType(skillName, scMove));

    //attack
   	skillName= n->getChild("attack-skill")->getAttribute("value")->getRestrictedValue();
	attackSkillType= static_cast<const AttackSkillType*>(ut.getSkillType(skillName, scAttack));
}

string AttackCommandType::getDesc(const TotalUpgrade *totalUpgrade) const{
    string str;
	Lang &lang= Lang::getInstance();
	
    str= name+"\n";
	if(attackSkillType->getEpCost()!=0){
        str+= lang.get("EpCost") + ": " + intToStr(attackSkillType->getEpCost()) + "\n";
	}

    //attack strength
    str+= lang.get("AttackStrenght")+": ";
    str+= intToStr(attackSkillType->getAttackStrength()-attackSkillType->getAttackVar()); 
    str+= "...";
    str+= intToStr(attackSkillType->getAttackStrength()+attackSkillType->getAttackVar()); 
	if(totalUpgrade->getAttackStrength()!=0){
        str+= "+"+intToStr(totalUpgrade->getAttackStrength());
	}
	str+= " ("+ attackSkillType->getAttackType()->getName() +")";
    str+= "\n";

    //splash radius
	if(attackSkillType->getSplashRadius()!=0){
        str+= lang.get("SplashRadius")+": "+intToStr(attackSkillType->getSplashRadius())+"\n";
	}

    //attack distance
    str+= lang.get("AttackDistance")+": "+intToStr(attackSkillType->getAttackRange());
	if(totalUpgrade->getAttackRange()!=0){
        str+= "+"+intToStr(totalUpgrade->getAttackRange()!=0);
	}
    str+="\n";

	//attack fields
	str+= lang.get("Fields") + ": ";
	for(int i= 0; i < fieldCount; i++){
		Field field = static_cast<Field>(i);
		if( attackSkillType->getAttackField(field) )
		{
			str+= SkillType::fieldToStr(field) + " ";
		}
	}
	str+="\n";

    //movement speed
    str+= lang.get("WalkSpeed")+": "+ intToStr(moveSkillType->getSpeed()) ;
	if(totalUpgrade->getMoveSpeed()!=0){
        str+= "+"+intToStr(totalUpgrade->getMoveSpeed());
	}
    str+="\n";
    
    str+= lang.get("AttackSpeed")+": "+ intToStr(attackSkillType->getSpeed()) +"\n";

    return str;
}

string AttackCommandType::toString() const{
	Lang &lang= Lang::getInstance();
		return lang.get("Attack");
}


// =====================================================
// 	class AttackStoppedCommandType
// =====================================================

//varios
AttackStoppedCommandType::AttackStoppedCommandType(){
    commandTypeClass= ccAttackStopped;
    clicks= cOne;
}

void AttackStoppedCommandType::update(UnitUpdater *unitUpdater, Unit *unit) const{
	unitUpdater->updateAttackStopped(unit);
}

void AttackStoppedCommandType::load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut){
    CommandType::load(id, n, dir, tt, ft, ut);

	//stop
   	string skillName= n->getChild("stop-skill")->getAttribute("value")->getRestrictedValue();
	stopSkillType= static_cast<const StopSkillType*>(ut.getSkillType(skillName, scStop));

    //attack
   	skillName= n->getChild("attack-skill")->getAttribute("value")->getRestrictedValue();
	attackSkillType= static_cast<const AttackSkillType*>(ut.getSkillType(skillName, scAttack));
}

string AttackStoppedCommandType::getDesc(const TotalUpgrade *totalUpgrade) const{
    Lang &lang= Lang::getInstance();
	string str;

    str= name+"\n";
	if(attackSkillType->getEpCost()!=0){
        str+= lang.get("EpCost")+": "+intToStr(attackSkillType->getEpCost())+"\n";
	}
   
    //attack strength
    str+= lang.get("AttackStrenght")+": ";
    str+= intToStr(attackSkillType->getAttackStrength()-attackSkillType->getAttackVar()); 
    str+="...";
    str+= intToStr(attackSkillType->getAttackStrength()+attackSkillType->getAttackVar()); 
    if(totalUpgrade->getAttackStrength()!=0)
        str+= "+"+intToStr(totalUpgrade->getAttackStrength());
    str+= " ("+ attackSkillType->getAttackType()->getName() +")";
    str+="\n";

    //splash radius
	if(attackSkillType->getSplashRadius()!=0){
        str+= lang.get("SplashRadius")+": "+intToStr(attackSkillType->getSplashRadius())+"\n";
	}

    //attack distance
    str+= lang.get("AttackDistance")+": "+floatToStr(attackSkillType->getAttackRange());
	if(totalUpgrade->getAttackRange()!=0){
        str+= "+"+intToStr(totalUpgrade->getAttackRange()!=0);
	}
    str+="\n";

	//attack fields
	str+= lang.get("Fields") + ": ";
	for(int i= 0; i < fieldCount; i++){
		Field field = static_cast<Field>(i);
		if( attackSkillType->getAttackField(field) )
		{
			str+= SkillType::fieldToStr(field) + " ";
		}
	}
	str+="\n";

    return str;
}

string AttackStoppedCommandType::toString() const{
	Lang &lang= Lang::getInstance();
	return lang.get("AttackStopped");
}


// =====================================================
// 	class BuildCommandType
// =====================================================

//varios
BuildCommandType::BuildCommandType(){
    commandTypeClass= ccBuild;
    clicks= cTwo;
}

BuildCommandType::~BuildCommandType(){
	deleteValues(builtSounds.getSounds().begin(), builtSounds.getSounds().end());
	deleteValues(startSounds.getSounds().begin(), startSounds.getSounds().end());
}

void BuildCommandType::update(UnitUpdater *unitUpdater, Unit *unit) const{
	unitUpdater->updateBuild(unit);
}

void BuildCommandType::load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut){
    CommandType::load(id, n, dir, tt, ft, ut);

	//move
   	string skillName= n->getChild("move-skill")->getAttribute("value")->getRestrictedValue();
	moveSkillType= static_cast<const MoveSkillType*>(ut.getSkillType(skillName, scMove));

    //build
   	skillName= n->getChild("build-skill")->getAttribute("value")->getRestrictedValue();
	buildSkillType= static_cast<const BuildSkillType*>(ut.getSkillType(skillName, scBuild));

	//buildings built
	const XmlNode *buildingsNode= n->getChild("buildings");
	for(int i=0; i<buildingsNode->getChildCount(); ++i){
		const XmlNode *buildingNode= buildingsNode->getChild("building", i);
		string name= buildingNode->getAttribute("name")->getRestrictedValue(); 
		buildings.push_back(ft->getUnitType(name));
	}

	//start sound
	const XmlNode *startSoundNode= n->getChild("start-sound");
	if(startSoundNode->getAttribute("enabled")->getBoolValue()){
		startSounds.resize(startSoundNode->getChildCount());
		for(int i=0; i<startSoundNode->getChildCount(); ++i){
			const XmlNode *soundFileNode= startSoundNode->getChild("sound-file", i);
			string path= soundFileNode->getAttribute("path")->getRestrictedValue();
			StaticSound *sound= new StaticSound();
			sound->load(dir + "/" + path);
			startSounds[i]= sound;
		}
	}

	//built sound
	const XmlNode *builtSoundNode= n->getChild("built-sound");
	if(builtSoundNode->getAttribute("enabled")->getBoolValue()){
		builtSounds.resize(builtSoundNode->getChildCount());
		for(int i=0; i<builtSoundNode->getChildCount(); ++i){
			const XmlNode *soundFileNode= builtSoundNode->getChild("sound-file", i);
			string path= soundFileNode->getAttribute("path")->getRestrictedValue();
			StaticSound *sound= new StaticSound();
			sound->load(dir + "/" + path);
			builtSounds[i]= sound;
		}
	}
}

string BuildCommandType::getDesc(const TotalUpgrade *totalUpgrade) const{
    string str;
	Lang &lang= Lang::getInstance();
	
    str= name+"\n";
    str+= lang.get("BuildSpeed")+": "+ intToStr(buildSkillType->getSpeed())+"\n";
	if(buildSkillType->getEpCost()!=0){
        str+= lang.get("EpCost")+": "+intToStr(buildSkillType->getEpCost())+"\n";
	}
    
    return str;
}

string BuildCommandType::toString() const{
	Lang &lang= Lang::getInstance();
	return lang.get("Build");
}

// =====================================================
// 	class HarvestCommandType
// =====================================================

//varios
HarvestCommandType::HarvestCommandType(){
    commandTypeClass= ccHarvest;
    clicks= cTwo;
}

void HarvestCommandType::update(UnitUpdater *unitUpdater, Unit *unit) const{
	unitUpdater->updateHarvest(unit);
}

void HarvestCommandType::load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut){
    
	CommandType::load(id, n, dir, tt, ft, ut);
	
	//move
   	string skillName= n->getChild("move-skill")->getAttribute("value")->getRestrictedValue();
	moveSkillType= static_cast<const MoveSkillType*>(ut.getSkillType(skillName, scMove));

    //harvest
   	skillName= n->getChild("harvest-skill")->getAttribute("value")->getRestrictedValue();
	harvestSkillType= static_cast<const HarvestSkillType*>(ut.getSkillType(skillName, scHarvest));

    //stop loaded
   	skillName= n->getChild("stop-loaded-skill")->getAttribute("value")->getRestrictedValue();
	stopLoadedSkillType= static_cast<const StopSkillType*>(ut.getSkillType(skillName, scStop));

    //move loaded
   	skillName= n->getChild("move-loaded-skill")->getAttribute("value")->getRestrictedValue();
	moveLoadedSkillType= static_cast<const MoveSkillType*>(ut.getSkillType(skillName, scMove));

	//resources can harvest
	const XmlNode *resourcesNode= n->getChild("harvested-resources");
	for(int i=0; i<resourcesNode->getChildCount(); ++i){
		const XmlNode *resourceNode= resourcesNode->getChild("resource", i);
		harvestedResources.push_back(tt->getResourceType(resourceNode->getAttribute("name")->getRestrictedValue()));
	}

	maxLoad= n->getChild("max-load")->getAttribute("value")->getIntValue();
	hitsPerUnit= n->getChild("hits-per-unit")->getAttribute("value")->getIntValue();
}

string HarvestCommandType::getDesc(const TotalUpgrade *totalUpgrade) const{
    
	Lang &lang= Lang::getInstance();
	string str;

    str= name+"\n";
    str+= lang.get("HarvestSpeed")+": "+ intToStr(harvestSkillType->getSpeed()/hitsPerUnit)+"\n";
    str+= lang.get("MaxLoad")+": "+ intToStr(maxLoad)+"\n";
    str+= lang.get("LoadedSpeed")+": "+ intToStr(moveLoadedSkillType->getSpeed())+"\n";
	if(harvestSkillType->getEpCost()!=0){
        str+= lang.get("EpCost")+": "+intToStr(harvestSkillType->getEpCost())+"\n";
	}
	str+=lang.get("Resources")+":\n";
	for(int i=0; i<getHarvestedResourceCount(); ++i){
		str+= getHarvestedResource(i)->getName()+"\n";
	}

    return str;
}

string HarvestCommandType::toString() const{
	Lang &lang= Lang::getInstance();
	return lang.get("Harvest");
}

bool HarvestCommandType::canHarvest(const ResourceType *resourceType) const{
	return find(harvestedResources.begin(), harvestedResources.end(), resourceType) != harvestedResources.end();
}

// =====================================================
// 	class RepairCommandType
// =====================================================

//varios
RepairCommandType::RepairCommandType(){
    commandTypeClass= ccRepair;
    clicks= cTwo;
}

RepairCommandType::~RepairCommandType(){
}

void RepairCommandType::update(UnitUpdater *unitUpdater, Unit *unit) const{
	unitUpdater->updateRepair(unit);
}

void RepairCommandType::load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut){
    
	CommandType::load(id, n, dir, tt, ft, ut);

	//move
   	string skillName= n->getChild("move-skill")->getAttribute("value")->getRestrictedValue();
	moveSkillType= static_cast<const MoveSkillType*>(ut.getSkillType(skillName, scMove));

	//repair
   	skillName= n->getChild("repair-skill")->getAttribute("value")->getRestrictedValue();
	repairSkillType= static_cast<const RepairSkillType*>(ut.getSkillType(skillName, scRepair));

	//repaired units
	const XmlNode *unitsNode= n->getChild("repaired-units");
	for(int i=0; i<unitsNode->getChildCount(); ++i){
		const XmlNode *unitNode= unitsNode->getChild("unit", i);
		repairableUnits.push_back(ft->getUnitType(unitNode->getAttribute("name")->getRestrictedValue()));
	}
}

string RepairCommandType::getDesc(const TotalUpgrade *totalUpgrade) const{
    Lang &lang= Lang::getInstance();
	string str;

    str= name+"\n";
    str+= lang.get("RepairSpeed")+": "+ intToStr(repairSkillType->getSpeed())+"\n";
	if(repairSkillType->getEpCost()!=0){
        str+= lang.get("EpCost")+": "+intToStr(repairSkillType->getEpCost())+"\n";
	}
    
    str+="\n"+lang.get("CanRepair")+":\n";
    for(int i=0; i<repairableUnits.size(); ++i){
        str+= (static_cast<const UnitType*>(repairableUnits[i]))->getName()+"\n";
    }

    return str;
}

string RepairCommandType::toString() const{
	Lang &lang= Lang::getInstance();
	return lang.get("Repair");
}

//get
bool RepairCommandType::isRepairableUnitType(const UnitType *unitType) const{
    for(int i=0; i<repairableUnits.size(); ++i){
		if(static_cast<const UnitType*>(repairableUnits[i])==unitType){
            return true;
		}
	}
    return false;
}

// =====================================================
// 	class ProduceCommandType
// =====================================================

//varios
ProduceCommandType::ProduceCommandType(){
    commandTypeClass= ccProduce;
    clicks= cOne;
}

void ProduceCommandType::update(UnitUpdater *unitUpdater, Unit *unit) const{
	unitUpdater->updateProduce(unit);
}

void ProduceCommandType::load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut){
    CommandType::load(id, n, dir, tt, ft, ut);

	//produce
   	string skillName= n->getChild("produce-skill")->getAttribute("value")->getRestrictedValue();
	produceSkillType= static_cast<const ProduceSkillType*>(ut.getSkillType(skillName, scProduce));

    string producedUnitName= n->getChild("produced-unit")->getAttribute("name")->getRestrictedValue();   
	producedUnit= ft->getUnitType(producedUnitName);
}

string ProduceCommandType::getDesc(const TotalUpgrade *totalUpgrade) const{
    string str= name+"\n";
	Lang &lang= Lang::getInstance();
    
    //prod speed
    str+= lang.get("ProductionSpeed")+": "+ intToStr(produceSkillType->getSpeed());
	if(totalUpgrade->getProdSpeed()!=0){
        str+="+" + intToStr(totalUpgrade->getProdSpeed());
	}
    str+="\n";

    //mpcost
	if(produceSkillType->getEpCost()!=0){
        str+= lang.get("EpCost")+": "+intToStr(produceSkillType->getEpCost())+"\n";
	}
    
    str+= "\n" + getProducedUnit()->getReqDesc();

    return str;
}

string ProduceCommandType::toString() const{
	Lang &lang= Lang::getInstance();
	return lang.get("Produce");
}

string ProduceCommandType::getReqDesc() const{
    return RequirableType::getReqDesc()+"\n"+getProducedUnit()->getReqDesc();
}

const ProducibleType *ProduceCommandType::getProduced() const{
	return producedUnit;
}

// =====================================================
// 	class UpgradeCommandType
// =====================================================

//varios
UpgradeCommandType::UpgradeCommandType(){
    commandTypeClass= ccUpgrade;
    clicks= cOne;
}

void UpgradeCommandType::update(UnitUpdater *unitUpdater, Unit *unit) const{
	unitUpdater->updateUpgrade(unit);
}

void UpgradeCommandType::load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut){
    
	CommandType::load(id, n, dir, tt, ft, ut);

	//upgrade
   	string skillName= n->getChild("upgrade-skill")->getAttribute("value")->getRestrictedValue();
	upgradeSkillType= static_cast<const UpgradeSkillType*>(ut.getSkillType(skillName, scUpgrade));

    string producedUpgradeName= n->getChild("produced-upgrade")->getAttribute("name")->getRestrictedValue();   
	producedUpgrade= ft->getUpgradeType(producedUpgradeName);
	
}

string UpgradeCommandType::getDesc(const TotalUpgrade *totalUpgrade) const{
    string str;
	Lang &lang= Lang::getInstance();

    str= name+"\n";
    str+= lang.get("UpgradeSpeed")+": "+ intToStr(upgradeSkillType->getSpeed())+"\n";
    if(upgradeSkillType->getEpCost()!=0)
        str+= lang.get("EpCost")+": "+intToStr(upgradeSkillType->getEpCost())+"\n";
    
    str+= "\n"+getProducedUpgrade()->getReqDesc();

    return str;
}

string UpgradeCommandType::toString() const{
	Lang &lang= Lang::getInstance();
	return lang.get("Upgrade");
}

string UpgradeCommandType::getReqDesc() const{
    return RequirableType::getReqDesc()+"\n"+getProducedUpgrade()->getReqDesc();
}

const ProducibleType *UpgradeCommandType::getProduced() const{
	return producedUpgrade;
}

// =====================================================
// 	class MorphCommandType
// =====================================================

//varios
MorphCommandType::MorphCommandType(){
    commandTypeClass= ccMorph;
    clicks= cOne;
}

void MorphCommandType::update(UnitUpdater *unitUpdater, Unit *unit) const{
	unitUpdater->updateMorph(unit);
}

void MorphCommandType::load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut){
    CommandType::load(id, n, dir, tt, ft, ut);

	//morph skill
   	string skillName= n->getChild("morph-skill")->getAttribute("value")->getRestrictedValue();
	morphSkillType= static_cast<const MorphSkillType*>(ut.getSkillType(skillName, scMorph));

    //morph unit
   	string morphUnitName= n->getChild("morph-unit")->getAttribute("name")->getRestrictedValue();   
	morphUnit= ft->getUnitType(morphUnitName);

    //discount
	discount= n->getChild("discount")->getAttribute("value")->getIntValue();   
}

string MorphCommandType::getDesc(const TotalUpgrade *totalUpgrade) const{
    string str= name+"\n";
	Lang &lang= Lang::getInstance();
    
    //prod speed
    str+= lang.get("MorphSpeed")+": "+ intToStr(morphSkillType->getSpeed())+"\n";

    //mpcost
	if(morphSkillType->getEpCost()!=0){
        str+= lang.get("EpCost")+": "+intToStr(morphSkillType->getEpCost())+"\n";
	}

    //discount
	if(discount!=0){
        str+= lang.get("Discount")+": "+intToStr(discount)+"%\n";
	}
    
    str+= "\n"+getProduced()->getReqDesc();

    return str;
}

string MorphCommandType::toString() const{
	Lang &lang= Lang::getInstance();
	return lang.get("Morph");
}

string MorphCommandType::getReqDesc() const{
    return RequirableType::getReqDesc() + "\n" + getProduced()->getReqDesc();
}

const ProducibleType *MorphCommandType::getProduced() const{
	return morphUnit;
}

// =====================================================
// 	class CommandFactory
// =====================================================

CommandTypeFactory::CommandTypeFactory(){
	registerClass<StopCommandType>("stop");
	registerClass<MoveCommandType>("move");
	registerClass<AttackCommandType>("attack");
	registerClass<AttackStoppedCommandType>("attack_stopped");
	registerClass<BuildCommandType>("build");
	registerClass<HarvestCommandType>("harvest");
	registerClass<RepairCommandType>("repair");
	registerClass<ProduceCommandType>("produce");
	registerClass<UpgradeCommandType>("upgrade");
	registerClass<MorphCommandType>("morph");
}

CommandTypeFactory &CommandTypeFactory::getInstance(){
	static CommandTypeFactory ctf;
	return ctf;
}

}}//end namespace
