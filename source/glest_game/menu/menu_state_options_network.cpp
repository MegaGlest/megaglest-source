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
#include "leak_dumper.h"

using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class MenuStateOptions
// =====================================================
MenuStateOptionsNetwork::MenuStateOptionsNetwork(Program *program, MainMenu *mainMenu, ProgramState **parentUI):
	MenuState(program, mainMenu, "config")
{
	try {
		containerName = "Options";
		Lang &lang= Lang::getInstance();
		Config &config= Config::getInstance();
		this->parentUI=parentUI;
		this->console.setOnlyChatMessagesInStoredLines(false);
		//modeinfos=list<ModeInfo> ();
		int leftLabelStart=50;
		int leftColumnStart=leftLabelStart+180;
		int rightLabelStart=450;
		int rightColumnStart=rightLabelStart+280;
		int buttonRowPos=50;
		int buttonStartPos=170;
		int captionOffset=75;
		int currentLabelStart=leftLabelStart;
		int currentColumnStart=leftColumnStart;
		int currentLine=700;
		int lineOffset=27;
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
		buttonMiscSection.init(400, 720,tabButtonWidth,tabButtonHeight);
		buttonMiscSection.setFont(CoreData::getInstance().getMenuFontVeryBig());
		buttonMiscSection.setFont3D(CoreData::getInstance().getMenuFontVeryBig3D());
		buttonMiscSection.setText(lang.get("Misc"));
		//NetworkSettings
		buttonNetworkSettings.registerGraphicComponent(containerName,"labelNetworkSettingsSection");
		buttonNetworkSettings.init(600, 700,tabButtonWidth,tabButtonHeight+20);
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


		// external server port
		labelPublishServerExternalPort.registerGraphicComponent(containerName,"labelPublishServerExternalPort");
		labelPublishServerExternalPort.init(currentLabelStart, currentLine, 150);
		labelPublishServerExternalPort.setText(lang.get("PublishServerExternalPort"));

		labelExternalPort.init(currentColumnStart,currentLine);
		string extPort= config.getString("PortExternal","not set");
		if(extPort == "not set" || extPort == "0"){
			extPort="   ---   ";
		}
		else{
			extPort="!!! "+extPort+" !!!";
		}
		labelExternalPort.setText(extPort);

		currentLine-=lineOffset;
		// server port
		labelServerPortLabel.registerGraphicComponent(containerName,"labelServerPortLabel");
		labelServerPortLabel.init(currentLabelStart,currentLine);
		labelServerPortLabel.setText(lang.get("ServerPort"));

		listBoxServerPort.registerGraphicComponent(containerName,"listBoxPublishServerExternalPort");
		listBoxServerPort.init(currentColumnStart, currentLine, 170);

		string portListString = config.getString("PortList",intToStr(GameConstants::serverPort).c_str());
		std::vector<std::string> portList;
		Tokenize(portListString,portList,",");

		string currentPort=config.getString("PortServer", intToStr(GameConstants::serverPort).c_str());
		int portSelectionIndex=0;
		for(int idx = 0; idx < portList.size(); idx++) {
			if(portList[idx] != "" && IsNumeric(portList[idx].c_str(),false)) {
				listBoxServerPort.pushBackItem(portList[idx]);
				if(currentPort==portList[idx])
				{
					portSelectionIndex=idx;
				}
			}
		}
		listBoxServerPort.setSelectedItemIndex(portSelectionIndex);

		currentLine-=lineOffset;
		labelFTPServerPortLabel.registerGraphicComponent(containerName,"labelFTPServerPortLabel");
		labelFTPServerPortLabel.init(currentLabelStart ,currentLine );
		labelFTPServerPortLabel.setText(lang.get("FTPServerPort"));

		int FTPPort = config.getInt("FTPServerPort",intToStr(ServerSocket::getFTPServerPort()).c_str());
		labelFTPServerPort.registerGraphicComponent(containerName,"labelFTPServerPort");
		labelFTPServerPort.init(currentColumnStart ,currentLine );
		labelFTPServerPort.setText(intToStr(FTPPort));
		currentLine-=lineOffset;
		labelFTPServerDataPortsLabel.registerGraphicComponent(containerName,"labelFTPServerDataPortsLabel");
		labelFTPServerDataPortsLabel.init(currentLabelStart ,currentLine );
		labelFTPServerDataPortsLabel.setText(lang.get("FTPServerDataPort"));

		char szBuf[8096]="";
		snprintf(szBuf,8096,"%d - %d",FTPPort + 1, FTPPort + GameConstants::maxPlayers);

		labelFTPServerDataPorts.registerGraphicComponent(containerName,"labelFTPServerDataPorts");
		labelFTPServerDataPorts.init(currentColumnStart,currentLine );
		labelFTPServerDataPorts.setText(szBuf);
		currentLine-=lineOffset;
		labelEnableFTPServer.registerGraphicComponent(containerName,"labelEnableFTPServer");
		labelEnableFTPServer.init(currentLabelStart ,currentLine);
		labelEnableFTPServer.setText(lang.get("EnableFTPServer"));

		checkBoxEnableFTPServer.registerGraphicComponent(containerName,"checkBoxEnableFTPServer");
		checkBoxEnableFTPServer.init(currentColumnStart ,currentLine );
		checkBoxEnableFTPServer.setValue(config.getBool("EnableFTPServer","true"));
		currentLine-=lineOffset;
		// FTP Config - start
		labelEnableFTP.registerGraphicComponent(containerName,"labelEnableFTP");
		labelEnableFTP.init(currentLabelStart ,currentLine);
		labelEnableFTP.setText(lang.get("EnableFTP"));

		checkBoxEnableFTP.registerGraphicComponent(containerName,"checkBoxEnableFTP");
		checkBoxEnableFTP.init(currentColumnStart ,currentLine );
		checkBoxEnableFTP.setValue(config.getBool("EnableFTPXfer","true"));
		currentLine-=lineOffset;

		labelEnableFTPServerInternetTilesetXfer.registerGraphicComponent(containerName,"labelEnableFTPServerInternetTilesetXfer");
		labelEnableFTPServerInternetTilesetXfer.init(currentLabelStart ,currentLine );
		labelEnableFTPServerInternetTilesetXfer.setText(lang.get("EnableFTPServerInternetTilesetXfer"));

		checkBoxEnableFTPServerInternetTilesetXfer.registerGraphicComponent(containerName,"checkBoxEnableFTPServerInternetTilesetXfer");
		checkBoxEnableFTPServerInternetTilesetXfer.init(currentColumnStart ,currentLine );
		checkBoxEnableFTPServerInternetTilesetXfer.setValue(config.getBool("EnableFTPServerInternetTilesetXfer","true"));

		currentLine-=lineOffset;

		labelEnableFTPServerInternetTechtreeXfer.registerGraphicComponent(containerName,"labelEnableFTPServerInternetTechtreeXfer");
		labelEnableFTPServerInternetTechtreeXfer.init(currentLabelStart ,currentLine );
		labelEnableFTPServerInternetTechtreeXfer.setText(lang.get("EnableFTPServerInternetTechtreeXfer"));

		checkBoxEnableFTPServerInternetTechtreeXfer.registerGraphicComponent(containerName,"checkBoxEnableFTPServerInternetTechtreeXfer");
		checkBoxEnableFTPServerInternetTechtreeXfer.init(currentColumnStart ,currentLine );
		checkBoxEnableFTPServerInternetTechtreeXfer.setValue(config.getBool("EnableFTPServerInternetTechtreeXfer","true"));

		currentLine-=lineOffset;


		// FTP config end

		// Privacy flag
		labelEnablePrivacy.registerGraphicComponent(containerName,"labelEnablePrivacy");
		labelEnablePrivacy.init(currentLabelStart ,currentLine);
		labelEnablePrivacy.setText(lang.get("PrivacyPlease"));

		checkBoxEnablePrivacy.registerGraphicComponent(containerName,"checkBoxEnablePrivacy");
		checkBoxEnablePrivacy.init(currentColumnStart ,currentLine );
		checkBoxEnablePrivacy.setValue(config.getBool("PrivacyPlease","false"));
		//currentLine-=lineOffset;
		// end

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

void MenuStateOptionsNetwork::reloadUI() {
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

	std::vector<string> listboxData;
	listboxData.push_back("None");
	listboxData.push_back("OpenAL");
// deprecated as of 3.6.1
//#ifdef WIN32
//	listboxData.push_back("DirectSound8");
//#endif


	listboxData.clear();
	listboxData.push_back("Bilinear");
	listboxData.push_back("Trilinear");

	listboxData.clear();
	for (float f=0.0;f<2.1f;f=f+0.1f) {
		listboxData.push_back(floatToStr(f));
	}
	listboxData.clear();
	for(int i= 0; i<Renderer::sCount; ++i){
		listboxData.push_back(lang.get(Renderer::shadowsToStr(static_cast<Renderer::Shadows>(i))));
	}


	labelServerPortLabel.setText(lang.get("ServerPort"));

	labelPublishServerExternalPort.setText(lang.get("PublishServerExternalPort"));

	labelEnableFTP.setText(lang.get("EnableFTP"));

	labelEnableFTPServer.setText(lang.get("EnableFTPServer"));

	labelFTPServerPortLabel.setText(lang.get("FTPServerPort"));

	labelFTPServerDataPortsLabel.setText(lang.get("FTPServerDataPort"));

	labelEnableFTPServerInternetTilesetXfer.setText(lang.get("EnableFTPServerInternetTilesetXfer"));

	labelEnableFTPServerInternetTechtreeXfer.setText(lang.get("EnableFTPServerInternetTechtreeXfer"));

	labelEnablePrivacy.setText(lang.get("PrivacyPlease"));

	buttonOk.setText(lang.get("Save"));
	buttonReturn.setText(lang.get("Return"));

	GraphicComponent::reloadFontsForRegisterGraphicComponents(containerName);
}

void MenuStateOptionsNetwork::showMessageBox(const string &text, const string &header, bool toggle){
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


void MenuStateOptionsNetwork::mouseClick(int x, int y, MouseButton mouseButton){

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

		}
	}
	else if(buttonOk.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundA());
		saveConfig();
		//mainMenu->setState(new MenuStateOptions(program, mainMenu,this->parentUI));
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
	else if(buttonAudioSection.mouseClick(x, y)){ 
			soundRenderer.playFx(coreData.getClickSoundA());
			mainMenu->setState(new MenuStateOptionsSound(program, mainMenu,this->parentUI)); // open keyboard shortcuts setup screen
			return;
		}
	else if(buttonNetworkSettings.mouseClick(x, y)){ 
			soundRenderer.playFx(coreData.getClickSoundA());
			//mainMenu->setState(new MenuStateOptionsNetwork(program, mainMenu,this->parentUI)); // open keyboard shortcuts setup screen
			return;
		}
	else if(buttonMiscSection.mouseClick(x, y)){ 
			soundRenderer.playFx(coreData.getClickSoundA());
			mainMenu->setState(new MenuStateOptions(program, mainMenu,this->parentUI)); // open keyboard shortcuts setup screen
			return;
		}
	else if(buttonVideoSection.mouseClick(x, y)){ 
			soundRenderer.playFx(coreData.getClickSoundA());
			mainMenu->setState(new MenuStateOptionsGraphics(program, mainMenu,this->parentUI)); // open keyboard shortcuts setup screen
			return;
		}
	else if(buttonKeyboardSetup.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundA());
		mainMenu->setState(new MenuStateKeysetup(program, mainMenu,this->parentUI)); // open keyboard shortcuts setup screen
		//showMessageBox("Not implemented yet", "Keyboard setup", false);
		return;
	}
	else
	{
		if(listBoxServerPort.mouseClick(x, y)){
			int selectedPort=strToInt(listBoxServerPort.getSelectedItem());
			if(selectedPort<10000){
				selectedPort=GameConstants::serverPort;
			}
			// use the following ports for ftp
			char szBuf[8096]="";
			snprintf(szBuf,8096,"%d - %d",selectedPort + 2, selectedPort + 1 + GameConstants::maxPlayers);
			labelFTPServerPort.setText(intToStr(selectedPort+1));
			labelFTPServerDataPorts.setText(szBuf);
		}

        checkBoxEnableFTP.mouseClick(x, y);
        checkBoxEnableFTPServer.mouseClick(x, y);

        checkBoxEnableFTPServerInternetTilesetXfer.mouseClick(x, y);
        checkBoxEnableFTPServerInternetTechtreeXfer.mouseClick(x, y);

        checkBoxEnablePrivacy.mouseClick(x, y);
	}
}

