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

#include "commander.h"

#include "world.h"
#include "unit.h"
#include "conversion.h"
#include "upgrade.h"
#include "command.h"
#include "command_type.h"
#include "network_manager.h"
#include "console.h"
#include "config.h"
#include "platform_util.h"
#include "leak_dumper.h"
#include "game.h"

using namespace Shared::Graphics;
using namespace Shared::Util;
using namespace Shared::Platform;

namespace Glest{ namespace Game{

// =====================================================
// 	class Commander
// =====================================================

// ===================== PUBLIC ========================

void Commander::init(World *world){
	this->world= world;
}

CommandResult Commander::tryGiveCommand(const Unit* unit, const CommandType *commandType, const Vec2i &pos, const UnitType* unitType, CardinalDir facing, bool tryQueue) const{
	
	NetworkCommand networkCommand(this->world,nctGiveCommand, unit->getId(), commandType->getId(), pos, unitType->getId(), -1, facing, tryQueue);
	return pushNetworkCommand(&networkCommand);
}

CommandResult Commander::tryGiveCommand(const Selection *selection, CommandClass commandClass, const Vec2i &pos, const Unit *targetUnit, bool tryQueue) const{
	
	if(!selection->isEmpty()){
		Vec2i refPos, currPos;
		CommandResultContainer results;

		refPos= computeRefPos(selection);

		//give orders to all selected units
		for(int i=0; i<selection->getCount(); ++i){
			const Unit *unit= selection->getUnit(i);
			const CommandType *ct= unit->getType()->getFirstCtOfClass(commandClass);
			if(ct!=NULL){
				int targetId= targetUnit==NULL? Unit::invalidId: targetUnit->getId();
				int unitId= selection->getUnit(i)->getId();
				Vec2i currPos= computeDestPos(refPos, selection->getUnit(i)->getPos(), pos);
				NetworkCommand networkCommand(this->world,nctGiveCommand, unitId, ct->getId(), currPos, -1, targetId, -1, tryQueue);

				//every unit is ordered to a different pos
				CommandResult result= pushNetworkCommand(&networkCommand);
				results.push_back(result);
			}
			else{
				results.push_back(crFailUndefined);
			}
		}
		return computeResult(results);
	}
	else{
		return crFailUndefined;
	}
}

CommandResult Commander::tryGiveCommand(const Selection *selection, const CommandType *commandType, const Vec2i &pos, const Unit *targetUnit, bool tryQueue) const{
	if(!selection->isEmpty() && commandType!=NULL){
		Vec2i refPos;
		CommandResultContainer results;

		refPos= computeRefPos(selection);

		//give orders to all selected units
		for(int i=0; i<selection->getCount(); ++i){
			int targetId= targetUnit==NULL? Unit::invalidId: targetUnit->getId();
			int unitId= selection->getUnit(i)->getId();
			Vec2i currPos= computeDestPos(refPos, selection->getUnit(i)->getPos(), pos);
			NetworkCommand networkCommand(this->world,nctGiveCommand, unitId, commandType->getId(), currPos, -1, targetId, -1, tryQueue);

			//every unit is ordered to a different position
			CommandResult result= pushNetworkCommand(&networkCommand);
			results.push_back(result);
		}

		return computeResult(results);
	}
	else{
		return crFailUndefined;
	}
}

//auto command
CommandResult Commander::tryGiveCommand(const Selection *selection, const Vec2i &pos, const Unit *targetUnit, bool tryQueue) const{
	if(!selection->isEmpty()){

		Vec2i refPos, currPos;
		CommandResultContainer results;

		//give orders to all selected units
		refPos= computeRefPos(selection);
		for(int i=0; i<selection->getCount(); ++i){

			//every unit is ordered to a different pos
			currPos= computeDestPos(refPos, selection->getUnit(i)->getPos(), pos);

			//get command type
			const CommandType *commandType= selection->getUnit(i)->computeCommandType(pos, targetUnit);

			//give commands
			if(commandType!=NULL){
				int targetId= targetUnit==NULL? Unit::invalidId: targetUnit->getId();
				int unitId= selection->getUnit(i)->getId();
				NetworkCommand networkCommand(this->world,nctGiveCommand, unitId, commandType->getId(), currPos, -1, targetId, -1, tryQueue);

				CommandResult result= pushNetworkCommand(&networkCommand);
				results.push_back(result);
			}
			else{
				results.push_back(crFailUndefined);
			}
		}
		return computeResult(results);
	}
	else{
		return crFailUndefined;
	}
}

CommandResult Commander::tryCancelCommand(const Selection *selection) const{

	for(int i=0; i<selection->getCount(); ++i){
		NetworkCommand command(this->world,nctCancelCommand, selection->getUnit(i)->getId());
		pushNetworkCommand(&command);
	}

	return crSuccess;
}

void Commander::trySetMeetingPoint(const Unit* unit, const Vec2i &pos)const{
	NetworkCommand command(this->world,nctSetMeetingPoint, unit->getId(), -1, pos);
	pushNetworkCommand(&command);
}


// ==================== PRIVATE ====================

Vec2i Commander::computeRefPos(const Selection *selection) const{
    Vec2i total= Vec2i(0);
    for(int i=0; i<selection->getCount(); ++i){
        total= total+selection->getUnit(i)->getPos();
    }

    return Vec2i(total.x/ selection->getCount(), total.y/ selection->getCount());
}

Vec2i Commander::computeDestPos(const Vec2i &refUnitPos, const Vec2i &unitPos, const Vec2i &commandPos) const{
    Vec2i pos;
	Vec2i posDiff= unitPos-refUnitPos;

	if(abs(posDiff.x)>=3){
		posDiff.x= posDiff.x % 3;
	}

	if(abs(posDiff.y)>=3){
		posDiff.y= posDiff.y % 3;
	}

	pos= commandPos+posDiff;
    world->getMap()->clampPos(pos);
    return pos;
}

CommandResult Commander::computeResult(const CommandResultContainer &results) const{
	switch(results.size()){
	case 0:
		return crFailUndefined;
	case 1:
		return results.front();
	default:
		for(int i=0; i<results.size(); ++i){
			if(results[i]!=crSuccess){
				return crSomeFailed;
			}
		}http://de.wikipedia.org/wiki/Iatrogen
		return crSuccess;
	}
}

CommandResult Commander::pushNetworkCommand(const NetworkCommand* networkCommand) const{
	GameNetworkInterface *gameNetworkInterface= NetworkManager::getInstance().getGameNetworkInterface();
	const Unit* unit= world->findUnitById(networkCommand->getUnitId());
	CommandResult cr= crSuccess;

	//validate unit
	if(unit==NULL){
	    char szBuf[1024]="";
	    sprintf(szBuf,"In [%s::%s - %d] Command refers to non existant unit id = %d. Game out of synch.",
            __FILE__,__FUNCTION__,__LINE__,networkCommand->getUnitId());
		throw runtime_error(szBuf);
	}

	//add the command to the interface
	gameNetworkInterface->requestCommand(networkCommand);

	//calculate the result of the command
	if(networkCommand->getNetworkCommandType()==nctGiveCommand){
		Command* command= buildCommand(networkCommand);
		cr= unit->checkCommand(command);
		delete command;
	}
	return cr;
}

void Commander::updateNetwork(){
	NetworkManager &networkManager= NetworkManager::getInstance();

	//chech that this is a keyframe
	if( !networkManager.isNetworkGame() || (world->getFrameCount() % GameConstants::networkFramePeriod)==0){

		GameNetworkInterface *gameNetworkInterface= NetworkManager::getInstance().getGameNetworkInterface();

		perfTimer.start();
		//update the keyframe
		gameNetworkInterface->updateKeyframe(world->getFrameCount());
		SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] gameNetworkInterface->updateKeyframe for %d took %d msecs\n",__FILE__,__FUNCTION__,__LINE__,world->getFrameCount(),perfTimer.getMillis());

		perfTimer.start();
		//give pending commands
		for(int i= 0; i < gameNetworkInterface->getPendingCommandCount(); ++i){
			giveNetworkCommand(gameNetworkInterface->getPendingCommand(i));
		}
		SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] giveNetworkCommand took %d msecs\n",__FILE__,__FUNCTION__,__LINE__,perfTimer.getMillis());
		gameNetworkInterface->clearPendingCommands();
	}
}

