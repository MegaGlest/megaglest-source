// ==============================================================
//  This file is part of MegaGlest (www.megaglest.org)
//
//  Copyright (C) 2014 Sebastian Riedel
//
//  You can redistribute this code and/or modify it under
//  the terms of the GNU General Public License as published
//  by the Free Software Foundation; either version 2 of the
//  License, or (at your option) any later version
// ==============================================================

#include "renderer.h"
#include "rendertile.h"

#include <QGraphicsScene>
#include <QAction>
//#include "map_preview.h"

#include <iostream>


Renderer::Renderer(){
    this->radius = 1;
    this->map = new Shared::Map::MapPreview();
    this->scene = new QGraphicsScene();
    this->historyPos = 0;
    this->width = this->map->getW();
    this->height = this->map->getH();
    this->createTiles();

    this->recalculateAll();
}

Renderer::~Renderer(){
        this->removeTiles();
        delete this->map;
    }

void Renderer::open(string path){
    this->addHistory();
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
    this->tiles = new RenderTile**[this->width];
    for(int column = 0; column < this->width; column++){
        this->tiles[column] = new RenderTile*[this->height];
        for(int row = 0; row < this->height; row++){
            this->tiles[column][row] = new RenderTile(this->scene, this, column, row);
        }
    }
}

void Renderer::removeTiles(){
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

Shared::Map::MapPreview *Renderer::getMap() const{
    return this->map;
}


RenderTile *Renderer::at(int column, int row) const{
    return this->tiles[column][row];
}

QGraphicsScene *Renderer::getScene() const{
    return this->scene;
}

void Renderer::addHistory(){
    this->history.push_back(this->map);
}

void Renderer::setRadius(QAction *radius){
    this->radius = QChar::digitValue(radius->objectName().at(6).unicode());//action1_diameter
    std::cout << this->radius << std::endl;
}

int Renderer::getRadius() const{
    return this->radius;
}
