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
	containerName = "Scenario";
	this->isTutorialMode = isTutorialMode;

	enableScenarioTexturePreview = Config::getInstance().getBool("EnableScenarioTexturePreview","true");
	scenarioLogoTexture=NULL;
	previewLoadDelayTimer=time(NULL);
	needToLoadTextures=true;

	Lang &lang= Lang::getInstance();
	NetworkManager &networkManager= NetworkManager::getInstance();
    try {
        networkManager.init(nrServer);
    }
	catch(const std::exception &ex) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s %d] Error detected:\n%s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);

        mainMessageBoxState=1;
        showMessageBox( "Error: " + string(ex.what()), "Error detected", false);
	}

	mainMessageBox.registerGraphicComponent(containerName,"mainMessageBox");
	mainMessageBox.init(lang.getString("Ok"));
	mainMessageBox.setEnabled(false);
	mainMessageBoxState=0;

	this->autoloadScenarioName = autoloadScenarioName;
    vector<string> results;

	this->dirList = dirList;

	int buttonStartY=50;
	int buttonStartX=70;

	buttonReturn.registerGraphicComponent(containerName,"buttonReturn");
    buttonReturn.init(buttonStartX, buttonStartY, 125);
	buttonReturn.setText(lang.getString("Return"));

    buttonPlayNow.registerGraphicComponent(containerName,"buttonPlayNow");
	buttonPlayNow.init(buttonStartX+150, buttonStartY, 125);
	buttonPlayNow.setText(lang.getString("PlayNow"));

	int startY=700;
	int startX=50;

    labelScenario.registerGraphicComponent(containerName,"labelScenario");
	labelScenario.init(startX, startY);

	listBoxScenario.registerGraphicComponent(containerName,"listBoxScenario");
    listBoxScenario.init(startX, startY-30, 290);

	labelScenarioName.registerGraphicComponent(containerName,"labelScenarioName");
	labelScenarioName.init(startX, startY-80);
	labelScenarioName.setFont(CoreData::getInstance().getMenuFontBig());
	labelScenarioName.setFont3D(CoreData::getInstance().getMenuFontBig3D());

	labelInfo.registerGraphicComponent(containerName,"labelInfo");
    labelInfo.init(startX, startY-110);
	labelInfo.setFont(CoreData::getInstance().getMenuFontNormal());
	labelInfo.setFont3D(CoreData::getInstance().getMenuFontNormal3D());

	if(this->isTutorialMode == true) {
		labelScenario.setText(lang.getString("Tutorial"));
	}
	else {
		labelScenario.setText(lang.getString("Scenario"));
	}

    //scenario listbox
	findDirs(dirList, results);
    scenarioFiles = results;
    //printf("scenarioFiles[0] [%s]\n",scenarioFiles[0].c_str());

	if(results.empty() == true) {
        //throw megaglest_runtime_error("There are no scenarios found to load");
        mainMessageBoxState=1;
        if(this->isTutorialMode == true) {
        	showMessageBox( "Error: There are no tutorials found to load", "Error detected", false);
        }
        else {
        	showMessageBox( "Error: There are no scenarios found to load", "Error detected", false);
        }
	}

	std::map<string,string> scenarioErrors;
	for(int i= 0; i < (int)results.size(); ++i){
			results[i] = formatString(results[i]);
	}
    listBoxScenario.setItems(results);

    try {
    	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] listBoxScenario.getSelectedItemIndex() = %d scenarioFiles.size() = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,listBoxScenario.getSelectedItemIndex(),(int)scenarioFiles.size());

    	if(listBoxScenario.getItemCount() > 0 && listBoxScenario.getSelectedItemIndex() >= 0 && listBoxScenario.getSelectedItemIndex() < (int)scenarioFiles.size()) {
    		string scenarioPath = Scenario::getScenarioPath(dirList, scenarioFiles[listBoxScenario.getSelectedItemIndex()]);
    		//printf("scenarioPath [%s]\n",scenarioPath.c_str());

    		loadScenarioInfo(scenarioPath, &scenarioInfo );
    		labelInfo.setText(scenarioInfo.desc);
    		if(scenarioInfo.namei18n != "") {
    			labelScenarioName.setText(scenarioInfo.namei18n);
    		}
    		else {
    			labelScenarioName.setText(listBoxScenario.getSelectedItem());
    		}
    	}

        GraphicComponent::applyAllCustomProperties(containerName);
    }
	catch(const std::exception &ex) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s %d] Error detected:\n%s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);

        mainMessageBoxState=1;
        showMessageBox( "Error: " + string(ex.what()), "Error detected", false);
	}

	if(scenarioErrors.empty() == false) {
        mainMessageBoxState=1;

        string errorMsg = "";
        for(std::map<string,string>::iterator iterMap = scenarioErrors.begin();
        		iterMap != scenarioErrors.end(); ++iterMap) {
        	errorMsg += "scenario: " + iterMap->first + " error text: " + iterMap->second.substr(0,400) + "\n";
        }
        showMessageBox( "Error loading scenario(s): " + errorMsg, "Error detected", false);
	}
}

