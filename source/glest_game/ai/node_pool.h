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
// File: node_pool.h
//

#ifndef _GLEST_GAME_PATHFINDER_NODE_POOL_H_
#define _GLEST_GAME_PATHFINDER_NODE_POOL_H_

#ifdef WIN32
    #include <winsock2.h>
    #include <winsock.h>
#endif

#include "vec.h"
#include "game_constants.h"
#include "heap.h"
#include "data_types.h"
#include <algorithm>
#include <set>
#include <list>
//#include <limits>

namespace Glest { namespace Game {

using Shared::Util::MinHeap;
using Shared::Graphics::Vec2i;
using namespace Shared::Platform;

#pragma pack(push, 4)
struct PosOff {				/**< A bit packed position (Vec2i) and offset (direction) pair		 */
	PosOff() : x(0), y(0), ox(0), oy(0) {}				 /**< Construct a PosOff [0,0]			*/
	PosOff(Vec2i pos) : ox(0),oy(0) { *this = pos; }	/**< Construct a PosOff [pos.x, pos.y] */
	PosOff(int x, int y) : x(x) ,y(y) ,ox(0), oy(0) {} /**< Construct a PosOff [x,y]		  */
	PosOff& operator=(Vec2i pos) {					  /**< Assign from Vec2i				 */
		assert(pos.x <= 8191 && pos.y <= 8191);
		x = pos.x; y = pos.y; return *this;
	}
	bool operator==(PosOff &p) const
							{ return x == p.x && y == p.y; }	 /**< compare position components only */
	Vec2i getPos() const	{ return Vec2i(x, y); }				/**< this packed pos as Vec2i		  */
	Vec2i getPrev()	const	{ return Vec2i(x + ox, y + oy); }  /**< return pos + offset				 */
	Vec2i getOffset() const	{ return Vec2i(ox, oy); }		  /**< return offset					*/
	bool hasOffset() const	{ return ox || oy; }			 /**< has an offset					   */
	bool valid() const		{ return x >= 0 && y >= 0; }	/**< is this position valid			  */

	int32  x : 14; /**< x coordinate  */
	int32  y : 14; /**< y coordinate */
	int32 ox :  2; /**< x offset	*/
	int32 oy :  2; /**< y offset   */
};
#pragma pack(pop)

// =====================================================
// struct AStarNode
// =====================================================
#pragma pack(push, 2)
struct AStarNode {			/**< A node structure for A* with NodePool					 */
	PosOff posOff;		   /**< position of this node, and direction of best path to it	*/
	float heuristic;	  /**< estimate of distance to goal							   */
	float distToHere;	 /**< cost from origin to this node							  */

	float est()	const	{ return distToHere + heuristic;}	 /**< estimate, costToHere + heuristic */
	Vec2i pos()	const	{ return posOff.getPos();		}	/**< position of this node			  */
	Vec2i prev() const	{ return posOff.getPrev();		}  /**< best path to this node is from	 */
	bool hasPrev() const{ return posOff.hasOffset();	} /**< has valid previous 'pointer'		*/

	int32 heap_ndx;
	void setHeapIndex(int ndx) { heap_ndx = ndx;  }
	int  getHeapIndex() const  { return heap_ndx; }

	bool operator<(const AStarNode &that) const {
		const float diff = est() - that.est();
		if (diff < 0.f) return true;
		if (diff > 0.f) return false;
		// tie, prefer closer to goal...
		const float h_diff = heuristic - that.heuristic;
		if (h_diff < 0.f) return true;
		if (h_diff > 0.f) return false;
		// still tied... just distinguish them somehow...
		return pos() < that.pos();
	}
}; // == 128 bits (16 bytes)
#pragma pack(pop)

// ========================================================
// class NodePool
// ========================================================
class NodePool {	/**< A NodeStorage class (template interface) for A* */
private:
	static const int size = 512;	/**< total number of AStarNodes in each pool   */
	AStarNode *stock; /**< The block of nodes */
	int counter;	 /**< current counter    */

