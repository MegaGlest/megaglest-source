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

enum ControlType{
    ctClosed,
	ctCpuEasy,
	ctCpu,
	ctCpuUltra,
	ctCpuMega,
	ctNetwork,
	ctHuman
};

class GameConstants {
public:
	static const int maxPlayers= 8;
	static const int serverPort= 61357;
	static const int updateFps= 40;
	static const int cameraFps= 100;
	static const int networkFramePeriod= 10;
	static const int networkExtraLatency= 200;

	static const char *folder_path_maps;
	static const char *folder_path_scenarios;
	static const char *folder_path_techs;
	static const char *folder_path_tilesets;
	static const char *folder_path_tutorials;
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
	enum Enum { NORTH, EAST, SOUTH, WEST };

	CardinalDir() : value(NORTH) {}
	CardinalDir(Enum v) : value(v) {}
	explicit CardinalDir(int v) {
		assertDirValid(v);
		value = static_cast<Enum>(v);
	}
	operator Enum() { return value; }
	//int asInt() { return (int)value; }

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
