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
#include <iterator>

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

void UpgradeTypeBase::copyDataFrom(UpgradeTypeBase *source) {
	upgradename = source->upgradename;
	maxHp = source->maxHp;
	maxHpIsMultiplier = source->maxHpIsMultiplier;
	maxHpRegeneration = source->maxHpRegeneration;
	sight = source->sight;
	sightIsMultiplier = source->sightIsMultiplier;
	maxEp = source->maxEp;
	maxEpIsMultiplier = source->maxEpIsMultiplier;
	maxEpRegeneration = source->maxEpRegeneration;
	armor = source->armor;
	armorIsMultiplier = source->armorIsMultiplier;
	attackStrength = source->attackStrength;
	attackStrengthIsMultiplier = source->attackStrengthIsMultiplier;
	attackStrengthMultiplierValueList = source->attackStrengthMultiplierValueList;
	attackRange = source->attackRange;
	attackRangeIsMultiplier = source->attackRangeIsMultiplier;
	attackRangeMultiplierValueList = source->attackRangeMultiplierValueList;
	moveSpeed = source->moveSpeed;
	moveSpeedIsMultiplier = source->moveSpeedIsMultiplier;
	moveSpeedIsMultiplierValueList = source->moveSpeedIsMultiplierValueList;
	prodSpeed = source->prodSpeed;
	prodSpeedIsMultiplier = source->prodSpeedIsMultiplier;
	prodSpeedProduceIsMultiplierValueList = source->prodSpeedProduceIsMultiplierValueList;
	prodSpeedUpgradeIsMultiplierValueList = source->prodSpeedUpgradeIsMultiplierValueList;
	prodSpeedMorphIsMultiplierValueList = source->prodSpeedMorphIsMultiplierValueList;
	attackSpeed = source->attackSpeed;
	attackSpeedIsMultiplier = source->attackSpeedIsMultiplier;
	attackSpeedIsMultiplierValueList = source->attackSpeedIsMultiplierValueList;
}

