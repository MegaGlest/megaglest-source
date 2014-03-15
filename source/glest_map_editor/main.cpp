// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "main.h"
#include <ctime>
#include "conversion.h"
#include "platform_common.h"
#include "config.h"
#include <iostream>
#include "platform_util.h"
#ifndef WIN32
#include <errno.h>
#endif
#include <memory>

using namespace Shared::Util;
using namespace Shared::PlatformCommon;
using namespace Glest::Game;
using namespace std;

namespace Glest { namespace Game {//need that somehow for default path detection
string getGameReadWritePath(string lookupKey) {
	string path = "";
	if(path == "" && getenv("GLESTHOME") != NULL) {
		path = getenv("GLESTHOME");
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
		
		renderer = new Renderer();
		
		newmap = new NewMap(renderer);//instance of new map dialog
		connect(ui->actionNew, SIGNAL(triggered()), newmap, SLOT(show()));//new map dialog window
		
		connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(openFile()));//open dialog window
		connect(ui->actionSave_as, SIGNAL(triggered()), this, SLOT(saveFile()));//save as dialog window


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
		//ui->graphicsView->show();
	}

	MainWindow::~MainWindow()
	{
		delete ui;
		delete newmap;
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
//}// end namespace


//initialize and open the window
int main(int argc, char *argv[]){
	//string name = "MegaGlest Editor";
	string version = "1.7.0";
	
	QApplication a(argc, argv);
	QStringList args = a.arguments();

	if(args.contains("--help") || args.contains("-h") || args.contains("-H") || args.contains("-?") || args.contains("?") || args.contains("-help") ){
		cout << "MegaGlest map editor v" << version << endl << endl;
		cout << "glest_map_editor [GBM OR MGM FILE]" << endl << endl;
		cout << "Creates or edits glest/megaglest maps." << endl;
		cout << "Draw with left mouse button (select what and how large area in menu or toolbar)" << endl;
		cout << "Pan trough the map with right mouse button" << endl;
		cout << "Zoom with middle mouse button or mousewheel" << endl;

		// std::cout << " ~ more helps should be written here ~" << std::endl;
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
