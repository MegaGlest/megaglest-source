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
#include "metrics.h"
#include "string_utils.h"

#include "leak_dumper.h"

namespace Glest{ namespace Game{


// =====================================================
// 	class MenuStateKeysetup
// =====================================================

MenuStateKeysetup::MenuStateKeysetup(Program *program, MainMenu *mainMenu,
		ProgramState **parentUI) :
	MenuState(program, mainMenu, "config")
{
	try {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		containerName = "KeySetup";

		this->parentUI = parentUI;
		hotkeyIndex = -1;
		hotkeyChar = SDLK_UNKNOWN;

		Lang &lang= Lang::getInstance();
		int buttonRowPos=80;
		// header
		labelTitle.registerGraphicComponent(containerName,"labelTitle");
		labelTitle.init(330,700);
		labelTitle.setFont(CoreData::getInstance().getMenuFontBig());
		labelTitle.setFont3D(CoreData::getInstance().getMenuFontBig3D());
		labelTitle.setText(lang.get("Keyboardsetup"));

		labelTestTitle.registerGraphicComponent(containerName,"labelTestTitle");
		labelTestTitle.init(50,700);
		labelTestTitle.setFont(CoreData::getInstance().getMenuFontBig());
		labelTestTitle.setFont3D(CoreData::getInstance().getMenuFontBig3D());
		labelTestTitle.setText(lang.get("KeyboardsetupTest"));

		labelTestValue.registerGraphicComponent(containerName,"labelTestValue");
		labelTestValue.init(50,670);
		labelTestValue.setFont(CoreData::getInstance().getMenuFontBig());
		labelTestValue.setFont3D(CoreData::getInstance().getMenuFontBig3D());
		labelTestValue.setText("");

		// mainMassegeBox
		mainMessageBox.registerGraphicComponent(containerName,"mainMessageBox");
		mainMessageBox.init(lang.get("Ok"));
		mainMessageBox.setEnabled(false);
		mainMessageBoxState=0;

		keyScrollBar.init(800,200,false,200,20);
		keyScrollBar.setLength(400);
		keyScrollBar.setElementCount(0);
		keyScrollBar.setVisibleSize(keyButtonsToRender);
		keyScrollBar.setVisibleStart(0);


		// buttons
		buttonOk.registerGraphicComponent(containerName,"buttonOk");
		buttonOk.init(200, buttonRowPos, 100);
		buttonOk.setText(lang.get("Ok"));

		buttonDefaults.registerGraphicComponent(containerName,"buttonDefaults");
		buttonDefaults.init(310, buttonRowPos, 100);
		buttonDefaults.setText(lang.get("Defaults"));

		buttonReturn.registerGraphicComponent(containerName,"buttonReturn");
		buttonReturn.init(420, buttonRowPos, 100);
		buttonReturn.setText(lang.get("Abort"));

		keyButtonsLineHeight=25;
		keyButtonsHeight=20;
		keyButtonsWidth=200;
		keyButtonsXBase=300;
		keyButtonsYBase=200+400-keyButtonsLineHeight;
		keyButtonsToRender=400/keyButtonsLineHeight;
		int labelWidth=100;

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));
		mergedProperties=configKeys.getMergedProperties();
		masterProperties=configKeys.getMasterProperties();
		//userProperties=configKeys.getUserProperties();
		userProperties.clear();

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		//throw megaglest_runtime_error("Test!");

		for(int i = 0; i < mergedProperties.size(); ++i) {

			string keyName = mergedProperties[i].second;
			if(keyName.length() > 0) {
				//char c = configKeys.translateStringToCharKey(keyName);
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
					keyName = mergedProperties[i].second;
				}
			}

			GraphicButton *button=new GraphicButton();
			button->init(keyButtonsXBase, keyButtonsYBase, keyButtonsWidth,keyButtonsHeight);
			button->setText(mergedProperties[i].first);
			keyButtons.push_back(button);
			GraphicLabel *label=new GraphicLabel();
			label->init(keyButtonsXBase+keyButtonsWidth+10,keyButtonsYBase,labelWidth,20);
			label->setText(keyName);
			labels.push_back(label);
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		keyScrollBar.init(keyButtonsXBase+keyButtonsWidth+labelWidth+20,200,false,200,20);
		keyScrollBar.setLength(400);
		keyScrollBar.setElementCount(keyButtons.size());
		keyScrollBar.setVisibleSize(keyButtonsToRender);
		keyScrollBar.setVisibleStart(0);
	}
	catch(const std::exception &ex) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s %d] Error detected:\n%s\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);

        mainMessageBoxState=1;
        showMessageBox( "Error: " + string(ex.what()), "Error detected", false);
	}
}

