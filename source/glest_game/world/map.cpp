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

#include "map.h"

#include <cassert>

#include "tileset.h"
#include "unit.h"
#include "resource.h"
#include "logger.h"
#include "tech_tree.h"
#include "config.h"
#include "leak_dumper.h"

using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class Cell
// =====================================================

Cell::Cell(){
	//game data
    for(int i=0; i<fieldCount; ++i){
        units[i]= NULL;
    }
	height= 0;
}

// ==================== misc ==================== 

//returns if the cell is free
bool Cell::isFree(Field field) const{		
	return getUnit(field)==NULL || getUnit(field)->isPutrefacting();
}

// =====================================================
// 	class SurfaceCell
// =====================================================

SurfaceCell::SurfaceCell(){
	object= NULL;
	vertex= Vec3f(0.f);
	normal= Vec3f(0.f, 1.f, 0.f);
	surfaceType= -1;
	surfaceTexture= NULL;
}

SurfaceCell::~SurfaceCell(){
	delete object;
}

bool SurfaceCell::isFree() const{
	return object==NULL || object->getWalkable();
}

void SurfaceCell::deleteResource(){
	delete object;
	object= NULL;
}

void SurfaceCell::setExplored(int teamIndex, bool explored){
	this->explored[teamIndex]= explored;
}

void SurfaceCell::setVisible(int teamIndex, bool visible){
    this->visible[teamIndex]= visible;
}

// =====================================================
// 	class Map
// =====================================================

// ===================== PUBLIC ======================== 

const int Map::cellScale= 2;
const int Map::mapScale= 2;

Map::Map(){
	cells= NULL;
	surfaceCells= NULL;
	startLocations= NULL;
}

Map::~Map(){
	Logger::getInstance().add("Cells", true);

	delete [] cells;
	delete [] surfaceCells;
	delete [] startLocations;
}

void Map::load(const string &path, TechTree *techTree, Tileset *tileset){
	
	struct MapFileHeader{
		int32 version;
		int32 maxPlayers;
		int32 width;
		int32 height;
		int32 altFactor;
		int32 waterLevel;
		int8 title[128];
		int8 author[128];
		int8 description[256];
	};

	try{
		FILE *f= fopen(path.c_str(), "rb");
		if(f!=NULL){

			//read header
			MapFileHeader header;
			fread(&header, sizeof(MapFileHeader), 1, f);

			if(next2Power(header.width) != header.width){
				throw runtime_error("Map width is not a power of 2");
			}

			if(next2Power(header.height) != header.height){
				throw runtime_error("Map height is not a power of 2");
			}

			heightFactor= header.altFactor;
			waterLevel= static_cast<float>((header.waterLevel-0.01f)/heightFactor);
			title= header.title;
			maxPlayers= header.maxPlayers;
			surfaceW= header.width;
			surfaceH= header.height;
			w= surfaceW*cellScale;
			h= surfaceH*cellScale;


			//start locations
			startLocations= new Vec2i[maxPlayers];
			for(int i=0; i<maxPlayers; ++i){
				int x, y;
				fread(&x, sizeof(int32), 1, f);
				fread(&y, sizeof(int32), 1, f);
				startLocations[i]= Vec2i(x, y)*cellScale;
			}


			//cells
			cells= new Cell[w*h];
			surfaceCells= new SurfaceCell[surfaceW*surfaceH];
			
			//read heightmap
			for(int j=0; j<surfaceH; ++j){
				for(int i=0; i<surfaceW; ++i){
					float32 alt;
					fread(&alt, sizeof(float32), 1, f);
					SurfaceCell *sc= getSurfaceCell(i, j);
					sc->setVertex(Vec3f(i*mapScale, alt / heightFactor, j*mapScale)); 
				}
			}

			//read surfaces
			for(int j=0; j<surfaceH; ++j){
				for(int i=0; i<surfaceW; ++i){
					int8 surf;
					fread(&surf, sizeof(int8), 1, f);
					getSurfaceCell(i, j)->setSurfaceType(surf-1); 
				}
			}
			
			//read objects and resources
			for(int j=0; j<h; j+= cellScale){
				for(int i=0; i<w; i+= cellScale){
					
					int8 objNumber;
					fread(&objNumber, sizeof(int8), 1, f);
					SurfaceCell *sc= getSurfaceCell(toSurfCoords(Vec2i(i, j)));
					if(objNumber==0){
						sc->setObject(NULL);
					}
					else if(objNumber <= Tileset::objCount){
						Object *o= new Object(tileset->getObjectType(objNumber-1), sc->getVertex());
						sc->setObject(o);
						for(int k=0; k<techTree->getResourceTypeCount(); ++k){
							const ResourceType *rt= techTree->getResourceType(k);
							if(rt->getClass()==rcTileset && rt->getTilesetObject()==objNumber){
								o->setResource(rt, Vec2i(i, j));
							}
						}
					}
					else{
						const ResourceType *rt= techTree->getTechResourceType(objNumber - Tileset::objCount) ;
						Object *o= new Object(NULL, sc->getVertex());
						o->setResource(rt, Vec2i(i, j));
						sc->setObject(o);
					}
				}
			}
		}
		else{
			throw runtime_error("Can't open file");
		}
		fclose(f);
	}
	catch(const exception &e){
		throw runtime_error("Error loading map: "+ path+ "\n"+ e.what());
	}
}

