// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Martio Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_SCRIPT_MANAGER_H_
#define _GLEST_GAME_SCRIPT_MANAGER_H_

#ifdef WIN32
    #include <winsock2.h>
    #include <winsock.h>
#endif

#include <string>
#include <list>
#include "lua_script.h"
#include "components.h"
#include "game_constants.h"
#include <map>
#include "xml_parser.h"
#include "randomgen.h"
#include "leak_dumper.h"

using std::string;
//using std::queue;
using std::list;
using Shared::Graphics::Vec2i;
using Shared::Lua::LuaScript;
using Shared::Lua::LuaHandle;
using Shared::Xml::XmlNode;
using Shared::Util::RandomGen;

namespace Glest{ namespace Game{

class World;
class Unit;
class GameCamera;

// =====================================================
//	class ScriptManagerMessage
// =====================================================

class ScriptManagerMessage {
private:
	string text;
	string header;
	int factionIndex;
	int teamIndex;

public:
	ScriptManagerMessage();
	ScriptManagerMessage(string text, string header, int factionIndex=-1,int teamIndex=-1);
	const string &getText() const	{return text;}
	const string &getHeader() const	{return header;}
	int getFactionIndex() const	{return factionIndex;}
	int getTeamIndex() const	{return teamIndex;}

	void saveGame(XmlNode *rootNode);
	void loadGame(const XmlNode *rootNode);
};

class PlayerModifiers{
public:
	PlayerModifiers();

	void disableAi()			{aiEnabled= false;}
	void enableAi()				{aiEnabled= true;}

	void setAsWinner()			{winner= true;}

	void disableConsume()		{consumeEnabled= false;}
	void enableConsume()			{consumeEnabled= true;}

	bool getWinner() const			{return winner;}

	bool getAiEnabled() const		{return aiEnabled;}
	bool getConsumeEnabled() const	{return consumeEnabled;}

	void saveGame(XmlNode *rootNode);
	void loadGame(const XmlNode *rootNode);

private:

	bool winner;
	bool aiEnabled;
	bool consumeEnabled;
};

// =====================================================
//	class ScriptManager
// =====================================================

enum CellTriggerEventType {
	ctet_Unit,
	ctet_UnitPos,
	ctet_Faction,
	ctet_FactionPos,
	ctet_UnitAreaPos,
	ctet_FactionAreaPos,
	ctet_AreaPos
};

class CellTriggerEvent {
public:
	CellTriggerEvent();
	CellTriggerEventType type;
	int sourceId;
	int destId;
	Vec2i destPos;
	Vec2i destPosEnd;

	int triggerCount;

	std::map<int,string> eventStateInfo;

	std::map<int,std::map<Vec2i, bool> > eventLookupCache;

	void saveGame(XmlNode *rootNode);
	void loadGame(const XmlNode *rootNode);
};

class TimerTriggerEvent {
public:
	TimerTriggerEvent();
	bool running;
	//time_t startTime;
	//time_t endTime;
	int startFrame;
	int endFrame;

	int triggerSecondsElapsed;

	void saveGame(XmlNode *rootNode);
	void loadGame(const XmlNode *rootNode);
};

class ScriptManager {
private:
	typedef list<ScriptManagerMessage> MessageQueue;

private:

	//lua
	string code;
	LuaScript luaScript;

	//world
	World *world;
	GameCamera *gameCamera;

	//misc
	MessageQueue messageQueue;
	GraphicMessageBox messageBox;
	string displayText;

	//last created unit
	string lastCreatedUnitName;
	int lastCreatedUnitId;

	//last dead unit
	string lastDeadUnitName;
	int lastDeadUnitId;
	int lastDeadUnitCauseOfDeath;

	//last dead unit's killer
	string lastDeadUnitKillerName;
	int lastDeadUnitKillerId;

	//last attacked unit
	string lastAttackedUnitName;
	int lastAttackedUnitId;

	//last attacking unit
	string lastAttackingUnitName;
	int lastAttackingUnitId;

	// end game state
	bool gameOver;
	bool gameWon;
	PlayerModifiers playerModifiers[GameConstants::maxPlayers];

	int currentTimerTriggeredEventId;
	int currentCellTriggeredEventId;
	int currentCellTriggeredEventUnitId;

	int currentCellTriggeredEventAreaEntryUnitId;
	int currentCellTriggeredEventAreaExitUnitId;

	int currentEventId;
	std::map<int,CellTriggerEvent> CellTriggerEventList;
	std::map<int,TimerTriggerEvent> TimerTriggerEventList;
	bool inCellTriggerEvent;
	std::vector<int> unRegisterCellTriggerEventList;

