// ==============================================================
// This file is part of MegaGlest (www.glest.org)
//
// Copyright (C) 2011 - by Mark Vejvoda <mark_vejvoda@hotmail.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
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

#include "main.h"
#include "ui_main.h"
//namespace MapEditor {
	MainWindow::MainWindow(QWidget *parent) :
		QMainWindow(parent),
		ui(new Ui::MainWindow)
	{
		ui->setupUi(this);
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

//initialize and open the window
int main(int argc, char *argv[]){
	if(argc==2){
		if(argv[1][0]=='-') { // any flag gives help and exits program.
			cout << "MegaGlest G3D Viewer TODO :D" << endl << endl;
			cout << "does stuff" << endl << endl;
			cout << endl;
			exit (0);
		}
	}
	
	QApplication a(argc, argv);
	MainWindow w;
	w.show();

	
	return a.exec();
}
