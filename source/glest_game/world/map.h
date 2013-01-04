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

#ifndef _GLEST_GAME_MAP_H_
#define _GLEST_GAME_MAP_H_

#ifdef WIN32
    #include <winsock2.h>
    #include <winsock.h>
#endif

#include "vec.h"
#include "math_util.h"
#include "command_type.h"
#include "logger.h"
#include "object.h"
#include "game_constants.h"
#include "selection.h"
#include <cassert>
#include "unit_type.h"
#include "command.h"
#include "checksum.h"
#include "leak_dumper.h"


namespace Glest{ namespace Game{

using Shared::Graphics::Vec4f;
using Shared::Graphics::Quad2i;
using Shared::Graphics::Rect2i;
using Shared::Graphics::Vec4f;
using Shared::Graphics::Vec2f;
using Shared::Graphics::Vec2i;
using Shared::Graphics::Texture2D;

class Tileset;
class Unit;
class Resource;
class TechTree;
class GameSettings;
class World;

// =====================================================
// 	class Cell
//
///	A map cell that holds info about units present on it
// =====================================================

class Cell {
private:
    Unit *units[fieldCount];	//units on this cell
    Unit *unitsWithEmptyCellMap[fieldCount];	//units with an empty cellmap on this cell
	float height;

private:
	Cell(Cell&);
	void operator=(Cell&);

public:
	Cell();

	//get
	inline Unit *getUnit(int field) const		{ if(field >= fieldCount) { throw megaglest_runtime_error("Invalid field value" + intToStr(field));} return units[field];}
	inline Unit *getUnitWithEmptyCellMap(int field) const		{ if(field >= fieldCount) { throw megaglest_runtime_error("Invalid field value" + intToStr(field));} return unitsWithEmptyCellMap[field];}
	inline float getHeight() const				{return height;}

	inline void setUnit(int field, Unit *unit)	{ if(field >= fieldCount) { throw megaglest_runtime_error("Invalid field value" + intToStr(field));} units[field]= unit;}
	inline void setUnitWithEmptyCellMap(int field, Unit *unit)	{ if(field >= fieldCount) { throw megaglest_runtime_error("Invalid field value" + intToStr(field));} unitsWithEmptyCellMap[field]= unit;}
	inline void setHeight(float height)		{this->height= height;}

	inline bool isFree(Field field) const {
		Unit *unit = getUnit(field);
		bool result = (unit == NULL || unit->isPutrefacting());

		if(result == false) {
			//printf("[%s] Line: %d returning false, unit id = %d [%s]\n",__FUNCTION__,__LINE__,getUnit(field)->getId(),getUnit(field)->getType()->getName().c_str());
		}

		return result;
	}

	inline bool isFreeOrMightBeFreeSoon(Vec2i originPos, Vec2i cellPos, Field field) const {
		Unit *unit = getUnit(field);
		bool result = (unit == NULL || unit->isPutrefacting());

		if(result == false) {
			if(originPos.dist(cellPos) > 5 && unit->getType()->isMobile() == true) {
				result = true;
			}

			//printf("[%s] Line: %d returning false, unit id = %d [%s]\n",__FUNCTION__,__LINE__,getUnit(field)->getId(),getUnit(field)->getType()->getName().c_str());
		}

		return result;
	}

	void saveGame(XmlNode *rootNode,int index) const;
	void loadGame(const XmlNode *rootNode, int index, World *world);
};

// =====================================================
// 	class SurfaceCell
//
//	A heightmap cell, each surface cell is composed by more than one Cell
// =====================================================

class SurfaceCell {
private:
	//geometry
	Vec3f vertex;
	Vec3f normal;
	Vec3f color;

	//tex coords
	Vec2f fowTexCoord;		//tex coords for TEXTURE1 when multitexturing and fogOfWar
	Vec2f surfTexCoord;		//tex coords for TEXTURE0

	//surface
	int surfaceType;
    const Texture2D *surfaceTexture;

	//object & resource
	Object *object;

	//visibility
	bool visible[GameConstants::maxPlayers + GameConstants::specialFactions];
    bool explored[GameConstants::maxPlayers + GameConstants::specialFactions];

	//cache
	bool nearSubmerged;
	bool cellChangedFromOriginalMapLoad;

public:
	SurfaceCell();
	~SurfaceCell();

