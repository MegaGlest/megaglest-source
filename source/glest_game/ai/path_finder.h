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
#include "fast_path_finder.h"
#include "leak_dumper.h"

using std::vector;
using Shared::Graphics::Vec2i;

namespace Glest { namespace Game {

//class Map;
//class Unit;

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
	class BadUnitNodeList {
	public:
		BadUnitNodeList() {
			unitSize = -1;
			//teamIndex = -1;
			field = fLand;
		}
		int unitSize;
		//int teamIndex;
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
			badCellList.clear();

			fa = createFastAstar();
		}
		~FactionState() {
			//fa = NULL;
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

		std::map<int,std::map<Field,BadUnitNodeList> > badCellList;

		FastAstar *fa;
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
	bool minorDebugPathfinder;

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
	TravelState aStarFast(Unit *unit, Vec2i finalPos, bool inBailout, int frameIndex, int maxNodeCount=-1,uint32 *searched_node_count=NULL);
	TravelState aStar(Unit *unit, const Vec2i &finalPos, bool inBailout, int frameIndex, int maxNodeCount=-1,uint32 *searched_node_count=NULL);
	//Node *newNode(FactionState &faction,int maxNodeCount);
	inline static Node *newNode(FactionState &faction, int maxNodeCount) {
		if( faction.nodePoolCount < faction.nodePool.size() &&
			//faction.nodePoolCount < faction.useMaxNodeCount) {
			faction.nodePoolCount < maxNodeCount) {
			Node *node= &(faction.nodePool[faction.nodePoolCount]);
			node->clear();
			faction.nodePoolCount++;
			return node;
		}
		return NULL;
	}

	Vec2i computeNearestFreePos(const Unit *unit, const Vec2i &targetPos);
	//float heuristic(const Vec2i &pos, const Vec2i &finalPos);
	inline static float heuristic(const Vec2i &pos, const Vec2i &finalPos) {
		return pos.dist(finalPos);
	}

	//bool openPos(const Vec2i &sucPos,FactionState &faction);
	inline static bool openPos(const Vec2i &sucPos, FactionState &faction) {
		if(faction.openPosList.find(sucPos) == faction.openPosList.end()) {
			return false;
		}
		return true;
	}


	//Node * minHeuristicFastLookup(FactionState &faction);
	inline static Node * minHeuristicFastLookup(FactionState &faction) {
		assert(faction.openNodesList.empty() == false);
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


	//bool processNode(Unit *unit, Node *node,const Vec2i finalPos, int i, int j, bool &nodeLimitReached, int maxNodeCount);
	inline bool processNode(Unit *unit, Node *node,const Vec2i finalPos, int i, int j, bool &nodeLimitReached,int maxNodeCount) {
		bool result = false;
		Vec2i sucPos= node->pos + Vec2i(i, j);

		int unitFactionIndex = unit->getFactionIndex();
	//	std::map<int, std::map<Vec2i,std::map<Vec2i, bool> > >::iterator iterFind1 = factions[unitFactionIndex].mapFromToNodeList.find(unit->getType()->getId());
	//	if(iterFind1 != factions[unitFactionIndex].mapFromToNodeList.end()) {
	//		std::map<Vec2i,std::map<Vec2i, bool> >::iterator iterFind2 = iterFind1->second.find(node->pos);
	//		if(iterFind2 != iterFind1->second.end()) {
	//			std::map<Vec2i, bool>::iterator iterFind3 = iterFind2->second.find(sucPos);
	//			if(iterFind3 != iterFind2->second.end()) {
	//				//printf("found duplicate check in processNode\n");
	//				return iterFind3->second;
	//			}
	//		}
	//	}

		//bool canUnitMoveToCell = map->aproxCanMove(unit, node->pos, sucPos);
		//bool canUnitMoveToCell = map->aproxCanMoveSoon(unit, node->pos, sucPos);
		if(openPos(sucPos, factions[unitFactionIndex]) == false &&
				canUnitMoveSoon(unit, node->pos, sucPos) == true) {
			//if node is not open and canMove then generate another node
			Node *sucNode= newNode(factions[unitFactionIndex],maxNodeCount);
			if(sucNode != NULL) {
				sucNode->pos= sucPos;
				sucNode->heuristic= heuristic(sucNode->pos, finalPos);
				sucNode->prev= node;
				sucNode->next= NULL;
				sucNode->exploredCell= map->getSurfaceCell(Map::toSurfCoords(sucPos))->isExplored(unit->getTeam());
				if(factions[unitFactionIndex].openNodesList.find(sucNode->heuristic) == factions[unitFactionIndex].openNodesList.end()) {
					factions[unitFactionIndex].openNodesList[sucNode->heuristic].clear();
					//factions[unitFactionIndex].openNodesList[sucNode->heuristic].reserve(PathFinder::pathFindNodesMax);
				}
				factions[unitFactionIndex].openNodesList[sucNode->heuristic].push_back(sucNode);
				factions[unitFactionIndex].openPosList[sucNode->pos] = true;

				result = true;
			}
			else {
				nodeLimitReached= true;
			}
		}

	//	factions[unitFactionIndex].mapFromToNodeList[unit->getType()->getId()][node->pos][sucPos] = result;
		return result;
	}

	void processNearestFreePos(const Vec2i &finalPos, int i, int j, int size, Field field, int teamIndex,Vec2i unitPos, Vec2i &nearestPos, float &nearestDist);
	int getPathFindExtendRefreshNodeCount(int factionIndex);


	void astarJPS(std::map<Vec2i,Vec2i> cameFrom, Node *& node,
			const Vec2i & finalPos, std::map<Vec2i,bool> closedNodes,
			std::map<std::pair<Vec2i,Vec2i> ,bool> canAddNode, Unit *& unit,
			bool & nodeLimitReached, int & maxNodeCount);

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

	//bool canUnitMoveSoon(const Unit *unit, const Vec2i &pos1, const Vec2i &pos2);
	inline bool canUnitMoveSoon(const Unit *unit, const Vec2i &pos1, const Vec2i &pos2) {
		bool result = true;

	//	std::map<int,std::map<Field,BadUnitNodeList> > &badCellList = factions[unit->getFactionIndex()].badCellList;
	//	if(badCellList.find(unit->getType()->getSize()) != badCellList.end()) {
	//		std::map<Field,BadUnitNodeList> &badFieldList = badCellList[unit->getType()->getSize()];
	//		if(badFieldList.find(unit->getCurrField()) != badFieldList.end()) {
	//			BadUnitNodeList &badList = badFieldList[unit->getCurrField()];
	//			if(badList.isPosBad(pos1,pos2) == true) {
	//				result = false;
	//			}
	//		}
	//	}
	//	if(result == true) {
	//		//bool canUnitMoveToCell = map->canMove(unit, unitPos, pos);
	//		//bool canUnitMoveToCell = map->aproxCanMove(unit, unitPos, pos);
	//		result = map->aproxCanMoveSoon(unit, pos1, pos2);
	//		if(result == false) {
	//			badCellList[unit->getType()->getSize()][unit->getCurrField()].badPosList[pos1][pos2]=false;
	//		}
	//	}

		result = map->aproxCanMoveSoon(unit, pos1, pos2);
		return result;
	}

	inline void doAStarPathSearch(bool & nodeLimitReached, int & whileLoopCount,
			int & unitFactionIndex, bool & pathFound, Node *& node, const Vec2i & finalPos,
			const bool tryJPSPathfinder, std::map<Vec2i,bool> closedNodes,
			std::map<Vec2i,Vec2i> cameFrom, std::map<std::pair<Vec2i,Vec2i> ,
			bool> canAddNode, Unit *& unit, int & maxNodeCount, int curFrameIndex)  {

		Chrono chrono;
		//if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();
		chrono.start();

		FactionState &factionState = factions[unitFactionIndex];
		while(nodeLimitReached == false) {
			whileLoopCount++;
			if(factionState.openNodesList.empty() == true) {
				pathFound = false;
				break;
			}
			node = minHeuristicFastLookup(factionState);
			if(node->pos == finalPos || node->exploredCell == false) {
				pathFound = true;
				break;
			}
			if(tryJPSPathfinder == true) {
				closedNodes[node->pos] = true;
			}
			if(factionState.closedNodesList.find(node->heuristic) == factionState.closedNodesList.end()) {
				factionState.closedNodesList[node->heuristic].clear();
				//factionState.closedNodesList[node->heuristic].reserve(PathFinder::pathFindNodesMax);
			}
			factionState.closedNodesList[node->heuristic].push_back(node);
			factionState.openPosList[node->pos] = true;
			if(tryJPSPathfinder == true) {
				astarJPS(cameFrom, node, finalPos, closedNodes, canAddNode, unit, nodeLimitReached, maxNodeCount);
			}
			else {
				int failureCount 	= 0;
				int cellCount 		= 0;
				int tryDirection 	= factionState.random.randRange(0, 3);

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

		//!!!
		//if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled == true && chrono.getMillis() > 1) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld nodeLimitReached = %d whileLoopCount = %d nodePoolCount = %d\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis(),nodeLimitReached,whileLoopCount,factionState.nodePoolCount);
		if(chrono.getMillis() > 1) {
			//printf("AStar for unit [%d - %s] took msecs: %lld nodeLimitReached = %d whileLoopCount = %d nodePoolCount = %d curFrameIndex = %d travel distance = %f\n",unit->getId(),unit->getFullName().c_str(), (long long int)chrono.getMillis(),nodeLimitReached,whileLoopCount,factionState.nodePoolCount,curFrameIndex,unit->getPos().dist(finalPos));
		}
	}

};

}}//end namespace

#endif
