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

using namespace	Shared::Xml;

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
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s %d] Error detected:\n%s\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);

        mainMessageBoxState=1;
        showMessageBox( "Error: " + string(ex.what()), "Error detected", false);
	}

	mainMessageBox.registerGraphicComponent(containerName,"mainMessageBox");
	mainMessageBox.init(lang.get("Ok"));
	mainMessageBox.setEnabled(false);
	mainMessageBoxState=0;

	this->autoloadScenarioName = autoloadScenarioName;
    vector<string> results;

	this->dirList = dirList;

	int startY=100;
	int startX=350;

	labelInfo.registerGraphicComponent(containerName,"labelInfo");
    labelInfo.init(startX, startY+130);
	labelInfo.setFont(CoreData::getInstance().getMenuFontNormal());
	labelInfo.setFont3D(CoreData::getInstance().getMenuFontNormal3D());

	buttonReturn.registerGraphicComponent(containerName,"buttonReturn");
    buttonReturn.init(startX, startY, 125);

    buttonPlayNow.registerGraphicComponent(containerName,"buttonPlayNow");
	buttonPlayNow.init(startX+175, startY, 125);

	listBoxScenario.registerGraphicComponent(containerName,"listBoxScenario");
    listBoxScenario.init(startX, startY+160, 190);

    labelScenario.registerGraphicComponent(containerName,"labelScenario");
	labelScenario.init(startX, startY+190);

	buttonReturn.setText(lang.get("Return"));
	buttonPlayNow.setText(lang.get("PlayNow"));

	if(this->isTutorialMode == true) {
		labelScenario.setText(lang.get("Tutorial"));
	}
	else {
		labelScenario.setText(lang.get("Scenario"));
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
	for(int i= 0; i<results.size(); ++i){
		results[i] = formatString(results[i]);
	}
    listBoxScenario.setItems(results);

    try {
    	if(listBoxScenario.getSelectedItemIndex() > 0 && listBoxScenario.getSelectedItemIndex() < scenarioFiles.size()) {
    		loadScenarioInfo(Scenario::getScenarioPath(dirList, scenarioFiles[listBoxScenario.getSelectedItemIndex()]), &scenarioInfo );
    		labelInfo.setText(scenarioInfo.desc);
    	}

        GraphicComponent::applyAllCustomProperties(containerName);
    }
	catch(const std::exception &ex) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s %d] Error detected:\n%s\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);

        mainMessageBoxState=1;
        showMessageBox( "Error: " + string(ex.what()), "Error detected", false);
	}
}

void MenuStateScenario::reloadUI() {
	Lang &lang= Lang::getInstance();

	console.resetFonts();
	mainMessageBox.init(lang.get("Ok"));
	labelInfo.setFont(CoreData::getInstance().getMenuFontNormal());
	labelInfo.setFont3D(CoreData::getInstance().getMenuFontNormal3D());

	buttonReturn.setText(lang.get("Return"));
	buttonPlayNow.setText(lang.get("PlayNow"));

    labelScenario.setText(lang.get("Scenario"));

	GraphicComponent::reloadFontsForRegisterGraphicComponents(containerName);
}

MenuStateScenario::~MenuStateScenario() {
	cleanupPreviewTexture();
}

void MenuStateScenario::cleanupPreviewTexture() {
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] scenarioLogoTexture [%p]\n",__FILE__,__FUNCTION__,__LINE__,scenarioLogoTexture);

	if(scenarioLogoTexture != NULL) {
		Renderer::getInstance().endTexture(rsGlobal, scenarioLogoTexture, false);
	}
	scenarioLogoTexture = NULL;
}

