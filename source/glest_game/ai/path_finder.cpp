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

#include "path_finder.h"

#include <algorithm>
#include <cassert>

#include "config.h"
#include "map.h"
#include "unit.h"
#include "unit_type.h"
#include "platform_common.h"
#include "command.h"
#include "faction.h"
#include "leak_dumper.h"

using namespace std;
using namespace Shared::Graphics;
using namespace Shared::Util;
using namespace Shared::PlatformCommon;

namespace Glest{ namespace Game{

// =====================================================
// 	class PathFinder
// =====================================================

// ===================== PUBLIC ======================== 

const int PathFinder::maxFreeSearchRadius= 10;	
//const int PathFinder::pathFindNodesMax= 400;
const int PathFinder::pathFindNodesMax= 500;
//const int PathFinder::pathFindRefresh= 10;
const int PathFinder::pathFindRefresh= 5;


PathFinder::PathFinder() {
	//nodePool= NULL;
	nodePool.clear();
	map=NULL;
}

PathFinder::PathFinder(const Map *map) {
	//nodePool= NULL;
	nodePool.clear();
	map=NULL;
	init(map);
}

void PathFinder::init(const Map *map) {
	//if(nodePool != NULL) {
	//	delete [] nodePool;
	//	nodePool = NULL;
	//}
	//nodePool= new Node[pathFindNodesMax];
	nodePool.resize(pathFindNodesMax);

	this->map= map;
}

PathFinder::~PathFinder(){
	//delete [] nodePool;
	//nodePool = NULL;
	nodePool.clear();
	map=NULL;
}

TravelState PathFinder::findPath(Unit *unit, const Vec2i &finalPos, bool *wasStuck) {
	
	if(map == NULL) {
		throw runtime_error("map == NULL");
	}
	//route cache
	UnitPathInterface *path= unit->getPath();
	if(finalPos == unit->getPos()) {
		//if arrived
		unit->setCurrSkill(scStop);

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPathFinder).enabled == true) {
			string commandDesc = "none";
			Command *command= unit->getCurrCommand();
			if(command != NULL && command->getCommandType() != NULL) {
				commandDesc = command->getCommandType()->toString();
			}
			char szBuf[1024]="";
			sprintf(szBuf,"State: arrived#1 at pos: %s, command [%s]",finalPos.getString().c_str(),commandDesc.c_str());
			unit->setCurrentUnitTitle(szBuf);
		}

		unit->getFaction()->addCachedPath(finalPos,unit);
		return tsArrived;
	}
	else {
		if(path->isEmpty() == false) {
			if(dynamic_cast<UnitPathBasic *>(path) != NULL) {
				//route cache
				UnitPathBasic *basicPath = dynamic_cast<UnitPathBasic *>(path);
				Vec2i pos= basicPath->pop();
				if(map->canMove(unit, unit->getPos(), pos)) {
					unit->setTargetPos(pos);
					unit->addCurrentTargetPathTakenCell(finalPos,pos);
					return tsMoving;
				}
			}
			else if(dynamic_cast<UnitPath *>(path) != NULL) {
				UnitPath *advPath = dynamic_cast<UnitPath *>(path);
				//route cache
				Vec2i pos= advPath->peek();
				if(map->canMove(unit, unit->getPos(), pos)) {
					advPath->pop();
					unit->setTargetPos(pos);
					return tsMoving;
				}
			}
			else {
				throw runtime_error("unsupported or missing path finder detected!");
			}
		}
	}
		
	TravelState ts = tsImpossible;
	std::vector<Vec2i> cachedPath = unit->getFaction()->findCachedPath(finalPos, unit);
	if(cachedPath.size() > 0) {
		path->clear();

		for(int i=0; i < cachedPath.size() && i < pathFindRefresh; ++i) {
			path->add(cachedPath[i]);
		}
		ts = tsMoving;
	}
	else {
		//route cache miss
		ts = aStar(unit, finalPos, false);
	}

