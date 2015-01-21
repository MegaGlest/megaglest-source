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

/**
 * @file
 * Classified the Upgrade type (which is sort of like a class for upgrades). Each upgrade has a
 * type that details the stats that it boosts and the units that it affects. Also has TotalUpgrade,
 * which is a sum of all upgrades applied to a particular unit (and is what determines how units
 * stats are modified by an upgrade.
 */

#ifndef _GLEST_GAME_UPGRADETYPE_H_
#define _GLEST_GAME_UPGRADETYPE_H_

#ifdef WIN32
    #include <winsock2.h>
    #include <winsock.h>
#endif

#include "element_type.h"
#include "checksum.h"
#include "conversion.h"
#include "xml_parser.h"
#include "leak_dumper.h"
#include <set>

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
class Faction;

/**
 * Groups all information used for upgrades. Attack boosts also use this class for modifying stats.
 */
class UpgradeTypeBase {
protected:
	string upgradename;
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
	/**
	 * List of the values (for each skill type) that the stat was boosted by. This is used so
	 * that we can restore the original values when the upgrade is removed (eg, an attack
	 * boost wears off).
	 */
    std::map<string,int> attackStrengthMultiplierValueList;

    int attackRange;
    bool attackRangeIsMultiplier;
    std::map<string,int> attackRangeMultiplierValueList; /**< @see #attackStrengthMultiplierValueList */

    int moveSpeed;
    bool moveSpeedIsMultiplier;
    std::map<string,int> moveSpeedIsMultiplierValueList; /**< @see #attackStrengthMultiplierValueList */

    int prodSpeed;
    bool prodSpeedIsMultiplier;
    std::map<string,int> prodSpeedProduceIsMultiplierValueList; /**< @see #attackStrengthMultiplierValueList */
    std::map<string,int> prodSpeedUpgradeIsMultiplierValueList; /**< @see #attackStrengthMultiplierValueList */
    std::map<string,int> prodSpeedMorphIsMultiplierValueList; /**< @see #attackStrengthMultiplierValueList */

	int attackSpeed;
	bool attackSpeedIsMultiplier;
	std::map<string,int> attackSpeedIsMultiplierValueList;

protected:

	virtual int getAttackStrength() const { return attackStrength; }
	virtual int getAttackRange() const { return attackRange; }
	virtual int getMoveSpeed() const { return moveSpeed; }
	virtual int getProdSpeed() const { return prodSpeed; }
	virtual int getAttackSpeed() const { return attackSpeed; }

    virtual int getMaxHpFromBoosts() const { return 0; }
    virtual int getMaxHpRegenerationFromBoosts() const { return 0; }
    virtual int getSightFromBoosts() const { return 0; }
    virtual int getMaxEpFromBoosts() const { return 0; }
    virtual int getMaxEpRegenerationFromBoosts() const { return 0; }
    virtual int getArmorFromBoosts() const { return 0; };
    virtual int getAttackStrengthFromBoosts(const AttackSkillType *st) const { return 0; }
    virtual int getAttackRangeFromBoosts(const AttackSkillType *st) const { return 0; }
    virtual int getMoveSpeedFromBoosts(const MoveSkillType *st) const { return 0; }
    virtual int getProdSpeedFromBoosts(const SkillType *st) const { return 0; }
    virtual int getAttackSpeedFromBoosts(const AttackSkillType *st) const { return 0; }

public:
	/**
	 * Creates an UpgradeTypeBase with values such that there are no stat changes.
	 */
    UpgradeTypeBase() {
        maxHp = 0;;
        maxHpIsMultiplier = false;
    	maxHpRegeneration = 0;
        sight = 0;
        sightIsMultiplier = false;
        maxEp = 0;;
        maxEpIsMultiplier = false;
    	maxEpRegeneration = 0;
        armor = 0;
        armorIsMultiplier = false;
        attackStrength = 0;
        attackStrengthIsMultiplier = false;
        attackRange = 0;
        attackRangeIsMultiplier = false;
        moveSpeed = 0;
        moveSpeedIsMultiplier = false;
        prodSpeed = 0;
        prodSpeedIsMultiplier = false;
        attackSpeed = 0;
        attackSpeedIsMultiplier = false;
    }
    virtual ~UpgradeTypeBase() {}

	virtual void copyDataFrom(UpgradeTypeBase *source);