void MenuStateScenario::mouseClick(int x, int y, MouseButton mouseButton) {
	CoreData &coreData= CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();

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
	}
	else if(buttonReturn.mouseClick(x,y)){
		soundRenderer.playFx(coreData.getClickSoundA());
		mainMenu->setState(new MenuStateNewGame(program, mainMenu));
		return;
    }
	else if(buttonPlayNow.mouseClick(x,y)){
		soundRenderer.playFx(coreData.getClickSoundC());
		launchGame();
		return;
	}
    else if(listBoxScenario.mouseClick(x, y)){
        try {
        	printf("In [%s::%s Line: %d] listBoxScenario.getSelectedItemIndex() = %d scenarioFiles.size() = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,listBoxScenario.getSelectedItemIndex(),scenarioFiles.size());

        	if(listBoxScenario.getSelectedItemIndex() > 0 && listBoxScenario.getSelectedItemIndex() < scenarioFiles.size()) {
        		loadScenarioInfo(Scenario::getScenarioPath(dirList, scenarioFiles[listBoxScenario.getSelectedItemIndex()]), &scenarioInfo);
        		labelInfo.setText(scenarioInfo.desc);
        	}
        }
        catch(const std::exception &ex) {
            char szBuf[4096]="";
            sprintf(szBuf,"In [%s::%s %d] Error detected:\n%s\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
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
		renderer.renderTextureQuad(300,350,400,300,scenarioLogoTexture,1.0f);
		//renderer.renderBackground(scenarioLogoTexture);
	}

	if(mainMessageBox.getEnabled()) {
		renderer.renderMessageBox(&mainMessageBox);
	}
	else {
		renderer.renderLabel(&labelInfo);
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
		listBoxScenario.setSelectedItem(formatString(this->autoloadScenarioName),false);

		if(listBoxScenario.getSelectedItem() != formatString(this->autoloadScenarioName)) {
			mainMessageBoxState=1;
			showMessageBox( "Could not find scenario name: " + formatString(this->autoloadScenarioName), "Scenario Missing", false);
			this->autoloadScenarioName = "";
		}
		else {
			if(listBoxScenario.getSelectedItemIndex() > 0 && listBoxScenario.getSelectedItemIndex() < scenarioFiles.size()) {
				loadScenarioInfo(Scenario::getScenarioPath(dirList, scenarioFiles[listBoxScenario.getSelectedItemIndex()]), &scenarioInfo);
				labelInfo.setText(scenarioInfo.desc);

				SoundRenderer &soundRenderer= SoundRenderer::getInstance();
				CoreData &coreData= CoreData::getInstance();
				soundRenderer.playFx(coreData.getClickSoundC());
				launchGame();
				return;
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
	printf("In [%s::%s Line: %d] scenarioInfo.file [%s] [%s][%s][%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,scenarioInfo.file.c_str(),scenarioInfo.tilesetName.c_str(),scenarioInfo.mapName.c_str(),scenarioInfo.techTreeName.c_str());

	if(scenarioInfo.file != "" && scenarioInfo.tilesetName != "" && scenarioInfo.mapName != "" && scenarioInfo.techTreeName != "") {
		GameSettings gameSettings;
		loadGameSettings(&scenarioInfo, &gameSettings);
		program->setState(new Game(program, &gameSettings, false));
	}
}

void MenuStateScenario::setScenario(int i) {
	listBoxScenario.setSelectedItemIndex(i);
	loadScenarioInfo(Scenario::getScenarioPath(dirList, scenarioFiles[listBoxScenario.getSelectedItemIndex()]), &scenarioInfo);
}

void MenuStateScenario::loadScenarioInfo(string file, ScenarioInfo *scenarioInfo) {
	Scenario::loadScenarioInfo(file, scenarioInfo);

	cleanupPreviewTexture();
	previewLoadDelayTimer=time(NULL);
	needToLoadTextures=true;
}

void MenuStateScenario::loadScenarioPreviewTexture(){
	if(enableScenarioTexturePreview == true) {
		//if(listBoxScenario.getSelectedItemIndex() >= 0) {
		if(listBoxScenario.getSelectedItemIndex() > 0 && listBoxScenario.getSelectedItemIndex() < scenarioFiles.size()) {
			GameSettings gameSettings;
			loadGameSettings(&scenarioInfo, &gameSettings);

			string scenarioLogo 	= "";
			bool loadingImageUsed 	= false;

			Game::extractScenarioLogoFile(&gameSettings, scenarioLogo, loadingImageUsed);

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] scenarioLogo [%s]\n",__FILE__,__FUNCTION__,__LINE__,scenarioLogo.c_str());

			if(scenarioLogo != "") {
				cleanupPreviewTexture();
				scenarioLogoTexture = Renderer::findFactionLogoTexture(scenarioLogo);
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
		char szBuf[1024]="";
		sprintf(szBuf,"listBoxScenario.getSelectedItemIndex() < 0, = %d",listBoxScenario.getSelectedItemIndex());
		throw megaglest_runtime_error(szBuf);
	}
	else if(listBoxScenario.getSelectedItemIndex() >= scenarioFiles.size()) {
		char szBuf[1024]="";
		sprintf(szBuf,"listBoxScenario.getSelectedItemIndex() >= scenarioFiles.size(), = [%d][%d]",listBoxScenario.getSelectedItemIndex(),(int)scenarioFiles.size());
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
