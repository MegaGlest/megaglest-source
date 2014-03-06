#ifndef RENDERTILE_H
#define RENDERTILE_H

#include "renderer.h"
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>

#include <iostream>
using namespace std;

class RenderTile{
	public:
		RenderTile(QGraphicsScene *scene, Renderer *renderer,int column=0,int row=0);
		~RenderTile();
		void recalculate();
		int getHeight() const;
		void setHeight(int height);
		void update();
	private:
		static const QColor SURFACE[];
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
		int height;
		int column;
		int row;
};

#endif