/*
void Commander::giveNetworkCommandSpecial(const NetworkCommand* networkCommand) const {
    switch(networkCommand->getNetworkCommandType()) {
        case nctNetworkCommand: {
            SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] found nctNetworkCommand\n",__FILE__,__FUNCTION__,__LINE__);
            switch(networkCommand->getCommandTypeId()) {
                case ncstRotateUnit: {
                    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] found ncstRotateUnit [%d]\n",__FILE__,__FUNCTION__,__LINE__,networkCommand->getTargetId());

                    int unitTypeId = networkCommand->getUnitId();
                    int factionIndex = networkCommand->getUnitTypeId();
                    int rotateAmount = networkCommand->getTargetId();

                    //const Faction *faction = world->getFaction(factionIndex);
                    //const UnitType* unitType= world->findUnitTypeById(faction->getType(), factionIndex);

                    char unitKey[50]="";
                    sprintf(unitKey,"%d_%d",unitTypeId,factionIndex);

                    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] unitKey = [%s]\n",__FILE__,__FUNCTION__,__LINE__,unitKey);

                    Game *game = this->world->getGame();
                    Gui *gui = game->getGui();
                    gui->setUnitTypeBuildRotation(unitKey,rotateAmount);
                    //unit->setRotateAmount(networkCommand->getTargetId());

                    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] found ncstRotateUnit [%d]\n",__FILE__,__FUNCTION__,__LINE__,networkCommand->getTargetId());
                    }
                    break;
            }
            SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] found nctNetworkCommand\n",__FILE__,__FUNCTION__,__LINE__);
            }
            break;
        default:
            assert(false);
    }
}
*/

