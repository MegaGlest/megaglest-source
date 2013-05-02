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
#include "leak_dumper.h"

using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class MenuStateOptions
// =====================================================
MenuStateOptionsSound::MenuStateOptionsSound(Program *program, MainMenu *mainMenu, ProgramState **parentUI):
	MenuState(program, mainMenu, "config")
{
	try {
		containerName = "Options";
		this->parentUI=parentUI;
		Lang &lang= Lang::getInstance();
		Config &config= Config::getInstance();
		this->console.setOnlyChatMessagesInStoredLines(false);

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
		buttonAudioSection.init(0, 700,tabButtonWidth,tabButtonHeight+20);
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
		buttonMiscSection.init(400, 720,tabButtonWidth,tabButtonHeight);
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

		//soundboxes
		labelSoundFactory.registerGraphicComponent(containerName,"labelSoundFactory");
		labelSoundFactory.init(currentLabelStart, currentLine);
		labelSoundFactory.setText(lang.get("SoundAndMusic"));

		listBoxSoundFactory.registerGraphicComponent(containerName,"listBoxSoundFactory");
		listBoxSoundFactory.init(currentColumnStart, currentLine, 100);
		listBoxSoundFactory.pushBackItem("None");
		listBoxSoundFactory.pushBackItem("OpenAL");
	// deprecated as of 3.6.1
	//#ifdef WIN32
		//listBoxSoundFactory.pushBackItem("DirectSound8");
	//#endif

		listBoxSoundFactory.setSelectedItem(config.getString("FactorySound"));
		currentLine-=lineOffset;

		labelVolumeFx.registerGraphicComponent(containerName,"labelVolumeFx");
		labelVolumeFx.init(currentLabelStart, currentLine);
		labelVolumeFx.setText(lang.get("FxVolume"));

		listBoxVolumeFx.registerGraphicComponent(containerName,"listBoxVolumeFx");
		listBoxVolumeFx.init(currentColumnStart, currentLine, 80);
		currentLine-=lineOffset;

		labelVolumeAmbient.registerGraphicComponent(containerName,"labelVolumeAmbient");
		labelVolumeAmbient.init(currentLabelStart, currentLine);

		listBoxVolumeAmbient.registerGraphicComponent(containerName,"listBoxVolumeAmbient");
		listBoxVolumeAmbient.init(currentColumnStart, currentLine, 80);
		labelVolumeAmbient.setText(lang.get("AmbientVolume"));
		currentLine-=lineOffset;

		labelVolumeMusic.registerGraphicComponent(containerName,"labelVolumeMusic");
		labelVolumeMusic.init(currentLabelStart, currentLine);

		listBoxVolumeMusic.registerGraphicComponent(containerName,"listBoxVolumeMusic");
		listBoxVolumeMusic.init(currentColumnStart, currentLine, 80);
		labelVolumeMusic.setText(lang.get("MusicVolume"));
		currentLine-=lineOffset;

		for(int i=0; i<=100; i+=5){
			listBoxVolumeFx.pushBackItem(intToStr(i));
			listBoxVolumeAmbient.pushBackItem(intToStr(i));
			listBoxVolumeMusic.pushBackItem(intToStr(i));
		}
		listBoxVolumeFx.setSelectedItem(intToStr(config.getInt("SoundVolumeFx")/5*5));
		listBoxVolumeAmbient.setSelectedItem(intToStr(config.getInt("SoundVolumeAmbient")/5*5));
		listBoxVolumeMusic.setSelectedItem(intToStr(config.getInt("SoundVolumeMusic")/5*5));

		currentLine-=lineOffset/2;



		//////////////////////////////////////////////////////////////////
		///////// RIGHT SIDE
		//////////////////////////////////////////////////////////////////

		currentLine=700; // reset line pos
		currentLabelStart=rightLabelStart; // set to right side
		currentColumnStart=rightColumnStart; // set to right side


		// buttons
		buttonOk.registerGraphicComponent(containerName,"buttonOk");
		buttonOk.init(buttonStartPos, buttonRowPos, 100);
		buttonOk.setText(lang.get("Save"));
		buttonReturn.setText(lang.get("Return"));

		buttonReturn.registerGraphicComponent(containerName,"buttonAbort");
		buttonReturn.init(buttonStartPos+110, buttonRowPos, 100);

		GraphicComponent::applyAllCustomProperties(containerName);
	}
	catch(exception &e) {
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error loading options: %s\n",__FILE__,__FUNCTION__,__LINE__,e.what());
		throw megaglest_runtime_error(string("Error loading options msg: ") + e.what());
	}
}

