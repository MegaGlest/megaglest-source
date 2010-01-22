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

#include "upgrade_type.h"

#include <algorithm>
#include <cassert>

#include "unit_type.h" 
#include "util.h"
#include "logger.h"
#include "lang.h"
#include "xml_parser.h"
#include "tech_tree.h"
#include "faction_type.h"
#include "resource.h"
#include "renderer.h"
#include "game_util.h"
#include "leak_dumper.h"

using namespace Shared::Util;
using namespace Shared::Xml;

namespace Glest{ namespace Game{

// =====================================================
// 	class UpgradeType
// =====================================================

// ==================== get ==================== 

bool UpgradeType::isAffected(const UnitType *unitType) const{
	return find(effects.begin(), effects.end(), unitType)!=effects.end();
}

// ==================== misc ==================== 

void UpgradeType::preLoad(const string &dir){
	name=lastDir(dir);
}

void UpgradeType::load(const string &dir, const TechTree *techTree, const FactionType *factionType, Checksum* checksum){
	string path;

	Logger::getInstance().add("Upgrade type: "+ formatString(name), true);

	path=dir+"/"+name+".xml";

	try{
		checksum->addFile(path);

		XmlTree xmlTree;
		xmlTree.load(path);
		const XmlNode *upgradeNode= xmlTree.getRootNode();

		//image
		const XmlNode *imageNode= upgradeNode->getChild("image");
		image= Renderer::getInstance().newTexture2D(rsGame);
		image->load(dir+"/"+imageNode->getAttribute("path")->getRestrictedValue());

		//image cancel
		const XmlNode *imageCancelNode= upgradeNode->getChild("image-cancel");
		cancelImage= Renderer::getInstance().newTexture2D(rsGame);
		cancelImage->load(dir+"/"+imageCancelNode->getAttribute("path")->getRestrictedValue());

		//upgrade time
		const XmlNode *upgradeTimeNode= upgradeNode->getChild("time");
		productionTime= upgradeTimeNode->getAttribute("value")->getIntValue();
		
		//unit requirements
		const XmlNode *unitRequirementsNode= upgradeNode->getChild("unit-requirements");
		for(int i=0; i<unitRequirementsNode->getChildCount(); ++i){
			const XmlNode *unitNode= 	unitRequirementsNode->getChild("unit", i);
			string name= unitNode->getAttribute("name")->getRestrictedValue();
			unitReqs.push_back(factionType->getUnitType(name));
		}

		//upgrade requirements
		const XmlNode *upgradeRequirementsNode= upgradeNode->getChild("upgrade-requirements");
		for(int i=0; i<upgradeRequirementsNode->getChildCount(); ++i){
			const XmlNode *upgradeReqNode= upgradeRequirementsNode->getChild("upgrade", i);
			string name= upgradeReqNode->getAttribute("name")->getRestrictedValue();
			upgradeReqs.push_back(factionType->getUpgradeType(name));
		}

		//resource requirements
		const XmlNode *resourceRequirementsNode= upgradeNode->getChild("resource-requirements");
		costs.resize(resourceRequirementsNode->getChildCount());
		for(int i=0; i<costs.size(); ++i){
			const XmlNode *resourceNode= 	resourceRequirementsNode->getChild("resource", i);
			string name= resourceNode->getAttribute("name")->getRestrictedValue();
			int amount= resourceNode->getAttribute("amount")->getIntValue();
			costs[i].init(techTree->getResourceType(name), amount);
		}

		//effects
		const XmlNode *effectsNode= upgradeNode->getChild("effects");
		for(int i=0; i<effectsNode->getChildCount(); ++i){
			const XmlNode *unitNode= effectsNode->getChild("unit", i);
			string name= unitNode->getAttribute("name")->getRestrictedValue();
			effects.push_back(factionType->getUnitType(name));
		}

		//values
		maxHp= upgradeNode->getChild("max-hp")->getAttribute("value")->getIntValue();
		maxEp= upgradeNode->getChild("max-ep")->getAttribute("value")->getIntValue();
		sight= upgradeNode->getChild("sight")->getAttribute("value")->getIntValue();
		attackStrength= upgradeNode->getChild("attack-strenght")->getAttribute("value")->getIntValue();
		attackRange= upgradeNode->getChild("attack-range")->getAttribute("value")->getIntValue();
		armor= upgradeNode->getChild("armor")->getAttribute("value")->getIntValue();
		moveSpeed= upgradeNode->getChild("move-speed")->getAttribute("value")->getIntValue();
		prodSpeed= upgradeNode->getChild("production-speed")->getAttribute("value")->getIntValue();
		
	}
	catch(const exception &e){
		throw runtime_error("Error loading UpgradeType: "+ dir + "\n" +e.what());
	}
}

string UpgradeType::getReqDesc() const{
    
    string str;
    int i;
	Lang &lang= Lang::getInstance();

    str= ProducibleType::getReqDesc();
	if(getEffectCount()>0){
		str+= "\n"+ lang.get("Upgrades")+":\n";
		for(i=0; i<getEffectCount(); ++i){
			str+= getEffect(i)->getName()+"\n";
		} 
	}
    
	if(maxHp!=0){
        str+= lang.get("Hp")+" +"+intToStr(maxHp);
	}
	if(sight!=0){
        str+= lang.get("Sight")+" +"+intToStr(sight);
	}
	if(maxEp!=0){
		str+= lang.get("Ep")+" +"+intToStr(maxEp)+"\n";
	}
	if(attackStrength!=0){
        str+= lang.get("AttackStrenght")+" +"+intToStr(attackStrength)+"\n";
	}
	if(attackRange!=0){
        str+= lang.get("AttackDistance")+" +"+intToStr(attackRange)+"\n"; 
	}
	if(armor!=0){
		str+= lang.get("Armor")+" +"+intToStr(armor)+"\n";
	}
	if(moveSpeed!=0){
		str+= lang.get("WalkSpeed")+"+ "+intToStr(moveSpeed)+"\n";
	}
	if(prodSpeed!=0){
		str+= lang.get("ProductionSpeed")+" +"+intToStr(prodSpeed)+"\n";
	}

    return str;
}



// ===============================
// 	class TotalUpgrade  
// ===============================

TotalUpgrade::TotalUpgrade(){
	reset();
}

void TotalUpgrade::reset(){
    maxHp= 0;
    maxEp= 0;
    sight=0;
	armor= 0;
    attackStrength= 0;
    attackRange= 0;
    moveSpeed= 0;
    prodSpeed=0;
}

void TotalUpgrade::sum(const UpgradeType *ut){
	maxHp+= ut->getMaxHp();
	maxEp+= ut->getMaxEp();
	sight+= ut->getSight();
	armor+= ut->getArmor();
	attackStrength+= ut->getAttackStrength();
	attackRange+= ut->getAttackRange();
	moveSpeed+= ut->getMoveSpeed();
    prodSpeed+= ut->getProdSpeed();
}

void TotalUpgrade::incLevel(const UnitType *ut){
	maxHp+= ut->getMaxHp()*50/100;
	maxEp+= ut->getMaxEp()*50/100;
	sight+= ut->getSight()*20/100;
	armor+= ut->getArmor()*50/100;
}

}}//end namespace
