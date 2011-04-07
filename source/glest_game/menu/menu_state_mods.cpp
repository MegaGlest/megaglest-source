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
// Need the include below for vc++ 2010 because Microsoft messed up their STL!
#include <iterator>
#include "leak_dumper.h"


namespace Glest{ namespace Game{

using namespace Shared::Util;

struct FormatString {
	void operator()(string &s) {
		s = formatString(s);
	}
};



// ===============================
// 	class ModInfo
// ===============================

ModInfo::ModInfo() {
	name		= "";
	url			= "";
	imageUrl	= "";
	description	= "";
	count		= "";
	crc			= "";
	type 		= mt_None;
}



// =====================================================
// 	class MenuStateConnectedGame
// =====================================================

MenuStateMods::MenuStateMods(Program *program, MainMenu *mainMenu) :
	MenuState(program, mainMenu, "mods") {

	containerName = "Mods";
	Lang &lang= Lang::getInstance();
	Config &config = Config::getInstance();

	modPreviewImage			= NULL;
	displayModPreviewImage	= false;

	ftpClientThread 		= NULL;
	selectedTechName		= "";
	selectedTilesetName		= "";
	selectedMapName 		= "";
	selectedScenarioName	= "";
	showFullConsole			= false;
	keyButtonsLineHeight	= 20;
	keyButtonsHeight		= 20;
	keyButtonsWidth			= 200;
	scrollListsYPos 		= 700;
	listBoxLength 			= 200;
	keyButtonsYBase			= scrollListsYPos;
	keyButtonsToRender		= listBoxLength / keyButtonsLineHeight;
	labelWidth				= 5;

	int installButtonYPos = scrollListsYPos-listBoxLength-20;

	int returnLineY = 80;

	//create
	techInfoXPos = 10;
	keyTechScrollBarTitle1.registerGraphicComponent(containerName,"keyTechScrollBarTitle1");
	keyTechScrollBarTitle1.init(techInfoXPos,scrollListsYPos + 20,labelWidth,20);
	keyTechScrollBarTitle1.setText(lang.get("TechTitle1"));
	keyTechScrollBarTitle1.setFont(CoreData::getInstance().getMenuFontNormal());
	keyTechScrollBarTitle2.registerGraphicComponent(containerName,"keyTechScrollBarTitle2");
	keyTechScrollBarTitle2.init(techInfoXPos + 200,scrollListsYPos + 20,labelWidth,20);
	keyTechScrollBarTitle2.setText(lang.get("TechTitle2"));
	keyTechScrollBarTitle2.setFont(CoreData::getInstance().getMenuFontNormal());

	mapInfoXPos = 270;
	keyMapScrollBarTitle1.registerGraphicComponent(containerName,"keyMapScrollBarTitle1");
	keyMapScrollBarTitle1.init(mapInfoXPos,scrollListsYPos + 20,labelWidth,20);
	keyMapScrollBarTitle1.setText(lang.get("MapTitle1"));
	keyMapScrollBarTitle1.setFont(CoreData::getInstance().getMenuFontNormal());
	keyMapScrollBarTitle2.registerGraphicComponent(containerName,"keyMapScrollBarTitle2");
	keyMapScrollBarTitle2.init(mapInfoXPos + 200,scrollListsYPos + 20,labelWidth,20);
	keyMapScrollBarTitle2.setText(lang.get("MapTitle2"));
	keyMapScrollBarTitle2.setFont(CoreData::getInstance().getMenuFontNormal());

	tilesetInfoXPos = 530;
	keyTilesetScrollBarTitle1.registerGraphicComponent(containerName,"keyTilesetScrollBarTitle1");
	keyTilesetScrollBarTitle1.init(tilesetInfoXPos,scrollListsYPos + 20,labelWidth,20);
	keyTilesetScrollBarTitle1.setText(lang.get("TilesetTitle1"));
	keyTilesetScrollBarTitle1.setFont(CoreData::getInstance().getMenuFontNormal());

	scenarioInfoXPos = 760;
	keyScenarioScrollBarTitle1.registerGraphicComponent(containerName,"keyScenarioScrollBarTitle1");
	keyScenarioScrollBarTitle1.init(scenarioInfoXPos,scrollListsYPos + 20,labelWidth,20);
	keyScenarioScrollBarTitle1.setText(lang.get("ScenarioTitle1"));
	keyScenarioScrollBarTitle1.setFont(CoreData::getInstance().getMenuFontNormal());

	mainMessageBoxState = ftpmsg_None;
    mainMessageBox.registerGraphicComponent(containerName,"mainMessageBox");
	mainMessageBox.init(lang.get("Yes"),lang.get("No"));
	mainMessageBox.setEnabled(false);


	lineHorizontal.init(0,installButtonYPos-60);
	lineVertical.init(500,returnLineY, 5,  installButtonYPos-60-returnLineY);
	lineVertical.setHorizontal(false);
	lineReturn.init(0, returnLineY);

	modDescrLabel.registerGraphicComponent(containerName,"modDescrLabel");
	modDescrLabel.init(50,installButtonYPos-60 - 20,450,20);
	modDescrLabel.setText("description is empty");

	buttonReturn.registerGraphicComponent(containerName,"buttonReturn");
	buttonReturn.init(450, returnLineY - 40, 125);
	buttonReturn.setText(lang.get("Return"));

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

	buttonInstallScenario.registerGraphicComponent(containerName,"buttonInstallScenario");
	buttonInstallScenario.init(scenarioInfoXPos + 20, installButtonYPos, 125);
	buttonInstallScenario.setText(lang.get("Install"));
	buttonRemoveScenario.registerGraphicComponent(containerName,"buttonRemoveScenario");
	buttonRemoveScenario.init(scenarioInfoXPos + 20, installButtonYPos-30, 125);
	buttonRemoveScenario.setText(lang.get("Remove"));

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	keyTilesetScrollBar.init(tilesetInfoXPos + keyButtonsWidth,scrollListsYPos-listBoxLength+keyButtonsLineHeight,false,200,20);
	keyTilesetScrollBar.setLength(listBoxLength);
	keyTilesetScrollBar.setElementCount(0);
	keyTilesetScrollBar.setVisibleSize(keyButtonsToRender);
	keyTilesetScrollBar.setVisibleStart(0);

	keyTechScrollBar.init(techInfoXPos + keyButtonsWidth + labelWidth + 20,scrollListsYPos-listBoxLength+keyButtonsLineHeight,false,200,20);
	keyTechScrollBar.setLength(listBoxLength);
	keyTechScrollBar.setElementCount(0);
	keyTechScrollBar.setVisibleSize(keyButtonsToRender);
	keyTechScrollBar.setVisibleStart(0);

	keyMapScrollBar.init(mapInfoXPos + keyButtonsWidth + labelWidth + 20,scrollListsYPos-listBoxLength+keyButtonsLineHeight,false,200,20);
	keyMapScrollBar.setLength(listBoxLength);
	keyMapScrollBar.setElementCount(0);
	keyMapScrollBar.setVisibleSize(keyButtonsToRender);
	keyMapScrollBar.setVisibleStart(0);

	keyScenarioScrollBar.init(scenarioInfoXPos + keyButtonsWidth,scrollListsYPos-listBoxLength+keyButtonsLineHeight,false,200,20);
	keyScenarioScrollBar.setLength(listBoxLength);
	keyScenarioScrollBar.setElementCount(0);
	keyScenarioScrollBar.setVisibleSize(keyButtonsToRender);
	keyScenarioScrollBar.setVisibleStart(0);

	GraphicComponent::applyAllCustomProperties(containerName);

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
	// Start http meta data thread
	modHttpServerThread = new SimpleTaskThread(this,0,200);
	modHttpServerThread->setUniqueID(__FILE__);
	modHttpServerThread->start();

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

	// Setup File Transfer thread
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

    std::pair<string,string> scenariosPath;
    vector<string> scenariosList = Config::getInstance().getPathListForType(ptScenarios);
    if(scenariosList.size() > 0) {
    	scenariosPath.first = scenariosList[0];
        if(scenariosList.size() > 1) {
        	scenariosPath.second = scenariosList[1];
        }
    }

	string fileArchiveExtension = config.getString("FileArchiveExtension","");
	string fileArchiveExtractCommand = config.getString("FileArchiveExtractCommand","");
	string fileArchiveExtractCommandParameters = config.getString("FileArchiveExtractCommandParameters","");

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

	ftpClientThread = new FTPClientThread(-1,"",
			mapsPath,tilesetsPath,techtreesPath,scenariosPath,
			this,fileArchiveExtension,fileArchiveExtractCommand,
			fileArchiveExtractCommandParameters);
	ftpClientThread->start();


	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void MenuStateMods::simpleTask(BaseThread *callingThread) {
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

    MutexSafeWrapper safeMutexThreadOwner(callingThread->getMutexThreadOwnerValid(),string(__FILE__) + "_" + intToStr(__LINE__));
    if(callingThread->getQuitStatus() == true || safeMutexThreadOwner.isValidMutex() == false) {
    	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
        return;
    }

    if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

    Lang &lang= Lang::getInstance();
    Config &config = Config::getInstance();
    string fileArchiveExtractCommand = config.getString("FileArchiveExtractCommand","");
	bool findArchive = executeShellCommand(fileArchiveExtractCommand);
	if(findArchive == false) {
		mainMessageBoxState = ftpmsg_None;
		mainMessageBox.init(lang.get("Ok"));
		showMessageBox(lang.get("ModRequires7z"), lang.get("Notice"), true);
	}

	std::string techsMetaData = "";
	std::string tilesetsMetaData = "";
	std::string mapsMetaData = "";
	std::string scenariosMetaData = "";

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(config.getString("Masterserver","") != "") {
		string baseURL = config.getString("Masterserver");

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d] About to call first http url, base [%s]..\n",__FILE__,__FUNCTION__,__LINE__,baseURL.c_str());

		CURL *handle = SystemFlags::initHTTP();
		CURLcode curlResult = CURLE_OK;
		techsMetaData = SystemFlags::getHTTP(baseURL + "showTechsForGlest.php",handle,-1,&curlResult);
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("techsMetaData [%s] curlResult = %d\n",techsMetaData.c_str(),curlResult);

	    if(callingThread->getQuitStatus() == true || safeMutexThreadOwner.isValidMutex() == false) {
	    	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
	        return;
	    }

	    if(curlResult != CURLE_OK) {
			string curlError = curl_easy_strerror(curlResult);
			char szBuf[1024]="";
			sprintf(szBuf,lang.get("ModErrorGettingServerData").c_str(),curlError.c_str());
			console.addLine(string("#1 ") + szBuf,true);
	    }

		if(curlResult == CURLE_OK ||
			(curlResult != CURLE_COULDNT_RESOLVE_HOST &&
			 curlResult != CURLE_COULDNT_CONNECT)) {

			tilesetsMetaData = SystemFlags::getHTTP(baseURL + "showTilesetsForGlest.php",handle,-1,&curlResult);
			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("tilesetsMetaData [%s]\n",tilesetsMetaData.c_str());

		    if(callingThread->getQuitStatus() == true || safeMutexThreadOwner.isValidMutex() == false) {
		    	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
		        return;
		    }

		    if(curlResult != CURLE_OK) {
				string curlError = curl_easy_strerror(curlResult);
				char szBuf[1024]="";
				sprintf(szBuf,lang.get("ModErrorGettingServerData").c_str(),curlError.c_str());
				console.addLine(string("#2 ") + szBuf,true);
		    }

			mapsMetaData = SystemFlags::getHTTP(baseURL + "showMapsForGlest.php",handle,-1,&curlResult);
			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("mapsMetaData [%s]\n",mapsMetaData.c_str());

		    if(callingThread->getQuitStatus() == true || safeMutexThreadOwner.isValidMutex() == false) {
		    	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
		        return;
		    }

		    if(curlResult != CURLE_OK) {
				string curlError = curl_easy_strerror(curlResult);
				char szBuf[1024]="";
				sprintf(szBuf,lang.get("ModErrorGettingServerData").c_str(),curlError.c_str());
				console.addLine(string("#3 ") + szBuf,true);
		    }

			scenariosMetaData = SystemFlags::getHTTP(baseURL + "showScenariosForGlest.php",handle,-1,&curlResult);
			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("scenariosMetaData [%s]\n",scenariosMetaData.c_str());

		    if(curlResult != CURLE_OK) {
				string curlError = curl_easy_strerror(curlResult);
				char szBuf[1024]="";
				sprintf(szBuf,lang.get("ModErrorGettingServerData").c_str(),curlError.c_str());
				console.addLine(string("#4 ") + szBuf,true);
		    }
		}
		SystemFlags::cleanupHTTP(&handle);
	}
	else {
        console.addLine(lang.get("MasterServerMissing"),true);
	}

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

    if(callingThread->getQuitStatus() == true || safeMutexThreadOwner.isValidMutex() == false) {
    	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
        return;
    }

    if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
	tilesetListRemote.clear();
	Tokenize(tilesetsMetaData,tilesetListRemote,"\n");

	getTilesetsLocalList();
	for(unsigned int i=0; i < tilesetListRemote.size(); i++) {
		string tilesetInfo = tilesetListRemote[i];
		std::vector<std::string> tilesetInfoList;
		Tokenize(tilesetInfo,tilesetInfoList,"|");

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("tilesetInfoList.size() [%d]\n",(int)tilesetInfoList.size());
		if(tilesetInfoList.size() >= 4) {
			ModInfo modinfo;
			modinfo.name = tilesetInfoList[0];
			modinfo.crc = tilesetInfoList[1];
			modinfo.description = tilesetInfoList[2];
			modinfo.url = tilesetInfoList[3];
			modinfo.imageUrl = tilesetInfoList[4];
			modinfo.type = mt_Tileset;

			//bool alreadyHasTileset = (std::find(tilesetFiles.begin(),tilesetFiles.end(),tilesetName) != tilesetFiles.end());
			tilesetCacheList[modinfo.name] = modinfo;

			GraphicButton *button=new GraphicButton();
			button->init(tilesetInfoXPos, keyButtonsYBase, keyButtonsWidth,keyButtonsHeight);
			button->setText(modinfo.name);
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

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

    if(callingThread->getQuitStatus() == true || safeMutexThreadOwner.isValidMutex() == false) {
    	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
        return;
    }

    if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

	techListRemote.clear();
	Tokenize(techsMetaData,techListRemote,"\n");

	getTechsLocalList();
	for(unsigned int i=0; i < techListRemote.size(); i++) {
		string techInfo = techListRemote[i];
		std::vector<std::string> techInfoList;
		Tokenize(techInfo,techInfoList,"|");

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("techInfoList.size() [%d]\n",(int)techInfoList.size());
		if(techInfoList.size() >= 5) {
			ModInfo modinfo;
			modinfo.name = techInfoList[0];
			modinfo.count = techInfoList[1];
			modinfo.crc = techInfoList[2];
			modinfo.description = techInfoList[3];
			modinfo.url = techInfoList[4];
			modinfo.imageUrl = techInfoList[5];
			modinfo.type = mt_Techtree;

			//bool alreadyHasTech = (std::find(techTreeFiles.begin(),techTreeFiles.end(),techName) != techTreeFiles.end());
			techCacheList[modinfo.name] = modinfo;

			GraphicButton *button=new GraphicButton();
			button->init(techInfoXPos, keyButtonsYBase, keyButtonsWidth,keyButtonsHeight);
			button->setText(modinfo.name);
			button->setUseCustomTexture(true);
			button->setCustomTexture(CoreData::getInstance().getCustomTexture());

			//if(alreadyHasTech == true) {
			//	button->setEnabled(false);
			//}
			keyTechButtons.push_back(button);
			GraphicLabel *label=new GraphicLabel();
			label->init(techInfoXPos + keyButtonsWidth+10,keyButtonsYBase,labelWidth,20);
			label->setText(modinfo.count);
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

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

    if(callingThread->getQuitStatus() == true || safeMutexThreadOwner.isValidMutex() == false) {
    	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
        return;
    }

    if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

	mapListRemote.clear();
	Tokenize(mapsMetaData,mapListRemote,"\n");

	getMapsLocalList();
	for(unsigned int i=0; i < mapListRemote.size(); i++) {
		string mapInfo = mapListRemote[i];
		std::vector<std::string> mapInfoList;
		Tokenize(mapInfo,mapInfoList,"|");

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("mapInfoList.size() [%d]\n",(int)mapInfoList.size());
		if(mapInfoList.size() >= 5) {
			ModInfo modinfo;
			modinfo.name = mapInfoList[0];
			modinfo.count = mapInfoList[1];
			modinfo.crc = mapInfoList[2];
			modinfo.description = mapInfoList[3];
			modinfo.url = mapInfoList[4];
			modinfo.imageUrl = mapInfoList[5];
			modinfo.type = mt_Map;

			//bool alreadyHasMap = (std::find(mapFiles.begin(),mapFiles.end(),mapName) != mapFiles.end());
			mapCacheList[modinfo.name] = modinfo;

			GraphicButton *button=new GraphicButton();
			button->init(mapInfoXPos, keyButtonsYBase, keyButtonsWidth,keyButtonsHeight);
			button->setText(modinfo.name);
			button->setUseCustomTexture(true);
			button->setCustomTexture(CoreData::getInstance().getCustomTexture());
			keyMapButtons.push_back(button);

			GraphicLabel *label=new GraphicLabel();
			label->init(mapInfoXPos + keyButtonsWidth + 10,keyButtonsYBase,labelWidth,20);
			label->setText(modinfo.count);
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

    if(callingThread->getQuitStatus() == true || safeMutexThreadOwner.isValidMutex() == false) {
    	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
        return;
    }

    if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);


	scenarioListRemote.clear();
	Tokenize(scenariosMetaData,scenarioListRemote,"\n");

	getScenariosLocalList();
	for(unsigned int i=0; i < scenarioListRemote.size(); i++) {
		string scenarioInfo = scenarioListRemote[i];
		std::vector<std::string> scenarioInfoList;
		Tokenize(scenarioInfo,scenarioInfoList,"|");

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("scenarioInfoList.size() [%d]\n",(int)scenarioInfoList.size());
		if(scenarioInfoList.size() >= 4) {
			ModInfo modinfo;
			modinfo.name = scenarioInfoList[0];
			modinfo.crc = scenarioInfoList[1];
			modinfo.description = scenarioInfoList[2];
			modinfo.url = scenarioInfoList[3];
			modinfo.imageUrl = scenarioInfoList[4];
			modinfo.type = mt_Scenario;

			scenarioCacheList[modinfo.name] = modinfo;

			GraphicButton *button=new GraphicButton();
			button->init(scenarioInfoXPos, keyButtonsYBase, keyButtonsWidth,keyButtonsHeight);
			button->setText(modinfo.name);
			button->setUseCustomTexture(true);
			button->setCustomTexture(CoreData::getInstance().getCustomTexture());
			keyScenarioButtons.push_back(button);
		}
	}
	for(unsigned int i=0; i < scenarioFilesUserData.size(); i++) {
		string scenarioName = scenarioFilesUserData[i];
		bool alreadyHasScenario = (scenarioCacheList.find(scenarioName) != scenarioCacheList.end());
		if(alreadyHasScenario == false) {
			vector<string> scenarioPaths = config.getPathListForType(ptScenarios);
	        string &scenarioPath = scenarioPaths[1];
	        endPathWithSlash(scenarioPath);
	        scenarioPath += scenarioName;

			GraphicButton *button=new GraphicButton();
			button->init(scenarioInfoXPos, keyButtonsYBase, keyButtonsWidth,keyButtonsHeight);
			button->setText(scenarioName);
			button->setUseCustomTexture(true);
			button->setCustomTexture(CoreData::getInstance().getCustomTexture());
			keyScenarioButtons.push_back(button);
		}
	}

    if(callingThread->getQuitStatus() == true || safeMutexThreadOwner.isValidMutex() == false) {
    	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
        return;
    }

    if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

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

	keyScenarioScrollBar.init(scenarioInfoXPos + keyButtonsWidth,scrollListsYPos-listBoxLength+keyButtonsLineHeight,false,200,20);
	keyScenarioScrollBar.setLength(listBoxLength);
	keyScenarioScrollBar.setElementCount(keyScenarioButtons.size());
	keyScenarioScrollBar.setVisibleSize(keyButtonsToRender);
	keyScenarioScrollBar.setVisibleStart(0);

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(modHttpServerThread != NULL) {
		modHttpServerThread->signalQuit();
	}

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
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
			ModInfo modinfo;
			modinfo.name = techInfoList[0];
			modinfo.count = techInfoList[1];
			modinfo.crc = techInfoList[2];
			modinfo.description = techInfoList[3];
			modinfo.url = techInfoList[4];
			modinfo.imageUrl = techInfoList[5];
			modinfo.type = mt_Techtree;
			techCacheList[modinfo.name] = modinfo;
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
			ModInfo modinfo;
			modinfo.name = tilesetInfoList[0];
			modinfo.crc = tilesetInfoList[1];
			modinfo.description = tilesetInfoList[2];
			modinfo.url = tilesetInfoList[3];
			modinfo.imageUrl = tilesetInfoList[4];
			modinfo.type = mt_Tileset;
			tilesetCacheList[modinfo.name] = modinfo;

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
			ModInfo modinfo;
			modinfo.name = mapInfoList[0];
			modinfo.count = mapInfoList[1];
			modinfo.crc = mapInfoList[2];
			modinfo.description = mapInfoList[3];
			modinfo.url = mapInfoList[4];
			modinfo.imageUrl = mapInfoList[5];
			modinfo.type = mt_Map;
			mapCacheList[modinfo.name] = modinfo;
		}
	}
}


void MenuStateMods::getScenariosLocalList() {
	Config &config = Config::getInstance();
	vector<string> results;
	findDirs(config.getPathListForType(ptScenarios), results);
	scenarioFiles = results;

	scenarioFilesUserData.clear();
	if(config.getPathListForType(ptScenarios).size() > 1) {
		string path = config.getPathListForType(ptScenarios)[1];
		endPathWithSlash(path);
		findDirs(path, scenarioFilesUserData, false, false);
	}
}

void MenuStateMods::refreshScenarios() {
	getScenariosLocalList();
	for(int i=0; i < scenarioListRemote.size(); i++) {
		string scenarioInfo = scenarioListRemote[i];
		std::vector<std::string> scenarioInfoList;
		Tokenize(scenarioInfo,scenarioInfoList,"|");
		if(scenarioInfoList.size() >= 4) {
			ModInfo modinfo;
			modinfo.name = scenarioInfoList[0];
			modinfo.crc = scenarioInfoList[1];
			modinfo.description = scenarioInfoList[2];
			modinfo.url = scenarioInfoList[3];
			modinfo.imageUrl = scenarioInfoList[4];
			modinfo.type = mt_Scenario;
		}
	}
}


void MenuStateMods::cleanUp() {
	clearUserButtons();

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
	if(modHttpServerThread != NULL) {
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
		modHttpServerThread->signalQuit();
		//modHttpServerThread->setThreadOwnerValid(false);

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
		if( modHttpServerThread->canShutdown(true) == true &&
			modHttpServerThread->shutdownAndWait() == true) {
			delete modHttpServerThread;
		}
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
		modHttpServerThread = NULL;
	}

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(ftpClientThread != NULL) {
	    ftpClientThread->setCallBackObject(NULL);
	    if(ftpClientThread->shutdownAndWait() == true) {
            delete ftpClientThread;
            ftpClientThread = NULL;
	    }
	    if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}

	cleanupPreviewTexture();
}

MenuStateMods::~MenuStateMods() {
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

	cleanUp();

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
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

	// Scenarios
	while(!keyScenarioButtons.empty()) {
		delete keyScenarioButtons.back();
		keyScenarioButtons.pop_back();
	}
}

void MenuStateMods::mouseClick(int x, int y, MouseButton mouseButton) {

	CoreData &coreData= CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();
	Lang &lang= Lang::getInstance();

	if(buttonReturn.mouseClick(x,y)) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		soundRenderer.playFx(coreData.getClickSoundA());

		if(fileFTPProgressList.size() > 0) {
			mainMessageBoxState = ftpmsg_Quit;
			mainMessageBox.init(lang.get("Yes"),lang.get("No"));
			char szBuf[1024]="";
			sprintf(szBuf,lang.get("ModDownloadInProgressCancelQuestion").c_str(),fileFTPProgressList.size());
			showMessageBox(szBuf, lang.get("Question"), true);
		}
		else {
			cleanUp();
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
			    else if(mainMessageBoxState == ftpmsg_GetScenario) {
			    	mainMessageBoxState = ftpmsg_None;

			    	Config &config = Config::getInstance();
			    	vector<string> scenarioPaths = config.getPathListForType(ptScenarios);
			    	if(scenarioPaths.size() > 1) {
			    		string removeScenario = scenarioPaths[1];
			    		endPathWithSlash(removeScenario);
			    		removeScenario += selectedScenarioName;

			    		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Removing Scenario [%s]\n",removeScenario.c_str());
			    		removeFolder(removeScenario);

			    		bool remoteHasScenario = (scenarioCacheList.find(selectedScenarioName) != scenarioCacheList.end());
			    		if(remoteHasScenario == false) {
							for(unsigned int i = 0; i < keyScenarioButtons.size(); ++i) {
								GraphicButton *button = keyScenarioButtons[i];
								if(button != NULL && button->getText() == selectedScenarioName) {
									delete button;
									keyScenarioButtons.erase(keyScenarioButtons.begin() + i);
									keyScenarioScrollBar.setElementCount(keyScenarioButtons.size());
									break;
								}
							}
			    		}
			    		MutexSafeWrapper safeMutexFTPProgress((ftpClientThread != NULL ? ftpClientThread->getProgressMutex() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
			            safeMutexFTPProgress.Lock();
			            Checksum::clearFileCache();
			            vector<string> paths        = Config::getInstance().getPathListForType(ptScenarios);
			            string pathSearchString     = string("/") + selectedScenarioName + string("/*");
			            const string filterFileExt  = ".xml";
			            clearFolderTreeContentsCheckSum(paths, pathSearchString, filterFileExt);
			            clearFolderTreeContentsCheckSumList(paths, pathSearchString, filterFileExt);
			            safeMutexFTPProgress.ReleaseLock();

			    		selectedScenarioName = "";
			    		refreshScenarios();
			    	}
			    }
			}
		}
	}
	else if(keyTechScrollBar.mouseClick(x, y)) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		soundRenderer.playFx(coreData.getClickSoundB());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
    }
	else if(keyTilesetScrollBar.mouseClick(x, y)) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		soundRenderer.playFx(coreData.getClickSoundB());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
    }
	else if(keyMapScrollBar.mouseClick(x, y)) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		soundRenderer.playFx(coreData.getClickSoundB());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
    }
	else if(keyScenarioScrollBar.mouseClick(x, y)) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		soundRenderer.playFx(coreData.getClickSoundB());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
    }

