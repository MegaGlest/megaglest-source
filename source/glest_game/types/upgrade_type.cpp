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

const string VALUE_PERCENT_MULTIPLIER_KEY_NAME = "value-percent-multiplier";

void UpgradeTypeBase::load(const XmlNode *upgradeNode) {
	//values
	maxHpIsMultiplier = false;
	maxHp= upgradeNode->getChild("max-hp")->getAttribute("value")->getIntValue();
	if(upgradeNode->getChild("max-hp")->getAttribute(VALUE_PERCENT_MULTIPLIER_KEY_NAME,false) != NULL) {
		maxHpIsMultiplier = upgradeNode->getChild("max-hp")->getAttribute(VALUE_PERCENT_MULTIPLIER_KEY_NAME)->getBoolValue();

		//printf("Found maxHpIsMultiplier = %d\n",maxHpIsMultiplier);
	}

	maxEpIsMultiplier = false;
	maxEp= upgradeNode->getChild("max-ep")->getAttribute("value")->getIntValue();
	if(upgradeNode->getChild("max-ep")->getAttribute(VALUE_PERCENT_MULTIPLIER_KEY_NAME,false) != NULL) {
		maxEpIsMultiplier = upgradeNode->getChild("max-ep")->getAttribute(VALUE_PERCENT_MULTIPLIER_KEY_NAME)->getBoolValue();

		//printf("Found maxEpIsMultiplier = %d\n",maxEpIsMultiplier);
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
			throw runtime_error("Unsupported skilltype in getProdSpeed!");
		}

		return result;
	}
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
		maxHp += (unit->getHp() * (ut->getMaxHp() / 100));
		//printf("#1.1 Maxhp maxHp = %d, unit->getHp() = %d ut->getMaxHp() = %d\n",maxHp,unit->getHp(),ut->getMaxHp());
	}
	else {
		//printf("#2 Maxhp maxHp = %d, unit->getHp() = %d ut->getMaxHp() = %d\n",maxHp,unit->getHp(),ut->getMaxHp());
		maxHp += ut->getMaxHp();
	}

	if(ut->getMaxEpIsMultiplier() == true) {
		maxEp += (unit->getEp() * (ut->getMaxEp() / 100));
	}
	else {
		maxEp += ut->getMaxEp();
	}

	if(ut->getSightIsMultiplier() == true) {
		sight += (unit->getType()->getSight() * (ut->getSight() / 100));
	}
	else {
		sight += ut->getSight();
	}

	if(ut->getArmorIsMultiplier() == true) {
		armor += (unit->getType()->getArmor() * (ut->getArmor() / 100));
	}
	else {
		armor += ut->getArmor();
	}

	if(ut->getAttackStrengthIsMultiplier() == true) {
		for(unsigned int i = 0; i < unit->getType()->getSkillTypeCount(); ++i) {
			const SkillType *skillType = unit->getType()->getSkillType(i);
			const AttackSkillType *ast = dynamic_cast<const AttackSkillType *>(skillType);
			if(ast != NULL) {
				attackStrengthMultiplierValueList[ast->getName()] += (ast->getAttackStrength() * (ut->getAttackStrength(NULL) / 100));
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
				attackRangeMultiplierValueList[ast->getName()] += (ast->getAttackRange() * (ut->getAttackRange(NULL) / 100));
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
				moveSpeedIsMultiplierValueList[mst->getName()] += (mst->getSpeed() * (ut->getMoveSpeed(NULL) / 100));

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
				prodSpeedProduceIsMultiplierValueList[pst->getName()] += (pst->getSpeed() * (ut->getProdSpeed(NULL) / 100));
			}
			const UpgradeSkillType *ust = dynamic_cast<const UpgradeSkillType *>(skillType);
			if(ust != NULL) {
				prodSpeedUpgradeIsMultiplierValueList[ust->getName()] += (ust->getSpeed() * (ut->getProdSpeed(NULL) / 100));
			}
			const MorphSkillType *mst = dynamic_cast<const MorphSkillType *>(skillType);
			if(mst != NULL) {
				prodSpeedMorphIsMultiplierValueList[mst->getName()] += (mst->getSpeed() * (ut->getProdSpeed(NULL) / 100));
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
		maxHp -= (unit->getHp() * (ut->getMaxHp() / 100));
	}
	else {
		maxHp -= ut->getMaxHp();
	}

	if(ut->getMaxEpIsMultiplier() == true) {
		maxEp -= (unit->getEp() * (ut->getMaxEp() / 100));
	}
	else {
		maxEp -= ut->getMaxEp();
	}

	if(ut->getSightIsMultiplier() == true) {
		sight -= (unit->getType()->getSight() * (ut->getSight() / 100));
	}
	else {
		sight -= ut->getSight();
	}

	if(ut->getArmorIsMultiplier() == true) {
		armor -= (unit->getType()->getArmor() * (ut->getArmor() / 100));
	}
	else {
		armor -= ut->getArmor();
	}

	if(ut->getAttackStrengthIsMultiplier() == true) {
		for(unsigned int i = 0; i < unit->getType()->getSkillTypeCount(); ++i) {
			const SkillType *skillType = unit->getType()->getSkillType(i);
			const AttackSkillType *ast = dynamic_cast<const AttackSkillType *>(skillType);
			if(ast != NULL) {
				attackStrengthMultiplierValueList[ast->getName()] -= (ast->getAttackStrength() * (ut->getAttackStrength(NULL) / 100));
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
				attackRangeMultiplierValueList[ast->getName()] -= (ast->getAttackRange() * (ut->getAttackRange(NULL) / 100));
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
				moveSpeedIsMultiplierValueList[mst->getName()] -= (mst->getSpeed() * (ut->getMoveSpeed(NULL) / 100));
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
				prodSpeedProduceIsMultiplierValueList[pst->getName()] -= (pst->getSpeed() * (ut->getProdSpeed(NULL) / 100));
			}
			const UpgradeSkillType *ust = dynamic_cast<const UpgradeSkillType *>(skillType);
			if(ust != NULL) {
				prodSpeedUpgradeIsMultiplierValueList[ust->getName()] -= (ust->getSpeed() * (ut->getProdSpeed(NULL) / 100));
			}
			const MorphSkillType *mst = dynamic_cast<const MorphSkillType *>(skillType);
			if(mst != NULL) {
				prodSpeedMorphIsMultiplierValueList[mst->getName()] -= (mst->getSpeed() * (ut->getProdSpeed(NULL) / 100));
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

}}//end namespace
