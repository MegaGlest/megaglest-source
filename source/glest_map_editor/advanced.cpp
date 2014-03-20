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

#include "advanced.hpp"
#include "ui_advanced.h"
#include "mapManipulator.hpp"
#include <iostream>

namespace MapEditor {
    Advanced::Advanced(MapManipulator *mapman, QWidget *parent) : QDialog(parent), ui(new Ui::Advanced), mapman(mapman){
        ui->setupUi(this);
        connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(create()));//create new map
    }

    Advanced::~Advanced(){
        delete ui;
    }

    void Advanced::changeEvent(QEvent *e){
        QDialog::changeEvent(e);
        switch (e->type()) {
         case QEvent::LanguageChange:
            ui->retranslateUi(this);
            break;
         default:
            break;
        }
    }

    void Advanced::create(){
        /*int width = ui->inputWidth->text().toInt();
        int height = ui->inputHeight->text().toInt();
        int surface = ui->inputSurface->currentIndex();
        int altitude = ui->inputAltitude->text().toFloat();
        int players = ui->inputPlayers->text().toInt();
        this->mapman->reset(width, height, surface, altitude, players);*/
    }
}
