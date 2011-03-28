// ==============================================================
// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martio Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "map.h"

#include <cassert>

#include "tileset.h"
#include "unit.h"
#include "resource.h"
#include "logger.h"
#include "tech_tree.h"
#include "config.h"
#include "util.h"
#include "game_settings.h"
#include "platform_util.h"
#include "pos_iterator.h"
#include "faction.h"
#include "command.h"
#include "leak_dumper.h"
#include "map_preview.h"

using namespace Shared::Graphics;
using namespace Shared::Util;
using namespace Shared::Platform;

namespace Glest{ namespace Game{

// =====================================================
// 	class Cell
// =====================================================

Cell::Cell() {
	//game data
    for(int i = 0; i < fieldCount; ++i) {
        units[i]= NULL;
        unitsWithEmptyCellMap[i]=NULL;
    }
	height= 0;
}

// ==================== misc ====================

//returns if the cell is free
bool Cell::isFree(Field field) const {
	return getUnit(field) == NULL || getUnit(field)->isPutrefacting();
}

// =====================================================
// 	class SurfaceCell
// =====================================================

SurfaceCell::SurfaceCell() {
	object= NULL;
	vertex= Vec3f(0.f);
	normal= Vec3f(0.f, 1.f, 0.f);
	surfaceType= -1;
	surfaceTexture= NULL;
	nearSubmerged = false;
}

SurfaceCell::~SurfaceCell() {
	delete object;
}

void SurfaceCell::end(){
	if(object!=NULL){
		object->end();
	}
}


bool SurfaceCell::isFree() const {
	return object==NULL || object->getWalkable();
}

void SurfaceCell::deleteResource() {
	delete object;
	object= NULL;
}

void SurfaceCell::setExplored(int teamIndex, bool explored) {
	this->explored[teamIndex]= explored;
}

void SurfaceCell::setVisible(int teamIndex, bool visible) {
    this->visible[teamIndex]= visible;
}

// =====================================================
// 	class Map
// =====================================================

// ===================== PUBLIC ========================

const int Map::cellScale= 2;
const int Map::mapScale= 2;

Map::Map() {
	cells= NULL;
	surfaceCells= NULL;
	startLocations= NULL;

	title="";
	waterLevel=0;
	heightFactor=0;
	cliffLevel=0;
	cameraHeight=0;
	w=0;
	h=0;
	surfaceW=0;
	surfaceH=0;
	maxPlayers=0;
	maxMapHeight=0;
}

Map::~Map() {
	Logger::getInstance().add("Cells", true);

	delete [] cells;
	cells = NULL;
	delete [] surfaceCells;
	surfaceCells = NULL;
	delete [] startLocations;
	startLocations = NULL;
}

void Map::end(){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
    Logger::getInstance().add("Map", true);
	//read heightmap
	for(int j = 0; j < surfaceH; ++j) {
		for(int i = 0; i < surfaceW; ++i) {
			getSurfaceCell(i, j)->end();
		}
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

int Map::getSurfaceCellArraySize() const {
	return (surfaceW * surfaceH);
}

SurfaceCell *Map::getSurfaceCell(const Vec2i &sPos) const {
	return getSurfaceCell(sPos.x, sPos.y);
}

SurfaceCell *Map::getSurfaceCell(int sx, int sy) const {
	int arrayIndex = sy * surfaceW + sx;
	if(arrayIndex < 0 || arrayIndex >= getSurfaceCellArraySize()) {
		throw runtime_error("arrayIndex >= getSurfaceCellArraySize(), arrayIndex = " + intToStr(arrayIndex) + " surfaceW = " + intToStr(surfaceW) + " surfaceH = " + intToStr(surfaceH));
	}
	else if(surfaceCells == NULL) {
		throw runtime_error("surfaceCells == NULL");
	}
	return &surfaceCells[arrayIndex];
}

int Map::getCellArraySize() const {
	return (w * h);
}

Cell *Map::getCell(const Vec2i &pos) const {
	return getCell(pos.x, pos.y);
}

Cell *Map::getCell(int x, int y) const {
	int arrayIndex = y * w + x;
	if(arrayIndex < 0 || arrayIndex >= getCellArraySize()) {
		//abort();
		throw runtime_error("arrayIndex >= getCellArraySize(), arrayIndex = " + intToStr(arrayIndex) + " w = " + intToStr(w) + " h = " + intToStr(h));
	}
	else if(cells == NULL) {
		throw runtime_error("cells == NULL");
	}

	return &cells[arrayIndex];
}

Vec2i Map::getStartLocation(int locationIndex) const {
	if(locationIndex >= maxPlayers) {
		throw runtime_error("locationIndex >= maxPlayers");
	}
	else if(startLocations == NULL) {
		throw runtime_error("startLocations == NULL");
	}

	return startLocations[locationIndex];
}

Checksum Map::load(const string &path, TechTree *techTree, Tileset *tileset) {
    Checksum mapChecksum;
	try{
		FILE *f = fopen(path.c_str(), "rb");
		if(f != NULL) {
		    mapChecksum.addFile(path);
		    checksumValue.addFile(path);
			//read header
			MapFileHeader header;
			size_t readBytes = fread(&header, sizeof(MapFileHeader), 1, f);

			if(next2Power(header.width) != header.width){
				throw runtime_error("Map width is not a power of 2");
			}

			if(next2Power(header.height) != header.height){
				throw runtime_error("Map height is not a power of 2");
			}

			heightFactor= header.heightFactor;
			waterLevel= static_cast<float>((header.waterLevel-0.01f)/heightFactor);
			title= header.title;
			maxPlayers= header.maxFactions;
			surfaceW= header.width;
			surfaceH= header.height;
			w= surfaceW*cellScale;
			h= surfaceH*cellScale;
			cliffLevel = 0;
			cameraHeight = 0;
			if(header.version==1){
				//desc = header.description;
			}
			else if(header.version==2){
				//desc = header.version2.short_desc;
				if(header.version2.cliffLevel > 0  && header.version2.cliffLevel < 5000){
					cliffLevel=static_cast<float>((header.version2.cliffLevel-0.01f)/(heightFactor));
				}
				if(header.version2.cameraHeight > 0 && header.version2.cameraHeight < 5000)
				{
					cameraHeight = header.version2.cameraHeight;
				}
			}

			//start locations
			startLocations= new Vec2i[maxPlayers];
			for(int i=0; i < maxPlayers; ++i) {
				int x=0, y=0;
				readBytes = fread(&x, sizeof(int32), 1, f);
				readBytes = fread(&y, sizeof(int32), 1, f);
				startLocations[i]= Vec2i(x, y)*cellScale;
			}

			//cells
			cells= new Cell[getCellArraySize()];
			surfaceCells= new SurfaceCell[getSurfaceCellArraySize()];

			//read heightmap
			for(int j = 0; j < surfaceH; ++j) {
				for(int i = 0; i < surfaceW; ++i) {
					float32 alt=0;
					readBytes = fread(&alt, sizeof(float32), 1, f);
					SurfaceCell *sc= getSurfaceCell(i, j);
					sc->setVertex(Vec3f(i*mapScale, alt / heightFactor, j*mapScale));
				}
			}

			//read surfaces
			for(int j = 0; j < surfaceH; ++j) {
				for(int i = 0; i < surfaceW; ++i) {
					int8 surf=0;
					readBytes = fread(&surf, sizeof(int8), 1, f);
					getSurfaceCell(i, j)->setSurfaceType(surf-1);
				}
			}

			//read objects and resources
			for(int j = 0; j < h; j += cellScale) {
				for(int i = 0; i < w; i += cellScale) {

					int8 objNumber=0;
					readBytes = fread(&objNumber, sizeof(int8), 1, f);
					SurfaceCell *sc= getSurfaceCell(toSurfCoords(Vec2i(i, j)));
					if(objNumber == 0) {
						sc->setObject(NULL);
					}
					else if(objNumber <= Tileset::objCount) {
						Object *o= new Object(tileset->getObjectType(objNumber-1), sc->getVertex(),Vec2i(i, j));
						sc->setObject(o);
						for(int k = 0; k < techTree->getResourceTypeCount(); ++k) {
							const ResourceType *rt= techTree->getResourceType(k);
							if(rt->getClass() == rcTileset && rt->getTilesetObject() == objNumber){
								o->setResource(rt, Vec2i(i, j));
							}
						}
					}
					else{
						const ResourceType *rt= techTree->getTechResourceType(objNumber - Tileset::objCount) ;
						Object *o= new Object(NULL, sc->getVertex(),Vec2i(i, j));
						o->setResource(rt, Vec2i(i, j));
						sc->setObject(o);
					}
				}
			}
		}
		else{
			throw runtime_error("Can't open file");
		}
		fclose(f);
	}
	catch(const exception &e){
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
		throw runtime_error("Error loading map: "+ path+ "\n"+ e.what());
	}

	return mapChecksum;
}

void Map::init(Tileset *tileset) {
	Logger::getInstance().add("Heightmap computations", true);
	maxMapHeight=0.0f;
	smoothSurface(tileset);
	computeNormals();
	computeInterpolatedHeights();
	computeNearSubmerged();
	computeCellColors();
}


// ==================== is ====================

bool Map::isInside(int x, int y) const {
	return x>=0 && y>=0 && x<w && y<h;
}

bool Map::isInside(const Vec2i &pos) const {
	return isInside(pos.x, pos.y);
}

bool Map::isInsideSurface(int sx, int sy) const {
	return sx>=0 && sy>=0 && sx<surfaceW && sy<surfaceH;
}

bool Map::isInsideSurface(const Vec2i &sPos) const {
	return isInsideSurface(sPos.x, sPos.y);
}

//returns if there is a resource next to a unit, in "resourcePos" is stored the relative position of the resource
bool Map::isResourceNear(const Vec2i &pos, const ResourceType *rt, Vec2i &resourcePos, int size, Unit *unit, bool fallbackToPeersHarvestingSameResource) const {
	for(int i = -1; i <= size; ++i) {
		for(int j = -1; j <= size; ++j) {
			Vec2i resPos = Vec2i(pos.x + i, pos.y + j);
			if(isInside(resPos) && isInsideSurface(toSurfCoords(resPos))) {
				Resource *r= getSurfaceCell(toSurfCoords(Vec2i(pos.x + i, pos.y + j)))->getResource();
				if(r != NULL) {
					if(r->getType() == rt) {
						resourcePos= pos + Vec2i(i,j);

						if(unit == NULL || unit->isBadHarvestPos(resourcePos) == false) {
							return true;
						}
					}
				}
			}
		}
	}

	if(fallbackToPeersHarvestingSameResource == true && unit != NULL) {
		// Look for another unit that is currently harvesting the same resource
		// type right now

		// Check the faction cache for a known position where we can harvest
		// this resource type
		Vec2i result = unit->getFaction()->getClosestResourceTypeTargetFromCache(unit, rt);
		if(result.x >= 0) {
			resourcePos = result;

			if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true) {
				char szBuf[4096]="";
							sprintf(szBuf,"[found peer harvest pos] pos [%s] resourcePos [%s] unit->getFaction()->getCacheResourceTargetListSize() [%d]",
									pos.getString().c_str(),resourcePos.getString().c_str(),unit->getFaction()->getCacheResourceTargetListSize());
				unit->logSynchData(__FILE__,__LINE__,szBuf);
			}

			if(unit->getPos().dist(resourcePos) <= size) {
				return true;
			}
		}
	}

	return false;
}

//returns if there is a resource next to a unit, in "resourcePos" is stored the relative position of the resource
bool Map::isResourceNear(const Vec2i &pos, int size, const ResourceType *rt, Vec2i &resourcePos) const {
	Vec2i p1 = pos + Vec2i(-1);
	Vec2i p2 = pos + Vec2i(size);
	Util::PerimeterIterator iter(p1, p2);
	while (iter.more()) {
		Vec2i cur = iter.next();
		if (isInside(cur) && isInsideSurface(toSurfCoords(cur))) {
			Resource *r = getSurfaceCell(toSurfCoords(cur))->getResource();
			if (r && r->getType() == rt) {
				resourcePos = cur;
				return true;
			}
		}
	}
	return false;
}

// ==================== free cells ====================

bool Map::isFreeCell(const Vec2i &pos, Field field) const {
	return
		isInside(pos) &&
		isInsideSurface(toSurfCoords(pos)) &&
		getCell(pos)->isFree(field) &&
		(field==fAir || getSurfaceCell(toSurfCoords(pos))->isFree()) &&
		(field!=fLand || getDeepSubmerged(getCell(pos)) == false);
}

bool Map::isFreeCellOrHasUnit(const Vec2i &pos, Field field, const Unit *unit) const {
	if(isInside(pos) && isInsideSurface(toSurfCoords(pos))) {
		Cell *c= getCell(pos);
		if(c->getUnit(unit->getCurrField()) == unit) {
			if(unit->getCurrField() == fAir) {
				const SurfaceCell *sc= getSurfaceCell(toSurfCoords(pos));
				if(sc != NULL) {
					if(getDeepSubmerged(sc) == true) {
						return false;
					}
					else if(field == fLand) {
						if(sc->isFree() == false) {
							return false;
						}
						else if(c->getUnit(field) != NULL) {
							return false;
						}
					}
				}
			}
			return true;
		}
		else{
			return isFreeCell(pos, field);
		}
	}
	return false;
}

bool Map::isAproxFreeCell(const Vec2i &pos, Field field, int teamIndex) const {
	if(isInside(pos) && isInsideSurface(toSurfCoords(pos))) {
		const SurfaceCell *sc= getSurfaceCell(toSurfCoords(pos));

		if(sc->isVisible(teamIndex)) {
			return isFreeCell(pos, field);
		}
		else if(sc->isExplored(teamIndex)) {
			return field==fLand? sc->isFree() && !getDeepSubmerged(getCell(pos)): true;
		}
		else {
			return true;
		}
	}
	return false;
}

bool Map::isFreeCells(const Vec2i & pos, int size, Field field) const  {
	for(int i=pos.x; i<pos.x+size; ++i) {
		for(int j=pos.y; j<pos.y+size; ++j) {
			Vec2i testPos(i,j);
			if(isFreeCell(testPos, field) == false) {
				return false;
			}
		}
	}
    return true;
}

bool Map::isFreeCellsOrHasUnit(const Vec2i &pos, int size, Field field,
		const Unit *unit, const UnitType *munit) const {
	if(unit == NULL) {
		throw runtime_error("unit == NULL");
	}
	if(munit == NULL) {
		throw runtime_error("munit == NULL");
	}
	for(int i=pos.x; i<pos.x+size; ++i) {
		for(int j=pos.y; j<pos.y+size; ++j) {
			if(isFreeCellOrHasUnit(Vec2i(i,j), field, unit) == false) {
				return false;
			}
		}
	}
	return true;
}

bool Map::isAproxFreeCells(const Vec2i &pos, int size, Field field, int teamIndex) const {
	for(int i=pos.x; i<pos.x+size; ++i) {
		for(int j=pos.y; j<pos.y+size; ++j) {
			if(isAproxFreeCell(Vec2i(i, j), field, teamIndex) == false) {
                return false;
			}
		}
	}
    return true;
}

bool Map::canOccupy(const Vec2i &pos, Field field, const UnitType *ut, CardinalDir facing) {
	if (ut->hasCellMap() && isInside(pos) && isInsideSurface(toSurfCoords(pos))) {
		for (int y=0; y < ut->getSize(); ++y) {
			for (int x=0; x < ut->getSize(); ++x) {
				Vec2i cellPos = pos + Vec2i(x, y);
				if(isInside(cellPos) && isInsideSurface(toSurfCoords(cellPos))) {
					if (ut->getCellMapCell(x, y, facing)) {
						if (isFreeCell(cellPos, field) == false) {
							return false;
						}
					}
				}
				else {
					false;
				}
			}
		}
		return true;
	}
	else {
		return isFreeCells(pos, ut->getSize(), field);
	}
}

// ==================== unit placement ====================

//checks if a unit can move from between 2 cells
bool Map::canMove(const Unit *unit, const Vec2i &pos1, const Vec2i &pos2, std::map<Vec2i, std::map<Vec2i, std::map<int, std::map<Field,bool> > > > *lookupCache) const {
	int size= unit->getType()->getSize();
	Field field= unit->getCurrField();

	if(lookupCache != NULL) {
		std::map<Vec2i, std::map<Vec2i, std::map<int, std::map<Field,bool> > > >::const_iterator iterFind1 = lookupCache->find(pos1);
		if(iterFind1 != lookupCache->end()) {
			std::map<Vec2i, std::map<int, std::map<Field,bool> > >::const_iterator iterFind2 = iterFind1->second.find(pos2);
			if(iterFind2 != iterFind1->second.end()) {
				std::map<int, std::map<Field,bool> >::const_iterator iterFind3 = iterFind2->second.find(size);
				if(iterFind3 != iterFind2->second.end()) {
					std::map<Field,bool>::const_iterator iterFind4 = iterFind3->second.find(field);
					if(iterFind4 != iterFind3->second.end()) {
						// Found this result in the cache
						return iterFind4->second;
					}
				}
			}
		}
	}

	for(int i=pos2.x; i<pos2.x+size; ++i) {
		for(int j=pos2.y; j<pos2.y+size; ++j) {
			if(isInside(i, j) && isInsideSurface(toSurfCoords(Vec2i(i,j)))) {
				if(getCell(i, j)->getUnit(field) != unit) {
					if(isFreeCell(Vec2i(i, j), field) == false) {
						if(lookupCache != NULL) {
							(*lookupCache)[pos1][pos2][size][field]=false;
						}

						return false;
					}
				}
			}
			else {
				if(lookupCache != NULL) {
					(*lookupCache)[pos1][pos2][size][field]=false;
				}

				return false;
			}
		}
	}
	if(unit == NULL || unit->isBadHarvestPos(pos2) == true) {
		if(lookupCache != NULL) {
			(*lookupCache)[pos1][pos2][size][field]=false;
		}

		return false;
	}

	if(lookupCache != NULL) {
		(*lookupCache)[pos1][pos2][size][field]=true;
	}

    return true;
}

//checks if a unit can move from between 2 cells using only visible cells (for pathfinding)
bool Map::aproxCanMove(const Unit *unit, const Vec2i &pos1, const Vec2i &pos2, std::map<Vec2i, std::map<Vec2i, std::map<int, std::map<int, std::map<Field,bool> > > > > *lookupCache) const {
	if(isInside(pos1) == false || isInsideSurface(toSurfCoords(pos1)) == false ||
	   isInside(pos2) == false || isInsideSurface(toSurfCoords(pos2)) == false) {
		return false;
	}

	int size= unit->getType()->getSize();
	int teamIndex= unit->getTeam();
	Field field= unit->getCurrField();

	if(lookupCache != NULL) {
		std::map<Vec2i, std::map<Vec2i, std::map<int, std::map<int, std::map<Field,bool> > > > >::const_iterator iterFind1 = lookupCache->find(pos1);
		if(iterFind1 != lookupCache->end()) {
			std::map<Vec2i, std::map<int, std::map<int, std::map<Field,bool> > > >::const_iterator iterFind2 = iterFind1->second.find(pos2);
			if(iterFind2 != iterFind1->second.end()) {
				std::map<int, std::map<int, std::map<Field,bool> > >::const_iterator iterFind3 = iterFind2->second.find(teamIndex);
				if(iterFind3 != iterFind2->second.end()) {
					std::map<int, std::map<Field,bool> >::const_iterator iterFind4 = iterFind3->second.find(size);
					if(iterFind4 != iterFind3->second.end()) {
						std::map<Field,bool>::const_iterator iterFind5 = iterFind4->second.find(field);
						if(iterFind5 != iterFind4->second.end()) {
							// Found this result in the cache
							return iterFind5->second;
						}
					}
				}
			}
		}
	}

	//single cell units
	if(size == 1) {
		if(isAproxFreeCell(pos2, field, teamIndex) == false) {
			if(lookupCache != NULL) {
				(*lookupCache)[pos1][pos2][teamIndex][size][field]=false;
			}
			return false;
		}
		if(pos1.x != pos2.x && pos1.y != pos2.y) {
			if(isAproxFreeCell(Vec2i(pos1.x, pos2.y), field, teamIndex) == false) {
				if(lookupCache != NULL) {
					(*lookupCache)[pos1][pos2][teamIndex][size][field]=false;
				}

				return false;
			}
			if(isAproxFreeCell(Vec2i(pos2.x, pos1.y), field, teamIndex) == false) {
				if(lookupCache != NULL) {
					(*lookupCache)[pos1][pos2][teamIndex][size][field]=false;
				}

				return false;
			}
		}

		if(unit == NULL || unit->isBadHarvestPos(pos2) == true) {
			if(lookupCache != NULL) {
				(*lookupCache)[pos1][pos2][teamIndex][size][field]=false;
			}

			return false;
		}

		if(lookupCache != NULL) {
			(*lookupCache)[pos1][pos2][teamIndex][size][field]=true;
		}

		return true;
	}
	//multi cell units
	else {
		for(int i = pos2.x; i < pos2.x + size; ++i) {
			for(int j = pos2.y; j < pos2.y + size; ++j) {

				Vec2i cellPos = Vec2i(i,j);
				if(isInside(cellPos) && isInsideSurface(toSurfCoords(cellPos))) {
					if(getCell(cellPos)->getUnit(unit->getCurrField()) != unit) {
						if(isAproxFreeCell(cellPos, field, teamIndex) == false) {
							if(lookupCache != NULL) {
								(*lookupCache)[pos1][pos2][teamIndex][size][field]=false;
							}

							return false;
						}
					}
				}
				else {

					if(lookupCache != NULL) {
						(*lookupCache)[pos1][pos2][teamIndex][size][field]=false;
					}

					return false;
				}
			}
		}

		if(unit == NULL || unit->isBadHarvestPos(pos2) == true) {
			if(lookupCache != NULL) {
				(*lookupCache)[pos1][pos2][teamIndex][size][field]=false;
			}

			return false;
		}

		if(lookupCache != NULL) {
			(*lookupCache)[pos1][pos2][teamIndex][size][field]=true;
		}
	}
	return true;
}

Vec2i Map::computeRefPos(const Selection *selection) const {
    Vec2i total= Vec2i(0);
    for(int i = 0; i < selection->getCount(); ++i) {
    	if(selection == NULL || selection->getUnit(i) == NULL) {
    		throw runtime_error("selection == NULL || selection->getUnit(i) == NULL");
    	}
        total = total + selection->getUnit(i)->getPos();
    }

    return Vec2i(total.x / selection->getCount(), total.y / selection->getCount());
}

Vec2i Map::computeDestPos(	const Vec2i &refUnitPos, const Vec2i &unitPos,
							const Vec2i &commandPos) const {
    Vec2i pos;
	Vec2i posDiff = unitPos - refUnitPos;

	if(abs(posDiff.x) >= 3){
		posDiff.x = posDiff.x % 3;
	}

	if(abs(posDiff.y) >= 3){
		posDiff.y = posDiff.y % 3;
	}

	pos = commandPos + posDiff;
    clampPos(pos);
    return pos;
}

std::pair<float,Vec2i> Map::getUnitDistanceToPos(const Unit *unit,Vec2i pos,const UnitType *ut) {
	if(unit == NULL) {
		throw runtime_error("unit == NULL");
	}

	std::pair<float,Vec2i> result(-1,Vec2i(0));
	int unitId= unit->getId();
	Vec2i unitPos= computeDestPos(unit->getPos(), unit->getPos(), pos);

	Vec2i start = pos - Vec2i(1);
	int unitTypeSize = 0;
	if(ut != NULL) {
		unitTypeSize = ut->getSize();
	}
	Vec2i end 	= pos + Vec2i(unitTypeSize);

	for(int i = start.x; i <= end.x; ++i) {
		for(int j = start.y; j <= end.y; ++j){
			Vec2i testPos(i,j);

			if(ut == NULL || isInUnitTypeCells(ut, pos,testPos) == false) {
				float distance = unitPos.dist(testPos);
				if(result.first < 0 || result.first > distance) {
					result.first = distance;
					result.second = testPos;
				}
			}
		}
	}

	return result;
}

const Unit * Map::findClosestUnitToPos(const Selection *selection, Vec2i originalBuildPos,
								 const UnitType *ut) const {
	const Unit *closestUnit = NULL;
	Vec2i refPos = computeRefPos(selection);

	Vec2i pos = originalBuildPos;

	float bestRange = -1;

	Vec2i start = pos - Vec2i(1);
	int unitTypeSize = 0;
	if(ut != NULL) {
		unitTypeSize = ut->getSize();
	}
	Vec2i end 	= pos + Vec2i(unitTypeSize);

	for(int i = 0; i < selection->getCount(); ++i) {
		const Unit *unit = selection->getUnit(i);
		int unitId= unit->getId();
		Vec2i unitBuilderPos= computeDestPos(refPos, unit->getPos(), pos);

		for(int i = start.x; i <= end.x; ++i) {
			for(int j = start.y; j <= end.y; ++j){
				Vec2i testPos(i,j);
				if(isInUnitTypeCells(ut, originalBuildPos,testPos) == false) {
					float distance = unitBuilderPos.dist(testPos);
					if(bestRange < 0 || bestRange > distance) {
						bestRange = distance;
						pos = testPos;
						closestUnit = unit;
					}
				}
			}
		}
	}

	return closestUnit;
}

Vec2i Map::findBestBuildApproach(const Unit *unit, Vec2i originalBuildPos,const UnitType *ut) const {
    Vec2i unitBuilderPos    = unit->getPos();
	Vec2i pos               = originalBuildPos;

	float bestRange = -1;

	Vec2i start = pos - Vec2i(1);
	Vec2i end 	= pos + Vec2i(ut->getSize());

	for(int i = start.x; i <= end.x; ++i) {
		for(int j = start.y; j <= end.y; ++j) {
			Vec2i testPos(i,j);
			if(isInUnitTypeCells(ut, originalBuildPos,testPos) == false) {
				float distance = unitBuilderPos.dist(testPos);
				if(bestRange < 0 || bestRange > distance) {
				    // Check if the cell is occupied by another unit
				    if(isFreeCellOrHasUnit(testPos, unit->getType()->getField(), unit) == true) {
                        bestRange = distance;
                        pos = testPos;
				    }
				}
			}
		}
	}

	return pos;
}

bool Map::isNextToUnitTypeCells(const UnitType *ut, const Vec2i &pos,
								const Vec2i &testPos) const {
	bool isInsideDestUnitCells = isInUnitTypeCells(ut, pos,testPos);
	if(isInsideDestUnitCells == false) {
		Cell *testCell = getCell(testPos);
		for(int i=-1; i <= ut->getSize(); ++i){
			for(int j = -1; j <= ut->getSize(); ++j) {
				Vec2i currPos = pos + Vec2i(i, j);
				if(isInside(currPos) == true && isInsideSurface(toSurfCoords(currPos)) == true) {
					//Cell *unitCell = getCell(currPos);
					//if(unitCell == testCell) {
					if(isNextTo(testPos,currPos) == true) {
						return true;
					}
				}
			}
		}
	}

	return false;
}

// is testPos in the cells of unitType where unitType's position is pos
bool Map::isInUnitTypeCells(const UnitType *ut, const Vec2i &pos,
							const Vec2i &testPos) const {
	assert(ut != NULL);
	if(ut == NULL) {
		throw runtime_error("ut == NULL");
	}

	if(isInside(testPos) && isInsideSurface(toSurfCoords(testPos))) {
		Cell *testCell = getCell(testPos);
		for(int i=0; i < ut->getSize(); ++i){
			for(int j = 0; j < ut->getSize(); ++j) {
				Vec2i currPos = pos + Vec2i(i, j);
				if(isInside(currPos) && isInsideSurface(toSurfCoords(currPos))) {
					Cell *unitCell = getCell(currPos);
					if(unitCell == testCell) {
						return true;
					}
				}
			}
		}
	}
	return false;
}

//put a units into the cells
void Map::putUnitCells(Unit *unit, const Vec2i &pos) {
	assert(unit != NULL);
	if(unit == NULL) {
		throw runtime_error("ut == NULL");
	}

    bool canPutInCell = true;
	const UnitType *ut= unit->getType();

	for(int i = 0; i < ut->getSize(); ++i) {
		for(int j = 0; j < ut->getSize(); ++j) {
			Vec2i currPos= pos + Vec2i(i, j);
			assert(isInside(currPos));
			if(isInside(currPos) == false) {
				throw runtime_error("isInside(currPos) == false");
			}

			if( ut->hasCellMap() == false || ut->getCellMapCell(i, j, unit->getModelFacing())) {
				if(getCell(currPos)->getUnit(unit->getCurrField()) != NULL &&
                   getCell(currPos)->getUnit(unit->getCurrField()) != unit) {

                    // If unit tries to move into a cell where another unit resides
                    // cancel the move command
                    if(unit->getCurrSkill() != NULL &&
                       unit->getCurrSkill()->getClass() == scMove) {
                        canPutInCell = false;
                        //unit->setCurrSkill(scStop);
                        //unit->finishCommand();

                        //SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] POSSIBLE ERROR [getCell(currPos)->getUnit(unit->getCurrField()) != NULL] currPos [%s] unit [%s] cell unit [%s]\n",
                        //      __FILE__,__FUNCTION__,__LINE__,
                        //      currPos.getString().c_str(),
                        //      unit->toString().c_str(),
                        //      getCell(currPos)->getUnit(unit->getCurrField())->toString().c_str());
                    }
                    // If the unit trying to move into the cell is not in the moving state
                    // it is likely being created or morphed so we will will log the error
                    else {
                        canPutInCell = false;
				        // throw runtime_error("getCell(currPos)->getUnit(unit->getCurrField()) != NULL");
                        SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] ERROR [getCell(currPos)->getUnit(unit->getCurrField()) != NULL] currPos [%s] unit [%s] cell unit [%s]\n",
                              __FILE__,__FUNCTION__,__LINE__,
                              currPos.getString().c_str(),
                              unit->toString().c_str(),
                              getCell(currPos)->getUnit(unit->getCurrField())->toString().c_str());
                    }
				}

                if(canPutInCell == true) {
                    getCell(currPos)->setUnit(unit->getCurrField(), unit);
                }
			}
			else if(ut->hasCellMap() == true &&
					ut->getAllowEmptyCellMap() == true &&
					ut->hasEmptyCellMap() == true) {
				getCell(currPos)->setUnitWithEmptyCellMap(unit->getCurrField(), unit);

				//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] currPos = %s unit = %s\n",
                //             __FILE__,__FUNCTION__,__LINE__,
                //             currPos.getString().c_str(),
                //             unit->toString().c_str());
			}
		}
	}
	if(canPutInCell == true) {
        unit->setPos(pos);
	}
}

//removes a unit from cells
void Map::clearUnitCells(Unit *unit, const Vec2i &pos) {
	assert(unit != NULL);
	if(unit == NULL) {
		throw runtime_error("unit == NULL");
	}

	const UnitType *ut= unit->getType();

	for(int i=0; i<ut->getSize(); ++i){
		for(int j=0; j<ut->getSize(); ++j){
			Vec2i currPos= pos + Vec2i(i, j);
			assert(isInside(currPos));
			if(isInside(currPos) == false) {
				throw runtime_error("isInside(currPos) == false");
			}

			if(ut->hasCellMap() == false || ut->getCellMapCell(i, j, unit->getModelFacing())) {
				// This seems to be a bad assert since you can clear the cell
				// for many reasons including a unit dieing.

				//assert(getCell(currPos)->getUnit(unit->getCurrField()) == unit || getCell(currPos)->getUnit(unit->getCurrField()) == NULL);
				//if(getCell(currPos)->getUnit(unit->getCurrField()) != unit && getCell(currPos)->getUnit(unit->getCurrField()) != NULL) {
				//	throw runtime_error("getCell(currPos)->getUnit(unit->getCurrField()) != unit");
					//SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] ERROR [getCell(currPos)->getUnit(unit->getCurrField()) != unit] currPos [%s] unit [%s] cell unit [%s]\n",
                    //          __FILE__,__FUNCTION__,__LINE__,
                    //          currPos.getString().c_str(),
                    //          unit->toString().c_str(),
                    //          (getCell(currPos)->getUnit(unit->getCurrField()) != NULL ? getCell(currPos)->getUnit(unit->getCurrField())->toString().c_str() : "NULL"));
				//}

                // Only clear the cell if its the unit we expect to clear out of it
                if(getCell(currPos)->getUnit(unit->getCurrField()) == unit) {
                    getCell(currPos)->setUnit(unit->getCurrField(), NULL);
                }
			}
			else if(ut->hasCellMap() == true &&
					ut->getAllowEmptyCellMap() == true &&
					ut->hasEmptyCellMap() == true) {
				getCell(currPos)->setUnitWithEmptyCellMap(unit->getCurrField(), NULL);

				//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] currPos = %s unit = %s\n",
                //             __FILE__,__FUNCTION__,__LINE__,
                //             currPos.getString().c_str(),
                //             unit->toString().c_str());
			}
		}
	}
}

// ==================== misc ====================

//return if unit is next to pos
bool Map::isNextTo(const Vec2i &pos, const Unit *unit) const {

	for(int i=-1; i<=1; ++i) {
		for(int j=-1; j<=1; ++j) {
			if(isInside(pos.x+i, pos.y+j) && isInsideSurface(toSurfCoords(Vec2i(pos.x+i, pos.y+j)))) {
				if(getCell(pos.x+i, pos.y+j)->getUnit(fLand) == unit) {
					return true;
				}
				else if(getCell(pos.x+i, pos.y+j)->getUnitWithEmptyCellMap(fLand) == unit) {
					return true;
				}
			}
		}
	}
    return false;
}

//return if unit is next to pos
bool Map::isNextTo(const Vec2i &pos, const Vec2i &nextToPos) const {

	for(int i=-1; i<=1; ++i) {
		for(int j=-1; j<=1; ++j) {
			if(isInside(pos.x+i, pos.y+j) && isInsideSurface(toSurfCoords(Vec2i(pos.x+i, pos.y+j)))) {
				if(getCell(pos.x+i, pos.y+j) == getCell(nextToPos.x,nextToPos.y)) {
					return true;
				}
			}
		}
	}
    return false;
}

void Map::clampPos(Vec2i &pos) const{
	if(pos.x<0){
        pos.x=0;
	}
	if(pos.y<0){
        pos.y=0;
	}
	if(pos.x>=w){
        pos.x=w-1;
	}
	if(pos.y>=h){
        pos.y=h-1;
	}
}

void Map::prepareTerrain(const Unit *unit) {
	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

	flatternTerrain(unit);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

    computeNormals();

    if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	computeInterpolatedHeights();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
}

// ==================== PRIVATE ====================

// ==================== compute ====================

void Map::flatternTerrain(const Unit *unit){
	float refHeight= getSurfaceCell(toSurfCoords(unit->getCenteredPos()))->getHeight();
	for(int i=-1; i<=unit->getType()->getSize(); ++i){
        for(int j=-1; j<=unit->getType()->getSize(); ++j){
            Vec2i pos= unit->getPos()+Vec2i(i, j);
            if(isInside(pos) && isInsideSurface(toSurfCoords(pos))) {
				Cell *c= getCell(pos);
				SurfaceCell *sc= getSurfaceCell(toSurfCoords(pos));
				//we change height if pos is inside world, if its free or ocupied by the currenty building
				if(sc->getObject() == NULL && (c->getUnit(fLand)==NULL || c->getUnit(fLand)==unit)) {
					sc->setHeight(refHeight);
				}
            }
        }
    }
}

//compute normals
void Map::computeNormals(){
    //compute center normals
    for(int i=1; i<surfaceW-1; ++i){
        for(int j=1; j<surfaceH-1; ++j){
            getSurfaceCell(i, j)->setNormal(
				getSurfaceCell(i, j)->getVertex().normal(getSurfaceCell(i, j-1)->getVertex(),
					getSurfaceCell(i+1, j)->getVertex(),
					getSurfaceCell(i, j+1)->getVertex(),
					getSurfaceCell(i-1, j)->getVertex()));
        }
    }
}

void Map::computeInterpolatedHeights(){

	for(int i=0; i<w; ++i){
		for(int j=0; j<h; ++j){
			getCell(i, j)->setHeight(getSurfaceCell(toSurfCoords(Vec2i(i, j)))->getHeight());
		}
	}

	for(int i=1; i<surfaceW-1; ++i){
		for(int j=1; j<surfaceH-1; ++j){
			for(int k=0; k<cellScale; ++k){
				for(int l=0; l<cellScale; ++l){
					if(k==0 && l==0){
						getCell(i*cellScale, j*cellScale)->setHeight(getSurfaceCell(i, j)->getHeight());
					}
					else if(k!=0 && l==0){
						getCell(i*cellScale+k, j*cellScale)->setHeight((
							getSurfaceCell(i, j)->getHeight()+
							getSurfaceCell(i+1, j)->getHeight())/2.f);
					}
					else if(l!=0 && k==0){
						getCell(i*cellScale, j*cellScale+l)->setHeight((
							getSurfaceCell(i, j)->getHeight()+
							getSurfaceCell(i, j+1)->getHeight())/2.f);
					}
					else{
						getCell(i*cellScale+k, j*cellScale+l)->setHeight((
							getSurfaceCell(i, j)->getHeight()+
							getSurfaceCell(i, j+1)->getHeight()+
							getSurfaceCell(i+1, j)->getHeight()+
							getSurfaceCell(i+1, j+1)->getHeight())/4.f);
					}
				}
			}
		}
	}
}

void Map::smoothSurface(Tileset *tileset) {
	float *oldHeights = new float[getSurfaceCellArraySize()];
	int arraySize=getSurfaceCellArraySize();

	for (int i = 0; i < getSurfaceCellArraySize(); ++i) {
		oldHeights[i] = surfaceCells[i].getHeight();
	}

	for (int i = 1; i < surfaceW - 1; ++i) {
		for (int j = 1; j < surfaceH - 1; ++j) {
			float height = 0.f;
			float numUsedToSmooth = 0.f;
			for (int k = -1; k <= 1; ++k) {
				for (int l = -1; l <= 1; ++l) {
#ifdef USE_STREFLOP
					if (cliffLevel<=0.1f || cliffLevel > streflop::fabs(oldHeights[(j) * surfaceW + (i)]
							- oldHeights[(j + k) * surfaceW + (i + l)])) {
#else
					if (cliffLevel<=0.1f || cliffLevel > fabs(oldHeights[(j) * surfaceW + (i)]
							- oldHeights[(j + k) * surfaceW + (i + l)])) {
#endif
						height += oldHeights[(j + k) * surfaceW + (i + l)];
						numUsedToSmooth++;
					}
					else {
						// we have something which should not be smoothed!
						// This is a cliff and must be textured -> set cliff texture
						getSurfaceCell(i, j)->setSurfaceType(5);
						//set invisible blocking object and replace resource objects
						//and non blocking objects with invisible blocker too
						Object *formerObject =
								getSurfaceCell(i, j)->getObject();
						if (formerObject != NULL) {
							if (formerObject->getWalkable()
									|| formerObject->getResource() != NULL) {
								delete formerObject;
								formerObject = NULL;
							}
						}
						if (formerObject == NULL) {
							Object *o = new Object(tileset->getObjectType(9),
									getSurfaceCell(i, j)->getVertex(), Vec2i(i,
											j));
							getSurfaceCell(i, j)->setObject(o);
						}
					}
				}
			}

			height /= numUsedToSmooth;
			if(maxMapHeight<height){
				maxMapHeight=height;
			}

			getSurfaceCell(i, j)->setHeight(height);
			Object *object = getSurfaceCell(i, j)->getObject();
			if (object != NULL) {
				object->setHeight(height);
			}
		}
	}
	delete[] oldHeights;
}

void Map::computeNearSubmerged(){

	for(int i=0; i<surfaceW-1; ++i){
		for(int j=0; j<surfaceH-1; ++j){
			bool anySubmerged= false;
			for(int k=-1; k<=2; ++k){
				for(int l=-1; l<=2; ++l){
					Vec2i pos= Vec2i(i+k, j+l);
					if(isInsideSurface(pos) && isInsideSurface(toSurfCoords(pos))) {
						if(getSubmerged(getSurfaceCell(pos)))
							anySubmerged= true;
					}
				}
			}
			getSurfaceCell(i, j)->setNearSubmerged(anySubmerged);
		}
	}
}

void Map::computeCellColors(){
	for(int i=0; i<surfaceW; ++i){
		for(int j=0; j<surfaceH; ++j){
			SurfaceCell *sc= getSurfaceCell(i, j);
			if(getDeepSubmerged(sc)){
				float factor= clamp(waterLevel-sc->getHeight()*1.5f, 1.f, 1.5f);
				sc->setColor(Vec3f(1.0f, 1.0f, 1.0f)/factor);
			}
			else{
				sc->setColor(Vec3f(1.0f, 1.0f, 1.0f));
			}
		}
	}
}

// static
string Map::getMapPath(const string &mapName, string scenarioDir, bool errorOnNotFound) {

    Config &config = Config::getInstance();
    vector<string> pathList = config.getPathListForType(ptMaps,scenarioDir);

    for(int idx = 0; idx < pathList.size(); idx++) {
        string map_path = pathList[idx];
    	endPathWithSlash(map_path);

        const string mega = map_path + mapName + ".mgm";
        const string glest = map_path + mapName + ".gbm";
        if (fileExists(mega)) {
            return mega;
        }
        else if (fileExists(glest)) {
            return glest;
        }
    }

	if(errorOnNotFound == true) {
		throw runtime_error("Map [" + mapName + "] not found, scenarioDir [" + scenarioDir + "]");
	}

	return "";
}

// =====================================================
// 	class PosCircularIterator
// =====================================================

PosCircularIterator::PosCircularIterator(const Map *map, const Vec2i &center, int radius){
	this->map= map;
	this->radius= radius;
	this->center= center;
	pos= center - Vec2i(radius, radius);
	pos.x-= 1;
}

bool PosCircularIterator::next(){

	//iterate while dont find a cell that is inside the world
	//and at less or equal distance that the radius
	do{
		pos.x++;
		if(pos.x > center.x+radius){
			pos.x= center.x-radius;
			pos.y++;
		}
		if(pos.y>center.y+radius)
			return false;
	}
#ifdef USE_STREFLOP
	while(streflop::floor(pos.dist(center)) >= (radius+1) || !map->isInside(pos) || !map->isInsideSurface(map->toSurfCoords(pos)) );
#else
	while(floor(pos.dist(center)) >= (radius+1) || !map->isInside(pos) || !map->isInsideSurface(map->toSurfCoords(pos)) );
#endif
	//while(!(pos.dist(center) <= radius && map->isInside(pos)));

	return true;
}

const Vec2i &PosCircularIterator::getPos(){
	return pos;
}


// =====================================================
// 	class PosQuadIterator
// =====================================================

PosQuadIterator::PosQuadIterator(const Map *map,const Quad2i &quad, int step) {
	this->map = map;

	this->quad= quad;
	this->boundingRect= quad.computeBoundingRect();
	this->step= step;
	pos= boundingRect.p[0];
	--pos.x;
	pos.x= (pos.x/step)*step;
	pos.y= (pos.y/step)*step;
	//map->clampPos(pos);
}

bool PosQuadIterator::next() {

	do {
		pos.x += step;
		if(pos.x > boundingRect.p[1].x) {
			pos.x = (boundingRect.p[0].x / step) * step;
			pos.y += step;
		}
		if(pos.y > boundingRect.p[1].y) {
			return false;
		}

		//printf("pos [%s] boundingRect.p[0] [%s] boundingRect.p[1] [%s]\n",pos.getString().c_str(),boundingRect.p[0].getString().c_str(),boundingRect.p[1].getString().c_str());
	}
	while(!quad.isInside(pos));

	return true;
}

void PosQuadIterator::skipX() {
	pos.x+= step;
}

const Vec2i &PosQuadIterator::getPos(){
	return pos;
}

}}//end namespace