void MenuStateKeysetup::reloadUI() {
	Lang &lang= Lang::getInstance();

	console.resetFonts();
	labelTitle.setFont(CoreData::getInstance().getMenuFontBig());
	labelTitle.setFont3D(CoreData::getInstance().getMenuFontBig3D());
	labelTitle.setText(lang.get("Keyboardsetup"));

	labelTestTitle.setFont(CoreData::getInstance().getMenuFontBig());
	labelTestTitle.setFont3D(CoreData::getInstance().getMenuFontBig3D());
	labelTestTitle.setText(lang.get("KeyboardsetupTest"));

	labelTestValue.setFont(CoreData::getInstance().getMenuFontBig());
	labelTestValue.setFont3D(CoreData::getInstance().getMenuFontBig3D());
	labelTestValue.setText("");

	// mainMassegeBox
	mainMessageBox.init(lang.get("Ok"));

	buttonOk.setText(lang.get("Ok"));

	buttonDefaults.setText(lang.get("Defaults"));

	buttonReturn.setText(lang.get("Abort"));

	GraphicComponent::reloadFontsForRegisterGraphicComponents(containerName);
}

void MenuStateKeysetup::cleanup() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	clearUserButtons();
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] END\n",__FILE__,__FUNCTION__,__LINE__);
}

MenuStateKeysetup::~MenuStateKeysetup() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	cleanup();
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] END\n",__FILE__,__FUNCTION__,__LINE__);
}

void MenuStateKeysetup::clearUserButtons() {
	while(!keyButtons.empty()) {
		delete keyButtons.back();
		keyButtons.pop_back();
	}
	while(!labels.empty()) {
			delete labels.back();
			labels.pop_back();
		}
}

void MenuStateKeysetup::mouseClick(int x, int y, MouseButton mouseButton){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	CoreData &coreData= CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();

	if(mainMessageBox.getEnabled()){
		int button= 0;
		if(mainMessageBox.mouseClick(x, y, button))
		{
			soundRenderer.playFx(coreData.getClickSoundA());
			if(button==0)
			{
				mainMessageBox.setEnabled(false);
			}
		}
	}
	else if(keyScrollBar.mouseClick(x, y)){
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			soundRenderer.playFx(coreData.getClickSoundB());
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	    }
    else if(buttonReturn.mouseClick(x, y)){
    	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		soundRenderer.playFx(coreData.getClickSoundB());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		if(this->parentUI != NULL) {
			*this->parentUI = NULL;
			delete *this->parentUI;
		}

		mainMenu->setState(new MenuStateOptions(program, mainMenu));
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
    }
    else if(buttonDefaults.mouseClick(x, y)){
    	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		soundRenderer.playFx(coreData.getClickSoundB());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

        Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));
        string userKeysFile = configKeys.getFileName(true);

        bool result = removeFile(userKeysFile.c_str());
        if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] delete file [%s] returned %d\n",__FILE__,__FUNCTION__,__LINE__,userKeysFile.c_str(),result);
        if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] delete file [%s] returned %d\n",__FILE__,__FUNCTION__,__LINE__,userKeysFile.c_str(),result);
        configKeys.reload();

		if(this->parentUI != NULL) {
			*this->parentUI = NULL;
			delete *this->parentUI;
		}

		mainMenu->setState(new MenuStateKeysetup(program, mainMenu));
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
    }
    else if(buttonOk.mouseClick(x, y)){
    	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		soundRenderer.playFx(coreData.getClickSoundB());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

        if(userProperties.empty() == false) {
			Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));
			string userKeysFile = configKeys.getFileName(true);
	        if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] save file [%s] userProperties.size() = " MG_SIZE_T_SPECIFIER "\n",__FILE__,__FUNCTION__,__LINE__,userKeysFile.c_str(),userProperties.size());
	        if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] save file [%s] userProperties.size() = " MG_SIZE_T_SPECIFIER "\n",__FILE__,__FUNCTION__,__LINE__,userKeysFile.c_str(),userProperties.size());

			configKeys.setUserProperties(userProperties);
			configKeys.save();
			configKeys.reload();
        }

		if(this->parentUI != NULL) {
			*this->parentUI = NULL;
			delete *this->parentUI;
		}

		mainMenu->setState(new MenuStateOptions(program, mainMenu));
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
    }
    else {
    	if ( keyScrollBar.getElementCount() != 0) {
			for (int i = keyScrollBar.getVisibleStart(); i
					<= keyScrollBar.getVisibleEnd(); ++i) {
				if (keyButtons[i]->mouseClick(x, y)) {
				    hotkeyIndex = i;
				    hotkeyChar = SDLK_UNKNOWN;
					break;
				}
			}
		}
    }
}

