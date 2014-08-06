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

#include "selection.hpp"
#include "tile.hpp"
#include "renderer.hpp"
#include <QPainter>
#include <iostream>

namespace MapEditor {

    //column and row are the position in Players of the Player
    Selection::Selection():QGraphicsItem(){
        this->column = 0;
        this->row = 0;
        this->width = 0;
        this->height = 0;
    }

    QRectF Selection::boundingRect() const{//need this for colidesWith … setPos is ignored
        return QRectF(column * Tile::getSize(),row * Tile::getSize(), width * Tile::getSize() - 1, height * Tile::getSize() - 1);
    }

    void Selection::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget){
        QPen red(Qt::red, 1, Qt::DashLine, Qt::RoundCap);
        red.setCosmetic(true);
        painter->setPen(red);
        painter->drawRect(this->boundingRect());
        //drawLine would produce overlapping lines -> ugly with half opacity
    }

    void Selection::move(int column,int row){
        this->column = column;
        this->row = row;
        this->prepareGeometryChange();
    }

    void Selection::resize(int width, int height){
        this->width = width;
        this->height = height;
        this->prepareGeometryChange();
    }

}
