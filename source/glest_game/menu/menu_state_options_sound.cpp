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

#include "menu_state_options_sound.h"

#include "renderer.h"
#include "game.h"
#include "program.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "config.h"
#include "menu_state_root.h"
#include "menu_state_options.h"
#include "util.h"
#include "menu_state_keysetup.h"
#include "menu_state_options_graphics.h"
#include "menu_state_options_network.h"
#include "menu_state_options_sound.h"
#include "string_utils.h"
#include "metrics.h"
#include "megaglest_cegui_manager.h"

#include "leak_dumper.h"

using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class MenuStateOptions
// =====================================================
MenuStateOptionsSound::MenuStateOptionsSound(Program *program, MainMenu *mainMenu, ProgramState **parentUI) :
	MenuState(program, mainMenu, "config") {

	try {
		containerName 	= "OptionsSound";
		this->parentUI 	= parentUI;
		this->console.setOnlyChatMessagesInStoredLines(false);

		setupCEGUIWidgets();

		GraphicComponent::applyAllCustomProperties(containerName);
	}
	catch(exception &e) {
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error loading options: %s\n",__FILE__,__FUNCTION__,__LINE__,e.what());
		throw megaglest_runtime_error(string("Error loading options msg: ") + e.what());
	}
}

void MenuStateOptionsSound::reloadUI() {

	console.resetFonts();
	GraphicComponent::reloadFontsForRegisterGraphicComponents(containerName);

	setupCEGUIWidgetsText();
}

void MenuStateOptionsSound::setupCEGUIWidgets() {

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	cegui_manager.unsubscribeEvents(this->containerName);
	cegui_manager.setCurrentLayout("OptionsMenuRoot.layout",containerName);

	cegui_manager.loadLayoutFromFile("OptionsMenuAudio.layout");
	cegui_manager.loadLayoutFromFile("OptionsMenuKeyboard.layout");;
	cegui_manager.loadLayoutFromFile("OptionsMenuMisc.layout");
	cegui_manager.loadLayoutFromFile("OptionsMenuNetwork.layout");
	cegui_manager.loadLayoutFromFile("OptionsMenuVideo.layout");

	setupCEGUIWidgetsText();

	cegui_manager.setControlEventCallback(containerName, "TabControl",
					cegui_manager.getEventTabControlSelectionChanged(), this);

	cegui_manager.setControlEventCallback(containerName,
			"TabControl/__auto_TabPane__/Audio/ButtonSave",
			cegui_manager.getEventButtonClicked(), this);

	cegui_manager.setControlEventCallback(containerName,
			"TabControl/__auto_TabPane__/Audio/ButtonReturn",
			cegui_manager.getEventButtonClicked(), this);

	cegui_manager.subscribeMessageBoxEventClicks(containerName, this);
	cegui_manager.subscribeMessageBoxEventClicks(containerName, this, "TabControl/__auto_TabPane__/Audio/MsgBox");
}

