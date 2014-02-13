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

#include "menu_state_root.h"

#include "renderer.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "config.h"
#include "menu_state_new_game.h"
#include "menu_state_load_game.h"
#include "menu_state_options.h"
#include "menu_state_about.h"
#include "menu_state_mods.h"
#include "auto_test.h"
#include "megaglest_cegui_manager.h"

#include "leak_dumper.h"

namespace Glest{ namespace Game{

// =====================================================
// 	class MenuStateRoot
// =====================================================

MenuStateRoot::MenuStateRoot(Program *program, MainMenu *mainMenu) :
	MenuState(program, mainMenu, "root"),
		MegaGlest_CEGUIManagerBackInterface() {

	containerName = "MainMenu";

	setupCEGUIWidgets();
}

void MenuStateRoot::setupCEGUIWidgets() {

	CoreData &coreData=  CoreData::getInstance();

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	cegui_manager.unsubscribeEvents(this->containerName);
	cegui_manager.setCurrentLayout("MainMenuRoot.layout",containerName);

	cegui_manager.setImageFileForControl("GameLogoImage", coreData.getLogoTexture()->getPath(), "GameLogo");

	for(int idx = 0; idx < (int)coreData.getLogoTextureExtraCount(); ++idx) {
		Texture2D *extraLogo = coreData.getLogoTextureExtra(idx);

		string extraLogoControlName = "ExtraLogo" + intToStr(idx+1);
		string extraLogoName 		= extraLogoControlName + "Image";
		cegui_manager.setImageFileForControl(extraLogoName, extraLogo->getPath(), extraLogoControlName);
	}

	cegui_manager.setControlEventCallback(containerName,
			"ButtonNewGame", cegui_manager.getEventClicked(), this);
	cegui_manager.setControlEventCallback(containerName,
			"ButtonLoadGame", cegui_manager.getEventClicked(), this);
	cegui_manager.setControlEventCallback(containerName,
			"ButtonMods", cegui_manager.getEventClicked(), this);
	cegui_manager.setControlEventCallback(containerName,
			"ButtonOptions", cegui_manager.getEventClicked(), this);
	cegui_manager.setControlEventCallback(containerName,
			"ButtonAbout", cegui_manager.getEventClicked(), this);
	cegui_manager.setControlEventCallback(containerName,
			"ButtonExit", cegui_manager.getEventClicked(), this);

	cegui_manager.subscribeMessageBoxEventClicks(containerName, this);
	cegui_manager.subscribeErrorMessageBoxEventClicks(containerName, this);

	setupCEGUIWidgetsText();
}

void MenuStateRoot::setupCEGUIWidgetsText() {

	Lang &lang= Lang::getInstance();
	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();

	cegui_manager.setCurrentLayout("MainMenuRoot.layout",containerName);

	string versionText = glestVersionString;
	if(EndsWith(versionText, "-dev") == true) {
		versionText = glestVersionString + " [" + getCompileDateTime() + ", " + getGITRevisionString() + "]";
	}

	cegui_manager.setControlText("GameVersion",versionText);
	cegui_manager.setControlText("ButtonNewGame",lang.getString("NewGame","",false,true));
	cegui_manager.setControlText("ButtonLoadGame",lang.getString("LoadGame","",false,true));
	cegui_manager.setControlText("ButtonMods",lang.getString("Mods","",false,true));
	cegui_manager.setControlText("ButtonOptions",lang.getString("Options","",false,true));
	cegui_manager.setControlText("ButtonAbout",lang.getString("About","",false,true));
	cegui_manager.setControlText("ButtonExit",lang.getString("Exit","",false,true));
}

bool MenuStateRoot::EventCallback(CEGUI::Window *ctl, std::string name) {

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	if(name == cegui_manager.getEventClicked()) {

		if(cegui_manager.isControlMessageBoxOk(ctl) == true) {
			CoreData &coreData=  CoreData::getInstance();
			SoundRenderer &soundRenderer = SoundRenderer::getInstance();
			soundRenderer.playFx(coreData.getClickSoundA());
			program->exit();
			return true;
		}
		else if(cegui_manager.isControlMessageBoxCancel(ctl) == true) {
			cegui_manager.hideMessageBox();
		}
		else if(cegui_manager.isControlErrorMessageBoxOk(ctl) == true) {
			cegui_manager.hideErrorMessageBox();
		}
		else if(ctl == cegui_manager.getControl("ButtonNewGame")) {
			CoreData &coreData=  CoreData::getInstance();
			SoundRenderer &soundRenderer = SoundRenderer::getInstance();
			soundRenderer.playFx(coreData.getClickSoundB());

			mainMenu->setState(new MenuStateNewGame(program, mainMenu));
			return true;
		}
		else if(ctl == cegui_manager.getControl("ButtonLoadGame")) {
			CoreData &coreData=  CoreData::getInstance();
			SoundRenderer &soundRenderer = SoundRenderer::getInstance();
			soundRenderer.playFx(coreData.getClickSoundB());

			mainMenu->setState(new MenuStateLoadGame(program, mainMenu));
			return true;
		}
		else if(ctl == cegui_manager.getControl("ButtonMods")) {
			CoreData &coreData=  CoreData::getInstance();
			SoundRenderer &soundRenderer = SoundRenderer::getInstance();
			soundRenderer.playFx(coreData.getClickSoundB());

			mainMenu->setState(new MenuStateMods(program, mainMenu));
			return true;
		}
		else if(ctl == cegui_manager.getControl("ButtonOptions")) {
			CoreData &coreData=  CoreData::getInstance();
			SoundRenderer &soundRenderer = SoundRenderer::getInstance();
			soundRenderer.playFx(coreData.getClickSoundB());

			mainMenu->setState(new MenuStateOptions(program, mainMenu));
			return true;
		}
		else if(ctl == cegui_manager.getControl("ButtonAbout")) {
			CoreData &coreData=  CoreData::getInstance();
			SoundRenderer &soundRenderer = SoundRenderer::getInstance();
			soundRenderer.playFx(coreData.getClickSoundB());

			mainMenu->setState(new MenuStateAbout(program, mainMenu));
			return true;
		}
		else if(ctl == cegui_manager.getControl("ButtonExit")) {
			CoreData &coreData=  CoreData::getInstance();
			SoundRenderer &soundRenderer = SoundRenderer::getInstance();
			soundRenderer.playFx(coreData.getClickSoundA());

			program->exit();
			return true;
		}
	}
	return false;
}

void MenuStateRoot::reloadUI() {

	setupCEGUIWidgetsText();
	console.resetFonts();
}

bool MenuStateRoot::isMasterserverMode() const {
	return GlobalStaticFlags::getIsNonGraphicalModeEnabled();
}

void MenuStateRoot::render() {
	if(isMasterserverMode() == true) {
		return;
	}

	Renderer &renderer 		= Renderer::getInstance();
	renderer.renderConsole(&console,false,true);

	if(program != NULL) program->renderProgramMsgBox();
}

void MenuStateRoot::update() {

	if(Config::getInstance().getBool("AutoTest")) {
		if(AutoTest::getInstance().mustExitGame() == false) {
			AutoTest::getInstance().updateRoot(program, mainMenu);
		}
		else {
			program->exit();
		}
		return;
	}
	console.update();
}

void MenuStateRoot::keyDown(SDL_KeyboardEvent key) {

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] key = [%c] [%d]\n",__FILE__,__FUNCTION__,__LINE__,key.keysym.sym,key.keysym.sym);
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] key = [%d - %c]\n",__FILE__,__FUNCTION__,__LINE__,key.keysym.sym,key.keysym.sym);

	Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));

	//exit
	if(isKeyPressed(configKeys.getSDLKey("ExitKey"),key) == true) {

		Lang &lang = Lang::getInstance();
		showMessageBox(lang.getString("ExitGame?"), "");
	}
	else if(MegaGlest_CEGUIManager::getInstance().isMessageBoxShowing() &&
			isKeyPressed(SDLK_RETURN,key) == true) {

		SDL_keysym keystate = key.keysym;
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] keystate.mod [%d]\n",__FILE__,__FUNCTION__,__LINE__,keystate.mod);

		if(keystate.mod & (KMOD_LALT | KMOD_RALT)) {
		}
		else {
			program->exit();
		}
	}
	else if(isKeyPressed(configKeys.getSDLKey("SaveGUILayout"),key) == true) {
		GraphicComponent::saveAllCustomProperties(containerName);
	}
}

void MenuStateRoot::showMessageBox(const string &text, const string &header) {

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	if(cegui_manager.isMessageBoxShowing() == false) {
		MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
		Lang &lang= Lang::getInstance();
		cegui_manager.displayMessageBox(header, text, lang.getString("Yes","",false,true),lang.getString("No","",false,true));
	}
	else {
		cegui_manager.hideMessageBox();
	}
}

//void MenuStateRoot::showErrorMessageBox(const string &text, const string &header) {
//
//	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
//	Lang &lang= Lang::getInstance();
//	cegui_manager.displayErrorMessageBox(header, text, lang.getString("Ok","",false,true));
//}

}}//end namespace
