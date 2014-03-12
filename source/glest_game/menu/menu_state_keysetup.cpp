// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2011-  by Titus Tscharntke
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "menu_state_keysetup.h"

#include "renderer.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "config.h"
#include "menu_state_options.h"
#include "menu_state_root.h"
#include "menu_state_keysetup.h"
#include "menu_state_options_graphics.h"
#include "menu_state_options_sound.h"
#include "menu_state_options_network.h"
#include "metrics.h"
#include "string_utils.h"
#include "megaglest_cegui_manager.h"

#include "leak_dumper.h"

namespace Glest{ namespace Game{


// =====================================================
// 	class MenuStateKeysetup
// =====================================================

MenuStateKeysetup::MenuStateKeysetup(Program *program, MainMenu *mainMenu,
		ProgramState **parentUI) :	MenuState(program, mainMenu, "config") {
	try {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		containerName = "KeySetup";

		mainMessageBoxState = 0;
		this->parentUI 		= parentUI;
		this->console.setOnlyChatMessagesInStoredLines(false);

		//Config &configKeys 	= Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));
		Config &config = Config::getInstance();
		Config &configKeys = Config::getInstance(
				std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys),
				std::pair<string,string>(Config::glestkeys_ini_filename,Config::glestuserkeys_ini_filename),
				std::pair<bool,bool>(true,false),config.getString("GlestKeysIniPath",""));

		mergedProperties 	= configKeys.getMergedProperties();
		masterProperties 	= configKeys.getMasterProperties();
		userProperties.clear();

		setupCEGUIWidgets();

		hotkeyChar 			= SDLK_UNKNOWN;
	}
	catch(const std::exception &ex) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s %d] Error detected:\n%s\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);

        mainMessageBoxState = 1;
        showMessageBox( "Error: " + string(ex.what()), "Error detected", false);
	}
}

void MenuStateKeysetup::reloadUI() {

	console.resetFonts();
	GraphicComponent::reloadFontsForRegisterGraphicComponents(containerName);

	setupCEGUIWidgets();
}

void MenuStateKeysetup::setupCEGUIWidgets() {

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
			"TabControl/__auto_TabPane__/Keyboard/ButtonSave",
			cegui_manager.getEventButtonClicked(), this);

	cegui_manager.setControlEventCallback(containerName,
			"TabControl/__auto_TabPane__/Keyboard/ButtonReturn",
			cegui_manager.getEventButtonClicked(), this);

	cegui_manager.setControlEventCallback(containerName,
			"TabControl/__auto_TabPane__/Keyboard/ButtonDefaults",
			cegui_manager.getEventButtonClicked(), this);

	cegui_manager.subscribeMessageBoxEventClicks(containerName, this);
	cegui_manager.subscribeMessageBoxEventClicks(containerName, this, "TabControl/__auto_TabPane__/Keyboard/MsgBox");
}

