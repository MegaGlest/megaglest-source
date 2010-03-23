// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Martiño Figueroa
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

namespace Glest{ namespace Game{

// =====================================================
//	class PlayerModifiers
// =====================================================

PlayerModifiers::PlayerModifiers(){
	winner= false;
	aiEnabled= true;
}

// =====================================================
//	class ScriptManager
// =====================================================

GraphicMessageBox	ScriptManager::messageBox;

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
	luaScript.registerFunction(giveUpgradeCommand, "giveUpgradeCommand");
	luaScript.registerFunction(disableAi, "disableAi");
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
	displayText= wrapString(Lang::getInstance().getScenarioString(text), displayTextWrapCount);
}

void ScriptManager::setCameraPosition(const Vec2i &pos){
	gameCamera->centerXZ(pos.x, pos.y);
}

void ScriptManager::createUnit(const string &unitName, int factionIndex, Vec2i pos){
	world->createUnit(unitName, factionIndex, pos);
}

void ScriptManager::giveResource(const string &resourceName, int factionIndex, int amount){
	world->giveResource(resourceName, factionIndex, amount);
}

void ScriptManager::givePositionCommand(int unitId, const string &commandName, const Vec2i &pos){
	world->givePositionCommand(unitId, commandName, pos);
}

void ScriptManager::giveProductionCommand(int unitId, const string &producedName){
	world->giveProductionCommand(unitId, producedName);
}

void ScriptManager::giveUpgradeCommand(int unitId, const string &producedName){
	world->giveUpgradeCommand(unitId, producedName);
}

void ScriptManager::disableAi(int factionIndex){
	if(factionIndex<GameConstants::maxPlayers){
		playerModifiers[factionIndex].disableAi();
	}
}

void ScriptManager::setPlayerAsWinner(int factionIndex){
	if(factionIndex<GameConstants::maxPlayers){
		playerModifiers[factionIndex].setAsWinner();
	}
}

void ScriptManager::endGame(){
	gameOver= true;
}

Vec2i ScriptManager::getStartLocation(int factionIndex){
	return world->getStartLocation(factionIndex);
}


Vec2i ScriptManager::getUnitPosition(int unitId){
	return world->getUnitPosition(unitId);
}

int ScriptManager::getUnitFaction(int unitId){
	return world->getUnitFactionIndex(unitId);
}

int ScriptManager::getResourceAmount(const string &resourceName, int factionIndex){
	return world->getResourceAmount(resourceName, factionIndex);
}

const string &ScriptManager::getLastCreatedUnitName(){
	return lastCreatedUnitName;
}

int ScriptManager::getLastCreatedUnitId(){
	return lastCreatedUnitId;
}

const string &ScriptManager::getLastDeadUnitName(){
	return lastDeadUnitName;
}

int ScriptManager::getLastDeadUnitId(){
	return lastDeadUnitId;
}

int ScriptManager::getUnitCount(int factionIndex){
	return world->getUnitCount(factionIndex);
}

int ScriptManager::getUnitCountOfType(int factionIndex, const string &typeName){
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
