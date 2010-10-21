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

#ifndef _MAPPREVIEW_MAP_H_
#define _MAPPREVIEW_MAP_H_

#include "util.h"
#include "types.h"
#include "randomgen.h"
#include <vector>

using Shared::Platform::int8;
using Shared::Platform::int32;
using Shared::Platform::float32;
using Shared::Util::RandomGen;

namespace Shared { namespace Map {

enum MapSurfaceType {
	st_Grass = 1,
	st_Secondary_Grass,
	st_Road,
	st_Stone,
	st_Ground

};

static const int MAX_TITLE_LENGTH 		= 128;
static const int MAX_AUTHOR_LENGTH 		= 128;
static const int MAX_DESCRIPTION_LENGTH = 256;

static const int MIN_MAP_CELL_DIMENSION		= 16;
static const int MAX_MAP_CELL_DIMENSION		= 1024;

static const int MIN_MAP_CELL_HEIGHT 		= 0;
static const int MAX_MAP_CELL_HEIGHT 		= 20;
static const int DEFAULT_MAP_CELL_HEIGHT 	= 10;

static const int MIN_MAP_FACTIONCOUNT 		= 1;
static const int MAX_MAP_FACTIONCOUNT 		= 8;
static const int DEFAULT_MAP_FACTIONCOUNT 	= 8;

static const int DEFAULT_MAP_CELL_WIDTH 		= 128;
static const int DEFAULT_MAP_CELL_LENGTH		= 128;

static const MapSurfaceType DEFAULT_MAP_CELL_SURFACE_TYPE 	= st_Grass;

static const int DEFAULT_MAP_CELL_HEIGHT_FACTOR		= 3;
static const int DEFAULT_MAP_WATER_DEPTH			= 4;

struct MapFileHeader {
	int32 version;
	int32 maxFactions;
	int32 width;
	int32 height;
	int32 altFactor;
	int32 waterLevel;
	int8 title[MAX_TITLE_LENGTH];
	int8 author[MAX_AUTHOR_LENGTH];
	int8 description[MAX_DESCRIPTION_LENGTH];
};

// ===============================================
//	class Map
// ===============================================

class MapPreview {
public:
	static const int maxHeight = 20;
	static const int minHeight = 0;

private:
	struct Cell {
		int surface;
		int object;
		int resource;
		float height;
	};

	struct StartLocation {
		int x;
		int y;
	};

	RandomGen random;
	string title;
	string author;
	string desc;
	string recScn;
	int type;
	int h;
	int w;
	int altFactor;
	int waterLevel;
	//Cell **cells;
	std::vector<std::vector<Cell> > cells;

	int maxFactions;
	//StartLocation *startLocations;
	std::vector<StartLocation> startLocations;
	int refAlt;

	bool fileLoaded;

public:
	MapPreview();
	~MapPreview();
	float getHeight(int x, int y) const;
	MapSurfaceType getSurface(int x, int y) const;
	int getObject(int x, int y) const;
	int getResource(int x, int y) const;
	int getStartLocationX(int index) const;
	int getStartLocationY(int index) const;
	int getHeightFactor() const;
	int getWaterLevel() const;
	bool inside(int x, int y);

	void setRefAlt(int x, int y);
	void setAdvanced(int altFactor, int waterLevel);
	void setTitle(const string &title);
	void setDesc(const string &desc);
	void setAuthor(const string &author);

	int getH() const			{return h;}
	int getW() const			{return w;}
	int getMaxFactions() const	{return maxFactions;}
	string getTitle() const		{return title;}
	string getDesc() const		{return desc;}
	string getAuthor() const	{return author;}

	void glestChangeHeight(int x, int y, int height, int radius);
	void pirateChangeHeight(int x, int y, int height, int radius);
	void changeSurface(int x, int y, MapSurfaceType surface, int radius);
	void changeObject(int x, int y, int object, int radius);
	void changeResource(int x, int y, int resource, int radius);
	void changeStartLocation(int x, int y, int player);

	void setHeight(int x, int y, float height);
	void setSurface(int x, int y, MapSurfaceType surface);
	void setObject(int x, int y, int object);
	void setResource(int x, int y, int resource);

	void flipX();
	void flipY();
	void reset(int w, int h, float alt, MapSurfaceType surf);
	void resize(int w, int h, float alt, MapSurfaceType surf);
	void resetFactions(int maxFactions);
	void randomizeHeights();
	void randomize();
	void switchSurfaces(MapSurfaceType surf1, MapSurfaceType surf2);

	void loadFromFile(const string &path);
	void saveToFile(const string &path);

	void resetHeights(int height);
	void sinRandomize(int strenght);
	void decalRandomize(int strenght);
	void applyNewHeight(float newHeight, int x, int y, int strenght);

	bool hasFileLoaded() const {return fileLoaded;}
};

}}// end namespace

#endif
