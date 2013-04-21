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

#ifndef _GLEST_GAME_MENUSTATEOPTIONS_NETWORK_H_
#define _GLEST_GAME_MENUSTATEOPTIONS_NETWORK_H_

#include "main_menu.h"
#include "leak_dumper.h"

namespace Glest{ namespace Game{

// ===============================
// 	class MenuStateOptionsNetwork
// ===============================

class MenuStateOptionsNetwork: public MenuState{
private:

	GraphicButton buttonOk;
	GraphicButton buttonReturn;

	GraphicButton buttonKeyboardSetup; // configure the keyboard
	GraphicButton buttonVideoSection;
	GraphicButton buttonAudioSection;
	GraphicButton buttonMiscSection;
	GraphicButton buttonNetworkSettings;


	GraphicMessageBox mainMessageBox;
	int mainMessageBoxState;

	GraphicLabel labelExternalPort;
	GraphicLabel labelServerPortLabel;

	GraphicLabel labelPublishServerExternalPort;
	GraphicListBox listBoxServerPort;

	GraphicLabel labelEnableFTP;
	GraphicCheckBox checkBoxEnableFTP;

	GraphicLabel labelEnableFTPServer;
	GraphicCheckBox checkBoxEnableFTPServer;

	GraphicLabel labelFTPServerPortLabel;
	GraphicLabel labelFTPServerPort;

	GraphicLabel labelFTPServerDataPortsLabel;
	GraphicLabel labelFTPServerDataPorts;

	GraphicLabel labelEnableFTPServerInternetTilesetXfer;
	GraphicCheckBox checkBoxEnableFTPServerInternetTilesetXfer;

	GraphicLabel labelEnableFTPServerInternetTechtreeXfer;
	GraphicCheckBox checkBoxEnableFTPServerInternetTechtreeXfer;

	GraphicLabel labelEnablePrivacy;
	GraphicCheckBox checkBoxEnablePrivacy;

	ProgramState **parentUI;

public:
	MenuStateOptionsNetwork(Program *program, MainMenu *mainMenu, ProgramState **parentUI=NULL);

	void mouseClick(int x, int y, MouseButton mouseButton);
	void mouseMove(int x, int y, const MouseState *mouseState);
	void render();
	//virtual void keyDown(SDL_KeyboardEvent key);
    virtual void keyPress(SDL_KeyboardEvent c);
    //virtual bool isInSpecialKeyCaptureEvent();

    virtual void reloadUI();


private:
	void saveConfig();
	void setActiveInputLable(GraphicLabel* newLable);
	void showMessageBox(const string &text, const string &header, bool toggle);
};

}}//end namespace

#endif
