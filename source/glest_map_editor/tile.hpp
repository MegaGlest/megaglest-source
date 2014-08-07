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

#ifndef TILE_HPP
#define TILE_HPP

#include <QGraphicsItem>

class QGraphicsScene;
class QGraphicsSceneMouseEvent;
class QGraphicsSceneHoverEvent;
class QGraphicsSceneDragDropEvent;
class QGraphicsRectItem;
class QGraphicsLineItem;
class QGraphicsItem;
class QColor;
class QString;

namespace MapEditor {
    class Renderer;

    class Tile:public QGraphicsItem{
        public:
            Tile(QGraphicsItem *parent, Renderer *renderer,int column=0,int row=0);
            //~Tile();
            /**
             * FIXME: critical, surfaces are enums
             * @param surface 0-4
             */
            void setSurface(int surface);
            /**
             * altitude
             */
            void setHeight(double height);
            /**
             * altitude
             */
            double getHeight() const;
            /**
             * get height from map
             */
            void updateHeight();
            /**
             * Setting color, borders, objects and resources depending on map cell.
             * Borders depend on the height of this tile and the height of the direct neighbors
             */
            void recalculate();
            /**
             * @return size of all tiles
             */
            static int getSize();
            virtual QRectF boundingRect() const;
            virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
        protected:
            virtual void mousePressEvent ( QGraphicsSceneMouseEvent * event);
            virtual void mouseMoveEvent ( QGraphicsSceneMouseEvent * event);
            virtual void mouseReleaseEvent ( QGraphicsSceneMouseEvent * event);
        private:
            /**
             * Changes the position of this tile, does not use built in setPos.
             * Uses an offset for bounding box and painter instead.
             * @param column New position
             * @param row New position
             */
            void move(int column, int row);
            static const QColor SURFACE[];
            static const QColor OBJECT[];
            static const QColor RESOURCE[];
            static const QString OBJECTSTR[];
            static int size;
            Renderer* renderer;
            bool topLine;
            bool rightLine;//right line of the tile on the left
            bool bottomLine;//bottom line of tile above
            bool leftLine;
            double height;
            int column;
            int row;
            QColor color;
            bool water;
            bool cliff;
            int object;
            QColor objectColor;
            int resource;
            QColor resourceColor;
            Qt::MouseButton pressedButton;
    };
}
#endif
