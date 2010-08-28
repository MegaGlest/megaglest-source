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
	streflop_init<streflop::Simple>();
#endif
	}
	~ScriptManager_STREFLOP_Wrapper() {
#ifdef USE_STREFLOP
	streflop_init<streflop::Double>();
#endif
	}
};

// =====================================================
//	class PlayerModifiers
// =====================================================

PlayerModifiers::PlayerModifiers(){
	winner			= false;
	aiEnabled		= true;
	hungerEnabled 	= true;
}

// =====================================================
//	class ScriptManager
// =====================================================
ScriptManager* ScriptManager::thisScriptManager= NULL;
const int ScriptManager::messageWrapCount= 30;
const int ScriptManager::displayTextWrapCount= 64;

void ScriptManager::init(World* world, GameCamera *gameCamera){
	const Scenario*	scenario= world->getScenario();

	this->world= world;
	this->gameCamera= gameCamera;

	//set static instance
	thisScriptManager= this;

	//register functions
	luaScript.registerFunction(showMessage, "showMessage");
	luaScript.registerFunction(setDisplayText, "setDisplayText");
	luaScript.registerFunction(clearDisplayText, "clearDisplayText");
	luaScript.registerFunction(setCameraPosition, "setCameraPosition");
	luaScript.registerFunction(createUnit, "createUnit");
	luaScript.registerFunction(giveResource, "giveResource");
	luaScript.registerFunction(givePositionCommand, "givePositionCommand");
	luaScript.registerFunction(giveProductionCommand, "giveProductionCommand");
	luaScript.registerFunction(giveAttackCommand, "giveAttackCommand");
	luaScript.registerFunction(giveUpgradeCommand, "giveUpgradeCommand");
	luaScript.registerFunction(disableAi, "disableAi");
	luaScript.registerFunction(enableAi, "enableAi");
	luaScript.registerFunction(disableHunger, "disableHunger");
	luaScript.registerFunction(enableHunger, "enableHunger");
	luaScript.registerFunction(setPlayerAsWinner, "setPlayerAsWinner");
	luaScript.registerFunction(endGame, "endGame");

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

	//call startup function
	luaScript.beginCall("startup");
	luaScript.endCall();
}

// ========================== events ===============================================

void ScriptManager::onMessageBoxOk(){
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
	luaScript.beginCall("resourceHarvested");
	luaScript.endCall();
}

void ScriptManager::onUnitCreated(const Unit* unit){
	lastCreatedUnitName= unit->getType()->getName();
	lastCreatedUnitId= unit->getId();
	luaScript.beginCall("unitCreated");
	luaScript.endCall();
	luaScript.beginCall("unitCreatedOfType_"+unit->getType()->getName());
	luaScript.endCall();
}

void ScriptManager::onUnitDied(const Unit* unit){
	lastDeadUnitName= unit->getType()->getName();
	lastDeadUnitId= unit->getId();
	luaScript.beginCall("unitDied");
	luaScript.endCall();
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

void ScriptManager::setCameraPosition(const Vec2i &pos){
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	gameCamera->centerXZ(pos.x, pos.y);
}

void ScriptManager::createUnit(const string &unitName, int factionIndex, Vec2i pos){
	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] unit [%s] factionIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,unitName.c_str(),factionIndex);
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	world->createUnit(unitName, factionIndex, pos);
}

void ScriptManager::giveResource(const string &resourceName, int factionIndex, int amount){
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	world->giveResource(resourceName, factionIndex, amount);
}

void ScriptManager::givePositionCommand(int unitId, const string &commandName, const Vec2i &pos){
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	world->givePositionCommand(unitId, commandName, pos);
}

void ScriptManager::giveAttackCommand(int unitId, int unitToAttackId) {
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	world->giveAttackCommand(unitId, unitToAttackId);
}

void ScriptManager::giveProductionCommand(int unitId, const string &producedName){
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	world->giveProductionCommand(unitId, producedName);
}

void ScriptManager::giveUpgradeCommand(int unitId, const string &producedName){
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	world->giveUpgradeCommand(unitId, producedName);
}

void ScriptManager::disableAi(int factionIndex){
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	if(factionIndex<GameConstants::maxPlayers){
		playerModifiers[factionIndex].disableAi();
	}
}

void ScriptManager::enableAi(int factionIndex){
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	if(factionIndex<GameConstants::maxPlayers){
		playerModifiers[factionIndex].enableAi();
	}
}

void ScriptManager::disableHunger(int factionIndex){
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	if(factionIndex<GameConstants::maxPlayers){
		playerModifiers[factionIndex].disableHunger();
	}
}

void ScriptManager::enableHunger(int factionIndex){
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	if(factionIndex<GameConstants::maxPlayers){
		playerModifiers[factionIndex].enableHunger();
	}
}

void ScriptManager::setPlayerAsWinner(int factionIndex){
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	if(factionIndex<GameConstants::maxPlayers){
		playerModifiers[factionIndex].setAsWinner();
	}
}

void ScriptManager::endGame(){
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	gameOver= true;
}

Vec2i ScriptManager::getStartLocation(int factionIndex){
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	return world->getStartLocation(factionIndex);
}


Vec2i ScriptManager::getUnitPosition(int unitId){
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	return world->getUnitPosition(unitId);
}

int ScriptManager::getUnitFaction(int unitId){
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	return world->getUnitFactionIndex(unitId);
}

int ScriptManager::getResourceAmount(const string &resourceName, int factionIndex){
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	return world->getResourceAmount(resourceName, factionIndex);
}

const string &ScriptManager::getLastCreatedUnitName(){
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	return lastCreatedUnitName;
}

int ScriptManager::getLastCreatedUnitId(){
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	return lastCreatedUnitId;
}

const string &ScriptManager::getLastDeadUnitName(){
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	return lastDeadUnitName;
}

int ScriptManager::getLastDeadUnitId(){
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	return lastDeadUnitId;
}

int ScriptManager::getUnitCount(int factionIndex){
	ScriptManager_STREFLOP_Wrapper streflopWrapper;
	return world->getUnitCount(factionIndex);
}

int ScriptManager::getUnitCountOfType(int factionIndex, const string &typeName){
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

int ScriptManager::createUnit(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] unit [%s] factionIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,luaArguments.getString(-3).c_str(),luaArguments.getInt(-2));

	thisScriptManager->createUnit(
		luaArguments.getString(-3),
		luaArguments.getInt(-2),
		luaArguments.getVec2i(-1));
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

int ScriptManager::disableHunger(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	thisScriptManager->disableHunger(luaArguments.getInt(-1));
	return luaArguments.getReturnCount();
}

int ScriptManager::enableHunger(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	thisScriptManager->enableHunger(luaArguments.getInt(-1));
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

}}//end namespace