void MenuStateOptionsSound::reloadUI() {
	Lang &lang= Lang::getInstance();

	console.resetFonts();
	mainMessageBox.init(lang.get("Ok"));

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

	labelSoundFactory.setText(lang.get("SoundAndMusic"));

	std::vector<string> listboxData;
	listboxData.push_back("None");
	listboxData.push_back("OpenAL");
// deprecated as of 3.6.1
//#ifdef WIN32
//	listboxData.push_back("DirectSound8");
//#endif

	listBoxSoundFactory.setItems(listboxData);

	labelVolumeFx.setText(lang.get("FxVolume"));

	labelVolumeAmbient.setText(lang.get("AmbientVolume"));
	labelVolumeMusic.setText(lang.get("MusicVolume"));


	listboxData.clear();

	buttonOk.setText(lang.get("Save"));
	buttonReturn.setText(lang.get("Return"));

	GraphicComponent::reloadFontsForRegisterGraphicComponents(containerName);
}



void MenuStateOptionsSound::showMessageBox(const string &text, const string &header, bool toggle){
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


void MenuStateOptionsSound::mouseClick(int x, int y, MouseButton mouseButton){

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
					mainMenu->setState(new MenuStateOptions(program, mainMenu));
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


					this->mainMenu->init();
				}
			}
		}
	}
	else if(buttonOk.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundA());
		saveConfig();
		//mainMenu->setState(new MenuStateOptions(program, mainMenu));
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
		//mainMenu->setState(new MenuStateKeysetup(program, mainMenu)); // open keyboard shortcuts setup screen
		//mainMenu->setState(new MenuStateOptionsGraphics(program, mainMenu)); // open keyboard shortcuts setup screen
		//mainMenu->setState(new MenuStateOptionsNetwork(program, mainMenu)); // open keyboard shortcuts setup screen
		mainMenu->setState(new MenuStateKeysetup(program, mainMenu)); // open keyboard shortcuts setup screen
		//showMessageBox("Not implemented yet", "Keyboard setup", false);
		return;
	}
	else if(buttonAudioSection.mouseClick(x, y)){ 
			soundRenderer.playFx(coreData.getClickSoundA());
			//mainMenu->setState(new MenuStateOptionsSound(program, mainMenu)); // open keyboard shortcuts setup screen
			return;
		}
	else if(buttonNetworkSettings.mouseClick(x, y)){ 
			soundRenderer.playFx(coreData.getClickSoundA());
			mainMenu->setState(new MenuStateOptionsNetwork(program, mainMenu)); // open keyboard shortcuts setup screen
			return;
		}
	else if(buttonMiscSection.mouseClick(x, y)){ 
			soundRenderer.playFx(coreData.getClickSoundA());
			mainMenu->setState(new MenuStateOptions(program, mainMenu)); // open keyboard shortcuts setup screen
			return;
		}
	else if(buttonVideoSection.mouseClick(x, y)){ 
			soundRenderer.playFx(coreData.getClickSoundA());
			mainMenu->setState(new MenuStateOptionsGraphics(program, mainMenu)); // open keyboard shortcuts setup screen
			return;
		}
	else
	{
		listBoxSoundFactory.mouseClick(x, y);
		listBoxVolumeFx.mouseClick(x, y);
		listBoxVolumeAmbient.mouseClick(x, y);
		listBoxVolumeMusic.mouseClick(x, y);
	}
}

