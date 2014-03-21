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
#include "mapManipulator.hpp"
#include "mainWindow.hpp"
#include "renderer.hpp"
#include "map_preview.h"
#include <QAction>
#include <QString>
#include <iostream>


namespace MapEditor{
    MapManipulator::MapManipulator(MainWindow *win){
        this->win = win;
        this->radius = 1;
    }

    MapManipulator::~MapManipulator(){
        delete this->win;
        delete this->renderer;
    }

    void MapManipulator::setRenderer( Renderer *renderer){
        this->renderer = renderer;
    }

    MainWindow *MapManipulator::getWindow(){
        return this->win;
    }

    void MapManipulator::setRadius(QAction *radius){
        this->radius = radius->objectName().right(1).toInt();
    }

    void MapManipulator::changeTile(int column, int row){
        QString penName = this->win->getPen()->objectName();
        if(!penName.compare("actionGrass")){
            this->renderer->getMap()->changeSurface(column, row, Shared::Map::st_Grass, this->radius);
        }else if(!penName.compare("actionSecondary_Grass")){
            this->renderer->getMap()->changeSurface(column, row, Shared::Map::st_Secondary_Grass, this->radius);
        }else if(!penName.compare("actionRoad")){
            this->renderer->getMap()->changeSurface(column, row, Shared::Map::st_Road, this->radius);
        }else if(!penName.compare("actionStone")){
            this->renderer->getMap()->changeSurface(column, row, Shared::Map::st_Stone, this->radius);
        }else if(!penName.compare("actionGround")){
            this->renderer->getMap()->changeSurface(column, row, Shared::Map::st_Ground, this->radius);
        }else if(!penName.compare("actionGold_unwalkable")){
            this->renderer->getMap()->changeResource(column, row, Shared::Map::st_Ground, this->radius);
        }else if(!penName.compare("actionGold_unwalkable")){
            this->renderer->getMap()->changeResource(column, row, Shared::Map::st_Ground, this->radius);
        }else if(!penName.compare("actionGold_unwalkable")){
            this->renderer->getMap()->changeResource(column, row, Shared::Map::st_Ground, this->radius);
        }else if(penName.contains("actionHeight")){
            int height = penName.right(1).toInt();
            if(penName.contains("actionHeight_")){//_ means -
                height = 0-height;
            }
            this->renderer->getMap()->glestChangeHeight(column, row, height, this->radius);
            this->renderer->updateMap();
        }else if(penName.contains("actionGradient")){
            int height = penName.right(1).toInt();
            if(penName.contains("actionGradient_")){//_ means -
                height = 0-height;
            }
            this->renderer->getMap()->pirateChangeHeight(column, row, height, this->radius);
            this->renderer->updateMap();
        }else if(penName.contains("actionResource")){
            int resource = penName.right(1).toInt();
            this->renderer->getMap()->changeResource(column, row, resource, this->radius);
        }else if(penName.contains("actionObject")){
            int object = penName.right(1).toInt(0,16);//hexadecimal A=10 etc.
            this->renderer->getMap()->changeObject(column, row, object, this->radius);
        }else if(penName.contains("actionPlayer")){
            int player = penName.right(1).toInt()-1;
            //std::cout << "move player" << player << std::endl;
            this->renderer->getMap()->changeStartLocation(column, row, player);
            this->renderer->updatePlayerPositions();
        }//else nothing to do

        //this->renderer->recalculateAll();
    }

    void MapManipulator::click(int column, int row){
        this->renderer->getMap()->setRefAlt(column, row);
        this->changeTile(column, row);
    }

    void MapManipulator::reset(int width, int height, int surface, float altitude, int players){
        //dostuff
        Shared::Map::MapSurfaceType surf = Shared::Map::st_Grass;
        switch(surface){
            case 1:
                surf = Shared::Map::st_Secondary_Grass;
                break;
            case 2:
                surf = Shared::Map::st_Road;
                break;
            case 3:
                surf = Shared::Map::st_Stone;
                break;
            case 4:
                surf = Shared::Map::st_Ground;
                break;
        }
        this->renderer->getMap()->reset(width, height, altitude, surf);
        //this->renderer->getMap()->resize(int w, int h, float alt, MapSurfaceType surf);
        this->renderer->getMap()->resetFactions(players);
        this->renderer->updatePlayerPositions();
        this->renderer->updateMaxPlayers();
        this->renderer->resize();//changes width and height; updates and recalculates map
        this->renderer->clearHistory();
    }
}
