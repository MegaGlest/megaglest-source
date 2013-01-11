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
#include "socket.h"

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

void CommandType::load(int id, const XmlNode *n, const string &dir,
		const TechTree *tt, const FactionType *ft, const UnitType &ut,
		std::map<string,vector<pair<string, string> > > &loadedFileList,
		string parentLoader) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	this->id= id;
	name= n->getChild("name")->getAttribute("value")->getRestrictedValue();

	//image
	const XmlNode *imageNode= n->getChild("image");
	image= Renderer::getInstance().newTexture2D(rsGame);

	string currentPath = dir;
	endPathWithSlash(currentPath);
	if(image) {
		image->load(imageNode->getAttribute("path")->getRestrictedValue(currentPath));
	}
	loadedFileList[imageNode->getAttribute("path")->getRestrictedValue(currentPath)].push_back(make_pair(parentLoader,imageNode->getAttribute("path")->getRestrictedValue()));

	//unit requirements
	const XmlNode *unitRequirementsNode= n->getChild("unit-requirements");
	for(int i = 0; i < unitRequirementsNode->getChildCount(); ++i) {
		const XmlNode *unitNode= 	unitRequirementsNode->getChild("unit", i);
		string name= unitNode->getAttribute("name")->getRestrictedValue();
		unitReqs.push_back(ft->getUnitType(name));
	}

	//upgrade requirements
	const XmlNode *upgradeRequirementsNode= n->getChild("upgrade-requirements");
	for(int i = 0; i < upgradeRequirementsNode->getChildCount(); ++i) {
		const XmlNode *upgradeReqNode= upgradeRequirementsNode->getChild("upgrade", i);
		string name= upgradeReqNode->getAttribute("name")->getRestrictedValue();
		upgradeReqs.push_back(ft->getUpgradeType(name));
	}

	//fog of war
	if(n->hasChild("fog-of-war-skill") == true) {
		string skillName= n->getChild("fog-of-war-skill")->getAttribute("value")->getRestrictedValue();
		fogOfWarSkillType = static_cast<const FogOfWarSkillType*>(ut.getSkillType(skillName, scFogOfWar));

		string skillAttachmentNames = n->getChild("fog-of-war-skill")->getAttribute("skill-attachments")->getValue();

		std::vector<std::string> skillList;
		Tokenize(skillAttachmentNames,skillList,",");
		for(unsigned int i = 0; i < skillList.size(); ++i) {
			string skillAttachName = skillList[i];
			fogOfWarSkillAttachments[skillAttachName] = true;
		}
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

bool CommandType::hasFogOfWarSkillType(string name) const {
	std::map<string,bool>::const_iterator iterFind = fogOfWarSkillAttachments.find(name);
	bool result = (iterFind != fogOfWarSkillAttachments.end());
	return result;
}

// =====================================================
// 	class StopCommandType
// =====================================================

//varios
StopCommandType::StopCommandType(){
    commandTypeClass= ccStop;
    clicks= cOne;
    stopSkillType=NULL;
}

void StopCommandType::update(UnitUpdater *unitUpdater, Unit *unit, int frameIndex) const {
	unitUpdater->updateStop(unit, frameIndex);
}

string StopCommandType::getDesc(const TotalUpgrade *totalUpgrade) const{
    string str;
	Lang &lang= Lang::getInstance();

    str= name+"\n";
	str+= lang.get("ReactionSpeed")+": "+ intToStr(stopSkillType->getSpeed())+"\n";
    if(stopSkillType->getEpCost()!=0)
        str+= lang.get("EpCost")+": "+intToStr(stopSkillType->getEpCost())+"\n";
 if(stopSkillType->getHpCost()!=0)
        str+= lang.get("HpCost")+": "+intToStr(stopSkillType->getHpCost())+"\n";
	str+=stopSkillType->getBoostDesc();
    return str;
}

string StopCommandType::toString() const{
	Lang &lang= Lang::getInstance();
	return lang.get("Stop");
}

void StopCommandType::load(int id, const XmlNode *n, const string &dir,
		const TechTree *tt, const FactionType *ft, const UnitType &ut,
		std::map<string,vector<pair<string, string> > > &loadedFileList,string parentLoader) {
	CommandType::load(id, n, dir, tt, ft, ut, loadedFileList,parentLoader);

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
    moveSkillType=NULL;
}

void MoveCommandType::update(UnitUpdater *unitUpdater, Unit *unit, int frameIndex) const {
	unitUpdater->updateMove(unit, frameIndex);
}

void MoveCommandType::load(int id, const XmlNode *n, const string &dir,
		const TechTree *tt, const FactionType *ft, const UnitType &ut,
		std::map<string,vector<pair<string, string> > > &loadedFileList,
		string parentLoader) {
    CommandType::load(id, n, dir, tt, ft, ut, loadedFileList,parentLoader);

	//move
   	string skillName= n->getChild("move-skill")->getAttribute("value")->getRestrictedValue();
	moveSkillType= static_cast<const MoveSkillType*>(ut.getSkillType(skillName, scMove));
}

string MoveCommandType::getDesc(const TotalUpgrade *totalUpgrade) const{
    string str;
	Lang &lang= Lang::getInstance();

    str= name+"\n";
    str+= lang.get("WalkSpeed")+": "+ intToStr(moveSkillType->getSpeed());
	if(totalUpgrade->getMoveSpeed(moveSkillType) != 0) {
        str+= "+" + intToStr(totalUpgrade->getMoveSpeed(moveSkillType));
	}
    str+="\n";
	if(moveSkillType->getEpCost()!=0){
        str+= lang.get("EpCost")+": "+intToStr(moveSkillType->getEpCost())+"\n";
	}
		if(moveSkillType->getHpCost()!=0){
        str+= lang.get("HpCost")+": "+intToStr(moveSkillType->getHpCost())+"\n";
	}
	str+=moveSkillType->getBoostDesc();
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
    moveSkillType=NULL;
    attackSkillType=NULL;
}

void AttackCommandType::update(UnitUpdater *unitUpdater, Unit *unit, int frameIndex) const {
	unitUpdater->updateAttack(unit, frameIndex);
}

void AttackCommandType::load(int id, const XmlNode *n, const string &dir,
		const TechTree *tt, const FactionType *ft, const UnitType &ut,
		std::map<string,vector<pair<string, string> > > &loadedFileList,
		string parentLoader) {
    CommandType::load(id, n, dir, tt, ft, ut, loadedFileList,parentLoader);

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
		if(attackSkillType->getHpCost()!=0){
        str+= lang.get("HpCost") + ": " + intToStr(attackSkillType->getHpCost()) + "\n";
	}

    //attack strength
    str+= lang.get("AttackStrenght")+": ";
    str+= intToStr(attackSkillType->getAttackStrength()-attackSkillType->getAttackVar());
    str+= "...";
    str+= intToStr(attackSkillType->getAttackStrength()+attackSkillType->getAttackVar());
	if(totalUpgrade->getAttackStrength(attackSkillType) != 0) {
        str+= "+"+intToStr(totalUpgrade->getAttackStrength(attackSkillType));
	}
	str+= " ("+ attackSkillType->getAttackType()->getName() +")";
    str+= "\n";

    //splash radius
	if(attackSkillType->getSplashRadius()!=0){
        str+= lang.get("SplashRadius")+": "+intToStr(attackSkillType->getSplashRadius())+"\n";
	}

    //attack distance
    str+= lang.get("AttackDistance")+": "+intToStr(attackSkillType->getAttackRange());
	if(totalUpgrade->getAttackRange(attackSkillType) != 0) {
        str+= "+"+intToStr(totalUpgrade->getAttackRange(attackSkillType) != 0);
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
	if(totalUpgrade->getMoveSpeed(moveSkillType) != 0) {
        str+= "+"+intToStr(totalUpgrade->getMoveSpeed(moveSkillType));
	}
    str+="\n";

    str+= lang.get("AttackSpeed")+": "+ intToStr(attackSkillType->getSpeed()) +"\n";
	str+=attackSkillType->getBoostDesc();
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
    stopSkillType=NULL;
    attackSkillType=NULL;
}

void AttackStoppedCommandType::update(UnitUpdater *unitUpdater, Unit *unit, int frameIndex) const {
	unitUpdater->updateAttackStopped(unit, frameIndex);
}

void AttackStoppedCommandType::load(int id, const XmlNode *n, const string &dir,
		const TechTree *tt, const FactionType *ft, const UnitType &ut,
		std::map<string,vector<pair<string, string> > > &loadedFileList,string parentLoader) {
    CommandType::load(id, n, dir, tt, ft, ut, loadedFileList,parentLoader);

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
		if(attackSkillType->getHpCost()!=0){
        str+= lang.get("HpCost")+": "+intToStr(attackSkillType->getHpCost())+"\n";
	}

    //attack strength
    str+= lang.get("AttackStrenght")+": ";
    str+= intToStr(attackSkillType->getAttackStrength()-attackSkillType->getAttackVar());
    str+="...";
    str+= intToStr(attackSkillType->getAttackStrength()+attackSkillType->getAttackVar());
    if(totalUpgrade->getAttackStrength(attackSkillType) != 0)
        str+= "+"+intToStr(totalUpgrade->getAttackStrength(attackSkillType));
    str+= " ("+ attackSkillType->getAttackType()->getName() +")";
    str+="\n";

    //splash radius
	if(attackSkillType->getSplashRadius()!=0){
        str+= lang.get("SplashRadius")+": "+intToStr(attackSkillType->getSplashRadius())+"\n";
	}

    //attack distance
    str+= lang.get("AttackDistance")+": "+intToStr(attackSkillType->getAttackRange());
	if(totalUpgrade->getAttackRange(attackSkillType) != 0) {
        str+= "+"+intToStr(totalUpgrade->getAttackRange(attackSkillType) != 0);
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
	str+=attackSkillType->getBoostDesc();
    return str;
}

string AttackStoppedCommandType::toString() const {
	Lang &lang= Lang::getInstance();
	return lang.get("AttackStopped");
}


// =====================================================
// 	class BuildCommandType
// =====================================================

//varios
BuildCommandType::BuildCommandType() {
    commandTypeClass= ccBuild;
    clicks= cTwo;
    moveSkillType=NULL;
    buildSkillType=NULL;
}

BuildCommandType::~BuildCommandType() {
	deleteValues(builtSounds.getSounds().begin(), builtSounds.getSounds().end());
	deleteValues(startSounds.getSounds().begin(), startSounds.getSounds().end());
}

void BuildCommandType::update(UnitUpdater *unitUpdater, Unit *unit, int frameIndex) const {
	unitUpdater->updateBuild(unit, frameIndex);
}

void BuildCommandType::load(int id, const XmlNode *n, const string &dir,
		const TechTree *tt, const FactionType *ft, const UnitType &ut,
		std::map<string,vector<pair<string, string> > > &loadedFileList, string parentLoader) {
    CommandType::load(id, n, dir, tt, ft, ut, loadedFileList, parentLoader);

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
			string currentPath = dir;
			endPathWithSlash(currentPath);
			string path= soundFileNode->getAttribute("path")->getRestrictedValue(currentPath,true);

			StaticSound *sound= new StaticSound();

			sound->load(path);
			loadedFileList[path].push_back(make_pair(parentLoader,soundFileNode->getAttribute("path")->getRestrictedValue()));
			startSounds[i]= sound;
		}
	}

	//built sound
	const XmlNode *builtSoundNode= n->getChild("built-sound");
	if(builtSoundNode->getAttribute("enabled")->getBoolValue()){
		builtSounds.resize(builtSoundNode->getChildCount());
		for(int i=0; i<builtSoundNode->getChildCount(); ++i){
			const XmlNode *soundFileNode= builtSoundNode->getChild("sound-file", i);
			string currentPath = dir;
			endPathWithSlash(currentPath);
			string path= soundFileNode->getAttribute("path")->getRestrictedValue(currentPath,true);

			StaticSound *sound= new StaticSound();

			sound->load(path);
			loadedFileList[path].push_back(make_pair(parentLoader,soundFileNode->getAttribute("path")->getRestrictedValue()));
			builtSounds[i]= sound;
		}
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

string BuildCommandType::getDesc(const TotalUpgrade *totalUpgrade) const{
    string str;
	Lang &lang= Lang::getInstance();

    str= name+"\n";
    str+= lang.get("BuildSpeed")+": "+ intToStr(buildSkillType->getSpeed())+"\n";
	if(buildSkillType->getEpCost()!=0){
        str+= lang.get("EpCost")+": "+intToStr(buildSkillType->getEpCost())+"\n";
	}
		if(buildSkillType->getHpCost()!=0){
        str+= lang.get("HpCost")+": "+intToStr(buildSkillType->getHpCost())+"\n";
	}
	str+=buildSkillType->getBoostDesc();
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
    moveSkillType=NULL;
    moveLoadedSkillType=NULL;
    harvestSkillType=NULL;
    stopLoadedSkillType=NULL;
    maxLoad=0;
    hitsPerUnit=0;
}

void HarvestCommandType::update(UnitUpdater *unitUpdater, Unit *unit, int frameIndex) const {
	unitUpdater->updateHarvest(unit, frameIndex);
}

void HarvestCommandType::load(int id, const XmlNode *n, const string &dir,
		const TechTree *tt, const FactionType *ft, const UnitType &ut,
		std::map<string,vector<pair<string, string> > > &loadedFileList, string parentLoader) {
	CommandType::load(id, n, dir, tt, ft, ut, loadedFileList, parentLoader);

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
	if(harvestSkillType->getHpCost()!=0){
        str+= lang.get("HpCost")+": "+intToStr(harvestSkillType->getHpCost())+"\n";
	}
	str+=lang.get("Resources")+":\n";
	for(int i=0; i<getHarvestedResourceCount(); ++i){
		str+= getHarvestedResource(i)->getName(true)+"\n";
	}
	str+=harvestSkillType->getBoostDesc();
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
    moveSkillType=NULL;
    repairSkillType=NULL;
}

RepairCommandType::~RepairCommandType(){
}

void RepairCommandType::update(UnitUpdater *unitUpdater, Unit *unit, int frameIndex) const {
	unitUpdater->updateRepair(unit, frameIndex);
}

void RepairCommandType::load(int id, const XmlNode *n, const string &dir,
		const TechTree *tt, const FactionType *ft, const UnitType &ut,
		std::map<string,vector<pair<string, string> > > &loadedFileList, string parentLoader) {
	CommandType::load(id, n, dir, tt, ft, ut, loadedFileList, parentLoader);

	// move skill (no longer required by means unit must already be beside unit to repair)
	// for example a hospital
	if(n->hasChild("move-skill") == true) {
		string skillName= n->getChild("move-skill")->getAttribute("value")->getRestrictedValue();
		moveSkillType= static_cast<const MoveSkillType*>(ut.getSkillType(skillName, scMove));
	}

	//repair
   	string skillName= n->getChild("repair-skill")->getAttribute("value")->getRestrictedValue();
	repairSkillType= static_cast<const RepairSkillType*>(ut.getSkillType(skillName, scRepair));

	//repaired units
	const XmlNode *unitsNode= n->getChild("repaired-units");
	for(int i=0; i<unitsNode->getChildCount(); ++i){
		const XmlNode *unitNode= unitsNode->getChild("unit", i);
		repairableUnits.push_back(ft->getUnitType(unitNode->getAttribute("name")->getRestrictedValue()));
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

string RepairCommandType::getDesc(const TotalUpgrade *totalUpgrade) const{
    Lang &lang= Lang::getInstance();
	string str;

    str= name+"\n";
    str+= lang.get("RepairSpeed")+": "+ intToStr(repairSkillType->getSpeed())+"\n";
	if(repairSkillType->getEpCost()!=0){
        str+= lang.get("EpCost")+": "+intToStr(repairSkillType->getEpCost())+"\n";
	}
	if(repairSkillType->getHpCost()!=0){
        str+= lang.get("HpCost")+": "+intToStr(repairSkillType->getHpCost())+"\n";
	}
    str+="\n"+lang.get("CanRepair")+":\n";
    for(int i=0; i<repairableUnits.size(); ++i){
        str+= (static_cast<const UnitType*>(repairableUnits[i]))->getName(true)+"\n";
    }
	str+=repairSkillType->getBoostDesc();
    return str;
}

string RepairCommandType::toString() const{
	Lang &lang= Lang::getInstance();
	return lang.get("Repair");
}

//get
bool RepairCommandType::isRepairableUnitType(const UnitType *unitType) const {
	for(int i = 0; i < repairableUnits.size(); ++i) {
		const UnitType *curUnitType = static_cast<const UnitType*>(repairableUnits[i]);
		//printf("Lookup index = %d Can repair unittype [%s][%p] looking for [%s][%p] lookup found result = %d\n",i,curUnitType->getName().c_str(),curUnitType,unitType->getName().c_str(),unitType,(curUnitType == unitType));
		if(curUnitType == unitType) {
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
    produceSkillType=NULL;
    producedUnit=NULL;
}

void ProduceCommandType::update(UnitUpdater *unitUpdater, Unit *unit, int frameIndex) const {
	unitUpdater->updateProduce(unit, frameIndex);
}

void ProduceCommandType::load(int id, const XmlNode *n, const string &dir,
		const TechTree *tt, const FactionType *ft, const UnitType &ut,
		std::map<string,vector<pair<string, string> > > &loadedFileList, string parentLoader) {
	CommandType::load(id, n, dir, tt, ft, ut, loadedFileList, parentLoader);

	//produce
   	string skillName= n->getChild("produce-skill")->getAttribute("value")->getRestrictedValue();
	produceSkillType= static_cast<const ProduceSkillType*>(ut.getSkillType(skillName, scProduce));

    string producedUnitName= n->getChild("produced-unit")->getAttribute("name")->getRestrictedValue();
	producedUnit= ft->getUnitType(producedUnitName);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

string ProduceCommandType::getDesc(const TotalUpgrade *totalUpgrade) const{
    string str= name+"\n";
	Lang &lang= Lang::getInstance();

    //prod speed
    str+= lang.get("ProductionSpeed")+": "+ intToStr(produceSkillType->getSpeed());
	if(totalUpgrade->getProdSpeed(produceSkillType)!=0){
        str+="+" + intToStr(totalUpgrade->getProdSpeed(produceSkillType));
	}
    str+="\n";

    //mpcost
	if(produceSkillType->getEpCost()!=0){
        str+= lang.get("EpCost")+": "+intToStr(produceSkillType->getEpCost())+"\n";
	}
	if(produceSkillType->getHpCost()!=0){
        str+= lang.get("hpCost")+": "+intToStr(produceSkillType->getHpCost())+"\n";
	}
    str+= "\n" + getProducedUnit()->getReqDesc();
	str+=produceSkillType->getBoostDesc();
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
    upgradeSkillType=NULL;
    producedUpgrade=NULL;
}

void UpgradeCommandType::update(UnitUpdater *unitUpdater, Unit *unit, int frameIndex) const {
	unitUpdater->updateUpgrade(unit, frameIndex);
}

void UpgradeCommandType::load(int id, const XmlNode *n, const string &dir,
		const TechTree *tt, const FactionType *ft, const UnitType &ut,
		std::map<string,vector<pair<string, string> > > &loadedFileList, string parentLoader) {

	CommandType::load(id, n, dir, tt, ft, ut, loadedFileList, parentLoader);

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
if(upgradeSkillType->getHpCost()!=0)
        str+= lang.get("HpCost")+": "+intToStr(upgradeSkillType->getHpCost())+"\n";
    str+= "\n"+getProducedUpgrade()->getReqDesc();
	str+=upgradeSkillType->getBoostDesc();
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
    morphSkillType=NULL;
    morphUnit=NULL;
    discount=0;
    ignoreResourceRequirements=false;
}

void MorphCommandType::update(UnitUpdater *unitUpdater, Unit *unit, int frameIndex) const {
	unitUpdater->updateMorph(unit, frameIndex);
}

void MorphCommandType::load(int id, const XmlNode *n, const string &dir,
		const TechTree *tt, const FactionType *ft, const UnitType &ut,
		std::map<string,vector<pair<string, string> > > &loadedFileList, string parentLoader) {
	CommandType::load(id, n, dir, tt, ft, ut, loadedFileList, parentLoader);

	//morph skill
   	string skillName= n->getChild("morph-skill")->getAttribute("value")->getRestrictedValue();
	morphSkillType= static_cast<const MorphSkillType*>(ut.getSkillType(skillName, scMorph));

    //morph unit
   	string morphUnitName= n->getChild("morph-unit")->getAttribute("name")->getRestrictedValue();
	morphUnit= ft->getUnitType(morphUnitName);

    //discount
	discount= n->getChild("discount")->getAttribute("value")->getIntValue();

	ignoreResourceRequirements = false;
	if(n->hasChild("ignore-resource-requirements") == true) {
		ignoreResourceRequirements= n->getChild("ignore-resource-requirements")->getAttribute("value")->getBoolValue();

		//printf("ignoreResourceRequirements = %d\n",ignoreResourceRequirements);
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
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
		if(morphSkillType->getHpCost()!=0){
        str+= lang.get("HpCost")+": "+intToStr(morphSkillType->getHpCost())+"\n";
	}

    //discount
	if(discount!=0){
        str+= lang.get("Discount")+": "+intToStr(discount)+"%\n";
	}

    str+= "\n"+getProduced()->getReqDesc(ignoreResourceRequirements);
	str+=morphSkillType->getBoostDesc();

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
// 	class SwitchTeamCommandType
// =====================================================

//varios
SwitchTeamCommandType::SwitchTeamCommandType(){
    commandTypeClass= ccSwitchTeam;
    clicks= cOne;
}

void SwitchTeamCommandType::update(UnitUpdater *unitUpdater, Unit *unit, int frameIndex) const {
	unitUpdater->updateSwitchTeam(unit, frameIndex);
}

void SwitchTeamCommandType::load(int id, const XmlNode *n, const string &dir,
		const TechTree *tt, const FactionType *ft, const UnitType &ut,
		std::map<string,vector<pair<string, string> > > &loadedFileList, string parentLoader) {
	CommandType::load(id, n, dir, tt, ft, ut, loadedFileList, parentLoader);

	//morph skill
   	//string skillName= n->getChild("morph-skill")->getAttribute("value")->getRestrictedValue();
	//morphSkillType= static_cast<const MorphSkillType*>(ut.getSkillType(skillName, scMorph));

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

string SwitchTeamCommandType::getDesc(const TotalUpgrade *totalUpgrade) const{
    string str= name+"\n";
	//Lang &lang= Lang::getInstance();

    //prod speed
    //str+= lang.get("MorphSpeed")+": "+ intToStr(morphSkillType->getSpeed())+"\n";

    //mpcost
//	if(morphSkillType->getEpCost()!=0){
//        str+= lang.get("EpCost")+": "+intToStr(morphSkillType->getEpCost())+"\n";
//	}
//		if(morphSkillType->getHpCost()!=0){
//        str+= lang.get("HpCost")+": "+intToStr(morphSkillType->getHpCost())+"\n";
//	}
//
//    //discount
//	if(discount!=0){
//        str+= lang.get("Discount")+": "+intToStr(discount)+"%\n";
//	}
//
//    str+= "\n"+getProduced()->getReqDesc();

    return str;
}

string SwitchTeamCommandType::toString() const{
	Lang &lang= Lang::getInstance();
	return lang.get("SwitchTeam");
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
	registerClass<SwitchTeamCommandType>("switch_team");
}

CommandTypeFactory &CommandTypeFactory::getInstance(){
	static CommandTypeFactory ctf;
	return ctf;
}

}}//end namespace
