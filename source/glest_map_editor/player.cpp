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

#include "player.hpp"
#include "tile.hpp"
#include "renderer.hpp"
#include <QPainter>
#include <iostream>

namespace MapEditor {

    //column and row are the position in Players of the Player
    Player::Player(const QColor &color):QGraphicsItem(){
        this->column = 0;
        this->row = 0;
        this->color = color;
        this->color.setAlphaF(.5);
    }

    QRectF Player::boundingRect() const{//need this for colidesWith … setPos is ignored
        return QRectF((column - 1) * Tile::getSize(),(row - 1) * Tile::getSize(), Tile::getSize()*3, Tile::getSize()*3);
    }

    void Player::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget){
        painter->translate((column - 1) * Tile::getSize(),(row - 1) * Tile::getSize());
        painter->setPen(QPen(this->color, 3, Qt::SolidLine, Qt::RoundCap));
        QLine lines[2] = {QLine(1,1,Tile::getSize()*3 - 2,Tile::getSize()*3 - 2),
                          QLine(1,Tile::getSize()*3 - 2,Tile::getSize()*3 - 2,1)};
        painter->drawLines(lines, 2);
        //drawLine would produce overlapping lines -> ugly with half opacity
    }

    void Player::move(int column,int row){
        this->column = column;
        this->row = row;
        this->prepareGeometryChange();
    }

}
