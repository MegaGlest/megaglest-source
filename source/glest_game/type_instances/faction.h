// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_FACTION_H_
#define _GLEST_GAME_FACTION_H_

#include <vector>
#include <map>

#include "upgrade.h"
#include "texture.h"
#include "resource.h"
#include "game_constants.h"
#include "command_type.h"
#include "leak_dumper.h"

using std::map;
using std::vector;

using Shared::Graphics::Texture2D;

namespace Glest{ namespace Game{

class Unit;
class TechTree;
class FactionType;
class ProducibleType;
class RequirableType;
class CommandType;
class UnitType;
class Game;
class ScriptManager;
class World;

// =====================================================
// 	class Faction
//
///	Each of the game players
// =====================================================

class FactionPathSuccessCache {
public:
	// The unit Size for the path findings
	int unitSize;
	// a List of paths with their # success counts
	vector<std::pair<vector<Vec2i>, int> > pathQueue;
};

class Faction {
private:
    typedef vector<Resource> Resources;
    typedef vector<Resource> Store;
	typedef vector<Faction*> Allies;
	typedef vector<Unit*> Units;
	typedef map<int, Unit*> UnitMap;

private:
	UpgradeManager upgradeManager; 

    Resources resources;
    Store store;
	Allies allies;
	Units units;
	UnitMap unitMap;
	World *world;
	ScriptManager *scriptManager;
	
    ControlType control;

	Texture2D *texture;
	FactionType *factionType;

	int index;
	int teamIndex;
	int startLocationIndex;

	bool thisFaction;

	bool cachingDisabled;
	std::map<Vec2i, std::vector<FactionPathSuccessCache> > successfulPathFinderTargetList;
	std::map<Vec2i,int> cacheResourceTargetList;
	std::map<Vec2i,bool> cachedCloseResourceTargetLookupList;
	//time_t lastResourceTargettListPurge;
	//int lastResourceTargettListPurge;

	//std::map<CommandClass,std::map<int,int> > cacheUnitCommandClassList;
	// This cache stores the units free cell movement calcs during a world
	// update of the faction
	//std::map<int,std::map<Field, std::map<Vec2i,std::map<Vec2i, > > > localCacheForUnitCellMovement;

public:
	Faction();
	~Faction();

    void init(
		FactionType *factionType, ControlType control, TechTree *techTree, Game *game,
		int factionIndex, int teamIndex, int startLocationIndex, bool thisFaction, bool giveResources);
	void end();

    //get
	const Resource *getResource(const ResourceType *rt) const;
	const Resource *getResource(int i) const			{return &resources[i];}
	int getStoreAmount(const ResourceType *rt) const;
	const FactionType *getType() const					{return factionType;}
	int getIndex() const								{return index;}
	int getTeam() const									{return teamIndex;}
	bool getCpuControl(bool enableServerControlledAI, bool isNetworkGame, NetworkRole role) const;
	bool getCpuControl() const;
	bool getCpuEasyControl() const						{return control==ctCpuEasy;}
	bool getCpuUltraControl() const						{return control==ctCpuUltra;}
	bool getCpuMegaControl() const						{return control==ctCpuMega;}
	ControlType getControlType() const					{return control;}
	Unit *getUnit(int i) const							{return units[i];}
	int getUnitCount() const							{return units.size();}		
	const UpgradeManager *getUpgradeManager() const		{return &upgradeManager;}
	const Texture2D *getTexture() const					{return texture;}
	int getStartLocationIndex() const					{return startLocationIndex;}
	bool getThisFaction() const							{return thisFaction;}

	//upgrades
	void startUpgrade(const UpgradeType *ut);
	void cancelUpgrade(const UpgradeType *ut);
	void finishUpgrade(const UpgradeType *ut);

	//cost application
	bool applyCosts(const ProducibleType *p);
	void applyDiscount(const ProducibleType *p, int discount);
	void applyStaticCosts(const ProducibleType *p);
	void applyStaticProduction(const ProducibleType *p);
	void deApplyCosts(const ProducibleType *p);
	void deApplyStaticCosts(const ProducibleType *p);
	void deApplyStaticConsumption(const ProducibleType *p);
	void applyCostsOnInterval(const ResourceType *rtApply);
	bool checkCosts(const ProducibleType *pt);

	//reqs
	bool reqsOk(const RequirableType *rt) const;
	bool reqsOk(const CommandType *ct) const;
    int getCountForMaxUnitCount(const UnitType *unitType) const;


	//diplomacy
	bool isAlly(const Faction *faction);

    //other
	Unit *findUnit(int id) const;
	void addUnit(Unit *unit);
	void removeUnit(Unit *unit);
	void addStore(const UnitType *unitType);
	void removeStore(const UnitType *unitType);

	//resources
	void incResourceAmount(const ResourceType *rt, int amount);
	void setResourceBalance(const ResourceType *rt, int balance);

	void setControlType(ControlType value) { control = value; }

	std::vector<Vec2i> findCachedPath(const Vec2i &target, Unit *unit);
	void addCachedPath(const Vec2i &target, Unit *unit);

	bool isResourceTargetInCache(const Vec2i &pos,bool incrementUseCounter=false);
	void addResourceTargetToCache(const Vec2i &pos,bool incrementUseCounter=true);
	void removeResourceTargetFromCache(const Vec2i &pos);
	void addCloseResourceTargetToCache(const Vec2i &pos);
	Vec2i getClosestResourceTypeTargetFromCache(Unit *unit, const ResourceType *type);
	Vec2i getClosestResourceTypeTargetFromCache(const Vec2i &pos, const ResourceType *type);
	void cleanupResourceTypeTargetCache(std::vector<Vec2i> *deleteListPtr);
	int getCacheResourceTargetListSize() const { return cacheResourceTargetList.size(); }

	Unit * findClosestUnitWithSkillClass(const Vec2i &pos,const CommandClass &cmdClass,
										const std::vector<SkillClass> &skillClassList,
										const UnitType *unitType);

	void deletePixels();

	int getFrameCount();
	std::string toString() const;

private:
	void limitResourcesToStore();
	void resetResourceAmount(const ResourceType *rt);
};

}}//end namespace

#endif
