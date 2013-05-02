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
#include "leak_dumper.h"

using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class MenuStateOptions
// =====================================================
MenuStateOptions::MenuStateOptions(Program *program, MainMenu *mainMenu, ProgramState **parentUI):
	MenuState(program, mainMenu, "config")
{
	try {
		containerName = "Options";
		this->parentUI=parentUI;
		Lang &lang= Lang::getInstance();
		Config &config= Config::getInstance();
		this->console.setOnlyChatMessagesInStoredLines(false);
		//modeinfos=list<ModeInfo> ();
		activeInputLabel=NULL;

		int leftLabelStart=50;
		int leftColumnStart=leftLabelStart+280;
		int rightLabelStart=450;
		int rightColumnStart=rightLabelStart+280;
		int buttonRowPos=50;
		int buttonStartPos=170;
		int captionOffset=75;
		int currentLabelStart=leftLabelStart;
		int currentColumnStart=leftColumnStart;
		int currentLine=700;
		int lineOffset=30;
		int tabButtonWidth=200;
		int tabButtonHeight=30;

		mainMessageBox.registerGraphicComponent(containerName,"mainMessageBox");
		mainMessageBox.init(lang.get("Ok"));
		mainMessageBox.setEnabled(false);
		mainMessageBoxState=0;

		buttonAudioSection.registerGraphicComponent(containerName,"buttonAudioSection");
		buttonAudioSection.init(0, 720,tabButtonWidth,tabButtonHeight);
		buttonAudioSection.setFont(CoreData::getInstance().getMenuFontVeryBig());
		buttonAudioSection.setFont3D(CoreData::getInstance().getMenuFontVeryBig3D());
		buttonAudioSection.setText(lang.get("Audio"));
		// Video Section
		buttonVideoSection.registerGraphicComponent(containerName,"labelVideoSection");
		buttonVideoSection.init(200, 720,tabButtonWidth,tabButtonHeight);
		buttonVideoSection.setFont(CoreData::getInstance().getMenuFontVeryBig());
		buttonVideoSection.setFont3D(CoreData::getInstance().getMenuFontVeryBig3D());
		buttonVideoSection.setText(lang.get("Video"));
		//currentLine-=lineOffset;
		//MiscSection
		buttonMiscSection.registerGraphicComponent(containerName,"labelMiscSection");
		buttonMiscSection.init(400, 700,tabButtonWidth,tabButtonHeight+20);
		buttonMiscSection.setFont(CoreData::getInstance().getMenuFontVeryBig());
		buttonMiscSection.setFont3D(CoreData::getInstance().getMenuFontVeryBig3D());
		buttonMiscSection.setText(lang.get("Misc"));
		//NetworkSettings
		buttonNetworkSettings.registerGraphicComponent(containerName,"labelNetworkSettingsSection");
		buttonNetworkSettings.init(600, 720,tabButtonWidth,tabButtonHeight);
		buttonNetworkSettings.setFont(CoreData::getInstance().getMenuFontVeryBig());
		buttonNetworkSettings.setFont3D(CoreData::getInstance().getMenuFontVeryBig3D());
		buttonNetworkSettings.setText(lang.get("Network"));

		//KeyboardSetup
		buttonKeyboardSetup.registerGraphicComponent(containerName,"buttonKeyboardSetup");
		buttonKeyboardSetup.init(800, 720,tabButtonWidth,tabButtonHeight);
		buttonKeyboardSetup.setFont(CoreData::getInstance().getMenuFontVeryBig());
		buttonKeyboardSetup.setFont3D(CoreData::getInstance().getMenuFontVeryBig3D());
		buttonKeyboardSetup.setText(lang.get("Keyboardsetup"));

		currentLine=650; // reset line pos
		currentLabelStart=leftLabelStart; // set to right side
		currentColumnStart=leftColumnStart; // set to right side

		//lang
		labelLang.registerGraphicComponent(containerName,"labelLang");
		labelLang.init(currentLabelStart, currentLine);
		labelLang.setText(lang.get("Language"));

		listBoxLang.registerGraphicComponent(containerName,"listBoxLang");
		listBoxLang.init(currentColumnStart, currentLine, 320);
		vector<string> langResults;

	//    string data_path = getGameReadWritePath(GameConstants::path_data_CacheLookupKey);
	//
	//    string userDataPath = getGameCustomCoreDataPath(data_path, "");
	//	findAll(userDataPath + "data/lang/*.lng", langResults, true, false);
	//
	//	vector<string> langResults2;
	//	findAll(data_path + "data/lang/*.lng", langResults2, true);
	//	if(langResults2.empty() && langResults.empty()) {
	//        throw megaglest_runtime_error("There are no lang files");
	//	}
	//	for(unsigned int i = 0; i < langResults2.size(); ++i) {
	//		string testLanguage = langResults2[i];
	//		if(std::find(langResults.begin(),langResults.end(),testLanguage) == langResults.end()) {
	//			langResults.push_back(testLanguage);
	//		}
	//	}
		languageList = Lang::getInstance().getDiscoveredLanguageList(true);
		for(map<string,string>::iterator iterMap = languageList.begin();
			iterMap != languageList.end(); ++iterMap) {
			langResults.push_back(iterMap->first + "-" + iterMap->second);
		}

		listBoxLang.setItems(langResults);

		pair<string,string> defaultLang = Lang::getInstance().getNavtiveNameFromLanguageName(config.getString("Lang"));
		if(defaultLang.first == "" && defaultLang.second == "") {
			defaultLang = Lang::getInstance().getNavtiveNameFromLanguageName(Lang::getInstance().getDefaultLanguage());
		}
		listBoxLang.setSelectedItem(defaultLang.second + "-" + defaultLang.first);
		currentLine-=lineOffset;

		//playerName
		labelPlayerNameLabel.registerGraphicComponent(containerName,"labelPlayerNameLabel");
		labelPlayerNameLabel.init(currentLabelStart,currentLine);
		labelPlayerNameLabel.setText(lang.get("Playername"));

		labelPlayerName.registerGraphicComponent(containerName,"labelPlayerName");
		labelPlayerName.init(currentColumnStart,currentLine);
		labelPlayerName.setText(config.getString("NetPlayerName",Socket::getHostName().c_str()));
		labelPlayerName.setFont(CoreData::getInstance().getMenuFontBig());
		labelPlayerName.setFont3D(CoreData::getInstance().getMenuFontBig3D());
		labelPlayerName.setMaxEditWidth(16);
		labelPlayerName.setMaxEditRenderWidth(200);
		currentLine-=lineOffset;

		//FontSizeAdjustment
		labelFontSizeAdjustment.registerGraphicComponent(containerName,"labelFontSizeAdjustment");
		labelFontSizeAdjustment.init(currentLabelStart,currentLine);
		labelFontSizeAdjustment.setText(lang.get("FontSizeAdjustment"));

		listFontSizeAdjustment.registerGraphicComponent(containerName,"listFontSizeAdjustment");
		listFontSizeAdjustment.init(currentColumnStart, currentLine, 80);
		for(int i=-5; i<=5; i+=1){
			listFontSizeAdjustment.pushBackItem(intToStr(i));
		}
		listFontSizeAdjustment.setSelectedItem(intToStr(config.getInt("FontSizeAdjustment")));

		currentLine-=lineOffset;
		// Screenshot type flag
		labelScreenShotType.registerGraphicComponent(containerName,"labelScreenShotType");
		labelScreenShotType.init(currentLabelStart ,currentLine);
		labelScreenShotType.setText(lang.get("ScreenShotFileType"));

		listBoxScreenShotType.registerGraphicComponent(containerName,"listBoxScreenShotType");
		listBoxScreenShotType.init(currentColumnStart ,currentLine, 80 );
		listBoxScreenShotType.pushBackItem("bmp");
		listBoxScreenShotType.pushBackItem("jpg");
		listBoxScreenShotType.pushBackItem("png");
		listBoxScreenShotType.pushBackItem("tga");
		listBoxScreenShotType.setSelectedItem(config.getString("ScreenShotFileType","jpg"));

		currentLine-=lineOffset;

		labelDisableScreenshotConsoleText.registerGraphicComponent(containerName,"lavelDisableScreenshotConsoleText");
		labelDisableScreenshotConsoleText.init(currentLabelStart ,currentLine);
		labelDisableScreenshotConsoleText.setText(lang.get("ScreenShotConsoleText"));

		checkBoxDisableScreenshotConsoleText.registerGraphicComponent(containerName,"checkBoxDisableScreenshotConsoleText");
		checkBoxDisableScreenshotConsoleText.init(currentColumnStart ,currentLine );
		checkBoxDisableScreenshotConsoleText.setValue(!config.getBool("DisableScreenshotConsoleText","false"));

		currentLine-=lineOffset;

		labelMouseMoveScrollsWorld.registerGraphicComponent(containerName,"labelMouseMoveScrollsWorld");
		labelMouseMoveScrollsWorld.init(currentLabelStart ,currentLine);
		labelMouseMoveScrollsWorld.setText(lang.get("MouseScrollsWorld"));

		checkBoxMouseMoveScrollsWorld.registerGraphicComponent(containerName,"checkBoxMouseMoveScrollsWorld");
		checkBoxMouseMoveScrollsWorld.init(currentColumnStart ,currentLine );
		checkBoxMouseMoveScrollsWorld.setValue(config.getBool("MouseMoveScrollsWorld","true"));
		currentLine-=lineOffset;

		labelVisibleHud.registerGraphicComponent(containerName,"lavelVisibleHud");
		labelVisibleHud.init(currentLabelStart ,currentLine);
		labelVisibleHud.setText(lang.get("VisibleHUD"));

		checkBoxVisibleHud.registerGraphicComponent(containerName,"checkBoxVisibleHud");
		checkBoxVisibleHud.init(currentColumnStart ,currentLine );
		checkBoxVisibleHud.setValue(config.getBool("VisibleHud","true"));

		currentLine-=lineOffset;

		labelChatStaysActive.registerGraphicComponent(containerName,"labelChatStaysActive");
		labelChatStaysActive.init(currentLabelStart ,currentLine);
		labelChatStaysActive.setText(lang.get("ChatStaysActive"));

		checkBoxChatStaysActive.registerGraphicComponent(containerName,"checkBoxChatStaysActive");
		checkBoxChatStaysActive.init(currentColumnStart ,currentLine );
		checkBoxChatStaysActive.setValue(config.getBool("ChatStaysActive","false"));

		currentLine-=lineOffset;

		labelTimeDisplay.registerGraphicComponent(containerName,"labelTimeDisplay");
		labelTimeDisplay.init(currentLabelStart ,currentLine);
		labelTimeDisplay.setText(lang.get("TimeDisplay"));

		checkBoxTimeDisplay.registerGraphicComponent(containerName,"checkBoxTimeDisplay");
		checkBoxTimeDisplay.init(currentColumnStart ,currentLine );
		checkBoxTimeDisplay.setValue(config.getBool("TimeDisplay","true"));

		currentLine-=lineOffset;

		labelLuaDisableSecuritySandbox.registerGraphicComponent(containerName,"labelLuaDisableSecuritySandbox");
		labelLuaDisableSecuritySandbox.init(currentLabelStart ,currentLine);
		labelLuaDisableSecuritySandbox.setText(lang.get("LuaDisableSecuritySandbox"));

		checkBoxLuaDisableSecuritySandbox.registerGraphicComponent(containerName,"checkBoxLuaDisableSecuritySandbox");
		checkBoxLuaDisableSecuritySandbox.init(currentColumnStart ,currentLine );
		checkBoxLuaDisableSecuritySandbox.setValue(config.getBool("DisableLuaSandbox","false"));

		luaMessageBox.registerGraphicComponent(containerName,"luaMessageBox");
		luaMessageBox.init(lang.get("Yes"),lang.get("No"));
		luaMessageBox.setEnabled(false);
		luaMessageBoxState=0;

		currentLine-=lineOffset;

		currentLine-=lineOffset/2;

		// buttons
		buttonOk.registerGraphicComponent(containerName,"buttonOk");
		buttonOk.init(buttonStartPos, buttonRowPos, 100);
		buttonOk.setText(lang.get("Save"));

		buttonReturn.registerGraphicComponent(containerName,"buttonAbort");
		buttonReturn.init(buttonStartPos+110, buttonRowPos, 100);
		buttonReturn.setText(lang.get("Return"));

		// Transifex related UI
		currentLine-=lineOffset*4;
		labelCustomTranslation.registerGraphicComponent(containerName,"labelCustomTranslation");
		labelCustomTranslation.init(currentLabelStart ,currentLine);
		labelCustomTranslation.setText(lang.get("CustomTranslation"));

		checkBoxCustomTranslation.registerGraphicComponent(containerName,"checkBoxCustomTranslation");
		checkBoxCustomTranslation.init(currentColumnStart ,currentLine );
		checkBoxCustomTranslation.setValue(false);
		currentLine-=lineOffset;

		labelTransifexUserLabel.registerGraphicComponent(containerName,"labelTransifexUserLabel");
		labelTransifexUserLabel.init(currentLabelStart + 20,currentLine);
		labelTransifexUserLabel.setText(lang.get("TransifexUserName"));

		labelTransifexPwdLabel.registerGraphicComponent(containerName,"labelTransifexPwdLabel");
		labelTransifexPwdLabel.init(currentLabelStart + 220 ,currentLine);
		labelTransifexPwdLabel.setText(lang.get("TransifexPwd"));

		labelTransifexI18NLabel.registerGraphicComponent(containerName,"labelTransifexI18NLabel");
		labelTransifexI18NLabel.init(currentLabelStart + 380 ,currentLine);
		labelTransifexI18NLabel.setText(lang.get("TransifexI18N"));

		currentLine-=lineOffset;

		labelTransifexUser.registerGraphicComponent(containerName,"labelTransifexUser");
		labelTransifexUser.init(currentLabelStart + 20,currentLine);
		labelTransifexUser.setMaxEditWidth(60);
		labelTransifexUser.setMaxEditRenderWidth(120);
		labelTransifexUser.setText(config.getString("TranslationGetURLUser","<none>"));

		labelTransifexPwd.registerGraphicComponent(containerName,"labelTransifexPwd");
		labelTransifexPwd.init(currentLabelStart + 220 ,currentLine);
		labelTransifexPwd.setIsPassword(true);
		labelTransifexPwd.setMaxEditWidth(60);
		labelTransifexPwd.setMaxEditRenderWidth(120);
		labelTransifexPwd.setText(config.getString("TranslationGetURLPassword",""));

		labelTransifexI18N.registerGraphicComponent(containerName,"labelTransifexI18N");
		labelTransifexI18N.init(currentLabelStart + 380 ,currentLine);
		labelTransifexI18N.setMaxEditWidth(3);
		labelTransifexI18N.setText(config.getString("TranslationGetURLLanguage","en"));
		currentLine-=lineOffset;

		buttonGetNewLanguageFiles.registerGraphicComponent(containerName,"buttonGetNewLanguageFiles");
		buttonGetNewLanguageFiles.init(currentLabelStart+20, currentLine, 200);
		buttonGetNewLanguageFiles.setText(lang.get("TransifexGetLanguageFiles"));

		buttonDeleteNewLanguageFiles.registerGraphicComponent(containerName,"buttonDeleteNewLanguageFiles");
		buttonDeleteNewLanguageFiles.init(currentLabelStart + 250, currentLine, 200);
		buttonDeleteNewLanguageFiles.setText(lang.get("TransifexDeleteLanguageFiles"));

		setupTransifexUI();

		GraphicComponent::applyAllCustomProperties(containerName);
	}
	catch(exception &e) {
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error loading options: %s\n",__FILE__,__FUNCTION__,__LINE__,e.what());
		throw megaglest_runtime_error(string("Error loading options msg: ") + e.what());
	}
}

