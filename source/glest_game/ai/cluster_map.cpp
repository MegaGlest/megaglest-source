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

#include <algorithm>

#include "cluster_map.h"
#include "node_pool.h"
#include "route_planner.h"

#include "line.h"

using std::min;
using Shared::Util::line;

#define _USE_LINE_PATH_ 1

#if DEBUG_RENDERING_ENABLED
#	include "debug_renderer.h"
#endif

namespace Glest { namespace Game {

int Edge::numEdges[fieldCount];
int Transition::numTransitions[fieldCount];

void Edge::zeroCounters() {
	for (int f = 0; f < fieldCount; ++f) {
		numEdges[f] = 0;
	}
}

void Transition::zeroCounters() {
	for (int f = 0; f < fieldCount; ++f) {
		numTransitions[f] = 0;
	}
}

ClusterMap::ClusterMap(AnnotatedMap *aMap, Cartographer *carto) 
		: carto(carto), aMap(aMap), dirty(false) {
	//_PROFILE_FUNCTION();
	w = aMap->getWidth() / GameConstants::clusterSize;
	h = aMap->getHeight() / GameConstants::clusterSize;
	vertBorders = new ClusterBorder[(w-1)*h];
	horizBorders = new ClusterBorder[w*(h-1)];

	Edge::zeroCounters();
	Transition::zeroCounters();

	// init Borders (and hence inter-cluster edges) & evaluate clusters (intra-cluster edges)
	for (int i = h - 1; i >= 0; --i) {
		for (int j = w - 1; j >= 0; --j) {
			Vec2i cluster(j, i);
			initCluster(cluster);
			evalCluster(cluster);
			//g_logger.clusterInit();
		}
	}
}

ClusterMap::~ClusterMap() {
	delete [] vertBorders;
	delete [] horizBorders;
	for (int f = 0; f < fieldCount; ++f) {
		assert(Edge::NumEdges(Field(f)) == 0);
		assert(Transition::NumTransitions(Field(f)) == 0);
		if (Edge::NumEdges(Field(f)) != 0 || Transition::NumTransitions(Field(f)) != 0) {
			throw runtime_error("memory leak");
		}
	}
}

#define LOG(x) {}

void ClusterMap::assertValid() {
	//bool valid[fieldCount];
	bool inUse[fieldCount];
	int numNodes[fieldCount];
	int numEdges[fieldCount];

	for (int f = 0; f < fieldCount; ++f) {
		typedef set<const Transition *> TSet;
		TSet tSet;

		typedef pair<Vec2i, bool> TKey;
		typedef map<TKey, const Transition *> TKMap;

		TKMap tkMap;

		//valid[f] = true;
		numNodes[f] = 0;
		numEdges[f] = 0;
		inUse[f] = aMap->maxClearance[f] != 0;
		if (f == fAir) inUse[f] = false;
		if (!inUse[f]) {
			continue;
		}

		// collect all transitions, checking for membership in tSet and tkMap (indicating an error)
		// and filling tSet and tkMap
		for (int i=0; i < (w - 1) * h; ++i) {	// vertical borders
			ClusterBorder *b = &vertBorders[i];
			for (int j=0; j < b->transitions[f].n; ++j) {
				const Transition *t = b->transitions[f].transitions[j]; 
				if (tSet.find(t) != tSet.end()) {
					LOG("single transition on multiple borders.\n");
					//valid[f] = false;
				} else {
					tSet.insert(t);
					TKey key(t->nwPos, t->vertical);
					if (tkMap.find(key) != tkMap.end()) {
						LOG("seperate transitions of same orientation on same cell.\n");
						//valid[f] = false;
					} else {
						tkMap[key] = t;
					}
				}
				++numNodes[f];
			}
		}
		for (int i=0; i < w * (h - 1); ++i) {	// horizontal borders
			ClusterBorder *b = &horizBorders[i];
			for (int j=0; j < b->transitions[f].n; ++j) {
				const Transition *t = b->transitions[f].transitions[j]; 
				if (tSet.find(t) != tSet.end()) {
					LOG("single transition on multiple borders.\n");
					//valid[f] = false;
				} else {
					tSet.insert(t);
					TKey key(t->nwPos, t->vertical);
					if (tkMap.find(key) != tkMap.end()) {
						LOG("seperate transitions of same orientation on same cell.\n");
						//valid[f] = false;
					} else {
						tkMap[key] = t;
					}
				}
				++numNodes[f];
			}
		}
		
		// with a complete collection, iterate, check all dest transitions
		for (TSet::iterator it = tSet.begin(); it != tSet.end(); ++it) {
			const Edges &edges = (*it)->edges;
			for (Edges::const_iterator eit = edges.begin(); eit != edges.end(); ++eit) {
				TSet::iterator it2 = tSet.find((*eit)->transition());
				if (it2 == tSet.end()) {
					LOG("Invalid edge.\n");
					//valid[f] = false;
				} else {
					if (*it == *it2) {
						LOG("self referential transition.\n");
						//valid[f] = false;
					}
				}
				++numEdges[f];
			}
		}
	}
	LOG("\nClusterMap::assertValid()");
	LOG("\n=========================\n");
	for (int f = 0; f < fieldCount; ++f) {
		if (!inUse[f]) {
			LOG("Field::" << FieldNames[f] << " not in use.\n");
		} else {
			LOG("Field::" << FieldNames[f] << " in use and " << (!valid[f]? "NOT " : "") << "valid.\n");
			LOG("\t" << numNodes[f] << " transitions inspected.\n");
			LOG("\t" << numEdges[f] << " edges inspected.\n");
		}
	}
}

/** Entrance init helper class */
class InsideOutIterator {
private:
	int  centre, incr, end;
	bool flip;

public:
	InsideOutIterator(int low, int high)
			: flip(false) {
		centre = low + (high - low) / 2;
		incr = 0;
		end = ((high - low) % 2 != 0) ? low - 1 : high + 1;
	}

