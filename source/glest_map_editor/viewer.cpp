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

#include "viewer.hpp"
#include <QMouseEvent>
#include <QLabel>
#include "mainWindow.hpp"
#include "tile.hpp"
#include <iostream>

namespace MapEditor {
    Viewer::Viewer(QWidget *parent) : QGraphicsView(parent){
    }

    Viewer::~Viewer(){
        //delete ui;
    }

    void Viewer::mouseMoveEvent(QMouseEvent *event){
        this->modeChecker(event->modifiers());
        QGraphicsView::mouseMoveEvent(event);
        QPointF point = mapToScene(event->pos());
        int column = point.x() / Tile::getSize();
        int row = point.y() / Tile::getSize();

        //is the mouse inside the field?
        if(column < 0 || row < 0 || column > this->sceneRect().width() / Tile::getSize() || row > this->sceneRect().height() / Tile::getSize()){
            this->status->pos->setText(QObject::tr("Position: (none)"));
            this->status->ingame->setText(QObject::tr("Ingame: (none)"));
        }else{
            this->status->pos->setText(QObject::tr("Position: %1, %2").arg(column).arg(row));
            this->status->ingame->setText(QObject::tr("Ingame: %1, %2").arg(column*2).arg(row*2));
            //this->status->object->setText(QObject::tr("Object: %1").arg(this->OBJECTSTR[this->object]));
        }

    }

    /*void Viewer::mousePressEvent(QMouseEvent *event){
        if(event->modifiers() == Qt::ControlModifier){
            this->setDragMode(QGraphicsView::ScrollHandDrag);
        }else if(event->modifiers() == Qt::AltModifier){
            this->setDragMode(QGraphicsView::RubberBandDrag);
        }else {
            this->setDragMode(QGraphicsView::NoDrag);
        }
        QGraphicsView::mousePressEvent(event);
    }*/

    void Viewer::keyPressEvent(QKeyEvent *event){
        this->modeChecker(event->modifiers());
        QGraphicsView::keyPressEvent(event);
    }
    void Viewer::keyReleaseEvent(QKeyEvent *event){
        this->modeChecker(event->modifiers());
        QGraphicsView::keyReleaseEvent(event);
    }
    void Viewer::LeaveEvent(QEvent *event){
        this->status->pos->setText(QObject::tr("Position: (none)"));
        this->status->ingame->setText(QObject::tr("Ingame: (none)"));
    }
    void Viewer::modeChecker(Qt::KeyboardModifiers mod){
        //std::cout << "test" << std::endl;
        if(mod == Qt::ControlModifier){
            this->setDragMode(QGraphicsView::ScrollHandDrag);
        }else if(mod == Qt::AltModifier){
            this->setDragMode(QGraphicsView::RubberBandDrag);
        }else {
            this->setDragMode(QGraphicsView::NoDrag);
        }
    }
    void Viewer::setStatus(Status *status){
        this->status = status;
    }
}
