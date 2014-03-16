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

#ifndef NEWMAP_H
#define NEWMAP_H

#include <QDialog>

namespace Ui {
class NewMap;
}

namespace MapEditor {
    class Renderer;

    class NewMap : public QDialog
    {
        Q_OBJECT

    public:
        explicit NewMap(Renderer *renderer, QWidget *parent = 0);
        ~NewMap();

    protected:
        void changeEvent(QEvent *e);

    private:
        Ui::NewMap *ui;
        Renderer *renderer;

    private slots:
        void create();
    };
}

#endif // NEWMAP_H
