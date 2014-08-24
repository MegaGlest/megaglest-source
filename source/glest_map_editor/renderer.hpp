/**
 * @file renderer.hpp
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


#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <QObject>

#include "map_preview.h"

#include <vector>

class QGraphicsScene;
class QGraphicsItemGroup;
class QAction;
/*namespace Shared{ namespace Map{
    class MapPreview;
} }*/

namespace MapEditor {
    class Tile;
    class Player;
    class Selection;
    class MapManipulator;
    struct Status;

    class Renderer: public QObject{//QObject because of the slot
        Q_OBJECT
        public:
            Renderer(MapManipulator *mapman, Status *status);
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
             * @return height of the map
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
            Tile *at(int column, int row) const;
            /**
             * Does that specific tile exist?
             * @param column Position of that tile
             * @param row Position of that tile
             * @return tile at a specific position
             */
            bool exists(int column, int row) const;
            /**
             *@return pointer for statusbar QLabels
             */
            Status* getStatus() const;
            /**
             *@return if water is visible
             */
            bool getWater() const;
            /**
             *@return if grid is visible
             */
            bool getGrid() const;
            /**
             *@return if only heihgt map is shown
             */
            bool getHeightMap() const;
            Selection *getSelectionRect();
        public slots:
            /**
             *@param water show water
             */
            void setWater(bool water);
            /**
             *@param grid show grid
             */
            void setGrid(bool grid);
            /**
             *@param heigtMap show only height map
             */
            void setHeightMap(bool heightMap);
            /**
             * redo last undone step
             */
            void redo();
            /**
             * undo last step
             */
            void undo();
            /**
             * Adds the actual map to the history
             */
            void addHistory();
            /**
             * Forgets the last change in history
             * Useful if you call different methods, who set their own history
             */
            void forgetLast();
            /**
             * vanishes whole history
             */
            void clearHistory();
            /**
             * Zooms in and keeps the tile in the center of the screen centered
             * This is the menu-zoom, the other zoom is in Viewer::wheelEvent
             */
            void zoomIn();
            /**
             * Zooms in and keeps the tile in the center of the screen centered
             * This is the menu-zoom, the other zoom is in Viewer::wheelEvent
             */
            void zoomOut();
            void fitZoom();
            /**
             * initiate a repaint of all tiles
             */
            void updateTiles();
            /**
             * updates positions of all players
             */
            void updatePlayerPositions();
            /**
             * discard not updated position changes
             */
            void restorePlayerPositions();
            /**
             * updates allowed amount of all players
             */
            void updateMaxPlayers();
        private:
            /**
             * Fill the scene with new tiles, depends on the map size
             */
            void createTiles();
            /**
             * Deletes all tile instances
             */
            void removeTiles();
            void zoom(int delta, int pixels);
            QGraphicsScene *scene;
            Tile*** Tiles;
            QGraphicsItemGroup *tileContainer;
            Player *player[8];
            QGraphicsItemGroup *playerContainer;
            Selection *selectionRect;
            Shared::Map::MapPreview *map;
            MapManipulator *mapman;
            int height;
            int width;
            std::vector<Shared::Map::MapPreview> history;
            int historyPos;
            Status *status;

            bool water;
            bool grid;
            bool heightMap;

            std::string filename;

    };
}

#endif