void MenuStateKeysetup::setupCEGUIWidgetsText() {

	Lang &lang								= Lang::getInstance();
	MegaGlest_CEGUIManager &cegui_manager 	= MegaGlest_CEGUIManager::getInstance();
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

	cegui_manager.setSelectedTabPage("TabControl", "Keyboard");

	if(cegui_manager.isChildControl(cegui_manager.getControl("TabControl/__auto_TabPane__/Keyboard"),"MsgBox") == false) {
		cegui_manager.cloneMessageBoxControl("MsgBox", ctlKeyboard);
	}

	if(this->parentUI != NULL) {
		cegui_manager.setControlVisible(ctlAudio,false);
		cegui_manager.setControlVisible(ctlMisc,false);
		cegui_manager.setControlVisible(ctlNetwork,false);
		cegui_manager.setControlVisible(ctlVideo,false);
	}

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlKeyboard,"LabelKeyboardSetupTitle"),lang.getString("Keyboardsetup","",false,true));

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlKeyboard,"LabelKeyboardTestTitle"),lang.getString("KeyboardsetupTest","",false,true));

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlKeyboard,"LabelKeyboardTestValue"),"");

	//Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));
	Config &config = Config::getInstance();
	Config &configKeys = Config::getInstance(
			std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys),
			std::pair<string,string>(Config::glestkeys_ini_filename,Config::glestuserkeys_ini_filename),
			std::pair<bool,bool>(true,false),config.getString("GlestKeysIniPath",""));

	vector<vector<string> > valueList;
	for(unsigned int index = 0; index < mergedProperties.size(); ++index) {

		string keyName = mergedProperties[index].second;
		if(keyName.length() > 0) {
			SDLKey c = configKeys.translateStringToSDLKey(keyName);
			if(c > SDLK_UNKNOWN && c < SDLK_LAST) {
				SDLKey keysym = static_cast<SDLKey>(c);
				// SDL skips capital letters
				if(keysym >= 65 && keysym <= 90) {
					keysym = (SDLKey)((int)keysym + 32);
				}
				keyName = SDL_GetKeyName(keysym);
			}
			else {
				keyName = "";
			}
			if(keyName == "unknown key" || keyName == "") {
				keyName = mergedProperties[index].second;
			}
		}

		vector<string> columnValues;
		columnValues.push_back(mergedProperties[index].first);
		columnValues.push_back(keyName);

		valueList.push_back(columnValues);
	}

	vector<pair<string,float> > columnValues;
	columnValues.push_back(make_pair("",1500));
	columnValues.push_back(make_pair("",200));
	cegui_manager.setColumnsForMultiColumnListControl(cegui_manager.getChildControl(ctlKeyboard,"MultiColumnListKeyMapping"), columnValues);
	cegui_manager.addItemsToMultiColumnListControl(cegui_manager.getChildControl(ctlKeyboard,"MultiColumnListKeyMapping"),valueList);

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlKeyboard,"ButtonSave"),lang.getString("Save","",false,true));
	cegui_manager.setControlText(cegui_manager.getChildControl(ctlKeyboard,"ButtonReturn"),lang.getString("Return","",false,true));
	cegui_manager.setControlText(cegui_manager.getChildControl(ctlKeyboard,"ButtonDefaults"),lang.getString("Defaults","",false,true));
}

void MenuStateKeysetup::callDelayedCallbacks() {
	if(hasDelayedCallbacks() == true) {
		for(unsigned int index = 0; index < delayedCallbackList.size(); ++index) {
			DelayCallbackFunction pCB = delayedCallbackList[index];
			(this->*pCB)();
		}
	}
}

void MenuStateKeysetup::delayedCallbackFunctionSelectAudioTab() {
	CoreData &coreData				= CoreData::getInstance();
	SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	cegui_manager.unsubscribeEvents(this->containerName);

	soundRenderer.playFx(coreData.getClickSoundA());
	mainMenu->setState(new MenuStateOptionsSound(program, mainMenu,this->parentUI));
}

void MenuStateKeysetup::delayedCallbackFunctionSelectMiscTab() {
	CoreData &coreData				= CoreData::getInstance();
	SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	cegui_manager.unsubscribeEvents(this->containerName);

	soundRenderer.playFx(coreData.getClickSoundA());
	mainMenu->setState(new MenuStateOptions(program, mainMenu,this->parentUI)); // open keyboard shortcuts setup screen
}

void MenuStateKeysetup::delayedCallbackFunctionSelectNetworkTab() {
	CoreData &coreData				= CoreData::getInstance();
	SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	cegui_manager.unsubscribeEvents(this->containerName);

	soundRenderer.playFx(coreData.getClickSoundA());
	mainMenu->setState(new MenuStateOptionsNetwork(program, mainMenu,this->parentUI)); // open keyboard shortcuts setup screen
}

void MenuStateKeysetup::delayedCallbackFunctionSelectVideoTab() {
	CoreData &coreData				= CoreData::getInstance();
	SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	cegui_manager.unsubscribeEvents(this->containerName);

	soundRenderer.playFx(coreData.getClickSoundA());
	mainMenu->setState(new MenuStateOptionsGraphics(program, mainMenu,this->parentUI)); // open keyboard shortcuts setup screen
}