void MenuStateOptionsSound::setupCEGUIWidgetsText() {

	Lang &lang		= Lang::getInstance();
	Config &config	= Config::getInstance();

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	cegui_manager.setCurrentLayout("OptionsMenuRoot.layout",containerName);

	CEGUI::Window *ctlAudio 	= cegui_manager.loadLayoutFromFile("OptionsMenuAudio.layout");
	CEGUI::Window *ctlKeyboard 	= cegui_manager.loadLayoutFromFile("OptionsMenuKeyboard.layout");
	CEGUI::Window *ctlMisc 		= cegui_manager.loadLayoutFromFile("OptionsMenuMisc.layout");
	CEGUI::Window *ctlNetwork 	= cegui_manager.loadLayoutFromFile("OptionsMenuNetwork.layout");
	CEGUI::Window *ctlVideo 	= cegui_manager.loadLayoutFromFile("OptionsMenuVideo.layout");

	cegui_manager.setControlText(ctlAudio,lang.getString("Audio","",false,true));
	cegui_manager.setControlText(ctlKeyboard,lang.getString("Keyboardsetup","",false,true));
	cegui_manager.setControlText(ctlMisc,lang.getString("Misc","",false,true));
	cegui_manager.setControlText(ctlNetwork,lang.getString("Network","",false,true));
	cegui_manager.setControlText(ctlVideo,lang.getString("Video","",false,true));

	if(cegui_manager.isChildControl(cegui_manager.getControl("TabControl"),"__auto_TabPane__/Audio") == false) {
		cegui_manager.addTabPageToTabControl("TabControl", ctlAudio,"",18);
	}
	if(cegui_manager.isChildControl(cegui_manager.getControl("TabControl"),"__auto_TabPane__/Keyboard") == false) {
		cegui_manager.addTabPageToTabControl("TabControl", ctlKeyboard,"",18);
	}
	if(cegui_manager.isChildControl(cegui_manager.getControl("TabControl"),"__auto_TabPane__/Misc") == false) {
		cegui_manager.addTabPageToTabControl("TabControl", ctlMisc,"",18);
	}
	if(cegui_manager.isChildControl(cegui_manager.getControl("TabControl"),"__auto_TabPane__/Network") == false) {
		cegui_manager.addTabPageToTabControl("TabControl", ctlNetwork,"",18);
	}
	if(cegui_manager.isChildControl(cegui_manager.getControl("TabControl"),"__auto_TabPane__/Video") == false) {
		cegui_manager.addTabPageToTabControl("TabControl", ctlVideo,"",18);
	}

	cegui_manager.setSelectedTabPage("TabControl", "Audio");

	if(cegui_manager.isChildControl(cegui_manager.getControl("TabControl/__auto_TabPane__/Audio"),"MsgBox") == false) {
		cegui_manager.cloneMessageBoxControl("MsgBox", ctlAudio);
	}

	vector<string> soundDevices;
	soundDevices.push_back("None");
	soundDevices.push_back("OpenAL");

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlAudio,"LabelSoundDevice"),lang.getString("SoundAndMusic","",false,true));
	cegui_manager.addItemsToComboBoxControl(
			cegui_manager.getChildControl(ctlAudio,"ComboBoxSoundDevice"), soundDevices,true);
	cegui_manager.setSelectedItemInComboBoxControl(
			cegui_manager.getChildControl(ctlAudio,"ComboBoxSoundDevice"), config.getString("FactorySound"),true);
	cegui_manager.setControlText(cegui_manager.getChildControl(ctlAudio,"ComboBoxSoundDevice"),config.getString("FactorySound"));

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlAudio,"LabelSoundFxVolume"),lang.getString("FxVolume","",false,true));
	cegui_manager.setSpinnerControlValues(cegui_manager.getChildControl(ctlAudio,"SpinnerSoundFxVolume"),0,100,config.getInt("SoundVolumeFx"),1);

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlAudio,"LabelAmbientVolume"),lang.getString("AmbientVolume","",false,true));
	cegui_manager.setSpinnerControlValues(cegui_manager.getChildControl(ctlAudio,"SpinnerAmbientVolume"),0,100,config.getInt("SoundVolumeAmbient"),1);

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlAudio,"LabelMusicVolume"),lang.getString("MusicVolume","",false,true));
	cegui_manager.setSpinnerControlValues(cegui_manager.getChildControl(ctlAudio,"SpinnerMusicVolume"),0,100,config.getInt("SoundVolumeMusic"),1);

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlAudio,"ButtonSave"),lang.getString("Save","",false,true));
	cegui_manager.setControlText(cegui_manager.getChildControl(ctlAudio,"ButtonReturn"),lang.getString("Return","",false,true));
}


void MenuStateOptionsSound::callDelayedCallbacks() {
	if(hasDelayedCallbacks() == true) {
		for(unsigned int index = 0; index < delayedCallbackList.size(); ++index) {
			DelayCallbackFunction pCB = delayedCallbackList[index];
			(this->*pCB)();
		}
	}
}

