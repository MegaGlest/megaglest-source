/**
 * @file mapManipulator.hpp
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
 * The map manipulator class glues the GUI and MapPreview.
 * This file is part of MegaGlest (megaglest.org)
 */

#ifndef MAPMANIPULATOR_HPP
#define MAPMANIPULATOR_HPP

#include <QObject>


class QAction;

namespace MapEditor{
    class MainWindow;
    class Renderer;

    struct SubMapHeader{
        int column;
        int row;
        int width;
        int height;
    };

    class MapManipulator: public QObject{
        Q_OBJECT
        public:
            MapManipulator(MainWindow *win);
            ~MapManipulator();
            /**
             * Set this immediately after colling the constructor!
             */
            void setRenderer(Renderer *renderer);
            MainWindow *getWindow();
        public slots:
            /**
             * Sets the radius for the drawing tool
             * @param *radius A QAction representing a radius size
             */
            void setRadius(QAction *radius);
            /**
             * Draws with selected drawing tool and radius
             * @param column Where you wanna draw
             * @param row Where you wanna draw
             */
            void changeTile(int column, int row);
            /**
             * Click which initiates drawing.
             * Chooses the height for gradient and height tools
             * @param column Where you wanna draw
             * @param row Where you wanna draw
             */
            void click(int column, int row);
            /**
             * Flips the map diagonal
             * Sets history
             */
            void flipDiagonal();
            /**
             * Flips the map on the X-axis
             * Sets history
             */
            void flipX();
            /**
             * Flips the map on the Y-axis
             * Sets history
             */
            void flipY();
            /**
             * randomizes all tile heights of the map
             */
            void randomizeHeights();
            /**
             * randomizes all tile heights and player positions of the map
             */
            void randomizeHeightsAndPlayers();
            /**
             * (necessary?) TODO: Sets history
             */
            void copyL2R();
            /**
             * (necessary?) TODO: Sets history
             */
            void copyT2B();
            /**
             * (necessary?) TODO: Sets history
             */
            void copyBL2TR();
            /**
             * (necessary?) TODO: Sets history
             */
            void rotateL2R();
            /**
             * (necessary?) TODO: Sets history
             */
            void rotateT2B();
            /**
             * (necessary?) TODO: Sets history
             */
            void rotateBL2TR();
            /**
             * (necessary?) TODO: Sets history
             */
            void rotateTL2BR();
            /**
             * replaces all surfaces a in the selected area with surface b and vice versa
             */
            void switchSurfaces(int a, int b);
            /**
             * sets textual informations for this map
             */
            void setInfo(const std::string &title, const std::string &description, const std::string &author);
            /**
             * sets advanced option for ingame view of the map
             */
            void setAdvanced(int heightFactor, int waterLevel, int cliffLevel, int cameraHeight);
            /**
             * Resets the actual map / creates a new one.
             * Clears history.
             * @param width New size of the map
             * @param height New size of the map
             * @param surface Whole map gets filled with this surface
             * @param altitude Whole map will have this altitude
             * @param players Number of players for this map
             */
            void reset(int width, int height, int surface, float altitude, int players);
            /**
             * new amount of allowed players on this map
             */
            void changePlayerAmount(int amount);
            /**
             * Expands or shrinks the map.
             * @param width New size of the map
             * @param height New size of the map
             * @param surface New areas gets filled with this surface
             * @param altitude New areas will have this altitude
             */
            void resize(int width, int height, int surface, float altitude);

            /**
             * Converts int to enum MapSurfaceType
             * @param surface A number that represents the surface
             */
            //Shared::Map::MapSurfaceType intToSurface(int surface);
            /**
             * define a selection rectangle
             * -1 means select all
             */
            void setSelection(int startColumn = -1, int startRow = -1, int endColumn = -1, int endRow = -1);
            /**
             *  select the whole map
             */
            void selectAll();
            /**
             *  decide if the selection should be a square
             */
            void enableSelctionsquare(bool enable);
            /**
             * Reads scene selection
             */
            void selectionChanged();
            /**
             * fit selection in scene
             * @param useUserSettings choose if you wanna respect user settings or ignore them
             */
            void fitSelection(bool useUserSettings = true);
            /**
             * copy the selected area
             */
            void copy();
            /**
             * paste to selected area
             */
            void paste();
            /**
             * heihgt of map (dimension)
             */
            int getHeight();
            /**
             * width of map (dimension)
             */
            int getWidth();
            /**
             * maximum of players allowed on this map
             */
            int getPlayerAmount();
        private:
            void updateEverything();
            int projectCol(int col, int row, bool rotate, char modus);
            int projectRow(int col, int row, bool rotate, char modus);
            /**
             * uses a methode on all selected tiles
             * @param modus c: copy; s: swap
             * TODO: more explaination
             */
            void axisTool(char modus, bool swap, bool rotate, bool inverted);

            MainWindow *win;//for user input
            Renderer *renderer;//for accessing the map
            int radius;
            int selectionStartColumn;
            int selectionStartRow;
            int selectionEndColumn;
            int selectionEndRow;
            bool selectionsquare;
    };
}

#endif
