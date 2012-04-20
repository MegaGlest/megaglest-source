// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2009 James McCulloch <silnarm at gmail>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================
//
// File: annotated_map.h
//
// Annotated Map, for use in pathfinding.
//

#ifndef _GLEST_GAME_ANNOTATED_MAP_H_
#define _GLEST_GAME_ANNOTATED_MAP_H_

#ifdef WIN32
    #include <winsock2.h>
    #include <winsock.h>
#endif

#include "vec.h"
#include "map.h"

namespace Glest { namespace Game {

using Shared::Platform::int64;

typedef list<Vec2i>::iterator VLIt;
typedef list<Vec2i>::const_iterator VLConIt;
typedef list<Vec2i>::reverse_iterator VLRevIt;
typedef list<Vec2i>::const_reverse_iterator VLConRevIt;

class World;

// =====================================================
// struct CellMetrics
// =====================================================
/** Stores clearance metrics for a cell.
  * 3 bits are used per field, allowing for a maximum moveable unit size of 7.
  * The left over bit is used for per team 'foggy' maps, the dirty bit is set when
  * an obstacle has been placed or removed in an area the team cannot currently see.
  * The team's annotated map is thus not updated until that cell becomes visible again.
  */
struct CellMetrics {
	CellMetrics() { memset(this, 0, sizeof(*this)); }
	
	uint16 get(const Field field) const { /**< get metrics for field */
		switch (field) {
			case fLand:		return field0;
			case fAir:		return field1;
			//case Field::ANY_WATER:	return field2;
			//case Field::DEEP_WATER:	return field3;
			//case Field::AMPHIBIOUS:	return field4;
			default: throw megaglest_runtime_error("Unknown Field passed to CellMetrics::get()");
		}
	}
	
	void set(const Field field, uint16 val) { /**< set metrics for field */
		switch (field) {
			case fLand:		field0 = val; return;
			case fAir:		field1 = val; return;
			//case Field::ANY_WATER:	field2 = val; return;
			//case Field::DEEP_WATER:	field3 = val; return;
			//case Field::AMPHIBIOUS:	field4 = val; return;
			default: throw megaglest_runtime_error("Unknown Field passed to CellMetrics::set()");
		}
	}
	
	void setAll(uint16 val)	{ /**< set clearance of all fields to val */
		field0 = field1 = field2 = field3 = field4 = val; 
	}

	bool operator!=(CellMetrics &that)	const { /**< comparison, ignoring dirty bit */
		if (field0 == that.field0 && field1 == that.field1 
		&& field2 == that.field2 && field3 == that.field3 && field4 == that.field4) {
			return false;
		}
		return true;
	}
	
	bool isDirty() const				{ return dirty; } /**< is this cell dirty */
	void setDirty(const bool val)		{ dirty = val;	} /**< set dirty flag */	

private:
	uint16 field0 : 3; /**< fLand = land + shallow water */
	uint16 field1 : 3; /**< fAir = air */
	uint16 field2 : 3; /**< Field::ANY_WATER = shallow + deep water */
	uint16 field3 : 3; /**< Field::DEEP_WATER = deep water */
	uint16 field4 : 3; /**< Field::AMPHIBIOUS = land + shallow + deep water */

	uint16  dirty : 1; /**< used in 'team' maps as a 'dirty bit' (clearances have changed
					     * but team hasn't seen that change yet). */
};

// =====================================================
// class MetricMap
// =====================================================
/** A wrapper class for the array of CellMetrics
  */
class MetricMap {
private:
	CellMetrics *metrics;
	int width,height;

public:
	MetricMap() : metrics(NULL), width(0), height(0) { }
	~MetricMap()			{ delete [] metrics; }

	void init(int w, int h) { 
		assert ( w > 0 && h > 0); 
		width = w; 
		height = h; 
		metrics = new CellMetrics[w * h];
	}

	void zero()				{ memset(metrics, 0, sizeof(CellMetrics) * width * height); }
	
	CellMetrics& operator[](const Vec2i &pos) const { return metrics[pos.y * width + pos.x]; }
};

class PathFinderTextureCallback;

// =====================================================
// class AnnotatedMap
// =====================================================
/** A 'search' map, annotated with clearance data and explored status
  * <p>A compact representation of the map with clearance information for each cell.
  * The clearance values are stored for each cell & field, and represent the 
  * clearance to the south and east of the cell. That is, the maximum size unit 
  * that can be 'positioned' in this cell (with units in Glest always using the 
  * north-west most cell they occupy as their 'position').</p>
  */
//TODO: pretty pictures for the doco...
class AnnotatedMap {
	friend class ClusterMap;
	friend class PathFinderTextureCallback;

private:
	int width, height;
	World *world;
	Map *cellMap;
	
public:
	AnnotatedMap(World *world);
	~AnnotatedMap();

	int getWidth()	{ return width;		}
	int getHeight()	{ return height;	}

	/** Maximum clearance allowed by the game. Hence, also maximum moveable unit size supported. */
	static const int maxClearanceValue = 7; // don't change me without also changing CellMetrics

	int maxClearance[fieldCount]; // maximum clearances needed for this world

	void initMapMetrics();

	void revealTile(const Vec2i &pos);

	void updateMapMetrics(const Vec2i &pos, const int size);

	/** Interface to the clearance metrics, can a unit of size occupy a cell(s) 
	  * @param pos position agent wishes to occupy
	  * @param size size of agent
	  * @param field field agent moves in
	  * @return true if position can be occupied, else false
	  */
	bool canOccupy(const Vec2i &pos, int size, Field field) const {
		assert(cellMap->isInside(pos));
		return metrics[pos].get(field) >= size ? true : false;
	}

	bool isDirty(const Vec2i &pos) const			{ return metrics[pos].isDirty(); }
	void setDirty(const Vec2i &pos, const bool val)	{ metrics[pos].setDirty(val);	}

	void annotateLocal(const Unit *unit);
	void clearLocalAnnotations(const Unit *unit);

private:
	// for initMetrics() and updateMapMetrics ()
	void computeClearances(const Vec2i &);
	uint32 computeClearance(const Vec2i &, Field);

	void cascadingUpdate(const Vec2i &pos, const int size, const Field field = fieldCount);
	void annotateUnit(const Unit *unit, const Field field);

	bool updateCell(const Vec2i &pos, const Field field);
	

	/** the original values of locations that have had local annotations applied */
	std::map<Vec2i,uint32> localAnnt;
	/** The metrics */
	MetricMap metrics;
};

}}

#endif
