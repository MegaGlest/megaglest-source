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

#include "route_planner.h"
#include "node_pool.h"

#include "config.h"
#include "map.h"
#include "game.h"
#include "unit.h"
#include "unit_type.h"

#include "leak_dumper.h"
#include <iostream>

#define PF_DEBUG(x) {	\
	stringstream _ss;	\
	_ss << x << endl;	\
	SystemFlags::OutputDebug(SystemFlags::debugPathFinder, _ss.str().c_str());	\
}
//#define PF_DEBUG(x) std::cout << x << endl


#if _GAE_DEBUG_EDITION_
#	include "debug_renderer.h"
#endif

#ifndef PATHFINDER_DEBUG_MESSAGES
#	define PATHFINDER_DEBUG_MESSAGES 0 
#endif

#if PATHFINDER_DEBUG_MESSAGES
#	define CONSOLE_LOG(x) {g_console.addLine(x); g_logger.add(x);}
#else
#	define CONSOLE_LOG(x) {}
#endif

#define _PROFILE_PATHFINDER()
//#define _PROFILE_PATHFINDER() _PROFILE_FUNCTION()

using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest { namespace Game {

#if _GAE_DEBUG_EDITION_
	template <typename NodeStorage>
	void collectOpenClosed(NodeStorage *ns) {
		list<Vec2i> *nodes = ns->getOpenNodes();
		list<Vec2i>::iterator nit = nodes->begin();
		for ( ; nit != nodes->end(); ++nit ) 
			g_debugRenderer.getPFCallback().openSet.insert(*nit);
		delete nodes;
		nodes = ns->getClosedNodes();
		for ( nit = nodes->begin(); nit != nodes->end(); ++nit )
			g_debugRenderer.getPFCallback().closedSet.insert(*nit);
		delete nodes;					
	}

	void collectPath(const Unit *unit) {
		const UnitPath &path = *unit->getPath();
		for (UnitPath::const_iterator pit = path.begin(); pit != path.end(); ++pit) 
			g_debugRenderer.getPFCallback().pathSet.insert(*pit);
	}

	void collectWaypointPath(const Unit *unit) {
		const WaypointPath &path = *unit->getWaypointPath();
		g_debugRenderer.clearWaypoints();
		WaypointPath::const_iterator it = path.begin();
		for ( ; it != path.end(); ++it) {
			Vec3f vert = g_world.getMap()->getTile(Map::toTileCoords(*it))->getVertex();
			vert.x += it->x % GameConstants::cellScale + 0.5f;
			vert.z += it->y % GameConstants::cellScale + 0.5f;
			vert.y += 0.15f;
			g_debugRenderer.addWaypoint(vert);
		}
	}

	void clearOpenClosed(const Vec2i &start, const Vec2i &target) {
		g_debugRenderer.getPFCallback().pathStart = start;
		g_debugRenderer.getPFCallback().pathDest = target;
		g_debugRenderer.getPFCallback().pathSet.clear();
		g_debugRenderer.getPFCallback().openSet.clear();
		g_debugRenderer.getPFCallback().closedSet.clear();
	}
#endif  // _GAE_DEBUG_EDITION_

// =====================================================
// 	class RoutePlanner
// =====================================================

// ===================== PUBLIC ========================

/** Construct RoutePlanner object */
RoutePlanner::RoutePlanner(World *world)
		: world(world)
		, nsgSearchEngine(NULL)
		, nodeStore(NULL)
		, tSearchEngine(NULL)
		, tNodeStore(NULL) {
#ifdef DEBUG_SEARCH_TEXTURES
	debug_texture_action=SHOW_PATH_ONLY;
#endif
	const int &w = world->getMap()->getW();
	const int &h = world->getMap()->getH();

	nodeStore = new NodePool(w, h);
	GridNeighbours gNeighbours(w, h);
	nsgSearchEngine = new SearchEngine<NodePool>(gNeighbours, nodeStore, true);
	nsgSearchEngine->setInvalidKey(Vec2i(-1));
	nsgSearchEngine->getNeighbourFunc().setSearchSpace(ssCellMap);

	int numNodes = w * h / 4096 * 250; // 250 nodes for every 16 clusters
	tNodeStore = new TransitionNodeStore(numNodes);
	TransitionNeighbours tNeighbours;
	tSearchEngine = new TransitionSearchEngine(tNeighbours, tNodeStore, true);
	tSearchEngine->setInvalidKey(NULL);
}

/** delete SearchEngine objects */
RoutePlanner::~RoutePlanner() {
	delete nsgSearchEngine;
	delete tSearchEngine;
}

/** Determine legality of a proposed move for a unit. This function is the absolute last say
  * in such matters.
  * @param unit the unit attempting to move
  * @param pos2 the position unit desires to move to
  * @return true if move may proceed, false if not legal.
  */
bool RoutePlanner::isLegalMove(Unit *unit, const Vec2i &pos2) const {
	assert(world->getMap()->isInside(pos2));
	assert(unit->getPos().dist(pos2) < 1.5);

	float d = unit->getPos().dist(pos2);
	if (d > 1.5 || d < 0.9f) {
		// path is invalid, this shouldn't happen... but it does.
		static_cast<UnitPath*>(unit->getPath())->clear();
		unit->getWaypointPath()->clear();
		return false;
	}

	const Vec2i &pos1 = unit->getPos();
	const int &size = unit->getType()->getSize();
	const Field &field = unit->getCurrField();

	AnnotatedMap *annotatedMap = world->getCartographer()->getMasterMap();
	if (!annotatedMap->canOccupy(pos2, size, field)) {
		return false; // obstruction in field
	}
	if ( pos1.x != pos2.x && pos1.y != pos2.y ) {
		// Proposed move is diagonal, check if cells either 'side' are free.
		//  eg..  XXXX
		//        X1FX  The Cells marked 'F' must both be free
		//        XF2X  for the move 1->2 to be legit
		//        XXXX
		Vec2i diag1, diag2;
		getDiags(pos1, pos2, size, diag1, diag2);
		if (!annotatedMap->canOccupy(diag1, 1, field) || !annotatedMap->canOccupy(diag2, 1, field)
		|| !world->getMap()->getCell(diag1)->isFree(field)
		|| !world->getMap()->getCell(diag2)->isFree(field)) {
			return false; // obstruction, can not move to pos2
		}
	}
	for (int i = pos2.x; i < unit->getType()->getSize() + pos2.x; ++i) {
		for (int j = pos2.y; j < unit->getType()->getSize() + pos2.y; ++j) {
			if (world->getMap()->getCell(i,j)->getUnit(field) != unit
			&& !world->getMap()->isFreeCell(Vec2i(i, j), field)) {
				return false; // blocked by another unit
			}
		}
	}
	// pos2 is free, and nothing is in the way
	return true;
}

string log_prelude(World *w, Unit *u) {
	stringstream ss;
	ss << "Frame: " << w->getFrameCount() << ", Unit: " << u->getId() << ", ";
	return ss.str();
}

ostream& operator<<(ostream &stream, const list<Vec2i> &posList) {
	list<Vec2i>::const_iterator itBegin = posList.begin();
	list<Vec2i>::const_iterator itEnd = posList.end();
	list<Vec2i>::const_iterator it = itBegin;
	stream << " [ ";
	for ( ; it != itEnd; ++it) {
		if (it != itBegin) {
			stream << ", ";
		}
		stream << *it;
	}
	stream << " ] ";
	return stream;
}

TravelState RoutePlanner::findPathToResource(Unit *unit, const Vec2i &targetPos, const ResourceType *rt) {
	PF_TRACE();
	assert(rt && (rt->getClass() == rcTech || rt->getClass() == rcTileset));
	ResourceMapKey mapKey(rt, unit->getCurrField(), unit->getType()->getSize());
	PMap1Goal goal(world->getCartographer()->getResourceMap(mapKey));

	PF_DEBUG(log_prelude(world, unit) << "findPathToResource() targetPos: " << targetPos 
		<< ", resource type: " << rt->getName());

	return findPathToGoal(unit, goal, targetPos);
}

TravelState RoutePlanner::findPathToStore(Unit *unit, const Unit *store) {
	PF_TRACE();
	Vec2i target = store->getCenteredPos();
	PMap1Goal goal(world->getCartographer()->getStoreMap(store, unit));

	PF_DEBUG(log_prelude(world, unit) << "findPathToStore() store: " << store->getId());

	return findPathToGoal(unit, goal, target);
}

TravelState RoutePlanner::findPathToBuildSite(Unit *unit, const UnitType *bType, 
											  const Vec2i &bPos, CardinalDir bFacing) {
	PF_TRACE();
	PatchMap<1> *pMap = world->getCartographer()->getSiteMap(bType, bPos, bFacing, unit);
	PMap1Goal goal(pMap);

	PF_DEBUG(log_prelude(world, unit) << "findPathToBuildSite() " << "building type: " << bType->getName()
		<< ", building pos: " << bPos << ", building facing = " << bFacing);

	return findPathToGoal(unit, goal, bPos + Vec2i(bType->getSize() / 2));
}


float RoutePlanner::quickSearch(Field field, int size, const Vec2i &start, const Vec2i &dest) {
	PF_TRACE();
	// setup search
	MoveCost moveCost(field, size, world->getCartographer()->getMasterMap());
	DiagonalDistance heuristic(dest);
	nsgSearchEngine->setStart(start, heuristic(start));

	PosGoal goal(dest);
	AStarResult r = nsgSearchEngine->aStar(goal, moveCost, heuristic);
	if (r == asrComplete && nsgSearchEngine->getGoalPos() == dest) {
		return nsgSearchEngine->getCostTo(dest);
	}
	return -1.f;
}

HAAStarResult RoutePlanner::setupHierarchicalOpenList(Unit *unit, const Vec2i &target) {
	// get Transitions for start cluster
	Transitions transitions;
	Vec2i startCluster = ClusterMap::cellToCluster(unit->getPos());
	ClusterMap *clusterMap = world->getCartographer()->getClusterMap();
	clusterMap->getTransitions(startCluster, unit->getCurrField(), transitions);

	DiagonalDistance dd(target);
	nsgSearchEngine->getNeighbourFunc().setSearchCluster(startCluster);

	bool startTrap = true;
	// attempt quick path from unit->pos to each transition, 
	// if successful add transition to open list

	AnnotatedMap *aMap = world->getCartographer()->getMasterMap();
	aMap->annotateLocal(unit);
	for (Transitions::iterator it = transitions.begin(); it != transitions.end(); ++it) {
		float cost = quickSearch(unit->getCurrField(), unit->getType()->getSize(), unit->getPos(), (*it)->nwPos);
		if (cost != -1.f) {
			tSearchEngine->setOpen(*it, dd((*it)->nwPos), cost);
			startTrap = false;
		}
	}
	aMap->clearLocalAnnotations(unit);
	if (startTrap) {
		// do again, without annnotations, return TRAPPED if all else goes well
		bool locked = true;
		for (Transitions::iterator it = transitions.begin(); it != transitions.end(); ++it) {
			float cost = quickSearch(unit->getCurrField(), unit->getType()->getSize(), unit->getPos(), (*it)->nwPos);
			if (cost != -1.f) {
				tSearchEngine->setOpen(*it, dd((*it)->nwPos), cost);
				locked = false;
			}
		}
		if (locked) {
			return hsrFailed;
		}
		return hsrStartTrap;
	}
	return hsrComplete;
}

HAAStarResult RoutePlanner::setupHierarchicalSearch(Unit *unit, const Vec2i &dest, TransitionGoal &goalFunc) {
	PF_TRACE();

	// set-up open list
	HAAStarResult res = setupHierarchicalOpenList(unit, dest);
	if (res == hsrFailed) {
		return hsrFailed;
	}
	bool startTrap = res == hsrStartTrap;

	// transitions to goal
	Transitions transitions;
	Vec2i cluster = ClusterMap::cellToCluster(dest);
	world->getCartographer()->getClusterMap()->getTransitions(cluster, unit->getCurrField(), transitions);
	nsgSearchEngine->getNeighbourFunc().setSearchCluster(cluster);

	bool goalTrap = true;
	// attempt quick path from dest to each transition, 
	// if successful add transition to goal set
	for (Transitions::iterator it = transitions.begin(); it != transitions.end(); ++it) {
		float cost = quickSearch(unit->getCurrField(), unit->getType()->getSize(), dest, (*it)->nwPos);
		if (cost != -1.f) {
			goalFunc.goalTransitions().insert(*it);
			goalTrap = false;
		}
	}
	return startTrap ? hsrStartTrap
		  : goalTrap ? hsrGoalTrap
					 : hsrComplete;
}

HAAStarResult RoutePlanner::findWaypointPath(Unit *unit, const Vec2i &dest, WaypointPath &waypoints) {
	PF_TRACE();
	TransitionGoal goal;
	HAAStarResult setupResult = setupHierarchicalSearch(unit, dest, goal);
	nsgSearchEngine->getNeighbourFunc().setSearchSpace(ssCellMap);
	if (setupResult == hsrFailed) {
		return hsrFailed;
	}
	TransitionCost cost(unit->getCurrField(), unit->getType()->getSize());
	TransitionHeuristic heuristic(dest);
	AStarResult res = tSearchEngine->aStar(goal,cost,heuristic);
	if (res == asrComplete) {
		WaypointPath &wpPath = *unit->getWaypointPath();
		wpPath.clear();
		waypoints.push(dest);
		const Transition *t = tSearchEngine->getGoalPos();
		while (t) {
			waypoints.push(t->nwPos);
			t = tSearchEngine->getPreviousPos(t);
		}
		return setupResult;
	}
	return hsrFailed;
}

/** goal function for search on cluster map when goal position is unexplored */
class UnexploredGoal {
private:
	set<const Transition*> potentialGoals;
	int team;
	Map *map;

public:
	UnexploredGoal(Map *map, int teamIndex) : team(teamIndex), map(map) {}
	bool operator()(const Transition *t, const float d) const {
		Edges::const_iterator it =  t->edges.begin();
		for ( ; it != t->edges.end(); ++it) {
			if (!map->getSurfaceCell(Map::toSurfCoords((*it)->transition()->nwPos))->isExplored(team)) {
				// leads to unexplored area, is a potential goal
				const_cast<UnexploredGoal*>(this)->potentialGoals.insert(t);
				return false;
			}
		}
		// always 'fails', is used to build a list of possible 'avenues of exploration'
		return false;
	}

	/** returns the best 'potential goal' transition found, or null if no potential goals */
	const Transition* getBestSeen(const Vec2i &currPos, const Vec2i &target) {
		const Transition *best = 0;
		float distToBest = Shared::Graphics::infinity;//1e6f//numeric_limits<float>::infinity();
		set<const Transition*>::iterator itEnd = potentialGoals.end();
		for (set<const Transition*>::iterator it = potentialGoals.begin(); it != itEnd; ++it) {
			float myDist = (*it)->nwPos.dist(target) + (*it)->nwPos.dist(currPos);
			if (myDist < distToBest) {
				best = *it;
				distToBest = myDist;
			}
		}
		return best;
	}
};

/** cost function for searching cluster map with an unexplored target */
class UnexploredCost {
	Field field;
	int size;
	int team;
	Map *map;

public:
	UnexploredCost(Field f, int s, int team, Map *map) : field(f), size(s), team(team), map(map) {}
	float operator()(const Transition *t, const Transition *t2) const {
		Edges::const_iterator it =  t->edges.begin();
		for ( ; it != t->edges.end(); ++it) {
			if ((*it)->transition() == t2) {
				break;
			}
		}
		assert(it != t->edges.end());
		if ((*it)->maxClear() >= size 
		&& map->getSurfaceCell(Map::toSurfCoords((*it)->transition()->nwPos))->isExplored(team)) {
			return (*it)->cost(size);
		}
		return -1.f;
	}
};

HAAStarResult RoutePlanner::findWaypointPathUnExplored(Unit *unit, const Vec2i &dest, WaypointPath &waypoints) {
	// set-up open list
	HAAStarResult res = setupHierarchicalOpenList(unit, dest);
	nsgSearchEngine->getNeighbourFunc().setSearchSpace(ssCellMap);
	if (res == hsrFailed) {
		return hsrFailed;
	}
	//bool startTrap = res == hsrStartTrap;
	UnexploredGoal goal(world->getMap(), unit->getTeam());
	UnexploredCost cost(unit->getCurrField(), unit->getType()->getSize(), unit->getTeam(), world->getMap());
	TransitionHeuristic heuristic(dest);
	tSearchEngine->aStar(goal, cost, heuristic);
	const Transition *t = goal.getBestSeen(unit->getPos(), dest);
	if (!t) {
		return hsrFailed;
	}
	WaypointPath &wpPath = *unit->getWaypointPath();
	wpPath.clear();
	while (t) {
		waypoints.push(t->nwPos);
		t = tSearchEngine->getPreviousPos(t);
	}
	return res; // return setup res (in case of start trap)
}


/** refine waypoint path, extend low level path to next waypoint.
  * @return true if successful, in which case waypoint will have been popped.
  * false on failure, in which case waypoint will not be popped. */
bool RoutePlanner::refinePath(Unit *unit) {
	PF_TRACE();
	WaypointPath &wpPath = *unit->getWaypointPath();

	UnitPathInterface *unitpath = unit->getPath();
	UnitPath *advPath = dynamic_cast<UnitPath *>(unitpath);
	if(advPath == NULL) {
		throw runtime_error("Invalid or NULL unit path pointer!");
	}

	UnitPath &path = *advPath;
	assert(!wpPath.empty());

	const Vec2i &startPos = path.empty() ? unit->getPos() : path.back();
	const Vec2i &destPos = wpPath.front();
	AnnotatedMap *aMap = world->getCartographer()->getAnnotatedMap(unit);
	
	MoveCost cost(unit, aMap);
	DiagonalDistance dd(destPos);
	PosGoal posGoal(destPos);

	nsgSearchEngine->setStart(startPos, dd(startPos));
	AStarResult res = nsgSearchEngine->aStar(posGoal, cost, dd);
	if (res != asrComplete) {
		return false;
	}
//	IF_DEBUG_EDITION( collectOpenClosed<NodePool>(nsgSearchEngine->getStorage()); )
	// extract path
	Vec2i pos = nsgSearchEngine->getGoalPos();
	assert(pos == destPos);
	list<Vec2i>::iterator it = path.end();
	while (pos.x != -1) {
		it = path.insert(it, pos);
		pos = nsgSearchEngine->getPreviousPos(pos);
	}
	// erase start point (already on path or is start pos)
	it = path.erase(it);
	// pop waypoint
	wpPath.pop();
	return true;
}

#undef max

void RoutePlanner::smoothPath(Unit *unit) {
	PF_TRACE();
	UnitPathInterface *path = unit->getPath();
	UnitPath *advPath = dynamic_cast<UnitPath *>(path);
	if(advPath == NULL) {
		throw runtime_error("Invalid or NULL unit path pointer!");
	}

	if (advPath->size() < 3) {
		return;
	}
	AnnotatedMap* const &aMap = world->getCartographer()->getMasterMap();
	int min_x = 1 << 17,
		max_x = -1, 
		min_y = 1 << 17,
		max_y = -1;
	set<Vec2i> onPath;
	UnitPath::iterator it = advPath->begin();
	for ( ; it  != advPath->end(); ++it) {
		if (it->x < min_x) min_x = it->x;
		if (it->x > max_x) max_x = it->x;
		if (it->y < min_y) min_y = it->y;
		if (it->y > max_y) max_y = it->y;
		onPath.insert(*it);
	}
	Rect2i bounds(min_x, min_y, max_x + 1, max_y + 1);

	it = advPath->begin();
	UnitPath::iterator nit = it;
	++nit;

	while (nit != advPath->end()) {
		onPath.erase(*it);
		Vec2i sp = *it;
		for (int d = 0; d < odCount; ++d) {
			Vec2i p = *it + OrdinalOffsets[d];
			if (p == *nit) continue;
			Vec2i intersect(-1);
			while (bounds.isInside(p)) {
				if (!aMap->canOccupy(p, unit->getType()->getSize(), unit->getCurrField())) {
					break;
				}
				if (d % 2 == 1) { // diagonal
					Vec2i off1, off2;
					getDiags(p - OrdinalOffsets[d], p, unit->getType()->getSize(), off1, off2);
					if (!aMap->canOccupy(off1, 1, unit->getCurrField())
					|| !aMap->canOccupy(off2, 1, unit->getCurrField())) {
						break;
					}
				}
				if (onPath.find(p) != onPath.end()) {
					intersect = p;
					break;
				}
				p += OrdinalOffsets[d];
			}
			if (intersect != Vec2i(-1)) {
				UnitPath::iterator eit = nit;
				while (*eit != intersect) {
					onPath.erase(*eit++);
				}
				nit = advPath->erase(nit, eit);
				sp += OrdinalOffsets[d];
				while (sp != intersect) {
					advPath->insert(nit, sp);
					onPath.insert(sp); // do we need this? Can these get us further hits ??
					sp += OrdinalOffsets[d];
				}
				break; // break foreach direction
			} // if shortcut
		} // foreach direction
		nit = ++it;
		++nit;
	}
}

TravelState RoutePlanner::doRouteCache(Unit *unit) {
	PF_TRACE();

	UnitPathInterface *unitpath = unit->getPath();
	UnitPath *advPath = dynamic_cast<UnitPath *>(unitpath);
	if(advPath == NULL) {
		throw runtime_error("Invalid or NULL unit path pointer!");
	}

	UnitPath &path = *advPath;
	WaypointPath &wpPath = *unit->getWaypointPath();
	assert(unit->getPos().dist(path.front()) < 1.5f);
	if (attemptMove(unit)) {
		if (!wpPath.empty() && path.size() < 12) {
			// if there are less than 12 steps left on this path, and there are more waypoints
//			IF_DEBUG_EDITION( clearOpenClosed(unit->getPos(), wpPath.back()); )
			while (!wpPath.empty() && path.size() < 24) {
				// refine path to at least 24 steps (or end of path)
				if (!refinePath(unit)) {
					CONSOLE_LOG( "refinePath() failed. [route cache]" )
					wpPath.clear(); // path has become invalid
					break;
				}
			}
			smoothPath(unit);
			PF_DEBUG(log_prelude(world, unit) << " MOVING [route cahce hit] from " << unit->getPos() 
				<< " to " << unit->getTargetPos());
			PF_DEBUG(log_prelude(world, unit) << "[low-level path further refined]");
			if (!wpPath.empty()) {
				PF_DEBUG("Waypoint Path: " << wpPath);
			}
			if (!path.empty()) {
				PF_DEBUG("LowLevel Path: " << path);
			}
//			IF_DEBUG_EDITION( collectPath(unit); )
		} else {
			PF_DEBUG(log_prelude(world, unit) << " MOVING [route cahce hit] from " << unit->getPos() 
				<< " to " << unit->getTargetPos());
		}

		return tsMoving;
	}
	// path blocked, quickSearch to next waypoint...
//	IF_DEBUG_EDITION( clearOpenClosed(unit->getPos(), wpPath.empty() ? path.back() : wpPath.front()); )
	if (repairPath(unit) && attemptMove(unit)) {
//		IF_DEBUG_EDITION( collectPath(unit); )
		PF_DEBUG(log_prelude(world, unit) << " MOVING [cached path repaired] from " << unit->getPos() 
			<< " to " << unit->getTargetPos());
		if (!wpPath.empty()) {
			PF_DEBUG("Waypoint Path: " << wpPath);
		}
		if (!path.empty()) {
			PF_DEBUG("LowLevel Path: " << path);
		}
		return tsMoving;
	}
	unit->setCurrSkill(scStop);
	return tsBlocked;
}

TravelState RoutePlanner::doQuickPathSearch(Unit *unit, const Vec2i &target) {
	PF_TRACE();
	AnnotatedMap *aMap = world->getCartographer()->getAnnotatedMap(unit);

	UnitPathInterface *unitpath = unit->getPath();
	UnitPath *advPath = dynamic_cast<UnitPath *>(unitpath);
	if(advPath == NULL) {
		throw runtime_error("Invalid or NULL unit path pointer!");
	}

	UnitPath &path = *advPath;
//	IF_DEBUG_EDITION( clearOpenClosed(unit->getPos(), target); )
	aMap->annotateLocal(unit);
	float cost = quickSearch(unit->getCurrField(), unit->getType()->getSize(), unit->getPos(), target);
	aMap->clearLocalAnnotations(unit);
//	IF_DEBUG_EDITION( collectOpenClosed<NodePool>(nodeStore); )
	if (cost != -1.f) {
		Vec2i pos = nsgSearchEngine->getGoalPos();
		while (pos.x != -1) {
			path.push_front(pos);
			pos = nsgSearchEngine->getPreviousPos(pos);
		}
		if (path.size() > 1) {
			path.pop();
			if (attemptMove(unit)) {
//				IF_DEBUG_EDITION( collectPath(unit); )
				PF_DEBUG(log_prelude(world, unit) << " MOVING [doQuickPathSearch() Ok.] from " << unit->getPos() 
					<< " to " << unit->getTargetPos());
				if (!path.empty()) {
					PF_DEBUG("LowLevel Path: " << path);
				}
				return tsMoving;
			}
		}
		path.clear();
	}
	PF_DEBUG(log_prelude(world, unit) << "doQuickPathSearch() : Failed.");
	return tsBlocked;
}

TravelState RoutePlanner::findAerialPath(Unit *unit, const Vec2i &targetPos) {
	PF_TRACE();
	AnnotatedMap *aMap = world->getCartographer()->getMasterMap();
	UnitPathInterface *unitpath = unit->getPath();
	UnitPath *advPath = dynamic_cast<UnitPath *>(unitpath);
	if(advPath == NULL) {
		throw runtime_error("Invalid or NULL unit path pointer!");
	}

	UnitPath &path = *advPath;
	PosGoal goal(targetPos);
	MoveCost cost(unit, aMap);
	DiagonalDistance dd(targetPos);

	nsgSearchEngine->setNodeLimit(256);
	nsgSearchEngine->setStart(unit->getPos(), dd(unit->getPos()));

	aMap->annotateLocal(unit);
	AStarResult res = nsgSearchEngine->aStar(goal, cost, dd);
	aMap->clearLocalAnnotations(unit);
	if (res == asrComplete || res == asrNodeLimit) {
		Vec2i pos = nsgSearchEngine->getGoalPos();
		while (pos.x != -1) {
			path.push_front(pos);
			pos = nsgSearchEngine->getPreviousPos(pos);
		}
		if (path.size() > 1) {
			path.pop();
			if (attemptMove(unit)) {
				PF_DEBUG(log_prelude(world, unit) << " MOVING [aerial path search Ok.] from " << unit->getPos() 
					<< " to " << unit->getTargetPos());
				if (!path.empty()) {
					PF_DEBUG("LowLevel Path: " << path);
				}
				return tsMoving;
			}
		} else {
			path.clear();
		}
	}
	PF_DEBUG(log_prelude(world, unit) << " BLOCKED [aerial path search failed.]");
	path.incBlockCount();
	unit->setCurrSkill(scStop);
	return tsBlocked;
}

/** Find a path to a location.
  * @param unit the unit requesting the path
  * @param finalPos the position the unit desires to go to
  * @return ARRIVED, MOVING, BLOCKED or IMPOSSIBLE
  */
TravelState RoutePlanner::findPathToLocation(Unit *unit, const Vec2i &finalPos) {
	PF_TRACE();
	UnitPathInterface *unitpath = unit->getPath();
	UnitPath *advPath = dynamic_cast<UnitPath *>(unitpath);
	if(advPath == NULL) {
		throw runtime_error("Invalid or NULL unit path pointer!");
	}

	UnitPath &path = *advPath;
	WaypointPath &wpPath = *unit->getWaypointPath();

	// if arrived (where we wanted to go)
	if(finalPos == unit->getPos()) {
		unit->setCurrSkill(scStop);
		PF_DEBUG(log_prelude(world, unit) << "findPathToLocation() : ARRIVED [1]");
		return tsArrived;
	}
	// route cache
	if (!path.empty()) { 
		if (doRouteCache(unit) == tsMoving) {
			return tsMoving;
		}
	}
	// route cache miss or blocked
	const Vec2i &target = computeNearestFreePos(unit, finalPos);

	// if arrived (as close as we can get to it)
	if (target == unit->getPos()) {
		unit->setCurrSkill(scStop);
		PF_DEBUG(log_prelude(world, unit) << "findPathToLocation() : ARRIVED [2]");
		return tsArrived;
	}
	path.clear();
	wpPath.clear();

	if (unit->getCurrField() == fAir) {
		return findAerialPath(unit, target);
	}
	PF_DEBUG(log_prelude(world, unit) << "findPathToLocation() " << "target pos: " << finalPos);

	// QuickSearch if close to target
	Vec2i startCluster = ClusterMap::cellToCluster(unit->getPos());
	Vec2i destCluster  = ClusterMap::cellToCluster(target);
	if (startCluster.dist(destCluster) < 3.f) {
		if (doQuickPathSearch(unit, target) == tsMoving) {
			return tsMoving;
		}
	}
	PF_TRACE();

	// Hierarchical Search
	tSearchEngine->reset();
	Map *map = world->getMap();
	HAAStarResult res;
	if (map->getSurfaceCell(Map::toSurfCoords(target))->isExplored(unit->getTeam())) {
		res = findWaypointPath(unit, target, wpPath);
	} else {
		res = findWaypointPathUnExplored(unit, target, wpPath);
	}
	if (res == hsrFailed) {
		PF_DEBUG(log_prelude(world, unit) << "findPathToLocation() IMPOSSIBLE");
		if (unit->getFaction()->getThisFaction()) {
			world->getGame()->getConsole()->addLine(Lang::getInstance().get("InvalidPosition"));
		}
		return tsImpossible;
	} else if (res == hsrStartTrap) {
		if (wpPath.size() < 2) {
			unit->setCurrSkill(scStop);
			PF_DEBUG(log_prelude(world, unit) << "findPathToLocation() BLOCKED [START_TRAP]");
			return tsBlocked;
		}
	}
	PF_TRACE();

	//	IF_DEBUG_EDITION( collectWaypointPath(unit); )
	if (wpPath.size() > 1) {
		wpPath.pop();
	}
	assert(!wpPath.empty());

	//	IF_DEBUG_EDITION( clearOpenClosed(unit->getPos(), target); )
	// refine path, to at least 20 steps (or end of path)
	AnnotatedMap *aMap = world->getCartographer()->getMasterMap();
	aMap->annotateLocal(unit);
	wpPath.condense();
	while (!wpPath.empty() && path.size() < 20) {
		if (!refinePath(unit)) {
			aMap->clearLocalAnnotations(unit);
			path.incBlockCount();
			unit->setCurrSkill(scStop);
			PF_DEBUG(log_prelude(world, unit) << "findPathToLocation() BLOCKED [refinePath() failed]");
			return tsBlocked;
		}
	}
	smoothPath(unit);
	aMap->clearLocalAnnotations(unit);
//	IF_DEBUG_EDITION( collectPath(unit); )
	if (path.empty()) {
		PF_DEBUG(log_prelude(world, unit) << "findPathToLocation() : BLOCKED ... [post hierarchical search failure, path empty.]" );
		unit->setCurrSkill(scStop);
		return tsBlocked;
	}
	if (attemptMove(unit)) {
		PF_DEBUG(log_prelude(world, unit) << " MOVING [Hierarchical Search Ok.] from " << unit->getPos() 
			<< " to " << unit->getTargetPos());
		if (!wpPath.empty()) {
			PF_DEBUG("Waypoint Path: " << wpPath);
		}
		if (!path.empty()) {
			PF_DEBUG("LowLevel Path: " << path);
		}
		return tsMoving;
	}
	PF_DEBUG(log_prelude(world, unit) << "findPathToLocation() : BLOCKED ... [possible invalid path?]");
	if (!wpPath.empty()) {
		PF_DEBUG("Waypoint Path: " << wpPath);
	}
	if (!path.empty()) {
		PF_DEBUG("LowLevel Path: " << path);
	}
	unit->setCurrSkill(scStop);
	path.incBlockCount();
	return tsBlocked;
}

TravelState RoutePlanner::customGoalSearch(PMap1Goal &goal, Unit *unit, const Vec2i &target) {
	PF_TRACE();

	UnitPathInterface *unitpath = unit->getPath();
	UnitPath *advPath = dynamic_cast<UnitPath *>(unitpath);
	if(advPath == NULL) {
		throw runtime_error("Invalid or NULL unit path pointer!");
	}

	UnitPath &path = *advPath;
	//WaypointPath &wpPath = *unit->getWaypointPath();
	const Vec2i &start = unit->getPos();

	// setup search
	AnnotatedMap *aMap = world->getCartographer()->getMasterMap();
	MoveCost moveCost(unit->getCurrField(), unit->getType()->getSize(), aMap);
	DiagonalDistance heuristic(target);
	nsgSearchEngine->setNodeLimit(512);
	nsgSearchEngine->setStart(start, heuristic(start));

	aMap->annotateLocal(unit);
	AStarResult r = nsgSearchEngine->aStar(goal, moveCost, heuristic);
	aMap->clearLocalAnnotations(unit);

	PF_TRACE();
	if (r == asrComplete) {
		Vec2i pos = nsgSearchEngine->getGoalPos();
//		IF_DEBUG_EDITION( clearOpenClosed(unit->getPos(), pos); )
//		IF_DEBUG_EDITION( collectOpenClosed<NodePool>(nsgSearchEngine->getStorage()); )
		while (pos.x != -1) {
			path.push_front(pos);
			pos = nsgSearchEngine->getPreviousPos(pos);
		}
		if (!path.empty()) path.pop();
//		IF_DEBUG_EDITION( collectPath(unit); )
		if (!path.empty() && attemptMove(unit)) {
			return tsMoving;
		}
		path.clear();
	}
	return tsBlocked;
}

TravelState RoutePlanner::findPathToGoal(Unit *unit, PMap1Goal &goal, const Vec2i &target) {
	PF_TRACE();
	UnitPathInterface *unitpath = unit->getPath();
	UnitPath *advPath = dynamic_cast<UnitPath *>(unitpath);
	if(advPath == NULL) {
		throw runtime_error("Invalid or NULL unit path pointer!");
	}

	UnitPath &path = *advPath;
	WaypointPath &wpPath = *unit->getWaypointPath();

	// if at goal
	if (goal(unit->getPos(), 0.f)) {
		unit->setCurrSkill(scStop);
		PF_DEBUG(log_prelude(world, unit) << "ARRIVED.");
		return tsArrived;
	}
	// route chache
	if (!path.empty()) {
		if (doRouteCache(unit) == tsMoving) {
			return tsMoving;
		}
		path.clear();
		wpPath.clear();
	}
	// try customGoalSearch if close to target
	if (unit->getPos().dist(target) < 50.f) {
		if (customGoalSearch(goal, unit, target) == tsMoving) {
			PF_DEBUG(log_prelude(world, unit) << " MOVING [customGoalSearch() Ok.] from " << unit->getPos() 
				<< " to " << unit->getTargetPos());
			if (!wpPath.empty()) {
				PF_DEBUG("Waypoint Path: " << wpPath);
			}
			if (!path.empty()) {
				PF_DEBUG("LowLevel Path: " << path);
			}
			return tsMoving;
		} else {
			PF_DEBUG(log_prelude(world, unit) << "BLOCKED. [customGoalSearch() Failed.]");
			unit->setCurrSkill(scStop);
			return tsBlocked;
		}
	}

	PF_TRACE();
	// Hierarchical Search
	tSearchEngine->reset();
	if (!findWaypointPath(unit, target, wpPath)) {
		PF_DEBUG(log_prelude(world, unit) << "IMPOSSIBLE.");
		if (unit->getFaction()->getThisFaction()) {
			//console.add(lang.get("Unreachable"));
		}
		return tsImpossible;
	}
//	IF_DEBUG_EDITION( collectWaypointPath(unit); )
	assert(wpPath.size() > 1);
	wpPath.pop();
//	IF_DEBUG_EDITION( clearOpenClosed(unit->getPos(), target); )
	// cull destination and waypoints close to it, when we get to the last remaining 
	// waypoint we'll do a 'customGoalSearch' to the target
	while (wpPath.size() > 1 && wpPath.back().dist(target) < 32.f) {
		wpPath.pop_back();
	}
	// refine path, to at least 20 steps (or end of path)
	AnnotatedMap *aMap = world->getCartographer()->getMasterMap();
	aMap->annotateLocal(unit);
	while (!wpPath.empty() && path.size() < 20) {
		if (!refinePath(unit)) {
			aMap->clearLocalAnnotations(unit);
			unit->setCurrSkill(scStop);
			PF_DEBUG(log_prelude(world, unit) << "BLOCKED [refinePath() failed].");
			return tsBlocked;
		}
	}
	smoothPath(unit);
	aMap->clearLocalAnnotations(unit);
//	IF_DEBUG_EDITION( collectPath(unit); )
	if (attemptMove(unit)) {
		PF_DEBUG(log_prelude(world, unit) << " MOVING [Hierarchical Search Ok.] from " << unit->getPos() 
			<< " to " << unit->getTargetPos());
		if (!wpPath.empty()) {
			PF_DEBUG("Waypoint Path: " << wpPath);
		}
		if (!path.empty()) {
			PF_DEBUG("LowLevel Path: " << path);
		}
		return tsMoving;
	}
	PF_DEBUG(log_prelude(world, unit) << "BLOCKED [hierarchical search, possible invalid path].");
	CONSOLE_LOG( "Hierarchical refined path blocked ? valid ?!? [Custom Goal Search]" )
	if (!wpPath.empty()) {
		PF_DEBUG("Waypoint Path: " << wpPath);
	}
	if (!path.empty()) {
		PF_DEBUG("LowLevel Path: " << path);
	}
	unit->setCurrSkill(scStop);
	return tsBlocked;
}

/** repair a blocked path
  * @param unit unit whose path is blocked 
  * @return true if repair succeeded */
bool RoutePlanner::repairPath(Unit *unit) {
	PF_TRACE();
	UnitPathInterface *unitpath = unit->getPath();
	UnitPath *advPath = dynamic_cast<UnitPath *>(unitpath);
	if(advPath == NULL) {
		throw runtime_error("Invalid or NULL unit path pointer!");
	}

	UnitPath &path = *advPath;
	WaypointPath &wpPath = *unit->getWaypointPath();
	
	Vec2i dest;
	if (path.size() < 10 && !wpPath.empty()) {
		dest = wpPath.front();
	} else {
		dest = path.back();
	}
	path.clear();

	AnnotatedMap *aMap = world->getCartographer()->getAnnotatedMap(unit);
	aMap->annotateLocal(unit);
	if (quickSearch(unit->getCurrField(), unit->getType()->getSize(), unit->getPos(), dest) != -1.f) {
		Vec2i pos = nsgSearchEngine->getGoalPos();
		while (pos.x != -1) {
			path.push_front(pos);
			pos = nsgSearchEngine->getPreviousPos(pos);
		}
		if (path.size() > 2) {
			path.pop();
			if (!wpPath.empty() && wpPath.front() == path.back()) {
				wpPath.pop();
			}
		} else {
			path.clear();
		}
	}
	aMap->clearLocalAnnotations(unit);
	if (!path.empty()) {
//		IF_DEBUG_EDITION ( 
//			collectOpenClosed<NodePool>(nsgSearchEngine->getStorage()); 
//			collectPath(unit);
//		)
		return true;
	}
	return false;
}

#if _GAE_DEBUG_EDITION_

TravelState RoutePlanner::doFullLowLevelAStar(Unit *unit, const Vec2i &dest) {
	UnitPath &path = *unit->getPath();
	WaypointPath &wpPath = *unit->getWaypointPath();
	const Vec2i &target = computeNearestFreePos(unit, dest);
	// if arrived (as close as we can get to it)
	if (target == unit->getPos()) {
		unit->setCurrSkill(scStop);
		return tsArrived;
	}
	path.clear();
	wpPath.clear();

	// Low Level Search with NodeMap (this is for testing purposes only, not node limited)

	// this is the 'straight' A* using the NodeMap (no node limit)
	AnnotatedMap *aMap = g_world.getCartographer()->getAnnotatedMap(unit);
	SearchEngine<NodeMap> *se = g_world.getCartographer()->getSearchEngine();
	DiagonalDistance dd(target);
	MoveCost cost(unit, aMap);
	PosGoal goal(target);
	se->setNodeLimit(-1);
	se->setStart(unit->getPos(), dd(unit->getPos()));
	AStarResult res = se->aStar(goal,cost,dd);
	list<Vec2i>::iterator it;
	IF_DEBUG_EDITION ( 
		list<Vec2i> *nodes = NULL;
		NodeMap* nm = se->getStorage();
	)
	Vec2i goalPos, pos;
	switch (res) {
		case asrComplete:
			goalPos = se->getGoalPos();
			pos = goalPos;
			while (pos.x != -1) {
				path.push(pos);
				pos = se->getPreviousPos(pos);
			}
			if (!path.empty()) path.pop();
			IF_DEBUG_EDITION ( 
				collectOpenClosed<NodeMap>(se->getStorage());
				collectPath(unit);
			)
			break; // case asrComplete

		case AStarResult::FAILURE:
			return tsImpossible;

		default:
			throw runtime_error("Something that shouldn't have happened, did happen :(");
	}
	if (path.empty()) {
		unit->setCurrSkill(scStop);
		return tsArrived;
	}
	if (attemptMove(unit)) return tsMoving; // should always succeed (if local annotations were applied)
	unit->setCurrSkill(scStop);
	path.incBlockCount();
	return tsBlocked;
}

#endif // _GAE_DEBUG_EDITION_

// ==================== PRIVATE ====================

// return finalPos if free, else a nearest free pos within maxFreeSearchRadius
// cells, or unit's current position if none found
//
/** find nearest free position a unit can occupy 
  * @param unit the unit in question
  * @param finalPos the position unit wishes to be
  * @return finalPos if free and occupyable by unit, else the closest such position, or the unit's 
  * current position if none found
  * @todo reimplement with Dijkstra search
  */
Vec2i RoutePlanner::computeNearestFreePos(const Unit *unit, const Vec2i &finalPos) {
	PF_TRACE();
	//unit data
	Vec2i unitPos= unit->getPos();
	int size= unit->getType()->getSize();
	Field field = unit->getCurrField();
	int teamIndex= unit->getTeam();

	//if finalPos is free return it
	if (world->getMap()->isAproxFreeCells(finalPos, size, field, teamIndex)) {
		return finalPos;
	}

	//find nearest pos
	Vec2i nearestPos = unitPos;
	float nearestDist = unitPos.dist(finalPos);
	for (int i = -maxFreeSearchRadius; i <= maxFreeSearchRadius; ++i) {
		for (int j = -maxFreeSearchRadius; j <= maxFreeSearchRadius; ++j) {
			Vec2i currPos = finalPos + Vec2i(i, j);
			if (world->getMap()->isAproxFreeCells(currPos, size, field, teamIndex)) {
				float dist = currPos.dist(finalPos);

				//if nearer from finalPos
				if (dist < nearestDist) {
					nearestPos = currPos;
					nearestDist = dist;
				//if the distance is the same compare distance to unit
				} else if (dist == nearestDist) {
					if (currPos.dist(unitPos) < nearestPos.dist(unitPos)) {
						nearestPos = currPos;
					}
				}
			}
		}
	}
	return nearestPos;
}

}} // end namespace Glest::Game::Search

