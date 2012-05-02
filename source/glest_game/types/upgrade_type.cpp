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
#include "faction.h"
#include "leak_dumper.h"

using namespace Shared::Util;
using namespace Shared::Xml;

namespace Glest{ namespace Game{

// =====================================================
// 	class UpgradeType
// =====================================================

// ==================== get ====================

const string VALUE_PERCENT_MULTIPLIER_KEY_NAME = "value-percent-multiplier";
const string VALUE_REGEN_KEY_NAME = "regeneration";

void UpgradeTypeBase::load(const XmlNode *upgradeNode, string upgradename) {
	this->upgradename = upgradename;
	//values
	maxHpIsMultiplier = false;
	maxHp= upgradeNode->getChild("max-hp")->getAttribute("value")->getIntValue();
	if(upgradeNode->getChild("max-hp")->getAttribute(VALUE_PERCENT_MULTIPLIER_KEY_NAME,false) != NULL) {
		maxHpIsMultiplier = upgradeNode->getChild("max-hp")->getAttribute(VALUE_PERCENT_MULTIPLIER_KEY_NAME)->getBoolValue();

		//printf("Found maxHpIsMultiplier = %d\n",maxHpIsMultiplier);
	}
	maxHpRegeneration = 0;
	//maxHpRegenerationIsMultiplier = false;
	if(upgradeNode->getChild("max-hp")->getAttribute(VALUE_REGEN_KEY_NAME,false) != NULL) {
		maxHpRegeneration = upgradeNode->getChild("max-hp")->getAttribute(VALUE_REGEN_KEY_NAME)->getIntValue();

		//printf("Found maxHpIsMultiplier = %d\n",maxHpIsMultiplier);
	}

	maxEpIsMultiplier = false;
	maxEp= upgradeNode->getChild("max-ep")->getAttribute("value")->getIntValue();
	if(upgradeNode->getChild("max-ep")->getAttribute(VALUE_PERCENT_MULTIPLIER_KEY_NAME,false) != NULL) {
		maxEpIsMultiplier = upgradeNode->getChild("max-ep")->getAttribute(VALUE_PERCENT_MULTIPLIER_KEY_NAME)->getBoolValue();

		//printf("Found maxEpIsMultiplier = %d\n",maxEpIsMultiplier);
	}
	maxEpRegeneration = 0;
	//maxEpRegenerationIsMultiplier = false;
	if(upgradeNode->getChild("max-ep")->getAttribute(VALUE_REGEN_KEY_NAME,false) != NULL) {
		maxEpRegeneration = upgradeNode->getChild("max-ep")->getAttribute(VALUE_REGEN_KEY_NAME)->getIntValue();

		//printf("Found maxHpIsMultiplier = %d\n",maxHpIsMultiplier);
	}

	sightIsMultiplier = false;
	sight= upgradeNode->getChild("sight")->getAttribute("value")->getIntValue();
	if(upgradeNode->getChild("sight")->getAttribute(VALUE_PERCENT_MULTIPLIER_KEY_NAME,false) != NULL) {
		sightIsMultiplier = upgradeNode->getChild("sight")->getAttribute(VALUE_PERCENT_MULTIPLIER_KEY_NAME)->getBoolValue();

		//printf("Found sightIsMultiplier = %d\n",sightIsMultiplier);
	}

	attackStrengthIsMultiplier = false;
	attackStrength= upgradeNode->getChild("attack-strenght")->getAttribute("value")->getIntValue();
	if(upgradeNode->getChild("attack-strenght")->getAttribute(VALUE_PERCENT_MULTIPLIER_KEY_NAME,false) != NULL) {
		attackStrengthIsMultiplier = upgradeNode->getChild("attack-strenght")->getAttribute(VALUE_PERCENT_MULTIPLIER_KEY_NAME)->getBoolValue();

		//printf("Found attackStrengthIsMultiplier = %d\n",attackStrengthIsMultiplier);
	}

	attackRangeIsMultiplier = false;
	attackRange= upgradeNode->getChild("attack-range")->getAttribute("value")->getIntValue();
	if(upgradeNode->getChild("attack-range")->getAttribute(VALUE_PERCENT_MULTIPLIER_KEY_NAME,false) != NULL) {
		attackRangeIsMultiplier = upgradeNode->getChild("attack-range")->getAttribute(VALUE_PERCENT_MULTIPLIER_KEY_NAME)->getBoolValue();

		//printf("Found attackRangeIsMultiplier = %d\n",attackRangeIsMultiplier);
	}

	armorIsMultiplier = false;
	armor= upgradeNode->getChild("armor")->getAttribute("value")->getIntValue();
	if(upgradeNode->getChild("armor")->getAttribute(VALUE_PERCENT_MULTIPLIER_KEY_NAME,false) != NULL) {
		armorIsMultiplier = upgradeNode->getChild("armor")->getAttribute(VALUE_PERCENT_MULTIPLIER_KEY_NAME)->getBoolValue();

		//printf("Found armorIsMultiplier = %d\n",armorIsMultiplier);
	}

	moveSpeedIsMultiplier = false;
	moveSpeed= upgradeNode->getChild("move-speed")->getAttribute("value")->getIntValue();
	if(upgradeNode->getChild("move-speed")->getAttribute(VALUE_PERCENT_MULTIPLIER_KEY_NAME,false) != NULL) {
		moveSpeedIsMultiplier = upgradeNode->getChild("move-speed")->getAttribute(VALUE_PERCENT_MULTIPLIER_KEY_NAME)->getBoolValue();

		//printf("Found moveSpeedIsMultiplier = %d\n",moveSpeedIsMultiplier);
	}

	prodSpeedIsMultiplier = false;
	prodSpeed= upgradeNode->getChild("production-speed")->getAttribute("value")->getIntValue();
	if(upgradeNode->getChild("production-speed")->getAttribute(VALUE_PERCENT_MULTIPLIER_KEY_NAME,false) != NULL) {
		prodSpeedIsMultiplier = upgradeNode->getChild("production-speed")->getAttribute(VALUE_PERCENT_MULTIPLIER_KEY_NAME)->getBoolValue();

		//printf("Found prodSpeedIsMultiplier = %d\n",prodSpeedIsMultiplier);
	}
}

int UpgradeTypeBase::getAttackStrength(const AttackSkillType *st) const	{
	if(attackStrengthIsMultiplier == false || st == NULL) {
		return attackStrength;
	}
	else {
		int result = 0;
		if(attackStrengthMultiplierValueList.find(st->getName()) != attackStrengthMultiplierValueList.end()) {
			result = attackStrengthMultiplierValueList.find(st->getName())->second;
		}
		return result;
	}
}
int UpgradeTypeBase::getAttackRange(const AttackSkillType *st) const {
	if(attackRangeIsMultiplier == false || st == NULL) {
		return attackRange;
	}
	else {
		int result = 0;
		if(attackRangeMultiplierValueList.find(st->getName()) != attackRangeMultiplierValueList.end()) {
			result = attackRangeMultiplierValueList.find(st->getName())->second;
		}
		return result;
	}
}

int UpgradeTypeBase::getMoveSpeed(const MoveSkillType *st) const {
	if(moveSpeedIsMultiplier == false || st == NULL) {
		//printf("getMoveSpeed moveSpeedIsMultiplier OFF st [%p]\n",st);
		return moveSpeed;
	}
	else {
		int result = 0;
		if(moveSpeedIsMultiplierValueList.find(st->getName()) != moveSpeedIsMultiplierValueList.end()) {
			result = moveSpeedIsMultiplierValueList.find(st->getName())->second;
		}

		//printf("getMoveSpeed moveSpeedIsMultiplier mst->getSpeed() = %d for skill [%s] result = %d\n",st->getSpeed(),st->getName().c_str(),result);
		return result;
	}
}

int UpgradeTypeBase::getProdSpeed(const SkillType *st) const {
	if(prodSpeedIsMultiplier == false || st == NULL) {
		return prodSpeed;
	}
	else {
		int result = 0;
		if(dynamic_cast<const ProduceSkillType *>(st) != NULL) {
			if(prodSpeedProduceIsMultiplierValueList.find(st->getName()) != prodSpeedProduceIsMultiplierValueList.end()) {
				result = prodSpeedProduceIsMultiplierValueList.find(st->getName())->second;
			}
		}
		else if(dynamic_cast<const UpgradeSkillType *>(st) != NULL) {
			if(prodSpeedUpgradeIsMultiplierValueList.find(st->getName()) != prodSpeedUpgradeIsMultiplierValueList.end()) {
				result = prodSpeedUpgradeIsMultiplierValueList.find(st->getName())->second;
			}
		}
		else if(dynamic_cast<const MorphSkillType *>(st) != NULL) {
			if(prodSpeedMorphIsMultiplierValueList.find(st->getName()) != prodSpeedMorphIsMultiplierValueList.end()) {
				result = prodSpeedMorphIsMultiplierValueList.find(st->getName())->second;
			}
		}
		else {
			throw megaglest_runtime_error("Unsupported skilltype in getProdSpeed!");
		}

		return result;
	}
}

string UpgradeTypeBase::getDesc() const{

    string str="";
    string indent="->";
    //int i;
	Lang &lang= Lang::getInstance();

	if(maxHp != 0) {
		if(maxHpIsMultiplier) {
			str += indent+lang.get("Hp") + " *" + intToStr(maxHp);
		}
		else {
			str += indent+lang.get("Hp") + " +" + intToStr(maxHp);
		}

		if(maxHpRegeneration != 0) {
			str += " [" + intToStr(maxHpRegeneration) + "]";
		}
	}
	if(sight != 0) {
		if(sightIsMultiplier) {
			str+= indent+lang.get("Sight") + " *" + intToStr(sight);
		}
		else {
			str+= indent+lang.get("Sight") + " +" + intToStr(sight);
		}
	}
	if(maxEp != 0) {
		if(maxEpIsMultiplier) {
			str+= indent+lang.get("Ep") + " *" + intToStr(maxEp)+"\n";
		}
		else {
			str+= indent+lang.get("Ep") + " +" + intToStr(maxEp)+"\n";
		}
		if(maxEpRegeneration != 0) {
			str += " [" + intToStr(maxEpRegeneration) + "]";
		}
	}
	if(attackStrength != 0) {
		if(attackStrengthIsMultiplier) {
			str+= indent+lang.get("AttackStrenght") + " *" + intToStr(attackStrength)+"\n";
		}
		else {
			str+= indent+lang.get("AttackStrenght") + " +" + intToStr(attackStrength)+"\n";
		}
	}
	if(attackRange != 0) {
		if(attackRangeIsMultiplier) {
			str+= indent+lang.get("AttackDistance") + " *" + intToStr(attackRange)+"\n";
		}
		else {
			str+= indent+lang.get("AttackDistance") + " +" + intToStr(attackRange)+"\n";
		}
	}
	if(armor != 0) {
		if(armorIsMultiplier) {
			str+= indent+lang.get("Armor") + " *" + intToStr(armor)+"\n";
		}
		else {
			str+= indent+lang.get("Armor") + " +" + intToStr(armor)+"\n";
		}
	}
	if(moveSpeed != 0) {
		if(moveSpeedIsMultiplier) {
			str+= indent+lang.get("WalkSpeed") + " *" + intToStr(moveSpeed)+"\n";
		}
		else {
			str+= indent+lang.get("WalkSpeed") + " +" + intToStr(moveSpeed)+"\n";
		}
	}
	if(prodSpeed != 0) {
		if(prodSpeedIsMultiplier) {
			str+= indent+lang.get("ProductionSpeed") + " *" + intToStr(prodSpeed)+"\n";
		}
		else {
			str+= indent+lang.get("ProductionSpeed") + " +" + intToStr(prodSpeed)+"\n";
		}
	}

    return str;
}

void UpgradeTypeBase::saveGame(XmlNode *rootNode) const {
	std::map<string,string> mapTagReplacements;
	XmlNode *upgradeTypeBaseNode = rootNode->addChild("UpgradeTypeBase");

	upgradeTypeBaseNode->addAttribute("upgradename",upgradename, mapTagReplacements);

////    int maxHp;
//	upgradeTypeBaseNode->addAttribute("maxHp",intToStr(maxHp), mapTagReplacements);
////    bool maxHpIsMultiplier;
//	upgradeTypeBaseNode->addAttribute("maxHpIsMultiplier",intToStr(maxHpIsMultiplier), mapTagReplacements);
////	int maxHpRegeneration;
//	upgradeTypeBaseNode->addAttribute("maxHpRegeneration",intToStr(maxHpRegeneration), mapTagReplacements);
////	//bool maxHpRegenerationIsMultiplier;
////
////    int sight;
//	upgradeTypeBaseNode->addAttribute("sight",intToStr(sight), mapTagReplacements);
////    bool sightIsMultiplier;
//	upgradeTypeBaseNode->addAttribute("sightIsMultiplier",intToStr(sightIsMultiplier), mapTagReplacements);
////    int maxEp;
//	upgradeTypeBaseNode->addAttribute("maxEp",intToStr(maxEp), mapTagReplacements);
////    bool maxEpIsMultiplier;
//	upgradeTypeBaseNode->addAttribute("maxEpIsMultiplier",intToStr(maxEpIsMultiplier), mapTagReplacements);
////	int maxEpRegeneration;
//	upgradeTypeBaseNode->addAttribute("maxEpRegeneration",intToStr(maxEpRegeneration), mapTagReplacements);
////	//bool maxEpRegenerationIsMultiplier;
////    int armor;
//	upgradeTypeBaseNode->addAttribute("armor",intToStr(armor), mapTagReplacements);
////    bool armorIsMultiplier;
//	upgradeTypeBaseNode->addAttribute("armorIsMultiplier",intToStr(armorIsMultiplier), mapTagReplacements);
////    int attackStrength;
//	upgradeTypeBaseNode->addAttribute("attackStrength",intToStr(attackStrength), mapTagReplacements);
////    bool attackStrengthIsMultiplier;
//	upgradeTypeBaseNode->addAttribute("attackStrengthIsMultiplier",intToStr(attackStrengthIsMultiplier), mapTagReplacements);
////    std::map<string,int> attackStrengthMultiplierValueList;
//	for(std::map<string,int>::const_iterator iterMap = attackStrengthMultiplierValueList.begin();
//			iterMap != attackStrengthMultiplierValueList.end(); ++iterMap) {
//		XmlNode *attackStrengthMultiplierValueListNode = upgradeTypeBaseNode->addChild("attackStrengthMultiplierValueList");
//
//		attackStrengthMultiplierValueListNode->addAttribute("key",iterMap->first, mapTagReplacements);
//		attackStrengthMultiplierValueListNode->addAttribute("value",intToStr(iterMap->second), mapTagReplacements);
//	}
////    int attackRange;
//	upgradeTypeBaseNode->addAttribute("attackRange",intToStr(attackRange), mapTagReplacements);
////    bool attackRangeIsMultiplier;
//	upgradeTypeBaseNode->addAttribute("attackRangeIsMultiplier",intToStr(attackRangeIsMultiplier), mapTagReplacements);
////    std::map<string,int> attackRangeMultiplierValueList;
//	for(std::map<string,int>::const_iterator iterMap = attackRangeMultiplierValueList.begin();
//			iterMap != attackRangeMultiplierValueList.end(); ++iterMap) {
//		XmlNode *attackRangeMultiplierValueListNode = upgradeTypeBaseNode->addChild("attackRangeMultiplierValueList");
//
//		attackRangeMultiplierValueListNode->addAttribute("key",iterMap->first, mapTagReplacements);
//		attackRangeMultiplierValueListNode->addAttribute("value",intToStr(iterMap->second), mapTagReplacements);
//	}
//
////    int moveSpeed;
//	upgradeTypeBaseNode->addAttribute("moveSpeed",intToStr(moveSpeed), mapTagReplacements);
////    bool moveSpeedIsMultiplier;
//	upgradeTypeBaseNode->addAttribute("moveSpeedIsMultiplier",intToStr(moveSpeedIsMultiplier), mapTagReplacements);
////    std::map<string,int> moveSpeedIsMultiplierValueList;
//	for(std::map<string,int>::const_iterator iterMap = moveSpeedIsMultiplierValueList.begin();
//			iterMap != moveSpeedIsMultiplierValueList.end(); ++iterMap) {
//		XmlNode *moveSpeedIsMultiplierValueListNode = upgradeTypeBaseNode->addChild("moveSpeedIsMultiplierValueList");
//
//		moveSpeedIsMultiplierValueListNode->addAttribute("key",iterMap->first, mapTagReplacements);
//		moveSpeedIsMultiplierValueListNode->addAttribute("value",intToStr(iterMap->second), mapTagReplacements);
//	}
//
////    int prodSpeed;
//	upgradeTypeBaseNode->addAttribute("prodSpeed",intToStr(prodSpeed), mapTagReplacements);
////    bool prodSpeedIsMultiplier;
//	upgradeTypeBaseNode->addAttribute("prodSpeedIsMultiplier",intToStr(prodSpeedIsMultiplier), mapTagReplacements);
////    std::map<string,int> prodSpeedProduceIsMultiplierValueList;
//	for(std::map<string,int>::const_iterator iterMap = prodSpeedProduceIsMultiplierValueList.begin();
//			iterMap != prodSpeedProduceIsMultiplierValueList.end(); ++iterMap) {
//		XmlNode *prodSpeedProduceIsMultiplierValueListNode = upgradeTypeBaseNode->addChild("prodSpeedProduceIsMultiplierValueList");
//
//		prodSpeedProduceIsMultiplierValueListNode->addAttribute("key",iterMap->first, mapTagReplacements);
//		prodSpeedProduceIsMultiplierValueListNode->addAttribute("value",intToStr(iterMap->second), mapTagReplacements);
//	}
//
////    std::map<string,int> prodSpeedUpgradeIsMultiplierValueList;
//	for(std::map<string,int>::const_iterator iterMap = prodSpeedUpgradeIsMultiplierValueList.begin();
//			iterMap != prodSpeedUpgradeIsMultiplierValueList.end(); ++iterMap) {
//		XmlNode *prodSpeedUpgradeIsMultiplierValueListNode = upgradeTypeBaseNode->addChild("prodSpeedUpgradeIsMultiplierValueList");
//
//		prodSpeedUpgradeIsMultiplierValueListNode->addAttribute("key",iterMap->first, mapTagReplacements);
//		prodSpeedUpgradeIsMultiplierValueListNode->addAttribute("value",intToStr(iterMap->second), mapTagReplacements);
//	}
//
////    std::map<string,int> prodSpeedMorphIsMultiplierValueList;
//	for(std::map<string,int>::const_iterator iterMap = prodSpeedMorphIsMultiplierValueList.begin();
//			iterMap != prodSpeedMorphIsMultiplierValueList.end(); ++iterMap) {
//		XmlNode *prodSpeedMorphIsMultiplierValueListNode = upgradeTypeBaseNode->addChild("prodSpeedMorphIsMultiplierValueList");
//
//		prodSpeedMorphIsMultiplierValueListNode->addAttribute("key",iterMap->first, mapTagReplacements);
//		prodSpeedMorphIsMultiplierValueListNode->addAttribute("value",intToStr(iterMap->second), mapTagReplacements);
//	}
}

const UpgradeType * UpgradeTypeBase::loadGame(const XmlNode *rootNode, Faction *faction) {
	const XmlNode *upgradeTypeBaseNode = rootNode->getChild("UpgradeTypeBase");

	//description = upgradeTypeBaseNode->getAttribute("description")->getValue();

	string upgradename = upgradeTypeBaseNode->getAttribute("upgradename")->getValue();
	return faction->getType()->getUpgradeType(upgradename);
	//    int maxHp;
//	maxHp = upgradeTypeBaseNode->getAttribute("maxHp")->getIntValue();
//	//    bool maxHpIsMultiplier;
//	maxHpIsMultiplier = upgradeTypeBaseNode->getAttribute("maxHpIsMultiplier")->getIntValue();
//	//	int maxHpRegeneration;
//	maxHpRegeneration = upgradeTypeBaseNode->getAttribute("maxHpRegeneration")->getIntValue();
//	//	//bool maxHpRegenerationIsMultiplier;
//	//
//	//    int sight;
//	sight = upgradeTypeBaseNode->getAttribute("sight")->getIntValue();
//	//    bool sightIsMultiplier;
//	sightIsMultiplier = upgradeTypeBaseNode->getAttribute("sightIsMultiplier")->getIntValue();
//	//    int maxEp;
//	maxEp = upgradeTypeBaseNode->getAttribute("maxEp")->getIntValue();
//	//    bool maxEpIsMultiplier;
//	maxEpIsMultiplier = upgradeTypeBaseNode->getAttribute("maxEpIsMultiplier")->getIntValue();
//	//	int maxEpRegeneration;
//	maxEpRegeneration = upgradeTypeBaseNode->getAttribute("maxEpRegeneration")->getIntValue();
//	//	//bool maxEpRegenerationIsMultiplier;
//	//    int armor;
//	armor = upgradeTypeBaseNode->getAttribute("armor")->getIntValue();
//	//    bool armorIsMultiplier;
//	armorIsMultiplier = upgradeTypeBaseNode->getAttribute("armorIsMultiplier")->getIntValue();
//	//    int attackStrength;
//	attackStrength = upgradeTypeBaseNode->getAttribute("attackStrength")->getIntValue();
//	//    bool attackStrengthIsMultiplier;
//	attackStrengthIsMultiplier = upgradeTypeBaseNode->getAttribute("attackStrengthIsMultiplier")->getIntValue();
//	//    std::map<string,int> attackStrengthMultiplierValueList;
//	vector<XmlNode *> attackStrengthMultiplierValueNodeList = upgradeTypeBaseNode->getChildList("attackStrengthMultiplierValueList");
//	for(unsigned int i = 0; i < attackStrengthMultiplierValueNodeList.size(); ++i) {
//		XmlNode *node = attackStrengthMultiplierValueNodeList[i];
//
//		attackStrengthMultiplierValueList[node->getAttribute("key")->getValue()] =
//		                                  node->getAttribute("value")->getIntValue();
//	}
//	//    int attackRange;
//	attackRange = upgradeTypeBaseNode->getAttribute("attackRange")->getIntValue();
//	//    bool attackRangeIsMultiplier;
//	attackRangeIsMultiplier = upgradeTypeBaseNode->getAttribute("attackRangeIsMultiplier")->getIntValue();
//	//    std::map<string,int> attackRangeMultiplierValueList;
//	vector<XmlNode *> attackRangeMultiplierValueNodeList = upgradeTypeBaseNode->getChildList("attackRangeMultiplierValueList");
//	for(unsigned int i = 0; i < attackRangeMultiplierValueNodeList.size(); ++i) {
//		XmlNode *node = attackRangeMultiplierValueNodeList[i];
//
//		attackRangeMultiplierValueList[node->getAttribute("key")->getValue()] =
//		                                  node->getAttribute("value")->getIntValue();
//	}
//
//	//    int moveSpeed;
//	moveSpeed = upgradeTypeBaseNode->getAttribute("moveSpeed")->getIntValue();
//	//    bool moveSpeedIsMultiplier;
//	moveSpeedIsMultiplier = upgradeTypeBaseNode->getAttribute("moveSpeedIsMultiplier")->getIntValue();
//	//    std::map<string,int> moveSpeedIsMultiplierValueList;
//	vector<XmlNode *> moveSpeedIsMultiplierValueNodeList = upgradeTypeBaseNode->getChildList("moveSpeedIsMultiplierValueList");
//	for(unsigned int i = 0; i < moveSpeedIsMultiplierValueNodeList.size(); ++i) {
//		XmlNode *node = moveSpeedIsMultiplierValueNodeList[i];
//
//		moveSpeedIsMultiplierValueList[node->getAttribute("key")->getValue()] =
//		                                  node->getAttribute("value")->getIntValue();
//	}
//
//	//    int prodSpeed;
//	prodSpeed = upgradeTypeBaseNode->getAttribute("prodSpeed")->getIntValue();
//	//    bool prodSpeedIsMultiplier;
//	prodSpeedIsMultiplier = upgradeTypeBaseNode->getAttribute("prodSpeedIsMultiplier")->getIntValue();
//	//    std::map<string,int> prodSpeedProduceIsMultiplierValueList;
//	vector<XmlNode *> prodSpeedProduceIsMultiplierValueNodeList = upgradeTypeBaseNode->getChildList("prodSpeedProduceIsMultiplierValueList");
//	for(unsigned int i = 0; i < prodSpeedProduceIsMultiplierValueNodeList.size(); ++i) {
//		XmlNode *node = prodSpeedProduceIsMultiplierValueNodeList[i];
//
//		prodSpeedProduceIsMultiplierValueList[node->getAttribute("key")->getValue()] =
//		                                  node->getAttribute("value")->getIntValue();
//	}
//
//	//    std::map<string,int> prodSpeedUpgradeIsMultiplierValueList;
//	vector<XmlNode *> prodSpeedUpgradeIsMultiplierValueNodeList = upgradeTypeBaseNode->getChildList("prodSpeedUpgradeIsMultiplierValueList");
//	for(unsigned int i = 0; i < prodSpeedUpgradeIsMultiplierValueNodeList.size(); ++i) {
//		XmlNode *node = prodSpeedUpgradeIsMultiplierValueNodeList[i];
//
//		prodSpeedUpgradeIsMultiplierValueList[node->getAttribute("key")->getValue()] =
//		                                  node->getAttribute("value")->getIntValue();
//	}
//
//	//    std::map<string,int> prodSpeedMorphIsMultiplierValueList;
//	vector<XmlNode *> prodSpeedMorphIsMultiplierValueNodeList = upgradeTypeBaseNode->getChildList("prodSpeedMorphIsMultiplierValueList");
//	for(unsigned int i = 0; i < prodSpeedMorphIsMultiplierValueNodeList.size(); ++i) {
//		XmlNode *node = prodSpeedMorphIsMultiplierValueNodeList[i];
//
//		prodSpeedMorphIsMultiplierValueList[node->getAttribute("key")->getValue()] =
//		                                  node->getAttribute("value")->getIntValue();
//	}
}

// ==================== misc ====================

string UpgradeType::getName(bool translatedValue) const {
	if(translatedValue == false) return name;

	Lang &lang = Lang::getInstance();
	return lang.getTechTreeString("UpgradeTypeName_" + name,name.c_str());
}

string UpgradeType::getReqDesc() const{
	Lang &lang= Lang::getInstance();
    string str= ProducibleType::getReqDesc();
    string indent="  ";
	if(getEffectCount()>0){
		str+= "\n"+ lang.get("Upgrades")+"\n";
	}
	str+=UpgradeTypeBase::getDesc();
	if(getEffectCount()>0){
		str+= lang.get("AffectedUnits")+"\n";
		for(int i=0; i<getEffectCount(); ++i){
					str+= indent+getEffect(i)->getName(true)+"\n";
		}
	}
	return str;
}

void UpgradeType::preLoad(const string &dir){
	name=lastDir(dir);
}

void UpgradeType::load(const string &dir, const TechTree *techTree,
		const FactionType *factionType, Checksum* checksum,
		Checksum* techtreeChecksum, std::map<string,vector<pair<string, string> > > &loadedFileList) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	char szBuf[1024]="";
	sprintf(szBuf,Lang::getInstance().get("LogScreenGameLoadingUpgradeType","",true).c_str(),formatString(name).c_str());
	Logger::getInstance().add(szBuf, true);

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
		if(image) {
			image->load(imageNode->getAttribute("path")->getRestrictedValue(currentPath,true));
		}
		loadedFileList[imageNode->getAttribute("path")->getRestrictedValue(currentPath,true)].push_back(make_pair(sourceXMLFile,imageNode->getAttribute("path")->getRestrictedValue()));

