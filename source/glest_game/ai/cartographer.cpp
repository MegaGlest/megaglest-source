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

#include "cartographer.h"
#include "game_constants.h"
#include "route_planner.h"

#include "pos_iterator.h"

#include "game.h"
#include "unit.h"
#include "unit_type.h"

//#include "profiler.h"
//#include "leak_dumper.h"

#include <algorithm>

#if DEBUG_RENDERING_ENABLED
#	include "debug_renderer.h"
#endif

using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest { namespace Game {

/** Construct Cartographer object. Requires game settings, factions & cell map to have been loaded.
  */
Cartographer::Cartographer(World *world)
		: world(world), cellMap(0), routePlanner(0) {
//	_PROFILE_FUNCTION();
	Logger::getInstance().add("Cartographer", true);

	cellMap = world->getMap();
	int w = cellMap->getW(), h = cellMap->getH();

	routePlanner = world->getRoutePlanner();
	masterMap = new AnnotatedMap(world);	
	clusterMap = new ClusterMap(masterMap, this);

	const TechTree *tt = world->getTechTree();
	vector<rt_ptr> harvestResourceTypes;
	for (int i = 0; i < tt->getResourceTypeCount(); ++i) {
		rt_ptr rt = tt->getResourceType(i);
		if (rt->getClass() == rcTech || rt->getClass() == rcTileset) {
			harvestResourceTypes.push_back(rt);
		}
	}
	for (int i = 0; i < tt->getTypeCount(); ++i) {
		const FactionType *ft = tt->getType(i);
		for (int j = 0; j < ft->getUnitTypeCount(); ++j) {
			const UnitType *ut = ft->getUnitType(j);
			for (int k=0; k < ut->getCommandTypeCount(); ++k) {
				const CommandType *ct = ut->getCommandType(k);
				if (ct->getClass() == ccHarvest) {
					const HarvestCommandType *hct = static_cast<const HarvestCommandType *>(ct);
					for (vector<rt_ptr>::iterator it = harvestResourceTypes.begin();
							it != harvestResourceTypes.end(); ++it) {
						if (hct->canHarvest(*it)) {
							ResourceMapKey key(*it, ut->getField(), ut->getSize());
							resourceMapKeys.insert(key);
						}
					}
				}
			}
		}
	}

	// find and catalog all resources...
	for (int x=0; x < cellMap->getSurfaceW() - 1; ++x) {
		for (int y=0; y < cellMap->getSurfaceH() - 1; ++y) {
			const Resource * const r = cellMap->getSurfaceCell(x,y)->getResource();
			if (r) {
				resourceLocations[r->getType()].push_back(Vec2i(x,y));
			}
		}
	}

	Rectangle rect(0, 0, cellMap->getW() - 3, cellMap->getH() - 3);
	for (set<ResourceMapKey>::iterator it = resourceMapKeys.begin(); it != resourceMapKeys.end(); ++it) {
		PatchMap<1> *pMap = new PatchMap<1>(rect, 0);
		initResourceMap(*it, pMap);
		resourceMaps[*it] = pMap;
	}
}

/** Destruct */
Cartographer::~Cartographer() {
	delete masterMap;
	delete clusterMap;

	// Goal Maps
	deleteMapValues(resourceMaps.begin(), resourceMaps.end());
	resourceMaps.clear();
	deleteMapValues(storeMaps.begin(), storeMaps.end());
	storeMaps.clear();
	deleteMapValues(siteMaps.begin(), siteMaps.end());
	siteMaps.clear();
}

void Cartographer::initResourceMap(ResourceMapKey key, PatchMap<1> *pMap) {
	const int &size = key.workerSize;
	const Field &field = key.workerField;
	const Map &map = *world->getMap();
	pMap->zeroMap();
	for (vector<Vec2i>::iterator it = resourceLocations[key.resourceType].begin();
			it != resourceLocations[key.resourceType].end(); ++it) {
		Resource *r = world->getMap()->getSurfaceCell(*it)->getResource();
		assert(r);

		Vec2i tl = *it * GameConstants::cellScale + OrdinalOffsets[odNorthWest] * size;
		Vec2i br(tl.x + size + 2, tl.y + size + 2);

		Util::PerimeterIterator iter(tl, br);
		while (iter.more()) {
			Vec2i pos = iter.next();
			if (map.isInside(pos) && masterMap->canOccupy(pos, size, field)) {
				pMap->setInfluence(pos, 1);
			}
		}
	}
}

void Cartographer::onResourceDepleted(Vec2i pos, const ResourceType *rt) {
	PF_TRACE();
	pos = Map::toUnitCoords(pos);
	updateMapMetrics(pos, GameConstants::cellScale);
	resDirtyAreas[rt].push_back(pos);
}

void Cartographer::fixupResourceMaps(const ResourceType *rt, const Vec2i &pos) {
	PF_TRACE();
	const Map &map = *world->getMap();
	Vec2i junk;
	for (set<ResourceMapKey>::iterator it = resourceMapKeys.begin(); it != resourceMapKeys.end(); ++it) {
		if (it->resourceType == rt) {
			PatchMap<1> *pMap = resourceMaps[*it];
			const int &size = it->workerSize;
			const Field &field = it->workerField;

			Vec2i tl = pos + OrdinalOffsets[odNorthWest] * size;
			Vec2i br(tl.x + size + 2, tl.y + size + 2);

			Util::RectIterator iter(tl, br);
			while (iter.more()) {
				Vec2i cur = iter.next();
				if (map.isInside(cur) && masterMap->canOccupy(cur, size, field)
				&& map.isResourceNear(cur, size, rt, junk)) {
					pMap->setInfluence(cur, 1);
				} else {
					pMap->setInfluence(cur, 0);
				}
			}
		}
	}
}

PatchMap<1>* Cartographer::buildSiteMap(BuildSiteMapKey key) {
	PF_TRACE();
	PatchMap<1> *sMap = siteMaps[key] = buildAdjacencyMap(key.buildingType, key.buildingPosition,
		key.buildingFacing, key.workerField, key.workerSize);
#	ifdef DEBUG_RENDERING_ENABLED
		debugAddBuildSiteMap(sMap);
#	endif
	return sMap;
}

PatchMap<1>* Cartographer::getResourceMap(ResourceMapKey key) {
	PF_TRACE();
	return resourceMaps[key];
}

PatchMap<1>* Cartographer::getStoreMap(StoreMapKey key, bool build) {
	PF_TRACE();
	StoreMaps::iterator it = storeMaps.find(key);
	if (it != storeMaps.end()) {
		return it->second;
	}
	if (build) {
		return buildStoreMap(key);
	} else {
		return 0;
	}
}

PatchMap<1>* Cartographer::getStoreMap(const Unit *store, const Unit *worker) {
	PF_TRACE();
	StoreMapKey key(store, worker->getCurrField(), worker->getType()->getSize());
	return getStoreMap(key);
}

PatchMap<1>* Cartographer::getSiteMap(BuildSiteMapKey key) {
	PF_TRACE();
	SiteMaps::iterator it = siteMaps.find(key);
	if (it != siteMaps.end()) {
		return it->second;
	}
	return buildSiteMap(key);

}

PatchMap<1>* Cartographer::getSiteMap(const UnitType *ut, const Vec2i &pos,
									  CardinalDir facing, Unit *worker) {
	PF_TRACE();
	BuildSiteMapKey key(ut, pos, facing, worker->getCurrField(), worker->getType()->getSize());
	return getSiteMap(key);
}

void Cartographer::onStoreDestroyed(Unit *unit) {
	///@todo fixme
//	delete storeMaps[unit];
//	storeMaps.erase(unit);
}

PatchMap<1>* Cartographer::buildAdjacencyMap(const UnitType *uType, const Vec2i &pos,
											 CardinalDir facing, Field f, int size) {
	PF_TRACE();
	const Vec2i mapPos = pos + (OrdinalOffsets[odNorthWest] * size);
	const int sx = pos.x;
	const int sy = pos.y;

	Rectangle rect(mapPos.x, mapPos.y, uType->getSize() + 2 + size, uType->getSize() + 2 + size);
	PatchMap<1> *pMap = new PatchMap<1>(rect, 0);
	pMap->zeroMap();

	PatchMap<1> tmpMap(rect, 0);
	tmpMap.zeroMap();

	// mark cells occupied by unitType at pos (on tmpMap)
	Util::RectIterator rIter(pos, pos + Vec2i(uType->getSize() - 1));
	while (rIter.more()) {
		Vec2i gpos = rIter.next();
		if (!uType->hasCellMap() || uType->getCellMapCell(gpos.x - sx, gpos.y - sy, facing)) {
			tmpMap.setInfluence(gpos, 1);
		}
	}

	// mark goal cells on result map
	rIter = Util::RectIterator(mapPos, pos + Vec2i(uType->getSize()));
	while (rIter.more()) {
		Vec2i gpos = rIter.next();
		if (tmpMap.getInfluence(gpos) || !masterMap->canOccupy(gpos, size, f)) {
			continue; // building or obstacle
		}
		Util::PerimeterIterator pIter(gpos - Vec2i(1), gpos + Vec2i(size));
		while (pIter.more()) {
			if (tmpMap.getInfluence(pIter.next())) {
				pMap->setInfluence(gpos, 1);
				break;
			}
		}
	}
	return pMap;
}

#ifdef DEBUG_RENDERING_ENABLED

	void Cartographer::debugAddBuildSiteMap(PatchMap<1> *siteMap) {
		Rectangle mapBounds = siteMap->getBounds();
		for (int ly = 0; ly < mapBounds.h; ++ly) {
			int y = mapBounds.y + ly;
			for (int lx = 0; lx < mapBounds.w; ++lx) {
				Vec2i pos(mapBounds.x + lx, y);
				if (siteMap->getInfluence(pos)) {
					getDebugRenderer().addBuildSiteCell(pos);
				}
			}
		}
	}

#endif

void Cartographer::tick() {
	PF_TRACE();
	if (clusterMap->isDirty()) {
		clusterMap->update();
	}
	for (ResourcePosMap::iterator it = resDirtyAreas.begin(); it != resDirtyAreas.end(); ++it) {
		if (!it->second.empty()) {
			for (V2iList::iterator posIt = it->second.begin(); posIt != it->second.end(); ++posIt) {
				fixupResourceMaps(it->first, *posIt);
			}
			it->second.clear();
		}
	}
}

}}
