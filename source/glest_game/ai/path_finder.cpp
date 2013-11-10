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

int PathFinder::pathFindNodesAbsoluteMax	= 900;
int PathFinder::pathFindNodesMax			= 2000;
const int PathFinder::pathFindBailoutRadius	= 20;
const int PathFinder::pathFindExtendRefreshForNodeCount	= 25;
const int PathFinder::pathFindExtendRefreshNodeCountMin	= 40;
const int PathFinder::pathFindExtendRefreshNodeCountMax	= 40;

PathFinder::PathFinder() {
	minorDebugPathfinder = false;
	map=NULL;
}

int PathFinder::getPathFindExtendRefreshNodeCount(FactionState &faction, bool mutexLock) {
	int refreshNodeCount = faction.random.randRange(PathFinder::pathFindExtendRefreshNodeCountMin,PathFinder::pathFindExtendRefreshNodeCountMax);
	return refreshNodeCount;
}

PathFinder::PathFinder(const Map *map) {
	minorDebugPathfinder = false;

	map=NULL;
	init(map);
}

void PathFinder::init(const Map *map) {
	for(int factionIndex = 0; factionIndex < GameConstants::maxPlayers; ++factionIndex) {
		FactionState &faction = factions.getFactionState(factionIndex);

		faction.nodePool.resize(pathFindNodesAbsoluteMax);
		faction.useMaxNodeCount = PathFinder::pathFindNodesMax;
	}
	this->map= map;
}

void PathFinder::init() {
	minorDebugPathfinder = false;
	map=NULL;
}

PathFinder::~PathFinder() {
	for(int factionIndex = 0; factionIndex < GameConstants::maxPlayers; ++factionIndex) {
		FactionState &faction = factions.getFactionState(factionIndex);

		faction.nodePool.clear();
	}
	factions.clear();
	map=NULL;
}

void PathFinder::clearCaches() {
	for(int factionIndex = 0; factionIndex < GameConstants::maxPlayers; ++factionIndex) {
		static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
		FactionState &faction = factions.getFactionState(factionIndex);
		MutexSafeWrapper safeMutex(faction.getMutexPreCache(),mutexOwnerId);

		faction.precachedTravelState.clear();
		faction.precachedPath.clear();
	}
}

void PathFinder::clearUnitPrecache(Unit *unit) {
	if(unit != NULL && factions.size() > unit->getFactionIndex()) {
		int factionIndex = unit->getFactionIndex();
		static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
		FactionState &faction = factions.getFactionState(factionIndex);
		MutexSafeWrapper safeMutex(faction.getMutexPreCache(),mutexOwnerId);

		faction.precachedTravelState[unit->getId()] = tsImpossible;
		faction.precachedPath[unit->getId()].clear();
	}
}

void PathFinder::removeUnitPrecache(Unit *unit) {
	if(unit != NULL && factions.size() > unit->getFactionIndex()) {
		int factionIndex = unit->getFactionIndex();
		static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
		FactionState &faction = factions.getFactionState(factionIndex);
		MutexSafeWrapper safeMutex(faction.getMutexPreCache(),mutexOwnerId);

		if(faction.precachedTravelState.find(unit->getId()) != faction.precachedTravelState.end()) {
			faction.precachedTravelState.erase(unit->getId());
		}
		if(faction.precachedPath.find(unit->getId()) != faction.precachedPath.end()) {
			faction.precachedPath.erase(unit->getId());
		}
	}
}