	//post actions
	switch(ts) {
	case tsBlocked:
	case tsArrived:

		if(ts == tsArrived) {
			unit->getFaction()->addCachedPath(finalPos,unit);
		}
		// The unit is stuck (not only blocked but unable to go anywhere for a while)
		// We will try to bail out of the immediate area
		if( ts == tsBlocked && unit->getInBailOutAttempt() == false &&
			path->isStuck() == true) {
			if(wasStuck != NULL) {
				*wasStuck = true;
			}
			unit->setInBailOutAttempt(true);

			// Try to bail out up to 20 cells away
			for(int bailoutX = -20; bailoutX <= 20 && ts == tsBlocked; ++bailoutX) {
				for(int bailoutY = -20; bailoutY <= 20 && ts == tsBlocked; ++bailoutY) {
					const Vec2i newFinalPos = finalPos + Vec2i(bailoutX,bailoutY);
					if(map->canMove(unit, unit->getPos(), newFinalPos)) {
						ts= aStar(unit, newFinalPos, true);

						/*
						if(ts == tsMoving) {
							unit->setInBailOutAttempt(false);

							if(dynamic_cast<UnitPathBasic *>(path) != NULL) {
								UnitPathBasic *basicPath = dynamic_cast<UnitPathBasic *>(path);
								Vec2i pos= basicPath->pop();
								if(map->canMove(unit, unit->getPos(), pos)) {
									unit->setTargetPos(pos);
								}
								else {
									unit->setCurrSkill(scStop);
									return tsBlocked;
								}
							}
							else if(dynamic_cast<UnitPath *>(path) != NULL) {
								UnitPath *advPath = dynamic_cast<UnitPath *>(path);
								Vec2i pos= advPath->peek();
								if(map->canMove(unit, unit->getPos(), pos)) {
									advPath->pop();
									unit->setTargetPos(pos);
								}
								else {
									unit->setCurrSkill(scStop);
									return tsBlocked;
								}
							}
							else {
								throw runtime_error("unsupported or missing path finder detected!");
							}
						}
						*/

						//else if(ts == tsArrived) {
						//	ts = aStar(unit, finalPos, true);
						//	break;
						//}
					}
				}
			}
			unit->setInBailOutAttempt(false);

			//if(ts == tsArrived) {
			//	ts = tsBlocked;
			//}
		}
		if(ts == tsArrived || ts == tsBlocked) {
			unit->setCurrSkill(scStop);
		}
		break;
	case tsMoving:
		{
			if(dynamic_cast<UnitPathBasic *>(path) != NULL) {
				UnitPathBasic *basicPath = dynamic_cast<UnitPathBasic *>(path);
				Vec2i pos= basicPath->pop();
				if(map->canMove(unit, unit->getPos(), pos)) {
					unit->setTargetPos(pos);
					unit->addCurrentTargetPathTakenCell(finalPos,pos);
				}
				else {
					unit->setCurrSkill(scStop);
					return tsBlocked;
				}
			}
			else if(dynamic_cast<UnitPath *>(path) != NULL) {
				UnitPath *advPath = dynamic_cast<UnitPath *>(path);
				Vec2i pos= advPath->peek();
				if(map->canMove(unit, unit->getPos(), pos)) {
					advPath->pop();
					unit->setTargetPos(pos);
				}
				else {
					unit->setCurrSkill(scStop);
					return tsBlocked;
				}
			}
			else {
				throw runtime_error("unsupported or missing path finder detected!");
			}
		}
		break;
	}
	return ts;
}

// ==================== PRIVATE ==================== 

