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

#include "network_types.h"
#include "util.h"
#include "unit.h"
#include "world.h"
#include "unit_type.h"
#include "game.h"
#include "gui.h"

#include "leak_dumper.h"

using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
//	class NetworkCommand
// =====================================================

NetworkCommand::NetworkCommand(World *world, int networkCommandType, int unitId,
								int commandTypeId, const Vec2i &pos, int unitTypeId,
								int targetId, int facing, bool wantQueue,
								CommandStateType commandStateType,
								int commandStateValue, int unitCommandGroupId)
		: networkCommandType(networkCommandType)
		, unitId(unitId)
		, commandTypeId(commandTypeId)
		, positionX(pos.x)
		, positionY(pos.y)
		, unitTypeId(unitTypeId)
		, wantQueue(wantQueue)
		, commandStateType(commandStateType)
		, commandStateValue(commandStateValue)
		, unitCommandGroupId(unitCommandGroupId) {

	assert(targetId == -1 || facing == -1);
	this->targetId = targetId >= 0 ? targetId : facing;
	this->fromFactionIndex = world->getThisFactionIndex();

    if(this->networkCommandType == nctGiveCommand) {
        const Unit *unit= world->findUnitById(this->unitId);

        //validate unit
        if(unit != NULL) {
        	this->unitFactionIndex = unit->getFaction()->getIndex();
        	this->unitFactionUnitCount = unit->getFaction()->getUnitCount();

            //const UnitType *unitType= world->findUnitTypeById(unit->getFaction()->getType(), this->unitTypeId);
            const CommandType *ct   = unit->getType()->findCommandTypeById(this->commandTypeId);
            if(ct != NULL && ct->getClass() == ccBuild) {
            	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] %s\n",__FILE__,__FUNCTION__,__LINE__,toString().c_str());
                CardinalDir::assertDirValid(facing);
                assert(targetId == -1);
            }
        }
    }

    if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] Created NetworkCommand as follows:\n%s\n",__FILE__,__FUNCTION__,__LINE__,toString().c_str());
}

void NetworkCommand::preprocessNetworkCommand(World *world) {
    if(networkCommandType == nctGiveCommand) {
        const Unit *unit= world->findUnitById(unitId);

        //validate unit
        if(unit != NULL) {
            //const UnitType *unitType= world->findUnitTypeById(unit->getFaction()->getType(), unitTypeId);
            const CommandType *ct   = unit->getType()->findCommandTypeById(commandTypeId);
            if(ct != NULL && ct->getClass() == ccBuild && targetId >= 0) {
				CardinalDir::assertDirValid(targetId);
            }
        }
        else {
        	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] (unit == NULL) %s\n",__FILE__,__FUNCTION__,__LINE__,toString().c_str());
        }
    }
}


string NetworkCommand::toString() const {
    char szBuf[8096]="";
    snprintf(szBuf,8096,"networkCommandType = %d\nunitId = %d\ncommandTypeId = %d\npositionX = %d\npositionY = %d\nunitTypeId = %d\ntargetId = %d\nwantQueue= %d\nfromFactionIndex = %d\nunitFactionUnitCount = %d\nunitFactionIndex = %d, commandStateType = %d, commandStateValue = %d, unitCommandGroupId = %d",
        networkCommandType,unitId,commandTypeId,positionX,positionY,unitTypeId,targetId,wantQueue,
        fromFactionIndex,unitFactionUnitCount,unitFactionIndex,commandStateType,commandStateValue,
        unitCommandGroupId);

    string result = szBuf;
    return result;
}

void NetworkCommand::toEndian() {
	networkCommandType = Shared::PlatformByteOrder::toCommonEndian(networkCommandType);
	unitId = Shared::PlatformByteOrder::toCommonEndian(unitId);
	unitTypeId = Shared::PlatformByteOrder::toCommonEndian(unitTypeId);
	commandTypeId = Shared::PlatformByteOrder::toCommonEndian(commandTypeId);
	positionX = Shared::PlatformByteOrder::toCommonEndian(positionX);
	positionY = Shared::PlatformByteOrder::toCommonEndian(positionY);
	targetId = Shared::PlatformByteOrder::toCommonEndian(targetId);
	wantQueue = Shared::PlatformByteOrder::toCommonEndian(wantQueue);
	fromFactionIndex = Shared::PlatformByteOrder::toCommonEndian(fromFactionIndex);
	unitFactionUnitCount = Shared::PlatformByteOrder::toCommonEndian(unitFactionUnitCount);
	unitFactionIndex = Shared::PlatformByteOrder::toCommonEndian(unitFactionIndex);
	commandStateType = Shared::PlatformByteOrder::toCommonEndian(commandStateType);
	commandStateValue = Shared::PlatformByteOrder::toCommonEndian(commandStateValue);
	unitCommandGroupId = Shared::PlatformByteOrder::toCommonEndian(unitCommandGroupId);

}
void NetworkCommand::fromEndian() {
	networkCommandType = Shared::PlatformByteOrder::fromCommonEndian(networkCommandType);
	unitId = Shared::PlatformByteOrder::fromCommonEndian(unitId);
	unitTypeId = Shared::PlatformByteOrder::fromCommonEndian(unitTypeId);
	commandTypeId = Shared::PlatformByteOrder::fromCommonEndian(commandTypeId);
	positionX = Shared::PlatformByteOrder::fromCommonEndian(positionX);
	positionY = Shared::PlatformByteOrder::fromCommonEndian(positionY);
	targetId = Shared::PlatformByteOrder::fromCommonEndian(targetId);
	wantQueue = Shared::PlatformByteOrder::fromCommonEndian(wantQueue);
	fromFactionIndex = Shared::PlatformByteOrder::fromCommonEndian(fromFactionIndex);
	unitFactionUnitCount = Shared::PlatformByteOrder::fromCommonEndian(unitFactionUnitCount);
	unitFactionIndex = Shared::PlatformByteOrder::fromCommonEndian(unitFactionIndex);
	commandStateType = Shared::PlatformByteOrder::fromCommonEndian(commandStateType);
	commandStateValue = Shared::PlatformByteOrder::fromCommonEndian(commandStateValue);
	unitCommandGroupId = Shared::PlatformByteOrder::fromCommonEndian(unitCommandGroupId);
}

