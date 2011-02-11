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
#include "randomgen.h"
#include "leak_dumper.h"

using namespace std;
using namespace Shared::Graphics;
using namespace Shared::Util;
using namespace Shared::PlatformCommon;
using Shared::Util::RandomGen;

namespace Glest{ namespace Game{

// =====================================================
// 	class PathFinder
// =====================================================

// ===================== PUBLIC ======================== 

const int PathFinder::maxFreeSearchRadius	= 10;
//const int PathFinder::pathFindNodesMax= 400;
const int PathFinder::pathFindNodesMax		= 1000;
const int PathFinder::pathFindRefresh		= 20;
const int PathFinder::pathFindBailoutRadius	= 20;


PathFinder::PathFinder() {
	nodePool.clear();
	map=NULL;
}

PathFinder::PathFinder(const Map *map) {
	nodePool.clear();

	map=NULL;
	init(map);
}

void PathFinder::init(const Map *map) {
	nodePool.resize(pathFindNodesMax);
	this->map= map;
}

PathFinder::~PathFinder(){
	nodePool.clear();
	map=NULL;
}

TravelState PathFinder::findPath(Unit *unit, const Vec2i &finalPos, bool *wasStuck) {
	if(map == NULL) {
		throw runtime_error("map == NULL");
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true) {
		char szBuf[4096]="";
		sprintf(szBuf,"[findPath] unit->getPos() [%s] finalPos [%s]",
				unit->getPos().getString().c_str(),finalPos.getString().c_str());
		unit->logSynchData(__FILE__,__LINE__,szBuf);
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

		//unit->getFaction()->addCachedPath(finalPos,unit);
		return tsArrived;
	}
	else {
		if(path->isEmpty() == false) {
			if(dynamic_cast<UnitPathBasic *>(path) != NULL) {
				//route cache
				UnitPathBasic *basicPath = dynamic_cast<UnitPathBasic *>(path);
				Vec2i pos= basicPath->pop();

				//if(map->canMove(unit, unit->getPos(), pos, &lookupCacheCanMove)) {
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
				//if(map->canMove(unit, unit->getPos(), pos, &lookupCacheCanMove)) {
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
	//std::vector<Vec2i> cachedPath = unit->getFaction()->findCachedPath(finalPos, unit);
	//if(cachedPath.size() > 0) {
	//	path->clear();
	//	for(int i=0; i < cachedPath.size() && i < pathFindRefresh; ++i) {
	//		path->add(cachedPath[i]);
	//	}
	//	ts = tsMoving;
	//}
	//else {
		//route cache miss
		ts = aStar(unit, finalPos, false);
	//}

	if(ts == tsBlocked) {
		//std::vector<Vec2i> cachedPath = unit->getFaction()->findCachedPath(finalPos, unit);
		//if(cachedPath.size() > 0) {
		//	path->clear();
        //
		//	for(int i=0; i < cachedPath.size() && i < pathFindRefresh; ++i) {
		//		path->add(cachedPath[i]);
		//	}
		//	ts = tsMoving;
		//	unit->addCurrentTargetPathTakenCell(Vec2i(-1),Vec2i(-1));
		//}
	}

	//post actions
	switch(ts) {
	case tsBlocked:
	case tsArrived:

		if(ts == tsArrived) {
			//unit->getFaction()->addCachedPath(finalPos,unit);
		}

		// The unit is stuck (not only blocked but unable to go anywhere for a while)
		// We will try to bail out of the immediate area
		if( ts == tsBlocked && unit->getInBailOutAttempt() == false &&
			path->isStuck() == true) {

			if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true) {
				char szBuf[4096]="";
				sprintf(szBuf,"[attempting to BAIL OUT] finalPos [%s] ts [%d]",
						finalPos.getString().c_str(),ts);
				unit->logSynchData(__FILE__,__LINE__,szBuf);
			}

			if(wasStuck != NULL) {
				*wasStuck = true;
			}
			unit->setInBailOutAttempt(true);

			//printf("#1 BAILOUT test unitid [%d]\n",unit->getId());

			bool useBailoutRadius = true;
			/*
			Command *command= unit->getCurrCommand();
			if(command != NULL) {
				const HarvestCommandType *hct= dynamic_cast<const HarvestCommandType*>(command->getCommandType());
				if(hct != NULL) {
					Resource *r = map->getSurfaceCell(Map::toSurfCoords(command->getPos()))->getResource();
					if(r != NULL && r->getType() != NULL) {
						Vec2i newFinalPos = unit->getFaction()->getClosestResourceTypeTargetFromCache(unit, r->getType());

						//printf("#2 BAILOUT test unitid [%d] newFinalPos [%s] finalPos [%s]\n",unit->getId(),newFinalPos.getString().c_str(),finalPos.getString().c_str());

						if(newFinalPos != finalPos) {
							bool canUnitMove = map->canMove(unit, unit->getPos(), newFinalPos);

							//printf("#3 BAILOUT test unitid [%d] newFinalPos [%s] finalPos [%s] canUnitMove [%d]\n",unit->getId(),newFinalPos.getString().c_str(),finalPos.getString().c_str(),canUnitMove);

							if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true) {
								char szBuf[4096]="";
								sprintf(szBuf,"[attempting to BAIL OUT] finalPos [%s] newFinalPos [%s] ts [%d] canUnitMove [%d]",
										finalPos.getString().c_str(),newFinalPos.getString().c_str(),ts,canUnitMove);
								unit->logSynchData(__FILE__,__LINE__,szBuf);
							}

							if(canUnitMove) {
								useBailoutRadius = false;
								ts= aStar(unit, newFinalPos, true);
							}
						}
					}
				}
			}
			*/

			if(useBailoutRadius == true) {
				//int tryRadius = random.randRange(-PathFinder::pathFindBailoutRadius, PathFinder::pathFindBailoutRadius);
				int tryRadius = random.randRange(0,1);

				//printf("#4 BAILOUT test unitid [%d] useBailoutRadius [%d] tryRadius [%d]\n",unit->getId(),useBailoutRadius,tryRadius);

				// Try to bail out up to PathFinder::pathFindBailoutRadius cells away
				if(tryRadius > 0) {
					for(int bailoutX = -PathFinder::pathFindBailoutRadius; bailoutX <= PathFinder::pathFindBailoutRadius && ts == tsBlocked; ++bailoutX) {
						for(int bailoutY = -PathFinder::pathFindBailoutRadius; bailoutY <= PathFinder::pathFindBailoutRadius && ts == tsBlocked; ++bailoutY) {
							const Vec2i newFinalPos = finalPos + Vec2i(bailoutX,bailoutY);
							bool canUnitMove = map->canMove(unit, unit->getPos(), newFinalPos);

							if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true) {
								char szBuf[4096]="";
								sprintf(szBuf,"[attempting to BAIL OUT] finalPos [%s] newFinalPos [%s] ts [%d] canUnitMove [%d]",
										finalPos.getString().c_str(),newFinalPos.getString().c_str(),ts,canUnitMove);
								unit->logSynchData(__FILE__,__LINE__,szBuf);
							}

							if(canUnitMove) {
								ts= aStar(unit, newFinalPos, true);
							}
						}
					}
				}
				else {
					for(int bailoutX = PathFinder::pathFindBailoutRadius; bailoutX >= -PathFinder::pathFindBailoutRadius && ts == tsBlocked; --bailoutX) {
						for(int bailoutY = PathFinder::pathFindBailoutRadius; bailoutY >= -PathFinder::pathFindBailoutRadius && ts == tsBlocked; --bailoutY) {
							const Vec2i newFinalPos = finalPos + Vec2i(bailoutX,bailoutY);
							bool canUnitMove = map->canMove(unit, unit->getPos(), newFinalPos);

							if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true) {
								char szBuf[4096]="";
								sprintf(szBuf,"[attempting to BAIL OUT] finalPos [%s] newFinalPos [%s] ts [%d] canUnitMove [%d]",
										finalPos.getString().c_str(),newFinalPos.getString().c_str(),ts,canUnitMove);
								unit->logSynchData(__FILE__,__LINE__,szBuf);
							}

							if(canUnitMove) {
								ts= aStar(unit, newFinalPos, true);
							}
						}
					}
				}
			}
			unit->setInBailOutAttempt(false);
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

				//if(map->canMove(unit, unit->getPos(), pos, &lookupCacheCanMove)) {
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
				//if(map->canMove(unit, unit->getPos(), pos, &lookupCacheCanMove)) {
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


void PathFinder::processNode(Unit *unit, Node *node,const Vec2i finalPos, int i, int j, bool &nodeLimitReached) {
	Vec2i sucPos= node->pos + Vec2i(i, j);

	bool canUnitMoveToCell = map->aproxCanMove(unit, node->pos, sucPos);
	if(openPos(sucPos) == false && canUnitMoveToCell == true) {
		//if node is not open and canMove then generate another node
		Node *sucNode= newNode();
		if(sucNode != NULL) {
			sucNode->pos= sucPos;
			sucNode->heuristic= heuristic(sucNode->pos, finalPos);
			sucNode->prev= node;
			sucNode->next= NULL;
			sucNode->exploredCell= map->getSurfaceCell(Map::toSurfCoords(sucPos))->isExplored(unit->getTeam());
			openNodesList[sucNode->heuristic].push_back(sucNode);
			openPosList[sucNode->pos] = true;
		}
		else {
			nodeLimitReached= true;
		}
	}
}

//route a unit using A* algorithm
TravelState PathFinder::aStar(Unit *unit, const Vec2i &targetPos, bool inBailout) {
	Chrono chrono;
	chrono.start();

	if(map == NULL) {
		throw runtime_error("map == NULL");
	}

	nodePoolCount= 0;
	openNodesList.clear();
	openPosList.clear();
	closedNodesList.clear();

	TravelState ts = tsImpossible;
	const Vec2i unitPos = unit->getPos();
	const Vec2i finalPos= computeNearestFreePos(unit, targetPos);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled == true && chrono.getMillis() > 4) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	UnitPathInterface *path= unit->getPath();
	// Check the previous path find cache for the unit to see if its good to
	// use
	const bool tryLastPathCache = true;
	if(tryLastPathCache == true && path != NULL) {
		UnitPathBasic *basicPathFinder = dynamic_cast<UnitPathBasic *>(path);
		if(basicPathFinder != NULL && basicPathFinder->getLastPathCacheQueueCount() > 0) {
			vector<Vec2i> cachedPath= basicPathFinder->getLastPathCacheQueue();
			for(int i = 0; i < cachedPath.size(); ++i) {
				Vec2i &pos1 = cachedPath[i];
				// Looking to find if the unit is in one of the cells in the cached path
				if(unitPos == pos1) {
					// Now see if we can re-use this path to get to the final destination
					for(int j = i+1; j < cachedPath.size(); ++j) {
						Vec2i &pos2 = cachedPath[j];
						bool canUnitMoveToCell = map->aproxCanMove(unit, pos1, pos2);
						if(canUnitMoveToCell == true) {
							if(pos2 == finalPos) {
								//on the way
								ts= tsMoving;

								if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled == true && chrono.getMillis() > 4) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

								//store path
								basicPathFinder->clear();

								int pathCount=0;
								for(int k=i+1; k <= j; k++) {
									if(pathCount < pathFindRefresh) {
										basicPathFinder->add(cachedPath[k]);
									}
									basicPathFinder->addToLastPathCache(cachedPath[k]);
									pathCount++;
								}

								if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled == true && chrono.getMillis() > 4) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

								if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true) {
									char szBuf[4096]="";
									sprintf(szBuf,"[Setting new path for unit] openNodesList.size() [%ld] openPosList.size() [%ld] finalPos [%s] targetPos [%s] inBailout [%d] ts [%d]",
											openNodesList.size(),openPosList.size(),finalPos.getString().c_str(),targetPos.getString().c_str(),inBailout,ts);
									unit->logSynchData(__FILE__,__LINE__,szBuf);
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

								if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled == true && chrono.getMillis() > 4) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

								return ts;
								//break;
							}
							else if(j - i > pathFindRefresh) {
								//on the way
								ts= tsMoving;

								if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled == true && chrono.getMillis() > 4) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

								//store path
								basicPathFinder->clear();

								int pathCount=0;
								for(int k=i+1; k <= cachedPath.size(); k++) {
									if(pathCount < pathFindRefresh) {
										basicPathFinder->add(cachedPath[k]);
									}
									basicPathFinder->addToLastPathCache(cachedPath[k]);
									pathCount++;
								}

								if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled == true && chrono.getMillis() > 4) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

								if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true) {
									char szBuf[4096]="";
									sprintf(szBuf,"[Setting new path for unit] openNodesList.size() [%ld] openPosList.size() [%ld] finalPos [%s] targetPos [%s] inBailout [%d] ts [%d]",
											openNodesList.size(),openPosList.size(),finalPos.getString().c_str(),targetPos.getString().c_str(),inBailout,ts);
									unit->logSynchData(__FILE__,__LINE__,szBuf);
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

								if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled == true && chrono.getMillis() > 4) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

								return ts;
							}
						}

						pos1 = pos2;
					}

					break;
				}
			}
		}
	}


	//path find algorithm

	//a) push starting pos into openNodes
	Node *firstNode= newNode();
	assert(firstNode != NULL);
	if(firstNode == NULL) {
		throw runtime_error("firstNode == NULL");
	}

	firstNode->next= NULL;
	firstNode->prev= NULL;
	firstNode->pos= unitPos;
	firstNode->heuristic= heuristic(unitPos, finalPos);
	firstNode->exploredCell= true;
	openNodesList[firstNode->heuristic].push_back(firstNode);
	openPosList[firstNode->pos] = true;

	//b) loop
	bool pathFound			= true;
	bool nodeLimitReached	= false;
	Node *node				= NULL;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled == true && chrono.getMillis() > 4) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	int whileLoopCount = 0;
	while(nodeLimitReached == false) {
		whileLoopCount++;
		
		//b1) is open nodes is empty => failed to find the path
		if(openNodesList.empty() == true) {
			pathFound= false;
			break;
		}

		//b2) get the minimum heuristic node
		//Nodes::iterator it = minHeuristic();
		node = minHeuristicFastLookup();

		//b3) if minHeuristic is the finalNode, or the path is no more explored => path was found
		if(node->pos == finalPos || node->exploredCell == false) {
			pathFound= true;
			break;
		}

		//b4) move this node from closedNodes to openNodes
		//add all succesors that are not in closedNodes or openNodes to openNodes
		closedNodesList[node->heuristic].push_back(node);
		openPosList[node->pos] = true;

		int tryDirection = random.randRange(0,3);
		if(tryDirection == 3) {
			for(int i = 1; i >= -1 && nodeLimitReached == false; --i) {
				for(int j = -1; j <= 1 && nodeLimitReached == false; ++j) {
					processNode(unit, node, finalPos, i, j, nodeLimitReached);
				}
			}
		}
		else if(tryDirection == 2) {
			for(int i = -1; i <= 1 && nodeLimitReached == false; ++i) {
				for(int j = 1; j >= -1 && nodeLimitReached == false; --j) {
					processNode(unit, node, finalPos, i, j, nodeLimitReached);
				}
			}
		}
		else if(tryDirection == 1) {
			for(int i = -1; i <= 1 && nodeLimitReached == false; ++i) {
				for(int j = -1; j <= 1 && nodeLimitReached == false; ++j) {
					processNode(unit, node, finalPos, i, j, nodeLimitReached);
				}
			}
		}
		else {
			for(int i = 1; i >= -1 && nodeLimitReached == false; --i) {
				for(int j = 1; j >= -1 && nodeLimitReached == false; --j) {
					processNode(unit, node, finalPos, i, j, nodeLimitReached);
				}
			}
		}
	} //while

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled == true && chrono.getMillis() > 4) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	Node *lastNode= node;

	//if consumed all nodes find best node (to avoid strange behaviour)
	if(nodeLimitReached == true) {
		if(closedNodesList.size() > 0) {
			float bestHeuristic = closedNodesList.begin()->first;
			if(bestHeuristic < lastNode->heuristic) {
				lastNode= closedNodesList.begin()->second[0];
			}
		}
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled == true && chrono.getMillis() > 4) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	//check results of path finding
	ts = tsImpossible;
	//UnitPathInterface *path= unit->getPath();
	if(pathFound == false || lastNode == firstNode) {
		//blocked
		if(SystemFlags::getSystemSettingType(SystemFlags::debugPathFinder).enabled == true) {
			string commandDesc = "none";
			Command *command= unit->getCurrCommand();
			if(command != NULL && command->getCommandType() != NULL) {
				commandDesc = command->getCommandType()->toString();
			}

			std::pair<Vec2i,int> lastHarvest = unit->getLastHarvestResourceTarget();

			char szBuf[1024]="";
			sprintf(szBuf,"State: blocked, cmd [%s] pos: [%s], dest pos: [%s], lastHarvest = [%s - %d], reason A= %d, B= %d, C= %d, D= %d, E= %d, F = %d",commandDesc.c_str(),unit->getPos().getString().c_str(), targetPos.getString().c_str(),lastHarvest.first.getString().c_str(),lastHarvest.second, pathFound,(lastNode == firstNode),path->getBlockCount(), path->isBlocked(), nodeLimitReached,path->isStuck());
			unit->setCurrentUnitTitle(szBuf);
		}

		ts= tsBlocked;
		path->incBlockCount();

		if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true) {
			char szBuf[4096]="";
			sprintf(szBuf,"[path for unit BLOCKED] openNodesList.size() [%ld] openPosList.size() [%ld] finalPos [%s] targetPos [%s] inBailout [%d] ts [%d]",
					openNodesList.size(),openPosList.size(),finalPos.getString().c_str(),targetPos.getString().c_str(),inBailout,ts);
			unit->logSynchData(__FILE__,__LINE__,szBuf);
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled == true && chrono.getMillis() > 4) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
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

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled == true && chrono.getMillis() > 4) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

		//store path
		path->clear();
		UnitPathBasic *basicPathFinder = dynamic_cast<UnitPathBasic *>(path);

		currNode= firstNode;
		for(int i=0; currNode->next != NULL; currNode= currNode->next, i++) {
			if(i < pathFindRefresh) {
				path->add(currNode->next->pos);
			}
			if(basicPathFinder) {
				basicPathFinder->addToLastPathCache(currNode->next->pos);
			}
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled == true && chrono.getMillis() > 4) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

		if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true) {
			char szBuf[4096]="";
			sprintf(szBuf,"[Setting new path for unit] openNodesList.size() [%ld] openPosList.size() [%ld] finalPos [%s] targetPos [%s] inBailout [%d] ts [%d]",
					openNodesList.size(),openPosList.size(),finalPos.getString().c_str(),targetPos.getString().c_str(),inBailout,ts);
			unit->logSynchData(__FILE__,__LINE__,szBuf);
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

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled == true && chrono.getMillis() > 4) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
	}

	openNodesList.clear();
	openPosList.clear();
	closedNodesList.clear();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled == true && chrono.getMillis() > 4) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld --------------------------- [END OF METHOD] ---------------------------\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

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

void PathFinder::processNearestFreePos(const Vec2i &finalPos, int i, int j, int size, Field field, int teamIndex,Vec2i unitPos, Vec2i &nearestPos, float &nearestDist) {
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
			processNearestFreePos(finalPos, i, j, size, field, teamIndex, unitPos, nearestPos, nearestDist);
		}
	}
	return nearestPos;
}

float PathFinder::heuristic(const Vec2i &pos, const Vec2i &finalPos) {
	return pos.dist(finalPos);
}

PathFinder::Node * PathFinder::minHeuristicFastLookup() {
	assert(openNodesList.empty() == false);
	if(openNodesList.empty() == true) {
		throw runtime_error("openNodesList.empty() == true");
	}

	Node *result = openNodesList.begin()->second[0];
	openNodesList.begin()->second.erase(openNodesList.begin()->second.begin());
	if(openNodesList.begin()->second.size() == 0) {
		openNodesList.erase(openNodesList.begin());
	}
	return result;
}

bool PathFinder::openPos(const Vec2i &sucPos) {
	if(openPosList.find(sucPos) == openPosList.end()) {
		return false;
	}
	return true;
}

}} //end namespace