void MenuStateOptionsNetwork::mouseMove(int x, int y, const MouseState *ms){
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
	listBoxServerPort.mouseMove(x, y);
	checkBoxEnableFTP.mouseMove(x, y);
	checkBoxEnableFTPServer.mouseMove(x, y);
    checkBoxEnableFTPServerInternetTilesetXfer.mouseMove(x, y);
    checkBoxEnableFTPServerInternetTechtreeXfer.mouseMove(x, y);
	checkBoxEnablePrivacy.mouseMove(x, y);
}

//bool MenuStateOptionsNetwork::isInSpecialKeyCaptureEvent() {
//	return (activeInputLabel != NULL);
//}
//
//void MenuStateOptionsNetwork::keyDown(SDL_KeyboardEvent key) {
//	if(activeInputLabel != NULL) {
//		keyDownEditLabel(key, &activeInputLabel);
//	}
//}

void MenuStateOptionsNetwork::keyPress(SDL_KeyboardEvent c) {
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

void MenuStateOptionsNetwork::render(){
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
		renderer.renderLabel(&labelServerPortLabel);
		renderer.renderLabel(&labelExternalPort);
		renderer.renderLabel(&labelPublishServerExternalPort);
		renderer.renderListBox(&listBoxServerPort);


        renderer.renderLabel(&labelEnableFTP);
        renderer.renderCheckBox(&checkBoxEnableFTP);

        renderer.renderLabel(&labelEnableFTPServer);
        renderer.renderCheckBox(&checkBoxEnableFTPServer);

        renderer.renderLabel(&labelFTPServerPortLabel);
        renderer.renderLabel(&labelFTPServerPort);
        renderer.renderLabel(&labelFTPServerDataPortsLabel);
        renderer.renderLabel(&labelFTPServerDataPorts);

        renderer.renderLabel(&labelEnableFTPServerInternetTilesetXfer);
        renderer.renderCheckBox(&checkBoxEnableFTPServerInternetTilesetXfer);
        renderer.renderLabel(&labelEnableFTPServerInternetTechtreeXfer);
        renderer.renderCheckBox(&checkBoxEnableFTPServerInternetTechtreeXfer);

        renderer.renderLabel(&labelEnablePrivacy);
        renderer.renderCheckBox(&checkBoxEnablePrivacy);

	}

	renderer.renderConsole(&console,false,true);
	if(program != NULL) program->renderProgramMsgBox();
}

void MenuStateOptionsNetwork::saveConfig(){
	Config &config= Config::getInstance();
	Lang &lang= Lang::getInstance();
	setActiveInputLable(NULL);


	lang.loadStrings(config.getString("Lang"));

	config.setString("PortServer", listBoxServerPort.getSelectedItem());
	config.setInt("FTPServerPort",config.getInt("PortServer")+1);
    config.setBool("EnableFTPXfer", checkBoxEnableFTP.getValue());
    config.setBool("EnableFTPServer", checkBoxEnableFTPServer.getValue());

    config.setBool("EnableFTPServerInternetTilesetXfer", checkBoxEnableFTPServerInternetTilesetXfer.getValue());
    config.setBool("EnableFTPServerInternetTechtreeXfer", checkBoxEnableFTPServerInternetTechtreeXfer.getValue());

    config.setBool("PrivacyPlease", checkBoxEnablePrivacy.getValue());

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
	console.addLine(lang.get("SettingsSaved"));
}

void MenuStateOptionsNetwork::setActiveInputLable(GraphicLabel *newLable) {
}

}}//end namespace
