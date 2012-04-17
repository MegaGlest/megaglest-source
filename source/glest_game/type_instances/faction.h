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

#ifndef _GLEST_GAME_FACTION_H_
#define _GLEST_GAME_FACTION_H_

#include <vector>
#include <map>

#include "upgrade.h"
#include "texture.h"
#include "resource.h"
#include "game_constants.h"
#include "command_type.h"
#include "base_thread.h"
#include <set>
#include "faction_type.h"
#include "leak_dumper.h"

using std::map;
using std::vector;
using std::set;

using Shared::Graphics::Texture2D;
using namespace Shared::PlatformCommon;

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
class Faction;
class GameSettings;

// =====================================================
// 	class Faction
//
///	Each of the game players
// =====================================================

struct CommandGroupUnitSorter {
	bool operator()(const Unit *l, const Unit *r);
	bool compare(const Unit *l, const Unit *r);
};

struct CommandGroupUnitSorterId {
	Faction *faction;
	bool operator()(const int l, const int r);
};

class FactionThread : public BaseThread {
protected:

	Faction *faction;
	Semaphore semTaskSignalled;
	Mutex *triggerIdMutex;
	std::pair<int,bool> frameIndex;

	virtual void setQuitStatus(bool value);
	virtual void setTaskCompleted(int frameIndex);
	virtual bool canShutdown(bool deleteSelfIfShutdownDelayed=false);

public:
	FactionThread(Faction *faction);
	virtual ~FactionThread();
    virtual void execute();
    void signalPathfinder(int frameIndex);
    bool isSignalPathfinderCompleted(int frameIndex);
};

class SwitchTeamVote {
public:

	int factionIndex;
	int oldTeam;
	int newTeam;
	bool voted;
	bool allowSwitchTeam;
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

	Mutex *unitsMutex;
	Units units;
	UnitMap unitMap;
	World *world;
	ScriptManager *scriptManager;
	
	FactionPersonalityType overridePersonalityType;
    ControlType control;

	Texture2D *texture;
	FactionType *factionType;

	int index;
	int teamIndex;
	int startLocationIndex;

	bool thisFaction;

	bool factionDisconnectHandled;

	bool cachingDisabled;
	std::map<Vec2i,int> cacheResourceTargetList;
	std::map<Vec2i,bool> cachedCloseResourceTargetLookupList;

	RandomGen random;
	FactionThread *workerThread;

	std::map<int,SwitchTeamVote> switchTeamVotes;
	int currentSwitchTeamVoteFactionIndex;

	set<int> livingUnits;
	set<Unit*> livingUnitsp;

	std::map<int,int> unitsMovingList;
	std::map<int,int> unitsPathfindingList;

	TechTree *techTree;
	const XmlNode *loadWorldNode;

public:
	Faction();
	~Faction();

	inline void addLivingUnits(int id) { livingUnits.insert(id); }
	inline void addLivingUnitsp(Unit *unit) { livingUnitsp.insert(unit); }

	inline bool isUnitInLivingUnitsp(Unit *unit) { return (livingUnitsp.find(unit) != livingUnitsp.end()); }
	inline void deleteLivingUnits(int id) { livingUnits.erase(id); }
	inline void deleteLivingUnitsp(Unit *unit) { livingUnitsp.erase(unit); }

	//std::map<int,int> unitsMovingList;
	void addUnitToMovingList(int unitId);
	void removeUnitFromMovingList(int unitId);
	int getUnitMovingListCount();

	void addUnitToPathfindingList(int unitId);
	void removeUnitFromPathfindingList(int unitId);
	int getUnitPathfindingListCount();
	void clearUnitsPathfinding();
	bool canUnitsPathfind();

    void init(
		FactionType *factionType, ControlType control, TechTree *techTree, Game *game,
		int factionIndex, int teamIndex, int startLocationIndex, bool thisFaction,
		bool giveResources, const XmlNode *loadWorldNode=NULL);
	void end();

	inline bool getFactionDisconnectHandled() const { return factionDisconnectHandled;}
	void setFactionDisconnectHandled(bool value) { factionDisconnectHandled=value;}

    //get
	const Resource *getResource(const ResourceType *rt) const;
	inline const Resource *getResource(int i) const			{return &resources[i];}
	int getStoreAmount(const ResourceType *rt) const;
	inline const FactionType *getType() const					{return factionType;}
	inline int getIndex() const								{return index;}