TravelState PathFinder::findPath(Unit *unit, const Vec2i &finalPos, bool *wasStuck, int frameIndex) {
	TravelState ts = tsImpossible;

	string codeLocation = "1";
	try {

	if(map == NULL) {
		throw megaglest_runtime_error("map == NULL");
	}

	unit->setCurrentPathFinderDesiredFinalPos(finalPos);

	codeLocation = "2";

	if(frameIndex >= 0) {
		codeLocation = "2";
		clearUnitPrecache(unit);
	}
	if(unit->getFaction()->canUnitsPathfind() == true) {
		codeLocation = "3";
		unit->getFaction()->addUnitToPathfindingList(unit->getId());
	}
	else {
		codeLocation = "4";
		if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
			char szBuf[8096]="";
			snprintf(szBuf,8096,"canUnitsPathfind() == false");
			unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
		}

		return tsBlocked;
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"[findPath] unit->getPos() [%s] finalPos [%s]",
				unit->getPos().getString().c_str(),finalPos.getString().c_str());
		unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
	}

	//route cache
	if(finalPos == unit->getPos()) {
		codeLocation = "5";
		if(frameIndex < 0) {
			codeLocation = "6";
			//if arrived
			unit->setCurrSkill(scStop);

			if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true) {
				char szBuf[8096]="";
				snprintf(szBuf,8096,"Unit finalPos [%s] == unit->getPos() [%s]",finalPos.getString().c_str(),unit->getPos().getString().c_str());
				unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
			}

		}
		if(SystemFlags::getSystemSettingType(SystemFlags::debugPathFinder).enabled == true) {
			string commandDesc = "none";
			Command *command= unit->getCurrCommand();
			if(command != NULL && command->getCommandType() != NULL) {
				commandDesc = command->getCommandType()->toString(false);
			}
			char szBuf[8096]="";
			snprintf(szBuf,8096,"State: arrived#1 at pos: %s, command [%s]",finalPos.getString().c_str(),commandDesc.c_str());
			unit->setCurrentUnitTitle(szBuf);
		}

		return tsArrived;
	}

	codeLocation = "7";
	UnitPathInterface *path= unit->getPath();
	if(path->isEmpty() == false) {
		codeLocation = "8";
		if(dynamic_cast<UnitPathBasic *>(path) != NULL) {
			codeLocation = "9";
			//route cache
			UnitPathBasic *basicPath = dynamic_cast<UnitPathBasic *>(path);
			Vec2i pos= basicPath->pop(frameIndex < 0);

			if(map->canMove(unit, unit->getPos(), pos)) {
				codeLocation = "10";
				if(frameIndex < 0) {
					codeLocation = "11";
					unit->setTargetPos(pos);

					if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true) {
						char szBuf[8096]="";
						snprintf(szBuf,8096,"map->canMove to pos [%s] from [%s]",pos.getString().c_str(),unit->getPos().getString().c_str());
						unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
					}

				}

				return tsMoving;
			}
		}
		else if(dynamic_cast<UnitPath *>(path) != NULL) {
			codeLocation = "12";
			UnitPath *advPath = dynamic_cast<UnitPath *>(path);
			//route cache
			Vec2i pos= advPath->peek();
			if(map->canMove(unit, unit->getPos(), pos)) {
				codeLocation = "13";
				if(frameIndex < 0) {
					advPath->pop();
					unit->setTargetPos(pos);
				}

				return tsMoving;
			}
		}
		else {
			throw megaglest_runtime_error("unsupported or missing path finder detected!");
		}
	}

	codeLocation = "14";
	if(path->isStuck() == true &&
			(unit->getLastStuckPos() == finalPos || path->getBlockCount() > 500) &&
		unit->isLastStuckFrameWithinCurrentFrameTolerance(frameIndex >= 0) == true) {
		codeLocation = "15";
		if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
			char szBuf[8096]="";
			snprintf(szBuf,8096,"path->isStuck() == true unit->getLastStuckPos() [%s] finalPos [%s] path->getBlockCount() [%d]",unit->getLastStuckPos().getString().c_str(),finalPos.getString().c_str(),path->getBlockCount());
			unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
		}

		return tsBlocked;
	}

	//route cache miss
	codeLocation = "16";
	int maxNodeCount=-1;
	if(unit->getUsePathfinderExtendedMaxNodes() == true) {
		codeLocation = "17";

		maxNodeCount= PathFinder::pathFindNodesAbsoluteMax;

		if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
			char szBuf[8096]="";
			snprintf(szBuf,8096,"maxNodeCount: %d",maxNodeCount);
			unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
		}
	}

	codeLocation = "18";
	bool minorDebugPathfinderPerformance = false;
	Chrono chrono;
	if(minorDebugPathfinderPerformance) chrono.start();

	uint32 searched_node_count = 0;
	minorDebugPathfinder = false;
	if(minorDebugPathfinder) printf("Legacy Pathfind Unit [%d - %s] from = %s to = %s frameIndex = %d\n",unit->getId(),unit->getType()->getName(false).c_str(),unit->getPos().getString().c_str(),finalPos.getString().c_str(),frameIndex);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"calling aStar()");
		unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
	}

	codeLocation = "19";
	ts = aStar(unit, finalPos, false, frameIndex, maxNodeCount,&searched_node_count);
	codeLocation = "20";
	//post actions
	switch(ts) {
		case tsBlocked:
		case tsArrived:
			codeLocation = "21";
			// The unit is stuck (not only blocked but unable to go anywhere for a while)
			// We will try to bail out of the immediate area
			if( ts == tsBlocked && unit->getInBailOutAttempt() == false &&
				path->isStuck() == true) {

				codeLocation = "22";
				if(minorDebugPathfinder) printf("Pathfind Unit [%d - %s] START BAILOUT ATTEMPT frameIndex = %d\n",unit->getId(),unit->getType()->getName(false).c_str(),frameIndex);

				if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
					char szBuf[8096]="";
					snprintf(szBuf,8096,"[attempting to BAIL OUT] finalPos [%s] ts [%d]",
							finalPos.getString().c_str(),ts);
					unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
				}

				if(wasStuck != NULL) {
					*wasStuck = true;
				}
				unit->setInBailOutAttempt(true);

				codeLocation = "23";
				bool unitImmediatelyBlocked = false;

				// First check if unit currently blocked all around them, if so don't try to pathfind
				const Vec2i unitPos = unit->getPos();
				int failureCount = 0;
				int cellCount = 0;

				codeLocation = "24";
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
				if(unitImmediatelyBlocked == false) {
					codeLocation = "25";

					int factionIndex = unit->getFactionIndex();
					FactionState &faction = factions.getFactionState(factionIndex);

					int tryRadius = faction.random.randRange(0,1);

					// Try to bail out up to PathFinder::pathFindBailoutRadius cells away
					if(tryRadius > 0) {
						for(int bailoutX = -PathFinder::pathFindBailoutRadius; bailoutX <= PathFinder::pathFindBailoutRadius && ts == tsBlocked; ++bailoutX) {
							for(int bailoutY = -PathFinder::pathFindBailoutRadius; bailoutY <= PathFinder::pathFindBailoutRadius && ts == tsBlocked; ++bailoutY) {
								codeLocation = "26";
								const Vec2i newFinalPos = finalPos + Vec2i(bailoutX,bailoutY);
								bool canUnitMove = map->canMove(unit, unit->getPos(), newFinalPos);

								if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
									char szBuf[8096]="";
									snprintf(szBuf,8096,"[attempting to BAIL OUT] finalPos [%s] newFinalPos [%s] ts [%d] canUnitMove [%d]",
											finalPos.getString().c_str(),newFinalPos.getString().c_str(),ts,canUnitMove);
									unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
								}

								if(canUnitMove) {
									codeLocation = "27";

									int maxBailoutNodeCount = (PathFinder::pathFindBailoutRadius * 2);

									if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
										char szBuf[8096]="";
										snprintf(szBuf,8096,"calling aStar()");
										unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
									}

									ts= aStar(unit, newFinalPos, true, frameIndex, maxBailoutNodeCount,&searched_node_count);
									codeLocation = "28";
								}
							}
						}
					}
					else {
						codeLocation = "29";
						for(int bailoutX = PathFinder::pathFindBailoutRadius; bailoutX >= -PathFinder::pathFindBailoutRadius && ts == tsBlocked; --bailoutX) {
							for(int bailoutY = PathFinder::pathFindBailoutRadius; bailoutY >= -PathFinder::pathFindBailoutRadius && ts == tsBlocked; --bailoutY) {
								codeLocation = "30";
								const Vec2i newFinalPos = finalPos + Vec2i(bailoutX,bailoutY);
								bool canUnitMove = map->canMove(unit, unit->getPos(), newFinalPos);

								if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
									char szBuf[8096]="";
									snprintf(szBuf,8096,"[attempting to BAIL OUT] finalPos [%s] newFinalPos [%s] ts [%d] canUnitMove [%d]",
											finalPos.getString().c_str(),newFinalPos.getString().c_str(),ts,canUnitMove);
									unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
								}

								if(canUnitMove) {
									codeLocation = "31";
									int maxBailoutNodeCount = (PathFinder::pathFindBailoutRadius * 2);

									if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
										char szBuf[8096]="";
										snprintf(szBuf,8096,"calling aStar()");
										unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
									}

									ts= aStar(unit, newFinalPos, true, frameIndex, maxBailoutNodeCount,&searched_node_count);
									codeLocation = "32";
								}
							}
						}
					}
				}
				codeLocation = "33";
				unit->setInBailOutAttempt(false);

				if(ts == tsBlocked) {
					codeLocation = "34";
					unit->setLastStuckFrameToCurrentFrame();
					unit->setLastStuckPos(finalPos);
				}
			}
			if(ts == tsArrived || ts == tsBlocked) {
				if(frameIndex < 0) {
					codeLocation = "35";
					unit->setCurrSkill(scStop);
				}
			}
			break;
		case tsMoving:
			{
				codeLocation = "36";
				if(dynamic_cast<UnitPathBasic *>(path) != NULL) {
					UnitPathBasic *basicPath = dynamic_cast<UnitPathBasic *>(path);
					Vec2i pos;
					if(frameIndex < 0) {
						codeLocation = "37";
						pos = basicPath->pop(frameIndex < 0);
					}
					else {
						codeLocation = "38";
						int factionIndex = unit->getFactionIndex();
						static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
						FactionState &faction = factions.getFactionState(factionIndex);
						MutexSafeWrapper safeMutexPreCache(faction.getMutexPreCache(),mutexOwnerId);

						if(faction.precachedPath[unit->getId()].size() <= 0) {
							throw megaglest_runtime_error("factions[unit->getFactionIndex()].precachedPath[unit->getId()].size() <= 0!");
						}

						pos = faction.precachedPath[unit->getId()][0];
						safeMutexPreCache.ReleaseLock();

						codeLocation = "39";
					}

					codeLocation = "40";
					if(map->canMove(unit, unit->getPos(), pos)) {
						if(frameIndex < 0) {
							codeLocation = "41";
							unit->setTargetPos(pos);
						}
					}
					else {
						codeLocation = "42";
						if(frameIndex < 0) {
							codeLocation = "43";
							unit->setCurrSkill(scStop);
						}

						if(minorDebugPathfinderPerformance && chrono.getMillis() >= 1) printf("Unit [%d - %s] astar #2 took [%lld] msecs, ts = %d searched_node_count = %d.\n",unit->getId(),unit->getType()->getName(false).c_str(),(long long int)chrono.getMillis(),ts,searched_node_count);

						if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
							char szBuf[8096]="";
							snprintf(szBuf,8096,"tsBlocked");
							unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
						}

						return tsBlocked;
					}
				}
				else if(dynamic_cast<UnitPath *>(path) != NULL) {
					codeLocation = "44";
					UnitPath *advPath = dynamic_cast<UnitPath *>(path);
					Vec2i pos= advPath->peek();
					if(map->canMove(unit, unit->getPos(), pos)) {
						if(frameIndex < 0) {
							codeLocation = "45";
							advPath->pop();
							unit->setTargetPos(pos);
						}
					}
					else {
						if(frameIndex < 0) {
							codeLocation = "46";
							unit->setCurrSkill(scStop);
						}

						if(minorDebugPathfinder) printf("Pathfind Unit [%d - %s] INT BAILOUT ATTEMPT BLOCKED frameIndex = %d\n",unit->getId(),unit->getType()->getName(false).c_str(),frameIndex);

						if(minorDebugPathfinderPerformance && chrono.getMillis() >= 1) printf("Unit [%d - %s] astar #3 took [%lld] msecs, ts = %d searched_node_count = %d.\n",unit->getId(),unit->getType()->getName(false).c_str(),(long long int)chrono.getMillis(),ts,searched_node_count);
						return tsBlocked;
					}
				}
				else {
					throw megaglest_runtime_error("unsupported or missing path finder detected!");
				}
			}
			break;
	}
	codeLocation = "47";
	if(minorDebugPathfinderPerformance && chrono.getMillis() >= 1) printf("Unit [%d - %s] astar took [%lld] msecs, ts = %d searched_node_count = %d.\n",unit->getId(),unit->getType()->getName(false).c_str(),(long long int)chrono.getMillis(),ts,searched_node_count);

	}
	catch(const exception &ex) {
		//setRunningStatus(false);

		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Loc [%s] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,codeLocation.c_str(),ex.what());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		throw megaglest_runtime_error(ex.what());
	}
	catch(...) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s %d] UNKNOWN error Loc [%s]\n",__FILE__,__FUNCTION__,__LINE__,codeLocation.c_str());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		throw megaglest_runtime_error(szBuf);
	}

	return ts;
}

