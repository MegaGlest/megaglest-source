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

// The order of directions is:
// N, NE, E, SE, S, SW, W, NW
typedef unsigned char direction;
#define NO_DIRECTION 8
typedef unsigned char directionset;

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
			//mapFromToNodeList.clear();
			//lastFromToNodeListFrame = -100;
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

		//int lastFromToNodeListFrame;
		//std::map<int, std::map<Vec2i,std::map<Vec2i, bool> > > mapFromToNodeList;
	};
	typedef vector<FactionState> FactionStateList;

public:
	static const int maxFreeSearchRadius;
	static const int pathFindRefresh;
	static const int pathFindBailoutRadius;
	static const int pathFindExtendRefreshForNodeCount;
	static const int pathFindExtendRefreshNodeCountMin;
	static const int pathFindExtendRefreshNodeCountMax;

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

	int findNodeIndex(Node *node, Nodes &nodeList);
	int findNodeIndex(Node *node, std::vector<Node> &nodeList);

	void saveGame(XmlNode *rootNode);
	void loadGame(const XmlNode *rootNode);

private:
	TravelState aStar(Unit *unit, const Vec2i &finalPos, bool inBailout, int frameIndex, int maxNodeCount=-1);
	Node *newNode(FactionState &faction,int maxNodeCount);
	Vec2i computeNearestFreePos(const Unit *unit, const Vec2i &targetPos);
	float heuristic(const Vec2i &pos, const Vec2i &finalPos);
	bool openPos(const Vec2i &sucPos,FactionState &faction);

	Node * minHeuristicFastLookup(FactionState &faction);

	bool processNode(Unit *unit, Node *node,const Vec2i finalPos, int i, int j, bool &nodeLimitReached, int maxNodeCount);
	void processNearestFreePos(const Vec2i &finalPos, int i, int j, int size, Field field, int teamIndex,Vec2i unitPos, Vec2i &nearestPos, float &nearestDist);
	int getPathFindExtendRefreshNodeCount(int factionIndex);


	bool contained(Vec2i c);
	direction directionOfMove(Vec2i to, Vec2i from) const;
	direction directionWeCameFrom(Vec2i node, Vec2i nodeFrom) const;
	bool isEnterable(Vec2i coord);
	Vec2i adjustInDirection(Vec2i c, int dir);
	bool directionIsDiagonal(direction dir) const;
	directionset forcedNeighbours(Vec2i coord,direction dir);
	bool implies(bool a, bool b) const;
	directionset addDirectionToSet(directionset dirs, direction dir) const;
	directionset naturalNeighbours(direction dir) const;
	direction nextDirectionInSet(directionset *dirs) const;
	Vec2i jump(Vec2i dest, direction dir, Vec2i start,std::vector<Vec2i> &path,int pathLength);
	bool addToOpenSet(Unit *unit, Node *node,const Vec2i finalPos, Vec2i sucPos, bool &nodeLimitReached,int maxNodeCount,Node **newNodeAdded,bool bypassChecks);
};

}}//end namespace

#endif
