// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Martio Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "menu_state_scenario.h"

#include "renderer.h"
#include "menu_state_new_game.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "menu_state_options.h"
#include "network_manager.h"
#include "config.h"
#include "auto_test.h"
#include "game.h"
#include "megaglest_cegui_manager.h"

#include "leak_dumper.h"

namespace Glest{ namespace Game{

using namespace	::Shared::Xml;

// =====================================================
// 	class MenuStateScenario
// =====================================================

MenuStateScenario::MenuStateScenario(Program *program, MainMenu *mainMenu,
		bool isTutorialMode, const vector<string> &dirList, string autoloadScenarioName) :
    MenuState(program, mainMenu, "scenario")
{
	containerName 				 = "Scenario";
	this->isTutorialMode 		 = isTutorialMode;
	enableScenarioTexturePreview = Config::getInstance().getBool("EnableScenarioTexturePreview","true");
	scenarioLogoTexture			 = NULL;
	previewLoadDelayTimer		 = time(NULL);
	needToLoadTextures			 = true;
	mainMessageBoxState 		 = 0;

	NetworkManager &networkManager= NetworkManager::getInstance();
    try {
        networkManager.init(nrServer);
    }
	catch(const std::exception &ex) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s %d] Error detected:\n%s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);

        mainMessageBoxState = 1;
        showMessageBox( "Error: " + string(ex.what()), "Error detected");
	}

	this->autoloadScenarioName 	= autoloadScenarioName;
	this->dirList 				= dirList;

	vector<string> results;
	findDirs(dirList, results);
    scenarioFiles = results;
//    printf("scenarioFiles count = %d\n",scenarioFiles.size());
//    for(unsigned int index = 0; index < scenarioFiles.size(); ++index) {
//    	string item = scenarioFiles[index];
//    	printf("scenarioFile index = %d file: [%s]\n",index,item.c_str());
//    }

    setupCEGUIWidgets();

	if(results.empty() == true) {
        mainMessageBoxState = 1;
        if(this->isTutorialMode == true) {
        	showMessageBox( "Error: There are no tutorials found to load", "Error detected");
        }
        else {
        	showMessageBox( "Error: There are no scenarios found to load", "Error detected");
        }
	}

    GraphicComponent::applyAllCustomProperties(containerName);
}


void MenuStateScenario::setupCEGUIWidgets() {

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	cegui_manager.unsubscribeEvents(this->containerName);
	CEGUI::Window *ctl = cegui_manager.setCurrentLayout("TutorialMenu.layout",containerName);
	cegui_manager.setControlVisible(ctl,true);

	cegui_manager.setControlEventCallback(containerName,"ComboBoxTutorial",
					cegui_manager.getEventComboboxChangeAccepted(), this);

	cegui_manager.setControlEventCallback(containerName,
			"ButtonPlay", cegui_manager.getEventButtonClicked(), this);
	cegui_manager.setControlEventCallback(containerName,
			"ButtonReturn", cegui_manager.getEventButtonClicked(), this);

	cegui_manager.subscribeMessageBoxEventClicks(containerName, this);
	cegui_manager.subscribeErrorMessageBoxEventClicks(containerName, this);

	setupCEGUIWidgetsText();
}