// ==================== PRIVATE ==================== 

//route a unit using A* algorithm
TravelState PathFinder::aStar(Unit *unit, const Vec2i &targetPos, bool inBailout,
		int frameIndex, int maxNodeCount, uint32 *searched_node_count) {
	TravelState ts = tsImpossible;

	string codeLocation = "1";
	try {


	if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex >= 0) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In aStar()");
		unit->logSynchDataThreaded(__FILE__,__LINE__,szBuf);
	}

	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

	if(map == NULL) {
		throw megaglest_runtime_error("map == NULL");
	}

	codeLocation = "2";

	if(maxNodeCount < 0) {
		codeLocation = "3";

		int factionIndex = unit->getFactionIndex();
		FactionState &faction = factions.getFactionState(factionIndex);

		maxNodeCount = faction.useMaxNodeCount;
	}

	if(maxNodeCount >= 1 && unit->getPathfindFailedConsecutiveFrameCount() >= 3) {
		maxNodeCount = 200;
	}

	codeLocation = "4";
	UnitPathInterface *path= unit->getPath();
	int unitFactionIndex = unit->getFactionIndex();

	int factionIndex = unit->getFactionIndex();
	FactionState &faction = factions.getFactionState(factionIndex);

	faction.nodePoolCount= 0;
	faction.openNodesList.clear();
	faction.openPosList.clear();
	faction.closedNodesList.clear();

	codeLocation = "5";
	// check the pre-cache to see if we can re-use a cached path
	if(frameIndex < 0) {
		codeLocation = "6";

		static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
		MutexSafeWrapper safeMutexPrecache(faction.getMutexPreCache(),mutexOwnerId);
		bool foundPrecacheTravelState = (faction.precachedTravelState.find(unit->getId()) != faction.precachedTravelState.end());
		safeMutexPrecache.ReleaseLock(true);

		if(foundPrecacheTravelState == true) {
			codeLocation = "7";

//			if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
//				char szBuf[8096]="";
//				snprintf(szBuf,8096,"factions[unitFactionIndex].precachedTravelState[unit->getId()]: %d",faction.precachedTravelState[unit->getId()]);
//				unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
//			}

			safeMutexPrecache.Lock();
			bool foundPrecacheTravelStateIsMoving = (faction.precachedTravelState[unit->getId()] == tsMoving);
			safeMutexPrecache.ReleaseLock(true);

			if(foundPrecacheTravelStateIsMoving == true) {
				codeLocation = "8";
				bool canMoveToCells = true;

				Vec2i lastPos = unit->getPos();

				safeMutexPrecache.Lock();
				int unitPrecachePathSize = faction.precachedPath[unit->getId()].size();
				safeMutexPrecache.ReleaseLock(true);

				for(int i=0; i < unitPrecachePathSize; i++) {
					codeLocation = "9";

					safeMutexPrecache.Lock();
					Vec2i nodePos = faction.precachedPath[unit->getId()][i];
					safeMutexPrecache.ReleaseLock(true);

					if(map->isInside(nodePos) == false || map->isInsideSurface(map->toSurfCoords(nodePos)) == false) {
						throw megaglest_runtime_error("Pathfinder invalid node path position = " + nodePos.getString() + " i = " + intToStr(i));
					}

					if(i < unit->getPathFindRefreshCellCount() ||
						(unitPrecachePathSize >= pathFindExtendRefreshForNodeCount &&
						 i < getPathFindExtendRefreshNodeCount(faction,false))) {
						codeLocation = "10";

						if(canUnitMoveSoon(unit, lastPos, nodePos) == false) {
							canMoveToCells = false;
							break;
						}
						lastPos = nodePos;
					}
					else {
						break;
					}
				}

				codeLocation = "11";
				if(canMoveToCells == true) {
					codeLocation = "12";
					path->clear();
					UnitPathBasic *basicPathFinder = dynamic_cast<UnitPathBasic *>(path);

					safeMutexPrecache.Lock();
					int unitPrecachePathSize = faction.precachedPath[unit->getId()].size();
					safeMutexPrecache.ReleaseLock(true);

					for(int i=0; i < unitPrecachePathSize; i++) {
						codeLocation = "13";

						safeMutexPrecache.Lock();
						Vec2i nodePos = faction.precachedPath[unit->getId()][i];
						safeMutexPrecache.ReleaseLock(true);

						if(map->isInside(nodePos) == false || map->isInsideSurface(map->toSurfCoords(nodePos)) == false) {
							throw megaglest_runtime_error("Pathfinder invalid node path position = " + nodePos.getString() + " i = " + intToStr(i));
						}

						//if(i < pathFindRefresh ||
						if(i < unit->getPathFindRefreshCellCount() ||
								(unitPrecachePathSize >= pathFindExtendRefreshForNodeCount &&
								 i < getPathFindExtendRefreshNodeCount(faction,false))) {
							codeLocation = "14";
							path->add(nodePos);
						}
					}
					codeLocation = "16";
					unit->setUsePathfinderExtendedMaxNodes(false);

//					if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
//						char szBuf[8096]="";
//						snprintf(szBuf,8096,"return factions[unitFactionIndex].precachedTravelState[unit->getId()];");
//						unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
//					}

					codeLocation = "17";
					safeMutexPrecache.Lock();
					return faction.precachedTravelState[unit->getId()];
				}
				else {
					codeLocation = "18";
					clearUnitPrecache(unit);
				}
			}
			else {
				codeLocation = "19";

				safeMutexPrecache.Lock();
				bool foundPrecacheTravelStateIsBlocked = (faction.precachedTravelState[unit->getId()] == tsBlocked);
				safeMutexPrecache.ReleaseLock(true);

				if(foundPrecacheTravelStateIsBlocked == true) {
					codeLocation = "20";
					path->incBlockCount();
					unit->setUsePathfinderExtendedMaxNodes(false);

//					if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
//						char szBuf[8096]="";
//						snprintf(szBuf,8096,"return factions[unitFactionIndex].precachedTravelState[unit->getId()];");
//						unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
//					}

					codeLocation = "21";
					safeMutexPrecache.Lock();
					return faction.precachedTravelState[unit->getId()];
				}
			}
		}
	}
	else {
		codeLocation = "22";
		clearUnitPrecache(unit);

		if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
			char szBuf[8096]="";
			snprintf(szBuf,8096,"[clearUnitPrecache]");
			unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
		}
	}

	codeLocation = "23";
	const Vec2i unitPos = unit->getPos();
	const Vec2i finalPos= computeNearestFreePos(unit, targetPos);

	codeLocation = "24";
	float dist= unitPos.dist(finalPos);

	faction.useMaxNodeCount = PathFinder::pathFindNodesMax;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled == true && chrono.getMillis() > 4) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());

	//path find algorithm
	codeLocation = "26";

	//a) push starting pos into openNodes
	Node *firstNode= newNode(faction,maxNodeCount);
	if(firstNode == NULL) {
		throw megaglest_runtime_error("firstNode == NULL");
	}

	firstNode->next= NULL;
	firstNode->prev= NULL;
	firstNode->pos= unitPos;
	firstNode->heuristic= heuristic(unitPos, finalPos);
	firstNode->exploredCell= true;
	if(faction.openNodesList.find(firstNode->heuristic) == faction.openNodesList.end()) {
		faction.openNodesList[firstNode->heuristic].clear();
	}
	faction.openNodesList[firstNode->heuristic].push_back(firstNode);
	faction.openPosList[firstNode->pos] = true;

	codeLocation = "27";
	//b) loop
	bool pathFound			= true;
	bool nodeLimitReached	= false;
	Node *node				= NULL;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled == true && chrono.getMillis() > 4) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());

	// First check if unit currently blocked all around them, if so don't try to pathfind
	if(inBailout == false && unitPos != finalPos) {
		codeLocation = "28";
		int failureCount = 0;
		int cellCount = 0;

		for(int i = -1; i <= 1; ++i) {
			for(int j = -1; j <= 1; ++j) {
				Vec2i pos = unitPos + Vec2i(i, j);
				if(pos != unitPos) {
					bool canUnitMoveToCell = canUnitMoveSoon(unit, unitPos, pos);
					if(canUnitMoveToCell == false) {
						failureCount++;
					}
					cellCount++;
				}
			}
		}
		nodeLimitReached = (failureCount == cellCount);
		pathFound = !nodeLimitReached;

		if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
			char szBuf[8096]="";
			snprintf(szBuf,8096,"nodeLimitReached: %d failureCount: %d cellCount: %d",nodeLimitReached,failureCount,cellCount);
			unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled == true && chrono.getMillis() > 1) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] **Check if dest blocked, distance for unit [%d - %s] from [%s] to [%s] is %.2f took msecs: %lld nodeLimitReached = %d, failureCount = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,unit->getId(),unit->getFullName(false).c_str(), unitPos.getString().c_str(), finalPos.getString().c_str(), dist,(long long int)chrono.getMillis(),nodeLimitReached,failureCount);

		codeLocation = "29";
		if(nodeLimitReached == false) {
			codeLocation = "30";
			// First check if final destination blocked
			failureCount = 0;
			cellCount = 0;

			for(int i = -1; i <= 1; ++i) {
				for(int j = -1; j <= 1; ++j) {
					Vec2i pos = finalPos + Vec2i(i, j);
					if(pos != finalPos) {
						bool canUnitMoveToCell = canUnitMoveSoon(unit, pos, finalPos);
						if(canUnitMoveToCell == false) {
							failureCount++;
						}
						cellCount++;
					}
				}
			}
			nodeLimitReached = (failureCount == cellCount);
			pathFound = !nodeLimitReached;

			if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
				char szBuf[8096]="";
				snprintf(szBuf,8096,"nodeLimitReached: %d failureCount: %d cellCount: %d",nodeLimitReached,failureCount,cellCount);
				unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
			}

			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled == true && chrono.getMillis() > 1) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] **Check if dest blocked, distance for unit [%d - %s] from [%s] to [%s] is %.2f took msecs: %lld nodeLimitReached = %d, failureCount = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,unit->getId(),unit->getFullName(false).c_str(), unitPos.getString().c_str(), finalPos.getString().c_str(), dist,(long long int)chrono.getMillis(),nodeLimitReached,failureCount);
		}
	}
	else {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
			char szBuf[8096]="";
			snprintf(szBuf,8096,"inBailout: %d unitPos: [%s] finalPos [%s]",inBailout,unitPos.getString().c_str(), finalPos.getString().c_str());
			unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
		}
	}
	//

	codeLocation = "31";
	// START
	std::map<std::pair<Vec2i,Vec2i> ,bool> canAddNode;
	std::map<Vec2i,bool> closedNodes;
	std::map<Vec2i,Vec2i> cameFrom;
	cameFrom[unitPos] = Vec2i(-1,-1);

	// Do the a-star base pathfind work if required
	int whileLoopCount = 0;
	if(nodeLimitReached == false) {
		codeLocation = "32";

		if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
			char szBuf[8096]="";
			snprintf(szBuf,8096,"Calling doAStarPathSearch nodeLimitReached: %d whileLoopCount: %d unitFactionIndex: %d pathFound: %d finalPos [%s] maxNodeCount: %d frameIndex: %d",nodeLimitReached, whileLoopCount, unitFactionIndex,
					pathFound, finalPos.getString().c_str(),  maxNodeCount,frameIndex);
			unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
		}

		doAStarPathSearch(nodeLimitReached, whileLoopCount, unitFactionIndex,
							pathFound, node, finalPos,
							closedNodes, cameFrom, canAddNode, unit, maxNodeCount,frameIndex);

		codeLocation = "33";
		if(searched_node_count != NULL) {
			*searched_node_count = whileLoopCount;
		}

		// Now see if the unit is eligible for pathfind max nodes boost?
		if(nodeLimitReached == true) {
			codeLocation = "34";
			unit->incrementPathfindFailedConsecutiveFrameCount();
		}
		else {
			codeLocation = "35";
			unit->resetPathfindFailedConsecutiveFrameCount();
		}

		codeLocation = "36";
		if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
			char szBuf[8096]="";
			snprintf(szBuf,8096,"Calling doAStarPathSearch nodeLimitReached: %d whileLoopCount: %d unitFactionIndex: %d pathFound: %d finalPos [%s] maxNodeCount: %d pathFindNodesAbsoluteMax: %d frameIndex: %d",nodeLimitReached, whileLoopCount, unitFactionIndex,
					pathFound, finalPos.getString().c_str(),  maxNodeCount,pathFindNodesAbsoluteMax,frameIndex);
			unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
		}

		if(nodeLimitReached == true && maxNodeCount != pathFindNodesAbsoluteMax) {
			codeLocation = "37";
			if(unit->isLastPathfindFailedFrameWithinCurrentFrameTolerance() == true) {
				codeLocation = "38";
				if(frameIndex < 0) {
					codeLocation = "39";
					unit->setLastPathfindFailedFrameToCurrentFrame();
					unit->setLastPathfindFailedPos(finalPos);
				}

				if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
					char szBuf[8096]="";
					snprintf(szBuf,8096,"calling aStar()");
					unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
				}

				return aStar(unit, targetPos, false, frameIndex, pathFindNodesAbsoluteMax);
			}
		}
	}
	else {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
			char szBuf[8096]="";
			snprintf(szBuf,8096,"nodeLimitReached: %d",nodeLimitReached);
			unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
		}
	}

	codeLocation = "40";

	Node *lastNode= node;

	//if consumed all nodes find best node (to avoid strange behaviour)
	if(nodeLimitReached == true) {
		codeLocation = "41";

		if(faction.closedNodesList.empty() == false) {
			codeLocation = "42";
			float bestHeuristic = faction.closedNodesList.begin()->first;
			if(bestHeuristic < lastNode->heuristic) {
				codeLocation = "43";
				lastNode= faction.closedNodesList.begin()->second[0];
			}
		}
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled == true && chrono.getMillis() > 4) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());

	codeLocation = "44";
	//check results of path finding
	ts = tsImpossible;
	if(pathFound == false || lastNode == firstNode) {
		codeLocation = "45";
		if(minorDebugPathfinder) printf("Legacy Pathfind Unit [%d - %s] NOT FOUND PATH count = %d frameIndex = %d\n",unit->getId(),unit->getType()->getName().c_str(),whileLoopCount,frameIndex);

		//blocked
		if(SystemFlags::getSystemSettingType(SystemFlags::debugPathFinder).enabled == true) {
			string commandDesc = "none";
			Command *command= unit->getCurrCommand();
			if(command != NULL && command->getCommandType() != NULL) {
				commandDesc = command->getCommandType()->toString(false);
			}

			std::pair<Vec2i,int> lastHarvest = unit->getLastHarvestResourceTarget();

			char szBuf[8096]="";
			snprintf(szBuf,8096,"State: blocked, cmd [%s] pos: [%s], dest pos: [%s], lastHarvest = [%s - %d], reason A= %d, B= %d, C= %d, D= %d, E= %d, F = %d",commandDesc.c_str(),unit->getPos().getString().c_str(), targetPos.getString().c_str(),lastHarvest.first.getString().c_str(),lastHarvest.second, pathFound,(lastNode == firstNode),path->getBlockCount(), path->isBlocked(), nodeLimitReached,path->isStuck());
			unit->setCurrentUnitTitle(szBuf);
		}

		if(frameIndex < 0) {
			codeLocation = "46";
			unit->setUsePathfinderExtendedMaxNodes(false);
		}

		ts= tsBlocked;
		if(frameIndex < 0) {
			codeLocation = "47";
			path->incBlockCount();
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled == true && chrono.getMillis() > 4) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());
	}
	else {
		codeLocation = "48";
		if(minorDebugPathfinder) printf("Legacy Pathfind Unit [%d - %s] FOUND PATH count = %d frameIndex = %d\n",unit->getId(),unit->getType()->getName().c_str(),whileLoopCount,frameIndex);
		//on the way
		ts= tsMoving;

		//build next pointers
		Node *currNode= lastNode;
		while(currNode->prev != NULL) {
			currNode->prev->next= currNode;
			currNode= currNode->prev;
		}

		codeLocation = "49";
		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled == true && chrono.getMillis() > 4) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());

		if(frameIndex < 0) {
			codeLocation = "50";
			if(maxNodeCount == pathFindNodesAbsoluteMax) {
				codeLocation = "51";
				unit->setUsePathfinderExtendedMaxNodes(true);
			}
			else {
				codeLocation = "52";
				unit->setUsePathfinderExtendedMaxNodes(false);
			}
		}

		//store path
		if(frameIndex < 0) {
			codeLocation = "53";
			path->clear();
		}

		codeLocation = "54";
		UnitPathBasic *basicPathFinder = dynamic_cast<UnitPathBasic *>(path);

		currNode= firstNode;

		static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
		MutexSafeWrapper safeMutexPreCache(faction.getMutexPreCache(),mutexOwnerId);
		safeMutexPreCache.ReleaseLock(true);

		for(int i=0; currNode->next != NULL; currNode= currNode->next, i++) {
			codeLocation = "55";
			Vec2i nodePos = currNode->next->pos;
			if(map->isInside(nodePos) == false || map->isInsideSurface(map->toSurfCoords(nodePos)) == false) {
				throw megaglest_runtime_error("Pathfinder invalid node path position = " + nodePos.getString() + " i = " + intToStr(i));
			}

			if(minorDebugPathfinder) printf("nodePos [%s]\n",nodePos.getString().c_str());

			if(frameIndex >= 0) {
				codeLocation = "56";

				safeMutexPreCache.Lock();
				faction.precachedPath[unit->getId()].push_back(nodePos);
				safeMutexPreCache.ReleaseLock(true);
			}
			else {
				codeLocation = "57";
				if(i < unit->getPathFindRefreshCellCount() ||
					(whileLoopCount >= pathFindExtendRefreshForNodeCount &&
					 i < getPathFindExtendRefreshNodeCount(faction,true))) {
					codeLocation = "58";
					path->add(nodePos);
				}
			}
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled == true && chrono.getMillis() > 4) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());

		if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
			char szBuf[8096]="";

			string pathToTake = "";
			for(int i = 0; i < path->getQueueCount(); ++i) {
				Vec2i &pos = path->getQueue()[i];
				if(pathToTake != "") {
					pathToTake += ", ";
				}
				pathToTake += pos.getString();
			}
			unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
			snprintf(szBuf,8096,"Path for unit to take = %s",pathToTake.c_str());
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPathFinder).enabled == true) {
			string commandDesc = "none";
			Command *command= unit->getCurrCommand();
			if(command != NULL && command->getCommandType() != NULL) {
				commandDesc = command->getCommandType()->toString(false);
			}

			char szBuf[8096]="";
			snprintf(szBuf,8096,"State: moving, cmd [%s] pos: %s dest pos: %s, Queue= %d",commandDesc.c_str(),unit->getPos().getString().c_str(), targetPos.getString().c_str(),path->getQueueCount());
			unit->setCurrentUnitTitle(szBuf);
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled == true && chrono.getMillis() > 4) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());
	}

	codeLocation = "60";

	faction.openNodesList.clear();
	faction.openPosList.clear();
	faction.closedNodesList.clear();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled == true && chrono.getMillis() > 4) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld --------------------------- [END OF METHOD] ---------------------------\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());

	if(frameIndex >= 0) {
		codeLocation = "61";

		static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
		FactionState &faction = factions.getFactionState(factionIndex);
		MutexSafeWrapper safeMutexPrecache(faction.getMutexPreCache(),mutexOwnerId);

		faction.precachedTravelState[unit->getId()] = ts;
	}
	else {
		if(SystemFlags::VERBOSE_MODE_ENABLED && chrono.getMillis() >= 5) printf("In [%s::%s Line: %d] astar took [%lld] msecs, ts = %d.\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,(long long int)chrono.getMillis(),ts);
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true && frameIndex < 0) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"return ts: %d",ts);
		unit->logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);
	}

	}
	catch(const exception &ex) {

		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Loc [%s] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,codeLocation.c_str(),ex.what());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		throw megaglest_runtime_error(ex.what());
	}
	catch(...) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s %d] UNKNOWN error Loc [%s]\n",__FILE__,__FUNCTION__,__LINE__,codeLocation.c_str());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		throw megaglest_runtime_error(szBuf);
	}

	return ts;
}

