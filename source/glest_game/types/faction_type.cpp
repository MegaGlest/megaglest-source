// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martio Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#include "faction_type.h"

#include "logger.h"
#include "util.h" 
#include "xml_parser.h"
#include "tech_tree.h"
#include "resource.h"
#include "platform_util.h"
#include "game_util.h"
#include "conversion.h"
#include "leak_dumper.h"

using namespace Shared::Util;
using namespace Shared::Xml;

namespace Glest{ namespace Game{

// ======================================================
//          Class FactionType                   
// ======================================================

FactionType::FactionType(){
	music= NULL;	
}

//load a faction, given a directory
void FactionType::load(const string &dir, const TechTree *techTree, Checksum* checksum){

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    name= lastDir(dir);

	Logger::getInstance().add("Faction type: "+ formatString(name), true);
    
	// a1) preload units
	string unitsPath= dir + "/units/*.";
	vector<string> unitFilenames;
    findAll(unitsPath, unitFilenames);
	unitTypes.resize(unitFilenames.size());
    for(int i=0; i<unitTypes.size(); ++i){
		string str= dir + "/units/" + unitFilenames[i];
		unitTypes[i].preLoad(str);

		SDL_PumpEvents();
    }

	// a2) preload upgrades
	string upgradesPath= dir + "/upgrades/*.";
	vector<string> upgradeFilenames;
    findAll(upgradesPath, upgradeFilenames);
	upgradeTypes.resize(upgradeFilenames.size());
    for(int i=0; i<upgradeTypes.size(); ++i){
		string str= dir + "/upgrades/" + upgradeFilenames[i];
		upgradeTypes[i].preLoad(str);

		SDL_PumpEvents();
    }
	
	// b1) load units
	try{
		for(int i=0; i<unitTypes.size(); ++i){
            string str= dir + "/units/" + unitTypes[i].getName();
            unitTypes[i].load(i, str, techTree, this, checksum);

			SDL_PumpEvents();
        }
    }
	catch(const exception &e){
		throw runtime_error("Error loading units: "+ dir + "\n" + e.what());
	}

	// b2) load upgrades
	try{
		for(int i=0; i<upgradeTypes.size(); ++i){
            string str= dir + "/upgrades/" + upgradeTypes[i].getName();
            upgradeTypes[i].load(str, techTree, this, checksum);

			SDL_PumpEvents();
        }
    }
	catch(const exception &e){
		throw runtime_error("Error loading upgrades: "+ dir + "\n" + e.what());
	}

	//open xml file
    string path= dir+"/"+name+".xml";
	checksum->addFile(path);

	XmlTree xmlTree;
	xmlTree.load(path);
	const XmlNode *factionNode= xmlTree.getRootNode();

	//read starting resources
	const XmlNode *startingResourcesNode= factionNode->getChild("starting-resources");

	startingResources.resize(startingResourcesNode->getChildCount());
	for(int i=0; i<startingResources.size(); ++i){
		const XmlNode *resourceNode= startingResourcesNode->getChild("resource", i);
		string name= resourceNode->getAttribute("name")->getRestrictedValue();
		int amount= resourceNode->getAttribute("amount")->getIntValue();
		startingResources[i].init(techTree->getResourceType(name), amount);

		SDL_PumpEvents();
	}

	//read starting units
	const XmlNode *startingUnitsNode= factionNode->getChild("starting-units");
	for(int i=0; i<startingUnitsNode->getChildCount(); ++i){
		const XmlNode *unitNode= startingUnitsNode->getChild("unit", i);
		string name= unitNode->getAttribute("name")->getRestrictedValue();
		int amount= unitNode->getAttribute("amount")->getIntValue();
		startingUnits.push_back(PairPUnitTypeInt(getUnitType(name), amount)); 

		SDL_PumpEvents();
	}

	//read music
	const XmlNode *musicNode= factionNode->getChild("music");
	bool value= musicNode->getAttribute("value")->getBoolValue();
	if(value){
		music= new StrSound();
		music->open(dir+"/"+musicNode->getAttribute("path")->getRestrictedValue());
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

FactionType::~FactionType(){
	delete music;
	music = NULL;
}

std::vector<std::string> FactionType::validateFactionType() {
	std::vector<std::string> results;

	// Check every unit's commands to validate that for every upgrade-requirements
	// upgrade we have a unit that can do the upgrade in the faction.
    for(int i=0; i<unitTypes.size(); ++i){
    	UnitType &unitType = unitTypes[i];
    	for(int j = 0; j < unitType.getCommandTypeCount(); ++j) {
    		const CommandType *cmdType = unitType.getCommandType(j);
    		if(cmdType != NULL) {
				for(int k = 0; k < cmdType->getUpgradeReqCount(); ++k) {
					const UpgradeType *upgradeType = cmdType->getUpgradeReq(k);

					if(upgradeType != NULL) {
						// Now lets find a unit that can produced-upgrade this upgrade
						bool foundUpgraderUnit = false;
						for(int l=0; l<unitTypes.size() && foundUpgraderUnit == false; ++l){
							UnitType &unitType2 = unitTypes[l];
							for(int m = 0; m < unitType2.getCommandTypeCount() && foundUpgraderUnit == false; ++m) {
								const CommandType *cmdType2 = unitType2.getCommandType(m);
								if(cmdType2 != NULL && dynamic_cast<const UpgradeCommandType *>(cmdType2) != NULL) {
									const UpgradeCommandType *uct = dynamic_cast<const UpgradeCommandType *>(cmdType2);
									const UpgradeType *upgradeType2 = uct->getProducedUpgrade();
									if(upgradeType2 != NULL && upgradeType2->getName() == upgradeType->getName()) {
										 foundUpgraderUnit = true;
									}
								}
							}
						}

						if(foundUpgraderUnit == false) {
							char szBuf[4096]="";
							sprintf(szBuf,"The Unit [%s] in Faction [%s] has the command [%s]\nwhich has upgrade requirement [%s] but there are no units able to perform the upgrade!",unitType.getName().c_str(),this->getName().c_str(),cmdType->getName().c_str(),upgradeType->getName().c_str());
							results.push_back(szBuf);
						}
					}
				}
    		}
    	}
    }

    return results;
}

std::vector<std::string> FactionType::validateFactionTypeResourceTypes(vector<ResourceType> &resourceTypes) {
	std::vector<std::string> results;

	// Check every unit's commands to validate that for every upgrade-requirements
	// upgrade we have a unit that can do the upgrade in the faction.
    for(int i=0; i<unitTypes.size(); ++i){
    	UnitType &unitType = unitTypes[i];
    	for(int j = 0; j < unitType.getCostCount() ; ++j) {
    		const Resource *r = unitType.getCost(j);
    		if(r != NULL && r->getType() != NULL) {
    			bool foundResourceType = false;
    			// Now lets find a matching faction resource type for the unit
    			for(int k=0; k<resourceTypes.size(); ++k){
    				ResourceType &rt = resourceTypes[k];

					if(r->getType()->getName() == rt.getName()) {
						foundResourceType = true;
					}
				}

				if(foundResourceType == false) {
					char szBuf[4096]="";
					sprintf(szBuf,"The Unit [%s] in Faction [%s] has the resource req [%s]\nbut there are no such resources in this tech!",unitType.getName().c_str(),this->getName().c_str(),r->getType()->getName().c_str());
					results.push_back(szBuf);
				}
    		}
    	}
    }

    return results;
}

// ==================== get ==================== 

const UnitType *FactionType::getUnitType(const string &name) const{     
    for(int i=0; i<unitTypes.size();i++){
		if(unitTypes[i].getName()==name){
            return &unitTypes[i];
		}
    }

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] scanning [%s] size = %d\n",__FILE__,__FUNCTION__,__LINE__,name.c_str(),unitTypes.size());
    for(int i=0; i<unitTypes.size();i++){
    	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] scanning [%s] idx = %d [%s]\n",__FILE__,__FUNCTION__,__LINE__,name.c_str(),i,unitTypes[i].getName().c_str());
    }

	throw runtime_error("Unit not found: "+name);
}

const UpgradeType *FactionType::getUpgradeType(const string &name) const{     
    for(int i=0; i<upgradeTypes.size();i++){
		if(upgradeTypes[i].getName()==name){
            return &upgradeTypes[i];
		}
    }

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] scanning [%s] size = %d\n",__FILE__,__FUNCTION__,__LINE__,name.c_str(),unitTypes.size());
    for(int i=0; i<upgradeTypes.size();i++){
    	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] scanning [%s] idx = %d [%s]\n",__FILE__,__FUNCTION__,__LINE__,name.c_str(),i,upgradeTypes[i].getName().c_str());
    }

	throw runtime_error("Upgrade not found: "+name);
}

int FactionType::getStartingResourceAmount(const ResourceType *resourceType) const{
	for(int i=0; i<startingResources.size(); ++i){
		if(startingResources[i].getType()==resourceType){
			return startingResources[i].getAmount();
		}
	}
	return 0;
}

std::string FactionType::toString() const {
	std::string result = "";

	result = "Faction Name: " + name + "\n";

	result += "Unit Type List count = " + intToStr(this->getUnitTypeCount()) + "\n";
    for(int i=0; i<unitTypes.size();i++) {
		result += unitTypes[i].toString() + "\n";
    }

	result += "Upgrade Type List count = " + intToStr(this->getUpgradeTypeCount()) + "\n";
    for(int i=0; i<upgradeTypes.size();i++) {
		result += "index: " + intToStr(i) + " " + upgradeTypes[i].getReqDesc() + "\n";
    }

	return result;
}

}}//end namespace
