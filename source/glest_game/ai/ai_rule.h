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

#ifndef _GLEST_GAME_AIRULE_H_
#define _GLEST_GAME_AIRULE_H_

#include <string>

#include "vec.h"
#include "skill_type.h"
#include "leak_dumper.h"

using std::string;

using Shared::Graphics::Vec2i;

namespace Glest{ namespace Game{

class Ai;
class Unit;
class UnitType;
class ProduceTask;
class BuildTask;
class UpgradeTask;
class ResourceType;

// =====================================================
//	class AiRule  
//
///	An action that the AI will perform periodically 
/// if the test succeeds
// =====================================================

class AiRule{
protected:
	Ai *ai;

public:
	AiRule(Ai *ai);
	virtual ~AiRule() {}

	virtual int getTestInterval() const= 0;	//in milliseconds
	virtual string getName() const= 0;

	virtual bool test()= 0;
	virtual void execute()= 0;
};

// =====================================================
//	class AiRuleWorkerHarvest
// =====================================================

class AiRuleWorkerHarvest: public AiRule{
private:
	int stoppedWorkerIndex;

public:
	AiRuleWorkerHarvest(Ai *ai);
	
	virtual int getTestInterval() const	{return 2000;}
	virtual string getName() const		{return "Worker stopped => Order worker to harvest";}

	virtual bool test();
	virtual void execute();
};

// =====================================================
//	class AiRuleRefreshHarvester
// =====================================================

class AiRuleRefreshHarvester: public AiRule{
private:
	int workerIndex;

public:
	AiRuleRefreshHarvester(Ai *ai);
	
	virtual int getTestInterval() const	{return 20000;}
	virtual string getName() const		{return "Worker reasigned to needed resource";}

	virtual bool test();
	virtual void execute();
};

// =====================================================
//	class AiRuleScoutPatrol
// =====================================================

class AiRuleScoutPatrol: public AiRule{
public:
	AiRuleScoutPatrol(Ai *ai);
	
	virtual int getTestInterval() const	{return 10000;}
	virtual string getName() const		{return "Base is stable => Send scout patrol";}

	virtual bool test();
	virtual void execute();
};

// =====================================================
//	class AiRuleRepair
// =====================================================

class AiRuleRepair: public AiRule{
private:
	int damagedUnitIndex;

public:
	AiRuleRepair(Ai *ai);
	
	virtual int getTestInterval() const	{return 10000;}
	virtual string getName() const		{return "Building Damaged => Repair";}

	virtual bool test();
	virtual void execute();
};

// =====================================================
//	class AiRuleReturnBase
// =====================================================

class AiRuleReturnBase: public AiRule{
private:
	int stoppedUnitIndex;
public:
	AiRuleReturnBase(Ai *ai);
	
	virtual int getTestInterval() const	{return 5000;}
	virtual string getName() const		{return "Stopped unit => Order return base";}

	virtual bool test();
	virtual void execute();
};

// =====================================================
//	class AiRuleMassiveAttack
// =====================================================

class AiRuleMassiveAttack: public AiRule{
private:
	static const int baseRadius= 25;

private:
	Vec2i attackPos;
	Field field;
	bool ultraAttack;

public:
	AiRuleMassiveAttack(Ai *ai);
	
	virtual int getTestInterval() const	{return 1000;}
	virtual string getName() const		{return "Unit under attack => Order massive attack";}

	virtual bool test();
	virtual void execute();
};

// =====================================================
//	class AiRuleAddTasks
// =====================================================

class AiRuleAddTasks: public AiRule{
public:
	AiRuleAddTasks(Ai *ai);
	
	virtual int getTestInterval() const	{return 5000;}
	virtual string getName() const		{return "Tasks empty => Add tasks";}

	virtual bool test();
	virtual void execute();
};

// =====================================================
//	class AiRuleBuildOneFarm
// =====================================================

class AiRuleBuildOneFarm: public AiRule{
private:
	const UnitType *farm;

public:
	AiRuleBuildOneFarm(Ai *ai);

	virtual int getTestInterval() const	{return 10000;}
	virtual string getName() const		{return "No farms => Build one";}

	virtual bool test();
	virtual void execute();
};

// =====================================================
//	class AiRuleProduceResourceProducer
// =====================================================

class AiRuleProduceResourceProducer: public AiRule{
private:
	static const int minStaticResources= 20;
	static const int longInterval=	60000;
	static const int shortInterval= 5000;
	const ResourceType *rt;
	int interval;

public:
	AiRuleProduceResourceProducer(Ai *ai);
	
	virtual int getTestInterval() const	{return interval;}
	virtual string getName() const		{return "No resources => Build Resource Producer";}

	virtual bool test();
	virtual void execute();
};

// =====================================================
//	class AiRuleProduce
// =====================================================

class AiRuleProduce: public AiRule{
private:
	const ProduceTask *produceTask;

public:
	AiRuleProduce(Ai *ai);

	virtual int getTestInterval() const	{return 2000;}
	virtual string getName() const		{return "Performing produce task";}

	virtual bool test();
	virtual void execute();

private:
	void produceGeneric(const ProduceTask *pt);
	void produceSpecific(const ProduceTask *pt);
};
// =====================================================
//	class AiRuleBuild
// =====================================================

class AiRuleBuild: public AiRule{
private:
	const BuildTask *buildTask;

public:
	AiRuleBuild(Ai *ai);

	virtual int getTestInterval() const	{return 2000;}
	virtual string getName() const		{return "Performing build task";}

	virtual bool test();
	virtual void execute();

private:
	void buildGeneric(const BuildTask *bt);
	void buildSpecific(const BuildTask *bt);
	void buildBestBuilding(const vector<const UnitType*> &buildings);

	bool isDefensive(const UnitType *building);
	bool isResourceProducer(const UnitType *building);
	bool isWarriorProducer(const UnitType *building);
};

// =====================================================
//	class AiRuleUpgrade
// =====================================================

class AiRuleUpgrade: public AiRule{
private:
	const UpgradeTask *upgradeTask;

public:
	AiRuleUpgrade(Ai *ai);

	virtual int getTestInterval() const	{return 2000;}
	virtual string getName() const		{return "Performing upgrade task";}

	virtual bool test();
	virtual void execute();

private:
	void upgradeSpecific(const UpgradeTask *upgt);
	void upgradeGeneric(const UpgradeTask *upgt);
};

// =====================================================
//	class AiRuleExpand
// =====================================================

class AiRuleExpand: public AiRule{
private:
	static const int expandDistance= 30;

private:
	Vec2i expandPos;
	const UnitType *storeType;

public:
	AiRuleExpand(Ai *ai);

	virtual int getTestInterval() const	{return 30000;}
	virtual string getName() const		{return "Expanding";}

	virtual bool test();
	virtual void execute();
};

}}//end namespace 

#endif