void MenuStateScenario::setupCEGUIWidgetsText() {

	Lang &lang= Lang::getInstance();
	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();

	cegui_manager.setCurrentLayout("TutorialMenu.layout",containerName);

	if(this->isTutorialMode == true) {
		cegui_manager.setControlText("LabelTutorialTitle",lang.getString("Tutorial","",false,true));
	}
	else {
		cegui_manager.setControlText("LabelTutorialTitle",lang.getString("Scenario","",false,true));
	}

	vector<string> results = scenarioFiles;
	if(results.empty() == true) {
        mainMessageBoxState = 1;
        if(this->isTutorialMode == true) {
        	showMessageBox( "Error: There are no tutorials found to load", "Error detected");
        }
        else {
        	showMessageBox( "Error: There are no scenarios found to load", "Error detected");
        }
	}

	std::map<string,string> scenarioErrors;
	for(unsigned int index = 0; index < results.size(); ++index) {
		results[index] = formatString(results[index]);
	}

	cegui_manager.addItemsToComboBoxControl(
			cegui_manager.getControl("ComboBoxTutorial"), results,false);
	if(results.empty() == false) {
		cegui_manager.setSelectedItemInComboBoxControl(
								cegui_manager.getControl("ComboBoxTutorial"), results[0],false);
	}

    try {
    	int selectedTutorialIndex = cegui_manager.getSelectedItemIndexFromComboBoxControl(cegui_manager.getControl("ComboBoxTutorial"));

    	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] listBoxScenario.getSelectedItemIndex() = %d scenarioFiles.size() = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,selectedTutorialIndex,(int)scenarioFiles.size());

    	if(selectedTutorialIndex >= 0 && selectedTutorialIndex < (int)scenarioFiles.size()) {
    		string scenarioPath = Scenario::getScenarioPath(dirList, scenarioFiles[selectedTutorialIndex]);
    		//printf("scenarioPath [%s]\n",scenarioPath.c_str());

    		loadScenarioInfo(scenarioPath, &scenarioInfo );
    		//labelInfo.setText(scenarioInfo.desc);
    		cegui_manager.setControlText("EditboxTutorialInfo",scenarioInfo.desc);

    		if(scenarioInfo.namei18n != "") {
    			//labelScenarioName.setText(scenarioInfo.namei18n);
    			cegui_manager.setControlText("LabelTutorialName",scenarioInfo.namei18n);

    		}
    		else {
    			//labelScenarioName.setText(listBoxScenario.getSelectedItem());
    			cegui_manager.setControlText("LabelTutorialName",
    					cegui_manager.getSelectedItemFromComboBoxControl(
    							cegui_manager.getControl("ComboBoxTutorial")));

    		}
    	}
    }
	catch(const std::exception &ex) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s %d] Error detected:\n%s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);

        mainMessageBoxState=1;
        showMessageBox( "Error: " + string(ex.what()), "Error detected");
	}

	if(scenarioErrors.empty() == false) {
        mainMessageBoxState = 1;

        string errorMsg = "";
        for(std::map<string,string>::iterator iterMap = scenarioErrors.begin();
        		iterMap != scenarioErrors.end(); ++iterMap) {

        	errorMsg += "scenario: " + iterMap->first + " error text: " + iterMap->second.substr(0,400) + "\n";
        }
        showMessageBox( "Error loading scenario(s): " + errorMsg, "Error detected");
	}

	cegui_manager.setControlText("ButtonPlay",lang.getString("PlayNow","",false,true));
	cegui_manager.setControlText("ButtonReturn",lang.getString("Return","",false,true));
}

void MenuStateScenario::callDelayedCallbacks() {
	if(hasDelayedCallbacks() == true) {
		while(hasDelayedCallbacks() == true) {
			MenuStateScenario::DelayCallbackFunction pCB = delayedCallbackList[0];
			delayedCallbackList.erase(delayedCallbackList.begin());
			bool hasMoreCallbacks = hasDelayedCallbacks();
			(this->*pCB)();
			if(hasMoreCallbacks == false) {
				return;
			}
		}
	}
}

void MenuStateScenario::delayedCallbackFunctionPlay() {
	CoreData &coreData				= CoreData::getInstance();
	SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

	soundRenderer.playFx(coreData.getClickSoundC());
	launchGame();
}

void MenuStateScenario::delayedCallbackFunctionReturn() {
	CoreData &coreData				= CoreData::getInstance();
	SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	cegui_manager.unsubscribeEvents(this->containerName);

	soundRenderer.playFx(coreData.getClickSoundA());
	mainMenu->setState(new MenuStateNewGame(program, mainMenu));
}

