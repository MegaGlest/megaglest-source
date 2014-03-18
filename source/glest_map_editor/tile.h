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

#ifndef TILE_H
#define TILE_H

#include <QGraphicsItem>

class QGraphicsScene;
class QGraphicsSceneMouseEvent;
class QGraphicsSceneHoverEvent;
class QGraphicsSceneDragDropEvent;
class QGraphicsRectItem;
class QGraphicsLineItem;
class QGraphicsItem;
class QColor;

namespace MapEditor {
        class Renderer;

        class Tile:public QGraphicsItem{
            public:
                Tile(QGraphicsScene *scene, Renderer *renderer,int column=0,int row=0);
                //~Tile();
                void setSurface(int surface);
                void setHeight(double height);
                double getHeight() const;
                void updateHeight();
                void recalculate();
                virtual QRectF boundingRect() const;
                virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
            protected:
                virtual void mousePressEvent ( QGraphicsSceneMouseEvent * event);
                virtual void mouseMoveEvent ( QGraphicsSceneMouseEvent * event);
                virtual void mouseReleaseEvent ( QGraphicsSceneMouseEvent * event);
                virtual void hoverEnterEvent ( QGraphicsSceneHoverEvent * event );
                virtual void dragEnterEvent (QGraphicsSceneDragDropEvent * event);
            private:
                void move(int column, int row);
                static const QColor SURFACE[];
                static const QColor OBJECT[];
                static const QColor RESOURCE[];
                static const int SIZE;
                Renderer* renderer;
                bool topLine;
                bool rightLine;
                bool bottomLine;
                bool leftLine;
                double height;
                int column;
                int row;
                QColor color;
                bool water;
                bool object;
                QColor objectColor;
                bool resource;
                QColor resourceColor;
        };
}
#endif
