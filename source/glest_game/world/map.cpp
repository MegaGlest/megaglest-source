// ==============================================================
// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
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
#include "map_preview.h"
#include "world.h"

#include "leak_dumper.h"

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

//returns if the cell is free

void Cell::saveGame(XmlNode *rootNode, int index) const {
	bool saveCell = false;
	if(saveCell == false) {
		for(unsigned int i = 0; i < fieldCount; ++i) {
			if(units[i] != NULL) {
				saveCell = true;
				break;
			}
			if(unitsWithEmptyCellMap[i] != NULL) {
				saveCell = true;
				break;
			}
		}
	}

	if(saveCell == true) {
		std::map<string,string> mapTagReplacements;
		XmlNode *cellNode = rootNode->addChild("Cell" + intToStr(index));
		cellNode->addAttribute("index",intToStr(index), mapTagReplacements);

	//    Unit *units[fieldCount];	//units on this cell
		for(unsigned int i = 0; i < fieldCount; ++i) {
			if(units[i] != NULL) {
				XmlNode *unitsNode = cellNode->addChild("units");
				unitsNode->addAttribute("field",intToStr(i), mapTagReplacements);
				unitsNode->addAttribute("unitid",intToStr(units[i]->getId()), mapTagReplacements);
			}
		}
	//    Unit *unitsWithEmptyCellMap[fieldCount];	//units with an empty cellmap on this cell
		for(unsigned int i = 0; i < fieldCount; ++i) {
			if(unitsWithEmptyCellMap[i] != NULL) {
				XmlNode *unitsWithEmptyCellMapNode = cellNode->addChild("unitsWithEmptyCellMap");
				unitsWithEmptyCellMapNode->addAttribute("field",intToStr(i), mapTagReplacements);
				unitsWithEmptyCellMapNode->addAttribute("unitid",intToStr(unitsWithEmptyCellMap[i]->getId()), mapTagReplacements);
			}
		}

	//	float height;
		cellNode->addAttribute("height",floatToStr(height), mapTagReplacements);
	}
}

void Cell::loadGame(const XmlNode *rootNode, int index, World *world) {
	if(rootNode->hasChild("Cell" + intToStr(index)) == true) {
		const XmlNode *cellNode = rootNode->getChild("Cell" + intToStr(index));

		int unitCount = cellNode->getChildCount();
		for(unsigned int i = 0; i < unitCount; ++i) {
			if(cellNode->hasChildAtIndex("units",i) == true) {
				const XmlNode *unitsNode = cellNode->getChild("units",i);
				int field = unitsNode->getAttribute("field")->getIntValue();
				int unitId = unitsNode->getAttribute("unitid")->getIntValue();
				units[field] = world->findUnitById(unitId);
			}
			if(cellNode->hasChildAtIndex("unitsWithEmptyCellMap",i) == true) {
				const XmlNode *unitsNode = cellNode->getChild("unitsWithEmptyCellMap",i);
				int field = unitsNode->getAttribute("field")->getIntValue();
				int unitId = unitsNode->getAttribute("unitid")->getIntValue();
				unitsWithEmptyCellMap[field] = world->findUnitById(unitId);
			}
		}
	}
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
	cellChangedFromOriginalMapLoad = false;

	for(int i = 0; i < GameConstants::maxPlayers + GameConstants::specialFactions; ++i) {
		visible[i] = false;
		explored[i] = false;
	}
}

SurfaceCell::~SurfaceCell() {
	delete object;
}

void SurfaceCell::end(){
	if(object!=NULL){
		object->end();
	}
}

void SurfaceCell::deleteResource() {
	cellChangedFromOriginalMapLoad = true;

	delete object;
	object= NULL;
}

void SurfaceCell::setHeight(float height, bool cellChangedFromOriginalMapLoadValue) {
	vertex.y= height;
	if(cellChangedFromOriginalMapLoadValue == true) {
		this->cellChangedFromOriginalMapLoad = true;
	}
}

bool SurfaceCell::decAmount(int value) {
	cellChangedFromOriginalMapLoad = true;

	return object->getResource()->decAmount(value);
}
void SurfaceCell::setExplored(int teamIndex, bool explored) {
	this->explored[teamIndex]= explored;
	//printf("Setting explored to %d for teamIndex %d\n",explored,teamIndex);
}

void SurfaceCell::setVisible(int teamIndex, bool visible) {
    this->visible[teamIndex]= visible;
}

