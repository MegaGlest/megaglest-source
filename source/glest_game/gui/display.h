// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_DISPLAY_H_
#define _GLEST_GAME_DISPLAY_H_

#include <string>

#include "texture.h"
#include "util.h"
#include "command_type.h"
#include "game_util.h"
#include "leak_dumper.h"

using std::string;

using Shared::Graphics::Texture2D;
using Shared::Graphics::Vec4f;
using Shared::Util::replaceBy;

namespace Glest{ namespace Game{

// =====================================================
// 	class Display
//
///	Display for unit commands, and unit selection
// =====================================================

class Display{
public:
	static const int cellSideCount= 4;
	static const int upCellCount= cellSideCount*cellSideCount;
	static const int downCellCount= cellSideCount*cellSideCount;
	static const int colorCount= 9;
	static const int imageSize= 32;
	static const int invalidPos= -1;
	static const int downY = imageSize*9;
	static const int infoStringY= imageSize*4;

private:
	string title;
	string text;
	string infoText;
	const Texture2D *upImages[upCellCount];
	const Texture2D *downImages[downCellCount];
	bool downLighted[downCellCount];
	const CommandType *commandTypes[downCellCount];
	CommandClass commandClasses[downCellCount];
	int progressBar;
	int downSelectedPos;
	Vec4f colors[colorCount];
	int currentColor;
	

public:
	Display();

	//get
	string getTitle() const							{return title;}
	string getText() const							{return text;}
	string getInfoText() const						{return infoText;}
	const Texture2D *getUpImage(int index) const	{return upImages[index];}
	const Texture2D *getDownImage(int index) const	{return downImages[index];}
	bool getDownLighted(int index) const			{return downLighted[index];}
	const CommandType *getCommandType(int i)		{return commandTypes[i];}
	CommandClass getCommandClass(int i)				{return commandClasses[i];}
	Vec4f getColor() const;
	int getProgressBar() const						{return progressBar;}
	int getDownSelectedPos() const					{return downSelectedPos;}
	
	//set
	void setTitle(const string title)					{this->title= formatString(title);}
	void setText(const string &text)					{this->text= formatString(text);}
	void setInfoText(const string infoText)				{this->infoText= formatString(infoText);}
	void setUpImage(int i, const Texture2D *image) 		{upImages[i]= image;}
	void setDownImage(int i, const Texture2D *image)	{downImages[i]= image;}
	void setCommandType(int i, const CommandType *ct)	{commandTypes[i]= ct;}
	void setCommandClass(int i, const CommandClass cc)	{commandClasses[i]= cc;}
	void setDownLighted(int i, bool lighted)			{downLighted[i]= lighted;}
	void setProgressBar(int i)							{progressBar= i;}
	void setDownSelectedPos(int i)						{downSelectedPos= i;}
	
	//misc
	void clear();
	void switchColor();
	int computeDownIndex(int x, int y);
	int computeDownX(int index) const;
	int computeDownY(int index) const;
	int computeUpX(int index) const;
	int computeUpY(int index) const;
};

}}//end namespace 

#endif