		//if(fileExists(imageNode->getAttribute("path")->getRestrictedValue(currentPath,true)) == false) {
		//	printf("\n***ERROR MISSING FILE [%s]\n",imageNode->getAttribute("path")->getRestrictedValue(currentPath,true).c_str());
		//}

		//image cancel
		const XmlNode *imageCancelNode= upgradeNode->getChild("image-cancel");
		cancelImage= Renderer::getInstance().newTexture2D(rsGame);
		if(cancelImage) {
			cancelImage->load(imageCancelNode->getAttribute("path")->getRestrictedValue(currentPath,true));
		}
		loadedFileList[imageCancelNode->getAttribute("path")->getRestrictedValue(currentPath,true)].push_back(make_pair(sourceXMLFile,imageCancelNode->getAttribute("path")->getRestrictedValue()));

		//if(fileExists(imageCancelNode->getAttribute("path")->getRestrictedValue(currentPath,true)) == false) {
		//	printf("\n***ERROR MISSING FILE [%s]\n",imageCancelNode->getAttribute("path")->getRestrictedValue(currentPath,true).c_str());
		//}

		//upgrade time
		const XmlNode *upgradeTimeNode= upgradeNode->getChild("time");
		productionTime= upgradeTimeNode->getAttribute("value")->getIntValue();