void MenuStateScenario::reloadUI() {
	Lang &lang= Lang::getInstance();

	console.resetFonts();
	mainMessageBox.init(lang.getString("Ok"));
	labelInfo.setFont(CoreData::getInstance().getMenuFontNormal());
	labelInfo.setFont3D(CoreData::getInstance().getMenuFontNormal3D());

	labelScenarioName.setFont(CoreData::getInstance().getMenuFontNormal());
	labelScenarioName.setFont3D(CoreData::getInstance().getMenuFontNormal3D());

	buttonReturn.setText(lang.getString("Return"));
	buttonPlayNow.setText(lang.getString("PlayNow"));

    labelScenario.setText(lang.getString("Scenario"));

	GraphicComponent::reloadFontsForRegisterGraphicComponents(containerName);
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

void MenuStateScenario::mouseClick(int x, int y, MouseButton mouseButton) {
	CoreData &coreData= CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();
	string advanceToItemStartingWith = "";

	if(mainMessageBox.getEnabled()){
		int button= 0;
		if(mainMessageBox.mouseClick(x, y, button)) {
			soundRenderer.playFx(coreData.getClickSoundA());
			if(button==0) {
				mainMessageBox.setEnabled(false);

				if(scenarioFiles.empty() == true && mainMessageBoxState ==1) {
					mainMenu->setState(new MenuStateNewGame(program, mainMenu));
					return;
				}
			}
		}
		return;
	}
    else {
    	if(::Shared::Platform::Window::isKeyStateModPressed(KMOD_SHIFT) == true) {
    		const wchar_t lastKey = ::Shared::Platform::Window::extractLastKeyPressed();
//        		xxx:
//        		string hehe=lastKey;
//        		printf("lastKey = %d [%c] '%s'\n",lastKey,lastKey,hehe);
    		advanceToItemStartingWith =  lastKey;
    	}
    }

	if(buttonReturn.mouseClick(x,y)){
		soundRenderer.playFx(coreData.getClickSoundA());
		mainMenu->setState(new MenuStateNewGame(program, mainMenu));
		return;
    }
	else if(buttonPlayNow.mouseClick(x,y)){
		soundRenderer.playFx(coreData.getClickSoundC());
		launchGame();
		return;
	}
    else if(listBoxScenario.mouseClick(x, y,advanceToItemStartingWith)){
        try {
        	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] listBoxScenario.getSelectedItemIndex() = %d scenarioFiles.size() = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,listBoxScenario.getSelectedItemIndex(),(int)scenarioFiles.size());

        	if(listBoxScenario.getItemCount() > 0 && listBoxScenario.getSelectedItemIndex() >= 0 && listBoxScenario.getSelectedItemIndex() < (int)scenarioFiles.size()) {
        		loadScenarioInfo(Scenario::getScenarioPath(dirList, scenarioFiles[listBoxScenario.getSelectedItemIndex()]), &scenarioInfo);
        		labelInfo.setText(scenarioInfo.desc);
        		if(scenarioInfo.namei18n != "") {
        			labelScenarioName.setText(scenarioInfo.namei18n);
        		}
        		else {
        			labelScenarioName.setText(listBoxScenario.getSelectedItem());
        		}
        	}
        }
        catch(const std::exception &ex) {
            char szBuf[8096]="";
            snprintf(szBuf,8096,"In [%s::%s %d] Error detected:\n%s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
            SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
            if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);

            mainMessageBoxState=1;
            showMessageBox( "Error: " + string(ex.what()), "Error detected", false);
        }
	}
}

void MenuStateScenario::mouseMove(int x, int y, const MouseState *ms){

	if (mainMessageBox.getEnabled()) {
		mainMessageBox.mouseMove(x, y);
	}

	listBoxScenario.mouseMove(x, y);

	buttonReturn.mouseMove(x, y);
	buttonPlayNow.mouseMove(x, y);
}

