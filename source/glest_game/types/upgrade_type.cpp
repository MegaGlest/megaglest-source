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

void UpgradeTypeBase::load(const XmlNode *upgradeNode) {
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

bool UpgradeType::isAffected(const UnitType *unitType) const{
	return find(effects.begin(), effects.end(), unitType)!=effects.end();
}

// ==================== misc ====================

void UpgradeType::preLoad(const string &dir){
	name=lastDir(dir);
}

void UpgradeType::load(const string &dir, const TechTree *techTree,
		const FactionType *factionType, Checksum* checksum,
		Checksum* techtreeChecksum, std::map<string,vector<pair<string, string> > > &loadedFileList) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	Logger::getInstance().add("Upgrade type: "+ formatString(name), true);

	string currentPath = dir;
	endPathWithSlash(currentPath);
	string path = currentPath + name + ".xml";
	string sourceXMLFile = path;

	try {
		checksum->addFile(path);
		techtreeChecksum->addFile(path);

		XmlTree xmlTree;
		std::map<string,string> mapExtraTagReplacementValues;
		mapExtraTagReplacementValues["$COMMONDATAPATH"] = techTree->getPath() + "/commondata/";
		xmlTree.load(path, Properties::getTagReplacementValues(&mapExtraTagReplacementValues));
		loadedFileList[path].push_back(make_pair(currentPath,currentPath));
		const XmlNode *upgradeNode= xmlTree.getRootNode();

		//image
		const XmlNode *imageNode= upgradeNode->getChild("image");
		image= Renderer::getInstance().newTexture2D(rsGame);
		image->load(imageNode->getAttribute("path")->getRestrictedValue(currentPath,true));
		loadedFileList[imageNode->getAttribute("path")->getRestrictedValue(currentPath,true)].push_back(make_pair(sourceXMLFile,imageNode->getAttribute("path")->getRestrictedValue()));

		//if(fileExists(imageNode->getAttribute("path")->getRestrictedValue(currentPath,true)) == false) {
		//	printf("\n***ERROR MISSING FILE [%s]\n",imageNode->getAttribute("path")->getRestrictedValue(currentPath,true).c_str());
		//}

		//image cancel
		const XmlNode *imageCancelNode= upgradeNode->getChild("image-cancel");
		cancelImage= Renderer::getInstance().newTexture2D(rsGame);
		cancelImage->load(imageCancelNode->getAttribute("path")->getRestrictedValue(currentPath,true));
		loadedFileList[imageCancelNode->getAttribute("path")->getRestrictedValue(currentPath,true)].push_back(make_pair(sourceXMLFile,imageCancelNode->getAttribute("path")->getRestrictedValue()));

		//if(fileExists(imageCancelNode->getAttribute("path")->getRestrictedValue(currentPath,true)) == false) {
		//	printf("\n***ERROR MISSING FILE [%s]\n",imageCancelNode->getAttribute("path")->getRestrictedValue(currentPath,true).c_str());
		//}

		//upgrade time
		const XmlNode *upgradeTimeNode= upgradeNode->getChild("time");
		productionTime= upgradeTimeNode->getAttribute("value")->getIntValue();

		std::map<string,int> sortedItems;

		//unit requirements
		const XmlNode *unitRequirementsNode= upgradeNode->getChild("unit-requirements");
		for(int i = 0; i < unitRequirementsNode->getChildCount(); ++i) {
			const XmlNode *unitNode= 	unitRequirementsNode->getChild("unit", i);
			string name= unitNode->getAttribute("name")->getRestrictedValue();
			sortedItems[name] = 0;
		}
		for(std::map<string,int>::iterator iterMap = sortedItems.begin();
				iterMap != sortedItems.end(); ++iterMap) {
			unitReqs.push_back(factionType->getUnitType(iterMap->first));
		}
		sortedItems.clear();

		//upgrade requirements
		const XmlNode *upgradeRequirementsNode= upgradeNode->getChild("upgrade-requirements");
		for(int i = 0; i < upgradeRequirementsNode->getChildCount(); ++i) {
			const XmlNode *upgradeReqNode= upgradeRequirementsNode->getChild("upgrade", i);
			string name= upgradeReqNode->getAttribute("name")->getRestrictedValue();
			sortedItems[name] = 0;
		}
		for(std::map<string,int>::iterator iterMap = sortedItems.begin();
				iterMap != sortedItems.end(); ++iterMap) {
			upgradeReqs.push_back(factionType->getUpgradeType(iterMap->first));
		}
		sortedItems.clear();

		//resource requirements
		int index = 0;
		const XmlNode *resourceRequirementsNode= upgradeNode->getChild("resource-requirements");
		costs.resize(resourceRequirementsNode->getChildCount());
		for(int i = 0; i < costs.size(); ++i) {
			const XmlNode *resourceNode= 	resourceRequirementsNode->getChild("resource", i);
			string name= resourceNode->getAttribute("name")->getRestrictedValue();
			int amount= resourceNode->getAttribute("amount")->getIntValue();
			sortedItems[name] = amount;
		}
		index = 0;
		for(std::map<string,int>::iterator iterMap = sortedItems.begin();
				iterMap != sortedItems.end(); ++iterMap) {
			costs[index].init(techTree->getResourceType(iterMap->first), iterMap->second);
			index++;
		}
		sortedItems.clear();

		//effects
		const XmlNode *effectsNode= upgradeNode->getChild("effects");
		for(int i = 0; i < effectsNode->getChildCount(); ++i) {
			const XmlNode *unitNode= effectsNode->getChild("unit", i);
			string name= unitNode->getAttribute("name")->getRestrictedValue();
			sortedItems[name] = 0;
		}
		for(std::map<string,int>::iterator iterMap = sortedItems.begin();
				iterMap != sortedItems.end(); ++iterMap) {
			effects.push_back(factionType->getUnitType(iterMap->first));
		}
		sortedItems.clear();

		//values
//		maxHp= upgradeNode->getChild("max-hp")->getAttribute("value")->getIntValue();
//		maxEp= upgradeNode->getChild("max-ep")->getAttribute("value")->getIntValue();
//		sight= upgradeNode->getChild("sight")->getAttribute("value")->getIntValue();
//		attackStrength= upgradeNode->getChild("attack-strenght")->getAttribute("value")->getIntValue();
//		attackRange= upgradeNode->getChild("attack-range")->getAttribute("value")->getIntValue();
//		armor= upgradeNode->getChild("armor")->getAttribute("value")->getIntValue();
//		moveSpeed= upgradeNode->getChild("move-speed")->getAttribute("value")->getIntValue();
//		prodSpeed= upgradeNode->getChild("production-speed")->getAttribute("value")->getIntValue();
		UpgradeTypeBase::load(upgradeNode);
	}
	catch(const exception &e){
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
		throw runtime_error("Error loading UpgradeType: "+ dir + "\n" +e.what());
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
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

TotalUpgrade::TotalUpgrade() {
	reset();
}

void TotalUpgrade::reset() {
    maxHp= 0;
    maxEp= 0;
    sight=0;
	armor= 0;
    attackStrength= 0;
    attackRange= 0;
    moveSpeed= 0;
    prodSpeed=0;
}

void TotalUpgrade::sum(const UpgradeTypeBase *ut) {
	maxHp+= ut->getMaxHp();
	//maxHp = max(0,maxHp);
	maxEp+= ut->getMaxEp();
	//maxEp = max(0,maxEp);
	sight+= ut->getSight();
	//sight = max(0,sight);
	armor+= ut->getArmor();
	//armor = max(0,armor);
	attackStrength+= ut->getAttackStrength();
	//attackStrength = max(0,attackStrength);
	attackRange+= ut->getAttackRange();
	//attackRange = max(0,attackRange);
	moveSpeed+= ut->getMoveSpeed();
	//moveSpeed = max(0,moveSpeed);
    prodSpeed+= ut->getProdSpeed();
    //prodSpeed = max(0,prodSpeed);
}

void TotalUpgrade::incLevel(const UnitType *ut) {
	maxHp+= ut->getMaxHp()*50/100;
	maxEp+= ut->getMaxEp()*50/100;
	sight+= ut->getSight()*20/100;
	armor+= ut->getArmor()*50/100;
}

void TotalUpgrade::apply(const UpgradeTypeBase *ut) {
	sum(ut);
}

void TotalUpgrade::deapply(const UpgradeTypeBase *ut) {
	maxHp-= ut->getMaxHp();
	maxEp-= ut->getMaxEp();
	sight-= ut->getSight();
	armor-= ut->getArmor();
	attackStrength-= ut->getAttackStrength();
	attackRange-= ut->getAttackRange();
	moveSpeed-= ut->getMoveSpeed();
    prodSpeed-= ut->getProdSpeed();
}

}}//end namespace
