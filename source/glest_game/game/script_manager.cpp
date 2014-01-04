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

ScriptManagerMessage::ScriptManagerMessage() : text(""), header("") {
	this->factionIndex			= -1;
	this->teamIndex				= -1;
	this->messageNotTranslated 	= true;
}

ScriptManagerMessage::ScriptManagerMessage(string textIn, string headerIn,
		int factionIndex,int teamIndex, bool messageNotTranslated) :
				text(textIn), header(headerIn) {
	this->factionIndex 			= factionIndex;
	this->teamIndex    			= teamIndex;
	this->messageNotTranslated 	= messageNotTranslated;
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
	scriptManagerMessageNode->addAttribute("messageNotTranslated",intToStr(messageNotTranslated), mapTagReplacements);
}

void ScriptManagerMessage::loadGame(const XmlNode *rootNode) {
	const XmlNode *scriptManagerMessageNode = rootNode;

	text = scriptManagerMessageNode->getAttribute("text")->getValue();
	header = scriptManagerMessageNode->getAttribute("header")->getValue();
	factionIndex = scriptManagerMessageNode->getAttribute("factionIndex")->getIntValue();
	teamIndex = scriptManagerMessageNode->getAttribute("teamIndex")->getIntValue();

	messageNotTranslated = true;
	if(scriptManagerMessageNode->hasAttribute("messageNotTranslated") == true) {
		messageNotTranslated = (scriptManagerMessageNode->getAttribute("messageNotTranslated")->getIntValue() != 0);
	}
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

//	Vec2i destPosEnd;
	cellTriggerEventNode->addAttribute("destPosEnd",destPosEnd.getString(), mapTagReplacements);
}

void CellTriggerEvent::loadGame(const XmlNode *rootNode) {
	const XmlNode *cellTriggerEventNode = rootNode->getChild("CellTriggerEvent");

	type = static_cast<CellTriggerEventType>(cellTriggerEventNode->getAttribute("type")->getIntValue());
	sourceId = cellTriggerEventNode->getAttribute("sourceId")->getIntValue();
	destId = cellTriggerEventNode->getAttribute("destId")->getIntValue();
	destPos = Vec2i::strToVec2(cellTriggerEventNode->getAttribute("destPos")->getValue());
	triggerCount = cellTriggerEventNode->getAttribute("triggerCount")->getIntValue();

	if(cellTriggerEventNode->hasAttribute("destPosEnd") == true) {
		destPosEnd = Vec2i::strToVec2(cellTriggerEventNode->getAttribute("destPosEnd")->getValue());
	}
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
ScriptManager* ScriptManager::thisScriptManager		= NULL;
const int ScriptManager::messageWrapCount			= 30;
const int ScriptManager::displayTextWrapCount		= 64;

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
	currentCellTriggeredEventUnitId = 0;
	currentEventId = 0;
	inCellTriggerEvent = false;
	rootNode = NULL;
	currentCellTriggeredEventAreaEntryUnitId = 0;
	currentCellTriggeredEventAreaExitUnitId = 0;
	lastDayNightTriggerStatus = 0;
	registeredDayNightEvent = false;

	lastUnitTriggerEventUnitId = -1;
	lastUnitTriggerEventType = utet_None;
}

ScriptManager::~ScriptManager() {

}

void ScriptManager::init(World* world, GameCamera *gameCamera, const XmlNode *rootNode) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	//printf("In [%s::%s Line: %d] rootNode [%p][%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,rootNode,(rootNode != NULL ? rootNode->getName().c_str() : "none"));
	this->rootNode = rootNode;
	const Scenario*	scenario= world->getScenario();

	this->world= world;
	this->gameCamera= gameCamera;

	//set static instance
	thisScriptManager= this;

	//printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	currentEventId = 1;
	CellTriggerEventList.clear();
	TimerTriggerEventList.clear();

	//printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	//register functions
	luaScript.registerFunction(networkShowMessageForFaction, "networkShowMessageForFaction");
	luaScript.registerFunction(networkShowMessageForTeam, "networkShowMessageForTeam");

	luaScript.registerFunction(networkSetCameraPositionForFaction, "networkSetCameraPositionForFaction");
	luaScript.registerFunction(networkSetCameraPositionForTeam, "networkSetCameraPositionForTeam");

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

	luaScript.registerFunction(playStaticVideo, "playStaticVideo");
	//luaScript.registerFunction(playStreamingVideo, "playStreamingVideo");
	//luaScript.registerFunction(stopStreamingVideo, "stopStreamingVideo");
	luaScript.registerFunction(stopAllVideo, "stopAllVideo");

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

	luaScript.registerFunction(registerCellAreaTriggerEventForUnitToLocation, "registerCellAreaTriggerEventForUnitToLocation");
	luaScript.registerFunction(registerCellAreaTriggerEventForFactionToLocation, "registerCellAreaTriggerEventForFactionToLocation");
	luaScript.registerFunction(registerCellAreaTriggerEvent, "registerCellAreaTriggerEvent");

	luaScript.registerFunction(getCellTriggerEventCount, "getCellTriggerEventCount");
	luaScript.registerFunction(unregisterCellTriggerEvent, "unregisterCellTriggerEvent");
	luaScript.registerFunction(startTimerEvent, "startTimerEvent");
	luaScript.registerFunction(startEfficientTimerEvent, "startEfficientTimerEvent");
	luaScript.registerFunction(resetTimerEvent, "resetTimerEvent");
	luaScript.registerFunction(stopTimerEvent, "stopTimerEvent");
	luaScript.registerFunction(getTimerEventSecondsElapsed, "timerEventSecondsElapsed");
	luaScript.registerFunction(getCellTriggeredEventId, "triggeredCellEventId");
	luaScript.registerFunction(getTimerTriggeredEventId, "triggeredTimerEventId");

	luaScript.registerFunction(getCellTriggeredEventAreaEntryUnitId, "triggeredEventAreaEntryUnitId");
	luaScript.registerFunction(getCellTriggeredEventAreaExitUnitId, "triggeredEventAreaExitUnitId");

	luaScript.registerFunction(getCellTriggeredEventUnitId, "triggeredCellEventUnitId");

	luaScript.registerFunction(setRandomGenInit, "setRandomGenInit");
	luaScript.registerFunction(getRandomGen, "getRandomGen");
	luaScript.registerFunction(getWorldFrameCount, "getWorldFrameCount");

	luaScript.registerFunction(getStartLocation, "startLocation");
	luaScript.registerFunction(getIsUnitAlive, "isUnitAlive");
	luaScript.registerFunction(getUnitPosition, "unitPosition");
	luaScript.registerFunction(setUnitPosition, "setUnitPosition");

	luaScript.registerFunction(addCellMarker, "addCellMarker");
	luaScript.registerFunction(removeCellMarker, "removeCellMarker");
	luaScript.registerFunction(showMarker, "showMarker");

	luaScript.registerFunction(getUnitFaction, "unitFaction");
	luaScript.registerFunction(getUnitName, "unitName");
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

	luaScript.registerFunction(isFreeCellsOrHasUnit, "isFreeCellsOrHasUnit");
	luaScript.registerFunction(isFreeCells, "isFreeCells");

	luaScript.registerFunction(getHumanFactionId, "humanFaction");

	luaScript.registerFunction(highlightUnit, "highlightUnit");
	luaScript.registerFunction(unhighlightUnit, "unhighlightUnit");

	luaScript.registerFunction(giveStopCommand, "giveStopCommand");
	luaScript.registerFunction(selectUnit, "selectUnit");
	luaScript.registerFunction(unselectUnit, "unselectUnit");
	luaScript.registerFunction(addUnitToGroupSelection, "addUnitToGroupSelection");
	luaScript.registerFunction(recallGroupSelection, "recallGroupSelection");
	luaScript.registerFunction(removeUnitFromGroupSelection, "removeUnitFromGroupSelection");
	luaScript.registerFunction(setAttackWarningsEnabled, "setAttackWarningsEnabled");
	luaScript.registerFunction(getAttackWarningsEnabled, "getAttackWarningsEnabled");

	luaScript.registerFunction(getIsDayTime, "getIsDayTime");
	luaScript.registerFunction(getIsNightTime, "getIsNightTime");
	luaScript.registerFunction(getTimeOfDay, "getTimeOfDay");
	luaScript.registerFunction(registerDayNightEvent, "registerDayNightEvent");
	luaScript.registerFunction(unregisterDayNightEvent, "unregisterDayNightEvent");

	luaScript.registerFunction(registerUnitTriggerEvent, "registerUnitTriggerEvent");
	luaScript.registerFunction(unregisterUnitTriggerEvent, "unregisterUnitTriggerEvent");
	luaScript.registerFunction(getLastUnitTriggerEventUnitId, "lastUnitTriggerEventUnit");
	luaScript.registerFunction(getLastUnitTriggerEventType, "lastUnitTriggerEventType");
	luaScript.registerFunction(getUnitProperty, "getUnitProperty");
	luaScript.registerFunction(getUnitPropertyName, "getUnitPropertyName");
	luaScript.registerFunction(disableSpeedChange, "disableSpeedChange");
	luaScript.registerFunction(enableSpeedChange, "enableSpeedChange");
	luaScript.registerFunction(getSpeedChangeEnabled, "getSpeedChangeEnabled");

	luaScript.registerFunction(storeSaveGameData, "storeSaveGameData");
	luaScript.registerFunction(loadSaveGameData, "loadSaveGameData");

	luaScript.registerFunction(getFactionPlayerType, "getFactionPlayerType");

	//load code
	for(int i= 0; i<scenario->getScriptCount(); ++i){
		const Script* script= scenario->getScript(i);
		luaScript.loadCode("function " + script->getName() + "()" + script->getCode() + "end\n", script->getName());
	}


	//!!!