    virtual string getUpgradeName() const { return upgradename; }
    virtual int getMaxHp() const			{return maxHp;}
    virtual int getMaxHpRegeneration() const			{return maxHpRegeneration;}
    virtual int getSight() const			{return sight;}
    virtual int getMaxEp() const			{return maxEp;}
    virtual int getMaxEpRegeneration() const			{return maxEpRegeneration;}
    virtual int getArmor() const			{return armor;}
    virtual int getAttackStrength(const AttackSkillType *st) const;
    virtual int getAttackRange(const AttackSkillType *st) const;
    virtual int getMoveSpeed(const MoveSkillType *st) const;
    virtual int getProdSpeed(const SkillType *st) const;
    virtual int getAttackSpeed(const AttackSkillType *st) const;

	virtual bool getAttackStrengthIsMultiplier() const	{return attackStrengthIsMultiplier;}
	virtual bool getMaxHpIsMultiplier() const			{return maxHpIsMultiplier;}
	virtual bool getSightIsMultiplier() const			{return sightIsMultiplier;}
	virtual bool getMaxEpIsMultiplier() const			{return maxEpIsMultiplier;}
	virtual bool getArmorIsMultiplier() const			{return armorIsMultiplier;}
	virtual bool getAttackRangeIsMultiplier() const		{return attackRangeIsMultiplier;}
	virtual bool getMoveSpeedIsMultiplier() const		{return moveSpeedIsMultiplier;}
	virtual bool getProdSpeedIsMultiplier() const		{return prodSpeedIsMultiplier;}
	virtual bool getAttackSpeedIsMultiplier() const		{return attackSpeedIsMultiplier;}

	/**
	 * Loads the upgrade values (stat boosts and whether or not the boosts use a multiplier) from an
	 * XML node.
	 * @param upgradeNode Node containing the stat boost elements (`max-hp`, `attack-strength`, etc).
	 * @param upgradename Unique identifier for the upgrade.
	 */

    virtual void load(const XmlNode *upgradeNode, string upgradename);
	
	/**
	 * Creates a string representation of the upgrade. All stat boosts are detailed on their own line
	 * with their corresponding boosts.
	 * @param translatedValue If true, the description is translated. Otherwise the description uses
	 * names as they appear in the XMLs.
	 */
    virtual string getDesc(bool translatedValue) const;
	
	/**
	 * Returns a string representation of this object. Lists all the value that the object stores.
	 * For debugging purposes, only.
	 */
    virtual std::string toString() const {
		std::string result = "";

		result += "upgradename =" + getUpgradeName();
		result += "maxHp = " + intToStr(getMaxHp());
		result += "maxHpIsMultiplier = " + intToStr(getMaxHpIsMultiplier());
		result += "maxHpRegeneration = " + intToStr(getMaxHpRegeneration());
		//result += "maxHpRegenerationIsMultiplier = " + intToStr(maxHpRegenerationIsMultiplier);

		result += " sight = " + intToStr(getSight());
		result += "sightIsMultiplier = " + intToStr(getSightIsMultiplier());

		result += " maxEp = " + intToStr(getMaxEp());
		result += " maxEpIsMultiplier = " + intToStr(getMaxEpIsMultiplier());
		result += " maxEpRegeneration = " + intToStr(getMaxEpRegeneration());
		//result += "maxEpRegenerationIsMultiplier = " + intToStr(maxEpRegenerationIsMultiplier);

		result += " armor = " + intToStr(getArmor());
		result += " armorIsMultiplier = " + intToStr(getArmorIsMultiplier());
		result += " attackStrength = " + intToStr(getAttackStrength());
		result += " attackStrengthIsMultiplier = " + intToStr(getAttackStrengthIsMultiplier());
		result += " attackRange = " + intToStr(getAttackRange());
		result += " attackRangeIsMultiplier = " + intToStr(getAttackRangeIsMultiplier());
		result += " moveSpeed = " + intToStr(getMoveSpeed());
		result += " moveSpeedIsMultiplier = " + intToStr(getMoveSpeedIsMultiplier());
		result += " prodSpeed = " + intToStr(getProdSpeed());
		result += " prodSpeedIsMultiplier = " + intToStr(getProdSpeedIsMultiplier());

		return result;
	}

	// TODO: It's not clear if these save game methods are being used, currently. I think
	// attack boosts might use the few lines that aren't commented out.
	virtual void saveGame(XmlNode *rootNode) const;
	virtual void saveGameBoost(XmlNode *rootNode) const;
	static const UpgradeType * loadGame(const XmlNode *rootNode, Faction *faction);
	void loadGameBoost(const XmlNode *rootNode);