void Map::init(){
	Logger::getInstance().add("Heightmap computations", true);
	smoothSurface();
	computeNormals();
	computeInterpolatedHeights();
	computeNearSubmerged();
	computeCellColors();
}


// ==================== is ==================== 

bool Map::isInside(int x, int y) const{
	return x>=0 && y>=0 && x<w && y<h;
}

bool Map::isInside(const Vec2i &pos) const{
	return isInside(pos.x, pos.y);
}

bool Map::isInsideSurface(int sx, int sy) const{
	return sx>=0 && sy>=0 && sx<surfaceW && sy<surfaceH;
}

bool Map::isInsideSurface(const Vec2i &sPos) const{
	return isInsideSurface(sPos.x, sPos.y);
}

//returns if there is a resource next to a unit, in "resourcePos" is stored the relative position of the resource
bool Map::isResourceNear(const Vec2i &pos, const ResourceType *rt, Vec2i &resourcePos) const{
	for(int i=-1; i<=1; ++i){
		for(int j=-1; j<=1; ++j){
			if(isInside(pos.x+i, pos.y+j)){
				Resource *r= getSurfaceCell(toSurfCoords(Vec2i(pos.x+i, pos.y+j)))->getResource();
				if(r!=NULL){
					if(r->getType()==rt){
						resourcePos= pos + Vec2i(i,j);
						return true;
					}
				}
			}
		}
	}
	return false;
}


// ==================== free cells ==================== 

bool Map::isFreeCell(const Vec2i &pos, Field field) const{
	return 
		isInside(pos) && 
		getCell(pos)->isFree(field) && 
		(field==fAir || getSurfaceCell(toSurfCoords(pos))->isFree()) &&
		(field!=fLand || !getDeepSubmerged(getCell(pos)));
}

bool Map::isFreeCellOrHasUnit(const Vec2i &pos, Field field, const Unit *unit) const{
	if(isInside(pos)){
		Cell *c= getCell(pos);
		if(c->getUnit(unit->getCurrField())==unit){
			return true;
		}
		else{
			return isFreeCell(pos, field);
		}
	}
	return false;
}

bool Map::isAproxFreeCell(const Vec2i &pos, Field field, int teamIndex) const{

	if(isInside(pos)){ 
		const SurfaceCell *sc= getSurfaceCell(toSurfCoords(pos));

		if(sc->isVisible(teamIndex)){
			return isFreeCell(pos, field);
		}
		else if(sc->isExplored(teamIndex)){
			return field==fLand? sc->isFree() && !getDeepSubmerged(getCell(pos)): true;
		}
		else{
			return true;
		}
	}	
	return false;
}