	// =====================================================
	// struct MarkerArray
	// =====================================================
	/** An Marker & Pointer Array supporting two mark types, open and closed. */
	///@todo replace pointers with indices, interleave mark and index arrays
	struct MarkerArray {
	private:
		int stride;				/**< stride of array   */
		unsigned int counter;  /**< the counter		  */
		unsigned int *marker; /**< the mark array	 */
		AStarNode **pArray;	 /**< the pointer array	*/
	public:
		MarkerArray(int w, int h)	: stride(w), counter(0) { 
			marker = new unsigned int[w * h]; 
			memset(marker, 0, w * h * sizeof(unsigned int)); 
			pArray = new AStarNode*[w * h]; 
			memset(pArray, 0, w * h * sizeof(AStarNode*)); 
		}
		~MarkerArray() { delete [] marker; delete [] pArray; }
		inline void newSearch() { counter += 2; }
		inline void setOpen(const Vec2i &pos)	{ marker[pos.y * stride + pos.x] = counter; }
		inline void setClosed(const Vec2i &pos)	{ marker[pos.y * stride + pos.x] = counter + 1; }
		inline bool isOpen(const Vec2i &pos)	{ return marker[pos.y * stride + pos.x] == counter; }
		inline bool isClosed(const Vec2i &pos)	{ return marker[pos.y * stride + pos.x] == counter + 1;  }
		inline bool isListed(const Vec2i &pos)	{ return marker[pos.y * stride + pos.x] >= counter; } /**< @deprecated not needed? */		
		inline void setNeither(const Vec2i &pos){ marker[pos.y * stride + pos.x] = 0; } /**< @deprecated not needed? */

		inline void set(const Vec2i &pos, AStarNode *ptr) { pArray[pos.y * stride + pos.x] = ptr; }	 /**< set the pointer for pos to ptr */
		inline AStarNode* get(const Vec2i &pos) { return pArray[pos.y * stride + pos.x]; }			/**< get the pointer for pos		*/
	};

private:
	AStarNode *leastH; /**< The 'best' node seen so far this search	   */
	int numNodes;	  /**< number of nodes used so far this search	  */
	int tmpMaxNodes; /**< a temporary maximum number of nodes to use */
	
	MarkerArray markerArray;	/**< An array the size of the map, indicating node status (unvisited, open, closed) */
	MinHeap<AStarNode> openHeap;  /**< the open list, binary heap with index aware nodes */

public:
	NodePool(int w, int h);
	~NodePool();

	// NodeStorage template interface
	//
	void setMaxNodes(const int max);
	void reset();

	bool isOpen(const Vec2i &pos)	{ return markerArray.isOpen(pos);	}	/**< test if a position is open */
	bool isClosed(const Vec2i &pos) { return markerArray.isClosed(pos); }	/**< test if a position is closed */
	bool isListed(const Vec2i &pos) { return markerArray.isListed(pos); }	/**< @deprecated needed for canPathOut() */

	bool setOpen(const Vec2i &pos, const Vec2i &prev, float h, float d);
	void updateOpen(const Vec2i &pos, const Vec2i &prev, const float cost);

	/** get the best candidate from the open list, and close it.
	  * @return the lowest estimate node from the open list, or -1,-1 if open list empty */
	Vec2i getBestCandidate() {
		if (openHeap.empty()) {
			return Vec2i(-1);
		}
		AStarNode *ptr = openHeap.extract();
		markerArray.setClosed(ptr->pos());
		return ptr->pos();
	}
	/** get the best heuristic node seen this search */
	Vec2i getBestSeen()						{ return leastH->pos(); }
	/** get the heuristic of the node at pos [known to be visited] */
	float getHeuristicAt(const Vec2i &pos)	{ return markerArray.get(pos)->heuristic;	}
	/** get the cost to the node at pos [known to be visited] */
	float getCostTo(const Vec2i &pos)		{ return markerArray.get(pos)->distToHere;	}
	/** get the estimate for the node at pos [known to be visited] */
	float getEstimateFor(const Vec2i &pos)	{ return markerArray.get(pos)->est();		}
	/** get the best path to the node at pos [known to be closed] */
	Vec2i getBestTo(const Vec2i &pos)		{ 
		AStarNode *ptr = markerArray.get(pos);
		assert(ptr);
		return ptr->hasPrev() ? ptr->prev() : Vec2i(-1);
	}

private:
	void addOpenNode(AStarNode *node);
	AStarNode*	newNode()	{ return ( counter < size ? &stock[counter++] : NULL ); } 

#if _GAE_DEBUG_EDITION_
public:
	// interface to support debugging textures
	list<Vec2i>* getOpenNodes();
	list<Vec2i>* getClosedNodes();
	list<Vec2i> listedNodes;
#endif
};

}}

#endif
