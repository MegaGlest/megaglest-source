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
// search_engine.h

#ifndef _GLEST_GAME_SEARCH_ENGINE_
#define _GLEST_GAME_SEARCH_ENGINE_

#include "math_util.h"
#include "game_constants.h"

/** Home of the templated A* algorithm and some of the bits that plug into it.*/
namespace Glest { namespace Game {

using Shared::Graphics::Vec2i;

static const float SQRT2 = Shared::Graphics::sqrt2;

enum OrdinalDir {
	odNorth, odNorthEast, odEast, odSouthEast, odSouth, odSouthWest, odWest, odNorthWest, odCount
};

const Vec2i OrdinalOffsets[odCount] = {
	Vec2i( 0, -1), // n
	Vec2i( 1, -1), // ne
	Vec2i( 1,  0), // e
	Vec2i( 1,  1), // se
	Vec2i( 0,  1), // s
	Vec2i(-1,  1), // sw
	Vec2i(-1,  0), // w
	Vec2i(-1, -1)  // nw
};

/*
	// NodeStorage template interface
	//
	class NodeStorage {
	public:
		void reset();
		void setMaxNode( int limit );
		
		bool isOpen( const Vec2i &pos );
		bool isClosed( const Vec2i &pos );

		bool setOpen( const Vec2i &pos, const Vec2i &prev, float h, float d );
		void updateOpen( const Vec2i &pos, const Vec2i &prev, const float cost );
		Vec2i getBestCandidate();
		Vec2i getBestSeen();

		float getHeuristicAt( const Vec2i &pos );
		float getCostTo( const Vec2i &pos );
		float getEstimateFor( const Vec2i &pos );
		Vec2i getBestTo( const Vec2i &pos );
	};
*/

enum SearchSpace { ssCellMap, ssTileMap };

/** Neighbour function for 'octile grid map' as search domain */
class GridNeighbours {
private:
	int x, y;
	int width, height;
	const int cellWidth, cellHeight;

public:
	GridNeighbours(int cellWidth, int cellHeight)
			: x(0), y(0)
			, width(cellWidth)
			, height(cellHeight)
			, cellWidth(cellWidth)
			, cellHeight(cellHeight) {
		assert(cellWidth % GameConstants::cellScale == 0);
		assert(cellHeight % GameConstants::cellScale == 0);
	}

	void operator()(Vec2i &pos, vector<Vec2i> &neighbours) const {
		for (int i = 0; i < odCount; ++i) {
			Vec2i nPos = pos + OrdinalOffsets[i];
			if (nPos.x >= x && nPos.x < x + width && nPos.y >= y && nPos.y < y + height) {
				neighbours.push_back(nPos);
			}
		}
	}
	/** Kludge to search on Cellmap or Tilemap... templated search domain should deprecate this */
	void setSearchSpace(SearchSpace s) {
		if (s == ssCellMap) {
			x = y = 0;
			width = cellWidth;
			height = cellHeight;
		} else if (s == ssTileMap) {
			x = y = 0;
			width = cellWidth / GameConstants::cellScale;
			height= cellHeight / GameConstants::cellScale;
		}
	}

	/** more kludgy search restriction stuff... */
	void setSearchCluster(Vec2i cluster) {
		x = cluster.x * GameConstants::clusterSize - 1;
		if (x < 0) x = 0;
		y = cluster.y * GameConstants::clusterSize - 1;
		if (y < 0) y = 0;
		width = GameConstants::clusterSize + 1;
		height = GameConstants::clusterSize + 1;
	}
};

enum AStarResult {
	asrFailed, asrComplete, asrNodeLimit, asrTimeLimit
};

// ========================================================
// class SearchEngine
// ========================================================
/** Generic (template) A*
  * @param NodeStorage NodeStorage class to use, must conform to implicit interface, see elsewhere
  * @param NeighbourFunc the function to generate neighbours for a search domain
  * @param DomainKey the key type of the search domain
  */
template< typename NodeStorage, typename NeighbourFunc = GridNeighbours, typename DomainKey = Vec2i >
class SearchEngine {
private:
	NodeStorage *nodeStorage;/**< NodeStorage for this SearchEngine					   */
	DomainKey goalPos;		/**< The goal pos (the 'result') from the last A* search  */
	DomainKey invalidKey;  /**< The DomainKey value indicating an invalid 'position' */
	int expandLimit,	  /**< limit on number of nodes to expand					*/
		nodeLimit,		 /**< limit on number of nodes to use					   */
		expanded;		/**< number of nodes expanded this/last run				  */
	bool ownStore;	   /**< wether or not this SearchEngine 'owns' its storage   */
	NeighbourFunc neighbourFunc;

public:
	/** construct & initialise NodeStorage 
	  * @param store NodeStorage to use
	  * @param own wether the SearchEngine should own the storage
	  */
	SearchEngine(NeighbourFunc neighbourFunc, NodeStorage *store = 0, bool own=false) 
			: nodeStorage(store)
			, expandLimit(-1)
			, nodeLimit(-1)
			, expanded(0)
			, ownStore(own)
			, neighbourFunc(neighbourFunc) {
	}

