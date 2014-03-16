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

#ifndef RENDERER_H
#define RENDERER_H

#include "map_preview.h"
#include <vector>
#include <QObject>

class RenderTile;
class QGraphicsScene;
class QAction;
/*namespace Shared{ namespace Map{
    class MapPreview;
} }*/

class Renderer: public QObject{//QObject because of the slot
    Q_OBJECT
    public:
        Renderer();
        ~Renderer();
        void reduceHeight(int height);
        void changeSurface(int surface);
        QGraphicsScene *getScene() const;
        RenderTile* at(int column, int row) const;
        void recalculateAll();
        void updateMap();
        int getHeight() const;
        int getWidth() const;
        void open(std::string path);
        Shared::Map::MapPreview *getMap() const;
        void newMap();
        int getRadius() const;
    public slots:
        void setRadius(QAction *radius);
    private:
        void removeTiles();
        void createTiles();
        QGraphicsScene *scene;
        RenderTile*** tiles;
        Shared::Map::MapPreview *map;
        int height;
        int width;
        std::vector<Shared::Map::MapPreview*> history;
        int historyPos;
        void addHistory();
        void redo();
        void undo();
        int radius;
};

#endif
