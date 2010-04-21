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

#ifndef _GLEST_GAME_GRAPHCOMPONENT_H_
#define _GLEST_GAME_GRAPHCOMPONENT_H_

#include <string>
#include <vector>

#include "font.h"

using std::string;
using std::vector;

using Shared::Graphics::Font2D;

namespace Glest{ namespace Game{

// ===========================================================
// 	class GraphicComponent
//
//	OpenGL renderer GUI components
// ===========================================================

class GraphicComponent{
public:
	static const float animSpeed;
	static const float fadeSpeed;

protected:
    int x, y, w, h;
    string text;
	const Font2D *font; 
	bool enabled;
	bool editable;

	static float anim;
	static float fade;

public:
	GraphicComponent();
	virtual ~GraphicComponent(){}

    void init(int x, int y, int w, int h);

	int getX() const				{return x;}
	int getY() const				{return y;}
	int getW() const				{return w;}
	int getH() const				{return h;}
	const string &getText() const	{return text;}
	const Font2D *getFont() const	{return font;}
	bool getEnabled() const			{return enabled;}
	bool getEditable() const			{return editable;}

	void setX(int x)					{this->x= x;}
	void setY(int y)					{this->y= y;}
	void setText(const string &text)	{this->text= text;}
	void setFont(const Font2D *font)	{this->font= font;}
	void setEnabled(bool enabled)		{this->enabled= enabled;}
	void setEditable(bool editable)		{this->editable= editable;}

    virtual bool mouseMove(int x, int y);
    virtual bool mouseClick(int x, int y);

	static void update();
	static void resetFade();
	static float getAnim()	{return anim;}
	static float getFade()	{return fade;}
};

// ===========================================================
// 	class GraphicLabel  
// ===========================================================

class GraphicLabel: public GraphicComponent{
public:
	static const int defH;
	static const int defW;

private:
	bool centered;

public:
	void init(int x, int y, int w=defW, int h=defH, bool centered= false);

	bool getCentered() const	{return centered;}

	void setCentered(bool centered)	{this->centered= centered;}
};

// ===========================================================
// 	class GraphicButton  
// ===========================================================

class GraphicButton: public GraphicComponent{
public:
	static const int defH;
	static const int defW;
	
private:
	bool lighted;
		
public:
	void init(int x, int y, int w=defW, int h=defH);

	bool getLighted() const			{return lighted;}
	
	void setLighted(bool lighted)	{this->lighted= lighted;}
	virtual bool mouseMove(int x, int y);  
};

// ===========================================================
// 	class GraphicListBox  
// ===========================================================

class GraphicListBox: public GraphicComponent{
public:
	static const int defH;
	static const int defW;

private:
    GraphicButton graphButton1, graphButton2;
    vector<string> items;
    int selectedItemIndex;
	
public:       
    void init(int x, int y, int w=defW, int h=defH);
    
	int getItemCount() const				{return items.size();}
	int getSelectedItemIndex() const		{return selectedItemIndex;}
	string getSelectedItem() const			{return items[selectedItemIndex];}
	const GraphicButton *getButton1() const	{return &graphButton1;}
	const GraphicButton *getButton2() const	{return &graphButton2;}

    void pushBackItem(string item);
    void setItems(const vector<string> &items);
	void setSelectedItemIndex(int index);
    void setSelectedItem(string item);
    void setEditable(bool editable);
    
    virtual bool mouseMove(int x, int y);
    virtual bool mouseClick(int x, int y);
};

// ===========================================================
// 	class GraphicMessageBox  
// ===========================================================

class GraphicMessageBox: public GraphicComponent{
public:
	static const int defH;
	static const int defW;

private:
	GraphicButton button1;
	GraphicButton button2;
	int buttonCount;
	string header;

public:
	void init(const string &button1Str, const string &button2Str);
	void init(const string &button1Str);

	int getButtonCount() const				{return buttonCount;}
	const GraphicButton *getButton1() const	{return &button1;}
	const GraphicButton *getButton2() const	{return &button2;}
	string getHeader() const				{return header;}
	
	void setHeader(string header)			{this->header= header;}

    virtual bool mouseMove(int x, int y);
    virtual bool mouseClick(int x, int y);
    bool mouseClick(int x, int y, int &clickedButton);
};

}}//end namespace

#endif