	/**
	 * Generates a checksum value for the upgrade.
	 */
	virtual Checksum getCRC() {
		Checksum crcForUpgradeType;

		crcForUpgradeType.addString(getUpgradeName());
		crcForUpgradeType.addInt(getMaxHp());
		crcForUpgradeType.addInt(getMaxHpIsMultiplier());
	    crcForUpgradeType.addInt(getMaxHpRegeneration());

	    crcForUpgradeType.addInt(getSight());
	    crcForUpgradeType.addInt(getSightIsMultiplier());

	    crcForUpgradeType.addInt(getMaxEp());
	    crcForUpgradeType.addInt(getMaxEpIsMultiplier());
	    crcForUpgradeType.addInt(getMaxEpRegeneration());

	    crcForUpgradeType.addInt(getArmor());
	    crcForUpgradeType.addInt(getArmorIsMultiplier());

	    crcForUpgradeType.addInt(getAttackStrength());
	    crcForUpgradeType.addInt(getAttackStrengthIsMultiplier());
	    //std::map<string,int> attackStrengthMultiplierValueList;
	    crcForUpgradeType.addInt64((int64)attackStrengthMultiplierValueList.size());

	    crcForUpgradeType.addInt(getAttackRange());
	    crcForUpgradeType.addInt(getAttackRangeIsMultiplier());
	    //std::map<string,int> attackRangeMultiplierValueList;
	    crcForUpgradeType.addInt64((int64)attackRangeMultiplierValueList.size());

	    crcForUpgradeType.addInt(getMoveSpeed());
	    crcForUpgradeType.addInt(getMoveSpeedIsMultiplier());
	    //std::map<string,int> moveSpeedIsMultiplierValueList;
	    crcForUpgradeType.addInt64((int64)moveSpeedIsMultiplierValueList.size());

	    crcForUpgradeType.addInt(getProdSpeed());
	    crcForUpgradeType.addInt(getProdSpeedIsMultiplier());
	    //std::map<string,int> prodSpeedProduceIsMultiplierValueList;
	    crcForUpgradeType.addInt64((int64)prodSpeedProduceIsMultiplierValueList.size());
	    //std::map<string,int> prodSpeedUpgradeIsMultiplierValueList;
	    crcForUpgradeType.addInt64((int64)prodSpeedUpgradeIsMultiplierValueList.size());
	    //std::map<string,int> prodSpeedMorphIsMultiplierValueList;
	    crcForUpgradeType.addInt64((int64)prodSpeedMorphIsMultiplierValueList.size());
		
	    crcForUpgradeType.addInt(getAttackSpeed());
	    crcForUpgradeType.addInt(getAttackSpeedIsMultiplier());

		return crcForUpgradeType;
	}
};

/**
 * Represents the type of upgrade. That is, the single upgrade as it appears in the faction's XML
 * files. Each upgrade has a single `UpgradeType`. Contains information about what units are
 * affected by the upgrade.
 */
class UpgradeType: public UpgradeTypeBase, public ProducibleType {
private:
	/**
	* Set of unit types (the "classes" of units, eg, swordman) that are affected by this upgrade.
	*/
    std::set<const UnitType*> effects;
    std::set<string> tags;

public:
	/**
	 * Sets the upgrade name to the directory name (the base name of `dir`).
	 * @param dir Path of the upgrade directory.
	 */
	void preLoad(const string &dir);

	/**
	 * Loads an upgrade from an XML file.
	 * @param dir Path of the upgrade directory. The file name is determined from this.
	 * @param techTree The techtree that this upgrade is in. Used to access the common data
	 * directory and to access resources.
	 * @param factionType The faction type (a unique type for each faction) that the upgrade belongs
	 * to. Used for accessing unit types that are in the unit requirements list.
	 * @param checksum Will have the checksum of the upgrade path added to it (treated the same way
	 * as the `techtreeChecksum`).
	 * @param techtreeChecksum Cumulative checksum for the techtree. The path of loaded upgrades
	 * is added to this checksum.
	 */
    void load(const string &dir, const TechTree *techTree,
    		const FactionType *factionType, Checksum* checksum,
    		Checksum* techtreeChecksum,
    		std::map<string,vector<pair<string, string> > > &loadedFileList,
    		bool validationMode=false);
	
	/**
	 * Obtains the upgrade name.
	 * @param translatedValue If true, the name is translated. Otherwise the name is returned as it
	 * appears in the XMLs.
	 */
    virtual string getName(bool translatedValue=false) const;
	string getTagName(string tag, bool translatedValue=false) const;

