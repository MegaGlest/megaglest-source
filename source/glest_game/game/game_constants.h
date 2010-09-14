#ifndef _GLEST_GAME_GAMECONSTANTS_H_
#define _GLEST_GAME_GAMECONSTANTS_H_

#include <cassert>
#include <stdio.h>

// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

namespace Glest{ namespace Game{

// =====================================================
//	class GameConstants
// =====================================================

enum PathFinderType {
	pfBasic,
	pfRoutePlanner
};

enum TravelState {
	tsArrived,
	tsMoving,
	tsBlocked,
	tsImpossible
};

enum ControlType {
    ctClosed,
	ctCpuEasy,
	ctCpu,
	ctCpuUltra,
	ctCpuMega,
	ctNetwork,
	ctHuman,

	ctNetworkCpuEasy,
	ctNetworkCpu,
	ctNetworkCpuUltra,
	ctNetworkCpuMega

};

enum NetworkRole {
	nrServer,
	nrClient,
	nrIdle
};

enum FactionPersonalityType {
	fpt_Normal,
	fpt_Observer,

	fpt_EndCount
};

class GameConstants {
public:
	static const int specialFactions = fpt_EndCount - 1;
	static const int maxPlayers= 8;
	static const int serverPort= 61357;
	//static const int updateFps= 40;
	//static const int cameraFps= 100;
	static int updateFps;
	static int cameraFps;

	static int networkFramePeriod;
	static const int networkPingInterval = 5;
	//static const int networkExtraLatency= 200;
	static const int maxClientConnectHandshakeSecs= 10;

	static const int cellScale = 2;
	static const int clusterSize = 16;

	static const char *folder_path_maps;
	static const char *folder_path_scenarios;
	static const char *folder_path_techs;
	static const char *folder_path_tilesets;
	static const char *folder_path_tutorials;

	static const char *NETWORK_SLOT_UNCONNECTED_SLOTNAME;

	static const char *folder_path_screenshots;

	static const char *OBSERVER_SLOTNAME;
	static const char *RANDOMFACTION_SLOTNAME;
};

enum PathType {
    ptMaps,
	ptScenarios,
	ptTechs,
	ptTilesets,
	ptTutorials
};

struct CardinalDir {
public:
	enum Enum { NORTH, EAST, SOUTH, WEST, COUNT };

	CardinalDir() : value(NORTH) {}
	CardinalDir(Enum v) : value(v) {}
	explicit CardinalDir(int v) {
		assertDirValid(v);
		value = static_cast<Enum>(v);
	}
	operator Enum() const { return value; }
	int asInt() const { return (int)value; }

	static void assertDirValid(int v) { assert(v >= 0 && v < 4); }
	void operator++() {
		//printf("In [%s::%s] Line: %d BEFORE +: value = %d\n",__FILE__,__FUNCTION__,__LINE__,asInt());
		value = static_cast<Enum>((value + 1) % 4);
		//printf("In [%s::%s] Line: %d AFTER +: value = %d\n",__FILE__,__FUNCTION__,__LINE__,asInt());
	}
	void operator--() { // mod with negative numbers is a 'grey area', hence the +3 rather than -1
		value = static_cast<Enum>((value + 3) % 4);
	}

private:
	Enum value;
};

}}//end namespace

#endif