//	string data_path= getGameReadWritePath(GameConstants::path_data_CacheLookupKey);
//	if(data_path != ""){
//		endPathWithSlash(data_path);
//	}
//	string sandboxScriptFilename = data_path + "data/core/scripts/sandbox.lua";
//	string sandboxLuaCode = getFileTextContents(sandboxScriptFilename);
//
//	//luaScript.loadCode(sandboxLuaCode + "\n", "megaglest_lua_sandbox");
//	luaScript.setSandboxWrapperFunctionName("runsandboxed");
//	luaScript.setSandboxCode(sandboxLuaCode);
//	luaScript.runCode(sandboxLuaCode);

//	// Setup the lua security sandbox here
//	luaScript.beginCall("megaglest_lua_sandbox");
//	luaScript.endCall();

	//setup message box
	messageBox.init( Lang::getInstance().getString("Ok") );
	messageBox.setEnabled(false);
	messageBox.setAutoWordWrap(false);

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

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	try {
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

			if(LuaScript::getDebugModeEnabled() == true) printf("Calling onLoad luaSavedGameData.size() = %d\n",(int)luaSavedGameData.size());

			luaScript.beginCall("onLoad");
			luaScript.endCall();
		}
	}
	catch(const megaglest_runtime_error &ex) {
		//string sErrBuf = "";
		//if(ex.wantStackTrace() == true) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");
		//}
		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

// ========================== events ===============================================

void ScriptManager::onMessageBoxOk(bool popFront) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	Lang &lang= Lang::getInstance();

	for(int index = 0; messageQueue.empty() == false; ++index) {
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

				string msgText   = wrapString(messageQueue.front().getText(), messageWrapCount);
				string msgHeader = messageQueue.front().getHeader();
				if(messageQueue.front().getMessageNotTranslated() == false) {

					msgText   = wrapString(lang.getScenarioString(messageQueue.front().getText()), messageWrapCount);
					msgHeader = lang.getScenarioString(messageQueue.front().getHeader());
				}
				messageBox.setText(msgText);
				messageBox.setHeader(msgHeader);
				break;
			}
			popFront = true;
		}
	}
}

void ScriptManager::onResourceHarvested(){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	if(this->rootNode == NULL) {
		luaScript.beginCall("resourceHarvested");
		luaScript.endCall();
	}
}

void ScriptManager::onUnitCreated(const Unit* unit){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	if(this->rootNode == NULL) {
		lastCreatedUnitName= unit->getType()->getName(false);
		lastCreatedUnitId= unit->getId();
		luaScript.beginCall("unitCreated");
		luaScript.endCall();
		luaScript.beginCall("unitCreatedOfType_"+unit->getType()->getName());
		luaScript.endCall();
	}
}

void ScriptManager::onUnitDied(const Unit* unit){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	if(this->rootNode == NULL) {
		if(unit->getLastAttackerUnitId() >= 0) {
			Unit *killer = world->findUnitById(unit->getLastAttackerUnitId());

			if(killer != NULL) {
				lastAttackingUnitName= killer->getType()->getName(false);
				lastAttackingUnitId= killer->getId();

				lastDeadUnitKillerName= killer->getType()->getName(false);
				lastDeadUnitKillerId= killer->getId();
			}
			else {
				lastDeadUnitKillerName= "";
				lastDeadUnitKillerId= -1;
			}
		}

		lastAttackedUnitName= unit->getType()->getName(false);
		lastAttackedUnitId= unit->getId();

		lastDeadUnitName= unit->getType()->getName(false);
		lastDeadUnitId= unit->getId();
		lastDeadUnitCauseOfDeath = unit->getCauseOfDeath();

		luaScript.beginCall("unitDied");
		luaScript.endCall();
	}
}

void ScriptManager::onUnitAttacked(const Unit* unit) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	if(this->rootNode == NULL) {
		lastAttackedUnitName= unit->getType()->getName(false);
		lastAttackedUnitId= unit->getId();
		luaScript.beginCall("unitAttacked");
		luaScript.endCall();
	}
}

void ScriptManager::onUnitAttacking(const Unit* unit) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	if(this->rootNode == NULL) {
		lastAttackingUnitName= unit->getType()->getName(false);
		lastAttackingUnitId= unit->getId();
		luaScript.beginCall("unitAttacking");
		luaScript.endCall();
	}
}

void ScriptManager::onGameOver(bool won) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

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
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] TimerTriggerEventList.size() = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,TimerTriggerEventList.size());

	for(std::map<int,TimerTriggerEvent>::iterator iterMap = TimerTriggerEventList.begin();
		iterMap != TimerTriggerEventList.end(); ++iterMap) {

		TimerTriggerEvent &event = iterMap->second;

		if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] event.running = %d, event.startTime = %lld, event.endTime = %lld, diff = %f\n",
													__FILE__,__FUNCTION__,__LINE__,event.running,(long long int)event.startFrame,(long long int)event.endFrame,(event.endFrame - event.startFrame));

		if(event.running == true) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

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

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] movingUnit = %p, CellTriggerEventList.size() = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,movingUnit,CellTriggerEventList.size());

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
			currentCellTriggeredEventAreaEntryUnitId = 0;
			currentCellTriggeredEventAreaExitUnitId = 0;
			currentCellTriggeredEventUnitId = 0;

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
							if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
						}
						else {
							srcInDst = world->getMap()->isNextToUnitTypeCells(destUnit->getType(), destUnit->getPos(),movingUnit->getPos());
							if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] movingUnit = %d, event.type = %d, movingUnit->getPos() = %s, event.sourceId = %d, event.destId = %d, event.destPos = %s, destUnit->getPos() = %s, srcInDst = %d\n",
														__FILE__,__FUNCTION__,__LINE__,movingUnit->getId(), event.type,movingUnit->getPos().getString().c_str(),event.sourceId,event.destId, event.destPos.getString().c_str(), destUnit->getPos().getString().c_str(),srcInDst);
						}
						triggerEvent = srcInDst;
						if(triggerEvent == true) {
							currentCellTriggeredEventUnitId = movingUnit->getId();
						}
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
						if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
					}
					triggerEvent = srcInDst;

					if(triggerEvent == true) {
						currentCellTriggeredEventUnitId = movingUnit->getId();
					}
				}
			}
			break;

			case ctet_UnitAreaPos:
			{
				if(movingUnit->getId() == event.sourceId) {
					bool srcInDst = false;

					// Cache area lookup so for each unitsize and pos its done only once
					bool foundInCache = false;
					std::map<int,std::map<Vec2i,bool> >::iterator iterFind1 = event.eventLookupCache.find(movingUnit->getType()->getSize());
					if(iterFind1 != event.eventLookupCache.end()) {
						std::map<Vec2i,bool>::iterator iterFind2 = iterFind1->second.find(movingUnit->getPos());
						if(iterFind2 != iterFind1->second.end()) {
							foundInCache = true;
							srcInDst = iterFind2->second;
						}
					}

					if(foundInCache == false) {
						for(int x = event.destPos.x; srcInDst == false && x <= event.destPosEnd.x; ++x) {
							for(int y = event.destPos.y; srcInDst == false && y <= event.destPosEnd.y; ++y) {
								srcInDst = world->getMap()->isInUnitTypeCells(movingUnit->getType(), Vec2i(x,y),movingUnit->getPos());
								if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] movingUnit = %d, event.type = %d, movingUnit->getPos() = %s, event.sourceId = %d, event.destId = %d, event.destPos = %s, srcInDst = %d\n",
																	__FILE__,__FUNCTION__,__LINE__,movingUnit->getId(),event.type,movingUnit->getPos().getString().c_str(),event.sourceId,event.destId,Vec2i(x,y).getString().c_str(),srcInDst);
							}
						}

						event.eventLookupCache[movingUnit->getType()->getSize()][movingUnit->getPos()] = srcInDst;
					}

					if(srcInDst == true) {
						if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
					}
					triggerEvent = srcInDst;
					if(triggerEvent == true) {
						currentCellTriggeredEventUnitId = movingUnit->getId();
					}
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
						if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
					}
					else {
						srcInDst = world->getMap()->isNextToUnitTypeCells(destUnit->getType(), destUnit->getPos(),movingUnit->getPos());
						if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] movingUnit = %d, event.type = %d, movingUnit->getPos() = %s, event.sourceId = %d, event.destId = %d, event.destPos = %s, destUnit->getPos() = %s, srcInDst = %d\n",
														__FILE__,__FUNCTION__,__LINE__,movingUnit->getId(),event.type,movingUnit->getPos().getString().c_str(),event.sourceId,event.destId,event.destPos.getString().c_str(),destUnit->getPos().getString().c_str(),srcInDst);
					}
					triggerEvent = srcInDst;
					if(triggerEvent == true) {
						currentCellTriggeredEventUnitId = movingUnit->getId();
					}
				}
			}
			break;

			case ctet_FactionPos:
			{
				if(movingUnit->getFactionIndex() == event.sourceId) {
					//printf("ctet_FactionPos event.destPos = [%s], movingUnit->getPos() [%s]\n",event.destPos.getString().c_str(),movingUnit->getPos().getString().c_str());

					bool srcInDst = world->getMap()->isInUnitTypeCells(movingUnit->getType(), event.destPos,movingUnit->getPos());
					if(srcInDst == true) {
						if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
					}
					triggerEvent = srcInDst;
					if(triggerEvent == true) {
						currentCellTriggeredEventUnitId = movingUnit->getId();
					}
				}
			}
			break;

			case ctet_FactionAreaPos:
			{
				if(movingUnit->getFactionIndex() == event.sourceId) {
					//if(event.sourceId == 1) printf("ctet_FactionPos event.destPos = [%s], movingUnit->getPos() [%s] Unit id = %d\n",event.destPos.getString().c_str(),movingUnit->getPos().getString().c_str(),movingUnit->getId());

					bool srcInDst = false;

					// Cache area lookup so for each unitsize and pos its done only once
					bool foundInCache = false;
					std::map<int,std::map<Vec2i,bool> >::iterator iterFind1 = event.eventLookupCache.find(movingUnit->getType()->getSize());
					if(iterFind1 != event.eventLookupCache.end()) {
						std::map<Vec2i,bool>::iterator iterFind2 = iterFind1->second.find(movingUnit->getPos());
						if(iterFind2 != iterFind1->second.end()) {
							foundInCache = true;
							srcInDst = iterFind2->second;
						}
					}

					if(foundInCache == false) {
						for(int x = event.destPos.x; srcInDst == false && x <= event.destPosEnd.x; ++x) {
							for(int y = event.destPos.y; srcInDst == false && y <= event.destPosEnd.y; ++y) {

								srcInDst = world->getMap()->isInUnitTypeCells(movingUnit->getType(), Vec2i(x,y),movingUnit->getPos());
								if(srcInDst == true) {
									if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
								}
							}
						}

						event.eventLookupCache[movingUnit->getType()->getSize()][movingUnit->getPos()] = srcInDst;
					}

					triggerEvent = srcInDst;
					if(triggerEvent == true) {
						//printf("!!!UNIT IN AREA!!! Faction area pos, moving unit faction= %d, trigger faction = %d, unit id = %d\n",movingUnit->getFactionIndex(),event.sourceId,movingUnit->getId());
						currentCellTriggeredEventUnitId = movingUnit->getId();
					}
				}
			}
			break;

			case ctet_AreaPos:
			{
				// Is the unit already in the cell range? If no check if they are entering it
				if(event.eventStateInfo.find(movingUnit->getId()) == event.eventStateInfo.end()) {
					//printf("ctet_FactionPos event.destPos = [%s], movingUnit->getPos() [%s]\n",event.destPos.getString().c_str(),movingUnit->getPos().getString().c_str());

					bool srcInDst = false;
					for(int x = event.destPos.x; srcInDst == false && x <= event.destPosEnd.x; ++x) {
						for(int y = event.destPos.y; srcInDst == false && y <= event.destPosEnd.y; ++y) {

							srcInDst = world->getMap()->isInUnitTypeCells(movingUnit->getType(), Vec2i(x,y),movingUnit->getPos());
							if(srcInDst == true) {
								if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

								currentCellTriggeredEventAreaEntryUnitId = movingUnit->getId();
								event.eventStateInfo[movingUnit->getId()] = Vec2i(x,y).getString();
							}
						}
					}
					triggerEvent = srcInDst;
					if(triggerEvent == true) {
						currentCellTriggeredEventUnitId = movingUnit->getId();
					}
				}
				// If unit is already in cell range check if they are leaving?
				else {
					bool srcInDst = false;
					for(int x = event.destPos.x; srcInDst == false && x <= event.destPosEnd.x; ++x) {
						for(int y = event.destPos.y; srcInDst == false && y <= event.destPosEnd.y; ++y) {

							srcInDst = world->getMap()->isInUnitTypeCells(movingUnit->getType(), Vec2i(x,y),movingUnit->getPos());
							if(srcInDst == true) {
								if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

								//event.eventStateInfo[movingUnit->getId()] = Vec2i(x,y);
							}
						}
					}
					triggerEvent = (srcInDst == false);
					if(triggerEvent == true) {
						currentCellTriggeredEventUnitId = movingUnit->getId();
					}

					if(triggerEvent == true) {
						currentCellTriggeredEventAreaExitUnitId = movingUnit->getId();

						event.eventStateInfo.erase(movingUnit->getId());
					}
				}
			}
			break;

			}

			if(triggerEvent == true) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

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