//route a unit using A* algorithm
TravelState PathFinder::aStar(Unit *unit, const Vec2i &targetPos, bool inBailout) {
	Chrono chrono;
	chrono.start();
	
	if(map == NULL) {
		throw runtime_error("map == NULL");
	}

	nodePoolCount= 0;
	const Vec2i finalPos= computeNearestFreePos(unit, targetPos);

	//if arrived
	/*
	if(finalPos == unit->getPos()) {
		Command *command= unit->getCurrCommand();
		if(command == NULL || command->getPos() != unit->getPos()) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugPathFinder).enabled == true) {
				string commandDesc = "none";
				Command *command= unit->getCurrCommand();
				if(command != NULL && command->getCommandType() != NULL) {
					commandDesc = command->getCommandType()->toString();
				}
				char szBuf[1024]="";
				sprintf(szBuf,"State: arrived#2 at pos: %s, command [%s] inBailout = %d",targetPos.getString().c_str(),commandDesc.c_str(),inBailout);
				unit->setCurrentUnitTitle(szBuf);
			}
			return tsArrived;
		}
	}
	*/

	//path find algorithm

	//a) push starting pos into openNodes
	Node *firstNode= newNode();
	assert(firstNode != NULL);
	if(firstNode == NULL) {
		throw runtime_error("firstNode == NULL");
	}

	firstNode->next= NULL;
	firstNode->prev= NULL;
	const Vec2i unitPos = unit->getPos();
	firstNode->pos= unitPos;
	firstNode->heuristic= heuristic(unitPos, finalPos);
	firstNode->exploredCell= true;
	openNodes.push_back(firstNode);

	//b) loop
	bool pathFound= true;
	bool nodeLimitReached= false;
	Node *node= NULL;

	//if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	while(nodeLimitReached == false) {
		
		//b1) is open nodes is empty => failed to find the path
		if(openNodes.empty() == true) {
			pathFound= false;
			break;
		}

		//b2) get the minimum heuristic node
		Nodes::iterator it = minHeuristic();
		node= *it;

		//b3) if minHeuristic is the finalNode, or the path is no more explored => path was found
		if(node->pos == finalPos || node->exploredCell == false) {
			pathFound= true;
			break;
		}

		//b4) move this node from closedNodes to openNodes
		//add all succesors that are not in closedNodes or openNodes to openNodes
		closedNodes.push_back(node);
		openNodes.erase(it);
		for(int i = -1; i <= 1 && nodeLimitReached == false; ++i) {
			for(int j = -1; j <= 1 && nodeLimitReached == false; ++j) {
				Vec2i sucPos= node->pos + Vec2i(i, j);
				if(openPos(sucPos) == false && map->aproxCanMove(unit, node->pos, sucPos)) {
					//if node is not open and canMove then generate another node
					Node *sucNode= newNode();
					if(sucNode != NULL) {
						sucNode->pos= sucPos;
						sucNode->heuristic= heuristic(sucNode->pos, finalPos); 
						sucNode->prev= node;
						sucNode->next= NULL;
						sucNode->exploredCell= map->getSurfaceCell(Map::toSurfCoords(sucPos))->isExplored(unit->getTeam());
						openNodes.push_back(sucNode);
					}
					else {
						nodeLimitReached= true;
					}
				}
			}
		}
	}//while

	if(chrono.getMillis() > 1) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld, openNodes.empty() = %d, pathFound = %d, nodeLimitReached = %d, unit = %s\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis(),openNodes.empty(),pathFound,nodeLimitReached,unit->getFullName().c_str());

	Node *lastNode= node;

	//if consumed all nodes find best node (to avoid strange behaviour)
	if(nodeLimitReached == true) {
		for(Nodes::iterator it= closedNodes.begin(); it != closedNodes.end(); ++it) {
			if((*it)->heuristic < lastNode->heuristic) {
				lastNode= *it;
			}
		}
	}

	//if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	//check results of path finding
	TravelState ts = tsImpossible;
	UnitPathInterface *path= unit->getPath();
	if(pathFound == false || lastNode == firstNode) {
		//blocked
		if(SystemFlags::getSystemSettingType(SystemFlags::debugPathFinder).enabled == true) {
			string commandDesc = "none";
			Command *command= unit->getCurrCommand();
			if(command != NULL && command->getCommandType() != NULL) {
				commandDesc = command->getCommandType()->toString();
			}

			std::pair<Vec2i,Chrono> lastHarvest = unit->getLastHarvestResourceTarget();

			char szBuf[1024]="";
			sprintf(szBuf,"State: blocked, cmd [%s] pos: [%s], dest pos: [%s], lastHarvest = [%s - %lld], reason A= %d, B= %d, C= %d, D= %d, E= %d, F = %d",commandDesc.c_str(),unit->getPos().getString().c_str(), targetPos.getString().c_str(),lastHarvest.first.getString().c_str(),lastHarvest.second.getMillis(), pathFound,(lastNode == firstNode),path->getBlockCount(), path->isBlocked(), nodeLimitReached,path->isStuck());
			unit->setCurrentUnitTitle(szBuf);
		}

		ts= tsBlocked;
		path->incBlockCount();
	}
	else {
		//on the way
		ts= tsMoving;

		//build next pointers
		Node *currNode= lastNode;
		while(currNode->prev != NULL) {
			currNode->prev->next= currNode;
			currNode= currNode->prev;
		}
		//store path
		path->clear();

		currNode= firstNode;
		for(int i=0; currNode->next != NULL && i < pathFindRefresh; currNode= currNode->next, i++) {
			path->add(currNode->next->pos);
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPathFinder).enabled == true) {
			string commandDesc = "none";
			Command *command= unit->getCurrCommand();
			if(command != NULL && command->getCommandType() != NULL) {
				commandDesc = command->getCommandType()->toString();
			}

			char szBuf[1024]="";
			sprintf(szBuf,"State: moving, cmd [%s] pos: %s dest pos: %s, Queue= %d",commandDesc.c_str(),unit->getPos().getString().c_str(), targetPos.getString().c_str(),path->getQueueCount());
			unit->setCurrentUnitTitle(szBuf);
		}
	}

	if(chrono.getMillis() > 2) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	//clean nodes
	openNodes.clear();
	closedNodes.clear();

	return ts;
}

