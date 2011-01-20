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

#include "unit_type.h"

#include <cassert>

#include "util.h"
#include "upgrade_type.h"
#include "resource_type.h"
#include "sound.h"
#include "logger.h"
#include "xml_parser.h"
#include "tech_tree.h"
#include "resource.h"
#include "renderer.h"
#include "game_util.h"
#include "unit_particle_type.h"
#include "faction.h"
#include "leak_dumper.h"

using namespace Shared::Xml;
using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest{ namespace Game{

// ===============================
// 	class Level
// ===============================

void Level::init(string name, int kills){
	this->name= name;
	this->kills= kills;
}

// =====================================================
// 	class UnitType
// =====================================================

// ===================== PUBLIC ========================

const char *UnitType::propertyNames[]= {"burnable", "rotated_climb"};

// ==================== creation and loading ====================

UnitType::UnitType() : ProducibleType() {

	meetingPointImage = NULL;
    lightColor= Vec3f(0.f);
    light= false;
    multiSelect= false;
	armorType= NULL;
	rotatedBuildPos=0;

    for(int i=0; i<ccCount; ++i){
        firstCommandTypeOfClass[i]= NULL;
    }

    for(int i=0; i<scCount; ++i){
    	firstSkillTypeOfClass[i] = NULL;
    }

	for(int i=0; i<pCount; ++i){
		properties[i]= false;
	}

	for(int i=0; i<fieldCount; ++i){
		fields[i]= false;
	}

	cellMap= NULL;
	allowEmptyCellMap=false;
	hpRegeneration= 0;
	epRegeneration= 0;
	maxUnitCount= 0;
	maxHp=0;
	maxEp=0;
	armor=0;
	sight=0;
	size=0;
	height=0;

	addItemToVault(&(this->maxHp),this->maxHp);
	addItemToVault(&(this->hpRegeneration),this->hpRegeneration);
	addItemToVault(&(this->maxEp),this->maxEp);
	addItemToVault(&(this->epRegeneration),this->epRegeneration);
	addItemToVault(&(this->maxUnitCount),this->maxUnitCount);
	addItemToVault(&(this->armor),this->armor);
	addItemToVault(&(this->sight),this->sight);
	addItemToVault(&(this->size),this->size);
	addItemToVault(&(this->height),this->height);
}

UnitType::~UnitType(){
	deleteValues(commandTypes.begin(), commandTypes.end());
	deleteValues(skillTypes.begin(), skillTypes.end());
	deleteValues(selectionSounds.getSounds().begin(), selectionSounds.getSounds().end());
	deleteValues(commandSounds.getSounds().begin(), commandSounds.getSounds().end());
	delete [] cellMap;
	//remove damageParticleSystemTypes
	while(!damageParticleSystemTypes.empty()){
		delete damageParticleSystemTypes.back();
		damageParticleSystemTypes.pop_back();
	}
}

void UnitType::preLoad(const string &dir){
	name= lastDir(dir);
}

void UnitType::load(int id,const string &dir, const TechTree *techTree, const FactionType *factionType, Checksum* checksum,Checksum* techtreeChecksum) {

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	string path = dir + "/" + name + ".xml";

	this->id= id;

	try{

		Logger::getInstance().add("Unit type: " + formatString(name), true);

		//file load
		checksum->addFile(path);
		techtreeChecksum->addFile(path);

		XmlTree xmlTree;
		xmlTree.load(path);
		const XmlNode *unitNode= xmlTree.getRootNode();

		const XmlNode *parametersNode= unitNode->getChild("parameters");

		//size
		//checkItemInVault(&(this->size),this->size);
		size= parametersNode->getChild("size")->getAttribute("value")->getIntValue();
		addItemToVault(&(this->size),this->size);

		//height
		//checkItemInVault(&(this->height),this->height);
		height= parametersNode->getChild("height")->getAttribute("value")->getIntValue();
		addItemToVault(&(this->height),this->height);

		//maxHp
		//checkItemInVault(&(this->maxHp),this->maxHp);
		maxHp = parametersNode->getChild("max-hp")->getAttribute("value")->getIntValue();
		addItemToVault(&(this->maxHp),this->maxHp);

		//hpRegeneration
		//checkItemInVault(&(this->hpRegeneration),this->hpRegeneration);
		hpRegeneration= parametersNode->getChild("max-hp")->getAttribute("regeneration")->getIntValue();
		addItemToVault(&(this->hpRegeneration),this->hpRegeneration);

		//maxEp
		//checkItemInVault(&(this->maxEp),this->maxEp);
		maxEp= parametersNode->getChild("max-ep")->getAttribute("value")->getIntValue();
		addItemToVault(&(this->maxEp),this->maxEp);

		if(maxEp != 0) {
			//epRegeneration
			//checkItemInVault(&(this->epRegeneration),this->epRegeneration);
			epRegeneration= parametersNode->getChild("max-ep")->getAttribute("regeneration")->getIntValue();
		}
		addItemToVault(&(this->epRegeneration),this->epRegeneration);

		//maxUnitCount
		if(parametersNode->hasChild("max-unit-count")) {
			//checkItemInVault(&(this->maxUnitCount),this->maxUnitCount);
			maxUnitCount= parametersNode->getChild("max-unit-count")->getAttribute("value")->getIntValue();
		}
		addItemToVault(&(this->maxUnitCount),this->maxUnitCount);

		//armor
		//checkItemInVault(&(this->armor),this->armor);
		armor= parametersNode->getChild("armor")->getAttribute("value")->getIntValue();
		addItemToVault(&(this->armor),this->armor);

		//armor type string
		string armorTypeName= parametersNode->getChild("armor-type")->getAttribute("value")->getRestrictedValue();
		armorType= techTree->getArmorType(armorTypeName);

		//sight
		//checkItemInVault(&(this->sight),this->sight);
		sight= parametersNode->getChild("sight")->getAttribute("value")->getIntValue();
		addItemToVault(&(this->sight),this->sight);

		//prod time
		productionTime= parametersNode->getChild("time")->getAttribute("value")->getIntValue();

		//multi selection
		multiSelect= parametersNode->getChild("multi-selection")->getAttribute("value")->getBoolValue();

		//cellmap
		allowEmptyCellMap = false;
		const XmlNode *cellMapNode= parametersNode->getChild("cellmap");
		bool hasCellMap= cellMapNode->getAttribute("value")->getBoolValue();
		if(hasCellMap == true) {
			if(cellMapNode->getAttribute("allowEmpty",false) != NULL) {
				allowEmptyCellMap = cellMapNode->getAttribute("allowEmpty")->getBoolValue();
			}

			cellMap= new bool[size*size];
			for(int i=0; i<size; ++i){
				const XmlNode *rowNode= cellMapNode->getChild("row", i);
				string row= rowNode->getAttribute("value")->getRestrictedValue();
				if(row.size()!=size){
					throw runtime_error("Cellmap row has not the same length as unit size");
				}
				for(int j=0; j<row.size(); ++j){
					cellMap[i*size+j]= row[j]=='0'? false: true;
				}
			}
		}

		//levels
		const XmlNode *levelsNode= parametersNode->getChild("levels");
		levels.resize(levelsNode->getChildCount());
		for(int i=0; i<levels.size(); ++i){
			const XmlNode *levelNode= levelsNode->getChild("level", i);
			levels[i].init(
				levelNode->getAttribute("name")->getRestrictedValue(),
				levelNode->getAttribute("kills")->getIntValue());
		}

		//fields
		const XmlNode *fieldsNode= parametersNode->getChild("fields");
		for(int i=0; i<fieldsNode->getChildCount(); ++i){
			const XmlNode *fieldNode= fieldsNode->getChild("field", i);
			string fieldName= fieldNode->getAttribute("value")->getRestrictedValue();
			if(fieldName=="land"){
				fields[fLand]= true;
			}
			else if(fieldName=="air"){
				fields[fAir]= true;
			}
			else{
				throw runtime_error("Not a valid field: "+fieldName+": "+ path);
			}
		}

		if (fields[fLand]) {
			field = fLand;
		} else if (fields[fAir]) {
			field = fAir;
		} else {
			throw runtime_error("Unit has no field: " + path);
		}

		//properties
		const XmlNode *propertiesNode= parametersNode->getChild("properties");
		for(int i=0; i<propertiesNode->getChildCount(); ++i){
			const XmlNode *propertyNode= propertiesNode->getChild("property", i);
			string propertyName= propertyNode->getAttribute("value")->getRestrictedValue();
			bool found= false;
			for(int i=0; i<pCount; ++i){
				if(propertyName==propertyNames[i]){
					properties[i]= true;
					found= true;
					break;
				}
			}
			if(!found){
				throw runtime_error("Unknown property: " + propertyName);
			}
		}
		//damage-particles
		if(parametersNode->hasChild("damage-particles")){
			const XmlNode *particleNode= parametersNode->getChild("damage-particles");
			bool particleEnabled= particleNode->getAttribute("value")->getBoolValue();
			if(particleEnabled){
				for(int i=0; i<particleNode->getChildCount(); ++i){
					const XmlNode *particleFileNode= particleNode->getChild("particle-file", i);
					string path= particleFileNode->getAttribute("path")->getRestrictedValue();
					UnitParticleSystemType *unitParticleSystemType= new UnitParticleSystemType();

					//Texture2D *newTexture = Renderer::getInstance().newTexture2D(rsGame);
					Texture2D *newTexture = NULL;

					unitParticleSystemType->load(dir,  dir + "/" + path, &Renderer::getInstance());
					if(unitParticleSystemType->hasTexture() == false) {
						//Renderer::getInstance().endLastTexture(rsGame,true);
					}

					damageParticleSystemTypes.push_back(unitParticleSystemType);
				}
			}
		}

		//light
		const XmlNode *lightNode= parametersNode->getChild("light");
		light= lightNode->getAttribute("enabled")->getBoolValue();
		if(light){
			lightColor.x= lightNode->getAttribute("red")->getFloatValue(0.f, 1.f);
			lightColor.y= lightNode->getAttribute("green")->getFloatValue(0.f, 1.f);
			lightColor.z= lightNode->getAttribute("blue")->getFloatValue(0.f, 1.f);
		}

		//rotationAllowed
		if(parametersNode->hasChild("rotationAllowed")){
			const XmlNode *rotationAllowedNode= parametersNode->getChild("rotationAllowed");
			rotationAllowed= rotationAllowedNode->getAttribute("value")->getBoolValue();
		}
		else
		{
			rotationAllowed=true;
		}


		//unit requirements
		const XmlNode *unitRequirementsNode= parametersNode->getChild("unit-requirements");
		for(int i=0; i<unitRequirementsNode->getChildCount(); ++i){
			const XmlNode *unitNode= 	unitRequirementsNode->getChild("unit", i);
			string name= unitNode->getAttribute("name")->getRestrictedValue();
			unitReqs.push_back(factionType->getUnitType(name));
		}

		//upgrade requirements
		const XmlNode *upgradeRequirementsNode= parametersNode->getChild("upgrade-requirements");
		for(int i=0; i<upgradeRequirementsNode->getChildCount(); ++i){
			const XmlNode *upgradeReqNode= upgradeRequirementsNode->getChild("upgrade", i);
			string name= upgradeReqNode->getAttribute("name")->getRestrictedValue();
			upgradeReqs.push_back(factionType->getUpgradeType(name));
		}

		//resource requirements
		const XmlNode *resourceRequirementsNode= parametersNode->getChild("resource-requirements");
		costs.resize(resourceRequirementsNode->getChildCount());
		for(int i=0; i<costs.size(); ++i){
			const XmlNode *resourceNode= resourceRequirementsNode->getChild("resource", i);
			string name= resourceNode->getAttribute("name")->getRestrictedValue();
			int amount= resourceNode->getAttribute("amount")->getIntValue();
			costs[i].init(techTree->getResourceType(name), amount);
		}

		//resources stored
		const XmlNode *resourcesStoredNode= parametersNode->getChild("resources-stored");
		storedResources.resize(resourcesStoredNode->getChildCount());
		for(int i=0; i<storedResources.size(); ++i){
			const XmlNode *resourceNode= resourcesStoredNode->getChild("resource", i);
			string name= resourceNode->getAttribute("name")->getRestrictedValue();
			int amount= resourceNode->getAttribute("amount")->getIntValue();
			storedResources[i].init(techTree->getResourceType(name), amount);
		}

		//image
		const XmlNode *imageNode= parametersNode->getChild("image");
		image= Renderer::getInstance().newTexture2D(rsGame);
		image->load(dir+"/"+imageNode->getAttribute("path")->getRestrictedValue());

		//image cancel
		const XmlNode *imageCancelNode= parametersNode->getChild("image-cancel");
		cancelImage= Renderer::getInstance().newTexture2D(rsGame);
		cancelImage->load(dir+"/"+imageCancelNode->getAttribute("path")->getRestrictedValue());

		//meeting point
		const XmlNode *meetingPointNode= parametersNode->getChild("meeting-point");
		meetingPoint= meetingPointNode->getAttribute("value")->getBoolValue();
		if(meetingPoint){
			meetingPointImage= Renderer::getInstance().newTexture2D(rsGame);
			meetingPointImage->load(dir+"/"+meetingPointNode->getAttribute("image-path")->getRestrictedValue());
		}

		//selection sounds
		const XmlNode *selectionSoundNode= parametersNode->getChild("selection-sounds");
		if(selectionSoundNode->getAttribute("enabled")->getBoolValue()){
			selectionSounds.resize(selectionSoundNode->getChildCount());
			for(int i=0; i<selectionSounds.getSounds().size(); ++i){
				const XmlNode *soundNode= selectionSoundNode->getChild("sound", i);
				string path= soundNode->getAttribute("path")->getRestrictedValue();
				StaticSound *sound= new StaticSound();
				sound->load(dir + "/" + path);
				selectionSounds[i]= sound;
			}
		}

		//command sounds
		const XmlNode *commandSoundNode= parametersNode->getChild("command-sounds");
		if(commandSoundNode->getAttribute("enabled")->getBoolValue()){
			commandSounds.resize(commandSoundNode->getChildCount());
			for(int i=0; i<commandSoundNode->getChildCount(); ++i){
				const XmlNode *soundNode= commandSoundNode->getChild("sound", i);
				string path= soundNode->getAttribute("path")->getRestrictedValue();
				StaticSound *sound= new StaticSound();
				sound->load(dir + "/" + path);
				commandSounds[i]= sound;
			}
		}

		//skills
		const XmlNode *skillsNode= unitNode->getChild("skills");
		skillTypes.resize(skillsNode->getChildCount());
		for(int i=0; i<skillTypes.size(); ++i){
			const XmlNode *sn= skillsNode->getChild("skill", i);
			const XmlNode *typeNode= sn->getChild("type");
			string classId= typeNode->getAttribute("value")->getRestrictedValue();
			SkillType *skillType= SkillTypeFactory::getInstance().newInstance(classId);
			skillType->load(sn, dir, techTree, factionType);
			skillTypes[i]= skillType;
		}

		//commands
		const XmlNode *commandsNode= unitNode->getChild("commands");
		commandTypes.resize(commandsNode->getChildCount());
		for(int i=0; i<commandTypes.size(); ++i){
			const XmlNode *commandNode= commandsNode->getChild("command", i);
			const XmlNode *typeNode= commandNode->getChild("type");
			string classId= typeNode->getAttribute("value")->getRestrictedValue();
			CommandType *commandType= CommandTypeFactory::getInstance().newInstance(classId);
			commandType->load(i, commandNode, dir, techTree, factionType, *this);
			commandTypes[i]= commandType;
		}

		computeFirstStOfClass();
		computeFirstCtOfClass();

		if(getFirstStOfClass(scStop)==NULL){
			throw runtime_error("Every unit must have at least one stop skill: "+ path);
		}
		if(getFirstStOfClass(scDie)==NULL){
			throw runtime_error("Every unit must have at least one die skill: "+ path);
		}

	}
	//Exception handling (conversions and so on);
	catch(const exception &e){
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
		throw runtime_error("Error loading UnitType: " + path + "\n" + e.what());
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

// ==================== get ====================

const CommandType *UnitType::getFirstCtOfClass(CommandClass commandClass) const{
	if(firstCommandTypeOfClass[commandClass] == NULL) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] commandClass = %d\n",__FILE__,__FUNCTION__,__LINE__,commandClass);

		/*
	    for(int j=0; j<ccCount; ++j){
	        for(int i=0; i<commandTypes.size(); ++i){
	            if(commandTypes[i]->getClass()== CommandClass(j)){
	                return commandTypes[i];
	            }
	        }
	    }
	    */

	    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}
    return firstCommandTypeOfClass[commandClass];
}

const SkillType *UnitType::getFirstStOfClass(SkillClass skillClass) const{
	if(firstSkillTypeOfClass[skillClass] == NULL) {
		/*
		for(int j= 0; j<scCount; ++j){
	        for(int i= 0; i<skillTypes.size(); ++i){
	            if(skillTypes[i]->getClass()== SkillClass(j)){
	                return skillTypes[i];
	            }
	        }
	    }
	    */
		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}
    return firstSkillTypeOfClass[skillClass];
}

const HarvestCommandType *UnitType::getFirstHarvestCommand(const ResourceType *resourceType, const Faction *faction) const {
	for(int i = 0; i < commandTypes.size(); ++i) {
		if(commandTypes[i]->getClass() == ccHarvest) {
			const HarvestCommandType *hct = static_cast<const HarvestCommandType*>(commandTypes[i]);

			if (faction->reqsOk(hct) == false) {
			 	continue;
			}

			if(hct->canHarvest(resourceType)) {
				return hct;
			}
		}
	}
	return NULL;
}

const AttackCommandType *UnitType::getFirstAttackCommand(Field field) const{
	for(int i=0; i<commandTypes.size(); ++i){
		if(commandTypes[i]->getClass()== ccAttack){
			const AttackCommandType *act= static_cast<const AttackCommandType*>(commandTypes[i]);
			if(act->getAttackSkillType()->getAttackField(field)){
				return act;
			}
		}
	}
	return NULL;
}

const RepairCommandType *UnitType::getFirstRepairCommand(const UnitType *repaired) const{
	for(int i=0; i<commandTypes.size(); ++i){
		if(commandTypes[i]->getClass()== ccRepair){
			const RepairCommandType *rct= static_cast<const RepairCommandType*>(commandTypes[i]);
			if(rct->isRepairableUnitType(repaired)){
				return rct;
			}
		}
	}
	return NULL;
}

bool UnitType::hasEmptyCellMap() const {
	//checkItemInVault(&(this->size),this->size);
	bool result = (size > 0);
	for(int i = 0; result == true && i < size; ++i) {
		for(int j = 0; j < size; ++j){
			if(cellMap[i*size+j] == true) {
				result = false;
				break;
			}
		}
	}

	return result;
}

bool UnitType::getCellMapCell(int x, int y, CardinalDir facing) const {
	assert(cellMap);
	if(cellMap == NULL) {
		throw runtime_error("cellMap == NULL");
	}

	//checkItemInVault(&(this->size),this->size);
	int tmp=0;
	switch (facing) {
		case CardinalDir::EAST:
			tmp = y;
			y = x;
			x = size - tmp - 1;
			break;
		case CardinalDir::SOUTH:
			x = size - x - 1;
			y = size - y - 1;
			break;
		case CardinalDir::WEST:
			tmp = x;
			x = y;
			y = size - tmp - 1;
			break;
		default:
			break;
	}
	return cellMap[y * size + x];
}

int UnitType::getStore(const ResourceType *rt) const{
    for(int i=0; i<storedResources.size(); ++i){
		if(storedResources[i].getType()==rt){
            return storedResources[i].getAmount();
		}
    }
    return 0;
}

const SkillType *UnitType::getSkillType(const string &skillName, SkillClass skillClass) const{
	for(int i=0; i<skillTypes.size(); ++i){
		if(skillTypes[i]->getName()==skillName){
			if(skillTypes[i]->getClass()==skillClass){
				return skillTypes[i];
			}
			else{
				throw runtime_error("Skill \""+skillName+"\" is not of class \""+SkillType::skillClassToStr(skillClass));
			}
		}
	}
	throw runtime_error("No skill named \""+skillName+"\"");
}

// ==================== totals ====================

int UnitType::getTotalMaxHp(const TotalUpgrade *totalUpgrade) const {
	checkItemInVault(&(this->maxHp),this->maxHp);
	return maxHp + totalUpgrade->getMaxHp();
}

int UnitType::getTotalMaxEp(const TotalUpgrade *totalUpgrade) const {
	checkItemInVault(&(this->maxEp),this->maxEp);
	return maxEp + totalUpgrade->getMaxEp();
}

int UnitType::getTotalArmor(const TotalUpgrade *totalUpgrade) const {
	checkItemInVault(&(this->armor),this->armor);
	return armor + totalUpgrade->getArmor();
}

int UnitType::getTotalSight(const TotalUpgrade *totalUpgrade) const {
	checkItemInVault(&(this->sight),this->sight);
	return sight + totalUpgrade->getSight();
}

// ==================== has ====================

bool UnitType::hasSkillClass(SkillClass skillClass) const {
    return firstSkillTypeOfClass[skillClass]!=NULL;
}

bool UnitType::hasCommandType(const CommandType *commandType) const {
    assert(commandType!=NULL);
    for(int i=0; i<commandTypes.size(); ++i) {
        if(commandTypes[i]==commandType) {
            return true;
        }
    }
    return false;
}

bool UnitType::hasCommandClass(CommandClass commandClass) const {
	return firstCommandTypeOfClass[commandClass]!=NULL;
}

bool UnitType::hasSkillType(const SkillType *skillType) const {
    assert(skillType!=NULL);
    for(int i=0; i<skillTypes.size(); ++i) {
        if(skillTypes[i]==skillType) {
            return true;
        }
    }
    return false;
}

bool UnitType::isOfClass(UnitClass uc) const{
	switch(uc){
	case ucWarrior:
		return hasSkillClass(scAttack) && !hasSkillClass(scHarvest);
	case ucWorker:
		return hasSkillClass(scBuild) || hasSkillClass(scRepair)|| hasSkillClass(scHarvest);
	case ucBuilding:
		return hasSkillClass(scBeBuilt);
	default:
		assert(false);
	}
	return false;
}

// ==================== PRIVATE ====================

void UnitType::computeFirstStOfClass() {
	for(int j= 0; j < scCount; ++j) {
        firstSkillTypeOfClass[j]= NULL;
        for(int i= 0; i < skillTypes.size(); ++i) {
            if(skillTypes[i]->getClass()== SkillClass(j)) {
                firstSkillTypeOfClass[j]= skillTypes[i];
                break;
            }
        }
    }
}

void UnitType::computeFirstCtOfClass() {
    for(int j = 0; j < ccCount; ++j) {
        firstCommandTypeOfClass[j]= NULL;
        for(int i = 0; i < commandTypes.size(); ++i) {
            if(commandTypes[i]->getClass() == CommandClass(j)) {
                firstCommandTypeOfClass[j] = commandTypes[i];
                break;
            }
        }
    }
}

const CommandType* UnitType::findCommandTypeById(int id) const{
	for(int i=0; i<getCommandTypeCount(); ++i){
		const CommandType* commandType= getCommandType(i);
		if(commandType->getId()==id){
			return commandType;
		}
	}
	return NULL;
}

const CommandType *UnitType::getCommandType(int i) const {
	if(i >= commandTypes.size()) {
		char szBuf[1024]="";
		sprintf(szBuf,"In [%s::%s Line: %d] i >= commandTypes.size(), i = %d, commandTypes.size() = %lu",__FILE__,__FUNCTION__,__LINE__,i,(unsigned long)commandTypes.size());
		throw runtime_error(szBuf);
	}
	return commandTypes[i];
}

string UnitType::getCommandTypeListDesc() const {
	string desc = "Commands: ";
	for(int i=0; i<getCommandTypeCount(); ++i){
		const CommandType* commandType= getCommandType(i);
		desc += " id = " + intToStr(commandType->getId()); + " toString: " + commandType->toString();
	}
	return desc;

}

string UnitType::getReqDesc() const{
	Lang &lang= Lang::getInstance();
	string desc = "Limits: ";
	string resultTxt="";

	checkItemInVault(&(this->maxUnitCount),this->maxUnitCount);
	if(getMaxUnitCount() > 0) {
		resultTxt += "\n" + lang.get("MaxUnitCount") + " " + intToStr(getMaxUnitCount());
	}
	if(resultTxt == "")
		return ProducibleType::getReqDesc();
	else
		return ProducibleType::getReqDesc()+"\nLimits: "+resultTxt;
}

std::string UnitType::toString() const {
	std::string result = "";

	result = "Unit Name: [" + name + "] id = " + intToStr(id);
	result += " maxHp = " + intToStr(maxHp);
	result += " hpRegeneration = " + intToStr(hpRegeneration);
	result += " maxEp = " + intToStr(maxEp);
	result += " epRegeneration = " + intToStr(epRegeneration);
	result += " maxUnitCount = " + intToStr(getMaxUnitCount());


	for(int i = 0; i < fieldCount; i++) {
		result += " fields index = " + intToStr(i) + " value = " + intToStr(fields[i]);
	}
	for(int i = 0; i < pCount; i++) {
		result += " properties index = " + intToStr(i) + " value = " + intToStr(properties[i]);
	}

	result += " armor = " + intToStr(armor);

	if(armorType != NULL) {
		result += " armorType Name: [" + armorType->getName() + " id = " +  intToStr(armorType->getId());
	}

	result += " light = " + intToStr(light);
	result += " lightColor = " + lightColor.getString();
	result += " multiSelect = " + intToStr(multiSelect);
	result += " sight = " + intToStr(sight);
	result += " size = " + intToStr(size);
	result += " height = " + intToStr(height);
	result += " rotatedBuildPos = " + floatToStr(rotatedBuildPos);
	result += " rotationAllowed = " + intToStr(rotationAllowed);

	if(cellMap != NULL) {
		result += " cellMap:";
		for(int i = 0; i < size; ++i) {
			for(int j = 0; j < size; ++j){
				result += " i = " + intToStr(i) + " j = " + intToStr(j) + " value = " + intToStr(cellMap[i*size+j]);
			}
		}
	}

	result += " skillTypes:";
	for(int i = 0; i < skillTypes.size(); ++i) {
		result += " i = " + intToStr(i) + " " + skillTypes[i]->toString();
	}

	result += " commandTypes:";
	for(int i = 0; i < commandTypes.size(); ++i) {
		result += " i = " + intToStr(i) + " " + commandTypes[i]->toString();
	}

	result += " storedResources:";
	for(int i = 0; i < storedResources.size(); ++i) {
		result += " i = " + intToStr(i) + " " + storedResources[i].getDescription();
	}

	result += " levels:";
	for(int i = 0; i < levels.size(); ++i) {
		result += " i = " + intToStr(i) + " " + levels[i].getName();
	}

	result += " meetingPoint = " + intToStr(meetingPoint);

	return result;
}

}}//end namespace
