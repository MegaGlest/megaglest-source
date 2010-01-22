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

#ifndef _GLEST_GAME_COMMANDTYPE_H_
#define _GLEST_GAME_COMMANDTYPE_H_

#include "element_type.h"
#include "resource_type.h"
#include "lang.h"
#include "skill_type.h"
#include "factory.h"
#include "xml_parser.h"
#include "sound_container.h"

namespace Glest{ namespace Game{

using Shared::Util::MultiFactory;

class UnitUpdater;
class Unit;
class UnitType;
class TechTree;
class FactionType;

enum CommandClass{
	ccStop,
	ccMove,
	ccAttack,
	ccAttackStopped,
	ccBuild,
	ccHarvest,
	ccRepair,
	ccProduce,
	ccUpgrade,
	ccMorph,

	ccCount,
	ccNull
};

enum Clicks{
	cOne,
	cTwo
};

// =====================================================
// 	class CommandType
//
///	A complex action performed by a unit, composed by skills
// =====================================================

class CommandType: public RequirableType{
protected:
    CommandClass commandTypeClass;
    Clicks clicks;
	int id;

public:
	static const int invalidId= -1;

public:
    virtual void update(UnitUpdater *unitUpdater, Unit *unit) const= 0;
    virtual void load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut);
    virtual string getDesc(const TotalUpgrade *totalUpgrade) const= 0;
	virtual string toString() const= 0;
	virtual const ProducibleType *getProduced() const	{return NULL;}
	virtual bool isQueuable() const						{return false;}

    //get
    CommandClass getClass() const;		
	Clicks getClicks() const		{return clicks;} 
	int getId() const				{return id;}  
};

// ===============================
// 	class StopCommandType  
// ===============================

class StopCommandType: public CommandType{
private:
    const StopSkillType* stopSkillType;

public:
    StopCommandType();
	virtual void update(UnitUpdater *unitUpdater, Unit *unit) const;
    virtual void load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut);
    virtual string getDesc(const TotalUpgrade *totalUpgrade) const;
	virtual string toString() const;

    //get
	const StopSkillType *getStopSkillType() const	{return stopSkillType;};
};


// ===============================
// 	class MoveCommandType  
// ===============================

class MoveCommandType: public CommandType{
private:
    const MoveSkillType *moveSkillType;

public:
    MoveCommandType();
	virtual void update(UnitUpdater *unitUpdater, Unit *unit) const;
    virtual void load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut);
    virtual string getDesc(const TotalUpgrade *totalUpgrade) const;
	virtual string toString() const;

    //get
	const MoveSkillType *getMoveSkillType() const	{return moveSkillType;};
};


// ===============================
// 	class AttackCommandType  
// ===============================

class AttackCommandType: public CommandType{
private:
    const MoveSkillType* moveSkillType;
    const AttackSkillType* attackSkillType;

public:
    AttackCommandType();
	virtual void update(UnitUpdater *unitUpdater, Unit *unit) const;
    virtual void load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut);
    virtual string getDesc(const TotalUpgrade *totalUpgrade) const;
	virtual string toString() const;

    //get
	const MoveSkillType * getMoveSkillType() const			{return moveSkillType;}
	const AttackSkillType * getAttackSkillType() const		{return attackSkillType;}
};

// =======================================
// 	class AttackStoppedCommandType  
// =======================================

class AttackStoppedCommandType: public CommandType{
private:
    const StopSkillType* stopSkillType;
    const AttackSkillType* attackSkillType;

public:
    AttackStoppedCommandType();
	virtual void update(UnitUpdater *unitUpdater, Unit *unit) const;
    virtual void load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut);
    virtual string getDesc(const TotalUpgrade *totalUpgrade) const;
	virtual string toString() const;

    //get
	const StopSkillType * getStopSkillType() const		{return stopSkillType;}
	const AttackSkillType * getAttackSkillType() const	{return attackSkillType;}
};


// ===============================
// 	class BuildCommandType  
// ===============================

class BuildCommandType: public CommandType{
private:
    const MoveSkillType* moveSkillType;
    const BuildSkillType* buildSkillType;
	vector<const UnitType*> buildings;
    SoundContainer startSounds;
    SoundContainer builtSounds;

public:
    BuildCommandType();
    ~BuildCommandType();
	virtual void update(UnitUpdater *unitUpdater, Unit *unit) const;
    virtual void load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut);
    virtual string getDesc(const TotalUpgrade *totalUpgrade) const;
	virtual string toString() const;

    //get
	const MoveSkillType *getMoveSkillType() const	{return moveSkillType;}
	const BuildSkillType *getBuildSkillType() const	{return buildSkillType;}
	int getBuildingCount() const					{return buildings.size();}
	const UnitType * getBuilding(int i) const		{return buildings[i];}
	StaticSound *getStartSound() const				{return startSounds.getRandSound();}
	StaticSound *getBuiltSound() const				{return builtSounds.getRandSound();}
};


