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

int PathFinder::pathFindNodesAbsoluteMax	= 900;
int PathFinder::pathFindNodesMax			= 2000;
const int PathFinder::pathFindRefresh		= 10;
const int PathFinder::pathFindBailoutRadius	= 20;
const int PathFinder::pathFindExtendRefreshForNodeCount	= 25;
const int PathFinder::pathFindExtendRefreshNodeCountMin	= 40;
const int PathFinder::pathFindExtendRefreshNodeCountMax	= 40;

PathFinder::PathFinder() {
	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		factions.push_back(FactionState());
	}
	map=NULL;
}

int PathFinder::getPathFindExtendRefreshNodeCount(int factionIndex) {
	int refreshNodeCount = factions[factionIndex].random.randRange(PathFinder::pathFindExtendRefreshNodeCountMin,PathFinder::pathFindExtendRefreshNodeCountMax);
	return refreshNodeCount;
}

PathFinder::PathFinder(const Map *map) {
	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		factions.push_back(FactionState());
	}

	map=NULL;
	init(map);
}

void PathFinder::init(const Map *map) {
	PathFinder::pathFindNodesMax = Config::getInstance().getInt("MaxPathfinderNodeCount",intToStr(PathFinder::pathFindNodesMax).c_str());

	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		factions[i].nodePool.resize(pathFindNodesAbsoluteMax);
		factions[i].useMaxNodeCount = PathFinder::pathFindNodesMax;
	}
	this->map= map;
}

PathFinder::~PathFinder() {
	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		factions[i].nodePool.clear();
	}
	factions.clear();
	map=NULL;
}

void PathFinder::clearUnitPrecache(Unit *unit) {
	factions[unit->getFactionIndex()].precachedTravelState[unit->getId()] = tsImpossible;
	factions[unit->getFactionIndex()].precachedPath[unit->getId()].clear();
}

void PathFinder::removeUnitPrecache(Unit *unit) {
	if(factions.size() > unit->getFactionIndex()) {
		if(factions[unit->getFactionIndex()].precachedTravelState.find(unit->getId()) != factions[unit->getFactionIndex()].precachedTravelState.end()) {
			factions[unit->getFactionIndex()].precachedTravelState.erase(unit->getId());
		}
		if(factions[unit->getFactionIndex()].precachedPath.find(unit->getId()) != factions[unit->getFactionIndex()].precachedPath.end()) {
			factions[unit->getFactionIndex()].precachedPath.erase(unit->getId());
		}
	}
}

