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
	music			= NULL;
	personalityType = fpt_Normal;
}

//load a faction, given a directory
void FactionType::load(const string &dir, const TechTree *techTree, Checksum* checksum,Checksum *techtreeChecksum) {

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    name= lastDir(dir);

    // Add special Observer Faction
    Lang &lang= Lang::getInstance();
    if(name == formatString(GameConstants::OBSERVER_SLOTNAME)) {
    	personalityType = fpt_Observer;
    }

	Logger::getInstance().add("Faction type: "+ formatString(name), true);

	if(personalityType == fpt_Normal) {
		// a1) preload units
		string unitsPath= dir + "/units/*.";
		vector<string> unitFilenames;
		findAll(unitsPath, unitFilenames);
		unitTypes.resize(unitFilenames.size());
		for(int i=0; i<unitTypes.size(); ++i) {
			string str= dir + "/units/" + unitFilenames[i];
			unitTypes[i].preLoad(str);

			SDL_PumpEvents();
		}

		// a2) preload upgrades
		string upgradesPath= dir + "/upgrades/*.";
		vector<string> upgradeFilenames;
		findAll(upgradesPath, upgradeFilenames, false, false);
		upgradeTypes.resize(upgradeFilenames.size());
		for(int i=0; i<upgradeTypes.size(); ++i) {
			string str= dir + "/upgrades/" + upgradeFilenames[i];
			upgradeTypes[i].preLoad(str);

			SDL_PumpEvents();
		}

		// b1) load units
		try{
			Logger &logger= Logger::getInstance();
			int progressBaseValue=logger.getProgress();
			for(int i = 0; i < unitTypes.size(); ++i) {
				string str= dir + "/units/" + unitTypes[i].getName();
				unitTypes[i].load(i, str, techTree, this, checksum,techtreeChecksum);
				logger.setProgress(progressBaseValue+(int)((((double)i + 1.0) / (double)unitTypes.size()) * 100.0/techTree->getTypeCount()));
				SDL_PumpEvents();
			}
		}
		catch(const exception &e) {
			SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
			throw runtime_error("Error loading units: "+ dir + "\n" + e.what());
		}

		// b2) load upgrades
		try{
			for(int i = 0; i < upgradeTypes.size(); ++i) {
				string str= dir + "/upgrades/" + upgradeTypes[i].getName();
				upgradeTypes[i].load(str, techTree, this, checksum,techtreeChecksum);

				SDL_PumpEvents();
			}
		}
		catch(const exception &e){
			SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
			throw runtime_error("Error loading upgrades: "+ dir + "\n" + e.what());
		}

		//open xml file
		string path= dir+"/"+name+".xml";
		checksum->addFile(path);
		techtreeChecksum->addFile(path);

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
	}
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

FactionType::~FactionType(){
	delete music;
	music = NULL;
}

std::vector<std::string> FactionType::validateFactionType() {
	std::vector<std::string> results;

    for(int i=0; i<unitTypes.size(); ++i){
    	UnitType &unitType = unitTypes[i];

    	for(int j = 0; j < unitType.getCommandTypeCount(); ++j) {
    		const CommandType *cmdType = unitType.getCommandType(j);
    		if(cmdType != NULL) {
    	    	// Check every unit's commands to validate that for every upgrade-requirements
    	    	// upgrade we have a unit that can do the upgrade in the faction.
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
										 break;
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

				// Ensure for each build type command that the build units
				// exist in this faction
				if(cmdType->getClass() == ccBuild) {
					const BuildCommandType *build = dynamic_cast<const BuildCommandType *>(cmdType);
					for(int k = 0; k < build->getBuildingCount(); ++k) {
						const UnitType *buildUnit = build->getBuilding(k);

						// Now lets find the unit that we should be able to build
						bool foundUnit = false;
						for(int l=0; l<unitTypes.size() && foundUnit == false; ++l){
							UnitType &unitType2 = unitTypes[l];
							if(unitType2.getName() == buildUnit->getName()) {
								foundUnit = true;

								// Now also validate the the unit to be built
								// has a be_built_skill
						    	if(buildUnit->hasSkillClass(scBeBuilt) == false) {
									char szBuf[4096]="";
									sprintf(szBuf,"The Unit [%s] in Faction [%s] has the command [%s]\nwhich can build the Unit [%s] but the Unit to be built\ndoes not have the skill class [be_built_skill] in this faction!",unitType.getName().c_str(),this->getName().c_str(),cmdType->getName().c_str(),buildUnit->getName().c_str());
									results.push_back(szBuf);
						    	}

						    	break;
							}
						}

						if(foundUnit == false) {
							char szBuf[4096]="";
							sprintf(szBuf,"The Unit [%s] in Faction [%s] has the command [%s]\nwhich can build the Unit [%s] but the Unit to be built does not exist in this faction!",unitType.getName().c_str(),this->getName().c_str(),cmdType->getName().c_str(),buildUnit->getName().c_str());
							results.push_back(szBuf);
						}
					}
				}
				// Ensure for each repair type command that the repair units
				// exist in this faction
				if(cmdType->getClass() == ccRepair) {
					const RepairCommandType *repair = dynamic_cast<const RepairCommandType *>(cmdType);
					for(int k = 0; k < repair->getRepairCount(); ++k) {
						const UnitType *repairUnit = repair->getRepair(k);

						// Now lets find the unit that we should be able to repair
						bool foundUnit = false;
						for(int l=0; l<unitTypes.size() && foundUnit == false; ++l){
							UnitType &unitType2 = unitTypes[l];
							if(unitType2.getName() == repairUnit->getName()) {
								 foundUnit = true;
								 break;
							}
						}

						if(foundUnit == false) {
							char szBuf[4096]="";
							sprintf(szBuf,"The Unit [%s] in Faction [%s] has the command [%s]\nwhich can repair the Unit [%s] but the Unit to be repaired does not exist in this faction!",unitType.getName().c_str(),this->getName().c_str(),cmdType->getName().c_str(),repairUnit->getName().c_str());
							results.push_back(szBuf);
						}
					}
				}
				// Ensure for each morph type command that the morph units
				// exist in this faction
				if(cmdType->getClass() == ccMorph) {
					const MorphCommandType *morph = dynamic_cast<const MorphCommandType *>(cmdType);
					if(morph != NULL) {
						const UnitType *morphUnit = morph->getMorphUnit();

						// Now lets find the unit that we should be able to morph
						// to
						bool foundUnit = false;
						for(int l=0; l<unitTypes.size() && foundUnit == false; ++l){
							UnitType &unitType2 = unitTypes[l];
							if(unitType2.getName() == morphUnit->getName()) {
								 foundUnit = true;
								 break;
							}
						}

						if(foundUnit == false) {
							char szBuf[4096]="";
							sprintf(szBuf,"The Unit [%s] in Faction [%s] has the command [%s]\nwhich can morph into the Unit [%s] but the Unit to be morphed to does not exist in this faction!",unitType.getName().c_str(),this->getName().c_str(),cmdType->getName().c_str(),morphUnit->getName().c_str());
							results.push_back(szBuf);
						}
					}
				}
    		}
    	}

		// Check every unit's unit requirements to validate that for every unit-requirements
		// we have the units required in the faction.
		for(int j = 0; j < unitType.getUnitReqCount(); ++j) {
			const UnitType *unitType2 = unitType.getUnitReq(j);
			if(unitType2 != NULL) {
				// Now lets find the required unit
				bool foundUnit = false;
				for(int l=0; l<unitTypes.size() && foundUnit == false; ++l){
					UnitType &unitType3 = unitTypes[l];

					if(unitType2->getName() == unitType3.getName()) {
						foundUnit = true;
						break;
					}
				}

				if(foundUnit == false) {
					char szBuf[4096]="";
					sprintf(szBuf,"The Unit [%s] in Faction [%s] has the required Unit [%s]\nbut the required unit does not exist in this faction!",unitType.getName().c_str(),this->getName().c_str(),unitType2->getName().c_str());
					results.push_back(szBuf);
				}
			}
		}

		// Now check that at least 1 other unit can produce, build or morph this unit
		bool foundUnit = false;
		for(int l=0; l<unitTypes.size() && foundUnit == false; ++l){
			UnitType &unitType2 = unitTypes[l];

	    	for(int j = 0; j < unitType2.getCommandTypeCount() && foundUnit == false; ++j) {
	    		const CommandType *cmdType = unitType2.getCommandType(j);
	    		if(cmdType != NULL) {
	    			// Check if this is a produce command
	    			if(cmdType->getClass() == ccProduce) {
	    				const ProduceCommandType *produce = dynamic_cast<const ProduceCommandType *>(cmdType);
						if(produce != NULL) {
							const UnitType *produceUnit = produce->getProducedUnit();

							if( produceUnit != NULL &&
								unitType.getId() != unitType2.getId() &&
								unitType.getName() == produceUnit->getName()) {
								 foundUnit = true;
								 break;
							}
						}
	    			}
	    			// Check if this is a build command
					if(cmdType->getClass() == ccBuild) {
						const BuildCommandType *build = dynamic_cast<const BuildCommandType *>(cmdType);
						for(int k = 0; k < build->getBuildingCount() && foundUnit == false; ++k) {
							const UnitType *buildUnit = build->getBuilding(k);

							if( buildUnit != NULL &&
								unitType.getId() != unitType2.getId() &&
								unitType.getName() == buildUnit->getName()) {
								 foundUnit = true;
								 break;
							}
						}
					}

	    			// Check if this is a morph command
					if(cmdType->getClass() == ccMorph) {
						const MorphCommandType *morph = dynamic_cast<const MorphCommandType *>(cmdType);
						const UnitType *morphUnit = morph->getMorphUnit();

						if( morphUnit != NULL &&
							unitType.getId() != unitType2.getId() &&
							unitType.getName() == morphUnit->getName()) {
							 foundUnit = true;
							 break;
						}
					}
	    		}
	    	}
		}

		if(foundUnit == false) {
			char szBuf[4096]="";
			sprintf(szBuf,"The Unit [%s] in Faction [%s] has no other units that can produce, build or morph into it in this faction!",unitType.getName().c_str(),this->getName().c_str());
			results.push_back(szBuf);
		}

        // Ensure that all attack skill types have valid values
        if(unitType.hasSkillClass(scAttack) == true) {
            for(int j = 0; j < unitType.getSkillTypeCount(); ++j) {
                const SkillType *st = unitType.getSkillType(j);
                if(st != NULL && dynamic_cast<const AttackSkillType *>(st) != NULL) {
                    const AttackSkillType *ast = dynamic_cast<const AttackSkillType *>(st);
                    if(ast->getAttackVar() < 0) {
                        char szBuf[4096]="";
                        sprintf(szBuf,"The Unit [%s] in Faction [%s] has the skill [%s] with an INVALID attack var value which is < 0 [%d]!",unitType.getName().c_str(),this->getName().c_str(),ast->getName().c_str(),ast->getAttackVar());
                        results.push_back(szBuf);
                    }
                }
            }
        }
        // end
    }

    return results;
}

std::vector<std::string> FactionType::validateFactionTypeResourceTypes(vector<ResourceType> &resourceTypes) {
	std::vector<std::string> results;

    for(int i=0; i<unitTypes.size(); ++i){
    	UnitType &unitType = unitTypes[i];

		// Check every unit's required resources to validate that for every resource-requirements
		// we have a resource in the faction.
    	for(int j = 0; j < unitType.getCostCount() ; ++j) {
    		const Resource *r = unitType.getCost(j);
    		if(r != NULL && r->getType() != NULL) {
    			bool foundResourceType = false;
    			// Now lets find a matching faction resource type for the unit
    			for(int k=0; k<resourceTypes.size(); ++k){
    				ResourceType &rt = resourceTypes[k];

					if(r->getType()->getName() == rt.getName()) {
						foundResourceType = true;
						break;
					}
				}

				if(foundResourceType == false) {
					char szBuf[4096]="";
					sprintf(szBuf,"The Unit [%s] in Faction [%s] has the resource req [%s]\nbut there are no such resources in this tech!",unitType.getName().c_str(),this->getName().c_str(),r->getType()->getName().c_str());
					results.push_back(szBuf);
				}
    		}
    	}

		// Check every unit's stored resources to validate that for every resources-stored
		// we have a resource in the faction.
		for(int j = 0; j < unitType.getStoredResourceCount() ; ++j) {
			const Resource *r = unitType.getStoredResource(j);
			if(r != NULL && r->getType() != NULL) {
				bool foundResourceType = false;
				// Now lets find a matching faction resource type for the unit
				for(int k=0; k<resourceTypes.size(); ++k){
					ResourceType &rt = resourceTypes[k];

					if(r->getType()->getName() == rt.getName()) {
						foundResourceType = true;
						break;
					}
				}

				if(foundResourceType == false) {
					char szBuf[4096]="";
					sprintf(szBuf,"The Unit [%s] in Faction [%s] has the stored resource [%s]\nbut there are no such resources in this tech!",unitType.getName().c_str(),this->getName().c_str(),r->getType()->getName().c_str());
					results.push_back(szBuf);
				}
			}
		}

    	for(int j = 0; j < unitType.getCommandTypeCount(); ++j) {
    		const CommandType *cmdType = unitType.getCommandType(j);
    		if(cmdType != NULL) {
				// Ensure for each harvest type command that the resource
				// exist in this faction
				if(cmdType->getClass() == ccHarvest) {
					const HarvestCommandType *harvest = dynamic_cast<const HarvestCommandType *>(cmdType);
					for(int k = 0; k < harvest->getHarvestedResourceCount(); ++k) {
						const ResourceType *harvestResource = harvest->getHarvestedResource(k);

						bool foundResourceType = false;
						// Now lets find a matching faction resource type for the unit
						for(int k=0; k<resourceTypes.size(); ++k){
							ResourceType &rt = resourceTypes[k];

							if(harvestResource->getName() == rt.getName()) {
								foundResourceType = true;
								break;
							}
						}

						if(foundResourceType == false) {
							char szBuf[4096]="";
							sprintf(szBuf,"The Unit [%s] in Faction [%s] has the command [%s] which can harvest the resource [%s]\nbut there are no such resources in this tech!",unitType.getName().c_str(),this->getName().c_str(),cmdType->getName().c_str(),harvestResource->getName().c_str());
							results.push_back(szBuf);
						}
					}
				}
    		}
    	}
    }

    return results;
}

std::vector<std::string> FactionType::validateFactionTypeUpgradeTypes() {
	std::vector<std::string> results;

	// For each upgrade type make sure there is at least 1 unit that can produce
	// the upgrade
	for(int i = 0; i < upgradeTypes.size(); ++i) {
		const UpgradeType &upgradeType = upgradeTypes[i];

		// First find a unit with a command type to upgrade to this Upgrade type
		bool foundUnit = false;
	    for(int j=0; j<unitTypes.size() && foundUnit == false; ++j){
	    	UnitType &unitType = unitTypes[j];
	    	for(int k = 0; k < unitType.getCommandTypeCount() && foundUnit == false; ++k) {
				const CommandType *cmdType = unitType.getCommandType(k);
				if(cmdType != NULL) {
					// Ensure for each build type command that the build units
					// exist in this faction
					if(cmdType->getClass() == ccUpgrade) {
						const UpgradeCommandType *upgrade = dynamic_cast<const UpgradeCommandType *>(cmdType);
						if(upgrade != NULL) {
							const UpgradeType *upgradeType2 = upgrade->getProducedUpgrade();

							if(upgradeType2 != NULL && upgradeType.getName() == upgradeType2->getName()) {
								 foundUnit = true;
								 break;
							}
						}
					}
				}
	    	}
	    }

		if(foundUnit == false) {
			char szBuf[4096]="";
			sprintf(szBuf,"The Upgrade Type [%s] in Faction [%s] has no Unit able to produce this upgrade in this faction!",upgradeType.getName().c_str(),this->getName().c_str());
			results.push_back(szBuf);
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

void FactionType::deletePixels() {
	for(int i = 0; i <unitTypes.size(); ++i) {
		UnitType &unitType = unitTypes[i];
		Texture2D *texture = unitType.getMeetingPointImage();
		if(texture != NULL) {
			texture->deletePixels();
		}
	}
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
