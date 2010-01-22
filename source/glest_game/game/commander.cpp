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

CommandResult Commander::tryGiveCommand(const Unit* unit, const CommandType *commandType, const Vec2i &pos, const UnitType* unitType) const{
	NetworkCommand networkCommand(nctGiveCommand, unit->getId(), commandType->getId(), pos, unitType->getId());
	return pushNetworkCommand(&networkCommand);
}

CommandResult Commander::tryGiveCommand(const Selection *selection, CommandClass commandClass, const Vec2i &pos, const Unit *targetUnit) const{
	
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
				NetworkCommand networkCommand(nctGiveCommand, unitId, ct->getId(), currPos, -1, targetId);
				
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

CommandResult Commander::tryGiveCommand(const Selection *selection, const CommandType *commandType, const Vec2i &pos, const Unit *targetUnit) const{
	if(!selection->isEmpty() && commandType!=NULL){
		Vec2i refPos;
		CommandResultContainer results;
		
		refPos= computeRefPos(selection);
		
		//give orders to all selected units
		for(int i=0; i<selection->getCount(); ++i){
			int targetId= targetUnit==NULL? Unit::invalidId: targetUnit->getId();
			int unitId= selection->getUnit(i)->getId();
			Vec2i currPos= computeDestPos(refPos, selection->getUnit(i)->getPos(), pos);
			NetworkCommand networkCommand(nctGiveCommand, unitId, commandType->getId(), currPos, -1, targetId);
				
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
CommandResult Commander::tryGiveCommand(const Selection *selection, const Vec2i &pos, const Unit *targetUnit) const{
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
				NetworkCommand networkCommand(nctGiveCommand, unitId, commandType->getId(), currPos, -1, targetId);

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
		NetworkCommand command(nctCancelCommand, selection->getUnit(i)->getId());
		pushNetworkCommand(&command);
	}

	return crSuccess;
}

void Commander::trySetMeetingPoint(const Unit* unit, const Vec2i &pos)const{
	NetworkCommand command(nctSetMeetingPoint, unit->getId(), -1, pos);
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
		}
		return crSuccess;
	}
}

CommandResult Commander::pushNetworkCommand(const NetworkCommand* networkCommand) const{
	GameNetworkInterface *gameNetworkInterface= NetworkManager::getInstance().getGameNetworkInterface();
	const Unit* unit= world->findUnitById(networkCommand->getUnitId());
	CommandResult cr= crSuccess;

	//validate unit
	if(unit==NULL){
		throw runtime_error("Command refers to non existant unit. Game out of synch.");
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

		//update the keyframe
		gameNetworkInterface->updateKeyframe(world->getFrameCount());

		//give pending commands
		for(int i= 0; i < gameNetworkInterface->getPendingCommandCount(); ++i){
			giveNetworkCommand(gameNetworkInterface->getPendingCommand(i));
		}
		gameNetworkInterface->clearPendingCommands();
	}
}

void Commander::giveNetworkCommand(const NetworkCommand* networkCommand) const{
	
	Unit* unit= world->findUnitById(networkCommand->getUnitId());
			
	//exec ute command, if unit is still alive
	if(unit!=NULL){
		switch(networkCommand->getNetworkCommandType()){
			case nctGiveCommand:{
				assert(networkCommand->getCommandTypeId()!=CommandType::invalidId);
				Command* command= buildCommand(networkCommand);
				unit->giveCommand(command);
				}
				break;
			case nctCancelCommand:
				unit->cancelCommand();
				break;
			case nctSetMeetingPoint:
				unit->setMeetingPos(networkCommand->getPosition());
				break;
			default:
				assert(false);
		}
	}
}

Command* Commander::buildCommand(const NetworkCommand* networkCommand) const{
	assert(networkCommand->getNetworkCommandType()==nctGiveCommand);

	Unit* target= NULL;
	const CommandType* ct= NULL;
	const Unit* unit= world->findUnitById(networkCommand->getUnitId());
	const UnitType* unitType= world->findUnitTypeById(unit->getFaction()->getType(), networkCommand->getUnitTypeId());
	
	//validate unit
	if(unit==NULL){
		throw runtime_error("Can not find unit with id: " + intToStr(networkCommand->getUnitId()) + ". Game out of synch.");
	}

	ct= unit->getType()->findCommandTypeById(networkCommand->getCommandTypeId());

	//validate command type
	if(ct==NULL){
		throw runtime_error("Can not find command type with id: " + intToStr(networkCommand->getCommandTypeId()) + " in unit: " + unit->getType()->getName() + ". Game out of synch.");
	}

	//get target, the target might be dead due to lag, cope with it
	if(networkCommand->getTargetId()!=Unit::invalidId){
		target= world->findUnitById(networkCommand->getTargetId());
	}

	//create command
	Command *command= NULL;
	if(unitType!=NULL){
		command= new Command(ct, networkCommand->getPosition(), unitType);
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