XmlNode * NetworkCommand::saveGame(XmlNode *rootNode) {
	std::map<string,string> mapTagReplacements;
	XmlNode *networkCommandNode = rootNode->addChild("NetworkCommand");

//	int16 networkCommandType;
	networkCommandNode->addAttribute("networkCommandType",intToStr(networkCommandType), mapTagReplacements);
//	int32 unitId;
	networkCommandNode->addAttribute("unitId",intToStr(unitId), mapTagReplacements);
//	int16 unitTypeId;
	networkCommandNode->addAttribute("unitTypeId",intToStr(unitTypeId), mapTagReplacements);
//	int16 commandTypeId;
	networkCommandNode->addAttribute("commandTypeId",intToStr(commandTypeId), mapTagReplacements);
//	int16 positionX;
	networkCommandNode->addAttribute("positionX",intToStr(positionX), mapTagReplacements);
//	int16 positionY;
	networkCommandNode->addAttribute("positionY",intToStr(positionY), mapTagReplacements);
//	int32 targetId;
	networkCommandNode->addAttribute("targetId",intToStr(targetId), mapTagReplacements);
//	int8 wantQueue;
	networkCommandNode->addAttribute("wantQueue",intToStr(wantQueue), mapTagReplacements);
//	int8 fromFactionIndex;
	networkCommandNode->addAttribute("fromFactionIndex",intToStr(fromFactionIndex), mapTagReplacements);
//	uint16 unitFactionUnitCount;
	networkCommandNode->addAttribute("unitFactionUnitCount",intToStr(unitFactionUnitCount), mapTagReplacements);
//	int8 unitFactionIndex;
	networkCommandNode->addAttribute("unitFactionIndex",intToStr(unitFactionIndex), mapTagReplacements);
//	int8 commandStateType;
	networkCommandNode->addAttribute("commandStateType",intToStr(commandStateType), mapTagReplacements);
//	int32 commandStateValue;
	networkCommandNode->addAttribute("commandStateValue",intToStr(commandStateValue), mapTagReplacements);
//	int32 unitCommandGroupId;
	networkCommandNode->addAttribute("unitCommandGroupId",intToStr(unitCommandGroupId), mapTagReplacements);

	return networkCommandNode;
}

void NetworkCommand::loadGame(const XmlNode *rootNode) {
	const XmlNode *networkCommandNode = rootNode;

//	int16 networkCommandType;
	networkCommandType = networkCommandNode->getAttribute("networkCommandType")->getIntValue();
//	int32 unitId;
	unitId = networkCommandNode->getAttribute("unitId")->getIntValue();
//	int16 unitTypeId;
	unitTypeId = networkCommandNode->getAttribute("unitTypeId")->getIntValue();
//	int16 commandTypeId;
	commandTypeId = networkCommandNode->getAttribute("commandTypeId")->getIntValue();
//	int16 positionX;
	positionX = networkCommandNode->getAttribute("positionX")->getIntValue();
//	int16 positionY;
	positionY = networkCommandNode->getAttribute("positionY")->getIntValue();
//	int32 targetId;
	targetId = networkCommandNode->getAttribute("targetId")->getIntValue();
//	int8 wantQueue;
	wantQueue = networkCommandNode->getAttribute("wantQueue")->getIntValue();
//	int8 fromFactionIndex;
	fromFactionIndex = networkCommandNode->getAttribute("fromFactionIndex")->getIntValue();
//	uint16 unitFactionUnitCount;
	unitFactionUnitCount = networkCommandNode->getAttribute("unitFactionUnitCount")->getIntValue();
//	int8 unitFactionIndex;
	unitFactionIndex = networkCommandNode->getAttribute("unitFactionIndex")->getIntValue();
//	int8 commandStateType;
	commandStateType = networkCommandNode->getAttribute("commandStateType")->getIntValue();
//	int32 commandStateValue;
	commandStateValue = networkCommandNode->getAttribute("commandStateValue")->getIntValue();
//	int32 unitCommandGroupId;
	unitCommandGroupId = networkCommandNode->getAttribute("unitCommandGroupId")->getIntValue();
}

}}//end namespace
