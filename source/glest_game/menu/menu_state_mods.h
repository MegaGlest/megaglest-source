// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2011 Mark Vejvoda
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef MENU_STATE_MODS_H_
#define MENU_STATE_MODS_H_

#include "main_menu.h"
#include "map_preview.h"
#include "miniftpclient.h"
#include <map>
#include <vector>
#include "miniftpclient.h"
#include "leak_dumper.h"

namespace Glest { namespace Game {

enum FTPMessageType {
    ftpmsg_None,
	ftpmsg_GetMap,
	ftpmsg_GetTileset,
	ftpmsg_GetTechtree,
	ftpmsg_Quit
};

typedef vector<GraphicButton*> UserButtons;
typedef vector<GraphicLabel*> GraphicLabels;

// ===============================
// 	class MenuStateMods
// ===============================

class MenuStateMods: public MenuState, public FTPClientCallbackInterface, public SimpleTaskCallbackInterface {
private:

	GraphicButton buttonReturn;
	GraphicLine lineReturn;

	GraphicMessageBox mainMessageBox;
	FTPMessageType mainMessageBoxState;

	int techInfoXPos;
	int mapInfoXPos;
	int tilesetInfoXPos;
	int labelWidth;
	int scrollListsYPos;

	GraphicButton buttonInstallTech;
	GraphicButton buttonRemoveTech;
	GraphicLabel keyTechScrollBarTitle1;
	GraphicLabel keyTechScrollBarTitle2;
	GraphicScrollBar keyTechScrollBar;
	UserButtons keyTechButtons;
	GraphicLabels labelsTech;

	GraphicButton buttonInstallTileset;
	GraphicButton buttonRemoveTileset;
	GraphicLabel keyTilesetScrollBarTitle1;
	GraphicScrollBar keyTilesetScrollBar;
	UserButtons keyTilesetButtons;

	GraphicButton buttonInstallMap;
	GraphicButton buttonRemoveMap;
	GraphicLabel keyMapScrollBarTitle1;
	GraphicLabel keyMapScrollBarTitle2;
	GraphicScrollBar keyMapScrollBar;
	UserButtons keyMapButtons;
	GraphicLabels labelsMap;

	int keyButtonsToRender;
	int keyButtonsYBase;
	int keyButtonsXBase;
	int keyButtonsLineHeight;
	int	keyButtonsHeight;
	int keyButtonsWidth;

	Console console;
	bool showFullConsole;

	string selectedTechName;
	std::vector<std::string> techListRemote;
	std::map<string, string> techCacheList;
	vector<string> techTreeFiles;
	vector<string> techTreeFilesUserData;

	string selectedTilesetName;
	std::vector<std::string> tilesetListRemote;
	std::map<string, string> tilesetCacheList;
	vector<string> tilesetFiles;
	vector<string> tilesetFilesUserData;

	string selectedMapName;
	std::vector<std::string> mapListRemote;
	std::map<string, string> mapCacheList;
	vector<string> mapFiles;
	vector<string> mapFilesUserData;

	FTPClientThread *ftpClientThread;
	std::map<string,pair<int,string> > fileFTPProgressList;

	SimpleTaskThread *modHttpServerThread;

	void getTechsLocalList();
	void refreshTechs();

	void getTilesetsLocalList();
	void refreshTilesets();

	void getMapsLocalList();
	void refreshMaps();

public:

	MenuStateMods(Program *program, MainMenu *mainMenu);
	~MenuStateMods();

	void mouseClick(int x, int y, MouseButton mouseButton);
	void mouseMove(int x, int y, const MouseState *mouseState);
	void render();
	void update();

    virtual void keyDown(char key);
    virtual void keyPress(char c);
    virtual void keyUp(char key);

    virtual void simpleTask(BaseThread *callingThread);

private:

    void cleanUp();
    MapInfo loadMapInfo(string file);
    void showMessageBox(const string &text, const string &header, bool toggle);
    void clearUserButtons();
    virtual void FTPClient_CallbackEvent(string itemName,
    		FTP_Client_CallbackType type, pair<FTP_Client_ResultType,string> result,void *userdata);
};

}}//end namespace

#endif /* MENU_STATE_MODS_H_ */
