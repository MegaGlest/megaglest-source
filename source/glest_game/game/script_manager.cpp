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

#include "script_manager.h"

#include "world.h"
#include "lang.h"
#include "game_camera.h"
#include "game.h"
#include "config.h"

#include "leak_dumper.h"

using namespace Shared::Platform;
using namespace Shared::Lua;
using namespace Shared::Util;

namespace Glest{ namespace Game{

//
// This class wraps streflop for the Lua ScriptMAnager. We need to toggle the data type
// for streflop to use when calling into glest from LUA as streflop may corrupt some
// numeric values passed from Lua otherwise
//
class ScriptManager_STREFLOP_Wrapper {
public:
	ScriptManager_STREFLOP_Wrapper() {
#ifdef USE_STREFLOP
	//streflop_init<streflop::Simple>();
#endif
	}
	~ScriptManager_STREFLOP_Wrapper() {
#ifdef USE_STREFLOP
	//streflop_init<streflop::Double>();
#endif
	}
};

ScriptManagerMessage::ScriptManagerMessage() {
	this->text= "";
	this->header= "";
	this->factionIndex=-1;
	this->teamIndex=-1;
}

ScriptManagerMessage::ScriptManagerMessage(string text, string header,
		int factionIndex,int teamIndex) {
	this->text			= text;
	this->header		= header;
	this->factionIndex = factionIndex;
	this->teamIndex    = teamIndex;
}

void ScriptManagerMessage::saveGame(XmlNode *rootNode) {
	std::map<string,string> mapTagReplacements;
	XmlNode *scriptManagerMessageNode = rootNode->addChild("ScriptManagerMessage");

	//string text;
	scriptManagerMessageNode->addAttribute("text",text, mapTagReplacements);
	//string header;
	scriptManagerMessageNode->addAttribute("header",header, mapTagReplacements);
	scriptManagerMessageNode->addAttribute("factionIndex",intToStr(factionIndex), mapTagReplacements);
	scriptManagerMessageNode->addAttribute("teamIndex",intToStr(teamIndex), mapTagReplacements);
}

void ScriptManagerMessage::loadGame(const XmlNode *rootNode) {
	const XmlNode *scriptManagerMessageNode = rootNode;

	text = scriptManagerMessageNode->getAttribute("text")->getValue();
	header = scriptManagerMessageNode->getAttribute("header")->getValue();
	factionIndex = scriptManagerMessageNode->getAttribute("factionIndex")->getIntValue();
	teamIndex = scriptManagerMessageNode->getAttribute("teamIndex")->getIntValue();
}

// =====================================================
//	class PlayerModifiers
// =====================================================

PlayerModifiers::PlayerModifiers(){
	winner			= false;
	aiEnabled		= true;
	consumeEnabled 	= true;
}

void PlayerModifiers::saveGame(XmlNode *rootNode) {
	std::map<string,string> mapTagReplacements;
	XmlNode *playerModifiersNode = rootNode->addChild("PlayerModifiers");

	//bool winner;
	playerModifiersNode->addAttribute("winner",intToStr(winner), mapTagReplacements);
	//bool aiEnabled;
	playerModifiersNode->addAttribute("aiEnabled",intToStr(aiEnabled), mapTagReplacements);
	//bool consumeEnabled;
	playerModifiersNode->addAttribute("consumeEnabled",intToStr(consumeEnabled), mapTagReplacements);
}

void PlayerModifiers::loadGame(const XmlNode *rootNode) {
	const XmlNode *playerModifiersNode = rootNode;

	winner = playerModifiersNode->getAttribute("winner")->getIntValue() != 0;
	aiEnabled = playerModifiersNode->getAttribute("aiEnabled")->getIntValue() != 0;
	consumeEnabled = playerModifiersNode->getAttribute("consumeEnabled")->getIntValue() != 0;
}

CellTriggerEvent::CellTriggerEvent() {
	type = ctet_Unit;
	sourceId = 0;
	destId = 0;
	//Vec2i destPos;

	triggerCount = 0;
}

void CellTriggerEvent::saveGame(XmlNode *rootNode) {
	std::map<string,string> mapTagReplacements;
	XmlNode *cellTriggerEventNode = rootNode->addChild("CellTriggerEvent");

//	CellTriggerEventType type;
	cellTriggerEventNode->addAttribute("type",intToStr(type), mapTagReplacements);
//	int sourceId;
	cellTriggerEventNode->addAttribute("sourceId",intToStr(sourceId), mapTagReplacements);
//	int destId;
	cellTriggerEventNode->addAttribute("destId",intToStr(destId), mapTagReplacements);
//	Vec2i destPos;
	cellTriggerEventNode->addAttribute("destPos",destPos.getString(), mapTagReplacements);
//	int triggerCount;
	cellTriggerEventNode->addAttribute("triggerCount",intToStr(triggerCount), mapTagReplacements);
}

void CellTriggerEvent::loadGame(const XmlNode *rootNode) {
	const XmlNode *cellTriggerEventNode = rootNode->getChild("CellTriggerEvent");

	type = static_cast<CellTriggerEventType>(cellTriggerEventNode->getAttribute("type")->getIntValue());
	sourceId = cellTriggerEventNode->getAttribute("sourceId")->getIntValue();
	destId = cellTriggerEventNode->getAttribute("destId")->getIntValue();
	destPos = Vec2i::strToVec2(cellTriggerEventNode->getAttribute("destPos")->getValue());
	triggerCount = cellTriggerEventNode->getAttribute("triggerCount")->getIntValue();
}

TimerTriggerEvent::TimerTriggerEvent() {
	running = false;
	startFrame = 0;
	endFrame = 0;
	triggerSecondsElapsed = 0;
}

void TimerTriggerEvent::saveGame(XmlNode *rootNode) {
	std::map<string,string> mapTagReplacements;
	XmlNode *timerTriggerEventNode = rootNode->addChild("TimerTriggerEvent");

//	bool running;
	timerTriggerEventNode->addAttribute("running",intToStr(running), mapTagReplacements);
//	//time_t startTime;
//	//time_t endTime;
//	int startFrame;
	timerTriggerEventNode->addAttribute("startFrame",intToStr(startFrame), mapTagReplacements);
//	int endFrame;
	timerTriggerEventNode->addAttribute("endFrame",intToStr(endFrame), mapTagReplacements);

	if(triggerSecondsElapsed > 0) {
		timerTriggerEventNode->addAttribute("triggerSecondsElapsed",intToStr(triggerSecondsElapsed), mapTagReplacements);
	}
}

void TimerTriggerEvent::loadGame(const XmlNode *rootNode) {
	const XmlNode *timerTriggerEventNode = rootNode->getChild("TimerTriggerEvent");

	running = timerTriggerEventNode->getAttribute("running")->getIntValue() != 0;
	startFrame = timerTriggerEventNode->getAttribute("startFrame")->getIntValue();
	endFrame = timerTriggerEventNode->getAttribute("endFrame")->getIntValue();
	if(timerTriggerEventNode->hasAttribute("triggerSecondsElapsed") == true) {
		triggerSecondsElapsed = timerTriggerEventNode->getAttribute("triggerSecondsElapsed")->getIntValue();
	}
}

// =====================================================
//	class ScriptManager
// =====================================================
ScriptManager* ScriptManager::thisScriptManager= NULL;
const int ScriptManager::messageWrapCount= 30;
const int ScriptManager::displayTextWrapCount= 64;

ScriptManager::ScriptManager() {
	world = NULL;
	gameCamera = NULL;
	lastCreatedUnitId = -1;
	lastDeadUnitId = 0;
	lastDeadUnitCauseOfDeath = 0;
	lastDeadUnitKillerId = 0;
	lastAttackedUnitId = 0;
	lastAttackingUnitId = 0;
	gameOver = false;
	gameWon = false;
	currentTimerTriggeredEventId = 0;
	currentCellTriggeredEventId = 0;
	currentEventId = 0;
	inCellTriggerEvent = false;
	rootNode = NULL;
}

ScriptManager::~ScriptManager() {

}

void ScriptManager::init(World* world, GameCamera *gameCamera, const XmlNode *rootNode) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//printf("In [%s::%s Line: %d] rootNode [%p][%s]\n",__FILE__,__FUNCTION__,__LINE__,rootNode,(rootNode != NULL ? rootNode->getName().c_str() : "none"));
	this->rootNode = rootNode;
	const Scenario*	scenario= world->getScenario();

	this->world= world;
	this->gameCamera= gameCamera;

	//set static instance
	thisScriptManager= this;

	//printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	currentEventId = 1;
	CellTriggerEventList.clear();
	TimerTriggerEventList.clear();

	//printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//register functions
	luaScript.registerFunction(networkShowMessageForFaction, "networkShowMessageForFaction");
	luaScript.registerFunction(networkShowMessageForTeam, "networkShowMessageForTeam");

	luaScript.registerFunction(showMessage, "showMessage");
	luaScript.registerFunction(setDisplayText, "setDisplayText");
	luaScript.registerFunction(addConsoleText, "addConsoleText");
	luaScript.registerFunction(addConsoleLangText, "addConsoleLangText");
	luaScript.registerFunction(DisplayFormattedText, "DisplayFormattedText");
	luaScript.registerFunction(DisplayFormattedText, "displayFormattedText");
	luaScript.registerFunction(DisplayFormattedLangText, "DisplayFormattedLangText");
	luaScript.registerFunction(DisplayFormattedLangText, "displayFormattedLangText");
	luaScript.registerFunction(clearDisplayText, "clearDisplayText");
	luaScript.registerFunction(setCameraPosition, "setCameraPosition");
	luaScript.registerFunction(createUnit, "createUnit");
	luaScript.registerFunction(createUnitNoSpacing, "createUnitNoSpacing");
	luaScript.registerFunction(destroyUnit, "destroyUnit");
	luaScript.registerFunction(giveKills, "giveKills");
	luaScript.registerFunction(morphToUnit, "morphToUnit");
	luaScript.registerFunction(moveToUnit, "moveToUnit");
	luaScript.registerFunction(playStaticSound, "playStaticSound");
	luaScript.registerFunction(playStreamingSound, "playStreamingSound");
	luaScript.registerFunction(stopStreamingSound, "stopStreamingSound");
	luaScript.registerFunction(stopAllSound, "stopAllSound");
	luaScript.registerFunction(togglePauseGame, "togglePauseGame");
	luaScript.registerFunction(giveResource, "giveResource");
	luaScript.registerFunction(givePositionCommand, "givePositionCommand");
	luaScript.registerFunction(giveProductionCommand, "giveProductionCommand");
	luaScript.registerFunction(giveAttackCommand, "giveAttackCommand");
	luaScript.registerFunction(giveUpgradeCommand, "giveUpgradeCommand");
	luaScript.registerFunction(giveAttackStoppedCommand, "giveAttackStoppedCommand");
	luaScript.registerFunction(disableAi, "disableAi");
	luaScript.registerFunction(enableAi, "enableAi");
	luaScript.registerFunction(getAiEnabled, "getAiEnabled");
	luaScript.registerFunction(disableConsume, "disableConsume");
	luaScript.registerFunction(enableConsume, "enableConsume");
	luaScript.registerFunction(getConsumeEnabled, "getConsumeEnabled");
	luaScript.registerFunction(setPlayerAsWinner, "setPlayerAsWinner");
	luaScript.registerFunction(endGame, "endGame");

