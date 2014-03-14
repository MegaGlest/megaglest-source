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

#include "menu_state_options.h"

#include "renderer.h"
#include "game.h"
#include "program.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "config.h"
#include "menu_state_root.h"
#include "util.h"
#include "menu_state_graphic_info.h"
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
MenuStateOptions::MenuStateOptions(Program *program, MainMenu *mainMenu, ProgramState **parentUI) :
	MenuState(program, mainMenu, "config") {

	try {
		containerName = "OptionsMisc";

		this->parentUI=parentUI;
		this->console.setOnlyChatMessagesInStoredLines(false);

		setupCEGUIWidgets();

		mainMessageBoxState = 0;
		luaMessageBoxState  = 0;

		languageList = Lang::getInstance().getDiscoveredLanguageList(true);
		setupTransifexUI();

		GraphicComponent::applyAllCustomProperties(containerName);
	}
	catch(exception &e) {
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error loading options: %s\n",__FILE__,__FUNCTION__,__LINE__,e.what());
		throw megaglest_runtime_error(string("Error loading options msg: ") + e.what());
	}
}

void MenuStateOptions::reloadUI() {
	console.resetFonts();
	GraphicComponent::reloadFontsForRegisterGraphicComponents(containerName);

	setupCEGUIWidgetsText();
}

void MenuStateOptions::setupCEGUIWidgets() {

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
			"TabControl/__auto_TabPane__/Misc/ComboBoxLanguage",
					cegui_manager.getEventComboboxClicked(), this);

	cegui_manager.setControlEventCallback(containerName,
			"TabControl/__auto_TabPane__/Misc/ComboBoxLanguage",
					cegui_manager.getEventComboboxChangeAccepted(), this);

	cegui_manager.setControlEventCallback(containerName,
			"TabControl/__auto_TabPane__/Misc/CheckboxDisableLuaSandbox",
			cegui_manager.getEventCheckboxClicked(), this);

	cegui_manager.setControlEventCallback(containerName,
			"TabControl/__auto_TabPane__/Misc/CheckboxAdvancedTranslation",
			cegui_manager.getEventCheckboxClicked(), this);

	cegui_manager.setControlEventCallback(containerName,
			"TabControl/__auto_TabPane__/Misc/GroupBoxAdvancedTranslation/__auto_contentpane__/ButtonDownloadFromTransifex",
			cegui_manager.getEventButtonClicked(), this);

	cegui_manager.setControlEventCallback(containerName,
			"TabControl/__auto_TabPane__/Misc/GroupBoxAdvancedTranslation/__auto_contentpane__/ButtonDeleteDownloadedTransifexFiles",
			cegui_manager.getEventButtonClicked(), this);

	cegui_manager.setControlEventCallback(containerName,
			"TabControl/__auto_TabPane__/Misc/ButtonSave",
			cegui_manager.getEventButtonClicked(), this);

	cegui_manager.setControlEventCallback(containerName,
			"TabControl/__auto_TabPane__/Misc/ButtonReturn",
			cegui_manager.getEventButtonClicked(), this);

	cegui_manager.subscribeMessageBoxEventClicks(containerName, this);
	cegui_manager.subscribeMessageBoxEventClicks(containerName, this, "TabControl/__auto_TabPane__/Misc/LuaMsgBox");

	setupTransifexUI();
}

