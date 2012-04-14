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

#ifndef _GLEST_GAME_CLUSTER_MAP_H_
#define _GLEST_GAME_CLUSTER_MAP_H_

#include "util.h"
#include "game_constants.h"
#include "skill_type.h"
#include "math_util.h"

#include <map>
#include <set>
#include <list>

namespace Glest { namespace Game {

using std::set;
using std::map;
using std::list;
using Shared::Util::deleteValues;

class AnnotatedMap;
class Cartographer;
struct Transition;

/** uni-directional edge, is owned by the source transition, contains pointer to dest */
struct Edge {
private:
	static int numEdges[fieldCount];

	const Transition *dest;
	vector<float> weights;

	Field f; // for diagnostics... remove this one day

public:
	Edge(const Transition *t, Field f) : f(f) {
		dest = t;
		++numEdges[f];
	}

	~Edge() {
		--numEdges[f];
	}

	void addWeight(const float w) { weights.push_back(w); }
	const Transition* transition() const { return dest; }
	float cost(int size) const { return weights[size-1]; }
	int maxClear() { return weights.size(); }

	static int NumEdges(Field f) { return numEdges[f]; }
	static void zeroCounters();
};
typedef list<Edge*> Edges;

/**  */
struct Transition {
private:
	static int numTransitions[fieldCount];
	Field f;

public:
	int clearance;
	Vec2i nwPos;
	bool vertical;
	Edges edges;

	Transition(Vec2i pos, int clear, bool vert, Field f) 
			: f(f), clearance(clear), nwPos(pos), vertical(vert) {
		++numTransitions[f];
	}
	~Transition() {
		deleteValues(edges.begin(), edges.end());
		--numTransitions[f];
	}

	static int NumTransitions(Field f) { return numTransitions[f]; }
	static void zeroCounters();
};

typedef vector<const Transition*> Transitions;

struct TransitionCollection {
	Transition *transitions[GameConstants::clusterSize / 2];
	unsigned n;

	TransitionCollection() {
		n = 0;
		memset(transitions, 0, sizeof(Transition*) * GameConstants::clusterSize / 2);
	}

	void add(Transition *t) {
		assert(n < GameConstants::clusterSize / 2);
		transitions[n++] = t;
	}

	void clear() {
		for (int i=0; i < n; ++i) {
			delete transitions[i];
		}
		n = 0;
	}

};

struct ClusterBorder {
	TransitionCollection transitions[fieldCount];

	~ClusterBorder() {
		for (int f = 0; f < fieldCount; ++f) {
			transitions[f].clear();
		}
	}
};

/** NeighbourFunc, describes abstract search space */
class TransitionNeighbours {
public:
	void operator()(const Transition* pos, vector<const Transition*> &neighbours) const {
		for (Edges::const_iterator it = pos->edges.begin(); it != pos->edges.end(); ++it) {
			neighbours.push_back((*it)->transition());
		}
	}
};

/** cost function for searching cluster map */
class TransitionCost {
	Field field;
	int size;

public:
	TransitionCost(Field f, int s) : field(f), size(s) {}
	float operator()(const Transition *t, const Transition *t2) const {
		Edges::const_iterator it =  t->edges.begin();
		for ( ; it != t->edges.end(); ++it) {
			if ((*it)->transition() == t2) {
				break;
			}
		}
		if (it == t->edges.end()) {
			throw megaglest_runtime_error("bad connection in ClusterMap.");
		}
		if ((*it)->maxClear() >= size) {
			return (*it)->cost(size);
		}
		return -1.f;
	}
};

/** goal function for search on cluster map */
class TransitionGoal {
	set<const Transition*> goals;
public:
	TransitionGoal() {}
	set<const Transition*>& goalTransitions() {return goals;}
	bool operator()(const Transition *t, const float d) const {
		return goals.find(t) != goals.end();
	}
};

#define POS_X ((cluster.x + 1) * GameConstants::clusterSize - i - 1)
#define POS_Y ((cluster.y + 1) * GameConstants::clusterSize - i - 1)

class ClusterMap {
#if _GAE_DEBUG_EDITION_
	friend class DebugRenderer;
#endif
private:
	int w, h;
	ClusterBorder *vertBorders, *horizBorders, sentinel;
	Cartographer *carto;
	AnnotatedMap *aMap;

	set<Vec2i> dirtyClusters;
	set<Vec2i> dirtyNorthBorders;
	set<Vec2i> dirtyWestBorders;
	bool dirty;

	int eClear[GameConstants::clusterSize];

public:
	ClusterMap(AnnotatedMap *aMap, Cartographer *carto);
	~ClusterMap();

	int getWidth() const	{ return w; }
	int getHeight() const	{ return h; }

