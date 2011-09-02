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
	ftpmsg_GetScenario,

	ftpmsg_ReplaceMap,
	ftpmsg_ReplaceTileset,
	ftpmsg_ReplaceTechtree,
	ftpmsg_ReplaceScenario,

	ftpmsg_Quit
};

enum ModType {
    mt_None,
    mt_Map,
    mt_Tileset,
    mt_Techtree,
    mt_Scenario
};

typedef vector<GraphicButton*> UserButtons;
typedef vector<GraphicLabel*> GraphicLabels;

// ===============================
// 	class ModInfo
// ===============================

class ModInfo {
public:
	string name;
	string url;
	string imageUrl;
	string description;
	string count; // used for faction count for example
	string crc;
	string localCRC;
	ModType type;
public:
	ModInfo();
};

// ===============================
// 	class MenuStateMods
// ===============================
class MenuStateMods: public MenuState, public FTPClientCallbackInterface, public SimpleTaskCallbackInterface {
private:

	GraphicButton buttonReturn;
	GraphicLine lineHorizontal;
	GraphicLine lineVertical;
	GraphicLine lineReturn;
	GraphicLine lineVerticalReturn;

	GraphicMessageBox mainMessageBox;
	FTPMessageType mainMessageBoxState;

	int techInfoXPos;
	int mapInfoXPos;
	int tilesetInfoXPos;
	int scenarioInfoXPos;
	int labelWidth;
	int scrollListsYPos;
	int listBoxLength;

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

	GraphicButton buttonInstallScenario;
	GraphicButton buttonRemoveScenario;
	GraphicLabel keyScenarioScrollBarTitle1;
	GraphicLabel keyScenarioScrollBarTitle2;
	GraphicScrollBar keyScenarioScrollBar;
	UserButtons keyScenarioButtons;

	GraphicLabel modDescrLabel;
	Texture2D *modPreviewImage;
	ModInfo modInfoSelected;
	map<string,bool> displayModPreviewImage;

	GraphicButton buttonInstalled;
	GraphicButton buttonAvailable;
	GraphicButton buttonConflict;
	GraphicButton buttonOnlyLocal;

	int keyButtonsToRender;
	int keyButtonsYBase;
	int keyButtonsXBase;
	int keyButtonsLineHeight;
	int	keyButtonsHeight;
	int keyButtonsWidth;

	//Console console;
	bool showFullConsole;

	string selectedTechName;
	std::vector<std::string> techListRemote;
	std::map<string, ModInfo> techCacheList;
	vector<string> techTreeFiles;
	vector<string> techTreeFilesUserData;

	string selectedTilesetName;
	std::vector<std::string> tilesetListRemote;
	std::map<string, ModInfo> tilesetCacheList;
	vector<string> tilesetFiles;
	vector<string> tilesetFilesUserData;

	string selectedMapName;
	std::vector<std::string> mapListRemote;
	std::map<string, ModInfo> mapCacheList;
	vector<string> mapFiles;
	vector<string> mapFilesUserData;

	string selectedScenarioName;
	std::vector<std::string> scenarioListRemote;
	std::map<string, ModInfo> scenarioCacheList;
	vector<string> scenarioFiles;
	vector<string> scenarioFilesUserData;

	FTPClientThread *ftpClientThread;
	std::map<string,pair<int,string> > fileFTPProgressList;

	SimpleTaskThread *modHttpServerThread;

	void getTechsLocalList();
	string refreshTechModInfo(string techInfo);
	void refreshTechs();

	void getTilesetsLocalList();
	string refreshTilesetModInfo(string tilesetInfo);
	void refreshTilesets();

	void getMapsLocalList();
	void refreshMaps();

	void getScenariosLocalList();
	void refreshScenarios();

	void showDesription(const ModInfo *modInfo);

	std::map<string,int32> mapCRCUpdateList;
public:

	MenuStateMods(Program *program, MainMenu *mainMenu);
	virtual ~MenuStateMods();

	void mouseClick(int x, int y, MouseButton mouseButton);
	void mouseMove(int x, int y, const MouseState *mouseState);
	void render();
	void update();

    virtual void keyDown(SDL_KeyboardEvent key);
    virtual void keyPress(SDL_KeyboardEvent c);
    virtual void keyUp(SDL_KeyboardEvent key);

    virtual void simpleTask(BaseThread *callingThread);

private:

    void cleanUp();
    MapInfo loadMapInfo(string file);
    void showMessageBox(const string &text, const string &header, bool toggle);
    void clearUserButtons();
    virtual void FTPClient_CallbackEvent(string itemName,
    		FTP_Client_CallbackType type, pair<FTP_Client_ResultType,string> result,void *userdata);

    string getPreviewImageFileForMod(const ModInfo *modInfo);
    void cleanupPreviewTexture();
};

}}//end namespace

#endif /* MENU_STATE_MODS_H_ */
