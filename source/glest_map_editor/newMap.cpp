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

#include "newMap.h"
#include "ui_newMap.h"
#include "renderer.h"
#include <iostream>

namespace MapEditor {
    NewMap::NewMap(Renderer *renderer, QWidget *parent) : QDialog(parent), ui(new Ui::NewMap), renderer(renderer){
        ui->setupUi(this);
        connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(create()));//create new map
    }

    NewMap::~NewMap(){
        delete ui;
    }

    void NewMap::changeEvent(QEvent *e){
        QDialog::changeEvent(e);
        switch (e->type()) {
         case QEvent::LanguageChange:
            ui->retranslateUi(this);
            break;
         default:
            break;
        }
    }

    void NewMap::create(){
        std::cout << "create: " << ui->inputAltitude->text().toStdString () << "; " << ui->inputHeight->text().toStdString () << "; " << ui->inputWidth->text().toStdString () << "; " << ui->inputPlayers->text().toStdString () << "; " << ui->inputSurface->currentIndex() << "; " << std::endl;
    }
}