void MenuStateKeysetup::delayedCallbackFunctionOk() {

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	cegui_manager.unsubscribeEvents(this->containerName);

	mainMenu->setState(new MenuStateOptions(program, mainMenu));
}

void MenuStateKeysetup::delayedCallbackFunctionReturn() {
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

void MenuStateKeysetup::delayedCallbackFunctionDefaults() {

	CoreData &coreData				= CoreData::getInstance();
	SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	cegui_manager.unsubscribeEvents(this->containerName);

	soundRenderer.playFx(coreData.getClickSoundB());

	//Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));
	Config &config = Config::getInstance();
	Config &configKeys = Config::getInstance(
			std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys),
			std::pair<string,string>(Config::glestkeys_ini_filename,Config::glestuserkeys_ini_filename),
			std::pair<bool,bool>(true,false),config.getString("GlestKeysIniPath",""));

	string userKeysFile = configKeys.getFileName(true);

	bool result = removeFile(userKeysFile.c_str());
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] delete file [%s] returned %d\n",__FILE__,__FUNCTION__,__LINE__,userKeysFile.c_str(),result);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] delete file [%s] returned %d\n",__FILE__,__FUNCTION__,__LINE__,userKeysFile.c_str(),result);
	configKeys.reload();

	if(this->parentUI != NULL) {
		// Set the parent pointer to NULL so the owner knows it was deleted
		*this->parentUI = NULL;
		// Delete the main menu
		delete mainMenu;
		return;
	}

	mainMenu->setState(new MenuStateKeysetup(program, mainMenu));
}

