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

NetworkCommand::NetworkCommand(World *world, int networkCommandType, int unitId, int commandTypeId, const Vec2i &pos, int unitTypeId, int targetId){
	this->networkCommandType= networkCommandType;
	this->unitId= unitId;
	this->commandTypeId= commandTypeId;
	this->positionX= pos.x;
	this->positionY= pos.y;
	this->unitTypeId= unitTypeId;
	this->targetId= targetId;

    if(this->networkCommandType == nctGiveCommand) {
        const Unit *unit= world->findUnitById(this->unitId);

        //validate unit
        if(unit != NULL) {
            const UnitType *unitType= world->findUnitTypeById(unit->getFaction()->getType(), this->unitTypeId);
            const CommandType *ct   = unit->getType()->findCommandTypeById(this->commandTypeId);
            if(ct != NULL && ct->getClass() == ccBuild) {
                SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] %s\n",__FILE__,__FUNCTION__,__LINE__,toString().c_str());

                Game *game  = world->getGame();
                Gui *gui    = game->getGui();

                int factionIndex = world->getThisFactionIndex();
                char unitKey[50]="";
                sprintf(unitKey,"%d_%d",this->unitTypeId,factionIndex);
                float unitTypeRotation = gui->getUnitTypeBuildRotation(unitKey);

                if(unitTypeRotation >= 0) {
                    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] attaching RotateUnit to ccBuild command for unitTypeid = %d, factionIndex = %d, unitTypeRotation = %f\n",__FILE__,__FUNCTION__,this->unitTypeId,factionIndex,unitTypeRotation);
                    this->targetId = unitTypeRotation;
                }
            }
        }
    }
}

void NetworkCommand::preprocessNetworkCommand(World *world) {
    if(networkCommandType == nctGiveCommand) {
        const Unit *unit= world->findUnitById(unitId);

        //validate unit
        if(unit != NULL) {
            const UnitType *unitType= world->findUnitTypeById(unit->getFaction()->getType(), unitTypeId);
            const CommandType *ct   = unit->getType()->findCommandTypeById(commandTypeId);
            if(ct != NULL && ct->getClass() == ccBuild && targetId >= 0) {
                Game *game  = world->getGame();
                Gui *gui    = game->getGui();

                int factionIndex = unit->getFactionIndex();
                char unitKey[50]="";
                sprintf(unitKey,"%d_%d",unitTypeId,factionIndex);
                gui->setUnitTypeBuildRotation(unitKey,targetId);

                SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] %s, unitKey = [%s] targetId = %d\n",__FILE__,__FUNCTION__,__LINE__,toString().c_str(),unitKey,targetId);

                targetId = Unit::invalidId;
            }
        }
        else {
            SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] (unit == NULL) %s\n",__FILE__,__FUNCTION__,__LINE__,toString().c_str());
        }
    }
}

string NetworkCommand::toString() const {
    char szBuf[1024]="";
    sprintf(szBuf,"networkCommandType = %d\nunitId = %d\ncommandTypeId = %d\npositionX = %d\nthis->positionY = %d\nunitTypeId = %d\ntargetId = %d",
        networkCommandType,unitId,commandTypeId,positionX,this->positionY,unitTypeId,targetId);

    string result = szBuf;
    return result;
}

/*
NetworkCommand::NetworkCommand(int networkCommandType, NetworkCommandSubType ncstType, int unitId, int value1, int value2) {
	this->networkCommandType= networkCommandType;
	this->unitId= unitId;
	this->commandTypeId= ncstType;
	this->positionX= -1;
	this->positionY= -1;
	this->unitTypeId= value1;
	this->targetId= value2;
}
*/

}}//end namespace