void UpgradeTypeBase::load(const XmlNode *upgradeNode, string upgradename) {
	this->upgradename = upgradename;
	//values
	maxHpIsMultiplier = false;
	if(upgradeNode->hasChild("max-hp") == true) {
		maxHp = upgradeNode->getChild("max-hp")->getAttribute("value")->getIntValue();
		if(upgradeNode->getChild("max-hp")->getAttribute(VALUE_PERCENT_MULTIPLIER_KEY_NAME,false) != NULL) {
			maxHpIsMultiplier = upgradeNode->getChild("max-hp")->getAttribute(VALUE_PERCENT_MULTIPLIER_KEY_NAME)->getBoolValue();

			//printf("Found maxHpIsMultiplier = %d\n",maxHpIsMultiplier);
		}
	}
	else {
		maxHp = 0;
	}
	maxHpRegeneration = 0;
	//maxHpRegenerationIsMultiplier = false;
	if(upgradeNode->hasChild("max-hp") == true) {
		if(upgradeNode->getChild("max-hp")->getAttribute(VALUE_REGEN_KEY_NAME,false) != NULL) {
			maxHpRegeneration = upgradeNode->getChild("max-hp")->getAttribute(VALUE_REGEN_KEY_NAME)->getIntValue();

			//printf("Found maxHpIsMultiplier = %d\n",maxHpIsMultiplier);
		}
	}

	maxEpIsMultiplier = false;
	if(upgradeNode->hasChild("max-ep") == true) {
		maxEp = upgradeNode->getChild("max-ep")->getAttribute("value")->getIntValue();
		if(upgradeNode->getChild("max-ep")->getAttribute(VALUE_PERCENT_MULTIPLIER_KEY_NAME,false) != NULL) {
			maxEpIsMultiplier = upgradeNode->getChild("max-ep")->getAttribute(VALUE_PERCENT_MULTIPLIER_KEY_NAME)->getBoolValue();

			//printf("Found maxEpIsMultiplier = %d\n",maxEpIsMultiplier);
		}
	}
	else {
		maxEp = 0;
	}
	maxEpRegeneration = 0;
	//maxEpRegenerationIsMultiplier = false;
	if(upgradeNode->hasChild("max-ep") == true) {
		if(upgradeNode->getChild("max-ep")->getAttribute(VALUE_REGEN_KEY_NAME,false) != NULL) {
			maxEpRegeneration = upgradeNode->getChild("max-ep")->getAttribute(VALUE_REGEN_KEY_NAME)->getIntValue();

			//printf("Found maxHpIsMultiplier = %d\n",maxHpIsMultiplier);
		}
	}

	sightIsMultiplier = false;
	if(upgradeNode->hasChild("sight") == true) {
		sight= upgradeNode->getChild("sight")->getAttribute("value")->getIntValue();
		if(upgradeNode->getChild("sight")->getAttribute(VALUE_PERCENT_MULTIPLIER_KEY_NAME,false) != NULL) {
			sightIsMultiplier = upgradeNode->getChild("sight")->getAttribute(VALUE_PERCENT_MULTIPLIER_KEY_NAME)->getBoolValue();

			//printf("Found sightIsMultiplier = %d\n",sightIsMultiplier);
		}
	}
	else {
		sight = 0;
	}

	attackStrengthIsMultiplier = false;

	std::vector<string> attackStrengthXMLTags;
	attackStrengthXMLTags.push_back("attack-strenght");
	attackStrengthXMLTags.push_back("attack-strength");
	if(upgradeNode->hasChildWithAliases(attackStrengthXMLTags) == true) {
		attackStrength= upgradeNode->getChildWithAliases(attackStrengthXMLTags)->getAttribute("value")->getIntValue();
		if(upgradeNode->getChildWithAliases(attackStrengthXMLTags)->getAttribute(VALUE_PERCENT_MULTIPLIER_KEY_NAME,false) != NULL) {
			attackStrengthIsMultiplier = upgradeNode->getChildWithAliases(attackStrengthXMLTags)->getAttribute(VALUE_PERCENT_MULTIPLIER_KEY_NAME)->getBoolValue();

			//printf("Found attackStrengthIsMultiplier = %d\n",attackStrengthIsMultiplier);
		}
	}
	else {
		attackStrength = 0;
	}

	attackRangeIsMultiplier = false;
	if(upgradeNode->hasChild("attack-range") == true) {
		attackRange= upgradeNode->getChild("attack-range")->getAttribute("value")->getIntValue();
		if(upgradeNode->getChild("attack-range")->getAttribute(VALUE_PERCENT_MULTIPLIER_KEY_NAME,false) != NULL) {
			attackRangeIsMultiplier = upgradeNode->getChild("attack-range")->getAttribute(VALUE_PERCENT_MULTIPLIER_KEY_NAME)->getBoolValue();

			//printf("Found attackRangeIsMultiplier = %d\n",attackRangeIsMultiplier);
		}
	}
	else {
		attackRange = 0;
	}

	armorIsMultiplier = false;
	if(upgradeNode->hasChild("armor") == true) {
		armor= upgradeNode->getChild("armor")->getAttribute("value")->getIntValue();
		if(upgradeNode->getChild("armor")->getAttribute(VALUE_PERCENT_MULTIPLIER_KEY_NAME,false) != NULL) {
			armorIsMultiplier = upgradeNode->getChild("armor")->getAttribute(VALUE_PERCENT_MULTIPLIER_KEY_NAME)->getBoolValue();

			//printf("Found armorIsMultiplier = %d\n",armorIsMultiplier);
		}
	}
	else {
		armor = 0;
	}

	moveSpeedIsMultiplier = false;
	if(upgradeNode->hasChild("move-speed") == true) {
		moveSpeed= upgradeNode->getChild("move-speed")->getAttribute("value")->getIntValue();
		if(upgradeNode->getChild("move-speed")->getAttribute(VALUE_PERCENT_MULTIPLIER_KEY_NAME,false) != NULL) {
			moveSpeedIsMultiplier = upgradeNode->getChild("move-speed")->getAttribute(VALUE_PERCENT_MULTIPLIER_KEY_NAME)->getBoolValue();

			//printf("Found moveSpeedIsMultiplier = %d\n",moveSpeedIsMultiplier);
		}
	}
	else {
		moveSpeed= 0;
	}

	prodSpeedIsMultiplier = false;
	if(upgradeNode->hasChild("production-speed") == true) {
		prodSpeed= upgradeNode->getChild("production-speed")->getAttribute("value")->getIntValue();
		if(upgradeNode->getChild("production-speed")->getAttribute(VALUE_PERCENT_MULTIPLIER_KEY_NAME,false) != NULL) {
			prodSpeedIsMultiplier = upgradeNode->getChild("production-speed")->getAttribute(VALUE_PERCENT_MULTIPLIER_KEY_NAME)->getBoolValue();

			//printf("Found prodSpeedIsMultiplier = %d\n",prodSpeedIsMultiplier);
		}
	}
	else {
		prodSpeed = 0;
	}

	attackSpeedIsMultiplier = false;
	if(upgradeNode->hasChild("attack-speed") == true) {
		attackSpeed= upgradeNode->getChild("attack-speed")->getAttribute("value")->getIntValue();
		if(upgradeNode->getChild("attack-speed")->getAttribute(VALUE_PERCENT_MULTIPLIER_KEY_NAME,false) != NULL) {
			attackSpeedIsMultiplier = upgradeNode->getChild("attack-speed")->getAttribute(VALUE_PERCENT_MULTIPLIER_KEY_NAME)->getBoolValue();

			//printf("Found prodSpeedIsMultiplier = %d\n",prodSpeedIsMultiplier);
		}
	}
	else {
		attackSpeed = 0;
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

int UpgradeTypeBase::getAttackSpeed(const AttackSkillType *st) const {
	if(attackSpeedIsMultiplier == false || st == NULL) {
		return attackSpeed;
	}
	else {
		int result = 0;
		if(attackSpeedIsMultiplierValueList.find(st->getName()) != attackSpeedIsMultiplierValueList.end()) {
			result = attackSpeedIsMultiplierValueList.find(st->getName())->second;
		}

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

string UpgradeTypeBase::getDesc(bool translatedValue) const{

    string str="";
    string indent="->";
	Lang &lang= Lang::getInstance();

	if(getMaxHp() != 0 || getMaxHpRegeneration() != 0) {
		str += indent+lang.getString("Hp",(translatedValue == true ? "" : "english")) + " +" + intToStr(maxHp);
		if(maxHpIsMultiplier) {
			str += "%";
		}
//		if(getMaxHpFromBoosts() != 0) {
//			str += " +" + intToStr(getMaxHpFromBoosts());
//		}
		if(getMaxHpRegeneration() != 0) {
			str += " (" + lang.getString("Regeneration",(translatedValue == true ? "" : "english")) + ": +" + intToStr(maxHpRegeneration);
//			if(getMaxHpRegenerationFromBoosts() != 0) {
//				str += " +" + intToStr(getMaxHpRegenerationFromBoosts());
//			}
			str += ")";
		}
	}

	if(getSight() != 0) {
		if(str != "") {
			str += "\n";
		}
		str += indent+lang.getString("Sight",(translatedValue == true ? "" : "english")) + " +" + intToStr(sight);
		if(sightIsMultiplier) {
			str += "%";
		}
//		if(getSightFromBoosts() != 0) {
//			str += " +" + intToStr(getSightFromBoosts());
//		}
	}

	if(getMaxEp() != 0 || getMaxEpRegeneration() != 0) {
		if(str != "") {
			str += "\n";
		}

		str += indent+lang.getString("Ep",(translatedValue == true ? "" : "english")) + " +" + intToStr(maxEp);
		if(maxEpIsMultiplier) {
			str += "%";
		}
//		if(getMaxEpFromBoosts() != 0) {
//			str += " +" + intToStr(getMaxEpFromBoosts());
//		}

		if(getMaxEpRegeneration() != 0) {
			str += " (" + lang.getString("Regeneration",(translatedValue == true ? "" : "english")) + ": +" + intToStr(maxEpRegeneration);
//			if(getMaxEpRegenerationFromBoosts() != 0) {
//				str += " +" + intToStr(getMaxEpRegenerationFromBoosts());
//			}
			str += ")";
		}
	}

	if(getAttackStrength() != 0) {
		if(str != "") {
			str += "\n";
		}

		str += indent+lang.getString("AttackStrenght",(translatedValue == true ? "" : "english")) + " +" + intToStr(attackStrength);
		if(attackStrengthIsMultiplier) {
			str += "%";
		}
//		if(getAttackStrengthFromBoosts(NULL) != 0) {
//			str += " +" + intToStr(getAttackStrengthFromBoosts(NULL));
//		}
	}

	if(getAttackRange() != 0) {
		if(str != "") {
			str += "\n";
		}

		str += indent+lang.getString("AttackDistance",(translatedValue == true ? "" : "english")) + " +" + intToStr(attackRange);
		if(attackRangeIsMultiplier) {
			str += "%";
		}
//		if(getAttackRangeFromBoosts(NULL) != 0) {
//			str += " +" + intToStr(getAttackRangeFromBoosts(NULL));
//		}
	}

	if(getArmor() != 0) {
		if(str != "") {
			str += "\n";
		}

		str += indent+lang.getString("Armor",(translatedValue == true ? "" : "english")) + " +" + intToStr(armor);
		if(armorIsMultiplier) {
			str += "%";
		}
//		if(getArmorFromBoosts() != 0) {
//			str += " +" + intToStr(getArmorFromBoosts());
//		}
	}

	if(getMoveSpeed() != 0) {
		if(str != "") {
			str += "\n";
		}

		str += indent+lang.getString("WalkSpeed",(translatedValue == true ? "" : "english")) + " +" + intToStr(moveSpeed);
		if(moveSpeedIsMultiplier) {
			str += "%";
		}
//		if(getMoveSpeedFromBoosts(NULL) != 0) {
//			str += " +" + intToStr(getMoveSpeedFromBoosts(NULL));
//		}
	}

	if(getProdSpeed() != 0) {
		if(str != "") {
			str += "\n";
		}

		str += indent+lang.getString("ProductionSpeed",(translatedValue == true ? "" : "english")) + " +" + intToStr(prodSpeed);
		if(prodSpeedIsMultiplier) {
			str += "%";
		}
//		if(getProdSpeedFromBoosts(NULL) != 0) {
//			str += " +" + intToStr(getProdSpeedFromBoosts(NULL));
//		}
	}

	if(getAttackSpeed() != 0) {
		if(str != "") {
			str += "\n";
		}

		str += indent+lang.getString("AttackSpeed",(translatedValue == true ? "" : "english")) + " +" + intToStr(attackSpeed);
		if(attackSpeedIsMultiplier) {
			str += "%";
		}
//		if(getAttackSpeedFromBoosts(NULL) != 0) {
//			str += " +" + intToStr(getAttackSpeedFromBoosts(NULL));
//		}
	}
	if(str != "") {
		str += "\n";
	}

    return str;
}

void UpgradeTypeBase::saveGameBoost(XmlNode *rootNode) const {
	std::map<string,string> mapTagReplacements;
	XmlNode *upgradeTypeBaseNode = rootNode->addChild("UpgradeTypeBaseBoost");

	upgradeTypeBaseNode->addAttribute("upgradename",upgradename, mapTagReplacements);

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

void UpgradeTypeBase::loadGameBoost(const XmlNode *rootNode) {
	const XmlNode *upgradeTypeBaseNode = rootNode->getChild("UpgradeTypeBaseBoost");

	//description = upgradeTypeBaseNode->getAttribute("description")->getValue();

	upgradename = upgradeTypeBaseNode->getAttribute("upgradename")->getValue();

	//    int maxHp;
	maxHp = upgradeTypeBaseNode->getAttribute("maxHp")->getIntValue();
	//    bool maxHpIsMultiplier;
	maxHpIsMultiplier = (upgradeTypeBaseNode->getAttribute("maxHpIsMultiplier")->getIntValue() != 0);
	//	int maxHpRegeneration;
	maxHpRegeneration = upgradeTypeBaseNode->getAttribute("maxHpRegeneration")->getIntValue();
	//	//bool maxHpRegenerationIsMultiplier;
	//
	//    int sight;
	sight = upgradeTypeBaseNode->getAttribute("sight")->getIntValue();
	//    bool sightIsMultiplier;
	sightIsMultiplier = (upgradeTypeBaseNode->getAttribute("sightIsMultiplier")->getIntValue() != 0);
	//    int maxEp;
	maxEp = upgradeTypeBaseNode->getAttribute("maxEp")->getIntValue();
	//    bool maxEpIsMultiplier;
	maxEpIsMultiplier = (upgradeTypeBaseNode->getAttribute("maxEpIsMultiplier")->getIntValue() != 0);
	//	int maxEpRegeneration;
	maxEpRegeneration = upgradeTypeBaseNode->getAttribute("maxEpRegeneration")->getIntValue();
	//	//bool maxEpRegenerationIsMultiplier;
	//    int armor;
	armor = upgradeTypeBaseNode->getAttribute("armor")->getIntValue();
	//    bool armorIsMultiplier;
	armorIsMultiplier = (upgradeTypeBaseNode->getAttribute("armorIsMultiplier")->getIntValue() != 0);
	//    int attackStrength;
	attackStrength = upgradeTypeBaseNode->getAttribute("attackStrength")->getIntValue();
	//    bool attackStrengthIsMultiplier;
	attackStrengthIsMultiplier = (upgradeTypeBaseNode->getAttribute("attackStrengthIsMultiplier")->getIntValue() != 0);
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
	attackRangeIsMultiplier = (upgradeTypeBaseNode->getAttribute("attackRangeIsMultiplier")->getIntValue() != 0);
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
	moveSpeedIsMultiplier = (upgradeTypeBaseNode->getAttribute("moveSpeedIsMultiplier")->getIntValue() != 0);
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
	prodSpeedIsMultiplier = (upgradeTypeBaseNode->getAttribute("prodSpeedIsMultiplier")->getIntValue() != 0);
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

string UpgradeType::getTagName(string tag, bool translatedValue) const {
	if(translatedValue == false) return tag;

	Lang &lang = Lang::getInstance();
	return lang.getTechTreeString("TagName_" + tag, tag.c_str());
}

string UpgradeType::getReqDesc(bool translatedValue) const{
	Lang &lang= Lang::getInstance();
    string str= ProducibleType::getReqDesc(translatedValue);
    string indent="  ";
	if(!effects.empty() || !tags.empty()){
		str+= "\n"+ lang.getString("Upgrades",(translatedValue == true ? "" : "english"))+"\n";
	}
	str+=UpgradeTypeBase::getDesc(translatedValue);
	if(!effects.empty() || !tags.empty()){
		str+= lang.getString("AffectedUnits",(translatedValue == true ? "" : "english"))+"\n";

		// We want the output to be sorted, so convert the set to a vector and sort that
		std::vector<const UnitType*> outputUnits(effects.begin(), effects.end());
		std::sort(outputUnits.begin(), outputUnits.end(), UnitTypeSorter());

		vector<const UnitType*>::iterator unitIter;
		for (unitIter = outputUnits.begin(); unitIter != outputUnits.end(); ++unitIter) {
			const UnitType *unit = *unitIter;
			str+= indent+unit->getName(translatedValue)+"\n";
		}

		// Do the same for tags
		std::vector<string> outputTags(tags.begin(), tags.end());
		std::sort(outputTags.begin(), outputTags.end());

		vector<string>::iterator tagIter;
		for (tagIter = outputTags.begin(); tagIter != outputTags.end(); ++tagIter) {
			string tag = *tagIter;
			str+= indent + lang.getString("TagDesc", (translatedValue == true ? "" : "english")) +
					" " + getTagName(tag,translatedValue)  + "\n";
		}
	}
	return str;
}

void UpgradeType::preLoad(const string &dir){
	name=lastDir(dir);
}

void UpgradeType::load(const string &dir, const TechTree *techTree,
		const FactionType *factionType, Checksum* checksum,
		Checksum* techtreeChecksum, std::map<string,
		vector<pair<string, string> > > &loadedFileList,
		bool validationMode) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	char szBuf[8096]="";
	snprintf(szBuf,8096,Lang::getInstance().getString("LogScreenGameLoadingUpgradeType","",true).c_str(),formatString(this->getName(true)).c_str());
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
		image = NULL; // Not used for upgrade types

		//image cancel
		cancelImage = NULL; // Not used for upgrade types

		//upgrade time
		const XmlNode *upgradeTimeNode= upgradeNode->getChild("time");
		productionTime= upgradeTimeNode->getAttribute("value")->getIntValue();

		std::map<string,int> sortedItems;

		//unit requirements
		bool hasDup = false;
		const XmlNode *unitRequirementsNode= upgradeNode->getChild("unit-requirements");
		for(int i = 0; i < (int)unitRequirementsNode->getChildCount(); ++i) {
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
		for(int i = 0; i < (int)upgradeRequirementsNode->getChildCount(); ++i) {
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

		costs.resize(resourceRequirementsNode->getChildCount());
		for(int i = 0; i < (int)costs.size(); ++i) {
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
			try {
				costs[index].init(techTree->getResourceType(iterMap->first), iterMap->second);
				index++;
			}
			catch(megaglest_runtime_error& ex) {
				if(validationMode == false) {
					throw;
				}
				else {
					SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\nFor UpgradeType: %s Cost: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what(),name.c_str(),iterMap->second);
				}
			}

		}
		sortedItems.clear();
		//hasDup = false;

		//effects -- get list of affected units
		const XmlNode *effectsNode= upgradeNode->getChild("effects");
		vector<XmlNode*> unitNodes= effectsNode->getChildList("unit");
		for(size_t i = 0; i < unitNodes.size(); ++i) {
			const XmlNode *unitNode= unitNodes.at(i);
			string name= unitNode->getAttribute("name")->getRestrictedValue();

			effects.insert(factionType->getUnitType(name));
		}

		//effects -- convert tags into units
		vector<XmlNode*> tagNodes= effectsNode->getChildList("tag");
		for(size_t i = 0; i < tagNodes.size(); ++i) {
			const XmlNode *tagNode= tagNodes.at(i);
			string name= tagNode->getAttribute("name")->getRestrictedValue();
			tags.insert(name);
		}

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
	if(std::find(effects.begin(), effects.end(), unitType)!=effects.end()) return true;

	const set<string> unitTags = unitType->getTags();
	set<string> intersect;
	set_intersection(tags.begin(),tags.end(),unitTags.begin(),unitTags.end(),
			std::inserter(intersect,intersect.begin()));
	if(!intersect.empty()) return true;

	return false;
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

	attackSpeed=0;
	attackSpeedIsMultiplier=false;

	boostUpgradeBase = NULL;
	boostUpgradeSourceUnit = -1;
	boostUpgradeDestUnit = -1;
}

void TotalUpgrade::sum(const UpgradeTypeBase *ut, const Unit *unit, bool boostMode) {
	maxHpIsMultiplier			= ut->getMaxHpIsMultiplier();
	sightIsMultiplier			= ut->getSightIsMultiplier();
	maxEpIsMultiplier			= ut->getMaxEpIsMultiplier();
	armorIsMultiplier			= ut->getArmorIsMultiplier();
	attackStrengthIsMultiplier	= ut->getAttackStrengthIsMultiplier();
	attackRangeIsMultiplier		= ut->getAttackRangeIsMultiplier();
	moveSpeedIsMultiplier		= ut->getMoveSpeedIsMultiplier();
	prodSpeedIsMultiplier		= ut->getProdSpeedIsMultiplier();
	attackSpeedIsMultiplier		= ut->getAttackSpeedIsMultiplier();

	if(ut->getMaxHpIsMultiplier() == true) {
		//printf("#1 Maxhp maxHp = %d, unit->getHp() = %d ut->getMaxHp() = %d\n",maxHp,unit->getHp(),ut->getMaxHp());
		int newValue = ((double)unit->getHp() * ((double)ut->getMaxHp() / (double)100));
		if(boostMode) {
			maxHp = newValue;
		}
		else {
			maxHp += newValue;
		}
		if(ut->getMaxHpRegeneration() != 0) {
			newValue = ((double)unit->getType()->getHpRegeneration() + ((double)max(maxHp,unit->getHp()) * ((double)ut->getMaxHpRegeneration() / (double)100)));
			if(boostMode) {
				maxHpRegeneration = newValue;
			}
			else {
				maxHpRegeneration += newValue;
			}
		}
		//printf("#1.1 Maxhp maxHp = %d, unit->getHp() = %d ut->getMaxHp() = %d\n",maxHp,unit->getHp(),ut->getMaxHp());
	}
	else {
		//printf("#2 Maxhp maxHp = %d, unit->getHp() = %d ut->getMaxHp() = %d\n",maxHp,unit->getHp(),ut->getMaxHp());
		int newValue = ut->getMaxHp();
		if(boostMode) {
			maxHp = newValue;
		}
		else {
			maxHp += newValue;
		}

		if(ut->getMaxHpRegeneration() != 0) {
			newValue = ut->getMaxHpRegeneration();
			if(boostMode) {
				maxHpRegeneration = newValue;
			}
			else {
				maxHpRegeneration += newValue;
			}
		}
	}

	if(ut->getMaxEpIsMultiplier() == true) {
		int newValue = ((double)unit->getEp() * ((double)ut->getMaxEp() / (double)100));
		if(boostMode) {
			maxEp = newValue;
		}
		else {
			maxEp += newValue;
		}

		if(ut->getMaxEpRegeneration() != 0) {
			newValue = ((double)unit->getType()->getEpRegeneration() + ((double)max(maxEp,unit->getEp()) * ((double)ut->getMaxEpRegeneration() / (double)100)));
			if(boostMode) {
				maxEpRegeneration = newValue;
			}
			else {
				maxEpRegeneration += newValue;
			}
		}
	}
	else {
		int newValue = ut->getMaxEp();
		if(boostMode) {
			maxEp = newValue;
		}
		else {
			maxEp += newValue;
		}

		if(ut->getMaxEpRegeneration() != 0) {
			newValue = ut->getMaxEpRegeneration();
			if(boostMode) {
				maxEpRegeneration = newValue;
			}
			else {
				maxEpRegeneration += newValue;
			}
		}
	}

	if(ut->getSightIsMultiplier() == true) {
		int newValue = ((double)unit->getType()->getSight() * ((double)ut->getSight() / (double)100));
		if(boostMode) {
			sight = newValue;
		}
		else {
			sight += newValue;
		}
	}
	else {
		int newValue = ut->getSight();
		if(boostMode) {
			sight = newValue;
		}
		else {
			sight += newValue;
		}
	}

	if(ut->getArmorIsMultiplier() == true) {
		int newValue = ((double)unit->getType()->getArmor() * ((double)ut->getArmor() / (double)100));
		if(boostMode) {
			armor = newValue;
		}
		else {
			armor += newValue;
		}
	}
	else {
		int newValue = ut->getArmor();
		if(boostMode) {
			armor = newValue;
		}
		else {
			armor += newValue;
		}
	}

	if(ut->getAttackStrengthIsMultiplier() == true) {
		for(unsigned int i = 0; i < (unsigned int)unit->getType()->getSkillTypeCount(); ++i) {
			const SkillType *skillType = unit->getType()->getSkillType(i);
			const AttackSkillType *ast = dynamic_cast<const AttackSkillType *>(skillType);
			if(ast != NULL) {
				int newValue = ((double)ast->getAttackStrength() * ((double)ut->getAttackStrength(NULL) / (double)100));
				if(boostMode) {
					attackStrengthMultiplierValueList[ast->getName()] = newValue;
				}
				else {
					attackStrengthMultiplierValueList[ast->getName()] += newValue;
				}
			}
		}
	}
	else {
		int newValue = ut->getAttackStrength(NULL);
		if(boostMode) {
			attackStrength = newValue;
		}
		else {
			attackStrength += newValue;
		}
	}

	if(ut->getAttackRangeIsMultiplier() == true) {
		for(unsigned int i = 0; i < (unsigned int)unit->getType()->getSkillTypeCount(); ++i) {
			const SkillType *skillType = unit->getType()->getSkillType(i);
			const AttackSkillType *ast = dynamic_cast<const AttackSkillType *>(skillType);
			if(ast != NULL) {
				int newValue = ((double)ast->getAttackRange() * ((double)ut->getAttackRange(NULL) / (double)100));
				if(boostMode) {
					attackRangeMultiplierValueList[ast->getName()] = newValue;
				}
				else {
					attackRangeMultiplierValueList[ast->getName()] += newValue;
				}
			}
		}
	}
	else {
		int newValue = ut->getAttackRange(NULL);
		if(boostMode) {
			attackRange = newValue;
		}
		else {
			attackRange += newValue;
		}
	}

	if(ut->getMoveSpeedIsMultiplier() == true) {
		for(unsigned int i = 0; i < (unsigned int)unit->getType()->getSkillTypeCount(); ++i) {
			const SkillType *skillType = unit->getType()->getSkillType(i);
			const MoveSkillType *mst = dynamic_cast<const MoveSkillType *>(skillType);
			if(mst != NULL) {
				int newValue = ((double)mst->getSpeed() * ((double)ut->getMoveSpeed(NULL) / (double)100));
				if(boostMode) {
					moveSpeedIsMultiplierValueList[mst->getName()] = newValue;
				}
				else {
					moveSpeedIsMultiplierValueList[mst->getName()] += newValue;
				}
			}
		}
	}
	else {
		int newValue = ut->getMoveSpeed(NULL);
		if(boostMode) {
			moveSpeed = newValue;
		}
		else {
			moveSpeed += newValue;
		}
	}

	if(ut->getProdSpeedIsMultiplier() == true) {
		for(unsigned int i = 0; i < (unsigned int)unit->getType()->getSkillTypeCount(); ++i) {
			const SkillType *skillType = unit->getType()->getSkillType(i);
			const ProduceSkillType *pst = dynamic_cast<const ProduceSkillType *>(skillType);
			if(pst != NULL) {
				int newValue = ((double)pst->getSpeed() * ((double)ut->getProdSpeed(NULL) / (double)100));
				if(boostMode) {
					prodSpeedProduceIsMultiplierValueList[pst->getName()] = newValue;
				}
				else {
					prodSpeedProduceIsMultiplierValueList[pst->getName()] += newValue;
				}
			}
			const UpgradeSkillType *ust = dynamic_cast<const UpgradeSkillType *>(skillType);
			if(ust != NULL) {
				int newValue = ((double)ust->getSpeed() * ((double)ut->getProdSpeed(NULL) / (double)100));
				if(boostMode) {
					prodSpeedUpgradeIsMultiplierValueList[ust->getName()] = newValue;
				}
				else {
					prodSpeedUpgradeIsMultiplierValueList[ust->getName()] += newValue;
				}
			}
			const MorphSkillType *mst = dynamic_cast<const MorphSkillType *>(skillType);
			if(mst != NULL) {
				int newValue = ((double)mst->getSpeed() * ((double)ut->getProdSpeed(NULL) / (double)100));
				if(boostMode) {
					prodSpeedMorphIsMultiplierValueList[mst->getName()] = newValue;
				}
				else {
					prodSpeedMorphIsMultiplierValueList[mst->getName()] += newValue;
				}
			}
		}
	}
	else {
		int newValue = ut->getProdSpeed(NULL);
		if(boostMode) {
			prodSpeed = newValue;
		}
		else {
			prodSpeed += newValue;
		}
	}
	
	if(ut->getAttackSpeedIsMultiplier() == true) {
		for(unsigned int i = 0; i < (unsigned int)unit->getType()->getSkillTypeCount(); ++i) {
			const SkillType *skillType = unit->getType()->getSkillType(i);
			const AttackSkillType *ast = dynamic_cast<const AttackSkillType *>(skillType);
			if(ast != NULL) {
				int newValue = ((double)ast->getSpeed() * ((double)ut->getAttackSpeed(NULL) / (double)100));
				if(boostMode) {
					attackSpeedIsMultiplierValueList[ast->getName()] = newValue;
				}
				else {
					attackSpeedIsMultiplierValueList[ast->getName()] += newValue;
				}
			}
		}
	}
	else {
		int newValue = ut->getAttackSpeed(NULL);
		if(boostMode) {
			attackSpeed = newValue;
		}
		else {
			attackSpeed += newValue;
		}
	}
}

void TotalUpgrade::apply(int sourceUnitId, const UpgradeTypeBase *ut, const Unit *unit) {
	//sum(ut, unit);

	//printf("====> About to apply boost: %s\nTo unit: %d\n\n",ut->toString().c_str(),unit->getId());
	TotalUpgrade *boostUpgrade = new TotalUpgrade();
	boostUpgrade->copyDataFrom(this);
	boostUpgrade->boostUpgradeBase = ut;
	boostUpgrade->boostUpgradeSourceUnit = sourceUnitId;
	boostUpgrade->boostUpgradeDestUnit = unit->getId();

	boostUpgrade->sum(ut,unit, true);
	boostUpgrades.push_back(boostUpgrade);
}

void TotalUpgrade::deapply(int sourceUnitId, const UpgradeTypeBase *ut,int destUnitId) {
	//printf("<****** About to de-apply boost: %s\nTo unit: %d\n\n",ut->toString().c_str(),destUnitId);

	bool removedBoost = false;
	for(unsigned int index = 0; index < boostUpgrades.size(); ++index) {
		TotalUpgrade *boost = boostUpgrades[index];
		if(boost->boostUpgradeSourceUnit == sourceUnitId &&
			boost->boostUpgradeBase->getUpgradeName() == ut->getUpgradeName() &&
			boost->boostUpgradeDestUnit == destUnitId) {

			boostUpgrades.erase(boostUpgrades.begin() + index);
			delete boost;
			removedBoost = true;

			//printf("de-apply boost FOUND!\n");
			break;
		}
	}
	if(removedBoost == false) {
		printf("\n\n!!!!!! de-apply boost NOT FOUND for sourceUnitId = %d, destUnitId = %d\n%s\n\nCurrent Boosts:\n",
				sourceUnitId,destUnitId,ut->toString().c_str());
		for(unsigned int index = 0; index < boostUpgrades.size(); ++index) {
			TotalUpgrade *boost = boostUpgrades[index];
			printf("\nBoost #%u\n%s\n",index,boost->toString().c_str());
		}
	}
}

int TotalUpgrade::getMaxHp() const {
	return maxHp + getMaxHpFromBoosts();
}
int TotalUpgrade::getMaxHpFromBoosts() const {
	int result = 0;
	for(unsigned int index = 0; index < boostUpgrades.size(); ++index) {
		TotalUpgrade *boost = boostUpgrades[index];
		result += boost->getMaxHp();
	}
	return result;
}
int TotalUpgrade::getMaxHpRegeneration() const {
	return maxHpRegeneration + getMaxHpRegenerationFromBoosts();
}
int TotalUpgrade::getMaxHpRegenerationFromBoosts() const {
	int result = 0;
	for(unsigned int index = 0; index < boostUpgrades.size(); ++index) {
		TotalUpgrade *boost = boostUpgrades[index];
		result += boost->getMaxHpRegeneration();
	}
	return result;
}
int TotalUpgrade::getSight() const {
	return sight + getSightFromBoosts();
}
int TotalUpgrade::getSightFromBoosts() const {
	int result = 0;
	for(unsigned int index = 0; index < boostUpgrades.size(); ++index) {
		TotalUpgrade *boost = boostUpgrades[index];
		result += boost->getSight();
	}
	return result;
}
int TotalUpgrade::getMaxEp() const {
	return maxEp + getMaxEpFromBoosts();
}
int TotalUpgrade::getMaxEpFromBoosts() const {
	int result = 0;
	for(unsigned int index = 0; index < boostUpgrades.size(); ++index) {
		TotalUpgrade *boost = boostUpgrades[index];
		result += boost->getMaxEp();
	}
	return result;
}

int TotalUpgrade::getMaxEpRegeneration() const {
	return maxEpRegeneration + getMaxEpRegenerationFromBoosts();
}
int TotalUpgrade::getMaxEpRegenerationFromBoosts() const {
	int result = 0;
	for(unsigned int index = 0; index < boostUpgrades.size(); ++index) {
		TotalUpgrade *boost = boostUpgrades[index];
		result += boost->getMaxEpRegeneration();
	}
	return result;
}

int TotalUpgrade::getArmor() const {
	return armor + getArmorFromBoosts();
}
int TotalUpgrade::getArmorFromBoosts() const {
	int result = 0;
	for(unsigned int index = 0; index < boostUpgrades.size(); ++index) {
		TotalUpgrade *boost = boostUpgrades[index];
		result += boost->getArmor();
	}
	return result;
}

int TotalUpgrade::getAttackStrength(const AttackSkillType *st) const {
	return UpgradeTypeBase::getAttackStrength(st) + getAttackStrengthFromBoosts(st);
}
int TotalUpgrade::getAttackStrengthFromBoosts(const AttackSkillType *st) const {
	int result = 0;
	for(unsigned int index = 0; index < boostUpgrades.size(); ++index) {
		TotalUpgrade *boost = boostUpgrades[index];
		result += boost->getAttackStrength(st);
	}
	return result;
}

int TotalUpgrade::getAttackRange(const AttackSkillType *st) const {
	return UpgradeTypeBase::getAttackRange(st) + getAttackRangeFromBoosts(st);
}
int TotalUpgrade::getAttackRangeFromBoosts(const AttackSkillType *st) const {
	int result = 0;
	for(unsigned int index = 0; index < boostUpgrades.size(); ++index) {
		TotalUpgrade *boost = boostUpgrades[index];
		result += boost->getAttackRange(st);
	}
	return result;
}

int TotalUpgrade::getMoveSpeed(const MoveSkillType *st) const {
	return UpgradeTypeBase::getMoveSpeed(st) + getMoveSpeedFromBoosts(st);
}
int TotalUpgrade::getMoveSpeedFromBoosts(const MoveSkillType *st) const {
	int result = 0;
	for(unsigned int index = 0; index < boostUpgrades.size(); ++index) {
		TotalUpgrade *boost = boostUpgrades[index];
		result += boost->getMoveSpeed(st);
	}
	return result;
}

int TotalUpgrade::getProdSpeed(const SkillType *st) const {
	return UpgradeTypeBase::getProdSpeed(st) + getProdSpeedFromBoosts(st);
}
int TotalUpgrade::getProdSpeedFromBoosts(const SkillType *st) const {
	int result = 0;
	for(unsigned int index = 0; index < boostUpgrades.size(); ++index) {
		TotalUpgrade *boost = boostUpgrades[index];
		result += boost->getProdSpeed(st);
	}
	return result;
}

int TotalUpgrade::getAttackSpeed(const AttackSkillType *st) const {
	return UpgradeTypeBase::getAttackSpeed(st) + getAttackSpeedFromBoosts(st);
}
int TotalUpgrade::getAttackSpeedFromBoosts(const AttackSkillType *st) const {
	int result = 0;
	for(unsigned int index = 0; index < boostUpgrades.size(); ++index) {
		TotalUpgrade *boost = boostUpgrades[index];
		result += boost->getAttackSpeed(st);
	}
	return result;
}

void TotalUpgrade::incLevel(const UnitType *ut) {
	maxHp += ut->getMaxHp()*50/100;
	maxEp += ut->getMaxEp()*50/100;
	sight += ut->getSight()*20/100;
	armor += ut->getArmor()*50/100;

	for(unsigned int index = 0; index < boostUpgrades.size(); ++index) {
		TotalUpgrade *boost = boostUpgrades[index];
		boost->copyDataFrom(this);
	}
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

	upgradeTypeBaseNode->addAttribute("attackSpeed",intToStr(attackSpeed), mapTagReplacements);
	upgradeTypeBaseNode->addAttribute("attackSpeedIsMultiplier",intToStr(attackSpeedIsMultiplier), mapTagReplacements);
	for(std::map<string,int>::const_iterator iterMap = attackSpeedIsMultiplierValueList.begin();
			iterMap != attackSpeedIsMultiplierValueList.end(); ++iterMap) {
		XmlNode *attackSpeedIsMultiplierValueListNode = upgradeTypeBaseNode->addChild("attackSpeedIsMultiplierValueList");

		attackSpeedIsMultiplierValueListNode->addAttribute("key",iterMap->first, mapTagReplacements);
		attackSpeedIsMultiplierValueListNode->addAttribute("value",intToStr(iterMap->second), mapTagReplacements);
	}

//	for(unsigned int index = 0; index < boostUpgrades.size(); ++index) {
//		TotalUpgrade *boost = boostUpgrades[index];
//		XmlNode *attackBoostListNode = upgradeTypeBaseNode->addChild("attackBoostList");
//		attackBoostListNode->addAttribute("unitId",intToStr(boost->boostUpgradeUnit->getId()), mapTagReplacements);
//
//		std::map<string,string> mapTagReplacements;
//		if(boost != NULL) {
//			boost->saveGameBoost(attackBoostListNode);
//			boost->boostUpgradeBase->saveGameBoost(attackBoostListNode);
//		}
//	}
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

	if(upgradeTypeBaseNode->hasAttribute("attackSpeed")){
		attackSpeed = upgradeTypeBaseNode->getAttribute("attackSpeed")->getIntValue();
		attackSpeedIsMultiplier = upgradeTypeBaseNode->getAttribute("attackSpeedIsMultiplier")->getIntValue() != 0;
		vector<XmlNode *> attackSpeedIsMultiplierValueNodeList = upgradeTypeBaseNode->getChildList("attackSpeedIsMultiplierValueList");
		for(unsigned int i = 0; i < attackSpeedIsMultiplierValueNodeList.size(); ++i) {
			XmlNode *node = attackSpeedIsMultiplierValueNodeList[i];

			attackSpeedIsMultiplierValueList[node->getAttribute("key")->getValue()] =
											  node->getAttribute("value")->getIntValue();
		}
	}

//	vector<XmlNode *> boostNodeList = upgradeTypeBaseNode->getChildList("attackBoostList");
//	for(unsigned int index = 0; index < boostNodeList.size(); ++index) {
//		XmlNode *boostNode = boostNodeList[index];
//
//	//for(unsigned int index = 0; index < boostUpgrades.size(); ++index) {
//	//	TotalUpgrade *boost = boostUpgrades[index];
//	//	XmlNode *attackBoostListNode = upgradeTypeBaseNode->addChild("attackBoostList");
//
////		std::map<string,string> mapTagReplacements;
////		if(boost != NULL) {
////			boost->saveGame(attackBoostListNode);
////		}
//		int unitId = boostNode->getAttribute("unitId")->getIntValue();
//		const Unit *unit = world->findUnitById(unitId);
//
//		TotalUpgrade *boostUpgrade = new TotalUpgrade();
//		//boostUpgrade->copyDataFrom(this);
//		boostUpgrade->loadGameBoost(boostNode);
//		boostUpgrade->loadGameBoost(boostNode);
//
//		boostUpgrade->boostUpgradeBase = ut;
//		boostUpgrade->boostUpgradeUnit = unit;
//
//		//boostUpgrade->sum(ut,unit);
//		boostUpgrades.push_back(boostUpgrade);
//
//		//boost->saveGameBoost(attackBoostListNode);
//		//boost->boostUpgradeBase->saveGameBoost(attackBoostListNode);
//
//		//apply(const UpgradeTypeBase *ut, unit);
//	}

}


}}//end namespace