bool Map::isFreeCells(const Vec2i & pos, int size, Field field) const{
	for(int i=pos.x; i<pos.x+size; ++i){
		for(int j=pos.y; j<pos.y+size; ++j){
			if(!isFreeCell(Vec2i(i,j), field)){
                return false;
			}
		}
	}
    return true;
}

bool Map::isFreeCellsOrHasUnit(const Vec2i &pos, int size, Field field, const Unit *unit) const{
	for(int i=pos.x; i<pos.x+size; ++i){
		for(int j=pos.y; j<pos.y+size; ++j){
			if(!isFreeCellOrHasUnit(Vec2i(i,j), field, unit)){
                return false;
			}
		}
	}
    return true;
}

bool Map::isAproxFreeCells(const Vec2i &pos, int size, Field field, int teamIndex) const{
	for(int i=pos.x; i<pos.x+size; ++i){
		for(int j=pos.y; j<pos.y+size; ++j){
			if(!isAproxFreeCell(Vec2i(i, j), field, teamIndex)){
                return false;
			}
		}
	}
    return true;
}


// ==================== unit placement ==================== 

//checks if a unit can move from between 2 cells
bool Map::canMove(const Unit *unit, const Vec2i &pos1, const Vec2i &pos2) const{
	int size= unit->getType()->getSize();

	for(int i=pos2.x; i<pos2.x+size; ++i){
		for(int j=pos2.y; j<pos2.y+size; ++j){
			if(isInside(i, j)){
				if(getCell(i, j)->getUnit(unit->getCurrField())!=unit){
					if(!isFreeCell(Vec2i(i, j), unit->getCurrField())){
						return false;
					}
				}
			}
			else{
				return false;
			}
		}
	}
    return true;
}

//checks if a unit can move from between 2 cells using only visible cells (for pathfinding)
bool Map::aproxCanMove(const Unit *unit, const Vec2i &pos1, const Vec2i &pos2) const{
	int size= unit->getType()->getSize();
	int teamIndex= unit->getTeam();
	Field field= unit->getCurrField();

	//single cell units
	if(size==1){
		if(!isAproxFreeCell(pos2, field, teamIndex)){
			return false;
		}
		if(pos1.x!=pos2.x && pos1.y!=pos2.y){
			if(!isAproxFreeCell(Vec2i(pos1.x, pos2.y), field, teamIndex)){
				return false;
			}
			if(!isAproxFreeCell(Vec2i(pos2.x, pos1.y), field, teamIndex)){
				return false;
			}
		}
		return true;
	}

	//multi cell units
	else{
		for(int i=pos2.x; i<pos2.x+size; ++i){
			for(int j=pos2.y; j<pos2.y+size; ++j){
				if(isInside(i, j)){
					if(getCell(i, j)->getUnit(unit->getCurrField())!=unit){
						if(!isAproxFreeCell(Vec2i(i, j), field, teamIndex)){
							return false;
						}
					}
				}
				else{
					return false;
				}
			}
		}
		return true;
	}
}

//put a units into the cells
void Map::putUnitCells(Unit *unit, const Vec2i &pos){
	
	assert(unit!=NULL);
	const UnitType *ut= unit->getType();

	for(int i=0; i<ut->getSize(); ++i){
		for(int j=0; j<ut->getSize(); ++j){
			Vec2i currPos= pos + Vec2i(i, j);
			assert(isInside(currPos));
			if(!ut->hasCellMap() || ut->getCellMapCell(i, j)){
				assert(getCell(currPos)->getUnit(unit->getCurrField())==NULL);
				getCell(currPos)->setUnit(unit->getCurrField(), unit);
			}

		}     
	}
	unit->setPos(pos);
}

