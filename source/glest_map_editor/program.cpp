#include "program.h"

#include "util.h" 

using namespace Shared::Util;

namespace Glest{ namespace MapEditor{

// ===============================================
//	class Program
// ===============================================

Program::Program(int w, int h){
     cellSize=6;
     ofsetX= 0;
     ofsetY= 0;
     map= new Map();
     renderer.init(w, h);
}

Program::~Program(){
     delete map;
}

void Program::changeMapHeight(int x, int y, int Height, int radius){
     map->changeHeight((x-ofsetX)/cellSize, (y+ofsetY)/cellSize, Height, radius);
}

void Program::changeMapSurface(int x, int y, int surface, int radius){
     map->changeSurface((x-ofsetX)/cellSize, (y+ofsetY)/cellSize, surface, radius);
}

void Program::changeMapObject(int x, int y, int object, int radius){
     map->changeObject((x-ofsetX)/cellSize, (y+ofsetY)/cellSize, object, radius);
}

void Program::changeMapResource(int x, int y, int resource, int radius){
     map->changeResource((x-ofsetX)/cellSize, (y+ofsetY)/cellSize, resource, radius);
}

void Program::changeStartLocation(int x, int y, int player){    
     map->changeStartLocation((x-ofsetX)/cellSize, (y+ofsetY)/cellSize, player);
}

void Program::renderMap(int w, int h){
     renderer.renderMap(map, ofsetX, ofsetY, w, h, cellSize);
}

void Program::setRefAlt(int x, int y){
	map->setRefAlt((x-ofsetX)/cellSize, (y+ofsetY)/cellSize);
}

void Program::flipX(){
	map->flipX();
}

void Program::flipY(){
	map->flipY();
}

void Program::randomizeMapHeights(){
	map->randomizeHeights();
}

void Program::randomizeMap(){
	map->randomize();
}

void Program::switchMapSurfaces(int surf1, int surf2){
	map->switchSurfaces(surf1, surf2);
}

void Program::reset(int w, int h, int alt, int surf){
     map->reset(w, h, (float) alt, surf);
}

void Program::resize(int w, int h, int alt, int surf){
     map->resize(w, h, (float) alt, surf);
}

void Program::resetPlayers(int maxPlayers){
     map->resetPlayers(maxPlayers);
}

void Program::setMapTitle(const string &title){
     map->setTitle(title);
}
 
void Program::setMapDesc(const string &desc){
     map->setDesc(desc);
}

void Program::setMapAuthor(const string &author){
     map->setAuthor(author);
}

void Program::setOfset(int x, int y){
     ofsetX+= x;
     ofsetY-= y;
}

void Program::incCellSize(int i){
    
	int minInc= 2-cellSize;

	if(i<minInc){
		i= minInc;
	}
	cellSize+= i;
	ofsetX-= (map->getW()*i)/2;
	ofsetY+= (map->getH()*i)/2;
}

void Program::resetOfset(){
     ofsetX= 0;
     ofsetY= 0;
     cellSize= 6;
}

void Program::setMapAdvanced(int altFactor, int waterLevel){
     map->setAdvanced(altFactor, waterLevel);
}

void Program::loadMap(const string &path){
     map->loadFromFile(path);
}

void Program::saveMap(const string &path){
     map->saveToFile(path);
}

}}// end namespace
