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

#include <string>
#include <queue>

#include "lua_script.h"
#include "components.h"
#include "game_constants.h"
#include <map>
#include "leak_dumper.h"

using std::string;
using std::queue;
using Shared::Graphics::Vec2i;
using Shared::Lua::LuaScript;
using Shared::Lua::LuaHandle;

namespace Glest{ namespace Game{

class World;
class Unit;
class GameCamera;

// =====================================================
//	class ScriptManagerMessage
// =====================================================

class ScriptManagerMessage{
private:
	string text;
	string header;

public:
	ScriptManagerMessage(string text, string header)	{this->text= text, this->header= header;}
	const string &getText() const	{return text;}
	const string &getHeader() const	{return header;}
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
	ctet_FactionPos
};

class CellTriggerEvent {
public:
	CellTriggerEventType type;
	int sourceId;
	int destId;
	Vec2i destPos;

	int triggerCount;
};

class TimerTriggerEvent {
public:
	bool running;
	//time_t startTime;
	//time_t endTime;
	int startFrame;
	int endFrame;

};

class ScriptManager {
private:
	typedef queue<ScriptManagerMessage> MessageQueue;

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

	// end game state
	bool gameOver;
	bool gameWon;
	PlayerModifiers playerModifiers[GameConstants::maxPlayers];

	int currentTimerTriggeredEventId;
	int currentCellTriggeredEventId;
	int currentEventId;
	std::map<int,CellTriggerEvent> CellTriggerEventList;
	std::map<int,TimerTriggerEvent> TimerTriggerEventList;
	bool inCellTriggerEvent;
	std::vector<int> unRegisterCellTriggerEventList;

private:
	static ScriptManager* thisScriptManager;

private:
	static const int messageWrapCount;
	static const int displayTextWrapCount;

public:

	ScriptManager();
	~ScriptManager();
	void init(World* world, GameCamera *gameCamera);

	//message box functions
	bool getMessageBoxEnabled() const									{return !messageQueue.empty();}
	GraphicMessageBox* getMessageBox()									{return &messageBox;}
	string getDisplayText() const										{return displayText;}
	bool getGameOver() const											{return gameOver;}
	bool getGameWon() const												{return gameWon;}
	const PlayerModifiers *getPlayerModifiers(int factionIndex) const	{return &playerModifiers[factionIndex];}

	//events
	void onMessageBoxOk();
	void onResourceHarvested();
	void onUnitCreated(const Unit* unit);
	void onUnitDied(const Unit* unit);
	void onGameOver(bool won);
	void onCellTriggerEvent(Unit *movingUnit);
	void onTimerTriggerEvent();

private:
	string wrapString(const string &str, int wrapCount);

	//wrappers, commands
	void showMessage(const string &text, const string &header);
	void clearDisplayText();
	void setDisplayText(const string &text);
	void addConsoleText(const string &text);
	void addConsoleLangText(const char *fmt,...);
	void DisplayFormattedText(const char *fmt,...);
	void DisplayFormattedLangText(const char *fmt,...);
	void setCameraPosition(const Vec2i &pos);
	void createUnit(const string &unitName, int factionIndex, Vec2i pos);

	void destroyUnit(int unitId);
	void morphToUnit(int unitId,const string &morphName, int ignoreRequirements);
	void moveToUnit(int unitId,int destUnitId);
	void giveAttackStoppedCommand(int unitId, const string &valueName,int ignoreRequirements);
	void playStaticSound(const string &playSound);
	void playStreamingSound(const string &playSound);
	void stopStreamingSound(const string &playSound);
	void stopAllSound();
	void togglePauseGame(int pauseStatus);

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
	int getCellTriggerEventCount(int eventId);
	void unregisterCellTriggerEvent(int eventId);
	int startTimerEvent();
	int resetTimerEvent(int eventId);
	int stopTimerEvent(int eventId);
	int getTimerEventSecondsElapsed(int eventId);
	int getCellTriggeredEventId();
	int getTimerTriggeredEventId();

