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


#include "map.h"

#include <cmath>
#include <stdexcept>

using namespace Shared::Util;
using namespace std;

namespace MapEditor {

// ===============================================
//	class Map
// ===============================================

// ================== PUBLIC =====================

Map::Map() {
	altFactor = 3;
	waterLevel = 4;
	cells = NULL;
	startLocations = NULL;
	reset(128, 128, 10.f, 1);
	resetFactions(8);
	title = "";
	desc = "";
	author = "";
	refAlt = 10;
}

Map::~Map() {
	delete [] startLocations;
	startLocations = NULL;

	for (int i = 0; i < h; i++) {
		delete [] cells[i];
	}
	delete [] cells;
	cells = NULL;
}


float Map::getHeight(int x, int y) const {
	return cells[x][y].height;
}

int Map::getSurface(int x, int y) const {
	return cells[x][y].surface;
}

int Map::getObject(int x, int y) const {
	return cells[x][y].object;
}

int Map::getResource(int x, int y) const {
	return cells[x][y].resource;
}

int Map::getStartLocationX(int index) const {
	return startLocations[index].x;
}

int Map::getStartLocationY(int index) const {
	return startLocations[index].y;
}

static int get_dist(int delta_x, int delta_y) {
	float dx = delta_x;
	float dy = delta_y;

	return static_cast<int>(sqrtf(dx * dx + dy * dy));
}

void Map::glestChangeHeight(int x, int y, int height, int radius) {

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


void Map::pirateChangeHeight(int x, int y, int height, int radius) {
	// Make sure not to try and blanket change the height over the bounds
	// Find our goal height for the centre of the brush
	int goalAlt;
	int overBounds = refAlt + height;
	if (overBounds > 20) {
		goalAlt = 20;
	}
	else if (overBounds < 0) {
		goalAlt = 0;
	} else {
		goalAlt = overBounds;
	}

	// If the radius is 1 don't bother doing any calculations
	if (radius == 1) {
		if(inside(x, y)){
			cells[x][y].height = goalAlt;
		}
		return;
	}

	// Get Old height reference points and compute gradients
	// from the heights of the sides and corners of the brush to the centre goal height
	float gradient[3][3]; // [i][j]
	int indexI = 0;
	for (int i = x - radius; i <= x + radius; i += radius) {
	int indexJ = 0;
		for (int j = y - radius; j <= y + radius; j += radius) {
			// round off the corners
			int ti, tj;
			if (abs(i - x) == abs(j - y)) {
				ti = (i - x) * 0.707 + x + 0.5;
				tj = (j - y) * 0.707 + y + 0.5;
			} else {
				ti = i;
				tj = j;
			}
			if (inside(ti, tj)) {
				gradient[indexI][indexJ] = (cells[ti][tj].height - goalAlt) / radius;
			//} else if (dist == 0) {
				//gradient[indexI][indexJ] = 0;
			} else {
				// assume outside the map bounds is height 10
				gradient[indexI][indexJ] = (10.0 - goalAlt) / radius;
			}
			//std::cout << "gradient[" << indexI << "][" << indexJ << "] = " << gradient[indexI][indexJ] << std::endl;
			//std::cout << "derived from height " << cells[ti][tj].height << " at " << ti << " " << tj << std::endl;
			indexJ++;
		}
		indexI++;
	}
	//std::cout << endl;

	// A brush with radius n cells should have a true radius of n-1 distance
	radius -= 1;
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
						} else {
							normI[1] = 1;
						}
					} else if (normIf < 0.33) {
						normI[0] = 1;
						if (normIf > 0) {
							normI[1] = 2;
						} else if (normIf < 0) {
							normI[1] = 0;
						} else /*(normIf == 0)*/ {
							normI[1] = 1;
						}
					} else {
						normI[0] = 2;
						if (normIf == 1) {
							normI[1] = 2;
						} else {
							normI[1] = 1;
						}
					}
					// find nearest j indices
					if (normJf < -0.33) {
						normJ[0] = 0;
						if (normJf == 0) {
							normJ[1] = 0;
						} else {
							normJ[1] = 1;
						}
					} else if (normJf < 0.33) {
						normJ[0] = 1;
						if (normJf > 0) {
							normJ[1] = 2;
						} else if (normJf < 0) {
							normJ[1] = 0;
						} else /*(normJf == 0)*/ {
							normJ[1] = 1;
						}
					} else {
						normJ[0] = 2;
						if (normJf == 1) {
							normJ[1] = 2;
						} else {
							normJ[1] = 1;
						}
					}

