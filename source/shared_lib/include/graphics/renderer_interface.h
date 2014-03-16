// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2014 Mark Vejvoda
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_GRAPHICS_RENDERERINTERFACE_H_
#define _SHARED_GRAPHICS_RENDERERINTERFACE_H_

#include "map_preview.h"
#include <map>

#include "leak_dumper.h"

using namespace Shared::Map;

namespace Shared { namespace Graphics {

class Texture2D;
class Model;

enum ResourceScope {
	rsGlobal,
	rsMenu,
	rsGame,

	rsCount
};

class RendererInterface {
public:
	virtual Texture2D *newTexture2D(ResourceScope rs) = 0;
	virtual Model *newModel(ResourceScope rs,const string &path,bool deletePixMapAfterLoad=false,std::map<string,vector<pair<string, string> > > *loadedFileList=NULL, string *sourceLoader=NULL) = 0;

	virtual ~RendererInterface() {}
};

class RendererMapInterface {
public:
	virtual void initMapSurface(int clientW, int clientH) = 0;
	virtual void renderMap(MapPreview *map, int x, int y, int clientW, int clientH, int cellSize, bool grid, bool heightMap, bool hideWater) = 0;

	virtual ~RendererMapInterface() {}
};

}}//end namespace

#endif
