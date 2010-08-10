// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2009-2010 James McCulloch
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_ROUTEPLANNER_H_
#define _GLEST_GAME_ROUTEPLANNER_H_

#include "game_constants.h"
#include "influence_map.h"
#include "annotated_map.h"
#include "cluster_map.h"
#include "config.h"
#include "profiler.h"

#include "search_engine.h"
#include "cartographer.h"
#include "node_pool.h"

#include "world.h"
#include "types.h"

#define PF_TRACE() SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__)

using Shared::Graphics::Vec2i;
using Shared::Platform::uint32;

namespace Glest { namespace Game {

/** maximum radius to search for a free position from a target position */
const int maxFreeSearchRadius = 10;
/** @deprecated not in use */
const int pathFindNodesMax = 2048;

typedef SearchEngine<TransitionNodeStore,TransitionNeighbours,const Transition*> TransitionSearchEngine;

class PMap1Goal {
protected:
	PatchMap<1> *pMap;

public:
	PMap1Goal(PatchMap<1> *pMap) : pMap(pMap) {}

	bool operator()(const Vec2i &pos, const float) const {
		if (pMap->getInfluence(pos)) {
			return true;
		}
		return false;
	}
};

/** gets the two 'diagonal' cells to check for obstacles when a unit is moving diagonally
  * @param s start pos
  * @param d destination pos
  * @param size size of unit
  * @return d1 & d2, the two cells to check
  * @warning assumes s & d are indeed diagonal, abs(s.x - d.x) == 1 && abs(s.y - d.y) == 1
  */
__inline void getDiags(const Vec2i &s, const Vec2i &d, const int size, Vec2i &d1, Vec2i &d2) {
#	define _SEARCH_ENGINE_GET_DIAGS_DEFINED_
	assert(abs(s.x - d.x) == 1 && abs(s.y - d.y) == 1);
	if (size == 1) {
		d1.x = s.x; d1.y = d.y;
		d2.x = d.x; d2.y = s.y;
		return;
	}
	if (d.x > s.x) {  // travelling east
		if (d.y > s.y) {  // se
			d1.x = d.x + size - 1; d1.y = s.y;
			d2.x = s.x; d2.y = d.y + size - 1;
		} else {  // ne
			d1.x = s.x; d1.y = d.y;
			d2.x = d.x + size - 1; d2.y = s.y - size + 1;
		}
	} else {  // travelling west
		if (d.y > s.y) {  // sw
			d1.x = d.x; d1.y = s.y;
			d2.x = s.x + size - 1; d2.y = d.y + size - 1;
		} else {  // nw
			d1.x = d.x; d1.y = s.y - size + 1;
			d2.x = s.x + size - 1; d2.y = d.y;
		}
	}
}

/** The movement cost function */
class MoveCost {
private:
	const int size;				  /**< size of agent	  */
	const Field field;			 /**< field to search in */
	const AnnotatedMap *aMap;	/**< map to search on   */

public:
	MoveCost(const Unit *unit, const AnnotatedMap *aMap) 
			: size(unit->getType()->getSize()), field(unit->getCurrField()), aMap(aMap) {}
	MoveCost(const Field field, const int size, const AnnotatedMap *aMap )
			: size(size), field(field), aMap(aMap) {}

	/** The cost function @param p1 position 1 @param p2 position 2 ('adjacent' p1)
	  * @return cost of move, possibly infinite */
	float operator()(const Vec2i &p1, const Vec2i &p2) const {
		assert(p1.dist(p2) < 1.5 && p1 != p2);
		//assert(g_map.isInside(p2));
		if (!aMap->canOccupy(p2, size, field)) {
			return -1.f;
		}
		if (p1.x != p2.x && p1.y != p2.y) {
			Vec2i d1, d2;
			getDiags(p1, p2, size, d1, d2);
			//assert(g_map.isInside(d1) && g_map.isInside(d2));
			if (!aMap->canOccupy(d1, 1, field) || !aMap->canOccupy(d2, 1, field) ) {
				return -1.f;
			}
			return SQRT2;
		}
		return 1.f;
		// todo... height
	}
};

enum HAAStarResult {
	hsrFailed,
	hsrComplete,
	hsrStartTrap,
	hsrGoalTrap
};

// =====================================================
// 	class RoutePlanner
// =====================================================
/**	Finds paths for units using SearchEngine<>::aStar<>() */ 
class RoutePlanner {
public:
	RoutePlanner(World *world);
	~RoutePlanner();

	TravelState findPathToLocation(Unit *unit, const Vec2i &finalPos);

	/** @see findPathToLocation() */
	TravelState findPath(Unit *unit, const Vec2i &finalPos) { 
		return findPathToLocation(unit, finalPos); 
	}

	TravelState findPathToResource(Unit *unit, const Vec2i &targetPos, const ResourceType *rt);

	TravelState findPathToStore(Unit *unit, const Unit *store);