void PathFinder::processNearestFreePos(const Vec2i &finalPos, int i, int j, int size, Field field, int teamIndex,Vec2i unitPos, Vec2i &nearestPos, float &nearestDist) {
	string codeLocation = "1";

	try {
	Vec2i currPos= finalPos + Vec2i(i, j);

	codeLocation = "2";
	if(map->isAproxFreeCells(currPos, size, field, teamIndex)) {
		codeLocation = "3";
		float dist= currPos.dist(finalPos);

		codeLocation = "4";
		//if nearer from finalPos
		if(dist < nearestDist){
			codeLocation = "5";
			nearestPos= currPos;
			nearestDist= dist;
		}
		//if the distance is the same compare distance to unit
		else if(dist == nearestDist){
			codeLocation = "6";
			if(currPos.dist(unitPos) < nearestPos.dist(unitPos)) {
				codeLocation = "7";
				nearestPos= currPos;
			}
		}
		codeLocation = "8";
	}

	}
	catch(const exception &ex) {

		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Loc [%s] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,codeLocation.c_str(),ex.what());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		throw megaglest_runtime_error(ex.what());
	}
	catch(...) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s %d] UNKNOWN error Loc [%s]\n",__FILE__,__FUNCTION__,__LINE__,codeLocation.c_str());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		throw megaglest_runtime_error(szBuf);
	}

}

