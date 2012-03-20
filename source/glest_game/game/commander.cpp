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
#include "game.h"
#include "game_settings.h"
#include "game.h"

using namespace Shared::Graphics;
using namespace Shared::Util;
using namespace Shared::Platform;

namespace Glest{ namespace Game{

// =====================================================
// 	class Commander
// =====================================================

// ===================== PUBLIC ========================

CommanderNetworkThread::CommanderNetworkThread() : BaseThread() {
	this->idStatus = make_pair<int,bool>(-1,false);
	this->commanderInterface = NULL;
}

CommanderNetworkThread::CommanderNetworkThread(CommanderNetworkCallbackInterface *commanderInterface) : BaseThread() {
	this->idStatus = make_pair<int,bool>(-1,false);
	this->commanderInterface = commanderInterface;
}

void CommanderNetworkThread::setQuitStatus(bool value) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d value = %d\n",__FILE__,__FUNCTION__,__LINE__,value);

	BaseThread::setQuitStatus(value);
	if(value == true) {
		signalUpdate(-1);
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
}

void CommanderNetworkThread::signalUpdate(int id) {
	MutexSafeWrapper safeMutex(&idMutex,string(__FILE__) + "_" + intToStr(__LINE__));
	this->idStatus.first = id;
	this->idStatus.second = false;
	safeMutex.ReleaseLock();

	semTaskSignalled.signal();
}

void CommanderNetworkThread::setTaskCompleted(int id) {
    MutexSafeWrapper safeMutex(&idMutex,string(__FILE__) + "_" + intToStr(__LINE__));
    this->idStatus.second = true;
    safeMutex.ReleaseLock();
}

bool CommanderNetworkThread::isSignalCompleted(int id) {
	MutexSafeWrapper safeMutex(&idMutex,string(__FILE__) + "_" + intToStr(__LINE__));
	bool result = this->idStatus.second;
	safeMutex.ReleaseLock();
	return result;
}

void CommanderNetworkThread::execute() {
    RunningStatusSafeWrapper runningStatus(this);
	try {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		//unsigned int idx = 0;
		for(;this->commanderInterface != NULL;) {
			if(getQuitStatus() == true) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
				break;
			}

			semTaskSignalled.waitTillSignalled();

			if(getQuitStatus() == true) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
				break;
			}

            MutexSafeWrapper safeMutex(&idMutex,string(__FILE__) + "_" + intToStr(__LINE__));
            if(idStatus.first > 0) {
                int updateId = this->idStatus.first;
                safeMutex.ReleaseLock();

                this->commanderInterface->commanderNetworkUpdateTask(updateId);
                setTaskCompleted(updateId);
            }
            else {
                safeMutex.ReleaseLock();
            }
			if(getQuitStatus() == true) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
				break;
			}
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}
	catch(const exception &ex) {
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		throw runtime_error(ex.what());
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
}

// =====================================================
//	class
// =====================================================

Commander::Commander() {
	//this->networkThread = new CommanderNetworkThread(this);
	//this->networkThread->setUniqueID(__FILE__);
	//this->networkThread->start();
	world=NULL;
}

Commander::~Commander() {
	//if(BaseThread::shutdownAndWait(networkThread) == true) {
    //    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
    //    delete networkThread;
    //    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	//}
	//networkThread = NULL;
}

void Commander::init(World *world){
	this->world= world;
}

bool Commander::canSubmitCommandType(const Unit *unit, const CommandType *commandType) const {
	bool canSubmitCommand=true;
	const MorphCommandType *mct = dynamic_cast<const MorphCommandType*>(commandType);
	if(mct && unit->getCommandSize() > 0) {
		Command *cur_command= unit->getCurrCommand();
		if(cur_command != NULL) {
			const MorphCommandType *cur_mct= dynamic_cast<const MorphCommandType*>(cur_command->getCommandType());
			if(cur_mct && unit->getCurrSkill() && unit->getCurrSkill()->getClass() == scMorph) {
				const UnitType *morphUnitType		= mct->getMorphUnit();
				const UnitType *cur_morphUnitType	= cur_mct->getMorphUnit();

				if(morphUnitType != NULL && cur_morphUnitType != NULL && morphUnitType->getId() == cur_morphUnitType->getId()) {
					canSubmitCommand = false;
				}
			}
		}
	}
	return canSubmitCommand;
}

CommandResult Commander::tryGiveCommand(const Selection *selection, const CommandType *commandType,
									const Vec2i &pos, const UnitType* unitType,
									CardinalDir facing, bool tryQueue,Unit *targetUnit) const {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(!selection->isEmpty() && commandType != NULL) {
		Vec2i refPos;
		CommandResultContainer results;

		refPos = world->getMap()->computeRefPos(selection);

		const Unit *builderUnit = world->getMap()->findClosestUnitToPos(selection, pos, unitType);
		//Vec2i  = world->getMap()->computeDestPos(refPos, builderUnit->getPos(), pos);
		//std::pair<float,Vec2i> distance = world->getMap()->getUnitDistanceToPos(builderUnit,pos,unitType);
		//builderUnit->setCurrentUnitTitle("Distance: " + floatToStr(distance.first) + " pos: " + distance.second.getString());

		int builderUnitId 					= builderUnit->getId();
		CommandStateType commandStateType 	= cst_None;
		int commandStateValue 				= -1;

		int unitCommandGroupId = -1;
		if(selection->getCount() > 1) {
			unitCommandGroupId = world->getNextCommandGroupId();
		}

		//bool unitSignalledToBuild = false;
		//give orders to all selected units
		for(int i = 0; i < selection->getCount(); ++i) {
			const Unit *unit = selection->getUnit(i);


			CommandResult result = crFailUndefined;
			bool canSubmitCommand = canSubmitCommandType(unit, commandType);
			if(canSubmitCommand == true) {
				int unitId= unit->getId();
				Vec2i currPos= world->getMap()->computeDestPos(refPos, unit->getPos(), pos);

				Vec2i usePos = currPos;
				const CommandType *useCommandtype = commandType;
				if(dynamic_cast<const BuildCommandType *>(commandType) != NULL) {
					usePos = pos;
					//if(unitSignalledToBuild == false) {
					//if(builderUnit->getId() == unitId)
					//	builderUnitId		 = unitId;
						//unitSignalledToBuild = true;
					//}
					//else {
					if(builderUnit->getId() != unitId) {
						useCommandtype 		= unit->getType()->getFirstRepairCommand(unitType);
						commandStateType 	= cst_linkedUnit;
						commandStateValue 	= builderUnitId;
						//tryQueue = true;
					}
					else {
						commandStateType 	= cst_None;
						commandStateValue 	= -1;
					}
				}

				if(useCommandtype != NULL) {
					NetworkCommand networkCommand(this->world,nctGiveCommand, unitId,
							useCommandtype->getId(), usePos, unitType->getId(),
							(targetUnit != NULL ? targetUnit->getId() : -1),
							facing, tryQueue, commandStateType,commandStateValue,
							unitCommandGroupId);

					//every unit is ordered to a the position
					result= pushNetworkCommand(&networkCommand);
				}
			}

			results.push_back(result);
		}

		return computeResult(results);
	}
	else{
		return crFailUndefined;
	}
}

CommandResult Commander::tryGiveCommand(const Unit* unit, const CommandType *commandType,
									const Vec2i &pos, const UnitType* unitType,
									CardinalDir facing, bool tryQueue,Unit *targetUnit,
									int unitGroupCommandId) const {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

	assert(this->world != NULL);
	assert(unit != NULL);
	assert(commandType != NULL);
	assert(unitType != NULL);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	CommandResult result = crFailUndefined;
	bool canSubmitCommand=canSubmitCommandType(unit, commandType);
	if(canSubmitCommand == true) {
		NetworkCommand networkCommand(this->world,nctGiveCommand, unit->getId(),
									commandType->getId(), pos, unitType->getId(),
									(targetUnit != NULL ? targetUnit->getId() : -1),
									facing, tryQueue,cst_None,-1,unitGroupCommandId);

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

		result = pushNetworkCommand(&networkCommand);
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	return result;
}

CommandResult Commander::tryGiveCommand(const Selection *selection, CommandClass commandClass,
		const Vec2i &pos, const Unit *targetUnit, bool tryQueue) const{

	if(selection->isEmpty() == false) {
		Vec2i refPos, currPos;
		CommandResultContainer results;

		refPos= world->getMap()->computeRefPos(selection);

		int unitCommandGroupId = -1;
		if(selection->getCount() > 1) {
			unitCommandGroupId = world->getNextCommandGroupId();
		}

		//give orders to all selected units
		for(int i = 0; i < selection->getCount(); ++i) {
			const Unit *unit= selection->getUnit(i);
			const CommandType *ct= unit->getType()->getFirstCtOfClass(commandClass);
			if(ct != NULL) {
				CommandResult result = crFailUndefined;
				bool canSubmitCommand=canSubmitCommandType(unit, ct);
				if(canSubmitCommand == true) {

					int targetId= targetUnit==NULL? Unit::invalidId: targetUnit->getId();
					int unitId= selection->getUnit(i)->getId();
					Vec2i currPos= world->getMap()->computeDestPos(refPos,
							selection->getUnit(i)->getPos(), pos);
					NetworkCommand networkCommand(this->world,nctGiveCommand,
							unitId, ct->getId(), currPos, -1, targetId, -1,
							tryQueue,cst_None,-1,unitCommandGroupId);

					//every unit is ordered to a different pos
					result= pushNetworkCommand(&networkCommand);
				}
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

CommandResult Commander::tryGiveCommand(const Selection *selection,
						const CommandType *commandType, const Vec2i &pos,
						const Unit *targetUnit, bool tryQueue) const {
	if(!selection->isEmpty() && commandType!=NULL){
		Vec2i refPos;
		CommandResultContainer results;

		refPos= world->getMap()->computeRefPos(selection);

		int unitCommandGroupId = -1;
		if(selection->getCount() > 1) {
			unitCommandGroupId = world->getNextCommandGroupId();
		}

		//give orders to all selected units
		for(int i = 0; i < selection->getCount(); ++i) {
			const Unit *unit = selection->getUnit(i);
			assert(unit != NULL);

			CommandResult result = crFailUndefined;
			bool canSubmitCommand=canSubmitCommandType(unit, commandType);
			if(canSubmitCommand == true) {
				int targetId= targetUnit==NULL? Unit::invalidId: targetUnit->getId();
				int unitId= unit->getId();
				Vec2i currPos= world->getMap()->computeDestPos(refPos, unit->getPos(), pos);
				NetworkCommand networkCommand(this->world,nctGiveCommand, unitId,
						commandType->getId(), currPos, -1, targetId, -1, tryQueue,
						cst_None, -1, unitCommandGroupId);

				//every unit is ordered to a different position
				result= pushNetworkCommand(&networkCommand);
			}
			results.push_back(result);
		}

		return computeResult(results);
	}
	else{
		return crFailUndefined;
	}
}

//auto command
CommandResult Commander::tryGiveCommand(const Selection *selection, const Vec2i &pos,
		const Unit *targetUnit, bool tryQueue, int unitCommandGroupId) const {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	CommandResult result = crFailUndefined;

	if(selection->isEmpty() == false){
		Vec2i refPos, currPos;
		CommandResultContainer results;

		if(unitCommandGroupId == -1 && selection->getCount() > 1) {
			unitCommandGroupId = world->getNextCommandGroupId();
		}

		//give orders to all selected units
		refPos= world->getMap()->computeRefPos(selection);
		for(int i=0; i < selection->getCount(); ++i) {
			//every unit is ordered to a different pos
			const Unit *unit = selection->getUnit(i);
			assert(unit != NULL);

			currPos= world->getMap()->computeDestPos(refPos, unit->getPos(), pos);

			//get command type
			const CommandType *commandType= unit->computeCommandType(pos, targetUnit);

			//give commands
			if(commandType != NULL) {
				int targetId= targetUnit==NULL? Unit::invalidId: targetUnit->getId();
				int unitId= unit->getId();

				CommandResult result = crFailUndefined;
				bool canSubmitCommand=canSubmitCommandType(unit, commandType);
				if(canSubmitCommand == true) {
					NetworkCommand networkCommand(this->world,nctGiveCommand,
							unitId, commandType->getId(), currPos, -1, targetId,
							-1, tryQueue, cst_None, -1, unitCommandGroupId);
					result= pushNetworkCommand(&networkCommand);
				}
				results.push_back(result);
			}
			else if(unit->isMeetingPointSettable() == true) {
				NetworkCommand command(this->world,nctSetMeetingPoint,
						unit->getId(), -1, currPos,-1,-1,-1,false,
						cst_None,-1,unitCommandGroupId);

				CommandResult result= pushNetworkCommand(&command);
				results.push_back(result);
			}
			else {
				results.push_back(crFailUndefined);
			}
		}
		result = computeResult(results);
	}

	//if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] result = %d\n",__FILE__,__FUNCTION__,__LINE__,result);

	return result;
}

CommandResult Commander::tryCancelCommand(const Selection *selection) const {

	int unitCommandGroupId = -1;
	if(selection->getCount() > 1) {
		unitCommandGroupId = world->getNextCommandGroupId();
	}

	for(int i = 0; i < selection->getCount(); ++i) {
		NetworkCommand command(this->world,nctCancelCommand,
				selection->getUnit(i)->getId(),-1,Vec2i(0),-1,-1,-1,false,
				cst_None,-1,unitCommandGroupId);
		pushNetworkCommand(&command);
	}

	return crSuccess;
}

void Commander::trySetMeetingPoint(const Unit* unit, const Vec2i &pos) const {
	NetworkCommand command(this->world,nctSetMeetingPoint, unit->getId(), -1, pos);
	pushNetworkCommand(&command);
}

void Commander::trySwitchTeam(const Faction* faction, int teamIndex) const {
	NetworkCommand command(this->world,nctSwitchTeam, faction->getIndex(), teamIndex);
	pushNetworkCommand(&command);
}

void Commander::trySwitchTeamVote(const Faction* faction, SwitchTeamVote *vote) const {
	NetworkCommand command(this->world,nctSwitchTeamVote, faction->getIndex(), vote->factionIndex,Vec2i(0),vote->allowSwitchTeam);
	pushNetworkCommand(&command);
}

void Commander::tryPauseGame() const {
	NetworkCommand command(this->world,nctPauseResume, true);
	pushNetworkCommand(&command);
}

void Commander::tryResumeGame() const {
	NetworkCommand command(this->world,nctPauseResume, false);
	pushNetworkCommand(&command);
}

// ==================== PRIVATE ====================

CommandResult Commander::computeResult(const CommandResultContainer &results) const {
	switch(results.size()) {
        case 0:
            return crFailUndefined;
        case 1:
            return results.front();
        default:
            for(int i = 0; i < results.size(); ++i) {
                if(results[i] != crSuccess) {
                    return crSomeFailed;
                }
            }
            return crSuccess;
	}
}

CommandResult Commander::pushNetworkCommand(const NetworkCommand* networkCommand) const {
	GameNetworkInterface *gameNetworkInterface= NetworkManager::getInstance().getGameNetworkInterface();
	CommandResult cr= crSuccess;

	//validate unit
	const Unit* unit = NULL;
	if( networkCommand->getNetworkCommandType() != nctSwitchTeam &&
		networkCommand->getNetworkCommandType() != nctSwitchTeamVote &&
		networkCommand->getNetworkCommandType() != nctPauseResume) {
		unit= world->findUnitById(networkCommand->getUnitId());
		if(unit == NULL) {
			char szBuf[1024]="";
			sprintf(szBuf,"In [%s::%s - %d] Command refers to non existent unit id = %d. Game out of synch.",__FILE__,__FUNCTION__,__LINE__,networkCommand->getUnitId());
			GameNetworkInterface *gameNetworkInterface= NetworkManager::getInstance().getGameNetworkInterface();
			if(gameNetworkInterface != NULL) {
				char szMsg[1024]="";
				sprintf(szMsg,"Player detected an error: Command refers to non existent unit id = %d. Game out of synch.",networkCommand->getUnitId());
				gameNetworkInterface->sendTextMessage(szMsg,-1, true, "");
			}
			throw runtime_error(szBuf);
		}
	}

	//add the command to the interface
	gameNetworkInterface->requestCommand(networkCommand);

	//calculate the result of the command
	if(networkCommand->getNetworkCommandType() == nctGiveCommand) {
		Command* command= buildCommand(networkCommand);
		cr= unit->checkCommand(command);
		delete command;
	}
	return cr;
}

void Commander::signalNetworkUpdate(Game *game) {
    updateNetwork(game);
}

void Commander::commanderNetworkUpdateTask(int id) {
    //updateNetwork(game);
}

bool Commander::getReplayCommandListForFrame(int worldFrameCount) {
	bool haveReplyCommands = false;
	if(replayCommandList.empty() == false) {
		//int worldFrameCount = world->getFrameCount();
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("worldFrameCount = %d replayCommandList.size() = %lu\n",worldFrameCount,replayCommandList.size());

		std::vector<NetworkCommand> replayList;
		for(unsigned int i = 0; i < replayCommandList.size(); ++i) {
			std::pair<int,NetworkCommand> &cmd = replayCommandList[i];
			if(cmd.first <= worldFrameCount) {
				replayList.push_back(cmd.second);
				haveReplyCommands = true;
			}
		}
		if(haveReplyCommands == true) {
			replayCommandList.erase(replayCommandList.begin(),replayCommandList.begin() + replayList.size());

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("worldFrameCount = %d GIVING COMMANDS replayList.size() = %lu\n",worldFrameCount,replayList.size());
			for(int i= 0; i < replayList.size(); ++i){
				giveNetworkCommand(&replayList[i]);
				//pushNetworkCommand(&replayList[i]);
			}
			GameNetworkInterface *gameNetworkInterface= NetworkManager::getInstance().getGameNetworkInterface();
			gameNetworkInterface->setKeyframe(worldFrameCount);
		}
	}
	return haveReplyCommands;
}

bool Commander::hasReplayCommandListForFrame() const {
	return (replayCommandList.empty() == false);
}

int Commander::getReplayCommandListForFrameCount() const {
	return replayCommandList.size();
}

void Commander::updateNetwork(Game *game) {
	NetworkManager &networkManager= NetworkManager::getInstance();

	//check that this is a keyframe
	if(game != NULL) {
        GameSettings *gameSettings = game->getGameSettings();
        if( networkManager.isNetworkGame() == false ||
            (world->getFrameCount() % gameSettings->getNetworkFramePeriod()) == 0) {

        	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] networkManager.isNetworkGame() = %d,world->getFrameCount() = %d, gameSettings->getNetworkFramePeriod() = %d\n",__FILE__,__FUNCTION__,__LINE__,networkManager.isNetworkGame(),world->getFrameCount(),gameSettings->getNetworkFramePeriod());

        	//std::vector<NetworkCommand> replayList = getReplayCommandListForFrame();
        	if(getReplayCommandListForFrame(world->getFrameCount()) == false) {
				GameNetworkInterface *gameNetworkInterface= NetworkManager::getInstance().getGameNetworkInterface();

				if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) perfTimer.start();
				//update the keyframe
				gameNetworkInterface->updateKeyframe(world->getFrameCount());
				if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && perfTimer.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] gameNetworkInterface->updateKeyframe for %d took %lld msecs\n",__FILE__,__FUNCTION__,__LINE__,world->getFrameCount(),perfTimer.getMillis());

				if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) perfTimer.start();
				//give pending commands
				for(int i= 0; i < gameNetworkInterface->getPendingCommandCount(); ++i){
					giveNetworkCommand(gameNetworkInterface->getPendingCommand(i));
				}
				if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && perfTimer.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] giveNetworkCommand took %lld msecs, PendingCommandCount = %d\n",__FILE__,__FUNCTION__,__LINE__,perfTimer.getMillis(),gameNetworkInterface->getPendingCommandCount());
				gameNetworkInterface->clearPendingCommands();
        	}
        }
	}
}

void Commander::addToReplayCommandList(NetworkCommand &command,int worldFrameCount) {
	replayCommandList.push_back(make_pair(worldFrameCount,command));
}

void Commander::giveNetworkCommand(NetworkCommand* networkCommand) const {

	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [START]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());


