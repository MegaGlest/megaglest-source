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
             * Sets history
             */
            void randomizeHeights();
            /**
             * Sets history
             */
            void randomizeHeightsAndPlayers();
            /**
             * Sets history
             */
            void copyL2R();
            /**
             * Sets history
             */
            void copyT2B();
            /**
             * Sets history
             */
            void copyBL2TR();
            /**
             * Sets history
             */
            void rotateL2R();
            /**
             * Sets history
             */
            void rotateT2B();
            /**
             * Sets history
             */
            void rotateBL2TR();
            /**
             * Sets history
             */
            void rotateTL2BR();
            /**
             * Sets history
             */
            void switchSurfaces(int a, int b);
            /**
             * Sets history
             */
            void setInfo(const std::string &title, const std::string &description, const std::string &author);
            /**
             * Sets history
             */
            void setAdvanced(int heightFactor, int waterLevel, int cliffLevel, int cameraHeight);
            /**
             * Sets history
             */
            void resetPlayers(int amount);
            /**
             * Sets history
             */
            void resize(int width, int height);
            /**
             * define a selection rectangle
             * -1 means select all
             */
            void setSelection(int startColumn, int startRow, int endColumn, int endRow);
            /**
             * Reads scene selection
             */
            void selectionChanged();
            /**
             * fit selection in scene
             */
            void fitSelection();
        private:
            void updateEverything();
            /**
             * uses a methode on all selected tiles
             * @param modus c: copy; s: swap
             * TODO: more explaination
             */
            void doToSelection(char modus, int columnLimit, int rowLimit ,bool invertColumn, bool invertRow);
            MainWindow *win;//for user input
            Renderer *renderer;//for accessing the map
            int radius;
            int selectionStartColumn;
            int selectionStartRow;
            int selectionEndColumn;
            int selectionEndRow;
    };
}

#endif