void SurfaceCell::saveGame(XmlNode *rootNode,int index) const {
	bool saveCell = (this->getCellChangedFromOriginalMapLoad() == true);

	if(saveCell == true) {
		std::map<string,string> mapTagReplacements;
		XmlNode *surfaceCellNode = rootNode->addChild("SurfaceCell" + intToStr(index));
		surfaceCellNode->addAttribute("index",intToStr(index), mapTagReplacements);

	//	//geometry
	//	Vec3f vertex;
		surfaceCellNode->addAttribute("vertex",vertex.getString(), mapTagReplacements);
	//	Vec3f normal;
		//surfaceCellNode->addAttribute("normal",normal.getString(), mapTagReplacements);
	//	Vec3f color;
		//surfaceCellNode->addAttribute("color",color.getString(), mapTagReplacements);
	//
	//	//tex coords
	//	Vec2f fowTexCoord;		//tex coords for TEXTURE1 when multitexturing and fogOfWar
		//surfaceCellNode->addAttribute("fowTexCoord",fowTexCoord.getString(), mapTagReplacements);
	//	Vec2f surfTexCoord;		//tex coords for TEXTURE0
		//surfaceCellNode->addAttribute("surfTexCoord",surfTexCoord.getString(), mapTagReplacements);
	//	//surface
	//	int surfaceType;
		//surfaceCellNode->addAttribute("surfaceType",intToStr(surfaceType), mapTagReplacements);
	//    const Texture2D *surfaceTexture;
	//
	//	//object & resource
	//	Object *object;
		if(object != NULL) {
			object->saveGame(surfaceCellNode);
		}
		else {
			XmlNode *objectNode = surfaceCellNode->addChild("Object");
			objectNode->addAttribute("isDeleted",intToStr(true), mapTagReplacements);
		}
	//	//visibility
	//	bool visible[GameConstants::maxPlayers + GameConstants::specialFactions];
//		for(unsigned int i = 0; i < GameConstants::maxPlayers; ++i) {
//			if(visible[i] == true) {
//				XmlNode *visibleNode = surfaceCellNode->addChild("visible");
//				visibleNode->addAttribute("index",intToStr(i), mapTagReplacements);
//				visibleNode->addAttribute("value",intToStr(visible[i]), mapTagReplacements);
//			}
//		}
//	//    bool explored[GameConstants::maxPlayers + GameConstants::specialFactions];
//		for(unsigned int i = 0; i < GameConstants::maxPlayers; ++i) {
//			if(explored[i] == true) {
//				XmlNode *exploredNode = surfaceCellNode->addChild("explored");
//				exploredNode->addAttribute("index",intToStr(i), mapTagReplacements);
//				exploredNode->addAttribute("value",intToStr(explored[i]), mapTagReplacements);
//			}
//		}

	//	//cache
	//	bool nearSubmerged;
		//surfaceCellNode->addAttribute("nearSubmerged",intToStr(nearSubmerged), mapTagReplacements);
	}
}

void SurfaceCell::loadGame(const XmlNode *rootNode, int index, World *world) {
	if(rootNode->hasChild("SurfaceCell" + intToStr(index)) == true) {
		const XmlNode *surfaceCellNode = rootNode->getChild("SurfaceCell" + intToStr(index));

		if(surfaceCellNode->hasAttribute("vertex") == true) {
			vertex = Vec3f::strToVec3(surfaceCellNode->getAttribute("vertex")->getValue());
		}

		//int visibleCount = cellNode->getChildCount();
		XmlNode *objectNode = surfaceCellNode->getChild("Object");
		if(objectNode->hasAttribute("isDeleted") == true) {
			this->deleteResource();
		}
		else {
			object->loadGame(surfaceCellNode,world->getTechTree());
		}

		//printf("Loading game, sc index [%d][%d]\n",index,visibleCount);

//		for(unsigned int i = 0; i < visibleCount; ++i) {
//			if(cellNode->hasChildAtIndex("visible",i) == true) {
//				const XmlNode *visibleNode = cellNode->getChild("visible",i);
//				int indexCell = visibleNode->getAttribute("index")->getIntValue();
//				bool value = visibleNode->getAttribute("value")->getIntValue();
//				visible[indexCell] = value;
//
//				//printf("Loading game, sc visible index [%d][%d][%d]\n",index,indexCell,value);
//			}
//			if(cellNode->hasChildAtIndex("explored",i) == true) {
//				const XmlNode *exploredNode = cellNode->getChild("explored",i);
//				int indexCell = exploredNode->getAttribute("index")->getIntValue();
//				bool value = exploredNode->getAttribute("value")->getIntValue();
//				explored[indexCell] = value;
//
//				//printf("Loading game, sc explored cell index [%d] exploredIndex [%d] value [%d]\n",index,indexCell,value);
//			}
//		}
	}
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
	Logger::getInstance().add(Lang::getInstance().get("LogScreenGameUnLoadingMapCells","",true), true);

	delete [] cells;
	cells = NULL;
	delete [] surfaceCells;
	surfaceCells = NULL;
	delete [] startLocations;
	startLocations = NULL;
}

