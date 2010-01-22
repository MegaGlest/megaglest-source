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

#include "water_effects.h"

#include "config.h"
#include "map.h"
#include "leak_dumper.h"

namespace Glest{ namespace Game{

// =====================================================
//	class WaterSplash
// =====================================================

WaterSplash::WaterSplash(const Vec2f &pos){
	this->pos= pos;
	anim= 0.f;
	enabled= true;

}

void WaterSplash::update(float amount){
	if(enabled){
		anim+= amount;
		if(anim>1.f){
			enabled= false;
		}
	}
}

// ===============================
// 	class WaterEffects  
// ===============================

WaterEffects::WaterEffects(){
	anim= 0;
}

void WaterEffects::update(){
	anim+= 0.5f/GameConstants::updateFps;
	if(anim>1.f){
		anim= 0;
	}
	for(int i=0; i<waterSplashes.size(); ++i){
		waterSplashes[i].update(1.f/GameConstants::updateFps);
	}
}

void WaterEffects::addWaterSplash(const Vec2f &pos){
	for(int i=0; i<waterSplashes.size(); ++i){
		if(!waterSplashes[i].getEnabled()){
			waterSplashes[i]= WaterSplash(pos);
			return;
		}
	}
	waterSplashes.push_back(WaterSplash(pos));
}

}}//end namespace
