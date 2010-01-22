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

#include "display.h"

#include "metrics.h"
#include "command_type.h"
#include "leak_dumper.h"

using namespace Shared::Graphics;

namespace Glest{ namespace Game{

// =====================================================
// 	class Display
// =====================================================

Display::Display(){
	clear();
}

//misc
void Display::clear(){
	for(int i=0; i<upCellCount; ++i){
		upImages[i]= NULL;
	}

	for(int i=0; i<downCellCount; ++i){
		downImages[i]= NULL;
		downLighted[i]= true;
		commandTypes[i]= NULL;
		commandClasses[i]= ccNull;
	}

	downSelectedPos= invalidPos;
	title.clear();
	text.clear();
	progressBar= -1;
}

int Display::computeDownIndex(int x, int y){
	y= y-(downY-cellSideCount*imageSize);
	
	if(y>imageSize*cellSideCount){
		return invalidPos;
	}

	int cellX= x/imageSize;
	int cellY= (y/imageSize) % cellSideCount;
	int index= (cellSideCount-cellY-1)*cellSideCount+cellX;;

	if(index<0 || index>=downCellCount || downImages[index]==NULL){
		index= invalidPos;
	}

	return index;
}

int Display::computeDownX(int index) const{
	return (index % cellSideCount) * imageSize;
}

int Display::computeDownY(int index) const{
	return Display::downY - (index/cellSideCount)*imageSize - imageSize;
}

int Display::computeUpX(int index) const{
	return (index % cellSideCount) * imageSize;
}

int Display::computeUpY(int index) const{
	return Metrics::getInstance().getDisplayH() - (index/cellSideCount)*imageSize - imageSize;
}

}}//end namespace