	void end(); //to kill particles
	//get
	inline const Vec3f &getVertex() const				{return vertex;}
	inline float getHeight() const						{return vertex.y;}
	inline const Vec3f &getColor() const				{return color;}
	inline const Vec3f &getNormal() const				{return normal;}
	inline int getSurfaceType() const					{return surfaceType;}
	inline const Texture2D *getSurfaceTexture() const	{return surfaceTexture;}
	inline Object *getObject() const					{return object;}
	inline Resource *getResource() const				{return object==NULL? NULL: object->getResource();}
	inline const Vec2f &getFowTexCoord() const			{return fowTexCoord;}
	inline const Vec2f &getSurfTexCoord() const		{return surfTexCoord;}
	inline bool getNearSubmerged() const				{return nearSubmerged;}

	inline bool isVisible(int teamIndex) const		{return visible[teamIndex];}
	inline bool isExplored(int teamIndex) const		{return explored[teamIndex];}

	//set
	inline void setVertex(const Vec3f &vertex)			{this->vertex= vertex;}
	inline void setHeight(float height, bool cellChangedFromOriginalMapLoadValue=false);
	inline void setNormal(const Vec3f &normal)			{this->normal= normal;}
	inline void setColor(const Vec3f &color)			{this->color= color;}
	inline void setSurfaceType(int surfaceType)		{this->surfaceType= surfaceType;}
	inline void setSurfaceTexture(const Texture2D *st)	{this->surfaceTexture= st;}
	inline void setObject(Object *object)				{this->object= object;}
	inline void setFowTexCoord(const Vec2f &ftc)		{this->fowTexCoord= ftc;}
	inline void setSurfTexCoord(const Vec2f &stc)		{this->surfTexCoord= stc;}
	void setExplored(int teamIndex, bool explored);
    void setVisible(int teamIndex, bool visible);
    inline void setNearSubmerged(bool nearSubmerged)	{this->nearSubmerged= nearSubmerged;}

	//misc
	void deleteResource();
	bool decAmount(int value);
	inline bool isFree() const {
		bool result = (object==NULL || object->getWalkable());

		if(result == false) {
			//printf("[%s] Line: %d returning false\n",__FUNCTION__,__LINE__);
		}
		return result;
	}
	bool getCellChangedFromOriginalMapLoad() const { return cellChangedFromOriginalMapLoad; }

	void saveGame(XmlNode *rootNode,int index) const;
	void loadGame(const XmlNode *rootNode, int index, World *world);
};


// =====================================================
// 	class Map
//
///	Represents the game map (and loads it from a gbm file)
// =====================================================

class FastAINodeCache {
public:
	FastAINodeCache(Unit *unit) {
		this->unit = unit;
	}
	Unit *unit;
	std::map<Vec2i,std::map<Vec2i,bool> > cachedCanMoveSoonList;
};

class Map {
public:
	static const int cellScale;	//number of cells per surfaceCell
	static const int mapScale;	//horizontal scale of surface

private:
	string title;
	float waterLevel;
	float heightFactor;
	float cliffLevel;
	int cameraHeight;
	int w;
	int h;
	int surfaceW;
	int surfaceH;
	int surfaceSize;

	int maxPlayers;
	Cell *cells;
	SurfaceCell *surfaceCells;
	Vec2i *startLocations;
	Checksum checksumValue;
	float maxMapHeight;
	string mapFile;

private:
	Map(Map&);
	void operator=(Map&);

public:
	Map();
	~Map();
	void end(); //to kill particles
	Checksum * getChecksumValue() { return &checksumValue; }

	void init(Tileset *tileset);
	Checksum load(const string &path, TechTree *techTree, Tileset *tileset);

	//get
	inline Cell *getCell(int x, int y, bool errorOnInvalid=true) const {
		int arrayIndex = y * w + x;
		if(arrayIndex < 0 || arrayIndex >= getCellArraySize()) {
			if(errorOnInvalid == false) {
				return NULL;
			}
			//abort();
			throw megaglest_runtime_error("arrayIndex >= getCellArraySize(), arrayIndex = " + intToStr(arrayIndex) + " w = " + intToStr(w) + " h = " + intToStr(h));
		}
		else if(cells == NULL) {
			if(errorOnInvalid == false) {
				return NULL;
			}

			throw megaglest_runtime_error("cells == NULL");
		}

		return &cells[arrayIndex];
	}
	inline Cell *getCell(const Vec2i &pos) const {
		return getCell(pos.x, pos.y);
	}

