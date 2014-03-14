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

#include "menu_state_new_game.h"

#include "renderer.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "config.h"
#include "menu_state_custom_game.h"
#include "menu_state_scenario.h"
#include "menu_state_join_game.h"
#include "menu_state_masterserver.h"
#include "menu_state_root.h"
#include "network_manager.h"
#include "auto_test.h"
#include "megaglest_cegui_manager.h"

#include "leak_dumper.h"

namespace Glest{ namespace Game{

// =====================================================
// 	class MenuStateNewGame
// =====================================================

MenuStateNewGame::MenuStateNewGame(Program *program, MainMenu *mainMenu):
	MenuState(program, mainMenu, "root")
{
	containerName = "NewGame";
	GraphicComponent::applyAllCustomProperties(containerName);

	setupCEGUIWidgets();

	NetworkManager::getInstance().end();
}

void MenuStateNewGame::setupCEGUIWidgets() {

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	cegui_manager.unsubscribeEvents(this->containerName);
	cegui_manager.setCurrentLayout("NewGameMenu.layout",containerName);

	cegui_manager.setControlEventCallback(containerName,
			"ButtonTutorial", cegui_manager.getEventButtonClicked(), this);
	cegui_manager.setControlEventCallback(containerName,
			"ButtonScenario", cegui_manager.getEventButtonClicked(), this);
	cegui_manager.setControlEventCallback(containerName,
			"ButtonCustomGame", cegui_manager.getEventButtonClicked(), this);
	cegui_manager.setControlEventCallback(containerName,
			"ButtonInternetGame", cegui_manager.getEventButtonClicked(), this);
	cegui_manager.setControlEventCallback(containerName,
			"ButtonLANGame", cegui_manager.getEventButtonClicked(), this);
	cegui_manager.setControlEventCallback(containerName,
			"ButtonReturn", cegui_manager.getEventButtonClicked(), this);

	cegui_manager.subscribeMessageBoxEventClicks(containerName, this);
	cegui_manager.subscribeErrorMessageBoxEventClicks(containerName, this);

	setupCEGUIWidgetsText();
}

void MenuStateNewGame::setupCEGUIWidgetsText() {

	Lang &lang= Lang::getInstance();
	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();

	cegui_manager.setCurrentLayout("NewGameMenu.layout",containerName);

	cegui_manager.setControlText("ButtonTutorial",lang.getString("Tutorial","",false,true));
	cegui_manager.setControlText("ButtonScenario",lang.getString("Scenario","",false,true));
	cegui_manager.setControlText("ButtonCustomGame",lang.getString("CustomGame","",false,true));
	cegui_manager.setControlText("ButtonInternetGame",lang.getString("JoinInternetGame","",false,true));
	cegui_manager.setControlText("ButtonLANGame",lang.getString("JoinGame","",false,true));
	cegui_manager.setControlText("ButtonReturn",lang.getString("Return","",false,true));
}

void MenuStateNewGame::callDelayedCallbacks() {
	if(hasDelayedCallbacks() == true) {
		for(unsigned int index = 0; index < delayedCallbackList.size(); ++index) {
			DelayCallbackFunction pCB = delayedCallbackList[index];
			(this->*pCB)();
		}
	}
}

void MenuStateNewGame::delayedCallbackFunctionTutorial() {
	CoreData &coreData				= CoreData::getInstance();
	SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	cegui_manager.unsubscribeEvents(this->containerName);

	soundRenderer.playFx(coreData.getClickSoundB());
	mainMenu->setState(new MenuStateScenario(program, mainMenu, true,
			Config::getInstance().getPathListForType(ptTutorials)));
}

void MenuStateNewGame::delayedCallbackFunctionScenario() {
	CoreData &coreData				= CoreData::getInstance();
	SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	cegui_manager.unsubscribeEvents(this->containerName);

	soundRenderer.playFx(coreData.getClickSoundB());
	mainMenu->setState(new MenuStateScenario(program, mainMenu, false,
			Config::getInstance().getPathListForType(ptScenarios)));
}

void MenuStateNewGame::delayedCallbackFunctionCustomGame() {
	CoreData &coreData				= CoreData::getInstance();
	SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	cegui_manager.unsubscribeEvents(this->containerName);

	soundRenderer.playFx(coreData.getClickSoundB());
	mainMenu->setState(new MenuStateCustomGame(program, mainMenu));
}

void MenuStateNewGame::delayedCallbackFunctionInternetGame() {
	CoreData &coreData				= CoreData::getInstance();
	SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	cegui_manager.unsubscribeEvents(this->containerName);

	soundRenderer.playFx(coreData.getClickSoundB());
	mainMenu->setState(new MenuStateMasterserver(program, mainMenu));
}

void MenuStateNewGame::delayedCallbackFunctionLANGame() {
	CoreData &coreData				= CoreData::getInstance();
	SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	cegui_manager.unsubscribeEvents(this->containerName);

	soundRenderer.playFx(coreData.getClickSoundB());
	mainMenu->setState(new MenuStateJoinGame(program, mainMenu));
}

void MenuStateNewGame::delayedCallbackFunctionReturn() {
	CoreData &coreData				= CoreData::getInstance();
	SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	cegui_manager.unsubscribeEvents(this->containerName);

	soundRenderer.playFx(coreData.getClickSoundB());
	mainMenu->setState(new MenuStateRoot(program, mainMenu));
}

bool MenuStateNewGame::EventCallback(CEGUI::Window *ctl, std::string name) {

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	if(name == cegui_manager.getEventButtonClicked()) {

		if(ctl == cegui_manager.getControl("ButtonTutorial")) {
			DelayCallbackFunction pCB = &MenuStateNewGame::delayedCallbackFunctionTutorial;
			delayedCallbackList.push_back(pCB);

			return true;
		}
		else if(ctl == cegui_manager.getControl("ButtonScenario")) {
			DelayCallbackFunction pCB = &MenuStateNewGame::delayedCallbackFunctionScenario;
			delayedCallbackList.push_back(pCB);

			return true;
		}
		else if(ctl == cegui_manager.getControl("ButtonCustomGame")) {
			DelayCallbackFunction pCB = &MenuStateNewGame::delayedCallbackFunctionCustomGame;
			delayedCallbackList.push_back(pCB);

			return true;
		}
		else if(ctl == cegui_manager.getControl("ButtonInternetGame")) {
			DelayCallbackFunction pCB = &MenuStateNewGame::delayedCallbackFunctionInternetGame;
			delayedCallbackList.push_back(pCB);

			return true;
		}
		else if(ctl == cegui_manager.getControl("ButtonLANGame")) {
			DelayCallbackFunction pCB = &MenuStateNewGame::delayedCallbackFunctionLANGame;
			delayedCallbackList.push_back(pCB);

			return true;
		}
		else if(ctl == cegui_manager.getControl("ButtonReturn")) {
			DelayCallbackFunction pCB = &MenuStateNewGame::delayedCallbackFunctionReturn;
			delayedCallbackList.push_back(pCB);

			return true;
		}
	}
	return false;
}

void MenuStateNewGame::reloadUI() {

	GraphicComponent::reloadFontsForRegisterGraphicComponents(containerName);
	setupCEGUIWidgetsText();
}

void MenuStateNewGame::mouseClick(int x, int y, MouseButton mouseButton) { }

void MenuStateNewGame::mouseMove(int x, int y, const MouseState *ms) { }

void MenuStateNewGame::render() {

	Renderer &renderer= Renderer::getInstance();
	renderer.renderConsole(&console,false,true);
	if(program != NULL) program->renderProgramMsgBox();
}

void MenuStateNewGame::update() {
	MenuState::update();
	if(Config::getInstance().getBool("AutoTest")) {
		AutoTest::getInstance().updateNewGame(program, mainMenu);
		return;
	}
	console.update();
}

void MenuStateNewGame::keyDown(SDL_KeyboardEvent key) {
	Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));
	if(isKeyPressed(configKeys.getSDLKey("SaveGUILayout"),key) == true) {
		GraphicComponent::saveAllCustomProperties(containerName);
	}
}

}}//end namespace
