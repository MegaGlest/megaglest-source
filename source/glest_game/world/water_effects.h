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

#ifndef _GLEST_GAME_WATER_EFFECTS_H_
#define _GLEST_GAME_WATER_EFFECTS_H_

#include <vector>
#include "vec.h"
#include "leak_dumper.h"

using std::vector;

using Shared::Graphics::Vec2f;

namespace Glest{ namespace Game{

class Map;

// =====================================================
//	class WaterSplash
// =====================================================

class WaterSplash{
private:
	Vec2f pos;
	int size;
	float anim;
	bool enabled;

public:
	WaterSplash(const Vec2f &pos, int size);

	void update(float amount);

	const Vec2f &getPos() const	{return pos;}
	const int &getSize() const	{return size;}
	float getAnim() const		{return anim;}
	bool getEnabled() const		{return enabled;}
};

// ===============================
// 	class WaterEffects  
//
/// List of water splashes
// ===============================

class WaterEffects{
public:
	typedef vector<WaterSplash> WaterSplashes;

private:
	WaterSplashes waterSplashes;
	float anim;
		
public:
	WaterEffects();

	void update(float speed);

	float getAmin() const	{return anim;}

	void addWaterSplash(const Vec2f &pos, int size);
	int getWaterSplashCount() const					{return waterSplashes.size();}
	const WaterSplash *getWaterSplash(int i) const	{return &waterSplashes[i];}
};

}}//end namespace

#endif
