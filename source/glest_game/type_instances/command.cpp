// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martio Figueroa
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
#include "leak_dumper.h"

using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class Command
// =====================================================

Command::Command(const CommandType *ct, const Vec2i &pos){
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] ct = [%p]\n",__FILE__,__FUNCTION__,__LINE__,ct);

    this->commandType= ct;  
    this->pos= pos;
	unitType= NULL;
	stateType			= cst_None;
	stateValue 			= -1;
} 

Command::Command(const CommandType *ct, Unit* unit){
    this->commandType= ct;  
    this->pos= Vec2i(0);
    this->unitRef= unit;
	unitType= NULL;
	if(unit!=NULL) {
		unit->resetHighlight();
		pos= unit->getCellPos();
	}
	stateType			= cst_None;
	stateValue 			= -1;
} 

Command::Command(const CommandType *ct, const Vec2i &pos, const UnitType *unitType, CardinalDir facing){
    this->commandType= ct;  
    this->pos= pos;
	this->unitType= unitType;
	this->facing = facing;
	stateType			= cst_None;
	stateValue 			= -1;

	if(this->unitType != NULL) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] unitType = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->unitType->toString().c_str());
	}
}

// =============== set ===============

void Command::setCommandType(const CommandType *commandType){
    this->commandType= commandType;
}

void Command::setPos(const Vec2i &pos){
     this->pos= pos;
}

void Command::setUnit(Unit *unit){
     this->unitRef= unit;
}

std::string Command::toString() const {
	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__, __LINE__);

	std::string result = "";
	if(commandType != NULL) {
		result = "commandType id = " + intToStr(commandType->getId()) + ", desc = " + commandType->toString() + ", pos = " + pos.getString() + ", facing = " + intToStr(facing.asInt());
	}
	else {
		result = "commandType = NULL";
	}
	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__, __LINE__);

	//if(unitRef.getUnit() != NULL) {
	if(unitRef.getUnitId() >= 0) {
		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__, __LINE__);

		result += ", unitRef.getUnit() id = " + intToStr(unitRef.getUnitId());
	}

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__, __LINE__);

	if(unitType != NULL) {
		result += ", unitTypeId = " + intToStr(unitType->getId());
		result += ", unitTypeDesc = " + unitType->getReqDesc();
	}

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__, __LINE__);

	result = ", stateType = " + intToStr(stateType) + ", stateValue = " + intToStr(stateValue);

	return result;
}

}}//end namespace
