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

#ifndef _GLEST_GAME_CARTOGRAPHER_H_
#define _GLEST_GAME_CARTOGRAPHER_H_

#include "game_constants.h"
#include "influence_map.h"
#include "annotated_map.h"
#include "cluster_map.h"

#include "world.h"
#include "config.h"

#include "search_engine.h"
#include "resource_type.h"

namespace Glest { namespace Game {

using std::make_pair;

class RoutePlanner;

struct ResourceMapKey {
	const ResourceType *resourceType;
	Field workerField;
	int workerSize;

	ResourceMapKey(const ResourceType *type, Field f, int s)
			: resourceType(type), workerField(f), workerSize(s) {}

	bool operator<(const ResourceMapKey &that) const {
		return (memcmp(this, &that, sizeof(ResourceMapKey)) < 0);
	}
};

struct StoreMapKey {
	const Unit *storeUnit;
	Field workerField;
	int workerSize;

	StoreMapKey(const Unit *store, Field f, int s)
			: storeUnit(store), workerField(f), workerSize(s) {}

	bool operator<(const StoreMapKey &that) const {
		return (memcmp(this, &that, sizeof(StoreMapKey)) < 0);
	}
};

struct BuildSiteMapKey {
	const UnitType *buildingType;
	Vec2i buildingPosition;
	CardinalDir buildingFacing;
	Field workerField;
	int workerSize;

	BuildSiteMapKey(const UnitType *type, const Vec2i &pos, CardinalDir facing, Field f, int s)
			: buildingType(type), buildingPosition(pos), buildingFacing(facing)
			, workerField(f), workerSize(s) {}

	bool operator<(const BuildSiteMapKey &that) const {
		return (memcmp(this, &that, sizeof(BuildSiteMapKey)) < 0);
	}
};

//
// Cartographer: 'Map' Manager
//
class Cartographer {
private:
	/** Master annotated map, always correct */
	AnnotatedMap *masterMap;

	/** The ClusterMap (Hierarchical map abstraction) */
	ClusterMap *clusterMap;

	typedef const ResourceType* rt_ptr;
	typedef vector<Vec2i> V2iList;

	typedef map<ResourceMapKey, PatchMap<1>*> ResourceMaps;	// goal maps for harvester path searches to resourecs
	typedef map<StoreMapKey,	PatchMap<1>*> StoreMaps;	// goal maps for harvester path searches to store
	typedef map<BuildSiteMapKey,PatchMap<1>*> SiteMaps;		// goal maps for building sites.

	typedef list<pair<rt_ptr, Vec2i> >	ResourcePosList;
	typedef map<rt_ptr, V2iList> ResourcePosMap;

	// Resources
	/** The locations of each and every resource on the map */
	ResourcePosMap resourceLocations;

	set<ResourceMapKey> resourceMapKeys;
	
	/** areas where resources have been depleted and updates are required */
	ResourcePosMap resDirtyAreas;

	ResourceMaps resourceMaps;	/**< Goal Maps for each tech & tileset resource */
	StoreMaps storeMaps;		/**< goal maps for resource stores */
	SiteMaps siteMaps;			/**< goal maps for building sites */

	World *world;
	Map *cellMap;
	RoutePlanner *routePlanner;

	void initResourceMap(ResourceMapKey key, PatchMap<1> *pMap);
	void fixupResourceMaps(const ResourceType *rt, const Vec2i &pos);

	PatchMap<1>* buildAdjacencyMap(const UnitType *uType, const Vec2i &pos, CardinalDir facing, Field f, int sz);

	PatchMap<1>* buildStoreMap(StoreMapKey key) {
		return (storeMaps[key] = 
			buildAdjacencyMap(key.storeUnit->getType(), key.storeUnit->getPos(), 
			key.storeUnit->getModelFacing(), key.workerField, key.workerSize));
	}

	PatchMap<1>* buildSiteMap(BuildSiteMapKey key);

#	ifdef DEBUG_RENDERING_ENABLED
		void debugAddBuildSiteMap(PatchMap<1>*);
#	endif

public:
	Cartographer(World *world);
	virtual ~Cartographer();

	RoutePlanner* getRoutePlanner() { return routePlanner; }

	/** Update the annotated maps when an obstacle has been added or removed from the map.
	  * @param pos position (north-west most cell) of obstacle
	  * @param size size of obstacle	*/
	void updateMapMetrics(const Vec2i &pos, const int size) { 
		masterMap->updateMapMetrics(pos, size);
	}

	void onResourceDepleted(Vec2i pos, const ResourceType *rt);
	void onStoreDestroyed(Unit *unit);

	void tick();

	PatchMap<1>* getResourceMap(ResourceMapKey key);

	PatchMap<1>* getStoreMap(StoreMapKey key, bool build=true);
	PatchMap<1>* getStoreMap(const Unit *store, const Unit *worker);
	
	PatchMap<1>* getSiteMap(BuildSiteMapKey key);
	PatchMap<1>* getSiteMap(const UnitType *ut, const Vec2i &pos, CardinalDir facing, Unit *worker);

	void adjustGlestimalMap(Field f, TypeMap<float> &iMap, const Vec2i &pos, float range);
	void buildGlestimalMap(Field f, V2iList &positions);

	ClusterMap* getClusterMap() const { return clusterMap; }

	AnnotatedMap* getMasterMap()				const	{ return masterMap;							  }
	AnnotatedMap* getAnnotatedMap(int team )			{ return masterMap;/*teamMaps[team];*/					  }
	AnnotatedMap* getAnnotatedMap(const Faction *faction) 	{ return getAnnotatedMap(faction->getTeam()); }
	AnnotatedMap* getAnnotatedMap(const Unit *unit)			{ return getAnnotatedMap(unit->getTeam());	  }
};

}}

#endif