	inline int getCellArraySize() const {
		return (w * h);
	}
	inline int getSurfaceCellArraySize() const {
		//return (surfaceW * surfaceH);
		return surfaceSize;
	}
	inline SurfaceCell *getSurfaceCell(int sx, int sy) const {
		int arrayIndex = sy * surfaceW + sx;
		if(arrayIndex < 0 || arrayIndex >= getSurfaceCellArraySize()) {
			throw megaglest_runtime_error("arrayIndex >= getSurfaceCellArraySize(), arrayIndex = " + intToStr(arrayIndex) +
					            " surfaceW = " + intToStr(surfaceW) + " surfaceH = " + intToStr(surfaceH) +
					            " sx: " + intToStr(sx) + " sy: " + intToStr(sy));
		}
		else if(surfaceCells == NULL) {
			throw megaglest_runtime_error("surfaceCells == NULL");
		}
		return &surfaceCells[arrayIndex];
	}
	inline SurfaceCell *getSurfaceCell(const Vec2i &sPos) const {
		return getSurfaceCell(sPos.x, sPos.y);
	}

	inline int getW() const											{return w;}
	inline int getH() const											{return h;}
	inline int getSurfaceW() const										{return surfaceW;}
	inline int getSurfaceH() const										{return surfaceH;}
	inline int getMaxPlayers() const										{return maxPlayers;}
	inline float getHeightFactor() const								{return heightFactor;}
	inline float getWaterLevel() const									{return waterLevel;}
	inline float getCliffLevel() const									{return cliffLevel;}
	inline int getCameraHeight() const									{return cameraHeight;}
	inline float getMaxMapHeight() const								{return maxMapHeight;}
	Vec2i getStartLocation(int locationIndex) const;
	inline bool getSubmerged(const SurfaceCell *sc) const				{return sc->getHeight()<waterLevel;}
	inline bool getSubmerged(const Cell *c) const						{return c->getHeight()<waterLevel;}
	inline bool getDeepSubmerged(const SurfaceCell *sc) const			{return sc->getHeight()<waterLevel-(1.5f/heightFactor);}
	inline bool getDeepSubmerged(const Cell *c) const					{return c->getHeight()<waterLevel-(1.5f/heightFactor);}
	//float getSurfaceHeight(const Vec2i &pos) const;

	//is
	inline bool isInside(int x, int y) const {
		return x>=0 && y>=0 && x<w && y<h;
	}
	inline bool isInside(const Vec2i &pos) const {
		return isInside(pos.x, pos.y);
	}
	inline bool isInsideSurface(int sx, int sy) const {
		return sx>=0 && sy>=0 && sx<surfaceW && sy<surfaceH;
	}
	inline bool isInsideSurface(const Vec2i &sPos) const {
		return isInsideSurface(sPos.x, sPos.y);
	}
	bool isResourceNear(const Vec2i &pos, const ResourceType *rt, Vec2i &resourcePos, int size, Unit *unit=NULL,bool fallbackToPeersHarvestingSameResource=false,Vec2i *resourceClickPos=NULL) const;

	//free cells
	bool isFreeCell(const Vec2i &pos, Field field) const;
	bool isFreeCellOrHasUnit(const Vec2i &pos, Field field, const Unit *unit) const;
	bool isAproxFreeCell(const Vec2i &pos, Field field, int teamIndex) const;
	bool isFreeCells(const Vec2i &pos, int size, Field field) const;
	bool isFreeCellsOrHasUnit(const Vec2i &pos, int size, Field field, const Unit *unit, const UnitType *munit, bool allowNullUnit=false) const;
	bool isAproxFreeCells(const Vec2i &pos, int size, Field field, int teamIndex) const;

	bool canOccupy(const Vec2i &pos, Field field, const UnitType *ut, CardinalDir facing);

	//unit placement
	bool aproxCanMove(const Unit *unit, const Vec2i &pos1, const Vec2i &pos2, std::map<Vec2i, std::map<Vec2i, std::map<int, std::map<int, std::map<Field,bool> > > > > *lookupCache=NULL) const;
	bool canMove(const Unit *unit, const Vec2i &pos1, const Vec2i &pos2,std::map<Vec2i, std::map<Vec2i, std::map<int, std::map<Field,bool> > > > *lookupCache=NULL) const;
    void putUnitCells(Unit *unit, const Vec2i &pos,bool ignoreSkill = false);
	void clearUnitCells(Unit *unit, const Vec2i &pos,bool ignoreSkill = false);

