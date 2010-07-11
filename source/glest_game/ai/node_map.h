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

#ifndef _GLEST_GAME_ASTAR_NODE_MAP_H_
#define _GLEST_GAME_ASTAR_NODE_MAP_H_

#include "game_constants.h"
#include "vec.h"
#include "world.h"

namespace Glest { namespace Game {

using Shared::Graphics::Vec2i;
using Shared::Platform::uint16;
using Shared::Platform::uint32;

#define MAX_MAP_COORD_BITS 16
#define MAX_MAP_COORD ((1<<MAX_MAP_COORD_BITS)-1)

#pragma pack(push, 1)

/** A bit packed position (Vec2i) */
struct PackedPos {
	/** Construct a PackedPos [0,0] */
	PackedPos() : x(0), y(0) {}
	/** Construct a PackedPos [x,y] */
	PackedPos(int x, int y) : x(x), y(y) {} 
	/** Construct a PackedPos [pos.x, pos.y] */
	PackedPos(Vec2i pos) { *this = pos; }
	/** Assign from Vec2i */
	PackedPos& operator=(Vec2i pos) {
		assert(pos.x <= MAX_MAP_COORD && pos.y <= MAX_MAP_COORD); 
		if (pos.x < 0 || pos.y < 0) {
			x = MAX_MAP_COORD; y = MAX_MAP_COORD; // invalid
		} else {
			x = pos.x; y = pos.y;
		}
		return *this;
	}
	/** this packed pos as Vec2i */
	operator Vec2i() { return Vec2i(x, y); }
	/** Comparision operator */
	bool operator==(PackedPos &that) { return x == that.x && y == that.y; }
	/** is this position valid */
	bool valid() {
		if ((x == 65535) || (y == 65535)) {
			return false;
		}
		return true;
	}
	// max == MAX_MAP_COORD,
	// MAX_MAP_COORD, MAX_MAP_COORD is considered 'invalid', this is ok still on a 
	// MAX_MAP_COORD * MAX_MAP_COORD map in Glest, because the far east and south 'tiles' 
	// (2 cells) are not valid either.
	uint16 x : MAX_MAP_COORD_BITS; /**< x coordinate */
	uint16 y : MAX_MAP_COORD_BITS; /**< y coordinate */
};

/** The cell structure for the node map. Stores all the usual A* node information plus
  * information on status (unvisited/open/closed) and an 'embedded' linked list for the
  * open set. */
struct NodeMapCell {
	/** <p>Node status for this search,</p>
	  * <ul><li>mark <  NodeMap::searchCounter     => unvisited</li>
	  *     <li>mark == NodeMap::searchCounter     => open</li>
	  *		<li>mark == NodeMap::searchCounter + 1 => closed</li></ul> */
	uint32 mark; 

	/** best route to here is from, valid only if this node is closed */
	PackedPos prevNode;
	/** 'next' node in open list, valid only if this node is open */
	PackedPos nextOpen;
	/** heuristic from this cell, valid only if node visited */
	float heuristic;
	/** cost to this cell, valid only if node visited */
	float distToHere;

	/** Construct NodeMapCell */
	NodeMapCell()		{ memset(this, 0, sizeof(*this)); }

	/** the estimate function, costToHere + heuristic */
	float estimate()	{ return heuristic + distToHere; }
};

#pragma pack(pop)

/** Wrapper for the NodeMapCell array */
class NodeMapCellArray {
private:
	NodeMapCell *array;
	int stride;
public:
	NodeMapCellArray()	{ array = NULL; }
	~NodeMapCellArray() { delete [] array; }

	void init(int w, int h)	{ delete [] array; array = new NodeMapCell[w * h]; stride = w; }

	/** index by Vec2i */
	NodeMapCell& operator[] (const Vec2i &pos)		{ return array[pos.y * stride + pos.x]; }
	/** index by PackedPos */
	NodeMapCell& operator[] (const PackedPos pos)	{ return array[pos.y * stride + pos.x]; }
};

/** A NodeStorage (template interface) compliant NodeMap. Keeps a store of nodes the size of 
  * the map, and embeds the open and cosed list within.  Uses some memory, but goes fast.
  */
class NodeMap {
public:
	NodeMap(Map *map);

	// NodeStorage template interface
	//
	void reset();
	/** set a maximum number of nodes to expand */
	void setMaxNodes(int limit)		{ nodeLimit = limit > 0 ? limit : -1;				}
	/** is the node at pos open */
	bool isOpen(const Vec2i &pos)	{ return nodeMap[pos].mark == searchCounter;		}
	/** is the node at pos closed */
	bool isClosed(const Vec2i &pos)	{ return nodeMap[pos].mark == searchCounter + 1;	}

	bool setOpen(const Vec2i &pos, const Vec2i &prev, float h, float d);
	void updateOpen(const Vec2i &pos, const Vec2i &prev, const float cost);
	Vec2i getBestCandidate();
	/** get the best heuristic node seen this search */
	Vec2i getBestSeen()		{ return bestH.valid() ? Vec2i(bestH) : Vec2i(-1); }

	/** get the heuristic of the node at pos [known to be visited] */
	float getHeuristicAt(const Vec2i &pos)	{ return nodeMap[pos].heuristic;	}
	/** get the cost to the node at pos [known to be visited] */
	float getCostTo(const Vec2i &pos)		{ return nodeMap[pos].distToHere;	}
	/** get the estimate for the node at pos [known to be visited] */
	float getEstimateFor(const Vec2i &pos)	{ return nodeMap[pos].estimate();	}
	/** get the best path to the node at pos [known to be visited] */
	Vec2i getBestTo(const Vec2i &pos)		{ 
		return nodeMap[pos].prevNode.valid() ? Vec2i(nodeMap[pos].prevNode) : Vec2i(-1);
	}

private:
	/** The array of nodes */
	NodeMapCellArray nodeMap;

	Map *cellMap;

	/** The stride of the array */
	int stride;
	/** The current node expansion limit */
	int nodeLimit;
	/** A counter, with NodeMapCell::mark determines validity of nodes in current search */
	uint32	searchCounter,
		/** Number of nodes expanded this/last search */
			nodeCount;
	/** An invalid PackedPos */
	PackedPos invalidPos;
	/** The lowest heuristic node seen this/last search */
	PackedPos bestH;
	/** The top of the open list is at */
	Vec2i openTop;

	/** Debug */
	bool assertValidPath(list<Vec2i> &path);
	/** Debug */
	bool assertOpen();
	/** Debug */
	void logOpen();

#ifdef _GAE_DEBUG_EDITION_
public:
	list<Vec2i>* getOpenNodes ();
	list<Vec2i>* getClosedNodes ();
	list<Vec2i> listedNodes;
#endif

};

}}

#endif