//removes a unit from cells
void Map::clearUnitCells(Unit *unit, const Vec2i &pos){
	
	assert(unit!=NULL);
	const UnitType *ut= unit->getType();

	for(int i=0; i<ut->getSize(); ++i){
		for(int j=0; j<ut->getSize(); ++j){
			Vec2i currPos= pos + Vec2i(i, j);
			assert(isInside(currPos));
			if(!ut->hasCellMap() || ut->getCellMapCell(i, j)){
				assert(getCell(currPos)->getUnit(unit->getCurrField())==unit);
				getCell(currPos)->setUnit(unit->getCurrField(), NULL);
			}
		}     
	}
}

// ==================== misc ==================== 

//returnis if unit is next to pos
bool Map::isNextTo(const Vec2i &pos, const Unit *unit) const{
     
	for(int i=-1; i<=1; ++i){
		for(int j=-1; j<=1; ++j){
			if(isInside(pos.x+i, pos.y+j)) {
				if(getCell(pos.x+i, pos.y+j)->getUnit(fLand)==unit){
					return true;
				}
			}
		}
	}
    return false;
}

void Map::clampPos(Vec2i &pos) const{
	if(pos.x<0){
        pos.x=0;
	}
	if(pos.y<0){
        pos.y=0;
	}
	if(pos.x>=w){
        pos.x=w-1;
	}
	if(pos.y>=h){
        pos.y=h-1;
	}
}

void Map::prepareTerrain(const Unit *unit){
	flatternTerrain(unit);
    computeNormals();
	computeInterpolatedHeights();
}

// ==================== PRIVATE ==================== 

// ==================== compute ==================== 

void Map::flatternTerrain(const Unit *unit){
	float refHeight= getSurfaceCell(toSurfCoords(unit->getCenteredPos()))->getHeight();
	for(int i=-1; i<=unit->getType()->getSize(); ++i){
        for(int j=-1; j<=unit->getType()->getSize(); ++j){
            Vec2i pos= unit->getPos()+Vec2i(i, j); 
			Cell *c= getCell(pos);
			SurfaceCell *sc= getSurfaceCell(toSurfCoords(pos));
            //we change height if pos is inside world, if its free or ocupied by the currenty building
			if(isInside(pos) && sc->getObject()==NULL && (c->getUnit(fLand)==NULL || c->getUnit(fLand)==unit)){
				sc->setHeight(refHeight);
            }
        }
    }
}

//compute normals
void Map::computeNormals(){  
    //compute center normals
    for(int i=1; i<surfaceW-1; ++i){
        for(int j=1; j<surfaceH-1; ++j){
            getSurfaceCell(i, j)->setNormal(
				getSurfaceCell(i, j)->getVertex().normal(getSurfaceCell(i, j-1)->getVertex(), 
					getSurfaceCell(i+1, j)->getVertex(), 
					getSurfaceCell(i, j+1)->getVertex(), 
					getSurfaceCell(i-1, j)->getVertex()));
        }
    }
}

void Map::computeInterpolatedHeights(){

	for(int i=0; i<w; ++i){
		for(int j=0; j<h; ++j){
			getCell(i, j)->setHeight(getSurfaceCell(toSurfCoords(Vec2i(i, j)))->getHeight());
		}
	}

	for(int i=1; i<surfaceW-1; ++i){
		for(int j=1; j<surfaceH-1; ++j){
			for(int k=0; k<cellScale; ++k){
				for(int l=0; l<cellScale; ++l){
					if(k==0 && l==0){
						getCell(i*cellScale, j*cellScale)->setHeight(getSurfaceCell(i, j)->getHeight());
					}
					else if(k!=0 && l==0){
						getCell(i*cellScale+k, j*cellScale)->setHeight((
							getSurfaceCell(i, j)->getHeight()+
							getSurfaceCell(i+1, j)->getHeight())/2.f);
					}
					else if(l!=0 && k==0){
						getCell(i*cellScale, j*cellScale+l)->setHeight((
							getSurfaceCell(i, j)->getHeight()+
							getSurfaceCell(i, j+1)->getHeight())/2.f);
					}
					else{
						getCell(i*cellScale+k, j*cellScale+l)->setHeight((
							getSurfaceCell(i, j)->getHeight()+
							getSurfaceCell(i, j+1)->getHeight()+
							getSurfaceCell(i+1, j)->getHeight()+
							getSurfaceCell(i+1, j+1)->getHeight())/4.f);
					}
				}
			}
		}
	}
}
	

