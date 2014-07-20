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

#ifdef WIN32
    #include <winsock2.h>
    #include <winsock.h>
#endif

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
    }
    virtual ~UpgradeTypeBase() {}

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
	
	/**
	 * Loads the upgrade values (stat boosts and whether or not the boosts use a multiplier) from an
	 * XML node.
	 * @param upgradeNode Node containing the stat boost elements (`max-hp`, `attack-strength`, etc).
	 * @param upgradename Unique identifier for the upgrade.
	 */
	void load(const XmlNode *upgradeNode, string upgradename);
	
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
	std::string toString() const {
		std::string result = "";

		result += "upgradename =" + upgradename;
		result += "maxHp = " + intToStr(maxHp);
		result += "maxHpIsMultiplier = " + intToStr(maxHpIsMultiplier);
		result += "maxHpRegeneration = " + intToStr(maxHpRegeneration);
		//result += "maxHpRegenerationIsMultiplier = " + intToStr(maxHpRegenerationIsMultiplier);

		result += " sight = " + intToStr(sight);
		result += "sightIsMultiplier = " + intToStr(sightIsMultiplier);

		result += " maxEp = " + intToStr(maxEp);
		result += " maxEpIsMultiplier = " + intToStr(maxEpIsMultiplier);
		result += " maxEpRegeneration = " + intToStr(maxEpRegeneration);
		//result += "maxEpRegenerationIsMultiplier = " + intToStr(maxEpRegenerationIsMultiplier);

		result += " armor = " + intToStr(armor);
		result += " armorIsMultiplier = " + intToStr(armorIsMultiplier);
		result += " attackStrength = " + intToStr(attackStrength);
		result += " attackStrengthIsMultiplier = " + intToStr(attackStrengthIsMultiplier);
		result += " attackRange = " + intToStr(attackRange);
		result += " attackRangeIsMultiplier = " + intToStr(attackRangeIsMultiplier);
		result += " moveSpeed = " + intToStr(moveSpeed);
		result += " moveSpeedIsMultiplier = " + intToStr(moveSpeedIsMultiplier);
		result += " prodSpeed = " + intToStr(prodSpeed);
		result += " prodSpeedIsMultiplier = " + intToStr(prodSpeedIsMultiplier);

		return result;
	}

	// TODO: It's not clear if these save game methods are being used, currently. I think
	// attack boosts might use the few lines that aren't commented out.
	virtual void saveGame(XmlNode *rootNode) const;
	static const UpgradeType * loadGame(const XmlNode *rootNode, Faction *faction);

	/**
	 * Generates a checksum value for the upgrade.
	 */
	Checksum getCRC() {
		Checksum crcForUpgradeType;

		crcForUpgradeType.addString(upgradename);
		crcForUpgradeType.addInt(maxHp);
		crcForUpgradeType.addInt(maxHpIsMultiplier);
	    crcForUpgradeType.addInt(maxHpRegeneration);

	    crcForUpgradeType.addInt(sight);
	    crcForUpgradeType.addInt(sightIsMultiplier);

	    crcForUpgradeType.addInt(maxEp);
	    crcForUpgradeType.addInt(maxEpIsMultiplier);
	    crcForUpgradeType.addInt(maxEpRegeneration);

	    crcForUpgradeType.addInt(armor);
	    crcForUpgradeType.addInt(armorIsMultiplier);

	    crcForUpgradeType.addInt(attackStrength);
	    crcForUpgradeType.addInt(attackStrengthIsMultiplier);
	    //std::map<string,int> attackStrengthMultiplierValueList;
	    crcForUpgradeType.addInt64((int64)attackStrengthMultiplierValueList.size());

	    crcForUpgradeType.addInt(attackRange);
	    crcForUpgradeType.addInt(attackRangeIsMultiplier);
	    //std::map<string,int> attackRangeMultiplierValueList;
	    crcForUpgradeType.addInt64((int64)attackRangeMultiplierValueList.size());

	    crcForUpgradeType.addInt(moveSpeed);
	    crcForUpgradeType.addInt(moveSpeedIsMultiplier);
	    //std::map<string,int> moveSpeedIsMultiplierValueList;
	    crcForUpgradeType.addInt64((int64)moveSpeedIsMultiplierValueList.size());

	    crcForUpgradeType.addInt(prodSpeed);
	    crcForUpgradeType.addInt(prodSpeedIsMultiplier);
	    //std::map<string,int> prodSpeedProduceIsMultiplierValueList;
	    crcForUpgradeType.addInt64((int64)prodSpeedProduceIsMultiplierValueList.size());
	    //std::map<string,int> prodSpeedUpgradeIsMultiplierValueList;
	    crcForUpgradeType.addInt64((int64)prodSpeedUpgradeIsMultiplierValueList.size());
	    //std::map<string,int> prodSpeedMorphIsMultiplierValueList;
	    crcForUpgradeType.addInt64((int64)prodSpeedMorphIsMultiplierValueList.size());

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
	 * List of unit types (the "classes" of units, eg, swordman) that are affected by this upgrade.
	 */
    vector<const UnitType*> effects;

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

    /**
	 * Returns the number of UnitTypes affected by this upgrade.
	 */
	int getEffectCount() const				{return (int)effects.size();}

    /**
	 * Returns a particular unit type affected by this upgrade.
	 * @param i Index of the unit type in the #effects list.
	 */
	const UnitType * getEffect(int i) const	{return effects[i];}

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
	void sum(const UpgradeTypeBase *ut, const Unit *unit);

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
	void apply(const UpgradeTypeBase *ut, const Unit *unit);

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
	void deapply(const UpgradeTypeBase *ut, const Unit *unit);

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
