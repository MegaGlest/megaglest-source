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
								int commandStateValue)
		: networkCommandType(networkCommandType)
		, unitId(unitId)
		, commandTypeId(commandTypeId)
		, positionX(pos.x)
		, positionY(pos.y)
		, unitTypeId(unitTypeId)
		, wantQueue(wantQueue)
		, commandStateType(commandStateType)
		, commandStateValue(commandStateValue) {

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
                SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] %s\n",__FILE__,__FUNCTION__,__LINE__,toString().c_str());
                CardinalDir::assertDirValid(facing);
                assert(targetId == -1);
            }
        }
    }

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] Created NetworkCommand as follows:\n%s\n",__FILE__,__FUNCTION__,__LINE__,toString().c_str());
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
            SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] (unit == NULL) %s\n",__FILE__,__FUNCTION__,__LINE__,toString().c_str());
        }
    }
}


string NetworkCommand::toString() const {
    char szBuf[1024]="";
    sprintf(szBuf,"networkCommandType = %d\nunitId = %d\ncommandTypeId = %d\npositionX = %d\npositionY = %d\nunitTypeId = %d\ntargetId = %d\nwantQueue= %d\nfromFactionIndex = %d\nunitFactionUnitCount = %d\nunitFactionIndex = %d, commandStateType = %d, commandStateValue = %d",
        networkCommandType,unitId,commandTypeId,positionX,positionY,unitTypeId,targetId,wantQueue,
        fromFactionIndex,unitFactionUnitCount,unitFactionIndex,commandStateType,commandStateValue);

    string result = szBuf;
    return result;
}

}}//end namespace
