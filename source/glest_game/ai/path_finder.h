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

#ifdef WIN32
    #include <winsock2.h>
    #include <winsock.h>
#endif

#include "vec.h"
#include <vector>
#include <map>
#include "game_constants.h"
#include "skill_type.h"
#include "map.h"
#include "unit.h"

#include "leak_dumper.h"

using std::vector;
using Shared::Graphics::Vec2i;

namespace Glest { namespace Game {

// =====================================================
// 	class PathFinder
//
///	Finds paths for units using a modification of the A* algorithm
// =====================================================

class PathFinder {
public:
	class BadUnitNodeList {
	public:
		BadUnitNodeList() {
			unitSize = -1;
			field = fLand;
		}
		int unitSize;
		Field field;
		std::map<Vec2i, std::map<Vec2i,bool> > badPosList;

		inline bool isPosBad(const Vec2i &pos1,const Vec2i &pos2) {
			bool result = false;

			std::map<Vec2i, std::map<Vec2i,bool> >::iterator iterFind = badPosList.find(pos1);
			if(iterFind != badPosList.end()) {
				std::map<Vec2i,bool>::iterator iterFind2 = iterFind->second.find(pos2);
				if(iterFind2 != iterFind->second.end()) {
					result = true;
				}
			}

			return result;
		}
	};

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
	protected:
		Mutex *factionMutexPrecache;
	public:
		FactionState() :
			//factionMutexPrecache(new Mutex) {
			factionMutexPrecache(NULL) {

			openPosList.clear();
			openNodesList.clear();
			closedNodesList.clear();
			nodePool.clear();
			nodePoolCount = 0;
			useMaxNodeCount = 0;

			precachedTravelState.clear();
			precachedPath.clear();
		}
		~FactionState() {

			delete factionMutexPrecache;
			factionMutexPrecache = NULL;
		}
		Mutex * getMutexPreCache() {
			return factionMutexPrecache;
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

	class FactionStateManager {
	protected:
		typedef vector<FactionState *> FactionStateList;
		FactionStateList factions;

		void init() {
			for(int index = 0; index < GameConstants::maxPlayers; ++index) {
				factions.push_back(new FactionState());
			}
		}

	public:
		FactionStateManager() {
			init();
		}
		~FactionStateManager() {
			clear();
		}

		FactionState & getFactionState(int index) {
			FactionState *faction = factions[index];
			return *faction;
		}
		void clear() {
			for(unsigned int index = 0; index < (unsigned int)factions.size(); ++index) {
				delete factions[index];
			}

			factions.clear();
		}
		int size() {
			return (int)factions.size();
		}
	};

public:
	static const int maxFreeSearchRadius;

	static const int pathFindBailoutRadius;
	static const int pathFindExtendRefreshForNodeCount;
	static const int pathFindExtendRefreshNodeCountMin;
	static const int pathFindExtendRefreshNodeCountMax;

private:

	static int pathFindNodesMax;
	static int pathFindNodesAbsoluteMax;


	FactionStateManager factions;
	const Map *map;
	bool minorDebugPathfinder;

public:
	PathFinder();
	PathFinder(const Map *map);
	~PathFinder();

	PathFinder(const PathFinder& obj) {
		init();
		throw megaglest_runtime_error("class PathFinder is NOT safe to copy!");
	}
	PathFinder & operator=(const PathFinder& obj) {
		init();
		throw megaglest_runtime_error("class PathFinder is NOT safe to assign!");
	}

	void init(const Map *map);
	TravelState findPath(Unit *unit, const Vec2i &finalPos, bool *wasStuck=NULL,int frameIndex=-1);
	void clearUnitPrecache(Unit *unit);
	void removeUnitPrecache(Unit *unit);
	void clearCaches();

	bool unitCannotMove(Unit *unit);

	int findNodeIndex(Node *node, Nodes &nodeList);
	int findNodeIndex(Node *node, std::vector<Node> &nodeList);

	void saveGame(XmlNode *rootNode);
	void loadGame(const XmlNode *rootNode);

private:
	void init();

	TravelState aStar(Unit *unit, const Vec2i &finalPos, bool inBailout,
			int frameIndex, int maxNodeCount=-1,uint32 *searched_node_count=NULL);
	inline static Node *newNode(FactionState &faction, int maxNodeCount) {
		if( faction.nodePoolCount < (int)faction.nodePool.size() &&
			faction.nodePoolCount < maxNodeCount) {
			Node *node= &(faction.nodePool[faction.nodePoolCount]);
			node->clear();
			faction.nodePoolCount++;
			return node;
		}
		return NULL;
	}

	Vec2i computeNearestFreePos(const Unit *unit, const Vec2i &targetPos);
	inline static float heuristic(const Vec2i &pos, const Vec2i &finalPos) {
		return pos.dist(finalPos);
	}

	inline static bool openPos(const Vec2i &sucPos, FactionState &faction) {
		if(faction.openPosList.find(sucPos) == faction.openPosList.end()) {
			return false;
		}
		return true;
	}

	inline static Node * minHeuristicFastLookup(FactionState &faction) {
		if(faction.openNodesList.empty() == true) {
			throw megaglest_runtime_error("openNodesList.empty() == true");
		}

		Node *result = faction.openNodesList.begin()->second[0];
		faction.openNodesList.begin()->second.erase(faction.openNodesList.begin()->second.begin());
		if(faction.openNodesList.begin()->second.size() == 0) {
			faction.openNodesList.erase(faction.openNodesList.begin());
		}
		return result;
	}

	inline bool processNode(Unit *unit, Node *node,const Vec2i finalPos,
			int i, int j, bool &nodeLimitReached,int maxNodeCount) {
		bool result = false;
		Vec2i sucPos= node->pos + Vec2i(i, j);

		int unitFactionIndex = unit->getFactionIndex();
		FactionState &faction = factions.getFactionState(unitFactionIndex);

		if(openPos(sucPos, faction) == false &&
				canUnitMoveSoon(unit, node->pos, sucPos) == true) {
			//if node is not open and canMove then generate another node
			Node *sucNode= newNode(faction,maxNodeCount);
			if(sucNode != NULL) {
				sucNode->pos= sucPos;
				sucNode->heuristic= heuristic(sucNode->pos, finalPos);
				sucNode->prev= node;
				sucNode->next= NULL;
				sucNode->exploredCell = map->getSurfaceCell(
						Map::toSurfCoords(sucPos))->isExplored(unit->getTeam());
				if(faction.openNodesList.find(sucNode->heuristic) == faction.openNodesList.end()) {
					faction.openNodesList[sucNode->heuristic].clear();
				}
				faction.openNodesList[sucNode->heuristic].push_back(sucNode);
				faction.openPosList[sucNode->pos] = true;

				result = true;
			}
			else {
				nodeLimitReached= true;
			}
		}

		return result;
	}

	void processNearestFreePos(const Vec2i &finalPos, int i, int j, int size,
			Field field, int teamIndex,Vec2i unitPos, Vec2i &nearestPos, float &nearestDist);
	int getPathFindExtendRefreshNodeCount(FactionState &faction);

	inline bool canUnitMoveSoon(const Unit *unit, const Vec2i &pos1, const Vec2i &pos2) {
		bool result = map->aproxCanMoveSoon(unit, pos1, pos2);
		return result;
	}

	inline void doAStarPathSearch(bool & nodeLimitReached, int & whileLoopCount,
			int & unitFactionIndex, bool & pathFound, Node *& node, const Vec2i & finalPos,
			std::map<Vec2i,bool> closedNodes,
			std::map<Vec2i,Vec2i> cameFrom, std::map<std::pair<Vec2i,Vec2i> ,
			bool> canAddNode, Unit *& unit, int & maxNodeCount, int curFrameIndex)  {

		FactionState &faction = factions.getFactionState(unitFactionIndex);

		while(nodeLimitReached == false) {
			whileLoopCount++;
			if(faction.openNodesList.empty() == true) {
				pathFound = false;
				break;
			}
			node = minHeuristicFastLookup(faction);
			if(node->pos == finalPos || node->exploredCell == false) {
				pathFound = true;
				break;
			}

			if(faction.closedNodesList.find(node->heuristic) == faction.closedNodesList.end()) {
				faction.closedNodesList[node->heuristic].clear();
			}
			faction.closedNodesList[node->heuristic].push_back(node);
			faction.openPosList[node->pos] = true;

			int failureCount 	= 0;
			int cellCount 		= 0;

			int tryDirection 	= faction.random.randRange(0, 3);
			if(tryDirection == 3) {
				for(int i = 1;i >= -1 && nodeLimitReached == false;--i) {
					for(int j = -1;j <= 1 && nodeLimitReached == false;++j) {
						if(processNode(unit, node, finalPos, i, j, nodeLimitReached, maxNodeCount) == false) {
							failureCount++;
						}
						cellCount++;
					}
				}
			}
			else if(tryDirection == 2) {
				for(int i = -1;i <= 1 && nodeLimitReached == false;++i) {
					for(int j = 1;j >= -1 && nodeLimitReached == false;--j) {
						if(processNode(unit, node, finalPos, i, j, nodeLimitReached, maxNodeCount) == false) {
							failureCount++;
						}
						cellCount++;
					}
				}
			}
			else if(tryDirection == 1) {
				for(int i = -1;i <= 1 && nodeLimitReached == false;++i) {
					for(int j = -1;j <= 1 && nodeLimitReached == false;++j) {
						if(processNode(unit, node, finalPos, i, j, nodeLimitReached, maxNodeCount) == false) {
							failureCount++;
						}
						cellCount++;
					}
				}
			}
			else {
				for(int i = 1;i >= -1 && nodeLimitReached == false;--i) {
					for(int j = 1;j >= -1 && nodeLimitReached == false;--j) {
						if(processNode(unit, node, finalPos, i, j, nodeLimitReached, maxNodeCount) == false) {
							failureCount++;
						}
						cellCount++;
					}
				}
			}
		}
	}

};

}}//end namespace

#endif