string ScriptManager::wrapString(const string &str, int wrapCount) {
	string returnString;

	int letterCount= 0;
	for(int i= 0; i < (int)str.size(); ++i){
		if(letterCount>wrapCount && str[i]==' ') {
			returnString += '\n';
			letterCount= 0;
		}
		else
		{
			returnString += str[i];
		}
		++letterCount;
	}

	return returnString;
}

void ScriptManager::networkShowMessageForFaction(const string &text, const string &header,int factionIndex) {
	messageQueue.push_back(ScriptManagerMessage(text, header, factionIndex));
	thisScriptManager->onMessageBoxOk(false);
}
void ScriptManager::networkShowMessageForTeam(const string &text, const string &header,int teamIndex) {
	// Team indexes are 0 based internally (but 1 based in the lua script) so convert
	teamIndex--;
	messageQueue.push_back(ScriptManagerMessage(text, header, -1, teamIndex));
	thisScriptManager->onMessageBoxOk(false);
}

void ScriptManager::networkSetCameraPositionForFaction(int factionIndex, const Vec2i &pos) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	if(factionIndex == this->world->getThisFactionIndex()) {
		gameCamera->centerXZ(pos.x, pos.y);
	}
}

void ScriptManager::networkSetCameraPositionForTeam(int teamIndex, const Vec2i &pos) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	if(teamIndex == this->world->getThisTeamIndex()) {
		gameCamera->centerXZ(pos.x, pos.y);
	}
}

void ScriptManager::showMessage(const string &text, const string &header){
	messageQueue.push_back(ScriptManagerMessage(text, header));
	thisScriptManager->onMessageBoxOk(false);
}

void ScriptManager::clearDisplayText(){
	displayText= "";
}

void ScriptManager::setDisplayText(const string &text){
	displayText= wrapString(Lang::getInstance().getScenarioString(text), displayTextWrapCount);
}

void ScriptManager::addConsoleText(const string &text){
	world->addConsoleText(text);
}
void ScriptManager::addConsoleLangText(const char *fmt, ...){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	
    va_list argList;
    va_start(argList, fmt);

    const int max_debug_buffer_size = 8096;
    char szBuf[max_debug_buffer_size]="";
    vsnprintf(szBuf,max_debug_buffer_size-1,fmt, argList);

	world->addConsoleTextWoLang(szBuf);
	va_end(argList);
	
}

void ScriptManager::DisplayFormattedText(const char *fmt, ...) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

    va_list argList;
    va_start(argList, fmt);

    const int max_debug_buffer_size = 8096;
    char szBuf[max_debug_buffer_size]="";
    vsnprintf(szBuf,max_debug_buffer_size-1,fmt, argList);

    displayText=szBuf;

	va_end(argList);
}
void ScriptManager::DisplayFormattedLangText(const char *fmt, ...) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

    va_list argList;
    va_start(argList, fmt);

    const int max_debug_buffer_size = 8096;
    char szBuf[max_debug_buffer_size]="";
    vsnprintf(szBuf,max_debug_buffer_size-1,fmt, argList);

    displayText=szBuf;

	va_end(argList);
}
void ScriptManager::setCameraPosition(const Vec2i &pos){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	gameCamera->centerXZ(pos.x, pos.y);
}

void ScriptManager::createUnit(const string &unitName, int factionIndex, Vec2i pos){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] unit [%s] factionIndex = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,unitName.c_str(),factionIndex);
	world->createUnit(unitName, factionIndex, pos);
}

void ScriptManager::createUnitNoSpacing(const string &unitName, int factionIndex, Vec2i pos){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] unit [%s] factionIndex = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,unitName.c_str(),factionIndex);
	world->createUnit(unitName, factionIndex, pos, false);
}

void ScriptManager::destroyUnit(int unitId){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] unit [%d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,unitId);
	Unit *unit = world->findUnitById(unitId);
	if(unit != NULL) {
		// Make sure they die
		bool unit_dead = unit->decHp(unit->getHp() * unit->getHp());
		if(unit_dead == false) {
			throw megaglest_runtime_error("unit_dead == false",true);
		}
		unit->kill();
		// If called from an existing die event we get a stack overflow
		//onUnitDied(unit);
	}
}
void ScriptManager::giveKills (int unitId, int amount){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] unit [%d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,unitId);
	Unit *unit = world->findUnitById(unitId);
	if(unit != NULL) {
		for(int index = 1; index <= amount; ++index) {
			unit->incKills(-1);
		}
	}
}

void ScriptManager::playStaticSound(const string &playSound) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] playSound [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,playSound.c_str());
	world->playStaticSound(playSound);
}
void ScriptManager::playStreamingSound(const string &playSound) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] playSound [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,playSound.c_str());
	world->playStreamingSound(playSound);
}

void ScriptManager::stopStreamingSound(const string &playSound) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] playSound [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,playSound.c_str());
	world->stopStreamingSound(playSound);
}

void ScriptManager::stopAllSound() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	world->stopAllSound();
}

void ScriptManager::playStaticVideo(const string &playVideo) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] playVideo [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,playVideo.c_str());
	world->playStaticVideo(playVideo);
}
void ScriptManager::playStreamingVideo(const string &playVideo) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] playVideo [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,playVideo.c_str());
	world->playStreamingVideo(playVideo);
}

void ScriptManager::stopStreamingVideo(const string &playVideo) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] playVideo [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,playVideo.c_str());
	world->stopStreamingVideo(playVideo);
}

void ScriptManager::stopAllVideo() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	world->stopAllVideo();
}

void ScriptManager::togglePauseGame(int pauseStatus) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] pauseStatus = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,pauseStatus);
	world->togglePauseGame((pauseStatus != 0),true);
}

void ScriptManager::morphToUnit(int unitId,const string &morphName, int ignoreRequirements) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] unit [%d] morphName [%s] forceUpgradesIfRequired = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,unitId,morphName.c_str(),ignoreRequirements);

	world->morphToUnit(unitId,morphName,(ignoreRequirements == 1));
}

void ScriptManager::moveToUnit(int unitId,int destUnitId) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] unit [%d] destUnitId [%d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,unitId,destUnitId);

	world->moveToUnit(unitId,destUnitId);
}

void ScriptManager::giveResource(const string &resourceName, int factionIndex, int amount){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	world->giveResource(resourceName, factionIndex, amount);
}

void ScriptManager::givePositionCommand(int unitId, const string &commandName, const Vec2i &pos){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	world->givePositionCommand(unitId, commandName, pos);
}

