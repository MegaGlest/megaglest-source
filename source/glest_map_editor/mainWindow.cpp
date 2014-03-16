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
#include <QFileDialog>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QActionGroup>
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

        renderer = new Renderer();
        newmap = new NewMap(renderer);//instance of new map dialog

        connect(ui->actionNew, SIGNAL(triggered()), newmap, SLOT(show()));//new map dialog window
        connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(openFile()));//open dialog window
        connect(ui->actionSave_as, SIGNAL(triggered()), this, SLOT(saveFile()));//save as dialog window

        //those actions form a selection group -> only one is selected
        gradientGroup = new QActionGroup(this);
        gradientGroup->addAction(ui->actionGradient_5);
        gradientGroup->addAction(ui->actionGradient_4);
        gradientGroup->addAction(ui->actionGradient_3);
        gradientGroup->addAction(ui->actionGradient_2);
        gradientGroup->addAction(ui->actionGradient_1);
        gradientGroup->addAction(ui->actionGradient0);
        gradientGroup->addAction(ui->actionGradient1);
        gradientGroup->addAction(ui->actionGradient2);
        gradientGroup->addAction(ui->actionGradient3);
        gradientGroup->addAction(ui->actionGradient4);
        gradientGroup->addAction(ui->actionGradient5);

        //one event for the whole group, they do all the same, but with different gradients
        //connect(gradientGroup,SIGNAL(triggered(QAction*)),????,SLOT(set???(QAction*)));

        heightGroup = new QActionGroup(this);
        heightGroup->addAction(ui->actionHeight_5);
        heightGroup->addAction(ui->actionHeight_4);
        heightGroup->addAction(ui->actionHeight_3);
        heightGroup->addAction(ui->actionHeight_2);
        heightGroup->addAction(ui->actionHeight_1);
        heightGroup->addAction(ui->actionHeight0);
        heightGroup->addAction(ui->actionHeight1);
        heightGroup->addAction(ui->actionHeight2);
        heightGroup->addAction(ui->actionHeight3);
        heightGroup->addAction(ui->actionHeight4);
        heightGroup->addAction(ui->actionHeight5);

        radiusGroup = new QActionGroup(this);
        radiusGroup->addAction(ui->action1_diameter);
        radiusGroup->addAction(ui->action2_diameter);
        radiusGroup->addAction(ui->action3_diameter);
        radiusGroup->addAction(ui->action4_diameter);
        radiusGroup->addAction(ui->action5_diameter);
        radiusGroup->addAction(ui->action6_diameter);
        radiusGroup->addAction(ui->action7_diameter);
        radiusGroup->addAction(ui->action8_diameter);
        radiusGroup->addAction(ui->action9_diameter);
        connect(radiusGroup,SIGNAL(triggered(QAction*)),this->renderer,SLOT(setRadius(QAction*)));

        surfaceGroup = new QActionGroup(this);
        surfaceGroup->addAction(ui->actionGrass);
        surfaceGroup->addAction(ui->actionSecondary_Grass);
        surfaceGroup->addAction(ui->actionRoad);
        surfaceGroup->addAction(ui->actionStone);
        surfaceGroup->addAction(ui->actionGround);

        resourceGroup = new QActionGroup(this);
        resourceGroup->addAction(ui->actionGold_unwalkable);
        resourceGroup->addAction(ui->actionStone_unwalkable);
        resourceGroup->addAction(ui->action3_custom);
        resourceGroup->addAction(ui->action4_custom);
        resourceGroup->addAction(ui->action5_custom);

        objectGroup = new QActionGroup(this);
        objectGroup->addAction(ui->actionNone_erase);
        objectGroup->addAction(ui->actionTree_harvestable);
        objectGroup->addAction(ui->actionDead_tree_Cactuses_Hornbush);
        objectGroup->addAction(ui->actionStone_object);
        objectGroup->addAction(ui->actionBush_Grass_walkable);
        objectGroup->addAction(ui->actionWater_object_Papyrus);
        objectGroup->addAction(ui->actionBig_tree_old_palm);
        objectGroup->addAction(ui->actionHanged_Impaled);
        objectGroup->addAction(ui->actionStatues);
        objectGroup->addAction(ui->actionMountain);
        objectGroup->addAction(ui->actionInvisible_Blocking_Object);

        playerGroup = new QActionGroup(this);
        playerGroup->addAction(ui->actionPlayer_1);
        playerGroup->addAction(ui->actionPlayer_2);
        playerGroup->addAction(ui->actionPlayer_3);
        playerGroup->addAction(ui->actionPlayer_4);
        playerGroup->addAction(ui->actionPlayer_5);
        playerGroup->addAction(ui->actionPlayer_6);
        playerGroup->addAction(ui->actionPlayer_7);
        playerGroup->addAction(ui->actionPlayer_8);

        ui->graphicsView->setScene(renderer->getScene());
        //ui->graphicsView->scale(5,5);
    }

    MainWindow::~MainWindow(){
        delete ui;
        delete newmap;
        delete gradientGroup;
        delete heightGroup;
        delete radiusGroup;
        delete surfaceGroup;
        delete resourceGroup;
        delete objectGroup;
        delete playerGroup;
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

    //TODO: move methods like this one to another class
    void MainWindow::setRadius(){
        std::cout << "test" << std::endl;
        this->renderer->setRadius(this->radiusGroup->checkedAction());
    }
}// end namespace


