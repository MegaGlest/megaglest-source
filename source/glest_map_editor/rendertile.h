#ifndef RENDERTILE_H
#define RENDERTILE_H

#include "renderer.h"
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
#include <QGraphicsItemGroup>
#include <QGraphicsSceneMouseEvent>

#include <iostream>
using namespace std;

class RenderTile:public QGraphicsItemGroup{
	public:
		RenderTile(QGraphicsScene *scene, Renderer *renderer,int column=0,int row=0);
		~RenderTile();
		void recalculate();
		double getHeight() const;
		void setHeight(double height);
		void setSurface(int surface);
		void update();
		void clicked(QGraphicsSceneMouseEvent * event);
	protected:
		virtual void mousePressEvent ( QGraphicsSceneMouseEvent * event);
		virtual void mouseMoveEvent ( QGraphicsSceneMouseEvent * event);
		virtual void mouseReleaseEvent ( QGraphicsSceneMouseEvent * event);
		virtual void hoverEnterEvent ( QGraphicsSceneHoverEvent * event );
		virtual void dragEnterEvent (QGraphicsSceneDragDropEvent * event);
	private:
		static const QColor SURFACE[];
		static const QColor OBJECT[];
		static const int SIZE;
		Renderer* renderer;
		void move(int column, int row);
		QGraphicsRectItem *rect;
		QGraphicsRectItem *water;
		QGraphicsRectItem *object;
		QGraphicsLineItem *topLine;
		QGraphicsLineItem *rightLine;
		QGraphicsLineItem *bottomLine;
		QGraphicsLineItem *leftLine;
		double height;
		int column;
		int row;

		int children;
		QGraphicsItem ** child;
};

#endif
