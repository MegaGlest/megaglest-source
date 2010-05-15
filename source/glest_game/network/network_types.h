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

using std::string;
using Shared::Platform::int8;
using Shared::Platform::int16;
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
	NetworkString()						{memset(buffer, 0, S);}
	void operator=(const string& str)	{strncpy(buffer, str.c_str(), S-1);}
	string getString() const			{return buffer;}
};

// =====================================================
//	class NetworkCommand
// =====================================================

enum NetworkCommandType {
	nctGiveCommand,
	nctCancelCommand,
	nctSetMeetingPoint
	//nctNetworkCommand
};

//enum NetworkCommandSubType {
//	ncstRotateUnit
//};

#pragma pack(push, 2)
class NetworkCommand{
private:
	int16 networkCommandType;
	int16 unitId;
	int16 commandTypeId;
	int16 positionX;
	int16 positionY;
	int16 unitTypeId;
	int16 targetId;
	int16 wantQueue;
	int16 fromFactionIndex;

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
		bool wantQueue = false);
	
	//NetworkCommand(int networkCommandType, NetworkCommandSubType ncstType, int unitId, int value1, int value2=-1);

	NetworkCommandType getNetworkCommandType() const	{return static_cast<NetworkCommandType>(networkCommandType);}
	int getUnitId() const								{return unitId;}
	int getCommandTypeId() const						{return commandTypeId;}
	Vec2i getPosition() const							{return Vec2i(positionX, positionY);}
	int getUnitTypeId() const							{return unitTypeId;}
	int getTargetId() const								{return targetId;}
	int getWantQueue() const							{return wantQueue;}
	int getFromFactionIndex() const						{return fromFactionIndex;}

    void preprocessNetworkCommand(World *world);
	string toString() const;
};
#pragma pack(pop)

}}//end namespace

#endif
