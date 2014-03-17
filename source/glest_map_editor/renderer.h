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

class QGraphicsScene;
class QAction;
/*namespace Shared{ namespace Map{
    class MapPreview;
} }*/

namespace MapEditor {
    class Tile;
    class MapManipulator;

    class Renderer{//QObject because of the slot
        public:
            Renderer(MapManipulator *mapman);
            ~Renderer();
            void reduceHeight(int height);
            void changeSurface(int surface);
            QGraphicsScene *getScene() const;
            Tile* at(int column, int row) const;
            void recalculateAll();
            void updateMap();
            int getHeight() const;
            int getWidth() const;
            void open(std::string path);
            void resize();
            void save();
            void save(std::string path);
            bool isSavable();
            std::string getFilename() const;
            void resetFilename();
            Shared::Map::MapPreview *getMap() const;
            MapManipulator *getMapManipulator() const;
            void newMap();
        private:
            void removeTiles();
            void createTiles();
            QGraphicsScene *scene;
            Tile*** Tiles;
            Shared::Map::MapPreview *map;
            MapManipulator *mapman;
            int height;
            int width;
            std::vector<Shared::Map::MapPreview*> history;
            int historyPos;
            void addHistory();
            void redo();
            void undo();
            std::string filename;
    };
}

#endif