		std::map<string,int> sortedItems;

		//unit requirements
		bool hasDup = false;
		const XmlNode *unitRequirementsNode= upgradeNode->getChild("unit-requirements");
		for(int i = 0; i < unitRequirementsNode->getChildCount(); ++i) {
			const XmlNode *unitNode= 	unitRequirementsNode->getChild("unit", i);
			string name= unitNode->getAttribute("name")->getRestrictedValue();

			if(sortedItems.find(name) != sortedItems.end()) {
				hasDup = true;
			}

			sortedItems[name] = 0;
		}

		if(hasDup) {
			printf("WARNING, upgrade type [%s] has one or more duplicate unit requirements\n",this->getName().c_str());
		}

		for(std::map<string,int>::iterator iterMap = sortedItems.begin();
				iterMap != sortedItems.end(); ++iterMap) {
			unitReqs.push_back(factionType->getUnitType(iterMap->first));
		}
		sortedItems.clear();
		hasDup = false;

		//upgrade requirements
		const XmlNode *upgradeRequirementsNode= upgradeNode->getChild("upgrade-requirements");
		for(int i = 0; i < upgradeRequirementsNode->getChildCount(); ++i) {
			const XmlNode *upgradeReqNode= upgradeRequirementsNode->getChild("upgrade", i);
			string name= upgradeReqNode->getAttribute("name")->getRestrictedValue();

			if(sortedItems.find(name) != sortedItems.end()) {
				hasDup = true;
			}

			sortedItems[name] = 0;
		}