	luaScript.registerFunction(startPerformanceTimer, "startPerformanceTimer");
	luaScript.registerFunction(endPerformanceTimer, "endPerformanceTimer");
	luaScript.registerFunction(getPerformanceTimerResults, "getPerformanceTimerResults");

	luaScript.registerFunction(registerCellTriggerEventForUnitToUnit, "registerCellTriggerEventForUnitToUnit");
	luaScript.registerFunction(registerCellTriggerEventForUnitToLocation, "registerCellTriggerEventForUnitToLocation");
	luaScript.registerFunction(registerCellTriggerEventForFactionToUnit, "registerCellTriggerEventForFactionToUnit");
	luaScript.registerFunction(registerCellTriggerEventForFactionToLocation, "registerCellTriggerEventForFactionToLocation");
	luaScript.registerFunction(getCellTriggerEventCount, "getCellTriggerEventCount");
	luaScript.registerFunction(unregisterCellTriggerEvent, "unregisterCellTriggerEvent");
	luaScript.registerFunction(startTimerEvent, "startTimerEvent");
	luaScript.registerFunction(startEfficientTimerEvent, "startEfficientTimerEvent");
	luaScript.registerFunction(resetTimerEvent, "resetTimerEvent");
	luaScript.registerFunction(stopTimerEvent, "stopTimerEvent");
	luaScript.registerFunction(getTimerEventSecondsElapsed, "timerEventSecondsElapsed");
	luaScript.registerFunction(getCellTriggeredEventId, "triggeredCellEventId");
	luaScript.registerFunction(getTimerTriggeredEventId, "triggeredTimerEventId");

	luaScript.registerFunction(setRandomGenInit, "setRandomGenInit");
	luaScript.registerFunction(getRandomGen, "getRandomGen");
	luaScript.registerFunction(getWorldFrameCount, "getWorldFrameCount");

	luaScript.registerFunction(getStartLocation, "startLocation");
	luaScript.registerFunction(getIsUnitAlive, "isUnitAlive");
	luaScript.registerFunction(getUnitPosition, "unitPosition");
	luaScript.registerFunction(setUnitPosition, "setUnitPosition");

	luaScript.registerFunction(getUnitFaction, "unitFaction");
	luaScript.registerFunction(getResourceAmount, "resourceAmount");

	luaScript.registerFunction(getLastCreatedUnitName, "lastCreatedUnitName");
	luaScript.registerFunction(getLastCreatedUnitId, "lastCreatedUnit");

	luaScript.registerFunction(getLastDeadUnitName, "lastDeadUnitName");
	luaScript.registerFunction(getLastDeadUnitId, "lastDeadUnit");
	luaScript.registerFunction(getLastDeadUnitCauseOfDeath, "lastDeadUnitCauseOfDeath");
	luaScript.registerFunction(getLastDeadUnitKillerName, "lastDeadUnitKillerName");
	luaScript.registerFunction(getLastDeadUnitKillerId, "lastDeadUnitKiller");

	luaScript.registerFunction(getLastAttackedUnitName, "lastAttackedUnitName");
	luaScript.registerFunction(getLastAttackedUnitId, "lastAttackedUnit");

	luaScript.registerFunction(getLastAttackingUnitName, "lastAttackingUnitName");
	luaScript.registerFunction(getLastAttackingUnitId, "lastAttackingUnit");

	luaScript.registerFunction(getUnitCount, "unitCount");
	luaScript.registerFunction(getUnitCountOfType, "unitCountOfType");

	luaScript.registerFunction(getIsGameOver, "isGameOver");
	luaScript.registerFunction(getGameWon, "gameWon");

	luaScript.registerFunction(getSystemMacroValue, "getSystemMacroValue");
	luaScript.registerFunction(scenarioDir, "scenarioDir");
	luaScript.registerFunction(getPlayerName, "getPlayerName");
	luaScript.registerFunction(getPlayerName, "playerName");

	luaScript.registerFunction(loadScenario, "loadScenario");

	luaScript.registerFunction(getUnitsForFaction, "getUnitsForFaction");
	luaScript.registerFunction(getUnitCurrentField, "getUnitCurrentField");

	//load code
	for(int i= 0; i<scenario->getScriptCount(); ++i){
		const Script* script= scenario->getScript(i);
		luaScript.loadCode("function " + script->getName() + "()" + script->getCode() + "end\n", script->getName());
	}

	//setup message box
	messageBox.init( Lang::getInstance().get("Ok") );
	messageBox.setEnabled(false);

	//last created unit
	lastCreatedUnitId= -1;
	lastDeadUnitName = "";
	lastDeadUnitId= -1;
	lastDeadUnitCauseOfDeath = ucodNone;
	lastDeadUnitKillerName = "";
	lastDeadUnitKillerId= -1;

	lastAttackedUnitName = "";
	lastAttackedUnitId = -1;
	lastAttackingUnitName = "";
	lastAttackingUnitId = -1;

	gameOver= false;
	gameWon = false;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	// Setup global functions and vars here
	luaScript.beginCall("global");
	luaScript.endCall();

	//call startup function
	if(this->rootNode == NULL) {
		luaScript.beginCall("startup");
		luaScript.endCall();
	}
	else {
		loadGame(this->rootNode);
		this->rootNode = NULL;
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

// ========================== events ===============================================

void ScriptManager::onMessageBoxOk(bool popFront) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	Lang &lang= Lang::getInstance();

	for(int i = 0;messageQueue.empty() == false;++i) {
		//printf("i = %d messageQueue.size() = %d popFront = %d\n",i,messageQueue.size(),popFront);
		if(popFront == true) {
			messageQueue.pop_front();
		}
		if(messageQueue.empty() == false) {
//			printf("onMessageBoxOk [%s] factionIndex = %d [%d] teamIndex = %d [%d][%d]\n",
//					wrapString(lang.getScenarioString(messageQueue.front().getText()), messageWrapCount).c_str(),
//					messageQueue.front().getFactionIndex(), this->world->getThisFactionIndex(),
//					messageQueue.front().getTeamIndex(),this->world->getThisTeamIndex(),this->world->getThisFaction()->getTeam());

			if((messageQueue.front().getFactionIndex() < 0 && messageQueue.front().getTeamIndex() < 0) ||
				messageQueue.front().getFactionIndex() == this->world->getThisFactionIndex() ||
				messageQueue.front().getTeamIndex() == this->world->getThisTeamIndex()) {
				messageBox.setEnabled(true);
				messageBox.setText(wrapString(lang.getScenarioString(messageQueue.front().getText()), messageWrapCount));
				messageBox.setHeader(lang.getScenarioString(messageQueue.front().getHeader()));
				break;
			}
			popFront = true;
		}
	}
}

void ScriptManager::onResourceHarvested(){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(this->rootNode == NULL) {
		luaScript.beginCall("resourceHarvested");
		luaScript.endCall();
	}
}

void ScriptManager::onUnitCreated(const Unit* unit){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(this->rootNode == NULL) {
		lastCreatedUnitName= unit->getType()->getName();
		lastCreatedUnitId= unit->getId();
		luaScript.beginCall("unitCreated");
		luaScript.endCall();
		luaScript.beginCall("unitCreatedOfType_"+unit->getType()->getName());
		luaScript.endCall();
	}
}

void ScriptManager::onUnitDied(const Unit* unit){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(this->rootNode == NULL) {
		if(unit->getLastAttackerUnitId() >= 0) {
			Unit *killer = world->findUnitById(unit->getLastAttackerUnitId());

			if(killer != NULL) {
				lastAttackingUnitName= killer->getType()->getName();
				lastAttackingUnitId= killer->getId();

				lastDeadUnitKillerName= killer->getType()->getName();
				lastDeadUnitKillerId= killer->getId();
			}
			else {
				lastDeadUnitKillerName= "";
				lastDeadUnitKillerId= -1;
			}
		}

		lastAttackedUnitName= unit->getType()->getName();
		lastAttackedUnitId= unit->getId();

		lastDeadUnitName= unit->getType()->getName();
		lastDeadUnitId= unit->getId();
		lastDeadUnitCauseOfDeath = unit->getCauseOfDeath();

		luaScript.beginCall("unitDied");
		luaScript.endCall();
	}
}

void ScriptManager::onUnitAttacked(const Unit* unit) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(this->rootNode == NULL) {
		lastAttackedUnitName= unit->getType()->getName();
		lastAttackedUnitId= unit->getId();
		luaScript.beginCall("unitAttacked");
		luaScript.endCall();
	}
}

void ScriptManager::onUnitAttacking(const Unit* unit) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(this->rootNode == NULL) {
		lastAttackingUnitName= unit->getType()->getName();
		lastAttackingUnitId= unit->getId();
		luaScript.beginCall("unitAttacking");
		luaScript.endCall();
	}
}

void ScriptManager::onGameOver(bool won) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	gameWon = won;
	luaScript.beginCall("gameOver");
	luaScript.endCall();
}

void ScriptManager::onTimerTriggerEvent() {
	if(TimerTriggerEventList.empty() == true) {
		return;
	}
	if(this->rootNode != NULL) {
		return;
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] TimerTriggerEventList.size() = %d\n",__FILE__,__FUNCTION__,__LINE__,TimerTriggerEventList.size());

	for(std::map<int,TimerTriggerEvent>::iterator iterMap = TimerTriggerEventList.begin();
		iterMap != TimerTriggerEventList.end(); ++iterMap) {

		TimerTriggerEvent &event = iterMap->second;

		if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] event.running = %d, event.startTime = %lld, event.endTime = %lld, diff = %f\n",
													__FILE__,__FUNCTION__,__LINE__,event.running,(long long int)event.startFrame,(long long int)event.endFrame,(event.endFrame - event.startFrame));

		if(event.running == true) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			// If using an efficient timer, check if its time to trigger
			// on the elapsed check
			if(event.triggerSecondsElapsed > 0) {
				int elapsed = (world->getFrameCount()-event.startFrame) / GameConstants::updateFps;
				if(elapsed < event.triggerSecondsElapsed) {
					continue;
				}
			}
			currentTimerTriggeredEventId = iterMap->first;
			luaScript.beginCall("timerTriggerEvent");
			luaScript.endCall();

			if(event.triggerSecondsElapsed > 0) {
				int timerId = iterMap->first;
				stopTimerEvent(timerId);
			}
		}
	}
}