void MenuStateOptions::reloadUI() {
	Lang &lang= Lang::getInstance();

	console.resetFonts();
	GraphicComponent::reloadFontsForRegisterGraphicComponents(containerName);
	mainMessageBox.init(lang.get("Ok"));
	luaMessageBox.init(lang.get("Yes"),lang.get("No"));

	buttonAudioSection.setFont(CoreData::getInstance().getMenuFontVeryBig());
	buttonAudioSection.setFont3D(CoreData::getInstance().getMenuFontVeryBig3D());
	buttonAudioSection.setText(lang.get("Audio"));

	buttonVideoSection.setFont(CoreData::getInstance().getMenuFontVeryBig());
	buttonVideoSection.setFont3D(CoreData::getInstance().getMenuFontVeryBig3D());
	buttonVideoSection.setText(lang.get("Video"));

	buttonMiscSection.setFont(CoreData::getInstance().getMenuFontVeryBig());
	buttonMiscSection.setFont3D(CoreData::getInstance().getMenuFontVeryBig3D());
	buttonMiscSection.setText(lang.get("Misc"));

	buttonNetworkSettings.setFont(CoreData::getInstance().getMenuFontVeryBig());
	buttonNetworkSettings.setFont3D(CoreData::getInstance().getMenuFontVeryBig3D());
	buttonNetworkSettings.setText(lang.get("Network"));

	buttonKeyboardSetup.setFont(CoreData::getInstance().getMenuFontVeryBig());
	buttonKeyboardSetup.setFont3D(CoreData::getInstance().getMenuFontVeryBig3D());
	buttonKeyboardSetup.setText(lang.get("Keyboardsetup"));

	labelVisibleHud.setText(lang.get("VisibleHUD"));
	labelChatStaysActive.setText(lang.get("ChatStaysActive"));
	labelTimeDisplay.setText(lang.get("TimeDisplay"));

	labelLuaDisableSecuritySandbox.setText(lang.get("LuaDisableSecuritySandbox"));

	labelLang.setText(lang.get("Language"));

	labelPlayerNameLabel.setText(lang.get("Playername"));

	labelPlayerName.setFont(CoreData::getInstance().getMenuFontBig());
	labelPlayerName.setFont3D(CoreData::getInstance().getMenuFontBig3D());

	labelFontSizeAdjustment.setText(lang.get("FontSizeAdjustment"));

	labelScreenShotType.setText(lang.get("ScreenShotFileType"));

	labelDisableScreenshotConsoleText.setText(lang.get("ScreenShotConsoleText"));

	labelMouseMoveScrollsWorld.setText(lang.get("MouseScrollsWorld"));


	buttonOk.setText(lang.get("Save"));
	buttonReturn.setText(lang.get("Return"));

	labelCustomTranslation.setText(lang.get("CustomTranslation"));
	buttonGetNewLanguageFiles.setText(lang.get("TransifexGetLanguageFiles"));
	buttonDeleteNewLanguageFiles.setText(lang.get("TransifexDeleteLanguageFiles"));
	labelTransifexUserLabel.setText(lang.get("TransifexUserName"));
	labelTransifexPwdLabel.setText(lang.get("TransifexPwd"));
	labelTransifexI18NLabel.setText(lang.get("TransifexI18N"));
}