					// Determine which gradients to use and take a weighted average
					if (abs(normIf) > abs(normJf)) {
						usedGrad =
								gradient[normI[0]] [normJ[0]] * abs(normJf) +
								gradient[normI[0]] [normJ[1]] * (1 - abs(normJf));
					} else if (abs(normIf) < abs(normJf)) {
						usedGrad =
								gradient[normI[0]] [normJ[0]] * abs(normIf) +
								gradient[normI[1]] [normJ[0]] * (1 - abs(normIf));
					} else {
						usedGrad =
								gradient[normI[0]] [normJ[0]];
					}


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

void Map::setHeight(int x, int y, float height) {
	cells[x][y].height = height;
}

void Map::setRefAlt(int x, int y) {
	if (inside(x, y)) {
		refAlt = static_cast<int>(cells[x][y].height);
	}
}

void Map::flipX() {
	Cell **oldCells = cells;

	cells = new Cell*[w];
	for (int i = 0; i < w; i++) {
		cells[i] = new Cell[h];
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

	for (int i = 0; i < w; i++) {
		delete [] oldCells[i];
	}
	delete [] oldCells;
}

void Map::flipY() {
	Cell **oldCells = cells;

	cells = new Cell*[w];
	for (int i = 0; i < w; i++) {
		cells[i] = new Cell[h];
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

	for (int i = 0; i < w; i++) {
		delete [] oldCells[i];
	}
	delete [] oldCells;
}

void Map::changeSurface(int x, int y, int surface, int radius) {
	int i, j;
	int dist;

	for (i = x - radius + 1; i < x + radius; i++) {
		for (j = y - radius + 1; j < y + radius; j++) {
			if (inside(i, j)) {
				dist = get_dist(i - x, j - y);
				if (radius >= dist) {
					cells[i][j].surface = surface;
				}
			}
		}
	}
}

void Map::setSurface(int x, int y, int surface) {
	cells[x][y].surface = surface;
}

void Map::changeObject(int x, int y, int object, int radius) {
	int i, j;
	int dist;

	for (i = x - radius + 1; i < x + radius; i++) {
		for (j = y - radius + 1; j < y + radius; j++) {
			if (inside(i, j)) {
				dist = get_dist(i - x, j - y);
				if (radius >= dist) {
					cells[i][j].object = object;
					cells[i][j].resource = 0;
				}
			}
		}
	}
}

void Map::setObject(int x, int y, int object) {
	cells[x][y].object = object;
	if (object != 0) cells[x][y].resource = 0;
}

void Map::changeResource(int x, int y, int resource, int radius) {
	int i, j;
	int dist;

	for (i = x - radius + 1; i < x + radius; i++) {
		for (j = y - radius + 1; j < y + radius; j++) {
			if (inside(i, j)) {
				dist = get_dist(i - x, j - y);
				if (radius >= dist) {
					cells[i][j].resource = resource;
					cells[i][j].object = 0;
				}
			}
		}
	}
}

void Map::setResource(int x, int y, int resource) {
	cells[x][y].resource = resource;
	if (resource != 0) cells[x][y].object = 0;
}

void Map::changeStartLocation(int x, int y, int faction) {
	if ((faction - 1) < maxFactions && inside(x, y)) {
		startLocations[faction].x = x;
		startLocations[faction].y = y;
	}
}

bool Map::inside(int x, int y) {
	return (x >= 0 && x < w && y >= 0 && y < h);
}

void Map::reset(int w, int h, float alt, int surf) {
	if (w < 16 || h < 16) {
		throw runtime_error("Size of map must be at least 16x16");
		return;
	}

	if (w > 1024 || h > 1024) {
		throw runtime_error("Size of map can be at most 1024x1024");
		return;
	}

	if (alt < 0 || alt > 20) {
		throw runtime_error("Height must be in the range 0-20");
		return;
	}

	if (surf < 1 || surf > 5) {
		throw runtime_error("Surface must be in the range 1-5");
		return;
	}

	if (cells != NULL) {
		for (int i = 0; i < this->w; i++) {
			delete [] cells[i];
		}
		delete [] cells;
	}

	this->w = w;
	this->h = h;
	this->maxFactions = maxFactions;

	cells = new Cell*[w];
	for (int i = 0; i < w; i++) {
		cells[i] = new Cell[h];
		for (int j = 0; j < h; j++) {
			cells[i][j].height = alt;
			cells[i][j].object = 0;
			cells[i][j].resource = 0;
			cells[i][j].surface = surf;
		}
	}
}

void Map::resize(int w, int h, float alt, int surf) {
	if (w < 16 || h < 16) {
		throw runtime_error("Size of map must be at least 16x16");
		return;
	}

	if (w > 1024 || h > 1024) {
		throw runtime_error("Size of map can be at most 1024x1024");
		return;
	}

	if (alt < 0 || alt > 20) {
		throw runtime_error("Height must be in the range 0-20");
		return;
	}

	if (surf < 1 || surf > 5) {
		throw runtime_error("Surface must be in the range 1-5");
		return;
	}

	int oldW = this->w;
	int oldH = this->h;
	this->w = w;
	this->h = h;
	this->maxFactions = maxFactions;

	//create new cells
	Cell **oldCells = cells;
	cells = new Cell*[w];
	for (int i = 0; i < w; i++) {
		cells[i] = new Cell[h];
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
	if (oldCells != NULL) {
		for (int i = 0; i < oldW; i++)
			delete [] oldCells[i];
		delete [] oldCells;
	}
}

void Map::resetFactions(int maxPlayers) {
	if (maxPlayers<1 || maxPlayers>8){
		throw runtime_error("Max Players must be in the range 1-8");
	}

	if (startLocations != NULL) {
		delete [] startLocations;
		startLocations = NULL;
	}

	maxFactions = maxPlayers;

	startLocations = new StartLocation[maxFactions];
	for (int i = 0; i < maxFactions; i++) {
		startLocations[i].x = 0;
		startLocations[i].y = 0;
	}
}

void Map::setTitle(const string &title) {
	this->title = title;
}

void Map::setDesc(const string &desc) {
	this->desc = desc;
}

void Map::setAuthor(const string &author) {
	this->author = author;
}

void Map::setAdvanced(int altFactor, int waterLevel) {
	this->altFactor = altFactor;
	this->waterLevel = waterLevel;
}

int Map::getHeightFactor() const {
	return altFactor;
}

int Map::getWaterLevel() const {
	return waterLevel;
}

void Map::randomizeHeights() {
	resetHeights(random.randRange(8, 10));
	sinRandomize(0);
	decalRandomize(4);
	sinRandomize(1);
}

void Map::randomize() {
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

void Map::switchSurfaces(int surf1, int surf2) {
	if (surf1 > 0 && surf1 <= 5 && surf2 > 0 && surf2 <= 5) {
		for (int i = 0; i < w; ++i) {
			for (int j = 0; j < h; ++j) {
				if (cells[i][j].surface == surf1) {
					cells[i][j].surface = surf2;
				} else if (cells[i][j].surface == surf2) {
					cells[i][j].surface = surf1;
				}
			}
		}
	} else {
		throw runtime_error("Incorrect surfaces");
	}
}

void Map::loadFromFile(const string &path) {

	FILE *f1 = fopen(path.c_str(), "rb");
	if (f1 != NULL) {

		//read header
		MapFileHeader header;
		fread(&header, sizeof(MapFileHeader), 1, f1);

		altFactor = header.altFactor;
		waterLevel = header.waterLevel;
		title = header.title;
		author = header.author;
		desc = header.description;

		//read start locations
		resetFactions(header.maxFactions);
		for (int i = 0; i < maxFactions; ++i) {
			fread(&startLocations[i].x, sizeof(int32), 1, f1);
			fread(&startLocations[i].y, sizeof(int32), 1, f1);
		}

		//read Heights
		reset(header.width, header.height, 10, 1);
		for (int j = 0; j < h; ++j) {
			for (int i = 0; i < w; ++i) {
				fread(&cells[i][j].height, sizeof(float), 1, f1);
			}
		}

		//read surfaces
		for (int j = 0; j < h; ++j) {
			for (int i = 0; i < w; ++i) {
				fread(&cells[i][j].surface, sizeof(int8), 1, f1);
			}
		}

		//read objects
		for (int j = 0; j < h; ++j) {
			for (int i = 0; i < w; ++i) {
				int8 obj;
				fread(&obj, sizeof(int8), 1, f1);
				if (obj <= 10) {
					cells[i][j].object = obj;
				} else {
					cells[i][j].resource = obj - 10;
				}
			}
		}

		fclose(f1);
	} else {
		throw runtime_error("error opening map file: " + path);
	}
}


void Map::saveToFile(const string &path) {

	FILE *f1 = fopen(path.c_str(), "wb");
	if (f1 != NULL) {

		//write header
		MapFileHeader header;

		header.version = 1;
		header.maxFactions = maxFactions;
		header.width = w;
		header.height = h;
		header.altFactor = altFactor;
		header.waterLevel = waterLevel;
		strncpy(header.title, title.c_str(), 128);
		strncpy(header.author, author.c_str(), 128);
		strncpy(header.description, desc.c_str(), 256);

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

	} else {
		throw runtime_error("Error opening map file: " + path);
	}

	void randomHeight(int x, int y, int height);
}

// ==================== PRIVATE ====================

void Map::resetHeights(int height) {
	for (int i = 0; i < w; ++i) {
		for (int j = 0; j < h; ++j) {
			cells[i][j].height = static_cast<float>(height);
		}
	}
}

void Map::sinRandomize(int strenght) {
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

			float sh = (sinf(normH * sinH1) + sin(normH * sinH2)) / 2.f;
			float sv = (sinf(normV * sinV1) + sin(normV * sinV2)) / 2.f;

			float newHeight = (ah + bh * sh + av + bv * sv) / 2.f;
			applyNewHeight(newHeight, i, j, strenght);
		}
	}
}

void Map::decalRandomize(int strenght) {
	//first row
	int lastHeight = 10;
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

void Map::applyNewHeight(float newHeight, int x, int y, int strenght) {
	cells[x][y].height = static_cast<float>(((cells[x][y].height * strenght) + newHeight) / (strenght + 1));
}

}// end namespace