PathFinder::Node *PathFinder::newNode() {
	if(nodePoolCount < pathFindNodesMax) {
		Node *node= &nodePool[nodePoolCount];
		nodePoolCount++;
		return node;
	}
	return NULL;
}

Vec2i PathFinder::computeNearestFreePos(const Unit *unit, const Vec2i &finalPos) {
	if(map == NULL) {
		throw runtime_error("map == NULL");
	}
	
	//unit data
	int size= unit->getType()->getSize();
	Field field= unit->getCurrField();
	int teamIndex= unit->getTeam();

	//if finalPos is free return it
	if(map->isAproxFreeCells(finalPos, size, field, teamIndex)) {
		return finalPos;
	}

	//find nearest pos
	Vec2i unitPos= unit->getPos();
	Vec2i nearestPos= unitPos;
	float nearestDist= unitPos.dist(finalPos);
	for(int i= -maxFreeSearchRadius; i <= maxFreeSearchRadius; ++i) {
		for(int j= -maxFreeSearchRadius; j <= maxFreeSearchRadius; ++j) {
			Vec2i currPos= finalPos + Vec2i(i, j);
			if(map->isAproxFreeCells(currPos, size, field, teamIndex)) {
				float dist= currPos.dist(finalPos);
				
				//if nearer from finalPos
				if(dist < nearestDist){
					nearestPos= currPos;
					nearestDist= dist;
				}
				//if the distance is the same compare distance to unit
				else if(dist == nearestDist){
					if(currPos.dist(unitPos) < nearestPos.dist(unitPos)) {
						nearestPos= currPos;
					}
				}
			}
		}
	}
	return nearestPos;
}

float PathFinder::heuristic(const Vec2i &pos, const Vec2i &finalPos) {
	return pos.dist(finalPos);
}

//returns an iterator to the lowest heuristic node
PathFinder::Nodes::iterator PathFinder::minHeuristic() {

	assert(openNodes.empty() == false);
	if(openNodes.empty() == true) {
		throw runtime_error("openNodes.empty() == true");
	}

	Nodes::iterator minNodeIt =  openNodes.begin();

	for(Nodes::iterator it= openNodes.begin(); it != openNodes.end(); ++it) {
		if((*it)->heuristic < (*minNodeIt)->heuristic){
			minNodeIt= it;
		}
	}

	return minNodeIt;
}

bool PathFinder::openPos(const Vec2i &sucPos) {

	for(Nodes::reverse_iterator it= closedNodes.rbegin(); it != closedNodes.rend(); ++it) {
		if(sucPos == (*it)->pos) {
			return true;
		}
	}

	//use reverse iterator to find a node faster
	for(Nodes::reverse_iterator it= openNodes.rbegin(); it != openNodes.rend(); ++it) {
		if(sucPos == (*it)->pos) {
			return true;
		}
	}

	return false;
}

}} //end namespace
