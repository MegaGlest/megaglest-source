// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2009	James McCulloch <silnarm at gmail>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================//
// File: annotated_map.cpp
//
// Annotated Map, for use in pathfinding.
//

#include "annotated_map.h"
#include "world.h"
#include "pos_iterator.h"
#include "cartographer.h"
#include "cluster_map.h"

#include "profiler.h"

namespace Glest { namespace Game {

/** Construct AnnotatedMap object, 'g_map' must be constructed and loaded
  * @param master true if this is the master map, false for a foggy map (default true)
  */
AnnotatedMap::AnnotatedMap(World *world) 
		: world(world)
		, cellMap(NULL) {
	//_PROFILE_FUNCTION();
	assert(world && world->getMap());
	cellMap = world->getMap();
	width = cellMap->getW();
	height = cellMap->getH();
	metrics.init(width, height);
	for (int f = fLand; f < fieldCount; ++f) {
		maxClearance[f] = 0;
	}
	const int &ftCount = world->getTechTree()->getTypeCount();
	const FactionType *factionType;
	for (int i = 0; i < ftCount; ++i) {
		factionType = world->getTechTree()->getType(i);
		const UnitType *unitType;
		for (int j = 0; j < factionType->getUnitTypeCount(); ++j) {
			unitType = factionType->getUnitType(j);
			if (unitType->isMobile()) {
				if (unitType->getSize() > maxClearance[unitType->getField()]) {
					maxClearance[unitType->getField()] = unitType->getSize();
				}
			}
		}
	}
	initMapMetrics();
}

AnnotatedMap::~AnnotatedMap() {
}

/** Initialise clearance data for a master map. */
void AnnotatedMap::initMapMetrics() {
//	_PROFILE_FUNCTION();
	Util::ReverseRectIterator iter(Vec2i(0,0), Vec2i(width - 1, height - 1));
	while (iter.more()) {
		computeClearances(iter.next());
	}
}

#define LOG_CLUSTER_DIRTYING(x) {}
//#define LOG_CLUSTER_DIRTYING(x) {cout << x;}

struct MudFlinger {
	ClusterMap *cm;

