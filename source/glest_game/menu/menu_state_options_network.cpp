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

#include "menu_state_options_network.h"

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
MenuStateOptionsNetwork::MenuStateOptionsNetwork(Program *program, MainMenu *mainMenu, ProgramState **parentUI) :
	MenuState(program, mainMenu, "config") {
	try {
		containerName = "OptionsNetwork";
		this->parentUI=parentUI;
		this->console.setOnlyChatMessagesInStoredLines(false);

		setupCEGUIWidgets();

		GraphicComponent::applyAllCustomProperties(containerName);
	}
	catch(exception &e) {
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error loading options: %s\n",__FILE__,__FUNCTION__,__LINE__,e.what());
		throw megaglest_runtime_error(string("Error loading options msg: ") + e.what());
	}
}

void MenuStateOptionsNetwork::reloadUI() {

	console.resetFonts();
	GraphicComponent::reloadFontsForRegisterGraphicComponents(containerName);

	setupCEGUIWidgetsText();
}

void MenuStateOptionsNetwork::setupCEGUIWidgets() {

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
			"TabControl/__auto_TabPane__/Network/ButtonSave",
			cegui_manager.getEventButtonClicked(), this);

	cegui_manager.setControlEventCallback(containerName,
			"TabControl/__auto_TabPane__/Network/ButtonReturn",
			cegui_manager.getEventButtonClicked(), this);

	cegui_manager.setControlEventCallback(containerName,
			"TabControl/__auto_TabPane__/Network/SpinnerServerPortNumber",
			cegui_manager.getEventSpinnerValueChanged(), this);

	cegui_manager.subscribeMessageBoxEventClicks(containerName, this);
	cegui_manager.subscribeMessageBoxEventClicks(containerName, this, "TabControl/__auto_TabPane__/Network/MsgBox");
}

void MenuStateOptionsNetwork::setupCEGUIWidgetsText() {

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

	cegui_manager.setSelectedTabPage("TabControl", "Network");

	if(cegui_manager.isChildControl(cegui_manager.getControl("TabControl/__auto_TabPane__/Network"),"MsgBox") == false) {
		cegui_manager.cloneMessageBoxControl("MsgBox", ctlNetwork);
	}

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlNetwork,"LabelExternalPortNumber"),lang.getString("PublishServerExternalPort","",false,true));

	string extPort= config.getString("PortExternal","not set");
	if(extPort == "not set" || extPort == "0") {
		extPort = "   ---   ";
	}
	else {
		extPort = "!!! " + extPort + " !!!";
	}
	cegui_manager.setControlText(cegui_manager.getChildControl(ctlNetwork,"LabelExternalPortNumberValue"),extPort);

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlNetwork,"LabelServerPortNumber"),lang.getString("ServerPort","",false,true));

	string portListString = config.getString("PortList",intToStr(GameConstants::serverPort).c_str());
	std::vector<std::string> portList;
	Tokenize(portListString,portList,",");

	string currentPort = config.getString("PortServer", intToStr(GameConstants::serverPort).c_str());

	int minPort = 9999999;
	int maxPort = 0;
	int selectedPort = 0;
	for(unsigned int idx = 0; idx < portList.size(); idx++) {
		if(portList[idx] != "" && IsNumeric(portList[idx].c_str(),false)) {
			int curPortNumber = strToInt(portList[idx]);
			minPort = min(minPort,curPortNumber);
			maxPort = max(maxPort,curPortNumber);
			if(currentPort == portList[idx]) {
				selectedPort = curPortNumber;
			}
		}
	}

	cegui_manager.setSpinnerControlValues(cegui_manager.getChildControl(ctlNetwork,"SpinnerServerPortNumber"),minPort,maxPort,selectedPort,1);

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlNetwork,"LabelFTPServerPortNumber"),lang.getString("FTPServerPort","",false,true));

	int FTPPort = config.getInt("FTPServerPort",intToStr(ServerSocket::getFTPServerPort()).c_str());
	cegui_manager.setControlText(cegui_manager.getChildControl(ctlNetwork,"LabelFTPServerPortNumberValue"),intToStr(FTPPort));

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlNetwork,"LabelFTPServerDataPorts"),lang.getString("FTPServerDataPort","",false,true));

	char szBuf[8096] = "";
	snprintf(szBuf,8096,"%d - %d",FTPPort + 1, FTPPort + GameConstants::maxPlayers);
	cegui_manager.setControlText(cegui_manager.getChildControl(ctlNetwork,"LabelFTPServerDataPortsValue"),szBuf);

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlNetwork,"LabelEnableFTPServer"),lang.getString("EnableFTPServer","",false,true));
	cegui_manager.setCheckboxControlChecked(cegui_manager.getChildControl(ctlNetwork,"CheckboxEnableFTPServer"),config.getBool("EnableFTPServer","true"));

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlNetwork,"LabelEnableFTPFileXfer"),lang.getString("EnableFTP","",false,true));
	cegui_manager.setCheckboxControlChecked(cegui_manager.getChildControl(ctlNetwork,"CheckboxEnableFTPFileXfer"),config.getBool("EnableFTPXfer","true"));

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlNetwork,"LabelEnableFTPTilesetFileXferInternet"),lang.getString("EnableFTPServerInternetTilesetXfer","",false,true));
	cegui_manager.setCheckboxControlChecked(cegui_manager.getChildControl(ctlNetwork,"CheckboxEnableFTPTilesetFileXferInternet"),config.getBool("EnableFTPServerInternetTilesetXfer","true"));

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlNetwork,"LabelEnableFTPTechtreeFileXferInternet"),lang.getString("EnableFTPServerInternetTechtreeXfer","",false,true));
	cegui_manager.setCheckboxControlChecked(cegui_manager.getChildControl(ctlNetwork,"CheckboxEnableFTPTechtreeFileXferInternet"),config.getBool("EnableFTPServerInternetTechtreeXfer","true"));

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlNetwork,"LabelEnablePrivacy"),lang.getString("PrivacyPlease","",false,true));
	cegui_manager.setCheckboxControlChecked(cegui_manager.getChildControl(ctlNetwork,"CheckboxEnablePrivacy"),config.getBool("PrivacyPlease","false"));

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlNetwork,"ButtonSave"),lang.getString("Save","",false,true));
	cegui_manager.setControlText(cegui_manager.getChildControl(ctlNetwork,"ButtonReturn"),lang.getString("Return","",false,true));
}


