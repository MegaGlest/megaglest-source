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
#include <map>
#include <typeinfo>
#include "font.h"
#include "texture.h"
#include "leak_dumper.h"
#include "vec.h"

using std::string;
using std::vector;

using Shared::Graphics::Font2D;
using Shared::Graphics::Font3D;
using namespace Shared::Graphics;
using Shared::Graphics::Vec3f;

namespace Glest{ namespace Game{

class GraphicComponent;

// ===========================================================
// 	class GraphicComponent
//
//	OpenGL renderer GUI components
// ===========================================================

class GraphicComponent {
public:
	static const float animSpeed;
	static const float fadeSpeed;

	static std::map<std::string, std::map<std::string, GraphicComponent *> > registeredGraphicComponentList;

protected:
    int x, y, w, h;
    string text;
	Font2D *font;
	Font3D *font3D;
	bool enabled;
	bool editable;
	bool visible;

	static float anim;
	static float fade;

	string instanceName;

public:
	GraphicComponent(std::string containerName="", std::string objName="");
	virtual ~GraphicComponent(){}

	static void clearRegisteredComponents(std::string containerName="");
	static void clearRegisterGraphicComponent(std::string containerName, std::string objName);
	static void clearRegisterGraphicComponent(std::string containerName, std::vector<std::string> objNameList);
	virtual void registerGraphicComponent(std::string containerName, std::string objName);
	static  GraphicComponent * findRegisteredComponent(std::string containerName, std::string objName);
	static void applyAllCustomProperties(std::string containerName);
	virtual void applyCustomProperties(std::string containerName);

	static bool saveAllCustomProperties(std::string containerName);
	virtual bool saveCustomProperties(std::string containerName);

	virtual void init(int x, int y, int w, int h);

	string getInstanceName() const { return instanceName; }
	void setInstanceName(string value) { instanceName = value; }

    virtual int getX() const				{return x;}
	virtual int getY() const				{return y;}
	virtual int getW() const				{return w;}
	virtual int getH() const				{return h;}
	virtual const string &getText() const	{return text;}
	virtual Font2D *getFont() 				{return font;}
	virtual Font3D *getFont3D() 			{return font3D;}
	virtual bool getEnabled() const			{return enabled;}
	virtual bool getEditable() const		{return editable;}
	virtual bool getVisible() const			{return visible;}

	virtual void setX(int x)					{this->x= x;}
	virtual void setY(int y)					{this->y= y;}
	virtual void setW(int w)					{this->w= w;}
	virtual void setH(int h)					{this->h= h;}
	virtual void setText(const string &text)	{this->text= text;}
	virtual void setFont(Font2D *font)			{this->font= font;}
	virtual void setFont3D(Font3D *font)		{this->font3D= font;}
	virtual void setEnabled(bool enabled)		{this->enabled= enabled;}
	virtual void setEditable(bool editable)		{this->editable= editable;}
	virtual void setVisible(bool value)			{this->visible = value;}

	virtual void reloadFonts();
	static void reloadFontsForRegisterGraphicComponents(std::string containerName);

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

class GraphicLabel: public GraphicComponent {
public:
	static const int defH;
	static const int defW;

private:
	bool centered;
	Vec3f textColor;
	bool wordWrap;

	int centeredW;
	int centeredH;

public:
	GraphicLabel();
	void init(int x, int y, int w=defW, int h=defH, bool centered= false, Vec3f textColor=Vec3f(1.f, 1.f, 1.f), bool wordWrap=false);

	bool getCentered() const	{return centered;}
	void setCentered(bool centered)	{this->centered= centered;}

	bool getCenteredW() const;
	void setCenteredW(bool centered);

	bool getCenteredH() const;
	void setCenteredH(bool centered);

	Vec3f getTextColor() const	{return textColor;}
	void setTextColor(Vec3f color)	{this->textColor= color;}

	bool getWordWrap() const { return wordWrap; }
	void setWordWrap(bool value) { wordWrap = value; }

};

// ===========================================================
// 	class GraphicButton  
// ===========================================================

class GraphicButton: public GraphicComponent {
public:
	static const int defH;
	static const int defW;
	
private:
	bool lighted;

	bool useCustomTexture;
	Texture *customTexture;
		
public:
	GraphicButton(std::string containerName="", std::string objName="");
	void init(int x, int y, int w=defW, int h=defH);

	bool getUseCustomTexture() const { return useCustomTexture; }
	Texture *getCustomTexture() const { return customTexture; }

	void setUseCustomTexture(bool value) { useCustomTexture=value; }
	void setCustomTexture(Texture *value) { customTexture=value; }

	bool getLighted() const			{return lighted;}
	
	void setLighted(bool lighted)	{this->lighted= lighted;}
	virtual bool mouseMove(int x, int y);  
};

// ===========================================================
// 	class GraphicListBox  
// ===========================================================

class GraphicListBox: public GraphicComponent {
public:
	static const int defH;
	static const int defW;

private:
    GraphicButton graphButton1, graphButton2;
    vector<string> items;
    int selectedItemIndex;
    bool lighted;
	Vec3f textColor;
	
public:
	GraphicListBox(std::string containerName="", std::string objName="");
    void init(int x, int y, int w=defW, int h=defH, Vec3f textColor=Vec3f(1.f, 1.f, 1.f));
    