bool MenuStateKeysetup::EventCallback(CEGUI::Window *ctl, std::string name) {

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	if(name == cegui_manager.getEventTabControlSelectionChanged()) {

		if(cegui_manager.isSelectedTabPage("TabControl", "Audio") == true) {
			DelayCallbackFunction pCB = &MenuStateKeysetup::delayedCallbackFunctionSelectAudioTab;
			delayedCallbackList.push_back(pCB);
		}
		else if(cegui_manager.isSelectedTabPage("TabControl", "Misc") == true) {
			DelayCallbackFunction pCB = &MenuStateKeysetup::delayedCallbackFunctionSelectMiscTab;
			delayedCallbackList.push_back(pCB);
		}
		else if(cegui_manager.isSelectedTabPage("TabControl", "Network") == true) {
			DelayCallbackFunction pCB = &MenuStateKeysetup::delayedCallbackFunctionSelectNetworkTab;
			delayedCallbackList.push_back(pCB);
		}
		else if(cegui_manager.isSelectedTabPage("TabControl", "Video") == true) {
			DelayCallbackFunction pCB = &MenuStateKeysetup::delayedCallbackFunctionSelectVideoTab;
			delayedCallbackList.push_back(pCB);
		}
		return true;
	}
	else if(name == cegui_manager.getEventButtonClicked()) {

		if(cegui_manager.isControlMessageBoxOk(ctl,"TabControl/__auto_TabPane__/Keyboard/MsgBox") == true) {

			CoreData &coreData				= CoreData::getInstance();
			SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

			soundRenderer.playFx(coreData.getClickSoundB());

			cegui_manager.hideMessageBox("TabControl/__auto_TabPane__/Keyboard/MsgBox");
			return true;
		}
		else if(cegui_manager.isControlMessageBoxCancel(ctl,"TabControl/__auto_TabPane__/Keyboard/MsgBox") == true) {

			CoreData &coreData				= CoreData::getInstance();
			SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

			soundRenderer.playFx(coreData.getClickSoundB());

			cegui_manager.hideMessageBox("TabControl/__auto_TabPane__/Keyboard/MsgBox");
			return true;
		}
		else if(ctl == cegui_manager.getControl("TabControl/__auto_TabPane__/Keyboard/ButtonSave")) {

			CoreData &coreData				= CoreData::getInstance();
			SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

			soundRenderer.playFx(coreData.getClickSoundB());

	        if(userProperties.empty() == false) {
				//Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));
	        	Config &config = Config::getInstance();
	        	Config &configKeys = Config::getInstance(
	        			std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys),
	        			std::pair<string,string>(Config::glestkeys_ini_filename,Config::glestuserkeys_ini_filename),
	        			std::pair<bool,bool>(true,false),config.getString("GlestKeysIniPath",""));

				string userKeysFile = configKeys.getFileName(true);
		        if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] save file [%s] userProperties.size() = " MG_SIZE_T_SPECIFIER "\n",__FILE__,__FUNCTION__,__LINE__,userKeysFile.c_str(),userProperties.size());
		        if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] save file [%s] userProperties.size() = " MG_SIZE_T_SPECIFIER "\n",__FILE__,__FUNCTION__,__LINE__,userKeysFile.c_str(),userProperties.size());

				configKeys.setUserProperties(userProperties);
				configKeys.save();
				configKeys.reload();
	       }

			Lang &lang= Lang::getInstance();
			console.addLine(lang.getString("SettingsSaved"));
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			return true;
		}
		else if(ctl == cegui_manager.getControl("TabControl/__auto_TabPane__/Keyboard/ButtonReturn")) {

			DelayCallbackFunction pCB = &MenuStateKeysetup::delayedCallbackFunctionReturn;
			delayedCallbackList.push_back(pCB);

			return true;
		}
		else if(ctl == cegui_manager.getControl("TabControl/__auto_TabPane__/Keyboard/ButtonDefaults")) {

			DelayCallbackFunction pCB = &MenuStateKeysetup::delayedCallbackFunctionDefaults;
			delayedCallbackList.push_back(pCB);

			return true;
		}

		//printf("Line: %d\n",__LINE__);
		//cegui_manager.printDebugControlInfo(ctl);
	}
	else if(name == cegui_manager.getEventSpinnerValueChanged()) {
		if(ctl == cegui_manager.getControl("TabControl/__auto_TabPane__/Keyboard/SpinnerServerPortNumber")) {

			int selectedPort = (int)cegui_manager.getSpinnerControlValue(
					cegui_manager.getControl("TabControl/__auto_TabPane__/Keyboard/SpinnerServerPortNumber"));

			if(selectedPort < 10000) {
				selectedPort = GameConstants::serverPort;
			}
			cegui_manager.setControlText(cegui_manager.getControl("TabControl/__auto_TabPane__/Keyboard/LabelFTPServerPortNumberValue"),intToStr(selectedPort+1));

			// use the following ports for ftp
			char szBuf[8096]="";
			snprintf(szBuf,8096,"%d - %d",selectedPort + 2, selectedPort + 1 + GameConstants::maxPlayers);
			cegui_manager.setControlText(cegui_manager.getControl("TabControl/__auto_TabPane__/Keyboard/LabelFTPServerDataPortsValue"),szBuf);

			return true;
		}
	}
	else if(name == cegui_manager.getEventMultiColumnListSelectionChanged()) {
		if(ctl == cegui_manager.getControl("TabControl/__auto_TabPane__/Keyboard/MultiColumnListKeyMapping")) {

			return true;
		}
	}

	return false;
}

MenuStateKeysetup::~MenuStateKeysetup() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void MenuStateKeysetup::mouseClick(int x, int y, MouseButton mouseButton) { }

void MenuStateKeysetup::mouseUp(int x, int y, const MouseButton mouseButton) { }

void MenuStateKeysetup::mouseMove(int x, int y, const MouseState *ms) { }

void MenuStateKeysetup::render(){

	Renderer &renderer = Renderer::getInstance();
	renderer.renderConsole(&console,false,true);
	if(program != NULL) program->renderProgramMsgBox();
}

void MenuStateKeysetup::update() {
	console.update();

	MenuState::update();
}