void ScriptManager::giveAttackCommand(int unitId, int unitToAttackId) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	world->giveAttackCommand(unitId, unitToAttackId);
}

void ScriptManager::giveProductionCommand(int unitId, const string &producedName){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	world->giveProductionCommand(unitId, producedName);
}

void ScriptManager::giveUpgradeCommand(int unitId, const string &producedName){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	world->giveUpgradeCommand(unitId, producedName);
}

void ScriptManager::giveAttackStoppedCommand(int unitId, const string &itemName,int ignoreRequirements) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	world->giveAttackStoppedCommand(unitId, itemName, (ignoreRequirements == 1));
}

void ScriptManager::disableAi(int factionIndex){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	if(factionIndex >= 0 && factionIndex < GameConstants::maxPlayers) {
		playerModifiers[factionIndex].disableAi();
		disableConsume(factionIndex); // by this we stay somehow compatible with old behaviour
	}
}

void ScriptManager::enableAi(int factionIndex){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	if(factionIndex >= 0 && factionIndex < GameConstants::maxPlayers) {
		playerModifiers[factionIndex].enableAi();
	}
}

bool ScriptManager::getAiEnabled(int factionIndex) {

	if(factionIndex >= 0 && factionIndex < GameConstants::maxPlayers) {
		return playerModifiers[factionIndex].getAiEnabled();
	}
	return false;
}

void ScriptManager::disableConsume(int factionIndex) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	if(factionIndex >= 0 && factionIndex < GameConstants::maxPlayers) {
		playerModifiers[factionIndex].disableConsume();
	}
}

void ScriptManager::enableConsume(int factionIndex) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	if(factionIndex >= 0 && factionIndex < GameConstants::maxPlayers) {
		playerModifiers[factionIndex].enableConsume();
	}
}

bool ScriptManager::getConsumeEnabled(int factionIndex) {

	if(factionIndex >= 0 && factionIndex < GameConstants::maxPlayers) {
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

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] Unit: %d will trigger cell event when reaching unit: %d, eventId = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,sourceUnitId,destUnitId,eventId);

	return eventId;
}

int ScriptManager::registerCellTriggerEventForUnitToLocation(int sourceUnitId, const Vec2i &pos) {
	CellTriggerEvent trigger;
	trigger.type = ctet_UnitPos;
	trigger.sourceId = sourceUnitId;
	trigger.destPos = pos;

	int eventId = currentEventId++;
	CellTriggerEventList[eventId] = trigger;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] Unit: %d will trigger cell event when reaching pos: %s, eventId = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,sourceUnitId,pos.getString().c_str(),eventId);

	return eventId;
}

int ScriptManager::registerCellAreaTriggerEventForUnitToLocation(int sourceUnitId, const Vec4i &pos) {
	CellTriggerEvent trigger;
	trigger.type = ctet_UnitAreaPos;
	trigger.sourceId = sourceUnitId;
	trigger.destPos.x = pos.x;
	trigger.destPos.y = pos.y;
	trigger.destPosEnd.x = pos.z;
	trigger.destPosEnd.y = pos.w;

	int eventId = currentEventId++;
	CellTriggerEventList[eventId] = trigger;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] Unit: %d will trigger cell event when reaching pos: %s, eventId = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,sourceUnitId,pos.getString().c_str(),eventId);

	return eventId;
}

int ScriptManager::registerCellTriggerEventForFactionToUnit(int sourceFactionId, int destUnitId) {
	CellTriggerEvent trigger;
	trigger.type = ctet_Faction;
	trigger.sourceId = sourceFactionId;
	trigger.destId = destUnitId;

	int eventId = currentEventId++;
	CellTriggerEventList[eventId] = trigger;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] Faction: %d will trigger cell event when reaching unit: %d, eventId = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,sourceFactionId,destUnitId,eventId);

	return eventId;
}

int ScriptManager::registerCellTriggerEventForFactionToLocation(int sourceFactionId, const Vec2i &pos) {
	CellTriggerEvent trigger;
	trigger.type = ctet_FactionPos;
	trigger.sourceId = sourceFactionId;
	trigger.destPos = pos;

	int eventId = currentEventId++;
	CellTriggerEventList[eventId] = trigger;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]Faction: %d will trigger cell event when reaching pos: %s, eventId = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,sourceFactionId,pos.getString().c_str(),eventId);

	return eventId;
}

int ScriptManager::registerCellAreaTriggerEventForFactionToLocation(int sourceFactionId, const Vec4i &pos) {
	CellTriggerEvent trigger;
	trigger.type = ctet_FactionAreaPos;
	trigger.sourceId = sourceFactionId;
	trigger.destPos.x = pos.x;
	trigger.destPos.y = pos.y;
	trigger.destPosEnd.x = pos.z;
	trigger.destPosEnd.y = pos.w;

	int eventId = currentEventId++;
	CellTriggerEventList[eventId] = trigger;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]Faction: %d will trigger cell event when reaching pos: %s, eventId = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,sourceFactionId,pos.getString().c_str(),eventId);

	return eventId;
}

int ScriptManager::registerCellAreaTriggerEvent(const Vec4i &pos) {
	CellTriggerEvent trigger;
	trigger.type = ctet_AreaPos;
	trigger.sourceId = -1;
	trigger.destPos.x = pos.x;
	trigger.destPos.y = pos.y;
	trigger.destPosEnd.x = pos.z;
	trigger.destPosEnd.y = pos.w;

	int eventId = currentEventId++;
	CellTriggerEventList[eventId] = trigger;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] trigger cell event when reaching pos: %s, eventId = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,pos.getString().c_str(),eventId);

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
			for(int i = 0; i < (int)unRegisterCellTriggerEventList.size(); ++i) {
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

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] TimerTriggerEventList.size() = %d, eventId = %d, trigger.startTime = %lld, trigger.endTime = %lld\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,TimerTriggerEventList.size(),eventId,(long long int)trigger.startFrame,(long long int)trigger.endFrame);

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

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] TimerTriggerEventList.size() = %d, eventId = %d, trigger.startTime = %lld, trigger.endTime = %lld\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,TimerTriggerEventList.size(),eventId,(long long int)trigger.startFrame,(long long int)trigger.endFrame);

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

		if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] TimerTriggerEventList.size() = %d, eventId = %d, trigger.startTime = %lld, trigger.endTime = %lld, result = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,TimerTriggerEventList.size(),eventId,(long long int)trigger.startFrame,(long long int)trigger.endFrame,result);
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

		if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] TimerTriggerEventList.size() = %d, eventId = %d, trigger.startTime = %lld, trigger.endTime = %lld, result = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,TimerTriggerEventList.size(),eventId,(long long int)trigger.startFrame,(long long int)trigger.endFrame,result);
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
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);


	if(factionIndex >= 0 && factionIndex < GameConstants::maxPlayers) {
		playerModifiers[factionIndex].setAsWinner();
	}
}

void ScriptManager::endGame() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	gameOver= true;
}

void ScriptManager::startPerformanceTimer() {

	if(world->getGame() == NULL) {
		throw megaglest_runtime_error("#1 world->getGame() == NULL",true);
	}
	world->getGame()->startPerformanceTimer();

}

void ScriptManager::endPerformanceTimer() {

	if(world->getGame() == NULL) {
		throw megaglest_runtime_error("#2 world->getGame() == NULL",true);
	}
	world->getGame()->endPerformanceTimer();

}

Vec2i ScriptManager::getPerformanceTimerResults() {

	if(world->getGame() == NULL) {
		throw megaglest_runtime_error("#3 world->getGame() == NULL",true);
	}
	return world->getGame()->getPerformanceTimerResults();
}

Vec2i ScriptManager::getStartLocation(int factionIndex) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	return world->getStartLocation(factionIndex);
}


Vec2i ScriptManager::getUnitPosition(int unitId) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	Vec2i result = world->getUnitPosition(unitId);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s] unitId = %d, pos [%s]\n",__FUNCTION__,unitId,result.getString().c_str());

	return result;
}

void ScriptManager::setUnitPosition(int unitId, Vec2i pos) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	return world->setUnitPosition(unitId,pos);
}

void ScriptManager::addCellMarker(Vec2i pos, int factionIndex, const string &note, const string &textureFile) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	world->addCellMarker(pos,factionIndex, note, textureFile);
}

void ScriptManager::removeCellMarker(Vec2i pos, int factionIndex) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	return world->removeCellMarker(pos,factionIndex);
}

void ScriptManager::showMarker(Vec2i pos, int factionIndex, const string &note, const string &textureFile, int flashCount) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	world->showMarker(pos,factionIndex, note, textureFile, flashCount);
}

int ScriptManager::getIsUnitAlive(int unitId) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	return world->getIsUnitAlive(unitId);
}

int ScriptManager::getUnitFaction(int unitId) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	return world->getUnitFactionIndex(unitId);
}
const string ScriptManager::getUnitName(int unitId) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	return world->getUnitName(unitId);
}

int ScriptManager::getResourceAmount(const string &resourceName, int factionIndex) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	return world->getResourceAmount(resourceName, factionIndex);
}

const string &ScriptManager::getLastCreatedUnitName() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	return lastCreatedUnitName;
}

int ScriptManager::getCellTriggeredEventId() {

	return currentCellTriggeredEventId;
}

int ScriptManager::getTimerTriggeredEventId() {

	return currentTimerTriggeredEventId;
}

int ScriptManager::getCellTriggeredEventAreaEntryUnitId() {

	return currentCellTriggeredEventAreaEntryUnitId;
}

int ScriptManager::getCellTriggeredEventAreaExitUnitId() {

	return currentCellTriggeredEventAreaExitUnitId;
}

int ScriptManager::getCellTriggeredEventUnitId() {

	return currentCellTriggeredEventUnitId;
}

void ScriptManager::setRandomGenInit(int seed) {

	random.init(seed);
}

int ScriptManager::getRandomGen(int minVal, int maxVal) {

	return random.randRange(minVal,maxVal);
}

