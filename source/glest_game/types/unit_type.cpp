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

#ifdef WIN32
    #include <winsock2.h>
    #include <winsock.h>
#endif

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

void Level::saveGame(XmlNode *rootNode) const {
	std::map<string,string> mapTagReplacements;
	XmlNode *levelNode = rootNode->addChild("Level");

	levelNode->addAttribute("name",name, mapTagReplacements);
	levelNode->addAttribute("kills",intToStr(kills), mapTagReplacements);
}

const Level * Level::loadGame(const XmlNode *rootNode,const UnitType *ut) {
	const Level *result = NULL;
	if(rootNode->hasChild("Level") == true) {
		const XmlNode *levelNode = rootNode->getChild("Level");

		result = ut->getLevel(levelNode->getAttribute("name")->getValue());
	}

	return result;
}

// =====================================================
// 	class UnitType
// =====================================================

// ===================== PUBLIC ========================

const char *UnitType::propertyNames[]= {"burnable", "rotated_climb"};

// ==================== creation and loading ====================

UnitType::UnitType() : ProducibleType() {

	countInVictoryConditions = ucvcNotSet;
	meetingPointImage = NULL;
    lightColor= Vec3f(0.f);
    light= false;
    multiSelect= false;
	armorType= NULL;
	rotatedBuildPos=0;

	field = fLand;
	id = 0;
	meetingPoint = false;
	rotationAllowed = false;

	countUnitDeathInStats=false;
	countUnitProductionInStats=false;
	countUnitKillInStats=false;
	countKillForUnitUpgrade=false;


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

void UnitType::preLoad(const string &dir) {
	name= lastDir(dir);
}

void UnitType::loaddd(int id,const string &dir, const TechTree *techTree, const string &techTreePath,
		const FactionType *factionType, Checksum* checksum,
		Checksum* techtreeChecksum, std::map<string,vector<pair<string, string> > > &loadedFileList) {

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	string currentPath = dir;
	endPathWithSlash(currentPath);
	string path = currentPath + name + ".xml";
	string sourceXMLFile = path;

	this->id= id;

	try {
		//Lang &lang= Lang::getInstance();

		char szBuf[8096]="";
		snprintf(szBuf,8096,Lang::getInstance().get("LogScreenGameLoadingUnitType","",true).c_str(),formatString(name).c_str());
		Logger::getInstance().add(szBuf, true);

		//file load
		checksum->addFile(path);
		techtreeChecksum->addFile(path);

		XmlTree xmlTree;
		std::map<string,string> mapExtraTagReplacementValues;
		mapExtraTagReplacementValues["$COMMONDATAPATH"] = techTreePath + "/commondata/";
		xmlTree.load(path, Properties::getTagReplacementValues(&mapExtraTagReplacementValues));
		loadedFileList[path].push_back(make_pair(dir,dir));

		const XmlNode *unitNode= xmlTree.getRootNode();

		const XmlNode *parametersNode= unitNode->getChild("parameters");

		if(parametersNode->hasChild("count-in-victory-conditions") == true) {
			bool countUnit = parametersNode->getChild("count-in-victory-conditions")->getAttribute("value")->getBoolValue();
			if(countUnit == true) {
				countInVictoryConditions = ucvcTrue;
			}
			else {
				countInVictoryConditions = ucvcFalse;
			}
		}

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
					throw megaglest_runtime_error("Cellmap row has not the same length as unit size");
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
				throw megaglest_runtime_error("Not a valid field: "+fieldName+": "+ path);
			}
		}

		if (fields[fLand]) {
			field = fLand;
		}
		else if (fields[fAir]) {
			field = fAir;
		}
		else {
			throw megaglest_runtime_error("Unit has no field: " + path);
		}

		//properties
		const XmlNode *propertiesNode= parametersNode->getChild("properties");
		for(int i = 0; i < propertiesNode->getChildCount(); ++i) {
			const XmlNode *propertyNode= propertiesNode->getChild("property", i);
			string propertyName= propertyNode->getAttribute("value")->getRestrictedValue();
			bool found= false;
			for(int i = 0; i < pCount; ++i) {
				if(propertyName == propertyNames[i]) {
					properties[i]= true;
					found= true;
					break;
				}
			}
			if(!found) {
				throw megaglest_runtime_error("Unknown property: " + propertyName);
			}
		}
		//damage-particles
		if(parametersNode->hasChild("damage-particles")) {
			const XmlNode *particleNode= parametersNode->getChild("damage-particles");
			bool particleEnabled= particleNode->getAttribute("value")->getBoolValue();

			if(particleEnabled) {
				for(int i = 0; i < particleNode->getChildCount(); ++i) {
					const XmlNode *particleFileNode= particleNode->getChild("particle-file", i);
					string path= particleFileNode->getAttribute("path")->getRestrictedValue();
					UnitParticleSystemType *unitParticleSystemType= new UnitParticleSystemType();

					//Texture2D *newTexture = Renderer::getInstance().newTexture2D(rsGame);
					//Texture2D *newTexture = NULL;

					unitParticleSystemType->load(particleFileNode, dir, currentPath + path,
							&Renderer::getInstance(),loadedFileList, sourceXMLFile,
							techTree->getPath());
					loadedFileList[currentPath + path].push_back(make_pair(sourceXMLFile,particleFileNode->getAttribute("path")->getRestrictedValue()));

					//if(unitParticleSystemType->hasTexture() == false) {
						//Renderer::getInstance().endLastTexture(rsGame,true);
					//}

					if(particleFileNode->getAttribute("minHp",false) != NULL && particleFileNode->getAttribute("maxHp",false) != NULL) {
						unitParticleSystemType->setMinmaxEnabled(true);
						unitParticleSystemType->setMinHp(particleFileNode->getAttribute("minHp")->getIntValue());
						unitParticleSystemType->setMaxHp(particleFileNode->getAttribute("maxHp")->getIntValue());

						if(particleFileNode->getAttribute("ispercentbased",false) != NULL) {
							unitParticleSystemType->setMinmaxIsPercent(particleFileNode->getAttribute("ispercentbased")->getBoolValue());
						}

						//printf("Found customized particle trigger by HP [%d to %d]\n",unitParticleSystemType->getMinHp(),unitParticleSystemType->getMaxHp());
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
		if(parametersNode->hasChild("rotationAllowed")) {
			const XmlNode *rotationAllowedNode= parametersNode->getChild("rotationAllowed");
			rotationAllowed= rotationAllowedNode->getAttribute("value")->getBoolValue();
		}
		else
		{
			rotationAllowed=true;
		}

		std::map<string,int> sortedItems;

		//unit requirements
		bool hasDup = false;
		const XmlNode *unitRequirementsNode= parametersNode->getChild("unit-requirements");
		for(int i=0; i<unitRequirementsNode->getChildCount(); ++i){
			const XmlNode *unitNode= 	unitRequirementsNode->getChild("unit", i);
			string name= unitNode->getAttribute("name")->getRestrictedValue();

			if(sortedItems.find(name) != sortedItems.end()) {
				hasDup = true;
			}

			sortedItems[name] = 0;
		}
		if(hasDup) {
			printf("WARNING, unit type [%s] has one or more duplicate unit requirements\n",this->getName().c_str());
		}

		for(std::map<string,int>::iterator iterMap = sortedItems.begin();
				iterMap != sortedItems.end(); ++iterMap) {
			unitReqs.push_back(factionType->getUnitType(iterMap->first));
		}
		sortedItems.clear();
		hasDup = false;

		//upgrade requirements
		const XmlNode *upgradeRequirementsNode= parametersNode->getChild("upgrade-requirements");
		for(int i=0; i<upgradeRequirementsNode->getChildCount(); ++i){
			const XmlNode *upgradeReqNode= upgradeRequirementsNode->getChild("upgrade", i);
			string name= upgradeReqNode->getAttribute("name")->getRestrictedValue();

			if(sortedItems.find(name) != sortedItems.end()) {
				hasDup = true;
			}

			sortedItems[name] = 0;
		}

		if(hasDup) {
			printf("WARNING, unit type [%s] has one or more duplicate upgrade requirements\n",this->getName().c_str());
		}

		for(std::map<string,int>::iterator iterMap = sortedItems.begin();
				iterMap != sortedItems.end(); ++iterMap) {
			upgradeReqs.push_back(factionType->getUpgradeType(iterMap->first));
		}
		sortedItems.clear();
		hasDup = false;

		//resource requirements
		const XmlNode *resourceRequirementsNode= parametersNode->getChild("resource-requirements");
		costs.resize(resourceRequirementsNode->getChildCount());
		for(int i = 0; i < costs.size(); ++i) {
			const XmlNode *resourceNode= resourceRequirementsNode->getChild("resource", i);
			string name= resourceNode->getAttribute("name")->getRestrictedValue();
			int amount= resourceNode->getAttribute("amount")->getIntValue();

			if(sortedItems.find(name) != sortedItems.end()) {
				hasDup = true;
			}
			sortedItems[name] = amount;
		}
		//if(hasDup || sortedItems.size() != costs.size()) printf("Found duplicate resource requirement, costs.size() = %d sortedItems.size() = %d\n",costs.size(),sortedItems.size());

		if(hasDup) {
			printf("WARNING, unit type [%s] has one or more duplicate resource requirements\n",this->getName().c_str());
		}

		if(sortedItems.size() < costs.size()) {
			costs.resize(sortedItems.size());
		}
		int index = 0;
		for(std::map<string,int>::iterator iterMap = sortedItems.begin();
				iterMap != sortedItems.end(); ++iterMap) {
			costs[index].init(techTree->getResourceType(iterMap->first), iterMap->second);
			index++;
		}
		sortedItems.clear();
		hasDup = false;

		//resources stored
		const XmlNode *resourcesStoredNode= parametersNode->getChild("resources-stored");
		storedResources.resize(resourcesStoredNode->getChildCount());
		for(int i=0; i<storedResources.size(); ++i){
			const XmlNode *resourceNode= resourcesStoredNode->getChild("resource", i);
			string name= resourceNode->getAttribute("name")->getRestrictedValue();
			int amount= resourceNode->getAttribute("amount")->getIntValue();

			if(sortedItems.find(name) != sortedItems.end()) {
				hasDup = true;
			}

			sortedItems[name] = amount;
		}

		if(hasDup) {
			printf("WARNING, unit type [%s] has one or more duplicate stored resources\n",this->getName().c_str());
		}

		if(sortedItems.size() < storedResources.size()) {
			storedResources.resize(sortedItems.size());
		}

		index = 0;
		for(std::map<string,int>::iterator iterMap = sortedItems.begin();
				iterMap != sortedItems.end(); ++iterMap) {
			storedResources[index].init(techTree->getResourceType(iterMap->first), iterMap->second);
			index++;
		}
		sortedItems.clear();

		//image
		const XmlNode *imageNode= parametersNode->getChild("image");
		image= Renderer::getInstance().newTexture2D(rsGame);
		if(image) {
			image->load(imageNode->getAttribute("path")->getRestrictedValue(currentPath));
		}
		loadedFileList[imageNode->getAttribute("path")->getRestrictedValue(currentPath)].push_back(make_pair(sourceXMLFile,imageNode->getAttribute("path")->getRestrictedValue()));

		//image cancel
		const XmlNode *imageCancelNode= parametersNode->getChild("image-cancel");
		cancelImage= Renderer::getInstance().newTexture2D(rsGame);
		if(cancelImage) {
			cancelImage->load(imageCancelNode->getAttribute("path")->getRestrictedValue(currentPath));
		}
		loadedFileList[imageCancelNode->getAttribute("path")->getRestrictedValue(currentPath)].push_back(make_pair(sourceXMLFile,imageCancelNode->getAttribute("path")->getRestrictedValue()));

		//meeting point
		const XmlNode *meetingPointNode= parametersNode->getChild("meeting-point");
		meetingPoint= meetingPointNode->getAttribute("value")->getBoolValue();
		if(meetingPoint) {
			meetingPointImage= Renderer::getInstance().newTexture2D(rsGame);
			if(meetingPointImage) {
				meetingPointImage->load(meetingPointNode->getAttribute("image-path")->getRestrictedValue(currentPath));
			}
			loadedFileList[meetingPointNode->getAttribute("image-path")->getRestrictedValue(currentPath)].push_back(make_pair(sourceXMLFile,meetingPointNode->getAttribute("image-path")->getRestrictedValue()));
		}

		//countUnitDeathInStats
		if(parametersNode->hasChild("count-unit-death-in-stats")){
			const XmlNode *countUnitDeathInStatsNode= parametersNode->getChild("count-unit-death-in-stats");
			countUnitDeathInStats= countUnitDeathInStatsNode->getAttribute("value")->getBoolValue();
		} else {
			countUnitDeathInStats=true;
		}
		//countUnitProductionInStats
		if(parametersNode->hasChild("count-unit-production-in-stats")){
			const XmlNode *countUnitProductionInStatsNode= parametersNode->getChild("count-unit-production-in-stats");
			countUnitProductionInStats= countUnitProductionInStatsNode->getAttribute("value")->getBoolValue();
		} else {
			countUnitProductionInStats=true;
		}
		//countUnitKillInStats
		if(parametersNode->hasChild("count-unit-kill-in-stats")){
			const XmlNode *countUnitKillInStatsNode= parametersNode->getChild("count-unit-kill-in-stats");
			countUnitKillInStats= countUnitKillInStatsNode->getAttribute("value")->getBoolValue();
		} else {
			countUnitKillInStats=true;
		}

		//countKillForUnitUpgrade
		if(parametersNode->hasChild("count-kill-for-unit-upgrade")){
			const XmlNode *countKillForUnitUpgradeNode= parametersNode->getChild("count-kill-for-unit-upgrade");
			countKillForUnitUpgrade= countKillForUnitUpgradeNode->getAttribute("value")->getBoolValue();
		} else {
			countKillForUnitUpgrade=true;
		}

		if(countKillForUnitUpgrade == false){
			// it makes no sense if we count it in stats but not for upgrades
			countUnitKillInStats=false;
		}

		//selection sounds
		const XmlNode *selectionSoundNode= parametersNode->getChild("selection-sounds");
		if(selectionSoundNode->getAttribute("enabled")->getBoolValue()){
			selectionSounds.resize(selectionSoundNode->getChildCount());
			for(int i = 0; i < selectionSounds.getSounds().size(); ++i) {
				const XmlNode *soundNode= selectionSoundNode->getChild("sound", i);
				string path= soundNode->getAttribute("path")->getRestrictedValue(currentPath);
				StaticSound *sound= new StaticSound();
				sound->load(path);
				loadedFileList[path].push_back(make_pair(sourceXMLFile,soundNode->getAttribute("path")->getRestrictedValue()));
				selectionSounds[i]= sound;
			}
		}

		//command sounds
		const XmlNode *commandSoundNode= parametersNode->getChild("command-sounds");
		if(commandSoundNode->getAttribute("enabled")->getBoolValue()) {
			commandSounds.resize(commandSoundNode->getChildCount());
			for(int i = 0; i < commandSoundNode->getChildCount(); ++i) {
				const XmlNode *soundNode= commandSoundNode->getChild("sound", i);
				string path= soundNode->getAttribute("path")->getRestrictedValue(currentPath);
				StaticSound *sound= new StaticSound();
				sound->load(path);
				loadedFileList[path].push_back(make_pair(sourceXMLFile,soundNode->getAttribute("path")->getRestrictedValue()));
				commandSounds[i]= sound;
			}
		}

		//skills

		const XmlNode *attackBoostsNode= NULL;
		if(unitNode->hasChild("attack-boosts") == true) {
			attackBoostsNode=unitNode->getChild("attack-boosts");
		}

		const XmlNode *skillsNode= unitNode->getChild("skills");
		skillTypes.resize(skillsNode->getChildCount());

		snprintf(szBuf,8096,Lang::getInstance().get("LogScreenGameLoadingUnitTypeSkills","",true).c_str(),formatString(name).c_str(),skillTypes.size());
		Logger::getInstance().add(szBuf, true);

		for(int i = 0; i < skillTypes.size(); ++i) {
			const XmlNode *sn= skillsNode->getChild("skill", i);
			const XmlNode *typeNode= sn->getChild("type");
			string classId= typeNode->getAttribute("value")->getRestrictedValue();
			SkillType *skillType= SkillTypeFactory::getInstance().newInstance(classId);

			skillType->load(sn, attackBoostsNode, dir, techTree, factionType, loadedFileList,sourceXMLFile);
			skillTypes[i]= skillType;

		}

		//commands
		const XmlNode *commandsNode= unitNode->getChild("commands");
		commandTypes.resize(commandsNode->getChildCount());
		for(int i = 0; i < commandTypes.size(); ++i) {
			const XmlNode *commandNode= commandsNode->getChild("command", i);
			const XmlNode *typeNode= commandNode->getChild("type");
			string classId= typeNode->getAttribute("value")->getRestrictedValue();
			CommandType *commandType= CommandTypeFactory::getInstance().newInstance(classId);
			commandType->load(i, commandNode, dir, techTree, factionType, *this,
					loadedFileList,sourceXMLFile);
			commandTypes[i]= commandType;
		}

		computeFirstStOfClass();
		computeFirstCtOfClass();

		if(getFirstStOfClass(scStop)==NULL){
			throw megaglest_runtime_error("Every unit must have at least one stop skill: "+ path);
		}
		if(getFirstStOfClass(scDie)==NULL){
			throw megaglest_runtime_error("Every unit must have at least one die skill: "+ path);
		}

	}
	//Exception handling (conversions and so on);
	catch(const exception &e){
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
		throw megaglest_runtime_error("Error loading UnitType: " + path + "\n" + e.what());
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

// ==================== get ====================

const Level *UnitType::getLevel(string name) const {
	const Level *result = NULL;
	for(unsigned int i = 0; i < levels.size(); ++i) {
		const Level &level = levels[i];
		if(level.getName() == name) {
			result = &level;
			break;
		}
	}

	return result;
}

const CommandType *UnitType::getFirstCtOfClass(CommandClass commandClass) const{
	if(firstCommandTypeOfClass[commandClass] == NULL) {
		//if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] commandClass = %d\n",__FILE__,__FUNCTION__,__LINE__,commandClass);

		/*
	    for(int j=0; j<ccCount; ++j){
	        for(int i=0; i<commandTypes.size(); ++i){
	            if(commandTypes[i]->getClass()== CommandClass(j)){
	                return commandTypes[i];
	            }
	        }
	    }
	    */

		//if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
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
	//printf("$$$ Unit [%s] commandTypes.size() = %d\n",this->getName().c_str(),(int)commandTypes.size());

	for(int i = 0; i < commandTypes.size(); ++i){
		if(commandTypes[i] == NULL) {
			throw megaglest_runtime_error("commandTypes[i] == NULL");
		}

		//printf("$$$ Unit [%s] i = %d, commandTypes[i] [%s]\n",this->getName().c_str(),(int)i, commandTypes[i]->toString().c_str());
		if(commandTypes[i]->getClass()== ccAttack){
			const AttackCommandType *act= dynamic_cast<const AttackCommandType*>(commandTypes[i]);
			if(act->getAttackSkillType()->getAttackField(field)) {
				//printf("## Unit [%s] i = %d, is found\n",this->getName().c_str(),(int)i);
				return act;
			}
		}
	}

	return NULL;
}

const AttackStoppedCommandType *UnitType::getFirstAttackStoppedCommand(Field field) const{
	//printf("$$$ Unit [%s] commandTypes.size() = %d\n",this->getName().c_str(),(int)commandTypes.size());

	for(int i = 0; i < commandTypes.size(); ++i){
		if(commandTypes[i] == NULL) {
			throw megaglest_runtime_error("commandTypes[i] == NULL");
		}

		//printf("$$$ Unit [%s] i = %d, commandTypes[i] [%s]\n",this->getName().c_str(),(int)i, commandTypes[i]->toString().c_str());
		if(commandTypes[i]->getClass()== ccAttackStopped){
			const AttackStoppedCommandType *act= dynamic_cast<const AttackStoppedCommandType*>(commandTypes[i]);
			if(act->getAttackSkillType()->getAttackField(field)) {
				//printf("## Unit [%s] i = %d, is found\n",this->getName().c_str(),(int)i);
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

Vec2i UnitType::getFirstOccupiedCellInCellMap(Vec2i currentPos) const {
	Vec2i cell = currentPos;
	//printf("\n\n\n\n^^^^^^^^^^ currentPos [%s] size [%d]\n",currentPos.getString().c_str(),size);

	//checkItemInVault(&(this->size),this->size);
	if(hasCellMap() == true) {
		for(int i = 0; i < size; ++i) {
			for(int j = 0; j < size; ++j){
				if(cellMap[i*size+j] == true) {
					cell.x += i;
					cell.y += j;
					//printf("\n^^^^^^^^^^ cell [%s] i [%d] j [%d]\n",cell.getString().c_str(),i,j);
					return cell;
				}
			}
		}
	}
	return cell;
}

bool UnitType::getCellMapCell(int x, int y, CardinalDir facing) const {
	assert(cellMap);
	if(cellMap == NULL) {
		throw megaglest_runtime_error("cellMap == NULL");
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
				throw megaglest_runtime_error("Skill \""+skillName+"\" is not of class \""+SkillType::skillClassToStr(skillClass));
			}
		}
	}
	throw megaglest_runtime_error("No skill named \""+skillName+"\"");
}

// ==================== totals ====================

int UnitType::getTotalMaxHp(const TotalUpgrade *totalUpgrade) const {
	checkItemInVault(&(this->maxHp),this->maxHp);
	int result = maxHp + totalUpgrade->getMaxHp();
	result = max(0,result);
	return result;
}

int UnitType::getTotalMaxHpRegeneration(const TotalUpgrade *totalUpgrade) const {
	checkItemInVault(&(this->hpRegeneration),this->hpRegeneration);
	int result = hpRegeneration + totalUpgrade->getMaxHpRegeneration();
	//result = max(0,result);
	return result;
}

int UnitType::getTotalMaxEp(const TotalUpgrade *totalUpgrade) const {
	checkItemInVault(&(this->maxEp),this->maxEp);
	int result = maxEp + totalUpgrade->getMaxEp();
	result = max(0,result);
	return result;
}

int UnitType::getTotalMaxEpRegeneration(const TotalUpgrade *totalUpgrade) const {
	checkItemInVault(&(this->epRegeneration),this->epRegeneration);
	int result = epRegeneration + totalUpgrade->getMaxEpRegeneration();
	//result = max(0,result);
	return result;
}

int UnitType::getTotalArmor(const TotalUpgrade *totalUpgrade) const {
	checkItemInVault(&(this->armor),this->armor);
	int result = armor + totalUpgrade->getArmor();
	result = max(0,result);
	return result;
}

int UnitType::getTotalSight(const TotalUpgrade *totalUpgrade) const {
	checkItemInVault(&(this->sight),this->sight);
	int result = sight + totalUpgrade->getSight();
	result = max(0,result);
	return result;
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
		break;
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
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] i >= commandTypes.size(), i = %d, commandTypes.size() = " MG_SIZE_T_SPECIFIER "",__FILE__,__FUNCTION__,__LINE__,i,commandTypes.size());
		throw megaglest_runtime_error(szBuf);
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

string UnitType::getName(bool translatedValue) const {
	if(translatedValue == false) return name;

	Lang &lang = Lang::getInstance();
	return lang.getTechTreeString("UnitTypeName_" + name,name.c_str());
}

std::string UnitType::toString() const {
	std::string result = "Unit Name: [" + name + "] id = " + intToStr(id);
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
		result += " cellMap: [" + intToStr(size) + "]";
		for(int i = 0; i < size; ++i) {
			for(int j = 0; j < size; ++j){
				result += " i = " + intToStr(i) + " j = " + intToStr(j) + " value = " + intToStr(cellMap[i*size+j]);
			}
		}
	}

	result += " skillTypes: [" + intToStr(skillTypes.size()) + "]";
	for(int i = 0; i < skillTypes.size(); ++i) {
		result += " i = " + intToStr(i) + " " + skillTypes[i]->toString();
	}

	result += " commandTypes: [" + intToStr(commandTypes.size()) + "]";
	for(int i = 0; i < commandTypes.size(); ++i) {
		result += " i = " + intToStr(i) + " " + commandTypes[i]->toString();
	}

	result += " storedResources: [" + intToStr(storedResources.size()) + "]";
	for(int i = 0; i < storedResources.size(); ++i) {
		result += " i = " + intToStr(i) + " " + storedResources[i].getDescription();
	}

	result += " levels: [" + intToStr(levels.size()) + "]";
	for(int i = 0; i < levels.size(); ++i) {
		result += " i = " + intToStr(i) + " " + levels[i].getName();
	}

	result += " meetingPoint = " + intToStr(meetingPoint);

	result += " countInVictoryConditions = " + intToStr(countInVictoryConditions);

	return result;
}

}}//end namespace
