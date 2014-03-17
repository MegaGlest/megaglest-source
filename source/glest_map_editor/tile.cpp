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

#include "tile.h"
#include "renderer.h"
#include "mapManipulator.h"
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
#include <QGraphicsItemGroup>
#include <QGraphicsSceneMouseEvent>
#include <iostream>

namespace MapEditor {
    //surface 0 is a cliff, megaglests surfaces start with 1
    const QColor Tile::SURFACE[] ={QColor(0xF2,0xCC,0x00),QColor(0x00,0xCC,0x00),QColor(0x66,0x99,0x0B),QColor(0x99,0x4C,0x00),QColor(0xB2,0xB2,0xB2),QColor(0xB2,0x7F,0x4C)};
    //const QColor Tile::OBJECT[] ={QColor(0xF2,0xCC,0x00),QColor(0x00,0xCC,0x00),QColor(0x66,0x99,0x0B),QColor(0x99,0x4C,0x00),QColor(0xB2,0xB2,0xB2),QColor(0xB2,0x7F,0x4C)};
    const int Tile::SIZE = 6;

    //column and row are the position in Tiles of the Tile
    Tile::Tile(QGraphicsScene *scene, Renderer *renderer, int column, int row):QGraphicsItemGroup(){//call the standard constructor of QGraphicsItemGroup
        scene->addItem(this);//add this group to the scene


        this->renderer=renderer;
        children = 7;
        child = new QGraphicsItem*[children];
        child[0] = this->rect = new QGraphicsRectItem(0, 0, SIZE, SIZE, this);
        child[1] = this->water = new QGraphicsRectItem(0, 0, SIZE, SIZE, this);
        float objectSpace = (SIZE*2)/5.0;
        child[2] = this->object = new QGraphicsRectItem(objectSpace, objectSpace, SIZE-(2*objectSpace), SIZE-(2*objectSpace), this);
        //borders are extra items for pseudo 3D effect, we rarely show all of them, they have the rectangle as parent
        child[3] = this->leftLine = new QGraphicsLineItem(0,0,0,SIZE-1,this);
        child[4] = this->topLine = new QGraphicsLineItem(0,0,SIZE-1,0,this);
        child[5] = this->rightLine = new QGraphicsLineItem(SIZE-1,0,SIZE-1,SIZE-1,this);
        child[6] = this->bottomLine = new QGraphicsLineItem(0,SIZE-1,SIZE-1,SIZE-1,this);

        this->height = this->renderer->getMap()->getHeight(column, row);

        for(int i = 0; i < children; i++){
            //child[i]->setAcceptHoverEvents(true);//for hover events
            //child[i]->setAcceptDrops(true);//for drag and drop -> hover while mouse pressed
        }

        this->rect->setPen(QPen(Qt::NoPen));//no borders
        this->water->setPen(QPen(Qt::NoPen));
        this->object->setPen(QPen(Qt::NoPen));
        this->water->setOpacity(0.4);

        QPen white(QColor(0xFF, 0xFF, 0xFF, 0x42 ));
        this->leftLine->setPen(white);//white and 50% opacity
        this->topLine->setPen(white);

        QPen black(QColor(0x00, 0x00, 0x00, 0x42 ));
        this->rightLine->setPen(black);//black and 50% opacity
        this->bottomLine->setPen(black);

        this->move(column, row);

    }

    Tile::~Tile(){
        this->rect->scene()->removeItem(this->rect);
        delete this->water;
        delete this->object;
        delete this->leftLine;
        delete this->topLine;
        delete this->rightLine;
        delete this->bottomLine;
        delete this->rect;

        delete child;
    }

    void Tile::move(int column, int row){
        QGraphicsItemGroup::setPos(column*SIZE,row*SIZE);
        this->row = row;
        this->column = column;
    }

    void Tile::update(){
        this->height = this->renderer->getMap()->getHeight(column, row);
    }

    void Tile::setHeight(double height){
        bool changed = (this->height != height);
        this->height = height;
        if(changed){
            this->renderer->getMap()->setHeight(this->column, this->row, height);
            this->recalculate();
            if(this->column > 0){
                this->renderer->at(this->column - 1, this->row)->recalculate();
            }
            if(this->row > 0){
                this->renderer->at(this->column, this->row - 1)->recalculate();
            }
            if(this->column < this->renderer->getWidth() - 1){
                this->renderer->at(this->column + 1, this->row)->recalculate();
            }
            if(this->row < this->renderer->getHeight() - 1){
                this->renderer->at(this->column, this->row + 1)->recalculate();
            }
        }
    }

