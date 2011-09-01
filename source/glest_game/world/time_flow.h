// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martio Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_TIMEFLOW_H_
#define _GLEST_GAME_TIMEFLOW_H_

#include "tileset.h"
#include "sound.h"
#include "leak_dumper.h"

namespace Glest{ namespace Game{

using Shared::Sound::StrSound;
using Shared::Sound::StrSound;
using Shared::Sound::StaticSound;


// =====================================================
// 	class TimeFlow  
//
/// Raises time related events (day/night cycle) 
// =====================================================

class TimeFlow{
public:
	static const float dusk;
	static const float dawn;

private:
	bool firstTime;
	Tileset *tileset;
	float time;
	float lastTime;
	float timeInc;

public:
	void init(Tileset *tileset);

	float getTime() const				{return time;}
	bool isDay() const					{return time>dawn && time<dusk;}
	bool isNight() const				{return !isDay();}
	bool isTotalNight() const			{return time<dawn+1.f || time>dusk-1.f;}
	float getTimeInc() const			{return timeInc;}

	void update();
private:
	bool isAproxTime(float time) const;
};

}} //end namespace

#endif
