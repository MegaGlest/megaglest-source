// ==============================================================
//  This file is part of MegaGlest (www.megaglest.org)
//
//  Copyright (C) 2014 Sebastian Riedel
//
//  You can redistribute this code and/or modify it under
//  the terms of the GNU General Public License as published
//  by the Free Software Foundation; either version 2 of the
//  License, or (at your option) any later version
// ==============================================================

#ifndef VIEWER_HPP
#define VIEWER_HPP

#include <QGraphicsView>

namespace MapEditor {
    struct Status;
    class Viewer : public QGraphicsView{
        Q_OBJECT
        public:
            explicit Viewer(QWidget *parent = 0);
            ~Viewer();
            void setStatus(Status *status);
        protected:
            virtual void mouseMoveEvent(QMouseEvent *event);
            //virtual void mousePressEvent(QMouseEvent *event);
            virtual void keyPressEvent(QKeyEvent *event);
            virtual void keyReleaseEvent(QKeyEvent *event);
            virtual void LeaveEvent(QEvent *event);
            void modeChecker(Qt::KeyboardModifiers mod);
            virtual void wheelEvent ( QWheelEvent * event );
        private:
            Status *status;
    };
}

#endif // VIEWE_HPP