Vec2i PathFinder::computeNearestFreePos(const Unit *unit, const Vec2i &finalPos) {
	string codeLocation = "1";

	Vec2i nearestPos(0,0);
	try {

	if(map == NULL) {
		throw megaglest_runtime_error("map == NULL");
	}
	
	codeLocation = "2";
	//unit data
	int size= unit->getType()->getSize();
	Field field= unit->getCurrField();
	int teamIndex= unit->getTeam();

	codeLocation = "3";
	//if finalPos is free return it
	if(map->isAproxFreeCells(finalPos, size, field, teamIndex)) {
		codeLocation = "4";
		return finalPos;
	}

	codeLocation = "5";
	//find nearest pos
	Vec2i unitPos= unit->getPosNotThreadSafe();
	nearestPos= unitPos;

	codeLocation = "6";
	float nearestDist= unitPos.dist(finalPos);

	codeLocation = "7";
	for(int i= -maxFreeSearchRadius; i <= maxFreeSearchRadius; ++i) {
		for(int j= -maxFreeSearchRadius; j <= maxFreeSearchRadius; ++j) {
			codeLocation = "8";
			processNearestFreePos(finalPos, i, j, size, field, teamIndex, unitPos, nearestPos, nearestDist);
		}
	}

	codeLocation = "9";
	}
	catch(const exception &ex) {

		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Loc [%s] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,codeLocation.c_str(),ex.what());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		throw megaglest_runtime_error(ex.what());
	}
	catch(...) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s %d] UNKNOWN error Loc [%s]\n",__FILE__,__FUNCTION__,__LINE__,codeLocation.c_str());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		throw megaglest_runtime_error(szBuf);
	}

	return nearestPos;
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