void ScriptManager::onCellTriggerEvent(Unit *movingUnit) {
	if(CellTriggerEventList.empty() == true) {
		return;
	}
	if(this->rootNode != NULL) {
		return;
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] movingUnit = %p, CellTriggerEventList.size() = %d\n",__FILE__,__FUNCTION__,__LINE__,movingUnit,CellTriggerEventList.size());

	// remove any delayed removals
	unregisterCellTriggerEvent(-1);

	inCellTriggerEvent = true;
	if(movingUnit != NULL) {
		//ScenarioInfo scenarioInfoStart = world->getScenario()->getInfo();

		for(std::map<int,CellTriggerEvent>::iterator iterMap = CellTriggerEventList.begin();
				iterMap != CellTriggerEventList.end(); ++iterMap) {
			CellTriggerEvent &event = iterMap->second;

			if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] movingUnit = %d, event.type = %d, movingUnit->getPos() = %s, event.sourceId = %d, event.destId = %d, event.destPos = %s\n",
														__FILE__,__FUNCTION__,__LINE__,movingUnit->getId(),event.type,movingUnit->getPos().getString().c_str(), event.sourceId,event.destId,event.destPos.getString().c_str());

			bool triggerEvent = false;

			switch(event.type) {
			case ctet_Unit:
			{
				Unit *destUnit = world->findUnitById(event.destId);
				if(destUnit != NULL) {
					if(movingUnit->getId() == event.sourceId) {
						bool srcInDst = world->getMap()->isInUnitTypeCells(destUnit->getType(), destUnit->getPos(),movingUnit->getPos());
						if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] movingUnit = %d, event.type = %d, movingUnit->getPos() = %s, event.sourceId = %d, event.destId = %d, event.destPos = %s, destUnit->getPos() = %s, srcInDst = %d\n",
													__FILE__,__FUNCTION__,__LINE__,movingUnit->getId(), event.type,movingUnit->getPos().getString().c_str(),event.sourceId,event.destId, event.destPos.getString().c_str(), destUnit->getPos().getString().c_str(),srcInDst);

						if(srcInDst == true) {
							if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
						}
						else {
							srcInDst = world->getMap()->isNextToUnitTypeCells(destUnit->getType(), destUnit->getPos(),movingUnit->getPos());
							if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] movingUnit = %d, event.type = %d, movingUnit->getPos() = %s, event.sourceId = %d, event.destId = %d, event.destPos = %s, destUnit->getPos() = %s, srcInDst = %d\n",
														__FILE__,__FUNCTION__,__LINE__,movingUnit->getId(), event.type,movingUnit->getPos().getString().c_str(),event.sourceId,event.destId, event.destPos.getString().c_str(), destUnit->getPos().getString().c_str(),srcInDst);
						}
						triggerEvent = srcInDst;
				   }
				}
			}
			break;
			case ctet_UnitPos:
			{
				if(movingUnit->getId() == event.sourceId) {
					bool srcInDst = world->getMap()->isInUnitTypeCells(movingUnit->getType(), event.destPos,movingUnit->getPos());
					if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] movingUnit = %d, event.type = %d, movingUnit->getPos() = %s, event.sourceId = %d, event.destId = %d, event.destPos = %s, srcInDst = %d\n",
														__FILE__,__FUNCTION__,__LINE__,movingUnit->getId(),event.type,movingUnit->getPos().getString().c_str(),event.sourceId,event.destId,event.destPos.getString().c_str(),srcInDst);

					if(srcInDst == true) {
						if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
					}
					triggerEvent = srcInDst;
				}
			}
			break;

			case ctet_Faction:
			{
				Unit *destUnit = world->findUnitById(event.destId);
				if(destUnit != NULL &&
				   movingUnit->getFactionIndex() == event.sourceId) {
					bool srcInDst = world->getMap()->isInUnitTypeCells(destUnit->getType(), destUnit->getPos(),movingUnit->getPos());
					if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] movingUnit = %d, event.type = %d, movingUnit->getPos() = %s, event.sourceId = %d, event.destId = %d, event.destPos = %s, srcInDst = %d\n",
														__FILE__,__FUNCTION__,__LINE__,movingUnit->getId(),event.type,movingUnit->getPos().getString().c_str(),event.sourceId,event.destId,event.destPos.getString().c_str(),srcInDst);

					if(srcInDst == true) {
						if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
					}
					else {
						srcInDst = world->getMap()->isNextToUnitTypeCells(destUnit->getType(), destUnit->getPos(),movingUnit->getPos());
						if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] movingUnit = %d, event.type = %d, movingUnit->getPos() = %s, event.sourceId = %d, event.destId = %d, event.destPos = %s, destUnit->getPos() = %s, srcInDst = %d\n",
														__FILE__,__FUNCTION__,__LINE__,movingUnit->getId(),event.type,movingUnit->getPos().getString().c_str(),event.sourceId,event.destId,event.destPos.getString().c_str(),destUnit->getPos().getString().c_str(),srcInDst);
					}
					triggerEvent = srcInDst;
				}
			}
			break;

			case ctet_FactionPos:
			{
				if(movingUnit->getFactionIndex() == event.sourceId) {
					//printf("ctet_FactionPos event.destPos = [%s], movingUnit->getPos() [%s]\n",event.destPos.getString().c_str(),movingUnit->getPos().getString().c_str());

					bool srcInDst = world->getMap()->isInUnitTypeCells(movingUnit->getType(), event.destPos,movingUnit->getPos());
					if(srcInDst == true) {
						if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
					}
					triggerEvent = srcInDst;
				}
			}
			break;

			}

			if(triggerEvent == true) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

				currentCellTriggeredEventId = iterMap->first;
				event.triggerCount++;

				luaScript.beginCall("cellTriggerEvent");
				luaScript.endCall();
			}

//			ScenarioInfo scenarioInfoEnd = world->getScenario()->getInfo();
//			if(scenarioInfoStart.file != scenarioInfoEnd.file) {
//				break;
//			}
		}
	}

	inCellTriggerEvent = false;
}

// ========================== lua wrappers ===============================================

string ScriptManager::wrapString(const string &str, int wrapCount){
	ScriptManager_STREFLOP_Wrapper streflopWrapper;

	string returnString;

	int letterCount= 0;
	for(int i= 0; i<str.size(); ++i){
		if(letterCount>wrapCount && str[i]==' '){
			returnString+= '\n';
			letterCount= 0;
		}
		else
		{
			returnString+= str[i];
		}
		++letterCount;
	}

	return returnString;
}

void ScriptManager::networkShowMessageForFaction(const string &text, const string &header,int factionIndex) {
	ScriptManager_STREFLOP_Wrapper streflopWrapper;

	messageQueue.push_back(ScriptManagerMessage(text, header, factionIndex));
	onMessageBoxOk(false);
}
void ScriptManager::networkShowMessageForTeam(const string &text, const string &header,int teamIndex) {
	ScriptManager_STREFLOP_Wrapper streflopWrapper;

	// Team indexes are 0 based internally (but 1 based in the lua script) so convert
	teamIndex--;
	messageQueue.push_back(ScriptManagerMessage(text, header, -1, teamIndex));
	onMessageBoxOk(false);
}

void ScriptManager::showMessage(const string &text, const string &header){
	ScriptManager_STREFLOP_Wrapper streflopWrapper;

	messageQueue.push_back(ScriptManagerMessage(text, header));
	onMessageBoxOk(false);
}

void ScriptManager::clearDisplayText(){
	displayText= "";
}

void ScriptManager::setDisplayText(const string &text){
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	displayText= wrapString(Lang::getInstance().getScenarioString(text), displayTextWrapCount);
}

void ScriptManager::addConsoleText(const string &text){
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	world->addConsoleText(text);
}
void ScriptManager::addConsoleLangText(const char *fmt, ...){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	ScriptManager_STREFLOP_Wrapper streflopWrapper;

	
    va_list argList;
    va_start(argList, fmt);

    const int max_debug_buffer_size = 8096;
    char szBuf[max_debug_buffer_size]="";
    vsnprintf(szBuf,max_debug_buffer_size-1,fmt, argList);

	world->addConsoleTextWoLang(szBuf);
	va_end(argList);
	
}

void ScriptManager::DisplayFormattedText(const char *fmt, ...) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	ScriptManager_STREFLOP_Wrapper streflopWrapper;

    va_list argList;
    va_start(argList, fmt);

    const int max_debug_buffer_size = 8096;
    char szBuf[max_debug_buffer_size]="";
    vsnprintf(szBuf,max_debug_buffer_size-1,fmt, argList);

    displayText=szBuf;

	va_end(argList);
}
void ScriptManager::DisplayFormattedLangText(const char *fmt, ...) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	ScriptManager_STREFLOP_Wrapper streflopWrapper;

	
    va_list argList;
    va_start(argList, fmt);

    const int max_debug_buffer_size = 8096;
    char szBuf[max_debug_buffer_size]="";
    vsnprintf(szBuf,max_debug_buffer_size-1,fmt, argList);

    displayText=szBuf;

	va_end(argList);
}
void ScriptManager::setCameraPosition(const Vec2i &pos){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	gameCamera->centerXZ(pos.x, pos.y);
}

void ScriptManager::createUnit(const string &unitName, int factionIndex, Vec2i pos){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] unit [%s] factionIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,unitName.c_str(),factionIndex);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	world->createUnit(unitName, factionIndex, pos);
}

void ScriptManager::createUnitNoSpacing(const string &unitName, int factionIndex, Vec2i pos){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] unit [%s] factionIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,unitName.c_str(),factionIndex);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	world->createUnit(unitName, factionIndex, pos, false);
}

void ScriptManager::destroyUnit(int unitId){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] unit [%d]\n",__FILE__,__FUNCTION__,__LINE__,unitId);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	Unit *unit = world->findUnitById(unitId);
	if(unit != NULL) {
		// Make sure they die
		unit->decHp(unit->getHp() * unit->getHp());
		unit->kill();
		// If called from an existing die event we get a stack overflow
		//onUnitDied(unit);
	}
}
void ScriptManager::giveKills (int unitId, int amount){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] unit [%d]\n",__FILE__,__FUNCTION__,__LINE__,unitId);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	Unit *unit = world->findUnitById(unitId);
	if(unit != NULL) {
		for(int i = 1; i <= amount; i++) {
		unit->incKills(-1);
		}
	}
}

void ScriptManager::playStaticSound(const string &playSound) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] playSound [%s]\n",__FILE__,__FUNCTION__,__LINE__,playSound.c_str());
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	world->playStaticSound(playSound);
}
void ScriptManager::playStreamingSound(const string &playSound) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] playSound [%s]\n",__FILE__,__FUNCTION__,__LINE__,playSound.c_str());
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	world->playStreamingSound(playSound);
}