void MenuStateKeysetup::mouseUp(int x, int y, const MouseButton mouseButton){
	if (mouseButton == mbLeft) {
		keyScrollBar.mouseUp(x, y);
	}
}

void MenuStateKeysetup::mouseMove(int x, int y, const MouseState *ms){
    buttonReturn.mouseMove(x, y);
    buttonOk.mouseMove(x, y);
    if (ms->get(mbLeft)) {
		keyScrollBar.mouseDown(x, y);
	} else {
		keyScrollBar.mouseMove(x, y);
	}

    if(keyScrollBar.getElementCount()!=0 ) {
    	for(int i = keyScrollBar.getVisibleStart(); i <= keyScrollBar.getVisibleEnd(); ++i) {
    		keyButtons[i]->mouseMove(x, y);
    	}
    }

}

void MenuStateKeysetup::render(){
	Renderer &renderer= Renderer::getInstance();

	//printf("MenuStateKeysetup::render A\n");

	if(mainMessageBox.getEnabled()) {
		//printf("MenuStateKeysetup::render B\n");
		renderer.renderMessageBox(&mainMessageBox);
	}
	else {
		//printf("MenuStateKeysetup::render C\n");
		renderer.renderButton(&buttonReturn);
		renderer.renderButton(&buttonDefaults);
		renderer.renderButton(&buttonOk);
		renderer.renderLabel(&labelTitle);
		renderer.renderLabel(&labelTestTitle);
		renderer.renderLabel(&labelTestValue);

		if(keyScrollBar.getElementCount()!=0 ) {
			for(int i = keyScrollBar.getVisibleStart(); i <= keyScrollBar.getVisibleEnd(); ++i) {
				if(hotkeyIndex == i) {
					renderer.renderButton(keyButtons[i],&YELLOW);
				}
				else {
					renderer.renderButton(keyButtons[i]);
				}
				renderer.renderLabel(labels[i]);
			}
		}
		renderer.renderScrollBar(&keyScrollBar);
	}

	renderer.renderConsole(&console,false,true);
	if(program != NULL) program->renderProgramMsgBox();
}

void MenuStateKeysetup::update() {
	//printf("MenuStateKeysetup::update A\n");

	if (keyScrollBar.getElementCount() != 0) {
		for (int i = keyScrollBar.getVisibleStart(); i
				<= keyScrollBar.getVisibleEnd(); ++i) {
			keyButtons[i]->setY(keyButtonsYBase - keyButtonsLineHeight * (i
					- keyScrollBar.getVisibleStart()));
			labels[i]->setY(keyButtonsYBase - keyButtonsLineHeight * (i
					- keyScrollBar.getVisibleStart()));
		}
	}

	console.update();
}



void MenuStateKeysetup::showMessageBox(const string &text, const string &header, bool toggle){
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


void MenuStateKeysetup::keyDown(SDL_KeyboardEvent key) {
	hotkeyChar = extractKeyPressed(key);
	//printf("\nkeyDown [%d]\n",hotkeyChar);

	string keyName = "";
	if(hotkeyChar > SDLK_UNKNOWN && hotkeyChar < SDLK_LAST) {
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] keyName [%s] char [%d][%d]\n",__FILE__,__FUNCTION__,__LINE__,keyName.c_str(),hotkeyChar,key.keysym.sym);
		keyName = SDL_GetKeyName(hotkeyChar);
	}
	//key = hotkeyChar;

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] keyName [%s] char [%d][%d]\n",__FILE__,__FUNCTION__,__LINE__,keyName.c_str(),hotkeyChar,key.keysym.sym);

