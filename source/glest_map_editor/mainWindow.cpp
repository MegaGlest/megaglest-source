// ==============================================================
//  This file is part of Glest (www.glest.org)
//
//  Copyright (C) 2001-2008 Martiño Figueroa
//
//  You can redistribute this code and/or modify it under
//  the terms of the GNU General Public License as published
//  by the Free Software Foundation; either version 2 of the
//  License, or (at your option) any later version
// ==============================================================

//#include <ctime>
//#include "conversion.h"
#include "platform_common.h"
#include "config.h"
#include <iostream>
//#include "platform_util.h"
#ifndef WIN32
#include <errno.h>
#endif
#include "mainWindow.h"
#include "ui_mainWindow.h"
#include "newMap.h"
#include "renderer.h"
#include "mapManipulator.h"
#include <QFileDialog>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QActionGroup>
//#include <QAction>
//#include <memory>

using namespace Shared::PlatformCommon;
using namespace Glest::Game;

namespace Glest { namespace Game {//need that somehow for default path detection
string getGameReadWritePath(string lookupKey) {
    string path = "";
    if(path == "" && getenv("GLESTHOME") != NULL) {
        path = getenv("GLESTHOME");
        if(path != "" && EndsWith(path, "/") == false && EndsWith(path, "\\") == false) {
            path += "/";
        }
    }
    return path;
}
}}


namespace MapEditor {
    MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent),ui(new Ui::MainWindow){
        ui->setupUi(this);

        renderer = new Renderer(new MapManipulator(this));
        newmap = new NewMap(renderer);//instance of new map dialog

        connect(ui->actionNew, SIGNAL(triggered()), newmap, SLOT(show()));//new map dialog window
        connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(openFile()));//open dialog window
        connect(ui->actionSave_as, SIGNAL(triggered()), this, SLOT(saveFile()));//save as dialog window

        //those actions form a selection group -> only one is selected
        radiusGroup = new QActionGroup(this);
        QList<QAction *> actions = ui->menuRadius->actions();
        while(!actions.isEmpty()){
            radiusGroup->addAction(actions.takeFirst());
        }
        connect(radiusGroup,SIGNAL(triggered(QAction*)),this->renderer->getMapManipulator(),SLOT(setRadius(QAction*)));

        penGroup = new QActionGroup(this);
        QList<QMenu *> penMenus;
        penMenus << ui->menuGradient << ui->menuSurface << ui->menuResource << ui->menuObject << ui->menuHeight << ui->menuPlayer;
        //adds all QActions from the menus listed above to penGroup
        while(!penMenus.isEmpty()){
            actions = penMenus.takeFirst()->actions();
            while(!actions.isEmpty()){
                penGroup->addAction(actions.takeFirst());
            }
        }

        ui->graphicsView->setScene(renderer->getScene());
        //ui->graphicsView->scale(5,5);
    }

    MainWindow::~MainWindow(){
        delete ui;
        delete newmap;
        delete radiusGroup;
        delete penGroup;
    }

    QAction *MainWindow::getPen() const{
        return penGroup->checkedAction();
    }

    //for translation ... somehow
    void MainWindow::changeEvent(QEvent *e){
        QMainWindow::changeEvent(e);
        switch (e->type()) {
        case QEvent::LanguageChange:
            ui->retranslateUi(this);
            break;
        default:
            break;
        }
    }

    void MainWindow::openFile(){
        Config &config = Config::getInstance();
        string userData = config.getString("UserData_Root","");
        string defaultPath = userData + "maps/";
        QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),QString::fromStdString(defaultPath),tr("Mega&Glest Map(*.mgm *.gbm)"));
        if(fileName != NULL){
            this->renderer->open(fileName.toStdString());
        }
    }

    void MainWindow::saveFile(){
        Config &config = Config::getInstance();
        string userData = config.getString("UserData_Root","");
        string defaultPath = userData + "maps/";
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"),QString::fromStdString(defaultPath),tr("MegaGlest Map(*.mgm);;Glest Map(*.gbm)"));
    }
}// end namespace