void ScriptManager::stopStreamingSound(const string &playSound) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] playSound [%s]\n",__FILE__,__FUNCTION__,__LINE__,playSound.c_str());
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	world->stopStreamingSound(playSound);
}

void ScriptManager::stopAllSound() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	world->stopAllSound();
}

void ScriptManager::togglePauseGame(int pauseStatus) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] pauseStatus = %d\n",__FILE__,__FUNCTION__,__LINE__,pauseStatus);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	world->togglePauseGame((pauseStatus != 0),true);
}

void ScriptManager::morphToUnit(int unitId,const string &morphName, int ignoreRequirements) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] unit [%d] morphName [%s] forceUpgradesIfRequired = %d\n",__FILE__,__FUNCTION__,__LINE__,unitId,morphName.c_str(),ignoreRequirements);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	world->morphToUnit(unitId,morphName,(ignoreRequirements == 1));
}

void ScriptManager::moveToUnit(int unitId,int destUnitId) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] unit [%d] destUnitId [%d]\n",__FILE__,__FUNCTION__,__LINE__,unitId,destUnitId);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	world->moveToUnit(unitId,destUnitId);
}

void ScriptManager::giveResource(const string &resourceName, int factionIndex, int amount){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	world->giveResource(resourceName, factionIndex, amount);
}

void ScriptManager::givePositionCommand(int unitId, const string &commandName, const Vec2i &pos){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	world->givePositionCommand(unitId, commandName, pos);
}

void ScriptManager::giveAttackCommand(int unitId, int unitToAttackId) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	world->giveAttackCommand(unitId, unitToAttackId);
}

void ScriptManager::giveProductionCommand(int unitId, const string &producedName){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	world->giveProductionCommand(unitId, producedName);
}

void ScriptManager::giveUpgradeCommand(int unitId, const string &producedName){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	world->giveUpgradeCommand(unitId, producedName);
}

void ScriptManager::giveAttackStoppedCommand(int unitId, const string &itemName,int ignoreRequirements) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	world->giveAttackStoppedCommand(unitId, itemName, (ignoreRequirements == 1));
}

void ScriptManager::disableAi(int factionIndex){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	if(factionIndex<GameConstants::maxPlayers){
		playerModifiers[factionIndex].disableAi();
		disableConsume(factionIndex); // by this we stay somehow compatible with old behaviour
	}
}

void ScriptManager::enableAi(int factionIndex){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	if(factionIndex<GameConstants::maxPlayers){
		playerModifiers[factionIndex].enableAi();
	}
}

bool ScriptManager::getAiEnabled(int factionIndex){
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	if(factionIndex<GameConstants::maxPlayers){
		return playerModifiers[factionIndex].getAiEnabled();
	}
	return false;
}

void ScriptManager::disableConsume(int factionIndex){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	if(factionIndex<GameConstants::maxPlayers){
		playerModifiers[factionIndex].disableConsume();
	}
}

void ScriptManager::enableConsume(int factionIndex){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	if(factionIndex<GameConstants::maxPlayers){
		playerModifiers[factionIndex].enableConsume();
	}
}

bool ScriptManager::getConsumeEnabled(int factionIndex){
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	if(factionIndex < GameConstants::maxPlayers){
		return playerModifiers[factionIndex].getConsumeEnabled();
	}
	return false;
}

int ScriptManager::registerCellTriggerEventForUnitToUnit(int sourceUnitId, int destUnitId) {
	CellTriggerEvent trigger;
	trigger.type = ctet_Unit;
	trigger.sourceId = sourceUnitId;
	trigger.destId = destUnitId;

	int eventId = currentEventId++;
	CellTriggerEventList[eventId] = trigger;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] Unit: %d will trigger cell event when reaching unit: %d, eventId = %d\n",__FILE__,__FUNCTION__,__LINE__,sourceUnitId,destUnitId,eventId);

	return eventId;
}

int ScriptManager::registerCellTriggerEventForUnitToLocation(int sourceUnitId, const Vec2i &pos) {
	CellTriggerEvent trigger;
	trigger.type = ctet_UnitPos;
	trigger.sourceId = sourceUnitId;
	trigger.destPos = pos;

	int eventId = currentEventId++;
	CellTriggerEventList[eventId] = trigger;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] Unit: %d will trigger cell event when reaching pos: %s, eventId = %d\n",__FILE__,__FUNCTION__,__LINE__,sourceUnitId,pos.getString().c_str(),eventId);

	return eventId;
}

int ScriptManager::registerCellTriggerEventForFactionToUnit(int sourceFactionId, int destUnitId) {
	CellTriggerEvent trigger;
	trigger.type = ctet_Faction;
	trigger.sourceId = sourceFactionId;
	trigger.destId = destUnitId;

	int eventId = currentEventId++;
	CellTriggerEventList[eventId] = trigger;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] Faction: %d will trigger cell event when reaching unit: %d, eventId = %d\n",__FILE__,__FUNCTION__,__LINE__,sourceFactionId,destUnitId,eventId);

	return eventId;
}

int ScriptManager::registerCellTriggerEventForFactionToLocation(int sourceFactionId, const Vec2i &pos) {
	CellTriggerEvent trigger;
	trigger.type = ctet_FactionPos;
	trigger.sourceId = sourceFactionId;
	trigger.destPos = pos;

	int eventId = currentEventId++;
	CellTriggerEventList[eventId] = trigger;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]Faction: %d will trigger cell event when reaching pos: %s, eventId = %d\n",__FILE__,__FUNCTION__,__LINE__,sourceFactionId,pos.getString().c_str(),eventId);

	return eventId;
}

int ScriptManager::getCellTriggerEventCount(int eventId) {
	int result = 0;
	if(CellTriggerEventList.find(eventId) != CellTriggerEventList.end()) {
		result = CellTriggerEventList[eventId].triggerCount;
	}

	return result;
}

void ScriptManager::unregisterCellTriggerEvent(int eventId) {
	if(CellTriggerEventList.find(eventId) != CellTriggerEventList.end()) {
		if(inCellTriggerEvent == false) {
			CellTriggerEventList.erase(eventId);
		}
		else {
			unRegisterCellTriggerEventList.push_back(eventId);
		}
	}

	if(inCellTriggerEvent == false) {
		if(unRegisterCellTriggerEventList.empty() == false) {
			for(int i = 0; i < unRegisterCellTriggerEventList.size(); ++i) {
				int delayedEventId = unRegisterCellTriggerEventList[i];
				if(CellTriggerEventList.find(delayedEventId) != CellTriggerEventList.end()) {
					CellTriggerEventList.erase(delayedEventId);
				}
			}
			unRegisterCellTriggerEventList.clear();
		}
	}
}

int ScriptManager::startTimerEvent() {
	TimerTriggerEvent trigger;
	trigger.running = true;
	//trigger.startTime = time(NULL);
	trigger.startFrame = world->getFrameCount();
	//trigger.endTime = 0;
	trigger.endFrame = 0;

	int eventId = currentEventId++;
	TimerTriggerEventList[eventId] = trigger;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] TimerTriggerEventList.size() = %d, eventId = %d, trigger.startTime = %lld, trigger.endTime = %lld\n",__FILE__,__FUNCTION__,__LINE__,TimerTriggerEventList.size(),eventId,(long long int)trigger.startFrame,(long long int)trigger.endFrame);

	return eventId;
}

int ScriptManager::startEfficientTimerEvent(int triggerSecondsElapsed) {
	TimerTriggerEvent trigger;
	trigger.running = true;
	//trigger.startTime = time(NULL);
	trigger.startFrame = world->getFrameCount();
	//trigger.endTime = 0;
	trigger.endFrame = 0;
	trigger.triggerSecondsElapsed = triggerSecondsElapsed;

	int eventId = currentEventId++;
	TimerTriggerEventList[eventId] = trigger;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] TimerTriggerEventList.size() = %d, eventId = %d, trigger.startTime = %lld, trigger.endTime = %lld\n",__FILE__,__FUNCTION__,__LINE__,TimerTriggerEventList.size(),eventId,(long long int)trigger.startFrame,(long long int)trigger.endFrame);

	return eventId;
}

int ScriptManager::resetTimerEvent(int eventId) {
	int result = 0;
	if(TimerTriggerEventList.find(eventId) != TimerTriggerEventList.end()) {
		TimerTriggerEvent &trigger = TimerTriggerEventList[eventId];
		result = getTimerEventSecondsElapsed(eventId);

		//trigger.startTime = time(NULL);
		trigger.startFrame = world->getFrameCount();
		//trigger.endTime = 0;
		trigger.endFrame = 0;
		trigger.running = true;

		if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] TimerTriggerEventList.size() = %d, eventId = %d, trigger.startTime = %lld, trigger.endTime = %lld, result = %d\n",__FILE__,__FUNCTION__,__LINE__,TimerTriggerEventList.size(),eventId,(long long int)trigger.startFrame,(long long int)trigger.endFrame,result);
	}
	return result;
}

int ScriptManager::stopTimerEvent(int eventId) {
	int result = 0;
	if(TimerTriggerEventList.find(eventId) != TimerTriggerEventList.end()) {
		TimerTriggerEvent &trigger = TimerTriggerEventList[eventId];
		//trigger.endTime = time(NULL);
		trigger.endFrame = world->getFrameCount();
		trigger.running = false;
		result = getTimerEventSecondsElapsed(eventId);

		if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] TimerTriggerEventList.size() = %d, eventId = %d, trigger.startTime = %lld, trigger.endTime = %lld, result = %d\n",__FILE__,__FUNCTION__,__LINE__,TimerTriggerEventList.size(),eventId,(long long int)trigger.startFrame,(long long int)trigger.endFrame,result);
	}

	return result;
}

int ScriptManager::getTimerEventSecondsElapsed(int eventId) {
	int result = 0;
	if(TimerTriggerEventList.find(eventId) != TimerTriggerEventList.end()) {
		TimerTriggerEvent &trigger = TimerTriggerEventList[eventId];
		if(trigger.running) {
			//result = (int)difftime(time(NULL),trigger.startTime);
			result = (world->getFrameCount()-trigger.startFrame) / GameConstants::updateFps;
		}
		else {
			//result = (int)difftime(trigger.endTime,trigger.startTime);
			result = (trigger.endFrame-trigger.startFrame) / GameConstants::updateFps;
		}

		//printf("Timer event id = %d seconmds elapsed = %d\n",eventId,result);
	}

	return result;
}

void ScriptManager::setPlayerAsWinner(int factionIndex) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	if(factionIndex<GameConstants::maxPlayers){
		playerModifiers[factionIndex].setAsWinner();
	}
}

void ScriptManager::endGame() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	gameOver= true;
}

void ScriptManager::startPerformanceTimer() {
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	if(world->getGame() == NULL) {
		throw megaglest_runtime_error("world->getGame() == NULL");
	}
	world->getGame()->startPerformanceTimer();

}