	Vec2i computeRefPos(const Selection *selection) const;
	Vec2i computeDestPos(	const Vec2i &refUnitPos, const Vec2i &unitPos,
							const Vec2i &commandPos) const;
	const Unit * findClosestUnitToPos(const Selection *selection, Vec2i originalBuildPos,
							    const UnitType *ut) const;
	bool isInUnitTypeCells(const UnitType *ut, const Vec2i &pos,const Vec2i &testPos) const;
	bool isNextToUnitTypeCells(const UnitType *ut, const Vec2i &pos,const Vec2i &testPos) const;
	Vec2i findBestBuildApproach(const Unit *unit, Vec2i originalBuildPos,const UnitType *ut) const;
	std::pair<float,Vec2i> getUnitDistanceToPos(const Unit *unit,Vec2i pos,const UnitType *ut);

	//misc
	bool isNextTo(const Vec2i &pos, const Unit *unit) const;
	bool isNextTo(const Vec2i &pos, const Vec2i &nextToPos) const;
	bool isNextTo(const Unit *unit1, const Unit *unit2) const;
	void clampPos(Vec2i &pos) const;

	void prepareTerrain(const Unit *unit);
	void flatternTerrain(const Unit *unit);
	void computeNormals();
	void computeInterpolatedHeights();

	//static
	inline static Vec2i toSurfCoords(const Vec2i &unitPos)		{return unitPos / cellScale;}
	inline static Vec2i toUnitCoords(const Vec2i &surfPos)		{return surfPos * cellScale;}
	static string getMapPath(const string &mapName, string scenarioDir="", bool errorOnNotFound=true);

	inline bool isFreeCellOrMightBeFreeSoon(Vec2i originPos, const Vec2i &pos, Field field) const {
		return
			isInside(pos) &&
			isInsideSurface(toSurfCoords(pos)) &&
			getCell(pos)->isFreeOrMightBeFreeSoon(originPos,pos,field) &&
			(field==fAir || getSurfaceCell(toSurfCoords(pos))->isFree()) &&
			(field!=fLand || getDeepSubmerged(getCell(pos)) == false);
	}