//	SDLKey keysym = SDLK_UNKNOWN;
//	if(keyName == "unknown key" || keyName == "") {
//		Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));
//		keysym = configKeys.translateSpecialStringToSDLKey(hotkeyChar);
//
//		if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] keysym [%d]\n",__FILE__,__FUNCTION__,__LINE__,keysym);
//
//		// SDL skips capital letters
//		if(keysym >= 65 && keysym <= 90) {
//			keysym = (SDLKey)((int)keysym + 32);
//		}
//		//if(keysym < 255) {
//		//	key = keysym;
//		//}
//		keyName = SDL_GetKeyName(keysym);
//	}

	char szCharText[20]="";
	snprintf(szCharText,20,"%c",hotkeyChar);
	char *utfStr = String::ConvertToUTF8(&szCharText[0]);

	char szBuf[8096] = "";
	snprintf(szBuf,8096,"%s [%s][%d][%d][%d][%d]",keyName.c_str(),utfStr,key.keysym.sym,hotkeyChar,key.keysym.unicode,key.keysym.mod);
	labelTestValue.setText(szBuf);

	delete [] utfStr;

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] hotkeyChar [%d]\n",__FILE__,__FUNCTION__,__LINE__,hotkeyChar);
}

void MenuStateKeysetup::keyPress(SDL_KeyboardEvent c) {
}

void MenuStateKeysetup::keyUp(SDL_KeyboardEvent key) {
	//Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));

    if(hotkeyIndex >= 0) {
    	if(hotkeyChar != 0) {
    		if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] char [%d][%d]\n",__FILE__,__FUNCTION__,__LINE__,hotkeyChar,key.keysym.sym);

    		string keyName = "";
			if(hotkeyChar > SDLK_UNKNOWN && hotkeyChar < SDLK_LAST) {
				keyName = SDL_GetKeyName(hotkeyChar);
			}
			key.keysym.sym = hotkeyChar;

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] keyName [%s] char [%d][%d]\n",__FILE__,__FUNCTION__,__LINE__,keyName.c_str(),hotkeyChar,key.keysym.sym);

			SDLKey keysym = SDLK_UNKNOWN;
			if(keyName == "unknown key" || keyName == "") {
//				Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));
//				keysym = configKeys.translateSpecialStringToSDLKey(hotkeyChar);
//
//				if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] keysym [%d]\n",__FILE__,__FUNCTION__,__LINE__,keysym);
//
//				// SDL skips capital letters
//				if(keysym >= 65 && keysym <= 90) {
//					keysym = (SDLKey)((int)keysym + 32);
//				}
//				if(keysym < 255) {
//					key = keysym;
//				}
//				keyName = SDL_GetKeyName(keysym);
			}

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] keyName [%s] char [%d][%d]\n",__FILE__,__FUNCTION__,__LINE__,keyName.c_str(),hotkeyChar,key.keysym.sym);

			if(keyName != "unknown key") {
				GraphicLabel *label= labels[hotkeyIndex];
				label->setText(keyName);

				pair<string,string> &nameValuePair = mergedProperties[hotkeyIndex];
				bool isNewUserKeyEntry = true;
				for(int i = 0; i < userProperties.size(); ++i) {
					string hotKeyName = userProperties[i].first;
					if(nameValuePair.first == hotKeyName) {
//						if(keysym <= SDLK_ESCAPE || keysym > 255) {
//							if(keysym <= SDLK_ESCAPE) {
//								userProperties[i].second = intToStr(extractKeyPressed(key));
//							}
//							else {
//								userProperties[i].second = keyName;
//							}
//						}
//						else {
//							userProperties[i].second = "";
//							userProperties[i].second.push_back(extractKeyPressed(key));
//						}
						userProperties[i].second = keyName;
						isNewUserKeyEntry = false;
						break;
					}
				}
				if(isNewUserKeyEntry == true) {
					pair<string,string> newNameValuePair = nameValuePair;
//					if(keysym <= SDLK_ESCAPE || keysym > 255) {
//						if(keysym <= SDLK_ESCAPE) {
//							newNameValuePair.second = intToStr(extractKeyPressed(key));
//						}
//						else {
//							newNameValuePair.second = keyName;
//						}
//					}
//					else {
//						newNameValuePair.second = extractKeyPressed(key);
//					}
					newNameValuePair.second = keyName;
					userProperties.push_back(newNameValuePair);
				}
			}
    	}
        hotkeyIndex = -1;
        hotkeyChar = SDLK_UNKNOWN;
    }
}

}}//end namespace
