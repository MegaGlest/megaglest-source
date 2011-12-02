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

#include "minimap.h"

#include <cassert>

#include "world.h"
#include "vec.h"
#include "renderer.h"
#include "config.h"
#include "object.h"
#include "game_settings.h"
#include "leak_dumper.h"

using namespace Shared::Graphics;

namespace Glest{ namespace Game{

// =====================================================
// 	class Minimap
// =====================================================

const float Minimap::exploredAlpha= 0.5f;

Minimap::Minimap() {
	fowPixmap0= NULL;
	fowPixmap1= NULL;
	fogOfWar= true;//Config::getInstance().getBool("FogOfWar");
	gameSettings= NULL;
	tex=NULL;
	fowTex=NULL;
}

void Minimap::init(int w, int h, const World *world, bool fogOfWar) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	int scaledW= w/Map::cellScale;
	int scaledH= h/Map::cellScale;
	int potW = next2Power(scaledW);
	int potH = next2Power(scaledH);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] scaledW = %d, scaledH = %d, potW = %d, potH = %d\n",__FILE__,__FUNCTION__,__LINE__,scaledW,scaledH,potW,potH);

	this->fogOfWar = fogOfWar;
	this->gameSettings = world->getGameSettings();
	Renderer &renderer= Renderer::getInstance();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	//fow pixmaps
	float f= 0.f;

	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false) {
		fowPixmap0= new Pixmap2D(potW, potH, 1);
		fowPixmap1= new Pixmap2D(potW, potH, 1);

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		fowPixmap0->setPixels(&f);
		if((this->gameSettings->getFlagTypes1() & ft1_show_map_resources) == ft1_show_map_resources) {
			f = 0.f;
			fowPixmap1->setPixels(&f);
			f = 0.5f;
			for (int y=1; y < scaledH - 1; ++y) {
				for (int x=1; x < scaledW - 1; ++x) {
					fowPixmap1->setPixel(x, y, &f);
				}
			}
		}
		else {
			fowPixmap1->setPixels(&f);
		}
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//fow tex
	fowTex= renderer.newTexture2D(rsGame);
	if(fowTex) {
		fowTex->setMipmap(false);
		fowTex->setPixmapInit(false);
		fowTex->setFormat(Texture::fAlpha);

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] scaledW = %d, scaledH = %d, potW = %d, potH = %d\n",__FILE__,__FUNCTION__,__LINE__,scaledW,scaledH,potW,potH);

		fowTex->getPixmap()->init(potW, potH, 1);
		fowTex->getPixmap()->setPixels(&f);
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//tex
	tex= renderer.newTexture2D(rsGame);
	if(tex) {
		tex->getPixmap()->init(scaledW, scaledH, 3);
		tex->setMipmap(false);
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	computeTexture(world);
}

Minimap::~Minimap() {
	Logger::getInstance().add(Lang::getInstance().get("LogScreenGameUnLoadingMiniMap","",true), true);
	delete fowPixmap0;
	fowPixmap0=NULL;
	delete fowPixmap1;
	fowPixmap1=NULL;
}

// ==================== set ====================

void Minimap::incFowTextureAlphaSurface(const Vec2i &sPos, float alpha) {
	if(fowPixmap1) {
		assert(sPos.x<fowPixmap1->getW() && sPos.y<fowPixmap1->getH());

		if(fowPixmap1->getPixelf(sPos.x, sPos.y)<alpha){
			fowPixmap1->setPixel(sPos.x, sPos.y, alpha);
		}
	}
}

void Minimap::resetFowTex() {
	if(fowTex) {
		Pixmap2D *tmpPixmap= fowPixmap0;
		fowPixmap0= fowPixmap1;
		fowPixmap1= tmpPixmap;

		// Could turn off ONLY fog of war by setting below to false
		bool overridefogOfWarValue = fogOfWar;

		for(int i=0; i<fowTex->getPixmap()->getW(); ++i){
			for(int j=0; j<fowTex->getPixmap()->getH(); ++j){
				if ((fogOfWar == false && overridefogOfWarValue == false) &&
					(gameSettings->getFlagTypes1() & ft1_show_map_resources) != ft1_show_map_resources) {
					float p0 = fowPixmap0->getPixelf(i, j);
					float p1 = fowPixmap1->getPixelf(i, j);
					if (p0 > p1) {
						fowPixmap1->setPixel(i, j, p0);
					}
					else {
						fowPixmap1->setPixel(i, j, p1);
					}
				}
				else if((fogOfWar && overridefogOfWarValue) ||
					(gameSettings->getFlagTypes1() & ft1_show_map_resources) != ft1_show_map_resources) {
					float p0= fowPixmap0->getPixelf(i, j);
					float p1= fowPixmap1->getPixelf(i, j);

					if(p1>exploredAlpha){
						fowPixmap1->setPixel(i, j, exploredAlpha);
					}
					if(p0>p1){
						fowPixmap1->setPixel(i, j, p0);
					}
				}
				else{
					fowPixmap1->setPixel(i, j, 1.f);
				}
			}
		}
	}
}

void Minimap::updateFowTex(float t) {
	if(fowPixmap0 && fowTex) {
		for(int i=0; i<fowPixmap0->getW(); ++i){
			for(int j=0; j<fowPixmap0->getH(); ++j){
				float p1= fowPixmap1->getPixelf(i, j);
				if(p1!=fowTex->getPixmap()->getPixelf(i, j)){
					float p0= fowPixmap0->getPixelf(i, j);
					fowTex->getPixmap()->setPixel(i, j, p0+(t*(p1-p0)));
				}
			}
		}
	}
}

// ==================== PRIVATE ====================

void Minimap::computeTexture(const World *world) {

	Vec3f color;
	const Map *map= world->getMap();

	if(tex) {
		tex->getPixmap()->setPixels(Vec4f(1.f, 1.f, 1.f, 0.1f).ptr());

		for(int j=0; j<tex->getPixmap()->getH(); ++j){
			for(int i=0; i<tex->getPixmap()->getW(); ++i){
				SurfaceCell *sc= map->getSurfaceCell(i, j);

				if(sc->getObject()==NULL || sc->getObject()->getType()==NULL){
					const Pixmap2D *p= world->getTileset()->getSurfPixmap(sc->getSurfaceType(), 0);
					color= p->getPixel3f(p->getW()/2, p->getH()/2);
					color= color * static_cast<float>(sc->getVertex().y/6.f);

					if(sc->getVertex().y<= world->getMap()->getWaterLevel()){
						color+= Vec3f(0.5f, 0.5f, 1.0f);
					}

					if(color.x>1.f) color.x=1.f;
					if(color.y>1.f) color.y=1.f;
					if(color.z>1.f) color.z=1.f;
				}
				else{
					color= sc->getObject()->getType()->getColor();
				}
				tex->getPixmap()->setPixel(i, j, color);
			}
		}
	}
}

}}//end namespace
