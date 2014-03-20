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

#include "tile.hpp"
#include "renderer.hpp"
//good idea?
#include "mainWindow.hpp"
#include "mapManipulator.hpp"
#include <QPainter>
//#include <QColor>
#include <QString>
#include <QObject>
#include <QLabel>
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
    const QString Tile::OBJECTSTR[] = {QObject::tr("None (erase)"),QObject::tr("Tree (harvestable)"),QObject::tr("Dead tree / Cactuses / Thornbush"),QObject::tr("Stone"),QObject::tr("Bush / Grass / Fern (walkable)"),QObject::tr("Water object / Reed / Papyrus (walkable)"),QObject::tr("Big tree / Old palm"),QObject::tr("Hanged / Impaled"),QObject::tr("Statues"),QObject::tr("Mountain"),QObject::tr("Invisible Blocking Object")};
    int Tile::size = 5;

    //column and row are the position in Tiles of the Tile
    Tile::Tile(QGraphicsScene *scene, Renderer *renderer, int column, int row):QGraphicsItem(){//call the standard constructor of QGraphicsItemGroup
        scene->addItem(this);//add this to the scene

        this->renderer=renderer;//TODO: do I need this?
        this->leftLine = this->topLine = this->rightLine = this->bottomLine = true;
        this->water = this->object = this->resource = this->cliff = false;

        this->color = this->objectColor = this->resourceColor = Qt::black;

        this->height = this->renderer->getMap()->getHeight(column, row);//the fuck ist width? - xDD

        this->setAcceptHoverEvents(true);

        this->move(column, row);

        //this->setAcceptedMouseButtons(Qt::NoButton);

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
        /*if(changed){TODO: maybe use a updateBorders()
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
        }*/
    }

    void Tile::updateHeight(){
        this->height = this->renderer->getMap()->getHeight(column, row);
        //std::cout << "update!" << std::endl;
    }

    void Tile::recalculate(){
        //this->height = this->renderer->getMap()->getHeight(column, row);
        bool changed = false;

        QColor base = SURFACE[this->renderer->getMap()->getSurface(column, row)];
        this->water = false;
        //change detected by base color change
        if(this->renderer->getMap()->isCliff(column, row)){
            base = SURFACE[0];
            if(!this->cliff){
                changed = true;
                this->cliff = true;
            }
        }else{
            if(this->cliff){
                changed = true;
                this->cliff = false;
            }
        }
        if(this->renderer->getHeightMap()){//showing water but no cliffs
            base = Qt::lightGray;
        }
        double waterlevel = this->renderer->getMap()->getWaterLevel();
        if(this->height <= waterlevel && this->renderer->getWater()){
            this->water = true;
            if(this->height <= waterlevel - 1.5){
                base = Qt::blue;
            }else{
                base = Qt::cyan;
            }
        }
        base = base.darker(100+10*(20 - this->height));//make it darker
        if(this->color != base){
            changed = true;
            this->color = base;
        }
        int resource = this->renderer->getMap()->getResource(column, row);
        if(resource != 0){
            if(this->resource != resource){
                this->resource = resource;
                changed = true;
                this->resourceColor = RESOURCE[resource-1];
            }
        }else{
            if(this->resource){
                changed = true;
                this->resource = 0;
            }
        }
        int object = this->renderer->getMap()->getObject(column, row);
        if(object != 0){
            if(this->object != object){
                this->object = object;
                changed = true;
                this->objectColor = OBJECT[object-1];
            }
        }else{
            if(this->object){
                changed = true;
                this->object = 0;
            }
        }

        //TODO: check for change
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

        if(changed){
            this->update();
        }
    }

    void Tile::modifySize(int size){
        if(size > 0 || Tile::size+size >= 3)
            Tile::size += size;
    }

    int Tile::getSize(){
        return Tile::size;
    }

    QRectF Tile::boundingRect() const{//need this for colidesWith … setPos is ignored
        return QRectF(column * this->size,row * this->size, this->size, this->size);
    }

    void Tile::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget){
        //painter->translate(column * this->size,row * this->size);
        painter->fillRect(this->boundingRect(),QBrush(this->color));
        painter->translate(column * this->size,row * this->size);
        if(!this->renderer->getHeightMap()){
            if(!this->water){
                painter->setPen(QColor(0xFF, 0xFF, 0xFF, 0x42 ));//white
                if(this->leftLine)
                    painter->drawLine(0,0,0,size);//left
                if(this->topLine)
                    painter->drawLine(0,0,size,0);//top
                painter->setPen(QColor(0x00, 0x00, 0x00, 0x42 ));//black
                if(this->rightLine)
                    painter->drawLine(size-1,0,size-1,size-1);//right
                if(this->bottomLine)
                    painter->drawLine(0,size-1,size-1,size-1);//bottom
            }
            if(this->resource){
                painter->setPen(this->resourceColor);
                painter->drawLine(1,1,size-2,size-2);
                painter->drawLine(1,size-1,size-2,2);
            }
            if(this->cliff){
                float objectSpace = (size*2)/3.0;
                painter->fillRect(QRectF(objectSpace,objectSpace,size-(2*objectSpace),size-(2*objectSpace)),QBrush(Qt::black));
            }
            if(this->object){
                float objectSpace = (size*2)/3.0;
                painter->fillRect(QRectF(objectSpace,objectSpace,size-(2*objectSpace),size-(2*objectSpace)),QBrush(this->objectColor));
            }
        }
        if(this->renderer->getGrid()){
            painter->setPen(Qt::black);
            painter->drawLine(size-1,0,size-1,size-1);//right
            painter->drawLine(0,size-1,size-1,size-1);//bottom
        }

    }

    void Tile::mousePressEvent ( QGraphicsSceneMouseEvent *event){
        //cout << "mouse pressed @" << this->column << "," << this->row << endl;
        //event->ignore();
        //this->renderer->addHistory();
        this->renderer->getMapManipulator()->click(this->column, this->row);
        this->renderer->recalculateAll();
    }

    void Tile::mouseMoveEvent ( QGraphicsSceneMouseEvent *event){
        QPointF lastPoint = event->lastScenePos();
        int lastColumn = lastPoint.x() / size;
        int lastRow = lastPoint.y() / size;

        QPointF point = event->scenePos();
        int column = point.x() / size;
        int row = point.y() / size;

        this->renderer->getStatus()->pos->setText(QObject::tr("Position: %1, %2").arg(column,3,10,QChar('0')).arg(row,3,10,QChar('0')));
        this->renderer->getStatus()->ingame->setText(QObject::tr("Ingame: %1, %2").arg(column*2,3,10,QChar('0')).arg(row*2,3,10,QChar('0')));

        //drawing a line from last point to actual position
        QPainterPath path(lastPoint);
        path.lineTo(point);


        //checking a bounding rectangle with possible tiles between the points
        //limited by map size
        int width = std::min(std::max(lastColumn,column),this->renderer->getWidth()-1);
        int height = std::min(std::max(lastRow,row),this->renderer->getHeight()-1);

        for(int nextColumn = std::max(0,std::min(lastColumn,column)); nextColumn <= width; nextColumn++){
            for(int nextRow = std::max(0,std::min(lastRow,row)); nextRow <= height; nextRow++){
                //if(nextColumn != lastColumn && nextRow != lastRow){//no need redrawing the old one
                    if(this->renderer->at(nextColumn,nextRow)->collidesWithPath(path)){

                        this->renderer->getMapManipulator()->changeTile(nextColumn, nextRow);
                    }

                //}
            }
        }
        this->renderer->recalculateAll();
    }

    void Tile::mouseReleaseEvent ( QGraphicsSceneMouseEvent *event){
        /*QPointF point = event->scenePos();
        int column = point.x() / size;
        int row = point.y() / size;
        std::cout << "mouse released @" << column << "," << row << std::endl;*/
        //this->renderer->getMap()->setHasChanged(false);
        this->renderer->addHistory();
    }

    void Tile::hoverEnterEvent ( QGraphicsSceneHoverEvent *event ){
        //std::cout << "hovered @" << column << "," << row << std::endl;
        //this->renderer->getMapManipulator()->changeTile(column, row);
        this->renderer->getStatus()->pos->setText(QObject::tr("Position: %1, %2").arg(column,3,10,QChar('0')).arg(row,3,10,QChar('0')));
        this->renderer->getStatus()->ingame->setText(QObject::tr("Ingame: %1, %2").arg(column*2,3,10,QChar('0')).arg(row*2,3,10,QChar('0')));
        this->renderer->getStatus()->object->setText(QObject::tr("Object: %1").arg(this->OBJECTSTR[this->object]));
    }

    void Tile::dragEnterEvent (QGraphicsSceneDragDropEvent *event){
        //std::cout << "drag hovered @" << column << "," << row << std::endl;
    }

    void Tile::wheelEvent ( QGraphicsSceneWheelEvent *event ){
        Tile::modifySize(event->delta()/80);
        this->renderer->getScene()->setSceneRect(this->renderer->getScene()->itemsBoundingRect());
        this->renderer->updateTiles();
    }

    void Tile::move(int column, int row){
        //QGraphicsItem::setPos(column*size,row*size);
        this->row = row;
        this->column = column;
    }
}