	TravelState findPathToBuildSite(Unit *unit, const UnitType *bType, const Vec2i &bPos, CardinalDir bFacing);

	bool isLegalMove(Unit *unit, const Vec2i &pos) const;

	SearchEngine<NodePool>* getSearchEngine() { return nsgSearchEngine; }

private:
	bool repairPath(Unit *unit);

	TravelState findAerialPath(Unit *unit, const Vec2i &targetPos);

	TravelState doRouteCache(Unit *unit);
	TravelState doQuickPathSearch(Unit *unit, const Vec2i &target);

	TravelState findPathToGoal(Unit *unit, PMap1Goal &goal, const Vec2i &targetPos);
	TravelState customGoalSearch(PMap1Goal &goal, Unit *unit, const Vec2i &target);

	float quickSearch(Field field, int Size, const Vec2i &start, const Vec2i &dest);
	bool refinePath(Unit *unit);
	void smoothPath(Unit *unit);

	HAAStarResult setupHierarchicalOpenList(Unit *unit, const Vec2i &target);
	HAAStarResult setupHierarchicalSearch(Unit *unit, const Vec2i &dest, TransitionGoal &goalFunc);
	HAAStarResult findWaypointPath(Unit *unit, const Vec2i &dest, WaypointPath &waypoints);
	HAAStarResult findWaypointPathUnExplored(Unit *unit, const Vec2i &dest, WaypointPath &waypoints);

	World *world;
	SearchEngine<NodePool>	 *nsgSearchEngine;
	NodePool *nodeStore;
	TransitionSearchEngine *tSearchEngine;
	TransitionNodeStore *tNodeStore;

	Vec2i computeNearestFreePos(const Unit *unit, const Vec2i &targetPos);

	bool attemptMove(Unit *unit) const {
		UnitPathInterface *path = unit->getPath();
		UnitPath *advPath = dynamic_cast<UnitPath *>(path);
		if(advPath == NULL) {
			throw runtime_error("Invalid or NULL unit path pointer!");
		}
		
		assert(advPath->isEmpty() == false);
		Vec2i pos = advPath->peek();
		if (isLegalMove(unit, pos)) {
			unit->setTargetPos(pos);
			advPath->pop();
			return true;
		}
		return false;
	}
#if _GAE_DEBUG_EDITION_
	TravelState doFullLowLevelAStar(Unit *unit, const Vec2i &dest);
#endif
#if DEBUG_SEARCH_TEXTURES
public:
	enum { SHOW_PATH_ONLY, SHOW_OPEN_CLOSED_SETS, SHOW_LOCAL_ANNOTATIONS } debug_texture_action;
#endif
}; // class RoutePlanner


/** Diaginal Distance Heuristic */
class DiagonalDistance {
public:
	DiagonalDistance(const Vec2i &target) : target(target) {}
	/** search target */
	Vec2i target;	
	/** The heuristic function. @param pos the position to calculate the heuristic for
	  * @return an estimate of the cost to target */
	float operator()(const Vec2i &pos) const {
		float dx = (float)abs(pos.x - target.x), 
			  dy = (float)abs(pos.y - target.y);
		float diag = dx < dy ? dx : dy;
		float straight = dx + dy - 2 * diag;
		return 1.4 * diag + straight;
	}
};

/** Goal function for 'normal' search */
class PosGoal {
private:
	Vec2i target; /**< search target */

public:
	PosGoal(const Vec2i &target) : target(target) {}
	
	/** The goal function  @param pos position to test
	  * @param costSoFar the cost of the shortest path to pos
	  * @return true if pos is target, else false	*/
	bool operator()(const Vec2i &pos, const float costSoFar) const { 
		return pos == target; 
	}
};

//TODO: put these somewhere sensible
class TransitionHeuristic {
	DiagonalDistance dd;
public:
	TransitionHeuristic(const Vec2i &target) : dd(target) {}
	bool operator()(const Transition *t) const {
		return dd(t->nwPos);
	}
};

#if 0 == 1
//
// just use DiagonalDistance to waypoint ??
class AbstractAssistedHeuristic {
public:
	AbstractAssistedHeuristic(const Vec2i &target, const Vec2i &waypoint, float wpCost) 
			: target(target), waypoint(waypoint), wpCost(wpCost) {}
	/** search target */
	Vec2i target, waypoint;	
	float wpCost;
	/** The heuristic function.
	  * @param pos the position to calculate the heuristic for
	  * @return an estimate of the cost to target
	  */
	float operator()(const Vec2i &pos) const {
		float dx = (float)abs(pos.x - waypoint.x), 
			  dy = (float)abs(pos.y - waypoint.y);
		float diag = dx < dy ? dx : dy;
		float straight = dx + dy - 2 * diag;
		return  1.4 * diag + straight + wpCost;

	}
};
#endif

}} // namespace 

#endif
