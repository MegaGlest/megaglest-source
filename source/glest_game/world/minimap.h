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

#include "pixmap.h"
#include "texture.h"

namespace Glest{ namespace Game{

using Shared::Graphics::Vec4f;
using Shared::Graphics::Vec3f;
using Shared::Graphics::Vec2i;
using Shared::Graphics::Pixmap2D;
using Shared::Graphics::Texture2D;

class World;

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
	Texture2D *tex;
	Texture2D *fowTex;    //Fog Of War Texture2D
	bool fogOfWar;

private:
	static const float exploredAlpha;

public:
    void init(int x, int y, const World *world, bool fogOfWar);
	Minimap();
	~Minimap();

	const Texture2D *getFowTexture() const	{return fowTex;}
	const Texture2D *getTexture() const		{return tex;}

	void incFowTextureAlphaSurface(const Vec2i &sPos, float alpha);
	void resetFowTex();
	void updateFowTex(float t);

private:
	void computeTexture(const World *world);
};

}}//end namespace 

#endif