void MenuStateKeysetup::showMessageBox(const string &text, const string &header, bool toggle){
	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	if(cegui_manager.isMessageBoxShowing("TabControl/__auto_TabPane__/Keyboard/MsgBox") == false) {

		Lang &lang= Lang::getInstance();
		cegui_manager.displayMessageBox(header, text, lang.getString("Ok","",false,true),lang.getString("Cancel","",false,true),"TabControl/__auto_TabPane__/Keyboard/MsgBox");
	}
	else {
		cegui_manager.hideMessageBox("TabControl/__auto_TabPane__/Keyboard/MsgBox");
	}
}

void MenuStateKeysetup::keyDown(SDL_KeyboardEvent key) {
	hotkeyChar = extractKeyPressed(key);

	string keyName = "";
	if(hotkeyChar > SDLK_UNKNOWN && hotkeyChar < SDLK_LAST) {
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] keyName [%s] char [%d][%d]\n",__FILE__,__FUNCTION__,__LINE__,keyName.c_str(),hotkeyChar,key.keysym.sym);
		keyName = SDL_GetKeyName(hotkeyChar);
	}

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] keyName [%s] char [%d][%d]\n",__FILE__,__FUNCTION__,__LINE__,keyName.c_str(),hotkeyChar,key.keysym.sym);

	char szCharText[20]="";
	snprintf(szCharText,20,"%c",hotkeyChar);
	char *utfStr = ConvertToUTF8(&szCharText[0]);

	char szBuf[8096] = "";
	snprintf(szBuf,8096,"%s [%s][%d][%d][%d][%d]",keyName.c_str(),utfStr,key.keysym.sym,hotkeyChar,key.keysym.unicode,key.keysym.mod);

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	cegui_manager.setControlText("TabControl/__auto_TabPane__/Keyboard/LabelKeyboardTestValue",szBuf, true);

	delete [] utfStr;

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] hotkeyChar [%d]\n",__FILE__,__FUNCTION__,__LINE__,hotkeyChar);
}

void MenuStateKeysetup::keyUp(SDL_KeyboardEvent key) {

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	int selectedRowIndex = cegui_manager.getSelectedRowForMultiColumnListControl(cegui_manager.getControl(
			"TabControl/__auto_TabPane__/Keyboard/MultiColumnListKeyMapping"));
    if(selectedRowIndex >= 0) {
    	if(hotkeyChar != 0) {
    		if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] char [%d][%d]\n",__FILE__,__FUNCTION__,__LINE__,hotkeyChar,key.keysym.sym);

    		string keyName = "";
			if(hotkeyChar > SDLK_UNKNOWN && hotkeyChar < SDLK_LAST) {
				keyName = SDL_GetKeyName(hotkeyChar);
			}
			key.keysym.sym = hotkeyChar;

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] keyName [%s] char [%d][%d]\n",__FILE__,__FUNCTION__,__LINE__,keyName.c_str(),hotkeyChar,key.keysym.sym);

			if(keyName != "unknown key") {
				//printf("Set pressed key to: %s\n",keyName.c_str());

				cegui_manager.setSelectedItemTextForMultiColumnListControl(
						cegui_manager.getControl(
								"TabControl/__auto_TabPane__/Keyboard/MultiColumnListKeyMapping"),
									keyName, 1);

				pair<string,string> &nameValuePair = mergedProperties[selectedRowIndex];
				bool isNewUserKeyEntry = true;
				for(unsigned int index = 0; index < userProperties.size(); ++index) {
					string hotKeyName = userProperties[index].first;
					if(nameValuePair.first == hotKeyName) {
						userProperties[index].second = keyName;
						isNewUserKeyEntry = false;
						break;
					}
				}
				if(isNewUserKeyEntry == true) {
					pair<string,string> newNameValuePair = nameValuePair;
					newNameValuePair.second = keyName;
					userProperties.push_back(newNameValuePair);
				}
			}
    	}
        hotkeyChar = SDLK_UNKNOWN;
    }
}

}}//end namespace
