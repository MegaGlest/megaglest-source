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

#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#ifdef WIN32
    #include <winsock2.h>
#endif

#include <QMainWindow>

class QGraphicsScene;
class QGraphicsView;
class QActionGroup;
class QAction;
class QLabel;

namespace Ui {
    class MainWindow;
}

namespace MapEditor {
    class Renderer;
    class NewMap;
    class Info;
    class Advanced;
    class SwitchSurfaces;

    struct Status{
        QLabel *file;
        QLabel *type;
        QLabel *object;
        QLabel *pos;
        QLabel *ingame;
    };

    class MainWindow : public QMainWindow{
        Q_OBJECT//for Qt, otherwise “slots” won’t work

        public:
            explicit MainWindow(QWidget *parent = 0);
            ~MainWindow();
            QAction *getPen() const;
            QGraphicsView *getView() const;
            void limitPlayers(int limit);

        protected:
            void changeEvent(QEvent *e);

        private:
            Ui::MainWindow *ui;
            Renderer *renderer;
            NewMap *newmap;
            NewMap *resize;
            NewMap *resetPlayers;
            SwitchSurfaces *switchSurfaces;
            //Advanced *advanced;
            //Info *info;
            QDialog *help;
            QDialog *about;
            QGraphicsScene *scene;
            QActionGroup *radiusGroup;
            QActionGroup *penGroup;
            //QActionGroup *mouseGroup;

        private slots:
            void openFile();
            void saveFile();
            void quickSave();
            void showPreview();
            void mouseBehavior(bool checked);
            void infoDialog();
            void advancedDialog();
            virtual void closeEvent ( QCloseEvent * event );
    };
}// end namespace


#endif