bool PathFinder::unitCannotMove(Unit *unit) {
	bool unitImmediatelyBlocked = false;

	string codeLocation = "1";
	try {
	// First check if unit currently blocked all around them, if so don't try to pathfind
	const Vec2i unitPos = unit->getPos();
	int failureCount = 0;
	int cellCount = 0;

	codeLocation = "2";
	for(int i = -1; i <= 1; ++i) {
		for(int j = -1; j <= 1; ++j) {
			codeLocation = "3";
			Vec2i pos = unitPos + Vec2i(i, j);
			if(pos != unitPos) {
				codeLocation = "4";
				bool canUnitMoveToCell = map->aproxCanMove(unit, unitPos, pos);
				if(canUnitMoveToCell == false) {
					failureCount++;
				}
				cellCount++;
			}
		}
	}
	codeLocation = "5";
	unitImmediatelyBlocked = (failureCount == cellCount);

	}
	catch(const exception &ex) {
		//setRunningStatus(false);

		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Loc [%s] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,codeLocation.c_str(),ex.what());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		throw megaglest_runtime_error(ex.what());
	}
	catch(...) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s %d] UNKNOWN error Loc [%s]\n",__FILE__,__FUNCTION__,__LINE__,codeLocation.c_str());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		throw megaglest_runtime_error(szBuf);
	}

	return unitImmediatelyBlocked;
}

