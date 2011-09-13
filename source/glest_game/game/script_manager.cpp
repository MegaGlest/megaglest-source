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

// =====================================================
//	class PlayerModifiers
// =====================================================

PlayerModifiers::PlayerModifiers(){
	winner			= false;
	aiEnabled		= true;
	consumeEnabled 	= true;
}

// =====================================================
//	class ScriptManager
// =====================================================
ScriptManager* ScriptManager::thisScriptManager= NULL;
const int ScriptManager::messageWrapCount= 30;
const int ScriptManager::displayTextWrapCount= 64;

void ScriptManager::init(World* world, GameCamera *gameCamera){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	const Scenario*	scenario= world->getScenario();

	this->world= world;
	this->gameCamera= gameCamera;

	//set static instance
	thisScriptManager= this;

	currentEventId = 1;
	CellTriggerEventList.clear();
	TimerTriggerEventList.clear();

	//register functions
	luaScript.registerFunction(showMessage, "showMessage");
	luaScript.registerFunction(setDisplayText, "setDisplayText");
	luaScript.registerFunction(addConsoleText, "addConsoleText");
	luaScript.registerFunction(DisplayFormattedText, "DisplayFormattedText");
	luaScript.registerFunction(clearDisplayText, "clearDisplayText");
	luaScript.registerFunction(setCameraPosition, "setCameraPosition");
	luaScript.registerFunction(createUnit, "createUnit");
	luaScript.registerFunction(destroyUnit, "destroyUnit");
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
	luaScript.registerFunction(resetTimerEvent, "resetTimerEvent");
	luaScript.registerFunction(stopTimerEvent, "stopTimerEvent");
	luaScript.registerFunction(getTimerEventSecondsElapsed, "timerEventSecondsElapsed");
	luaScript.registerFunction(getCellTriggeredEventId, "triggeredCellEventId");
	luaScript.registerFunction(getTimerTriggeredEventId, "triggeredTimerEventId");

	luaScript.registerFunction(getStartLocation, "startLocation");
	luaScript.registerFunction(getUnitPosition, "unitPosition");
	luaScript.registerFunction(getUnitFaction, "unitFaction");
	luaScript.registerFunction(getResourceAmount, "resourceAmount");
	luaScript.registerFunction(getLastCreatedUnitName, "lastCreatedUnitName");
	luaScript.registerFunction(getLastCreatedUnitId, "lastCreatedUnit");
	luaScript.registerFunction(getLastDeadUnitName, "lastDeadUnitName");
	luaScript.registerFunction(getLastDeadUnitId, "lastDeadUnit");
	luaScript.registerFunction(getUnitCount, "unitCount");
	luaScript.registerFunction(getUnitCountOfType, "unitCountOfType");

	luaScript.registerFunction(getGameWon, "gameWon");

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
	lastDeadUnitId= -1;
	gameOver= false;
	gameWon = false;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//call startup function
	luaScript.beginCall("startup");
	luaScript.endCall();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

// ========================== events ===============================================

void ScriptManager::onMessageBoxOk(){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	Lang &lang= Lang::getInstance();

	if(!messageQueue.empty()){
		messageQueue.pop();
		if(!messageQueue.empty()){
			messageBox.setText(wrapString(lang.getScenarioString(messageQueue.front().getText()), messageWrapCount));
			messageBox.setHeader(lang.getScenarioString(messageQueue.front().getHeader()));
		}
	}
}

void ScriptManager::onResourceHarvested(){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	luaScript.beginCall("resourceHarvested");
	luaScript.endCall();
}

void ScriptManager::onUnitCreated(const Unit* unit){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	lastCreatedUnitName= unit->getType()->getName();
	lastCreatedUnitId= unit->getId();
	luaScript.beginCall("unitCreated");
	luaScript.endCall();
	luaScript.beginCall("unitCreatedOfType_"+unit->getType()->getName());
	luaScript.endCall();
}

void ScriptManager::onUnitDied(const Unit* unit){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	lastDeadUnitName= unit->getType()->getName();
	lastDeadUnitId= unit->getId();
	luaScript.beginCall("unitDied");
	luaScript.endCall();
}

void ScriptManager::onGameOver(bool won) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	gameWon = won;
	luaScript.beginCall("gameOver");
	luaScript.endCall();
}

void ScriptManager::onTimerTriggerEvent() {
	if(TimerTriggerEventList.size() <= 0) {
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

			currentTimerTriggeredEventId = iterMap->first;

			luaScript.beginCall("timerTriggerEvent");
			luaScript.endCall();
		}
	}
}

void ScriptManager::onCellTriggerEvent(Unit *movingUnit) {
	if(CellTriggerEventList.size() <= 0) {
		return;
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] movingUnit = %p, CellTriggerEventList.size() = %d\n",__FILE__,__FUNCTION__,__LINE__,movingUnit,CellTriggerEventList.size());

	// remove any delayed removals
	unregisterCellTriggerEvent(-1);

	inCellTriggerEvent = true;
	if(movingUnit != NULL) {
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
					bool srcInDst = world->getMap()->isInUnitTypeCells(0, event.destPos,movingUnit->getPos());
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

void ScriptManager::showMessage(const string &text, const string &header){
	ScriptManager_STREFLOP_Wrapper streflopWrapper;

	Lang &lang= Lang::getInstance();

	messageQueue.push(ScriptManagerMessage(text, header));
	messageBox.setEnabled(true);
	messageBox.setText(wrapString(lang.getScenarioString(messageQueue.front().getText()), messageWrapCount));
	messageBox.setHeader(lang.getScenarioString(messageQueue.front().getHeader()));
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

void ScriptManager::DisplayFormattedText(const char *fmt, ...) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	ScriptManager_STREFLOP_Wrapper streflopWrapper;

    va_list argList;
    va_start(argList, fmt);

    const int max_debug_buffer_size = 8096;
    char szBuf[max_debug_buffer_size]="";
    vsnprintf(szBuf,max_debug_buffer_size-1,fmt, argList);

	//displayText= wrapString(Lang::getInstance().getScenarioString(text), displayTextWrapCount);
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
	world->togglePauseGame((pauseStatus != 0));
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
		throw runtime_error("world->getGame() == NULL");
	}
	world->getGame()->startPerformanceTimer();

}

void ScriptManager::endPerformanceTimer() {
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	if(world->getGame() == NULL) {
		throw runtime_error("world->getGame() == NULL");
	}
	world->getGame()->endPerformanceTimer();

}

Vec2i ScriptManager::getPerformanceTimerResults() {
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	if(world->getGame() == NULL) {
		throw runtime_error("world->getGame() == NULL");
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

bool ScriptManager::getGameWon() {
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	return gameWon;
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

// ========================== lua callbacks ===============================================

int ScriptManager::showMessage(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	thisScriptManager->showMessage(luaArguments.getString(-2), luaArguments.getString(-1));
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

int ScriptManager::destroyUnit(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] unit [%d]\n",__FILE__,__FUNCTION__,__LINE__,luaArguments.getInt(-1));

	thisScriptManager->destroyUnit(luaArguments.getInt(-1));
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
			throw runtime_error(szBuf);
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

int ScriptManager::getGameWon(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	luaArguments.returnInt(thisScriptManager->getGameWon());
	return luaArguments.getReturnCount();
}


}}//end namespace
