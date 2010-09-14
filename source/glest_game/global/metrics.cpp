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

#include "metrics.h"	

//#include "element_type.h"
#include <stdexcept>
#include "leak_dumper.h"

using namespace std;

namespace Glest{ namespace Game{


// =====================================================
// 	class Metrics
// =====================================================

Metrics::Metrics(){
	Config &config= Config::getInstance();
	
	virtualW= 1000;
	virtualH= 750;

	screenW= config.getInt("ScreenWidth");
	screenH= config.getInt("ScreenHeight");
	
	minimapX= 10;
	minimapY= 750-128-30+16;
	minimapW= 128;
	minimapH= 128;

	displayX= 800;
	displayY= 250;
	displayW= 128;
	displayH= 480;	
}

const Metrics &Metrics::getInstance(){
	static const Metrics metrics;
	return metrics;
}

float Metrics::getAspectRatio() const{
	if(screenH == 0) {
		throw runtime_error("div by 0 screenH == 0");
	}
	return static_cast<float>(screenW)/screenH;
}

int Metrics::toVirtualX(int w) const{
	if(screenW == 0) {
		throw runtime_error("div by 0 screenW == 0");
	}
	return w*virtualW/screenW;
}

int Metrics::toVirtualY(int h) const{
	if(screenH == 0) {
		throw runtime_error("div by 0 screenH == 0");
	}
	return h*virtualH/screenH;
}

bool Metrics::isInDisplay(int x, int y) const{
	return 
        x > displayX &&
        y > displayY &&
        x < displayX+displayW &&
        y < displayY+displayH;
}

bool Metrics::isInMinimap(int x, int y) const{
	return 
        x > minimapX &&
        y > minimapY &&
        x < minimapX+minimapW &&
        y < minimapY+minimapH;
}

}}// end namespace
