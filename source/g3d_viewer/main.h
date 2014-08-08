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

#ifndef _SHADER_G3DVIEWER_MAIN_H_
#define _SHADER_G3DVIEWER_MAIN_H_

#ifdef WIN32
    #include <winsock2.h>
#endif

#include <string>
#include <GL/glew.h>
//#include <wx/wx.h>
//#include <wx/glcanvas.h>
//#include <wx/clrpicker.h>
//#include <wx/colordlg.h>

#include "renderer.h"
#include "util.h"
#include "particle_type.h"
#include "unit_particle_type.h"


#include <QMainWindow>
#include <QActionGroup>
#include <QColor>
#include <QColorDialog>
#include <QFileDialog>
namespace Shared{ namespace G3dViewer{
class GLWidget;
}}
using std::string;
using namespace Glest::Game;

    namespace Ui {
        class MainWindow;
    }

    class MainWindow : public QMainWindow
    {
        Q_OBJECT

    public:
        explicit MainWindow(QWidget *parent = 0);
        ~MainWindow();
        int showRuntimeError(const std::string text, const Shared::Platform::megaglest_runtime_error &ex);

    protected:
        void changeEvent(QEvent *e);

    private:
        Ui::MainWindow *ui;
        QActionGroup *playerGroup;
        Shared::G3dViewer::GLWidget *glWidget;


    private slots:
        void colorChooser();
        void openXMLFile();
        void openG3DFile();
    };

#endif