	else if(buttonInstallTech.mouseClick(x, y) && buttonInstallTech.getEnabled()) {
		soundRenderer.playFx(coreData.getClickSoundB());
		if(selectedTechName != "") {
			bool alreadyHasTech = (std::find(techTreeFiles.begin(),techTreeFiles.end(),selectedTechName) != techTreeFiles.end());
			if(alreadyHasTech == true) {
				mainMessageBoxState = ftpmsg_None;
				mainMessageBox.init(lang.get("Ok"));
				char szBuf[1024]="";
				sprintf(szBuf,lang.get("ModTechAlreadyInstalled").c_str(),selectedTechName.c_str());
				showMessageBox(szBuf, lang.get("Notice"), true);
			}
			else {
				string techName = selectedTechName;
				string techURL = techCacheList[techName].url;
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
			showMessageBox(lang.get("ModSelectTechToInstall"), lang.get("Notice"), true);
		}
	}
	else if(buttonRemoveTech.mouseClick(x, y) && buttonRemoveTech.getEnabled()) {
		soundRenderer.playFx(coreData.getClickSoundB());
		if(selectedTechName != "") {
			bool alreadyHasTech = (std::find(techTreeFiles.begin(),techTreeFiles.end(),selectedTechName) != techTreeFiles.end());
			if(alreadyHasTech == true) {
				mainMessageBoxState = ftpmsg_GetTechtree;

				char szBuf[1024]="";
				sprintf(szBuf,lang.get("ModRemoveTechConfirm").c_str(),selectedTechName.c_str());
				showMessageBox(szBuf, lang.get("Question"), true);
			}
			else {
				mainMessageBoxState = ftpmsg_None;
				mainMessageBox.init(lang.get("Ok"));

				char szBuf[1024]="";
				sprintf(szBuf,lang.get("ModCannotRemoveTechNotInstalled").c_str(),selectedTechName.c_str());
				showMessageBox(szBuf, lang.get("Notice"), true);
			}
		}
		else {
			mainMessageBoxState = ftpmsg_None;
			mainMessageBox.init(lang.get("Ok"));

			showMessageBox(lang.get("ModSelectTechToRemove"), lang.get("Notice"), true);
		}
	}

	else if(buttonInstallTileset.mouseClick(x, y) && buttonInstallTileset.getEnabled()) {
		soundRenderer.playFx(coreData.getClickSoundB());
		if(selectedTilesetName != "") {
			bool alreadyHasTileset = (std::find(tilesetFiles.begin(),tilesetFiles.end(),selectedTilesetName) != tilesetFiles.end());
			if(alreadyHasTileset == true) {
				mainMessageBoxState = ftpmsg_None;
				mainMessageBox.init(lang.get("Ok"));
				char szBuf[1024]="";
				sprintf(szBuf,lang.get("ModTilesetAlreadyInstalled").c_str(),selectedTilesetName.c_str());
				showMessageBox(szBuf, lang.get("Notice"), true);
			}
			else {
				string tilesetName = selectedTilesetName;
				string tilesetURL = tilesetCacheList[tilesetName].url;
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
			showMessageBox(lang.get("ModSelectTilesetToInstall"), lang.get("Notice"), true);
		}
	}
	else if(buttonRemoveTileset.mouseClick(x, y) && buttonRemoveTileset.getEnabled()) {
		soundRenderer.playFx(coreData.getClickSoundB());
		if(selectedTilesetName != "") {
			bool alreadyHasTileset = (std::find(tilesetFiles.begin(),tilesetFiles.end(),selectedTilesetName) != tilesetFiles.end());
			if(alreadyHasTileset == true) {
				mainMessageBoxState = ftpmsg_GetTileset;

				char szBuf[1024]="";
				sprintf(szBuf,lang.get("ModRemoveTilesetConfirm").c_str(),selectedTilesetName.c_str());
				showMessageBox(szBuf, lang.get("Question"), true);
			}
			else {
				mainMessageBoxState = ftpmsg_None;
				mainMessageBox.init(lang.get("Ok"));

				char szBuf[1024]="";
				sprintf(szBuf,lang.get("ModCannotRemoveTilesetNotInstalled").c_str(),selectedTilesetName.c_str());
				showMessageBox(szBuf, lang.get("Notice"), true);
			}
		}
		else {
			mainMessageBoxState = ftpmsg_None;
			mainMessageBox.init(lang.get("Ok"));
			showMessageBox(lang.get("ModSelectTilesetToRemove"), lang.get("Notice"), true);
		}
	}

	else if(buttonInstallMap.mouseClick(x, y) && buttonInstallMap.getEnabled()) {
		soundRenderer.playFx(coreData.getClickSoundB());
		if(selectedMapName != "") {
			bool alreadyHasMap = (std::find(mapFiles.begin(),mapFiles.end(),selectedMapName) != mapFiles.end());
			if(alreadyHasMap == true) {
				mainMessageBoxState = ftpmsg_None;
				mainMessageBox.init(lang.get("Ok"));
				char szBuf[1024]="";
				sprintf(szBuf,lang.get("ModMapAlreadyInstalled").c_str(),selectedMapName.c_str());
				showMessageBox(szBuf, lang.get("Notice"), true);
			}
			else {
				string mapName = selectedMapName;
				string mapURL = mapCacheList[mapName].url;
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
			showMessageBox(lang.get("ModSelectMapToInstall"), lang.get("Notice"), true);
		}
	}
	else if(buttonRemoveMap.mouseClick(x, y) && buttonRemoveMap.getEnabled()) {
		soundRenderer.playFx(coreData.getClickSoundB());
		if(selectedMapName != "") {
			bool alreadyHasMap = (std::find(mapFiles.begin(),mapFiles.end(),selectedMapName) != mapFiles.end());
			if(alreadyHasMap == true) {
				mainMessageBoxState = ftpmsg_GetMap;

				char szBuf[1024]="";
				sprintf(szBuf,lang.get("ModRemoveMapConfirm").c_str(),selectedMapName.c_str());
				showMessageBox(szBuf, lang.get("Question"), true);
			}
			else {
				mainMessageBoxState = ftpmsg_None;
				mainMessageBox.init(lang.get("Ok"));

				char szBuf[1024]="";
				sprintf(szBuf,lang.get("ModCannotRemoveMapNotInstalled").c_str(),selectedMapName.c_str());
				showMessageBox(szBuf, lang.get("Notice"), true);
			}
		}
		else {
			mainMessageBoxState = ftpmsg_None;
			mainMessageBox.init(lang.get("Ok"));
			showMessageBox(lang.get("ModSelectMapToRemove"), lang.get("Notice"), true);
		}
	}

	else if(buttonInstallScenario.mouseClick(x, y) && buttonInstallScenario.getEnabled()) {
		soundRenderer.playFx(coreData.getClickSoundB());
		if(selectedScenarioName != "") {
			bool alreadyHasScenario = (std::find(scenarioFiles.begin(),scenarioFiles.end(),selectedScenarioName) != scenarioFiles.end());
			if(alreadyHasScenario == true) {
				mainMessageBoxState = ftpmsg_None;
				mainMessageBox.init(lang.get("Ok"));
				char szBuf[1024]="";
				sprintf(szBuf,lang.get("ModScenarioAlreadyInstalled").c_str(),selectedScenarioName.c_str());
				showMessageBox(szBuf, lang.get("Notice"), true);
			}
			else {
				string scenarioName = selectedScenarioName;
				string scenarioURL = scenarioCacheList[scenarioName].url;

				//if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d] adding file to download [%s]\n",__FILE__,__FUNCTION__,__LINE__,scenarioURL.c_str());
				ftpClientThread->addScenarioToRequests(scenarioName,scenarioURL);
				MutexSafeWrapper safeMutexFTPProgress((ftpClientThread != NULL ? ftpClientThread->getProgressMutex() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
				fileFTPProgressList[scenarioName] = pair<int,string>(0,"");
				safeMutexFTPProgress.ReleaseLock();
				buttonInstallScenario.setEnabled(false);
			}
		}
		else {
			mainMessageBoxState = ftpmsg_None;
			mainMessageBox.init(lang.get("Ok"));
			showMessageBox(lang.get("ModSelectScenarioToInstall"), lang.get("Notice"), true);
		}
	}
	else if(buttonRemoveScenario.mouseClick(x, y) && buttonRemoveScenario.getEnabled()) {
		soundRenderer.playFx(coreData.getClickSoundB());
		if(selectedScenarioName != "") {
			bool alreadyHasScenario = (std::find(scenarioFiles.begin(),scenarioFiles.end(),selectedScenarioName) != scenarioFiles.end());
			if(alreadyHasScenario == true) {
				mainMessageBoxState = ftpmsg_GetScenario;

				char szBuf[1024]="";
				sprintf(szBuf,lang.get("ModRemoveScenarioConfirm").c_str(),selectedScenarioName.c_str());
				showMessageBox(szBuf, lang.get("Question"), true);
			}
			else {
				mainMessageBoxState = ftpmsg_None;
				mainMessageBox.init(lang.get("Ok"));

				char szBuf[1024]="";
				sprintf(szBuf,lang.get("ModCannotRemoveScenarioNotInstalled").c_str(),selectedScenarioName.c_str());
				showMessageBox(szBuf, lang.get("Notice"), true);
			}
		}
		else {
			mainMessageBoxState = ftpmsg_None;
			mainMessageBox.init(lang.get("Ok"));
			showMessageBox(lang.get("ModSelectScenarioToRemove"), lang.get("Notice"), true);
		}
	}

    else {
    	if(keyMapScrollBar.getElementCount() != 0) {
			for (int i = keyMapScrollBar.getVisibleStart();
					i <= keyMapScrollBar.getVisibleEnd(); ++i) {
				if(keyMapButtons[i]->mouseClick(x, y) && keyMapButtons[i]->getEnabled()) {
					string mapName = keyMapButtons[i]->getText();
					selectedTechName		= "";
					selectedTilesetName		= "";
					selectedMapName 		= "";
					selectedScenarioName	= "";
					if(mapName != "") {
						selectedMapName = mapName;
						showDesription(&mapCacheList[selectedMapName]);
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
					selectedTechName		= "";
					selectedTilesetName		= "";
					selectedMapName 		= "";
					selectedScenarioName	= "";
					if(techName != "") {
						selectedTechName = techName;
						showDesription(&techCacheList[selectedTechName]);
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
					selectedTechName		= "";
					selectedTilesetName		= "";
					selectedMapName 		= "";
					selectedScenarioName	= "";
					if(tilesetName != "") {
						selectedTilesetName = tilesetName;
						showDesription(&tilesetCacheList[selectedTilesetName]);
					}
					break;
				}
			}
		}
    	if(keyScenarioScrollBar.getElementCount() != 0) {
			for (int i = keyScenarioScrollBar.getVisibleStart();
					i <= keyScenarioScrollBar.getVisibleEnd(); ++i) {
				if(keyScenarioButtons[i]->mouseClick(x, y) && keyScenarioButtons[i]->getEnabled()) {
					string scenarioName = keyScenarioButtons[i]->getText();
					selectedTechName		= "";
					selectedTilesetName		= "";
					selectedMapName 		= "";
					selectedScenarioName	= "";
					if(scenarioName != "") {
						selectedScenarioName = scenarioName;
						showDesription(&scenarioCacheList[selectedScenarioName]);
					}
					break;
				}
			}
		}

    }


	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

string MenuStateMods::getPreviewImageFileForMod(const ModInfo *modInfo) {
	string fileName = "";
	if(modInfo->imageUrl != "") {
		Config &config = Config::getInstance();
	    string userData = config.getString("UserData_Root","");
	    if(userData != "") {
	    	endPathWithSlash(userData);
	    }
	    string tempPath = userData + "temp/";
        if(isdir(tempPath.c_str()) == true) {
			fileName = tempPath;
			switch(modInfo->type) {
				case mt_Map:
					fileName += "map_";
					break;
				case mt_Tileset:
					fileName += "tileset_";
					break;
				case mt_Techtree:
					fileName += "tech_";
					break;
				case mt_Scenario:
					fileName += "scenario_";
					break;
			}
			fileName += extractFileFromDirectoryPath(modInfo->imageUrl);
        }
	}
	return fileName;
}

void MenuStateMods::showDesription(const ModInfo *modInfo) {
	displayModPreviewImage = false;
	modInfoSelected = *modInfo;
	modDescrLabel.setText(modInfo->description);

	//printf("### modInfo->imageUrl [%s]\n",modInfo->imageUrl.c_str());

	if(modInfo->imageUrl != "") {
	    string tempImage  = getPreviewImageFileForMod(modInfo);
	    if(tempImage != "" && fileExists(tempImage) == false) {
	    	ftpClientThread->addFileToRequests(tempImage,modInfo->imageUrl);
	    }
	    else {
	    	displayModPreviewImage = true;
	    }
	}
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
	buttonInstallScenario.mouseMove(x, y);
	buttonRemoveScenario.mouseMove(x, y);

    if (ms->get(mbLeft)) {
		keyMapScrollBar.mouseDown(x, y);
		keyTechScrollBar.mouseDown(x, y);
		keyTilesetScrollBar.mouseDown(x, y);
		keyScenarioScrollBar.mouseDown(x, y);
	}
    else {
		keyMapScrollBar.mouseMove(x, y);
		keyTechScrollBar.mouseMove(x, y);
		keyTilesetScrollBar.mouseMove(x, y);
		keyScenarioScrollBar.mouseMove(x, y);
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
    if(keyScenarioScrollBar.getElementCount() !=0) {
    	for(int i = keyScenarioScrollBar.getVisibleStart(); i <= keyScenarioScrollBar.getVisibleEnd(); ++i) {
    		keyScenarioButtons[i]->mouseMove(x, y);
    	}
    }
}

void MenuStateMods::cleanupPreviewTexture() {
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] scenarioLogoTexture [%p]\n",__FILE__,__FUNCTION__,__LINE__,modPreviewImage);

	if(modPreviewImage != NULL) {
		Renderer::getInstance().endTexture(rsGlobal, modPreviewImage, false);
	}
	modPreviewImage = NULL;
}


void MenuStateMods::render() {
	try {
		Renderer &renderer= Renderer::getInstance();

		renderer.renderLine(&lineHorizontal);
		renderer.renderLine(&lineVertical);
		renderer.renderLine(&lineReturn);
		renderer.renderButton(&buttonReturn);

		renderer.renderButton(&buttonInstallTech);
		renderer.renderButton(&buttonRemoveTech);
		renderer.renderButton(&buttonInstallTileset);
		renderer.renderButton(&buttonRemoveTileset);
		renderer.renderButton(&buttonInstallMap);
		renderer.renderButton(&buttonRemoveMap);
		renderer.renderButton(&buttonInstallScenario);
		renderer.renderButton(&buttonRemoveScenario);

		renderer.renderLabel(&modDescrLabel);
		if(displayModPreviewImage == true) {
			if(modPreviewImage == NULL) {
				string tempImage = getPreviewImageFileForMod(&modInfoSelected);

				//printf("### Render tempImage [%s] fileExists(tempImage) = %d\n",tempImage.c_str(),fileExists(tempImage));

				if(tempImage != "" && fileExists(tempImage) == true) {
					cleanupPreviewTexture();
					modPreviewImage = Renderer::findFactionLogoTexture(tempImage);
				}
			}
			if(modPreviewImage != NULL) {
				renderer.renderTextureQuad(508,90,485,325,modPreviewImage,1.0f);
			}
		}

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
					//Vec4f fontColor=Vec4f(1.0f, 0.0f, 0.0f, 0.75f);
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

		// Render Scenario List
		renderer.renderLabel(&keyScenarioScrollBarTitle1);
		if(keyScenarioScrollBar.getElementCount() != 0) {
			for(int i = keyScenarioScrollBar.getVisibleStart();
					i <= keyScenarioScrollBar.getVisibleEnd(); ++i) {
				if(i >= keyScenarioButtons.size()) {
					char szBuf[1024]="";
					sprintf(szBuf,"i >= keyScenarioButtons.size(), i = %d keyScenarioButtons.size() = %d",i,(int)keyScenarioButtons.size());
					throw runtime_error(szBuf);
				}
				bool alreadyHasScenario = (std::find(scenarioFiles.begin(),scenarioFiles.end(),keyScenarioButtons[i]->getText()) != scenarioFiles.end());
				if(keyScenarioButtons[i]->getText() == selectedScenarioName) {
					bool lightedOverride = true;
					renderer.renderButton(keyScenarioButtons[i],&WHITE,&lightedOverride);
				}
				else if(alreadyHasScenario == true) {
					Vec4f buttonColor = WHITE;
					buttonColor.w = 0.75f;
					renderer.renderButton(keyScenarioButtons[i],&buttonColor);
				}
				else {
					Vec4f fontColor=Vec4f(200.0f/255.0f, 187.0f/255.0f, 190.0f/255.0f, 0.75f);
					renderer.renderButton(keyScenarioButtons[i],&fontColor);
				}
			}
		}
		renderer.renderScrollBar(&keyScenarioScrollBar);

        MutexSafeWrapper safeMutexFTPProgress((ftpClientThread != NULL ? ftpClientThread->getProgressMutex() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
        if(fileFTPProgressList.size() > 0) {
        	Lang &lang= Lang::getInstance();
            int yLocation = buttonReturn.getY();
            for(std::map<string,pair<int,string> >::iterator iterMap = fileFTPProgressList.begin();
                iterMap != fileFTPProgressList.end(); ++iterMap) {

                string progressLabelPrefix = lang.get("ModDownloading") + " " + iterMap->first + " ";
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

	// Tileset List
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

	// Scenario List
	if (keyScenarioScrollBar.getElementCount() != 0) {
		for (int i = keyScenarioScrollBar.getVisibleStart();
				i <= keyScenarioScrollBar.getVisibleEnd(); ++i) {
			if(i >= keyScenarioButtons.size()) {
				char szBuf[1024]="";
				sprintf(szBuf,"i >= keyScenarioButtons.size(), i = %d, keyScenarioButtons.size() = %d",i,(int)keyScenarioButtons.size());
				throw runtime_error(szBuf);
			}

			int yPos = keyButtonsYBase - keyButtonsLineHeight *
						(i - keyScenarioScrollBar.getVisibleStart());
			keyScenarioButtons[i]->setY(yPos);
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
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

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
    else if(type == ftp_cct_File) {
        if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Got FTP Callback for [%s] result = %d [%s]\n",itemName.c_str(),result.first,result.second.c_str());

        MutexSafeWrapper safeMutexFTPProgress((ftpClientThread != NULL ? ftpClientThread->getProgressMutex() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
        fileFTPProgressList.erase(itemName);
        safeMutexFTPProgress.ReleaseLock();

        //printf("### downloaded file [%s] result = %d\n",itemName.c_str(),result.first);

        if(result.first == ftp_crt_SUCCESS) {
        	displayModPreviewImage = true;
        }
//        else {
//            curl_version_info_data *curlVersion= curl_version_info(CURLVERSION_NOW);
//
//			char szBuf[1024]="";
//			sprintf(szBuf,lang.get("ModDownloadMapFail").c_str(),itemName.c_str(),curlVersion->version,result.second.c_str());
//            console.addLine(szBuf,true);
//        }
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
    		Checksum checksum;
    		string file = Map::getMapPath(itemName,"",false);
    		checksum.addFile(file);
    		uint32 CRCMapValue = checksum.getSum();

			char szBuf[1024]="";
			sprintf(szBuf,lang.get("ModDownloadMapSuccess").c_str(),itemName.c_str());
            console.addLine(szBuf,true);
        }
        else {
            curl_version_info_data *curlVersion= curl_version_info(CURLVERSION_NOW);

			char szBuf[1024]="";
			sprintf(szBuf,lang.get("ModDownloadMapFail").c_str(),itemName.c_str(),curlVersion->version,result.second.c_str());
            console.addLine(szBuf,true);
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

			char szBuf[1024]="";
			sprintf(szBuf,lang.get("ModDownloadTilesetSuccess").c_str(),itemName.c_str());
           	console.addLine(szBuf,true);

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
        }
        else {
            curl_version_info_data *curlVersion= curl_version_info(CURLVERSION_NOW);

			char szBuf[1024]="";
			sprintf(szBuf,lang.get("ModDownloadTilesetFail").c_str(),itemName.c_str(),curlVersion->version,result.second.c_str());
           	console.addLine(szBuf,true);
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

			char szBuf[1024]="";
			sprintf(szBuf,lang.get("ModDownloadTechSuccess").c_str(),itemName.c_str());
           	console.addLine(szBuf,true);

            // START
            // Clear the CRC Cache if it is populated
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
        }
        else {
            curl_version_info_data *curlVersion= curl_version_info(CURLVERSION_NOW);

			char szBuf[1024]="";
			sprintf(szBuf,lang.get("ModDownloadTechFail").c_str(),itemName.c_str(),curlVersion->version,result.second.c_str());
           	console.addLine(szBuf,true);
        }
    }
    else if(type == ftp_cct_Scenario) {
    	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Got FTP Callback for [%s] result = %d [%s]\n",itemName.c_str(),result.first,result.second.c_str());

        MutexSafeWrapper safeMutexFTPProgress((ftpClientThread != NULL ? ftpClientThread->getProgressMutex() : NULL),string(__FILE__) + "_" + intToStr(__LINE__));
        fileFTPProgressList.erase(itemName);
        safeMutexFTPProgress.ReleaseLock(true);

        selectedTilesetName = "";
        buttonInstallTileset.setEnabled(true);

        if(result.first == ftp_crt_SUCCESS) {
        	refreshScenarios();

			char szBuf[1024]="";
			sprintf(szBuf,lang.get("ModDownloadScenarioSuccess").c_str(),itemName.c_str());
           	console.addLine(szBuf,true);

            // START
            // Clear the CRC Cache if it is populated
            //
            // Clear the CRC file Cache
            safeMutexFTPProgress.Lock();
            Checksum::clearFileCache();

            vector<string> paths        = Config::getInstance().getPathListForType(ptScenarios);
            string pathSearchString     = string("/") + itemName + string("/*");
            const string filterFileExt  = ".xml";
            clearFolderTreeContentsCheckSum(paths, pathSearchString, filterFileExt);
            clearFolderTreeContentsCheckSumList(paths, pathSearchString, filterFileExt);

            // Refresh CRC
            Config &config = Config::getInstance();
            uint32 CRCTilesetValue = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptScenarios,""), string("/") + itemName + string("/*"), ".xml", NULL);

            safeMutexFTPProgress.ReleaseLock();
            // END
        }
        else {
            curl_version_info_data *curlVersion= curl_version_info(CURLVERSION_NOW);

			char szBuf[1024]="";
			sprintf(szBuf,lang.get("ModDownloadScenarioFail").c_str(),itemName.c_str(),curlVersion->version,result.second.c_str());
           	console.addLine(szBuf,true);
        }
    }

}

}}//end namespace
