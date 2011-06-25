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

int PathFinder::pathFindNodesAbsoluteMax	= 900;
int PathFinder::pathFindNodesMax			= 1500;
const int PathFinder::pathFindRefresh		= 10;
const int PathFinder::pathFindBailoutRadius	= 20;
const int PathFinder::pathFindExtendRefreshForNodeCount	= 25;
const int PathFinder::pathFindExtendRefreshNodeCountMin	= 40;
const int PathFinder::pathFindExtendRefreshNodeCountMax	= 60;

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
	bool canUnitMoveToCell = map->aproxCanMove(unit, node->pos, sucPos);
	if(canUnitMoveToCell == true && openPos(sucPos, factions[unit->getFactionIndex()]) == false) {
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
	return result;
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
						if(map->aproxCanMove(unit, lastPos, nodePos) == false) {
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
					bool canUnitMoveToCell = map->aproxCanMove(unit, unitPos, pos);
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
						bool canUnitMoveToCell = map->aproxCanMove(unit, pos, finalPos);
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
	// Do the a-star base pathfind work if required
	int whileLoopCount = 0;
	if(nodeLimitReached == false) {
		while(nodeLimitReached == false) {
			whileLoopCount++;

			//b1) is open nodes is empty => failed to find the path
			if(factions[unit->getFactionIndex()].openNodesList.empty() == true) {
				//printf("$$$$ Path for Unit [%d - %s] inBailout = %d BLOCKED\n",unit->getId(),unit->getFullName().c_str(),inBailout);
				pathFound= false;
				break;
			}

			//b2) get the minimum heuristic node
			//Nodes::iterator it = minHeuristic();
			node = minHeuristicFastLookup(factions[unit->getFactionIndex()]);

			//b3) if minHeuristic is the finalNode, or the path is no more explored => path was found
			if(node->pos == finalPos || node->exploredCell == false) {
				pathFound= true;
				break;
			}

			//printf("$$$$ Path for Unit [%d - %s] node [%s] whileLoopCount = %d nodePoolCount = %d inBailout = %d\n",unit->getId(),unit->getFullName().c_str(), node->pos.getString().c_str(), whileLoopCount,factions[unit->getFactionIndex()].nodePoolCount,inBailout);

			//b4) move this node from closedNodes to openNodes
			//add all succesors that are not in closedNodes or openNodes to openNodes
			if(factions[unit->getFactionIndex()].closedNodesList.find(node->heuristic) == factions[unit->getFactionIndex()].closedNodesList.end()) {
				factions[unit->getFactionIndex()].closedNodesList[node->heuristic].clear();
			}
			factions[unit->getFactionIndex()].closedNodesList[node->heuristic].push_back(node);
			factions[unit->getFactionIndex()].openPosList[node->pos] = true;

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

		UnitPathBasic *basicPathFinder = dynamic_cast<UnitPathBasic *>(path);

		currNode= firstNode;
		for(int i=0; currNode->next != NULL; currNode= currNode->next, i++) {
			Vec2i nodePos = currNode->next->pos;
			if(map->isInside(nodePos) == false || map->isInsideSurface(map->toSurfCoords(nodePos)) == false) {
				throw runtime_error("Pathfinder invalid node path position = " + nodePos.getString() + " i = " + intToStr(i));
			}

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

}} //end namespace