void PathFinder::saveGame(XmlNode *rootNode) {
	std::map<string,string> mapTagReplacements;
	XmlNode *pathfinderNode = rootNode->addChild("PathFinder");

	pathfinderNode->addAttribute("pathFindNodesMax",intToStr(pathFindNodesMax), mapTagReplacements);
	pathfinderNode->addAttribute("pathFindNodesAbsoluteMax",intToStr(pathFindNodesAbsoluteMax), mapTagReplacements);
	for(unsigned int i = 0; i < factions.size(); ++i) {
		FactionState &factionState = factions.getFactionState(i);
		XmlNode *factionsNode = pathfinderNode->addChild("factions");

		for(unsigned int j = 0; j < factionState.nodePool.size(); ++j) {
			Node *curNode = &factionState.nodePool[j];
			XmlNode *nodePoolNode = factionsNode->addChild("nodePool");

			nodePoolNode->addAttribute("pos",curNode->pos.getString(), mapTagReplacements);
			int nextIdx = findNodeIndex(curNode->next, factionState.nodePool);
			nodePoolNode->addAttribute("next",intToStr(nextIdx), mapTagReplacements);
			int prevIdx = findNodeIndex(curNode->prev, factionState.nodePool);
			nodePoolNode->addAttribute("prev",intToStr(prevIdx), mapTagReplacements);
			nodePoolNode->addAttribute("heuristic",floatToStr(curNode->heuristic,6), mapTagReplacements);
			nodePoolNode->addAttribute("exploredCell",intToStr(curNode->exploredCell), mapTagReplacements);
		}

		factionsNode->addAttribute("nodePoolCount",intToStr(factionState.nodePoolCount), mapTagReplacements);
		factionsNode->addAttribute("random",intToStr(factionState.random.getLastNumber()), mapTagReplacements);
		factionsNode->addAttribute("useMaxNodeCount",intToStr(factionState.useMaxNodeCount), mapTagReplacements);
	}
}

