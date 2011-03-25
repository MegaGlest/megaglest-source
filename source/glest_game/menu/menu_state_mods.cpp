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

#include "menu_state_mods.h"

#include "renderer.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "config.h"
#include "menu_state_root.h"
#include "metrics.h"
#include "conversion.h"
#include <algorithm>
#include <curl/curl.h>
#include "cache_manager.h"
#include "leak_dumper.h"


namespace Glest{ namespace Game{

using namespace Shared::Util;

struct FormatString {
	void operator()(string &s) {
		s = formatString(s);
	}
};

// =====================================================
// 	class MenuStateConnectedGame
// =====================================================

MenuStateMods::MenuStateMods(Program *program, MainMenu *mainMenu) :
	MenuState(program, mainMenu, "mods") {

	containerName = "Mods";
	Lang &lang= Lang::getInstance();
	Config &config = Config::getInstance();

	ftpClientThread 		= NULL;
	selectedTechName		= "";
	selectedTilesetName		= "";
	selectedMapName 		= "";
	showFullConsole			= false;
	keyButtonsLineHeight	= 20;
	keyButtonsHeight		= 20;
	keyButtonsWidth			= 200;
	scrollListsYPos 	= 700;
	//keyButtonsYBase			= scrollListsYPos - keyButtonsLineHeight;
	keyButtonsYBase			= scrollListsYPos;
	keyButtonsToRender		= 400 / keyButtonsLineHeight;
	labelWidth			= 5;

	//create
	techInfoXPos = 10;
	keyTechScrollBarTitle1.registerGraphicComponent(containerName,"keyTechScrollBarTitle1");
	keyTechScrollBarTitle1.init(techInfoXPos,scrollListsYPos + 20,labelWidth,20);
	keyTechScrollBarTitle1.setText(lang.get("TechTitle1"));
	keyTechScrollBarTitle1.setFont(CoreData::getInstance().getMenuFontBig());
	keyTechScrollBarTitle2.registerGraphicComponent(containerName,"keyTechScrollBarTitle2");
	keyTechScrollBarTitle2.init(techInfoXPos + 200,scrollListsYPos + 20,labelWidth,20);
	keyTechScrollBarTitle2.setText(lang.get("TechTitle2"));
	keyTechScrollBarTitle2.setFont(CoreData::getInstance().getMenuFontBig());

	mapInfoXPos = 270;
	keyMapScrollBarTitle1.registerGraphicComponent(containerName,"keyMapScrollBarTitle1");
	keyMapScrollBarTitle1.init(mapInfoXPos,scrollListsYPos + 20,labelWidth,20);
	keyMapScrollBarTitle1.setText(lang.get("MapTitle1"));
	keyMapScrollBarTitle1.setFont(CoreData::getInstance().getMenuFontBig());
	keyMapScrollBarTitle2.registerGraphicComponent(containerName,"keyMapScrollBarTitle2");
	keyMapScrollBarTitle2.init(mapInfoXPos + 200,scrollListsYPos + 20,labelWidth,20);
	keyMapScrollBarTitle2.setText(lang.get("MapTitle2"));
	keyMapScrollBarTitle2.setFont(CoreData::getInstance().getMenuFontBig());

	tilesetInfoXPos = 530;
	keyTilesetScrollBarTitle1.registerGraphicComponent(containerName,"keyTilesetScrollBarTitle1");
	keyTilesetScrollBarTitle1.init(tilesetInfoXPos,scrollListsYPos + 20,labelWidth,20);
	keyTilesetScrollBarTitle1.setText(lang.get("TilesetTitle1"));
	keyTilesetScrollBarTitle1.setFont(CoreData::getInstance().getMenuFontBig());

	mainMessageBoxState = ftpmsg_None;
    mainMessageBox.registerGraphicComponent(containerName,"mainMessageBox");
	mainMessageBox.init(lang.get("Yes"),lang.get("No"));
	mainMessageBox.setEnabled(false);

	buttonReturn.registerGraphicComponent(containerName,"buttonReturn");
	buttonReturn.init(450, 140, 125);
	buttonReturn.setText(lang.get("Return"));

	int installButtonYPos = 280;
	buttonInstallTech.registerGraphicComponent(containerName,"buttonInstallTech");
	buttonInstallTech.init(techInfoXPos + 40, installButtonYPos, 125);
	buttonInstallTech.setText(lang.get("Install"));
	buttonRemoveTech.registerGraphicComponent(containerName,"buttonRemoveTech");
	buttonRemoveTech.init(techInfoXPos + 40, installButtonYPos-30, 125);
	buttonRemoveTech.setText(lang.get("Remove"));

	buttonInstallTileset.registerGraphicComponent(containerName,"buttonInstallTileset");
	buttonInstallTileset.init(tilesetInfoXPos + 20, installButtonYPos, 125);
	buttonInstallTileset.setText(lang.get("Install"));
	buttonRemoveTileset.registerGraphicComponent(containerName,"buttonRemoveTileset");
	buttonRemoveTileset.init(tilesetInfoXPos + 20, installButtonYPos-30, 125);
	buttonRemoveTileset.setText(lang.get("Remove"));

	buttonInstallMap.registerGraphicComponent(containerName,"buttonInstallMap");
	buttonInstallMap.init(mapInfoXPos + 40, installButtonYPos, 125);
	buttonInstallMap.setText(lang.get("Install"));
	buttonRemoveMap.registerGraphicComponent(containerName,"buttonRemoveMap");
	buttonRemoveMap.init(mapInfoXPos + 40, installButtonYPos-30, 125);
	buttonRemoveMap.setText(lang.get("Remove"));

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

//	std::string techsMetaData = "";
//	std::string tilesetsMetaData = "";
//	std::string mapsMetaData = "";
//	if(Config::getInstance().getString("Masterserver","") != "") {
//		string baseURL = Config::getInstance().getString("Masterserver");
//
//		CURL *handle = SystemFlags::initHTTP();
//		techsMetaData = SystemFlags::getHTTP(baseURL + "showTechsForGlest.php",handle);
//		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("techsMetaData [%s]\n",techsMetaData.c_str());
//		tilesetsMetaData = SystemFlags::getHTTP(baseURL + "showTilesetsForGlest.php",handle);
//		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("tilesetsMetaData [%s]\n",tilesetsMetaData.c_str());
//		mapsMetaData = SystemFlags::getHTTP(baseURL + "showMapsForGlest.php",handle);
//		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("mapsMetaData [%s]\n",mapsMetaData.c_str());
//		SystemFlags::cleanupHTTP(&handle);
//	}
//
//	tilesetListRemote.clear();
//	Tokenize(tilesetsMetaData,tilesetListRemote,"\n");
//
//	getTilesetsLocalList();
//	for(unsigned int i=0; i < tilesetListRemote.size(); i++) {
//		string tilesetInfo = tilesetListRemote[i];
//		std::vector<std::string> tilesetInfoList;
//		Tokenize(tilesetInfo,tilesetInfoList,"|");
//
//		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("tilesetInfoList.size() [%d]\n",(int)tilesetInfoList.size());
//		if(tilesetInfoList.size() >= 4) {
//			string tilesetName = tilesetInfoList[0];
//			string tilesetCRC = tilesetInfoList[1];
//			string tilesetDescription = tilesetInfoList[2];
//			string tilesetURL = tilesetInfoList[3];
//			//bool alreadyHasTileset = (std::find(tilesetFiles.begin(),tilesetFiles.end(),tilesetName) != tilesetFiles.end());
//			tilesetCacheList[tilesetName] = tilesetURL;
//
//			GraphicButton *button=new GraphicButton();
//			button->init(tilesetInfoXPos, keyButtonsYBase, keyButtonsWidth,keyButtonsHeight);
//			button->setText(tilesetName);
//			button->setUseCustomTexture(true);
//			button->setCustomTexture(CoreData::getInstance().getCustomTexture());
//
//			//if(alreadyHasTileset == true) {
//			//	button->setEnabled(false);
//			//}
//			keyTilesetButtons.push_back(button);
//		}
//	}
//	for(unsigned int i=0; i < tilesetFilesUserData.size(); i++) {
//		string tilesetName = tilesetFilesUserData[i];
//		bool alreadyHasTileset = (tilesetCacheList.find(tilesetName) != tilesetCacheList.end());
//		if(alreadyHasTileset == false) {
//			GraphicButton *button=new GraphicButton();
//			button->init(tilesetInfoXPos, keyButtonsYBase, keyButtonsWidth,keyButtonsHeight);
//			button->setText(tilesetName);
//			button->setUseCustomTexture(true);
//			button->setCustomTexture(CoreData::getInstance().getCustomTexture());
//			keyTilesetButtons.push_back(button);
//		}
//	}
//
//	techListRemote.clear();
//	Tokenize(techsMetaData,techListRemote,"\n");
//
//	getTechsLocalList();
//	for(unsigned int i=0; i < techListRemote.size(); i++) {
//		string techInfo = techListRemote[i];
//		std::vector<std::string> techInfoList;
//		Tokenize(techInfo,techInfoList,"|");
//
//		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("techInfoList.size() [%d]\n",(int)techInfoList.size());
//		if(techInfoList.size() >= 5) {
//			string techName = techInfoList[0];
//			string techFactionCount = techInfoList[1];
//			string techCRC = techInfoList[2];
//			string techDescription = techInfoList[3];
//			string techURL = techInfoList[4];
//			//bool alreadyHasTech = (std::find(techTreeFiles.begin(),techTreeFiles.end(),techName) != techTreeFiles.end());
//			techCacheList[techName] = techURL;
//
//			GraphicButton *button=new GraphicButton();
//			button->init(techInfoXPos, keyButtonsYBase, keyButtonsWidth,keyButtonsHeight);
//			button->setText(techName);
//			button->setUseCustomTexture(true);
//			button->setCustomTexture(CoreData::getInstance().getCustomTexture());
//
//			//if(alreadyHasTech == true) {
//			//	button->setEnabled(false);
//			//}
//			keyTechButtons.push_back(button);
//			GraphicLabel *label=new GraphicLabel();
//			label->init(techInfoXPos + keyButtonsWidth+10,keyButtonsYBase,labelWidth,20);
//			label->setText(techFactionCount);
//			labelsTech.push_back(label);
//		}
//	}
//	for(unsigned int i=0; i < techTreeFilesUserData.size(); i++) {
//		string techName = techTreeFilesUserData[i];
//		bool alreadyHasTech = (techCacheList.find(techName) != techCacheList.end());
//		if(alreadyHasTech == false) {
//			vector<string> techPaths = config.getPathListForType(ptTechs);
//	        string &techPath = techPaths[1];
//	        endPathWithSlash(techPath);
//	        vector<string> factions;
//	        findAll(techPath + techName + "/factions/*.", factions, false, false);
//
//			GraphicButton *button=new GraphicButton();
//			button->init(techInfoXPos, keyButtonsYBase, keyButtonsWidth,keyButtonsHeight);
//			button->setText(techName);
//			button->setUseCustomTexture(true);
//			button->setCustomTexture(CoreData::getInstance().getCustomTexture());
//			keyTechButtons.push_back(button);
//
//			int techFactionCount = factions.size();
//			GraphicLabel *label=new GraphicLabel();
//			label->init(techInfoXPos + keyButtonsWidth+10,keyButtonsYBase,labelWidth,20);
//			label->setText(intToStr(techFactionCount));
//			labelsTech.push_back(label);
//		}
//	}
//
//	mapListRemote.clear();
//	Tokenize(mapsMetaData,mapListRemote,"\n");
//
//	getMapsLocalList();
//	for(unsigned int i=0; i < mapListRemote.size(); i++) {
//		string mapInfo = mapListRemote[i];
//		std::vector<std::string> mapInfoList;
//		Tokenize(mapInfo,mapInfoList,"|");
//
//		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("mapInfoList.size() [%d]\n",(int)mapInfoList.size());
//		if(mapInfoList.size() >= 5) {
//			string mapName = mapInfoList[0];
//			string mapPlayerCount = mapInfoList[1];
//			string mapCRC = mapInfoList[2];
//			string mapDescription = mapInfoList[3];
//			string mapURL = mapInfoList[4];
//			//bool alreadyHasMap = (std::find(mapFiles.begin(),mapFiles.end(),mapName) != mapFiles.end());
//			mapCacheList[mapName] = mapURL;
//
//			GraphicButton *button=new GraphicButton();
//			button->init(mapInfoXPos, keyButtonsYBase, keyButtonsWidth,keyButtonsHeight);
//			button->setText(mapName);
//			button->setUseCustomTexture(true);
//			button->setCustomTexture(CoreData::getInstance().getCustomTexture());
//			keyMapButtons.push_back(button);
//
//			GraphicLabel *label=new GraphicLabel();
//			label->init(mapInfoXPos + keyButtonsWidth + 10,keyButtonsYBase,labelWidth,20);
//			label->setText(mapPlayerCount);
//			labelsMap.push_back(label);
//		}
//	}
//	for(unsigned int i=0; i < mapFilesUserData.size(); i++) {
//		string mapName = mapFilesUserData[i];
//		bool alreadyHasMap = (mapCacheList.find(mapName) != mapCacheList.end());
//		if(alreadyHasMap == false) {
//			vector<string> mapPaths = config.getPathListForType(ptMaps);
//	        string &mapPath = mapPaths[1];
//	        endPathWithSlash(mapPath);
//	        mapPath += mapName;
//	        MapInfo mapInfo = loadMapInfo(mapPath);
//
//			GraphicButton *button=new GraphicButton();
//			button->init(mapInfoXPos, keyButtonsYBase, keyButtonsWidth,keyButtonsHeight);
//			button->setText(mapName);
//			button->setUseCustomTexture(true);
//			button->setCustomTexture(CoreData::getInstance().getCustomTexture());
//			keyMapButtons.push_back(button);
//
//			int mapPlayerCount = mapInfo.players;
//			GraphicLabel *label=new GraphicLabel();
//			label->init(mapInfoXPos + keyButtonsWidth + 10,keyButtonsYBase,labelWidth,20);
//			label->setText(intToStr(mapPlayerCount));
//			labelsMap.push_back(label);
//		}
//	}
//
//	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
//
	int listBoxLength = 400;
	keyTilesetScrollBar.init(tilesetInfoXPos + keyButtonsWidth,scrollListsYPos-listBoxLength+keyButtonsLineHeight,false,200,20);
	keyTilesetScrollBar.setLength(listBoxLength);
	//keyTilesetScrollBar.setElementCount(keyTilesetButtons.size());
	keyTilesetScrollBar.setElementCount(0);
	keyTilesetScrollBar.setVisibleSize(keyButtonsToRender);
	keyTilesetScrollBar.setVisibleStart(0);

	keyTechScrollBar.init(techInfoXPos + keyButtonsWidth + labelWidth + 20,scrollListsYPos-listBoxLength+keyButtonsLineHeight,false,200,20);
	keyTechScrollBar.setLength(listBoxLength);
	//keyTechScrollBar.setElementCount(keyTechButtons.size());
	keyTechScrollBar.setElementCount(0);
	keyTechScrollBar.setVisibleSize(keyButtonsToRender);
	keyTechScrollBar.setVisibleStart(0);

	keyMapScrollBar.init(mapInfoXPos + keyButtonsWidth + labelWidth + 20,scrollListsYPos-listBoxLength+keyButtonsLineHeight,false,200,20);
	keyMapScrollBar.setLength(listBoxLength);
	//keyMapScrollBar.setElementCount(keyMapButtons.size());
	keyMapScrollBar.setElementCount(0);
	keyMapScrollBar.setVisibleSize(keyButtonsToRender);
	keyMapScrollBar.setVisibleStart(0);

	GraphicComponent::applyAllCustomProperties(containerName);

	modHttpServerThread = new SimpleTaskThread(this,0,200);
	modHttpServerThread->setUniqueID(__FILE__);
	modHttpServerThread->start();

    findDirs(config.getPathListForType(ptTilesets), tilesetFiles);
    findDirs(config.getPathListForType(ptTechs), techTreeFiles);

	vector<string> mapPathList = config.getPathListForType(ptMaps);
	std::pair<string,string> mapsPath;
	if(mapPathList.size() > 0) {
		mapsPath.first = mapPathList[0];
	}
	if(mapPathList.size() > 1) {
		mapsPath.second = mapPathList[1];
	}
	std::pair<string,string> tilesetsPath;
	vector<string> tilesetsList = Config::getInstance().getPathListForType(ptTilesets);
	if(tilesetsList.size() > 0) {
		tilesetsPath.first = tilesetsList[0];
		if(tilesetsList.size() > 1) {
			tilesetsPath.second = tilesetsList[1];
		}
	}

	std::pair<string,string> techtreesPath;
	vector<string> techtreesList = Config::getInstance().getPathListForType(ptTechs);
	if(techtreesList.size() > 0) {
		techtreesPath.first = techtreesList[0];
		if(techtreesList.size() > 1) {
			techtreesPath.second = techtreesList[1];
		}
	}

	string fileArchiveExtension = config.getString("FileArchiveExtension","");
	string fileArchiveExtractCommand = config.getString("FileArchiveExtractCommand","");
	string fileArchiveExtractCommandParameters = config.getString("FileArchiveExtractCommandParameters","");

	ftpClientThread = new FTPClientThread(-1,"",
			mapsPath,tilesetsPath,techtreesPath,
			this,fileArchiveExtension,fileArchiveExtractCommand,
			fileArchiveExtractCommandParameters);
	ftpClientThread->start();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void MenuStateMods::simpleTask(BaseThread *callingThread) {
	std::string techsMetaData = "";
	std::string tilesetsMetaData = "";
	std::string mapsMetaData = "";

	Config &config = Config::getInstance();
	if(config.getString("Masterserver","") != "") {
		string baseURL = config.getString("Masterserver");

		CURL *handle = SystemFlags::initHTTP();
		techsMetaData = SystemFlags::getHTTP(baseURL + "showTechsForGlest.php",handle);
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("techsMetaData [%s]\n",techsMetaData.c_str());
		tilesetsMetaData = SystemFlags::getHTTP(baseURL + "showTilesetsForGlest.php",handle);
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("tilesetsMetaData [%s]\n",tilesetsMetaData.c_str());
		mapsMetaData = SystemFlags::getHTTP(baseURL + "showMapsForGlest.php",handle);
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("mapsMetaData [%s]\n",mapsMetaData.c_str());
		SystemFlags::cleanupHTTP(&handle);
	}

	tilesetListRemote.clear();
	Tokenize(tilesetsMetaData,tilesetListRemote,"\n");

	getTilesetsLocalList();
	for(unsigned int i=0; i < tilesetListRemote.size(); i++) {
		string tilesetInfo = tilesetListRemote[i];
		std::vector<std::string> tilesetInfoList;
		Tokenize(tilesetInfo,tilesetInfoList,"|");

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("tilesetInfoList.size() [%d]\n",(int)tilesetInfoList.size());
		if(tilesetInfoList.size() >= 4) {
			string tilesetName = tilesetInfoList[0];
			string tilesetCRC = tilesetInfoList[1];
			string tilesetDescription = tilesetInfoList[2];
			string tilesetURL = tilesetInfoList[3];
			//bool alreadyHasTileset = (std::find(tilesetFiles.begin(),tilesetFiles.end(),tilesetName) != tilesetFiles.end());
			tilesetCacheList[tilesetName] = tilesetURL;

			GraphicButton *button=new GraphicButton();
			button->init(tilesetInfoXPos, keyButtonsYBase, keyButtonsWidth,keyButtonsHeight);
			button->setText(tilesetName);
			button->setUseCustomTexture(true);
			button->setCustomTexture(CoreData::getInstance().getCustomTexture());

			//if(alreadyHasTileset == true) {
			//	button->setEnabled(false);
			//}
			keyTilesetButtons.push_back(button);
		}
	}
	for(unsigned int i=0; i < tilesetFilesUserData.size(); i++) {
		string tilesetName = tilesetFilesUserData[i];
		bool alreadyHasTileset = (tilesetCacheList.find(tilesetName) != tilesetCacheList.end());
		if(alreadyHasTileset == false) {
			GraphicButton *button=new GraphicButton();
			button->init(tilesetInfoXPos, keyButtonsYBase, keyButtonsWidth,keyButtonsHeight);
			button->setText(tilesetName);
			button->setUseCustomTexture(true);
			button->setCustomTexture(CoreData::getInstance().getCustomTexture());
			keyTilesetButtons.push_back(button);
		}
	}

	techListRemote.clear();
	Tokenize(techsMetaData,techListRemote,"\n");

	getTechsLocalList();
	for(unsigned int i=0; i < techListRemote.size(); i++) {
		string techInfo = techListRemote[i];
		std::vector<std::string> techInfoList;
		Tokenize(techInfo,techInfoList,"|");

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("techInfoList.size() [%d]\n",(int)techInfoList.size());
		if(techInfoList.size() >= 5) {
			string techName = techInfoList[0];
			string techFactionCount = techInfoList[1];
			string techCRC = techInfoList[2];
			string techDescription = techInfoList[3];
			string techURL = techInfoList[4];
			//bool alreadyHasTech = (std::find(techTreeFiles.begin(),techTreeFiles.end(),techName) != techTreeFiles.end());
			techCacheList[techName] = techURL;

			GraphicButton *button=new GraphicButton();
			button->init(techInfoXPos, keyButtonsYBase, keyButtonsWidth,keyButtonsHeight);
			button->setText(techName);
			button->setUseCustomTexture(true);
			button->setCustomTexture(CoreData::getInstance().getCustomTexture());

			//if(alreadyHasTech == true) {
			//	button->setEnabled(false);
			//}
			keyTechButtons.push_back(button);
			GraphicLabel *label=new GraphicLabel();
			label->init(techInfoXPos + keyButtonsWidth+10,keyButtonsYBase,labelWidth,20);
			label->setText(techFactionCount);
			labelsTech.push_back(label);
		}
	}
	for(unsigned int i=0; i < techTreeFilesUserData.size(); i++) {
		string techName = techTreeFilesUserData[i];
		bool alreadyHasTech = (techCacheList.find(techName) != techCacheList.end());
		if(alreadyHasTech == false) {
			vector<string> techPaths = config.getPathListForType(ptTechs);
	        string &techPath = techPaths[1];
	        endPathWithSlash(techPath);
	        vector<string> factions;
	        findAll(techPath + techName + "/factions/*.", factions, false, false);

			GraphicButton *button=new GraphicButton();
			button->init(techInfoXPos, keyButtonsYBase, keyButtonsWidth,keyButtonsHeight);
			button->setText(techName);
			button->setUseCustomTexture(true);
			button->setCustomTexture(CoreData::getInstance().getCustomTexture());
			keyTechButtons.push_back(button);

			int techFactionCount = factions.size();
			GraphicLabel *label=new GraphicLabel();
			label->init(techInfoXPos + keyButtonsWidth+10,keyButtonsYBase,labelWidth,20);
			label->setText(intToStr(techFactionCount));
			labelsTech.push_back(label);
		}
	}

	mapListRemote.clear();
	Tokenize(mapsMetaData,mapListRemote,"\n");

	getMapsLocalList();
	for(unsigned int i=0; i < mapListRemote.size(); i++) {
		string mapInfo = mapListRemote[i];
		std::vector<std::string> mapInfoList;
		Tokenize(mapInfo,mapInfoList,"|");

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("mapInfoList.size() [%d]\n",(int)mapInfoList.size());
		if(mapInfoList.size() >= 5) {
			string mapName = mapInfoList[0];
			string mapPlayerCount = mapInfoList[1];
			string mapCRC = mapInfoList[2];
			string mapDescription = mapInfoList[3];
			string mapURL = mapInfoList[4];
			//bool alreadyHasMap = (std::find(mapFiles.begin(),mapFiles.end(),mapName) != mapFiles.end());
			mapCacheList[mapName] = mapURL;

			GraphicButton *button=new GraphicButton();
			button->init(mapInfoXPos, keyButtonsYBase, keyButtonsWidth,keyButtonsHeight);
			button->setText(mapName);
			button->setUseCustomTexture(true);
			button->setCustomTexture(CoreData::getInstance().getCustomTexture());
			keyMapButtons.push_back(button);

			GraphicLabel *label=new GraphicLabel();
			label->init(mapInfoXPos + keyButtonsWidth + 10,keyButtonsYBase,labelWidth,20);
			label->setText(mapPlayerCount);
			labelsMap.push_back(label);
		}
	}
	for(unsigned int i=0; i < mapFilesUserData.size(); i++) {
		string mapName = mapFilesUserData[i];
		bool alreadyHasMap = (mapCacheList.find(mapName) != mapCacheList.end());
		if(alreadyHasMap == false) {
			vector<string> mapPaths = config.getPathListForType(ptMaps);
	        string &mapPath = mapPaths[1];
	        endPathWithSlash(mapPath);
	        mapPath += mapName;
	        MapInfo mapInfo = loadMapInfo(mapPath);

			GraphicButton *button=new GraphicButton();
			button->init(mapInfoXPos, keyButtonsYBase, keyButtonsWidth,keyButtonsHeight);
			button->setText(mapName);
			button->setUseCustomTexture(true);
			button->setCustomTexture(CoreData::getInstance().getCustomTexture());
			keyMapButtons.push_back(button);

			int mapPlayerCount = mapInfo.players;
			GraphicLabel *label=new GraphicLabel();
			label->init(mapInfoXPos + keyButtonsWidth + 10,keyButtonsYBase,labelWidth,20);
			label->setText(intToStr(mapPlayerCount));
			labelsMap.push_back(label);
		}
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	int listBoxLength = 400;
	keyTilesetScrollBar.init(tilesetInfoXPos + keyButtonsWidth,scrollListsYPos-listBoxLength+keyButtonsLineHeight,false,200,20);
	keyTilesetScrollBar.setLength(listBoxLength);
	keyTilesetScrollBar.setElementCount(keyTilesetButtons.size());
	keyTilesetScrollBar.setVisibleSize(keyButtonsToRender);
	keyTilesetScrollBar.setVisibleStart(0);

	keyTechScrollBar.init(techInfoXPos + keyButtonsWidth + labelWidth + 20,scrollListsYPos-listBoxLength+keyButtonsLineHeight,false,200,20);
	keyTechScrollBar.setLength(listBoxLength);
	keyTechScrollBar.setElementCount(keyTechButtons.size());
	keyTechScrollBar.setVisibleSize(keyButtonsToRender);
	keyTechScrollBar.setVisibleStart(0);

	keyMapScrollBar.init(mapInfoXPos + keyButtonsWidth + labelWidth + 20,scrollListsYPos-listBoxLength+keyButtonsLineHeight,false,200,20);
	keyMapScrollBar.setLength(listBoxLength);
	keyMapScrollBar.setElementCount(keyMapButtons.size());
	keyMapScrollBar.setVisibleSize(keyButtonsToRender);
	keyMapScrollBar.setVisibleStart(0);

	modHttpServerThread->signalQuit();
}

MapInfo MenuStateMods::loadMapInfo(string file) {
	Lang &lang= Lang::getInstance();

	MapInfo mapInfo;
	//memset(&mapInfo,0,sizeof(mapInfo));
	try{
		FILE *f= fopen(file.c_str(), "rb");
		if(f != NULL) {

			MapFileHeader header;
			size_t readBytes = fread(&header, sizeof(MapFileHeader), 1, f);

			mapInfo.size.x= header.width;
			mapInfo.size.y= header.height;
			mapInfo.players= header.maxFactions;

			mapInfo.desc= lang.get("MaxPlayers")+": "+intToStr(mapInfo.players)+"\n";
			mapInfo.desc+=lang.get("Size")+": "+intToStr(mapInfo.size.x) + " x " + intToStr(mapInfo.size.y);

			fclose(f);
		}
	}
	catch(exception &e) {
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s] loading map [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what(),file.c_str());
		throw runtime_error("Error loading map file: [" + file + "] msg: " + e.what());
	}

	return mapInfo;
}

void MenuStateMods::getTechsLocalList() {
	Config &config = Config::getInstance();
	vector<string> results;
	findDirs(config.getPathListForType(ptTechs), results);
	techTreeFiles = results;

	techTreeFilesUserData.clear();
	if(config.getPathListForType(ptTechs).size() > 1) {
		string path = config.getPathListForType(ptTechs)[1];
		endPathWithSlash(path);
		findDirs(path, techTreeFilesUserData, false, false);
	}
}

void MenuStateMods::refreshTechs() {
	getTechsLocalList();
	for(int i=0; i < techListRemote.size(); i++) {
		string techInfo = techListRemote[i];
		std::vector<std::string> techInfoList;
		Tokenize(techInfo,techInfoList,"|");
		if(techInfoList.size() >= 5) {
			string techName = techInfoList[0];
			string techFactionCount = techInfoList[1];
			string techCRC = techInfoList[2];
			string techDescription = techInfoList[3];
			string techURL = techInfoList[4];
			//bool alreadyHasTech = (std::find(techTreeFiles.begin(),techTreeFiles.end(),techName) != techTreeFiles.end());
			techCacheList[techName] = techURL;

			//GraphicButton *button= keyTechButtons[i];
			//if(alreadyHasTech == true) {
			//	button->setEnabled(false);
			//}
		}
	}
}

void MenuStateMods::getTilesetsLocalList() {
	Config &config = Config::getInstance();
	vector<string> results;
	findDirs(config.getPathListForType(ptTilesets), results);
	tilesetFiles = results;

	tilesetFilesUserData.clear();
	if(config.getPathListForType(ptTilesets).size() > 1) {
		string path = config.getPathListForType(ptTilesets)[1];
		endPathWithSlash(path);
		findDirs(path, tilesetFilesUserData, false, false);
	}
}

void MenuStateMods::refreshTilesets() {
	getTilesetsLocalList();
	for(int i=0; i < tilesetListRemote.size(); i++) {
		string tilesetInfo = tilesetListRemote[i];
		std::vector<std::string> tilesetInfoList;
		Tokenize(tilesetInfo,tilesetInfoList,"|");
		if(tilesetInfoList.size() >= 4) {
			string tilesetName = tilesetInfoList[0];
			string tilesetCRC = tilesetInfoList[1];
			string tilesetDescription = tilesetInfoList[2];
			string tilesetURL = tilesetInfoList[3];
			//bool alreadyHasTileset = (std::find(tilesetFiles.begin(),tilesetFiles.end(),tilesetName) != tilesetFiles.end());
			tilesetCacheList[tilesetName] = tilesetURL;

			//GraphicButton *button= keyTilesetButtons[i];
			//if(alreadyHasTileset == true) {
			//	button->setEnabled(false);
			//}
		}
	}
}

void MenuStateMods::getMapsLocalList() {
	Config &config = Config::getInstance();
	vector<string> results;
	set<string> allMaps;
    findAll(config.getPathListForType(ptMaps), "*.gbm", results, false, false);
	copy(results.begin(), results.end(), std::inserter(allMaps, allMaps.begin()));
	results.clear();
    findAll(config.getPathListForType(ptMaps), "*.mgm", results, false, false);
	copy(results.begin(), results.end(), std::inserter(allMaps, allMaps.begin()));
	results.clear();

	copy(allMaps.begin(), allMaps.end(), std::back_inserter(results));
	mapFiles = results;

	mapFilesUserData.clear();
	if(config.getPathListForType(ptMaps).size() > 1) {
		string path = config.getPathListForType(ptMaps)[1];
		endPathWithSlash(path);

		vector<string> results2;
		set<string> allMaps2;
	    findAll(path + "*.gbm", results2, false, false);
		copy(results2.begin(), results2.end(), std::inserter(allMaps2, allMaps2.begin()));

		results2.clear();
	    findAll(path + "*.mgm", results2, false, false);
		copy(results2.begin(), results2.end(), std::inserter(allMaps2, allMaps2.begin()));

		results2.clear();
		copy(allMaps2.begin(), allMaps2.end(), std::back_inserter(results2));
		mapFilesUserData = results2;
		//printf("\n\nMap path [%s] mapFilesUserData.size() = %d\n\n\n",path.c_str(),mapFilesUserData.size());
	}

}

void MenuStateMods::refreshMaps() {
	getMapsLocalList();
	for(int i=0; i < mapListRemote.size(); i++) {
		string mapInfo = mapListRemote[i];
		std::vector<std::string> mapInfoList;
		Tokenize(mapInfo,mapInfoList,"|");
		if(mapInfoList.size() >= 5) {
			string mapName = mapInfoList[0];
			string mapPlayerCount = mapInfoList[1];
			string mapCRC = mapInfoList[2];
			string mapDescription = mapInfoList[3];
			string mapURL = mapInfoList[4];
			//bool alreadyHasMap = (std::find(mapFiles.begin(),mapFiles.end(),mapName) != mapFiles.end());
			mapCacheList[mapName] = mapURL;

			//GraphicButton *button= keyMapButtons[i];
			//if(alreadyHasMap == true) {
			//	button->setEnabled(false);
			//}
		}
	}
}

MenuStateMods::~MenuStateMods() {
	clearUserButtons();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
	if(modHttpServerThread != NULL) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
		modHttpServerThread->setThreadOwnerValid(false);

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
		if( modHttpServerThread->canShutdown(true) == true &&
			modHttpServerThread->shutdownAndWait() == true) {
			delete modHttpServerThread;
		}
		modHttpServerThread = NULL;
	}

	if(ftpClientThread != NULL) {
	    ftpClientThread->setCallBackObject(NULL);
	    if(ftpClientThread->shutdownAndWait() == true) {
            delete ftpClientThread;
            ftpClientThread = NULL;
	    }
	}
}

void MenuStateMods::clearUserButtons() {
	// Techs
	while(!keyTechButtons.empty()) {
		delete keyTechButtons.back();
		keyTechButtons.pop_back();
	}
	while(!labelsTech.empty()) {
		delete labelsTech.back();
		labelsTech.pop_back();
	}

	// Tilesets
	while(!keyTilesetButtons.empty()) {
		delete keyTilesetButtons.back();
		keyTilesetButtons.pop_back();
	}

	// Maps
	while(!keyMapButtons.empty()) {
		delete keyMapButtons.back();
		keyMapButtons.pop_back();
	}
	while(!labelsMap.empty()) {
		delete labelsMap.back();
		labelsMap.pop_back();
	}
}

void MenuStateMods::mouseClick(int x, int y, MouseButton mouseButton) {

	CoreData &coreData= CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();
	Lang &lang= Lang::getInstance();

	if(buttonReturn.mouseClick(x,y)) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		soundRenderer.playFx(coreData.getClickSoundA());

		if(fileFTPProgressList.size() > 0) {
			mainMessageBoxState = ftpmsg_Quit;
			mainMessageBox.init(lang.get("Yes"),lang.get("No"));
			showMessageBox("You currently have: " + intToStr(fileFTPProgressList.size()) + " files downloading, exit and abort these file(s)?", "Question?", true);
		}
		else {
			mainMenu->setState(new MenuStateRoot(program, mainMenu));
			return;
		}
    }
	else if(mainMessageBox.getEnabled()) {
		int button= 1;
		if(mainMessageBox.mouseClick(x, y, button)) {
			soundRenderer.playFx(coreData.getClickSoundA());
			mainMessageBox.setEnabled(false);
			mainMessageBox.init(lang.get("Yes"),lang.get("No"));
			if(button == 1) {
			    if(mainMessageBoxState == ftpmsg_Quit) {
			    	mainMessageBoxState = ftpmsg_None;
					mainMenu->setState(new MenuStateRoot(program, mainMenu));
					return;
			    }
			    else if(mainMessageBoxState == ftpmsg_GetMap) {
			    	mainMessageBoxState = ftpmsg_None;

			    	Config &config = Config::getInstance();
			    	vector<string> mapPaths = config.getPathListForType(ptMaps);
			    	if(mapPaths.size() > 1) {
			    		string removeMap = mapPaths[1];
			    		endPathWithSlash(removeMap);
			    		removeMap += selectedMapName;

			    		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Removing Map [%s]\n",removeMap.c_str());
			    		removeFile(removeMap);

			    		bool remoteHasMap = (mapCacheList.find(selectedMapName) != mapCacheList.end());
			    		if(remoteHasMap == false) {
							for(unsigned int i = 0; i < keyMapButtons.size(); ++i) {
								GraphicButton *button = keyMapButtons[i];
								if(button != NULL && button->getText() == selectedMapName) {
									delete button;
									keyMapButtons.erase(keyMapButtons.begin() + i);
									labelsMap.erase(labelsMap.begin() + i);
									keyMapScrollBar.setElementCount(keyMapButtons.size());
									break;
								}
							}
			    		}

			    		selectedMapName = "";
			    		refreshMaps();
			    		Checksum::clearFileCache();
			    	}
			    }
			    else if(mainMessageBoxState == ftpmsg_GetTileset) {
			    	mainMessageBoxState = ftpmsg_None;

			    	Config &config = Config::getInstance();
			    	vector<string> tilesetPaths = config.getPathListForType(ptTilesets);
			    	if(tilesetPaths.size() > 1) {
			    		string removeTileset = tilesetPaths[1];
			    		endPathWithSlash(removeTileset);
			    		removeTileset += selectedTilesetName;

			    		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Removing Tileset [%s]\n",removeTileset.c_str());
			    		removeFolder(removeTileset);

			    		bool remoteHasTileset = (tilesetCacheList.find(selectedTilesetName) != tilesetCacheList.end());
			    		if(remoteHasTileset == false) {
							for(unsigned int i = 0; i < keyTilesetButtons.size(); ++i) {
								GraphicButton *button = keyTilesetButtons[i];
								if(button != NULL && button->getText() == selectedTilesetName) {
									delete button;
									keyTilesetButtons.erase(keyTilesetButtons.begin() + i);
									keyTilesetScrollBar.setElementCount(keyTilesetButtons.size());
									break;
								}
							}
			    		}
			    		MutexSafeWrapper safeMutexFTPProgress((ftpClientThread != NULL ? ftpClientThread->getProgressMutex() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
			            safeMutexFTPProgress.Lock();
			            Checksum::clearFileCache();
			            vector<string> paths        = Config::getInstance().getPathListForType(ptTilesets);
			            string pathSearchString     = string("/") + selectedTilesetName + string("/*");
			            const string filterFileExt  = ".xml";
			            clearFolderTreeContentsCheckSum(paths, pathSearchString, filterFileExt);
			            clearFolderTreeContentsCheckSumList(paths, pathSearchString, filterFileExt);
			            safeMutexFTPProgress.ReleaseLock();

			    		selectedTilesetName = "";
			    		refreshTilesets();
			    	}
			    }
			    else if(mainMessageBoxState == ftpmsg_GetTechtree) {
			    	mainMessageBoxState = ftpmsg_None;

			    	Config &config = Config::getInstance();
			    	vector<string> techPaths = config.getPathListForType(ptTechs);
			    	if(techPaths.size() > 1) {
			    		string removeTech = techPaths[1];
			    		endPathWithSlash(removeTech);
			    		removeTech+= selectedTechName;

			    		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Removing Techtree [%s]\n",removeTech.c_str());
			    		removeFolder(removeTech);

			    		bool remoteHasTech = (techCacheList.find(selectedTechName) != techCacheList.end());
			    		if(remoteHasTech == false) {
							for(unsigned int i = 0; i < keyTechButtons.size(); ++i) {
								GraphicButton *button = keyTechButtons[i];
								if(button != NULL && button->getText() == selectedTechName) {
									delete button;
									keyTechButtons.erase(keyTechButtons.begin() + i);
									labelsTech.erase(labelsTech.begin() + i);
									keyTechScrollBar.setElementCount(keyTechButtons.size());
									break;
								}
							}
			    		}

			    		MutexSafeWrapper safeMutexFTPProgress((ftpClientThread != NULL ? ftpClientThread->getProgressMutex() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
			            // Clear the CRC file Cache
			            safeMutexFTPProgress.Lock();
			            Checksum::clearFileCache();
			            vector<string> paths        = Config::getInstance().getPathListForType(ptTechs);
			            string pathSearchString     = string("/") + selectedTechName + string("/*");
			            const string filterFileExt  = ".xml";
			            clearFolderTreeContentsCheckSum(paths, pathSearchString, filterFileExt);
			            clearFolderTreeContentsCheckSumList(paths, pathSearchString, filterFileExt);
			            safeMutexFTPProgress.ReleaseLock();

			    		selectedTechName = "";
			    		refreshTechs();
			    	}
			    }
			}
		}
	}
	else if(keyTechScrollBar.mouseClick(x, y)) {
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			soundRenderer.playFx(coreData.getClickSoundB());
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
    }
	else if(keyTilesetScrollBar.mouseClick(x, y)) {
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			soundRenderer.playFx(coreData.getClickSoundB());
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
    }
	else if(keyMapScrollBar.mouseClick(x, y)) {
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			soundRenderer.playFx(coreData.getClickSoundB());
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
    }
	else if(buttonInstallTech.mouseClick(x, y) && buttonInstallTech.getEnabled()) {
		soundRenderer.playFx(coreData.getClickSoundB());
		if(selectedTechName != "") {
			bool alreadyHasTech = (std::find(techTreeFiles.begin(),techTreeFiles.end(),selectedTechName) != techTreeFiles.end());
			if(alreadyHasTech == true) {
				mainMessageBoxState = ftpmsg_None;
				mainMessageBox.init(lang.get("Ok"));
				showMessageBox("You already have the tech: " + selectedTechName + " installed.", "Notice", true);
			}
			else {
				string techName = selectedTechName;
				string techURL = techCacheList[techName];
				ftpClientThread->addTechtreeToRequests(techName,techURL);
				MutexSafeWrapper safeMutexFTPProgress((ftpClientThread != NULL ? ftpClientThread->getProgressMutex() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
				fileFTPProgressList[techName] = pair<int,string>(0,"");
				safeMutexFTPProgress.ReleaseLock();
				buttonInstallTech.setEnabled(false);
			}
		}
		else {
			mainMessageBoxState = ftpmsg_None;
			mainMessageBox.init(lang.get("Ok"));
			showMessageBox("You must first select a tech to install.", "Notice", true);
		}
	}
	else if(buttonRemoveTech.mouseClick(x, y) && buttonRemoveTech.getEnabled()) {
		soundRenderer.playFx(coreData.getClickSoundB());
		if(selectedTechName != "") {
			bool alreadyHasTech = (std::find(techTreeFiles.begin(),techTreeFiles.end(),selectedTechName) != techTreeFiles.end());
			if(alreadyHasTech == true) {
				mainMessageBoxState = ftpmsg_GetTechtree;
				showMessageBox("Are you sure you want to remove the tech: " + selectedTechName, "Question?", true);
			}
			else {
				mainMessageBoxState = ftpmsg_None;
				mainMessageBox.init(lang.get("Ok"));
				showMessageBox("You do not have the tech: " + selectedTechName + " installed.", "Notice", true);
			}
		}
		else {
			mainMessageBoxState = ftpmsg_None;
			mainMessageBox.init(lang.get("Ok"));
			showMessageBox("You must first select a tech to remove.", "Notice", true);
		}
	}
	else if(buttonInstallTileset.mouseClick(x, y) && buttonInstallTileset.getEnabled()) {
		soundRenderer.playFx(coreData.getClickSoundB());
		if(selectedTilesetName != "") {
			bool alreadyHasTileset = (std::find(tilesetFiles.begin(),tilesetFiles.end(),selectedTilesetName) != tilesetFiles.end());
			if(alreadyHasTileset == true) {
				mainMessageBoxState = ftpmsg_None;
				mainMessageBox.init(lang.get("Ok"));
				showMessageBox("You already have the tileset: " + selectedTilesetName + " installed.", "Notice", true);
			}
			else {
				string tilesetName = selectedTilesetName;
				string tilesetURL = tilesetCacheList[tilesetName];
				ftpClientThread->addTilesetToRequests(tilesetName,tilesetURL);
				MutexSafeWrapper safeMutexFTPProgress((ftpClientThread != NULL ? ftpClientThread->getProgressMutex() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
				fileFTPProgressList[tilesetName] = pair<int,string>(0,"");
				safeMutexFTPProgress.ReleaseLock();
				buttonInstallTileset.setEnabled(false);
			}
		}
		else {
			mainMessageBoxState = ftpmsg_None;
			mainMessageBox.init(lang.get("Ok"));
			showMessageBox("You must first select a tileset to install.", "Notice", true);
		}
	}
	else if(buttonRemoveTileset.mouseClick(x, y) && buttonRemoveTileset.getEnabled()) {
		soundRenderer.playFx(coreData.getClickSoundB());
		if(selectedTilesetName != "") {
			bool alreadyHasTileset = (std::find(tilesetFiles.begin(),tilesetFiles.end(),selectedTilesetName) != tilesetFiles.end());
			if(alreadyHasTileset == true) {
				mainMessageBoxState = ftpmsg_GetTileset;
				showMessageBox("Are you sure you want to remove the tileset: " + selectedTilesetName, "Question?", true);
			}
			else {
				mainMessageBoxState = ftpmsg_None;
				mainMessageBox.init(lang.get("Ok"));
				showMessageBox("You do not have the tileset: " + selectedTilesetName + " installed.", "Notice", true);
			}
		}
		else {
			mainMessageBoxState = ftpmsg_None;
			mainMessageBox.init(lang.get("Ok"));
			showMessageBox("You must first select a tileset to remove.", "Notice", true);
		}
	}
	else if(buttonInstallMap.mouseClick(x, y) && buttonInstallMap.getEnabled()) {
		soundRenderer.playFx(coreData.getClickSoundB());
		if(selectedMapName != "") {
			bool alreadyHasMap = (std::find(mapFiles.begin(),mapFiles.end(),selectedMapName) != mapFiles.end());
			if(alreadyHasMap == true) {
				mainMessageBoxState = ftpmsg_None;
				mainMessageBox.init(lang.get("Ok"));
				showMessageBox("You already have the map: " + selectedMapName + " installed.", "Notice", true);
			}
			else {
				string mapName = selectedMapName;
				string mapURL = mapCacheList[mapName];
				ftpClientThread->addMapToRequests(mapName,mapURL);
				MutexSafeWrapper safeMutexFTPProgress((ftpClientThread != NULL ? ftpClientThread->getProgressMutex() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
				fileFTPProgressList[mapName] = pair<int,string>(0,"");
				safeMutexFTPProgress.ReleaseLock();
				buttonInstallMap.setEnabled(false);
			}
		}
		else {
			mainMessageBoxState = ftpmsg_None;
			mainMessageBox.init(lang.get("Ok"));
			showMessageBox("You must first select a map to install.", "Notice", true);
		}
	}
	else if(buttonRemoveMap.mouseClick(x, y) && buttonRemoveMap.getEnabled()) {
		soundRenderer.playFx(coreData.getClickSoundB());
		if(selectedMapName != "") {
			bool alreadyHasMap = (std::find(mapFiles.begin(),mapFiles.end(),selectedMapName) != mapFiles.end());
			if(alreadyHasMap == true) {
				mainMessageBoxState = ftpmsg_GetMap;
				showMessageBox("Are you sure you want to remove the map: " + selectedMapName, "Question?", true);
			}
			else {
				mainMessageBoxState = ftpmsg_None;
				mainMessageBox.init(lang.get("Ok"));
				showMessageBox("You do not have the map: " + selectedMapName + " installed.", "Notice", true);
			}
		}
		else {
			mainMessageBoxState = ftpmsg_None;
			mainMessageBox.init(lang.get("Ok"));
			showMessageBox("You must first select a map to remove.", "Notice", true);
		}
	}
    else {
    	if(keyMapScrollBar.getElementCount() != 0) {
			for (int i = keyMapScrollBar.getVisibleStart();
					i <= keyMapScrollBar.getVisibleEnd(); ++i) {
				if(keyMapButtons[i]->mouseClick(x, y) && keyMapButtons[i]->getEnabled()) {
					string mapName = keyMapButtons[i]->getText();
					if(mapName != "") {
						selectedMapName = mapName;
					}
					break;
				}
			}
		}
    	if(keyTechScrollBar.getElementCount() != 0) {
			for (int i = keyTechScrollBar.getVisibleStart();
					i <= keyTechScrollBar.getVisibleEnd(); ++i) {
				if(keyTechButtons[i]->mouseClick(x, y) && keyTechButtons[i]->getEnabled()) {
					string techName = keyTechButtons[i]->getText();
					if(techName != "") {
						selectedTechName = techName;
					}
					break;
				}
			}
		}
    	if(keyTilesetScrollBar.getElementCount() != 0) {
			for (int i = keyTilesetScrollBar.getVisibleStart();
					i <= keyTilesetScrollBar.getVisibleEnd(); ++i) {
				if(keyTilesetButtons[i]->mouseClick(x, y) && keyTilesetButtons[i]->getEnabled()) {
					string tilesetName = keyTilesetButtons[i]->getText();
					if(tilesetName != "") {
						selectedTilesetName = tilesetName;
					}
					break;
				}
			}
		}
    }


	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void MenuStateMods::mouseMove(int x, int y, const MouseState *ms) {
	buttonReturn.mouseMove(x, y);

	if (mainMessageBox.getEnabled()) {
		mainMessageBox.mouseMove(x, y);
	}

	buttonInstallTech.mouseMove(x, y);
	buttonRemoveTech.mouseMove(x, y);
	buttonInstallTileset.mouseMove(x, y);
	buttonRemoveTileset.mouseMove(x, y);
	buttonInstallMap.mouseMove(x, y);
	buttonRemoveMap.mouseMove(x, y);

    if (ms->get(mbLeft)) {
		keyMapScrollBar.mouseDown(x, y);
		keyTechScrollBar.mouseDown(x, y);
		keyTilesetScrollBar.mouseDown(x, y);
	}
    else {
		keyMapScrollBar.mouseMove(x, y);
		keyTechScrollBar.mouseMove(x, y);
		keyTilesetScrollBar.mouseMove(x, y);
	}

    if(keyMapScrollBar.getElementCount() !=0) {
    	for(int i = keyMapScrollBar.getVisibleStart(); i <= keyMapScrollBar.getVisibleEnd(); ++i) {
    		keyMapButtons[i]->mouseMove(x, y);
    	}
    }
    if(keyTechScrollBar.getElementCount() !=0) {
    	for(int i = keyTechScrollBar.getVisibleStart(); i <= keyTechScrollBar.getVisibleEnd(); ++i) {
    		keyTechButtons[i]->mouseMove(x, y);
    	}
    }
    if(keyTilesetScrollBar.getElementCount() !=0) {
    	for(int i = keyTilesetScrollBar.getVisibleStart(); i <= keyTilesetScrollBar.getVisibleEnd(); ++i) {
    		keyTilesetButtons[i]->mouseMove(x, y);
    	}
    }
}

void MenuStateMods::render() {
	try {
		Renderer &renderer= Renderer::getInstance();

		renderer.renderButton(&buttonReturn);
		renderer.renderButton(&buttonInstallTech);
		renderer.renderButton(&buttonRemoveTech);
		renderer.renderButton(&buttonInstallTileset);
		renderer.renderButton(&buttonRemoveTileset);
		renderer.renderButton(&buttonInstallMap);
		renderer.renderButton(&buttonRemoveMap);

		// Render Tech List
		renderer.renderLabel(&keyTechScrollBarTitle1);
		renderer.renderLabel(&keyTechScrollBarTitle2);
		if(keyTechScrollBar.getElementCount() != 0) {
			for(int i = keyTechScrollBar.getVisibleStart();
					i <= keyTechScrollBar.getVisibleEnd(); ++i) {
				bool alreadyHasTech = (std::find(techTreeFiles.begin(),techTreeFiles.end(),keyTechButtons[i]->getText()) != techTreeFiles.end());
				if(keyTechButtons[i]->getText() == selectedTechName) {
					bool lightedOverride = true;
					renderer.renderButton(keyTechButtons[i],&WHITE,&lightedOverride);
				}
				else if(alreadyHasTech == true) {
					Vec4f buttonColor = WHITE;
					buttonColor.w = 0.75f;
					renderer.renderButton(keyTechButtons[i],&buttonColor);
				}
				else {
					Vec4f fontColor=Vec4f(200.0f/255.0f, 187.0f/255.0f, 190.0f/255.0f, 0.75f);
					renderer.renderButton(keyTechButtons[i],&fontColor);
				}
				renderer.renderLabel(labelsTech[i]);
			}
		}
		renderer.renderScrollBar(&keyTechScrollBar);

		// Render Tileset List
		renderer.renderLabel(&keyTilesetScrollBarTitle1);
		if(keyTilesetScrollBar.getElementCount() != 0) {
			for(int i = keyTilesetScrollBar.getVisibleStart();
					i <= keyTilesetScrollBar.getVisibleEnd(); ++i) {
				bool alreadyHasTileset = (std::find(tilesetFiles.begin(),tilesetFiles.end(),keyTilesetButtons[i]->getText()) != tilesetFiles.end());
				if(keyTilesetButtons[i]->getText() == selectedTilesetName) {
					bool lightedOverride = true;
					renderer.renderButton(keyTilesetButtons[i],&WHITE,&lightedOverride);
				}
				else if(alreadyHasTileset == true) {
					Vec4f buttonColor = WHITE;
					buttonColor.w = 0.75f;
					renderer.renderButton(keyTilesetButtons[i],&buttonColor);
				}
				else {
					Vec4f fontColor=Vec4f(200.0f/255.0f, 187.0f/255.0f, 190.0f/255.0f, 0.75f);
					renderer.renderButton(keyTilesetButtons[i],&fontColor);
				}
			}
		}
		renderer.renderScrollBar(&keyTilesetScrollBar);

		// Render Map list
		renderer.renderLabel(&keyMapScrollBarTitle1);
		renderer.renderLabel(&keyMapScrollBarTitle2);
		if(keyMapScrollBar.getElementCount() != 0) {
			for(int i = keyMapScrollBar.getVisibleStart();
					i <= keyMapScrollBar.getVisibleEnd(); ++i) {
				bool alreadyHasMap = (std::find(mapFiles.begin(),mapFiles.end(),keyMapButtons[i]->getText()) != mapFiles.end());
				if(keyMapButtons[i]->getText() == selectedMapName) {
					bool lightedOverride = true;
					renderer.renderButton(keyMapButtons[i],&WHITE,&lightedOverride);
				}
				else if(alreadyHasMap == true) {
					Vec4f buttonColor = WHITE;
					buttonColor.w = 0.75f;
					renderer.renderButton(keyMapButtons[i],&buttonColor);
				}
				else {
					Vec4f fontColor=Vec4f(200.0f/255.0f, 187.0f/255.0f, 190.0f/255.0f, 0.75f);
					renderer.renderButton(keyMapButtons[i],&fontColor);
				}
				renderer.renderLabel(labelsMap[i]);
			}
		}
		renderer.renderScrollBar(&keyMapScrollBar);

        MutexSafeWrapper safeMutexFTPProgress((ftpClientThread != NULL ? ftpClientThread->getProgressMutex() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
        if(fileFTPProgressList.size() > 0) {
            int yLocation = buttonReturn.getY();
            for(std::map<string,pair<int,string> >::iterator iterMap = fileFTPProgressList.begin();
                iterMap != fileFTPProgressList.end(); ++iterMap) {

                string progressLabelPrefix = "Downloading " + iterMap->first + " [" + iterMap->second.second + "] ";
                //if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\nRendering file progress with the following prefix [%s]\n",progressLabelPrefix.c_str());

                renderer.renderProgressBar(
                    iterMap->second.first,
                    10,
                    yLocation,
                    CoreData::getInstance().getDisplayFontSmall(),
                    350,progressLabelPrefix);

                yLocation -= 10;
            }
        }
        safeMutexFTPProgress.ReleaseLock();

        renderer.renderConsole(&console,showFullConsole,true);

		if(mainMessageBox.getEnabled()) {
			renderer.renderMessageBox(&mainMessageBox);
		}
	}
	catch(const std::exception &ex) {
		char szBuf[1024]="";
		sprintf(szBuf,"In [%s::%s %d] error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		throw runtime_error(szBuf);
	}
}

void MenuStateMods::update() {
	Chrono chrono;
	chrono.start();

	Lang &lang= Lang::getInstance();

	// Tech List
	if (keyTechScrollBar.getElementCount() != 0) {
		for (int i = keyTechScrollBar.getVisibleStart();
				i <= keyTechScrollBar.getVisibleEnd(); ++i) {
			if(i >= keyTechButtons.size()) {
				char szBuf[1024]="";
				sprintf(szBuf,"i >= keyTechButtons.size(), i = %d, keyTechButtons.size() = %d",i,(int)keyTechButtons.size());
				throw runtime_error(szBuf);
			}

			keyTechButtons[i]->setY(keyButtonsYBase - keyButtonsLineHeight * (i
					- keyTechScrollBar.getVisibleStart()));
			labelsTech[i]->setY(keyButtonsYBase - keyButtonsLineHeight * (i
					- keyTechScrollBar.getVisibleStart()));
		}
	}

	// Tech List
	if (keyTilesetScrollBar.getElementCount() != 0) {
		for (int i = keyTilesetScrollBar.getVisibleStart();
				i <= keyTilesetScrollBar.getVisibleEnd(); ++i) {
			if(i >= keyTilesetButtons.size()) {
				char szBuf[1024]="";
				sprintf(szBuf,"i >= keyTilesetButtons.size(), i = %d, keyTilesetButtons.size() = %d",i,(int)keyTilesetButtons.size());
				throw runtime_error(szBuf);
			}

			int yPos = keyButtonsYBase - keyButtonsLineHeight *
						(i - keyTilesetScrollBar.getVisibleStart());
			keyTilesetButtons[i]->setY(yPos);
		}
	}

	// Map List
	if (keyMapScrollBar.getElementCount() != 0) {
		for (int i = keyMapScrollBar.getVisibleStart();
				i <= keyMapScrollBar.getVisibleEnd(); ++i) {
			if(i >= keyMapButtons.size()) {
				char szBuf[1024]="";
				sprintf(szBuf,"i >= keyMapButtons.size(), i = %d, keyMapButtons.size() = %d",i,(int)keyMapButtons.size());
				throw runtime_error(szBuf);
			}

			keyMapButtons[i]->setY(keyButtonsYBase - keyButtonsLineHeight * (i
					- keyMapScrollBar.getVisibleStart()));
			labelsMap[i]->setY(keyButtonsYBase - keyButtonsLineHeight * (i
					- keyMapScrollBar.getVisibleStart()));
		}
	}

	console.update();
}

void MenuStateMods::keyDown(char key) {
	Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));
	if(key == configKeys.getCharKey("ShowFullConsole")) {
		showFullConsole= true;
	}
}

void MenuStateMods::keyPress(char c) {
}

void MenuStateMods::keyUp(char key) {
	Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));
	if(key== configKeys.getCharKey("ShowFullConsole")) {
		showFullConsole= false;
	}
}

void MenuStateMods::showMessageBox(const string &text, const string &header, bool toggle) {
	if(!toggle){
		mainMessageBox.setEnabled(false);
	}

	if(!mainMessageBox.getEnabled()){
		mainMessageBox.setText(text);
		mainMessageBox.setHeader(header);
		mainMessageBox.setEnabled(true);
	}
	else{
		mainMessageBox.setEnabled(false);
	}
}

void MenuStateMods::FTPClient_CallbackEvent(string itemName,
		FTP_Client_CallbackType type, pair<FTP_Client_ResultType,string> result,void *userdata) {
    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

    Lang &lang= Lang::getInstance();
    if(type == ftp_cct_DownloadProgress) {
        FTPClientCallbackInterface::FtpProgressStats *stats = (FTPClientCallbackInterface::FtpProgressStats *)userdata;
        if(stats != NULL) {
            int fileProgress = 0;
            if(stats->download_total > 0) {
                fileProgress = ((stats->download_now / stats->download_total) * 100.0);
            }
            //if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Got FTP Callback for [%s] current file [%s] fileProgress = %d [now = %f, total = %f]\n",itemName.c_str(),stats->currentFilename.c_str(), fileProgress,stats->download_now,stats->download_total);

            MutexSafeWrapper safeMutexFTPProgress((ftpClientThread != NULL ? ftpClientThread->getProgressMutex() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
            pair<int,string> lastProgress = fileFTPProgressList[itemName];
            fileFTPProgressList[itemName] = pair<int,string>(fileProgress,stats->currentFilename);
            safeMutexFTPProgress.ReleaseLock();
        }
    }
    else if(type == ftp_cct_Map) {
        if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Got FTP Callback for [%s] result = %d [%s]\n",itemName.c_str(),result.first,result.second.c_str());

        MutexSafeWrapper safeMutexFTPProgress((ftpClientThread != NULL ? ftpClientThread->getProgressMutex() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
        fileFTPProgressList.erase(itemName);
        safeMutexFTPProgress.ReleaseLock();
        selectedMapName = "";
        buttonInstallMap.setEnabled(true);

        if(result.first == ftp_crt_SUCCESS) {
        	refreshMaps();

            // Clear the CRC file Cache
            Checksum::clearFileCache();
            //lastCheckedCRCMapValue = -1;
    		Checksum checksum;
    		string file = Map::getMapPath(itemName,"",false);
    		//console.addLine("Checking map CRC [" + file + "]");
    		checksum.addFile(file);
    		uint32 CRCMapValue = checksum.getSum();

            char szMsg[1024]="";
//            if(lang.hasString("DataMissingMapSuccessDownload") == true) {
//            	sprintf(szMsg,lang.get("DataMissingMapSuccessDownload").c_str(),getHumanPlayerName().c_str(),gameSettings->getMap().c_str());
//            }
//            else {
            	sprintf(szMsg,"SUCCESSFULLY downloaded the map: %s",itemName.c_str());
//            }
            //clientInterface->sendTextMessage(szMsg,-1, true);
            console.addLine(szMsg,true);
        }
        else {
            curl_version_info_data *curlVersion= curl_version_info(CURLVERSION_NOW);

            char szMsg[1024]="";
//            if(lang.hasString("DataMissingMapFailDownload") == true) {
//            	sprintf(szMsg,lang.get("DataMissingMapFailDownload").c_str(),getHumanPlayerName().c_str(),gameSettings->getMap().c_str(),curlVersion->version);
//            }
//            else {
            	sprintf(szMsg,"FAILED to download the map: [%s] using CURL version [%s] [%s]",itemName.c_str(),curlVersion->version,result.second.c_str());
//            }
            //clientInterface->sendTextMessage(szMsg,-1, true);
            console.addLine(szMsg,true);
        }
    }
    else if(type == ftp_cct_Tileset) {
    	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Got FTP Callback for [%s] result = %d [%s]\n",itemName.c_str(),result.first,result.second.c_str());

        MutexSafeWrapper safeMutexFTPProgress((ftpClientThread != NULL ? ftpClientThread->getProgressMutex() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
        fileFTPProgressList.erase(itemName);
        safeMutexFTPProgress.ReleaseLock(true);

        selectedTilesetName = "";
        buttonInstallTileset.setEnabled(true);

        if(result.first == ftp_crt_SUCCESS) {
        	refreshTilesets();

            char szMsg[1024]="";
//            if(lang.hasString("DataMissingTilesetSuccessDownload") == true) {
//            	sprintf(szMsg,lang.get("DataMissingTilesetSuccessDownload").c_str(),getHumanPlayerName().c_str(),gameSettings->getTileset().c_str());
//            }
//            else {
           	sprintf(szMsg,"SUCCESSFULLY downloaded the tileset: %s",itemName.c_str());
//            }
           	console.addLine(szMsg,true);

            // START
            // Clear the CRC Cache if it is populated
            //
            // Clear the CRC file Cache
            safeMutexFTPProgress.Lock();
            Checksum::clearFileCache();

            vector<string> paths        = Config::getInstance().getPathListForType(ptTilesets);
            string pathSearchString     = string("/") + itemName + string("/*");
            const string filterFileExt  = ".xml";
            clearFolderTreeContentsCheckSum(paths, pathSearchString, filterFileExt);
            clearFolderTreeContentsCheckSumList(paths, pathSearchString, filterFileExt);

            // Refresh CRC
            Config &config = Config::getInstance();
            uint32 CRCTilesetValue = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTilesets,""), string("/") + itemName + string("/*"), ".xml", NULL);

            safeMutexFTPProgress.ReleaseLock();
            // END

            // Reload tilesets for the UI
            //findDirs(Config::getInstance().getPathListForType(ptTilesets), tilesetFiles);
        }
        else {
            curl_version_info_data *curlVersion= curl_version_info(CURLVERSION_NOW);

            char szMsg[1024]="";
//            if(lang.hasString("DataMissingTilesetFailDownload") == true) {
//            	sprintf(szMsg,lang.get("DataMissingTilesetFailDownload").c_str(),getHumanPlayerName().c_str(),gameSettings->getTileset().c_str(),curlVersion->version);
//            }
//            else {
           	sprintf(szMsg,"FAILED to download the tileset: [%s] using CURL version [%s] [%s]",itemName.c_str(),curlVersion->version,result.second.c_str());
//            }
           	console.addLine(szMsg,true);
        }
    }
    else if(type == ftp_cct_Techtree) {
    	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Got FTP Callback for [%s] result = %d [%s]\n",itemName.c_str(),result.first,result.second.c_str());

        MutexSafeWrapper safeMutexFTPProgress((ftpClientThread != NULL ? ftpClientThread->getProgressMutex() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
        fileFTPProgressList.erase(itemName);
        safeMutexFTPProgress.ReleaseLock(true);

        selectedTechName = "";
        buttonInstallTech.setEnabled(true);

        if(result.first == ftp_crt_SUCCESS) {
        	refreshTechs();

            char szMsg[1024]="";
//            if(lang.hasString("DataMissingTechtreeSuccessDownload") == true) {
//            	sprintf(szMsg,lang.get("DataMissingTechtreeSuccessDownload").c_str(),getHumanPlayerName().c_str(),gameSettings->getTech().c_str());
//            }
//            else {
           	sprintf(szMsg,"SUCCESSFULLY downloaded the techtree: %s",itemName.c_str());
//            }
           	console.addLine(szMsg,true);

            // START
            // Clear the CRC Cache if it is populated
            //
            // Clear the CRC file Cache
            safeMutexFTPProgress.Lock();
            Checksum::clearFileCache();

            vector<string> paths        = Config::getInstance().getPathListForType(ptTechs);
            string pathSearchString     = string("/") + itemName + string("/*");
            const string filterFileExt  = ".xml";
            clearFolderTreeContentsCheckSum(paths, pathSearchString, filterFileExt);
            clearFolderTreeContentsCheckSumList(paths, pathSearchString, filterFileExt);

            // Refresh CRC
            Config &config = Config::getInstance();
            uint32 CRCTechtreeValue = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTechs,""), string("/") + itemName + string("/*"), ".xml", NULL);

            safeMutexFTPProgress.ReleaseLock();
            // END

            // Reload tilesets for the UI
            //findDirs(Config::getInstance().getPathListForType(ptTechs), techTreeFiles);
        }
        else {
            curl_version_info_data *curlVersion= curl_version_info(CURLVERSION_NOW);

            char szMsg[1024]="";
//            if(lang.hasString("DataMissingTechtreeFailDownload") == true) {
//            	sprintf(szMsg,lang.get("DataMissingTechtreeFailDownload").c_str(),getHumanPlayerName().c_str(),gameSettings->getTech().c_str(),curlVersion->version);
//            }
//            else {
           	sprintf(szMsg,"FAILED to download the techtree: [%s] using CURL version [%s] [%s]",itemName.c_str(),curlVersion->version,result.second.c_str());
//            }
           	console.addLine(szMsg,true);
        }
    }
}

}}//end namespace