	/** Detruct, will delete storage if ownStore was set true */
	~SearchEngine() { 
		if (ownStore) {
			delete nodeStorage;
		}
	}

	/** Attach NodeStorage
	  * @param store NodeStorage to use
	  * @param own wether the SearchEngine should own the storage
	  */
	void setStorage(NodeStorage *store, bool own=false)	{ 
		nodeStorage = store;
		ownStore = own;
	}

	/** Set the DomainKey value indicating an invalid position, this must be set before use!
	  * @param key the invalid DomainKey value
	  */
	void setInvalidKey(DomainKey key) { invalidKey = key; }

	/** @return a pointer to this engine's node storage */
	NodeStorage* getStorage() { return nodeStorage; }

	NeighbourFunc& getNeighbourFunc() { return neighbourFunc; }

	/** Reset the node storage */
	void reset() { nodeStorage->reset(); nodeStorage->setMaxNodes(nodeLimit > 0 ? nodeLimit : -1); }
	
	/** Add a position to the open set with 'd' cost to here and invalid prev (a start position)
	  * @param pos position to set as open
	  * @param h heuristc, estimate to goal from pos
	  * @param d distance or cost to node (optional, defaults to 0)
	  */
	void setOpen(DomainKey pos, float h, float d = 0.f) { 
		nodeStorage->setOpen(pos, invalidKey, h, d); 
	}
	
	/** Reset the node storage and add pos to open 
	  * @param pos position to set as open
	  * @param h heuristc, estimate to goal from pos
	  * @param d distance or cost to node (optional, defaults to 0)
	  */
	void setStart(DomainKey pos, float h, float d = 0.f) {
		nodeStorage->reset();
		if ( nodeLimit > 0 ) {
			nodeStorage->setMaxNodes(nodeLimit);
		}
		nodeStorage->setOpen(pos, invalidKey, h, d);
	}

	/** Retrieve the goal of last search, position of last goal reached */
	DomainKey getGoalPos() { return goalPos; }
	
	/** Best path to pos is from */
	DomainKey getPreviousPos(const DomainKey &pos) { return nodeStorage->getBestTo(pos); }
	
	/** limit search to use at most limit nodes */
	void setNodeLimit(int limit) { nodeLimit = limit > 0 ? limit : -1; }
	
	/** set an 'expanded nodes' limit, for a resumable search */
	void setTimeLimit(int limit) { expandLimit = limit > 0 ? limit : -1; }

	/** How many nodes were expanded last search */
	int getExpandedLastRun() { return expanded; }

	/** Retrieves cost to the node at pos (known to be visited) */
	float getCostTo(const DomainKey &pos) {
		assert(nodeStorage->isOpen(pos) || nodeStorage->isClosed(pos));
		return nodeStorage->getCostTo(pos);
	}

	/** A* Algorithm (Just the loop, does not do any setup or post-processing) 
	  * @param GoalFunc <b>template parameter</b> Goal function class
	  * @param CostFunc <b>template parameter</b> Cost function class
	  * @param Heuristic <b>template parameter</b> Heuristic function class
	  * @param goalFunc goal function object
	  * @param costFunc cost function object
	  * @param heuristic heuristic function object
	  */
	template< typename GoalFunc, typename CostFunc, typename Heuristic >
	AStarResult aStar(GoalFunc &goalFunc, CostFunc &costFunc, Heuristic &heuristic) {
		expanded = 0;
		DomainKey minPos(invalidKey);
		vector<DomainKey> neighbours;
		while (true) {
			// get best open
			minPos = nodeStorage->getBestCandidate();
			if (minPos == invalidKey) { // failure
				goalPos = invalidKey;
				return asrFailed; 
			}
			if (goalFunc(minPos, nodeStorage->getCostTo(minPos))) { // success
				goalPos = minPos;
				return asrComplete;
			}
			// expand it...
			neighbourFunc(minPos, neighbours);
			while (!neighbours.empty()) {
				DomainKey nPos = neighbours.back();
				neighbours.pop_back();
				if (nodeStorage->isClosed(nPos)) {
					continue;
				}
				float cost = costFunc(minPos, nPos);
				if (cost == -1.f) {
					continue;
				}
				if (nodeStorage->isOpen(nPos)) {
					nodeStorage->updateOpen(nPos, minPos, cost);
				} else {
					const float &costToMin = nodeStorage->getCostTo(minPos);
					if (!nodeStorage->setOpen(nPos, minPos, heuristic(nPos), costToMin + cost)) {
						goalPos = nodeStorage->getBestSeen();
						return asrNodeLimit;
					}
				}
			} 
			expanded++;
			if (expanded == expandLimit) { // run limit
				goalPos = invalidKey;
				return asrTimeLimit;
			}
		}
		return asrFailed; // impossible... just keeping the compiler from complaining
	}
};

}}

#endif
