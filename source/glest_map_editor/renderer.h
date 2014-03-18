/**
 * @file renderer.h
 * @author  Sebastian Riedel
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * The renderer class is the visual representation of the map.
 * Consists of tiles.
 * This file is part of MegaGlest (megaglest.org)
 */


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
            /**
             * opens a map
             * @param path Path of that file
             */
            void open(std::string path);
            /**
             * Use this method if the map changed its size
             * Rebuilds the whole map
             * Recalculates all tiles
             * Shrinks the scene if necessary
             * takes some time
             */
            void resize();
            /**
             * quicksave, does nothing if no filename is deposited
             */
            void save();
            /**
             * saves a map
             * @param path Path of that file
             */
            void save(std::string path);
            /**
             * @return true when a filename is deposited
             */
            bool isSavable();
            /**
             * @return deposited filename
             */
            std::string getFilename() const;
            /**
             * forget deposited filename
             */
            void resetFilename();
            /**
             * Calls recalculate() of every tile
             */
            void recalculateAll();
            /**
             * Calls updateHeight of every tile
             */
            void updateMap();
            /**
             * @return heigt of the map
             */
            int getHeight() const;
            /**
             * @return width of the map
             */
            int getWidth() const;
            /**
             * @return an instance of the map itself
             */
            Shared::Map::MapPreview *getMap() const;
            /**
             * @return an instance of the map manipulator
             */
            MapManipulator *getMapManipulator() const;
            /**
             * @return the Qt scene, all tiles are part of this scene
             */
            QGraphicsScene *getScene() const;
            /**
             * Access a specific tile
             * @param column Position of that tile
             * @param row Position of that tile
             * @return tile at a specific position
             */
            Tile* at(int column, int row) const;
        private:
            /**
             * Fill the scene with new tiles, depends on the map size
             */
            void createTiles();
            /**
             * Deletes all tile instances
             */
            void removeTiles();
            /**
             * Adds the actual map to the history
             * TODO: remove undone maps
             */
            void addHistory();
            /**
             * redo last undone step
             * TODO: implement it
             */
            void redo();
            /**
             * undo last step
             * TODO: implement it
             */
            void undo();
            QGraphicsScene *scene;
            Tile*** Tiles;
            Shared::Map::MapPreview *map;
            MapManipulator *mapman;
            int height;
            int width;
            std::vector<Shared::Map::MapPreview*> history;
            int historyPos;

            std::string filename;
    };
}

#endif
