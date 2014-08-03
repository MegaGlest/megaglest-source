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

#ifndef NEWMAP_HPP
#define NEWMAP_HPP

#include <QDialog>

namespace Ui {
class NewMap;
}

namespace MapEditor {
    class MapManipulator;

    class NewMap : public QDialog
    {
        Q_OBJECT

    public:
        explicit NewMap(MapManipulator *mapman, char modus, QWidget *parent = 0);
        ~NewMap();

    protected:
        void changeEvent(QEvent *e);

    private:
        Ui::NewMap *ui;
        MapManipulator *mapman;
        char modus;

    private slots:
        void create();
    };
}

#endif // NEWMAP_HPP
