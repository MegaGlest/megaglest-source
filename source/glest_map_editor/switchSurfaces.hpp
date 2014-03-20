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

#ifndef SWITCHSURFACES_HPP
#define SWITCHSURFACES_HPP

#include <QDialog>

namespace Ui {
class SwitchSurfaces;
}

namespace MapEditor {
    class MapManipulator;

    class SwitchSurfaces : public QDialog
    {
        Q_OBJECT

    public:
        explicit SwitchSurfaces(MapManipulator *mapman, QWidget *parent = 0);
        ~SwitchSurfaces();

    protected:
        void changeEvent(QEvent *e);

    private:
        Ui::SwitchSurfaces *ui;
        MapManipulator *mapman;

    private slots:
        void create();
    };
}

#endif // NEWMAP_HPP