void Commander::giveNetworkCommand(NetworkCommand* networkCommand) const {

    networkCommand->preprocessNetworkCommand(this->world);
    /*
    if(networkCommand->getNetworkCommandType() == nctNetworkCommand) {
        giveNetworkCommandSpecial(networkCommand);
    }
    else
    */
    {
        Unit* unit= world->findUnitById(networkCommand->getUnitId());

        //exec ute command, if unit is still alive
        if(unit!=NULL) {
            switch(networkCommand->getNetworkCommandType()){
                case nctGiveCommand:{
                    assert(networkCommand->getCommandTypeId()!=CommandType::invalidId);

                    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] found nctGiveCommand networkCommand->getUnitId() = %d\n",__FILE__,__FUNCTION__,__LINE__,networkCommand->getUnitId());

                    Command* command= buildCommand(networkCommand);
                    unit->giveCommand(command, networkCommand->getWantQueue());

                    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] found nctGiveCommand networkCommand->getUnitId() = %d\n",__FILE__,__FUNCTION__,__LINE__,networkCommand->getUnitId());
                    }
                    break;
                case nctCancelCommand: {
                    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] found nctCancelCommand\n",__FILE__,__FUNCTION__,__LINE__);
                    unit->cancelCommand();
                    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] found nctCancelCommand\n",__FILE__,__FUNCTION__,__LINE__);
                }
                    break;
                case nctSetMeetingPoint: {
                    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] found nctSetMeetingPoint\n",__FILE__,__FUNCTION__,__LINE__);
                    unit->setMeetingPos(networkCommand->getPosition());
                    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] found nctSetMeetingPoint\n",__FILE__,__FUNCTION__,__LINE__);
                }
                    break;
                default:
                    assert(false);
            }
        }
        else {
            SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] NULL Unit for id = %d, networkCommand->getNetworkCommandType() = %d\n",__FILE__,__FUNCTION__,__LINE__,networkCommand->getUnitId(),networkCommand->getNetworkCommandType());
        }
    }
}

Command* Commander::buildCommand(const NetworkCommand* networkCommand) const{
	assert(networkCommand->getNetworkCommandType()==nctGiveCommand);

	Unit* target= NULL;
	const CommandType* ct= NULL;
	const Unit* unit= world->findUnitById(networkCommand->getUnitId());

	//validate unit
	if(unit == NULL) {
	    char szBuf[1024]="";
	    sprintf(szBuf,"In [%s::%s - %d] Can not find unit with id: %d. Game out of synch.",
            __FILE__,__FUNCTION__,__LINE__,networkCommand->getUnitId());

		throw runtime_error(szBuf);
	}

    const UnitType* unitType= world->findUnitTypeById(unit->getFaction()->getType(), networkCommand->getUnitTypeId());
	ct= unit->getType()->findCommandTypeById(networkCommand->getCommandTypeId());

	//validate command type
	if(ct == NULL) {

	    char szBuf[1024]="";
	    sprintf(szBuf,"In [%s::%s - %d] Can not find command type with id = %d in unit = %d [%s][%s]. Game out of synch.",
            __FILE__,__FUNCTION__,__LINE__,networkCommand->getCommandTypeId(),unit->getId(), unit->getFullName().c_str(),unit->getDesc().c_str());

		throw runtime_error(szBuf);
	}

	CardinalDir facing;
	// get facing/target ... the target might be dead due to lag, cope with it
	if (ct->getClass() == ccBuild) {
		assert(networkCommand->getTargetId() >= 0 && networkCommand->getTargetId() < 4);
		facing = CardinalDir(networkCommand->getTargetId());
	} else if (networkCommand->getTargetId() != Unit::invalidId ) {
		target= world->findUnitById(networkCommand->getTargetId());
	}

	//create command
	Command *command= NULL;
	if(unitType!=NULL){
		command= new Command(ct, networkCommand->getPosition(), unitType, facing);
	}
	else if(target==NULL){
		command= new Command(ct, networkCommand->getPosition());
	}
	else{
		command= new Command(ct, target);
	}

	//issue command
	return command;
}

}}//end namespace