bool MenuStateScenario::EventCallback(CEGUI::Window *ctl, std::string name) {

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	if(name == cegui_manager.getEventButtonClicked()) {

		if(cegui_manager.isControlMessageBoxOk(ctl) == true) {

			MenuStateScenario::DelayCallbackFunction pCB = &MenuStateScenario::delayedCallbackFunctionReturn;
			delayedCallbackList.push_back(pCB);

			cegui_manager.hideMessageBox();
			return true;
		}
		else if(cegui_manager.isControlMessageBoxCancel(ctl) == true) {

			CoreData &coreData				= CoreData::getInstance();
			SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

			soundRenderer.playFx(coreData.getClickSoundA());
			cegui_manager.hideMessageBox();
			return true;
		}
		else if(ctl == cegui_manager.getControl("ButtonPlay")) {
			MenuStateScenario::DelayCallbackFunction pCB = &MenuStateScenario::delayedCallbackFunctionPlay;
			delayedCallbackList.push_back(pCB);

			return true;
		}
		else if(ctl == cegui_manager.getControl("ButtonReturn")) {
			MenuStateScenario::DelayCallbackFunction pCB = &MenuStateScenario::delayedCallbackFunctionReturn;
			delayedCallbackList.push_back(pCB);

			return true;
		}
	}
	else if(name == cegui_manager.getEventComboboxChangeAccepted()) {
		if(ctl == cegui_manager.getControl("ComboBoxTutorial")) {

			try {
				int selectedId = cegui_manager.getSelectedItemIdFromComboBoxControl(cegui_manager.getControl("ComboBoxTutorial"));

				if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] listBoxScenario.getSelectedItemIndex() = %d scenarioFiles.size() = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,selectedId,(int)scenarioFiles.size());

				if(selectedId >= 0) {
					if(selectedId < (int)scenarioFiles.size()) {
						loadScenarioInfo(Scenario::getScenarioPath(dirList, scenarioFiles[selectedId]), &scenarioInfo);

			    		cegui_manager.setControlText("EditboxTutorialInfo",scenarioInfo.desc);

			    		if(scenarioInfo.namei18n != "") {
			    			cegui_manager.setControlText("LabelTutorialName",scenarioInfo.namei18n);

			    		}
			    		else {
			    			cegui_manager.setControlText("LabelTutorialName",
			    					cegui_manager.getSelectedItemFromComboBoxControl(
			    							cegui_manager.getControl("ComboBoxTutorial")));
			    		}
					}
				}
			}
			catch(const std::exception &ex) {
				char szBuf[8096]="";
				snprintf(szBuf,8096,"In [%s::%s %d] Error detected:\n%s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
				SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);

				mainMessageBoxState=1;
				showMessageBox( "Error: " + string(ex.what()), "Error detected");
			}

			return true;
		}
	}

	return false;
}


void MenuStateScenario::reloadUI() {

	console.resetFonts();
	GraphicComponent::reloadFontsForRegisterGraphicComponents(containerName);

	setupCEGUIWidgetsText();
}

MenuStateScenario::~MenuStateScenario() {
	cleanupPreviewTexture();
}

void MenuStateScenario::cleanupPreviewTexture() {
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] scenarioLogoTexture [%p]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,scenarioLogoTexture);

	if(scenarioLogoTexture != NULL) {
		Renderer::getInstance().endTexture(rsGlobal, scenarioLogoTexture, false);
	}
	scenarioLogoTexture = NULL;
}

void MenuStateScenario::mouseClick(int x, int y, MouseButton mouseButton) { }

void MenuStateScenario::mouseMove(int x, int y, const MouseState *ms) { }

void MenuStateScenario::render() {

	Renderer &renderer= Renderer::getInstance();
	renderer.renderConsole(&console,false,true);
	if(program != NULL) program->renderProgramMsgBox();
}

