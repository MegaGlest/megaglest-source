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

#ifndef _MAPEDITOR_PROGRAM_H_
#define _MAPEDITOR_PROGRAM_H_

//#include "map.h"
#include "map_preview.h"
//#include "renderer.h"
#include "base_renderer.h"

#include <stack>

using std::stack;
using namespace Shared::Map;
using namespace Shared::Graphics;

namespace MapEditor {

class MainWindow;

enum ChangeType {
	ctNone = -1,
	ctHeight,
	ctSurface,
	ctObject,
	ctResource,
	ctLocation,
	ctGradient,
	ctAll
};

// =============================================
// class Undo Point
// A linked list class that is more of an extension / modification on
// the already existing Cell struct in map.h
// Provides the ability to only specify a certain property of the map to change
// =============================================
class UndoPoint {
	private:
		// Only keep a certain number of undo points in memory otherwise
		// Big projects could hog a lot of memory
		const static int MAX_UNDO_LIST_SIZE = 100; // TODO get feedback on this value
		static int undoCount;

		ChangeType change;

		// Pointers to arrays of each property
		int *surface;
		int *object;
		int *resource;
		float *height;

		// Map width and height
		static int w;
		static int h;

	public:
		UndoPoint();
		~UndoPoint();
		void init(ChangeType change);
		void revert();

		inline ChangeType getChange() const 	{ return change; }
};

class ChangeStack : public std::stack<UndoPoint> {
public:
	static const unsigned int maxSize = 100;

	void clear() { c.clear(); }

	void push(UndoPoint p) {
		if (c.size() >= maxSize) {
			c.pop_front();
		}
		stack<UndoPoint>::push(p);
	}
};

// ===============================================
// class Program
// ===============================================

class Program {
private:
	//Renderer renderer;
	BaseRenderer renderer;
	int ofsetX, ofsetY;
	int cellSize;
	bool grid; // show grid option
	//static Map *map;
	static MapPreview *map;
	friend class UndoPoint;

	ChangeStack undoStack, redoStack;

public:
	Program(int w, int h);
	~Program();

	//map cell change
	void glestChangeMapHeight(int x, int y, int Height, int radius);
	void pirateChangeMapHeight(int x, int y, int Height, int radius);
	void changeMapSurface(int x, int y, int surface, int radius);
	void changeMapObject(int x, int y, int object, int radius);
	void changeMapResource(int x, int y, int resource, int radius);
	void changeStartLocation(int x, int y, int player);

	void setUndoPoint(ChangeType change);
	bool undo();
	bool redo();

	//map ops
	void reset(int w, int h, int alt, int surf);
	void resize(int w, int h, int alt, int surf);
	void resetFactions(int maxFactions);
	void setRefAlt(int x, int y);
	void flipX();
	void flipY();
	void mirrorX();
	void mirrorY();
	void mirrorXY();
	void rotatecopyX();
	void rotatecopyY();
	void rotatecopyXY();
	void rotatecopyCorner();
	void shiftLeft();
	void shiftRight();
	void shiftUp();
	void shiftDown();

	void randomizeMapHeights();
	void randomizeMap();
	void switchMapSurfaces(int surf1, int surf2);
	void loadMap(const string &path);
	void saveMap(const string &path);

	//map misc
	bool setMapTitle(const string &title);
	bool setMapDesc(const string &desc);
	bool setMapAuthor(const string &author);
	void setMapAdvanced(int altFactor, int waterLevel, int minimumCliffHeight);

	//misc
	void renderMap(int w, int h);
	void setOfset(int x, int y);
	void incCellSize(int i);
	void resetOfset();
	bool setGridOnOff();

	int getObject(int x, int y);
	int getResource(int x, int y);
	static const MapPreview *getMap() {return map;}
};

}// end namespace

#endif
