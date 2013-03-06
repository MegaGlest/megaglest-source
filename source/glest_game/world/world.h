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

#ifndef _GLEST_GAME_WORLD_H_
#define _GLEST_GAME_WORLD_H_

#ifdef WIN32
    #include <winsock2.h>
    #include <winsock.h>
#endif

#include "vec.h"
#include "math_util.h"
#include "resource.h"
#include "tech_tree.h"
#include "tileset.h"
#include "console.h"
#include "map.h"
#include "scenario.h"
#include "minimap.h"
#include "logger.h"
#include "stats.h"
#include "time_flow.h"
#include "upgrade.h"
#include "water_effects.h"
#include "faction.h"
#include "unit_updater.h"
#include "randomgen.h"
#include "game_constants.h"
#include "leak_dumper.h"

namespace Glest{ namespace Game{

using Shared::Graphics::Quad2i;
using Shared::Graphics::Rect2i;
using Shared::Util::RandomGen;

class Faction;
class Unit;
class Config;
class Game;
class GameSettings;
class ScriptManager;
class StaticSound;
class StrSound;

// =====================================================
// 	class World
//
///	The game world: Map + Tileset + TechTree
// =====================================================

class ExploredCellsLookupKey {
public:

	Vec2i pos;
	int sightRange;
	int teamIndex;
};

class ExploredCellsLookupItem {
public:

	int ExploredCellsLookupItemCacheTimerCountIndex;
	std::vector<SurfaceCell *> exploredCellList;
	std::vector<SurfaceCell *> visibleCellList;

	static time_t lastDebug;
};

class World {
private:
	typedef vector<Faction *> Factions;

	std::map<Vec2i, std::map<int, ExploredCellsLookupItem > > ExploredCellsLookupItemCache;
	std::map<int,ExploredCellsLookupKey> ExploredCellsLookupItemCacheTimer;
	int ExploredCellsLookupItemCacheTimerCount;

	bool enableFowAlphaCellsLookupItemCache;
	//std::map<Vec2i, std::map<int, FowAlphaCellsLookupItem > > FowAlphaCellsLookupItemCache;

public:
	static const int generationArea= 100;
	static const float airHeight;
	static const int indirectSightRange= 5;

private:

	Map map;
	Tileset tileset;
	//TechTree techTree;
	TechTree *techTree;
	TimeFlow timeFlow;
	Scenario scenario;

	UnitUpdater unitUpdater;
    WaterEffects waterEffects;
    WaterEffects attackEffects; // onMiniMap
	Minimap minimap;
    Stats stats;	//BattleEnd will delete this object

	Factions factions;

	RandomGen random;

	ScriptManager* scriptManager;

	int thisFactionIndex;
	int thisTeamIndex;
	int frameCount;
	//int nextUnitId;
	Mutex mutexFactionNextUnitId;
	std::map<int,int> mapFactionNextUnitId;

	//config
	bool fogOfWarOverride;
	bool fogOfWar;
	int fogOfWarSmoothingFrameSkip;
	bool fogOfWarSmoothing;
	int fogOfWarSkillTypeValue;

	Game *game;
	Chrono chronoPerfTimer;
	bool perfTimerEnabled;

	bool unitParticlesEnabled;
	bool staggeredFactionUpdates;
	std::map<string,StaticSound *> staticSoundList;
	std::map<string,StrSound *> streamSoundList;

	uint32 nextCommandGroupId;

	string queuedScenarioName;
	bool queuedScenarioKeepFactions;

	bool disableAttackEffects;

	const XmlNode *loadWorldNode;

	MasterSlaveThreadController masterController;

	bool originalGameFogOfWar;
	std::map<int,std::pair<const Unit *,const FogOfWarSkillType *> > mapFogOfWarUnitList;

public:
	World();
	~World();
	void cleanup();
	void end(); //to die before selection does
	void endScenario(); //to die before selection does

	void addFogOfWarSkillType(const Unit *unit,const FogOfWarSkillType *fowst);
	void removeFogOfWarSkillType(const Unit *unit);
	bool removeFogOfWarSkillTypeFromList(const Unit *unit);

