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

#ifndef _GLEST_GAME_TECHTREE_H_
#define _GLEST_GAME_TECHTREE_H_

#include <set>

#include "util.h"
#include "resource_type.h"
#include "faction_type.h"
#include "damage_multiplier.h"

namespace Glest{ namespace Game{

// =====================================================
// 	class TechTree
//
///	A set of factions and resources
// =====================================================

class TechTree{
private:
	typedef vector<ResourceType> ResourceTypes;
	typedef vector<FactionType> FactionTypes;
	typedef vector<ArmorType> ArmorTypes;
	typedef vector<AttackType> AttackTypes;

private:
    string desc;
    ResourceTypes resourceTypes;
    FactionTypes factionTypes;
	ArmorTypes armorTypes;
	AttackTypes attackTypes;
	DamageMultiplierTable damageMultiplierTable;

public:
    void loadTech(const vector<string> pathList, const string &techName, set<string> &factions, Checksum* checksum);
    void load(const string &dir, set<string> &factions, Checksum* checksum);
    ~TechTree();

    //get
	int getResourceTypeCount() const							{return resourceTypes.size();}
	int getTypeCount() const									{return factionTypes.size();}
	const FactionType *getType(int i) const						{return &factionTypes[i];}
	const ResourceType *getResourceType(int i) const			{return &resourceTypes[i];}
    const string &getDesc() const								{return desc;}
	const FactionType *getType(const string &name) const;
	const ResourceType *getResourceType(const string &name) const;
    const ResourceType *getTechResourceType(int i) const;
    const ResourceType *getFirstTechResourceType() const;
	const ArmorType *getArmorType(const string &name) const;
	const AttackType *getAttackType(const string &name) const;
	float getDamageMultiplier(const AttackType *att, const ArmorType *art) const;
};

}} //end namespace

#endif