	inline bool isAproxFreeCellOrMightBeFreeSoon(Vec2i originPos,const Vec2i &pos, Field field, int teamIndex) const {
		if(isInside(pos) && isInsideSurface(toSurfCoords(pos))) {
			const SurfaceCell *sc= getSurfaceCell(toSurfCoords(pos));

			if(sc->isVisible(teamIndex)) {
				return isFreeCellOrMightBeFreeSoon(originPos, pos, field);
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

	//checks if a unit can move from between 2 cells using only visible cells (for pathfinding)
	inline bool aproxCanMoveSoon(const Unit *unit, const Vec2i &pos1, const Vec2i &pos2) const {
		if(isInside(pos1) == false || isInsideSurface(toSurfCoords(pos1)) == false ||
		   isInside(pos2) == false || isInsideSurface(toSurfCoords(pos2)) == false) {

			//printf("[%s] Line: %d returning false\n",__FUNCTION__,__LINE__);
			return false;
		}

		if(unit == NULL) {
			throw megaglest_runtime_error("unit == NULL");
		}

		int size= unit->getType()->getSize();
		int teamIndex= unit->getTeam();
		Field field= unit->getCurrField();

		const bool *cachedResult = unit->getFaction()->aproxCanMoveSoonCached(size,field,pos1,pos2);
		if(cachedResult != NULL) {
			return *cachedResult;
		}

		//single cell units
		if(size == 1) {
			if(isAproxFreeCellOrMightBeFreeSoon(unit->getPos(),pos2, field, teamIndex) == false) {

				//printf("[%s] Line: %d returning false\n",__FUNCTION__,__LINE__);
				unit->getFaction()->addAproxCanMoveSoonCached(size,field, pos1, pos2, false);
				return false;
			}
			if(pos1.x != pos2.x && pos1.y != pos2.y) {
				if(isAproxFreeCellOrMightBeFreeSoon(unit->getPos(),Vec2i(pos1.x, pos2.y), field, teamIndex) == false) {

					//Unit *cellUnit = getCell(Vec2i(pos1.x, pos2.y))->getUnit(field);
					//Object * obj = getSurfaceCell(toSurfCoords(Vec2i(pos1.x, pos2.y)))->getObject();

					//printf("[%s] Line: %d returning false cell [%s] free [%d] cell unitid = %d object class = %d\n",__FUNCTION__,__LINE__,Vec2i(pos1.x, pos2.y).getString().c_str(),this->isFreeCell(Vec2i(pos1.x, pos2.y),field),(cellUnit != NULL ? cellUnit->getId() : -1),(obj != NULL ? obj->getType()->getClass() : -1));
					//printf("[%s] Line: %d returning false\n",__FUNCTION__,__LINE__);
					unit->getFaction()->addAproxCanMoveSoonCached(size,field, pos1, pos2, false);
					return false;
				}
				if(isAproxFreeCellOrMightBeFreeSoon(unit->getPos(),Vec2i(pos2.x, pos1.y), field, teamIndex) == false) {
					//printf("[%s] Line: %d returning false\n",__FUNCTION__,__LINE__);
					unit->getFaction()->addAproxCanMoveSoonCached(size,field, pos1, pos2, false);
					return false;
				}
			}

			bool isBadHarvestPos = false;
			//if(unit != NULL) {
				Command *command= unit->getCurrCommand();
				if(command != NULL) {
					const HarvestCommandType *hct = dynamic_cast<const HarvestCommandType*>(command->getCommandType());
					if(hct != NULL && unit->isBadHarvestPos(pos2) == true) {
						isBadHarvestPos = true;
					}
				}
			//}

			if(unit == NULL || isBadHarvestPos == true) {

				//printf("[%s] Line: %d returning false\n",__FUNCTION__,__LINE__);
				unit->getFaction()->addAproxCanMoveSoonCached(size,field, pos1, pos2, false);
				return false;
			}

			unit->getFaction()->addAproxCanMoveSoonCached(size,field, pos1, pos2, true);
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
								unit->getFaction()->addAproxCanMoveSoonCached(size,field, pos1, pos2, false);
								return false;
							}
						}
					}
					else {

						//printf("[%s] Line: %d returning false\n",__FUNCTION__,__LINE__);
						unit->getFaction()->addAproxCanMoveSoonCached(size,field, pos1, pos2, false);
						return false;
					}
				}
			}

			bool isBadHarvestPos = false;
			Command *command= unit->getCurrCommand();
			if(command != NULL) {
				const HarvestCommandType *hct = dynamic_cast<const HarvestCommandType*>(command->getCommandType());
				if(hct != NULL && unit->isBadHarvestPos(pos2) == true) {
					isBadHarvestPos = true;
				}
			}

			if(isBadHarvestPos == true) {

				//printf("[%s] Line: %d returning false\n",__FUNCTION__,__LINE__);
				unit->getFaction()->addAproxCanMoveSoonCached(size,field, pos1, pos2, false);
				return false;
			}

		}
		unit->getFaction()->addAproxCanMoveSoonCached(size,field, pos1, pos2, true);
		return true;
	}

	string getMapFile() const { return mapFile; }

	void saveGame(XmlNode *rootNode) const;
	void loadGame(const XmlNode *rootNode,World *world);

private:
	//compute
	void smoothSurface(Tileset *tileset);
	void computeNearSubmerged();
	void computeCellColors();
    void putUnitCellsPrivate(Unit *unit, const Vec2i &pos, const UnitType *ut, bool isMorph);
};


// ===============================
// 	class PosCircularIterator
// ===============================

class PosCircularIterator{
private:
	Vec2i center;
	int radius;
	const Map *map;
	Vec2i pos;

public:
	PosCircularIterator(const Map *map, const Vec2i &center, int radius);
	bool next();
	const Vec2i &getPos();
};

// ===============================
// 	class PosQuadIterator
// ===============================

class PosQuadIterator {
private:
	Quad2i quad;
	Rect2i boundingRect;
	Vec2i pos;
	int step;
	const Map *map;

public:
	PosQuadIterator(const Map *map,const Quad2i &quad, int step=1);
	bool next();
	void skipX();
	const Vec2i &getPos();
};

} } //end namespace

#endif