	int operator*() const {
		return centre + (flip ? incr : -incr);
	}

	void operator++() {
		flip = !flip;
		if (flip) ++incr;
	}

	bool more() {
		return **this != end;
	}
};

void ClusterMap::addBorderTransition(EntranceInfo &info) {
	assert(info.max_clear > 0 && info.startPos != -1 && info.endPos != -1);	
	if (info.run < 12) {
		// find central most pos with max clearance
		InsideOutIterator it(info.endPos, info.startPos);
		while (it.more()) {
			if (eClear[info.startPos - *it] == info.max_clear) {
				Transition *t;
				if (info.vert) {
					t = new Transition(Vec2i(info.pos, *it), info.max_clear, true, info.f);
				} else {
					t = new Transition(Vec2i(*it, info.pos), info.max_clear, false, info.f);
				}
				info.cb->transitions[info.f].add(t);
				return;
			}
			++it;
		}
		assert(false);
	} else {
		// look for two, as close as possible to 1/4 and 3/4 accross
		int l1 = info.endPos;
		int h1 = info.endPos + (info.startPos - info.endPos) / 2 - 1;
		int l2 = info.endPos + (info.startPos - info.endPos) / 2;
		int h2 = info.startPos;
		InsideOutIterator it(l1, h1);
		int first_at = -1;
		while (it.more()) {
			if (eClear[info.startPos - *it] == info.max_clear) {
				first_at = *it;
				break;
			}
			++it;
		}
		if (first_at != -1) {
			it = InsideOutIterator(l2, h2);
			int next_at = -1;
			while (it.more()) {
				if (eClear[info.startPos - *it] == info.max_clear) {
					next_at = *it;
					break;
				}
				++it;
			}
			if (next_at != -1) {
				Transition *t1, *t2;
				if (info.vert) {
					t1 = new Transition(Vec2i(info.pos, first_at), info.max_clear, true, info.f);
					t2 = new Transition(Vec2i(info.pos, next_at), info.max_clear, true, info.f);
				} else {
					t1 = new Transition(Vec2i(first_at, info.pos), info.max_clear, false, info.f);
					t2 = new Transition(Vec2i(next_at, info.pos), info.max_clear, false, info.f);
				}
				info.cb->transitions[info.f].add(t1);
				info.cb->transitions[info.f].add(t2);
				return;
			}
		}
		// failed to find two, just add one...
		it = InsideOutIterator(info.endPos, info.startPos);
		while (it.more()) {
			if (eClear[info.startPos - *it] == info.max_clear) {
				Transition *t;
				if (info.vert) {
					t = new Transition(Vec2i(info.pos, *it), info.max_clear, true, info.f);
				} else {
					t = new Transition(Vec2i(*it, info.pos), info.max_clear, false, info.f);
				}
				info.cb->transitions[info.f].add(t);
				return;
			}
			++it;
		}
		assert(false);
	}
}

void ClusterMap::initClusterBorder(const Vec2i &cluster, bool north) {
	//_PROFILE_FUNCTION();
	ClusterBorder *cb = north ? getNorthBorder(cluster) : getWestBorder(cluster);
	EntranceInfo inf;
	inf.cb = cb;
	inf.vert = !north;
	if (cb != &sentinel) {
		int pos = north ? cluster.y * GameConstants::clusterSize - 1
						: cluster.x * GameConstants::clusterSize - 1;
		inf.pos = pos;
		int pos2 = pos + 1;
		bool clear = false;  // true while evaluating a Transition, false when obstacle hit
		inf.max_clear = -1; // max clearance seen for current Transition
		inf.startPos = -1; // start position of entrance
		inf.endPos = -1;  // end position of entrance
		inf.run = 0;	 // to count entrance 'width'
		for (int f = 0; f < fieldCount; ++f) {
			if (!aMap->maxClearance[f] || f == fAir) continue;

#			if DEBUG_RENDERING_ENABLED
				if (f == fLand) {	
					for (int i=0; i < cb->transitions[f].n; ++i) {
						getDebugRenderer().getCMOverlay().entranceCells.erase(
							cb->transitions[f].transitions[i]->nwPos
						);
					}
				}
#			endif

			cb->transitions[f].clear();
			clear = false;
			inf.f = Field(f);
			inf.max_clear = -1;
			for (int i=0; i < GameConstants::clusterSize; ++i) {
				int clear1, clear2;
				if (north) {
					clear1 = aMap->metrics[Vec2i(POS_X,pos)].get(inf.f);
					clear2 = aMap->metrics[Vec2i(POS_X,pos2)].get(inf.f);
				} else {
					clear1 = aMap->metrics[Vec2i(pos, POS_Y)].get(Field(f));
					clear2 = aMap->metrics[Vec2i(pos2, POS_Y)].get(Field(f));
				}
				int local = min(clear1, clear2);
				if (local) {
					if (!clear) {
						clear = true;
						inf.startPos = north ? POS_X : POS_Y;
					}
					eClear[inf.run++] = local;
					inf.endPos = north ? POS_X : POS_Y;
					if (local > inf.max_clear) {
						inf.max_clear = local;
					} 
				} else {
					if (clear) {
						addBorderTransition(inf);
						inf.run = 0;
						inf.startPos = inf.endPos = inf.max_clear = -1;
						clear = false;
					}
				}
			} // for i < clusterSize
			if (clear) {
				addBorderTransition(inf);
				inf.run = 0;
				inf.startPos = inf.endPos = inf.max_clear = -1;
				clear = false;
			}
		}// for each Field

#		if DEBUG_RENDERING_ENABLED
			for (int i=0; i < cb->transitions[fLand].n; ++i) {
				getDebugRenderer().getCMOverlay().entranceCells.insert(
					cb->transitions[fLand].transitions[i]->nwPos
				);
			}
#		endif

	} // if not sentinel
}

/** function object for line alg. 'visit' */
struct Visitor {
	vector<Vec2i> &results;
	Visitor(vector<Vec2i> &res) : results(res) {}