void MenuStateOptions::setupTransifexUI() {
	buttonGetNewLanguageFiles.setEnabled(checkBoxCustomTranslation.getValue());
	buttonDeleteNewLanguageFiles.setEnabled(checkBoxCustomTranslation.getValue());
	labelTransifexUserLabel.setEnabled(checkBoxCustomTranslation.getValue());
	labelTransifexUser.setEnabled(checkBoxCustomTranslation.getValue());
	labelTransifexPwdLabel.setEnabled(checkBoxCustomTranslation.getValue());
	labelTransifexPwd.setEnabled(checkBoxCustomTranslation.getValue());
	labelTransifexI18NLabel.setEnabled(checkBoxCustomTranslation.getValue());
	labelTransifexI18N.setEnabled(checkBoxCustomTranslation.getValue());
}

void MenuStateOptions::showMessageBox(const string &text, const string &header, bool toggle){
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

void MenuStateOptions::showLuaMessageBox(const string &text, const string &header, bool toggle) {
	if(!toggle) {
		luaMessageBox.setEnabled(false);
	}

	if(!luaMessageBox.getEnabled()){
		luaMessageBox.setText(text);
		luaMessageBox.setHeader(header);
		luaMessageBox.setEnabled(true);
	}
	else{
		luaMessageBox.setEnabled(false);
	}
}

void MenuStateOptions::mouseClick(int x, int y, MouseButton mouseButton){

	Config &config= Config::getInstance();
	CoreData &coreData= CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();

	if(mainMessageBox.getEnabled()) {
		int button= 0;
		if(mainMessageBox.mouseClick(x, y, button)) {
			soundRenderer.playFx(coreData.getClickSoundA());
			if(button == 0) {
				if(mainMessageBoxState == 1) {
					mainMessageBoxState=0;
					mainMessageBox.setEnabled(false);
					saveConfig();

					Lang &lang= Lang::getInstance();
					mainMessageBox.init(lang.get("Ok"));
					mainMenu->setState(new MenuStateRoot(program, mainMenu));
				}
				else {
					mainMessageBox.setEnabled(false);

					Lang &lang= Lang::getInstance();
					mainMessageBox.init(lang.get("Ok"));
				}
			}
			else {
				if(mainMessageBoxState == 1) {
					mainMessageBoxState=0;
					mainMessageBox.setEnabled(false);

					Lang &lang= Lang::getInstance();
					mainMessageBox.init(lang.get("Ok"));
				}
			}
		}
	}
	else if(luaMessageBox.getEnabled()){
		int button= 0;
		if(luaMessageBox.mouseClick(x, y, button)) {
			checkBoxLuaDisableSecuritySandbox.setValue(false);
			soundRenderer.playFx(coreData.getClickSoundA());
			if(button == 0) {
				if(luaMessageBoxState == 1) {
					checkBoxLuaDisableSecuritySandbox.setValue(true);
				}
			}
			luaMessageBox.setEnabled(false);
		}
	}
	else if(checkBoxLuaDisableSecuritySandbox.mouseClick(x, y)) {
		if(checkBoxLuaDisableSecuritySandbox.getValue() == true) {
			checkBoxLuaDisableSecuritySandbox.setValue(false);

			luaMessageBoxState=1;
			Lang &lang= Lang::getInstance();
			showLuaMessageBox(lang.get("LuaDisableSecuritySandboxWarning"), lang.get("Question"), false);
		}
	}
	else if(buttonOk.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundA());

		string currentFontSizeAdjustment=config.getString("FontSizeAdjustment");
		string selectedFontSizeAdjustment=listFontSizeAdjustment.getSelectedItem();
		if(currentFontSizeAdjustment != selectedFontSizeAdjustment){
			mainMessageBoxState=1;
			Lang &lang= Lang::getInstance();
			showMessageBox(lang.get("RestartNeeded"), lang.get("FontSizeAdjustmentChanged"), false);
			return;
		}

		saveConfig();
		//mainMenu->setState(new MenuStateRoot(program, mainMenu));
		reloadUI();
		return;
    }
	else if(buttonReturn.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundA());
		if(this->parentUI != NULL) {
			*this->parentUI = NULL;
			delete *this->parentUI;
		}
		mainMenu->setState(new MenuStateRoot(program, mainMenu));
		return;
    }
	else if(buttonKeyboardSetup.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundA());
		mainMenu->setState(new MenuStateKeysetup(program, mainMenu,this->parentUI)); // open keyboard shortcuts setup screen
		//showMessageBox("Not implemented yet", "Keyboard setup", false);
		return;
	}
	else if(buttonAudioSection.mouseClick(x, y)){ 
			soundRenderer.playFx(coreData.getClickSoundA());
			mainMenu->setState(new MenuStateOptionsSound(program, mainMenu,this->parentUI)); // open keyboard shortcuts setup screen
			return;
		}
	else if(buttonNetworkSettings.mouseClick(x, y)){ 
			soundRenderer.playFx(coreData.getClickSoundA());
			mainMenu->setState(new MenuStateOptionsNetwork(program, mainMenu,this->parentUI)); // open keyboard shortcuts setup screen
			return;
		}
	else if(buttonMiscSection.mouseClick(x, y)){ 
			soundRenderer.playFx(coreData.getClickSoundA());
			//mainMenu->setState(new MenuStateOptions(program, mainMenu,this->parentUI)); // open keyboard shortcuts setup screen
			return;
		}
	else if(buttonVideoSection.mouseClick(x, y)){ 
			soundRenderer.playFx(coreData.getClickSoundA());
			mainMenu->setState(new MenuStateOptionsGraphics(program, mainMenu,this->parentUI)); // open keyboard shortcuts setup screen
			return;
		}
	else if(checkBoxCustomTranslation.mouseClick(x, y)) {
		setupTransifexUI();
	}
	else if(buttonDeleteNewLanguageFiles.mouseClick(x, y)) {
		soundRenderer.playFx(coreData.getClickSoundA());

		setActiveInputLable(NULL);

		if(labelTransifexI18N.getText() != "") {
			Lang &lang= Lang::getInstance();
			string language = lang.getLanguageFile(labelTransifexI18N.getText());

			if(language != "") {
				bool foundFilesToDelete = false;

				Config &config = Config::getInstance();
				string data_path = config.getString("UserData_Root","");
				if(data_path != "") {
					endPathWithSlash(data_path);
				}

				if(data_path != "") {

					string txnURLFileListMapping = Config::getInstance().getString("TranslationGetURLFileListMapping");
					vector<string> languageFileMappings;
					Tokenize(txnURLFileListMapping,languageFileMappings,"|");

					Config &config = Config::getInstance();

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
					lang.loadStrings(toLower(language));
				}

				if(foundFilesToDelete == true) {
					mainMessageBoxState=0;
					Lang &lang= Lang::getInstance();
					showMessageBox(lang.get("TransifexDeleteSuccess"), lang.get("Notice"), false);
				}
			}
		}
	}
	else if(buttonGetNewLanguageFiles.mouseClick(x, y)) {
		soundRenderer.playFx(coreData.getClickSoundA());

		setActiveInputLable(NULL);

		string orig_txnURLUser = Config::getInstance().getString("TranslationGetURLUser");
		//string orig_txnURLPwd = Config::getInstance().getString("TranslationGetURLPassword","");
		string orig_txnURLLang = Config::getInstance().getString("TranslationGetURLLanguage");

		Config::getInstance().setString("TranslationGetURLUser",labelTransifexUser.getText());
		Config::getInstance().setString("TranslationGetURLPassword",labelTransifexPwd.getText(),true);
		Config::getInstance().setString("TranslationGetURLLanguage",labelTransifexI18N.getText());

		bool saveChanges = (orig_txnURLUser != labelTransifexUser.getText() ||
				orig_txnURLLang != labelTransifexI18N.getText());

		string txnURL = Config::getInstance().getString("TranslationGetURL");
		string txnURLUser = Config::getInstance().getString("TranslationGetURLUser");
		string txnURLPwd = Config::getInstance().getString("TranslationGetURLPassword");
		string txnURLLang = Config::getInstance().getString("TranslationGetURLLanguage");
		string txnURLFileList = Config::getInstance().getString("TranslationGetURLFileList");
		string txnURLFileListMapping = Config::getInstance().getString("TranslationGetURLFileListMapping");

		string txnURLDetails = Config::getInstance().getString("TranslationGetURLDetails");

		string credentials = txnURLUser + ":" + txnURLPwd;

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

			bool gotDownloads = false;
			bool reloadLanguage = false;
			string langName = "";

			CURL *handle = SystemFlags::initHTTP();
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
					lang.loadStrings(toLower(langName));
				}
			}

			if(gotDownloads == true) {
				mainMessageBoxState=0;
				Lang &lang= Lang::getInstance();
				showMessageBox(lang.get("TransifexDownloadSuccess") + "\n" + langName, lang.get("Notice"), false);
			}
		}
		return;
	}
	else if(labelPlayerName.mouseClick(x, y) && ( activeInputLabel != &labelPlayerName )){
			setActiveInputLable(&labelPlayerName);
	}
	else if(labelTransifexUser.mouseClick(x, y) && ( activeInputLabel != &labelTransifexUser )){
			setActiveInputLable(&labelTransifexUser);
	}
	else if(labelTransifexPwd.mouseClick(x, y) && ( activeInputLabel != &labelTransifexPwd )){
			setActiveInputLable(&labelTransifexPwd);
	}
	else if(labelTransifexI18N.mouseClick(x, y) && ( activeInputLabel != &labelTransifexI18N )){
			setActiveInputLable(&labelTransifexI18N);
	}
	else
	{
		listBoxLang.mouseClick(x, y);
		listFontSizeAdjustment.mouseClick(x, y);

        listBoxScreenShotType.mouseClick(x, y);

        checkBoxDisableScreenshotConsoleText.mouseClick(x, y);
        checkBoxMouseMoveScrollsWorld.mouseClick(x, y);
        checkBoxVisibleHud.mouseClick(x, y);
        checkBoxChatStaysActive.mouseClick(x, y);
        checkBoxTimeDisplay.mouseClick(x, y);
		checkBoxLuaDisableSecuritySandbox.mouseClick(x, y);
	}
}