void MenuStateScenario::update() {

	if(Config::getInstance().getBool("AutoTest")) {
		AutoTest::getInstance().updateScenario(this);
		return;
	}
	if(this->autoloadScenarioName != "") {
		string scenarioPath = Scenario::getScenarioPath(dirList, this->autoloadScenarioName);

		//if(SystemFlags::VERBOSE_MODE_ENABLED) printf("[%s:%s] Line: %d this->autoloadScenarioName [%s] scenarioPath [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->autoloadScenarioName.c_str(),scenarioPath.c_str());
		printf("[%s:%s] Line: %d this->autoloadScenarioName [%s] scenarioPath [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->autoloadScenarioName.c_str(),scenarioPath.c_str());

		loadScenarioInfo(scenarioPath, &scenarioInfo );
		//if(scenarioInfo.namei18n != "") {
		//	this->autoloadScenarioName = scenarioInfo.namei18n;
		//}
		//else {
		this->autoloadScenarioName = formatString(this->autoloadScenarioName);
		//}

		//if(SystemFlags::VERBOSE_MODE_ENABLED) printf("[%s:%s] Line: %d this->autoloadScenarioName [%s] scenarioPath [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->autoloadScenarioName.c_str(),scenarioPath.c_str());
		printf("[%s:%s] Line: %d this->autoloadScenarioName [%s] scenarioPath [%s] file [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->autoloadScenarioName.c_str(),scenarioPath.c_str(),scenarioInfo.file.c_str());

		//listBoxScenario.setSelectedItem(this->autoloadScenarioName,false);
		MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
		cegui_manager.setSelectedItemInComboBoxControl(
								cegui_manager.getControl("ComboBoxTutorial"), this->autoloadScenarioName,false);
		string selectedItem = cegui_manager.getSelectedItemFromComboBoxControl(cegui_manager.getControl("ComboBoxTutorial"));
		int selectedItemId = cegui_manager.getSelectedItemIdFromComboBoxControl(cegui_manager.getControl("ComboBoxTutorial"));

		if(selectedItem != this->autoloadScenarioName) {
			mainMessageBoxState = 1;
			showMessageBox( "Could not find scenario name: " + this->autoloadScenarioName, "Scenario Missing");
			this->autoloadScenarioName = "";
		}
		else {
			try {
				this->autoloadScenarioName = "";
				if(selectedItemId >= 0 && selectedItemId < (int)scenarioFiles.size()) {

					printf("[%s:%s] Line: %d scenarioFiles[listBoxScenario.getSelectedItemIndex()] [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,scenarioFiles[selectedItemId].c_str());

					loadScenarioInfo(Scenario::getScenarioPath(dirList, scenarioFiles[selectedItemId]), &scenarioInfo);

					printf("[%s:%s] Line: %d scenarioInfo.file [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,scenarioInfo.file.c_str());

		    		cegui_manager.setControlText("EditboxTutorialInfo",scenarioInfo.desc);

		    		if(scenarioInfo.namei18n != "") {
		    			cegui_manager.setControlText("LabelTutorialName",scenarioInfo.namei18n);

		    		}
		    		else {
		    			cegui_manager.setControlText("LabelTutorialName",
		    					cegui_manager.getSelectedItemFromComboBoxControl(
		    							cegui_manager.getControl("ComboBoxTutorial")));
		    		}


					SoundRenderer &soundRenderer= SoundRenderer::getInstance();
					CoreData &coreData= CoreData::getInstance();
					soundRenderer.playFx(coreData.getClickSoundC());
					launchGame();
					return;
				}
			}
			catch(const std::exception &ex) {
				char szBuf[8096]="";
				snprintf(szBuf,8096,"In [%s::%s %d] Error detected:\n%s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
				SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);

				mainMessageBoxState=1;
				showMessageBox( "Error: " + string(ex.what()), "Error detected");
			}
		}
	}

	if(needToLoadTextures) {
		// this delay is done to make it possible to switch faster
		if(difftime(time(NULL), previewLoadDelayTimer) >= 2){
			loadScenarioPreviewTexture();
			needToLoadTextures= false;
		}
	}
	console.update();

	MenuState::update();
}

void MenuStateScenario::launchGame() {
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] scenarioInfo.file [%s] [%s][%s][%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,scenarioInfo.file.c_str(),scenarioInfo.tilesetName.c_str(),scenarioInfo.mapName.c_str(),scenarioInfo.techTreeName.c_str());

	if(scenarioInfo.file != "" && scenarioInfo.tilesetName != "" && scenarioInfo.mapName != "" && scenarioInfo.techTreeName != "") {
		GameSettings gameSettings;
		loadGameSettings(&scenarioInfo, &gameSettings);

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] gameSettings.getScenarioDir() [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,gameSettings.getScenarioDir().c_str());

		const vector<string> pathTechList = Config::getInstance().getPathListForType(ptTechs,gameSettings.getScenarioDir());
		if(TechTree::exists(gameSettings.getTech(), pathTechList) == false) {
			char szBuf[8096]="";
			snprintf(szBuf,8096,"Line ref: %d Error: cannot find techtree [%s]\n",__LINE__,scenarioInfo.techTreeName.c_str());
			SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);

	        mainMessageBoxState=1;
	        showMessageBox( szBuf, "Error detected");

			return;
		}

		MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
		cegui_manager.unsubscribeEvents(this->containerName);
		CEGUI::Window *ctl = cegui_manager.setCurrentLayout("TutorialMenu.layout",containerName);
		cegui_manager.setControlVisible(ctl,false);

		program->setState(new Game(program, &gameSettings, false));
		return;
	}
}

void MenuStateScenario::setScenario(int i) {
	//listBoxScenario.setSelectedItemIndex(i);
	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	cegui_manager.setSelectedItemInComboBoxControl(
							cegui_manager.getControl("ComboBoxTutorial"), i);

	loadScenarioInfo(Scenario::getScenarioPath(dirList, scenarioFiles[i]), &scenarioInfo);
}

void MenuStateScenario::loadScenarioInfo(string file, ScenarioInfo *scenarioInfo) {
	bool isTutorial = Scenario::isGameTutorial(file);

	//printf("[%s:%s] Line: %d file [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,file.c_str());

	Scenario::loadScenarioInfo(file, scenarioInfo, isTutorial);

	cleanupPreviewTexture();
	previewLoadDelayTimer=time(NULL);
	needToLoadTextures=true;
}

void MenuStateScenario::loadScenarioPreviewTexture(){
	if(enableScenarioTexturePreview == true) {

		MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
		int selectedId = cegui_manager.getSelectedItemIdFromComboBoxControl(cegui_manager.getControl("ComboBoxTutorial"));
		if(selectedId >= 0 && selectedId < (int)scenarioFiles.size()) {
			GameSettings gameSettings;
			loadGameSettings(&scenarioInfo, &gameSettings);

			string scenarioLogo 	= "";
			bool loadingImageUsed 	= false;

			Game::extractScenarioLogoFile(&gameSettings, scenarioLogo, loadingImageUsed);

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] scenarioLogo [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,scenarioLogo.c_str());

			if(scenarioLogo != "") {
				cleanupPreviewTexture();
				scenarioLogoTexture = Renderer::findTexture(scenarioLogo);
			}
			else {
				cleanupPreviewTexture();
				scenarioLogoTexture = NULL;
			}

			//printf("Image preview: [%s]\n",(scenarioLogoTexture != NULL ? scenarioLogoTexture->getPath().c_str() : ""));

			cegui_manager.setImageFileForControl("TutorialPreviewImage_" + scenarioLogo,
					(scenarioLogoTexture != NULL ? scenarioLogoTexture->getPath() : ""),
						"TutorialPreview");
		}
	}
}