	int getItemCount() const				{return items.size();}
	string getItem(int index) const			{return items[index];}
	int getSelectedItemIndex() const		{return selectedItemIndex;}
	string getSelectedItem() const			{return items[selectedItemIndex];}
	GraphicButton *getButton1() 			{return &graphButton1;}
	GraphicButton *getButton2() 			{return &graphButton2;}
	bool getLighted() const					{return lighted;}
    void setLighted(bool lighted)			{this->lighted= lighted;}
	Vec3f getTextColor() const				{return textColor;}
    void setTextColor(Vec3f color)			{this->textColor= color;}

    void pushBackItem(string item);
    void setItems(const vector<string> &items);
	void setSelectedItemIndex(int index, bool errorOnMissing=true);
    void setSelectedItem(string item, bool errorOnMissing=true);
    void setEditable(bool editable);

    bool hasItem(string item) const;

	virtual void setY(int y);
    
    virtual bool mouseMove(int x, int y);
    virtual bool mouseClick(int x, int y);
};

// ===========================================================
// 	class GraphicMessageBox  
// ===========================================================

class GraphicMessageBox: public GraphicComponent {
public:
	static const int defH;
	static const int defW;

private:
	GraphicButton button1;
	GraphicButton button2;
	int buttonCount;
	string header;

public:
	GraphicMessageBox(std::string containerName="", std::string objName="");
	void init(const string &button1Str, const string &button2Str, int newWidth=-1,int newHeight=-1);
	void init(const string &button1Str, int newWidth=-1,int newHeight=-1);

	int getButtonCount() const				{return buttonCount;}
	GraphicButton *getButton1() 			{return &button1;}
	GraphicButton *getButton2() 			{return &button2;}
	string getHeader() const				{return header;}

	virtual void setX(int x);
	virtual void setY(int y);
	
	void setHeader(string header)			{this->header= header;}

    virtual bool mouseMove(int x, int y);
    virtual bool mouseClick(int x, int y);
    bool mouseClick(int x, int y, int &clickedButton);
};

// ===========================================================
// 	class GraphicLine  
// ===========================================================

class GraphicLine: public GraphicComponent {
public:
	static const int defH;
	static const int defW;
	
private:
	bool horizontal;	
	
public:
	GraphicLine(std::string containerName="", std::string objName="");
	void init(int x, int y, int w=defW, int h=defH);
	bool getHorizontal() const		{return horizontal;}
	void setHorizontal(bool horizontal) 		{this->horizontal= horizontal;}
};

// ===========================================================
// 	class GraphicCheckBox  
// ===========================================================

class GraphicCheckBox: public GraphicComponent {
public:
	static const int defH;
	static const int defW;
	
private:
	bool value;
	bool lighted;

public:
	GraphicCheckBox(std::string containerName="", std::string objName="");
	void init(int x, int y, int w=defW, int h=defH);
	bool getValue() const		{return value;}
	void setValue(bool value) 		{this->value= value;}
	bool getLighted() const			{return lighted;}
	void setLighted(bool lighted)	{this->lighted= lighted;}
	virtual bool mouseMove(int x, int y);  
    virtual bool mouseClick(int x, int y);
};

// ===========================================================
// 	class GraphicScrollBar
// ===========================================================

class GraphicScrollBar: public GraphicComponent {
public:
	static const int defLength;
	static const int defThickness;

private:
	bool lighted;
	bool horizontal;
	int elementCount;
	int visibleSize;
	int visibleStart;

	// position on component for renderer
	int visibleCompPosStart;
	int visibleCompPosEnd;

public:
	GraphicScrollBar(std::string containerName="", std::string objName="");
	void init(int x, int y, bool horizontal,int length=defLength, int thickness=defThickness);
	virtual bool mouseDown(int x, int y);
	virtual bool mouseMove(int x, int y);
	virtual bool mouseClick(int x, int y);


	bool getHorizontal() const		{return horizontal;}
	int getLength() const;
	void setLength(int length)	{horizontal?setW(length):setH(length);}
	int getThickness() const;


	bool getLighted() const			{return lighted;}
	void setLighted(bool lighted)	{this->lighted= lighted;}

	int getElementCount() const		{return elementCount;}
	void setElementCount(int elementCount);
	int getVisibleSize() const		{return visibleSize;}
	void setVisibleSize(int visibleSize);
	int getVisibleStart() const		{return visibleStart;}
	int getVisibleEnd() const		{return visibleStart+visibleSize>elementCount-1?elementCount-1: visibleStart+visibleSize-1;}
	void setVisibleStart(int visibleStart);

	int getVisibleCompPosStart() const		{return visibleCompPosStart;}
	int getVisibleCompPosEnd() const		{return visibleCompPosEnd;}
};

// ===========================================================
// 	class PopupMenu
// ===========================================================

class PopupMenu: public GraphicComponent {
public:
	static const int defH;
	static const int defW;

private:
	std::vector<GraphicButton> buttons;
	string header;

public:
	PopupMenu();
	~PopupMenu();
	void init(string menuHeader, std::vector<string> menuItems);

	std::vector<GraphicButton> & getMenuItems() {return buttons;}
	string getHeader() const				{return header;}

	virtual void setX(int x);
	virtual void setY(int y);

	void setHeader(string header)			{this->header= header;}

    virtual bool mouseMove(int x, int y);
    virtual bool mouseClick(int x, int y);
    std::pair<int,string> mouseClickedMenuItem(int x, int y);
};

}}//end namespace
#endif