void MenuStateOptions::mouseMove(int x, int y, const MouseState *ms){
	if (mainMessageBox.getEnabled()) {
		mainMessageBox.mouseMove(x, y);
	}
	if (luaMessageBox.getEnabled()) {
		luaMessageBox.mouseMove(x, y);
	}

	buttonOk.mouseMove(x, y);
	buttonReturn.mouseMove(x, y);
	buttonKeyboardSetup.mouseMove(x, y);
	buttonAudioSection.mouseMove(x, y);
	buttonNetworkSettings.mouseMove(x, y);
	buttonMiscSection.mouseMove(x, y);
	buttonVideoSection.mouseMove(x, y);
	buttonGetNewLanguageFiles.mouseMove(x, y);
	buttonDeleteNewLanguageFiles.mouseMove(x, y);
	listBoxLang.mouseMove(x, y);
	listBoxLang.mouseMove(x, y);
	listFontSizeAdjustment.mouseMove(x, y);
	listBoxScreenShotType.mouseMove(x, y);
	checkBoxDisableScreenshotConsoleText.mouseMove(x, y);
	checkBoxMouseMoveScrollsWorld.mouseMove(x, y);
	checkBoxVisibleHud.mouseMove(x, y);
    checkBoxChatStaysActive.mouseMove(x, y);
    checkBoxTimeDisplay.mouseMove(x, y);
	checkBoxLuaDisableSecuritySandbox.mouseMove(x, y);
	checkBoxCustomTranslation.mouseMove(x, y);
}

