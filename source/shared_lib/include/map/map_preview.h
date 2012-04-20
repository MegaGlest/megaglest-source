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

#ifndef _MAPPREVIEW_MAP_H_
#define _MAPPREVIEW_MAP_H_

#include "util.h"
#include "data_types.h"
#include "randomgen.h"
#include "vec.h"
#include <vector>

using Shared::Platform::int8;
using Shared::Platform::int32;
using Shared::Platform::float32;
using Shared::Util::RandomGen;
using Shared::Graphics::Vec2i;

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
static const int MAX_DESCRIPTION_LENGTH_VERSION2 = 128;

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
static const int DEFAULT_CLIFF_HEIGHT				= 0;

enum MapVersionType {
	mapver_1 = 1,
	mapver_2,

	mapver_MAX
};

static const int MAP_FORMAT_VERSION = mapver_MAX - 1;

struct MapFileHeader {
	int32 version;
	int32 maxFactions;
	int32 width;
	int32 height;
	int32 heightFactor;
	int32 waterLevel;
	int8 title[MAX_TITLE_LENGTH];
	int8 author[MAX_AUTHOR_LENGTH];
	union {
		int8 description[MAX_DESCRIPTION_LENGTH];
		struct {
			int8 short_desc[MAX_DESCRIPTION_LENGTH_VERSION2];
			int32 magic; // 0x01020304 for meta
			int32 cliffLevel;
			int32 cameraHeight;
			int8 meta[116];
		} version2;
	};
};

class MapInfo {
public:

	Vec2i size;
	int players;
	string desc;

	MapInfo() {
		size 	= Vec2i(0,0);
		players = 0;
		desc 	= "";
	}
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
	int heightFactor;
	int waterLevel;
	int cliffLevel;
	int cameraHeight;
	//Cell **cells;
	std::vector<std::vector<Cell> > cells;

	int maxFactions;
	//StartLocation *startLocations;
	std::vector<StartLocation> startLocations;
	int refAlt;

	bool fileLoaded;
	string mapFileLoaded;

public:
	MapPreview();
	~MapPreview();
	float getHeight(int x, int y) const;
	bool isCliff(int x,int y);
	MapSurfaceType getSurface(int x, int y) const;
	int getObject(int x, int y) const;
	int getResource(int x, int y) const;
	int getStartLocationX(int index) const;
	int getStartLocationY(int index) const;
	int getHeightFactor() const{return heightFactor;}
	int getWaterLevel() const{return waterLevel;}
	int getCliffLevel() const{return cliffLevel;}
	int getCameraHeight() const{return cameraHeight;}

	bool inside(int x, int y);

	void setRefAlt(int x, int y);
	void setAdvanced(int heightFactor, int waterLevel, int cliffLevel, int cameraHeight);
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
	void copyXY(int x, int y, int sx, int sy);  // destination x,y = source sx,sy
	void swapXY(int x, int y, int sx, int sy);
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
	string getMapFileLoaded() const { return mapFileLoaded; }

	static bool loadMapInfo(string file, MapInfo *mapInfo, string i18nMaxMapPlayersTitle,string i18nMapSizeTitle,bool errorOnInvalidMap=true);
	static string getMapPath(const vector<string> &pathList, const string &mapName, string scenarioDir="", bool errorOnNotFound=true);
	static vector<string> findAllValidMaps(const vector<string> &pathList,
			string scenarioDir, bool getUserDataOnly=false, bool cutExtension=true,
			vector<string> *invalidMapList=NULL);
};

}}// end namespace

#endif