void ScriptManager::endPerformanceTimer() {
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	if(world->getGame() == NULL) {
		throw megaglest_runtime_error("world->getGame() == NULL");
	}
	world->getGame()->endPerformanceTimer();

}

Vec2i ScriptManager::getPerformanceTimerResults() {
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	if(world->getGame() == NULL) {
		throw megaglest_runtime_error("world->getGame() == NULL");
	}
	return world->getGame()->getPerformanceTimerResults();
}

Vec2i ScriptManager::getStartLocation(int factionIndex) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	return world->getStartLocation(factionIndex);
}


Vec2i ScriptManager::getUnitPosition(int unitId) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	return world->getUnitPosition(unitId);
}

void ScriptManager::setUnitPosition(int unitId, Vec2i pos) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	return world->setUnitPosition(unitId,pos);
}

int ScriptManager::getIsUnitAlive(int unitId) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	return world->getIsUnitAlive(unitId);
}

int ScriptManager::getUnitFaction(int unitId) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	return world->getUnitFactionIndex(unitId);
}

int ScriptManager::getResourceAmount(const string &resourceName, int factionIndex) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	return world->getResourceAmount(resourceName, factionIndex);
}

const string &ScriptManager::getLastCreatedUnitName() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	return lastCreatedUnitName;
}

int ScriptManager::getCellTriggeredEventId() {
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	return currentCellTriggeredEventId;
}

int ScriptManager::getTimerTriggeredEventId() {
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	return currentTimerTriggeredEventId;
}

void ScriptManager::setRandomGenInit(int seed) {
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	random.init(seed);
}

int ScriptManager::getRandomGen(int minVal, int maxVal) {
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	return random.randRange(minVal,maxVal);
}

int ScriptManager::getWorldFrameCount() {
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	return world->getFrameCount();
}

bool ScriptManager::getGameWon() const {
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	return gameWon;
}

bool ScriptManager::getIsGameOver() const {
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	return gameOver;
}

int ScriptManager::getLastCreatedUnitId() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	return lastCreatedUnitId;
}

const string &ScriptManager::getLastDeadUnitName() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	return lastDeadUnitName;
}

int ScriptManager::getLastDeadUnitId() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	return lastDeadUnitId;
}

int ScriptManager::getLastDeadUnitCauseOfDeath() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	return lastDeadUnitCauseOfDeath;
}

const string &ScriptManager::getLastDeadUnitKillerName() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	return lastDeadUnitKillerName;
}

int ScriptManager::getLastDeadUnitKillerId() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	return lastDeadUnitKillerId;
}

const string &ScriptManager::getLastAttackedUnitName() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	return lastAttackedUnitName;
}

int ScriptManager::getLastAttackedUnitId() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	return lastAttackedUnitId;
}

const string &ScriptManager::getLastAttackingUnitName() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	return lastAttackingUnitName;
}

int ScriptManager::getLastAttackingUnitId() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	return lastAttackingUnitId;
}

int ScriptManager::getUnitCount(int factionIndex) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	return world->getUnitCount(factionIndex);
}

int ScriptManager::getUnitCountOfType(int factionIndex, const string &typeName) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	return world->getUnitCountOfType(factionIndex, typeName);
}

const string ScriptManager::getSystemMacroValue(const string &key) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	return world->getSystemMacroValue(key);
}

const string ScriptManager::getPlayerName(int factionIndex) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	return world->getPlayerName(factionIndex);
}

void ScriptManager::loadScenario(const string &name, bool keepFactions) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;

	world->setQueuedScenario(name,keepFactions);
}

vector<int> ScriptManager::getUnitsForFaction(int factionIndex,const string& commandTypeName, int field) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;

	return world->getUnitsForFaction(factionIndex,commandTypeName, field);
}

int ScriptManager::getUnitCurrentField(int unitId) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;

	return world->getUnitCurrentField(unitId);
}

// ========================== lua callbacks ===============================================

int ScriptManager::showMessage(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	thisScriptManager->showMessage(luaArguments.getString(-2), luaArguments.getString(-1));
	return luaArguments.getReturnCount();
}

int ScriptManager::networkShowMessageForFaction(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	thisScriptManager->networkShowMessageForFaction(luaArguments.getString(-3), luaArguments.getString(-2), luaArguments.getInt(-1));
	return luaArguments.getReturnCount();
}

int ScriptManager::networkShowMessageForTeam(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	thisScriptManager->networkShowMessageForTeam(luaArguments.getString(-3), luaArguments.getString(-2), luaArguments.getInt(-1));
	return luaArguments.getReturnCount();
}

int ScriptManager::setDisplayText(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	thisScriptManager->setDisplayText(luaArguments.getString(-1));
	return luaArguments.getReturnCount();
}

int ScriptManager::addConsoleText(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	thisScriptManager->addConsoleText(luaArguments.getString(-1));
	return luaArguments.getReturnCount();
}

int ScriptManager::clearDisplayText(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	thisScriptManager->clearDisplayText();
	return luaArguments.getReturnCount();
}

int ScriptManager::setCameraPosition(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	thisScriptManager->setCameraPosition(Vec2i(luaArguments.getVec2i(-1)));
	return luaArguments.getReturnCount();
}

int ScriptManager::createUnit(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] unit [%s] factionIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,luaArguments.getString(-3).c_str(),luaArguments.getInt(-2));

	thisScriptManager->createUnit(
		luaArguments.getString(-3),
		luaArguments.getInt(-2),
		luaArguments.getVec2i(-1));
	return luaArguments.getReturnCount();
}

int ScriptManager::createUnitNoSpacing(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] unit [%s] factionIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,luaArguments.getString(-3).c_str(),luaArguments.getInt(-2));

	thisScriptManager->createUnitNoSpacing(
		luaArguments.getString(-3),
		luaArguments.getInt(-2),
		luaArguments.getVec2i(-1));
	return luaArguments.getReturnCount();
}

int ScriptManager::destroyUnit(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] unit [%d]\n",__FILE__,__FUNCTION__,__LINE__,luaArguments.getInt(-1));

	thisScriptManager->destroyUnit(luaArguments.getInt(-1));
	return luaArguments.getReturnCount();
}
int ScriptManager::giveKills(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] unit [%d]\n",__FILE__,__FUNCTION__,__LINE__,luaArguments.getInt(-1));

	thisScriptManager->giveKills(luaArguments.getInt(-2),luaArguments.getInt(-1));
	return luaArguments.getReturnCount();
}

int ScriptManager::morphToUnit(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] unit [%d] morphName [%s] forceUpgrade = %d\n",__FILE__,__FUNCTION__,__LINE__,luaArguments.getInt(-3),luaArguments.getString(-2).c_str(),luaArguments.getInt(-1));

	thisScriptManager->morphToUnit(luaArguments.getInt(-3),luaArguments.getString(-2),luaArguments.getInt(-1));
	return luaArguments.getReturnCount();
}

int ScriptManager::moveToUnit(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] unit [%d] dest unit [%d]\n",__FILE__,__FUNCTION__,__LINE__,luaArguments.getInt(-2),luaArguments.getInt(-1));

	thisScriptManager->moveToUnit(luaArguments.getInt(-2),luaArguments.getInt(-1));
	return luaArguments.getReturnCount();
}

int ScriptManager::playStaticSound(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] sound [%s]\n",__FILE__,__FUNCTION__,__LINE__,luaArguments.getString(-1).c_str());
	thisScriptManager->playStaticSound(luaArguments.getString(-1));
	return luaArguments.getReturnCount();
}

int ScriptManager::playStreamingSound(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] sound [%s]\n",__FILE__,__FUNCTION__,__LINE__,luaArguments.getString(-1).c_str());
	thisScriptManager->playStreamingSound(luaArguments.getString(-1));
	return luaArguments.getReturnCount();
}

int ScriptManager::stopStreamingSound(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] sound [%s]\n",__FILE__,__FUNCTION__,__LINE__,luaArguments.getString(-1).c_str());
	thisScriptManager->stopStreamingSound(luaArguments.getString(-1));
	return luaArguments.getReturnCount();
}

int ScriptManager::stopAllSound(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	thisScriptManager->stopAllSound();
	return luaArguments.getReturnCount();
}

int ScriptManager::togglePauseGame(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] value = %d\n",__FILE__,__FUNCTION__,__LINE__,luaArguments.getInt(-1));
	thisScriptManager->togglePauseGame(luaArguments.getInt(-1));
	return luaArguments.getReturnCount();
}
int ScriptManager::giveResource(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	thisScriptManager->giveResource(luaArguments.getString(-3), luaArguments.getInt(-2), luaArguments.getInt(-1));
	return luaArguments.getReturnCount();
}

int ScriptManager::givePositionCommand(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	thisScriptManager->givePositionCommand(
		luaArguments.getInt(-3),
		luaArguments.getString(-2),
		luaArguments.getVec2i(-1));
	return luaArguments.getReturnCount();
}

int ScriptManager::giveAttackCommand(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	thisScriptManager->giveAttackCommand(
		luaArguments.getInt(-2),
		luaArguments.getInt(-1));
	return luaArguments.getReturnCount();
}

int ScriptManager::giveProductionCommand(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	thisScriptManager->giveProductionCommand(
		luaArguments.getInt(-2),
		luaArguments.getString(-1));
	return luaArguments.getReturnCount();
}

int ScriptManager::giveUpgradeCommand(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	thisScriptManager->giveUpgradeCommand(
		luaArguments.getInt(-2),
		luaArguments.getString(-1));
	return luaArguments.getReturnCount();
}

int ScriptManager::giveAttackStoppedCommand(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	thisScriptManager->giveAttackStoppedCommand(
		luaArguments.getInt(-3),
		luaArguments.getString(-2),
		luaArguments.getInt(-1));
	return luaArguments.getReturnCount();
}

int ScriptManager::disableAi(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	thisScriptManager->disableAi(luaArguments.getInt(-1));
	return luaArguments.getReturnCount();
}

int ScriptManager::enableAi(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	thisScriptManager->enableAi(luaArguments.getInt(-1));
	return luaArguments.getReturnCount();
}

int ScriptManager::getAiEnabled(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	bool result = thisScriptManager->getAiEnabled(luaArguments.getInt(-1));
	luaArguments.returnInt(result);
	return luaArguments.getReturnCount();
}

int ScriptManager::disableConsume(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	thisScriptManager->disableConsume(luaArguments.getInt(-1));
	return luaArguments.getReturnCount();
}

int ScriptManager::enableConsume(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	thisScriptManager->enableConsume(luaArguments.getInt(-1));
	return luaArguments.getReturnCount();
}

int ScriptManager::getConsumeEnabled(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	bool result = thisScriptManager->getConsumeEnabled(luaArguments.getInt(-1));
	luaArguments.returnInt(result);
	return luaArguments.getReturnCount();
}