void MenuStateOptionsSound::mouseMove(int x, int y, const MouseState *ms){
	if (mainMessageBox.getEnabled()) {
		mainMessageBox.mouseMove(x, y);
	}
	buttonOk.mouseMove(x, y);
	buttonReturn.mouseMove(x, y);
	buttonKeyboardSetup.mouseMove(x, y);
	buttonAudioSection.mouseMove(x, y);
	buttonNetworkSettings.mouseMove(x, y);
	buttonMiscSection.mouseMove(x, y);
	buttonVideoSection.mouseMove(x, y);

	listBoxSoundFactory.mouseMove(x, y);
	listBoxVolumeFx.mouseMove(x, y);
	listBoxVolumeAmbient.mouseMove(x, y);
	listBoxVolumeMusic.mouseMove(x, y);
}

//bool MenuStateOptionsSound::isInSpecialKeyCaptureEvent() {
//	return (activeInputLabel != NULL);
//}
//
//void MenuStateOptionsSound::keyDown(SDL_KeyboardEvent key) {
//	if(activeInputLabel != NULL) {
//		keyDownEditLabel(key, &activeInputLabel);
//	}
//}

void MenuStateOptionsSound::keyPress(SDL_KeyboardEvent c) {
//	if(activeInputLabel != NULL) {
//	    //printf("[%d]\n",c); fflush(stdout);
//		if( &labelPlayerName 	== activeInputLabel ||
//			&labelTransifexUser == activeInputLabel ||
//			&labelTransifexPwd == activeInputLabel ||
//			&labelTransifexI18N == activeInputLabel) {
//			keyPressEditLabel(c, &activeInputLabel);
//		}
//	}
//	else {
		Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));
		if(isKeyPressed(configKeys.getSDLKey("SaveGUILayout"),c) == true) {
			GraphicComponent::saveAllCustomProperties(containerName);
			//Lang &lang= Lang::getInstance();
			//console.addLine(lang.get("GUILayoutSaved") + " [" + (saved ? lang.get("Yes") : lang.get("No"))+ "]");
		}
//	}
}

void MenuStateOptionsSound::render(){
	Renderer &renderer= Renderer::getInstance();

	if(mainMessageBox.getEnabled()){
		renderer.renderMessageBox(&mainMessageBox);
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
		renderer.renderListBox(&listBoxSoundFactory);
		renderer.renderLabel(&labelSoundFactory);


		renderer.renderListBox(&listBoxVolumeFx);
		renderer.renderLabel(&labelVolumeFx);
		renderer.renderListBox(&listBoxVolumeAmbient);
		renderer.renderLabel(&labelVolumeAmbient);
		renderer.renderListBox(&listBoxVolumeMusic);
		renderer.renderLabel(&labelVolumeMusic);



	}

	renderer.renderConsole(&console,false,true);
	if(program != NULL) program->renderProgramMsgBox();
}

void MenuStateOptionsSound::saveConfig(){
	Config &config= Config::getInstance();
	Lang &lang= Lang::getInstance();
	setActiveInputLable(NULL);

	config.setString("FactorySound", listBoxSoundFactory.getSelectedItem());
	config.setString("SoundVolumeFx", listBoxVolumeFx.getSelectedItem());
	config.setString("SoundVolumeAmbient", listBoxVolumeAmbient.getSelectedItem());
	CoreData::getInstance().getMenuMusic()->setVolume(strToInt(listBoxVolumeMusic.getSelectedItem())/100.f);
	config.setString("SoundVolumeMusic", listBoxVolumeMusic.getSelectedItem());

	config.save();

	if(config.getBool("DisableLuaSandbox","false") == true) {
		LuaScript::setDisableSandbox(true);
	}

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
	console.addLine(lang.get("SettingsSaved"));
}

void MenuStateOptionsSound::setActiveInputLable(GraphicLabel *newLable) {
}

}}//end namespace