	//get
	inline int getMaxPlayers() const						{return map.getMaxPlayers();}
	inline int getThisFactionIndex() const					{return thisFactionIndex;}

	inline int getThisTeamIndex() const					{return thisTeamIndex;}
	inline void setThisTeamIndex(int team) 				{ thisTeamIndex=team;}

	inline const Faction *getThisFaction() const			{return factions[thisFactionIndex];}
	inline Faction *getThisFactionPtr() 					{return factions[thisFactionIndex];}

	inline int getFactionCount() const						{return factions.size();}
	inline const Map *getMap() const 						{return &map;}
	inline Map *getMapPtr()  								{return &map;}

	inline const Tileset *getTileset() const 				{return &tileset;}
	inline const TechTree *getTechTree() const 			{return techTree;}
	inline const Scenario *getScenario() const 			{return &scenario;}
	inline const TimeFlow *getTimeFlow() const				{return &timeFlow;}
	inline Tileset *getTileset() 							{return &tileset;}
	inline Map *getMap() 									{return &map;}
	inline const Faction *getFaction(int i) const			{return factions[i];}
	inline Faction *getFaction(int i) 						{return factions[i];}
	inline const Minimap *getMinimap() const				{return &minimap;}
	inline Minimap *getMiniMapObject() 					{return &minimap;}
	inline const Stats *getStats() const					{return &stats;};
	inline Stats *getStats()								{return &stats;};
	inline const WaterEffects *getWaterEffects() const		{return &waterEffects;}
	inline const WaterEffects *getAttackEffects() const		{return &attackEffects;}
	int getNextUnitId(Faction *faction);
	int getNextCommandGroupId();
	inline int getFrameCount() const						{return frameCount;}

	//init & load
	void init(Game *game, bool createUnits, bool initFactions=true);
	Checksum loadTileset(const vector<string> pathList, const string &tilesetName,
			Checksum* checksum, std::map<string,vector<pair<string, string> > > &loadedFileList);
	Checksum loadTileset(const string &dir, Checksum* checksum,
			std::map<string,vector<pair<string, string> > > &loadedFileList);
	void clearTileset();
	Checksum loadTech(const vector<string> pathList, const string &techName,
			set<string> &factions, Checksum* checksum,std::map<string,vector<pair<string, string> > > &loadedFileList);
	Checksum loadMap(const string &path, Checksum* checksum);
	Checksum loadScenario(const string &path, Checksum* checksum,bool resetCurrentScenario=false,const XmlNode *rootNode=NULL);
	void setQueuedScenario(string scenarioName,bool keepFactions);
	inline string getQueuedScenario() const { return queuedScenarioName; }
	inline bool getQueuedScenarioKeepFactions() const { return queuedScenarioKeepFactions; }
	void initUnitsForScenario();

	//misc
	void update();
	Unit* findUnitById(int id) const;
	const UnitType* findUnitTypeById(const FactionType* factionType, int id);
	bool placeUnit(const Vec2i &startLoc, int radius, Unit *unit, bool spaciated= false);
	void moveUnitCells(Unit *unit);

	bool toRenderUnit(const Unit *unit, const Quad2i &visibleQuad) const;
	bool toRenderUnit(const Unit *unit) const;
	bool toRenderUnit(const UnitBuildInfo &pendingUnit) const;


	Unit *nearestStore(const Vec2i &pos, int factionIndex, const ResourceType *rt);
	void addAttackEffects(const Unit *unit);

	//scripting interface
	void morphToUnit(int unitId,const string &morphName,bool ignoreRequirements);
	void createUnit(const string &unitName, int factionIndex, const Vec2i &pos,bool spaciated = true);
	void givePositionCommand(int unitId, const string &commandName, const Vec2i &pos);
	vector<int> getUnitsForFaction(int factionIndex,const string& commandTypeName,int field);
	int getUnitCurrentField(int unitId);
	bool getIsUnitAlive(int unitId);
	void giveAttackCommand(int unitId, int unitToAttackId);
	void giveProductionCommand(int unitId, const string &producedName);
	void giveUpgradeCommand(int unitId, const string &upgradeName);
	void giveAttackStoppedCommand(int unitId, const string &itemName,bool ignoreRequirements);
	void playStaticSound(const string &playSound);
	void playStreamingSound(const string &playSound);
	void stopStreamingSound(const string &playSound);
	void stopAllSound();
	void moveToUnit(int unitId, int destUnitId);
	void togglePauseGame(bool pauseStatus, bool forceAllowPauseStateChange);
	void addConsoleText(const string &text);
	void addConsoleTextWoLang(const string &text);