	RandomGen random;
	const XmlNode *rootNode;

private:
	static ScriptManager* thisScriptManager;

private:
	static const int messageWrapCount;
	static const int displayTextWrapCount;

public:

	ScriptManager();
	~ScriptManager();
	void init(World* world, GameCamera *gameCamera,const XmlNode *rootNode);

	//message box functions
	bool getMessageBoxEnabled() const									{return !messageQueue.empty();}
	GraphicMessageBox* getMessageBox()									{return &messageBox;}
	string getDisplayText() const										{return displayText;}
	const PlayerModifiers *getPlayerModifiers(int factionIndex) const	{return &playerModifiers[factionIndex];}

	//events
	void onMessageBoxOk(bool popFront=true);
	void onResourceHarvested();
	void onUnitCreated(const Unit* unit);
	void onUnitDied(const Unit* unit);
	void onUnitAttacked(const Unit* unit);
	void onUnitAttacking(const Unit* unit);
	void onGameOver(bool won);
	void onCellTriggerEvent(Unit *movingUnit);
	void onTimerTriggerEvent();

	bool getGameWon() const;
	bool getIsGameOver() const;

	void saveGame(XmlNode *rootNode);
	void loadGame(const XmlNode *rootNode);

private:
	string wrapString(const string &str, int wrapCount);

	//wrappers, commands
	void networkShowMessageForFaction(const string &text, const string &header,int factionIndex);
	void networkShowMessageForTeam(const string &text, const string &header,int teamIndex);

	void networkSetCameraPositionForFaction(int factionIndex, const Vec2i &pos);
	void networkSetCameraPositionForTeam(int teamIndex, const Vec2i &pos);

	void showMessage(const string &text, const string &header);
	void clearDisplayText();
	void setDisplayText(const string &text);
	void addConsoleText(const string &text);
	void addConsoleLangText(const char *fmt,...);
	void DisplayFormattedText(const char *fmt,...);
	void DisplayFormattedLangText(const char *fmt,...);
	void setCameraPosition(const Vec2i &pos);
	void createUnit(const string &unitName, int factionIndex, Vec2i pos);
	void createUnitNoSpacing(const string &unitName, int factionIndex, Vec2i pos);

	void destroyUnit(int unitId);
	void giveKills(int unitId, int amount);
	void morphToUnit(int unitId,const string &morphName, int ignoreRequirements);
	void moveToUnit(int unitId,int destUnitId);
	void giveAttackStoppedCommand(int unitId, const string &valueName,int ignoreRequirements);
	void playStaticSound(const string &playSound);
	void playStreamingSound(const string &playSound);
	void stopStreamingSound(const string &playSound);
	void stopAllSound();
	void togglePauseGame(int pauseStatus);

	void playStaticVideo(const string &playVideo);
	void playStreamingVideo(const string &playVideo);
	void stopStreamingVideo(const string &playVideo);
	void stopAllVideo();

	void giveResource(const string &resourceName, int factionIndex, int amount);
	void givePositionCommand(int unitId, const string &producedName, const Vec2i &pos);
	void giveProductionCommand(int unitId, const string &producedName);
	void giveAttackCommand(int unitId, int unitToAttackId);
	void giveUpgradeCommand(int unitId, const string &upgradeName);

	void disableAi(int factionIndex);
	void enableAi(int factionIndex);
	void disableConsume(int factionIndex);
	void enableConsume(int factionIndex);

	int registerCellTriggerEventForUnitToUnit(int sourceUnitId, int destUnitId);
	int registerCellTriggerEventForUnitToLocation(int sourceUnitId, const Vec2i &pos);
	int registerCellTriggerEventForFactionToUnit(int sourceFactionId, int destUnitId);
	int registerCellTriggerEventForFactionToLocation(int sourceFactionId, const Vec2i &pos);

	int registerCellAreaTriggerEventForUnitToLocation(int sourceUnitId, const Vec4i &pos);
	int registerCellAreaTriggerEventForFactionToLocation(int sourceFactionId, const Vec4i &pos);

	int registerCellAreaTriggerEvent(const Vec4i &pos);

	int getCellTriggerEventCount(int eventId);
	void unregisterCellTriggerEvent(int eventId);
	int startTimerEvent();
	int startEfficientTimerEvent(int triggerSecondsElapsed);
	int resetTimerEvent(int eventId);
	int stopTimerEvent(int eventId);
	int getTimerEventSecondsElapsed(int eventId);

	int getCellTriggeredEventId();
	int getTimerTriggeredEventId();

	int getCellTriggeredEventAreaEntryUnitId();
	int getCellTriggeredEventAreaExitUnitId();

	int getCellTriggeredEventUnitId();

