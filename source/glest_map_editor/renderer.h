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

#ifndef _MAPEDITOR_RENDERER_H_
#define _MAPEDITOR_RENDERER_H_

#include "map.h"

namespace MapEditor {

// ===============================================
//	class Renderer
// ===============================================

class Renderer {
public:
	void init(int clientW, int clientH);
	void renderMap(Map *map, int x, int y, int clientW, int clientH, int cellSize);
};

}// end namespace

#endif