TravelState PathFinder::findPath(Unit *unit, const Vec2i &finalPos, bool *wasStuck, int frameIndex) {
	if(map == NULL) {
		throw runtime_error("map == NULL");
	}
	if(frameIndex >= 0) {
		clearUnitPrecache(unit);
	}

//	if(frameIndex != factions[unit->getFactionIndex()].lastFromToNodeListFrame) {
//		if(factions[unit->getFactionIndex()].mapFromToNodeList.size() > 0) {
//			printf("clear duplicate list = %lu for faction = %d frameIndex = %d\n",factions[unit->getFactionIndex()].mapFromToNodeList.size(),unit->getFactionIndex(),frameIndex);
//			factions[unit->getFactionIndex()].mapFromToNodeList.clear();
//		}
//		factions[unit->getFactionIndex()].lastFromToNodeListFrame = frameIndex;
//	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
		char szBuf[4096]="";
		sprintf(szBuf,"[findPath] unit->getPos() [%s] finalPos [%s]",
				unit->getPos().getString().c_str(),finalPos.getString().c_str());
		unit->logSynchData(__FILE__,__LINE__,szBuf);
	}

	//route cache
	if(finalPos == unit->getPos()) {
		if(frameIndex < 0) {
			//if arrived
			unit->setCurrSkill(scStop);
		}
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

		return tsArrived;
	}

	UnitPathInterface *path= unit->getPath();
	if(path->isEmpty() == false) {
		if(dynamic_cast<UnitPathBasic *>(path) != NULL) {
			//route cache
			UnitPathBasic *basicPath = dynamic_cast<UnitPathBasic *>(path);
			Vec2i pos= basicPath->pop(frameIndex < 0);

			if(map->canMove(unit, unit->getPos(), pos)) {
				if(frameIndex < 0) {
					unit->setTargetPos(pos);
				}
				return tsMoving;
			}
		}
		else if(dynamic_cast<UnitPath *>(path) != NULL) {
			UnitPath *advPath = dynamic_cast<UnitPath *>(path);
			//route cache
			Vec2i pos= advPath->peek();
			if(map->canMove(unit, unit->getPos(), pos)) {
				if(frameIndex < 0) {
					advPath->pop();
					unit->setTargetPos(pos);
				}
				return tsMoving;
			}
		}
		else {
			throw runtime_error("unsupported or missing path finder detected!");
		}
	}

	if(path->isStuck() == true && unit->getLastStuckPos() == finalPos &&
		unit->isLastStuckFrameWithinCurrentFrameTolerance() == true) {

		//printf("$$$$ Unit STILL BLOCKED for [%d - %s]\n",unit->getId(),unit->getFullName().c_str());
		return tsBlocked;
	}

	TravelState ts = tsImpossible;
	//route cache miss

	int maxNodeCount=-1;
	if(unit->getUsePathfinderExtendedMaxNodes() == true) {
		const bool showConsoleDebugInfo = Config::getInstance().getBool("EnablePathfinderDistanceOutput","false");
		if(showConsoleDebugInfo || SystemFlags::VERBOSE_MODE_ENABLED) {
			printf("\n\n\n\n### Continued call to AStar with LARGE maxnodes for unit [%d - %s]\n\n",unit->getId(),unit->getFullName().c_str());
		}

		maxNodeCount= PathFinder::pathFindNodesAbsoluteMax;
	}

	ts = aStar(unit, finalPos, false, frameIndex, maxNodeCount);

	//post actions
	switch(ts) {
	case tsBlocked:
	case tsArrived:

		// The unit is stuck (not only blocked but unable to go anywhere for a while)
		// We will try to bail out of the immediate area
		if( ts == tsBlocked && unit->getInBailOutAttempt() == false &&
			path->isStuck() == true) {

			//printf("$$$$ Unit START BAILOUT ATTEMPT for [%d - %s]\n",unit->getId(),unit->getFullName().c_str());

			if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
				char szBuf[4096]="";
				sprintf(szBuf,"[attempting to BAIL OUT] finalPos [%s] ts [%d]",
						finalPos.getString().c_str(),ts);
				unit->logSynchData(__FILE__,__LINE__,szBuf);
			}

			if(wasStuck != NULL) {
				*wasStuck = true;
			}
			unit->setInBailOutAttempt(true);

			bool useBailoutRadius = Config::getInstance().getBool("EnableBailoutPathfinding","true");
			if(useBailoutRadius == true) {

				//!!!
				bool unitImmediatelyBlocked = false;

				// First check if unit currently blocked all around them, if so don't try to pathfind
				const bool showConsoleDebugInfo = Config::getInstance().getBool("EnablePathfinderDistanceOutput","false");
				const Vec2i unitPos = unit->getPos();
				int failureCount = 0;
				int cellCount = 0;

				for(int i = -1; i <= 1; ++i) {
					for(int j = -1; j <= 1; ++j) {
						Vec2i pos = unitPos + Vec2i(i, j);
						if(pos != unitPos) {
							bool canUnitMoveToCell = map->aproxCanMove(unit, unitPos, pos);
							if(canUnitMoveToCell == false) {
								failureCount++;
							}
							cellCount++;
						}
					}
				}
				unitImmediatelyBlocked = (failureCount == cellCount);

				//if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled == true && chrono.getMillis() > 1) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] **Check if dest blocked, distance for unit [%d - %s] from [%s] to [%s] is %.2f took msecs: %lld unitImmediatelyBlocked = %d, failureCount = %d\n",__FILE__,__FUNCTION__,__LINE__,unit->getId(),unit->getFullName().c_str(), unitPos.getString().c_str(), finalPos.getString().c_str(), dist,(long long int)chrono.getMillis(),unitImmediatelyBlocked,failureCount);
				if(showConsoleDebugInfo && unitImmediatelyBlocked) {
					printf("**Check if src blocked [%d], unit [%d - %s] from [%s] to [%s] unitImmediatelyBlocked = %d, failureCount = %d [%d]\n",
							unitImmediatelyBlocked, unit->getId(),unit->getFullName().c_str(), unitPos.getString().c_str(), finalPos.getString().c_str(), unitImmediatelyBlocked,failureCount,cellCount);
				}

//				if(unitImmediatelyBlocked == false) {
//					// First check if final destination blocked
//					failureCount = 0;
//					cellCount = 0;
//
//					for(int i = -1; i <= 1; ++i) {
//						for(int j = -1; j <= 1; ++j) {
//							Vec2i pos = finalPos + Vec2i(i, j);
//							if(pos != finalPos) {
//								bool canUnitMoveToCell = map->aproxCanMove(unit, pos, finalPos);
//								if(canUnitMoveToCell == false) {
//									failureCount++;
//								}
//								cellCount++;
//							}
//						}
//					}
//					unitImmediatelyBlocked = (failureCount == cellCount);
//
//					if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled == true && chrono.getMillis() > 1) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] **Check if dest blocked, distance for unit [%d - %s] from [%s] to [%s] is %.2f took msecs: %lld unitImmediatelyBlocked = %d, failureCount = %d\n",__FILE__,__FUNCTION__,__LINE__,unit->getId(),unit->getFullName().c_str(), unitPos.getString().c_str(), finalPos.getString().c_str(), dist,(long long int)chrono.getMillis(),unitImmediatelyBlocked,failureCount);
//					if(showConsoleDebugInfo && nodeLimitReached) {
//						printf("**Check if dest blocked [%d - %d], unit [%d - %s] from [%s] to [%s] distance %.2f took msecs: %lld unitImmediatelyBlocked = %d, failureCount = %d [%d]\n",
//								nodeLimitReached, inBailout, unit->getId(),unit->getFullName().c_str(), unitPos.getString().c_str(), finalPos.getString().c_str(), dist,(long long int)chrono.getMillis(),unitImmediatelyBlocked,failureCount,cellCount);
//					}
//				}
				//
				//!!!

				if(unitImmediatelyBlocked == false) {
					int tryRadius = factions[unit->getFactionIndex()].random.randRange(0,1);

					// Try to bail out up to PathFinder::pathFindBailoutRadius cells away
					if(tryRadius > 0) {
						for(int bailoutX = -PathFinder::pathFindBailoutRadius; bailoutX <= PathFinder::pathFindBailoutRadius && ts == tsBlocked; ++bailoutX) {
							for(int bailoutY = -PathFinder::pathFindBailoutRadius; bailoutY <= PathFinder::pathFindBailoutRadius && ts == tsBlocked; ++bailoutY) {
								const Vec2i newFinalPos = finalPos + Vec2i(bailoutX,bailoutY);
								bool canUnitMove = map->canMove(unit, unit->getPos(), newFinalPos);

								if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
									char szBuf[4096]="";
									sprintf(szBuf,"[attempting to BAIL OUT] finalPos [%s] newFinalPos [%s] ts [%d] canUnitMove [%d]",
											finalPos.getString().c_str(),newFinalPos.getString().c_str(),ts,canUnitMove);
									unit->logSynchData(__FILE__,__LINE__,szBuf);
								}

								if(canUnitMove) {
									//printf("$$$$ Unit BAILOUT(1) ASTAR ATTEMPT for [%d - %s] newFinalPos = [%s]\n",unit->getId(),unit->getFullName().c_str(),newFinalPos.getString().c_str());

									int maxBailoutNodeCount = (PathFinder::pathFindBailoutRadius * 2);
									ts= aStar(unit, newFinalPos, true, frameIndex, maxBailoutNodeCount);
								}
							}
						}
					}
					else {
						for(int bailoutX = PathFinder::pathFindBailoutRadius; bailoutX >= -PathFinder::pathFindBailoutRadius && ts == tsBlocked; --bailoutX) {
							for(int bailoutY = PathFinder::pathFindBailoutRadius; bailoutY >= -PathFinder::pathFindBailoutRadius && ts == tsBlocked; --bailoutY) {
								const Vec2i newFinalPos = finalPos + Vec2i(bailoutX,bailoutY);
								bool canUnitMove = map->canMove(unit, unit->getPos(), newFinalPos);

								if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
									char szBuf[4096]="";
									sprintf(szBuf,"[attempting to BAIL OUT] finalPos [%s] newFinalPos [%s] ts [%d] canUnitMove [%d]",
											finalPos.getString().c_str(),newFinalPos.getString().c_str(),ts,canUnitMove);
									unit->logSynchData(__FILE__,__LINE__,szBuf);
								}

								if(canUnitMove) {
									//printf("$$$$ Unit BAILOUT(1) ASTAR ATTEMPT for [%d - %s] newFinalPos = [%s]\n",unit->getId(),unit->getFullName().c_str(),newFinalPos.getString().c_str());
									int maxBailoutNodeCount = (PathFinder::pathFindBailoutRadius * 2);
									ts= aStar(unit, newFinalPos, true, frameIndex, maxBailoutNodeCount);
								}
							}
						}
					}
				}
			}
			unit->setInBailOutAttempt(false);

			//printf("$$$$ Unit END BAILOUT ATTEMPT for [%d - %s] ts = %d\n",unit->getId(),unit->getFullName().c_str(),ts);

			if(ts == tsBlocked) {
				unit->setLastStuckFrameToCurrentFrame();
				unit->setLastStuckPos(finalPos);
			}
		}
		if(ts == tsArrived || ts == tsBlocked) {
			if(frameIndex < 0) {
				unit->setCurrSkill(scStop);
			}
		}
		break;
	case tsMoving:
		{
			if(dynamic_cast<UnitPathBasic *>(path) != NULL) {
				UnitPathBasic *basicPath = dynamic_cast<UnitPathBasic *>(path);
				Vec2i pos;
				if(frameIndex < 0) {
					pos = basicPath->pop(frameIndex < 0);
				}
				else {
					pos = factions[unit->getFactionIndex()].precachedPath[unit->getId()][0];
				}

				if(map->canMove(unit, unit->getPos(), pos)) {
					if(frameIndex < 0) {
						unit->setTargetPos(pos);
					}
				}
				else {
					if(frameIndex < 0) {
						unit->setCurrSkill(scStop);
					}
					return tsBlocked;
				}
			}
			else if(dynamic_cast<UnitPath *>(path) != NULL) {
				UnitPath *advPath = dynamic_cast<UnitPath *>(path);
				Vec2i pos= advPath->peek();
				if(map->canMove(unit, unit->getPos(), pos)) {
					if(frameIndex < 0) {
						advPath->pop();
						unit->setTargetPos(pos);
					}
				}
				else {
					if(frameIndex < 0) {
						unit->setCurrSkill(scStop);
					}
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


bool PathFinder::processNode(Unit *unit, Node *node,const Vec2i finalPos, int i, int j, bool &nodeLimitReached,int maxNodeCount) {
	bool result = false;
	Vec2i sucPos= node->pos + Vec2i(i, j);

//	std::map<int, std::map<Vec2i,std::map<Vec2i, bool> > >::iterator iterFind1 = factions[unit->getFactionIndex()].mapFromToNodeList.find(unit->getType()->getId());
//	if(iterFind1 != factions[unit->getFactionIndex()].mapFromToNodeList.end()) {
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
	if(openPos(sucPos, factions[unit->getFactionIndex()]) == false &&
			map->aproxCanMoveSoon(unit, node->pos, sucPos) == true) {
		//if node is not open and canMove then generate another node
		Node *sucNode= newNode(factions[unit->getFactionIndex()],maxNodeCount);
		if(sucNode != NULL) {
			sucNode->pos= sucPos;
			sucNode->heuristic= heuristic(sucNode->pos, finalPos);
			sucNode->prev= node;
			sucNode->next= NULL;
			sucNode->exploredCell= map->getSurfaceCell(Map::toSurfCoords(sucPos))->isExplored(unit->getTeam());
			if(factions[unit->getFactionIndex()].openNodesList.find(sucNode->heuristic) == factions[unit->getFactionIndex()].openNodesList.end()) {
				factions[unit->getFactionIndex()].openNodesList[sucNode->heuristic].clear();
			}
			factions[unit->getFactionIndex()].openNodesList[sucNode->heuristic].push_back(sucNode);
			factions[unit->getFactionIndex()].openPosList[sucNode->pos] = true;

			result = true;
		}
		else {
			nodeLimitReached= true;
		}
	}

//	factions[unit->getFactionIndex()].mapFromToNodeList[unit->getType()->getId()][node->pos][sucPos] = result;
	return result;
}

bool PathFinder::addToOpenSet(Unit *unit, Node *node,const Vec2i finalPos, Vec2i sucPos, bool &nodeLimitReached,int maxNodeCount,Node **newNodeAdded, bool bypassChecks) {
	bool result = false;

	*newNodeAdded=NULL;
	//Vec2i sucPos= node->pos + Vec2i(i, j);
	if(bypassChecks == true ||
		(map->aproxCanMoveSoon(unit, node->pos, sucPos) == true && openPos(sucPos, factions[unit->getFactionIndex()]) == false)) {
		//if node is not open and canMove then generate another node
		Node *sucNode= newNode(factions[unit->getFactionIndex()],maxNodeCount);
		if(sucNode != NULL) {
			sucNode->pos= sucPos;
			sucNode->heuristic= heuristic(sucNode->pos, finalPos);
			sucNode->prev= node;
			sucNode->next= NULL;
			sucNode->exploredCell= map->getSurfaceCell(Map::toSurfCoords(sucPos))->isExplored(unit->getTeam());
			if(factions[unit->getFactionIndex()].openNodesList.find(sucNode->heuristic) == factions[unit->getFactionIndex()].openNodesList.end()) {
				factions[unit->getFactionIndex()].openNodesList[sucNode->heuristic].clear();
			}
			factions[unit->getFactionIndex()].openNodesList[sucNode->heuristic].push_back(sucNode);
			factions[unit->getFactionIndex()].openPosList[sucNode->pos] = true;

			*newNodeAdded=sucNode;
			result = true;
		}
		else {
			nodeLimitReached= true;
		}
	}
	return result;
}


direction PathFinder::directionOfMove(Vec2i to, Vec2i from) const {
	if (from.x == to.x) {
		if (from.y == to.y)
			return -1;
		else if (from.y < to.y)
			return 4;
		else // from.y > to.y
			return 0;
	}
	else if (from.x < to.x) {
		if (from.y == to.y)
			return 2;
		else if (from.y < to.y)
			return 3;
		else // from.y > to.y
			return 1;
	}
	else { // from.x > to.x
		if (from.y == to.y)
			return 6;
		else if (from.y < to.y)
			return 5;
		else // from.y > to.y
			return 7;
	}

}

direction PathFinder::directionWeCameFrom(Vec2i node, Vec2i nodeFrom) const {
	direction result = NO_DIRECTION;
	if(nodeFrom.x >= 0 && nodeFrom.y >= 0) {
		result = directionOfMove(node, nodeFrom);
	}

	//printf("directionWeCameFrom node [%s] nodeFrom [%s] result = %d\n",node.getString().c_str(),nodeFrom.getString().c_str(),result);

	return result;
}

// is this coordinate contained within the map bounds?
bool PathFinder::contained(Vec2i c) {
	return (map->isInside(c) == true && map->isInsideSurface(map->toSurfCoords(c)) == true);
}

// is this coordinate within the map bounds, and also walkable?
bool PathFinder::isEnterable(Vec2i coord) {
	//node node = getIndex (astar->bounds, coord);
	//return contained(coord) && astar->grid[node];
	return contained(coord);
}
// the coordinate one tile in the given direction
Vec2i PathFinder::adjustInDirection(Vec2i c, int dir) {
	// we want to implement "rotation" - that is, for instance, we can
	// subtract 2 from the direction "north" and get "east"
	// C's modulo operator doesn't quite behave the right way to do this,
	// but for our purposes this kluge should be good enough
	switch ((dir + 65536) % 8) {
	case 0: return Vec2i(c.x, c.y - 1);
	case 1: return Vec2i(c.x + 1, c.y - 1);
	case 2: return Vec2i(c.x + 1, c.y );
	case 3: return Vec2i(c.x + 1, c.y + 1);
	case 4: return Vec2i(c.x, c.y + 1);
	case 5: return Vec2i(c.x - 1, c.y + 1);
	case 6: return Vec2i(c.x - 1, c.y);
	case 7: return Vec2i(c.x - 1, c.y - 1);
	}
	return Vec2i( -1, -1 );
}

bool PathFinder::directionIsDiagonal(direction dir) const {
	return (dir % 2) != 0;
}

// logical implication operator
bool PathFinder::implies(bool a, bool b) const {
	return a ? b : true;
}

directionset PathFinder::addDirectionToSet(directionset dirs, direction dir) const {
	return dirs | 1 << dir;
}

directionset PathFinder::forcedNeighbours(Vec2i coord,direction dir) {
	if (dir == NO_DIRECTION)
		return 0;

	directionset dirs = 0;
#define ENTERABLE(n) isEnterable(adjustInDirection(coord, (dir + (n)) % 8))
	if (directionIsDiagonal(dir)) {
		if (!implies (ENTERABLE (6), ENTERABLE (5)))
			dirs = addDirectionToSet (dirs, (dir + 6) % 8);
		if (!implies (ENTERABLE (2), ENTERABLE (3)))
			dirs = addDirectionToSet (dirs, (dir + 2) % 8);
	}
	else {
		if (!implies (ENTERABLE (7), ENTERABLE (6)))
			dirs = addDirectionToSet (dirs, (dir + 7) % 8);
		if (!implies (ENTERABLE (1), ENTERABLE (2)))
			dirs = addDirectionToSet (dirs, (dir + 1) % 8);
	}
#undef ENTERABLE
	return dirs;
}

directionset PathFinder::naturalNeighbours(direction dir) const {
	if (dir == NO_DIRECTION)
		return 255;

	directionset dirs = 0;
	dirs = addDirectionToSet (dirs, dir);
	if (directionIsDiagonal (dir)) {
		dirs = addDirectionToSet (dirs, (dir + 1) % 8);
		dirs = addDirectionToSet (dirs, (dir + 7) % 8);
	}
	return dirs;
}

// return and remove a direction from the set
// returns NO_DIRECTION if the set was empty
direction PathFinder::nextDirectionInSet(directionset *dirs) const {
	for (int i = 0; i < 8; i++) {
		char bit = 1 << i;
		if (*dirs & bit) {
			*dirs ^= bit;
			return i;
		}
	}
	return NO_DIRECTION;
}

// directly translated from "algorithm 2" in the paper
Vec2i PathFinder::jump(Vec2i dest, direction dir, Vec2i start,std::vector<Vec2i> &path, int pathLength) {
	Vec2i coord = adjustInDirection(start, dir);
	//printf("jump dir [%u] start [%s] coord [%s] dest [%s]\n",dir,start.getString().c_str(),coord.getString().c_str(),dest.getString().c_str());

	if (!isEnterable(coord))
		return Vec2i(-1,-1);

	if(path.size() > max(300,pathLength*2)) {
	//if(path.size() > 2000) {
		//printf("path.size() > pathLength [%d]\n",pathLength);
		//return Vec2i(-1,-1);
		return coord;
	}
	path.push_back(coord);

	//int node = getIndex (astar->bounds, coord);
	if (coord == dest || forcedNeighbours(coord, dir)) {
		//path.push_back(coord);
		//printf("jump #1 = %d [%d]\n",(int)path.size(),pathLength);
		return coord;
	}

	if(directionIsDiagonal(dir)) {
		Vec2i next = jump(dest, (dir + 7) % 8, coord,path,pathLength);
		if (next.x >= 0) {
			//path.push_back(coord);
			//printf("jump #2 = %d [%d]\n",(int)path.size(),pathLength);
			return coord;
		}
		next = jump(dest, (dir + 1) % 8, coord, path,pathLength);
		if (next.x >= 0) {
			//path.push_back(coord);
			//printf("jump #3 = %d [%d]\n",(int)path.size(),pathLength);
			return coord;
		}
	}
	//else {
	//path.push_back(coord);
	//}
	return jump(dest, dir, coord, path,pathLength);
}


//route a unit using A* algorithm
TravelState PathFinder::aStar(Unit *unit, const Vec2i &targetPos, bool inBailout,
		int frameIndex, int maxNodeCount) {

	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

	if(map == NULL) {
		throw runtime_error("map == NULL");
	}

	const bool showConsoleDebugInfo = Config::getInstance().getBool("EnablePathfinderDistanceOutput","false");
	const bool tryLastPathCache = Config::getInstance().getBool("EnablePathfinderCache","false");

	if(maxNodeCount < 0) {
		maxNodeCount = factions[unit->getFactionIndex()].useMaxNodeCount;
	}
	UnitPathInterface *path= unit->getPath();

	factions[unit->getFactionIndex()].nodePoolCount= 0;
	factions[unit->getFactionIndex()].openNodesList.clear();
	factions[unit->getFactionIndex()].openPosList.clear();
	factions[unit->getFactionIndex()].closedNodesList.clear();

	TravelState ts = tsImpossible;

	if(frameIndex < 0) {
		if(factions[unit->getFactionIndex()].precachedTravelState.find(unit->getId()) != factions[unit->getFactionIndex()].precachedTravelState.end()) {
			if(factions[unit->getFactionIndex()].precachedTravelState[unit->getId()] == tsMoving) {
				bool canMoveToCells = true;

				Vec2i lastPos = unit->getPos();
				for(int i=0; i < factions[unit->getFactionIndex()].precachedPath[unit->getId()].size(); i++) {
					Vec2i nodePos = factions[unit->getFactionIndex()].precachedPath[unit->getId()][i];
					if(map->isInside(nodePos) == false || map->isInsideSurface(map->toSurfCoords(nodePos)) == false) {
						throw runtime_error("Pathfinder invalid node path position = " + nodePos.getString() + " i = " + intToStr(i));
					}

					if(i < pathFindRefresh ||
						(factions[unit->getFactionIndex()].precachedPath[unit->getId()].size() >= pathFindExtendRefreshForNodeCount &&
						 i < getPathFindExtendRefreshNodeCount(unit->getFactionIndex()))) {
						//!!! Test MV
						if(map->aproxCanMoveSoon(unit, lastPos, nodePos) == false) {
							canMoveToCells = false;
							break;
						}
						lastPos = nodePos;
					}
					else {
						break;
					}
				}

				if(canMoveToCells == true) {
					path->clear();
					UnitPathBasic *basicPathFinder = dynamic_cast<UnitPathBasic *>(path);

					for(int i=0; i < factions[unit->getFactionIndex()].precachedPath[unit->getId()].size(); i++) {
						Vec2i nodePos = factions[unit->getFactionIndex()].precachedPath[unit->getId()][i];
						if(map->isInside(nodePos) == false || map->isInsideSurface(map->toSurfCoords(nodePos)) == false) {
							throw runtime_error("Pathfinder invalid node path position = " + nodePos.getString() + " i = " + intToStr(i));
						}

						if(i < pathFindRefresh ||
								(factions[unit->getFactionIndex()].precachedPath[unit->getId()].size() >= pathFindExtendRefreshForNodeCount &&
								 i < getPathFindExtendRefreshNodeCount(unit->getFactionIndex()))) {
							path->add(nodePos);
						}
						//else if(tryLastPathCache == false) {
						//	break;
						//}

						//if(tryLastPathCache == true && basicPathFinder) {
						if(basicPathFinder) {
							basicPathFinder->addToLastPathCache(nodePos);
						}
					}
					unit->setUsePathfinderExtendedMaxNodes(false);
					return factions[unit->getFactionIndex()].precachedTravelState[unit->getId()];
				}
				else {
					clearUnitPrecache(unit);
				}
			}
			else if(factions[unit->getFactionIndex()].precachedTravelState[unit->getId()] == tsBlocked) {
				path->incBlockCount();
				unit->setUsePathfinderExtendedMaxNodes(false);
				return factions[unit->getFactionIndex()].precachedTravelState[unit->getId()];
			}
		}
	}
	else {
		clearUnitPrecache(unit);
	}

	const Vec2i unitPos = unit->getPos();
	const Vec2i finalPos= computeNearestFreePos(unit, targetPos);

	float dist= unitPos.dist(finalPos);
	factions[unit->getFactionIndex()].useMaxNodeCount = PathFinder::pathFindNodesMax;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled == true && chrono.getMillis() > 4) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	// Check the previous path find cache for the unit to see if its good to
	// use
	if(showConsoleDebugInfo || tryLastPathCache) {
		if(showConsoleDebugInfo && dist > 60) printf("Distance from [%d - %s] to destination is %.2f tryLastPathCache = %d\n",unit->getId(),unit->getFullName().c_str(), dist,tryLastPathCache);

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
									if(frameIndex < 0) {
										basicPathFinder->clear();
									}

									int pathCount=0;
									for(int k=i+1; k <= j; k++) {
										if(k >= cachedPath.size()) {
											throw runtime_error("k >= cachedPath.size() k = " + intToStr(k) + " cachedPath.size() = " + intToStr(cachedPath.size()));
										}

										if(frameIndex >= 0) {
											factions[unit->getFactionIndex()].precachedPath[unit->getId()].push_back(cachedPath[k]);
										}
										else {
											if(pathCount < pathFindRefresh) {
												basicPathFinder->add(cachedPath[k]);
											}
											basicPathFinder->addToLastPathCache(cachedPath[k]);
										}
										pathCount++;
									}

									if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled == true && chrono.getMillis() > 4) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

									if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
										char szBuf[4096]="";
										sprintf(szBuf,"[Setting new path for unit] openNodesList.size() [%lu] openPosList.size() [%lu] finalPos [%s] targetPos [%s] inBailout [%d] ts [%d]",
												factions[unit->getFactionIndex()].openNodesList.size(),factions[unit->getFactionIndex()].openPosList.size(),finalPos.getString().c_str(),targetPos.getString().c_str(),inBailout,ts);
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

									unit->setUsePathfinderExtendedMaxNodes(false);
									return ts;
								}
								else if(j - i > pathFindRefresh) {
									//on the way
									ts= tsMoving;

									if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled == true && chrono.getMillis() > 4) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

									//store path
									if(frameIndex < 0) {
										basicPathFinder->clear();
									}

									int pathCount=0;
									for(int k=i+1; k < cachedPath.size(); k++) {
										if(k >= cachedPath.size()) {
											throw runtime_error("#2 k >= cachedPath.size() k = " + intToStr(k) + " cachedPath.size() = " + intToStr(cachedPath.size()));
										}

										if(frameIndex >= 0) {
											factions[unit->getFactionIndex()].precachedPath[unit->getId()].push_back(cachedPath[k]);
										}
										else {
											if(pathCount < pathFindRefresh) {
												basicPathFinder->add(cachedPath[k]);
											}
											basicPathFinder->addToLastPathCache(cachedPath[k]);
										}
										pathCount++;
									}

									if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled == true && chrono.getMillis() > 4) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

									if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
										char szBuf[4096]="";
										sprintf(szBuf,"[Setting new path for unit] openNodesList.size() [%lu] openPosList.size() [%lu] finalPos [%s] targetPos [%s] inBailout [%d] ts [%d]",
												factions[unit->getFactionIndex()].openNodesList.size(),factions[unit->getFactionIndex()].openPosList.size(),finalPos.getString().c_str(),targetPos.getString().c_str(),inBailout,ts);
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

									unit->setUsePathfinderExtendedMaxNodes(false);
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
	}

	//path find algorithm

	//a) push starting pos into openNodes
	Node *firstNode= newNode(factions[unit->getFactionIndex()],maxNodeCount);
	assert(firstNode != NULL);
	if(firstNode == NULL) {
		throw runtime_error("firstNode == NULL");
	}

	firstNode->next= NULL;
	firstNode->prev= NULL;
	firstNode->pos= unitPos;
	firstNode->heuristic= heuristic(unitPos, finalPos);
	firstNode->exploredCell= true;
	if(factions[unit->getFactionIndex()].openNodesList.find(firstNode->heuristic) == factions[unit->getFactionIndex()].openNodesList.end()) {
		factions[unit->getFactionIndex()].openNodesList[firstNode->heuristic].clear();
	}
	factions[unit->getFactionIndex()].openNodesList[firstNode->heuristic].push_back(firstNode);
	factions[unit->getFactionIndex()].openPosList[firstNode->pos] = true;

	//b) loop
	bool pathFound			= true;
	bool nodeLimitReached	= false;
	Node *node				= NULL;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled == true && chrono.getMillis() > 4) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	// First check if unit currently blocked all around them, if so don't try to pathfind
	if(inBailout == false && unitPos != finalPos) {
		int failureCount = 0;
		int cellCount = 0;

		for(int i = -1; i <= 1; ++i) {
			for(int j = -1; j <= 1; ++j) {
				Vec2i pos = unitPos + Vec2i(i, j);
				if(pos != unitPos) {
					//!!! Test MV
					bool canUnitMoveToCell = map->aproxCanMoveSoon(unit, unitPos, pos);
					if(canUnitMoveToCell == false) {
						failureCount++;
					}
					cellCount++;
				}
			}
		}
		nodeLimitReached = (failureCount == cellCount);
		pathFound = !nodeLimitReached;

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled == true && chrono.getMillis() > 1) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] **Check if dest blocked, distance for unit [%d - %s] from [%s] to [%s] is %.2f took msecs: %lld nodeLimitReached = %d, failureCount = %d\n",__FILE__,__FUNCTION__,__LINE__,unit->getId(),unit->getFullName().c_str(), unitPos.getString().c_str(), finalPos.getString().c_str(), dist,(long long int)chrono.getMillis(),nodeLimitReached,failureCount);
		if(showConsoleDebugInfo && nodeLimitReached) {
			printf("**Check if src blocked [%d - %d], unit [%d - %s] from [%s] to [%s] distance %.2f took msecs: %lld nodeLimitReached = %d, failureCount = %d [%d]\n",
					nodeLimitReached, inBailout, unit->getId(),unit->getFullName().c_str(), unitPos.getString().c_str(), finalPos.getString().c_str(), dist,(long long int)chrono.getMillis(),nodeLimitReached,failureCount,cellCount);
		}

		if(nodeLimitReached == false) {
			// First check if final destination blocked
			failureCount = 0;
			cellCount = 0;

			for(int i = -1; i <= 1; ++i) {
				for(int j = -1; j <= 1; ++j) {
					Vec2i pos = finalPos + Vec2i(i, j);
					if(pos != finalPos) {
						//!!! Test MV
						bool canUnitMoveToCell = map->aproxCanMoveSoon(unit, pos, finalPos);
						if(canUnitMoveToCell == false) {
							failureCount++;
						}
						cellCount++;
					}
				}
			}
			nodeLimitReached = (failureCount == cellCount);
			pathFound = !nodeLimitReached;

			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled == true && chrono.getMillis() > 1) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] **Check if dest blocked, distance for unit [%d - %s] from [%s] to [%s] is %.2f took msecs: %lld nodeLimitReached = %d, failureCount = %d\n",__FILE__,__FUNCTION__,__LINE__,unit->getId(),unit->getFullName().c_str(), unitPos.getString().c_str(), finalPos.getString().c_str(), dist,(long long int)chrono.getMillis(),nodeLimitReached,failureCount);
			if(showConsoleDebugInfo && nodeLimitReached) {
				printf("**Check if dest blocked [%d - %d], unit [%d - %s] from [%s] to [%s] distance %.2f took msecs: %lld nodeLimitReached = %d, failureCount = %d [%d]\n",
						nodeLimitReached, inBailout, unit->getId(),unit->getFullName().c_str(), unitPos.getString().c_str(), finalPos.getString().c_str(), dist,(long long int)chrono.getMillis(),nodeLimitReached,failureCount,cellCount);
			}
		}
	}
	//

	// START
	//Vec2i cameFrom= unit->getPos();
	//Vec2i cameFrom(-1,-1);
	std::map<std::pair<Vec2i,Vec2i> ,bool> canAddNode;
	std::map<Vec2i,bool> closedNodes;
	std::map<Vec2i,Vec2i> cameFrom;
	cameFrom[unitPos] = Vec2i(-1,-1);
	//cameFrom[unitPos] = unit->getPos();

	// Do the a-star base pathfind work if required
	int whileLoopCount = 0;
	if(nodeLimitReached == false) {
		//printf("\n\n\n====== START AStar-JPS Pathfinder start [%s] end [%s]\n",unitPos.getString().c_str(),finalPos.getString().c_str());

		const bool tryJPSPathfinder = false;

		while(nodeLimitReached == false) {
			whileLoopCount++;

			//b1) is open nodes is empty => failed to find the path
			if(factions[unit->getFactionIndex()].openNodesList.empty() == true) {
				//printf("$$$$ Path for Unit [%d - %s] inBailout = %d BLOCKED\n",unit->getId(),unit->getFullName().c_str(),inBailout);
				//printf("Path blocked\n");
				pathFound= false;
				break;
			}

			//b2) get the minimum heuristic node
			//Nodes::iterator it = minHeuristic();
			node = minHeuristicFastLookup(factions[unit->getFactionIndex()]);
			//printf("current node [%s]\n",node->pos.getString().c_str());

			//b3) if minHeuristic is the finalNode, or the path is no more explored => path was found
			if(node->pos == finalPos || node->exploredCell == false) {
				//printf("Path found\n");
				pathFound= true;
				break;
			}

			if(tryJPSPathfinder) {
				closedNodes[node->pos]=true;
			}

			//printf("$$$$ Path for Unit [%d - %s] node [%s] whileLoopCount = %d nodePoolCount = %d inBailout = %d\n",unit->getId(),unit->getFullName().c_str(), node->pos.getString().c_str(), whileLoopCount,factions[unit->getFactionIndex()].nodePoolCount,inBailout);

			//b4) move this node from closedNodes to openNodes
			//add all succesors that are not in closedNodes or openNodes to openNodes
			if(factions[unit->getFactionIndex()].closedNodesList.find(node->heuristic) == factions[unit->getFactionIndex()].closedNodesList.end()) {
				factions[unit->getFactionIndex()].closedNodesList[node->heuristic].clear();
			}
			factions[unit->getFactionIndex()].closedNodesList[node->heuristic].push_back(node);
			factions[unit->getFactionIndex()].openPosList[node->pos] = true;

			if(tryJPSPathfinder) {
				Vec2i cameFromPos(-1,-1);
				if(cameFrom.find(node->pos) != cameFrom.end()) {
					cameFromPos = cameFrom[node->pos];
				}
				direction from = directionWeCameFrom(node->pos,cameFromPos);
				directionset dirs = forcedNeighbours(node->pos, from) | naturalNeighbours(from);

				//printf("JPS #1 node->pos [%s] from = %u dirs = %u\n",node->pos.getString().c_str(),from,dirs);

				bool canAddEntirePath = false;
				bool foundQuickRoute = false;
				for (int dir = nextDirectionInSet(&dirs); dir != NO_DIRECTION; dir = nextDirectionInSet(&dirs)) {
				//for (int dir = 0; dir < 8; dir++) {
					std::vector<Vec2i> path;
					Vec2i newNode = jump(finalPos, dir, node->pos,path,(int)node->pos.dist(finalPos));
					//Vec2i newNode = adjustInDirection(node->pos, dir);

					//printf("examine node from [%u][%u] - current node [%s] next possible node [%s]\n",from,dirs,node->pos.getString().c_str(),newNode.getString().c_str());

					//coord_t newCoord = getCoord (bounds, newNode);

					// this'll also bail out if jump() returned -1
					if (!contained(newNode))
						continue;

					if(closedNodes.find(newNode) != closedNodes.end())
						continue;
					//if(factions[unit->getFactionIndex()].closedNodesList.find(node->heuristic) == factions[unit->getFactionIndex()].closedNodesList.end()) {

					//addToOpenSet (&astar, newNode, node);
					//printf("JPS #2 node->pos [%s] newNode [%s] path.size() [%d] pos [%s]\n",node->pos.getString().c_str(),newNode.getString().c_str(),(int)path.size(),path[0].getString().c_str());

					Vec2i newPath = path[0];

					//bool canUnitMoveToCell = map->aproxCanMove(unit, node->pos, newPath);
					//bool posOpen = (openPos(newPath, factions[unit->getFactionIndex()]) == false);
					//bool isFreeCell = map->isFreeCell(newPath,unit->getType()->getField());

					if(canAddNode.find(make_pair(node->pos,newPath)) == canAddNode.end()) {
						Node *newNode=NULL;
						if(addToOpenSet(unit, node, finalPos, newPath, nodeLimitReached, maxNodeCount,&newNode,false) == true) {
							//cameFrom = node->pos;
							cameFrom[newPath]=node->pos;
							foundQuickRoute = true;

							if(path.size() > 1 && path[path.size()-1] == finalPos) {
								canAddEntirePath = true;
								for(unsigned int x = 1; x < path.size(); ++x) {
									Vec2i futureNode = path[x];

									bool canUnitMoveToCell = map->aproxCanMoveSoon(unit, newNode->pos, futureNode);
									if(canUnitMoveToCell != true || openPos(futureNode, factions[unit->getFactionIndex()]) == true) {
										canAddEntirePath = false;
										canAddNode[make_pair(node->pos,futureNode)]=false;
										//printf("COULD NOT ADD ENTIRE PATH! canUnitMoveToCell = %d\n",canUnitMoveToCell);
										break;
									}
								}
								if(canAddEntirePath == true) {
									//printf("add node - ENTIRE PATH!\n");

									for(unsigned int x = 1; x < path.size(); ++x) {
										Vec2i futureNode = path[x];

										Node *newNode2=NULL;
										addToOpenSet(unit, newNode, finalPos, futureNode, nodeLimitReached, maxNodeCount,&newNode2, true);
										newNode=newNode2;
									}

									//Node *result = factions[unit->getFactionIndex()].openNodesList.begin()->second[0];
									//if(result->pos == finalPos || result->exploredCell == false) {
									//	printf("Will break out of pathfinding now!\n");
									//}
								}
							}
							//printf("add node - current node [%s] next possible node [%s] canUnitMoveToCell [%d] posOpen [%d] isFreeCell [%d]\n",node->pos.getString().c_str(),newPath.getString().c_str(),canUnitMoveToCell,posOpen,isFreeCell);
						}
						else {
							//printf("COULD NOT add node - current node [%s] next possible node [%s] canUnitMoveToCell [%d] posOpen [%d] isFreeCell [%d]\n",node->pos.getString().c_str(),newPath.getString().c_str(),canUnitMoveToCell,posOpen,isFreeCell);
							canAddNode[make_pair(node->pos,newPath)]=false;
						}
					}

					//if(canAddEntirePath == true) {
					//	break;
					//}
				}

				if(foundQuickRoute == false) {
					for (int dir = 0; dir < 8; dir++) {
						Vec2i newNode = adjustInDirection(node->pos, dir);

						//printf("examine node from [%u][%u] - current node [%s] next possible node [%s]\n",from,dirs,node->pos.getString().c_str(),newNode.getString().c_str());

						//coord_t newCoord = getCoord (bounds, newNode);

						// this'll also bail out if jump() returned -1
						if (!contained(newNode))
							continue;

						if(closedNodes.find(newNode) != closedNodes.end())
							continue;
						//if(factions[unit->getFactionIndex()].closedNodesList.find(node->heuristic) == factions[unit->getFactionIndex()].closedNodesList.end()) {

						//addToOpenSet (&astar, newNode, node);
						//printf("JPS #3 node->pos [%s] newNode [%s]\n",node->pos.getString().c_str(),newNode.getString().c_str());

						Vec2i newPath = newNode;

						//bool canUnitMoveToCell = map->aproxCanMove(unit, node->pos, newPath);
						//bool posOpen = (openPos(newPath, factions[unit->getFactionIndex()]) == false);
						//bool isFreeCell = map->isFreeCell(newPath,unit->getType()->getField());

						if(canAddNode.find(make_pair(node->pos,newPath)) == canAddNode.end()) {
							Node *newNode=NULL;
							if(addToOpenSet(unit, node, finalPos, newPath, nodeLimitReached, maxNodeCount,&newNode, false) == true) {
								//cameFrom = node->pos;
								cameFrom[newPath]=node->pos;
								foundQuickRoute = true;

								//printf("#2 add node - current node [%s] next possible node [%s] canUnitMoveToCell [%d] posOpen [%d] isFreeCell [%d]\n",node->pos.getString().c_str(),newPath.getString().c_str(),canUnitMoveToCell,posOpen,isFreeCell);
							}
							else {
								//printf("#2 COULD NOT add node - current node [%s] next possible node [%s] canUnitMoveToCell [%d] posOpen [%d] isFreeCell [%d]\n",node->pos.getString().c_str(),newPath.getString().c_str(),canUnitMoveToCell,posOpen,isFreeCell);
								canAddNode[make_pair(node->pos,newPath)]=false;
							}
						}
					}
				}
			}
			else {
				int failureCount = 0;
				int cellCount = 0;

				int tryDirection = factions[unit->getFactionIndex()].random.randRange(0,3);
				if(tryDirection == 3) {
					for(int i = 1; i >= -1 && nodeLimitReached == false; --i) {
						for(int j = -1; j <= 1 && nodeLimitReached == false; ++j) {
							if(processNode(unit, node, finalPos, i, j, nodeLimitReached, maxNodeCount) == false) {
								failureCount++;
							}
							cellCount++;
						}
					}
				}
				else if(tryDirection == 2) {
					for(int i = -1; i <= 1 && nodeLimitReached == false; ++i) {
						for(int j = 1; j >= -1 && nodeLimitReached == false; --j) {
							if(processNode(unit, node, finalPos, i, j, nodeLimitReached, maxNodeCount) == false) {
								failureCount++;
							}

							cellCount++;
						}
					}
				}
				else if(tryDirection == 1) {
					for(int i = -1; i <= 1 && nodeLimitReached == false; ++i) {
						for(int j = -1; j <= 1 && nodeLimitReached == false; ++j) {
							if(processNode(unit, node, finalPos, i, j, nodeLimitReached, maxNodeCount) == false) {
								failureCount++;
							}

							cellCount++;
						}
					}
				}
				else {
					for(int i = 1; i >= -1 && nodeLimitReached == false; --i) {
						for(int j = 1; j >= -1 && nodeLimitReached == false; --j) {
							if(processNode(unit, node, finalPos, i, j, nodeLimitReached, maxNodeCount) == false) {
								failureCount++;
							}

							cellCount++;
						}
					}
				}
			}
		} //while

		// Now see if the unit is eligble for pathfind max nodes boost?
		if(nodeLimitReached == true && maxNodeCount != pathFindNodesAbsoluteMax) {
			if(unit->isLastPathfindFailedFrameWithinCurrentFrameTolerance() == true) {
				if(frameIndex < 0) {
					unit->setLastPathfindFailedFrameToCurrentFrame();
					unit->setLastPathfindFailedPos(finalPos);
				}

				if(showConsoleDebugInfo || SystemFlags::VERBOSE_MODE_ENABLED) {
					printf("\n\n\n\n$$$ Calling AStar with LARGE maxnodes for unit [%d - %s]\n\n",unit->getId(),unit->getFullName().c_str());
				}

				return aStar(unit, targetPos, false, frameIndex, pathFindNodesAbsoluteMax);
			}
		}
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled == true && chrono.getMillis() > 1) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld nodeLimitReached = %d whileLoopCount = %d nodePoolCount = %d\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis(),nodeLimitReached,whileLoopCount,factions[unit->getFactionIndex()].nodePoolCount);
	if(showConsoleDebugInfo && chrono.getMillis() > 2) {
		printf("Distance for unit [%d - %s] from [%s] to [%s] is %.2f took msecs: %lld nodeLimitReached = %d whileLoopCount = %d nodePoolCount = %d\n",unit->getId(),unit->getFullName().c_str(), unitPos.getString().c_str(), finalPos.getString().c_str(), dist,(long long int)chrono.getMillis(),nodeLimitReached,whileLoopCount,factions[unit->getFactionIndex()].nodePoolCount);
	}

	Node *lastNode= node;

	//if consumed all nodes find best node (to avoid strange behaviour)
	if(nodeLimitReached == true) {
		if(factions[unit->getFactionIndex()].closedNodesList.size() > 0) {
			float bestHeuristic = factions[unit->getFactionIndex()].closedNodesList.begin()->first;
			if(bestHeuristic < lastNode->heuristic) {
				lastNode= factions[unit->getFactionIndex()].closedNodesList.begin()->second[0];
			}
		}
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled == true && chrono.getMillis() > 4) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	//check results of path finding
	ts = tsImpossible;
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

		if(frameIndex < 0) {
			unit->setUsePathfinderExtendedMaxNodes(false);
		}

		ts= tsBlocked;
		if(frameIndex < 0) {
			path->incBlockCount();
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
			char szBuf[4096]="";
			sprintf(szBuf,"[path for unit BLOCKED] openNodesList.size() [%lu] openPosList.size() [%lu] finalPos [%s] targetPos [%s] inBailout [%d] ts [%d]",
					factions[unit->getFactionIndex()].openNodesList.size(),factions[unit->getFactionIndex()].openPosList.size(),finalPos.getString().c_str(),targetPos.getString().c_str(),inBailout,ts);
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

		if(frameIndex < 0) {
			if(maxNodeCount == pathFindNodesAbsoluteMax) {
				unit->setUsePathfinderExtendedMaxNodes(true);
			}
			else {
				unit->setUsePathfinderExtendedMaxNodes(false);
			}
		}

		//store path
		if(frameIndex < 0) {
			path->clear();
		}

		if(pathFound == true) {
			//printf("FULL PATH FOUND from [%s] to [%s]\n",unitPos.getString().c_str(),finalPos.getString().c_str());
		}

		UnitPathBasic *basicPathFinder = dynamic_cast<UnitPathBasic *>(path);

		currNode= firstNode;
		for(int i=0; currNode->next != NULL; currNode= currNode->next, i++) {
			Vec2i nodePos = currNode->next->pos;
			if(map->isInside(nodePos) == false || map->isInsideSurface(map->toSurfCoords(nodePos)) == false) {
				throw runtime_error("Pathfinder invalid node path position = " + nodePos.getString() + " i = " + intToStr(i));
			}

			//printf("nodePos [%s]\n",nodePos.getString().c_str());

			if(frameIndex >= 0) {
				factions[unit->getFactionIndex()].precachedPath[unit->getId()].push_back(nodePos);
			}
			else {
				if(i < pathFindRefresh ||
					(whileLoopCount >= pathFindExtendRefreshForNodeCount &&
					 i < getPathFindExtendRefreshNodeCount(unit->getFactionIndex()))) {
					path->add(nodePos);
				}
				//else if(tryLastPathCache == false) {
				//	break;
				//}

				//if(tryLastPathCache == true && basicPathFinder) {
				if(basicPathFinder) {
					basicPathFinder->addToLastPathCache(nodePos);
				}
			}
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled == true && chrono.getMillis() > 4) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

		if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
			char szBuf[4096]="";
			sprintf(szBuf,"[Setting new path for unit] openNodesList.size() [%lu] openPosList.size() [%lu] finalPos [%s] targetPos [%s] inBailout [%d] ts [%d]",
					factions[unit->getFactionIndex()].openNodesList.size(),factions[unit->getFactionIndex()].openPosList.size(),finalPos.getString().c_str(),targetPos.getString().c_str(),inBailout,ts);
			unit->logSynchData(__FILE__,__LINE__,szBuf);

			string pathToTake = "";
			for(int i = 0; i < path->getQueueCount(); ++i) {
				Vec2i &pos = path->getQueue()[i];
				if(pathToTake != "") {
					pathToTake += ", ";
				}
				pathToTake += pos.getString();
			}
			unit->logSynchData(__FILE__,__LINE__,szBuf);
			sprintf(szBuf,"Path for unit to take = %s",pathToTake.c_str());
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

	factions[unit->getFactionIndex()].openNodesList.clear();
	factions[unit->getFactionIndex()].openPosList.clear();
	factions[unit->getFactionIndex()].closedNodesList.clear();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled == true && chrono.getMillis() > 4) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld --------------------------- [END OF METHOD] ---------------------------\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	if(frameIndex >= 0) {
		factions[unit->getFactionIndex()].precachedTravelState[unit->getId()] = ts;
	}
	else {
		if(SystemFlags::VERBOSE_MODE_ENABLED && chrono.getMillis() >= 5) printf("In [%s::%s Line: %d] astar took [%lld] msecs, ts = %d.\n",__FILE__,__FUNCTION__,__LINE__,(long long int)chrono.getMillis(),ts);
	}

	//printf("$$$$ Path for Unit [%d - %s] return value = %d inBailout = %d\n",unit->getId(),unit->getFullName().c_str(),ts,inBailout);

	return ts;
}