int ScriptManager::registerCellTriggerEventForUnitToUnit(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	int result = thisScriptManager->registerCellTriggerEventForUnitToUnit(luaArguments.getInt(-2),luaArguments.getInt(-1));
	luaArguments.returnInt(result);
	return luaArguments.getReturnCount();
}

int ScriptManager::registerCellTriggerEventForUnitToLocation(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	int result = thisScriptManager->registerCellTriggerEventForUnitToLocation(luaArguments.getInt(-2),luaArguments.getVec2i(-1));
	luaArguments.returnInt(result);
	return luaArguments.getReturnCount();
}

int ScriptManager::registerCellTriggerEventForFactionToUnit(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	int result = thisScriptManager->registerCellTriggerEventForFactionToUnit(luaArguments.getInt(-2),luaArguments.getInt(-1));
	luaArguments.returnInt(result);
	return luaArguments.getReturnCount();
}

int ScriptManager::registerCellTriggerEventForFactionToLocation(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	int result = thisScriptManager->registerCellTriggerEventForFactionToLocation(luaArguments.getInt(-2),luaArguments.getVec2i(-1));
	luaArguments.returnInt(result);
	return luaArguments.getReturnCount();
}

int ScriptManager::getCellTriggerEventCount(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	int result = thisScriptManager->getCellTriggerEventCount(luaArguments.getInt(-1));
	luaArguments.returnInt(result);
	return luaArguments.getReturnCount();
}

int ScriptManager::unregisterCellTriggerEvent(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	thisScriptManager->unregisterCellTriggerEvent(luaArguments.getInt(-1));
	return luaArguments.getReturnCount();
}

int ScriptManager::startTimerEvent(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	int result = thisScriptManager->startTimerEvent();
	luaArguments.returnInt(result);
	return luaArguments.getReturnCount();
}

int ScriptManager::startEfficientTimerEvent(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	int result = thisScriptManager->startEfficientTimerEvent(luaArguments.getInt(-1));
	luaArguments.returnInt(result);
	return luaArguments.getReturnCount();
}

int ScriptManager::stopTimerEvent(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	int result = thisScriptManager->stopTimerEvent(luaArguments.getInt(-1));
	luaArguments.returnInt(result);
	return luaArguments.getReturnCount();
}

int ScriptManager::resetTimerEvent(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	int result = thisScriptManager->resetTimerEvent(luaArguments.getInt(-1));
	luaArguments.returnInt(result);
	return luaArguments.getReturnCount();
}

int ScriptManager::getTimerEventSecondsElapsed(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	int result = thisScriptManager->getTimerEventSecondsElapsed(luaArguments.getInt(-1));
	luaArguments.returnInt(result);
	return luaArguments.getReturnCount();
}

int ScriptManager::setPlayerAsWinner(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	thisScriptManager->setPlayerAsWinner(luaArguments.getInt(-1));
	return luaArguments.getReturnCount();
}

int ScriptManager::endGame(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	thisScriptManager->endGame();
	return luaArguments.getReturnCount();
}

int ScriptManager::startPerformanceTimer(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	thisScriptManager->startPerformanceTimer();
	return luaArguments.getReturnCount();
}

int ScriptManager::endPerformanceTimer(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	thisScriptManager->endPerformanceTimer();
	return luaArguments.getReturnCount();
}

int ScriptManager::getPerformanceTimerResults(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	Vec2i results= thisScriptManager->getPerformanceTimerResults();
	luaArguments.returnVec2i(results);
	return luaArguments.getReturnCount();
}

int ScriptManager::getStartLocation(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	Vec2i pos= thisScriptManager->getStartLocation(luaArguments.getInt(-1));
	luaArguments.returnVec2i(pos);
	return luaArguments.getReturnCount();
}

int ScriptManager::getUnitPosition(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	Vec2i pos= thisScriptManager->getUnitPosition(luaArguments.getInt(-1));
	luaArguments.returnVec2i(pos);
	return luaArguments.getReturnCount();
}

int ScriptManager::setUnitPosition(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	thisScriptManager->setUnitPosition(luaArguments.getInt(-2),luaArguments.getVec2i(-1));
	return luaArguments.getReturnCount();
}

int ScriptManager::getUnitFaction(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	int factionIndex= thisScriptManager->getUnitFaction(luaArguments.getInt(-1));
	luaArguments.returnInt(factionIndex);
	return luaArguments.getReturnCount();
}

int ScriptManager::getResourceAmount(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	luaArguments.returnInt(thisScriptManager->getResourceAmount(luaArguments.getString(-2), luaArguments.getInt(-1)));
	return luaArguments.getReturnCount();
}

int ScriptManager::getLastCreatedUnitName(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	luaArguments.returnString(thisScriptManager->getLastCreatedUnitName());
	return luaArguments.getReturnCount();
}

int ScriptManager::getLastCreatedUnitId(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	luaArguments.returnInt(thisScriptManager->getLastCreatedUnitId());
	return luaArguments.getReturnCount();
}

int ScriptManager::getCellTriggeredEventId(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	luaArguments.returnInt(thisScriptManager->getCellTriggeredEventId());
	return luaArguments.getReturnCount();
}

int ScriptManager::getTimerTriggeredEventId(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	luaArguments.returnInt(thisScriptManager->getTimerTriggeredEventId());
	return luaArguments.getReturnCount();
}

int ScriptManager::setRandomGenInit(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	thisScriptManager->setRandomGenInit(luaArguments.getInt(-1));
	return luaArguments.getReturnCount();
}

int ScriptManager::getRandomGen(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	luaArguments.returnInt(thisScriptManager->getRandomGen(luaArguments.getInt(-2),luaArguments.getInt(-1)));
	return luaArguments.getReturnCount();
}

int ScriptManager::getWorldFrameCount(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	luaArguments.returnInt(thisScriptManager->getWorldFrameCount());
	return luaArguments.getReturnCount();
}

int ScriptManager::getLastDeadUnitName(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	luaArguments.returnString(thisScriptManager->getLastDeadUnitName());
	return luaArguments.getReturnCount();
}

int ScriptManager::getLastDeadUnitId(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	luaArguments.returnInt(thisScriptManager->getLastDeadUnitId());
	return luaArguments.getReturnCount();
}

int ScriptManager::getLastDeadUnitCauseOfDeath(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	luaArguments.returnInt(thisScriptManager->getLastDeadUnitCauseOfDeath());
	return luaArguments.getReturnCount();
}

int ScriptManager::getLastDeadUnitKillerName(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	luaArguments.returnString(thisScriptManager->getLastDeadUnitKillerName());
	return luaArguments.getReturnCount();
}

int ScriptManager::getLastDeadUnitKillerId(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	luaArguments.returnInt(thisScriptManager->getLastDeadUnitKillerId());
	return luaArguments.getReturnCount();
}

int ScriptManager::getLastAttackedUnitName(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	luaArguments.returnString(thisScriptManager->getLastAttackedUnitName());
	return luaArguments.getReturnCount();
}

int ScriptManager::getLastAttackedUnitId(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	luaArguments.returnInt(thisScriptManager->getLastAttackedUnitId());
	return luaArguments.getReturnCount();
}

int ScriptManager::getLastAttackingUnitName(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	luaArguments.returnString(thisScriptManager->getLastAttackingUnitName());
	return luaArguments.getReturnCount();
}

int ScriptManager::getLastAttackingUnitId(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	luaArguments.returnInt(thisScriptManager->getLastAttackingUnitId());
	return luaArguments.getReturnCount();
}

int ScriptManager::getSystemMacroValue(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	luaArguments.returnString(thisScriptManager->getSystemMacroValue(luaArguments.getString(-1)));
	return luaArguments.getReturnCount();
}

int ScriptManager::scenarioDir(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	luaArguments.returnString(thisScriptManager->getSystemMacroValue("$SCENARIO_PATH"));
	return luaArguments.getReturnCount();
}

int ScriptManager::getPlayerName(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	luaArguments.returnString(thisScriptManager->getPlayerName(luaArguments.getInt(-1)));
	return luaArguments.getReturnCount();
}

int ScriptManager::getUnitCount(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	luaArguments.returnInt(thisScriptManager->getUnitCount(luaArguments.getInt(-1)));
	return luaArguments.getReturnCount();
}

int ScriptManager::getUnitCountOfType(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	luaArguments.returnInt(thisScriptManager->getUnitCountOfType(luaArguments.getInt(-2), luaArguments.getString(-1)));
	return luaArguments.getReturnCount();
}

int ScriptManager::DisplayFormattedText(LuaHandle* luaHandle) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	  //const char *ret;
	  //lua_lock(luaHandle);
	  //luaC_checkGC(luaHandle);

	  const int max_args_allowed = 8;
	  int args = lua_gettop(luaHandle);
	  if(lua_checkstack(luaHandle, args+1)) {
		LuaArguments luaArguments(luaHandle);
		string fmt = luaArguments.getString(-args);
	    //va_list argList;
	    //va_start(argList, fmt.c_str() );

		if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"DisplayFormattedText args = %d!\n",args);

		if(args == 1) {
			thisScriptManager->DisplayFormattedText(fmt.c_str());
		}
		else if(args == 2) {
			thisScriptManager->DisplayFormattedText(fmt.c_str(),
					luaArguments.getGenericData(-args+1));
		}
		else if(args == 3) {
			thisScriptManager->DisplayFormattedText(fmt.c_str(),
					luaArguments.getGenericData(-args+1),
					luaArguments.getGenericData(-args+2));
		}
		else if(args == 4) {
			thisScriptManager->DisplayFormattedText(fmt.c_str(),
					luaArguments.getGenericData(-args+1),
					luaArguments.getGenericData(-args+2),
					luaArguments.getGenericData(-args+3));
		}
		else if(args == 5) {
			thisScriptManager->DisplayFormattedText(fmt.c_str(),
					luaArguments.getGenericData(-args+1),
					luaArguments.getGenericData(-args+2),
					luaArguments.getGenericData(-args+3),
					luaArguments.getGenericData(-args+4));
		}
		else if(args == 6) {
			thisScriptManager->DisplayFormattedText(fmt.c_str(),
					luaArguments.getGenericData(-args+1),
					luaArguments.getGenericData(-args+2),
					luaArguments.getGenericData(-args+3),
					luaArguments.getGenericData(-args+4),
					luaArguments.getGenericData(-args+5));
		}
		else if(args == 7) {
			thisScriptManager->DisplayFormattedText(fmt.c_str(),
					luaArguments.getGenericData(-args+1),
					luaArguments.getGenericData(-args+2),
					luaArguments.getGenericData(-args+3),
					luaArguments.getGenericData(-args+4),
					luaArguments.getGenericData(-args+5),
					luaArguments.getGenericData(-args+6));
		}
		else if(args == max_args_allowed) {
			thisScriptManager->DisplayFormattedText(fmt.c_str(),
					luaArguments.getGenericData(-args+1),
					luaArguments.getGenericData(-args+2),
					luaArguments.getGenericData(-args+3),
					luaArguments.getGenericData(-args+4),
					luaArguments.getGenericData(-args+5),
					luaArguments.getGenericData(-args+6),
					luaArguments.getGenericData(-args+7));
		}
		else {
			char szBuf[1024]="";
			sprintf(szBuf,"Invalid parameter count in method [%s] args = %d [argument count must be between 1 and %d]",__FUNCTION__,args,max_args_allowed);
			throw megaglest_runtime_error(szBuf);
		}

	    //va_end(argList);
  	  }
	  //lua_unlock(luaHandle);
	  return 1;

