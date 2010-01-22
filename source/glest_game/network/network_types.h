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

enum NetworkCommandType{
	nctGiveCommand,
	nctCancelCommand,
	nctSetMeetingPoint
};

class NetworkCommand{
private:
	int16 networkCommandType;
	int16 unitId;
	int16 commandTypeId;
	int16 positionX;
	int16 positionY;
	int16 unitTypeId;
	int16 targetId;

public:
	NetworkCommand(){};
	NetworkCommand(int networkCommandType, int unitId, int commandTypeId= -1, const Vec2i &pos= Vec2i(0), int unitTypeId= -1, int targetId= -1);

	NetworkCommandType getNetworkCommandType() const	{return static_cast<NetworkCommandType>(networkCommandType);}
	int getUnitId() const								{return unitId;}
	int getCommandTypeId() const						{return commandTypeId;}
	Vec2i getPosition() const							{return Vec2i(positionX, positionY);}
	int getUnitTypeId() const							{return unitTypeId;}
	int getTargetId() const								{return targetId;}
};

}}//end namespace

#endif