void MenuStateScenario::render(){

	Renderer &renderer= Renderer::getInstance();

	if(scenarioLogoTexture != NULL) {
		renderer.renderTextureQuad(450,200,533,400,scenarioLogoTexture,1.0f);
		//renderer.renderBackground(scenarioLogoTexture);
	}

	if(mainMessageBox.getEnabled()) {
		renderer.renderMessageBox(&mainMessageBox);
	}
	else {
		renderer.renderLabel(&labelInfo);
		renderer.renderLabel(&labelScenarioName);

		renderer.renderLabel(&labelScenario);
		renderer.renderListBox(&listBoxScenario);

		renderer.renderButton(&buttonReturn);
		renderer.renderButton(&buttonPlayNow);
	}
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

		//printf("[%s:%s] Line: %d this->autoloadScenarioName [%s] scenarioPath [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->autoloadScenarioName.c_str(),scenarioPath.c_str());

		loadScenarioInfo(scenarioPath, &scenarioInfo );
		if(scenarioInfo.namei18n != "") {
			this->autoloadScenarioName = scenarioInfo.namei18n;
		}
		else {
			this->autoloadScenarioName = formatString(this->autoloadScenarioName);
		}

		listBoxScenario.setSelectedItem(this->autoloadScenarioName,false);

		if(listBoxScenario.getSelectedItem() != this->autoloadScenarioName) {
			mainMessageBoxState=1;
			showMessageBox( "Could not find scenario name: " + this->autoloadScenarioName, "Scenario Missing", false);
			this->autoloadScenarioName = "";
		}
		else {
			try {
				this->autoloadScenarioName = "";
				if(listBoxScenario.getItemCount() > 0 && listBoxScenario.getSelectedItemIndex() >= 0 && listBoxScenario.getSelectedItemIndex() < (int)scenarioFiles.size()) {
					loadScenarioInfo(Scenario::getScenarioPath(dirList, scenarioFiles[listBoxScenario.getSelectedItemIndex()]), &scenarioInfo);
					labelInfo.setText(scenarioInfo.desc);
	        		if(scenarioInfo.namei18n != "") {
	        			labelScenarioName.setText(scenarioInfo.namei18n);
	        		}
	        		else {
	        			labelScenarioName.setText(listBoxScenario.getSelectedItem());
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
				showMessageBox( "Error: " + string(ex.what()), "Error detected", false);
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
	        showMessageBox( szBuf, "Error detected", false);

			return;
		}
		program->setState(new Game(program, &gameSettings, false));
		return;
	}
}

void MenuStateScenario::setScenario(int i) {
	listBoxScenario.setSelectedItemIndex(i);
	loadScenarioInfo(Scenario::getScenarioPath(dirList, scenarioFiles[listBoxScenario.getSelectedItemIndex()]), &scenarioInfo);
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
		//if(listBoxScenario.getSelectedItemIndex() >= 0) {
		if(listBoxScenario.getItemCount() > 0 && listBoxScenario.getSelectedItemIndex() >= 0 && listBoxScenario.getSelectedItemIndex() < (int)scenarioFiles.size()) {
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
		}
	}
}

void MenuStateScenario::loadGameSettings(const ScenarioInfo *scenarioInfo, GameSettings *gameSettings){
	if(listBoxScenario.getSelectedItemIndex() < 0) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"listBoxScenario.getSelectedItemIndex() < 0, = %d",listBoxScenario.getSelectedItemIndex());
		throw megaglest_runtime_error(szBuf);
	}
	else if(listBoxScenario.getSelectedItemIndex() >= (int)scenarioFiles.size()) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"listBoxScenario.getSelectedItemIndex() >= scenarioFiles.size(), = [%d][%d]",listBoxScenario.getSelectedItemIndex(),(int)scenarioFiles.size());
		throw megaglest_runtime_error(szBuf);
	}

	Scenario::loadGameSettings(dirList,scenarioInfo, gameSettings, formatString(scenarioFiles[listBoxScenario.getSelectedItemIndex()]));
}

void MenuStateScenario::showMessageBox(const string &text, const string &header, bool toggle){
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

void MenuStateScenario::keyDown(SDL_KeyboardEvent key) {
	Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));
	if(isKeyPressed(configKeys.getSDLKey("SaveGUILayout"),key) == true) {
		GraphicComponent::saveAllCustomProperties(containerName);
	}
}

}}//end namespace