	inline void setDirty(const Vec2i &pos) {
		Vec2i cluster = ClusterMap::cellToCluster(pos);
		cm->setClusterDirty(cluster);
		LOG_CLUSTER_DIRTYING( "MapMetrics changed @ pos = " << pos << endl )
		LOG_CLUSTER_DIRTYING( cout << "\tCluster = " << cluster << " dirty\n" )
		int ymod = pos.y % GameConstants::clusterSize;
		if (ymod == 0) {
			cm->setNorthBorderDirty(cluster);
			LOG_CLUSTER_DIRTYING( "\tNorth border dirty\n" )
		} else if (ymod == GameConstants::clusterSize - 1) {
			cm->setNorthBorderDirty(Vec2i(cluster.x, cluster.y + 1));
			LOG_CLUSTER_DIRTYING( "\tSouth border dirty\n" )
		}
		int xmod = pos.x % GameConstants::clusterSize;
		if (xmod == 0) {
			cm->setWestBorderDirty(cluster);
			LOG_CLUSTER_DIRTYING( "\tWest border dirty\n" )
		} else if (xmod == GameConstants::clusterSize - 1) {
			cm->setWestBorderDirty(Vec2i(cluster.x + 1, cluster.y));
			LOG_CLUSTER_DIRTYING( "\tEast border dirty\n" )
		}
	}
} mudFlinger;

/** Update clearance data, when an obstactle is placed or removed from the map	*
  * @param pos the cell co-ordinates of the obstacle added/removed				*
  * @param size the size of the obstacle										*/
void AnnotatedMap::updateMapMetrics(const Vec2i &pos, const int size) {
	assert(cellMap->isInside(pos));
	assert(cellMap->isInside(pos.x + size - 1, pos.y + size - 1));
	//_PROFILE_FUNCTION();

	// need to throw mud on the ClusterMap
	mudFlinger.cm = world->getCartographer()->getClusterMap();	

	// 1. re-evaluate the cells occupied (or formerly occupied)
	for (int i = size - 1; i >= 0 ; --i) {
		for (int j = size - 1; j >= 0; --j) {
			Vec2i occPos = pos;
			occPos.x += i; occPos.y += j;
			CellMetrics old = metrics[occPos];
			computeClearances(occPos);
			if (old != metrics[occPos]) {
				mudFlinger.setDirty(occPos);
			}
		}
	}
	// 2. propegate changes...
	cascadingUpdate(pos, size);
}

/** Perform a 'cascading update' of clearance metrics having just changed clearances				*
  * @param pos the cell co-ordinates of the obstacle added/removed									*
  * @param size the size of the obstacle															*
  * @param field the field to update (temporary), or fieldCount to update all fields (permanent)	*/
void AnnotatedMap::cascadingUpdate(const Vec2i &pos, const int size,  const Field field) {
	list<Vec2i> *leftList, *aboveList, leftList1, leftList2, aboveList1, aboveList2;
	leftList = &leftList1;
	aboveList = &aboveList1;
	// both the left and above lists need to be sorted, bigger values first (right->left, bottom->top)
	for (int i = size - 1; i >= 0; --i) {
		// Check if positions are on map, (the '+i' components are along the sides of the building/object, 
		// so we assume they are ok). If so, list them
		if (pos.x-1 >= 0) {
			leftList->push_back(Vec2i(pos.x-1,pos.y+i));
		}
		if (pos.y-1 >= 0) {
			aboveList->push_back(Vec2i(pos.x+i,pos.y-1));
		}
	}
	// the cell to the nothwest...
	Vec2i *corner = NULL;
	Vec2i cornerHolder(pos.x - 1, pos.y - 1);
	if (pos.x - 1 >= 0 && pos.y - 1 >= 0) {
		corner = &cornerHolder;
	}
	while (!leftList->empty() || !aboveList->empty() || corner) {
		// the left and above lists for the next loop iteration
		list<Vec2i> *newLeftList, *newAboveList;
		newLeftList = leftList == &leftList1 ? &leftList2 : &leftList1;
		newAboveList = aboveList == &aboveList1 ? &aboveList2 : &aboveList1;
		if (!leftList->empty()) {
			for (VLIt it = leftList->begin(); it != leftList->end(); ++it) {
				if (updateCell(*it, field) &&  it->x - 1 >= 0) { 
					// if we updated and there is a cell to the left, add it to
					newLeftList->push_back(Vec2i(it->x-1,it->y)); // the new left list
				}
			}
		}
		if (!aboveList->empty()) {
			for (VLIt it = aboveList->begin(); it != aboveList->end(); ++it) {
				if (updateCell(*it, field) && it->y - 1 >= 0) {
					newAboveList->push_back(Vec2i(it->x,it->y-1));
				}
			}
		}
		if (corner) {
			// Deal with the corner...
			if (updateCell(*corner, field)) {
				int x = corner->x, y  = corner->y;
				if (x - 1 >= 0) {
					newLeftList->push_back(Vec2i(x-1,y));
					if (y - 1 >= 0) {
						*corner = Vec2i(x-1,y-1);
					} else {
						corner = NULL;
					}
				} else {
					corner = NULL;
				}
				if (y - 1 >= 0) { 
					newAboveList->push_back(Vec2i(x,y-1));
				}
			} else {
				corner = NULL;
			}
		}
		leftList->clear(); 
		leftList = newLeftList;
		aboveList->clear(); 
		aboveList = newAboveList;
	}// end while
}

/** cascadingUpdate() helper */
bool AnnotatedMap::updateCell(const Vec2i &pos, const Field field) {
	if (field == fieldCount) { // permanent annotation, update all
		//if (eMap && !eMap->isExplored(Map::toTileCoords(pos))) { 
			// if not master map, stop if cells are unexplored
		//	return false;
		//}
		CellMetrics old = metrics[pos];
		computeClearances(pos);
		if (old != metrics[pos]) {
			mudFlinger.setDirty(pos);
			return true;
		}
	} else { // local annotation, only check field, store original clearances
		uint32 old = metrics[pos].get(field);
		if (old) {
			computeClearance(pos, field);
			if (old > metrics[pos].get(field)) {
				if (localAnnt.find(pos) == localAnnt.end()) {
					localAnnt[pos] = old; // was original clearance
				}
				return true;
			}
		}
	}
	return false;
}

/** Compute clearances (all fields) for a location
  * @param pos the cell co-ordinates 
  */
void AnnotatedMap::computeClearances(const Vec2i &pos) {
	assert(cellMap->isInside(pos));
	if (pos.x >= cellMap->getW() - 2 || pos.y >= cellMap->getH() - 2) {
		metrics[pos].setAll(0);
		return;
	}
	Cell *cell = cellMap->getCell(pos);
	// is there a building here, or an object on the tile ??
	bool surfaceBlocked = (cell->getUnit(fLand) && !cell->getUnit(fLand)->getType()->isMobile())
						|| !cellMap->getSurfaceCell(cellMap->toSurfCoords(pos))->isFree();
	// Walkable
	if (surfaceBlocked || cellMap->getDeepSubmerged(cell))
		metrics[pos].set(fLand, 0);
	else
		computeClearance(pos, fLand);
/*
	// Any Water
	if ( surfaceBlocked || !cell->isSubmerged() || !maxClearance[Field::ANY_WATER] )
		metrics[pos].set(Field::ANY_WATER, 0);
	else
		computeClearance(pos, Field::ANY_WATER);

	// Deep Water
	if ( surfaceBlocked || !cell->isDeepSubmerged() || !maxClearance[Field::DEEP_WATER] )
		metrics[pos].set(Field::DEEP_WATER, 0);
	else
		computeClearance(pos, Field::DEEP_WATER);

	// Amphibious:
	if ( surfaceBlocked || !maxClearance[Field::AMPHIBIOUS] )
		metrics[pos].set(Field::AMPHIBIOUS, 0);
	else
		computeClearance(pos, Field::AMPHIBIOUS);
*/
	// Air
	computeClearance(pos, fAir);
}

/** Computes clearance based on metrics to the south and east.
  * Does NOT check if this cell is an obstactle, assumes metrics of cells to 
  * the south, south-east & east are correct
  * @param pos the co-ordinates of the cell
  * @param field the field to update
  */
uint32 AnnotatedMap::computeClearance( const Vec2i &pos, Field f ) {
	uint32 clear = metrics[Vec2i(pos.x, pos.y + 1)].get(f);
	if ( clear > metrics[Vec2i(pos.x + 1, pos.y + 1)].get(f) ) {
		clear = metrics[Vec2i(pos.x + 1, pos.y + 1)].get(f);
	}
	if ( clear > metrics[Vec2i(pos.x + 1, pos.y)].get(f) ) {
		clear = metrics[Vec2i(pos.x + 1, pos.y)].get(f);
	}
	clear ++;
	if ( clear > maxClearance[f] )  {
		clear = maxClearance[f];
	}
	metrics[pos].set(f, clear);
	return clear;
}
/** Perform 'local annotations', annotate the map to treat other mobile units in
  * the vincinity of unit as obstacles
  * @param unit the unit about to perform a search
  * @param field the field that the unit is about to search in
  */
void AnnotatedMap::annotateLocal(const Unit *unit) {
	//_PROFILE_FUNCTION();
	const Field &field = unit->getCurrField();
	const Vec2i &pos = unit->getPos();
	const int &size = unit->getType()->getSize();
	assert(cellMap->isInside(pos));
	assert(cellMap->isInside(pos.x + size - 1, pos.y + size - 1));
	const int dist = 3;
	set<Unit*> annotate;

	// find surrounding units
	for ( int y = pos.y - dist; y < pos.y + size + dist; ++y ) {
		for ( int x = pos.x - dist; x < pos.x + size + dist; ++x ) {
			if ( cellMap->isInside(x, y) && metrics[Vec2i(x, y)].get(field) ) { // clearance != 0
				Unit *u = cellMap->getCell(x, y)->getUnit(field);
				if ( u && u != unit ) { // the set will take care of duplicates for us
					annotate.insert(u);
				}
			}
		}
	}
	// annotate map for each nearby unit
	for ( set<Unit*>::iterator it = annotate.begin(); it != annotate.end(); ++it ) {
		annotateUnit(*it, field);
	}
}

/** Temporarily annotate the map, to treat unit as an obstacle
  * @param unit the unit to treat as an obstacle
  * @param field the field to annotate
  */
void AnnotatedMap::annotateUnit(const Unit *unit, const Field field) {
	const int size = unit->getType()->getSize();
	const Vec2i &pos = unit->getPos();
	assert(cellMap->isInside(pos));
	assert(cellMap->isInside(pos.x + size - 1, pos.y + size - 1));
	// first, re-evaluate the cells occupied
	for (int i = size - 1; i >= 0 ; --i) {
		for (int j = size - 1; j >= 0; --j) {
			Vec2i occPos = pos;
			occPos.x += i; occPos.y += j;
			if (!unit->getType()->hasCellMap() || unit->getType()->getCellMapCell(i, j, unit->getModelFacing())) {
				if (localAnnt.find(occPos) == localAnnt.end()) {
					localAnnt[occPos] = metrics[occPos].get(field);
				}
				metrics[occPos].set(field, 0);
			} else {
				uint32 old =  metrics[occPos].get(field);
				computeClearance(occPos, field);
				if (old != metrics[occPos].get(field) && localAnnt.find(occPos) == localAnnt.end()) {
					localAnnt[occPos] = old;
				}
			}
		}
	}
	// propegate changes to left and above
	cascadingUpdate(pos, size, field);
}

/** Clear all local annotations								*
  * @param field the field annotations were applied to		*/
void AnnotatedMap::clearLocalAnnotations(const Unit *unit) {
	//_PROFILE_FUNCTION();
	const Field &field = unit->getCurrField();
	for ( map<Vec2i,uint32>::iterator it = localAnnt.begin(); it != localAnnt.end(); ++ it ) {
		assert(it->second <= maxClearance[field]);
		assert(cellMap->isInside(it->first));
		metrics[it->first].set(field, it->second);
	}
	localAnnt.clear();
}

#if _GAE_DEBUG_EDITION_

list<pair<Vec2i,uint32> >* AnnotatedMap::getLocalAnnotations() {
	list<pair<Vec2i,uint32> > *ret = new list<pair<Vec2i,uint32> >();
	for ( map<Vec2i,uint32>::iterator it = localAnnt.begin(); it != localAnnt.end(); ++ it )
		ret->push_back(pair<Vec2i,uint32>(it->first,metrics[it->first].get(fLand)));
	return ret;
}

#endif

}}

