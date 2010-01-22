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

#ifndef _GLEST_GAME_ELEMENTTYPE_H_ 
#define _GLEST_GAME_ELEMENTTYPE_H_

#include <vector>
#include <string>

#include "texture.h"
#include "resource.h"

using std::vector;
using std::string;

using Shared::Graphics::Texture2D;

namespace Glest{ namespace Game{

class UpgradeType;
class TechTree;
class UnitType;
class UpgradeType;
class DisplayableType;
class ResourceType;

// =====================================================
// 	class DisplayableType
//
///	Base class for anything that has a name and a portrait
// =====================================================

class DisplayableType{
protected:
	string name;		//name
	Texture2D *image;	//portrait  

public:
	DisplayableType();
	virtual ~DisplayableType(){};

	//get
	string getName() const				{return name;}
	const Texture2D *getImage() const	{return image;}
};


// =====================================================
// 	class RequirableType  
//
///	Base class for anything that has requirements
// =====================================================

class RequirableType: public DisplayableType{
private:
	typedef vector<const UnitType*> UnitReqs;
	typedef vector<const UpgradeType*> UpgradeReqs;

protected:
	UnitReqs unitReqs;			//needed units
	UpgradeReqs upgradeReqs;	//needed upgrades

public:
	//get
	int getUpgradeReqCount() const						{return upgradeReqs.size();}
	int getUnitReqCount() const							{return unitReqs.size();}
	const UpgradeType *getUpgradeReq(int i) const		{return upgradeReqs[i];}
	const UnitType *getUnitReq(int i) const				{return unitReqs[i];}
    
    //other
    virtual string getReqDesc() const;
};


// =====================================================
// 	class ProducibleType  
//
///	Base class for anything that can be produced
// =====================================================

class ProducibleType: public RequirableType{
private:
	typedef vector<Resource> Costs;

protected:
	Costs costs;
    Texture2D *cancelImage;
	int productionTime;

public:
    ProducibleType();
	virtual ~ProducibleType();

    //get
	int getCostCount() const						{return costs.size();}
	const Resource *getCost(int i) const			{return &costs[i];}
	const Resource *getCost(const ResourceType *rt) const;
	int getProductionTime() const					{return productionTime;}
	const Texture2D *getCancelImage() const	{return cancelImage;}
            
    //varios
    void checkCostStrings(TechTree *techTree);
    
	virtual string getReqDesc() const;
};

}}//end namespace

#endif
