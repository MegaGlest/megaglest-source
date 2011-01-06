// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "menu_state_options.h"

#include "renderer.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "config.h"
#include "menu_state_root.h"
#include "util.h"
#include "menu_state_graphic_info.h"
#include "menu_state_keysetup.h"

#include "leak_dumper.h"

using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class MenuStateOptions
// =====================================================
MenuStateOptions::MenuStateOptions(Program *program, MainMenu *mainMenu):
	MenuState(program, mainMenu, "config")
{
	containerName = "Options";
	Lang &lang= Lang::getInstance();
	Config &config= Config::getInstance();
	//modeinfos=list<ModeInfo> ();
	Shared::PlatformCommon::getFullscreenVideoModes(&modeInfos);
	activeInputLabel=NULL;

	int leftline=700;
	int rightline=700;
	int leftLabelStart=250;
	int leftColumnStart=leftLabelStart+150;
	int rightLabelStart=500;
	int rightColumnStart=rightLabelStart+150;
	int buttonRowPos=80;
	int captionOffset=75;

	mainMessageBox.registerGraphicComponent(containerName,"mainMessageBox");
	mainMessageBox.init(lang.get("Ok"));
	mainMessageBox.setEnabled(false);
	mainMessageBoxState=0;

	labelAudioSection.registerGraphicComponent(containerName,"labelAudioSection");
	labelAudioSection.init(leftLabelStart+captionOffset, leftline);
	labelAudioSection.setFont(CoreData::getInstance().getMenuFontVeryBig());
	labelAudioSection.setText(lang.get("Audio"));
	leftline-=30;

	//soundboxes
	labelSoundFactory.registerGraphicComponent(containerName,"labelSoundFactory");
	labelSoundFactory.init(leftLabelStart, leftline);
	labelSoundFactory.setText(lang.get("SoundAndMusic"));

	listBoxSoundFactory.registerGraphicComponent(containerName,"listBoxSoundFactory");
	listBoxSoundFactory.init(leftColumnStart, leftline, 100);
	listBoxSoundFactory.pushBackItem("None");
	listBoxSoundFactory.pushBackItem("OpenAL");
#ifdef WIN32
	listBoxSoundFactory.pushBackItem("DirectSound8");
#endif

	listBoxSoundFactory.setSelectedItem(config.getString("FactorySound"));
	leftline-=30;

	labelVolumeFx.registerGraphicComponent(containerName,"labelVolumeFx");
	labelVolumeFx.init(leftLabelStart, leftline);
	labelVolumeFx.setText(lang.get("FxVolume"));

	listBoxVolumeFx.registerGraphicComponent(containerName,"listBoxVolumeFx");
	listBoxVolumeFx.init(leftColumnStart, leftline, 80);
	leftline-=30;

	labelVolumeAmbient.registerGraphicComponent(containerName,"labelVolumeAmbient");
	labelVolumeAmbient.init(leftLabelStart, leftline);

	listBoxVolumeAmbient.registerGraphicComponent(containerName,"listBoxVolumeAmbient");
	listBoxVolumeAmbient.init(leftColumnStart, leftline, 80);
	labelVolumeAmbient.setText(lang.get("AmbientVolume"));
	leftline-=30;

	labelVolumeMusic.registerGraphicComponent(containerName,"labelVolumeMusic");
	labelVolumeMusic.init(leftLabelStart, leftline);

	listBoxVolumeMusic.registerGraphicComponent(containerName,"listBoxVolumeMusic");
	listBoxVolumeMusic.init(leftColumnStart, leftline, 80);
	labelVolumeMusic.setText(lang.get("MusicVolume"));
	leftline-=30;

	for(int i=0; i<=100; i+=5){
		listBoxVolumeFx.pushBackItem(intToStr(i));
		listBoxVolumeAmbient.pushBackItem(intToStr(i));
		listBoxVolumeMusic.pushBackItem(intToStr(i));
	}
	listBoxVolumeFx.setSelectedItem(intToStr(config.getInt("SoundVolumeFx")/5*5));
	listBoxVolumeAmbient.setSelectedItem(intToStr(config.getInt("SoundVolumeAmbient")/5*5));
	listBoxVolumeMusic.setSelectedItem(intToStr(config.getInt("SoundVolumeMusic")/5*5));


	//leftline-=30;
	labelMiscSection.registerGraphicComponent(containerName,"labelMiscSection");
	labelMiscSection.init(leftLabelStart+captionOffset, leftline);
	labelMiscSection.setFont(CoreData::getInstance().getMenuFontVeryBig());
	labelMiscSection.setText(lang.get("Misc"));
	leftline-=30;

	//lang
	labelLang.registerGraphicComponent(containerName,"labelLang");
	labelLang.init(leftLabelStart, leftline);
	labelLang.setText(lang.get("Language"));

	listBoxLang.registerGraphicComponent(containerName,"listBoxLang");
	listBoxLang.init(leftColumnStart, leftline, 170);
	vector<string> langResults;

    string data_path = getGameReadWritePath(GameConstants::path_data_CacheLookupKey);
	findAll(data_path + "data/lang/*.lng", langResults, true);
	if(langResults.empty()){
        throw runtime_error("There is no lang file");
	}
    listBoxLang.setItems(langResults);
	listBoxLang.setSelectedItem(config.getString("Lang"));
	leftline-=30;

	//playerName
	labelPlayerNameLabel.registerGraphicComponent(containerName,"labelPlayerNameLabel");
	labelPlayerNameLabel.init(leftLabelStart,leftline);
	labelPlayerNameLabel.setText(lang.get("Playername"));

	labelPlayerName.registerGraphicComponent(containerName,"labelPlayerName");
	labelPlayerName.init(leftColumnStart,leftline);
	labelPlayerName.setText(config.getString("NetPlayerName",Socket::getHostName().c_str()));
	leftline-=30;

	//FontSizeAdjustment
	labelFontSizeAdjustment.registerGraphicComponent(containerName,"labelFontSizeAdjustment");
	labelFontSizeAdjustment.init(leftLabelStart,leftline);
	labelFontSizeAdjustment.setText(lang.get("FontSizeAdjustment"));

	listFontSizeAdjustment.registerGraphicComponent(containerName,"listFontSizeAdjustment");
	listFontSizeAdjustment.init(leftColumnStart, leftline, 80);
	for(int i=-5; i<=5; i+=1){
		listFontSizeAdjustment.pushBackItem(intToStr(i));
	}
	listFontSizeAdjustment.setSelectedItem(intToStr(config.getInt("FontSizeAdjustment")));

	leftline-=30;
	// server port
	labelServerPortLabel.registerGraphicComponent(containerName,"labelServerPortLabel");
	labelServerPortLabel.init(leftLabelStart,leftline);
	labelServerPortLabel.setText(lang.get("ServerPort"));
	labelServerPort.init(leftColumnStart,leftline);
	string port=intToStr(config.getInt("ServerPort"));
	if(port!="61357"){
		port=port +" ("+lang.get("NonStandardPort")+"!!)";
	}
	else{
		port=port +" ("+lang.get("StandardPort")+")";
	}

	labelServerPort.setText(port);

    // FTP Config - start
	labelEnableFTP.registerGraphicComponent(containerName,"labelEnableFTP");
	labelEnableFTP.init(leftLabelStart + 330,leftline);
	labelEnableFTP.setText(lang.get("EnableFTP"));

	checkBoxMapEnableFTP.registerGraphicComponent(containerName,"checkBoxMapEnableFTP");
	checkBoxMapEnableFTP.init(leftLabelStart + 450,leftline);
	checkBoxMapEnableFTP.setValue(config.getBool("EnableFTPXfer"));

	labelFTPServerPortLabel.registerGraphicComponent(containerName,"labelFTPServerPortLabel");
	labelFTPServerPortLabel.init(leftLabelStart + 330,leftline - 30);
	labelFTPServerPortLabel.setText(lang.get("FTPServerPort"));

    int FTPPort = config.getInt("FTPServerPort",intToStr(ServerSocket::getFTPServerPort()).c_str());
	labelFTPServerPort.registerGraphicComponent(containerName,"labelFTPServerPort");
	labelFTPServerPort.init(leftLabelStart + 330,leftline - 40);
	labelFTPServerPort.setText(intToStr(FTPPort));

	labelFTPServerDataPortsLabel.registerGraphicComponent(containerName,"labelFTPServerDataPortsLabel");
	labelFTPServerDataPortsLabel.init(leftLabelStart + 450,leftline - 30);
	labelFTPServerDataPortsLabel.setText(lang.get("FTPServerDataPort"));

    char szBuf[1024]="";
    sprintf(szBuf,"%d - %d",FTPPort + 1, FTPPort + GameConstants::maxPlayers);

	labelFTPServerDataPorts.registerGraphicComponent(containerName,"labelFTPServerDataPorts");
	labelFTPServerDataPorts.init(leftLabelStart + 450,leftline - 40);
	labelFTPServerDataPorts.setText(szBuf);

    // FTP config end

	// external server port
	leftline-=30;

	labelPublishServerExternalPort.registerGraphicComponent(containerName,"labelPublishServerExternalPort");
	labelPublishServerExternalPort.init(leftLabelStart, leftline, 150);
	labelPublishServerExternalPort.setText(lang.get("PublishServerExternalPort"));

	listBoxPublishServerExternalPort.registerGraphicComponent(containerName,"listBoxPublishServerExternalPort");
	listBoxPublishServerExternalPort.init(leftColumnStart, leftline, 170);
	string supportExternalPortList = config.getString("MasterServerExternalPortList",intToStr(GameConstants::serverPort).c_str());
	std::vector<std::string> externalPortList;
	Tokenize(supportExternalPortList,externalPortList,",");

	string currentPort=config.getString("MasterServerExternalPort", "61357");
	int masterServerExternalPortSelectionIndex=0;
	for(int idx = 0; idx < externalPortList.size(); idx++) {
		if(externalPortList[idx] != "" && IsNumeric(externalPortList[idx].c_str(),false)) {
			listBoxPublishServerExternalPort.pushBackItem(externalPortList[idx]);
			if(currentPort==externalPortList[idx])
			{
				masterServerExternalPortSelectionIndex=idx;
			}
		}
	}
	listBoxPublishServerExternalPort.setSelectedItemIndex(masterServerExternalPortSelectionIndex);

	// Video Section
	leftline-=30;
	labelVideoSection.registerGraphicComponent(containerName,"labelVideoSection");
	labelVideoSection.init(leftLabelStart+captionOffset, leftline);
	labelVideoSection.setFont(CoreData::getInstance().getMenuFontVeryBig());
	labelVideoSection.setText(lang.get("Video"));
	leftline-=30;

	//resolution
	labelScreenModes.registerGraphicComponent(containerName,"labelScreenModes");
	labelScreenModes.init(leftLabelStart, leftline);
	labelScreenModes.setText(lang.get("Resolution"));

	listBoxScreenModes.registerGraphicComponent(containerName,"listBoxScreenModes");
	listBoxScreenModes.init(leftColumnStart, leftline, 170);

	string currentResString = config.getString("ScreenWidth") + "x" +
							  config.getString("ScreenHeight") + "-" +
							  intToStr(config.getInt("ColorBits"));
	bool currentResolutionFound = false;
	for(list<ModeInfo>::const_iterator it= modeInfos.begin(); it!=modeInfos.end(); ++it){
		if((*it).getString() == currentResString) {
			currentResolutionFound = true;
		}
		listBoxScreenModes.pushBackItem((*it).getString());
	}
	if(currentResolutionFound == false) {
		listBoxScreenModes.pushBackItem(currentResString);
	}
	listBoxScreenModes.setSelectedItem(currentResString);
	leftline-=30;


	//FullscreenWindowed
	labelFullscreenWindowed.registerGraphicComponent(containerName,"labelFullscreenWindowed");
	labelFullscreenWindowed.init(leftLabelStart, leftline);

	checkBoxFullscreenWindowed.registerGraphicComponent(containerName,"checkBoxFullscreenWindowed");
	checkBoxFullscreenWindowed.init(leftColumnStart, leftline);
	labelFullscreenWindowed.setText(lang.get("Windowed"));
	checkBoxFullscreenWindowed.setValue(config.getBool("Windowed"));
	leftline-=30;

	//filter
	labelFilter.registerGraphicComponent(containerName,"labelFilter");
	labelFilter.init(leftLabelStart, leftline);
	labelFilter.setText(lang.get("Filter"));

	listBoxFilter.registerGraphicComponent(containerName,"listBoxFilter");
	listBoxFilter.init(leftColumnStart, leftline, 170);
	listBoxFilter.pushBackItem("Bilinear");
	listBoxFilter.pushBackItem("Trilinear");
	listBoxFilter.setSelectedItem(config.getString("Filter"));
	leftline-=30;

	//shadows
	labelShadows.registerGraphicComponent(containerName,"labelShadows");
	labelShadows.init(leftLabelStart, leftline);
	labelShadows.setText(lang.get("Shadows"));

	listBoxShadows.registerGraphicComponent(containerName,"listBoxShadows");
	listBoxShadows.init(leftColumnStart, leftline, 170);
	for(int i= 0; i<Renderer::sCount; ++i){
		listBoxShadows.pushBackItem(lang.get(Renderer::shadowsToStr(static_cast<Renderer::Shadows>(i))));
	}
	string str= config.getString("Shadows");
	listBoxShadows.setSelectedItemIndex(clamp(Renderer::strToShadows(str), 0, Renderer::sCount-1));
	leftline-=30;

	//textures 3d
	labelTextures3D.registerGraphicComponent(containerName,"labelTextures3D");
	labelTextures3D.init(leftLabelStart, leftline);

	checkBoxTextures3D.registerGraphicComponent(containerName,"checkBoxTextures3D");
	checkBoxTextures3D.init(leftColumnStart, leftline);
	labelTextures3D.setText(lang.get("Textures3D"));
	checkBoxTextures3D.setValue(config.getBool("Textures3D"));
	leftline-=30;

	//lights
	labelLights.registerGraphicComponent(containerName,"labelLights");
	labelLights.init(leftLabelStart, leftline);
	labelLights.setText(lang.get("MaxLights"));

	listBoxLights.registerGraphicComponent(containerName,"listBoxLights");
	listBoxLights.init(leftColumnStart, leftline, 80);
	for(int i= 1; i<=8; ++i){
		listBoxLights.pushBackItem(intToStr(i));
	}
	listBoxLights.setSelectedItemIndex(clamp(config.getInt("MaxLights")-1, 0, 7));
	leftline-=30;

	//unit particles
	labelUnitParticles.registerGraphicComponent(containerName,"labelUnitParticles");
	labelUnitParticles.init(leftLabelStart,leftline);
	labelUnitParticles.setText(lang.get("ShowUnitParticles"));

	checkBoxUnitParticles.registerGraphicComponent(containerName,"checkBoxUnitParticles");
	checkBoxUnitParticles.init(leftColumnStart,leftline);
	checkBoxUnitParticles.setValue(config.getBool("UnitParticles"));
	leftline-=30;

	//unit particles
	labelMapPreview.registerGraphicComponent(containerName,"labelMapPreview");
	labelMapPreview.init(leftLabelStart,leftline);
	labelMapPreview.setText(lang.get("ShowMapPreview"));

	checkBoxMapPreview.registerGraphicComponent(containerName,"checkBoxMapPreview");
	checkBoxMapPreview.init(leftColumnStart,leftline);
	checkBoxMapPreview.setValue(config.getBool("MapPreview","true"));
	leftline-=30;

	// buttons
	buttonOk.registerGraphicComponent(containerName,"buttonOk");
	buttonOk.init(200, buttonRowPos, 100);
	buttonOk.setText(lang.get("Ok"));
	buttonAbort.setText(lang.get("Abort"));

	buttonAbort.registerGraphicComponent(containerName,"buttonAbort");
	buttonAbort.init(310, buttonRowPos, 100);
	buttonAutoConfig.setText(lang.get("AutoConfig"));

	buttonAutoConfig.registerGraphicComponent(containerName,"buttonAutoConfig");
	buttonAutoConfig.init(450, buttonRowPos, 125);

	buttonVideoInfo.setText(lang.get("VideoInfo"));
	buttonVideoInfo.registerGraphicComponent(containerName,"buttonVideoInfo");
	buttonVideoInfo.init(585, buttonRowPos, 125); // was 620

	buttonKeyboardSetup.setText(lang.get("Keyboardsetup"));
	buttonKeyboardSetup.registerGraphicComponent(containerName,"buttonKeyboardSetup");
	buttonKeyboardSetup.init(720, buttonRowPos, 125);

	GraphicComponent::applyAllCustomProperties(containerName);
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



void MenuStateOptions::mouseClick(int x, int y, MouseButton mouseButton){

	Config &config= Config::getInstance();
	CoreData &coreData= CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();

	if(mainMessageBox.getEnabled()){
		int button= 1;
		if(mainMessageBox.mouseClick(x, y, button))
		{
			soundRenderer.playFx(coreData.getClickSoundA());
			if(button==1)
			{
				if(mainMessageBoxState==1)
				{
					mainMessageBox.setEnabled(false);
					saveConfig();
					mainMenu->setState(new MenuStateRoot(program, mainMenu));
				}
				else
					mainMessageBox.setEnabled(false);
			}
		}
	}
	else if(buttonOk.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundA());

		string currentResolution=config.getString("ScreenWidth")+"x"+config.getString("ScreenHeight")+"-"+intToStr(config.getInt("ColorBits"));
		string selectedResolution=listBoxScreenModes.getSelectedItem();
		if(currentResolution!=selectedResolution){
			mainMessageBoxState=1;
			Lang &lang= Lang::getInstance();
			showMessageBox(lang.get("RestartNeeded"), lang.get("ResolutionChanged"), false);
			return;
		}
		string currentFontSizeAdjustment=config.getString("FontSizeAdjustment");
		string selectedFontSizeAdjustment=listFontSizeAdjustment.getSelectedItem();
		if(currentFontSizeAdjustment!=selectedFontSizeAdjustment){
			mainMessageBoxState=1;
			Lang &lang= Lang::getInstance();
			showMessageBox(lang.get("RestartNeeded"), lang.get("FontSizeAdjustmentChanged"), false);
			return;
		}

		bool currentFullscreenWindowed=config.getBool("Windowed");
		bool selectedFullscreenWindowed = checkBoxFullscreenWindowed.getValue();
		if(currentFullscreenWindowed!=selectedFullscreenWindowed){
			mainMessageBoxState=1;
			Lang &lang= Lang::getInstance();
			showMessageBox(lang.get("RestartNeeded"), lang.get("DisplaySettingsChanged"), false);
			return;
		}

		saveConfig();
		mainMenu->setState(new MenuStateRoot(program, mainMenu));
    }
	else if(buttonAbort.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundA());
		mainMenu->setState(new MenuStateRoot(program, mainMenu));
    }
	else if(buttonAutoConfig.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundA());
		Renderer::getInstance().autoConfig();
		saveConfig();
		mainMenu->setState(new MenuStateOptions(program, mainMenu));
	}
	else if(buttonVideoInfo.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundA());
		mainMenu->setState(new MenuStateGraphicInfo(program, mainMenu));
	}
	else if(buttonKeyboardSetup.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundA());
		mainMenu->setState(new MenuStateKeysetup(program, mainMenu)); // open keyboard shortcuts setup screen
		//showMessageBox("Not implemented yet", "Keyboard setup", false);
	}
	else if(labelPlayerName.mouseClick(x, y) && ( activeInputLabel != &labelPlayerName )){
			setActiveInputLable(&labelPlayerName);
	}
	else
	{
		listBoxLang.mouseClick(x, y);
		listBoxShadows.mouseClick(x, y);
		listBoxFilter.mouseClick(x, y);
		checkBoxTextures3D.mouseClick(x, y);
		checkBoxUnitParticles.mouseClick(x, y);
		checkBoxMapPreview.mouseClick(x, y);
		listBoxLights.mouseClick(x, y);
		listBoxSoundFactory.mouseClick(x, y);
		listBoxVolumeFx.mouseClick(x, y);
		listBoxVolumeAmbient.mouseClick(x, y);
		listBoxVolumeMusic.mouseClick(x, y);
		listBoxScreenModes.mouseClick(x, y);
		listFontSizeAdjustment.mouseClick(x, y);
		checkBoxFullscreenWindowed.mouseClick(x, y);
		listBoxPublishServerExternalPort.mouseClick(x, y);

        checkBoxMapEnableFTP.mouseClick(x, y);
	}
}

