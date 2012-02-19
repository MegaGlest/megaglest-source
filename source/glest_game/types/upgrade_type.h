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

#ifndef _GLEST_GAME_UPGRADETYPE_H_
#define _GLEST_GAME_UPGRADETYPE_H_

#include "element_type.h"
#include "checksum.h"
#include "conversion.h"
#include "xml_parser.h"
#include "leak_dumper.h"

using Shared::Util::Checksum;
using namespace Shared::Util;
using namespace Shared::Xml;

namespace Glest { namespace Game {

class TechTree;
class FactionType;
class UnitType;
class Unit;
class SkillType;
class AttackSkillType;
class MoveSkillType;
class ProduceSkillType;

// ===============================
// 	class UpgradeTypeBase
// ===============================

class UpgradeTypeBase {
protected:
    int maxHp;
    bool maxHpIsMultiplier;
	int maxHpRegeneration;
	//bool maxHpRegenerationIsMultiplier;

    int sight;
    bool sightIsMultiplier;

    int maxEp;
    bool maxEpIsMultiplier;
	int maxEpRegeneration;
	//bool maxEpRegenerationIsMultiplier;

    int armor;
    bool armorIsMultiplier;

    int attackStrength;
    bool attackStrengthIsMultiplier;
    std::map<string,int> attackStrengthMultiplierValueList;

    int attackRange;
    bool attackRangeIsMultiplier;
    std::map<string,int> attackRangeMultiplierValueList;

    int moveSpeed;
    bool moveSpeedIsMultiplier;
    std::map<string,int> moveSpeedIsMultiplierValueList;

    int prodSpeed;
    bool prodSpeedIsMultiplier;
    std::map<string,int> prodSpeedProduceIsMultiplierValueList;
    std::map<string,int> prodSpeedUpgradeIsMultiplierValueList;
    std::map<string,int> prodSpeedMorphIsMultiplierValueList;

public:
	int getMaxHp() const			{return maxHp;}
	bool getMaxHpIsMultiplier() const			{return maxHpIsMultiplier;}
	int getMaxHpRegeneration() const			{return maxHpRegeneration;}
	//bool getMaxHpRegenerationIsMultiplier() const			{return maxHpRegenerationIsMultiplier;}

	int getSight() const			{return sight;}
	bool getSightIsMultiplier() const			{return sightIsMultiplier;}

	int getMaxEp() const			{return maxEp;}
	bool getMaxEpIsMultiplier() const			{return maxEpIsMultiplier;}
	int getMaxEpRegeneration() const			{return maxEpRegeneration;}
	//bool getMaxEpRegenerationIsMultiplier() const			{return maxEpRegenerationIsMultiplier;}

	int getArmor() const			{return armor;}
	bool getArmorIsMultiplier() const			{return armorIsMultiplier;}

	int getAttackStrength(const AttackSkillType *st) const;
	bool getAttackStrengthIsMultiplier() const	{return attackStrengthIsMultiplier;}
	int getAttackRange(const AttackSkillType *st) const;
	bool getAttackRangeIsMultiplier() const		{return attackRangeIsMultiplier;}
	int getMoveSpeed(const MoveSkillType *st) const;
	bool getMoveSpeedIsMultiplier() const		{return moveSpeedIsMultiplier;}
	int getProdSpeed(const SkillType *st) const;
	bool getProdSpeedIsMultiplier() const		{return prodSpeedIsMultiplier;}

	void load(const XmlNode *upgradeNode);

	virtual string getDesc() const;
	std::string toString() const {
		std::string result = "";

		result += "maxHp = " + intToStr(maxHp);
		result += "maxHpIsMultiplier = " + intToStr(maxHpIsMultiplier);
		result += "maxHpRegeneration = " + intToStr(maxHpRegeneration);
		//result += "maxHpRegenerationIsMultiplier = " + intToStr(maxHpRegenerationIsMultiplier);

		result += " sight = " + intToStr(sight);
		result += "sightIsMultiplier = " + intToStr(sightIsMultiplier);

		result += " maxEp = " + intToStr(maxEp);
		result += "maxEpIsMultiplier = " + intToStr(maxEpIsMultiplier);
		result += " maxEpRegeneration = " + intToStr(maxEpRegeneration);
		//result += "maxEpRegenerationIsMultiplier = " + intToStr(maxEpRegenerationIsMultiplier);

		result += " armor = " + intToStr(armor);
		result += "armorIsMultiplier = " + intToStr(armorIsMultiplier);
		result += " attackStrength = " + intToStr(attackStrength);
		result += "attackStrengthIsMultiplier = " + intToStr(attackStrengthIsMultiplier);
		result += " attackRange = " + intToStr(attackRange);
		result += "attackRangeIsMultiplier = " + intToStr(attackRangeIsMultiplier);
		result += " moveSpeed = " + intToStr(moveSpeed);
		result += "moveSpeedIsMultiplier = " + intToStr(moveSpeedIsMultiplier);
		result += " prodSpeed = " + intToStr(prodSpeed);
		result += "prodSpeedIsMultiplier = " + intToStr(prodSpeedIsMultiplier);

		return result;
	}
};

// ===============================
// 	class UpgradeType
// ===============================

class UpgradeType: public UpgradeTypeBase, public ProducibleType {
private:
    vector<const UnitType*> effects;

public:
	void preLoad(const string &dir);
    void load(const string &dir, const TechTree *techTree,
    		const FactionType *factionType, Checksum* checksum,
    		Checksum* techtreeChecksum, std::map<string,vector<pair<string, string> > > &loadedFileList);

    //get all
	int getEffectCount() const				{return effects.size();}
	const UnitType * getEffect(int i) const	{return effects[i];}
	bool isAffected(const UnitType *unitType) const;

    //other methods
	virtual string getReqDesc() const;
};

// ===============================
// 	class TotalUpgrade
// ===============================

class TotalUpgrade: public UpgradeTypeBase {
public:
	TotalUpgrade();

	void reset();
	void sum(const UpgradeTypeBase *ut, const Unit *unit);
	void incLevel(const UnitType *ut);

	void apply(const UpgradeTypeBase *ut, const Unit *unit);
	void deapply(const UpgradeTypeBase *ut, const Unit *unit);
};

}}//end namespace

#endif
