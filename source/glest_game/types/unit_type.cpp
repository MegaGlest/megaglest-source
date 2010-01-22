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

UnitType::UnitType(){

    lightColor= Vec3f(0.f);
    light= false;
    multiSelect= false;
	armorType= NULL;

	for(int i=0; i<pCount; ++i){
		properties[i]= false;
	}

	for(int i=0; i<fieldCount; ++i){
		fields[i]= false;
	}

	cellMap= NULL;
	hpRegeneration= 0;
	epRegeneration= 0;
} 

UnitType::~UnitType(){
	deleteValues(commandTypes.begin(), commandTypes.end());
	deleteValues(skillTypes.begin(), skillTypes.end());
	deleteValues(selectionSounds.getSounds().begin(), selectionSounds.getSounds().end());
	deleteValues(commandSounds.getSounds().begin(), commandSounds.getSounds().end());
	delete [] cellMap;
}

void UnitType::preLoad(const string &dir){
	name= lastDir(dir);
}

void UnitType::load(int id,const string &dir, const TechTree *techTree, const FactionType *factionType, Checksum* checksum){
    
	this->id= id;
    string path;

	try{

		Logger::getInstance().add("Unit type: " + formatString(name), true);

		//file load
		path= dir+"/"+name+".xml";

		checksum->addFile(path);

		XmlTree xmlTree;
		xmlTree.load(path);
		const XmlNode *unitNode= xmlTree.getRootNode();

		const XmlNode *parametersNode= unitNode->getChild("parameters");

		//size
		size= parametersNode->getChild("size")->getAttribute("value")->getIntValue();
		
		//height
		height= parametersNode->getChild("height")->getAttribute("value")->getIntValue();
		
		//maxHp
		maxHp= parametersNode->getChild("max-hp")->getAttribute("value")->getIntValue();
		
		//hpRegeneration
		hpRegeneration= parametersNode->getChild("max-hp")->getAttribute("regeneration")->getIntValue();
		
		//maxEp
		maxEp= parametersNode->getChild("max-ep")->getAttribute("value")->getIntValue();
		
		if(maxEp!=0){
			//wpRegeneration
			epRegeneration= parametersNode->getChild("max-ep")->getAttribute("regeneration")->getIntValue();
		}

		//armor
		armor= parametersNode->getChild("armor")->getAttribute("value")->getIntValue();
		
		//armor type string
		string armorTypeName= parametersNode->getChild("armor-type")->getAttribute("value")->getRestrictedValue();
		armorType= techTree->getArmorType(armorTypeName);

		//sight
		sight= parametersNode->getChild("sight")->getAttribute("value")->getIntValue();
		
		//prod time
		productionTime= parametersNode->getChild("time")->getAttribute("value")->getIntValue();

		//multi selection
		multiSelect= parametersNode->getChild("multi-selection")->getAttribute("value")->getBoolValue();

		//cellmap
		const XmlNode *cellMapNode= parametersNode->getChild("cellmap");
		bool hasCellMap= cellMapNode->getAttribute("value")->getBoolValue();
		if(hasCellMap){
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

		//light
		const XmlNode *lightNode= parametersNode->getChild("light");
		light= lightNode->getAttribute("enabled")->getBoolValue();
		if(light){
			lightColor.x= lightNode->getAttribute("red")->getFloatValue(0.f, 1.f);
			lightColor.y= lightNode->getAttribute("green")->getFloatValue(0.f, 1.f);
			lightColor.z= lightNode->getAttribute("blue")->getFloatValue(0.f, 1.f);	
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
		throw runtime_error("Error loading UnitType: " + path + "\n" + e.what());
	}
}

// ==================== get ==================== 

const CommandType *UnitType::getFirstCtOfClass(CommandClass commandClass) const{
    return firstCommandTypeOfClass[commandClass];
}

const SkillType *UnitType::getFirstStOfClass(SkillClass skillClass) const{
    return firstSkillTypeOfClass[skillClass];
}

const HarvestCommandType *UnitType::getFirstHarvestCommand(const ResourceType *resourceType) const{
	for(int i=0; i<commandTypes.size(); ++i){
		if(commandTypes[i]->getClass()== ccHarvest){
			const HarvestCommandType *hct= static_cast<const HarvestCommandType*>(commandTypes[i]);
			if(hct->canHarvest(resourceType)){
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

int UnitType::getTotalMaxHp(const TotalUpgrade *totalUpgrade) const{
	return maxHp + totalUpgrade->getMaxHp();
}

int UnitType::getTotalMaxEp(const TotalUpgrade *totalUpgrade) const{
	return maxEp + totalUpgrade->getMaxEp();
}

int UnitType::getTotalArmor(const TotalUpgrade *totalUpgrade) const{
	return armor + totalUpgrade->getArmor();
}

int UnitType::getTotalSight(const TotalUpgrade *totalUpgrade) const{
	return sight + totalUpgrade->getSight();
}

// ==================== has ==================== 

bool UnitType::hasSkillClass(SkillClass skillClass) const{
    return firstSkillTypeOfClass[skillClass]!=NULL;        
}

bool UnitType::hasCommandType(const CommandType *commandType) const{
    assert(commandType!=NULL);
    for(int i=0; i<commandTypes.size(); ++i){
        if(commandTypes[i]==commandType){
            return true;
        }
    }
    return false;
}

bool UnitType::hasCommandClass(CommandClass commandClass) const{
	return firstCommandTypeOfClass[commandClass]!=NULL;
}

bool UnitType::hasSkillType(const SkillType *skillType) const{
    assert(skillType!=NULL);
    for(int i=0; i<skillTypes.size(); ++i){
        if(skillTypes[i]==skillType){
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
		return hasSkillClass(scBuild) || hasSkillClass(scRepair);
	case ucBuilding:
		return hasSkillClass(scBeBuilt);
	default:
		assert(false);
	}
	return false;
}

// ==================== PRIVATE ==================== 

void UnitType::computeFirstStOfClass(){
	for(int j= 0; j<scCount; ++j){
        firstSkillTypeOfClass[j]= NULL;
        for(int i= 0; i<skillTypes.size(); ++i){
            if(skillTypes[i]->getClass()== SkillClass(j)){
                firstSkillTypeOfClass[j]= skillTypes[i];  
                break;
            }
        }
    }
}

void UnitType::computeFirstCtOfClass(){
    for(int j=0; j<ccCount; ++j){
        firstCommandTypeOfClass[j]= NULL;
        for(int i=0; i<commandTypes.size(); ++i){
            if(commandTypes[i]->getClass()== CommandClass(j)){
                firstCommandTypeOfClass[j]= commandTypes[i];  
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

}}//end namespace