	inline int getTeam() const									{return teamIndex;}
	void setTeam(int team) 								{teamIndex=team;}

	inline TechTree * getTechTree() const						{ return techTree; }
	const SwitchTeamVote * getFirstSwitchTeamVote() const;
	SwitchTeamVote * getSwitchTeamVote(int factionIndex);
	void setSwitchTeamVote(SwitchTeamVote &vote);
	inline int getCurrentSwitchTeamVoteFactionIndex() const { return currentSwitchTeamVoteFactionIndex; }
	void setCurrentSwitchTeamVoteFactionIndex(int index) { currentSwitchTeamVoteFactionIndex = index; }

	bool getCpuControl(bool enableServerControlledAI, bool isNetworkGame, NetworkRole role) const;
	bool getCpuControl() const;
	inline bool getCpuEasyControl() const						{return control==ctCpuEasy;}
	inline bool getCpuUltraControl() const						{return control==ctCpuUltra;}
	inline bool getCpuMegaControl() const						{return control==ctCpuMega;}
	inline ControlType getControlType() const					{return control;}

	FactionPersonalityType getPersonalityType() const;
	void setPersonalityType(FactionPersonalityType pType) { overridePersonalityType=pType; }
	int getAIBehaviorStaticOverideValue(AIBehaviorStaticValueCategory type) const;

	Unit *getUnit(int i) const;
	int getUnitCount() const;
	Mutex * getUnitMutex() {return unitsMutex;}

	inline const UpgradeManager *getUpgradeManager() const		{return &upgradeManager;}
	inline const Texture2D *getTexture() const					{return texture;}
	inline int getStartLocationIndex() const					{return startLocationIndex;}
	inline bool getThisFaction() const							{return thisFaction;}

	//upgrades
	void startUpgrade(const UpgradeType *ut);
	void cancelUpgrade(const UpgradeType *ut);
	void finishUpgrade(const UpgradeType *ut);

	//cost application
	bool applyCosts(const ProducibleType *p,const CommandType *ct);
	void applyDiscount(const ProducibleType *p, int discount);
	void applyStaticCosts(const ProducibleType *p,const CommandType *ct);
	void applyStaticProduction(const ProducibleType *p,const CommandType *ct);
	void deApplyCosts(const ProducibleType *p,const CommandType *ct);
	void deApplyStaticCosts(const ProducibleType *p,const CommandType *ct);
	void deApplyStaticConsumption(const ProducibleType *p,const CommandType *ct);
	void applyCostsOnInterval(const ResourceType *rtApply);
	bool checkCosts(const ProducibleType *pt,const CommandType *ct);

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

	bool isResourceTargetInCache(const Vec2i &pos,bool incrementUseCounter=false);
	void addResourceTargetToCache(const Vec2i &pos,bool incrementUseCounter=true);
	void removeResourceTargetFromCache(const Vec2i &pos);
	void addCloseResourceTargetToCache(const Vec2i &pos);
	Vec2i getClosestResourceTypeTargetFromCache(Unit *unit, const ResourceType *type);
	Vec2i getClosestResourceTypeTargetFromCache(const Vec2i &pos, const ResourceType *type);
	void cleanupResourceTypeTargetCache(std::vector<Vec2i> *deleteListPtr);
	inline int getCacheResourceTargetListSize() const { return cacheResourceTargetList.size(); }

	Unit * findClosestUnitWithSkillClass(const Vec2i &pos,const CommandClass &cmdClass,
										const std::vector<SkillClass> &skillClassList,
										const UnitType *unitType);

	void deletePixels();

	inline World * getWorld() { return world; }
	int getFrameCount();
	void signalWorkerThread(int frameIndex);
	bool isWorkerThreadSignalCompleted(int frameIndex);
	void limitResourcesToStore();

	void sortUnitsByCommandGroups();

	bool canCreateUnit(const UnitType *ut, bool checkBuild, bool checkProduce, bool checkMorph) const;

	string getCacheStats();
	uint64 getCacheKBytes(uint64 *cache1Size, uint64 *cache2Size);

	std::string toString() const;

	void saveGame(XmlNode *rootNode);
	void loadGame(const XmlNode *rootNode, int factionIndex,GameSettings *settings,World *world);

private:
	void resetResourceAmount(const ResourceType *rt);
};

}}//end namespace

#endif