void MenuStateOptions::setupCEGUIWidgetsText() {

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

	cegui_manager.setSelectedTabPage("TabControl", "Misc");

	//cegui_manager.dumpWindowNames("Test #1");
	if(cegui_manager.isChildControl(cegui_manager.getControl("TabControl/__auto_TabPane__/Misc"),"LuaMsgBox") == false) {
		cegui_manager.cloneMessageBoxControl("LuaMsgBox", ctlMisc);
	}
	cegui_manager.setControlText(cegui_manager.getChildControl(ctlMisc,"LabelLanguage"),lang.getString("Language","",false,true));

	languageList = lang.getDiscoveredLanguageList(true);
	pair<string,string> defaultLang = lang.getNavtiveNameFromLanguageName(config.getString("Lang"));
	if(defaultLang.first == "" && defaultLang.second == "") {
		defaultLang = lang.getNavtiveNameFromLanguageName(lang.getDefaultLanguage());
	}
	string defaultLanguageText = defaultLang.second + "-" + defaultLang.first;
	string defaultLanguageTextFormatted = defaultLanguageText;

	int langCount = 0;
	map<string,int> langResultsMap;
	vector<string> langResults;
	for(map<string,string>::iterator iterMap = languageList.begin();
			iterMap != languageList.end(); ++iterMap) {

		string language = iterMap->first + "-" + iterMap->second;
		string languageFont = "";
		if(lang.hasString("MEGAGLEST_FONT",iterMap->first) == true) {

			bool langIsDefault = false;
			if(defaultLanguageText == language) {
				langIsDefault = true;
			}
			string fontFile = lang.getString("MEGAGLEST_FONT",iterMap->first);
			if(	lang.hasString("MEGAGLEST_FONT_FAMILY")) {
				string fontFamily = lang.getString("MEGAGLEST_FONT_FAMILY",iterMap->first);
				fontFile = findFont(fontFile.c_str(),fontFamily.c_str());
			}

			cegui_manager.addFont("MEGAGLEST_FONT_" + iterMap->first, fontFile, 10.0f);
			language = iterMap->first + "-[font='" + "MEGAGLEST_FONT_" + iterMap->first + "-10.00']" + iterMap->second;

			if(langIsDefault == true) {
				defaultLanguageTextFormatted = language;
			}
		}
		langResults.push_back(language);
		langResultsMap[language] = langCount;
		langCount++;
	}

	cegui_manager.addItemsToComboBoxControl(
			cegui_manager.getChildControl(ctlMisc,"ComboBoxLanguage"), langResultsMap,false);

	cegui_manager.setSelectedItemInComboBoxControl(
			cegui_manager.getChildControl(ctlMisc,"ComboBoxLanguage"), defaultLanguageTextFormatted,false);
	cegui_manager.setControlText(cegui_manager.getChildControl(ctlMisc,"ComboBoxLanguage"),defaultLanguageText);

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlMisc,"LabelPlayerName"),lang.getString("Playername","",false,true));
	cegui_manager.setControlText(cegui_manager.getChildControl(ctlMisc,"EditboxPlayerName"),config.getString("NetPlayerName",Socket::getHostName().c_str()));

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlMisc,"LabelFontAdjustment"),lang.getString("FontSizeAdjustment","",false,true));
	cegui_manager.setSpinnerControlValues(cegui_manager.getChildControl(ctlMisc,"SpinnerFontAdjustment"),-5,5,config.getInt("FontSizeAdjustment"));

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlMisc,"LabelScreenshotFormat"),lang.getString("ScreenShotFileType","",false,true));
	vector<string> langScreenshotFormats;
	langScreenshotFormats.push_back("bmp");
	langScreenshotFormats.push_back("jpg");
	langScreenshotFormats.push_back("png");
	langScreenshotFormats.push_back("tga");

	cegui_manager.addItemsToComboBoxControl(
			cegui_manager.getChildControl(ctlMisc,"ComboBoxScreenshotFormat"), langScreenshotFormats);
	cegui_manager.setSelectedItemInComboBoxControl(
			cegui_manager.getChildControl(ctlMisc,"ComboBoxScreenshotFormat"), config.getString("ScreenShotFileType","jpg"));

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlMisc,"LabelShowScreenshotSaved"),lang.getString("ScreenShotConsoleText","",false,true));
	cegui_manager.setCheckboxControlChecked(cegui_manager.getChildControl(ctlMisc,"CheckboxShowScreenshotSaved"),!config.getBool("DisableScreenshotConsoleText","false"));

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlMisc,"LabelMouseMovesCamera"),lang.getString("MouseScrollsWorld","",false,true));
	cegui_manager.setCheckboxControlChecked(cegui_manager.getChildControl(ctlMisc,"CheckboxMouseMovesCamera"),config.getBool("MouseMoveScrollsWorld","true"));

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlMisc,"LabelCameraMoveSpeed"),lang.getString("CameraMoveSpeed","",false,true));
	cegui_manager.setSpinnerControlValues(cegui_manager.getChildControl(ctlMisc,"SpinnerCameraMoveSpeed"),15,50,config.getFloat("CameraMoveSpeed","15"),5);

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlMisc,"LabelVisibleHUD"),lang.getString("VisibleHUD","",false,true));
	cegui_manager.setCheckboxControlChecked(cegui_manager.getChildControl(ctlMisc,"CheckboxVisibleHUD"),config.getBool("VisibleHud","true"));

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlMisc,"LabelChatRemainsActive"),lang.getString("ChatStaysActive","",false,true));
	cegui_manager.setCheckboxControlChecked(cegui_manager.getChildControl(ctlMisc,"CheckboxChatRemainsActive"),config.getBool("ChatStaysActive","false"));

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlMisc,"LabelDisplayRealAndGameTime"),lang.getString("TimeDisplay","",false,true));
	cegui_manager.setCheckboxControlChecked(cegui_manager.getChildControl(ctlMisc,"CheckboxDisplayRealAndGameTime"),config.getBool("TimeDisplay","true"));

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlMisc,"LabelDisableLuaSandbox"),lang.getString("LuaDisableSecuritySandbox","",false,true));
	cegui_manager.setCheckboxControlChecked(cegui_manager.getChildControl(ctlMisc,"CheckboxDisableLuaSandbox"),config.getBool("DisableLuaSandbox","false"));

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlMisc,"LabelAdvancedTranslation"),lang.getString("CustomTranslation","",false,true));

	cegui_manager.setCheckboxControlChecked(cegui_manager.getChildControl(ctlMisc,"CheckboxAdvancedTranslation"),false);

	cegui_manager.setControlText("TabControl/__auto_TabPane__/Misc/GroupBoxAdvancedTranslation/__auto_contentpane__/LabelTransifexUsername",lang.getString("TransifexUserName","",false,true));
	cegui_manager.setControlText("TabControl/__auto_TabPane__/Misc/GroupBoxAdvancedTranslation/__auto_contentpane__/EditboxTransifexUsername",config.getString("TranslationGetURLUser","<none>"));

	cegui_manager.setControlText("TabControl/__auto_TabPane__/Misc/GroupBoxAdvancedTranslation/__auto_contentpane__/LabelTransifexPassword",lang.getString("TransifexPwd","",false,true));
	cegui_manager.setControlText("TabControl/__auto_TabPane__/Misc/GroupBoxAdvancedTranslation/__auto_contentpane__/EditboxTransifexPassword",config.getString("TranslationGetURLPassword",""));

	cegui_manager.setControlText("TabControl/__auto_TabPane__/Misc/GroupBoxAdvancedTranslation/__auto_contentpane__/LabelTransifexLanguageCode",lang.getString("TransifexI18N","",false,true));
	cegui_manager.setControlText("TabControl/__auto_TabPane__/Misc/GroupBoxAdvancedTranslation/__auto_contentpane__/EditboxTransifexLanguageCode",config.getString("TranslationGetURLLanguage","en"));

	cegui_manager.setControlText("TabControl/__auto_TabPane__/Misc/GroupBoxAdvancedTranslation/__auto_contentpane__/ButtonDownloadFromTransifex",lang.getString("TransifexGetLanguageFiles","",false,true));
	cegui_manager.setControlText("TabControl/__auto_TabPane__/Misc/GroupBoxAdvancedTranslation/__auto_contentpane__/ButtonDeleteDownloadedTransifexFiles",lang.getString("TransifexDeleteLanguageFiles","",false,true));

	cegui_manager.setControlText("TabControl/__auto_TabPane__/Misc/ButtonSave",lang.getString("Save","",false,true));
	cegui_manager.setControlText("TabControl/__auto_TabPane__/Misc/ButtonReturn",lang.getString("Return","",false,true));

	cegui_manager.hideMessageBox();
	cegui_manager.hideMessageBox("TabControl/__auto_TabPane__/Misc/LuaMsgBox");

	//throw runtime_error("test!");
}

