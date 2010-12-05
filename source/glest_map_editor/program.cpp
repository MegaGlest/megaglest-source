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


#include "program.h"

#include "util.h"

#include <iostream>
using namespace Shared::Util;

namespace MapEditor {

////////////////////////////
// class UndoPoint
////////////////////////////
int UndoPoint::w = 0;
int UndoPoint::h = 0;

UndoPoint::UndoPoint()
		: change(ctNone)
		, surface(0)
		, object(0)
		, resource(0)
		, height(0) {
	w = Program::map->getW();
	h = Program::map->getH();
}
/*
UndoPoint::UndoPoint(ChangeType change) {
	w = Program::map->getW();
	h = Program::map->getH();

	init(change);
}*/

void UndoPoint::init(ChangeType change) {
	this->change = change;
	switch (change) {
		// Back up everything
		case ctAll:
		// Back up heights
		case ctHeight:
		case ctGradient:
			// Build an array of heights from the map
			//std::cout << "Building an array of heights to use for our UndoPoint" << std::endl;
			height = new float[w * h];
			for (int i = 0; i < w; ++i) {
				for (int j = 0; j < h; ++j) {
					 height[j * w + i] = Program::map->getHeight(i, j);
				}
			}
			//std::cout << "Built the array" << std::endl;
			if (change != ctAll) break;
		// Back up surfaces
		case ctSurface:
			surface = new int[w * h];
			for (int i = 0; i < w; ++i) {
				for (int j = 0; j < h; ++j) {
					 surface[j * w + i] = Program::map->getSurface(i, j);
				}
			}
			if (change != ctAll) break;
		// Back up both objects and resources if either changes
		case ctObject:
		case ctResource:
			object = new int[w * h];
			for (int i = 0; i < w; ++i) {
				for (int j = 0; j < h; ++j) {
					 object[j * w + i] = Program::map->getObject(i, j);
				}
			}
			resource = new int[w * h];
			for (int i = 0; i < w; ++i) {
				for (int j = 0; j < h; ++j) {
					 resource[j * w + i] = Program::map->getResource(i, j);
				}
			}
			break;
	}
}

UndoPoint::~UndoPoint() {
	delete [] height;
	delete [] resource;
	delete [] object;
	delete [] surface;
}

void UndoPoint::revert() {
	//std::cout << "attempting to revert last changes to " << undoID << std::endl;
	switch (change) {
		// Revert Everything
		case ctAll:
		// Revert Heights
		case ctHeight:
		case ctGradient:
			// Restore the array of heights to the map
			//std::cout << "attempting to restore the height array" << std::endl;
			for (int i = 0; i < w; i++) {
				for (int j = 0; j < h; j++) {
					 Program::map->setHeight(i, j, height[j * w + i]);
				}
			}
			if (change != ctAll) break;
		// Revert surfaces
		case ctSurface:
			//std::cout << "attempting to restore the surface array" << std::endl;
			for (int i = 0; i < w; i++) {
				for (int j = 0; j < h; j++) {
					 Program::map->setSurface(i, j, static_cast<MapSurfaceType>(surface[j * w + i]));
				}
			}
			if (change != ctAll) break;
		// Revert objects and resources
		case ctObject:
		case ctResource:
			for (int i = 0; i < w; i++) {
				for (int j = 0; j < h; j++) {
					 Program::map->setObject(i, j, object[j * w + i]);
				}
			}
			for (int i = 0; i < w; i++) {
				for (int j = 0; j < h; j++) {
					 Program::map->setResource(i, j, resource[j * w + i]);
				}
			}
			break;
	}
	//std::cout << "reverted changes (we hope)" << std::endl;
}

// ===============================================
//	class Program
// ===============================================

MapPreview *Program::map = NULL;

Program::Program(int w, int h) {
	cellSize = 6;
	grid=false;
	ofsetX = 0;
	ofsetY = 0;
	map = new MapPreview();
	renderer.initMapSurface(w, h);
}

Program::~Program() {
	delete map;
}

int Program::getObject(int x, int y) {
	int i=(x - ofsetX) / cellSize;
	int j= (y + ofsetY) / cellSize;
	if (map->inside(i, j)) {
		return map->getObject(i,j);
	}
	else{
		return 0;
	}
}

int Program::getResource(int x, int y) {
	int i=(x - ofsetX) / cellSize;
	int j= (y + ofsetY) / cellSize;
	if (map->inside(i, j)) {
		return map->getResource(i,j);
	}
	else{
		return 0;
	}
}

// TODO: move editor-specific code from shared_lib to here.
void Program::glestChangeMapHeight(int x, int y, int Height, int radius) {
	map->glestChangeHeight((x - ofsetX) / cellSize, (y + ofsetY) / cellSize, Height, radius);
}

void Program::pirateChangeMapHeight(int x, int y, int Height, int radius) {
	map->pirateChangeHeight((x - ofsetX) / cellSize, (y + ofsetY) / cellSize, Height, radius);
}

void Program::changeMapSurface(int x, int y, int surface, int radius) {
	map->changeSurface((x - ofsetX) / cellSize, (y + ofsetY) / cellSize, static_cast<MapSurfaceType>(surface), radius);
}

void Program::changeMapObject(int x, int y, int object, int radius) {
	map->changeObject((x - ofsetX) / cellSize, (y + ofsetY) / cellSize, object, radius);
}

void Program::changeMapResource(int x, int y, int resource, int radius) {
	map->changeResource((x - ofsetX) / cellSize, (y + ofsetY) / cellSize, resource, radius);
}

void Program::changeStartLocation(int x, int y, int player) {
	map->changeStartLocation((x - ofsetX) / cellSize, (y + ofsetY) / cellSize, player);
}

void Program::setUndoPoint(ChangeType change) {
	if (change == ctLocation) return;

	undoStack.push(UndoPoint());
	undoStack.top().init(change);

	redoStack.clear();
}

bool Program::undo() {
	if (undoStack.empty()) {
		return false;
	}
	// push current state onto redo stack
	redoStack.push(UndoPoint());
	redoStack.top().init(undoStack.top().getChange());

	undoStack.top().revert();
	undoStack.pop();
	return true;
}

bool Program::redo() {
	if (redoStack.empty()) {
		return false;
	}
	// push current state onto undo stack
	undoStack.push(UndoPoint());
	undoStack.top().init(redoStack.top().getChange());

	redoStack.top().revert();
	redoStack.pop();
	return true;
}

void Program::renderMap(int w, int h) {
	renderer.renderMap(map, ofsetX, ofsetY, w, h, cellSize, grid);
}

void Program::setRefAlt(int x, int y) {
	map->setRefAlt((x - ofsetX) / cellSize, (y + ofsetY) / cellSize);
}

void Program::flipX() {
	map->flipX();
}

void Program::flipY() {
	map->flipY();
}

void Program::mirrorX() { // copy left to right
    int w=map->getW();
    int h=map->getH();
	for (int i = 0; i < w/2; i++) {
		for (int j = 0; j < h; j++) {
			map->copyXY(w-i-1,j  ,  i,j);
		}
	}
}

void Program::mirrorY() { // copy top to bottom
    int w=map->getW();
    int h=map->getH();
	for (int i = 0; i < w; i++) {
		for (int j = 0; j < h/2; j++) {
			map->copyXY(i,h-j-1  ,  i,j);
		}
	}
}

void Program::mirrorXY() { // copy leftbottom to topright, can handle non-sqaure maps
    int w=map->getW();
    int h=map->getH();
    if (h==w) {
        for (int i = 0; i < w-1; i++) {
            for (int j = i+1; j < h; j++) {
                map->copyXY(j,i  ,  i,j);
            }
        }
    }
    // Non-sqaure maps:
    else if (h < w) { // copy horizontal strips
        int s=w/h; // 2 if twice as wide as heigh
        for (int i = 0; i < w/s-1; i++) {
            for (int j = i+1; j < h; j++) {
                for (int p = 0; p < s; p++) map->copyXY(j*s+p,i  ,  i*s+p,j);
            }
        }
    }
    else { // copy vertical strips
        int s=h/w; // 2 if twice as heigh as wide
        for (int i = 0; i < w-1; i++) {
            for (int j = i+1; j < h/s; j++) {
                for (int p = 0; p < s; p++) map->copyXY(j,i*s+p  ,  i,j*s+p);
            }
        }
    }
}

void Program::rotatecopyX() {
    int w=map->getW();
    int h=map->getH();
	for (int i = 0; i < w/2; i++) {
		for (int j = 0; j < h; j++) {
			map->copyXY(w-i-1,h-j-1  ,  i,j);
		}
	}
}

void Program::rotatecopyY() {
    int w=map->getW();
    int h=map->getH();
	for (int i = 0; i < w; i++) {
		for (int j = 0; j < h/2; j++) {
			map->copyXY(w-i-1,h-j-1  ,  i,j);
		}
	}
}

void Program::rotatecopyXY() {
    int w=map->getW();
    int h=map->getH();
    int sw=w/h; if(sw<1) sw=1; // x-squares per y
    int sh=h/w; if(sh<1) sh=1; // y-squares per x
    if (sh==1)
        for (int j = 0; j < h-1; j++) { // row by row!
            for (int i = j*sw; i < w; i++) {
                map->copyXY(i,j  ,  w-i-1,h-j-1);
            }
        }
    else
        for (int i = 0; i <w; i++) { // column for colum!
            for (int j = h-1-i*sh; j >= 0; j--) {
                map->copyXY(w-i-1,j  ,  i,h-j-1);
            }
        }
}

void Program::rotatecopyCorner() { // rotate top left 1/4 to top right 1/4
    int w=map->getW();
    int h=map->getH();
    if (h==w) {
        for (int i = 0; i < w/2; i++) {
            for (int j = 0; j < h/2; j++) {
                map->copyXY(w-j-1,i  ,  i,j);
            }
        }
    }
    // Non-sqaure maps:
    else if (h < w) { // copy horizontal strips
        int s=w/h; // 2 if twice as wide as heigh
        for (int i = 0; i < w/s/2; i++) {
            for (int j = 0; j < h/2; j++) {
                for (int p = 0; p < s; p++) map->copyXY(w-j*s-1-p,i  ,  i*s+p,j);
            }
        }
    }
    else { // copy vertical strips
        int s=h/w; // 2 if twice as heigh as wide
        for (int i = 0; i < w/2; i++) {
            for (int j = 0; j < h/s/2; j++) {
                for (int p = 0; p < s; p++) map->copyXY(w-j-1,i*s+p  ,  i,j*s+p);
            }
        }
    }
}


void Program::randomizeMapHeights() {
	map->randomizeHeights();
}

void Program::randomizeMap() {
	map->randomize();
}

void Program::switchMapSurfaces(int surf1, int surf2) {
	map->switchSurfaces(static_cast<MapSurfaceType>(surf1), static_cast<MapSurfaceType>(surf2));
}

void Program::reset(int w, int h, int alt, int surf) {
	undoStack.clear();
	redoStack.clear();
	map->reset(w, h, (float) alt, static_cast<MapSurfaceType>(surf));
}

void Program::resize(int w, int h, int alt, int surf) {
	map->resize(w, h, (float) alt, static_cast<MapSurfaceType>(surf));
}

void Program::resetFactions(int maxFactions) {
	map->resetFactions(maxFactions);
}

bool Program::setMapTitle(const string &title) {
	if (map->getTitle() != title) {
		map->setTitle(title);
		return true;
	}
	return false;
}

bool Program::setMapDesc(const string &desc) {
	if (map->getDesc() != desc) {
		map->setDesc(desc);
		return true;
	}
	return false;
}

bool Program::setMapAuthor(const string &author) {
	if (map->getAuthor() != author) {
		map->setAuthor(author);
		return true;
	}
	return false;
}

void Program::setOfset(int x, int y) {
	ofsetX += x;
	ofsetY -= y;
}

void Program::incCellSize(int i) {

	int minInc = 2 - cellSize;

	if (i < minInc) {
		i = minInc;
	}
	cellSize += i;
	ofsetX -= (map->getW() * i) / 2;
	ofsetY += (map->getH() * i) / 2;
}

void Program::resetOfset() {
	ofsetX = 0;
	ofsetY = 0;
	cellSize = 6;
}

bool Program::setGridOnOff() {
    grid=!grid;
    return grid;
}

void Program::setMapAdvanced(int altFactor, int waterLevel) {
	map->setAdvanced(altFactor, waterLevel);
}

void Program::loadMap(const string &path) {
	undoStack.clear();
	redoStack.clear();
	map->loadFromFile(path);
}

void Program::saveMap(const string &path) {
	map->saveToFile(path);
}

}// end namespace
