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

#ifndef _GLEST_GAME_MINIMAP_H_
#define _GLEST_GAME_MINIMAP_H_

#ifdef WIN32
    #include <winsock2.h>
    #include <winsock.h>
#endif

#include "pixmap.h"
#include "texture.h"
#include "xml_parser.h"
#include "leak_dumper.h"

namespace Glest{ namespace Game{

using Shared::Graphics::Vec4f;
using Shared::Graphics::Vec3f;
using Shared::Graphics::Vec2i;
using Shared::Graphics::Pixmap2D;
using Shared::Graphics::Texture2D;
using Shared::Xml::XmlNode;

class World;
class GameSettings;

enum ExplorationState{
    esNotExplored,
    esExplored,
    esVisible
};

// =====================================================
// 	class Minimap
//
/// State of the in-game minimap
// =====================================================

class Minimap{
private:
	Pixmap2D *fowPixmap0;
	Pixmap2D *fowPixmap1;
	Pixmap2D *fowPixmap0Copy;
	Pixmap2D *fowPixmap1Copy;

	Texture2D *tex;
	Texture2D *fowTex;    //Fog Of War Texture2D
	bool fogOfWar;
	const GameSettings *gameSettings;

private:
	static const float exploredAlpha;

public:
    void init(int x, int y, const World *world, bool fogOfWar);
	Minimap();
	~Minimap();

	const Texture2D *getFowTexture() const	{return fowTex;}
	const Texture2D *getTexture() const		{return tex;}

	void incFowTextureAlphaSurface(const Vec2i &sPos, float alpha, bool isIncrementalUpdate=false);
	void resetFowTex();
	void updateFowTex(float t);
	void setFogOfWar(bool value);

	void copyFowTex();
	void restoreFowTex();

	void saveGame(XmlNode *rootNode);
	void loadGame(const XmlNode *rootNode);

private:
	void computeTexture(const World *world);
};

}}//end namespace

#endif
