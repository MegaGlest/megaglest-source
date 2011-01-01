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

#ifndef _GLEST_GAME_CHATMANAGER_H_
#define _GLEST_GAME_CHATMANAGER_H_

#include <string>
#include "font.h"
#include "leak_dumper.h"

using std::string;
using Shared::Graphics::Font2D;

namespace Glest{ namespace Game{

class Console;

// =====================================================
//	class ChatManager
// =====================================================

class ChatManager{

private:
	bool editEnabled;
	bool teamMode;
	bool disableTeamMode;
	Console* console;
	string text;
	int thisTeamIndex;
	bool inMenu;
	string manualPlayerNameOverride;
	int xPos;
	int yPos;
	int maxTextLenght;
	Font2D *font;
	

public:
	ChatManager();
	void init(Console* console, int thisTeamIndex, const bool inMenu=false, string manualPlayerNameOverride="");

	void keyDown(char key);
	void keyUp(char key);
	void keyPress(char c);
	void updateNetwork();

	bool getEditEnabled() const	{return editEnabled;}
	bool getTeamMode() const	{return teamMode;}
	bool getInMenu() const	{return inMenu;}
	string getText() const		{return text;}
	int getXPos() const {return xPos;}
	void setXPos(int xPos)	{this->xPos= xPos;}
	int getYPos() const {return yPos;}
	void setYPos(int yPos)	{this->yPos= yPos;}
	int getMaxTextLenght() const {return maxTextLenght;}
	void setMaxTextLenght(int maxTextLenght)	{this->maxTextLenght= maxTextLenght;}
	Font2D *getFont() const {return font;}
	void setFont(Font2D *font)	{this->font= font;}
	void addText(string text);
	void switchOnEdit();
	

	bool getDisableTeamMode() const { return disableTeamMode; }
	void setDisableTeamMode(bool value);
};

}}//end namespace

#endif
