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
// File: node_pool.cpp
//
#include "node_pool.h"
#include "world.h"
#include "map.h"

namespace Glest { namespace Game {

// =====================================================
// 	class NodePool
// =====================================================

NodePool::NodePool(int w, int h)
		: counter(0)
		, leastH(NULL)
		, numNodes(0)
		, tmpMaxNodes(size)
		, markerArray(w, h) {
	stock = new AStarNode[size];
}

NodePool::~NodePool() {
	delete [] stock;
}

/** reset the node pool for a new search (resets tmpMaxNodes too) */
void NodePool::reset() {
	numNodes = 0;
	counter = 0;
	tmpMaxNodes = size;
	leastH = NULL;
	markerArray.newSearch();
	openHeap.clear();
	//IF_DEBUG_EDITION( listedNodes.clear(); )
}
/** set a maximum number of nodes to expand */
void NodePool::setMaxNodes(const int max) {
	assert(max >= 32 && max <= size); // reasonable number ?
	assert(!numNodes); // can't do this after we've started using it.
	tmpMaxNodes = max;
}

/** marks an unvisited position as open
  * @param pos the position to open
  * @param prev the best known path to pos is from
  * @param h the heuristic for pos
  * @param d the costSoFar for pos
  * @return true if added, false if node limit reached		*/
bool NodePool::setOpen(const Vec2i &pos, const Vec2i &prev, float h, float d) {
	assert(!isOpen(pos));
	AStarNode *node = newNode();
	if (!node) { // NodePool exhausted
		return false;
	}
	//IF_DEBUG_EDITION( listedNodes.push_back(pos); )
	node->posOff = pos;
	if (prev.x >= 0) {
		node->posOff.ox = prev.x - pos.x;
		node->posOff.oy = prev.y - pos.y;
	} else {
		node->posOff.ox = 0;
		node->posOff.oy = 0;
	}
	node->distToHere = d;
	node->heuristic = h;
	addOpenNode(node);
	if (!numNodes || h < leastH->heuristic) {
		leastH = node;
	}
	numNodes++;
	return true;
}

/** add a new node to the open list @param node pointer to the node to add */
void NodePool::addOpenNode(AStarNode *node) {
	assert(!isOpen(node->pos()));
	markerArray.setOpen(node->pos());
	markerArray.set(node->pos(), node);
	openHeap.insert(node);
}

/** conditionally update a node on the open list. Tests if a path through a new nieghbour
  * is better than the existing known best path to pos, updates if so.
  * @param pos the open postion to test
  * @param prev the new path from
  * @param cost the cost to here from prev	*/
void NodePool::updateOpen(const Vec2i &pos, const Vec2i &prev, const float cost) {
	assert(isClosed(prev));
	assert(isOpen(pos));
	AStarNode *posNode, *prevNode;
	posNode = markerArray.get(pos);
	prevNode = markerArray.get(prev);
	if (prevNode->distToHere + cost < posNode->distToHere) {
		posNode->posOff.ox = prev.x - pos.x;
		posNode->posOff.oy = prev.y - pos.y;
		posNode->distToHere = prevNode->distToHere + cost;
		openHeap.promote(posNode);
	}
}

#if _GAE_DEBUG_EDITION_

list<Vec2i>* NodePool::getOpenNodes() {
	list<Vec2i> *ret = new list<Vec2i>();
	list<Vec2i>::iterator it = listedNodes.begin();
	for ( ; it != listedNodes.end (); ++it) {
		if (isOpen(*it)) ret->push_back(*it);
	}
	return ret;
}

list<Vec2i>* NodePool::getClosedNodes() {
	list<Vec2i> *ret = new list<Vec2i>();
	list<Vec2i>::iterator it = listedNodes.begin();
	for ( ; it != listedNodes.end(); ++it) {
		if (isClosed(*it)) ret->push_back(*it);
	}
	return ret;
}

#endif // _GAE_DEBUG_EDITION_

}}