bool MenuStateOptions::isInSpecialKeyCaptureEvent() {
	return (activeInputLabel != NULL);
}

void MenuStateOptions::keyDown(SDL_KeyboardEvent key) {
	if(activeInputLabel != NULL) {
		keyDownEditLabel(key, &activeInputLabel);
	}
}

void MenuStateOptions::keyPress(SDL_KeyboardEvent c) {
	if(activeInputLabel != NULL) {
	    //printf("[%d]\n",c); fflush(stdout);
		if( &labelPlayerName 	== activeInputLabel ||
			&labelTransifexUser == activeInputLabel ||
			&labelTransifexPwd == activeInputLabel ||
			&labelTransifexI18N == activeInputLabel) {
			keyPressEditLabel(c, &activeInputLabel);
		}
	}
	else {
		Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));
		if(isKeyPressed(configKeys.getSDLKey("SaveGUILayout"),c) == true) {
			GraphicComponent::saveAllCustomProperties(containerName);
			//Lang &lang= Lang::getInstance();
			//console.addLine(lang.get("GUILayoutSaved") + " [" + (saved ? lang.get("Yes") : lang.get("No"))+ "]");
		}
	}
}

void MenuStateOptions::render(){
	Renderer &renderer= Renderer::getInstance();

	if(mainMessageBox.getEnabled()){
		renderer.renderMessageBox(&mainMessageBox);
	}
	else if(luaMessageBox.getEnabled()){
		renderer.renderMessageBox(&luaMessageBox);
	}
	else
	{
		renderer.renderButton(&buttonOk);
		renderer.renderButton(&buttonReturn);
		renderer.renderButton(&buttonKeyboardSetup);
		renderer.renderButton(&buttonVideoSection);
		renderer.renderButton(&buttonAudioSection);
		renderer.renderButton(&buttonMiscSection);
		renderer.renderButton(&buttonNetworkSettings);

		renderer.renderLabel(&labelCustomTranslation);
		renderer.renderCheckBox(&checkBoxCustomTranslation);

		if(buttonGetNewLanguageFiles.getEnabled()) renderer.renderButton(&buttonGetNewLanguageFiles);
		if(buttonDeleteNewLanguageFiles.getEnabled()) renderer.renderButton(&buttonDeleteNewLanguageFiles);
		if(labelTransifexUserLabel.getEnabled()) renderer.renderLabel(&labelTransifexUserLabel);
		if(labelTransifexPwdLabel.getEnabled()) renderer.renderLabel(&labelTransifexPwdLabel);
		if(labelTransifexI18NLabel.getEnabled()) renderer.renderLabel(&labelTransifexI18NLabel);
		if(labelTransifexUser.getEnabled()) renderer.renderLabel(&labelTransifexUser);
		if(labelTransifexPwd.getEnabled()) renderer.renderLabel(&labelTransifexPwd);
		if(labelTransifexI18N.getEnabled()) renderer.renderLabel(&labelTransifexI18N);

		renderer.renderListBox(&listBoxLang);
		renderer.renderLabel(&labelLang);
		renderer.renderLabel(&labelPlayerNameLabel);
		renderer.renderLabel(&labelPlayerName);
		renderer.renderListBox(&listFontSizeAdjustment);
		renderer.renderLabel(&labelFontSizeAdjustment);

        renderer.renderLabel(&labelScreenShotType);
        renderer.renderListBox(&listBoxScreenShotType);

        renderer.renderLabel(&labelDisableScreenshotConsoleText);
        renderer.renderCheckBox(&checkBoxDisableScreenshotConsoleText);

        renderer.renderLabel(&labelMouseMoveScrollsWorld);
        renderer.renderCheckBox(&checkBoxMouseMoveScrollsWorld);

        renderer.renderLabel(&labelVisibleHud);
        renderer.renderLabel(&labelChatStaysActive);
        renderer.renderLabel(&labelTimeDisplay);

        renderer.renderLabel(&labelLuaDisableSecuritySandbox);
        renderer.renderCheckBox(&checkBoxLuaDisableSecuritySandbox);

        renderer.renderCheckBox(&checkBoxVisibleHud);
        renderer.renderCheckBox(&checkBoxChatStaysActive);
        renderer.renderCheckBox(&checkBoxTimeDisplay);

	}

	renderer.renderConsole(&console,false,true);
	if(program != NULL) program->renderProgramMsgBox();
}