    void Tile::setSurface(int surface){
        this->renderer->getMap()->setSurface(this->column, this->row, Shared::Map::st_Road);
        this->recalculate();
    }

    double Tile::getHeight() const{
        return this->height;
    }

    void Tile::recalculate(){
        QColor base = SURFACE[this->renderer->getMap()->getSurface(column, row)];
        bool showObject = false;
        bool showWater = false;
        if(this->renderer->getMap()->isCliff(column, row)){
            base = SURFACE[0];
            showObject = true;
            this->object->setBrush(Qt::black);
        }
        //TODO: draw objects ... getObject
        double waterlevel = this->renderer->getMap()->getWaterLevel();
        if(this->height <= waterlevel){
            //showWater = true;
            if(this->height <= waterlevel - 1.5){
                //this->water->setBrush(QBrush(Qt::blue));
                base = Qt::blue;
            }else{
                //this->water->setBrush(QBrush(Qt::cyan));
                base = Qt::cyan;
            }
        }

        this->object->setVisible(showObject);
        this->water->setVisible(showWater);
        this->rect->setBrush(base.darker(100+10*(20 - this->height)));

        //get the heights of all surrounding Tiles
        if(this->column > 0){
            double leftHeight = this->renderer->at(this->column - 1, this->row)->getHeight();
            this->leftLine->setVisible(this->height > leftHeight);
        }else{
            this->leftLine->setVisible(false);
        }

        if(this->row > 0){
            double topHeight = this->renderer->at(this->column, this->row - 1)->getHeight();
            this->topLine->setVisible(this->height > topHeight);
        }else{
            this->topLine->setVisible(false);
        }

        if(this->column < this->renderer->getWidth() - 1){
            double rightHeight = this->renderer->at(this->column + 1, this->row)->getHeight();
            this->rightLine->setVisible(this->height > rightHeight);
        }else{
            this->rightLine->setVisible(false);
        }

        if(this->row < this->renderer->getHeight() - 1){
            double bottomHeight = this->renderer->at(this->column, this->row + 1)->getHeight();
            this->bottomLine->setVisible(this->height > bottomHeight);
        }else{
            this->bottomLine->setVisible(false);
        }

    }


    void Tile::mousePressEvent ( QGraphicsSceneMouseEvent *event){
        cout << "mouse pressed @" << this->column << "," << this->row << endl;
        //this->setSurface(0);
        //this->renderer->getMap()->glestChangeHeight(column, row, 15, 4);
        /*this->renderer->getMap()->changeSurface(column, row, Shared::Map::st_Road, this->renderer->getRadius());
        this->renderer->updateMap();
        this->renderer->recalculateAll();*/
        //event->ignore();
        this->renderer->getMapManipulator()->changeTile(this->column, this->row);
    }

    void Tile::mouseMoveEvent ( QGraphicsSceneMouseEvent *event){
        QPointF point = event->scenePos();
        int column = point.x() / SIZE;
        int row = point.y() / SIZE;
        /*if(column >= 0 && row >= 0 && row < this->renderer->getHeight() && column < this->renderer->getWidth()){
            this->renderer->at(column,row)->setSurface(0);
        }
        //this->renderer->getMap()->glestChangeHeight(column, row, 15, 4);
        this->renderer->getMap()->changeSurface(column, row, Shared::Map::st_Road, this->renderer->getRadius());
        this->renderer->updateMap();
        this->renderer->recalculateAll();*/
        //std::cout << "mouse moved @" << column << "," << row << std::endl;
        this->renderer->getMapManipulator()->changeTile(column, row);
    }

    void Tile::mouseReleaseEvent ( QGraphicsSceneMouseEvent *event){
        QPointF point = event->scenePos();
        int column = point.x() / SIZE;
        int row = point.y() / SIZE;
        std::cout << "mouse released @" << column << "," << row << std::endl;
        //this->renderer->getMap()->setHasChanged(false);
    }

    void Tile::hoverEnterEvent ( QGraphicsSceneHoverEvent *event ){
        //std::cout << "hovered @" << column << "," << row << std::endl;
    }

    void Tile::dragEnterEvent (QGraphicsSceneDragDropEvent *event){
        //std::cout << "drag hovered @" << column << "," << row << std::endl;
    }
}