void MenuStateScenario::loadGameSettings(const ScenarioInfo *scenarioInfo, GameSettings *gameSettings){

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	int selectedId = cegui_manager.getSelectedItemIdFromComboBoxControl(cegui_manager.getControl("ComboBoxTutorial"));
	if(selectedId < 0) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"listBoxScenario.getSelectedItemIndex() < 0, = %d",selectedId);
		throw megaglest_runtime_error(szBuf);
	}
	else if(selectedId >= (int)scenarioFiles.size()) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"listBoxScenario.getSelectedItemIndex() >= scenarioFiles.size(), = [%d][%d]",selectedId,(int)scenarioFiles.size());
		throw megaglest_runtime_error(szBuf);
	}

	Scenario::loadGameSettings(dirList,scenarioInfo, gameSettings, formatString(scenarioFiles[selectedId]));
}

void MenuStateScenario::showMessageBox(const string &text, const string &header, bool okOnly) {
	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	if(cegui_manager.isMessageBoxShowing() == false) {
		MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
		Lang &lang= Lang::getInstance();
		if(okOnly == true) {
			cegui_manager.displayMessageBox(header, text, lang.getString("Ok","",false,true),"");
		}
		else {
			cegui_manager.displayMessageBox(header, text, lang.getString("Yes","",false,true),lang.getString("No","",false,true));
		}
	}
	else {
		cegui_manager.hideMessageBox();
	}
}

void MenuStateScenario::keyDown(SDL_KeyboardEvent key) {
	Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));
	if(isKeyPressed(configKeys.getSDLKey("SaveGUILayout"),key) == true) {
		GraphicComponent::saveAllCustomProperties(containerName);
	}
}

}}//end namespace