void MenuStateOptions::saveConfig(){
	Config &config= Config::getInstance();
	Lang &lang= Lang::getInstance();
	setActiveInputLable(NULL);

	if(labelPlayerName.getText().length()>0)
	{
		config.setString("NetPlayerName", labelPlayerName.getText());
	}
	//Copy values
	map<string,string>::iterator iterMap = languageList.begin();
	std::advance(iterMap, listBoxLang.getSelectedItemIndex());

	config.setString("Lang", iterMap->first);
	lang.loadStrings(config.getString("Lang"));

	config.setString("FontSizeAdjustment", listFontSizeAdjustment.getSelectedItem());
    config.setString("ScreenShotFileType", listBoxScreenShotType.getSelectedItem());

    config.setBool("DisableScreenshotConsoleText", !checkBoxDisableScreenshotConsoleText.getValue());
    config.setBool("MouseMoveScrollsWorld", checkBoxMouseMoveScrollsWorld.getValue());
    config.setBool("VisibleHud", checkBoxVisibleHud.getValue());
    config.setBool("ChatStaysActive", checkBoxChatStaysActive.getValue());
    config.setBool("TimeDisplay", checkBoxTimeDisplay.getValue());

	config.setBool("DisableLuaSandbox", checkBoxLuaDisableSecuritySandbox.getValue());
	config.save();

	if(config.getBool("DisableLuaSandbox","false") == true) {
		LuaScript::setDisableSandbox(true);
	}
	Renderer::getInstance().loadConfig();
	console.addLine(lang.get("SettingsSaved"));
}

void MenuStateOptions::setActiveInputLable(GraphicLabel *newLable) {
	MenuState::setActiveInputLabel(newLable,&activeInputLabel);

	if(newLable == &labelTransifexPwd) {
		labelTransifexPwd.setIsPassword(false);
	}
	else {
		labelTransifexPwd.setIsPassword(true);
	}
}

}}//end namespace