void Map::end(){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
    Logger::getInstance().add(Lang::getInstance().get("LogScreenGameUnLoadingMap","",true), true);
	//read heightmap
	for(int j = 0; j < surfaceH; ++j) {
		for(int i = 0; i < surfaceW; ++i) {
			getSurfaceCell(i, j)->end();
		}
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

Vec2i Map::getStartLocation(int locationIndex) const {
	if(locationIndex >= maxPlayers) {
		char szBuf[4096]="";
		sprintf(szBuf,"locationIndex >= maxPlayers [%d] [%d]",locationIndex, maxPlayers);
		printf("%s\n",szBuf);
		//throw megaglest_runtime_error(szBuf);
		assert(locationIndex < maxPlayers);
	}
	else if(startLocations == NULL) {
		throw megaglest_runtime_error("startLocations == NULL");
	}

	return startLocations[locationIndex];
}

Checksum Map::load(const string &path, TechTree *techTree, Tileset *tileset) {
    Checksum mapChecksum;
	try{
#ifdef WIN32
		FILE *f= _wfopen(utf8_decode(path).c_str(), L"rb");
#else
		FILE *f = fopen(path.c_str(), "rb");
#endif
		if(f != NULL) {
			mapFile = path;

		    mapChecksum.addFile(path);
		    checksumValue.addFile(path);
			//read header
			MapFileHeader header;
			size_t readBytes = fread(&header, sizeof(MapFileHeader), 1, f);
			if(readBytes != 1) {
				throw megaglest_runtime_error("Invalid map header detected for file: " + path);
			}

			if(next2Power(header.width) != header.width){
				throw megaglest_runtime_error("Map width is not a power of 2");
			}

			if(next2Power(header.height) != header.height){
				throw megaglest_runtime_error("Map height is not a power of 2");
			}

			heightFactor= header.heightFactor;
			if(heightFactor>100){
				heightFactor=heightFactor/100;
			}
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
			if(f) fclose(f);

			throw megaglest_runtime_error("Can't open file");
		}
		fclose(f);
	}
	catch(const exception &e){
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
		throw megaglest_runtime_error("Error loading map: "+ path+ "\n"+ e.what());
	}

	return mapChecksum;
}

void Map::init(Tileset *tileset) {
	Logger::getInstance().add(Lang::getInstance().get("LogScreenGameUnLoadingMap","",true), true);
	maxMapHeight=0.0f;
	smoothSurface(tileset);
	computeNormals();
	computeInterpolatedHeights();
	computeNearSubmerged();
	computeCellColors();
}


// ==================== is ====================

class FindBestPos  {
public:
	float distanceFromUnitNoAdjustment;
	float distanceFromClickNoAdjustment;
	Vec2i resourcePosNoAdjustment;
};

//returns if there is a resource next to a unit, in "resourcePos" is stored the relative position of the resource
bool Map::isResourceNear(const Vec2i &pos, const ResourceType *rt, Vec2i &resourcePos,
		int size, Unit *unit, bool fallbackToPeersHarvestingSameResource,
		Vec2i *resourceClickPos) const {

	bool resourceNear = false;
	float distanceFromUnit=-1;
	float distanceFromClick=-1;

	if(resourceClickPos) {
		//printf("+++++++++ unit [%s - %d] pos = [%s] resourceClickPos [%s]\n",unit->getFullName().c_str(),unit->getId(),pos.getString().c_str(),resourceClickPos->getString().c_str());
	}
	for(int i = -size; i <= size; ++i) {
		for(int j = -size; j <= size; ++j) {
			Vec2i resPos = Vec2i(pos.x + i, pos.y + j);
			if(resourceClickPos) {
				resPos = Vec2i(resourceClickPos->x + i, resourceClickPos->y + j);
			}
			Vec2i surfCoords = toSurfCoords(resPos);

			if(isInside(resPos) && isInsideSurface(surfCoords)) {
				Resource *r= getSurfaceCell(surfCoords)->getResource();
				if(r != NULL) {
					if(r->getType() == rt) {
						if(resourceClickPos) {
							//printf("****** unit [%s - %d] resPos = [%s] resourceClickPos->dist(resPos) [%f] distanceFromClick [%f] unit->getCenteredPos().dist(resPos) [%f] distanceFromUnit [%f]\n",unit->getFullName().c_str(),unit->getId(),resPos.getString().c_str(),resourceClickPos->dist(resPos),distanceFromClick,unit->getCenteredPos().dist(resPos),distanceFromUnit);
						}
						if(resourceClickPos == NULL ||
							(distanceFromClick < 0 || resourceClickPos->dist(resPos) <= distanceFromClick)) {
							if(unit == NULL ||
								(distanceFromUnit < 0 || unit->getCenteredPos().dist(resPos) <= distanceFromUnit)) {

								bool isResourceNextToUnit = (resourceClickPos == NULL);
								for(int i1 = -size; isResourceNextToUnit == false && i1 <= size; ++i1) {
									for(int j1 = -size; j1 <= size; ++j1) {
										Vec2i resPos1 = Vec2i(pos.x + i1, pos.y + j1);
										if(resPos == resPos1) {
											isResourceNextToUnit = true;
											break;
										}
									}
								}
								if(isResourceNextToUnit == true) {
									if(resourceClickPos != NULL) {
										distanceFromClick = resourceClickPos->dist(resPos);
									}
									if(unit != NULL) {
										distanceFromUnit = unit->getCenteredPos().dist(resPos);
									}

									resourcePos= pos + Vec2i(i,j);

									if(unit == NULL || unit->isBadHarvestPos(resourcePos) == false) {
										resourceNear = true;

										if(resourceClickPos) {
											//printf("@@@@@@@@ unit [%s - %d] resPos = [%s] resourceClickPos->dist(resPos) [%f] distanceFromClick [%f] unit->getCenteredPos().dist(resPos) [%f] distanceFromUnit [%f]\n",unit->getFullName().c_str(),unit->getId(),resPos.getString().c_str(),resourceClickPos->dist(resPos),distanceFromClick,unit->getCenteredPos().dist(resPos),distanceFromUnit);
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if(resourceNear == false) {
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
					resourceNear = true;

					if(resourceClickPos) {
						//printf("###### unit [%s - %d]\n",unit->getFullName().c_str(),unit->getId());
					}
				}
			}
		}
	}

	if(resourceNear == false && resourceClickPos != NULL) {
		std::vector<FindBestPos> bestPosList;

		//if(resourceClickPos) {
			//printf("^^^^^ unit [%s - %d]\n",unit->getFullName().c_str(),unit->getId());
		//}

		for(int i = -1; i <= 1; ++i) {
			for(int j = -1; j <= 1; ++j) {
				Vec2i resPos = Vec2i(resourceClickPos->x + i, resourceClickPos->y + j);
				Vec2i surfCoords = toSurfCoords(resPos);

				if(isInside(resPos) && isInsideSurface(surfCoords)) {
					Resource *r= getSurfaceCell(surfCoords)->getResource();
					if(r != NULL) {
						if(r->getType() == rt) {
							//printf("^^^^^^ unit [%s - %d] resPos = [%s] resourceClickPos->dist(resPos) [%f] distanceFromClick [%f] unit->getCenteredPos().dist(resPos) [%f] distanceFromUnit [%f]\n",unit->getFullName().c_str(),unit->getId(),resPos.getString().c_str(),resourceClickPos->dist(resPos),distanceFromClick,unit->getCenteredPos().dist(resPos),distanceFromUnit);

							if(unit == NULL ||
								(distanceFromUnit < 0 || unit->getCenteredPos().dist(resPos) <= (distanceFromUnit + 2.0))) {

								if(resourceClickPos->dist(resPos) <= 1.0) {
									FindBestPos bestPosItem;

									bestPosItem.distanceFromUnitNoAdjustment = unit->getCenteredPos().dist(resPos);
									bestPosItem.distanceFromClickNoAdjustment =distanceFromClick = resourceClickPos->dist(resPos);
									bestPosItem.resourcePosNoAdjustment = resPos;

									bestPosList.push_back(bestPosItem);
								}

								//printf("!!!! unit [%s - %d] resPos = [%s] resourceClickPos->dist(resPos) [%f] distanceFromClick [%f] unit->getCenteredPos().dist(resPos) [%f] distanceFromUnit [%f]\n",unit->getFullName().c_str(),unit->getId(),resPos.getString().c_str(),resourceClickPos->dist(resPos),distanceFromClick,unit->getCenteredPos().dist(resPos),distanceFromUnit);
								if(distanceFromClick < 0 || resourceClickPos->dist(resPos) <= distanceFromClick) {
									if(resourceClickPos != NULL) {
										distanceFromClick = resourceClickPos->dist(resPos);
									}
									if(unit != NULL) {
										distanceFromUnit = unit->getCenteredPos().dist(resPos);
									}

									*resourceClickPos = resPos;

									if(unit == NULL || unit->isBadHarvestPos(*resourceClickPos) == false) {
										//resourceNear = true;

										//printf("%%----------- unit [%s - %d] resPos = [%s] resourceClickPos->dist(resPos) [%f] distanceFromClick [%f] unit->getCenteredPos().dist(resPos) [%f] distanceFromUnit [%f]\n",unit->getFullName().c_str(),unit->getId(),resPos.getString().c_str(),resourceClickPos->dist(resPos),distanceFromClick,unit->getCenteredPos().dist(resPos),distanceFromUnit);
									}
								}
							}
						}
					}
				}
			}
		}

		float bestUnitDist = distanceFromUnit;
		for(unsigned int i = 0; i < bestPosList.size(); ++i) {
			FindBestPos &bestPosItem = bestPosList[i];

			if(bestPosItem.distanceFromUnitNoAdjustment < bestUnitDist) {
				bestUnitDist = bestPosItem.distanceFromUnitNoAdjustment;
				*resourceClickPos = bestPosItem.resourcePosNoAdjustment;

				if(unit == NULL || unit->isBadHarvestPos(*resourceClickPos) == false) {
					//printf("%%----------- unit [%s - %d] resourceClickPos [%s] bestUnitDist [%f]\n",unit->getFullName().c_str(),unit->getId(),resourceClickPos->getString().c_str(),bestUnitDist);
				}
			}
		}
	}

	return resourceNear;
}

//returns if there is a resource next to a unit, in "resourcePos" is stored the relative position of the resource
bool Map::isResourceNear(const Vec2i &pos, int size, const ResourceType *rt, Vec2i &resourcePos) const {
	Vec2i p1 = pos + Vec2i(-size);
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
		if(unit->getCurrField() != field) {
			return isFreeCell(pos, field);
		}
		Cell *c= getCell(pos);
		if(c->getUnit(unit->getCurrField()) == unit) {
			if(unit->getCurrField() == fAir) {
				if(field == fAir) {
					return true;
				}
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

	//printf("[%s] Line: %d returning false\n",__FUNCTION__,__LINE__);
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
		throw megaglest_runtime_error("unit == NULL");
	}
	if(munit == NULL) {
		throw megaglest_runtime_error("munit == NULL");
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

	bool isBadHarvestPos = false;
	if(unit != NULL) {
		Command *command= unit->getCurrCommand();
		if(command != NULL) {
			const HarvestCommandType *hct = dynamic_cast<const HarvestCommandType*>(command->getCommandType());
			if(hct != NULL && unit->isBadHarvestPos(pos2) == true) {
				isBadHarvestPos = true;
			}
		}
	}

	if(unit == NULL || isBadHarvestPos == true) {
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

		//printf("[%s] Line: %d returning false\n",__FUNCTION__,__LINE__);
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
							if(iterFind5->second == false) {
								//printf("[%s] Line: %d returning false\n",__FUNCTION__,__LINE__);
							}
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

			//printf("[%s] Line: %d returning false\n",__FUNCTION__,__LINE__);
			return false;
		}
		if(pos1.x != pos2.x && pos1.y != pos2.y) {
			if(isAproxFreeCell(Vec2i(pos1.x, pos2.y), field, teamIndex) == false) {
				if(lookupCache != NULL) {
					(*lookupCache)[pos1][pos2][teamIndex][size][field]=false;
				}

				//Unit *cellUnit = getCell(Vec2i(pos1.x, pos2.y))->getUnit(field);
				//Object * obj = getSurfaceCell(toSurfCoords(Vec2i(pos1.x, pos2.y)))->getObject();

				//printf("[%s] Line: %d returning false cell [%s] free [%d] cell unitid = %d object class = %d\n",__FUNCTION__,__LINE__,Vec2i(pos1.x, pos2.y).getString().c_str(),this->isFreeCell(Vec2i(pos1.x, pos2.y),field),(cellUnit != NULL ? cellUnit->getId() : -1),(obj != NULL ? obj->getType()->getClass() : -1));
				//printf("[%s] Line: %d returning false\n",__FUNCTION__,__LINE__);
				return false;
			}
			if(isAproxFreeCell(Vec2i(pos2.x, pos1.y), field, teamIndex) == false) {
				if(lookupCache != NULL) {
					(*lookupCache)[pos1][pos2][teamIndex][size][field]=false;
				}

				//printf("[%s] Line: %d returning false\n",__FUNCTION__,__LINE__);
				return false;
			}
		}

		bool isBadHarvestPos = false;
		if(unit != NULL) {
			Command *command= unit->getCurrCommand();
			if(command != NULL) {
				const HarvestCommandType *hct = dynamic_cast<const HarvestCommandType*>(command->getCommandType());
				if(hct != NULL && unit->isBadHarvestPos(pos2) == true) {
					isBadHarvestPos = true;
				}
			}
		}

		if(unit == NULL || isBadHarvestPos == true) {
			if(lookupCache != NULL) {
				(*lookupCache)[pos1][pos2][teamIndex][size][field]=false;
			}

			//printf("[%s] Line: %d returning false\n",__FUNCTION__,__LINE__);
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

							//printf("[%s] Line: %d returning false\n",__FUNCTION__,__LINE__);
							return false;
						}
					}
				}
				else {

					if(lookupCache != NULL) {
						(*lookupCache)[pos1][pos2][teamIndex][size][field]=false;
					}

					//printf("[%s] Line: %d returning false\n",__FUNCTION__,__LINE__);
					return false;
				}
			}
		}

		bool isBadHarvestPos = false;
		if(unit != NULL) {
			Command *command= unit->getCurrCommand();
			if(command != NULL) {
				const HarvestCommandType *hct = dynamic_cast<const HarvestCommandType*>(command->getCommandType());
				if(hct != NULL && unit->isBadHarvestPos(pos2) == true) {
					isBadHarvestPos = true;
				}
			}
		}

		if(unit == NULL || isBadHarvestPos == true) {
			if(lookupCache != NULL) {
				(*lookupCache)[pos1][pos2][teamIndex][size][field]=false;
			}

			//printf("[%s] Line: %d returning false\n",__FUNCTION__,__LINE__);
			return false;
		}

		if(lookupCache != NULL) {
			(*lookupCache)[pos1][pos2][teamIndex][size][field]=true;
		}
	}
	return true;
}

//checks if a unit can move from between 2 cells using only visible cells (for pathfinding)
bool Map::aproxCanMoveSoon(const Unit *unit, const Vec2i &pos1, const Vec2i &pos2) const {
	if(isInside(pos1) == false || isInsideSurface(toSurfCoords(pos1)) == false ||
	   isInside(pos2) == false || isInsideSurface(toSurfCoords(pos2)) == false) {

		//printf("[%s] Line: %d returning false\n",__FUNCTION__,__LINE__);
		return false;
	}

	int size= unit->getType()->getSize();
	int teamIndex= unit->getTeam();
	Field field= unit->getCurrField();

	//single cell units
	if(size == 1) {
		if(isAproxFreeCellOrMightBeFreeSoon(unit->getPos(),pos2, field, teamIndex) == false) {

			//printf("[%s] Line: %d returning false\n",__FUNCTION__,__LINE__);
			return false;
		}
		if(pos1.x != pos2.x && pos1.y != pos2.y) {
			if(isAproxFreeCellOrMightBeFreeSoon(unit->getPos(),Vec2i(pos1.x, pos2.y), field, teamIndex) == false) {

				//Unit *cellUnit = getCell(Vec2i(pos1.x, pos2.y))->getUnit(field);
				//Object * obj = getSurfaceCell(toSurfCoords(Vec2i(pos1.x, pos2.y)))->getObject();

				//printf("[%s] Line: %d returning false cell [%s] free [%d] cell unitid = %d object class = %d\n",__FUNCTION__,__LINE__,Vec2i(pos1.x, pos2.y).getString().c_str(),this->isFreeCell(Vec2i(pos1.x, pos2.y),field),(cellUnit != NULL ? cellUnit->getId() : -1),(obj != NULL ? obj->getType()->getClass() : -1));
				//printf("[%s] Line: %d returning false\n",__FUNCTION__,__LINE__);
				return false;
			}
			if(isAproxFreeCellOrMightBeFreeSoon(unit->getPos(),Vec2i(pos2.x, pos1.y), field, teamIndex) == false) {
				//printf("[%s] Line: %d returning false\n",__FUNCTION__,__LINE__);
				return false;
			}
		}

		bool isBadHarvestPos = false;
		if(unit != NULL) {
			Command *command= unit->getCurrCommand();
			if(command != NULL) {
				const HarvestCommandType *hct = dynamic_cast<const HarvestCommandType*>(command->getCommandType());
				if(hct != NULL && unit->isBadHarvestPos(pos2) == true) {
					isBadHarvestPos = true;
				}
			}
		}

		if(unit == NULL || isBadHarvestPos == true) {

			//printf("[%s] Line: %d returning false\n",__FUNCTION__,__LINE__);
			return false;
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
						if(isAproxFreeCellOrMightBeFreeSoon(unit->getPos(),cellPos, field, teamIndex) == false) {

							//printf("[%s] Line: %d returning false\n",__FUNCTION__,__LINE__);
							return false;
						}
					}
				}
				else {

					//printf("[%s] Line: %d returning false\n",__FUNCTION__,__LINE__);
					return false;
				}
			}
		}

		bool isBadHarvestPos = false;
		if(unit != NULL) {
			Command *command= unit->getCurrCommand();
			if(command != NULL) {
				const HarvestCommandType *hct = dynamic_cast<const HarvestCommandType*>(command->getCommandType());
				if(hct != NULL && unit->isBadHarvestPos(pos2) == true) {
					isBadHarvestPos = true;
				}
			}
		}

		if(unit == NULL || isBadHarvestPos == true) {

			//printf("[%s] Line: %d returning false\n",__FUNCTION__,__LINE__);
			return false;
		}

	}
	return true;
}

Vec2i Map::computeRefPos(const Selection *selection) const {
    Vec2i total= Vec2i(0);
    for(int i = 0; i < selection->getCount(); ++i) {
    	if(selection == NULL || selection->getUnit(i) == NULL) {
    		throw megaglest_runtime_error("selection == NULL || selection->getUnit(i) == NULL");
    	}
        total = total + selection->getUnit(i)->getPos();
    }

    return Vec2i(total.x / selection->getCount(), total.y / selection->getCount());
}

Vec2i Map::computeDestPos(	const Vec2i &refUnitPos, const Vec2i &unitPos,
							const Vec2i &commandPos) const {
    Vec2i pos;
// no more random needed
//	Vec2i posDiff = unitPos - refUnitPos;
//
//	if(abs(posDiff.x) >= 3){
//		posDiff.x = posDiff.x % 3;
//	}
//
//	if(abs(posDiff.y) >= 3){
//		posDiff.y = posDiff.y % 3;
//	}

	pos = commandPos; //+ posDiff;
    clampPos(pos);
    return pos;
}

std::pair<float,Vec2i> Map::getUnitDistanceToPos(const Unit *unit,Vec2i pos,const UnitType *ut) {
	if(unit == NULL) {
		throw megaglest_runtime_error("unit == NULL");
	}

	std::pair<float,Vec2i> result(-1,Vec2i(0));
	//int unitId= unit->getId();
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
		//int unitId= unit->getId();
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

	Vec2i start = pos - Vec2i(unit->getType()->getSize());
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
		//Cell *testCell = getCell(testPos);
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
		throw megaglest_runtime_error("ut == NULL");
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
		throw megaglest_runtime_error("ut == NULL");
	}

    bool canPutInCell = true;
	const UnitType *ut= unit->getType();

	for(int i = 0; i < ut->getSize(); ++i) {
		for(int j = 0; j < ut->getSize(); ++j) {
			Vec2i currPos= pos + Vec2i(i, j);
			assert(isInside(currPos));
			if(isInside(currPos) == false) {
				throw megaglest_runtime_error("isInside(currPos) == false");
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
				        // throw megaglest_runtime_error("getCell(currPos)->getUnit(unit->getCurrField()) != NULL");
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
		throw megaglest_runtime_error("unit == NULL");
	}

	const UnitType *ut= unit->getType();

	for(int i=0; i<ut->getSize(); ++i){
		for(int j=0; j<ut->getSize(); ++j){
			Vec2i currPos= pos + Vec2i(i, j);
			assert(isInside(currPos));
			if(isInside(currPos) == false) {
				throw megaglest_runtime_error("isInside(currPos) == false");
			}

			if(ut->hasCellMap() == false || ut->getCellMapCell(i, j, unit->getModelFacing())) {
				// This seems to be a bad assert since you can clear the cell
				// for many reasons including a unit dieing.

				//assert(getCell(currPos)->getUnit(unit->getCurrField()) == unit || getCell(currPos)->getUnit(unit->getCurrField()) == NULL);
				//if(getCell(currPos)->getUnit(unit->getCurrField()) != unit && getCell(currPos)->getUnit(unit->getCurrField()) != NULL) {
				//	throw megaglest_runtime_error("getCell(currPos)->getUnit(unit->getCurrField()) != unit");
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
bool Map::isNextTo(const Unit *unit1, const Unit *unit2) const {
	Vec2i pos = unit1->getPos();
	const UnitType *ut = unit1->getType();
	for (int y=-1; y < ut->getSize()+1; ++y) {
		for (int x=-1; x < ut->getSize()+1; ++x) {
			Vec2i cellPos = pos + Vec2i(x, y);
			if(isInside(cellPos) && isInsideSurface(toSurfCoords(cellPos))) {
				if(getCell(cellPos)->getUnit(fLand) == unit2) {
					return true;
				}
				else if(getCell(cellPos)->getUnitWithEmptyCellMap(fLand) == unit2) {
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
					sc->setHeight(refHeight,true);
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
	//int arraySize=getSurfaceCellArraySize();

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
					if (cliffLevel<=0.1f || cliffLevel > streflop::fabs(static_cast<streflop::Simple>(oldHeights[(j) * surfaceW + (i)]
							- oldHeights[(j + k) * surfaceW + (i + l)]))) {
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
		//abort();
		throw megaglest_runtime_error("Map not found [" + mapName + "]\nScenario [" + scenarioDir + "]");
	}

	return "";
}

void Map::saveGame(XmlNode *rootNode) const {
	std::map<string,string> mapTagReplacements;
	XmlNode *mapNode = rootNode->addChild("Map");

//	string title;
	mapNode->addAttribute("title",title, mapTagReplacements);
//	float waterLevel;
	mapNode->addAttribute("waterLevel",floatToStr(waterLevel), mapTagReplacements);
//	float heightFactor;
	mapNode->addAttribute("heightFactor",floatToStr(heightFactor), mapTagReplacements);
//	float cliffLevel;
	mapNode->addAttribute("cliffLevel",floatToStr(cliffLevel), mapTagReplacements);
//	int cameraHeight;
	mapNode->addAttribute("cameraHeight",intToStr(cameraHeight), mapTagReplacements);
//	int w;
	mapNode->addAttribute("w",intToStr(w), mapTagReplacements);
//	int h;
	mapNode->addAttribute("h",intToStr(h), mapTagReplacements);
//	int surfaceW;
	mapNode->addAttribute("surfaceW",intToStr(surfaceW), mapTagReplacements);
//	int surfaceH;
	mapNode->addAttribute("surfaceH",intToStr(surfaceH), mapTagReplacements);
//	int maxPlayers;
	mapNode->addAttribute("maxPlayers",intToStr(maxPlayers), mapTagReplacements);
//	Cell *cells;
	//printf("getCellArraySize() = %d\n",getCellArraySize());
//	for(unsigned int i = 0; i < getCellArraySize(); ++i) {
//		Cell &cell = cells[i];
//		cell.saveGame(mapNode,i);
//	}
//	SurfaceCell *surfaceCells;
	//printf("getSurfaceCellArraySize() = %d\n",getSurfaceCellArraySize());

	string exploredList = "";
	string visibleList = "";

	for(unsigned int i = 0; i < getSurfaceCellArraySize(); ++i) {
		SurfaceCell &surfaceCell = surfaceCells[i];

		if(exploredList != "") {
			exploredList += ",";
		}

		for(unsigned int j = 0; j < GameConstants::maxPlayers; ++j) {
			if(j > 0) {
				exploredList += "|";
			}

			exploredList += intToStr(surfaceCell.isExplored(j));
		}

		if(visibleList != "") {
			visibleList += ",";
		}

		for(unsigned int j = 0; j < GameConstants::maxPlayers; ++j) {
			if(j > 0) {
				visibleList += "|";
			}

			visibleList += intToStr(surfaceCell.isVisible(j));
		}

		surfaceCell.saveGame(mapNode,i);

		if(i > 0 && i % 100 == 0) {
			XmlNode *surfaceCellNode = mapNode->addChild("SurfaceCell");
			surfaceCellNode->addAttribute("batchIndex",intToStr(i), mapTagReplacements);
			surfaceCellNode->addAttribute("exploredList",exploredList, mapTagReplacements);
			surfaceCellNode->addAttribute("visibleList",visibleList, mapTagReplacements);

			exploredList = "";
			visibleList = "";
		}
	}

	if(exploredList != "") {
		XmlNode *surfaceCellNode = mapNode->addChild("SurfaceCell");
		surfaceCellNode->addAttribute("batchIndex",intToStr(getSurfaceCellArraySize()), mapTagReplacements);
		surfaceCellNode->addAttribute("exploredList",exploredList, mapTagReplacements);
		surfaceCellNode->addAttribute("visibleList",visibleList, mapTagReplacements);
	}

//	Vec2i *startLocations;
	for(unsigned int i = 0; i < maxPlayers; ++i) {
		XmlNode *startLocationsNode = mapNode->addChild("startLocations");
		startLocationsNode->addAttribute("location",startLocations[i].getString(), mapTagReplacements);
	}
//	Checksum checksumValue;
//	mapNode->addAttribute("checksumValue",intToStr(checksumValue.getSum()), mapTagReplacements);
//	float maxMapHeight;
	mapNode->addAttribute("maxMapHeight",floatToStr(maxMapHeight), mapTagReplacements);
//	string mapFile;
	mapNode->addAttribute("mapFile",mapFile, mapTagReplacements);
}

void Map::loadGame(const XmlNode *rootNode, World *world) {
	const XmlNode *mapNode = rootNode->getChild("Map");

	//description = gameSettingsNode->getAttribute("description")->getValue();

//	for(unsigned int i = 0; i < getCellArraySize(); ++i) {
//		Cell &cell = cells[i];
//		cell.saveGame(mapNode,i);
//	}
//	for(unsigned int i = 0; i < getSurfaceCellArraySize(); ++i) {
//		SurfaceCell &surfaceCell = surfaceCells[i];
//		surfaceCell.saveGame(mapNode,i);
//	}

//	printf("getCellArraySize() = %d\n",getCellArraySize());
//	for(unsigned int i = 0; i < getCellArraySize(); ++i) {
//		Cell &cell = cells[i];
//		cell.loadGame(mapNode,i,world);
//	}

//	printf("getSurfaceCellArraySize() = %d\n",getSurfaceCellArraySize());
	for(unsigned int i = 0; i < getSurfaceCellArraySize(); ++i) {
		SurfaceCell &surfaceCell = surfaceCells[i];
		surfaceCell.loadGame(mapNode,i,world);
	}

	int surfaceCellIndexExplored = 0;
	int surfaceCellIndexVisible = 0;
	vector<XmlNode *> surfaceCellNodeList = mapNode->getChildList("SurfaceCell");
	for(unsigned int i = 0; i < surfaceCellNodeList.size(); ++i) {
		XmlNode *surfaceCellNode = surfaceCellNodeList[i];

		//XmlNode *surfaceCellNode = mapNode->getChild("SurfaceCell");
		string exploredList = surfaceCellNode->getAttribute("exploredList")->getValue();
		string visibleList = surfaceCellNode->getAttribute("visibleList")->getValue();
		//int batchIndex = surfaceCellNode->getAttribute("batchIndex")->getIntValue();

		vector<string> tokensExplored;
		Tokenize(exploredList,tokensExplored,",");

		//printf("=====================\nNew batchIndex = %d batchsize = %d\n",batchIndex,tokensExplored.size());
		//for(unsigned int j = 0; j < tokensExplored.size(); ++j) {
			//string valueList = tokensExplored[j];
			//printf("valueList [%s]\n",valueList.c_str());
		//}
		for(unsigned int j = 0; j < tokensExplored.size(); ++j) {
			string valueList = tokensExplored[j];

			//int surfaceCellIndex = (i * tokensExplored.size()) + j;
			//printf("Loading sc = %d batchIndex = %d\n",surfaceCellIndexExplored,batchIndex);
			SurfaceCell &surfaceCell = surfaceCells[surfaceCellIndexExplored];

			vector<string> tokensExploredValue;
			Tokenize(valueList,tokensExploredValue,"|");

//			if(tokensExploredValue.size() != GameConstants::maxPlayers) {
//				for(unsigned int k = 0; k < tokensExploredValue.size(); ++k) {
//					string value = tokensExploredValue[k];
//					printf("k = %d [%s]\n",k,value.c_str());
//				}
//				throw megaglest_runtime_error("tokensExploredValue.size() [" + intToStr(tokensExploredValue.size()) + "] != GameConstants::maxPlayers");
//			}
			for(unsigned int k = 0; k < tokensExploredValue.size(); ++k) {
				string value = tokensExploredValue[k];

				surfaceCell.setExplored(k,strToInt(value) != 0);
			}
			surfaceCellIndexExplored++;
		}

		vector<string> tokensVisible;
		Tokenize(visibleList,tokensVisible,",");
		for(unsigned int j = 0; j < tokensVisible.size(); ++j) {
			string valueList = tokensVisible[j];

			//int surfaceCellIndex = (i * tokensVisible.size()) + j;
			SurfaceCell &surfaceCell = surfaceCells[surfaceCellIndexVisible];

			vector<string> tokensVisibleValue;
			Tokenize(valueList,tokensVisibleValue,"|");

//			if(tokensVisibleValue.size() != GameConstants::maxPlayers) {
//				throw megaglest_runtime_error("tokensVisibleValue.size() [" + intToStr(tokensVisibleValue.size()) + "] != GameConstants::maxPlayers");
//			}

			for(unsigned int k = 0; k < tokensVisibleValue.size(); ++k) {
				string value = tokensVisibleValue[k];

				surfaceCell.setVisible(k,strToInt(value) != 0);
			}
			surfaceCellIndexVisible++;
		}
	}

    computeNormals();
	computeInterpolatedHeights();
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
	while(streflop::floor(static_cast<streflop::Simple>(pos.dist(center))) >= (radius+1) || !map->isInside(pos) || !map->isInsideSurface(map->toSurfCoords(pos)) );
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