void MenuStateOptions::callDelayedCallbacks() {
	if(hasDelayedCallbacks() == true) {
		for(unsigned int index = 0; index < delayedCallbackList.size(); ++index) {
			DelayCallbackFunction pCB = delayedCallbackList[index];
			(this->*pCB)();
		}
	}
}

void MenuStateOptions::delayedCallbackFunctionSelectAudioTab() {
	CoreData &coreData				= CoreData::getInstance();
	SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	cegui_manager.unsubscribeEvents(this->containerName);

	soundRenderer.playFx(coreData.getClickSoundA());
	mainMenu->setState(new MenuStateOptionsSound(program, mainMenu,this->parentUI)); // open keyboard shortcuts setup screen
}

void MenuStateOptions::delayedCallbackFunctionSelectKeyboardTab() {
	CoreData &coreData				= CoreData::getInstance();
	SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	cegui_manager.unsubscribeEvents(this->containerName);

	soundRenderer.playFx(coreData.getClickSoundA());
	mainMenu->setState(new MenuStateKeysetup(program, mainMenu,this->parentUI)); // open keyboard shortcuts setup screen
}

void MenuStateOptions::delayedCallbackFunctionSelectNetworkTab() {
	CoreData &coreData				= CoreData::getInstance();
	SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	cegui_manager.unsubscribeEvents(this->containerName);

	soundRenderer.playFx(coreData.getClickSoundA());
	mainMenu->setState(new MenuStateOptionsNetwork(program, mainMenu,this->parentUI)); // open keyboard shortcuts setup screen
}

void MenuStateOptions::delayedCallbackFunctionVideoTab() {
	CoreData &coreData				= CoreData::getInstance();
	SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	cegui_manager.unsubscribeEvents(this->containerName);

	soundRenderer.playFx(coreData.getClickSoundA());
	mainMenu->setState(new MenuStateOptionsGraphics(program, mainMenu,this->parentUI)); // open keyboard shortcuts setup screen
}