		if(hasDup) {
			printf("WARNING, upgrade type [%s] has one or more duplicate upgrade requirements\n",this->getName().c_str());
		}

		for(std::map<string,int>::iterator iterMap = sortedItems.begin();
				iterMap != sortedItems.end(); ++iterMap) {
			upgradeReqs.push_back(factionType->getUpgradeType(iterMap->first));
		}
		sortedItems.clear();
		hasDup = false;

		//resource requirements
		int index = 0;
		const XmlNode *resourceRequirementsNode= upgradeNode->getChild("resource-requirements");
		hasDup = false;
		costs.resize(resourceRequirementsNode->getChildCount());
		for(int i = 0; i < costs.size(); ++i) {
			const XmlNode *resourceNode= 	resourceRequirementsNode->getChild("resource", i);
			string name= resourceNode->getAttribute("name")->getRestrictedValue();
			int amount= resourceNode->getAttribute("amount")->getIntValue();

			if(sortedItems.find(name) != sortedItems.end()) {
				hasDup = true;
			}

			sortedItems[name] = amount;
		}

		//if(hasDup || sortedItems.size() != costs.size()) printf("Found duplicate resource requirement, costs.size() = %d sortedItems.size() = %d\n",costs.size(),sortedItems.size());

		if(hasDup) {
			printf("WARNING, upgrade type [%s] has one or more duplicate resource requirements\n",this->getName().c_str());
		}