int ScriptManager::getWorldFrameCount() {

	return world->getFrameCount();
}

bool ScriptManager::getGameWon() const {

	return gameWon;
}

bool ScriptManager::getIsGameOver() const {

	return gameOver;
}

int ScriptManager::getLastCreatedUnitId() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	return lastCreatedUnitId;
}

const string &ScriptManager::getLastDeadUnitName() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	return lastDeadUnitName;
}

int ScriptManager::getLastDeadUnitId() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	return lastDeadUnitId;
}

int ScriptManager::getLastDeadUnitCauseOfDeath() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	return lastDeadUnitCauseOfDeath;
}

const string &ScriptManager::getLastDeadUnitKillerName() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	return lastDeadUnitKillerName;
}

int ScriptManager::getLastDeadUnitKillerId() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	return lastDeadUnitKillerId;
}

const string &ScriptManager::getLastAttackedUnitName() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	return lastAttackedUnitName;
}

int ScriptManager::getLastAttackedUnitId() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	return lastAttackedUnitId;
}

const string &ScriptManager::getLastAttackingUnitName() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	return lastAttackingUnitName;
}

int ScriptManager::getLastAttackingUnitId() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	return lastAttackingUnitId;
}

int ScriptManager::getUnitCount(int factionIndex) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	return world->getUnitCount(factionIndex);
}

int ScriptManager::getUnitCountOfType(int factionIndex, const string &typeName) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	return world->getUnitCountOfType(factionIndex, typeName);
}

const string ScriptManager::getSystemMacroValue(const string &key) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	return world->getSystemMacroValue(key);
}

const string ScriptManager::getPlayerName(int factionIndex) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	return world->getPlayerName(factionIndex);
}

void ScriptManager::loadScenario(const string &name, bool keepFactions) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);


	world->setQueuedScenario(name,keepFactions);
}

vector<int> ScriptManager::getUnitsForFaction(int factionIndex,const string& commandTypeName, int field) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);


	return world->getUnitsForFaction(factionIndex,commandTypeName, field);
}

int ScriptManager::getUnitCurrentField(int unitId) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);


	return world->getUnitCurrentField(unitId);
}

int ScriptManager::isFreeCellsOrHasUnit(int field, int unitId, Vec2i pos) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);


	Unit* unit= world->findUnitById(unitId);
	if(unit == NULL) {
		throw megaglest_runtime_error("unit == NULL",true);
	}
	int result = world->getMap()->isFreeCellsOrHasUnit(pos,unit->getType()->getSize(),static_cast<Field>(field),unit,NULL,true);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s] unitId = %d, [%s] pos [%s] field = %d result = %d\n",__FUNCTION__,unitId,unit->getType()->getName(false).c_str(),pos.getString().c_str(),field,result);

	return result;
}

int ScriptManager::isFreeCells(int unitSize, int field, Vec2i pos) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	int result = world->getMap()->isFreeCellsOrHasUnit(pos,unitSize,static_cast<Field>(field),NULL,NULL,true);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s] unitSize = %d, pos [%s] field = %d result = %d\n",__FUNCTION__,unitSize,pos.getString().c_str(),field,result);

	return result;
}

int ScriptManager::getHumanFactionId() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	return this->world->getThisFactionIndex();
}

void ScriptManager::highlightUnit(int unitId, float radius, float thickness, Vec4f color) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);


	world->highlightUnit(unitId, radius, thickness, color);
}

void ScriptManager::unhighlightUnit(int unitId) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);


	world->unhighlightUnit(unitId);
}

void ScriptManager::giveStopCommand(int unitId) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	world->giveStopCommand(unitId);
}

bool ScriptManager::selectUnit(int unitId) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	return world->selectUnit(unitId);
}

void ScriptManager::unselectUnit(int unitId) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	world->unselectUnit(unitId);
}

void ScriptManager::addUnitToGroupSelection(int unitId,int groupIndex) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	world->addUnitToGroupSelection(unitId,groupIndex);
}
void ScriptManager::recallGroupSelection(int groupIndex) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	world->recallGroupSelection(groupIndex);
}
void ScriptManager::removeUnitFromGroupSelection(int unitId,int groupIndex) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	world->removeUnitFromGroupSelection(unitId,groupIndex);
}

void ScriptManager::setAttackWarningsEnabled(bool enabled) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	world->setAttackWarningsEnabled(enabled);
}

bool ScriptManager::getAttackWarningsEnabled() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	return world->getAttackWarningsEnabled();
}

void ScriptManager::registerUnitTriggerEvent(int unitId) {
	UnitTriggerEventList[unitId]=utet_None;
}

void ScriptManager::unregisterUnitTriggerEvent(int unitId) {
	UnitTriggerEventList.erase(unitId);
}

int ScriptManager::getLastUnitTriggerEventUnitId() {
	return lastUnitTriggerEventUnitId;
}
UnitTriggerEventType ScriptManager::getLastUnitTriggerEventType() {
	return lastUnitTriggerEventType;
}

int ScriptManager::getUnitProperty(int unitId, UnitTriggerEventType type) {
	int result = -1;

	//printf("File: %s line: %d type: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,type);

	Unit *unit= world->findUnitById(unitId);
	if(unit != NULL) {
		switch(type) {
			case utet_None:
				result = -2;
			break;
			case utet_HPChanged:
				result = unit->getHp();
			break;
			case utet_EPChanged:
				result = unit->getEp();
			break;
			case utet_LevelChanged:
				result = -3;
				if(unit->getLevel() != NULL) {
					result = unit->getLevel()->getKills();
				}
			break;
			case utet_FieldChanged:
				result = unit->getCurrField();
			break;
			case utet_SkillChanged:
				result = -4;
				if(unit->getCurrSkill() != NULL) {
					result = unit->getCurrSkill()->getClass();
				}
			break;
			default:
				result = -1000;
				break;
		}
	}
	//printf("File: %s line: %d result: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,result);
	return result;
}
const string ScriptManager::getUnitPropertyName(int unitId, UnitTriggerEventType type) {
	string result = "";
	Unit *unit= world->findUnitById(unitId);
	if(unit != NULL) {
		switch(type) {
			case utet_None:
				result = "";
			break;
			case utet_HPChanged:
				result = "";
			break;
			case utet_EPChanged:
				result = "";
			break;
			case utet_LevelChanged:
				result = "";
				if(unit->getLevel() != NULL) {
					result = unit->getLevel()->getName(false);
				}
			break;
			case utet_FieldChanged:
				result = "";
			break;
			case utet_SkillChanged:
				result = "";
				if(unit->getCurrSkill() != NULL) {
					result = unit->getCurrSkill()->getName();
				}
			break;
			default:
				result = "???";
				break;
		}
	}
	return result;
}

void ScriptManager::onUnitTriggerEvent(const Unit *unit, UnitTriggerEventType event) {
	//static bool inEvent = false;
	//if(inEvent == true) {
	//	printf("\n\n!!!!!!!!!!!!!!! File: %s line: %d unit [%d - %s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,unit->getId(),unit->getType()->getName().c_str());
	//	return;
	//}
	//inEvent = true;
	//printf("File: %s line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__);
	if(UnitTriggerEventList.empty() == false) {
		//printf("File: %s line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__);
		std::map<int,UnitTriggerEventType>::iterator iterFind = UnitTriggerEventList.find(unit->getId());
		if(iterFind != UnitTriggerEventList.end()) {
			//printf("File: %s line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__);

			lastUnitTriggerEventUnitId = unit->getId();
			lastUnitTriggerEventType = event;

			//printf("File: %s line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__);

			luaScript.beginCall("unitTriggerEvent");
			luaScript.endCall();

			//printf("File: %s line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__);
		}
	}
	//inEvent = false;
	//printf("File: %s line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__);
}

void ScriptManager::registerDayNightEvent() {
	registeredDayNightEvent = true;
}

void ScriptManager::unregisterDayNightEvent() {
	registeredDayNightEvent = false;
}

void ScriptManager::onDayNightTriggerEvent() {
	if(registeredDayNightEvent == true) {
		bool isDay = (this->getIsDayTime() == 1);
		if((lastDayNightTriggerStatus != 1 && isDay == true) ||
			(lastDayNightTriggerStatus != 2 && isDay == false)) {
			if(isDay == true) {
				lastDayNightTriggerStatus = 1;
			}
			else {
				lastDayNightTriggerStatus = 2;
			}

			printf("Triggering daynight event isDay: %d [%f]\n",isDay,getTimeOfDay());

			luaScript.beginCall("dayNightTriggerEvent");
			luaScript.endCall();
		}
	}
}

int ScriptManager::getIsDayTime() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);


	const TimeFlow *tf= world->getTimeFlow();
	if(tf == NULL) {
		throw megaglest_runtime_error("#1 tf == NULL",true);
	}
	return tf->isDay();
}
int ScriptManager::getIsNightTime() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);


	const TimeFlow *tf= world->getTimeFlow();
	if(tf == NULL) {
		throw megaglest_runtime_error("#2 tf == NULL",true);
	}
	return tf->isNight();
}
float ScriptManager::getTimeOfDay() {
	//printf("File: %s line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);


	const TimeFlow *tf= world->getTimeFlow();
	if(tf == NULL) {
		throw megaglest_runtime_error("#3 tf == NULL",true);
	}
	//printf("File: %s line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__);
	return tf->getTime();
}

void ScriptManager::disableSpeedChange() {
	if(world->getGame() == NULL) {
		throw megaglest_runtime_error("#4 world->getGame() == NULL");
	}
	world->getGame()->setDisableSpeedChange(true);
}
void ScriptManager::enableSpeedChange() {
	if(world->getGame() == NULL) {
		throw megaglest_runtime_error("#5 world->getGame() == NULL");
	}
	world->getGame()->setDisableSpeedChange(false);
}

bool ScriptManager::getSpeedChangeEnabled() {
	if(world->getGame() == NULL) {
		throw megaglest_runtime_error("#6 world->getGame() == NULL");
	}
	return world->getGame()->getDisableSpeedChange();
}

