#include "rendertile.h"
const QColor RenderTile::SURFACE[] ={QColor(0x00,0xA0,0x00),QColor(0x53,0x6E,0x0B),QColor(0x80,0x00,0x00),QColor(0x80,0x80,0x80),QColor(0x7C,0x46,0x45)};
const int RenderTile::SIZE = 6;
//const int RenderTile::SIZE = 16;

//column and row are the position in tiles of the tile 
RenderTile::RenderTile(QGraphicsScene *scene, Renderer *renderer, int column, int row){
	this->renderer=renderer;
	this->rect = scene->addRect( QRectF(0, 0, SIZE, SIZE) );//rect and lines are only moved together with setPos
	//borders are extra items for pseudo 3D effect, we rarely show all of them, they have the rectangle as parent
	this->leftLine = new QGraphicsLineItem(0,0,0,SIZE-1,this->rect);
	this->topLine = new QGraphicsLineItem(0,0,SIZE-1,0,this->rect);
	this->rightLine = new QGraphicsLineItem(SIZE-1,0,SIZE-1,SIZE-1,this->rect);
	this->bottomLine = new QGraphicsLineItem(0,SIZE-1,SIZE-1,SIZE-1,this->rect);

	//this->height = 10;//default; we need to store those for determining if border must be redrawn
	this->height = this->renderer->getMap()->getHeight(column, row);


	this->rect->setPen(QPen(Qt::NoPen));//no borders

	QPen white(QColor(0xFF, 0xFF, 0xFF, 0x42 ));
	QPen black(QColor(0x00, 0x00, 0x00, 0x42 ));

	this->leftLine->setPen(white);//white and 50% opacity
	this->topLine->setPen(white);
	this->rightLine->setPen(black);//black and 50% opacity
	this->bottomLine->setPen(black);

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
	this->rect->setPos(column*SIZE,row*SIZE);
	this->row = row;
	this->column = column;
}

void RenderTile::update(){
	this->height = this->renderer->getMap()->getHeight(column, row);
	//this->recalculate();
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
	QColor base = SURFACE[this->renderer->getMap()->getSurface(column, row) - 1];
	if(this->height >= 10){
		this->rect->setBrush(base.lighter(100+5*(this->height - 10)));//choose a color for the rect
	}else{
		this->rect->setBrush(base.darker(100+20*(10 - this->height)));//choose a color for the rect
	}
	//grass: 00A000;2nd grass: 536E0B; road: 800000; stone: 808080; ground: 7C4645;	//icon color
	//grass: 00A000;2nd grass: 536E0B; road: 800000; stone: 808080; ground: 7C4645; //base

	//get the heights of all surrounding tiles
	if(this->column > 0){
		int leftHeight = this->renderer->at(this->column - 1, this->row)->getHeight();
		this->leftLine->setVisible(this->height > leftHeight);
	}else{
		this->leftLine->setVisible(false);
	}
	
	if(this->row > 0){
		int topHeight = this->renderer->at(this->column, this->row - 1)->getHeight();
		this->topLine->setVisible(this->height > topHeight);
	}else{
		this->topLine->setVisible(false);
	}

	//cout << this->renderer->getWidth() << endl;
	
	if(this->column < this->renderer->getWidth() - 1){
		int rightHeight = this->renderer->at(this->column + 1, this->row)->getHeight();
		//cout << this->height << " > " << rightHeight << endl;
		this->rightLine->setVisible(this->height > rightHeight);
	}else{
		this->rightLine->setVisible(false);
	}
	
	if(this->row < this->renderer->getHeight() - 1){
		int bottomHeight = this->renderer->at(this->column, this->row + 1)->getHeight();
		//cout << this->height << " > " << bottomHeight << endl;
		this->bottomLine->setVisible(this->height > bottomHeight);
	}else{
		this->bottomLine->setVisible(false);
	}

}