void MenuStateOptions::delayedCallbackFunctionReturn() {
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

void MenuStateOptions::delayedCallbackFunctionOk() {

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	cegui_manager.unsubscribeEvents(this->containerName);

	mainMenu->setState(new MenuStateRoot(program, mainMenu));
}

bool MenuStateOptions::EventCallback(CEGUI::Window *ctl, std::string name) {

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	if(name == cegui_manager.getEventTabControlSelectionChanged()) {

		if(cegui_manager.isSelectedTabPage("TabControl", "Audio") == true) {

			DelayCallbackFunction pCB = &MenuStateOptions::delayedCallbackFunctionSelectAudioTab;
			delayedCallbackList.push_back(pCB);
		}
		else if(cegui_manager.isSelectedTabPage("TabControl", "Keyboard") == true) {

			DelayCallbackFunction pCB = &MenuStateOptions::delayedCallbackFunctionSelectKeyboardTab;
			delayedCallbackList.push_back(pCB);
		}
		else if(cegui_manager.isSelectedTabPage("TabControl", "Network") == true) {

			DelayCallbackFunction pCB = &MenuStateOptions::delayedCallbackFunctionSelectNetworkTab;
			delayedCallbackList.push_back(pCB);
		}
		else if(cegui_manager.isSelectedTabPage("TabControl", "Video") == true) {

			DelayCallbackFunction pCB = &MenuStateOptions::delayedCallbackFunctionVideoTab;
			delayedCallbackList.push_back(pCB);
		}

		return true;
	}
	else if(name == cegui_manager.getEventButtonClicked()) {

		if(cegui_manager.isControlMessageBoxOk(ctl) == true) {

			CoreData &coreData				= CoreData::getInstance();
			SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

			soundRenderer.playFx(coreData.getClickSoundA());
			if(mainMessageBoxState == 1) {
				mainMessageBoxState = 0;
				saveConfig();

				//mainMenu->setState(new MenuStateRoot(program, mainMenu));
				DelayCallbackFunction pCB = &MenuStateOptions::delayedCallbackFunctionOk;
				delayedCallbackList.push_back(pCB);

				return true;
			}

			cegui_manager.hideMessageBox();
		}
		else if(cegui_manager.isControlMessageBoxCancel(ctl) == true) {

			CoreData &coreData				= CoreData::getInstance();
			SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

			soundRenderer.playFx(coreData.getClickSoundA());

			if(mainMessageBoxState == 1) {
				mainMessageBoxState = 0;
			}

			cegui_manager.hideMessageBox();
		}
		else if(cegui_manager.isControlMessageBoxOk(ctl,"TabControl/__auto_TabPane__/Misc/LuaMsgBox") == true) {

			CoreData &coreData				= CoreData::getInstance();
			SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

			soundRenderer.playFx(coreData.getClickSoundA());
			cegui_manager.setCheckboxControlChecked(
										cegui_manager.getControl("TabControl/__auto_TabPane__/Misc/CheckboxDisableLuaSandbox"),true,true);

			cegui_manager.hideMessageBox("TabControl/__auto_TabPane__/Misc/LuaMsgBox");
		}
		else if(cegui_manager.isControlMessageBoxCancel(ctl,"TabControl/__auto_TabPane__/Misc/LuaMsgBox") == true) {

			CoreData &coreData				= CoreData::getInstance();
			SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

			soundRenderer.playFx(coreData.getClickSoundA());
			cegui_manager.setCheckboxControlChecked(
										cegui_manager.getControl("TabControl/__auto_TabPane__/Misc/CheckboxDisableLuaSandbox"),false,true);

			cegui_manager.hideMessageBox("TabControl/__auto_TabPane__/Misc/LuaMsgBox");

		}
		else if(ctl == cegui_manager.getControl(
				"TabControl/__auto_TabPane__/Misc/GroupBoxAdvancedTranslation/__auto_contentpane__/ButtonDownloadFromTransifex")) {

			CoreData &coreData				= CoreData::getInstance();
			SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

			soundRenderer.playFx(coreData.getClickSoundA());

			string orig_txnURLUser = Config::getInstance().getString("TranslationGetURLUser");
			//string orig_txnURLPwd = Config::getInstance().getString("TranslationGetURLPassword","");
			string orig_txnURLLang = Config::getInstance().getString("TranslationGetURLLanguage");

			string tsfUserName = cegui_manager.getControlText("TabControl/__auto_TabPane__/Misc/GroupBoxAdvancedTranslation/__auto_contentpane__/EditboxTransifexUsername");
			string tsfLanguage = cegui_manager.getControlText("TabControl/__auto_TabPane__/Misc/GroupBoxAdvancedTranslation/__auto_contentpane__/EditboxTransifexLanguageCode");

			Config::getInstance().setString("TranslationGetURLUser",tsfUserName);
			Config::getInstance().setString("TranslationGetURLPassword",
					cegui_manager.getControlText(
							"TabControl/__auto_TabPane__/Misc/GroupBoxAdvancedTranslation/__auto_contentpane__/EditboxTransifexPassword"),true);
			Config::getInstance().setString("TranslationGetURLLanguage",tsfLanguage);

			bool saveChanges = (orig_txnURLUser != tsfUserName ||
								orig_txnURLLang != tsfLanguage);

			string txnURL 				 = Config::getInstance().getString("TranslationGetURL");
			string txnURLUser 			 = Config::getInstance().getString("TranslationGetURLUser");
			string txnURLPwd 			 = Config::getInstance().getString("TranslationGetURLPassword");
			string txnURLLang 			 = Config::getInstance().getString("TranslationGetURLLanguage");
			string txnURLFileList 		 = Config::getInstance().getString("TranslationGetURLFileList");
			string txnURLFileListMapping = Config::getInstance().getString("TranslationGetURLFileListMapping");
			string txnURLDetails 		 = Config::getInstance().getString("TranslationGetURLDetails");
			string credentials 			 = txnURLUser + ":" + txnURLPwd;

			printf("URL1 [%s] credentials [%s]\n",txnURL.c_str(),credentials.c_str());

			//txnURLUser = SystemFlags::escapeURL(txnURLUser,handle);
			//replaceAll(txnURL,"$user",txnURLUser);

			//printf("URL2 [%s]\n",txnURL.c_str());

			//txnURLPwd = SystemFlags::escapeURL(txnURLPwd,handle);
			//replaceAll(txnURL,"$password",txnURLPwd);

			//printf("URL3 [%s]\n",txnURL.c_str());

			replaceAll(txnURL,"$language",txnURLLang);

			printf("URL4 [%s]\n",txnURL.c_str());

			//txnURLFileList
			vector<string> languageFiles;
			Tokenize(txnURLFileList,languageFiles,"|");

			vector<string> languageFileMappings;
			Tokenize(txnURLFileListMapping,languageFileMappings,"|");

			printf("URL5 file count = " MG_SIZE_T_SPECIFIER ", " MG_SIZE_T_SPECIFIER " [%s]\n",languageFiles.size(),languageFileMappings.size(),(languageFiles.empty() == false ? languageFiles[0].c_str() : ""));

			if(languageFiles.empty() == false) {

				bool gotDownloads 	= false;
				bool reloadLanguage = false;
				string langName 	= "";
				CURL *handle 		= SystemFlags::initHTTP();

				for(unsigned int i = 0; i < languageFiles.size(); ++i) {
					string fileURL = txnURL;
					replaceAll(fileURL,"$file",languageFiles[i]);

					if(langName == "") {
						// Get language name for file
						string fileURLDetails = txnURLDetails;
						replaceAll(fileURLDetails,"$file",languageFiles[0]);

						printf(" i = %u Trying [%s]\n",i,fileURLDetails.c_str());
						curl_easy_setopt(handle, CURLOPT_VERBOSE, 1);
						curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0L);
						curl_easy_setopt(handle, CURLOPT_USERPWD, credentials.c_str());
						std::string fileDataDetails = SystemFlags::getHTTP(fileURLDetails,handle);

						//		 "available_languages": [
						//		        {
						//		            "code_aliases": " ",
						//		            "code": "ca",
						//		            "name": "Catalan"
						//		        },
						//		        {
						//		            "code_aliases": " ",
						//		            "code": "zh",
						//		            "name": "Chinese"
						//		        },
						// curl -i -L --user softcoder -X GET https://www.transifex.com/api/2/project/megaglest/resource/main-language-file/?details

						string search_detail_key = "\"code\": \"" + txnURLLang + "\"";
						size_t posDetails = fileDataDetails.find( search_detail_key, 0 );
						if( posDetails != fileDataDetails.npos ) {
							posDetails = fileDataDetails.find( "\"name\": \"", posDetails+search_detail_key.length() );

							if( posDetails != fileDataDetails.npos ) {

								size_t posDetailsEnd = fileDataDetails.find( "\"", posDetails + 9 );

								langName = fileDataDetails.substr(posDetails + 9, posDetailsEnd - (posDetails + 9));
								replaceAll(langName,",","");
								replaceAll(langName,"\\","");
								replaceAll(langName,"/","");
								replaceAll(langName,"?","");
								replaceAll(langName,":","");
								replaceAll(langName,"@","");
								replaceAll(langName,"!","");
								replaceAll(langName,"*","");
								langName = trim(langName);
								replaceAll(langName," ","-");
							}

							printf("PARSED Language filename [%s]\n",langName.c_str());
						}
					}

					printf("i = %u Trying [%s]\n",i,fileURL.c_str());
					curl_easy_setopt(handle, CURLOPT_VERBOSE, 1);
					curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0L);
					curl_easy_setopt(handle, CURLOPT_USERPWD, credentials.c_str());
					std::string fileData = SystemFlags::getHTTP(fileURL,handle);

					// "content": "
					// ",
					// "mimetype": "text/plain"
					size_t pos = fileData.find( "\"content\": \"", 0 );
					if( pos != fileData.npos ) {
						fileData = fileData.substr(pos+12, fileData.length());

						pos = fileData.find( "\",\n", 0 );
						if( pos != fileData.npos ) {
							fileData = fileData.substr(0, pos);
						}

						replaceAll(fileData,"\\\\n","$requires-newline$");
						replaceAll(fileData,"\\n","\n");
						replaceAll(fileData,"$requires-newline$","\\n");

						//replaceAll(fileData,"&quot;","\"");
						replaceAllHTMLEntities(fileData);


						printf("PARSED Language text\n[%s]\n",fileData.c_str());

						//vector<string> languageName;
						//Tokenize(fileData,languageName," ");
						//printf("PARSED Language Name guessed to be [%s]\n",languageName[1].c_str());

						//string data_path= getGameReadWritePath(GameConstants::path_data_CacheLookupKey);
						//if(data_path != ""){
							//endPathWithSlash(data_path);
						//}
						Config &config = Config::getInstance();
						string data_path = config.getString("UserData_Root","");
						if(data_path != "") {
							endPathWithSlash(data_path);
						}

						string outputFile = languageFileMappings[i];
						replaceAll(outputFile,"$language",toLower(langName));
						//string lngFile = getGameCustomCoreDataPath(data_path, "data/lang/" + toLower(languageName[1]) + ".lng");
						string lngFile = getGameCustomCoreDataPath(data_path, outputFile);

						string lngPath = extractDirectoryPathFromFile(lngFile);
						createDirectoryPaths(lngPath);

						printf("Save data to Language Name [%s]\n",lngFile.c_str());
						saveDataToFile(lngFile, fileData);
						gotDownloads = true;

						reloadLanguage = true;
						if(saveChanges == true) {
							saveChanges = false;
							config.save();
						}
					}
					else {
						printf("UNPARSED Language text\n[%s]\n",fileData.c_str());
					}
				}

				SystemFlags::cleanupHTTP(&handle);

				if(reloadLanguage == true && langName != "") {
					Lang &lang= Lang::getInstance();
					if(lang.isLanguageLocal(toLower(langName)) == true) {
						lang.loadGameStrings(toLower(langName));
					}
				}

				if(gotDownloads == true) {
					mainMessageBoxState = 0;
					Lang &lang= Lang::getInstance();
					showMessageBox(lang.getString("TransifexDownloadSuccess","",false,true) +
													"\n" + langName,
								   lang.getString("Notice","",false,true), true);
				}
			}
		}
		else if(ctl == cegui_manager.getControl(
				"TabControl/__auto_TabPane__/Misc/GroupBoxAdvancedTranslation/__auto_contentpane__/ButtonDeleteDownloadedTransifexFiles")) {

			CoreData &coreData				= CoreData::getInstance();
			SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

			soundRenderer.playFx(coreData.getClickSoundA());

			string tsfLanguage = cegui_manager.getControlText(
					"TabControl/__auto_TabPane__/Misc/GroupBoxAdvancedTranslation/__auto_contentpane__/EditboxTransifexLanguageCode");

			if(tsfLanguage != "") {
				Lang &lang		= Lang::getInstance();
				string language = lang.getLanguageFile(tsfLanguage);

				if(language != "") {
					bool foundFilesToDelete = false;

					Config &config 	 = Config::getInstance();
					string data_path = config.getString("UserData_Root","");
					if(data_path != "") {
						endPathWithSlash(data_path);
					}

					if(data_path != "") {

						string txnURLFileListMapping = Config::getInstance().getString("TranslationGetURLFileListMapping");
						vector<string> languageFileMappings;
						Tokenize(txnURLFileListMapping,languageFileMappings,"|");

						// Cleanup Scenarios
						vector<string> scenarioPaths = config.getPathListForType(ptScenarios);
						if(scenarioPaths.size() > 1) {
							string &scenarioPath = scenarioPaths[1];
							endPathWithSlash(scenarioPath);

							vector<string> scenarioList;
							findDirs(scenarioPath, scenarioList, false,false);
							for(unsigned int i = 0; i < scenarioList.size(); ++i) {
								string scenario = scenarioList[i];

								vector<string> langResults;
								findAll(scenarioPath + scenario + "/*.lng", langResults, false, false);
								for(unsigned int j = 0; j < langResults.size(); ++j) {
									string testLanguage = langResults[j];

									string removeLngFile = scenarioPath + scenario + "/" + testLanguage;

									if(EndsWith(testLanguage,language + ".lng") == true) {

										for(unsigned int k = 0; k < languageFileMappings.size(); ++k) {
											string mapping = languageFileMappings[k];
											replaceAll(mapping,"$language",language);

											//printf("Comparing found [%s] with [%s]\n",removeLngFile.c_str(),mapping.c_str());

											if(EndsWith(removeLngFile,mapping) == true) {
												printf("About to delete file [%s]\n",removeLngFile.c_str());
												removeFile(removeLngFile);
												foundFilesToDelete = true;
												break;
											}
										}
									}
								}
							}
						}

						// Cleanup tutorials
						vector<string> tutorialPaths = config.getPathListForType(ptTutorials);
						if(tutorialPaths.size() > 1) {
							string &tutorialPath = tutorialPaths[1];
							endPathWithSlash(tutorialPath);

							vector<string> tutorialList;
							findDirs(tutorialPath, tutorialList, false, false);
							for(unsigned int i = 0; i < tutorialList.size(); ++i) {
								string tutorial = tutorialList[i];

								vector<string> langResults;
								findAll(tutorialPath + tutorial + "/*.lng", langResults, false, false);
								for(unsigned int j = 0; j < langResults.size(); ++j) {
									string testLanguage = langResults[j];

									string removeLngFile = tutorialPath + tutorial + "/" + testLanguage;
									if(EndsWith(testLanguage,language + ".lng") == true) {


										for(unsigned int k = 0; k < languageFileMappings.size(); ++k) {
											string mapping = languageFileMappings[k];
											replaceAll(mapping,"$language",language);

											//printf("Comparing found [%s] with [%s]\n",removeLngFile.c_str(),mapping.c_str());

											if(EndsWith(removeLngFile,mapping) == true) {
												printf("About to delete file [%s]\n",removeLngFile.c_str());
												removeFile(removeLngFile);
												foundFilesToDelete = true;
												break;
											}
										}
									}
								}
							}
						}

						// Cleanup main and hint language files
						string mainLngFile = data_path + "data/lang/" + language + ".lng";
						if(fileExists(mainLngFile) == true) {

							for(unsigned int k = 0; k < languageFileMappings.size(); ++k) {
								string mapping = languageFileMappings[k];
								replaceAll(mapping,"$language",language);

								if(EndsWith(mainLngFile,mapping) == true) {
									printf("About to delete file [%s]\n",mainLngFile.c_str());
									removeFile(mainLngFile);
									foundFilesToDelete = true;
									break;
								}
							}
						}

						string hintLngFile = data_path + "data/lang/hint/hint_" + language + ".lng";
						if(fileExists(hintLngFile) == true) {
							for(unsigned int k = 0; k < languageFileMappings.size(); ++k) {
								string mapping = languageFileMappings[k];
								replaceAll(mapping,"$language",language);

								if(EndsWith(hintLngFile,mapping) == true) {
									printf("About to delete file [%s]\n",hintLngFile.c_str());
									removeFile(hintLngFile);
									foundFilesToDelete = true;
									break;
								}
							}
						}
					}

					if(lang.isLanguageLocal(toLower(language)) == true) {
						lang.loadGameStrings(toLower(language));
					}

					if(foundFilesToDelete == true) {
						mainMessageBoxState = 0;
						showMessageBox(lang.getString("TransifexDeleteSuccess","",false,true), lang.getString("Notice","",false,true), true);
					}
				}
			}
		}
		else if(ctl == cegui_manager.getControl("TabControl/__auto_TabPane__/Misc/ButtonSave")) {

			CoreData &coreData				= CoreData::getInstance();
			SoundRenderer &soundRenderer	= SoundRenderer::getInstance();
			Config &config 					= Config::getInstance();

			soundRenderer.playFx(coreData.getClickSoundA());

			string currentFontSizeAdjustment  = config.getString("FontSizeAdjustment");
			string selectedFontSizeAdjustment = intToStr((int)cegui_manager.getSpinnerControlValue(cegui_manager.getControl("TabControl/__auto_TabPane__/Misc/SpinnerFontAdjustment")));

			if(currentFontSizeAdjustment != selectedFontSizeAdjustment) {

				mainMessageBoxState = 1;
				Lang &lang= Lang::getInstance();
				showMessageBox(lang.getString("RestartNeeded","",false,true), lang.getString("FontSizeAdjustmentChanged","",false,true), false);
				return true;
			}

			saveConfig();
			reloadUI();

			return true;
		}
		else if(ctl == cegui_manager.getControl("TabControl/__auto_TabPane__/Misc/ButtonReturn")) {

			DelayCallbackFunction pCB = &MenuStateOptions::delayedCallbackFunctionReturn;
			delayedCallbackList.push_back(pCB);

			return true;
		}
	}
	else if(name == cegui_manager.getEventComboboxChangeAccepted()) {
		if(ctl == cegui_manager.getControl("TabControl/__auto_TabPane__/Misc/ComboBoxLanguage")) {

			int selectedId = cegui_manager.getSelectedItemIdFromComboBoxControl(cegui_manager.getControl("TabControl/__auto_TabPane__/Misc/ComboBoxLanguage"));
			if(selectedId >= 0) {
				map<string,string>::iterator iterMap = languageList.begin();
				std::advance(iterMap,selectedId);
				string language = iterMap->first + "-" + iterMap->second;

				cegui_manager.setControlText("TabControl/__auto_TabPane__/Misc/ComboBoxLanguage",language);
			}
		}
	}
	else if(name == cegui_manager.getEventCheckboxClicked()) {

		if(ctl == cegui_manager.getControl("TabControl/__auto_TabPane__/Misc/CheckboxAdvancedTranslation")) {
			setupTransifexUI();
		}
		else if(ctl == cegui_manager.getControl("TabControl/__auto_TabPane__/Misc/CheckboxDisableLuaSandbox")) {

			if(cegui_manager.getCheckboxControlChecked(
					cegui_manager.getControl("TabControl/__auto_TabPane__/Misc/CheckboxDisableLuaSandbox")) == true) {

				cegui_manager.setCheckboxControlChecked(
							cegui_manager.getControl("TabControl/__auto_TabPane__/Misc/CheckboxDisableLuaSandbox"),false,true);

				luaMessageBoxState=1;
				Lang &lang= Lang::getInstance();
				showLuaMessageBox(lang.getString("LuaDisableSecuritySandboxWarning","",false,true), lang.getString("Question","",false,true), false);
			}
		}

	}
	return false;
}

