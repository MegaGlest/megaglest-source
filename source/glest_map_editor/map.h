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

// This file is not used anoymore

#ifndef _MAPEDITOR_MAP_H_
#define _MAPEDITOR_MAP_H_

#include "util.h"
#include "types.h"
#include "randomgen.h"

using Shared::Platform::int8;
using Shared::Platform::int32;
using Shared::Platform::float32;
using Shared::Util::RandomGen;

namespace MapEditor {

struct MapFileHeader {
	int32 version;
	int32 maxFactions;
	int32 width;
	int32 height;
	int32 altFactor;
	int32 waterLevel;
	int8 title[128];
	int8 author[128];
	int8 description[256];
};

// ===============================================
//	class Map
// ===============================================

class Map {
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
	Cell **cells;
	int maxFactions;
	StartLocation *startLocations;
	int refAlt;

public:
	Map();
	~Map();
	float getHeight(int x, int y) const;
	int getSurface(int x, int y) const;
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
	void changeSurface(int x, int y, int surface, int radius);
	void changeObject(int x, int y, int object, int radius);
	void changeResource(int x, int y, int resource, int radius);
	void changeStartLocation(int x, int y, int player);

	void setHeight(int x, int y, float height);
	void setSurface(int x, int y, int surface);
	void setObject(int x, int y, int object);
	void setResource(int x, int y, int resource);

	void flipX();
	void flipY();
	void mirrorX();
	void mirrorY();
	void mirrorXY();
	void rotatecopyX();
	void rotatecopyY();
	void rotatecopyXY();
	void rotatecopyCorner();
	void reset(int w, int h, float alt, int surf);
	void resize(int w, int h, float alt, int surf);
	void resetFactions(int maxFactions);
	void randomizeHeights();
	void randomize();
	void switchSurfaces(int surf1, int surf2);

	void loadFromFile(const string &path);
	void saveToFile(const string &path);

public:
	void resetHeights(int height);
	void sinRandomize(int strenght);
	void decalRandomize(int strenght);
	void applyNewHeight(float newHeight, int x, int y, int strenght);
};

}// end namespace

#endif
