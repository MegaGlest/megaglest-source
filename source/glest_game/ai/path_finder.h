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

#ifndef _GLEST_GAME_PATHFINDER_H_
#define _GLEST_GAME_PATHFINDER_H_

#include "vec.h"

#include <vector>
#include <map>
#include "game_constants.h"
#include "skill_type.h"
#include "leak_dumper.h"

using std::vector;
using Shared::Graphics::Vec2i;

namespace Glest { namespace Game {

class Map;
class Unit;

// =====================================================
// 	class PathFinder
//
///	Finds paths for units using a modification of the A* algorithm
// =====================================================

class PathFinder {
public:
	class Node {
	public:
		Node() {
			clear();
		}
		void clear() {
			pos.x = 0;
			pos.y = 0;
			next=NULL;
			prev=NULL;
			heuristic=0.0;
			exploredCell=false;
		}
		Vec2i pos;
		Node *next;
		Node *prev;
		float heuristic;
		bool exploredCell;
	};
	typedef vector<Node*> Nodes;

	class FactionState {
	public:
		FactionState() {
			openPosList.clear();
			openNodesList.clear();
			closedNodesList.clear();
			nodePool.clear();
			nodePoolCount = 0;
			useMaxNodeCount = 0;
			precachedTravelState.clear();
			precachedPath.clear();
		}
		std::map<Vec2i, bool> openPosList;
		std::map<float, Nodes> openNodesList;
		std::map<float, Nodes> closedNodesList;
		std::vector<Node> nodePool;
		int nodePoolCount;
		RandomGen random;
		int useMaxNodeCount;

		std::map<int,TravelState> precachedTravelState;
		std::map<int,std::vector<Vec2i> > precachedPath;
	};
	typedef vector<FactionState> FactionStateList;

public:
	static const int maxFreeSearchRadius;
	static const int pathFindRefresh;
	static const int pathFindBailoutRadius;

private:

	static int pathFindNodesMax;
	static int pathFindNodesAbsoluteMax;

	FactionStateList factions;
	const Map *map;

public:
	PathFinder();
	PathFinder(const Map *map);
	~PathFinder();
	void init(const Map *map);
	TravelState findPath(Unit *unit, const Vec2i &finalPos, bool *wasStuck=NULL,int frameIndex=-1);
	void clearUnitPrecache(Unit *unit);
	void removeUnitPrecache(Unit *unit);

private:
	TravelState aStar(Unit *unit, const Vec2i &finalPos, bool inBailout, int frameIndex, int maxNodeCount=-1);
	Node *newNode(FactionState &faction,int maxNodeCount);
	Vec2i computeNearestFreePos(const Unit *unit, const Vec2i &targetPos);
	float heuristic(const Vec2i &pos, const Vec2i &finalPos);
	bool openPos(const Vec2i &sucPos,FactionState &faction);

	Node * minHeuristicFastLookup(FactionState &faction);

	bool processNode(Unit *unit, Node *node,const Vec2i finalPos, int i, int j, bool &nodeLimitReached, int maxNodeCount);
	void processNearestFreePos(const Vec2i &finalPos, int i, int j, int size, Field field, int teamIndex,Vec2i unitPos, Vec2i &nearestPos, float &nearestDist);
};

}}//end namespace

#endif