PathFinder::Node *PathFinder::newNode(FactionState &faction, int maxNodeCount) {
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

PathFinder::Node * PathFinder::minHeuristicFastLookup(FactionState &faction) {
	assert(faction.openNodesList.empty() == false);
	if(faction.openNodesList.empty() == true) {
		throw runtime_error("openNodesList.empty() == true");
	}

	Node *result = faction.openNodesList.begin()->second[0];
	faction.openNodesList.begin()->second.erase(faction.openNodesList.begin()->second.begin());
	if(faction.openNodesList.begin()->second.size() == 0) {
		faction.openNodesList.erase(faction.openNodesList.begin());
	}
	return result;
}

bool PathFinder::openPos(const Vec2i &sucPos, FactionState &faction) {
	if(faction.openPosList.find(sucPos) == faction.openPosList.end()) {
		return false;
	}
	return true;
}

int PathFinder::findNodeIndex(Node *node, Nodes &nodeList) {
	int index = -1;
	if(node != NULL) {
		for(unsigned int i = 0; i < nodeList.size(); ++i) {
			Node *curnode = nodeList[i];
			if(node == curnode) {
				index = i;
				break;
			}
		}
	}
	return index;
}

int PathFinder::findNodeIndex(Node *node, std::vector<Node> &nodeList) {
	int index = -1;
	if(node != NULL) {
		for(unsigned int i = 0; i < nodeList.size(); ++i) {
			Node &curnode = nodeList[i];
			if(node == &curnode) {
				index = i;
				break;
			}
		}
	}
	return index;
}

void PathFinder::saveGame(XmlNode *rootNode) {
	std::map<string,string> mapTagReplacements;
	XmlNode *pathfinderNode = rootNode->addChild("PathFinder");

//	static int pathFindNodesMax;
	pathfinderNode->addAttribute("pathFindNodesMax",intToStr(pathFindNodesMax), mapTagReplacements);
//	static int pathFindNodesAbsoluteMax;
	pathfinderNode->addAttribute("pathFindNodesAbsoluteMax",intToStr(pathFindNodesAbsoluteMax), mapTagReplacements);
//	FactionStateList factions;
	for(unsigned int i = 0; i < factions.size(); ++i) {
		FactionState &factionState = factions[i];
		XmlNode *factionsNode = pathfinderNode->addChild("factions");

//		std::map<Vec2i, bool> openPosList;
//		XmlNode *openPosListNode = factionsNode->addChild("openPosList");
//		for(std::map<Vec2i, bool>::iterator iterMap = factionState.openPosList.begin();
//				iterMap != factionState.openPosList.end(); ++iterMap) {
//			openPosListNode->addAttribute("key",iterMap->first.getString(), mapTagReplacements);
//			openPosListNode->addAttribute("value",intToStr(iterMap->second), mapTagReplacements);
//		}
////		std::map<float, Nodes> openNodesList;
//		XmlNode *openNodesListNode = factionsNode->addChild("openNodesList");
//		for(std::map<float, Nodes>::iterator iterMap = factionState.openNodesList.begin();
//				iterMap != factionState.openNodesList.end(); ++iterMap) {
//
//			Nodes &nodeList = iterMap->second;
//			for(unsigned int j = 0; j < nodeList.size(); ++j) {
//				Node *curNode = nodeList[j];
//				XmlNode *openNodesListNodeNode = factionsNode->addChild("openNodesListNode");
//				openNodesListNodeNode->addAttribute("key",floatToStr(iterMap->first), mapTagReplacements);
//
////				Vec2i pos;
//				openNodesListNodeNode->addAttribute("pos",curNode->pos.getString(), mapTagReplacements);
////				Node *next;
//				int nextIdx = findNodeIndex(curNode->next, nodeList);
//				openNodesListNodeNode->addAttribute("next",intToStr(nextIdx), mapTagReplacements);
////				Node *prev;
//				int prevIdx = findNodeIndex(curNode->prev, nodeList);
//				openNodesListNodeNode->addAttribute("prev",intToStr(nextIdx), mapTagReplacements);
////				float heuristic;
//				openNodesListNodeNode->addAttribute("heuristic",floatToStr(curNode->heuristic), mapTagReplacements);
////				bool exploredCell;
//				openNodesListNodeNode->addAttribute("exploredCell",intToStr(curNode->exploredCell), mapTagReplacements);
//			}
//		}
//
////		std::map<float, Nodes> closedNodesList;
//		XmlNode *closedNodesListNode = factionsNode->addChild("closedNodesList");
//		for(std::map<float, Nodes>::iterator iterMap = factionState.closedNodesList.begin();
//				iterMap != factionState.closedNodesList.end(); ++iterMap) {
//
//			Nodes &nodeList = iterMap->second;
//			for(unsigned int j = 0; j < nodeList.size(); ++j) {
//				Node *curNode = nodeList[j];
//				XmlNode *closedNodesListNodeNode = factionsNode->addChild("closedNodesListNode");
//				closedNodesListNodeNode->addAttribute("key",floatToStr(iterMap->first), mapTagReplacements);
//
////				Vec2i pos;
//				closedNodesListNodeNode->addAttribute("pos",curNode->pos.getString(), mapTagReplacements);
////				Node *next;
//				int nextIdx = findNodeIndex(curNode->next, nodeList);
//				closedNodesListNodeNode->addAttribute("next",intToStr(nextIdx), mapTagReplacements);
////				Node *prev;
//				int prevIdx = findNodeIndex(curNode->prev, nodeList);
//				closedNodesListNodeNode->addAttribute("prev",intToStr(nextIdx), mapTagReplacements);
////				float heuristic;
//				closedNodesListNodeNode->addAttribute("heuristic",floatToStr(curNode->heuristic), mapTagReplacements);
////				bool exploredCell;
//				closedNodesListNodeNode->addAttribute("exploredCell",intToStr(curNode->exploredCell), mapTagReplacements);
//			}
//		}

//		std::vector<Node> nodePool;
		for(unsigned int j = 0; j < factionState.nodePool.size(); ++j) {
			Node *curNode = &factionState.nodePool[j];
			XmlNode *nodePoolNode = factionsNode->addChild("nodePool");
			//closedNodesListNodeNode->addAttribute("key",iterMap->first.getString(), mapTagReplacements);

//				Vec2i pos;
			nodePoolNode->addAttribute("pos",curNode->pos.getString(), mapTagReplacements);
//				Node *next;
			int nextIdx = findNodeIndex(curNode->next, factionState.nodePool);
			nodePoolNode->addAttribute("next",intToStr(nextIdx), mapTagReplacements);
//				Node *prev;
			int prevIdx = findNodeIndex(curNode->prev, factionState.nodePool);
			nodePoolNode->addAttribute("prev",intToStr(nextIdx), mapTagReplacements);
//				float heuristic;
			nodePoolNode->addAttribute("heuristic",floatToStr(curNode->heuristic), mapTagReplacements);
//				bool exploredCell;
			nodePoolNode->addAttribute("exploredCell",intToStr(curNode->exploredCell), mapTagReplacements);
		}

//		int nodePoolCount;
		factionsNode->addAttribute("nodePoolCount",intToStr(factionState.nodePoolCount), mapTagReplacements);
//		RandomGen random;
		factionsNode->addAttribute("random",intToStr(factionState.random.getLastNumber()), mapTagReplacements);
//		int useMaxNodeCount;
		factionsNode->addAttribute("useMaxNodeCount",intToStr(factionState.useMaxNodeCount), mapTagReplacements);
//
//		std::map<int,TravelState> precachedTravelState;
//		std::map<int,std::vector<Vec2i> > precachedPath;

	}
//	const Map *map;

}

void PathFinder::loadGame(const XmlNode *rootNode) {
	const XmlNode *pathfinderNode = rootNode->getChild("PathFinder");

	//attackWarnRange = unitupdaterNode->getAttribute("attackWarnRange")->getFloatValue();

	//	static int pathFindNodesMax;
	pathFindNodesMax = pathfinderNode->getAttribute("pathFindNodesMax")->getIntValue();
	//	static int pathFindNodesAbsoluteMax;
	pathFindNodesAbsoluteMax = pathfinderNode->getAttribute("pathFindNodesAbsoluteMax")->getIntValue();

	vector<XmlNode *> factionsNodeList = pathfinderNode->getChildList("factions");
	for(unsigned int i = 0; i < factionsNodeList.size(); ++i) {
		XmlNode *factionsNode = factionsNodeList[i];

		FactionState &factionState = factions[i];
	//		std::vector<Node> nodePool;
		vector<XmlNode *> nodePoolListNode = factionsNode->getChildList("nodePool");
		for(unsigned int j = 0; j < nodePoolListNode.size(); ++j) {
			XmlNode *nodePoolNode = nodePoolListNode[j];

			Node *curNode = &factionState.nodePool[j];

			//closedNodesListNodeNode->addAttribute("key",iterMap->first.getString(), mapTagReplacements);

	//				Vec2i pos;
			curNode->pos = Vec2i::strToVec2(nodePoolNode->getAttribute("pos")->getValue());
	//				Node *next;
			curNode->next = &factionState.nodePool[nodePoolNode->getAttribute("next")->getIntValue()];
	//				Node *prev;
			curNode->prev = &factionState.nodePool[nodePoolNode->getAttribute("prev")->getIntValue()];
	//				float heuristic;
			curNode->heuristic = nodePoolNode->getAttribute("heuristic")->getFloatValue();
	//				bool exploredCell;
			curNode->exploredCell = nodePoolNode->getAttribute("exploredCell")->getIntValue();
		}

		//		int nodePoolCount;
		factionState.nodePoolCount = factionsNode->getAttribute("nodePoolCount")->getIntValue();
		//		RandomGen random;
		factionState.random.setLastNumber(factionsNode->getAttribute("random")->getIntValue());
		//		int useMaxNodeCount;
		factionState.useMaxNodeCount = factionsNode->getAttribute("useMaxNodeCount")->getIntValue();
		//
		//		std::map<int,TravelState> precachedTravelState;
		//		std::map<int,std::vector<Vec2i> > precachedPath;
	}
	//	const Map *map;
}


}} //end namespace
