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

#ifndef _GLEST_GAME_UPGRADE_H_
#define _GLEST_GAME_UPGRADE_H_

#ifdef WIN32
    #include <winsock2.h>
    #include <winsock.h>
#endif

#include <vector>
#include <string>
#include <map>
#include "xml_parser.h"
#include "leak_dumper.h"

using std::vector;
using std::map;
using Shared::Xml::XmlNode;

namespace Glest { namespace Game {

class Unit;
class UpgradeType;
class Faction;

/**
 * Stores the state of the upgrade (whether or not the upgrading process is complete).
 */
enum UpgradeState {
	usUpgrading, /**< The upgrade is currently in progress. */
	usUpgraded, /**< The upgrade is completed. */

	upgradeStateCount // TODO: This should be unnecessary -- there's no need to iterate over this enum
};

class UpgradeManager;
class TotalUpgrade;

/**
 * An instance of an upgrade. Factions will typically have one upgrade of each type. This object
 * groups the type, faction, and upgrade state (ie, has the upgrade been obtained yet?).
 */
class Upgrade {
private:
	UpgradeState state;
	// TODO: I believe this is unnecessary. As far as I can tell, it's only used for checking
	// that the unit we're applying UpgradeManager::computeTotalUpgrade to is in this faction. However,
	// I don't see an circumstances when it wouldn't be (since the UpgradeManager already an aggregate
	// of a faction and Unit directly gets the UpgradeManager from the faction (so it must have the
	// same faction as the upgrades in the UpgradeManager).
	int factionIndex;
	const UpgradeType *type;

	friend class UpgradeManager;

	Upgrade();
public:
	/**
	 * Creates an upgrade. The upgrade state will be set to UpgradeState::usUpgrading.
	 * @param upgradeType The type of the upgrade that this corresponds to. Upgrade types are
	 * essentially "classes" for upgrades.
	 * @param factionIndex The index of the faction that the upgrade belongs to.
	 */
	Upgrade(const UpgradeType *upgradeType, int factionIndex);

private:
	UpgradeState getState() const;
	int getFactionIndex() const;
	const UpgradeType * getType() const;

	void setState(UpgradeState state);
	
	/**
	 * Retrieves a string representation of the upgrade (detailing its state, type, and faction).
	 */
	std::string toString() const;
	
	/**
	 * Saves the object state into the given node.
	 * @param rootNode The UpgradeManager node to save object info to.
	 */
	void saveGame(XmlNode *rootNode);
	
	/**
	 * Loads the object state from the given node.
	 * @param rootNode The UpgradeManager node to retrieve object info from.
	 * @param faction The faction that the upgrade belongs to. Used to convert the upgrade type from
	 * the XML string.
	 */
	static Upgrade * loadGame(const XmlNode *rootNode,Faction *faction);
};

/**
 * Manages upgrades by starting, stopping, and finishing upgrades. Each faction has their own
 * upgrade manager.
 */
class UpgradeManager{
private:
	typedef vector<Upgrade*> Upgrades;
	typedef map<const UpgradeType *,int> UgradesLookup;

	/**
	 * List of upgrades that the upgrade manager is working with (either in progress or finished).
	 */
	Upgrades upgrades;

	/**
	 * Maps UpgradeType to the index of the upgrade in UpgradeManager::upgrades.
	 */
	UgradesLookup upgradesLookup;
public:
	~UpgradeManager();

	int getUpgradeCount() const		{return (int)upgrades.size();}
	
	/**
	 * Starts an upgrade.
	 * @param upgradeType The type of the upgrade to start.
	 * @param factionIndex Passed to the constructor of the Upgrade.
	 */
	void startUpgrade(const UpgradeType *upgradeType, int factionIndex);

	/**
	 * Cancels an upgrade before it is finished. The upgrade is removed from the UpgradeManager.
	 * @param upgradeType The type of the upgrade to remove.
	 * @throws megaglest_runtime_error If there is no upgrade of the desired type in the UpgradeManager.
	 */
	void cancelUpgrade(const UpgradeType *upgradeType);

	/**
	 * Sets an Upgrade in the UpgradeManager as finished (ie, the state is UpgradeState::usUpgraded).
	 * @param upgradeType The type of the upgrade to complete.
	 * @throws megaglest_runtime_error If there is no upgrade of the desired type in the UpgradeManager.
	 */
	void finishUpgrade(const UpgradeType *upgradeType);
	
	/**
	 * Returns true if an Upgrade of the desired type has state UpgradeState::usUpgraded (ie, is
	 * finished upgrading).
	 * @param upgradeType The type of the upgrade in question.
	 */
	bool isUpgraded(const UpgradeType *upgradeType) const;

	/**
	 * Returns true if an Upgrade of the desired type has state UpgradeState::usUpgrading (ie, is
	 * currently in progress).
	 * @param upgradeType The type of the upgrade in question.
	 */
	bool isUpgrading(const UpgradeType *upgradeType) const;

	/**
	 * Returns true if an Upgrade of the desired type exists in the UpgradeManager.
	 * @param upgradeType The type of the upgrade in question.
	 */
	bool isUpgradingOrUpgraded(const UpgradeType *upgradeType) const;

	/**
	 * [Sums up](@ref TotalUpgrade::sum) the effect of all upgrades for this faction as they apply
	 * to a particular unit.
	 * @param unit The unit that the TotalUpgrade applies to. This is necessary because some
	 * upgrades provide percentage boosts.
	 * @param totalUpgrade The TotalUpgrade object to modify. Note that it is cleared before values
	 * are calculated.
	 */
	void computeTotalUpgrade(const Unit *unit, TotalUpgrade *totalUpgrade) const;
	
	/**
	 * Retrieves a string representation of the UpgradeManager. Contains the contents of
	 * Upgrade::toString for all upgrades in the UpgradeManager.
	 */
	std::string toString() const;

	/**
	 * Adds a node for the UpgradeManager that contains all the upgrade nodes, saving the object's
	 * state.
	 * @param rootNode The faction node to add the UpgradeManager node to.
	 * @see Upgrade::saveGame
	 */
	void saveGame(XmlNode *rootNode);

	/**
	 * Loads all the upgrades from the UpgradeManager node, effectively reloading the object's
	 * state.
	 * @param rootNode The faction node to get the UpgradeManager node from.
	 * @param faction Only passed to Upgrade::loadGame (which does the actual loading of each
	 * Upgrade object.
	 */
	void loadGame(const XmlNode *rootNode,Faction *faction);
};

}}//end namespace

#endif