void MenuStateOptionsSound::delayedCallbackFunctionSelectKeyboardTab() {
	CoreData &coreData				= CoreData::getInstance();
	SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	cegui_manager.unsubscribeEvents(this->containerName);

	soundRenderer.playFx(coreData.getClickSoundA());
	mainMenu->setState(new MenuStateKeysetup(program, mainMenu,this->parentUI)); // open keyboard shortcuts setup screen
}

void MenuStateOptionsSound::delayedCallbackFunctionSelectMiscTab() {
	CoreData &coreData				= CoreData::getInstance();
	SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	cegui_manager.unsubscribeEvents(this->containerName);

	soundRenderer.playFx(coreData.getClickSoundA());
	mainMenu->setState(new MenuStateOptions(program, mainMenu,this->parentUI)); // open keyboard shortcuts setup screen
}

void MenuStateOptionsSound::delayedCallbackFunctionSelectNetworkTab() {
	CoreData &coreData				= CoreData::getInstance();
	SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	cegui_manager.unsubscribeEvents(this->containerName);

	soundRenderer.playFx(coreData.getClickSoundA());
	mainMenu->setState(new MenuStateOptionsNetwork(program, mainMenu,this->parentUI)); // open keyboard shortcuts setup screen
}

void MenuStateOptionsSound::delayedCallbackFunctionSelectVideoTab() {
	CoreData &coreData				= CoreData::getInstance();
	SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	cegui_manager.unsubscribeEvents(this->containerName);

	soundRenderer.playFx(coreData.getClickSoundA());
	mainMenu->setState(new MenuStateOptionsGraphics(program, mainMenu,this->parentUI)); // open keyboard shortcuts setup screen
}

void MenuStateOptionsSound::delayedCallbackFunctionOk() {

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	cegui_manager.unsubscribeEvents(this->containerName);

	mainMenu->setState(new MenuStateOptions(program, mainMenu));
}

void MenuStateOptionsSound::delayedCallbackFunctionReturn() {
	CoreData &coreData				= CoreData::getInstance();
	SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	cegui_manager.unsubscribeEvents(this->containerName);

	soundRenderer.playFx(coreData.getClickSoundA());
	if(this->parentUI != NULL) {
		*this->parentUI = NULL;
		delete *this->parentUI;
	}
	mainMenu->setState(new MenuStateRoot(program, mainMenu));
}

bool MenuStateOptionsSound::EventCallback(CEGUI::Window *ctl, std::string name) {

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	if(name == cegui_manager.getEventTabControlSelectionChanged()) {

		if(cegui_manager.isSelectedTabPage("TabControl", "Keyboard") == true) {
			DelayCallbackFunction pCB = &MenuStateOptionsSound::delayedCallbackFunctionSelectKeyboardTab;
			delayedCallbackList.push_back(pCB);
		}
		else if(cegui_manager.isSelectedTabPage("TabControl", "Misc") == true) {
			DelayCallbackFunction pCB = &MenuStateOptionsSound::delayedCallbackFunctionSelectMiscTab;
			delayedCallbackList.push_back(pCB);
		}
		else if(cegui_manager.isSelectedTabPage("TabControl", "Network") == true) {
			DelayCallbackFunction pCB = &MenuStateOptionsSound::delayedCallbackFunctionSelectNetworkTab;
			delayedCallbackList.push_back(pCB);
		}
		else if(cegui_manager.isSelectedTabPage("TabControl", "Video") == true) {
			DelayCallbackFunction pCB = &MenuStateOptionsSound::delayedCallbackFunctionSelectVideoTab;
			delayedCallbackList.push_back(pCB);
		}
		return true;
	}
	else if(name == cegui_manager.getEventButtonClicked()) {

		if(cegui_manager.isControlMessageBoxOk(ctl,"TabControl/__auto_TabPane__/Audio/MsgBox") == true) {

			CoreData &coreData				= CoreData::getInstance();
			SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

			soundRenderer.playFx(coreData.getClickSoundA());

			cegui_manager.hideMessageBox("TabControl/__auto_TabPane__/Audio/MsgBox");
			return true;
		}
		else if(cegui_manager.isControlMessageBoxCancel(ctl,"TabControl/__auto_TabPane__/Audio/MsgBox") == true) {

			CoreData &coreData				= CoreData::getInstance();
			SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

			soundRenderer.playFx(coreData.getClickSoundA());

			cegui_manager.hideMessageBox("TabControl/__auto_TabPane__/Audio/MsgBox");
			return true;
		}
		else if(ctl == cegui_manager.getControl("TabControl/__auto_TabPane__/Audio/ButtonSave")) {

			CoreData &coreData				= CoreData::getInstance();
			SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

			soundRenderer.playFx(coreData.getClickSoundA());
			saveConfig();

			return true;
		}
		else if(ctl == cegui_manager.getControl("TabControl/__auto_TabPane__/Audio/ButtonReturn")) {

			DelayCallbackFunction pCB = &MenuStateOptionsSound::delayedCallbackFunctionReturn;
			delayedCallbackList.push_back(pCB);

			return true;
		}
		//printf("Line: %d\n",__LINE__);
		//cegui_manager.printDebugControlInfo(ctl);
	}

	return false;
}