	bool getAiEnabled(int factionIndex);
	bool getConsumeEnabled(int factionIndex);

	void setPlayerAsWinner(int factionIndex);
	void endGame();

	void startPerformanceTimer();
	void endPerformanceTimer();
	Vec2i getPerformanceTimerResults();

	//wrappers, queries
	Vec2i getStartLocation(int factionIndex);
	Vec2i getUnitPosition(int unitId);
	int getUnitFaction(int unitId);
	int getResourceAmount(const string &resourceName, int factionIndex);
	const string &getLastCreatedUnitName();
	int getLastCreatedUnitId();
	const string &getLastDeadUnitName();
	int getLastDeadUnitId();
	int getUnitCount(int factionIndex);
	int getUnitCountOfType(int factionIndex, const string &typeName);

	bool getGameWon();

	void loadScenario(const string &name, bool keepFactions);

	//callbacks, commands
	static int showMessage(LuaHandle* luaHandle);
	static int setDisplayText(LuaHandle* luaHandle);
	static int addConsoleText(LuaHandle* luaHandle);
	static int addConsoleLangText(LuaHandle* luaHandle);
	static int DisplayFormattedText(LuaHandle* luaHandle);
	static int DisplayFormattedLangText(LuaHandle* luaHandle);
	static int clearDisplayText(LuaHandle* luaHandle);
	static int setCameraPosition(LuaHandle* luaHandle);
	static int createUnit(LuaHandle* luaHandle);

	static int destroyUnit(LuaHandle* luaHandle);
	static int morphToUnit(LuaHandle* luaHandle);
	static int moveToUnit(LuaHandle* luaHandle);
	static int giveAttackStoppedCommand(LuaHandle* luaHandle);
	static int playStaticSound(LuaHandle* luaHandle);
	static int playStreamingSound(LuaHandle* luaHandle);
	static int stopStreamingSound(LuaHandle* luaHandle);
	static int stopAllSound(LuaHandle* luaHandle);
	static int togglePauseGame(LuaHandle* luaHandle);

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
	static int getCellTriggerEventCount(LuaHandle* luaHandle);
	static int unregisterCellTriggerEvent(LuaHandle* luaHandle);
	static int startTimerEvent(LuaHandle* luaHandle);
	static int resetTimerEvent(LuaHandle* luaHandle);
	static int stopTimerEvent(LuaHandle* luaHandle);
	static int getTimerEventSecondsElapsed(LuaHandle* luaHandle);

	static int getCellTriggeredEventId(LuaHandle* luaHandle);
	static int getTimerTriggeredEventId(LuaHandle* luaHandle);

	static int setPlayerAsWinner(LuaHandle* luaHandle);
	static int endGame(LuaHandle* luaHandle);

	static int startPerformanceTimer(LuaHandle* luaHandle);
	static int endPerformanceTimer(LuaHandle* luaHandle);
	static int getPerformanceTimerResults(LuaHandle* luaHandle);

	//callbacks, queries
	static int getStartLocation(LuaHandle* luaHandle);
	static int getUnitPosition(LuaHandle* luaHandle);
	static int getUnitFaction(LuaHandle* luaHandle);
	static int getResourceAmount(LuaHandle* luaHandle);
	static int getLastCreatedUnitName(LuaHandle* luaHandle);
	static int getLastCreatedUnitId(LuaHandle* luaHandle);
	static int getLastDeadUnitName(LuaHandle* luaHandle);
	static int getLastDeadUnitId(LuaHandle* luaHandle);
	static int getUnitCount(LuaHandle* luaHandle);
	static int getUnitCountOfType(LuaHandle* luaHandle);

	static int getGameWon(LuaHandle* luaHandle);

	static int loadScenario(LuaHandle* luaHandle);
};

}}//end namespace

#endif
