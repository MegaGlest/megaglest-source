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

#ifndef _MAPEDITOR_MAIN_H_
#define _MAPEDITOR_MAIN_H_

#ifdef WIN32
    #include <winsock2.h>
    #include <winsock.h>
#endif

#include <string>
#include <vector>

#include "util.h"
#include "platform_common.h"

using std::string;
using std::vector;
using std::pair;
using namespace Shared::PlatformCommon;






#include "newmap.h"
#include "renderer.h"
#include <QMainWindow>
#include <QFileDialog>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
//namespace MapEditor {
	namespace Ui {
		class MainWindow;
	}

	class MainWindow : public QMainWindow
	{
		Q_OBJECT

	public:
		explicit MainWindow(QWidget *parent = 0);
		~MainWindow();

	protected:
		void changeEvent(QEvent *e);

	private:
		Ui::MainWindow *ui;
		Renderer *renderer;
		NewMap *newmap;
		QGraphicsScene *scene; 

	private slots:
		void openFile();
		void saveFile();
	};
//}// end namespace


#endif
