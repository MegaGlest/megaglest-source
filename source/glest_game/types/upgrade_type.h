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

using Shared::Util::Checksum;

namespace Glest{ namespace Game{

class TechTree;
class FactionType;
class UnitType;

// ===============================
// 	class UpgradeTypeBase 
// ===============================

class UpgradeTypeBase{
protected:
    int maxHp;
    int sight;
    int maxEp;
    int armor;
    int attackStrength;
    int attackRange;
    int moveSpeed;
    int prodSpeed;    

public:
	int getMaxHp() const			{return maxHp;}
	int getSight() const			{return sight;}
	int getMaxEp() const			{return maxEp;}
	int getArmor() const			{return armor;}
	int getAttackStrength() const	{return attackStrength;}
	int getAttackRange() const		{return attackRange;}
	int getMoveSpeed() const		{return moveSpeed;}
	int getProdSpeed() const		{return prodSpeed;}
};

// ===============================
// 	class UpgradeType  
// ===============================

class UpgradeType: public UpgradeTypeBase, public ProducibleType{
private:
    vector<const UnitType*> effects; 
    
public:
	void preLoad(const string &dir);
    void load(const string &dir, const TechTree *techTree, const FactionType *factionType, Checksum* checksum);

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

class TotalUpgrade: public UpgradeTypeBase{
public:
	TotalUpgrade();

	void reset();
	void sum(const UpgradeType *ut); 
	void incLevel(const UnitType *ut);
};

}}//end namespace

#endif