	world->getGame()->addNetworkCommandToReplayList(networkCommand,world->getFrameCount());

    networkCommand->preprocessNetworkCommand(this->world);

    if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [after networkCommand->preprocessNetworkCommand]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

    bool commandWasHandled = false;
    // Handle special commands first (that just use network command members as placeholders)
    switch(networkCommand->getNetworkCommandType()) {
    	case nctSwitchTeam: {
        	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] found nctSwitchTeam\n",__FILE__,__FUNCTION__,__LINE__);

        	commandWasHandled = true;
        	int factionIndex = networkCommand->getUnitId();
        	int newTeam = networkCommand->getCommandTypeId();

        	// Auto join empty team or ask players to join
        	bool autoJoinTeam = true;
        	for(int i = 0; i < world->getFactionCount(); ++i) {
        		if(newTeam == world->getFaction(i)->getTeam()) {
        			autoJoinTeam = false;
        			break;
        		}
        	}

        	if(autoJoinTeam == true) {
        		Faction *faction = world->getFaction(factionIndex);
        		int oldTeam = faction->getTeam();
        		faction->setTeam(newTeam);
        		GameSettings *settings = world->getGameSettingsPtr();
        		settings->setTeam(factionIndex,newTeam);

        		if(factionIndex == world->getThisFactionIndex()) {
        			world->setThisTeamIndex(newTeam);
					GameNetworkInterface *gameNetworkInterface= NetworkManager::getInstance().getGameNetworkInterface();
					if(gameNetworkInterface != NULL) {

	    		    	Lang &lang= Lang::getInstance();
	    		    	const vector<string> languageList = settings->getUniqueNetworkPlayerLanguages();
	    		    	for(unsigned int i = 0; i < languageList.size(); ++i) {
							char szMsg[1024]="";
							if(lang.hasString("PlayerSwitchedTeam",languageList[i]) == true) {
								sprintf(szMsg,lang.get("PlayerSwitchedTeam",languageList[i]).c_str(),settings->getNetworkPlayerName(factionIndex).c_str(),oldTeam,newTeam);
							}
							else {
								sprintf(szMsg,"Player %s switched from team# %d to team# %d.",settings->getNetworkPlayerName(factionIndex).c_str(),oldTeam,newTeam);
							}
							bool localEcho = lang.isLanguageLocal(languageList[i]);
							gameNetworkInterface->sendTextMessage(szMsg,-1, localEcho,languageList[i]);
	    		    	}
					}
        		}
        	}
        	else {
        		for(int i = 0; i < world->getFactionCount(); ++i) {
					if(newTeam == world->getFaction(i)->getTeam()) {
						Faction *faction = world->getFaction(factionIndex);

						SwitchTeamVote vote;
						vote.factionIndex = factionIndex;
						vote.allowSwitchTeam = false;
						vote.oldTeam = faction->getTeam();
						vote.newTeam = newTeam;
						vote.voted = false;

						world->getFaction(i)->setSwitchTeamVote(vote);
					}
        		}
        	}

            if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [after unit->setMeetingPos]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

            if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] found nctSetMeetingPoint\n",__FILE__,__FUNCTION__,__LINE__);
        }
            break;

        case nctSwitchTeamVote: {
        	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] found nctSwitchTeamVote\n",__FILE__,__FUNCTION__,__LINE__);

        	commandWasHandled = true;

        	int votingFactionIndex = networkCommand->getUnitId();
        	int factionIndex = networkCommand->getCommandTypeId();
        	bool allowSwitchTeam = networkCommand->getUnitTypeId();

        	Faction *faction = world->getFaction(votingFactionIndex);

			SwitchTeamVote *vote = faction->getSwitchTeamVote(factionIndex);
			if(vote == NULL) {
				throw runtime_error("vote == NULL");
			}
			vote->voted = true;
			vote->allowSwitchTeam = allowSwitchTeam;

        	// Join the new team if > 50 % said yes
			int newTeamTotalMemberCount=0;
			int newTeamVotedYes=0;
			int newTeamVotedNo=0;

        	for(int i = 0; i < world->getFactionCount(); ++i) {
        		if(vote->newTeam == world->getFaction(i)->getTeam()) {
        			newTeamTotalMemberCount++;

        			SwitchTeamVote *teamVote = world->getFaction(i)->getSwitchTeamVote(factionIndex);
        			if(teamVote != NULL && teamVote->voted == true) {
        				if(teamVote->allowSwitchTeam == true) {
        					newTeamVotedYes++;
        				}
        				else {
        					newTeamVotedNo++;
        				}
        			}
        		}
        	}

        	// If > 50% of team vote yes, switch th eplayers team
        	if(newTeamTotalMemberCount > 0 && newTeamVotedYes > 0 &&
        		newTeamVotedYes / newTeamTotalMemberCount > 0.5) {
        		Faction *faction = world->getFaction(factionIndex);
        		int oldTeam = faction->getTeam();
        		faction->setTeam(vote->newTeam);
        		GameSettings *settings = world->getGameSettingsPtr();
        		settings->setTeam(factionIndex,vote->newTeam);

        		if(factionIndex == world->getThisFactionIndex()) {
        			world->setThisTeamIndex(vote->newTeam);
					GameNetworkInterface *gameNetworkInterface= NetworkManager::getInstance().getGameNetworkInterface();
					if(gameNetworkInterface != NULL) {

	    		    	Lang &lang= Lang::getInstance();
	    		    	const vector<string> languageList = settings->getUniqueNetworkPlayerLanguages();
	    		    	for(unsigned int i = 0; i < languageList.size(); ++i) {
							char szMsg[1024]="";
							if(lang.hasString("PlayerSwitchedTeam",languageList[i]) == true) {
								sprintf(szMsg,lang.get("PlayerSwitchedTeam",languageList[i]).c_str(),settings->getNetworkPlayerName(factionIndex).c_str(),oldTeam,vote->newTeam);
							}
							else {
								sprintf(szMsg,"Player %s switched from team# %d to team# %d.",settings->getNetworkPlayerName(factionIndex).c_str(),oldTeam,vote->newTeam);
							}
							bool localEcho = lang.isLanguageLocal(languageList[i]);
							gameNetworkInterface->sendTextMessage(szMsg,-1, localEcho,languageList[i]);
	    		    	}
					}
        		}
        	}
        	else if(newTeamTotalMemberCount == (newTeamVotedYes + newTeamVotedNo)) {
        		if(factionIndex == world->getThisFactionIndex()) {
        			GameSettings *settings = world->getGameSettingsPtr();
            		Faction *faction = world->getFaction(factionIndex);
            		int oldTeam = faction->getTeam();

					GameNetworkInterface *gameNetworkInterface= NetworkManager::getInstance().getGameNetworkInterface();
					if(gameNetworkInterface != NULL) {

	    		    	Lang &lang= Lang::getInstance();
	    		    	const vector<string> languageList = settings->getUniqueNetworkPlayerLanguages();
	    		    	for(unsigned int i = 0; i < languageList.size(); ++i) {
							char szMsg[1024]="";
							if(lang.hasString("PlayerSwitchedTeamDenied",languageList[i]) == true) {
								sprintf(szMsg,lang.get("PlayerSwitchedTeamDenied",languageList[i]).c_str(),settings->getNetworkPlayerName(factionIndex).c_str(),oldTeam,vote->newTeam);
							}
							else {
								sprintf(szMsg,"Player %s was denied the request to switch from team# %d to team# %d.",settings->getNetworkPlayerName(factionIndex).c_str(),oldTeam,vote->newTeam);
							}
							bool localEcho = lang.isLanguageLocal(languageList[i]);
							gameNetworkInterface->sendTextMessage(szMsg,-1, localEcho,languageList[i]);
	    		    	}
					}
        		}
        	}

            if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [after unit->setMeetingPos]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

            if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] found nctSetMeetingPoint\n",__FILE__,__FUNCTION__,__LINE__);
        }
            break;

        case nctPauseResume: {
        	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] found nctPauseResume\n",__FILE__,__FUNCTION__,__LINE__);

        	commandWasHandled = true;

        	bool pauseGame = networkCommand->getUnitId();
       		Game *game = this->world->getGame();

       		//printf("nctPauseResume pauseGame = %d\n",pauseGame);
       		game->setPaused(pauseGame,true);

            if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [after unit->setMeetingPos]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

            if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] found nctSetMeetingPoint\n",__FILE__,__FUNCTION__,__LINE__);
        }
            break;

    }

    /*
    if(networkCommand->getNetworkCommandType() == nctNetworkCommand) {
        giveNetworkCommandSpecial(networkCommand);
    }
    else
    */
    if(commandWasHandled == false) {
        Unit* unit= world->findUnitById(networkCommand->getUnitId());

        if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [after world->findUnitById]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

        if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Running command NetworkCommandType = %d, unitid = %d [%p] factionindex = %d\n",networkCommand->getNetworkCommandType(),networkCommand->getUnitId(),unit,(unit != NULL ? unit->getFactionIndex() : -1));
        //execute command, if unit is still alive
        if(unit != NULL) {
            switch(networkCommand->getNetworkCommandType()) {
                case nctGiveCommand:{
                    assert(networkCommand->getCommandTypeId() != CommandType::invalidId);

                    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] found nctGiveCommand networkCommand->getUnitId() = %d\n",__FILE__,__FUNCTION__,__LINE__,networkCommand->getUnitId());

                    Command* command= buildCommand(networkCommand);

                    if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [after buildCommand]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

                    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] command = %p\n",__FILE__,__FUNCTION__,__LINE__,command);

                    unit->giveCommand(command, (networkCommand->getWantQueue() != 0));

                    if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [after unit->giveCommand]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

                    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] found nctGiveCommand networkCommand->getUnitId() = %d\n",__FILE__,__FUNCTION__,__LINE__,networkCommand->getUnitId());
                    }
                    break;
                case nctCancelCommand: {
                	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] found nctCancelCommand\n",__FILE__,__FUNCTION__,__LINE__);

                	unit->cancelCommand();

                	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [after unit->cancelCommand]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

                	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] found nctCancelCommand\n",__FILE__,__FUNCTION__,__LINE__);
                }
                    break;
                case nctSetMeetingPoint: {
                	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] found nctSetMeetingPoint\n",__FILE__,__FUNCTION__,__LINE__);

                    unit->setMeetingPos(networkCommand->getPosition());

                    if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [after unit->setMeetingPos]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

                    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] found nctSetMeetingPoint\n",__FILE__,__FUNCTION__,__LINE__);
                }
                    break;

                default:
                    assert(false);
            }
        }
        else {
        	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] NULL Unit for id = %d, networkCommand->getNetworkCommandType() = %d\n",__FILE__,__FUNCTION__,__LINE__,networkCommand->getUnitId(),networkCommand->getNetworkCommandType());
        }
    }

    if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [END]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
}