	void setRandomGenInit(int seed);
	int getRandomGen(int minVal, int maxVal);
	int getWorldFrameCount();

	bool getAiEnabled(int factionIndex);
	bool getConsumeEnabled(int factionIndex);

	void setPlayerAsWinner(int factionIndex);
	void endGame();

	void startPerformanceTimer();
	void endPerformanceTimer();
	Vec2i getPerformanceTimerResults();

	//wrappers, queries
	int getIsUnitAlive(int unitId);
	Vec2i getStartLocation(int factionIndex);
	Vec2i getUnitPosition(int unitId);
	int getUnitFaction(int unitId);
	const string getUnitName(int unitId);
	int getResourceAmount(const string &resourceName, int factionIndex);
	const string &getLastCreatedUnitName();
	int getLastCreatedUnitId();

	void setUnitPosition(int unitId, Vec2i pos);

	void addCellMarker(Vec2i pos, int factionIndex, const string &note, const string &textureFile);
	void removeCellMarker(Vec2i pos, int factionIndex);

	void showMarker(Vec2i pos, int factionIndex, const string &note, const string &textureFile, int flashCount);

	const string &getLastDeadUnitName();
	int getLastDeadUnitId();
	int getLastDeadUnitCauseOfDeath();
	const string &getLastDeadUnitKillerName();
	int getLastDeadUnitKillerId();

	const string &getLastAttackedUnitName();
	int getLastAttackedUnitId();

	const string &getLastAttackingUnitName();
	int getLastAttackingUnitId();

	int getUnitCount(int factionIndex);
	int getUnitCountOfType(int factionIndex, const string &typeName);

	const string getSystemMacroValue(const string &key);
	const string getPlayerName(int factionIndex);

	vector<int> getUnitsForFaction(int factionIndex,const string& commandTypeName, int field);
	int getUnitCurrentField(int unitId);

	void loadScenario(const string &name, bool keepFactions);

	int isFreeCellsOrHasUnit(int field, int unitId,Vec2i pos);
	int isFreeCells(int unitSize, int field,Vec2i pos);

	int getHumanFactionId();
	void highlightUnit(int unitId, float radius, float thickness, Vec4f color);
	void unhighlightUnit(int unitId);

	//callbacks, commands
	static int networkShowMessageForFaction(LuaHandle* luaHandle);
	static int networkShowMessageForTeam(LuaHandle* luaHandle);

	static int networkSetCameraPositionForFaction(LuaHandle* luaHandle);
	static int networkSetCameraPositionForTeam(LuaHandle* luaHandle);

	static int showMessage(LuaHandle* luaHandle);
	static int setDisplayText(LuaHandle* luaHandle);
	static int addConsoleText(LuaHandle* luaHandle);
	static int addConsoleLangText(LuaHandle* luaHandle);
	static int DisplayFormattedText(LuaHandle* luaHandle);
	static int DisplayFormattedLangText(LuaHandle* luaHandle);
	static int clearDisplayText(LuaHandle* luaHandle);
	static int setCameraPosition(LuaHandle* luaHandle);
	static int createUnit(LuaHandle* luaHandle);
	static int createUnitNoSpacing(LuaHandle* luaHandle);

	static int destroyUnit(LuaHandle* luaHandle);
	static int giveKills(LuaHandle* luaHandle);
	static int morphToUnit(LuaHandle* luaHandle);
	static int moveToUnit(LuaHandle* luaHandle);
	static int giveAttackStoppedCommand(LuaHandle* luaHandle);
	static int playStaticSound(LuaHandle* luaHandle);
	static int playStreamingSound(LuaHandle* luaHandle);
	static int stopStreamingSound(LuaHandle* luaHandle);
	static int stopAllSound(LuaHandle* luaHandle);
	static int togglePauseGame(LuaHandle* luaHandle);

	static int playStaticVideo(LuaHandle* luaHandle);
	static int playStreamingVideo(LuaHandle* luaHandle);
	static int stopStreamingVideo(LuaHandle* luaHandle);
	static int stopAllVideo(LuaHandle* luaHandle);

	static int giveResource(LuaHandle* luaHandle);
	static int givePositionCommand(LuaHandle* luaHandle);
	static int giveProductionCommand(LuaHandle* luaHandle);
	static int giveAttackCommand(LuaHandle* luaHandle);
	static int giveUpgradeCommand(LuaHandle* luaHandle);

	static int disableAi(LuaHandle* luaHandle);
	static int enableAi(LuaHandle* luaHandle);

	static int disableConsume(LuaHandle* luaHandle);
	static int enableConsume(LuaHandle* luaHandle);