void MenuStateOptionsSound::mouseClick(int x, int y, MouseButton mouseButton) { }

void MenuStateOptionsSound::mouseMove(int x, int y, const MouseState *ms) { }

void MenuStateOptionsSound::keyPress(SDL_KeyboardEvent c) {

	Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));
	if(isKeyPressed(configKeys.getSDLKey("SaveGUILayout"),c) == true) {
		GraphicComponent::saveAllCustomProperties(containerName);
	}
}

void MenuStateOptionsSound::render() {

	Renderer &renderer = Renderer::getInstance();
	renderer.renderConsole(&console,false,true);

	if(program != NULL) program->renderProgramMsgBox();
}

void MenuStateOptionsSound::saveConfig() {

	Config &config	= Config::getInstance();
	Lang &lang		= Lang::getInstance();
	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();

	string selectedSoundDevice = cegui_manager.getSelectedItemFromComboBoxControl(
				cegui_manager.getControl("TabControl/__auto_TabPane__/Audio/ComboBoxSoundDevice"));

	config.setString("FactorySound", selectedSoundDevice);
	config.setString("SoundVolumeFx", intToStr(cegui_manager.getSpinnerControlValue(
			cegui_manager.getControl("TabControl/__auto_TabPane__/Audio/SpinnerSoundFxVolume"))));

	config.setString("SoundVolumeAmbient", intToStr(cegui_manager.getSpinnerControlValue(
			cegui_manager.getControl("TabControl/__auto_TabPane__/Audio/SpinnerAmbientVolume"))));

	int musicVolume = cegui_manager.getSpinnerControlValue(
				cegui_manager.getControl("TabControl/__auto_TabPane__/Audio/SpinnerMusicVolume"));
	CoreData::getInstance().getMenuMusic()->setVolume(musicVolume / 100.f);
	config.setString("SoundVolumeMusic", intToStr(musicVolume));

	config.save();

    SoundRenderer &soundRenderer= SoundRenderer::getInstance();
    soundRenderer.stopAllSounds();
    program->stopSoundSystem();
    soundRenderer.init(program->getWindow());
    soundRenderer.loadConfig();
    soundRenderer.setMusicVolume(CoreData::getInstance().getMenuMusic());
    program->startSoundSystem();

    if(CoreData::getInstance().hasMainMenuVideoFilename() == false) {
    	soundRenderer.playMusic(CoreData::getInstance().getMenuMusic());
    }

	Renderer::getInstance().loadConfig();
	console.addLine(lang.getString("SettingsSaved"));
}

}}//end namespace