	void giveResource(const string &resourceName, int factionIndex, int amount);
	int getResourceAmount(const string &resourceName, int factionIndex);
	Vec2i getStartLocation(int factionIndex);
	Vec2i getUnitPosition(int unitId);
	void setUnitPosition(int unitId, Vec2i pos);

	void addCellMarker(Vec2i pos, int factionIndex, const string &note, const string textureFile);
	void removeCellMarker(Vec2i pos, int factionIndex);
	void showMarker(Vec2i pos, int factionIndex, const string &note, const string textureFile,int flashCount);

	int getUnitFactionIndex(int unitId);
	const string getUnitName(int unitId);
	int getUnitCount(int factionIndex);
	int getUnitCountOfType(int factionIndex, const string &typeName);

	const string getSystemMacroValue(const string key);
	const string getPlayerName(int factionIndex);

	void highlightUnit(int unitId,float radius, float thickness, Vec4f color);
	void unhighlightUnit(int unitId);

	void giveStopCommand(int unitId);

	bool selectUnit(int unitId);
	void unselectUnit(int unitId);
	void addUnitToGroupSelection(int unitId,int groupIndex);
	void removeUnitFromGroupSelection(int unitId,int groupIndex);
	void recallGroupSelection(int groupIndex);
	void setAttackWarningsEnabled(bool enabled);
	bool getAttackWarningsEnabled();


	inline Game * getGame() { return game; }
	const GameSettings * getGameSettings() const;

	GameSettings * getGameSettingsPtr();

	std::vector<std::string> validateFactionTypes();
	std::vector<std::string> validateResourceTypes();

	void setFogOfWar(bool value);
	bool getFogOfWar() const { return fogOfWar; }

	std::string DumpWorldToLog(bool consoleBasicInfoOnly = false) const;

	inline int getUpdateFps(int factionIndex) const {
		int result = GameConstants::updateFps;
		//if(factionIndex != -1 && staggeredFactionUpdates == true) {
		//	result = (GameConstants::updateFps / GameConstants::maxPlayers);
		//}
		return result;
	}
	bool canTickWorld() const;

	void exploreCells(const Vec2i &newPos, int sightRange, int teamIndex);
	bool showWorldForPlayer(int factionIndex, bool excludeFogOfWarCheck=false) const;

	inline UnitUpdater * getUnitUpdater() { return &unitUpdater; }

	void playStaticVideo(const string &playVideo);
	void playStreamingVideo(const string &playVideo);
	void stopStreamingVideo(const string &playVideo);
	void stopAllVideo();

	void removeResourceTargetFromCache(const Vec2i &pos);

	string getExploredCellsLookupItemCacheStats();
	string getFowAlphaCellsLookupItemCacheStats();
	string getAllFactionsCacheStats();

	void placeUnitAtLocation(const Vec2i &location, int radius, Unit *unit, bool spaciated);
	void saveGame(XmlNode *rootNode);
	void loadGame(const XmlNode *rootNode);

	void clearCaches();
	void refreshAllUnitExplorations();

private:

	void initCells(bool fogOfWar);
	void initSplattedTextures();
	void initFactionTypes(GameSettings *gs);
	void initMinimap();
	void initUnits();
	void initMap();
	//void initExplorationState();

	//misc
	void tick();
	bool canTickFaction(int factionIdx);
	int tickFactionIndex();
	void computeFow(int factionIdxToTick=-1);

	void updateAllTilesetObjects();
	void updateAllFactionUnits();
	void underTakeDeadFactionUnits();
	void updateAllFactionConsumableCosts();
};

}}//end namespace

#endif