void MenuStateOptions::mouseMove(int x, int y, const MouseState *ms){
	if (mainMessageBox.getEnabled()) {
			mainMessageBox.mouseMove(x, y);
		}
	buttonOk.mouseMove(x, y);
	buttonAbort.mouseMove(x, y);
	buttonAutoConfig.mouseMove(x, y);
	buttonVideoInfo.mouseMove(x, y);
	buttonKeyboardSetup.mouseMove(x, y);
	listBoxLang.mouseMove(x, y);
	listBoxSoundFactory.mouseMove(x, y);
	listBoxVolumeFx.mouseMove(x, y);
	listBoxVolumeAmbient.mouseMove(x, y);
	listBoxVolumeMusic.mouseMove(x, y);
	listBoxLang.mouseMove(x, y);
	listBoxFilter.mouseMove(x, y);
	listBoxShadows.mouseMove(x, y);
	checkBoxTextures3D.mouseMove(x, y);
	checkBoxUnitParticles.mouseMove(x, y);
	checkBoxMapPreview.mouseMove(x, y);
	listBoxLights.mouseMove(x, y);
	listBoxScreenModes.mouseMove(x, y);
	checkBoxFullscreenWindowed.mouseMove(x, y);
	listFontSizeAdjustment.mouseMove(x, y);
	listBoxPublishServerExternalPort.mouseMove(x, y);

	checkBoxMapEnableFTP.mouseMove(x, y);
}