	static int getAiEnabled(LuaHandle* luaHandle);
	static int getConsumeEnabled(LuaHandle* luaHandle);

	static int registerCellTriggerEventForUnitToUnit(LuaHandle* luaHandle);
	static int registerCellTriggerEventForUnitToLocation(LuaHandle* luaHandle);
	static int registerCellTriggerEventForFactionToUnit(LuaHandle* luaHandle);
	static int registerCellTriggerEventForFactionToLocation(LuaHandle* luaHandle);

	static int registerCellAreaTriggerEventForUnitToLocation(LuaHandle* luaHandle);
	static int registerCellAreaTriggerEventForFactionToLocation(LuaHandle* luaHandle);

	static int registerCellAreaTriggerEvent(LuaHandle* luaHandle);

	static int getCellTriggerEventCount(LuaHandle* luaHandle);
	static int unregisterCellTriggerEvent(LuaHandle* luaHandle);
	static int startTimerEvent(LuaHandle* luaHandle);
	static int startEfficientTimerEvent(LuaHandle* luaHandle);
	static int resetTimerEvent(LuaHandle* luaHandle);
	static int stopTimerEvent(LuaHandle* luaHandle);
	static int getTimerEventSecondsElapsed(LuaHandle* luaHandle);

	static int getCellTriggeredEventId(LuaHandle* luaHandle);
	static int getTimerTriggeredEventId(LuaHandle* luaHandle);

	static int getCellTriggeredEventAreaEntryUnitId(LuaHandle* luaHandle);
	static int getCellTriggeredEventAreaExitUnitId(LuaHandle* luaHandle);

	static int getCellTriggeredEventUnitId(LuaHandle* luaHandle);

	static int setRandomGenInit(LuaHandle* luaHandle);
	static int getRandomGen(LuaHandle* luaHandle);
	static int getWorldFrameCount(LuaHandle* luaHandle);

	static int setPlayerAsWinner(LuaHandle* luaHandle);
	static int endGame(LuaHandle* luaHandle);

	static int startPerformanceTimer(LuaHandle* luaHandle);
	static int endPerformanceTimer(LuaHandle* luaHandle);
	static int getPerformanceTimerResults(LuaHandle* luaHandle);

	//callbacks, queries
	static int getIsUnitAlive(LuaHandle* luaHandle);
	static int getStartLocation(LuaHandle* luaHandle);
	static int getUnitPosition(LuaHandle* luaHandle);
	static int getUnitFaction(LuaHandle* luaHandle);
	static int getUnitName(LuaHandle* luaHandle);
	static int getResourceAmount(LuaHandle* luaHandle);
	static int getLastCreatedUnitName(LuaHandle* luaHandle);
	static int getLastCreatedUnitId(LuaHandle* luaHandle);

	static int setUnitPosition(LuaHandle* luaHandle);

	static int addCellMarker(LuaHandle* luaHandle);
	static int removeCellMarker(LuaHandle* luaHandle);

	static int showMarker(LuaHandle* luaHandle);

	static int getLastDeadUnitName(LuaHandle* luaHandle);
	static int getLastDeadUnitId(LuaHandle* luaHandle);
	static int getLastDeadUnitCauseOfDeath(LuaHandle* luaHandle);
	static int getLastDeadUnitKillerName(LuaHandle* luaHandle);
	static int getLastDeadUnitKillerId(LuaHandle* luaHandle);

	static int getLastAttackedUnitName(LuaHandle* luaHandle);
	static int getLastAttackedUnitId(LuaHandle* luaHandle);

	static int getLastAttackingUnitName(LuaHandle* luaHandle);
	static int getLastAttackingUnitId(LuaHandle* luaHandle);

	static int getUnitCount(LuaHandle* luaHandle);
	static int getUnitCountOfType(LuaHandle* luaHandle);

	static int getGameWon(LuaHandle* luaHandle);
	static int getIsGameOver(LuaHandle* luaHandle);

	static int getSystemMacroValue(LuaHandle* luaHandle);
	static int getPlayerName(LuaHandle* luaHandle);
	static int scenarioDir(LuaHandle* luaHandle);

	static int loadScenario(LuaHandle* luaHandle);

	static int getUnitsForFaction(LuaHandle* luaHandle);
	static int getUnitCurrentField(LuaHandle* luaHandle);

	static int isFreeCellsOrHasUnit(LuaHandle* luaHandle);
	static int isFreeCells(LuaHandle* luaHandle);

	static int getHumanFactionId(LuaHandle* luaHandle);

	static int highlightUnit(LuaHandle* luaHandle);
	static int unhighlightUnit(LuaHandle* luaHandle);
};

}}//end namespace

#endif