	/**
	 * Determines if a unit is affected by this upgrade.
	 * @param unitType The UnitType we are checking (to see if they're affected).
	 * @return True if the unit is affected, false otherwise.
	 */
	bool isAffected(const UnitType *unitType) const;

    /**
	 * Creates a description for this upgrade. Lists the affected units.
	 */
	virtual string getReqDesc(bool translatedValue) const;

	//virtual void saveGame(XmlNode *rootNode) const;
	//virtual void loadGame(const XmlNode *rootNode);
};

/**
 * Keeps track of the cumulative effects of upgrades on units. This allows us to apply multiple
 * upgrades to a unit with the effects stacking.
 */
class TotalUpgrade: public UpgradeTypeBase {

private:

	// List of boosts
	const UpgradeTypeBase *boostUpgradeBase;
	int boostUpgradeSourceUnit;
	int boostUpgradeDestUnit;
	std::vector<TotalUpgrade *> boostUpgrades;

public:
	TotalUpgrade();
	virtual ~TotalUpgrade() {}

	/**
	 * Resets all stat boosts (so there's effectively no upgrade).
	 */
	void reset();

	/**
	 * Adds an upgrade to this one, stacking the effects. Note that multipliers are stacked
	 * by multiplying by the original, unboosted amount, and then adding that to the TotalUpgrade.
	 * @param ut The upgrade to apply.
	 * @param unit The unit this TotalUpgrade is associated with (since when we use a multiplier,
	 * the stats raise by an amount relative to the unit's base stats).
	 */
	void sum(const UpgradeTypeBase *ut, const Unit *unit, bool boostMode=false);

	/**
	 * Increases the level of the unit. Doing so results in their HP, EP, and armour going up by
	 * 50% while their sight goes up by 20%.
	 * @param ut The unit type to get the original stats from (so we can determine just how much
	 * to increase the stats by on level up).
	 */
	void incLevel(const UnitType *ut);

	/**
	 * Applies the upgrade. Just a delegate to TotalUpgrade::sum.
	 */
	void apply(int sourceUnitId, const UpgradeTypeBase *ut, const Unit *unit);

	/**
	 * Removes the effect of an upgrade to a specific unit. Using this after applying the upgrade
	 * is an invariant. ie,
	 * 
	 *     totalUpgrade->apply(upgrade, unit);
	 *     totalUpgrade->deapply(upgrade, unit);
	 *     // totalUpgrade is now the same as before the call to apply()
	 *
	 * @param ut The upgrade to remove.
	 * @param unit The unit this TotalUpgrade is associated with (since when we use a multiplier,
	 * the stats were raise by an amount relative to the unit's base stats).
	 */
	void deapply(int sourceUnitId, const UpgradeTypeBase *ut,int destUnitId);

    virtual int getMaxHp() const;
    virtual int getMaxHpRegeneration() const;
    virtual int getSight() const;
    virtual int getMaxEp() const;
    virtual int getMaxEpRegeneration() const;
    virtual int getArmor() const;
    virtual int getAttackStrength(const AttackSkillType *st) const;
    virtual int getAttackRange(const AttackSkillType *st) const;
    virtual int getMoveSpeed(const MoveSkillType *st) const;
    virtual int getProdSpeed(const SkillType *st) const;
    virtual int getAttackSpeed(const AttackSkillType *st) const;

    virtual int getMaxHpFromBoosts() const;
    virtual int getMaxHpRegenerationFromBoosts() const;
    virtual int getSightFromBoosts() const;
    virtual int getMaxEpFromBoosts() const;
    virtual int getMaxEpRegenerationFromBoosts() const;
    virtual int getArmorFromBoosts() const;
    virtual int getAttackStrengthFromBoosts(const AttackSkillType *st) const;
    virtual int getAttackRangeFromBoosts(const AttackSkillType *st) const;
    virtual int getMoveSpeedFromBoosts(const MoveSkillType *st) const;
    virtual int getProdSpeedFromBoosts(const SkillType *st) const;
    virtual int getAttackSpeedFromBoosts(const AttackSkillType *st) const;

	/**
	 * Creates the XML for the save game file. Essentially just stores everything about its state.
	 * @rootNode The node of the unit that this TotalUpgrade object belongs to.
	 */
	void saveGame(XmlNode *rootNode) const;

	/**
	 * Reloads the object's state from a saved game.
	 * @rootNode The node of the unit that this TotalUpgrade object belongs to.
	 */
	void loadGame(const XmlNode *rootNode);
};

}}//end namespace

#endif