	static Vec2i cellToCluster (const Vec2i &cellPos) {
		return Vec2i(cellPos.x / GameConstants::clusterSize, cellPos.y / GameConstants::clusterSize);
	}
	// ClusterBorder getters
	ClusterBorder* getNorthBorder(Vec2i cluster) {
		return ( cluster.y == 0 ) ? &sentinel : &horizBorders[(cluster.y - 1) * w + cluster.x ];
	}
	ClusterBorder* getEastBorder(Vec2i cluster) {
		return ( cluster.x == w - 1 ) ? &sentinel : &vertBorders[cluster.y * (w - 1) + cluster.x ];
	}
	ClusterBorder* getSouthBorder(Vec2i cluster) {
		return ( cluster.y == h - 1 ) ? &sentinel : &horizBorders[cluster.y * w + cluster.x];
	}
	ClusterBorder* getWestBorder(Vec2i cluster) { 
		return ( cluster.x == 0 ) ? &sentinel : &vertBorders[cluster.y * (w - 1) + cluster.x - 1]; 
	}
	ClusterBorder* getBorder(Vec2i cluster, CardinalDir dir) {
		switch (dir) {
			case CardinalDir::NORTH:
				return getNorthBorder(cluster);
			case CardinalDir::EAST:
				return getEastBorder(cluster);
			case CardinalDir::SOUTH:
				return getSouthBorder(cluster);
			case CardinalDir::WEST:
				return getWestBorder(cluster);
			default:
				throw megaglest_runtime_error("ClusterMap::getBorder() passed dodgey direction");
		}
		return 0; // keep compiler quiet
	}
	void getTransitions(const Vec2i &cluster, Field f, Transitions &t);

	bool isDirty()	{ return dirty; }
	void update();

	void setClusterDirty(const Vec2i &cluster)		{ dirty = true; dirtyClusters.insert(cluster);		}
	void setNorthBorderDirty(const Vec2i &cluster)	{ dirty = true; dirtyNorthBorders.insert(cluster);	}
	void setWestBorderDirty(const Vec2i &cluster)	{ dirty = true; dirtyWestBorders.insert(cluster);	}

	void assertValid();

private:
	struct EntranceInfo {
		ClusterBorder *cb;
		Field f;
		bool vert;
		int pos, max_clear, startPos, endPos, run;
	};
	void addBorderTransition(EntranceInfo &info);
	void initClusterBorder(const Vec2i &cluster, bool north);

	/** initialise/re-initialise cluster (evaluates north and west borders) */
	void initCluster(const Vec2i &cluster) {
		initClusterBorder(cluster, true);
		initClusterBorder(cluster, false);
	}

	void evalCluster(const Vec2i &cluster);

	float linePathLength(Field f, int size, const Vec2i &start, const Vec2i &dest);
	float aStarPathLength(Field f, int size, const Vec2i &start, const Vec2i &dest);

	void disconnectCluster(const Vec2i &cluster);
};

struct TransitionAStarNode {
	const Transition *pos, *prev;
	float heuristic;			  /**< estimate of distance to goal		*/
	float distToHere;			 /**< cost from origin to this node	   */
	float est()	const	{		/**< estimate, costToHere + heuristic */
		return distToHere + heuristic;	
	}
};

// ========================================================
// class TransitionNodeStorage
// ========================================================
// NodeStorage template interface
class TransitionNodeStore {
private:
	list<TransitionAStarNode*> openList;
	set<const Transition*> open;
	set<const Transition*> closed;
	map<const Transition*, TransitionAStarNode*> listed;

	int size, nodeCount;
	TransitionAStarNode *stock;

	TransitionAStarNode* getNode();
	void insertIntoOpen(TransitionAStarNode *node);
	bool assertOpen();

public:
	TransitionNodeStore(int size) : size(size), stock(NULL) {
		stock = new TransitionAStarNode[size]; 
		reset();
	}
	~TransitionNodeStore() { 
		delete [] stock; 
	}

	void reset() { nodeCount = 0; open.clear(); closed.clear(); openList.clear(); listed.clear(); }
	void setMaxNodes(int limit) { }
	
	bool isOpen(const Transition* pos)		{ return open.find(pos) != open.end();		}
	bool isClosed(const Transition* pos)	{ return closed.find(pos) != closed.end();	}

	bool setOpen(const Transition* pos, const Transition* prev, float h, float d);
	void updateOpen(const Transition* pos, const Transition* &prev, const float cost);
	const Transition* getBestCandidate();
	Transition* getBestSeen();

	float getHeuristicAt(const Transition* &pos)		{ return listed[pos]->heuristic;	}
	float getCostTo(const Transition* pos)				{ return listed[pos]->distToHere;	}
	float getEstimateFor(const Transition* pos)			{ return listed[pos]->est();		}
	const Transition* getBestTo(const Transition* pos)	{ return listed[pos]->prev;			}
};


}}

#endif