/*
	int args=lua_gettop(luaHandle);
	if(lua_checkstack(luaHandle, args+1))
	{
	  va_list argList;
	  int i;
	  //lua_getfield(luaHandle, LUA_GLOBALSINDEX, "print");
	  for(i = 1;i <= args; i++) {
	    lua_pushvalue(luaHandle, i);
	  }
	  lua_call(luaHandle, args, 0);
	}
	else
	{
	  return luaL_error(luaHandle, "cannot grow stack");
	}
*/

/*
		luax_getfunction(L, mod, fn);
		for (int i = 0; i < n; i++) {
			lua_pushvalue(L, idxs[i]); // The arguments.
		}
		lua_call(L, n, 1); // Call the function, n args, one return value.
		lua_replace(L, idxs[0]); // Replace the initial argument with the new object.
		return 0;
*/
}
int ScriptManager::addConsoleLangText(LuaHandle* luaHandle){
if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	  //const char *ret;
	  //lua_lock(luaHandle);
	  //luaC_checkGC(luaHandle);

	  const int max_args_allowed = 8;
	  int args = lua_gettop(luaHandle);
	  if(lua_checkstack(luaHandle, args+1)) {
		LuaArguments luaArguments(luaHandle);
		string fmt = luaArguments.getString(-args);
	    //va_list argList;
	    //va_start(argList, fmt.c_str() );

		if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"DisplayFormattedText args = %d!\n",args);

		if(args == 1) {
			thisScriptManager->addConsoleLangText(Lang::getInstance().getScenarioString(fmt).c_str());
		}
		else if(args == 2) {
			thisScriptManager->addConsoleLangText(Lang::getInstance().getScenarioString(fmt).c_str(),
					luaArguments.getGenericData(-args+1));
		}
		else if(args == 3) {
			thisScriptManager->addConsoleLangText(Lang::getInstance().getScenarioString(fmt).c_str(),
					luaArguments.getGenericData(-args+1),
					luaArguments.getGenericData(-args+2));
		}
		else if(args == 4) {
			thisScriptManager->addConsoleLangText(Lang::getInstance().getScenarioString(fmt).c_str(),
					luaArguments.getGenericData(-args+1),
					luaArguments.getGenericData(-args+2),
					luaArguments.getGenericData(-args+3));
		}
		else if(args == 5) {
			thisScriptManager->addConsoleLangText(Lang::getInstance().getScenarioString(fmt).c_str(),
					luaArguments.getGenericData(-args+1),
					luaArguments.getGenericData(-args+2),
					luaArguments.getGenericData(-args+3),
					luaArguments.getGenericData(-args+4));
		}
		else if(args == 6) {
			thisScriptManager->addConsoleLangText(Lang::getInstance().getScenarioString(fmt).c_str(),
					luaArguments.getGenericData(-args+1),
					luaArguments.getGenericData(-args+2),
					luaArguments.getGenericData(-args+3),
					luaArguments.getGenericData(-args+4),
					luaArguments.getGenericData(-args+5));
		}
		else if(args == 7) {
			thisScriptManager->addConsoleLangText(Lang::getInstance().getScenarioString(fmt).c_str(),
					luaArguments.getGenericData(-args+1),
					luaArguments.getGenericData(-args+2),
					luaArguments.getGenericData(-args+3),
					luaArguments.getGenericData(-args+4),
					luaArguments.getGenericData(-args+5),
					luaArguments.getGenericData(-args+6));
		}
		else if(args == max_args_allowed) {
			thisScriptManager->addConsoleLangText(Lang::getInstance().getScenarioString(fmt).c_str(),
					luaArguments.getGenericData(-args+1),
					luaArguments.getGenericData(-args+2),
					luaArguments.getGenericData(-args+3),
					luaArguments.getGenericData(-args+4),
					luaArguments.getGenericData(-args+5),
					luaArguments.getGenericData(-args+6),
					luaArguments.getGenericData(-args+7));
		}
		else {
			char szBuf[1024]="";
			sprintf(szBuf,"Invalid parameter count in method [%s] args = %d [argument count must be between 1 and %d]",__FUNCTION__,args,max_args_allowed);
			throw megaglest_runtime_error(szBuf);
		}

	    //va_end(argList);
  	  }
	  //lua_unlock(luaHandle);
	  return 1;

}
int ScriptManager::DisplayFormattedLangText(LuaHandle* luaHandle) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	  //const char *ret;
	  //lua_lock(luaHandle);
	  //luaC_checkGC(luaHandle);

	  const int max_args_allowed = 8;
	  int args = lua_gettop(luaHandle);
	  if(lua_checkstack(luaHandle, args+1)) {
		LuaArguments luaArguments(luaHandle);
		string fmt = luaArguments.getString(-args);
	    //va_list argList;
	    //va_start(argList, fmt.c_str() );

		if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"DisplayFormattedText args = %d!\n",args);

		if(args == 1) {
			thisScriptManager->DisplayFormattedLangText(Lang::getInstance().getScenarioString(fmt).c_str());
		}
		else if(args == 2) {
			thisScriptManager->DisplayFormattedLangText(Lang::getInstance().getScenarioString(fmt).c_str(),
					luaArguments.getGenericData(-args+1));
		}
		else if(args == 3) {
			thisScriptManager->DisplayFormattedLangText(Lang::getInstance().getScenarioString(fmt).c_str(),
					luaArguments.getGenericData(-args+1),
					luaArguments.getGenericData(-args+2));
		}
		else if(args == 4) {
			thisScriptManager->DisplayFormattedLangText(Lang::getInstance().getScenarioString(fmt).c_str(),
					luaArguments.getGenericData(-args+1),
					luaArguments.getGenericData(-args+2),
					luaArguments.getGenericData(-args+3));
		}
		else if(args == 5) {
			thisScriptManager->DisplayFormattedLangText(Lang::getInstance().getScenarioString(fmt).c_str(),
					luaArguments.getGenericData(-args+1),
					luaArguments.getGenericData(-args+2),
					luaArguments.getGenericData(-args+3),
					luaArguments.getGenericData(-args+4));
		}
		else if(args == 6) {
			thisScriptManager->DisplayFormattedLangText(Lang::getInstance().getScenarioString(fmt).c_str(),
					luaArguments.getGenericData(-args+1),
					luaArguments.getGenericData(-args+2),
					luaArguments.getGenericData(-args+3),
					luaArguments.getGenericData(-args+4),
					luaArguments.getGenericData(-args+5));
		}
		else if(args == 7) {
			thisScriptManager->DisplayFormattedLangText(Lang::getInstance().getScenarioString(fmt).c_str(),
					luaArguments.getGenericData(-args+1),
					luaArguments.getGenericData(-args+2),
					luaArguments.getGenericData(-args+3),
					luaArguments.getGenericData(-args+4),
					luaArguments.getGenericData(-args+5),
					luaArguments.getGenericData(-args+6));
		}
		else if(args == max_args_allowed) {
			thisScriptManager->DisplayFormattedLangText(Lang::getInstance().getScenarioString(fmt).c_str(),
					luaArguments.getGenericData(-args+1),
					luaArguments.getGenericData(-args+2),
					luaArguments.getGenericData(-args+3),
					luaArguments.getGenericData(-args+4),
					luaArguments.getGenericData(-args+5),
					luaArguments.getGenericData(-args+6),
					luaArguments.getGenericData(-args+7));
		}
		else {
			char szBuf[1024]="";
			sprintf(szBuf,"Invalid parameter count in method [%s] args = %d [argument count must be between 1 and %d]",__FUNCTION__,args,max_args_allowed);
			throw megaglest_runtime_error(szBuf);
		}

	    //va_end(argList);
  	  }
	  //lua_unlock(luaHandle);
	  return 1;

}
int ScriptManager::getGameWon(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	luaArguments.returnInt(thisScriptManager->getGameWon());
	return luaArguments.getReturnCount();
}

int ScriptManager::getIsGameOver(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	luaArguments.returnInt(thisScriptManager->getIsGameOver());
	return luaArguments.getReturnCount();
}

int ScriptManager::loadScenario(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	thisScriptManager->loadScenario(luaArguments.getString(-2),luaArguments.getInt(-1) != 0);
	return luaArguments.getReturnCount();
}

int ScriptManager::getUnitsForFaction(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	vector<int> units= thisScriptManager->getUnitsForFaction(luaArguments.getInt(-3),luaArguments.getString(-2), luaArguments.getInt(-1));
	luaArguments.returnVectorInt(units);
	return luaArguments.getReturnCount();

}

int ScriptManager::getUnitCurrentField(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	luaArguments.returnInt(thisScriptManager->getUnitCurrentField(luaArguments.getInt(-1)));
	return luaArguments.getReturnCount();
}

int ScriptManager::getIsUnitAlive(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	luaArguments.returnInt(thisScriptManager->getIsUnitAlive(luaArguments.getInt(-1)));
	return luaArguments.getReturnCount();
}

