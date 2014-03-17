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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#ifdef WIN32
    #include <winsock2.h>
    #include <winsock.h>
#endif

#include <QMainWindow>

class QGraphicsScene;
class QActionGroup;
class QAction;

namespace Ui {
    class MainWindow;
}

namespace MapEditor {
    class Renderer;
    class NewMap;

    class MainWindow : public QMainWindow{
        Q_OBJECT//for Qt, otherwise “slots” won’t work

        public:
            explicit MainWindow(QWidget *parent = 0);
            ~MainWindow();
            QAction *getPen() const;

        protected:
            void changeEvent(QEvent *e);

        private:
            Ui::MainWindow *ui;
            Renderer *renderer;
            NewMap *newmap;
            QGraphicsScene *scene;
            QActionGroup *radiusGroup;
            QActionGroup *penGroup;

        private slots:
            void openFile();
            void saveFile();
            void quickSave();
            void showPreview();
    };
}// end namespace


#endif
