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

#include "tech_tree.h"

#include <cassert>

#include "util.h"
#include "resource.h"
#include "faction_type.h"
#include "logger.h"
#include "xml_parser.h"
#include "platform_util.h"
#include "game_util.h"
#include "leak_dumper.h"

using namespace Shared::Util;
using namespace Shared::Xml;

namespace Glest{ namespace Game{

// =====================================================
// 	class TechTree
// =====================================================

void TechTree::load(const string &dir, set<string> &factions, Checksum* checksum){

	string str;
    vector<string> filenames;
	string name= lastDir(dir);
		
	Logger::getInstance().add("TechTree: "+ formatString(name), true);

	//load resources
	str= dir+"/resources/*.";
    
    try{
        findAll(str, filenames);
		resourceTypes.resize(filenames.size());

        for(int i=0; i<filenames.size(); ++i){
            str=dir+"/resources/"+filenames[i];
            resourceTypes[i].load(str, checksum);
        }
    }
    catch(const exception &e){
		throw runtime_error("Error loading Resource Types: "+ dir + "\n" + e.what());
    }

	//load tech tree xml info
	try{
		XmlTree	xmlTree;
		string path= dir+"/"+lastDir(dir)+".xml";

		bool bCanProcessFile = true;
#ifdef _WINDOWS

		DWORD fileAttributes = GetFileAttributes(path.c_str());
		if( (fileAttributes & FILE_ATTRIBUTE_HIDDEN) == FILE_ATTRIBUTE_HIDDEN)
		{
			bCanProcessFile = false;
		}

#endif

		if(bCanProcessFile == true)
		{
			checksum->addFile(path);

			xmlTree.load(path);
			const XmlNode *techTreeNode= xmlTree.getRootNode();

			//attack types
			const XmlNode *attackTypesNode= techTreeNode->getChild("attack-types"); 
			attackTypes.resize(attackTypesNode->getChildCount());
			for(int i=0; i<attackTypes.size(); ++i){
				const XmlNode *attackTypeNode= attackTypesNode->getChild("attack-type", i);
				attackTypes[i].setName(attackTypeNode->getAttribute("name")->getRestrictedValue());
				attackTypes[i].setId(i);
			}

			//armor types
			const XmlNode *armorTypesNode= techTreeNode->getChild("armor-types"); 
			armorTypes.resize(armorTypesNode->getChildCount());
			for(int i=0; i<armorTypes.size(); ++i){
				const XmlNode *armorTypeNode= armorTypesNode->getChild("armor-type", i);
				armorTypes[i].setName(armorTypeNode->getAttribute("name")->getRestrictedValue());
				armorTypes[i].setId(i);
			}

			//damage multipliers
			damageMultiplierTable.init(attackTypes.size(), armorTypes.size());
			const XmlNode *damageMultipliersNode= techTreeNode->getChild("damage-multipliers"); 
			for(int i=0; i<damageMultipliersNode->getChildCount(); ++i){
				const XmlNode *damageMultiplierNode= damageMultipliersNode->getChild("damage-multiplier", i);
				const AttackType *attackType= getAttackType(damageMultiplierNode->getAttribute("attack")->getRestrictedValue());
				const ArmorType *armorType= getArmorType(damageMultiplierNode->getAttribute("armor")->getRestrictedValue());
				float multiplier= damageMultiplierNode->getAttribute("value")->getFloatValue();
				damageMultiplierTable.setDamageMultiplier(attackType, armorType, multiplier);
			}
		}
    }
    catch(const exception &e){
		throw runtime_error("Error loading Tech Tree: "+ dir + "\n" + e.what());
    }

	//load factions
	str= dir+"/factions/*.";
    try{
		factionTypes.resize(factions.size());

		int i=0;
		for ( set<string>::iterator it = factions.begin(); it != factions.end(); ++it ) {
			str=dir+"/factions/" + *it;
			factionTypes[i++].load(str, this, checksum);
        }
    }
	catch(const exception &e){
		throw runtime_error("Error loading Faction Types: "+ dir + "\n" + e.what());
    }

}

TechTree::~TechTree(){
	Logger::getInstance().add("Tech tree", true);
}


// ==================== get ==================== 

const FactionType *TechTree::getType(const string &name) const{    
     for(int i=0; i<factionTypes.size(); ++i){
          if(factionTypes[i].getName()==name){
               return &factionTypes[i];
          }
     }
	 throw runtime_error("Faction not found: "+name);
}

const ResourceType *TechTree::getTechResourceType(int i) const{
    
     for(int j=0; j<getResourceTypeCount(); ++j){
          const ResourceType *rt= getResourceType(j);
          if(rt->getResourceNumber()==i && rt->getClass()==rcTech)
               return getResourceType(j);
     }

     return getFirstTechResourceType();
}

const ResourceType *TechTree::getFirstTechResourceType() const{
     for(int i=0; i<getResourceTypeCount(); ++i){
          const ResourceType *rt= getResourceType(i);
          if(rt->getResourceNumber()==1 && rt->getClass()==rcTech)
               return getResourceType(i);
     }

	 throw runtime_error("This tech tree has not tech resources, one at least is required");
}

const ResourceType *TechTree::getResourceType(const string &name) const{
    
	for(int i=0; i<resourceTypes.size(); ++i){
		if(resourceTypes[i].getName()==name){
			return &resourceTypes[i];
		}
	}

	throw runtime_error("Resource Type not found: "+name);
}

const ArmorType *TechTree::getArmorType(const string &name) const{
	for(int i=0; i<armorTypes.size(); ++i){
		if(armorTypes[i].getName()==name){
			return &armorTypes[i];
		}
	}

	throw runtime_error("Armor Type not found: "+name);
}

const AttackType *TechTree::getAttackType(const string &name) const{
	for(int i=0; i<attackTypes.size(); ++i){
		if(attackTypes[i].getName()==name){
			return &attackTypes[i];
		}
	}

	throw runtime_error("Attack Type not found: "+name);
}

float TechTree::getDamageMultiplier(const AttackType *att, const ArmorType *art) const{
	return damageMultiplierTable.getDamageMultiplier(att, art);
}

}}//end namespace
