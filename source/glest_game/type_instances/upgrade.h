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

enum UpgradeState {
	usUpgrading, 
	usUpgraded,

	upgradeStateCount
};

class UpgradeManager;
class TotalUpgrade;

// =====================================================
// 	class Upgrade  
//
/// A bonus to an UnitType
// =====================================================

class Upgrade {
private:
	UpgradeState state;
	int factionIndex;
	const UpgradeType *type;

	friend class UpgradeManager;

	Upgrade();
public:
	Upgrade(const UpgradeType *upgradeType, int factionIndex);

private:
	//get
	UpgradeState getState() const;
	int getFactionIndex() const;
	const UpgradeType * getType() const;

	//set
	void setState(UpgradeState state);

	std::string toString() const;

	void saveGame(XmlNode *rootNode);
	static Upgrade * loadGame(const XmlNode *rootNode,Faction *faction);
};


// ===============================
// 	class UpgradeManager  
// ===============================

class UpgradeManager{
private:	
	typedef vector<Upgrade*> Upgrades;
	typedef map<const UpgradeType *,int> UgradesLookup;
	Upgrades upgrades;
	UgradesLookup upgradesLookup;
public:
	~UpgradeManager();

	int getUpgradeCount() const		{return upgrades.size();}

	void startUpgrade(const UpgradeType *upgradeType, int factionIndex);
	void cancelUpgrade(const UpgradeType *upgradeType);
	void finishUpgrade(const UpgradeType *upgradeType);

	bool isUpgraded(const UpgradeType *upgradeType) const;
	bool isUpgrading(const UpgradeType *upgradeType) const;
	bool isUpgradingOrUpgraded(const UpgradeType *upgradeType) const;
	void computeTotalUpgrade(const Unit *unit, TotalUpgrade *totalUpgrade) const;

	std::string toString() const;
	void saveGame(XmlNode *rootNode);
	void loadGame(const XmlNode *rootNode,Faction *faction);
};

}}//end namespace

#endif