void Map::smoothSurface(){

	float *oldHeights= new float[surfaceW*surfaceH];

	for(int i=0; i<surfaceW*surfaceH; ++i){
		oldHeights[i]= surfaceCells[i].getHeight();
	}

	for(int i=1; i<surfaceW-1; ++i){
		for(int j=1; j<surfaceH-1; ++j){

			float height= 0.f;
			for(int k=-1; k<=1; ++k){
				for(int l=-1; l<=1; ++l){
					height+= oldHeights[(j+k)*surfaceW+(i+l)];
				}
			}
			height/= 9.f;

			getSurfaceCell(i, j)->setHeight(height);
			Object *object= getSurfaceCell(i, j)->getObject();
			if(object!=NULL){
				object->setHeight(height);
			}
		}
	}

	delete [] oldHeights;
}

void Map::computeNearSubmerged(){

	for(int i=0; i<surfaceW-1; ++i){
		for(int j=0; j<surfaceH-1; ++j){
			bool anySubmerged= false;
			for(int k=-1; k<=2; ++k){
				for(int l=-1; l<=2; ++l){
					Vec2i pos= Vec2i(i+k, j+l);
					if(isInsideSurface(pos)){
						if(getSubmerged(getSurfaceCell(pos)))
							anySubmerged= true;
					}
				}
			}
			getSurfaceCell(i, j)->setNearSubmerged(anySubmerged);
		}
	}
}

void Map::computeCellColors(){
	for(int i=0; i<surfaceW; ++i){
		for(int j=0; j<surfaceH; ++j){
			SurfaceCell *sc= getSurfaceCell(i, j);
			if(getDeepSubmerged(sc)){
				float factor= clamp(waterLevel-sc->getHeight()*1.5f, 1.f, 1.5f);
				sc->setColor(Vec3f(1.0f, 1.0f, 1.0f)/factor);
			}
			else{
				sc->setColor(Vec3f(1.0f, 1.0f, 1.0f));
			}
		}
	}
}

// =====================================================
// 	class PosCircularIterator
// =====================================================

PosCircularIterator::PosCircularIterator(const Map *map, const Vec2i &center, int radius){
	this->map= map;
	this->radius= radius;
	this->center= center;
	pos= center - Vec2i(radius, radius);
	pos.x-= 1;
}

bool PosCircularIterator::next(){
	
	//iterate while dont find a cell that is inside the world
	//and at less or equal distance that the radius 
	do{
		pos.x++;
		if(pos.x > center.x+radius){
			pos.x= center.x-radius;
			pos.y++;
		}
		if(pos.y>center.y+radius)
			return false;
	}
	while(floor(pos.dist(center)) >= (radius+1) || !map->isInside(pos));
	//while(!(pos.dist(center) <= radius && map->isInside(pos)));
	
	return true;
}

const Vec2i &PosCircularIterator::getPos(){
	return pos;
}


// =====================================================
// 	class PosQuadIterator
// =====================================================

PosQuadIterator::PosQuadIterator(const Map *map, const Quad2i &quad, int step){
	this->map= map;
	this->quad= quad;
	this->boundingRect= quad.computeBoundingRect();
	this->step= step;
	pos= boundingRect.p[0];
	--pos.x;
	pos.x= (pos.x/step)*step;
	pos.y= (pos.y/step)*step;
}

bool PosQuadIterator::next(){
	
	do{
		pos.x+= step;
		if(pos.x > boundingRect.p[1].x){
			pos.x= (boundingRect.p[0].x/step)*step; 
			pos.y+= step;
		}
		if(pos.y>boundingRect.p[1].y)
			return false;
	}
	while(!quad.isInside(pos));
	
	return true;
}

void PosQuadIterator::skipX(){
	pos.x+= step;
} 

const Vec2i &PosQuadIterator::getPos(){
	return pos;
}
      
}}//end namespace