	void operator()(int x, int y) {
		results.push_back(Vec2i(x, y));
	}
};

/** compute path length using midpoint line algorithm, @return infinite if path not possible, else cost */
float ClusterMap::linePathLength(Field f, int size, const Vec2i &start, const Vec2i &dest) {
	//_PROFILE_FUNCTION();
	if (start == dest) {
		return 0.f;
	}
	vector<Vec2i> linePath;
	Visitor visitor(linePath);
	line(start.x, start.y, dest.x, dest.y, visitor);
	assert(linePath.size() >= 2);
	MoveCost costFunc(f, size, aMap);
	vector<Vec2i>::iterator it = linePath.begin();
	vector<Vec2i>::iterator nIt = it + 1;
	float cost = 0.f;
	while (nIt != linePath.end()) {
		float add = costFunc(*it++, *nIt++);
		if (add != -1.f) {
			cost += add;
		} else {
			return -1.f;
		}
	}
	return cost;
}

/** compute path length using A* (with node limit), @return infinite if path not possible, else cost */
float ClusterMap::aStarPathLength(Field f, int size, const Vec2i &start, const Vec2i &dest) {
	//_PROFILE_FUNCTION();
	if (start == dest) {
		return 0.f;
	}
	SearchEngine<NodePool> *se = carto->getRoutePlanner()->getSearchEngine();
	MoveCost costFunc(f, size, aMap);
	DiagonalDistance dd(dest);
	se->setNodeLimit(GameConstants::clusterSize * GameConstants::clusterSize);
	se->setStart(start, dd(start));
	PosGoal goal(dest);
	AStarResult res = se->aStar<PosGoal,MoveCost,DiagonalDistance>(goal, costFunc, dd);
	Vec2i goalPos = se->getGoalPos();
	if (res != asrComplete || goalPos != dest) {
		return -1.f;
	}
	return se->getCostTo(goalPos);
}

void ClusterMap::getTransitions(const Vec2i &cluster, Field f, Transitions &t) {
	ClusterBorder *b = getNorthBorder(cluster);
	for (int i=0; i < b->transitions[f].n; ++i) {
		t.push_back(b->transitions[f].transitions[i]);
	}
	b = getEastBorder(cluster);
	for (int i=0; i < b->transitions[f].n; ++i) {
		t.push_back(b->transitions[f].transitions[i]);
	}
	b = getSouthBorder(cluster);
	for (int i=0; i < b->transitions[f].n; ++i) {
		t.push_back(b->transitions[f].transitions[i]);
	}
	b = getWestBorder(cluster);
	for (int i=0; i < b->transitions[f].n; ++i) {
		t.push_back(b->transitions[f].transitions[i]);
	}
}

void ClusterMap::disconnectCluster(const Vec2i &cluster) {
	//cout << "Disconnecting cluster " << cluster << endl;
	for (int f = 0; f < fieldCount; ++f) {
		if (!aMap->maxClearance[f] || f == fAir) continue;
		Transitions t;
		getTransitions(cluster, Field(f), t);
		set<const Transition*> tset;
		for (Transitions::iterator it = t.begin(); it != t.end(); ++it) {
			tset.insert(*it);
		}
		//int del = 0;
		for (Transitions::iterator it = t.begin(); it != t.end(); ++it) {
			Transition *t = const_cast<Transition*>(*it);
			Edges::iterator eit = t->edges.begin(); 
			while (eit != t->edges.end()) {
				if (tset.find((*eit)->transition()) != tset.end()) {
					delete *eit;
					eit = t->edges.erase(eit);
					//++del;
				} else {
					++eit;
				}
			}
		}
		//cout << "\tDeleted " << del << " edges in Field = " << FieldNames[f] << ".\n";
	}
	
}

void ClusterMap::update() {
	//_PROFILE_FUNCTION();
	//cout << "ClusterMap::update()" << endl;
	for (set<Vec2i>::iterator it = dirtyNorthBorders.begin(); it != dirtyNorthBorders.end(); ++it) {
		if (it->y > 0 && it->y < h) {
			dirtyClusters.insert(Vec2i(it->x, it->y));
			dirtyClusters.insert(Vec2i(it->x, it->y - 1));
		}
	}
	for (set<Vec2i>::iterator it = dirtyWestBorders.begin(); it != dirtyWestBorders.end(); ++it) {
		if (it->x > 0 && it->x < w) {
			dirtyClusters.insert(Vec2i(it->x, it->y));
			dirtyClusters.insert(Vec2i(it->x - 1, it->y));
		}
	}
	for (set<Vec2i>::iterator it = dirtyClusters.begin(); it != dirtyClusters.end(); ++it) {
		//cout << "cluster " << *it << " dirty." << endl;
		disconnectCluster(*it);
	}
	for (set<Vec2i>::iterator it = dirtyNorthBorders.begin(); it != dirtyNorthBorders.end(); ++it) {
		//cout << "cluster " << *it << " north border dirty." << endl;
		initClusterBorder(*it, true);
	}
	for (set<Vec2i>::iterator it = dirtyWestBorders.begin(); it != dirtyWestBorders.end(); ++it) {
		//cout << "cluster " << *it << " west border dirty." << endl;
		initClusterBorder(*it, false);
	}
	for (set<Vec2i>::iterator it = dirtyClusters.begin(); it != dirtyClusters.end(); ++it) {
		evalCluster(*it);
	}
	
	dirtyClusters.clear();
	dirtyNorthBorders.clear();
	dirtyWestBorders.clear();
	dirty = false;
}


/** compute intra-cluster path lengths */
void ClusterMap::evalCluster(const Vec2i &cluster) {
	//_PROFILE_FUNCTION();
	//int linePathSuccess = 0, linePathFail = 0;
	SearchEngine<NodePool> *se = carto->getRoutePlanner()->getSearchEngine();
	se->getNeighbourFunc().setSearchCluster(cluster);
	Transitions transitions;
	for (int f = 0; f < fieldCount; ++f) {
		if (!aMap->maxClearance[f] || f == fAir) continue;
		transitions.clear();
		getTransitions(cluster, Field(f), transitions);
		Transitions::iterator it = transitions.begin();
		for ( ; it != transitions.end(); ++it) { // foreach transition
			Transition *t = const_cast<Transition*>(*it);
			Vec2i start = t->nwPos;
			Transitions::iterator it2 = transitions.begin();
			for ( ; it2 != transitions.end(); ++it2) { // foreach other transition
				const Transition* &t2 = *it2;
				if (t == t2) continue;
				Vec2i dest = t2->nwPos;
#				if _USE_LINE_PATH_
					float cost = linePathLength(Field(f), 1, start, dest);
					if (cost == -1.f) {
						cost  = aStarPathLength(Field(f), 1, start, dest);
					}
#				else
					float cost  = aStarPathLength(Field(f), 1, start, dest);
#				endif
				if (cost == -1.f) continue;
				Edge *e = new Edge(t2, Field(f));
				t->edges.push_back(e);
				e->addWeight(cost);
				int size = 2;
				int maxClear = t->clearance > t2->clearance ? t2->clearance : t->clearance;
				while (size <= maxClear) {
#					if _USE_LINE_PATH_
						cost = linePathLength(Field(f), 1, start, dest);
						if (cost == -1.f) {
							cost  = aStarPathLength(Field(f), size, start, dest);
						}
#					else
						float cost  = aStarPathLength(Field(f), size, start, dest);
#					endif
					if (cost == -1.f) {
						break;
					}
					e->addWeight(cost);
					assert(size == e->maxClear());
					++size;
				}
			} // for each other transition
		} // for each transition
	} // for each Field
	se->getNeighbourFunc().setSearchSpace(ssCellMap);
}

// ========================================================
// class TransitionNodeStorage
// ========================================================

TransitionAStarNode* TransitionNodeStore::getNode() {
	if (nodeCount == size) {
		//assert(false);
		return NULL;
	}
	return &stock[nodeCount++];
}

void TransitionNodeStore::insertIntoOpen(TransitionAStarNode *node) {
	if (openList.empty()) {
		openList.push_front(node);
		return;
	}
	list<TransitionAStarNode*>::iterator it = openList.begin();
	while (it != openList.end() && (*it)->est() <= node->est()) {
		++it;
	}
	openList.insert(it, node);
}

bool TransitionNodeStore::assertOpen() {
	if (openList.size() < 2) {
		return true;
	}
	set<const Transition*> seen;
	list<TransitionAStarNode*>::iterator it1, it2 = openList.begin();
	it1 = it2;
	seen.insert((*it1)->pos);
	for (++it2; it2 != openList.end(); ++it2) {
		if (seen.find((*it2)->pos) != seen.end()) {
			LOG("open list has cycle... that's bad.");
			return false;
		}
		seen.insert((*it2)->pos);
		if ((*it1)->est() > (*it2)->est() + 0.0001f) { // stupid inaccurate fp
			LOG("Open list corrupt: it1.est() == " << (*it1)->est()
				<< " > it2.est() == " << (*it2)->est());
			return false;
		}
	}
	set<const Transition*>::iterator it = open.begin();
	for ( ; it != open.end(); ++it) {
		if (seen.find(*it) == seen.end()) {
			LOG("node marked open not on open list.");
			return false;
		}
	}
	it = seen.begin();
	for ( ; it != seen.end(); ++it) {
		if (open.find(*it) == open.end()) {
			LOG("node on open list not marked open.");
			return false;
		}
	}
	return true;
}

Transition* TransitionNodeStore::getBestSeen() {
	assert(false); 
	return NULL;
}

bool TransitionNodeStore::setOpen(const Transition* pos, const Transition* prev, float h, float d) {
	assert(open.find(pos) == open.end());
	assert(closed.find(pos) == closed.end());
	
	TransitionAStarNode *node = getNode();
	if (!node) return false;
	node->pos = pos;
	node->prev = prev;
	node->distToHere = d;
	node->heuristic = h;
	open.insert(pos);
	insertIntoOpen(node);
	listed[pos] = node;
	
	return true;
}

void TransitionNodeStore::updateOpen(const Transition* pos, const Transition* &prev, const float cost) {
	assert(open.find(pos) != open.end());
	assert(closed.find(prev) != closed.end());

	TransitionAStarNode *prevNode = listed[prev];
	TransitionAStarNode *posNode = listed[pos];
	if (prevNode->distToHere + cost < posNode->distToHere) {
		openList.remove(posNode);
		posNode->prev = prev;
		posNode->distToHere = prevNode->distToHere + cost;
		insertIntoOpen(posNode);
	}
}

const Transition* TransitionNodeStore::getBestCandidate() {
	if (openList.empty()) return NULL;
	TransitionAStarNode *node = openList.front();
	const Transition *best = node->pos;
	openList.pop_front();
	open.erase(open.find(best));
	closed.insert(best);
	return best;
}

}}
