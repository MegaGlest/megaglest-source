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

#ifndef _GLEST_GAME_NETWORKTYPES_H_
#define _GLEST_GAME_NETWORKTYPES_H_

#include <string>

#include "types.h"
#include "vec.h"
#include "command.h"
#include "leak_dumper.h"

using std::string;
using std::min;
using Shared::Platform::int8;
using Shared::Platform::uint8;
using Shared::Platform::int16;
using Shared::Platform::uint16;
using Shared::Platform::int32;
using Shared::Graphics::Vec2i;

namespace Glest{ namespace Game{

class World;

// =====================================================
//	class NetworkString
// =====================================================

template<int S>
class NetworkString{
private:
	char buffer[S];

public:
	NetworkString()	{
		memset(buffer, 0, S);
	}
	NetworkString & operator=(const string& str) {
		// ensure we don't have a buffer overflow
		int maxBufferSize = sizeof(buffer) / sizeof(buffer[0]);
		strncpy(buffer, str.c_str(), min(S-1,maxBufferSize-1));

		return *this;
	}
	void nullTerminate() {
		int maxBufferSize = sizeof(buffer) / sizeof(buffer[0]);
		buffer[maxBufferSize-1] = '\0';
	}

	string getString() const { return (buffer[0] != '\0' ? buffer : ""); }
};

// =====================================================
//	class NetworkCommand
// =====================================================

enum NetworkCommandType {
	nctGiveCommand,
	nctCancelCommand,
	nctSetMeetingPoint,
	nctSwitchTeam,
	nctSwitchTeamVote,
	nctPauseResume
	//nctNetworkCommand
};

//enum NetworkCommandSubType {
//	ncstRotateUnit
//};

#pragma pack(push, 1)
class NetworkCommand {
private:
	int16 networkCommandType;
	int32 unitId;
	int16 unitTypeId;
	int16 commandTypeId;
	int16 positionX;
	int16 positionY;
	int32 targetId;
	int8 wantQueue;
	int8 fromFactionIndex;
	uint16 unitFactionUnitCount;
	int8 unitFactionIndex;
	int8 commandStateType;
	int32 commandStateValue;
	int32 unitCommandGroupId;

public:
	NetworkCommand(){};
	NetworkCommand(
		World *world,
		int networkCommandType,
		int unitId,
		int commandTypeId= -1,
		const Vec2i &pos= Vec2i(0),
		int unitTypeId= -1,
		int targetId= -1,
		int facing= -1,
		bool wantQueue = false,
		CommandStateType commandStateType = cst_None,
		int commandTypeStateValue = -1,
		int unitCommandGroupId = -1);

	NetworkCommandType getNetworkCommandType() const	{return static_cast<NetworkCommandType>(networkCommandType);}
	int getUnitId() const								{return unitId;}
	int getCommandTypeId() const						{return commandTypeId;}
	Vec2i getPosition() const							{return Vec2i(positionX, positionY);}
	int getUnitTypeId() const							{return unitTypeId;}
	int getTargetId() const								{return targetId;}
	int getWantQueue() const							{return wantQueue;}
	int getFromFactionIndex() const						{return fromFactionIndex;}
	int getUnitFactionUnitCount() const					{return unitFactionUnitCount;}
	int getUnitFactionIndex() const						{return unitFactionIndex;}

	CommandStateType getCommandStateType() const 		{return static_cast<CommandStateType>(commandStateType);}
	int getCommandStateValue() const				 	{return commandStateValue;}

	int getUnitCommandGroupId() const					{ return unitCommandGroupId; }

    void preprocessNetworkCommand(World *world);
	string toString() const;
};
#pragma pack(pop)

}}//end namespace

#endif