void MenuStateOptions::keyDown(char key){
	if(activeInputLabel!=NULL)
	{
		if(key==vkBack){
			string text= activeInputLabel->getText();
			if(text.size()>1){
				text.erase(text.end()-2);
			}
			activeInputLabel->setText(text);
		}
	}
}

void MenuStateOptions::keyPress(char c){
	if(activeInputLabel!=NULL)
	{
	    //printf("[%d]\n",c); fflush(stdout);
		int maxTextSize= 16;
		if(&labelPlayerName==activeInputLabel){
			if((c>='0' && c<='9')||(c>='a' && c<='z')||(c>='A' && c<='Z')||
				// (c>=(192-256) && c<=(255-256))||     // test some support for accented letters in names, is this ok? (latin1 signed char)
				// no master server breaks, and a russian translation with game switched to KOI-8p encoding? probably irc too.
				// (use Shared::Platform::charSet in shared_lib/include/platform/sdl/gl_wrap.h ?)
				(c=='-')||(c=='(')||(c==')')){
				if(activeInputLabel->getText().size()<maxTextSize){
					string text= activeInputLabel->getText();
					text.insert(text.end()-1, c);
					activeInputLabel->setText(text);
				}
			}
		}
	}
	else {
		Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));
		if(c == configKeys.getCharKey("SaveGUILayout")) {
			bool saved = GraphicComponent::saveAllCustomProperties(containerName);
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
	else
	{
		renderer.renderButton(&buttonOk);
		renderer.renderButton(&buttonAbort);
		renderer.renderButton(&buttonAutoConfig);
		renderer.renderButton(&buttonVideoInfo);
		renderer.renderButton(&buttonKeyboardSetup);
		renderer.renderListBox(&listBoxLang);
		renderer.renderListBox(&listBoxShadows);
		renderer.renderCheckBox(&checkBoxTextures3D);
		renderer.renderCheckBox(&checkBoxUnitParticles);
		renderer.renderCheckBox(&checkBoxMapPreview);
		renderer.renderListBox(&listBoxLights);
		renderer.renderListBox(&listBoxFilter);
		renderer.renderListBox(&listBoxSoundFactory);
		renderer.renderListBox(&listBoxVolumeFx);
		renderer.renderListBox(&listBoxVolumeAmbient);
		renderer.renderListBox(&listBoxVolumeMusic);
		renderer.renderLabel(&labelLang);
		renderer.renderLabel(&labelPlayerNameLabel);
		renderer.renderLabel(&labelPlayerName);
		renderer.renderLabel(&labelShadows);
		renderer.renderLabel(&labelTextures3D);
		renderer.renderLabel(&labelUnitParticles);
		renderer.renderLabel(&labelMapPreview);
		renderer.renderLabel(&labelLights);
		renderer.renderLabel(&labelFilter);
		renderer.renderLabel(&labelSoundFactory);
		renderer.renderLabel(&labelVolumeFx);
		renderer.renderLabel(&labelVolumeAmbient);
		renderer.renderLabel(&labelVolumeMusic);
		renderer.renderLabel(&labelVideoSection);
		renderer.renderLabel(&labelAudioSection);
		renderer.renderLabel(&labelMiscSection);
		renderer.renderLabel(&labelScreenModes);
		renderer.renderListBox(&listBoxScreenModes);
		renderer.renderLabel(&labelServerPortLabel);
		renderer.renderLabel(&labelServerPort);
		renderer.renderListBox(&listFontSizeAdjustment);
		renderer.renderLabel(&labelFontSizeAdjustment);
		renderer.renderLabel(&labelFullscreenWindowed);
		renderer.renderCheckBox(&checkBoxFullscreenWindowed);
		renderer.renderLabel(&labelPublishServerExternalPort);
		renderer.renderListBox(&listBoxPublishServerExternalPort);

        renderer.renderLabel(&labelEnableFTP);
        renderer.renderCheckBox(&checkBoxMapEnableFTP);
        renderer.renderLabel(&labelFTPServerPortLabel);
        renderer.renderLabel(&labelFTPServerPort);
        renderer.renderLabel(&labelFTPServerDataPortsLabel);
        renderer.renderLabel(&labelFTPServerDataPorts);
	}

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
	config.setString("Lang", listBoxLang.getSelectedItem());
	lang.loadStrings(config.getString("Lang"));

	int index= listBoxShadows.getSelectedItemIndex();
	config.setString("Shadows", Renderer::shadowsToStr(static_cast<Renderer::Shadows>(index)));

	config.setBool("Windowed", checkBoxFullscreenWindowed.getValue());
	config.setString("Filter", listBoxFilter.getSelectedItem());
	config.setBool("Textures3D", checkBoxTextures3D.getValue());
	config.setBool("UnitParticles", (checkBoxUnitParticles.getValue()));
	config.setBool("MapPreview", checkBoxMapPreview.getValue());
	config.setInt("MaxLights", listBoxLights.getSelectedItemIndex()+1);
	config.setString("FactorySound", listBoxSoundFactory.getSelectedItem());
	config.setString("SoundVolumeFx", listBoxVolumeFx.getSelectedItem());
	config.setString("SoundVolumeAmbient", listBoxVolumeAmbient.getSelectedItem());
	config.setString("FontSizeAdjustment", listFontSizeAdjustment.getSelectedItem());
	CoreData::getInstance().getMenuMusic()->setVolume(strToInt(listBoxVolumeMusic.getSelectedItem())/100.f);
	config.setString("SoundVolumeMusic", listBoxVolumeMusic.getSelectedItem());
	config.setString("MasterServerExternalPort", listBoxPublishServerExternalPort.getSelectedItem());
    config.setBool("EnableFTPXfer", checkBoxMapEnableFTP.getValue());

	string currentResolution=config.getString("ScreenWidth")+"x"+config.getString("ScreenHeight");
	string selectedResolution=listBoxScreenModes.getSelectedItem();
	if(currentResolution!=selectedResolution){
		for(list<ModeInfo>::const_iterator it= modeInfos.begin(); it!=modeInfos.end(); ++it){
			if((*it).getString()==selectedResolution)
			{
				config.setInt("ScreenWidth",(*it).width);
				config.setInt("ScreenHeight",(*it).height);
				config.setInt("ColorBits",(*it).depth);
			}
		}
	}

	config.save();

    SoundRenderer &soundRenderer= SoundRenderer::getInstance();
    soundRenderer.stopAllSounds();
    bool initOk = soundRenderer.init(program->getWindow());
    soundRenderer.loadConfig();
    soundRenderer.setMusicVolume(CoreData::getInstance().getMenuMusic());
	soundRenderer.playMusic(CoreData::getInstance().getMenuMusic());

	Renderer::getInstance().loadConfig();
}

void MenuStateOptions::setActiveInputLable(GraphicLabel *newLable)
{
	if(newLable!=NULL){
		string text= newLable->getText();
		size_t found;
		found=text.find_last_of("_");
		if (found==string::npos)
		{
			text=text+"_";
		}
		newLable->setText(text);
	}
	if(activeInputLabel!=NULL && !activeInputLabel->getText().empty()){
		string text= activeInputLabel->getText();
		size_t found;
		found=text.find_last_of("_");
		if (found!=string::npos)
		{
			text=text.substr(0,found);
		}
		activeInputLabel->setText(text);
	}
	activeInputLabel=newLable;
}

}}//end namespace