Command* Commander::buildCommand(const NetworkCommand* networkCommand) const {
	assert(networkCommand->getNetworkCommandType()==nctGiveCommand);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] networkCommand [%s]\n",__FILE__,__FUNCTION__,__LINE__,networkCommand->toString().c_str());

	if(world == NULL) {
	    char szBuf[1024]="";
	    sprintf(szBuf,"In [%s::%s Line: %d] world == NULL for unit with id: %d",__FILE__,__FUNCTION__,__LINE__,networkCommand->getUnitId());
		throw runtime_error(szBuf);
	}

	Unit* target= NULL;
	const CommandType* ct= NULL;
	const Unit* unit= world->findUnitById(networkCommand->getUnitId());

	//validate unit
	if(unit == NULL) {
	    char szBuf[1024]="";
	    sprintf(szBuf,"In [%s::%s Line: %d] Can not find unit with id: %d. Game out of synch.",__FILE__,__FUNCTION__,__LINE__,networkCommand->getUnitId());
	    SystemFlags::OutputDebug(SystemFlags::debugError,"%s\n",szBuf);
	    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s\n",szBuf);

        GameNetworkInterface *gameNetworkInterface= NetworkManager::getInstance().getGameNetworkInterface();
        if(gameNetworkInterface != NULL) {
            char szMsg[1024]="";
            sprintf(szMsg,"Player detected an error: Can not find unit with id: %d. Game out of synch.",networkCommand->getUnitId());
            gameNetworkInterface->sendTextMessage(szMsg,-1, true, "");
        }

		throw runtime_error(szBuf);
	}

    ct= unit->getType()->findCommandTypeById(networkCommand->getCommandTypeId());

	if(unit->getFaction()->getIndex() != networkCommand->getUnitFactionIndex()) {

	    char szBuf[10400]="";
	    sprintf(szBuf,"In [%s::%s Line: %d]\nUnit / Faction mismatch for network command = [%s]\n%s\nfor unit = %d\n[%s]\n[%s]\nactual local factionIndex = %d.\nGame out of synch.",
            __FILE__,__FUNCTION__,__LINE__,networkCommand->toString().c_str(),unit->getType()->getCommandTypeListDesc().c_str(),unit->getId(), unit->getFullName().c_str(),unit->getDesc().c_str(),unit->getFaction()->getIndex());

	    SystemFlags::OutputDebug(SystemFlags::debugError,"%s\n",szBuf);
	    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s\n",szBuf);
	    //std::string worldLog = world->DumpWorldToLog();
	    world->DumpWorldToLog();

        GameNetworkInterface *gameNetworkInterface= NetworkManager::getInstance().getGameNetworkInterface();
        if(gameNetworkInterface != NULL && gameNetworkInterface->isConnected() == true) {
            char szMsg[1024]="";
            sprintf(szMsg,"Player detected an error: Unit / Faction mismatch for unitId: %d",networkCommand->getUnitId());
            gameNetworkInterface->sendTextMessage(szMsg,-1, true, "");
            sprintf(szMsg,"Local faction index = %d, remote index = %d. Game out of synch.",unit->getFaction()->getIndex(),networkCommand->getUnitFactionIndex());
            gameNetworkInterface->sendTextMessage(szMsg,-1, true, "");

        }
        else if(gameNetworkInterface != NULL) {
            char szMsg[1024]="";
            sprintf(szMsg,"Player detected an error: Connection lost, possible Unit / Faction mismatch for unitId: %d",networkCommand->getUnitId());
            gameNetworkInterface->sendTextMessage(szMsg,-1, true,"");
            sprintf(szMsg,"Local faction index = %d, remote index = %d. Game out of synch.",unit->getFaction()->getIndex(),networkCommand->getUnitFactionIndex());
            gameNetworkInterface->sendTextMessage(szMsg,-1, true,"");
        }

	    std::string sError = "Error [#1]: Game is out of sync (Unit / Faction mismatch)\nplease check log files for details.";
		throw runtime_error(sError);
	}
