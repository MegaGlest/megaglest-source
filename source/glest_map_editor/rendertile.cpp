#include "rendertile.h"

//const int RenderTile::size = 16;

//column and row are the position in tiles of the tile 
RenderTile::RenderTile(QGraphicsScene *scene, Renderer *renderer, int column, int row):size(8){
	this->renderer=renderer;
	this->rect = scene->addRect( QRectF(0, 0, size, size) );//rect and lines are only moved together with setPos
	//borders are extra items for pseudo 3D effect, we rarely show all of them, they have the rectangle as parent
	this->leftLine = new QGraphicsLineItem(0,0,0,size-1,this->rect);
	this->topLine = new QGraphicsLineItem(0,0,size-1,0,this->rect);
	this->rightLine = new QGraphicsLineItem(size-1,0,size-1,size-1,this->rect);
	this->bottomLine = new QGraphicsLineItem(0,size-1,size-1,size-1,this->rect);

	//this->height = 10;//default; we need to store those for determining if border must be redrawn
	this->height = this->renderer->getMap()->getHeight(column, row);


	this->rect->setPen(QPen(Qt::NoPen));//no borders

	this->leftLine->setPen(QPen(QColor(0xFF, 0xFF, 0xFF, 0x37 )));//white and 50% opacity
	this->topLine->setPen(QPen(QColor(0xFF, 0xFF, 0xFF, 0x37 )));
	this->rightLine->setPen(QPen(QColor(0x00, 0x00, 0x00, 0x37 )));//black and 50% opacity
	this->bottomLine->setPen(QPen(QColor(0x00, 0x00, 0x00, 0x37 )));

	this->move(column, row);
	//this->recalculate();
	
	this->leftLine->setVisible(false);
	this->topLine->setVisible(false);
	this->rightLine->setVisible(false);
	this->bottomLine->setVisible(false);
}

RenderTile::~RenderTile(){
	this->rect->scene()->removeItem(this->rect);
	delete this->leftLine;
	delete this->topLine;
	delete this->rightLine;
	delete this->bottomLine;
	delete this->rect;
}

void RenderTile::move(int column, int row){
	this->rect->setPos(column*size,row*size);
	this->row = row;
	this->column = column;
}

void RenderTile::update(){
	this->height = this->renderer->getMap()->getHeight(column, row);
	this->recalculate();
}

void RenderTile::setHeight(int height){
	bool changed = (this->height != height);
	this->height = height;
	if(changed){
		this->recalculate();
		if(this->column > 0){
			this->renderer->at(this->column - 1, this->row)->recalculate();
		}
		if(this->row > 0){
			this->renderer->at(this->column, this->row - 1)->recalculate();
		}
		if(this->column < this->renderer->getWidth() - 1){
			this->renderer->at(this->column + 1, this->row)->recalculate();
		}
		if(this->row < this->renderer->getHeight() - 1){
			this->renderer->at(this->column, this->row + 1)->recalculate();
		}
	}
}

int RenderTile::getHeight() const{
	return this->height;
}

void RenderTile::recalculate(){
	this->rect->setBrush(QColor(0, 0x5F + (this->height * 8), 0));//choose a color for the rect

	//get the heights of all surrounding tiles
	if(this->column > 0){
		int leftHeight = this->renderer->at(this->column - 1, this->row)->getHeight();
		this->leftLine->setVisible(this->height > leftHeight);
	}
	if(this->row > 0){
		int topHeight = this->renderer->at(this->column, this->row - 1)->getHeight();
		this->topLine->setVisible(this->height > topHeight);
	}
	if(this->column < this->renderer->getWidth() - 1){
		int rightHeight = this->renderer->at(this->column + 1, this->row)->getHeight();
		this->rightLine->setVisible(this->height > rightHeight);
	}
	if(this->row < this->renderer->getHeight() - 1){
		int bottomHeight = this->renderer->at(this->column, this->row + 1)->getHeight();
		this->bottomLine->setVisible(this->height > bottomHeight);
	}

}
