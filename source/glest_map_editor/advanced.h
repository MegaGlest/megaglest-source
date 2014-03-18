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

#ifndef ADVANCED_H
#define ADVANCED_H

#include <QDialog>

namespace Ui {
class Advanced;
}

namespace MapEditor {
    class MapManipulator;

    class Advanced : public QDialog
    {
        Q_OBJECT

    public:
        explicit Advanced(MapManipulator *mapman, QWidget *parent = 0);
        ~Advanced();

    protected:
        void changeEvent(QEvent *e);

    private:
        Ui::Advanced *ui;
        MapManipulator *mapman;

    private slots:
        void create();
    };
}

#endif // NEWMAP_H
