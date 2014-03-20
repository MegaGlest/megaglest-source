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

#ifndef INFO_HPPPP
#define INFO_HPPPP

#include <QDialog>

namespace Ui {
class Info;
}

namespace MapEditor {
    class MapManipulator;

    class Info : public QDialog
    {
        Q_OBJECT

    public:
        explicit Info(MapManipulator *mapman, QWidget *parent = 0);
        ~Info();

    protected:
        void changeEvent(QEvent *e);

    private:
        Ui::Info *ui;
        MapManipulator *mapman;

    private slots:
        void create();
    };
}

#endif // NEWMAP_HPP