void PathFinder::loadGame(const XmlNode *rootNode) {
	const XmlNode *pathfinderNode = rootNode->getChild("PathFinder");

	vector<XmlNode *> factionsNodeList = pathfinderNode->getChildList("factions");
	for(unsigned int i = 0; i < factionsNodeList.size(); ++i) {
		XmlNode *factionsNode = factionsNodeList[i];

		FactionState &factionState = factions.getFactionState(i);
		vector<XmlNode *> nodePoolListNode = factionsNode->getChildList("nodePool");
		for(unsigned int j = 0; j < nodePoolListNode.size() && j < pathFindNodesAbsoluteMax; ++j) {
			XmlNode *nodePoolNode = nodePoolListNode[j];

			Node *curNode = &factionState.nodePool[j];
			curNode->pos = Vec2i::strToVec2(nodePoolNode->getAttribute("pos")->getValue());
			int nextNode = nodePoolNode->getAttribute("next")->getIntValue();
			if(nextNode >= 0) {
				curNode->next = &factionState.nodePool[nextNode];
			}
			else {
				curNode->next = NULL;
			}

			int prevNode = nodePoolNode->getAttribute("prev")->getIntValue();
			if(prevNode >= 0) {
				curNode->prev = &factionState.nodePool[prevNode];
			}
			else {
				curNode->prev = NULL;
			}
			curNode->heuristic = nodePoolNode->getAttribute("heuristic")->getFloatValue();
			curNode->exploredCell = nodePoolNode->getAttribute("exploredCell")->getIntValue() != 0;
		}

		factionState.nodePoolCount = factionsNode->getAttribute("nodePoolCount")->getIntValue();
		factionState.random.setLastNumber(factionsNode->getAttribute("random")->getIntValue());
		factionState.useMaxNodeCount = PathFinder::pathFindNodesMax;
	}
}

}} //end namespace
