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
#include "mainWindow.hpp"

#include "ui_mainWindow.h"
#include "ui_help.h"
#include "ui_about.h"
#include "ui_info.h"
#include "ui_advanced.h"

#include "newMap.hpp"
//#include "advanced.hpp"
#include "switchSurfaces.hpp"
#include "renderer.hpp"
#include "mapManipulator.hpp"
#include <QFileDialog>
#include <QMessageBox>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QActionGroup>
#include <QProcess>
#include <QCoreApplication>
#include <QLabel>
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

        Status *status = new Status();
        status->file = new QLabel(tr("File: %1").arg("desert_river"));
        ui->statusBar->addWidget(status->file);
        status->type = new QLabel(".gbm");
        ui->statusBar->addWidget(status->type);
        status->pos = new QLabel(tr("Position: 0000, 0000","for determining maximum content size"));
        status->pos->setFixedWidth(status->pos->sizeHint().width());
        status->pos->setText(QObject::tr("Position: (none)"));
        ui->statusBar->addWidget(status->pos);
        status->ingame = new QLabel(tr("Ingame: 0000, 0000","for determining maximum content size"));
        status->ingame->setFixedWidth(status->ingame->sizeHint().width());
        status->ingame->setText(QObject::tr("Position: (none)"));
        ui->statusBar->addWidget(status->ingame);
        status->object = new QLabel(tr("Object: %1").arg("None (Erase)"));
        status->object->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Fixed);
        ui->statusBar->addWidget(status->object);

        MapManipulator *mapman = new MapManipulator(this);
        renderer = new Renderer(mapman, status);
        ui->graphicsView->setStatus(status);
        newmap = new NewMap(mapman,this);//instance of new map dialog
        resize = new NewMap(mapman,this);//instance of new map dialog
        resetPlayers = new NewMap(mapman,this);//instance of new map dialog
        //info = new Info(mapman,this);//instance of new map dialog
        switchSurfaces = new SwitchSurfaces(mapman,this);//instance of new map dialog
        //advanced = new Advanced(mapman,this);//instance of new map dialog

        help = new QDialog(this);//no need for a whole new class
        {
            Ui::Help *ui = new Ui::Help;
            ui->setupUi(help);
        }

        about = new QDialog(this);//no need for a whole new class
        {
            Ui::About *ui = new Ui::About;
            ui->setupUi(about);
        }

        //file
        connect(ui->actionNew, SIGNAL( triggered() ), newmap, SLOT( show() ));//new map dialog window
        connect(ui->actionOpen, SIGNAL( triggered() ), this, SLOT( openFile() ));//open dialog window
        connect(ui->actionSave, SIGNAL( triggered() ), this, SLOT( quickSave() ));//save as dialog window
        connect(ui->actionSave_as, SIGNAL( triggered() ), this, SLOT( saveFile() ));//save as dialog window
        connect(ui->actionPreview, SIGNAL( triggered() ), this, SLOT( showPreview() ));//show a preview with megaglest game
        //edit
        connect(ui->actionUndo, SIGNAL( triggered() ), renderer, SLOT( undo() ));
        connect(ui->actionRedo, SIGNAL( triggered() ), renderer, SLOT( redo() ));
        connect(ui->actionCopy, SIGNAL( triggered() ), mapman, SLOT( copy() ));
        connect(ui->actionPaste, SIGNAL( triggered() ), mapman, SLOT( paste() ));
        connect(ui->actionResize, SIGNAL( triggered() ), resize, SLOT( show() ));
        connect(ui->actionReset_Players, SIGNAL( triggered() ), resetPlayers, SLOT( show() ));
        connect(ui->actionFlip_Diagonal, SIGNAL( triggered() ), mapman, SLOT( flipDiagonal() ));
        connect(ui->actionFlip_X, SIGNAL( triggered() ), mapman, SLOT( flipX() ));
        connect(ui->actionFlip_Y, SIGNAL( triggered() ), mapman, SLOT( flipY() ));
        connect(ui->actionCopy_left_to_right, SIGNAL( triggered() ), mapman, SLOT( copyL2R() ));
        connect(ui->actionCopy_1, SIGNAL( triggered() ), mapman, SLOT( copyT2B() ));
        connect(ui->actionCopy_2, SIGNAL( triggered() ), mapman, SLOT( copyBL2TR() ));
        connect(ui->actionRotate_Left_to_Right, SIGNAL( triggered() ), mapman, SLOT( rotateL2R() ));
        connect(ui->actionTop_to_Bottom, SIGNAL( triggered() ), mapman, SLOT( rotateT2B() ));
        connect(ui->actionBottom_Left_to_Top_Right, SIGNAL( triggered() ), mapman, SLOT( rotateBL2TR() ));
        connect(ui->actionRotate_Top_Left_to_Bottom_Right, SIGNAL( triggered() ), mapman, SLOT( rotateTL2BR() ));
        connect(ui->actionRandomize_Heights, SIGNAL( triggered() ), mapman, SLOT( randomizeHeights() ));
        connect(ui->actionRandomize_Heights_Players, SIGNAL( triggered() ), mapman, SLOT( randomizeHeightsAndPlayers() ));
        connect(ui->actionInfo, SIGNAL( triggered() ), this, SLOT( infoDialog() ));
        connect(ui->actionSwitch_Surfaces, SIGNAL( triggered() ), switchSurfaces, SLOT( show() ));
        connect(ui->actionAdvanced, SIGNAL( triggered() ), this, SLOT( advancedDialog() ));
        connect(ui->actionSelect_all, SIGNAL( triggered() ), mapman, SLOT( setSelection() ));
        connect(ui->actionSquare_selection, SIGNAL( toggled(bool) ), mapman, SLOT( enableSelctionsquare(bool) ));
        //view
        connect(ui->actionZoom_in, SIGNAL( triggered() ), renderer, SLOT( zoomIn() ));
        connect(ui->actionZoom_out, SIGNAL( triggered() ), renderer, SLOT( zoomOut() ));
        connect(ui->actionReset_zoom, SIGNAL( triggered() ), renderer, SLOT( fitZoom() ));
        connect(ui->actionGrid, SIGNAL( toggled(bool) ), renderer, SLOT( setGrid(bool) ));
        connect(ui->actionHeight_Map, SIGNAL( toggled(bool) ), renderer, SLOT( setHeightMap(bool) ));
        connect(ui->actionWater, SIGNAL( toggled(bool) ), renderer, SLOT( setWater(bool) ));
        connect(ui->actionHelp, SIGNAL( triggered() ), help, SLOT( show() ));
        connect(ui->actionAbout, SIGNAL( triggered() ), about, SLOT( show() ));

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

        /*mouseGroup = new QActionGroup(this);
        mouseGroup->addAction(ui->actionDraw);
        mouseGroup->addAction(ui->actionMove);
        mouseGroup->addAction(ui->actionSelect);*/

        connect(ui->actionSelect,SIGNAL(toggled(bool)),this,SLOT(mouseBehavior(bool)));

        ui->graphicsView->setScene(renderer->getScene());
        //ui->graphicsView->scale(5,5);

        /*ui->statusBar->setStyleSheet(
        "QStatusBar::item { border: 1px solid red; border-radius: 3px; border-style:inset;} "
        );*/
        this->show();
        this->renderer->fitZoom();
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

    QGraphicsView *MainWindow::getView() const{
        return ui->graphicsView;
    }

    void MainWindow::limitPlayers(int limit){
        QList<QAction *>actions = ui->menuPlayer->actions();

        for (int i = 0; i < actions.size(); i++) {
            actions[i]->setEnabled(i < limit);
        }
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
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), QString::fromStdString(defaultPath),tr("MegaGlest Map(*.mgm);;Glest Map(*.gbm)"),0,QFileDialog::DontUseNativeDialog);//does not support auto-suffix on gnu/linux

        bool reallySave = true;
        if(fileName != NULL){
            if(!fileName.endsWith(".mgm") && !fileName.endsWith(".gbm")){//gnu/linux has no auto-suffixes
                fileName += ".mgm";
                if(QFile(fileName).exists()){
                    QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Save File"), tr("This file already exists.\nDo you want to replace it?"),QMessageBox::Yes|QMessageBox::No);
                    reallySave = (reply == QMessageBox::Yes);
                }
            }

            if(reallySave){
                this->renderer->save(fileName.toStdString());
            }else{
                this->saveFile();
            }
        }
    }

    void MainWindow::quickSave(){
        if(this->renderer->isSavable()){
            this->renderer->save();
        }else{
            this->saveFile();
        }
    }

    //TODO: generate on the fly scenario for testing --load-scenario=x
    void MainWindow::showPreview(){
        Config &config = Config::getInstance();
        string userData = config.getString("UserData_Root","");
        string defaultPath = userData + "maps/";
        //this way we don’t need to save before previewing
        string mapName = "temporary_editor_preview";
        string fileName = defaultPath+mapName+".mgm";
        this->renderer->getMap()->saveToFile(fileName);
        QString path = QCoreApplication::applicationDirPath().append("/megaglest");
#ifdef WIN32
        path.append(".exe");
#endif
        std::cout << "megaglest path: " << path.toStdString() << std::endl;
        QProcess::execute(path,QStringList(QString::fromStdString("--preview-map="+mapName))/* */);//*/<<"--data-path=/usr/share/megaglest/"<<"--ini-path=/usr/share/megaglest");
        QFile(QString::fromStdString(fileName)).remove();//don’t need that anymore
    }

    void MainWindow::mouseBehavior(bool checked){
        /*if(action->objectName().contains("Draw")){
            std::cout << "draw" << std::endl;
            ui->graphicsView->setDragMode(QGraphicsView::NoDrag);
        }else if(action->objectName().contains("Move")){
            std::cout << "draw" << std::endl;
            ui->graphicsView->setDragMode(QGraphicsView::ScrollHandDrag);
        }else if(action->objectName().contains("Select")){
            std::cout << "draw" << std::endl;
            ui->graphicsView->setDragMode(QGraphicsView::RubberBandDrag);
        }*/
        if(checked){
            ui->graphicsView->setDragMode(QGraphicsView::RubberBandDrag);
        }else{
            ui->graphicsView->setDragMode(QGraphicsView::ScrollHandDrag);
        }
    }

    void MainWindow::infoDialog(){
        QString title = QString(this->renderer->getMap()->getTitle().c_str());
        QString description = QString(this->renderer->getMap()->getDesc().c_str());
        QString author = QString(this->renderer->getMap()->getAuthor().c_str());

        QDialog info(this);
        Ui::Info ui;
        ui.setupUi(&info);
        //set values
        ui.inputTitle->setText(title);
        ui.inputDescription->setText(description);
        ui.inputAuthor->setText(author);

        if(info.exec()){
            //get new values
            title = ui.inputTitle->text();
            description = ui.inputDescription->text();
            author = ui.inputAuthor->text();
            this->renderer->getMapManipulator()->setInfo(title.toStdString(), description.toStdString(), author.toStdString());
        }
    }

    void MainWindow::advancedDialog(){
        int height = this->renderer->getMap()->getHeightFactor();
        int water = this->renderer->getMap()->getWaterLevel();
        int cliff = this->renderer->getMap()->getCliffLevel();
        int camera = this->renderer->getMap()->getCameraHeight();

        QDialog advanced(this);
        Ui::Advanced ui;
        ui.setupUi(&advanced);
        //set values
        ui.inputHeightFactor->setValue(height);
        ui.inputWaterLevel->setValue(water);
        ui.inputCliffLevel->setValue(cliff);
        ui.inputCameraHeight->setValue(camera);

        if(advanced.exec()){
            //get new values
            height = ui.inputHeightFactor->value();
            water = ui.inputWaterLevel->value();
            cliff = ui.inputCliffLevel->value();
            camera = ui.inputCameraHeight->value();
            this->renderer->getMapManipulator()->setAdvanced(height, water, cliff, camera);
        }
    }
}// end namespace


