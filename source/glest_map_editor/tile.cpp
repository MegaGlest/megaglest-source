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
#include <QPainter>
//#include <QColor>
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
#include <QGraphicsItemGroup>
#include <QGraphicsSceneMouseEvent>
#include <iostream>

namespace MapEditor {
    //surface 0 is a cliff, megaglests surfaces start with 1
    const QColor Tile::SURFACE[] = {QColor(0xF2,0xCC,0x00),QColor(0x00,0xCC,0x00),QColor(0x66,0x99,0x0B),QColor(0x99,0x4C,0x00),QColor(0xB2,0xB2,0xB2),QColor(0xB2,0x7F,0x4C)};
    const QColor Tile::OBJECT[] = {QColor(0xFF,0x00,0x00),QColor(0xFF,0xFF,0xFF),QColor(0x7F,0x7F,0xFF),QColor(0x00,0x00,0xFF),QColor(0x7F,0x7F,0x7F),QColor(0xFF,0xCC,0x7F),QColor(0x00,0xFF,0xFF),QColor(0xB2,0x19,0x4C),QColor(0x7F,0xFF,0x19),QColor(0xFF,0x33,0xCC)};
    const QColor Tile::RESOURCE[] = {QColor(0xDD,0xDD,0x00),QColor(0x80,0x80,0x80),QColor(0xFF,0x00,0x00),QColor(0x00,0x00,0xFF),QColor(0x00,0x80,0x80)};
    const int Tile::SIZE = 8;

    //column and row are the position in Tiles of the Tile
    Tile::Tile(QGraphicsScene *scene, Renderer *renderer, int column, int row):QGraphicsItem(){//call the standard constructor of QGraphicsItemGroup
        scene->addItem(this);//add this to the scene

        this->renderer=renderer;//TODO: do I need this?
        this->leftLine = this->topLine = this->rightLine = this->bottomLine = true;
        this->water = this->object = this->resource = false;

        this->color = this->objectColor = this->resourceColor = Qt::black;

        this->height = this->renderer->getMap()->getHeight(column, row);//the fuck ist width? - xDD

        this->setAcceptHoverEvents(true);

        this->move(column, row);

    }

    /*Tile::~Tile(){
    }*/

    void Tile::setSurface(int surface){//TODO:checking if still needed
        this->renderer->getMap()->setSurface(this->column, this->row, Shared::Map::st_Road);
        this->recalculate();
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

    double Tile::getHeight() const{
        return this->height;
    }

    void Tile::updateHeight(){
        this->height = this->renderer->getMap()->getHeight(column, row);
        //std::cout << "update!" << std::endl;
    }

    void Tile::recalculate(){
        //this->height = this->renderer->getMap()->getHeight(column, row);
        QColor base = SURFACE[this->renderer->getMap()->getSurface(column, row)];
        this->object = false;
        this->water = false;
        this->resource = false;
        if(this->renderer->getMap()->isCliff(column, row)){
            base = SURFACE[0];
            this->object = true;
            this->objectColor = Qt::black;
        }

        double waterlevel = this->renderer->getMap()->getWaterLevel();
        if(this->height <= waterlevel){
            this->water = true;
            if(this->height <= waterlevel - 1.5){
                base = Qt::blue;
            }else{
                base = Qt::cyan;
            }
        }
        this->color = base.darker(100+10*(20 - this->height));//base;
        int resource = this->renderer->getMap()->getResource(column, row);
        if(resource != 0){
            this->resource = true;
            this->resourceColor = RESOURCE[resource-1];
        }
        int object = this->renderer->getMap()->getObject(column, row);
        if(object != 0){
            this->object = true;
            this->objectColor = OBJECT[object-1];
        }

        //get the heights of all surrounding Tiles
        if(this->column > 0){
            double leftHeight = this->renderer->at(this->column - 1, this->row)->getHeight();
            this->leftLine = this->height > leftHeight;
        }else{
            this->leftLine = false;
        }

        if(this->row > 0){
            double topHeight = this->renderer->at(this->column, this->row - 1)->getHeight();
            this->topLine = this->height > topHeight;
        }else{
            this->topLine = false;
        }

        if(this->column < this->renderer->getWidth() - 1){
            double rightHeight = this->renderer->at(this->column + 1, this->row)->getHeight();
            this->rightLine = this->height > rightHeight;
        }else{
            this->rightLine = false;
        }

        if(this->row < this->renderer->getHeight() - 1){
            double bottomHeight = this->renderer->at(this->column, this->row + 1)->getHeight();
            this->bottomLine = this->height > bottomHeight;
        }else{
            this->bottomLine = false;
        }
        this->update();
    }

    QRectF Tile::boundingRect() const{
        return QRectF(0,0,this->SIZE,this->SIZE);
    }

    void Tile::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget){
        painter->fillRect(QRectF(0,0,this->SIZE,this->SIZE),QBrush(this->color));

        if(!this->water){
            painter->setPen(QColor(0xFF, 0xFF, 0xFF, 0x42 ));//white
            if(this->leftLine)
                painter->drawLine(0,0,0,SIZE);//left
            if(this->topLine)
                painter->drawLine(0,0,SIZE,0);//top
            painter->setPen(QColor(0x00, 0x00, 0x00, 0x42 ));//black
            if(this->rightLine)
                painter->drawLine(SIZE-1,0,SIZE-1,SIZE-1);//right
            if(this->bottomLine)
                painter->drawLine(0,SIZE-1,SIZE-1,SIZE-1);//bottom
        }
        if(this->resource){
            painter->setPen(this->resourceColor);
            painter->drawLine(1,1,SIZE-2,SIZE-2);
            painter->drawLine(1,SIZE-1,SIZE-2,2);
        }
        if(this->object){
            float objectSpace = (SIZE*2)/3.0;
            painter->fillRect(QRectF(objectSpace,objectSpace,SIZE-(2*objectSpace),SIZE-(2*objectSpace)),QBrush(this->objectColor));
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
        this->renderer->getMapManipulator()->click(this->column, this->row);
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
        //this->renderer->getMapManipulator()->changeTile(column, row);
    }

    void Tile::dragEnterEvent (QGraphicsSceneDragDropEvent *event){
        //std::cout << "drag hovered @" << column << "," << row << std::endl;
    }

    void Tile::move(int column, int row){
        QGraphicsItem::setPos(column*SIZE,row*SIZE);
        this->row = row;
        this->column = column;
    }
}
