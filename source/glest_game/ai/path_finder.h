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
		Node() : pos(0,0) {
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

public:
	static const int maxFreeSearchRadius;
	static const int pathFindNodesMax;
	static const int pathFindRefresh;

private:
	std::map<Vec2i, bool> openPosList;
	std::map<float, Nodes> openNodesList;
	std::map<float, Nodes> closedNodesList;

	std::vector<Node> nodePool;
	int nodePoolCount;
	const Map *map;

	//std::map<Vec2i, std::map<Vec2i, std::map<int, std::map<Field,bool> > > > lookupCacheCanMove;
	std::map<Vec2i, std::map<Vec2i, std::map<int, std::map<int, std::map<Field,bool> > > > > moveLookupCacheApproxCanMove;

public:
	PathFinder();
	PathFinder(const Map *map);
	~PathFinder();
	void init(const Map *map);
	TravelState findPath(Unit *unit, const Vec2i &finalPos, bool *wasStuck=NULL,bool clearLookupCache=true);

private:
	TravelState aStar(Unit *unit, const Vec2i &finalPos, bool inBailout);
	Node *newNode();
	Vec2i computeNearestFreePos(const Unit *unit, const Vec2i &targetPos);
	float heuristic(const Vec2i &pos, const Vec2i &finalPos);
	bool openPos(const Vec2i &sucPos);

	Node * minHeuristicFastLookup();
};

}}//end namespace

#endif
