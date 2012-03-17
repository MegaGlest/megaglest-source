// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_MENUSTATELOADGAME_H_
#define _GLEST_GAME_MENUSTATELOADGAME_H_

#include "main_menu.h"
#include "leak_dumper.h"

namespace Glest{ namespace Game{

// ===============================
// 	class MenuStateLoadGame
// ===============================
//typedef vector<GraphicButton*> SaveSlotButtons;
class MenuStateLoadGame: public MenuState{
private:
	GraphicButton loadButton;
	GraphicButton deleteButton;
	GraphicButton abortButton;
	vector<GraphicButton*> slots;
	vector<GraphicComponent*> slotsGB;
	vector<string> filenames;
	GraphicScrollBar slotsScrollBar;
	GraphicButton* selectedButton;

	GraphicButton* buttonToDelete;

	Texture2D *previewTexture;
	Texture2D *needsToBeFreedTexture;

	GraphicLabel headerLabel;
	GraphicLabel noSavedGamesLabel;
	GraphicLabel savedGamesLabel;
	GraphicLabel infoHeaderLabel;
	GraphicLabel infoTextLabel;

	GraphicLine lines[2];

	string saveGameDir;
	int slotLinesYBase;
	int slotsLineHeight;

public:
	MenuStateLoadGame(Program *program, MainMenu *mainMenu);
	~MenuStateLoadGame();

	void mouseClick(int x, int y, MouseButton mouseButton);
	void mouseMove(int x, int y, const MouseState *mouseState);
	void update();
	void render();
	virtual void keyDown(SDL_KeyboardEvent key);

	void reloadUI();
private:
	void clearSlots();
	void deleteSlot(int i);
	void listFiles(int keyButtonsXBase, int keyButtonsYBase, int keyButtonsWidth, int keyButtonsHeight);
};


}}//end namespace

#endif