void MenuStateOptionsNetwork::callDelayedCallbacks() {
	if(hasDelayedCallbacks() == true) {
		for(unsigned int index = 0; index < delayedCallbackList.size(); ++index) {
			DelayCallbackFunction pCB = delayedCallbackList[index];
			(this->*pCB)();
		}
	}
}

void MenuStateOptionsNetwork::delayedCallbackFunctionSelectAudioTab() {
	CoreData &coreData				= CoreData::getInstance();
	SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	cegui_manager.unsubscribeEvents(this->containerName);

	soundRenderer.playFx(coreData.getClickSoundA());
	mainMenu->setState(new MenuStateOptionsSound(program, mainMenu,this->parentUI));
}

void MenuStateOptionsNetwork::delayedCallbackFunctionSelectKeyboardTab() {
	CoreData &coreData				= CoreData::getInstance();
	SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	cegui_manager.unsubscribeEvents(this->containerName);

	soundRenderer.playFx(coreData.getClickSoundA());
	mainMenu->setState(new MenuStateKeysetup(program, mainMenu,this->parentUI)); // open keyboard shortcuts setup screen
}

void MenuStateOptionsNetwork::delayedCallbackFunctionSelectMiscTab() {
	CoreData &coreData				= CoreData::getInstance();
	SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	cegui_manager.unsubscribeEvents(this->containerName);

	soundRenderer.playFx(coreData.getClickSoundA());
	mainMenu->setState(new MenuStateOptions(program, mainMenu,this->parentUI)); // open keyboard shortcuts setup screen
}

void MenuStateOptionsNetwork::delayedCallbackFunctionSelectVideoTab() {
	CoreData &coreData				= CoreData::getInstance();
	SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	cegui_manager.unsubscribeEvents(this->containerName);

	soundRenderer.playFx(coreData.getClickSoundA());
	mainMenu->setState(new MenuStateOptionsGraphics(program, mainMenu,this->parentUI)); // open keyboard shortcuts setup screen
}

void MenuStateOptionsNetwork::delayedCallbackFunctionOk() {

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	cegui_manager.unsubscribeEvents(this->containerName);

	mainMenu->setState(new MenuStateOptions(program, mainMenu));
}

