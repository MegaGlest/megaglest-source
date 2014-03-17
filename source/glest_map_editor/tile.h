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

#include <QGraphicsItemGroup>

class QGraphicsScene;
class QGraphicsSceneMouseEvent;
class QGraphicsSceneHoverEvent;
class QGraphicsSceneDragDropEvent;
class QGraphicsRectItem;
class QGraphicsLineItem;
class QGraphicsItem;

namespace MapEditor {
        class Renderer;

        class Tile:public QGraphicsItemGroup{
            public:
                Tile(QGraphicsScene *scene, Renderer *renderer,int column=0,int row=0);
                ~Tile();
                void recalculate();
                double getHeight() const;
                void setHeight(double height);
                void setSurface(int surface);
                void update();
                void clicked(QGraphicsSceneMouseEvent * event);
            protected:
                virtual void mousePressEvent ( QGraphicsSceneMouseEvent * event);
                virtual void mouseMoveEvent ( QGraphicsSceneMouseEvent * event);
                virtual void mouseReleaseEvent ( QGraphicsSceneMouseEvent * event);
                virtual void hoverEnterEvent ( QGraphicsSceneHoverEvent * event );
                virtual void dragEnterEvent (QGraphicsSceneDragDropEvent * event);
            private:
                static const QColor SURFACE[];
                static const QColor OBJECT[];
                static const int SIZE;
                Renderer* renderer;
                void move(int column, int row);
                QGraphicsRectItem *rect;
                QGraphicsRectItem *water;
                QGraphicsRectItem *object;
                QGraphicsLineItem *topLine;
                QGraphicsLineItem *rightLine;
                QGraphicsLineItem *bottomLine;
                QGraphicsLineItem *leftLine;
                double height;
                int column;
                int row;

                int children;
                QGraphicsItem ** child;
        };
}
#endif
