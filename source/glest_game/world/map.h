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

#ifndef _GLEST_GAME_MAP_H_
#define _GLEST_GAME_MAP_H_

#include "vec.h"
#include "math_util.h"
#include "command_type.h"
#include "logger.h"
#include "object.h"
#include "game_constants.h"
#include "selection.h"
#include <cassert>
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

// =====================================================
// 	class Cell
//
///	A map cell that holds info about units present on it
// =====================================================

class Cell{
private:
    Unit *units[fieldCount];	//units on this cell
	float height;

private:
	Cell(Cell&);
	void operator=(Cell&);

public:
	Cell();

	//get
	Unit *getUnit(int field) const		{return units[field];}
	float getHeight() const				{return height;}

	void setUnit(int field, Unit *unit)	{units[field]= unit;}
	void setHeight(float height)		{this->height= height;}

	bool isFree(Field field) const;
};

// =====================================================
// 	class SurfaceCell
//
//	A heightmap cell, each surface cell is composed by more than one Cell
// =====================================================

class SurfaceCell{
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

public:
	SurfaceCell();
	~SurfaceCell();

	//get
	const Vec3f &getVertex() const				{return vertex;}
	float getHeight() const						{return vertex.y;}
	const Vec3f &getColor() const				{return color;}
	const Vec3f &getNormal() const				{return normal;}
	int getSurfaceType() const					{return surfaceType;}
	const Texture2D *getSurfaceTexture() const	{return surfaceTexture;}
	Object *getObject() const					{return object;}
	Resource *getResource() const				{return object==NULL? NULL: object->getResource();}
	const Vec2f &getFowTexCoord() const			{return fowTexCoord;}
	const Vec2f &getSurfTexCoord() const		{return surfTexCoord;}
	bool getNearSubmerged() const				{return nearSubmerged;}

	bool isVisible(int teamIndex) const			{return visible[teamIndex];}
	bool isExplored(int teamIndex) const		{return explored[teamIndex];}

	//set
	void setVertex(const Vec3f &vertex)			{this->vertex= vertex;}
	void setHeight(float height)				{vertex.y= height;}
	void setNormal(const Vec3f &normal)			{this->normal= normal;}
	void setColor(const Vec3f &color)			{this->color= color;}
	void setSurfaceType(int surfaceType)		{this->surfaceType= surfaceType;}
	void setSurfaceTexture(const Texture2D *st)	{this->surfaceTexture= st;}
	void setObject(Object *object)				{this->object= object;}
	void setFowTexCoord(const Vec2f &ftc)		{this->fowTexCoord= ftc;}
	void setSurfTexCoord(const Vec2f &stc)		{this->surfTexCoord= stc;}
	void setExplored(int teamIndex, bool explored);
    void setVisible(int teamIndex, bool visible);
	void setNearSubmerged(bool nearSubmerged)	{this->nearSubmerged= nearSubmerged;}

	//misc
	void deleteResource();
	bool isFree() const;
};


// =====================================================
// 	class Map
//
///	Represents the game map (and loads it from a gbm file)
// =====================================================

class Map{
public:
	static const int cellScale;	//number of cells per surfaceCell
	static const int mapScale;	//horizontal scale of surface

private:
	string title;
	float waterLevel;
	float heightFactor;
	int w;
	int h;
	int surfaceW;
	int surfaceH;
	int maxPlayers;
	Cell *cells;
	SurfaceCell *surfaceCells;
	Vec2i *startLocations;

private:
	Map(Map&);
	void operator=(Map&);

public:
	Map();
	~Map();

	void init();
	void load(const string &path, TechTree *techTree, Tileset *tileset);