void MenuStateOptionsNetwork::delayedCallbackFunctionReturn() {
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

bool MenuStateOptionsNetwork::EventCallback(CEGUI::Window *ctl, std::string name) {

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	if(name == cegui_manager.getEventTabControlSelectionChanged()) {

		if(cegui_manager.isSelectedTabPage("TabControl", "Audio") == true) {
			DelayCallbackFunction pCB = &MenuStateOptionsNetwork::delayedCallbackFunctionSelectAudioTab;
			delayedCallbackList.push_back(pCB);
		}
		else if(cegui_manager.isSelectedTabPage("TabControl", "Keyboard") == true) {
			DelayCallbackFunction pCB = &MenuStateOptionsNetwork::delayedCallbackFunctionSelectKeyboardTab;
			delayedCallbackList.push_back(pCB);
		}
		else if(cegui_manager.isSelectedTabPage("TabControl", "Misc") == true) {
			DelayCallbackFunction pCB = &MenuStateOptionsNetwork::delayedCallbackFunctionSelectMiscTab;
			delayedCallbackList.push_back(pCB);
		}
		else if(cegui_manager.isSelectedTabPage("TabControl", "Video") == true) {
			DelayCallbackFunction pCB = &MenuStateOptionsNetwork::delayedCallbackFunctionSelectVideoTab;
			delayedCallbackList.push_back(pCB);
		}
		return true;
	}
	else if(name == cegui_manager.getEventButtonClicked()) {

		if(cegui_manager.isControlMessageBoxOk(ctl,"TabControl/__auto_TabPane__/Network/MsgBox") == true) {

			CoreData &coreData				= CoreData::getInstance();
			SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

			soundRenderer.playFx(coreData.getClickSoundA());

			cegui_manager.hideMessageBox("TabControl/__auto_TabPane__/Network/MsgBox");
			return true;
		}
		else if(cegui_manager.isControlMessageBoxCancel(ctl,"TabControl/__auto_TabPane__/Network/MsgBox") == true) {

			CoreData &coreData				= CoreData::getInstance();
			SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

			soundRenderer.playFx(coreData.getClickSoundA());

			cegui_manager.hideMessageBox("TabControl/__auto_TabPane__/Network/MsgBox");
			return true;
		}
		else if(ctl == cegui_manager.getControl("TabControl/__auto_TabPane__/Network/ButtonSave")) {

			CoreData &coreData				= CoreData::getInstance();
			SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

			soundRenderer.playFx(coreData.getClickSoundA());
			saveConfig();

			return true;
		}
		else if(ctl == cegui_manager.getControl("TabControl/__auto_TabPane__/Network/ButtonReturn")) {

			DelayCallbackFunction pCB = &MenuStateOptionsNetwork::delayedCallbackFunctionReturn;
			delayedCallbackList.push_back(pCB);

			return true;
		}
		//printf("Line: %d\n",__LINE__);
		//cegui_manager.printDebugControlInfo(ctl);
	}
	else if(name == cegui_manager.getEventSpinnerValueChanged()) {
		if(ctl == cegui_manager.getControl("TabControl/__auto_TabPane__/Network/SpinnerServerPortNumber")) {

			int selectedPort = (int)cegui_manager.getSpinnerControlValue(
					cegui_manager.getControl("TabControl/__auto_TabPane__/Network/SpinnerServerPortNumber"));

			if(selectedPort < 10000) {
				selectedPort = GameConstants::serverPort;
			}
			cegui_manager.setControlText(cegui_manager.getControl("TabControl/__auto_TabPane__/Network/LabelFTPServerPortNumberValue"),intToStr(selectedPort+1));

			// use the following ports for ftp
			char szBuf[8096]="";
			snprintf(szBuf,8096,"%d - %d",selectedPort + 2, selectedPort + 1 + GameConstants::maxPlayers);
			cegui_manager.setControlText(cegui_manager.getControl("TabControl/__auto_TabPane__/Network/LabelFTPServerDataPortsValue"),szBuf);

			return true;
		}
	}

	return false;
}

void MenuStateOptionsNetwork::mouseClick(int x, int y, MouseButton mouseButton) { }

void MenuStateOptionsNetwork::mouseMove(int x, int y, const MouseState *ms) { }

void MenuStateOptionsNetwork::keyPress(SDL_KeyboardEvent c) {
	Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));
	if(isKeyPressed(configKeys.getSDLKey("SaveGUILayout"),c) == true) {
		GraphicComponent::saveAllCustomProperties(containerName);
	}
}

void MenuStateOptionsNetwork::render() {

	Renderer &renderer= Renderer::getInstance();
	renderer.renderConsole(&console,false,true);
	if(program != NULL) program->renderProgramMsgBox();
}

void MenuStateOptionsNetwork::saveConfig(){
	Config &config= Config::getInstance();
	Lang &lang= Lang::getInstance();
	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();

	lang.loadGameStrings(config.getString("Lang"));

	config.setInt("PortServer", (int)cegui_manager.getSpinnerControlValue(
			cegui_manager.getControl("TabControl/__auto_TabPane__/Network/SpinnerServerPortNumber")));
	config.setInt("FTPServerPort",config.getInt("PortServer")+1);

    config.setBool("EnableFTPXfer", cegui_manager.getCheckboxControlChecked(
			cegui_manager.getControl("TabControl/__auto_TabPane__/Network/CheckboxEnableFTPFileXfer")));
    config.setBool("EnableFTPServer", cegui_manager.getCheckboxControlChecked(
			cegui_manager.getControl("TabControl/__auto_TabPane__/Network/CheckboxEnableFTPServer")));

    config.setBool("EnableFTPServerInternetTilesetXfer", cegui_manager.getCheckboxControlChecked(
			cegui_manager.getControl("TabControl/__auto_TabPane__/Network/CheckboxEnableFTPTilesetFileXferInternet")));
    config.setBool("EnableFTPServerInternetTechtreeXfer", cegui_manager.getCheckboxControlChecked(
			cegui_manager.getControl("TabControl/__auto_TabPane__/Network/CheckboxEnableFTPTechtreeFileXferInternet")));

    config.setBool("PrivacyPlease", cegui_manager.getCheckboxControlChecked(
			cegui_manager.getControl("TabControl/__auto_TabPane__/Network/CheckboxEnablePrivacy")));

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
