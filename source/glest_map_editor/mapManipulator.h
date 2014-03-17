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
#ifndef MAPMANIPULATOR_H
#define MAPMANIPULATOR_H

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
            void setRenderer(Renderer *renderer);
        public slots:
            void setRadius(QAction *radius);
            void changeTile(int column, int row);
        private:
            MainWindow *win;//for user input
            Renderer *renderer;//for accessing the map
            int radius;
    };
}

#endif