// ===============================
// 	class HarvestCommandType  
// ===============================

class HarvestCommandType: public CommandType{
private:
    const MoveSkillType *moveSkillType;
    const MoveSkillType *moveLoadedSkillType;
    const HarvestSkillType *harvestSkillType;
    const StopSkillType *stopLoadedSkillType;
	vector<const ResourceType*> harvestedResources;
	int maxLoad;
    int hitsPerUnit;

public:
    HarvestCommandType();
	virtual void update(UnitUpdater *unitUpdater, Unit *unit) const;
    virtual void load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut);
    virtual string getDesc(const TotalUpgrade *totalUpgrade) const;
	virtual string toString() const;

    //get
	const MoveSkillType *getMoveSkillType() const			{return moveSkillType;}
	const MoveSkillType *getMoveLoadedSkillType() const		{return moveLoadedSkillType;}
	const HarvestSkillType *getHarvestSkillType() const		{return harvestSkillType;}
	const StopSkillType *getStopLoadedSkillType() const		{return stopLoadedSkillType;}
	int getMaxLoad() const									{return maxLoad;}
	int getHitsPerUnit() const								{return hitsPerUnit;}
	int getHarvestedResourceCount() const					{return harvestedResources.size();}
	const ResourceType* getHarvestedResource(int i) const	{return harvestedResources[i];}
	bool canHarvest(const ResourceType *resourceType) const;
};


// ===============================
// 	class RepairCommandType  
// ===============================

class RepairCommandType: public CommandType{
private:
    const MoveSkillType* moveSkillType;
    const RepairSkillType* repairSkillType;
    vector<const UnitType*>  repairableUnits;

public:
    RepairCommandType();
    ~RepairCommandType();
	virtual void update(UnitUpdater *unitUpdater, Unit *unit) const;
    virtual void load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut);
    virtual string getDesc(const TotalUpgrade *totalUpgrade) const;
	virtual string toString() const;

    //get
	const MoveSkillType *getMoveSkillType() const			{return moveSkillType;};
	const RepairSkillType *getRepairSkillType() const		{return repairSkillType;};
    bool isRepairableUnitType(const UnitType *unitType) const;
};


// ===============================
// 	class ProduceCommandType  
// ===============================

class ProduceCommandType: public CommandType{
private:
    const ProduceSkillType* produceSkillType;
	const UnitType *producedUnit;

public:
    ProduceCommandType();
	virtual void update(UnitUpdater *unitUpdater, Unit *unit) const;
    virtual void load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut);
    virtual string getDesc(const TotalUpgrade *totalUpgrade) const;
    virtual string getReqDesc() const;
	virtual string toString() const;
	virtual const ProducibleType *getProduced() const;
	virtual bool isQueuable() const						{return true;}

    //get
	const ProduceSkillType *getProduceSkillType() const	{return produceSkillType;}
	const UnitType *getProducedUnit() const				{return producedUnit;}
};


// ===============================
// 	class UpgradeCommandType  
// ===============================

class UpgradeCommandType: public CommandType{
private:
    const UpgradeSkillType* upgradeSkillType;
    const UpgradeType* producedUpgrade;

public:
    UpgradeCommandType();
	virtual void update(UnitUpdater *unitUpdater, Unit *unit) const;
    virtual void load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut);
    virtual string getDesc(const TotalUpgrade *totalUpgrade) const;
	virtual string toString() const;
	virtual string getReqDesc() const;
	virtual const ProducibleType *getProduced() const;
	virtual bool isQueuable() const						{return true;}

    //get
	const UpgradeSkillType *getUpgradeSkillType() const		{return upgradeSkillType;}
	const UpgradeType *getProducedUpgrade() const			{return producedUpgrade;}
};

// ===============================
// 	class MorphCommandType  
// ===============================

class MorphCommandType: public CommandType{
private:
    const MorphSkillType* morphSkillType;
    const UnitType* morphUnit;
	int discount;

public:
    MorphCommandType();
	virtual void update(UnitUpdater *unitUpdater, Unit *unit) const;
    virtual void load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut);
    virtual string getDesc(const TotalUpgrade *totalUpgrade) const;
	virtual string toString() const;
	virtual string getReqDesc() const;
	virtual const ProducibleType *getProduced() const;

    //get
	const MorphSkillType *getMorphSkillType() const		{return morphSkillType;}
	const UnitType *getMorphUnit() const				{return morphUnit;}
	int getDiscount() const								{return discount;}
};

// ===============================
// 	class CommandFactory  
// ===============================

class CommandTypeFactory: public MultiFactory<CommandType>{
private:
	CommandTypeFactory();

public:
	static CommandTypeFactory &getInstance();
};

}}//end namespace

#endif
