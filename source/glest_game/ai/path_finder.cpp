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

#include "path_finder.h"

#include <algorithm>
#include <cassert>

#include "config.h"
#include "map.h"
#include "unit.h"
#include "unit_type.h"
#include "leak_dumper.h"

using namespace std;
using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class PathFinder
// =====================================================

// ===================== PUBLIC ======================== 

const int PathFinder::maxFreeSearchRadius= 10;	
const int PathFinder::pathFindNodesMax= 400;
const int PathFinder::pathFindRefresh= 10;


PathFinder::PathFinder(){
	nodePool= NULL;
}

PathFinder::PathFinder(const Map *map){
	init(map);
	nodePool= NULL;
}

void PathFinder::init(const Map *map){
	nodePool= new Node[pathFindNodesMax];
	this->map= map;
}

PathFinder::~PathFinder(){
	delete [] nodePool;
}

PathFinder::TravelState PathFinder::findPath(Unit *unit, const Vec2i &finalPos){
	
	//route cache
	UnitPath *path= unit->getPath();
	if(finalPos==unit->getPos()){
		//if arrived
		unit->setCurrSkill(scStop);
		return tsArrived;
	}
	else if(!path->isEmpty()){
		//route cache
		Vec2i pos= path->pop();
		if(map->canMove(unit, unit->getPos(), pos)){
			unit->setTargetPos(pos);
			return tsOnTheWay;
		}
	}
		
	//route cache miss
	TravelState ts= aStar(unit, finalPos);

	//post actions
	switch(ts){
	case tsBlocked:
	case tsArrived:
		unit->setCurrSkill(scStop);
		break;
	case tsOnTheWay:
		Vec2i pos= path->pop();
		if(map->canMove(unit, unit->getPos(), pos)){
			unit->setTargetPos(pos);
		}
		else{
			unit->setCurrSkill(scStop);
			return tsBlocked;
		}
		break;
	}
	return ts;
}

// ==================== PRIVATE ==================== 

//route a unit using A* algorithm
PathFinder::TravelState PathFinder::aStar(Unit *unit, const Vec2i &targetPos){
	
	nodePoolCount= 0;
	const Vec2i finalPos= computeNearestFreePos(unit, targetPos);

	//if arrived
	if(finalPos==unit->getPos()){
		return tsArrived;
	}

	//path find algorithm

	//a) push starting pos into openNodes
	Node *firstNode= newNode();
	assert(firstNode!=NULL);;
	firstNode->next= NULL;
	firstNode->prev= NULL;
	firstNode->pos= unit->getPos();
	firstNode->heuristic= heuristic(unit->getPos(), finalPos);
	firstNode->exploredCell= true;
	openNodes.push_back(firstNode);

	//b) loop
	bool pathFound= true;
	bool nodeLimitReached= false;
	Node *node= NULL;

	while(!nodeLimitReached){
		
		//b1) is open nodes is empty => failed to find the path
		if(openNodes.empty()){
			pathFound= false;
			break;
		}

		//b2) get the minimum heuristic node
		Nodes::iterator it = minHeuristic();
		node= *it;

		//b3) if minHeuristic is the finalNode, or the path is no more explored => path was found
		if(node->pos==finalPos || !node->exploredCell){
			pathFound= true;
			break;
		}

		//b4) move this node from closedNodes to openNodes
		//add all succesors that are not in closedNodes or openNodes to openNodes
		closedNodes.push_back(node);
		openNodes.erase(it);
		for(int i=-1; i<=1 && !nodeLimitReached; ++i){
			for(int j=-1; j<=1 && !nodeLimitReached; ++j){
				Vec2i sucPos= node->pos + Vec2i(i, j);
				if(!openPos(sucPos) && map->aproxCanMove(unit, node->pos, sucPos)){
					//if node is not open and canMove then generate another node
					Node *sucNode= newNode();
					if(sucNode!=NULL){
						sucNode->pos= sucPos;
						sucNode->heuristic= heuristic(sucNode->pos, finalPos); 
						sucNode->prev= node;
						sucNode->next= NULL;
						sucNode->exploredCell= map->getSurfaceCell(Map::toSurfCoords(sucPos))->isExplored(unit->getTeam());
						openNodes.push_back(sucNode);
					}
					else{
						nodeLimitReached= true;
					}
				}
			}
		}
	}//while

	Node *lastNode= node;

	//if consumed all nodes find best node (to avoid strage behaviour)
	if(nodeLimitReached){
		for(Nodes::iterator it= closedNodes.begin(); it!=closedNodes.end(); ++it){
			if((*it)->heuristic < lastNode->heuristic){
				lastNode= *it;
			}
		}
	}

	//check results of path finding
	TravelState ts;
	UnitPath *path= unit->getPath();
	if(pathFound==false || lastNode==firstNode){
		//blocked
		ts= tsBlocked;
		path->incBlockCount();
	}
	else {
		//on the way
		ts= tsOnTheWay;

		//build next pointers
		Node *currNode= lastNode;
		while(currNode->prev!=NULL){
			currNode->prev->next= currNode;
			currNode= currNode->prev;
		}
		//store path
		path->clear();

		currNode= firstNode;
		for(int i=0; currNode->next!=NULL && i<pathFindRefresh; currNode= currNode->next, i++){
			path->push(currNode->next->pos);
		}
	}

	//clean nodes
	openNodes.clear();
	closedNodes.clear();

	return ts;
}

