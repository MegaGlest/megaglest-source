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

#include "newMap.hpp"
#include "ui_newMap.h"
#include "mapManipulator.hpp"
#include <iostream>

namespace MapEditor {
    NewMap::NewMap(MapManipulator *mapman, char modus, QWidget *parent) : QDialog(parent), ui(new Ui::NewMap), mapman(mapman){
        ui->setupUi(this);
        this->modus = modus;
        //ui->inputPlayers->deleteLater();
        //ui->labelPlayers->deleteLater();
        //QDialog::adjustSize();
        connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(create()));//create new map
        //if(modus != 'n'){
            switch(modus){
                case 'r'://resize
                    //delete not needed fields
                    ui->inputPlayers->deleteLater();
                    ui->labelPlayers->deleteLater();
                    //set actual values to status of current map
                    ui->inputHeight->setValue(this->mapman->getHeight());
                    ui->inputWidth->setValue(this->mapman->getWidth());
                break;
                case 'p'://reset players
                    //delete not needed fields
                    ui->inputWidth->deleteLater();
                    ui->labelWidth->deleteLater();
                    ui->inputHeight->deleteLater();
                    ui->labelHeight->deleteLater();
                    ui->inputSurface->deleteLater();
                    ui->labelSurface->deleteLater();
                    ui->inputAltitude->deleteLater();
                    ui->labelAltitude->deleteLater();
                    //set actual values to status of current map
                    ui->inputPlayers->setValue(this->mapman->getPlayerAmount());
                break;
            }

        //}
        //ui->formLayout->adjustSize();
        QDialog::adjustSize();
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
        int width=0,height=0,surface=0,altitude=0,players=0;

        if(modus != 'p'){// only player reset does not need this
            width = ui->inputWidth->text().toInt();
            height = ui->inputHeight->text().toInt();
            surface = ui->inputSurface->currentIndex();
            altitude = ui->inputAltitude->text().toFloat();
        }
        if(modus != 'r'){//only resize does not need this
            players = ui->inputPlayers->text().toInt();
        }

        switch(modus){
            case 'r'://resize
                this->mapman->resize(width, height, surface, altitude);
                break;
            case 'p'://reset players
                this->mapman->changePlayerAmount(players);
                break;
            case 'n'://new map
                this->mapman->reset(width, height, surface, altitude, players);
                break;
        }
    }
}
