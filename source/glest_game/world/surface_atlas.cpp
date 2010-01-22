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

#include "surface_atlas.h"

#include <stdexcept>
#include <algorithm>

#include "renderer.h"
#include "util.h"
#include "math_util.h"
#include "leak_dumper.h"

using namespace std;
using namespace Shared::Util;
using namespace Shared::Graphics;

namespace Glest{ namespace Game{

// =====================================================
//	class PixmapInfo
// =====================================================

SurfaceInfo::SurfaceInfo(const Pixmap2D *lu, const Pixmap2D *ru, const Pixmap2D *ld, const Pixmap2D *rd){
	this->leftDown= ld;
	this->leftUp= lu;
	this->rightDown= rd;
	this->rightUp= ru;
	center= NULL;
}

SurfaceInfo::SurfaceInfo(const Pixmap2D *center){
	this->leftDown= NULL;
	this->leftUp= NULL;
	this->rightDown= NULL;
	this->rightUp= NULL;
	this->center= center;
}

bool SurfaceInfo::operator==(const SurfaceInfo &si) const{
	return 
		this->center == si.getCenter() &&
		this->leftDown == si.getLeftDown() &&
		this->leftUp == si.getLeftUp() &&
		this->rightDown == si.getRightDown() &&
		this->rightUp == si.getRightUp();
}

// ===============================
// 	class SurfaceAtlas
// ===============================

SurfaceAtlas::SurfaceAtlas(){
	surfaceSize= -1;
}

void SurfaceAtlas::addSurface(SurfaceInfo *si){
	
	//check dimensions
	if(si->getCenter()!=NULL){
		checkDimensions(si->getCenter());
	}
	else{
		checkDimensions(si->getLeftUp());
		checkDimensions(si->getLeftDown());
		checkDimensions(si->getRightUp());
		checkDimensions(si->getRightDown());
	}

	//add info
	SurfaceInfos::iterator it= find(surfaceInfos.begin(), surfaceInfos.end(), *si);
	if(it==surfaceInfos.end()){
		//add new texture
		Texture2D *t= Renderer::getInstance().newTexture2D(rsGame);
		t->setWrapMode(Texture::wmClampToEdge);
		t->getPixmap()->init(surfaceSize, surfaceSize, 3);
		
		si->setCoord(Vec2f(0.f, 0.f));
		si->setTexture(t);
		surfaceInfos.push_back(*si);
		
		//copy texture to pixmap
		if(si->getCenter()!=NULL){
			t->getPixmap()->copy(si->getCenter());
		}
		else{
			t->getPixmap()->splat(si->getLeftUp(), si->getRightUp(), si->getLeftDown(), si->getRightDown());
		}
	}
	else{
		si->setCoord(it->getCoord());
		si->setTexture(it->getTexture());
	}
}

float SurfaceAtlas::getCoordStep() const{
	return 1.f;
}

void SurfaceAtlas::checkDimensions(const Pixmap2D *p){
	if(surfaceSize==-1){
		surfaceSize= p->getW();
	}
	else if(p->getW()!=surfaceSize || p->getH()!=surfaceSize){
		throw runtime_error("Bad surface texture dimensions");
	}
}

}}//end namespace
