// ==============================================================
// This file is part of MegaGlest (www.glest.org)
//
// Copyright (C) 2011 - by Mark Vejvoda <mark_vejvoda@hotmail.com>
//
//  You can redistribute this code and/or modify it under
//  the terms of the GNU General Public License as published
//  by the Free Software Foundation; either version 2 of the
//  License, or (at your option) any later version
// ==============================================================

#include "main.h"

#include <stdexcept>

#include "model_gl.h"
#include "graphics_interface.h"
#include "util.h"
#include "conversion.h"
#include "platform_common.h"
#include "xml_parser.h"
#include <iostream>
//#include <wx/event.h>
#include "config.h"
#include "game_constants.h"
//#include <wx/stdpaths.h>
#include <platform_util.h>
//#include "interpolation.h"

#ifndef WIN32
#include <errno.h>
#endif

//#include <wx/filename.h>

#ifndef WIN32
  #define stricmp strcasecmp
  #define strnicmp strncasecmp
  #define _strnicmp strncasecmp
#endif

using namespace Shared::Platform;
using namespace Shared::PlatformCommon;
using namespace Shared::Graphics;
using namespace Shared::Graphics::Gl;
using namespace Shared::Util;
using namespace Shared::Xml;

using namespace std;
using namespace Glest::Game;

#ifdef _WIN32
const char *folderDelimiter = "\\";
#else
const char *folderDelimiter = "/";
#endif

//int GameConstants::updateFps= 40;
//int GameConstants::cameraFps= 100;

const string g3dviewerVersionString= "v1.3.6";

// Because g3d should always support alpha transparency
string fileFormat = "png";

namespace Glest { namespace Game {

string getGameReadWritePath(string lookupKey) {
    string path = "";
    if(path == "" && getenv("GLESTHOME") != NULL) {
        path = safeCharPtrCopy(getenv("GLESTHOME"),8096);
        if(path != "" && EndsWith(path, "/") == false && EndsWith(path, "\\") == false) {
            path += "/";
        }

        //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] path to be used for read/write files [%s]\n",__FILE__,__FUNCTION__,__LINE__,path.c_str());
    }

    return path;
}
}}

#include "ui_main.h"
//#include <QGLWidget>
#include "moc_glwidget.cpp"
//otherwise it will be included in the automoc.cpp
#include <QMessageBox>
//namespace MapEditor {
    MainWindow::MainWindow(QWidget *parent) :
        QMainWindow(parent),
        ui(new Ui::MainWindow)
    {
        ui->setupUi(this);

        QGLFormat glFormat;
        glFormat.setVersion( 1, 3 );
        glFormat.setProfile( QGLFormat::NoProfile ); // Requires >=Qt-4.8.0
        //~ glFormat.setVersion( 3, 3 );
        //~ glFormat.setProfile( QGLFormat::CompatibilityProfile ); // Requires >=Qt-4.8.0
        //~ //glFormat.setProfile( QGLFormat::CoreProfile ); // Requires >=Qt-4.8.0
        glFormat.setSampleBuffers( true );
        glWidget = new Shared::G3dViewer::GLWidget(glFormat,this);
        //glWidget = new GLWidget(this);
        ui->verticalLayout->addWidget(glWidget);


        connect(ui->actionChange_Background_Color,SIGNAL(triggered()),this,SLOT(colorChooser()));
        connect(ui->actionLoad_G3D_Model,SIGNAL(triggered()),this,SLOT(openG3DFile()));
        connect(ui->actionLoad_Particle_XML,SIGNAL(triggered()),this,SLOT(openXMLFile()));
        connect(ui->actionLoad_Projectile_Particle_XML,SIGNAL(triggered()),this,SLOT(openXMLFile()));
        connect(ui->actionLoad_Splash_Particle_XML,SIGNAL(triggered()),this,SLOT(openXMLFile()));

        playerGroup = new QActionGroup(this);
        playerGroup->addAction(ui->actionPlayer_1);
        playerGroup->addAction(ui->actionPlayer_2);
        playerGroup->addAction(ui->actionPlayer_3);
        playerGroup->addAction(ui->actionPlayer_4);
        playerGroup->addAction(ui->actionPlayer_5);
        playerGroup->addAction(ui->actionPlayer_6);
        playerGroup->addAction(ui->actionPlayer_7);
        playerGroup->addAction(ui->actionPlayer_8);

    }

    MainWindow::~MainWindow()
    {
        delete ui;
    }

    int MainWindow::showRuntimeError(const std::string text, const Shared::Platform::megaglest_runtime_error &ex) {
        std::cerr << ex.what() << std::endl;
        QMessageBox messageBox( QMessageBox::Critical, tr("A runtime error occured!"), tr("A runtime error occured!"), QMessageBox::Ok, this);
        messageBox.setInformativeText(tr(text.c_str()));
        messageBox.setDetailedText(ex.what());
        return messageBox.exec();
    }

        //for translation ... somehow
    void MainWindow::changeEvent(QEvent *e)
    {
        QMainWindow::changeEvent(e);
        switch (e->type()) {
        case QEvent::LanguageChange:
            ui->retranslateUi(this);
            break;
        default:
            break;
        }
    }

    void MainWindow::colorChooser(){
        QColor chosen = QColorDialog::getColor(QColor(0x00,0x00,0x00));
    }

    void MainWindow::openG3DFile(){
        Config &config = Config::getInstance();
        string userData = config.getString("UserData_Root","");
        string defaultPath = userData + "techs/";
        QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),QString::fromStdString(defaultPath),tr("G3D file (*.g3d)"));
        /*if(fileName != NULL){
            this->renderer->open(fileName.toStdString());
        }*/
        if(fileName != NULL){
            this->glWidget->loadModel(fileName);
        }else throw "no file";
    }

    void MainWindow::openXMLFile(){
        Config &config = Config::getInstance();
        string userData = config.getString("UserData_Root","");
        string defaultPath = userData + "techs/";
        QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),QString::fromStdString(defaultPath),tr("XML file (*.xml)"));
    }

//initialize and open the window
int main(int argc, char *argv[]){
    string version = "1.4.0";

    QApplication a(argc, argv);
    QStringList args = a.arguments();

    if(args.contains("--help") || args.contains("-h") || args.contains("-H") || args.contains("-?") || args.contains("?") || args.contains("-help") ){
        cout << "MegaGlest G3D Viewer v" << version << endl << endl;
        cout << "does stuff" << endl << endl;
        cout << endl;
        exit (0);
    }
    if(args.contains("--version")){
        cout << version << endl;

        cout << endl;
        exit (0);
    }
    MainWindow w;
    w.show();


    return a.exec();
}