	//get
	Cell *getCell(int x, int y) const							{return &cells[y*w+x];}
	Cell *getCell(const Vec2i &pos) const						{return getCell(pos.x, pos.y);}
	SurfaceCell *getSurfaceCell(int sx, int sy) const			{return &surfaceCells[sy*surfaceW+sx];}
	SurfaceCell *getSurfaceCell(const Vec2i &sPos) const		{return getSurfaceCell(sPos.x, sPos.y);}
	int getW() const											{return w;}
	int getH() const											{return h;}
	int getSurfaceW() const										{return surfaceW;}
	int getSurfaceH() const										{return surfaceH;}
	int getMaxPlayers() const									{return maxPlayers;}
	float getHeightFactor() const								{return heightFactor;}
	float getWaterLevel() const									{return waterLevel;}
	Vec2i getStartLocation(int loactionIndex) const				{return startLocations[loactionIndex];}
	bool getSubmerged(const SurfaceCell *sc) const				{return sc->getHeight()<waterLevel;}
	bool getSubmerged(const Cell *c) const						{return c->getHeight()<waterLevel;}
	bool getDeepSubmerged(const SurfaceCell *sc) const			{return sc->getHeight()<waterLevel-(1.5f/heightFactor);}
	bool getDeepSubmerged(const Cell *c) const					{return c->getHeight()<waterLevel-(1.5f/heightFactor);}
	float getSurfaceHeight(const Vec2i &pos) const;

	//is
	bool isInside(int x, int y) const;
	bool isInside(const Vec2i &pos) const;
	bool isInsideSurface(int sx, int sy) const;
	bool isInsideSurface(const Vec2i &sPos) const;
	bool isResourceNear(const Vec2i &pos, const ResourceType *rt, Vec2i &resourcePos, int size, Unit *unit=NULL,bool fallbackToPeersHarvestingSameResource=false) const;
	bool isResourceNear(const Vec2i &pos, int size, const ResourceType *rt, Vec2i &resourcePos) const;

	//free cells
	bool isFreeCell(const Vec2i &pos, Field field) const;
	bool isFreeCellOrHasUnit(const Vec2i &pos, Field field, const Unit *unit) const;
	bool isAproxFreeCell(const Vec2i &pos, Field field, int teamIndex) const;
	bool isFreeCells(const Vec2i &pos, int size, Field field) const;
	bool isFreeCellsOrHasUnit(const Vec2i &pos, int size, Field field, const Unit *unit) const;
	bool isAproxFreeCells(const Vec2i &pos, int size, Field field, int teamIndex) const;

	bool canOccupy(const Vec2i &pos, Field field, const UnitType *ut, CardinalDir facing);

	//unit placement
	bool aproxCanMove(const Unit *unit, const Vec2i &pos1, const Vec2i &pos2) const;
	bool canMove(const Unit *unit, const Vec2i &pos1, const Vec2i &pos2) const;
    void putUnitCells(Unit *unit, const Vec2i &pos);
	void clearUnitCells(Unit *unit, const Vec2i &pos);

	Vec2i computeRefPos(const Selection *selection) const;
	Vec2i computeDestPos(	const Vec2i &refUnitPos, const Vec2i &unitPos,
							const Vec2i &commandPos) const;
	const Unit * findClosestUnitToPos(const Selection *selection, Vec2i originalBuildPos,
							    const UnitType *ut) const;
	bool isInUnitTypeCells(const UnitType *ut, const Vec2i &pos,const Vec2i &testPos) const;
	bool isNextToUnitTypeCells(const UnitType *ut, const Vec2i &pos,const Vec2i &testPos) const;
	Vec2i findBestBuildApproach(Vec2i unitBuilderPos, Vec2i originalBuildPos,
								const UnitType *ut) const;
	std::pair<float,Vec2i> getUnitDistanceToPos(const Unit *unit,Vec2i pos,const UnitType *ut);

	//misc
	bool isNextTo(const Vec2i &pos, const Unit *unit) const;
	bool isNextTo(const Vec2i &pos, const Vec2i &nextToPos) const;
	void clampPos(Vec2i &pos) const;

	void prepareTerrain(const Unit *unit);
	void flatternTerrain(const Unit *unit);
	void computeNormals();
	void computeInterpolatedHeights();

	//static
	static Vec2i toSurfCoords(const Vec2i &unitPos)		{return unitPos / cellScale;}
	static Vec2i toUnitCoords(const Vec2i &surfPos)		{return surfPos * cellScale;}
	static string getMapPath(const string &mapName, string scenarioDir="", bool errorOnNotFound=true);

private:
	//compute
	void smoothSurface();
	void computeNearSubmerged();
	void computeCellColors();
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
public:
	PosQuadIterator(const Quad2i &quad, int step=1);
	bool next();
	void skipX();
	const Vec2i &getPos();
};

} } //end namespace

#endif
