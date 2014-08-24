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

#include <QCoreApplication>
#include <QMouseEvent>
#include <QLabel>

#include "viewer.hpp"

#include "mainWindow.hpp"
#include "tile.hpp"

#include <iostream>


namespace MapEditor {
    Viewer::Viewer(QWidget *parent) : QGraphicsView(parent){
        setTransformationAnchor(AnchorUnderMouse);
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

    /*void Viewer::keyPressEvent(QKeyEvent *event){
        this->modeChecker(event->modifiers());
        QGraphicsView::keyPressEvent(event);
    }
    void Viewer::keyReleaseEvent(QKeyEvent *event){
        this->modeChecker(event->modifiers());
        QGraphicsView::keyReleaseEvent(event);
    }*/

    void Viewer::LeaveEvent(QEvent *event){
        this->status->pos->setText(QObject::tr("Position: (none)"));
        this->status->ingame->setText(QObject::tr("Ingame: (none)"));
    }
    void Viewer::modeChecker(Qt::KeyboardModifiers mod){
        if(mod == Qt::ControlModifier){
            this->setDragMode(QGraphicsView::ScrollHandDrag);
        }else if(mod == Qt::ShiftModifier){
            this->setDragMode(QGraphicsView::RubberBandDrag);
        }else {
            this->setDragMode(QGraphicsView::NoDrag);
        }
    }
    void Viewer::setStatus(Status *status){
        this->status = status;
    }

    //other zoom is in renderer
    void Viewer::wheelEvent ( QWheelEvent *event ){
        this->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

        double oldScale = transform().m11();
        //each level has 2 further pixels
        double oldLevel = (((Tile::getSize() * oldScale) - Tile::getSize())/2);

        double px = ((2.0)/Tile::getSize());//scale for 2px
        if(event->delta() > 0) {// zoom in
            if(oldLevel < 10*Tile::getSize()){
                double scale = 1.0+(oldLevel+1)*px;
                //create a new transformation matrix
                setTransform(QTransform().scale(scale, scale));
            }
        } else {// zoomout
            if(oldLevel > 1){
                double scale = 1.0+(oldLevel-1)*px;
                setTransform(QTransform().scale(scale, scale));
            }else{
                resetTransform();
            }
        }
        event->accept();
    }
}