/*
    I don't think we can validate in unit type since it can be different for certain commands (like attack and build etc)

	else if(networkCommand->getUnitTypeId() >= 0 &&
            unit->getType()->getId() != networkCommand->getUnitTypeId() &&
            ct->getClass() != ccBuild) {
	    char szBuf[4096]="";
	    sprintf(szBuf,"In [%s::%s Line: %d]\nUnit / Type mismatch for network command = [%s]\n%s\nfor unit = %d\n[%s]\n[%s]\nactual local factionIndex = %d.\nactual local unitTypeId = %d.\nGame out of synch.",
            __FILE__,__FUNCTION__,__LINE__,networkCommand->toString().c_str(),unit->getType()->getCommandTypeListDesc().c_str(),unit->getId(), unit->getFullName().c_str(),unit->getDesc().c_str(),unit->getFaction()->getIndex(),unit->getType()->getId());

	    SystemFlags::OutputDebug(SystemFlags::debugError,"%s\n",szBuf);
	    SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s\n",szBuf);
	    std::string worldLog = world->DumpWorldToLog();

        GameNetworkInterface *gameNetworkInterface= NetworkManager::getInstance().getGameNetworkInterface();
        if(gameNetworkInterface != NULL) {
            char szMsg[1024]="";
            sprintf(szMsg,"Player detected an error: Unit / Faction mismatch for unitId: %d, Local faction index = %d, remote idnex = %d. Game out of synch.",networkCommand->getUnitId(),unit->getFaction()->getIndex(),networkCommand->getUnitFactionIndex());
            gameNetworkInterface->sendTextMessage(szMsg,-1, true);
        }

	    std::string sError = "Error [#2]: Game is out of sync (unit type mismatch)\nplease check log files for details.";
		throw runtime_error(sError);
	}
*/

    const UnitType* unitType= world->findUnitTypeById(unit->getFaction()->getType(), networkCommand->getUnitTypeId());

	// debug test!
	//throw runtime_error("Test missing command type!");

	//validate command type
	if(ct == NULL) {
	    char szBuf[10400]="";
	    sprintf(szBuf,"In [%s::%s Line: %d]\nCan not find command type for network command = [%s]\n%s\nfor unit = %d\n[%s]\n[%s]\nactual local factionIndex = %d.\nUnit Type Info:\n[%s]\nNetwork unit type:\n[%s]\nGame out of synch.",
            __FILE__,__FUNCTION__,__LINE__,networkCommand->toString().c_str(),unit->getType()->getCommandTypeListDesc().c_str(),
            unit->getId(), unit->getFullName().c_str(),unit->getDesc().c_str(),unit->getFaction()->getIndex(),unit->getType()->toString().c_str(),
            (unitType != NULL ? unitType->getName().c_str() : "null"));

	    SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s\n",szBuf);
	    SystemFlags::OutputDebug(SystemFlags::debugError,"%s\n",szBuf);
	    //std::string worldLog = world->DumpWorldToLog();
	    world->DumpWorldToLog();

        GameNetworkInterface *gameNetworkInterface= NetworkManager::getInstance().getGameNetworkInterface();
        if(gameNetworkInterface != NULL) {
            char szMsg[1024]="";
            sprintf(szMsg,"Player detected an error: Can not find command type: %d for unitId: %d [%s]. Game out of synch.",networkCommand->getCommandTypeId(),networkCommand->getUnitId(),(unitType != NULL ? unitType->getName().c_str() : "null"));
            gameNetworkInterface->sendTextMessage(szMsg,-1, true, "");
        }

	    std::string sError = "Error [#3]: Game is out of sync, please check log files for details.";
	    abort();
		throw runtime_error(sError);
	}

	CardinalDir facing;
	// get facing/target ... the target might be dead due to lag, cope with it
	if (ct->getClass() == ccBuild) {
		//assert(networkCommand->getTargetId() >= 0 && networkCommand->getTargetId() < 4);
		if(networkCommand->getTargetId() < 0 || networkCommand->getTargetId() >= 4) {
			char szBuf[1024]="";
			sprintf(szBuf,"networkCommand->getTargetId() >= 0 && networkCommand->getTargetId() < 4, [%s]",networkCommand->toString().c_str());
			throw runtime_error(szBuf);
		}
		facing = CardinalDir(networkCommand->getTargetId());
	}
	else if (networkCommand->getTargetId() != Unit::invalidId ) {
		target= world->findUnitById(networkCommand->getTargetId());
	}

	//create command
	Command *command= NULL;
	if(unitType != NULL) {
		command= new Command(ct, networkCommand->getPosition(), unitType, facing);
	}
	else if(target == NULL) {
		command= new Command(ct, networkCommand->getPosition());
	}
	else {
		command= new Command(ct, target);
	}

	// Add in any special state
	CommandStateType commandStateType 	= networkCommand->getCommandStateType();
	int commandStateValue 				= networkCommand->getCommandStateValue();

	command->setStateType(commandStateType);
	command->setStateValue(commandStateValue);
	command->setUnitCommandGroupId(networkCommand->getUnitCommandGroupId());

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//issue command
	return command;
}

}}//end namespace
