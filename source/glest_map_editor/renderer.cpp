#include "renderer.h"
#include "rendertile.h"

Renderer::Renderer(){
	this->map = new MapPreview();
	//this->map->loadFromFile("/home/dabascht/projects/megaglest/git/works.gbm");
	this->scene = new QGraphicsScene();
	//this->width = 64;
	//this->height = 64;
	this->width = this->map->getW();
	this->height = this->map->getH();
	this->createTiles();

	this->recalculateAll();
}

Renderer::~Renderer(){
		cout << "~Renderer()" << endl;
		this->removeTiles();
		delete this->map;
	}

void Renderer::open(string path){
	this->map->loadFromFile(path);
	if(this->width == this->map->getW() && this->height == this->map->getH()){
		this->updateMap();
		this->recalculateAll();
	}else{
		this->removeTiles();
		this->width = this->map->getW();
		this->height = this->map->getH();
		this->createTiles();
		this->recalculateAll();
	}
}

void Renderer::createTiles(){
	cout << "createTiles()" << endl;
	cout << "height: " << height << "; width: " << width << endl;
	this->tiles = new RenderTile**[this->width];
	for(int column = 0; column < this->width; column++){
		this->tiles[column] = new RenderTile*[this->height];
		for(int row = 0; row < this->height; row++){
			this->tiles[column][row] = new RenderTile(this->scene, this, column, row);
		}
	}
	cout << "survived creation!" << endl;
}

void Renderer::removeTiles(){
	cout << "removeTiles()" << endl;
	for(int column = 0; column < this->width; column++){
		for(int row = 0; row < this->height; row++){
			delete this->tiles[column][row];
		}
		delete[] this->tiles[column];
	}
	delete[] this->tiles;
}

void Renderer::recalculateAll(){
	for(int column = 0; column < this->width; column++){
		for(int row = 0; row < this->height; row++){
			this->tiles[column][row]->recalculate();
		}
	}
}

void Renderer::updateMap(){
	for(int column = 0; column < this->width; column++){
		for(int row = 0; row < this->height; row++){
			this->tiles[column][row]->update();
		}
	}
}


int Renderer::getHeight() const{
	return this->height;
}

int Renderer::getWidth() const{
	return this->width;
}

MapPreview *Renderer::getMap() const{
	return this->map;
}


RenderTile *Renderer::at(int column, int row) const{
	return this->tiles[column][row];
}

QGraphicsScene *Renderer::getScene() const{
	return this->scene;
}
