// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================


#include "map_preview.h"

#include "math_wrapper.h"
#include <cstdlib>
#include <stdexcept>

using namespace Shared::Util;
using namespace std;

namespace Shared { namespace Map {

// ===============================================
//	class MapPreview
// ===============================================

// ================== PUBLIC =====================

MapPreview::MapPreview() {
	fileLoaded = false;
	heightFactor 	= DEFAULT_MAP_CELL_HEIGHT_FACTOR;
	waterLevel 	= DEFAULT_MAP_WATER_DEPTH;
	cliffLevel = DEFAULT_CLIFF_HEIGHT;
	//cells = NULL;
	cells.clear();
	//startLocations = NULL;
	startLocations.clear();
	reset(DEFAULT_MAP_CELL_WIDTH, DEFAULT_MAP_CELL_LENGTH, (float)DEFAULT_MAP_CELL_HEIGHT, DEFAULT_MAP_CELL_SURFACE_TYPE);
	resetFactions(DEFAULT_MAP_FACTIONCOUNT);
	title = "";
	desc = "";
	author = "";
	refAlt = DEFAULT_MAP_CELL_HEIGHT;
}

MapPreview::~MapPreview() {
	//delete [] startLocations;
	//startLocations = NULL;
	startLocations.clear();

	//for (int i = 0; i < h; i++) {
	//	delete [] cells[i];
	//}
	//delete [] cells;
	//cells = NULL;
	cells.clear();
}

float MapPreview::getHeight(int x, int y) const {
	return cells[x][y].height;
}

bool MapPreview::isCliff(int x, int y){
	if(cliffLevel == 0)
		return false;
	for(int k= -1; k <= 1; ++k){
		for(int l= -1; l <= 1; ++l){
			int xToCheck= x + l;
			int yToCheck= y + k;
			if(xToCheck < 0 || yToCheck < 0 || xToCheck >= w || yToCheck >= h){
				//ignore
			}
			else if(cliffLevel <= abs((int)(getHeight(x, y) - getHeight(xToCheck, yToCheck)))) {
				return true;
			}
		}
	}
	return false;
}

MapSurfaceType MapPreview::getSurface(int x, int y) const {
	return static_cast<MapSurfaceType>(cells[x][y].surface);
}

int MapPreview::getObject(int x, int y) const {
	return cells[x][y].object;
}

int MapPreview::getResource(int x, int y) const {
	return cells[x][y].resource;
}

int MapPreview::getStartLocationX(int index) const {
	return startLocations[index].x;
}

int MapPreview::getStartLocationY(int index) const {
	return startLocations[index].y;
}

static int get_dist(int delta_x, int delta_y) {
	float dx = (float)delta_x;
	float dy = (float)delta_y;
#ifdef USE_STREFLOP
	return static_cast<int>(streflop::sqrtf(dx * dx + dy * dy)+0.5); // round correctly
#else
	return static_cast<int>(sqrtf(dx * dx + dy * dy)+0.5); // round correctly
#endif
}

void MapPreview::glestChangeHeight(int x, int y, int height, int radius) {

	for (int i = x - radius + 1; i < x + radius; i++) {
		for (int j = y - radius + 1; j < y + radius; j++) {
			if (inside(i, j)) {
				int dist = get_dist(i - x, j - y);
				if (radius > dist) {
					int oldAlt = static_cast<int>(cells[i][j].height);
					int altInc = height * (radius - dist - 1) / radius;
					if (height > 0) {
						altInc++;
					}
					if (height < 0) {
						altInc--;
					}
					int newAlt = refAlt + altInc;
					if ((height > 0 && newAlt > oldAlt) || (height < 0 && newAlt < oldAlt) || height == 0) {
						if (newAlt >= 0 && newAlt <= 20) {
							cells[i][j].height = static_cast<float>(newAlt);
						}
					}
				}
			}
		}
	}
}


void MapPreview::pirateChangeHeight(int x, int y, int height, int radius) {
	// Make sure not to try and blanket change the height over the bounds
	// Find our goal height for the center of the brush

	int overBounds = refAlt + height;
	int goalAlt = overBounds;
	if (overBounds > 20) {
		goalAlt = 20;
	}
	else if (overBounds < 0) {
		goalAlt = 0;
	}

	// If the radius is 1 don't bother doing any calculations
	if (radius == 1) {
		if(inside(x, y)){
			cells[x][y].height = (float)goalAlt;
		}
		return;
	}

	// Get Old height reference points and compute gradients
	// from the heights of the sides and corners of the brush to the center goal height
	float gradient[3][3]; // [i][j]
	int indexI = 0;
	for (int i = x - radius; i <= x + radius; i += radius) {
	int indexJ = 0;
		for (int j = y - radius; j <= y + radius; j += radius) {
			// round off the corners
			int ti=0, tj=0;
			if (abs(i - x) == abs(j - y)) {
				ti = (int)((i - x) * 0.707 + x + 0.5);
				tj = (int)((j - y) * 0.707 + y + 0.5);
			} else {
				ti = i;
				tj = j;
			}
			if (inside(ti, tj)) {
				gradient[indexI][indexJ] = (cells[ti][tj].height - (float)goalAlt) / (float)radius;
			//} else if (dist == 0) {
				//gradient[indexI][indexJ] = 0;
			}
			else {
				// assume outside the map bounds is height 10
				gradient[indexI][indexJ] = (10.0f - (float)goalAlt) / (float)radius;
			}
			//std::cout << "gradient[" << indexI << "][" << indexJ << "] = " << gradient[indexI][indexJ] << std::endl;
			//std::cout << "derived from height " << cells[ti][tj].height << " at " << ti << " " << tj << std::endl;
			indexJ++;
		}
		indexI++;
	}
	//std::cout << endl;

	//  // A brush with radius n cells should have a true radius of n-1 distance  // No becasue then "radius" 1==2
	// radius -= 1;
	for (int i = x - radius; i <= x + radius; i++) {
		for (int j = y - radius; j <= y + radius; j++) {
			int dist = get_dist(i - x, j - y);
			if (inside(i, j) && dist < radius) {
					// Normalize di and dj and round them to an int so they can be used as indicies
					float normIf = (float(i - x)/ radius);
					float normJf = (float(j - y)/ radius);
					int normI[2];
					int normJ[2];
					float usedGrad;

					// Build a search box to find the gradients we are concerned about
					// Find the nearest i indices
					if (normIf < -0.33) {
						normI[0] = 0;
						if (normIf == 0) {
							normI[1] = 0;
						}
						else {
							normI[1] = 1;
						}
					}
					else if (normIf < 0.33) {
						normI[0] = 1;
						if (normIf > 0) {
							normI[1] = 2;
						}
						else if (normIf < 0) {
							normI[1] = 0;
						}
						else /*(normIf == 0)*/ {
							normI[1] = 1;
						}
					}
					else {
						normI[0] = 2;
						if (normIf == 1) {
							normI[1] = 2;
						}
						else {
							normI[1] = 1;
						}
					}
					// find nearest j indices
					if (normJf < -0.33) {
						normJ[0] = 0;
						if (normJf == 0) {
							normJ[1] = 0;
						}
						else {
							normJ[1] = 1;
						}
					}
					else if (normJf < 0.33) {
						normJ[0] = 1;
						if (normJf > 0) {
							normJ[1] = 2;
						}
						else if (normJf < 0) {
							normJ[1] = 0;
						}
						else /*(normJf == 0)*/ {
							normJ[1] = 1;
						}
					}
					else {
						normJ[0] = 2;
						if (normJf == 1) {
							normJ[1] = 2;
						}
						else {
							normJ[1] = 1;
						}
					}

					// Determine which gradients to use and take a weighted average
#ifdef USE_STREFLOP
					if (streflop::fabs(normIf) > streflop::fabs(normJf)) {

						usedGrad =
								gradient[normI[0]] [normJ[0]] * streflop::fabs(normJf) +
								gradient[normI[0]] [normJ[1]] * (1 - streflop::fabs(normJf));
					}
					else if (streflop::fabs(normIf) < streflop::fabs(normJf)) {
						usedGrad =
								gradient[normI[0]] [normJ[0]] * streflop::fabs(normIf) +
								gradient[normI[1]] [normJ[0]] * (1 - streflop::fabs(normIf));
					}
					else {
						usedGrad =
								gradient[normI[0]] [normJ[0]];
					}

#else
					if (fabs(normIf) > fabs(normJf)) {

						usedGrad =
								gradient[normI[0]] [normJ[0]] * fabs(normJf) +
								gradient[normI[0]] [normJ[1]] * (1 - fabs(normJf));
					}
					else if (fabs(normIf) < fabs(normJf)) {
						usedGrad =
								gradient[normI[0]] [normJ[0]] * fabs(normIf) +
								gradient[normI[1]] [normJ[0]] * (1 - fabs(normIf));
					}
					else {
						usedGrad =
								gradient[normI[0]] [normJ[0]];
					}
#endif

					float newAlt = usedGrad * dist + goalAlt;

					// if the change in height and what is supposed to be the change in height
					// are the same sign then we can change the height
					if (	((newAlt - cells[i][j].height) > 0 && height > 0) ||
							((newAlt - cells[i][j].height) < 0 && height < 0) ||
							height == 0) {
						cells[i][j].height = newAlt;
					}
				}
		}
	}
}

void MapPreview::setHeight(int x, int y, float height) {
	cells[x][y].height = height;
}

void MapPreview::setRefAlt(int x, int y) {
	if (inside(x, y)) {
		refAlt = static_cast<int>(cells[x][y].height);
	}
}

void MapPreview::flipX() {
	//Cell **oldCells = cells;
	std::vector<std::vector<Cell> > oldCells = cells;

	//cells = new Cell*[w];
	cells.clear();
	cells.resize(w);
	for (int i = 0; i < w; i++) {
		//cells[i] = new Cell[h];
		cells[i].resize(h);
		for (int j = 0; j < h; j++) {
			cells[i][j].height = oldCells[w-i-1][j].height;
			cells[i][j].object = oldCells[w-i-1][j].object;
			cells[i][j].resource = oldCells[w-i-1][j].resource;
			cells[i][j].surface = oldCells[w-i-1][j].surface;
		}
	}

	for (int i = 0; i < maxFactions; ++i) {
		startLocations[i].x = w - startLocations[i].x - 1;
	}

	//for (int i = 0; i < w; i++) {
	//	delete [] oldCells[i];
	//}
	//delete [] oldCells;
}

void MapPreview::flipY() {
	//Cell **oldCells = cells;
	std::vector<std::vector<Cell> > oldCells = cells;

	//cells = new Cell*[w];
	cells.clear();
	cells.resize(w);

	for (int i = 0; i < w; i++) {
		//cells[i] = new Cell[h];
		cells[i].resize(h);
		for (int j = 0; j < h; j++) {
			cells[i][j].height = oldCells[i][h-j-1].height;
			cells[i][j].object = oldCells[i][h-j-1].object;
			cells[i][j].resource = oldCells[i][h-j-1].resource;
			cells[i][j].surface = oldCells[i][h-j-1].surface;
		}
	}

	for (int i = 0; i < maxFactions; ++i) {
		startLocations[i].y = h - startLocations[i].y - 1;
	}

	//for (int i = 0; i < w; i++) {
	//	delete [] oldCells[i];
	//}
	//delete [] oldCells;
}

// Copy a cell in the map from one cell to another, used by MirrorXY etc
void MapPreview::copyXY(int x, int y, int sx, int sy) {
		cells[x][y].height   = cells[sx][sy].height;
		cells[x][y].object   = cells[sx][sy].object;
		cells[x][y].resource = cells[sx][sy].resource;
		cells[x][y].surface  = cells[sx][sy].surface;
}

void MapPreview::changeSurface(int x, int y, MapSurfaceType surface, int radius) {
	int i, j;
	int dist;

	for (i = x - radius + 1; i < x + radius; i++) {
		for (j = y - radius + 1; j < y + radius; j++) {
			if (inside(i, j)) {
				dist = get_dist(i - x, j - y);
				if (radius > dist) {  // was >=
					cells[i][j].surface = surface;
				}
			}
		}
	}
}

void MapPreview::setSurface(int x, int y, MapSurfaceType surface) {
	cells[x][y].surface = surface;
}

void MapPreview::changeObject(int x, int y, int object, int radius) {
	int i, j;
	int dist;

	for (i = x - radius + 1; i < x + radius; i++) {
		for (j = y - radius + 1; j < y + radius; j++) {
			if (inside(i, j)) {
				dist = get_dist(i - x, j - y);
				if (radius > dist) {  // was >=
					cells[i][j].object = object;
					cells[i][j].resource = 0;
				}
			}
		}
	}
}

void MapPreview::setObject(int x, int y, int object) {
	cells[x][y].object = object;
	if (object != 0) cells[x][y].resource = 0;
}

void MapPreview::changeResource(int x, int y, int resource, int radius) {
	int i, j;
	int dist;

	for (i = x - radius + 1; i < x + radius; i++) {
		for (j = y - radius + 1; j < y + radius; j++) {
			if (inside(i, j)) {
				dist = get_dist(i - x, j - y);
				if (radius > dist) {  // was >=
					cells[i][j].resource = resource;
					cells[i][j].object = 0;
				}
			}
		}
	}
}

void MapPreview::setResource(int x, int y, int resource) {
	cells[x][y].resource = resource;
	if (resource != 0) cells[x][y].object = 0;
}

void MapPreview::changeStartLocation(int x, int y, int faction) {
	if ((faction - 1) < maxFactions && inside(x, y)) {
		startLocations[faction].x = x;
		startLocations[faction].y = y;
	}
}

bool MapPreview::inside(int x, int y) {
	return (x >= 0 && x < w && y >= 0 && y < h);
}

void MapPreview::reset(int w, int h, float alt, MapSurfaceType surf) {
	if (w < MIN_MAP_CELL_DIMENSION || h < MIN_MAP_CELL_DIMENSION) {
		char szBuf[1024]="";
		sprintf(szBuf,"Size of map must be at least %dx%d",MIN_MAP_CELL_DIMENSION,MIN_MAP_CELL_DIMENSION);
		throw runtime_error(szBuf);
		return;
	}

	if (w > MAX_MAP_CELL_DIMENSION || h > MAX_MAP_CELL_DIMENSION) {
		char szBuf[1024]="";
		sprintf(szBuf,"Size of map can be at most %dx%d",MAX_MAP_CELL_DIMENSION,MAX_MAP_CELL_DIMENSION);
		throw runtime_error(szBuf);
	}

	if (alt < MIN_MAP_CELL_HEIGHT || alt > MAX_MAP_CELL_HEIGHT) {
		char szBuf[1024]="";
		sprintf(szBuf,"Height must be in the range %d-%d",MIN_MAP_CELL_HEIGHT,MAX_MAP_CELL_HEIGHT);
		throw runtime_error(szBuf);
	}

	if (surf < st_Grass || surf > st_Ground) {
		char szBuf[1024]="";
		sprintf(szBuf,"Surface must be in the range %d-%d",st_Grass,st_Ground);
		throw runtime_error(szBuf);
	}

	//if (cells != NULL) {
	//	for (int i = 0; i < this->w; i++) {
	//		delete [] cells[i];
	//	}
	//	delete [] cells;
	//}
	cells.clear();

	this->w = w;
	this->h = h;
	this->maxFactions = maxFactions;

	//cells = new Cell*[w];
	cells.resize(w);
	for (int i = 0; i < w; i++) {
		//cells[i] = new Cell[h];
		cells[i].resize(h);
		for (int j = 0; j < h; j++) {
			cells[i][j].height = alt;
			cells[i][j].object = 0;
			cells[i][j].resource = 0;
			cells[i][j].surface = surf;
		}
	}
}

void MapPreview::resize(int w, int h, float alt, MapSurfaceType surf) {
	if (w < MIN_MAP_CELL_DIMENSION || h < MIN_MAP_CELL_DIMENSION) {
		char szBuf[1024]="";
		sprintf(szBuf,"Size of map must be at least %dx%d",MIN_MAP_CELL_DIMENSION,MIN_MAP_CELL_DIMENSION);
		throw runtime_error(szBuf);
		return;
	}

	if (w > MAX_MAP_CELL_DIMENSION || h > MAX_MAP_CELL_DIMENSION) {
		char szBuf[1024]="";
		sprintf(szBuf,"Size of map can be at most %dx%d",MAX_MAP_CELL_DIMENSION,MAX_MAP_CELL_DIMENSION);
		throw runtime_error(szBuf);
	}

	if (alt < MIN_MAP_CELL_HEIGHT || alt > MAX_MAP_CELL_HEIGHT) {
		char szBuf[1024]="";
		sprintf(szBuf,"Height must be in the range %d-%d",MIN_MAP_CELL_HEIGHT,MAX_MAP_CELL_HEIGHT);
		throw runtime_error(szBuf);
	}

	if (surf < st_Grass || surf > st_Ground) {
		char szBuf[1024]="";
		sprintf(szBuf,"Surface must be in the range %d-%d",st_Grass,st_Ground);
		throw runtime_error(szBuf);
	}

	int oldW = this->w;
	int oldH = this->h;
	this->w = w;
	this->h = h;
	this->maxFactions = maxFactions;

	//create new cells
	//Cell **oldCells = cells;
	std::vector<std::vector<Cell> > oldCells = cells;

	//cells = new Cell*[w];
	cells.resize(w);
	for (int i = 0; i < w; i++) {
		//cells[i] = new Cell[h];
		cells[i].resize(h);
		for (int j = 0; j < h; j++) {
			cells[i][j].height = alt;
			cells[i][j].object = 0;
			cells[i][j].resource = 0;
			cells[i][j].surface = surf;
		}
	}

	int wOffset = w < oldW ? 0 : (w - oldW) / 2;
	int hOffset = h < oldH ? 0 : (h - oldH) / 2;
	//assign old values to cells
	for (int i = 0; i < oldW; i++) {
		for (int j = 0; j < oldH; j++) {
			if (i + wOffset < w && j + hOffset < h) {
				cells[i+wOffset][j+hOffset].height = oldCells[i][j].height;
				cells[i+wOffset][j+hOffset].object = oldCells[i][j].object;
				cells[i+wOffset][j+hOffset].resource = oldCells[i][j].resource;
				cells[i+wOffset][j+hOffset].surface = oldCells[i][j].surface;
			}
		}
	}
	for (int i = 0; i < maxFactions; ++i) {
		startLocations[i].x += wOffset;
		startLocations[i].y += hOffset;
	}

	//delete old cells
	//if (oldCells != NULL) {
	//	for (int i = 0; i < oldW; i++)
	//		delete [] oldCells[i];
	//	delete [] oldCells;
	//}
}

void MapPreview::resetFactions(int maxPlayers) {
	if (maxPlayers < MIN_MAP_FACTIONCOUNT || maxPlayers > MAX_MAP_FACTIONCOUNT) {
		char szBuf[1024]="";
		sprintf(szBuf,"Max Players must be in the range %d-%d",MIN_MAP_FACTIONCOUNT,MAX_MAP_FACTIONCOUNT);
		throw runtime_error(szBuf);
	}

	//if (startLocations != NULL) {
	//	delete [] startLocations;
	//	startLocations = NULL;
	//}
	startLocations.clear();

	maxFactions = maxPlayers;

	//startLocations = new StartLocation[maxFactions];
	startLocations.resize(maxFactions);
	for (int i = 0; i < maxFactions; i++) {
		startLocations[i].x = 0;
		startLocations[i].y = 0;
	}
}

void MapPreview::setTitle(const string &title) {
	this->title = title;
}

void MapPreview::setDesc(const string &desc) {
	this->desc = desc;
}

void MapPreview::setAuthor(const string &author) {
	this->author = author;
}

void MapPreview::setAdvanced(int heightFactor, int waterLevel, int cliffLevel) {
	this->heightFactor = heightFactor;
	this->waterLevel = waterLevel;
	this->cliffLevel = cliffLevel;
}

void MapPreview::randomizeHeights() {
	resetHeights(random.randRange(8, 10));
	sinRandomize(0);
	decalRandomize(4);
	sinRandomize(1);
}

void MapPreview::randomize() {
	randomizeHeights();

	int slPlaceFactorX = random.randRange(0, 1);
	int slPlaceFactorY = random.randRange(0, 1) * 2;

	for (int i = 0; i < maxFactions; ++i) {
		StartLocation sl;
		float slNoiseFactor = random.randRange(0.5f, 0.8f);

		sl.x = static_cast<int>(w * slNoiseFactor * ((i + slPlaceFactorX) % 2) + w * (1.f - slNoiseFactor) / 2.f);
		sl.y = static_cast<int>(h * slNoiseFactor * (((i + slPlaceFactorY) / 2) % 2) + h * (1.f - slNoiseFactor) / 2.f);
		startLocations[i] = sl;
	}
}

void MapPreview::switchSurfaces(MapSurfaceType surf1, MapSurfaceType surf2) {
	if (surf1 >= st_Grass && surf1 <= st_Ground && surf2 >= st_Grass && surf2 <= st_Ground) {
		for (int i = 0; i < w; ++i) {
			for (int j = 0; j < h; ++j) {
				if (cells[i][j].surface == surf1) {
					cells[i][j].surface = surf2;
				}
				else if (cells[i][j].surface == surf2) {
					cells[i][j].surface = surf1;
				}
			}
		}
	}
	else {
		throw runtime_error("Incorrect surfaces");
	}
}

void MapPreview::loadFromFile(const string &path) {

	FILE *f1 = fopen(path.c_str(), "rb");
	if (f1 != NULL) {

		//read header
		MapFileHeader header;
		size_t bytes = fread(&header, sizeof(MapFileHeader), 1, f1);

		heightFactor = header.heightFactor;
		waterLevel = header.waterLevel;
		title = header.title;
		author = header.author;
		cliffLevel = 0;
		if(header.version==1){
			desc = header.description;
		}
		else if(header.version==2){
			desc = header.version2.short_desc;
			cliffLevel=header.version2.cliffLevel;
		}

		//read start locations
		resetFactions(header.maxFactions);
		for (int i = 0; i < maxFactions; ++i) {
			bytes = fread(&startLocations[i].x, sizeof(int32), 1, f1);
			bytes = fread(&startLocations[i].y, sizeof(int32), 1, f1);
		}

		//read Heights
		reset(header.width, header.height, (float)DEFAULT_MAP_CELL_HEIGHT, DEFAULT_MAP_CELL_SURFACE_TYPE);
		for (int j = 0; j < h; ++j) {
			for (int i = 0; i < w; ++i) {
				bytes = fread(&cells[i][j].height, sizeof(float), 1, f1);
			}
		}

		//read surfaces
		for (int j = 0; j < h; ++j) {
			for (int i = 0; i < w; ++i) {
				bytes = fread(&cells[i][j].surface, sizeof(int8), 1, f1);
			}
		}

		//read objects
		for (int j = 0; j < h; ++j) {
			for (int i = 0; i < w; ++i) {
				int8 obj;
				bytes = fread(&obj, sizeof(int8), 1, f1);
				if (obj <= 10) {
					cells[i][j].object = obj;
				}
				else {
					cells[i][j].resource = obj - 10;
				}
			}
		}

		fclose(f1);

		fileLoaded = true;
	}
	else {
		throw runtime_error("error opening map file: " + path);
	}
}


void MapPreview::saveToFile(const string &path) {

	FILE *f1 = fopen(path.c_str(), "wb");
	if (f1 != NULL) {

		//write header
		MapFileHeader header;

		header.version = MAP_FORMAT_VERSION;
		header.maxFactions = maxFactions;
		header.width = w;
		header.height = h;
		header.heightFactor = heightFactor;
		header.waterLevel = waterLevel;
		strncpy(header.title, title.c_str(), MAX_TITLE_LENGTH);
		strncpy(header.author, author.c_str(), MAX_AUTHOR_LENGTH);
		strncpy(header.version2.short_desc, desc.c_str(), MAX_DESCRIPTION_LENGTH_VERSION2);
		header.version2.magic= 0x01020304;
		header.version2.cliffLevel= cliffLevel;


		fwrite(&header, sizeof(MapFileHeader), 1, f1);

		//write start locations
		for (int i = 0; i < maxFactions; ++i) {
			fwrite(&startLocations[i].x, sizeof(int32), 1, f1);
			fwrite(&startLocations[i].y, sizeof(int32), 1, f1);
		}

		//write Heights
		for (int j = 0; j < h; ++j) {
			for (int i = 0; i < w; ++i) {
				fwrite(&cells[i][j].height, sizeof(float32), 1, f1);
			}
		}

		//write surfaces
		for (int j = 0; j < h; ++j) {
			for (int i = 0; i < w; ++i) {
				fwrite(&cells[i][j].surface, sizeof(int8), 1, f1);
			}
		}

		//write objects
		for (int j = 0; j < h; ++j) {
			for (int i = 0; i < w; ++i) {
				if (cells[i][j].resource == 0)
					fwrite(&cells[i][j].object, sizeof(int8), 1, f1);
				else {
					int8 res = cells[i][j].resource + 10;
					fwrite(&res, sizeof(int8), 1, f1);
				}
			}
		}

		fclose(f1);

	}
	else {
		throw runtime_error("Error opening map file: " + path);
	}

	void randomHeight(int x, int y, int height);
}

// ==================== PRIVATE ====================

void MapPreview::resetHeights(int height) {
	for (int i = 0; i < w; ++i) {
		for (int j = 0; j < h; ++j) {
			cells[i][j].height = static_cast<float>(height);
		}
	}
}

void MapPreview::sinRandomize(int strenght) {
	float sinH1 = random.randRange(5.f, 40.f);
	float sinH2 = random.randRange(5.f, 40.f);
	float sinV1 = random.randRange(5.f, 40.f);
	float sinV2 = random.randRange(5.f, 40.f);
	float ah = static_cast<float>(10 + random.randRange(-2, 2));
	float bh = static_cast<float>((maxHeight - minHeight) / random.randRange(2, 3));
	float av = static_cast<float>(10 + random.randRange(-2, 2));
	float bv = static_cast<float>((maxHeight - minHeight) / random.randRange(2, 3));

	for (int i = 0; i < w; ++i) {
		for (int j = 0; j < h; ++j) {
			float normH = static_cast<float>(i) / w;
			float normV = static_cast<float>(j) / h;

#ifdef USE_STREFLOP
			float sh = (streflop::sinf(normH * sinH1) + streflop::sin(normH * sinH2)) / 2.f;
			float sv = (streflop::sinf(normV * sinV1) + streflop::sin(normV * sinV2)) / 2.f;
#else
			float sh = (sinf(normH * sinH1) + sin(normH * sinH2)) / 2.f;
			float sv = (sinf(normV * sinV1) + sin(normV * sinV2)) / 2.f;
#endif
			float newHeight = (ah + bh * sh + av + bv * sv) / 2.f;
			applyNewHeight(newHeight, i, j, strenght);
		}
	}
}

void MapPreview::decalRandomize(int strenght) {
	//first row
	int lastHeight = DEFAULT_MAP_CELL_HEIGHT;
	for (int i = 0; i < w; ++i) {
		lastHeight += random.randRange(-1, 1);
		lastHeight = clamp(lastHeight, minHeight, maxHeight);
		applyNewHeight(static_cast<float>(lastHeight), i, 0, strenght);
	}

	//other rows
	for (int j = 1; j < h; ++j) {
		int height = static_cast<int>(cells[0][j-1].height + random.randRange(-1, 1));
		applyNewHeight(static_cast<float>(clamp(height, minHeight, maxHeight)), 0, j, strenght);
		for (int i = 1; i < w; ++i) {
			height = static_cast<int>((cells[i][j-1].height + cells[i-1][j].height) / 2.f + random.randRange(-1, 1));
			float newHeight = static_cast<float>(clamp(height, minHeight, maxHeight));
			applyNewHeight(newHeight, i, j, strenght);
		}
	}
}

void MapPreview::applyNewHeight(float newHeight, int x, int y, int strenght) {
	cells[x][y].height = static_cast<float>(((cells[x][y].height * strenght) + newHeight) / (strenght + 1));
}

}}// end namespace