void MenuStateOptions::setupTransifexUI() {

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	CEGUI::Window *ctl = cegui_manager.loadLayoutFromFile("OptionsMenuMisc.layout");
	CEGUI::Window *advancedTranslation = cegui_manager.getChildControl(ctl,"GroupBoxAdvancedTranslation");
	cegui_manager.setControlVisible(advancedTranslation,
			cegui_manager.getCheckboxControlChecked(cegui_manager.getChildControl(ctl,"CheckboxAdvancedTranslation")));
}

void MenuStateOptions::showMessageBox(const string &text, const string &header, bool okOnly){

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

void MenuStateOptions::showLuaMessageBox(const string &text, const string &header, bool okOnly) {

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	if(cegui_manager.isMessageBoxShowing("TabControl/__auto_TabPane__/Misc/LuaMsgBox") == false) {

		Lang &lang= Lang::getInstance();
		cegui_manager.displayMessageBox(header, text, lang.getString("Yes","",false,true),lang.getString("No","",false,true),"TabControl/__auto_TabPane__/Misc/LuaMsgBox");
	}
	else {
		cegui_manager.hideMessageBox("TabControl/__auto_TabPane__/Misc/LuaMsgBox");
	}
}

void MenuStateOptions::mouseClick(int x, int y, MouseButton mouseButton) { }

void MenuStateOptions::mouseMove(int x, int y, const MouseState *ms) { }

bool MenuStateOptions::isInSpecialKeyCaptureEvent() {
	return false;
}

void MenuStateOptions::keyDown(SDL_KeyboardEvent key) {}

void MenuStateOptions::keyPress(SDL_KeyboardEvent c) {}

void MenuStateOptions::render(){
	Renderer &renderer= Renderer::getInstance();

	renderer.renderConsole(&console,false,true);
	if(program != NULL) program->renderProgramMsgBox();
}

void MenuStateOptions::saveConfig() {

	Config &config	= Config::getInstance();
	Lang &lang		= Lang::getInstance();

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();

	string playerName = cegui_manager.getControlText(
			"TabControl/__auto_TabPane__/Misc/EditboxPlayerName");

	if(playerName.length() > 0)	{
		config.setString("NetPlayerName", playerName);
	}
	//Copy values
	map<string,string>::iterator iterMapFind = languageList.begin();

	string selectedLangName = "";
	string selectedLangText = cegui_manager.getControlText(cegui_manager.getControl("TabControl/__auto_TabPane__/Misc/ComboBoxLanguage"));
	for(map<string,string>::iterator iterMap = languageList.begin();
			iterMap != languageList.end(); ++iterMap) {

		string language = iterMap->first + "-" + iterMap->second;
		if(selectedLangText == language) {
			selectedLangName = iterMap->first;
			break;
		}
	}

	config.setString("Lang", selectedLangName);
	lang.loadGameStrings(config.getString("Lang"));

	string selectedFontSizeAdjustment = intToStr((int)cegui_manager.getSpinnerControlValue(cegui_manager.getControl("TabControl/__auto_TabPane__/Misc/SpinnerFontAdjustment")));
	config.setString("FontSizeAdjustment", selectedFontSizeAdjustment);

    config.setString("ScreenShotFileType", cegui_manager.getSelectedItemFromComboBoxControl(
			cegui_manager.getControl("TabControl/__auto_TabPane__/Misc/ComboBoxScreenshotFormat")));

    config.setBool("DisableScreenshotConsoleText", cegui_manager.getCheckboxControlChecked(
			cegui_manager.getControl("TabControl/__auto_TabPane__/Misc/CheckboxShowScreenshotSaved")) == false);

    config.setBool("MouseMoveScrollsWorld", cegui_manager.getCheckboxControlChecked(
			cegui_manager.getControl("TabControl/__auto_TabPane__/Misc/CheckboxMouseMovesCamera")));

    string selectedCameraMoveSpeed = intToStr((int)cegui_manager.getSpinnerControlValue(cegui_manager.getControl("TabControl/__auto_TabPane__/Misc/SpinnerCameraMoveSpeed")));
	config.setString("CameraMoveSpeed", selectedCameraMoveSpeed);

    config.setBool("VisibleHud", cegui_manager.getCheckboxControlChecked(
			cegui_manager.getControl("TabControl/__auto_TabPane__/Misc/CheckboxVisibleHUD")));

    config.setBool("ChatStaysActive", cegui_manager.getCheckboxControlChecked(
			cegui_manager.getControl("TabControl/__auto_TabPane__/Misc/CheckboxChatRemainsActive")));
    config.setBool("TimeDisplay", cegui_manager.getCheckboxControlChecked(
			cegui_manager.getControl("TabControl/__auto_TabPane__/Misc/CheckboxDisplayRealAndGameTime")));

	config.setBool("DisableLuaSandbox", cegui_manager.getCheckboxControlChecked(
			cegui_manager.getControl("TabControl/__auto_TabPane__/Misc/CheckboxDisableLuaSandbox")));
	config.save();

	if(config.getBool("DisableLuaSandbox","false") == true) {
		LuaScript::setDisableSandbox(true);
	}

	Renderer::getInstance().loadConfig();
	console.addLine(lang.getString("SettingsSaved"));
}

}}//end namespace