void ScriptManager::saveGame(XmlNode *rootNode) {
	std::map<string,string> mapTagReplacements;
	XmlNode *scriptManagerNode = rootNode->addChild("ScriptManager");

	//lua
//	string code;
	scriptManagerNode->addAttribute("code",code, mapTagReplacements);
//	LuaScript luaScript;
//
//	//world
//	World *world;
//	GameCamera *gameCamera;
//
//	//misc
//	MessageQueue messageQueue;
	for(std::list<ScriptManagerMessage>::iterator it = messageQueue.begin(); it != messageQueue.end(); ++it) {
		(*it).saveGame(scriptManagerNode);
	}

	//printf("==== ScriptManager Savegame messageBox [%d][%s][%s] displayText [%s]\n",messageBox.getEnabled(),messageBox.getText().c_str(),messageBox.getHeader().c_str(),displayText.c_str());

//	GraphicMessageBox messageBox;
	scriptManagerNode->addAttribute("messageBox_enabled",intToStr(messageBox.getEnabled()), mapTagReplacements);
	scriptManagerNode->addAttribute("messageBox_text",messageBox.getText(), mapTagReplacements);
	scriptManagerNode->addAttribute("messageBox_header",messageBox.getHeader(), mapTagReplacements);

//	string displayText;
	scriptManagerNode->addAttribute("displayText",displayText, mapTagReplacements);
//
//	//last created unit
//	string lastCreatedUnitName;
	scriptManagerNode->addAttribute("lastCreatedUnitName",lastCreatedUnitName, mapTagReplacements);
//	int lastCreatedUnitId;
	scriptManagerNode->addAttribute("lastCreatedUnitId",intToStr(lastCreatedUnitId), mapTagReplacements);
//
//	//last dead unit
//	string lastDeadUnitName;
	scriptManagerNode->addAttribute("lastDeadUnitName",lastDeadUnitName, mapTagReplacements);
//	int lastDeadUnitId;
	scriptManagerNode->addAttribute("lastDeadUnitId",intToStr(lastDeadUnitId), mapTagReplacements);
//	int lastDeadUnitCauseOfDeath;
	scriptManagerNode->addAttribute("lastDeadUnitCauseOfDeath",intToStr(lastDeadUnitCauseOfDeath), mapTagReplacements);
//
//	//last dead unit's killer
//	string lastDeadUnitKillerName;
	scriptManagerNode->addAttribute("lastDeadUnitKillerName",lastDeadUnitKillerName, mapTagReplacements);
//	int lastDeadUnitKillerId;
	scriptManagerNode->addAttribute("lastDeadUnitKillerId",intToStr(lastDeadUnitKillerId), mapTagReplacements);
//
//	//last attacked unit
//	string lastAttackedUnitName;
	scriptManagerNode->addAttribute("lastAttackedUnitName",lastAttackedUnitName, mapTagReplacements);
//	int lastAttackedUnitId;
	scriptManagerNode->addAttribute("lastAttackedUnitId",intToStr(lastAttackedUnitId), mapTagReplacements);
//
//	//last attacking unit
//	string lastAttackingUnitName;
	scriptManagerNode->addAttribute("lastAttackingUnitName",lastAttackingUnitName, mapTagReplacements);
//	int lastAttackingUnitId;
	scriptManagerNode->addAttribute("lastAttackingUnitId",intToStr(lastAttackingUnitId), mapTagReplacements);
//
//	// end game state
//	bool gameOver;
	scriptManagerNode->addAttribute("gameOver",intToStr(gameOver), mapTagReplacements);
//	bool gameWon;
	scriptManagerNode->addAttribute("gameWon",intToStr(gameWon), mapTagReplacements);
//	PlayerModifiers playerModifiers[GameConstants::maxPlayers];
	for(unsigned int i = 0; i < GameConstants::maxPlayers; ++i) {
		PlayerModifiers &player = playerModifiers[i];
		player.saveGame(scriptManagerNode);
	}
//	int currentTimerTriggeredEventId;
	scriptManagerNode->addAttribute("currentTimerTriggeredEventId",intToStr(currentTimerTriggeredEventId), mapTagReplacements);
//	int currentCellTriggeredEventId;
	scriptManagerNode->addAttribute("currentCellTriggeredEventId",intToStr(currentCellTriggeredEventId), mapTagReplacements);
//	int currentEventId;
	scriptManagerNode->addAttribute("currentEventId",intToStr(currentEventId), mapTagReplacements);
//	std::map<int,CellTriggerEvent> CellTriggerEventList;
	for(std::map<int,CellTriggerEvent>::iterator iterMap = CellTriggerEventList.begin();
			iterMap != CellTriggerEventList.end(); ++iterMap) {
		XmlNode *cellTriggerEventListNode = scriptManagerNode->addChild("CellTriggerEventList");

		cellTriggerEventListNode->addAttribute("key",intToStr(iterMap->first), mapTagReplacements);
		iterMap->second.saveGame(cellTriggerEventListNode);
	}
//	std::map<int,TimerTriggerEvent> TimerTriggerEventList;
	for(std::map<int,TimerTriggerEvent>::iterator iterMap = TimerTriggerEventList.begin();
			iterMap != TimerTriggerEventList.end(); ++iterMap) {
		XmlNode *timerTriggerEventListNode = scriptManagerNode->addChild("TimerTriggerEventList");

		timerTriggerEventListNode->addAttribute("key",intToStr(iterMap->first), mapTagReplacements);
		iterMap->second.saveGame(timerTriggerEventListNode);
	}

//	bool inCellTriggerEvent;
	scriptManagerNode->addAttribute("inCellTriggerEvent",intToStr(inCellTriggerEvent), mapTagReplacements);
//	std::vector<int> unRegisterCellTriggerEventList;
	for(unsigned int i = 0; i < unRegisterCellTriggerEventList.size(); ++i) {
		XmlNode *unRegisterCellTriggerEventListNode = scriptManagerNode->addChild("unRegisterCellTriggerEventList");
		unRegisterCellTriggerEventListNode->addAttribute("eventId",intToStr(unRegisterCellTriggerEventList[i]), mapTagReplacements);
	}

	luaScript.saveGame(scriptManagerNode);
}

void ScriptManager::loadGame(const XmlNode *rootNode) {
	const XmlNode *scriptManagerNode = rootNode->getChild("ScriptManager");

//	string code;
	code = scriptManagerNode->getAttribute("code")->getValue();
//	LuaScript luaScript;
//
//	//world
//	World *world;
//	GameCamera *gameCamera;
//
//	//misc
//	MessageQueue messageQueue;
	messageQueue.clear();
	vector<XmlNode *> messageQueueNodeList = scriptManagerNode->getChildList("ScriptManagerMessage");
	for(unsigned int i = 0; i < messageQueueNodeList.size(); ++i) {
		XmlNode *node = messageQueueNodeList[i];
		ScriptManagerMessage msg;
		msg.loadGame(node);
		messageQueue.push_back(msg);
	}

//	GraphicMessageBox messageBox;
	messageBox.setEnabled(scriptManagerNode->getAttribute("messageBox_enabled")->getIntValue() != 0);
	messageBox.setText(wrapString(scriptManagerNode->getAttribute("messageBox_text")->getValue(),messageWrapCount));
	messageBox.setHeader(scriptManagerNode->getAttribute("messageBox_header")->getValue());

//	string displayText;
	displayText = scriptManagerNode->getAttribute("displayText")->getValue();
//
//	//last created unit
//	string lastCreatedUnitName;
	lastCreatedUnitName = scriptManagerNode->getAttribute("lastCreatedUnitName")->getValue();
//	int lastCreatedUnitId;
	lastCreatedUnitId = scriptManagerNode->getAttribute("lastCreatedUnitId")->getIntValue();
//
//	//last dead unit
//	string lastDeadUnitName;
	lastDeadUnitName = scriptManagerNode->getAttribute("lastDeadUnitName")->getValue();
//	int lastDeadUnitId;
	lastDeadUnitId = scriptManagerNode->getAttribute("lastDeadUnitId")->getIntValue();
//	int lastDeadUnitCauseOfDeath;
	lastDeadUnitCauseOfDeath = scriptManagerNode->getAttribute("lastDeadUnitCauseOfDeath")->getIntValue();
//
//	//last dead unit's killer
//	string lastDeadUnitKillerName;
	lastDeadUnitKillerName = scriptManagerNode->getAttribute("lastDeadUnitKillerName")->getValue();
//	int lastDeadUnitKillerId;
	lastDeadUnitKillerId = scriptManagerNode->getAttribute("lastDeadUnitKillerId")->getIntValue();
//
//	//last attacked unit
//	string lastAttackedUnitName;
	lastAttackedUnitName = scriptManagerNode->getAttribute("lastAttackedUnitName")->getValue();
//	int lastAttackedUnitId;
	lastAttackedUnitId = scriptManagerNode->getAttribute("lastAttackedUnitId")->getIntValue();
//
//	//last attacking unit
//	string lastAttackingUnitName;
	lastAttackingUnitName = scriptManagerNode->getAttribute("lastAttackingUnitName")->getValue();
//	int lastAttackingUnitId;
	lastAttackingUnitId = scriptManagerNode->getAttribute("lastAttackingUnitId")->getIntValue();
//
//	// end game state
//	bool gameOver;
	gameOver = scriptManagerNode->getAttribute("gameOver")->getIntValue() != 0;
//	bool gameWon;
	gameWon = scriptManagerNode->getAttribute("gameWon")->getIntValue() != 0;
//	PlayerModifiers playerModifiers[GameConstants::maxPlayers];
	vector<XmlNode *> playerModifiersNodeList = scriptManagerNode->getChildList("PlayerModifiers");
	for(unsigned int i = 0; i < playerModifiersNodeList.size(); ++i) {
		XmlNode *node = playerModifiersNodeList[i];
		playerModifiers[i].loadGame(node);
	}
//	int currentTimerTriggeredEventId;
	currentTimerTriggeredEventId = scriptManagerNode->getAttribute("currentTimerTriggeredEventId")->getIntValue();
//	int currentCellTriggeredEventId;
	currentCellTriggeredEventId = scriptManagerNode->getAttribute("currentCellTriggeredEventId")->getIntValue();
//	int currentEventId;
	currentEventId = scriptManagerNode->getAttribute("currentEventId")->getIntValue();
//	std::map<int,CellTriggerEvent> CellTriggerEventList;
	vector<XmlNode *> cellTriggerEventListNodeList = scriptManagerNode->getChildList("CellTriggerEventList");
	for(unsigned int i = 0; i < cellTriggerEventListNodeList.size(); ++i) {
		XmlNode *node = cellTriggerEventListNodeList[i];
		CellTriggerEvent event;
		event.loadGame(node);
		CellTriggerEventList[node->getAttribute("key")->getIntValue()] = event;
	}

//	std::map<int,TimerTriggerEvent> TimerTriggerEventList;
	vector<XmlNode *> timerTriggerEventListNodeList = scriptManagerNode->getChildList("TimerTriggerEventList");
	for(unsigned int i = 0; i < timerTriggerEventListNodeList.size(); ++i) {
		XmlNode *node = timerTriggerEventListNodeList[i];

		TimerTriggerEvent event;
		event.loadGame(node);
		TimerTriggerEventList[node->getAttribute("key")->getIntValue()] = event;
	}

//	bool inCellTriggerEvent;
	inCellTriggerEvent = scriptManagerNode->getAttribute("inCellTriggerEvent")->getIntValue() != 0;
//	std::vector<int> unRegisterCellTriggerEventList;
	vector<XmlNode *> unRegisterCellTriggerEventListNodeList = scriptManagerNode->getChildList("unRegisterCellTriggerEventList");
	for(unsigned int i = 0; i < unRegisterCellTriggerEventListNodeList.size(); ++i) {
		XmlNode *node = unRegisterCellTriggerEventListNodeList[i];
		unRegisterCellTriggerEventList.push_back(node->getAttribute("eventId")->getIntValue());
	}

	luaScript.loadGame(scriptManagerNode);
}


}}//end namespace
