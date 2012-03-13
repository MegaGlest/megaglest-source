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

#include "command.h"

#include "command_type.h"
#include "util.h"
#include "conversion.h"
#include "unit_type.h"
#include "faction.h"
#include "world.h"
#include "leak_dumper.h"

using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class Command
// =====================================================
Command::Command() {
    this->commandType= NULL;
    this->unitRef= NULL;
	unitType= NULL;
	stateType			= cst_None;
	stateValue 			= -1;
	unitCommandGroupId	= -1;
}

Command::Command(const CommandType *ct, const Vec2i &pos){
	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] ct = [%p]\n",__FILE__,__FUNCTION__,__LINE__,ct);

    this->commandType= ct;
    this->pos= pos;
    this->originalPos = this->pos;
    this->unitRef= NULL;
	unitType= NULL;
	stateType			= cst_None;
	stateValue 			= -1;
	unitCommandGroupId	= -1;
}

Command::Command(const CommandType *ct, Unit* unit) {
    this->commandType= ct;
    this->pos= Vec2i(0);
    this->originalPos = this->pos;
    this->unitRef= unit;
	unitType= NULL;
	if(unit!=NULL) {
		unit->resetHighlight();
		pos= unit->getCellPos();
	}
	stateType			= cst_None;
	stateValue 			= -1;
	unitCommandGroupId	= -1;
}

Command::Command(const CommandType *ct, const Vec2i &pos, const UnitType *unitType, CardinalDir facing) {
    this->commandType= ct;
    this->pos= pos;
    this->originalPos = this->pos;
    this->unitRef= NULL;
	this->unitType= unitType;
	this->facing = facing;
	stateType			= cst_None;
	stateValue 			= -1;
	unitCommandGroupId	= -1;

	//if(this->unitType != NULL) {
	//	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] unitType = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->unitType->toString().c_str());
	//}
}

int Command::getPriority(){
	if(this->commandType->commandTypeClass==ccAttack && getUnit()==NULL){
		return 5; // attacks to the ground have low priority
	}
	return this->commandType->getTypePriority();
}
// =============== set ===============

void Command::setCommandType(const CommandType *commandType) {
    this->commandType= commandType;
}

void Command::setPos(const Vec2i &pos){
     this->pos= pos;
}

void Command::setOriginalPos(const Vec2i &pos) {
     this->originalPos= pos;
}

void Command::setPosToOriginalPos() {
	this->pos= this->originalPos;
}

void Command::setUnit(Unit *unit){
     this->unitRef= unit;
}

std::string Command::toString() const {
	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__, __LINE__);

	std::string result = "";
	if(commandType != NULL) {
		result = "commandType id = " + intToStr(commandType->getId()) + ", desc = " + commandType->toString();
	}
	else {
		result = "commandType = NULL";
	}
	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__, __LINE__);

	result += ", pos = " + pos.getString() + ", originalPos = " + originalPos.getString() + ", facing = " + intToStr(facing.asInt());

	//if(unitRef.getUnit() != NULL) {
	if(unitRef.getUnitId() >= 0) {
		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__, __LINE__);

		result += ", unitRef.getUnit() id = " + intToStr(unitRef.getUnitId());

		// The code below causes a STACK OVERFLOW!
		//if(unitRef.getUnit() != NULL) {
		//	result += ", unitRef.getUnit() = " + unitRef.getUnit()->toString();
		//}
	}

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__, __LINE__);

	if(unitType != NULL) {
		result += ", unitTypeId = " + intToStr(unitType->getId());
		result += ", unitTypeDesc = " + unitType->getReqDesc();
	}

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__, __LINE__);

	result += ", stateType = " + intToStr(stateType) + ", stateValue = " + intToStr(stateValue);


	result += ", unitCommandGroupId = " + intToStr(unitCommandGroupId);

	return result;
}

void Command::saveGame(XmlNode *rootNode, Faction *faction) {
	std::map<string,string> mapTagReplacements;
	XmlNode *commandNode = rootNode->addChild("Command");

//    const CommandType *commandType;
	if(commandType != NULL) {
		commandNode->addAttribute("commandType",intToStr(commandType->getId()), mapTagReplacements);
	}
//    Vec2i originalPos;
	commandNode->addAttribute("originalPos",originalPos.getString(), mapTagReplacements);
//    Vec2i pos;
	commandNode->addAttribute("pos",pos.getString(), mapTagReplacements);
//	UnitReference unitRef;		//target unit, used to move and attack optionally
	unitRef.saveGame(commandNode);
//	CardinalDir facing;			// facing, for build command
	commandNode->addAttribute("facing",intToStr(facing), mapTagReplacements);
//	const UnitType *unitType;	//used for build
	if(unitType != NULL) {
		commandNode->addAttribute("unitTypeId",intToStr(unitType->getId()), mapTagReplacements);
		commandNode->addAttribute("unitTypeFactionIndex",intToStr(faction->getIndex()), mapTagReplacements);
	}
//	CommandStateType stateType;
	commandNode->addAttribute("stateType",intToStr(stateType), mapTagReplacements);
//	int stateValue;
	commandNode->addAttribute("stateValue",intToStr(stateValue), mapTagReplacements);
//	int unitCommandGroupId;
	commandNode->addAttribute("unitCommandGroupId",intToStr(unitCommandGroupId), mapTagReplacements);
}

Command * Command::loadGame(const XmlNode *rootNode,const UnitType *ut,World *world) {
	Command *result = new Command();
	const XmlNode *commandNode = rootNode;

	//description = commandNode->getAttribute("description")->getValue();

	//    const CommandType *commandType;
	if(commandNode->hasAttribute("commandType") == true) {
		int cmdTypeId = commandNode->getAttribute("commandType")->getIntValue();
		result->commandType = ut->findCommandTypeById(cmdTypeId);
	}
	//    Vec2i originalPos;
	result->originalPos = Vec2i::strToVec2(commandNode->getAttribute("originalPos")->getValue());
	//    Vec2i pos;
	result->pos = Vec2i::strToVec2(commandNode->getAttribute("pos")->getValue());
	//	UnitReference unitRef;		//target unit, used to move and attack optionally
	result->unitRef.loadGame(commandNode,world);
	//	CardinalDir facing;			// facing, for build command
	result->facing = static_cast<CardinalDir>(commandNode->getAttribute("facing")->getIntValue());
	//	const UnitType *unitType;	//used for build
	if(commandNode->hasAttribute("unitTypeId") == true) {
		//result->unitType = ut;
		int unitTypeId = commandNode->getAttribute("unitTypeId")->getIntValue();
		int unitTypeFactionIndex = commandNode->getAttribute("unitTypeFactionIndex")->getIntValue();
		Faction *faction = world->getFaction(unitTypeFactionIndex);
		result->unitType = world->findUnitTypeById(faction->getType(),unitTypeId);
	}
	//	CommandStateType stateType;
	result->stateType = static_cast<CommandStateType>(commandNode->getAttribute("stateType")->getIntValue());
	//	int stateValue;
	result->stateValue = commandNode->getAttribute("stateValue")->getIntValue();
	//	int unitCommandGroupId;
	result->unitCommandGroupId = commandNode->getAttribute("unitCommandGroupId")->getIntValue();

	return result;
}

}}//end namespace
