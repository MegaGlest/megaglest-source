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

#ifndef _GLEST_GAME_COMMANDER_H_
#define _GLEST_GAME_COMMANDER_H_

#ifdef WIN32
    #include <winsock2.h>
    #include <winsock.h>
#endif

#include <vector>
#include "vec.h"
#include "selection.h"
#include "command_type.h"
#include "platform_util.h"
#include "base_thread.h"
#include "leak_dumper.h"

using std::vector;

namespace Glest{ namespace Game{

using Shared::Graphics::Vec2i;
using Shared::PlatformCommon::Chrono;

class World;
class Unit;
class Command;
class CommandType;
class NetworkCommand;
class Game;
class SwitchTeamVote;

// =====================================================
// 	class Commander
//
///	Gives commands to the units
// =====================================================

//
// This interface describes the methods a callback object must implement
//
class CommanderNetworkCallbackInterface {
public:
	virtual void commanderNetworkUpdateTask(int id) = 0;
	virtual ~CommanderNetworkCallbackInterface() {}
};

class CommanderNetworkThread : public BaseThread
{
protected:

	CommanderNetworkCallbackInterface *commanderInterface;
	Semaphore semTaskSignalled;

	virtual void setQuitStatus(bool value);
	virtual void setTaskCompleted(int id);
	Mutex idMutex;
	std::pair<int,bool> idStatus;

public:
	CommanderNetworkThread();
	CommanderNetworkThread(CommanderNetworkCallbackInterface *commanderInterface);
    virtual void execute();
    void signalUpdate(int id);
    bool isSignalCompleted(int id);
};

class Commander : public CommanderNetworkCallbackInterface {
private:
	typedef vector<std::pair<CommandResult,string> > CommandResultContainer;

private:
    World *world;
	Chrono perfTimer;

	Mutex commandMutex;
	//CommanderNetworkThread *networkThread;
	//Game *game;
	std::vector<std::pair<int,NetworkCommand> > replayCommandList;

public:
    Commander();
    virtual ~Commander();

    void signalNetworkUpdate(Game *game);
    void init(World *world);
	void updateNetwork(Game *game);

	void addToReplayCommandList(NetworkCommand &command,int worldFrameCount);
	bool getReplayCommandListForFrame(int worldFrameCount);
	bool hasReplayCommandListForFrame() const;
	int getReplayCommandListForFrameCount() const;

	std::pair<CommandResult,string> tryGiveCommand(const Selection *selection, const CommandType *commandType,
										const Vec2i &pos, const UnitType* unitType,
										CardinalDir facing, bool tryQueue,Unit *targetUnit=NULL) const;

	std::pair<CommandResult,string> tryGiveCommand(const Unit* unit, const CommandType *commandType, const Vec2i &pos, const UnitType* unitType, CardinalDir facing, bool tryQueue = false,Unit *targetUnit=NULL,int unitGroupCommandId=-1) const;
	std::pair<CommandResult,string> tryGiveCommand(const Selection *selection, CommandClass commandClass, const Vec2i &pos= Vec2i(0), const Unit *targetUnit= NULL, bool tryQueue = false) const;
	std::pair<CommandResult,string> tryGiveCommand(const Selection *selection, const CommandType *commandType, const Vec2i &pos= Vec2i(0), const Unit *targetUnit= NULL, bool tryQueue = false) const;
	std::pair<CommandResult,string> tryGiveCommand(const Selection *selection, const Vec2i &pos, const Unit *targetUnit= NULL, bool tryQueue = false, int unitCommandGroupId = -1) const;
	CommandResult tryCancelCommand(const Selection *selection) const;
	void trySetMeetingPoint(const Unit* unit, const Vec2i &pos) const;
	void trySwitchTeam(const Faction* faction, int teamIndex) const;
	void trySwitchTeamVote(const Faction* faction, SwitchTeamVote *vote) const;
	void tryDisconnectNetworkPlayer(const Faction* faction, int playerIndex) const;

	void tryPauseGame(bool joinNetworkGame, bool clearCaches) const;
	void tryResumeGame(bool joinNetworkGame, bool clearCaches) const;

	void tryNetworkPlayerDisconnected(int factionIndex) const;

	Command* buildCommand(const NetworkCommand* networkCommand) const;

private:
	std::pair<CommandResult,string> pushNetworkCommand(const NetworkCommand* networkCommand) const;
	//void giveNetworkCommandSpecial(const NetworkCommand* networkCommand) const;

	std::pair<CommandResult,string> computeResult(const CommandResultContainer &results) const;
	void giveNetworkCommand(NetworkCommand* networkCommand) const;

	virtual void commanderNetworkUpdateTask(int id);
	bool canSubmitCommandType(const Unit *unit, const CommandType *commandType) const;
};

}} //end namespace

#endif
