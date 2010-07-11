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

#include "node_map.h"
#include "annotated_map.h" // iterator typedefs

namespace Glest { namespace Game {

/** Construct a NodeMap */
NodeMap::NodeMap(Map *map)
		: cellMap(map)
		, nodeLimit(-1)
		, searchCounter(1)
		, nodeCount(0)
		, openTop(-1) {
	invalidPos.x = invalidPos.y = 65535;
	assert(!invalidPos.valid());
	bestH = invalidPos;
	stride = cellMap->getW();
	nodeMap.init(cellMap->getW(), cellMap->getH());
}

/** resets the NodeMap for use */
void NodeMap::reset() {
	bestH = invalidPos;
	searchCounter += 2;
	nodeLimit = -1;
	nodeCount = 0;
	openTop = Vec2i(-1);
#if _GAE_DEBUG_EDITION_
		listedNodes.clear();
#endif
}

/** get the best candidate from the open list, and close it.
  * @return the lowest estimate node from the open list, or -1,-1 if open list empty */
Vec2i NodeMap::getBestCandidate() {
	if (openTop.x < 0) {
		return  Vec2i(-1);	// empty
	}
	assert(nodeMap[openTop].mark == searchCounter);
	nodeMap[openTop].mark++; // set pos closed
	Vec2i ret = openTop;
	// set new openTop...
	if (nodeMap[openTop].nextOpen.valid()) {
		openTop = nodeMap[openTop].nextOpen;
	} else {
		openTop.x = -1;
	}
	return ret;
}

/** marks an unvisited position as open
  * @param pos the position to open
  * @param prev the best known path to pos is from
  * @param h the heuristic for pos
  * @param d the costSoFar for pos
  * @return true if added, false if node limit reached
  */
bool NodeMap::setOpen(const Vec2i &pos, const Vec2i &prev, float h, float d) {
	assert(nodeMap[pos].mark < searchCounter);
	if (nodeCount == nodeLimit) {
		return false;
	}
	nodeMap[pos].prevNode = prev;
	nodeMap[pos].mark = searchCounter;
	nodeMap[pos].heuristic = h;
	nodeMap[pos].distToHere = d;
	float est = h + d;
	nodeCount ++;

#	if _GAE_DEBUG_EDITION_
		listedNodes.push_back ( pos );
#	endif

	if (!bestH.valid() || nodeMap[pos].heuristic < nodeMap[bestH].heuristic) {
		bestH = pos;
	}
	if (openTop.x == -1) { // top
		openTop = pos;
		nodeMap[pos].nextOpen = invalidPos;
		return true;
	}
	PackedPos prevOpen, looking = openTop;
	// find insert spot...
	while (true) {
		assert(looking.x != 255);
		assert(isOpen(looking));

		if (est < nodeMap[looking].estimate()) {
			// pos better than looking, insert before 'looking'
			nodeMap[pos].nextOpen = looking;
			if (openTop == looking) {
				openTop = pos;
			} else {
				assert(nodeMap[prevOpen].nextOpen == looking);
				assert(prevOpen.valid());
				nodeMap[prevOpen].nextOpen = pos;
			}
			break;
		} else { // est >= nodeMap[looking].estimate()
			if (nodeMap[looking].nextOpen.valid()) {
				prevOpen = looking;
				looking = nodeMap[looking].nextOpen;
			} else { // end of list
				// insert after nodeMap[looking], set nextOpen - 1
				nodeMap[looking].nextOpen = pos;
				nodeMap[pos].nextOpen = invalidPos;
				break;
			}
		}
	} // while
	return true;
}
/** conditionally update a node on the open list. Tests if a path through a new nieghbour
  * is better than the existing known best path to pos, updates if so.
  * @param pos the open postion to test
  * @param prev the new path from
  * @param d the distance to here through prev
  */
void NodeMap::updateOpen(const Vec2i &pos, const Vec2i &prev, const float d) {
	const float dist = nodeMap[prev].distToHere + d;
	if (dist < nodeMap[pos].distToHere) {
		//LOG ("Updating open node.");
		nodeMap[pos].distToHere = dist;
		nodeMap[pos].prevNode = prev;
		const float est = nodeMap[pos].estimate();

		if (pos == openTop) { // is top of open list anyway
			//LOG("was already top");
			return;
		}
		PackedPos oldNext = nodeMap[pos].nextOpen;
		if (est < nodeMap[openTop].estimate()) {
			nodeMap[pos].nextOpen = openTop;
			openTop = pos;
			PackedPos ptmp, tmp = nodeMap[pos].nextOpen;
			while (pos != tmp) {
				assert(nodeMap[tmp].nextOpen.valid());
				ptmp = tmp;
				tmp = nodeMap[tmp].nextOpen;
			}
			nodeMap[ptmp].nextOpen = oldNext;
			//LOG ( "moved to top" );
			return;
		}
		PackedPos ptmp = openTop, tmp = nodeMap[openTop].nextOpen;
		while (true) {
			if (pos == tmp) { // no move needed
				//LOG("was not moved");
				return;
			}
			if (est < nodeMap[tmp].estimate()) {
				nodeMap[pos].nextOpen = tmp;
				nodeMap[ptmp].nextOpen = pos;
				while (pos != tmp) {
					assert(nodeMap[tmp].nextOpen.valid());
					ptmp = tmp;
					tmp = nodeMap[tmp].nextOpen;
				}
				//LOG("was moved up");
				nodeMap[ptmp].nextOpen = oldNext;
				return;
			}
			ptmp = tmp;
			assert(nodeMap[tmp].nextOpen.valid());
			tmp = nodeMap[tmp].nextOpen;
		}
		throw runtime_error("SearchMap::updateOpen() called with non-open position");
	}
}

#define LOG(x) {} // {std::cout << x << endl;}

void NodeMap::logOpen() {
	if (openTop == Vec2i(-1)) {
		LOG("Open list is empty.");
		return;
	}
	static char buffer[4096];
	char *ptr = buffer;
	PackedPos p = openTop;
	while (p.valid()) {
		ptr += sprintf(ptr, "%d,%d", p.x, p.y);
		if (nodeMap[p].nextOpen.valid()) {
			ptr += sprintf(ptr, " => ");
			if (ptr - buffer > 4000) {
				sprintf(ptr, " => plus more . . .");
				break;
			}
		}
		p = nodeMap[p].nextOpen;
	}
	LOG(buffer);
}

bool NodeMap::assertOpen() {
	PackedPos cp;
	set<Vec2i> seen;
	if (openTop.x == -1) {
		return true;
	}
	// iterate over list, build 'seen' set, checking nextOpen is not there already
	cp = openTop;
	while (cp.valid()) {
		seen.insert(cp);
		if (seen.find(nodeMap[cp].nextOpen) != seen.end()) {
			LOG("BIG TIME ERROR: open list is cyclic.");
			return false;
		}
		cp = nodeMap[cp].nextOpen;
	}
	// scan entire grid, check that all nodes marked open are indeed on the open list...
	set<Vec2i> valid;
	for (int y=0; y < cellMap->getH(); ++y) {
		for (int x=0; x < cellMap->getW(); ++x) {
			Vec2i pos(x, y);
			if (isOpen(pos)) {
				if (seen.find(pos) == seen.end()) {
					LOG("ERROR: open list missing open node, or non open node claiming to be open.");
					return false;
				}
				valid.insert(pos);
			}
		}
	}
	// ... and that all nodes on the list are marked open
	for (set<Vec2i>::iterator it = seen.begin(); it != seen.end(); ++it) {
		if (valid.find(*it) == valid.end()) {
			LOG("ERROR: node on open list was not marked open");
			return false;
		}
	}
	return true;
}

bool NodeMap::assertValidPath(list<Vec2i> &path) {
	if (path.size() < 2) return true;
	VLIt it = path.begin();
	Vec2i prevPos = *it;
	for (++it; it != path.end(); ++it) {
		if (prevPos.dist(*it) < 0.99f || prevPos.dist(*it) > 1.42f) {
			return false;
		}
		prevPos = *it;
	}
	return true;
}

#if _GAE_DEBUG_EDITION_
/*
list<pair<Vec2i,uint32>>* SearchMap::getLocalAnnotations() {
	list<pair<Vec2i,uint32>> *ret = new list<pair<Vec2i,uint32>>();
	for ( map<Vec2i,uint32>::iterator it = localAnnt.begin(); it != localAnnt.end(); ++ it ) {
		ret->push_back(pair<Vec2i,uint32>(it->first,nodeMap[it->first].getClearance(Field::LAND)));
	}
	return ret;
}
*/

list<Vec2i>* NodeMap::getOpenNodes() {
	list<Vec2i> *ret = new list<Vec2i>();
	list<Vec2i>::iterator it = listedNodes.begin();
	for ( ; it != listedNodes.end(); ++it ) {
		if ( nodeMap[*it].mark == searchCounter ) ret->push_back(*it);
	}
	return ret;
}

list<Vec2i>* NodeMap::getClosedNodes() {
	list<Vec2i> *ret = new list<Vec2i>();
	list<Vec2i>::iterator it = listedNodes.begin();
	for ( ; it != listedNodes.end(); ++it ) {
		if ( nodeMap[*it].mark == searchCounter + 1 ) ret->push_back(*it);
	}
	return ret;
}

#endif // defined ( _GAE_DEBUG_EDITION_ )

}}