PathFinder::Node *PathFinder::newNode(){
	if(nodePoolCount<pathFindNodesMax){
		Node *node= &nodePool[nodePoolCount];
		nodePoolCount++;
		return node;
	}
	return NULL;
}

Vec2i PathFinder::computeNearestFreePos(const Unit *unit, const Vec2i &finalPos){
	
	//unit data
	Vec2i unitPos= unit->getPos();
	int size= unit->getType()->getSize();
	Field field= unit->getCurrField();
	int teamIndex= unit->getTeam();

	//if finalPos is free return it
	if(map->isAproxFreeCells(finalPos, size, field, teamIndex)){
		return finalPos;
	}

	//find nearest pos
	Vec2i nearestPos= unitPos;
	float nearestDist= unitPos.dist(finalPos);
	for(int i= -maxFreeSearchRadius; i<=maxFreeSearchRadius; ++i){
		for(int j= -maxFreeSearchRadius; j<=maxFreeSearchRadius; ++j){
			Vec2i currPos= finalPos + Vec2i(i, j);
			if(map->isAproxFreeCells(currPos, size, field, teamIndex)){
				float dist= currPos.dist(finalPos);
				
				//if nearer from finalPos
				if(dist<nearestDist){
					nearestPos= currPos;
					nearestDist= dist;
				}
				//if the distance is the same compare distance to unit
				else if(dist==nearestDist){
					if(currPos.dist(unitPos)<nearestPos.dist(unitPos)){
						nearestPos= currPos;
					}
				}
			}
		}
	}
	return nearestPos;
}

float PathFinder::heuristic(const Vec2i &pos, const Vec2i &finalPos){
	return pos.dist(finalPos);
}

//returns an iterator to the lowest heuristic node
PathFinder::Nodes::iterator PathFinder::minHeuristic(){

	Nodes::iterator minNodeIt=  openNodes.begin();

	assert(!openNodes.empty());

	for(Nodes::iterator it= openNodes.begin(); it!=openNodes.end(); ++it){
		if((*it)->heuristic < (*minNodeIt)->heuristic){
			minNodeIt= it;
		}
	}

	return minNodeIt;
}

bool PathFinder::openPos(const Vec2i &sucPos){

	for(Nodes::reverse_iterator it= closedNodes.rbegin(); it!=closedNodes.rend(); ++it){
		if(sucPos==(*it)->pos){
			return true;
		}
	}

	//use reverse iterator to find a node faster
	for(Nodes::reverse_iterator it= openNodes.rbegin(); it!=openNodes.rend(); ++it){
		if(sucPos==(*it)->pos){
			return true;
		}
	}

	return false;
}

}} //end namespace