		if(sortedItems.size() < costs.size()) {
			costs.resize(sortedItems.size());
		}

		index = 0;
		for(std::map<string,int>::iterator iterMap = sortedItems.begin();
				iterMap != sortedItems.end(); ++iterMap) {
			costs[index].init(techTree->getResourceType(iterMap->first), iterMap->second);
			index++;
		}
		sortedItems.clear();
		hasDup = false;

		//effects
		const XmlNode *effectsNode= upgradeNode->getChild("effects");
		for(int i = 0; i < effectsNode->getChildCount(); ++i) {
			const XmlNode *unitNode= effectsNode->getChild("unit", i);
			string name= unitNode->getAttribute("name")->getRestrictedValue();

			if(sortedItems.find(name) != sortedItems.end()) {
				hasDup = true;
			}

			sortedItems[name] = 0;
		}

		if(hasDup) {
			printf("WARNING, upgrade type [%s] has one or more duplicate effects\n",this->getName().c_str());
		}

		for(std::map<string,int>::iterator iterMap = sortedItems.begin();
				iterMap != sortedItems.end(); ++iterMap) {
			effects.push_back(factionType->getUnitType(iterMap->first));
		}
		sortedItems.clear();

		//values
		UpgradeTypeBase::load(upgradeNode,name);
	}
	catch(const exception &e){
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
		throw megaglest_runtime_error("Error loading UpgradeType: "+ dir + "\n" +e.what());
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

bool UpgradeType::isAffected(const UnitType *unitType) const{
	return find(effects.begin(), effects.end(), unitType)!=effects.end();
}

//void UpgradeType::saveGame(XmlNode *rootNode) const {
//	UpgradeTypeBase::saveGame(rootNode);
//	ProducibleType::saveGame(rootNode);
//
//	std::map<string,string> mapTagReplacements;
//	XmlNode *upgradeTypeNode = rootNode->addChild("UpgradeType");
//
//	//upgradeTypeNode->addAttribute("maxHp",intToStr(maxHp), mapTagReplacements);
//	//vector<const UnitType*> effects;
//	for(unsigned int i = 0; i < effects.size(); ++i) {
//		XmlNode *unitTypeNode = rootNode->addChild("UnitType");
//
//		const UnitType *ut = effects[i];
//		unitTypeNode->addAttribute("name",ut->getName(), mapTagReplacements);
//	}
//}
//
//void UpgradeType::loadGame(const XmlNode *rootNode, Faction *faction) {
//	//UpgradeTypeBase::loadGame(rootNode);
//	//ProducibleType::loadGame(rootNode);
//
//	//const XmlNode *upgradeTypeNode = rootNode->getChild("UpgradeType");
//
//	//maxHp = upgradeTypeNode->getAttribute("maxHp")->getIntValue();
//
////	vector<XmlNode *> unitTypeNodeList = upgradeTypeNode->getChildList("UnitType");
////	for(unsigned int i = 0; i < unitTypeNodeList.size(); ++i) {
////		XmlNode *node = unitTypeNodeList[i];
////	}
//}

// ===============================
// 	class TotalUpgrade
// ===============================

TotalUpgrade::TotalUpgrade() {
	reset();
}

void TotalUpgrade::reset() {
    maxHp= 0;
    maxHpIsMultiplier=false;
    maxHpRegeneration = 0;

    maxEp= 0;
    maxEpIsMultiplier = false;
    maxEpRegeneration = 0;

    sight=0;
    sightIsMultiplier=false;

	armor= 0;
	armorIsMultiplier=false;

    attackStrength= 0;
    attackStrengthIsMultiplier=false;

    attackRange= 0;
    attackRangeIsMultiplier=false;

    moveSpeed= 0;
    moveSpeedIsMultiplier=false;

    prodSpeed=0;
    prodSpeedIsMultiplier=false;
}

void TotalUpgrade::sum(const UpgradeTypeBase *ut, const Unit *unit) {
	maxHpIsMultiplier			= ut->getMaxHpIsMultiplier();
	sightIsMultiplier			= ut->getSightIsMultiplier();
	maxEpIsMultiplier			= ut->getMaxEpIsMultiplier();
	armorIsMultiplier			= ut->getArmorIsMultiplier();
	attackStrengthIsMultiplier	= ut->getAttackStrengthIsMultiplier();
	attackRangeIsMultiplier		= ut->getAttackRangeIsMultiplier();
	moveSpeedIsMultiplier		= ut->getMoveSpeedIsMultiplier();
	prodSpeedIsMultiplier		= ut->getProdSpeedIsMultiplier();

	if(ut->getMaxHpIsMultiplier() == true) {
		//printf("#1 Maxhp maxHp = %d, unit->getHp() = %d ut->getMaxHp() = %d\n",maxHp,unit->getHp(),ut->getMaxHp());
		maxHp += ((double)unit->getHp() * ((double)ut->getMaxHp() / (double)100));
		if(ut->getMaxHpRegeneration() != 0) {
			maxHpRegeneration += ((double)unit->getType()->getHpRegeneration() + ((double)max(maxHp,unit->getHp()) * ((double)ut->getMaxHpRegeneration() / (double)100)));
		}
		//printf("#1.1 Maxhp maxHp = %d, unit->getHp() = %d ut->getMaxHp() = %d\n",maxHp,unit->getHp(),ut->getMaxHp());
	}
	else {
		//printf("#2 Maxhp maxHp = %d, unit->getHp() = %d ut->getMaxHp() = %d\n",maxHp,unit->getHp(),ut->getMaxHp());
		maxHp += ut->getMaxHp();
		if(ut->getMaxHpRegeneration() != 0) {
			maxHpRegeneration += ut->getMaxHpRegeneration();
		}
	}

	if(ut->getMaxEpIsMultiplier() == true) {
		maxEp += ((double)unit->getEp() * ((double)ut->getMaxEp() / (double)100));
		if(ut->getMaxHpRegeneration() != 0) {
			maxEpRegeneration += ((double)unit->getType()->getEpRegeneration() + ((double)max(maxEp,unit->getEp()) * ((double)ut->getMaxEpRegeneration() / (double)100)));
		}
	}
	else {
		maxEp += ut->getMaxEp();
		if(ut->getMaxEpRegeneration() != 0) {
			maxEpRegeneration += ut->getMaxEpRegeneration();
		}
	}

	if(ut->getSightIsMultiplier() == true) {
		sight += ((double)unit->getType()->getSight() * ((double)ut->getSight() / (double)100));
	}
	else {
		sight += ut->getSight();
	}

	if(ut->getArmorIsMultiplier() == true) {
		armor += ((double)unit->getType()->getArmor() * ((double)ut->getArmor() / (double)100));
	}
	else {
		armor += ut->getArmor();
	}

	if(ut->getAttackStrengthIsMultiplier() == true) {
		for(unsigned int i = 0; i < unit->getType()->getSkillTypeCount(); ++i) {
			const SkillType *skillType = unit->getType()->getSkillType(i);
			const AttackSkillType *ast = dynamic_cast<const AttackSkillType *>(skillType);
			if(ast != NULL) {
				attackStrengthMultiplierValueList[ast->getName()] += ((double)ast->getAttackStrength() * ((double)ut->getAttackStrength(NULL) / (double)100));
			}
		}
	}
	else {
		attackStrength += ut->getAttackStrength(NULL);
	}

	if(ut->getAttackRangeIsMultiplier() == true) {
		for(unsigned int i = 0; i < unit->getType()->getSkillTypeCount(); ++i) {
			const SkillType *skillType = unit->getType()->getSkillType(i);
			const AttackSkillType *ast = dynamic_cast<const AttackSkillType *>(skillType);
			if(ast != NULL) {
				attackRangeMultiplierValueList[ast->getName()] += ((double)ast->getAttackRange() * ((double)ut->getAttackRange(NULL) / (double)100));
			}
		}
	}
	else {
		attackRange += ut->getAttackRange(NULL);
	}

	if(ut->getMoveSpeedIsMultiplier() == true) {
		//printf("BEFORE Applying moveSpeedIsMultiplier\n");

		for(unsigned int i = 0; i < unit->getType()->getSkillTypeCount(); ++i) {
			const SkillType *skillType = unit->getType()->getSkillType(i);
			const MoveSkillType *mst = dynamic_cast<const MoveSkillType *>(skillType);
			if(mst != NULL) {
				moveSpeedIsMultiplierValueList[mst->getName()] += ((double)mst->getSpeed() * ((double)ut->getMoveSpeed(NULL) / (double)100));

				//printf("Applying moveSpeedIsMultiplier for unit [%s - %d], mst->getSpeed() = %d ut->getMoveSpeed(NULL) = %d newmoveSpeed = %d for skill [%s]\n",unit->getType()->getName().c_str(),unit->getId(), mst->getSpeed(),ut->getMoveSpeed(NULL),moveSpeedIsMultiplierValueList[mst->getName()],mst->getName().c_str());
			}
		}

		//printf("AFTER Applying moveSpeedIsMultiplierd\n");
	}
	else {
		moveSpeed += ut->getMoveSpeed(NULL);
	}

	if(ut->getProdSpeedIsMultiplier() == true) {
		for(unsigned int i = 0; i < unit->getType()->getSkillTypeCount(); ++i) {
			const SkillType *skillType = unit->getType()->getSkillType(i);
			const ProduceSkillType *pst = dynamic_cast<const ProduceSkillType *>(skillType);
			if(pst != NULL) {
				prodSpeedProduceIsMultiplierValueList[pst->getName()] += ((double)pst->getSpeed() * ((double)ut->getProdSpeed(NULL) / (double)100));
			}
			const UpgradeSkillType *ust = dynamic_cast<const UpgradeSkillType *>(skillType);
			if(ust != NULL) {
				prodSpeedUpgradeIsMultiplierValueList[ust->getName()] += ((double)ust->getSpeed() * ((double)ut->getProdSpeed(NULL) / (double)100));
			}
			const MorphSkillType *mst = dynamic_cast<const MorphSkillType *>(skillType);
			if(mst != NULL) {
				prodSpeedMorphIsMultiplierValueList[mst->getName()] += ((double)mst->getSpeed() * ((double)ut->getProdSpeed(NULL) / (double)100));
			}
		}
	}
	else {
		prodSpeed += ut->getProdSpeed(NULL);
	}
}

void TotalUpgrade::apply(const UpgradeTypeBase *ut, const Unit *unit) {
	sum(ut, unit);
}

void TotalUpgrade::deapply(const UpgradeTypeBase *ut,const Unit *unit) {
	maxHpIsMultiplier			= ut->getMaxHpIsMultiplier();
	sightIsMultiplier			= ut->getSightIsMultiplier();
	maxEpIsMultiplier			= ut->getMaxEpIsMultiplier();
	armorIsMultiplier			= ut->getArmorIsMultiplier();
	attackStrengthIsMultiplier	= ut->getAttackStrengthIsMultiplier();
	attackRangeIsMultiplier		= ut->getAttackRangeIsMultiplier();
	moveSpeedIsMultiplier		= ut->getMoveSpeedIsMultiplier();
	prodSpeedIsMultiplier		= ut->getProdSpeedIsMultiplier();

	if(ut->getMaxHpIsMultiplier() == true) {
		maxHp -= ((double)unit->getHp() * ((double)ut->getMaxHp() / (double)100));
		if(ut->getMaxHpRegeneration() != 0) {
			maxHpRegeneration -= ((double)unit->getType()->getHpRegeneration() + ((double)max(maxHp,unit->getHp()) * ((double)ut->getMaxHpRegeneration() / (double)100)));
		}
	}
	else {
		maxHp -= ut->getMaxHp();
		if(ut->getMaxHpRegeneration() != 0) {
			maxHpRegeneration -= ut->getMaxHpRegeneration();
		}
	}

	if(ut->getMaxEpIsMultiplier() == true) {
		maxEp -= ((double)unit->getEp() * ((double)ut->getMaxEp() / (double)100));
		if(ut->getMaxEpRegeneration() != 0) {
			maxEpRegeneration += ((double)unit->getType()->getEpRegeneration() + ((double)max(maxEp,unit->getEp()) * ((double)ut->getMaxEpRegeneration() / (double)100)));
		}
	}
	else {
		maxEp -= ut->getMaxEp();
		if(ut->getMaxEpRegeneration() != 0) {
			maxEpRegeneration -= ut->getMaxEpRegeneration();
		}
	}

	if(ut->getSightIsMultiplier() == true) {
		sight -= ((double)unit->getType()->getSight() * ((double)ut->getSight() / (double)100));
	}
	else {
		sight -= ut->getSight();
	}

	if(ut->getArmorIsMultiplier() == true) {
		armor -= ((double)unit->getType()->getArmor() * ((double)ut->getArmor() / (double)100));
	}
	else {
		armor -= ut->getArmor();
	}

	if(ut->getAttackStrengthIsMultiplier() == true) {
		for(unsigned int i = 0; i < unit->getType()->getSkillTypeCount(); ++i) {
			const SkillType *skillType = unit->getType()->getSkillType(i);
			const AttackSkillType *ast = dynamic_cast<const AttackSkillType *>(skillType);
			if(ast != NULL) {
				attackStrengthMultiplierValueList[ast->getName()] -= ((double)ast->getAttackStrength() * ((double)ut->getAttackStrength(NULL) / (double)100));
			}
		}
	}
	else {
		attackStrength -= ut->getAttackStrength(NULL);
	}

	if(ut->getAttackRangeIsMultiplier() == true) {
		for(unsigned int i = 0; i < unit->getType()->getSkillTypeCount(); ++i) {
			const SkillType *skillType = unit->getType()->getSkillType(i);
			const AttackSkillType *ast = dynamic_cast<const AttackSkillType *>(skillType);
			if(ast != NULL) {
				attackRangeMultiplierValueList[ast->getName()] -= ((double)ast->getAttackRange() * ((double)ut->getAttackRange(NULL) / (double)100));
			}
		}
	}
	else {
		attackRange -= ut->getAttackRange(NULL);
	}

	if(ut->getMoveSpeedIsMultiplier() == true) {
		//printf("BEFORE Applying moveSpeedIsMultiplier, moveSpeed = %d, ut->getMoveSpeed() = %d\n",moveSpeed,ut->getMoveSpeed());

		for(unsigned int i = 0; i < unit->getType()->getSkillTypeCount(); ++i) {
			const SkillType *skillType = unit->getType()->getSkillType(i);
			const MoveSkillType *mst = dynamic_cast<const MoveSkillType *>(skillType);
			if(mst != NULL) {
				moveSpeedIsMultiplierValueList[mst->getName()] -= ((double)mst->getSpeed() * ((double)ut->getMoveSpeed(NULL) / (double)100));
			}
		}

		//printf("AFTER Applying moveSpeedIsMultiplier, moveSpeed = %d\n",moveSpeed);
	}
	else {
		moveSpeed -= ut->getMoveSpeed(NULL);
	}

	if(ut->getProdSpeedIsMultiplier() == true) {
		for(unsigned int i = 0; i < unit->getType()->getSkillTypeCount(); ++i) {
			const SkillType *skillType = unit->getType()->getSkillType(i);
			const ProduceSkillType *pst = dynamic_cast<const ProduceSkillType *>(skillType);
			if(pst != NULL) {
				prodSpeedProduceIsMultiplierValueList[pst->getName()] -= ((double)pst->getSpeed() * ((double)ut->getProdSpeed(NULL) / (double)100));
			}
			const UpgradeSkillType *ust = dynamic_cast<const UpgradeSkillType *>(skillType);
			if(ust != NULL) {
				prodSpeedUpgradeIsMultiplierValueList[ust->getName()] -= ((double)ust->getSpeed() * ((double)ut->getProdSpeed(NULL) / (double)100));
			}
			const MorphSkillType *mst = dynamic_cast<const MorphSkillType *>(skillType);
			if(mst != NULL) {
				prodSpeedMorphIsMultiplierValueList[mst->getName()] -= ((double)mst->getSpeed() * ((double)ut->getProdSpeed(NULL) / (double)100));
			}
		}
	}
	else {
		prodSpeed -= ut->getProdSpeed(NULL);
	}
}

void TotalUpgrade::incLevel(const UnitType *ut) {
	maxHp += ut->getMaxHp()*50/100;
	maxEp += ut->getMaxEp()*50/100;
	sight += ut->getSight()*20/100;
	armor += ut->getArmor()*50/100;
}

void TotalUpgrade::saveGame(XmlNode *rootNode) const {
	std::map<string,string> mapTagReplacements;
	XmlNode *upgradeTypeBaseNode = rootNode->addChild("TotalUpgrade");

//    int maxHp;
	upgradeTypeBaseNode->addAttribute("maxHp",intToStr(maxHp), mapTagReplacements);
//    bool maxHpIsMultiplier;
	upgradeTypeBaseNode->addAttribute("maxHpIsMultiplier",intToStr(maxHpIsMultiplier), mapTagReplacements);
//	int maxHpRegeneration;
	upgradeTypeBaseNode->addAttribute("maxHpRegeneration",intToStr(maxHpRegeneration), mapTagReplacements);
//	//bool maxHpRegenerationIsMultiplier;
//
//    int sight;
	upgradeTypeBaseNode->addAttribute("sight",intToStr(sight), mapTagReplacements);
//    bool sightIsMultiplier;
	upgradeTypeBaseNode->addAttribute("sightIsMultiplier",intToStr(sightIsMultiplier), mapTagReplacements);
//    int maxEp;
	upgradeTypeBaseNode->addAttribute("maxEp",intToStr(maxEp), mapTagReplacements);
//    bool maxEpIsMultiplier;
	upgradeTypeBaseNode->addAttribute("maxEpIsMultiplier",intToStr(maxEpIsMultiplier), mapTagReplacements);
//	int maxEpRegeneration;
	upgradeTypeBaseNode->addAttribute("maxEpRegeneration",intToStr(maxEpRegeneration), mapTagReplacements);
//	//bool maxEpRegenerationIsMultiplier;
//    int armor;
	upgradeTypeBaseNode->addAttribute("armor",intToStr(armor), mapTagReplacements);
//    bool armorIsMultiplier;
	upgradeTypeBaseNode->addAttribute("armorIsMultiplier",intToStr(armorIsMultiplier), mapTagReplacements);
//    int attackStrength;
	upgradeTypeBaseNode->addAttribute("attackStrength",intToStr(attackStrength), mapTagReplacements);
//    bool attackStrengthIsMultiplier;
	upgradeTypeBaseNode->addAttribute("attackStrengthIsMultiplier",intToStr(attackStrengthIsMultiplier), mapTagReplacements);
//    std::map<string,int> attackStrengthMultiplierValueList;
	for(std::map<string,int>::const_iterator iterMap = attackStrengthMultiplierValueList.begin();
			iterMap != attackStrengthMultiplierValueList.end(); ++iterMap) {
		XmlNode *attackStrengthMultiplierValueListNode = upgradeTypeBaseNode->addChild("attackStrengthMultiplierValueList");

		attackStrengthMultiplierValueListNode->addAttribute("key",iterMap->first, mapTagReplacements);
		attackStrengthMultiplierValueListNode->addAttribute("value",intToStr(iterMap->second), mapTagReplacements);
	}
//    int attackRange;
	upgradeTypeBaseNode->addAttribute("attackRange",intToStr(attackRange), mapTagReplacements);
//    bool attackRangeIsMultiplier;
	upgradeTypeBaseNode->addAttribute("attackRangeIsMultiplier",intToStr(attackRangeIsMultiplier), mapTagReplacements);
//    std::map<string,int> attackRangeMultiplierValueList;
	for(std::map<string,int>::const_iterator iterMap = attackRangeMultiplierValueList.begin();
			iterMap != attackRangeMultiplierValueList.end(); ++iterMap) {
		XmlNode *attackRangeMultiplierValueListNode = upgradeTypeBaseNode->addChild("attackRangeMultiplierValueList");

		attackRangeMultiplierValueListNode->addAttribute("key",iterMap->first, mapTagReplacements);
		attackRangeMultiplierValueListNode->addAttribute("value",intToStr(iterMap->second), mapTagReplacements);
	}

//    int moveSpeed;
	upgradeTypeBaseNode->addAttribute("moveSpeed",intToStr(moveSpeed), mapTagReplacements);
//    bool moveSpeedIsMultiplier;
	upgradeTypeBaseNode->addAttribute("moveSpeedIsMultiplier",intToStr(moveSpeedIsMultiplier), mapTagReplacements);
//    std::map<string,int> moveSpeedIsMultiplierValueList;
	for(std::map<string,int>::const_iterator iterMap = moveSpeedIsMultiplierValueList.begin();
			iterMap != moveSpeedIsMultiplierValueList.end(); ++iterMap) {
		XmlNode *moveSpeedIsMultiplierValueListNode = upgradeTypeBaseNode->addChild("moveSpeedIsMultiplierValueList");

		moveSpeedIsMultiplierValueListNode->addAttribute("key",iterMap->first, mapTagReplacements);
		moveSpeedIsMultiplierValueListNode->addAttribute("value",intToStr(iterMap->second), mapTagReplacements);
	}

//    int prodSpeed;
	upgradeTypeBaseNode->addAttribute("prodSpeed",intToStr(prodSpeed), mapTagReplacements);
//    bool prodSpeedIsMultiplier;
	upgradeTypeBaseNode->addAttribute("prodSpeedIsMultiplier",intToStr(prodSpeedIsMultiplier), mapTagReplacements);
//    std::map<string,int> prodSpeedProduceIsMultiplierValueList;
	for(std::map<string,int>::const_iterator iterMap = prodSpeedProduceIsMultiplierValueList.begin();
			iterMap != prodSpeedProduceIsMultiplierValueList.end(); ++iterMap) {
		XmlNode *prodSpeedProduceIsMultiplierValueListNode = upgradeTypeBaseNode->addChild("prodSpeedProduceIsMultiplierValueList");

		prodSpeedProduceIsMultiplierValueListNode->addAttribute("key",iterMap->first, mapTagReplacements);
		prodSpeedProduceIsMultiplierValueListNode->addAttribute("value",intToStr(iterMap->second), mapTagReplacements);
	}

//    std::map<string,int> prodSpeedUpgradeIsMultiplierValueList;
	for(std::map<string,int>::const_iterator iterMap = prodSpeedUpgradeIsMultiplierValueList.begin();
			iterMap != prodSpeedUpgradeIsMultiplierValueList.end(); ++iterMap) {
		XmlNode *prodSpeedUpgradeIsMultiplierValueListNode = upgradeTypeBaseNode->addChild("prodSpeedUpgradeIsMultiplierValueList");

		prodSpeedUpgradeIsMultiplierValueListNode->addAttribute("key",iterMap->first, mapTagReplacements);
		prodSpeedUpgradeIsMultiplierValueListNode->addAttribute("value",intToStr(iterMap->second), mapTagReplacements);
	}

//    std::map<string,int> prodSpeedMorphIsMultiplierValueList;
	for(std::map<string,int>::const_iterator iterMap = prodSpeedMorphIsMultiplierValueList.begin();
			iterMap != prodSpeedMorphIsMultiplierValueList.end(); ++iterMap) {
		XmlNode *prodSpeedMorphIsMultiplierValueListNode = upgradeTypeBaseNode->addChild("prodSpeedMorphIsMultiplierValueList");

		prodSpeedMorphIsMultiplierValueListNode->addAttribute("key",iterMap->first, mapTagReplacements);
		prodSpeedMorphIsMultiplierValueListNode->addAttribute("value",intToStr(iterMap->second), mapTagReplacements);
	}
}

void TotalUpgrade::loadGame(const XmlNode *rootNode) {
	const XmlNode *upgradeTypeBaseNode = rootNode->getChild("TotalUpgrade");

	//description = upgradeTypeBaseNode->getAttribute("description")->getValue();

	//    int maxHp;
	maxHp = upgradeTypeBaseNode->getAttribute("maxHp")->getIntValue();
	//    bool maxHpIsMultiplier;
	maxHpIsMultiplier = upgradeTypeBaseNode->getAttribute("maxHpIsMultiplier")->getIntValue() != 0;
	//	int maxHpRegeneration;
	maxHpRegeneration = upgradeTypeBaseNode->getAttribute("maxHpRegeneration")->getIntValue();
	//	//bool maxHpRegenerationIsMultiplier;
	//
	//    int sight;
	sight = upgradeTypeBaseNode->getAttribute("sight")->getIntValue();
	//    bool sightIsMultiplier;
	sightIsMultiplier = upgradeTypeBaseNode->getAttribute("sightIsMultiplier")->getIntValue() != 0;
	//    int maxEp;
	maxEp = upgradeTypeBaseNode->getAttribute("maxEp")->getIntValue();
	//    bool maxEpIsMultiplier;
	maxEpIsMultiplier = upgradeTypeBaseNode->getAttribute("maxEpIsMultiplier")->getIntValue() != 0;
	//	int maxEpRegeneration;
	maxEpRegeneration = upgradeTypeBaseNode->getAttribute("maxEpRegeneration")->getIntValue();
	//	//bool maxEpRegenerationIsMultiplier;
	//    int armor;
	armor = upgradeTypeBaseNode->getAttribute("armor")->getIntValue();
	//    bool armorIsMultiplier;
	armorIsMultiplier = upgradeTypeBaseNode->getAttribute("armorIsMultiplier")->getIntValue() != 0;
	//    int attackStrength;
	attackStrength = upgradeTypeBaseNode->getAttribute("attackStrength")->getIntValue();
	//    bool attackStrengthIsMultiplier;
	attackStrengthIsMultiplier = upgradeTypeBaseNode->getAttribute("attackStrengthIsMultiplier")->getIntValue() != 0;
	//    std::map<string,int> attackStrengthMultiplierValueList;
	vector<XmlNode *> attackStrengthMultiplierValueNodeList = upgradeTypeBaseNode->getChildList("attackStrengthMultiplierValueList");
	for(unsigned int i = 0; i < attackStrengthMultiplierValueNodeList.size(); ++i) {
		XmlNode *node = attackStrengthMultiplierValueNodeList[i];

		attackStrengthMultiplierValueList[node->getAttribute("key")->getValue()] =
		                                  node->getAttribute("value")->getIntValue();
	}
	//    int attackRange;
	attackRange = upgradeTypeBaseNode->getAttribute("attackRange")->getIntValue();
	//    bool attackRangeIsMultiplier;
	attackRangeIsMultiplier = upgradeTypeBaseNode->getAttribute("attackRangeIsMultiplier")->getIntValue() != 0;
	//    std::map<string,int> attackRangeMultiplierValueList;
	vector<XmlNode *> attackRangeMultiplierValueNodeList = upgradeTypeBaseNode->getChildList("attackRangeMultiplierValueList");
	for(unsigned int i = 0; i < attackRangeMultiplierValueNodeList.size(); ++i) {
		XmlNode *node = attackRangeMultiplierValueNodeList[i];

		attackRangeMultiplierValueList[node->getAttribute("key")->getValue()] =
		                                  node->getAttribute("value")->getIntValue();
	}

	//    int moveSpeed;
	moveSpeed = upgradeTypeBaseNode->getAttribute("moveSpeed")->getIntValue();
	//    bool moveSpeedIsMultiplier;
	moveSpeedIsMultiplier = upgradeTypeBaseNode->getAttribute("moveSpeedIsMultiplier")->getIntValue() != 0;
	//    std::map<string,int> moveSpeedIsMultiplierValueList;
	vector<XmlNode *> moveSpeedIsMultiplierValueNodeList = upgradeTypeBaseNode->getChildList("moveSpeedIsMultiplierValueList");
	for(unsigned int i = 0; i < moveSpeedIsMultiplierValueNodeList.size(); ++i) {
		XmlNode *node = moveSpeedIsMultiplierValueNodeList[i];

		moveSpeedIsMultiplierValueList[node->getAttribute("key")->getValue()] =
		                                  node->getAttribute("value")->getIntValue();
	}

	//    int prodSpeed;
	prodSpeed = upgradeTypeBaseNode->getAttribute("prodSpeed")->getIntValue();
	//    bool prodSpeedIsMultiplier;
	prodSpeedIsMultiplier = upgradeTypeBaseNode->getAttribute("prodSpeedIsMultiplier")->getIntValue() != 0;
	//    std::map<string,int> prodSpeedProduceIsMultiplierValueList;
	vector<XmlNode *> prodSpeedProduceIsMultiplierValueNodeList = upgradeTypeBaseNode->getChildList("prodSpeedProduceIsMultiplierValueList");
	for(unsigned int i = 0; i < prodSpeedProduceIsMultiplierValueNodeList.size(); ++i) {
		XmlNode *node = prodSpeedProduceIsMultiplierValueNodeList[i];

		prodSpeedProduceIsMultiplierValueList[node->getAttribute("key")->getValue()] =
		                                  node->getAttribute("value")->getIntValue();
	}

	//    std::map<string,int> prodSpeedUpgradeIsMultiplierValueList;
	vector<XmlNode *> prodSpeedUpgradeIsMultiplierValueNodeList = upgradeTypeBaseNode->getChildList("prodSpeedUpgradeIsMultiplierValueList");
	for(unsigned int i = 0; i < prodSpeedUpgradeIsMultiplierValueNodeList.size(); ++i) {
		XmlNode *node = prodSpeedUpgradeIsMultiplierValueNodeList[i];

		prodSpeedUpgradeIsMultiplierValueList[node->getAttribute("key")->getValue()] =
		                                  node->getAttribute("value")->getIntValue();
	}

	//    std::map<string,int> prodSpeedMorphIsMultiplierValueList;
	vector<XmlNode *> prodSpeedMorphIsMultiplierValueNodeList = upgradeTypeBaseNode->getChildList("prodSpeedMorphIsMultiplierValueList");
	for(unsigned int i = 0; i < prodSpeedMorphIsMultiplierValueNodeList.size(); ++i) {
		XmlNode *node = prodSpeedMorphIsMultiplierValueNodeList[i];

		prodSpeedMorphIsMultiplierValueList[node->getAttribute("key")->getValue()] =
		                                  node->getAttribute("value")->getIntValue();
	}
}


}}//end namespace