void ScriptManager::addMessageToQueue(ScriptManagerMessage msg) {
	messageQueue.push_back(msg);
}

void ScriptManager::storeSaveGameData(string name, string value) {

	if(LuaScript::getDebugModeEnabled() == true) printf("storeSaveGameData name [%s] value [%s]\n",name.c_str(),value.c_str());

	luaSavedGameData[name] = value;
}

string ScriptManager::loadSaveGameData(string name) {
	string value = luaSavedGameData[name];

	if(LuaScript::getDebugModeEnabled() == true) printf("loadSaveGameData result name [%s] value [%s]\n",name.c_str(),value.c_str());

	return value;
}

ControlType ScriptManager::getFactionPlayerType(int factionIndex) {
	Faction *faction = world->getFaction(factionIndex);
	if(faction != NULL) {
		return faction->getControlType();
	}
	return ctClosed;
}
// ========================== lua callbacks ===============================================

int ScriptManager::showMessage(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	thisScriptManager->showMessage(luaArguments.getString(-2), luaArguments.getString(-1));
	return luaArguments.getReturnCount();
}

int ScriptManager::networkShowMessageForFaction(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);

	try {
		thisScriptManager->networkShowMessageForFaction(luaArguments.getString(-3), luaArguments.getString(-2), luaArguments.getInt(-1));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::networkShowMessageForTeam(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);

	try {
		thisScriptManager->networkShowMessageForTeam(luaArguments.getString(-3), luaArguments.getString(-2), luaArguments.getInt(-1));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::networkSetCameraPositionForFaction(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);

	try {
		thisScriptManager->networkSetCameraPositionForFaction(luaArguments.getInt(-2), luaArguments.getVec2i(-1));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::networkSetCameraPositionForTeam(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);

	try {
		thisScriptManager->networkSetCameraPositionForTeam(luaArguments.getInt(-2), luaArguments.getVec2i(-1));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}


int ScriptManager::setDisplayText(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);

	try {
		thisScriptManager->setDisplayText(luaArguments.getString(-1));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::addConsoleText(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);

	try {
		thisScriptManager->addConsoleText(luaArguments.getString(-1));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::clearDisplayText(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);

	try {
		thisScriptManager->clearDisplayText();
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::setCameraPosition(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);

	try {
		thisScriptManager->setCameraPosition(Vec2i(luaArguments.getVec2i(-1)));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::createUnit(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] unit [%s] factionIndex = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,luaArguments.getString(-3).c_str(),luaArguments.getInt(-2));

	try {
		thisScriptManager->createUnit(
			luaArguments.getString(-3),
			luaArguments.getInt(-2),
			luaArguments.getVec2i(-1));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::createUnitNoSpacing(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] unit [%s] factionIndex = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,luaArguments.getString(-3).c_str(),luaArguments.getInt(-2));

	try {
		thisScriptManager->createUnitNoSpacing(
			luaArguments.getString(-3),
			luaArguments.getInt(-2),
			luaArguments.getVec2i(-1));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::destroyUnit(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] unit [%d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,luaArguments.getInt(-1));

	try {
		thisScriptManager->destroyUnit(luaArguments.getInt(-1));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}
int ScriptManager::giveKills(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] unit [%d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,luaArguments.getInt(-1));

	try {
		thisScriptManager->giveKills(luaArguments.getInt(-2),luaArguments.getInt(-1));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::morphToUnit(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] unit [%d] morphName [%s] forceUpgrade = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,luaArguments.getInt(-3),luaArguments.getString(-2).c_str(),luaArguments.getInt(-1));

	try {
		thisScriptManager->morphToUnit(luaArguments.getInt(-3),luaArguments.getString(-2),luaArguments.getInt(-1));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::moveToUnit(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] unit [%d] dest unit [%d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,luaArguments.getInt(-2),luaArguments.getInt(-1));

	try {
		thisScriptManager->moveToUnit(luaArguments.getInt(-2),luaArguments.getInt(-1));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::playStaticSound(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] sound [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,luaArguments.getString(-1).c_str());

	try {
		thisScriptManager->playStaticSound(luaArguments.getString(-1));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::playStreamingSound(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] sound [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,luaArguments.getString(-1).c_str());

	try {
		thisScriptManager->playStreamingSound(luaArguments.getString(-1));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::stopStreamingSound(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] sound [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,luaArguments.getString(-1).c_str());

	try {
		thisScriptManager->stopStreamingSound(luaArguments.getString(-1));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::stopAllSound(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	try {
		thisScriptManager->stopAllSound();
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::playStaticVideo(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] sound [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,luaArguments.getString(-1).c_str());

	try {
		thisScriptManager->playStaticVideo(luaArguments.getString(-1));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::playStreamingVideo(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] sound [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,luaArguments.getString(-1).c_str());

	try {
		thisScriptManager->playStreamingVideo(luaArguments.getString(-1));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::stopStreamingVideo(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] sound [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,luaArguments.getString(-1).c_str());

	try {
		thisScriptManager->stopStreamingVideo(luaArguments.getString(-1));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::stopAllVideo(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	try {
		thisScriptManager->stopAllVideo();
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::togglePauseGame(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] value = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,luaArguments.getInt(-1));

	try {
		thisScriptManager->togglePauseGame(luaArguments.getInt(-1));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}
int ScriptManager::giveResource(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	try {
		thisScriptManager->giveResource(luaArguments.getString(-3), luaArguments.getInt(-2), luaArguments.getInt(-1));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::givePositionCommand(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	try {
		thisScriptManager->givePositionCommand(
			luaArguments.getInt(-3),
			luaArguments.getString(-2),
			luaArguments.getVec2i(-1));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::giveAttackCommand(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	try {
		thisScriptManager->giveAttackCommand(
			luaArguments.getInt(-2),
			luaArguments.getInt(-1));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::giveProductionCommand(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	try {
		thisScriptManager->giveProductionCommand(
			luaArguments.getInt(-2),
			luaArguments.getString(-1));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::giveUpgradeCommand(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	try {
		thisScriptManager->giveUpgradeCommand(
			luaArguments.getInt(-2),
			luaArguments.getString(-1));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::giveAttackStoppedCommand(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	try {
		thisScriptManager->giveAttackStoppedCommand(
			luaArguments.getInt(-3),
			luaArguments.getString(-2),
			luaArguments.getInt(-1));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::disableAi(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	try {
		thisScriptManager->disableAi(luaArguments.getInt(-1));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::enableAi(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	try {
		thisScriptManager->enableAi(luaArguments.getInt(-1));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::getAiEnabled(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	try {
		bool result = thisScriptManager->getAiEnabled(luaArguments.getInt(-1));
		luaArguments.returnInt(result);
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::disableConsume(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	try {
		thisScriptManager->disableConsume(luaArguments.getInt(-1));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::enableConsume(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	try {
		thisScriptManager->enableConsume(luaArguments.getInt(-1));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::getConsumeEnabled(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	try {
		bool result = thisScriptManager->getConsumeEnabled(luaArguments.getInt(-1));
		luaArguments.returnInt(result);
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::registerCellTriggerEventForUnitToUnit(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		int result = thisScriptManager->registerCellTriggerEventForUnitToUnit(luaArguments.getInt(-2),luaArguments.getInt(-1));
		luaArguments.returnInt(result);
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::registerCellTriggerEventForUnitToLocation(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		int result = thisScriptManager->registerCellTriggerEventForUnitToLocation(luaArguments.getInt(-2),luaArguments.getVec2i(-1));
		luaArguments.returnInt(result);
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::registerCellAreaTriggerEventForUnitToLocation(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		int result = thisScriptManager->registerCellAreaTriggerEventForUnitToLocation(luaArguments.getInt(-2),luaArguments.getVec4i(-1));
		luaArguments.returnInt(result);
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::registerCellTriggerEventForFactionToUnit(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		int result = thisScriptManager->registerCellTriggerEventForFactionToUnit(luaArguments.getInt(-2),luaArguments.getInt(-1));
		luaArguments.returnInt(result);
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::registerCellTriggerEventForFactionToLocation(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		int result = thisScriptManager->registerCellTriggerEventForFactionToLocation(luaArguments.getInt(-2),luaArguments.getVec2i(-1));
		luaArguments.returnInt(result);
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::registerCellAreaTriggerEventForFactionToLocation(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		int result = thisScriptManager->registerCellAreaTriggerEventForFactionToLocation(luaArguments.getInt(-2),luaArguments.getVec4i(-1));
		luaArguments.returnInt(result);
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::registerCellAreaTriggerEvent(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		int result = thisScriptManager->registerCellAreaTriggerEvent(luaArguments.getVec4i(-1));
		luaArguments.returnInt(result);
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::getCellTriggerEventCount(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		int result = thisScriptManager->getCellTriggerEventCount(luaArguments.getInt(-1));
		luaArguments.returnInt(result);
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::unregisterCellTriggerEvent(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		thisScriptManager->unregisterCellTriggerEvent(luaArguments.getInt(-1));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::startTimerEvent(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		int result = thisScriptManager->startTimerEvent();
		luaArguments.returnInt(result);
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::startEfficientTimerEvent(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		int result = thisScriptManager->startEfficientTimerEvent(luaArguments.getInt(-1));
		luaArguments.returnInt(result);
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::stopTimerEvent(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		int result = thisScriptManager->stopTimerEvent(luaArguments.getInt(-1));
		luaArguments.returnInt(result);
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::resetTimerEvent(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		int result = thisScriptManager->resetTimerEvent(luaArguments.getInt(-1));
		luaArguments.returnInt(result);
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::getTimerEventSecondsElapsed(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		int result = thisScriptManager->getTimerEventSecondsElapsed(luaArguments.getInt(-1));
		luaArguments.returnInt(result);
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::setPlayerAsWinner(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	try {
		thisScriptManager->setPlayerAsWinner(luaArguments.getInt(-1));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::endGame(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	try {
		thisScriptManager->endGame();
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::startPerformanceTimer(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	try {
		thisScriptManager->startPerformanceTimer();
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::endPerformanceTimer(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	try {
		thisScriptManager->endPerformanceTimer();
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::getPerformanceTimerResults(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	try {
		Vec2i results= thisScriptManager->getPerformanceTimerResults();
		luaArguments.returnVec2i(results);
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::getStartLocation(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	try {
		Vec2i pos= thisScriptManager->getStartLocation(luaArguments.getInt(-1));
		luaArguments.returnVec2i(pos);
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::getUnitPosition(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	try {
		Vec2i pos= thisScriptManager->getUnitPosition(luaArguments.getInt(-1));
		luaArguments.returnVec2i(pos);
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::setUnitPosition(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	try {
		thisScriptManager->setUnitPosition(luaArguments.getInt(-2),luaArguments.getVec2i(-1));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::addCellMarker(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);

	try {
		//printf("LUA addCellMarker --> START\n");

		int factionIndex = luaArguments.getInt(-4);

		//printf("LUA addCellMarker --> START 1\n");

		Vec2i pos = luaArguments.getVec2i(-1);

		//printf("LUA addCellMarker --> START 2\n");

		string note = luaArguments.getString(-3);

		//printf("LUA addCellMarker --> START 3\n");

		string texture = luaArguments.getString(-2);

		//printf("LUA addCellMarker --> faction [%d] pos [%s] note [%s] texture [%s]\n",factionIndex,pos.getString().c_str(),note.c_str(),texture.c_str());

		thisScriptManager->addCellMarker(pos,factionIndex,note,texture);
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::removeCellMarker(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);

	try {
		int factionIndex = luaArguments.getInt(-2);
		Vec2i pos = luaArguments.getVec2i(-1);

		thisScriptManager->removeCellMarker(pos,factionIndex);
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::showMarker(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);

	try {
		int flashCount = luaArguments.getInt(-5);
		//printf("LUA addCellMarker --> START\n");

		int factionIndex = luaArguments.getInt(-4);

		//printf("LUA addCellMarker --> START 1\n");

		Vec2i pos = luaArguments.getVec2i(-1);

		//printf("LUA addCellMarker --> START 2\n");

		string note = luaArguments.getString(-3);

		//printf("LUA addCellMarker --> START 3\n");

		string texture = luaArguments.getString(-2);

		//printf("LUA addCellMarker --> faction [%d] pos [%s] note [%s] texture [%s]\n",factionIndex,pos.getString().c_str(),note.c_str(),texture.c_str());

		thisScriptManager->showMarker(pos,factionIndex,note,texture,flashCount);
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::getUnitFaction(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	try {
		int factionIndex= thisScriptManager->getUnitFaction(luaArguments.getInt(-1));
		luaArguments.returnInt(factionIndex);
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}
int ScriptManager::getUnitName(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	try {
		const string unitname = thisScriptManager->getUnitName(luaArguments.getInt(-1));
		luaArguments.returnString(unitname);
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::getResourceAmount(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	try {
		luaArguments.returnInt(thisScriptManager->getResourceAmount(luaArguments.getString(-2), luaArguments.getInt(-1)));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::getLastCreatedUnitName(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	try {
		luaArguments.returnString(thisScriptManager->getLastCreatedUnitName());
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::getLastCreatedUnitId(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	try {
		luaArguments.returnInt(thisScriptManager->getLastCreatedUnitId());
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::getCellTriggeredEventId(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	try {
		luaArguments.returnInt(thisScriptManager->getCellTriggeredEventId());
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::getTimerTriggeredEventId(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	try {
		luaArguments.returnInt(thisScriptManager->getTimerTriggeredEventId());
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::getCellTriggeredEventAreaEntryUnitId(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	try {
		luaArguments.returnInt(thisScriptManager->getCellTriggeredEventAreaEntryUnitId());
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::getCellTriggeredEventAreaExitUnitId(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	try {
		luaArguments.returnInt(thisScriptManager->getCellTriggeredEventAreaExitUnitId());
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::getCellTriggeredEventUnitId(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	try {
		luaArguments.returnInt(thisScriptManager->getCellTriggeredEventUnitId());
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::setRandomGenInit(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	try {
		thisScriptManager->setRandomGenInit(luaArguments.getInt(-1));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::getRandomGen(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	try {
		luaArguments.returnInt(thisScriptManager->getRandomGen(luaArguments.getInt(-2),luaArguments.getInt(-1)));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::getWorldFrameCount(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	try {
		luaArguments.returnInt(thisScriptManager->getWorldFrameCount());
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::getLastDeadUnitName(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	try {
		luaArguments.returnString(thisScriptManager->getLastDeadUnitName());
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::getLastDeadUnitId(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	try {
		luaArguments.returnInt(thisScriptManager->getLastDeadUnitId());
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::getLastDeadUnitCauseOfDeath(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	try {
		luaArguments.returnInt(thisScriptManager->getLastDeadUnitCauseOfDeath());
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::getLastDeadUnitKillerName(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	try {
		luaArguments.returnString(thisScriptManager->getLastDeadUnitKillerName());
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::getLastDeadUnitKillerId(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	try {
		luaArguments.returnInt(thisScriptManager->getLastDeadUnitKillerId());
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::getLastAttackedUnitName(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		luaArguments.returnString(thisScriptManager->getLastAttackedUnitName());
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::getLastAttackedUnitId(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		luaArguments.returnInt(thisScriptManager->getLastAttackedUnitId());
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::getLastAttackingUnitName(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		luaArguments.returnString(thisScriptManager->getLastAttackingUnitName());
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::getLastAttackingUnitId(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		luaArguments.returnInt(thisScriptManager->getLastAttackingUnitId());
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::getSystemMacroValue(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		luaArguments.returnString(thisScriptManager->getSystemMacroValue(luaArguments.getString(-1)));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::scenarioDir(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		luaArguments.returnString(thisScriptManager->getSystemMacroValue("$SCENARIO_PATH"));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::getPlayerName(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		luaArguments.returnString(thisScriptManager->getPlayerName(luaArguments.getInt(-1)));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::getUnitCount(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	try {
		luaArguments.returnInt(thisScriptManager->getUnitCount(luaArguments.getInt(-1)));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::getUnitCountOfType(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	try {
		luaArguments.returnInt(thisScriptManager->getUnitCountOfType(luaArguments.getInt(-2), luaArguments.getString(-1)));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::DisplayFormattedText(LuaHandle* luaHandle) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	try {
		//const char *ret;
		//lua_lock(luaHandle);
		//luaC_checkGC(luaHandle);

		int args = lua_gettop(luaHandle);
		if(lua_checkstack(luaHandle, args+1)) {
			LuaArguments luaArguments(luaHandle);
			string fmt = luaArguments.getString(-args);
			//va_list argList;
			//va_start(argList, fmt.c_str() );

			if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"DisplayFormattedText args = %d!\n",args);

			const int max_args_allowed = 8;
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
				char szBuf[8096]="";
				snprintf(szBuf,8096,"Invalid parameter count in method [%s] args = %d [argument count must be between 1 and %d]",__FUNCTION__,args,max_args_allowed);
				throw megaglest_runtime_error(szBuf);
			}

			//va_end(argList);
		}
		//lua_unlock(luaHandle);
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

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
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	try {
		//const char *ret;
		//lua_lock(luaHandle);
		//luaC_checkGC(luaHandle);

		int args = lua_gettop(luaHandle);
		if(lua_checkstack(luaHandle, args+1)) {
			LuaArguments luaArguments(luaHandle);
			string fmt = luaArguments.getString(-args);
			//va_list argList;
			//va_start(argList, fmt.c_str() );

			if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"DisplayFormattedText args = %d!\n",args);

			const int max_args_allowed = 8;
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
				char szBuf[8096]="";
				snprintf(szBuf,8096,"Invalid parameter count in method [%s] args = %d [argument count must be between 1 and %d]",__FUNCTION__,args,max_args_allowed);
				throw megaglest_runtime_error(szBuf);
			}

			//va_end(argList);
		}
		//lua_unlock(luaHandle);
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return 1;
}

int ScriptManager::DisplayFormattedLangText(LuaHandle* luaHandle) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	try {
		//const char *ret;
		//lua_lock(luaHandle);
		//luaC_checkGC(luaHandle);

		int args = lua_gettop(luaHandle);
		if(lua_checkstack(luaHandle, args+1)) {
			LuaArguments luaArguments(luaHandle);
			string fmt = luaArguments.getString(-args);
			//va_list argList;
			//va_start(argList, fmt.c_str() );

			if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"DisplayFormattedText args = %d!\n",args);

			const int max_args_allowed = 8;
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
				char szBuf[8096]="";
				snprintf(szBuf,8096,"Invalid parameter count in method [%s] args = %d [argument count must be between 1 and %d]",__FUNCTION__,args,max_args_allowed);
				throw megaglest_runtime_error(szBuf);
			}

			//va_end(argList);
		}
		//lua_unlock(luaHandle);

	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return 1;
}

int ScriptManager::getGameWon(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	try {
		luaArguments.returnInt(thisScriptManager->getGameWon());
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::getIsGameOver(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	try {
		luaArguments.returnInt(thisScriptManager->getIsGameOver());
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::loadScenario(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		thisScriptManager->loadScenario(luaArguments.getString(-2),luaArguments.getInt(-1) != 0);
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::getUnitsForFaction(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		vector<int> units= thisScriptManager->getUnitsForFaction(luaArguments.getInt(-3),luaArguments.getString(-2), luaArguments.getInt(-1));
		luaArguments.returnVectorInt(units);
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();

}

int ScriptManager::getUnitCurrentField(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		luaArguments.returnInt(thisScriptManager->getUnitCurrentField(luaArguments.getInt(-1)));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::getIsUnitAlive(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		luaArguments.returnInt(thisScriptManager->getIsUnitAlive(luaArguments.getInt(-1)));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::isFreeCellsOrHasUnit(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);

	try {
		int result= thisScriptManager->isFreeCellsOrHasUnit(
				luaArguments.getInt(-3),
				luaArguments.getInt(-2),
				luaArguments.getVec2i(-1));

		luaArguments.returnInt(result);
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::isFreeCells(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);

	try {
		int result= thisScriptManager->isFreeCells(
				luaArguments.getInt(-3),
				luaArguments.getInt(-2),
				luaArguments.getVec2i(-1));

		luaArguments.returnInt(result);
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::getHumanFactionId(LuaHandle* luaHandle) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	LuaArguments luaArguments(luaHandle);
	try {
		luaArguments.returnInt(thisScriptManager->getHumanFactionId());
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::highlightUnit(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		thisScriptManager->highlightUnit(luaArguments.getInt(-4), luaArguments.getFloat(-3), luaArguments.getFloat(-2), luaArguments.getVec4f(-1));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::unhighlightUnit(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		thisScriptManager->unhighlightUnit(luaArguments.getInt(-1));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::giveStopCommand(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		thisScriptManager->giveStopCommand(luaArguments.getInt(-1));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::selectUnit(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		luaArguments.returnInt(thisScriptManager->selectUnit(luaArguments.getInt(-1)));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::unselectUnit(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		thisScriptManager->unselectUnit(luaArguments.getInt(-1));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::addUnitToGroupSelection(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		thisScriptManager->addUnitToGroupSelection(luaArguments.getInt(-2),luaArguments.getInt(-1));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::recallGroupSelection(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		thisScriptManager->recallGroupSelection(luaArguments.getInt(-1));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::removeUnitFromGroupSelection(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		thisScriptManager->removeUnitFromGroupSelection(luaArguments.getInt(-2),luaArguments.getInt(-1));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::setAttackWarningsEnabled(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		thisScriptManager->setAttackWarningsEnabled((luaArguments.getInt(-1) == 0 ? false : true));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}
int ScriptManager::getAttackWarningsEnabled(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		luaArguments.returnInt(thisScriptManager->getAttackWarningsEnabled());
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::getIsDayTime(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		luaArguments.returnInt(thisScriptManager->getIsDayTime());
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}
int ScriptManager::getIsNightTime(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		luaArguments.returnInt(thisScriptManager->getIsNightTime());
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}
int ScriptManager::getTimeOfDay(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		luaArguments.returnFloat(thisScriptManager->getTimeOfDay());
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::registerDayNightEvent(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		thisScriptManager->registerDayNightEvent();
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}
int ScriptManager::unregisterDayNightEvent(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		thisScriptManager->unregisterDayNightEvent();
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::registerUnitTriggerEvent(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		thisScriptManager->registerUnitTriggerEvent(luaArguments.getInt(-1));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}
int ScriptManager::unregisterUnitTriggerEvent(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		thisScriptManager->unregisterUnitTriggerEvent(luaArguments.getInt(-1));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}
int ScriptManager::getLastUnitTriggerEventUnitId(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		luaArguments.returnInt(thisScriptManager->getLastUnitTriggerEventUnitId());
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}
int ScriptManager::getLastUnitTriggerEventType(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		luaArguments.returnInt(static_cast<int>(thisScriptManager->getLastUnitTriggerEventType()));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::getUnitProperty(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		int value = thisScriptManager->getUnitProperty(luaArguments.getInt(-2),static_cast<UnitTriggerEventType>(luaArguments.getInt(-1)));
		luaArguments.returnInt(value);
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}
int ScriptManager::getUnitPropertyName(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		const string unitname = thisScriptManager->getUnitPropertyName(luaArguments.getInt(-2),static_cast<UnitTriggerEventType>(luaArguments.getInt(-1)));
		luaArguments.returnString(unitname);
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::disableSpeedChange(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		thisScriptManager->disableSpeedChange();
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}
int ScriptManager::enableSpeedChange(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		thisScriptManager->enableSpeedChange();
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}
int ScriptManager::getSpeedChangeEnabled(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		luaArguments.returnInt(thisScriptManager->getSpeedChangeEnabled());
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::storeSaveGameData(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		thisScriptManager->storeSaveGameData(luaArguments.getString(-2),luaArguments.getString(-1));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::loadSaveGameData(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		luaArguments.returnString(thisScriptManager->loadSaveGameData(luaArguments.getString(-1)));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();
}

int ScriptManager::getFactionPlayerType(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	try {
		luaArguments.returnInt(thisScriptManager->getFactionPlayerType(luaArguments.getInt(-1)));
	}
	catch(const megaglest_runtime_error &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nThe game may no longer be stable!\nerror [") + string(ex.what()) + string("]\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,sErrBuf.c_str());

		thisScriptManager->addMessageToQueue(ScriptManagerMessage(sErrBuf.c_str(), "error",-1,-1,true));
		thisScriptManager->onMessageBoxOk(false);
	}

	return luaArguments.getReturnCount();

}

void ScriptManager::saveGame(XmlNode *rootNode) {
	std::map<string,string> mapTagReplacements;
	XmlNode *scriptManagerNode = rootNode->addChild("ScriptManager");

	//lua
//	string code;
	scriptManagerNode->addAttribute("code",code, mapTagReplacements);
//	LuaScript luaScript;

	luaScript.beginCall("onSave");
	luaScript.endCall();

	if(LuaScript::getDebugModeEnabled() == true) printf("After onSave luaSavedGameData.size() = %d\n",(int)luaSavedGameData.size());
	for(std::map<string, string>::iterator iterMap = luaSavedGameData.begin();
			iterMap != luaSavedGameData.end(); ++iterMap) {

		XmlNode *savedGameDataItemNode = scriptManagerNode->addChild("SavedGameDataItem");

		savedGameDataItemNode->addAttribute("key",iterMap->first, mapTagReplacements);
		savedGameDataItemNode->addAttribute("value",iterMap->second, mapTagReplacements);
	}

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
	for(unsigned int i = 0; i < (unsigned int)GameConstants::maxPlayers; ++i) {
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

	scriptManagerNode->addAttribute("registeredDayNightEvent",intToStr(registeredDayNightEvent), mapTagReplacements);
	scriptManagerNode->addAttribute("lastDayNightTriggerStatus",intToStr(lastDayNightTriggerStatus), mapTagReplacements);

	for(std::map<int,UnitTriggerEventType>::iterator iterMap = UnitTriggerEventList.begin();
			iterMap != UnitTriggerEventList.end(); ++iterMap) {
		XmlNode *unitTriggerEventListNode = scriptManagerNode->addChild("UnitTriggerEventList");

		unitTriggerEventListNode->addAttribute("unitId",intToStr(iterMap->first), mapTagReplacements);
		unitTriggerEventListNode->addAttribute("eventType",intToStr(iterMap->second), mapTagReplacements);
	}
	scriptManagerNode->addAttribute("lastUnitTriggerEventUnitId",intToStr(lastUnitTriggerEventUnitId), mapTagReplacements);
	scriptManagerNode->addAttribute("lastUnitTriggerEventType",intToStr(lastUnitTriggerEventType), mapTagReplacements);

	luaScript.saveGame(scriptManagerNode);
}

void ScriptManager::loadGame(const XmlNode *rootNode) {
	const XmlNode *scriptManagerNode = rootNode->getChild("ScriptManager");

//	string code;
	code = scriptManagerNode->getAttribute("code")->getValue();
//	LuaScript luaScript;

	vector<XmlNode *> savedGameDataItemNodeList = scriptManagerNode->getChildList("SavedGameDataItem");

	if(LuaScript::getDebugModeEnabled() == true) printf("In loadGame savedGameDataItemNodeList.size() = %d\n",(int)savedGameDataItemNodeList.size());

	for(unsigned int i = 0; i < savedGameDataItemNodeList.size(); ++i) {
		XmlNode *node = savedGameDataItemNodeList[i];
		string key = node->getAttribute("key")->getValue();
		string value = node->getAttribute("value")->getValue();

		luaSavedGameData[key] = value;
	}

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

	if(scriptManagerNode->hasAttribute("registeredDayNightEvent") == true) {
		registeredDayNightEvent = scriptManagerNode->getAttribute("registeredDayNightEvent")->getIntValue() != 0;
	}
	if(scriptManagerNode->hasAttribute("lastDayNightTriggerStatus") == true) {
		lastDayNightTriggerStatus = scriptManagerNode->getAttribute("lastDayNightTriggerStatus")->getIntValue();
	}

	vector<XmlNode *> unitTriggerEventListNodeList = scriptManagerNode->getChildList("UnitTriggerEventList");
	for(unsigned int i = 0; i < unitTriggerEventListNodeList.size(); ++i) {
		XmlNode *node = unitTriggerEventListNodeList[i];

		UnitTriggerEventType eventType = utet_None;
		int unitId = node->getAttribute("unitId")->getIntValue();
		if(node->hasAttribute("eventType") == true) {
			eventType = static_cast<UnitTriggerEventType>(node->getAttribute("eventType")->getIntValue());
		}
		else if(node->hasAttribute("evenType") == true) {
			eventType = static_cast<UnitTriggerEventType>(node->getAttribute("evenType")->getIntValue());
		}
		UnitTriggerEventList[unitId] = eventType;
	}
	if(scriptManagerNode->hasAttribute("lastUnitTriggerEventUnitId") == true) {
		lastUnitTriggerEventUnitId = scriptManagerNode->getAttribute("lastUnitTriggerEventUnitId")->getIntValue();
	}
	if(scriptManagerNode->hasAttribute("lastUnitTriggerEventType") == true) {
		lastUnitTriggerEventType = static_cast<UnitTriggerEventType>(scriptManagerNode->getAttribute("lastUnitTriggerEventType")->getIntValue());
	}

	luaScript.loadGame(scriptManagerNode);
}


}}//end namespace
