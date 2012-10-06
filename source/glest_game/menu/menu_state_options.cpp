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
#include "game.h"
#include "program.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "config.h"
#include "menu_state_root.h"
#include "util.h"
#include "menu_state_graphic_info.h"
#include "menu_state_keysetup.h"
#include "string_utils.h"
#include "leak_dumper.h"

using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class MenuStateOptions
// =====================================================
MenuStateOptions::MenuStateOptions(Program *program, MainMenu *mainMenu):
	MenuState(program, mainMenu, "config")
{
	try {
		containerName = "Options";
		Lang &lang= Lang::getInstance();
		Config &config= Config::getInstance();
		//modeinfos=list<ModeInfo> ();
		Shared::PlatformCommon::getFullscreenVideoModes(&modeInfos,!config.getBool("Windowed"));
		activeInputLabel=NULL;

		int leftLabelStart=50;
		int leftColumnStart=leftLabelStart+180;
		int rightLabelStart=420;
		int rightColumnStart=rightLabelStart+310;
		int buttonRowPos=80;
		int buttonStartPos=170;
		int captionOffset=75;
		int currentLabelStart=leftLabelStart;
		int currentColumnStart=leftColumnStart;
		int currentLine=700;
		int lineOffset=27;

		mainMessageBox.registerGraphicComponent(containerName,"mainMessageBox");
		mainMessageBox.init(lang.get("Ok"));
		mainMessageBox.setEnabled(false);
		mainMessageBoxState=0;

		labelAudioSection.registerGraphicComponent(containerName,"labelAudioSection");
		labelAudioSection.init(currentLabelStart+captionOffset, currentLine);
		labelAudioSection.setFont(CoreData::getInstance().getMenuFontVeryBig());
		labelAudioSection.setFont3D(CoreData::getInstance().getMenuFontVeryBig3D());
		labelAudioSection.setText(lang.get("Audio"));
		currentLine-=lineOffset;

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

		currentLine-=lineOffset;
		// Video Section
		labelVideoSection.registerGraphicComponent(containerName,"labelVideoSection");
		labelVideoSection.init(currentLabelStart+captionOffset, currentLine);
		labelVideoSection.setFont(CoreData::getInstance().getMenuFontVeryBig());
		labelVideoSection.setFont3D(CoreData::getInstance().getMenuFontVeryBig3D());
		labelVideoSection.setText(lang.get("Video"));
		currentLine-=lineOffset;

		//resolution
		labelScreenModes.registerGraphicComponent(containerName,"labelScreenModes");
		labelScreenModes.init(currentLabelStart, currentLine);
		labelScreenModes.setText(lang.get("Resolution"));

		listBoxScreenModes.registerGraphicComponent(containerName,"listBoxScreenModes");
		listBoxScreenModes.init(currentColumnStart, currentLine, 170);

		string currentResString = config.getString("ScreenWidth") + "x" +
								  config.getString("ScreenHeight") + "-" +
								  intToStr(config.getInt("ColorBits"));
		bool currentResolutionFound = false;
		for(vector<ModeInfo>::const_iterator it= modeInfos.begin(); it!=modeInfos.end(); ++it){
			if((*it).getString() == currentResString) {
				currentResolutionFound = true;
			}
			listBoxScreenModes.pushBackItem((*it).getString());
		}
		if(currentResolutionFound == false) {
			listBoxScreenModes.pushBackItem(currentResString);
		}
		listBoxScreenModes.setSelectedItem(currentResString);
		currentLine-=lineOffset;


		//FullscreenWindowed
		labelFullscreenWindowed.registerGraphicComponent(containerName,"labelFullscreenWindowed");
		labelFullscreenWindowed.init(currentLabelStart, currentLine);

		checkBoxFullscreenWindowed.registerGraphicComponent(containerName,"checkBoxFullscreenWindowed");
		checkBoxFullscreenWindowed.init(currentColumnStart, currentLine);
		labelFullscreenWindowed.setText(lang.get("Windowed"));
		checkBoxFullscreenWindowed.setValue(config.getBool("Windowed"));
		currentLine-=lineOffset;

		//gammaCorrection
		labelGammaCorrection.registerGraphicComponent(containerName,"labelGammaCorrection");
		labelGammaCorrection.init(currentLabelStart, currentLine);
		labelGammaCorrection.setText(lang.get("GammaCorrection"));

		listBoxGammaCorrection.registerGraphicComponent(containerName,"listBoxGammaCorrection");
		listBoxGammaCorrection.init(currentColumnStart, currentLine, 170);
		for (float f=0.5;f<3.0f;f=f+0.1f) {
			listBoxGammaCorrection.pushBackItem(floatToStr(f));
		}
		float gammaValue=config.getFloat("GammaValue","1.0");
		if(gammaValue==0.0f) gammaValue=1.0f;
		listBoxGammaCorrection.setSelectedItem(floatToStr(gammaValue));

		currentLine-=lineOffset;

		//filter
		labelFilter.registerGraphicComponent(containerName,"labelFilter");
		labelFilter.init(currentLabelStart, currentLine);
		labelFilter.setText(lang.get("Filter"));

		listBoxFilter.registerGraphicComponent(containerName,"listBoxFilter");
		listBoxFilter.init(currentColumnStart, currentLine, 170);
		listBoxFilter.pushBackItem("Bilinear");
		listBoxFilter.pushBackItem("Trilinear");
		listBoxFilter.setSelectedItem(config.getString("Filter"));
		currentLine-=lineOffset;

		//shadows
		labelShadows.registerGraphicComponent(containerName,"labelShadows");
		labelShadows.init(currentLabelStart, currentLine);
		labelShadows.setText(lang.get("Shadows"));

		listBoxShadows.registerGraphicComponent(containerName,"listBoxShadows");
		listBoxShadows.init(currentColumnStart, currentLine, 170);
		for(int i= 0; i<Renderer::sCount; ++i){
			listBoxShadows.pushBackItem(lang.get(Renderer::shadowsToStr(static_cast<Renderer::Shadows>(i))));
		}
		string str= config.getString("Shadows");
		listBoxShadows.setSelectedItemIndex(clamp(Renderer::strToShadows(str), 0, Renderer::sCount-1));
		currentLine-=lineOffset;

		//shadows
		labelShadowTextureSize.registerGraphicComponent(containerName,"labelShadowTextureSize");
		labelShadowTextureSize.init(currentLabelStart, currentLine);
		labelShadowTextureSize.setText(lang.get("ShadowTextureSize"));

		listBoxShadowTextureSize.registerGraphicComponent(containerName,"listBoxShadowTextureSize");
		listBoxShadowTextureSize.init(currentColumnStart, currentLine, 170);
		listBoxShadowTextureSize.pushBackItem("256");
		listBoxShadowTextureSize.pushBackItem("512");
		listBoxShadowTextureSize.pushBackItem("1024");
		listBoxShadowTextureSize.setSelectedItemIndex(1,false);
		listBoxShadowTextureSize.setSelectedItem(intToStr(config.getInt("ShadowTextureSize","512")),false);
		currentLine-=lineOffset;

		//textures 3d
		labelTextures3D.registerGraphicComponent(containerName,"labelTextures3D");
		labelTextures3D.init(currentLabelStart, currentLine);

		checkBoxTextures3D.registerGraphicComponent(containerName,"checkBoxTextures3D");
		checkBoxTextures3D.init(currentColumnStart, currentLine);
		labelTextures3D.setText(lang.get("Textures3D"));
		checkBoxTextures3D.setValue(config.getBool("Textures3D"));
		currentLine-=lineOffset;

		//lights
		labelLights.registerGraphicComponent(containerName,"labelLights");
		labelLights.init(currentLabelStart, currentLine);
		labelLights.setText(lang.get("MaxLights"));

		listBoxLights.registerGraphicComponent(containerName,"listBoxLights");
		listBoxLights.init(currentColumnStart, currentLine, 80);
		for(int i= 1; i<=8; ++i){
			listBoxLights.pushBackItem(intToStr(i));
		}
		listBoxLights.setSelectedItemIndex(clamp(config.getInt("MaxLights")-1, 0, 7));
		currentLine-=lineOffset;

		//unit particles
		labelUnitParticles.registerGraphicComponent(containerName,"labelUnitParticles");
		labelUnitParticles.init(currentLabelStart,currentLine);
		labelUnitParticles.setText(lang.get("ShowUnitParticles"));

		checkBoxUnitParticles.registerGraphicComponent(containerName,"checkBoxUnitParticles");
		checkBoxUnitParticles.init(currentColumnStart,currentLine);
		checkBoxUnitParticles.setValue(config.getBool("UnitParticles","true"));
		currentLine-=lineOffset;

		//tileset particles
		labelTilesetParticles.registerGraphicComponent(containerName,"labelTilesetParticles");
		labelTilesetParticles.init(currentLabelStart,currentLine);
		labelTilesetParticles.setText(lang.get("ShowTilesetParticles"));

		checkBoxTilesetParticles.registerGraphicComponent(containerName,"checkBoxTilesetParticles");
		checkBoxTilesetParticles.init(currentColumnStart,currentLine);
		checkBoxTilesetParticles.setValue(config.getBool("TilesetParticles","true"));
		currentLine-=lineOffset;

		//unit particles
		labelMapPreview.registerGraphicComponent(containerName,"labelMapPreview");
		labelMapPreview.init(currentLabelStart,currentLine);
		labelMapPreview.setText(lang.get("ShowMapPreview"));

		checkBoxMapPreview.registerGraphicComponent(containerName,"checkBoxMapPreview");
		checkBoxMapPreview.init(currentColumnStart,currentLine);
		checkBoxMapPreview.setValue(config.getBool("MapPreview","true"));
		currentLine-=lineOffset;

		// Texture Compression flag
		labelEnableTextureCompression.registerGraphicComponent(containerName,"labelEnableTextureCompression");
		labelEnableTextureCompression.init(currentLabelStart ,currentLine);
		labelEnableTextureCompression.setText(lang.get("EnableTextureCompression"));

		checkBoxEnableTextureCompression.registerGraphicComponent(containerName,"checkBoxEnableTextureCompression");
		checkBoxEnableTextureCompression.init(currentColumnStart ,currentLine );
		checkBoxEnableTextureCompression.setValue(config.getBool("EnableTextureCompression","false"));
		currentLine-=lineOffset;

		labelVisibleHud.registerGraphicComponent(containerName,"lavelVisibleHud");
		labelVisibleHud.init(currentLabelStart ,currentLine);
		labelVisibleHud.setText(lang.get("VisibleHUD"));

		checkBoxVisibleHud.registerGraphicComponent(containerName,"checkBoxVisibleHud");
		checkBoxVisibleHud.init(currentColumnStart ,currentLine );
		checkBoxVisibleHud.setValue(config.getBool("VisibleHud","true"));
		currentLine-=lineOffset;

		labelRainEffect.registerGraphicComponent(containerName,"labelRainEffect");
		labelRainEffect.init(currentLabelStart ,currentLine);
		labelRainEffect.setText(lang.get("RainEffectMenuGame"));

		checkBoxRainEffectMenu.registerGraphicComponent(containerName,"checkBoxRainEffectMenu");
		checkBoxRainEffectMenu.init(currentColumnStart ,currentLine );
		checkBoxRainEffectMenu.setValue(config.getBool("RainEffectMenu","true"));

		labelRainEffectSeparator.registerGraphicComponent(containerName,"labelRainEffect");
		labelRainEffectSeparator.init(currentColumnStart+30 ,currentLine);
		labelRainEffectSeparator.setText("/");

		checkBoxRainEffect.registerGraphicComponent(containerName,"checkBoxRainEffect");
		checkBoxRainEffect.init(currentColumnStart+42 ,currentLine );
		checkBoxRainEffect.setValue(config.getBool("RainEffect","true"));
		currentLine-=lineOffset;

		labelVideos.registerGraphicComponent(containerName,"labelVideos");
		labelVideos.init(currentLabelStart ,currentLine);
		labelVideos.setText(lang.get("EnableVideos"));

		checkBoxVideos.registerGraphicComponent(containerName,"checkBoxVideos");
		checkBoxVideos.init(currentColumnStart ,currentLine );
		checkBoxVideos.setValue(config.getBool("EnableVideos","true"));

		// end

		//////////////////////////////////////////////////////////////////
		///////// RIGHT SIDE
		//////////////////////////////////////////////////////////////////

		currentLine=700; // reset line pos
		currentLabelStart=rightLabelStart; // set to right side
		currentColumnStart=rightColumnStart; // set to right side


		//currentLine-=lineOffset;
		labelMiscSection.registerGraphicComponent(containerName,"labelMiscSection");
		labelMiscSection.init(currentLabelStart+captionOffset, currentLine);
		labelMiscSection.setFont(CoreData::getInstance().getMenuFontVeryBig());
		labelMiscSection.setFont3D(CoreData::getInstance().getMenuFontVeryBig3D());
		labelMiscSection.setText(lang.get("Misc"));
		currentLine-=lineOffset;

		//lang
		labelLang.registerGraphicComponent(containerName,"labelLang");
		labelLang.init(currentLabelStart, currentLine);
		labelLang.setText(lang.get("Language"));

		listBoxLang.registerGraphicComponent(containerName,"listBoxLang");
		listBoxLang.init(currentColumnStart, currentLine, 260);
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
		currentLine-=lineOffset;
		currentLine-=lineOffset;

		labelNetworkSettings.registerGraphicComponent(containerName,"labelNetworkSettingsSection");
		labelNetworkSettings.init(currentLabelStart+captionOffset, currentLine);
		labelNetworkSettings.setFont(CoreData::getInstance().getMenuFontVeryBig());
		labelNetworkSettings.setFont3D(CoreData::getInstance().getMenuFontVeryBig3D());
		labelNetworkSettings.setText(lang.get("Network"));
		currentLine-=lineOffset;
		// server port
		labelServerPortLabel.registerGraphicComponent(containerName,"labelServerPortLabel");
		labelServerPortLabel.init(currentLabelStart,currentLine);
		labelServerPortLabel.setText(lang.get("ServerPort"));
		labelServerPort.init(currentColumnStart,currentLine);
		string port=intToStr(config.getInt("PortServer"));
		if(port != intToStr(GameConstants::serverPort)){
			port=port +" ("+lang.get("NonStandardPort")+"!!)";
		}
		else{
			port=port +" ("+lang.get("StandardPort")+")";
		}

		labelServerPort.setText(port);

		// external server port
		currentLine-=lineOffset;

		labelPublishServerExternalPort.registerGraphicComponent(containerName,"labelPublishServerExternalPort");
		labelPublishServerExternalPort.init(currentLabelStart, currentLine, 150);
		labelPublishServerExternalPort.setText(lang.get("PublishServerExternalPort"));

		listBoxServerPort.registerGraphicComponent(containerName,"listBoxPublishServerExternalPort");
		listBoxServerPort.init(currentColumnStart, currentLine, 170);

		string portListString = config.getString("PortList",intToStr(GameConstants::serverPort).c_str());
		std::vector<std::string> portList;
		Tokenize(portListString,portList,",");

		string currentPort=config.getString("PortExternal", intToStr(GameConstants::serverPort).c_str());
		int masterServerExternalPortSelectionIndex=0;
		for(int idx = 0; idx < portList.size(); idx++) {
			if(portList[idx] != "" && IsNumeric(portList[idx].c_str(),false)) {
				listBoxServerPort.pushBackItem(portList[idx]);
				if(currentPort==portList[idx])
				{
					masterServerExternalPortSelectionIndex=idx;
				}
			}
		}
		listBoxServerPort.setSelectedItemIndex(masterServerExternalPortSelectionIndex);

		currentLine-=lineOffset;
		// FTP Config - start
		labelEnableFTP.registerGraphicComponent(containerName,"labelEnableFTP");
		labelEnableFTP.init(currentLabelStart ,currentLine);
		labelEnableFTP.setText(lang.get("EnableFTP"));

		checkBoxEnableFTP.registerGraphicComponent(containerName,"checkBoxEnableFTP");
		checkBoxEnableFTP.init(currentColumnStart ,currentLine );
		checkBoxEnableFTP.setValue(config.getBool("EnableFTPXfer","true"));
		currentLine-=lineOffset;
		labelEnableFTPServer.registerGraphicComponent(containerName,"labelEnableFTPServer");
		labelEnableFTPServer.init(currentLabelStart ,currentLine);
		labelEnableFTPServer.setText(lang.get("EnableFTPServer"));

		checkBoxEnableFTPServer.registerGraphicComponent(containerName,"checkBoxEnableFTPServer");
		checkBoxEnableFTPServer.init(currentColumnStart ,currentLine );
		checkBoxEnableFTPServer.setValue(config.getBool("EnableFTPServer","true"));
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

		char szBuf[1024]="";
		sprintf(szBuf,"%d - %d",FTPPort + 1, FTPPort + GameConstants::maxPlayers);

		labelFTPServerDataPorts.registerGraphicComponent(containerName,"labelFTPServerDataPorts");
		labelFTPServerDataPorts.init(currentColumnStart,currentLine );
		labelFTPServerDataPorts.setText(szBuf);
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
		buttonOk.setText(lang.get("Ok"));
		buttonAbort.setText(lang.get("Abort"));

		buttonAbort.registerGraphicComponent(containerName,"buttonAbort");
		buttonAbort.init(buttonStartPos+110, buttonRowPos, 100);
		buttonAutoConfig.setText(lang.get("AutoConfig"));

		buttonAutoConfig.registerGraphicComponent(containerName,"buttonAutoConfig");
		buttonAutoConfig.init(buttonStartPos+250, buttonRowPos, 125);

		buttonVideoInfo.setText(lang.get("VideoInfo"));
		buttonVideoInfo.registerGraphicComponent(containerName,"buttonVideoInfo");
		buttonVideoInfo.init(buttonStartPos+385, buttonRowPos, 125); // was 620

		buttonKeyboardSetup.setText(lang.get("Keyboardsetup"));
		buttonKeyboardSetup.registerGraphicComponent(containerName,"buttonKeyboardSetup");
		buttonKeyboardSetup.init(buttonStartPos+520, buttonRowPos, 145);

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
	mainMessageBox.init(lang.get("Ok"));

	labelAudioSection.setFont(CoreData::getInstance().getMenuFontVeryBig());
	labelAudioSection.setFont3D(CoreData::getInstance().getMenuFontVeryBig3D());
	labelAudioSection.setText(lang.get("Audio"));

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

	labelVideoSection.setFont(CoreData::getInstance().getMenuFontVeryBig());
	labelVideoSection.setFont3D(CoreData::getInstance().getMenuFontVeryBig3D());
	labelVideoSection.setText(lang.get("Video"));

	labelScreenModes.setText(lang.get("Resolution"));

	labelFullscreenWindowed.setText(lang.get("Windowed"));
	labelFilter.setText(lang.get("Filter"));

	listboxData.clear();
	listboxData.push_back("Bilinear");
	listboxData.push_back("Trilinear");
	listBoxFilter.setItems(listboxData);

	listboxData.clear();
	for (float f=0.0;f<2.1f;f=f+0.1f) {
		listboxData.push_back(floatToStr(f));
	}
	listBoxGammaCorrection.setItems(listboxData);


	labelShadows.setText(lang.get("Shadows"));
	labelShadowTextureSize.setText(lang.get("ShadowTextureSize"));

	listboxData.clear();
	for(int i= 0; i<Renderer::sCount; ++i){
		listboxData.push_back(lang.get(Renderer::shadowsToStr(static_cast<Renderer::Shadows>(i))));
	}
	listBoxShadows.setItems(listboxData);

	labelTextures3D.setText(lang.get("Textures3D"));

	labelLights.setText(lang.get("MaxLights"));

	labelUnitParticles.setText(lang.get("ShowUnitParticles"));

	labelTilesetParticles.setText(lang.get("ShowTilesetParticles"));

	labelMapPreview.setText(lang.get("ShowMapPreview"));

	labelEnableTextureCompression.setText(lang.get("EnableTextureCompression"));

	labelVisibleHud.setText(lang.get("VisibleHUD"));

	labelRainEffect.setText(lang.get("RainEffect"));

	labelVideos.setText(lang.get("EnableVideos"));

	labelMiscSection.setFont(CoreData::getInstance().getMenuFontVeryBig());
	labelMiscSection.setFont3D(CoreData::getInstance().getMenuFontVeryBig3D());
	labelMiscSection.setText(lang.get("Misc"));

	labelLang.setText(lang.get("Language"));

	labelPlayerNameLabel.setText(lang.get("Playername"));

	labelPlayerName.setFont(CoreData::getInstance().getMenuFontBig());
	labelPlayerName.setFont3D(CoreData::getInstance().getMenuFontBig3D());

	labelFontSizeAdjustment.setText(lang.get("FontSizeAdjustment"));

	labelScreenShotType.setText(lang.get("ScreenShotFileType"));

	labelDisableScreenshotConsoleText.setText(lang.get("ScreenShotConsoleText"));

	labelMouseMoveScrollsWorld.setText(lang.get("MouseScrollsWorld"));

	labelNetworkSettings.setFont(CoreData::getInstance().getMenuFontVeryBig());
	labelNetworkSettings.setFont3D(CoreData::getInstance().getMenuFontVeryBig3D());
	labelNetworkSettings.setText(lang.get("Network"));

	labelServerPortLabel.setText(lang.get("ServerPort"));
	Config &config= Config::getInstance();
	string port = intToStr(config.getInt("PortServer"));
	if(port != intToStr(GameConstants::serverPort).c_str()) {
		port = port +" ("+lang.get("NonStandardPort")+"!!)";
	}
	else{
		port = port +" ("+lang.get("StandardPort")+")";
	}

	labelServerPort.setText(port);

	labelPublishServerExternalPort.setText(lang.get("PublishServerExternalPort"));

	labelEnableFTP.setText(lang.get("EnableFTP"));

	labelEnableFTPServer.setText(lang.get("EnableFTPServer"));

	labelFTPServerPortLabel.setText(lang.get("FTPServerPort"));

	labelFTPServerDataPortsLabel.setText(lang.get("FTPServerDataPort"));

	labelEnableFTPServerInternetTilesetXfer.setText(lang.get("EnableFTPServerInternetTilesetXfer"));

	labelEnableFTPServerInternetTechtreeXfer.setText(lang.get("EnableFTPServerInternetTechtreeXfer"));

	labelEnablePrivacy.setText(lang.get("PrivacyPlease"));

	buttonOk.setText(lang.get("Ok"));
	buttonAbort.setText(lang.get("Abort"));

	buttonAutoConfig.setText(lang.get("AutoConfig"));

	buttonVideoInfo.setText(lang.get("VideoInfo"));

	buttonKeyboardSetup.setText(lang.get("Keyboardsetup"));

	GraphicComponent::reloadFontsForRegisterGraphicComponents(containerName);
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
		int button= 0;
		if(mainMessageBox.mouseClick(x, y, button))
		{
			soundRenderer.playFx(coreData.getClickSoundA());
			if(button==0)
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
		return;
    }
	else if(buttonAbort.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundA());

		// reset the gamma to former value
		string currentGammaCorrection=config.getString("GammaValue","1.0");
		string selectedGammaCorrection=listFontSizeAdjustment.getSelectedItem();
		if(currentGammaCorrection!=selectedGammaCorrection){
			float gammaValue=strToFloat(currentGammaCorrection);
			if(gammaValue==0.0f) gammaValue=1.0f;
			if(gammaValue!=0.0){
				program->getWindow()->setGamma(gammaValue);
				SDL_SetGamma(gammaValue, gammaValue, gammaValue);
			}
		}

		mainMenu->setState(new MenuStateRoot(program, mainMenu));
		return;
    }
	else if(buttonAutoConfig.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundA());
		Renderer::getInstance().autoConfig();
		//saveConfig();
		mainMenu->setState(new MenuStateOptions(program, mainMenu));
		return;
	}
	else if(buttonVideoInfo.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundA());
		mainMenu->setState(new MenuStateGraphicInfo(program, mainMenu));
		return;
	}
	else if(buttonKeyboardSetup.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundA());
		mainMenu->setState(new MenuStateKeysetup(program, mainMenu)); // open keyboard shortcuts setup screen
		//showMessageBox("Not implemented yet", "Keyboard setup", false);
		return;
	}
	else if(labelPlayerName.mouseClick(x, y) && ( activeInputLabel != &labelPlayerName )){
			setActiveInputLable(&labelPlayerName);
	}
	else
	{
		listBoxLang.mouseClick(x, y);
		listBoxShadows.mouseClick(x, y);
		listBoxShadowTextureSize.mouseClick(x, y);
		listBoxFilter.mouseClick(x, y);
		if(listBoxGammaCorrection.mouseClick(x, y)){
			float gammaValue=strToFloat(listBoxGammaCorrection.getSelectedItem());
			if(gammaValue!=0.0){
				program->getWindow()->setGamma(gammaValue);
				SDL_SetGamma(gammaValue, gammaValue, gammaValue);
			}
		}
		checkBoxTextures3D.mouseClick(x, y);
		checkBoxUnitParticles.mouseClick(x, y);
		checkBoxTilesetParticles.mouseClick(x, y);
		checkBoxMapPreview.mouseClick(x, y);
		listBoxLights.mouseClick(x, y);
		listBoxSoundFactory.mouseClick(x, y);
		listBoxVolumeFx.mouseClick(x, y);
		listBoxVolumeAmbient.mouseClick(x, y);
		listBoxVolumeMusic.mouseClick(x, y);
		listBoxScreenModes.mouseClick(x, y);
		listFontSizeAdjustment.mouseClick(x, y);
		checkBoxFullscreenWindowed.mouseClick(x, y);
		if(listBoxServerPort.mouseClick(x, y)){
			int selectedPort=strToInt(listBoxServerPort.getSelectedItem());
			if(selectedPort<10000){
				selectedPort=GameConstants::serverPort;
			}
			// use the following ports for ftp
			char szBuf[1024]="";
			sprintf(szBuf,"%d - %d",selectedPort + 1, selectedPort + GameConstants::maxPlayers);
			labelFTPServerPort.setText(intToStr(selectedPort));
			labelFTPServerDataPorts.setText(szBuf);
		}

        checkBoxEnableFTP.mouseClick(x, y);
        checkBoxEnableFTPServer.mouseClick(x, y);

        checkBoxEnableFTPServerInternetTilesetXfer.mouseClick(x, y);
        checkBoxEnableFTPServerInternetTechtreeXfer.mouseClick(x, y);

        checkBoxEnablePrivacy.mouseClick(x, y);

        checkBoxEnableTextureCompression.mouseClick(x, y);

        listBoxScreenShotType.mouseClick(x, y);

        checkBoxDisableScreenshotConsoleText.mouseClick(x, y);
        checkBoxMouseMoveScrollsWorld.mouseClick(x, y);
        checkBoxVisibleHud.mouseClick(x, y);
        checkBoxRainEffect.mouseClick(x,y);
        checkBoxRainEffectMenu.mouseClick(x,y);

		checkBoxVideos.mouseClick(x,y);
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
	listBoxGammaCorrection.mouseMove(x, y);
	listBoxShadows.mouseMove(x, y);
	checkBoxTextures3D.mouseMove(x, y);
	checkBoxUnitParticles.mouseMove(x, y);
	checkBoxTilesetParticles.mouseMove(x, y);
	checkBoxMapPreview.mouseMove(x, y);
	listBoxLights.mouseMove(x, y);
	listBoxScreenModes.mouseMove(x, y);
	checkBoxFullscreenWindowed.mouseMove(x, y);
	listFontSizeAdjustment.mouseMove(x, y);
	listBoxServerPort.mouseMove(x, y);
	checkBoxEnableFTP.mouseMove(x, y);
	checkBoxEnableFTPServer.mouseMove(x, y);
    checkBoxEnableFTPServerInternetTilesetXfer.mouseMove(x, y);
    checkBoxEnableFTPServerInternetTechtreeXfer.mouseMove(x, y);
	checkBoxEnablePrivacy.mouseMove(x, y);
	checkBoxEnableTextureCompression.mouseMove(x, y);
	listBoxScreenShotType.mouseMove(x, y);
	checkBoxDisableScreenshotConsoleText.mouseMove(x, y);
	checkBoxMouseMoveScrollsWorld.mouseMove(x, y);
	checkBoxVisibleHud.mouseMove(x, y);
	checkBoxRainEffect.mouseMove(x, y);
	checkBoxRainEffectMenu.mouseMove(x, y);

	checkBoxVideos.mouseMove(x, y);
}

bool MenuStateOptions::isInSpecialKeyCaptureEvent() {
	return (activeInputLabel != NULL);
}

void MenuStateOptions::keyDown(SDL_KeyboardEvent key) {
	if(activeInputLabel != NULL) {
		/*
		string text= activeInputLabel->getText();
		if(isKeyPressed(SDLK_BACKSPACE,key) == true && text.length() > 0) {
			size_t found = text.find_last_of("_");
			if (found == string::npos) {
				text.erase(text.end() - 1);
			}
			else {
				if(text.size() > 1) {
					text.erase(text.end() - 2);
				}
			}

			activeInputLabel->setText(text);
		}
		*/
		keyDownEditLabel(key, &activeInputLabel);
	}
}

void MenuStateOptions::keyPress(SDL_KeyboardEvent c) {
	if(activeInputLabel != NULL) {
	    //printf("[%d]\n",c); fflush(stdout);
		if(&labelPlayerName == activeInputLabel) {
			/*
			SDLKey key = extractKeyPressed(c);
			if((key>='0' && key<='9')||(key>='a' && key<='z')||(key>='A' && key<='Z')||
				 (key>=(192-256) && key<=(255-256))||
				 (key=='-')||(key=='_')||(key=='(')||(key==')')){
			//if(isAllowedInputTextKey(key)) {
				const int maxTextSize= activeInputLabel->getMaxEditWidth();
				if(activeInputLabel->getText().size()<maxTextSize){
					string text= activeInputLabel->getText();
					//text.insert(text.end()-1, key);
					char szCharText[20]="";
					sprintf(szCharText,"%c",key);
					char *utfStr = String::ConvertToUTF8(&szCharText[0]);
					if(text.size() > 0) {
						text.insert(text.end() -1, utfStr[0]);
					}
					else {
						text = utfStr[0];
					}
					delete [] utfStr;

					activeInputLabel->setText(text);
				}
			}
			*/
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
		renderer.renderCheckBox(&checkBoxTilesetParticles);
		renderer.renderCheckBox(&checkBoxMapPreview);
		renderer.renderListBox(&listBoxLights);
		renderer.renderListBox(&listBoxFilter);
		renderer.renderListBox(&listBoxGammaCorrection);
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
		renderer.renderLabel(&labelTilesetParticles);
		renderer.renderLabel(&labelMapPreview);
		renderer.renderLabel(&labelLights);
		renderer.renderLabel(&labelFilter);
		renderer.renderLabel(&labelGammaCorrection);
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
		renderer.renderListBox(&listBoxServerPort);
		renderer.renderLabel(&labelNetworkSettings);


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

        renderer.renderLabel(&labelEnableTextureCompression);
        renderer.renderCheckBox(&checkBoxEnableTextureCompression);

        renderer.renderLabel(&labelScreenShotType);
        renderer.renderListBox(&listBoxScreenShotType);

        renderer.renderLabel(&labelDisableScreenshotConsoleText);
        renderer.renderCheckBox(&checkBoxDisableScreenshotConsoleText);

        renderer.renderLabel(&labelMouseMoveScrollsWorld);
        renderer.renderCheckBox(&checkBoxMouseMoveScrollsWorld);

        renderer.renderLabel(&labelVisibleHud);
        renderer.renderCheckBox(&checkBoxVisibleHud);

        renderer.renderLabel(&labelRainEffect);
        renderer.renderCheckBox(&checkBoxRainEffect);
        renderer.renderLabel(&labelRainEffectSeparator);
        renderer.renderCheckBox(&checkBoxRainEffectMenu);

		renderer.renderLabel(&labelShadowTextureSize);
		renderer.renderListBox(&listBoxShadowTextureSize);

        renderer.renderLabel(&labelVideos);
        renderer.renderCheckBox(&checkBoxVideos);
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

	int index= listBoxShadows.getSelectedItemIndex();
	config.setString("Shadows", Renderer::shadowsToStr(static_cast<Renderer::Shadows>(index)));

	string texSizeString= listBoxShadowTextureSize.getSelectedItem();
	config.setInt("ShadowTextureSize",strToInt(texSizeString) );

	config.setBool("Windowed", checkBoxFullscreenWindowed.getValue());
	config.setString("Filter", listBoxFilter.getSelectedItem());
	config.setFloat("GammaValue", strToFloat(listBoxGammaCorrection.getSelectedItem()));
	config.setBool("Textures3D", checkBoxTextures3D.getValue());
	config.setBool("UnitParticles", (checkBoxUnitParticles.getValue()));
	config.setBool("TilesetParticles", (checkBoxTilesetParticles.getValue()));
	config.setBool("MapPreview", checkBoxMapPreview.getValue());
	config.setInt("MaxLights", listBoxLights.getSelectedItemIndex()+1);
	config.setString("FactorySound", listBoxSoundFactory.getSelectedItem());
	config.setString("SoundVolumeFx", listBoxVolumeFx.getSelectedItem());
	config.setString("SoundVolumeAmbient", listBoxVolumeAmbient.getSelectedItem());
	config.setString("FontSizeAdjustment", listFontSizeAdjustment.getSelectedItem());
	CoreData::getInstance().getMenuMusic()->setVolume(strToInt(listBoxVolumeMusic.getSelectedItem())/100.f);
	config.setString("SoundVolumeMusic", listBoxVolumeMusic.getSelectedItem());
	config.setString("PortExternal", listBoxServerPort.getSelectedItem());
	config.setInt("FTPServerPort",config.getInt("PortExternal")+1);
    config.setBool("EnableFTPXfer", checkBoxEnableFTP.getValue());
    config.setBool("EnableFTPServer", checkBoxEnableFTPServer.getValue());

    config.setBool("EnableFTPServerInternetTilesetXfer", checkBoxEnableFTPServerInternetTilesetXfer.getValue());
    config.setBool("EnableFTPServerInternetTechtreeXfer", checkBoxEnableFTPServerInternetTechtreeXfer.getValue());

    config.setBool("PrivacyPlease", checkBoxEnablePrivacy.getValue());

    config.setBool("EnableTextureCompression", checkBoxEnableTextureCompression.getValue());

    config.setString("ScreenShotFileType", listBoxScreenShotType.getSelectedItem());

    config.setBool("DisableScreenshotConsoleText", !checkBoxDisableScreenshotConsoleText.getValue());
    config.setBool("MouseMoveScrollsWorld", checkBoxMouseMoveScrollsWorld.getValue());
    config.setBool("VisibleHud", checkBoxVisibleHud.getValue());
    config.setBool("RainEffect", checkBoxRainEffect.getValue());
    config.setBool("RainEffectMenu", checkBoxRainEffectMenu.getValue());

    config.setBool("EnableVideos", checkBoxVideos.getValue());

	string currentResolution=config.getString("ScreenWidth")+"x"+config.getString("ScreenHeight");
	string selectedResolution=listBoxScreenModes.getSelectedItem();
	if(currentResolution!=selectedResolution){
		for(vector<ModeInfo>::const_iterator it= modeInfos.begin(); it!=modeInfos.end(); ++it){
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
    program->stopSoundSystem();
    soundRenderer.init(program->getWindow());
    soundRenderer.loadConfig();
    soundRenderer.setMusicVolume(CoreData::getInstance().getMenuMusic());
    program->startSoundSystem();

    if(CoreData::getInstance().hasMainMenuVideoFilename() == false) {
    	soundRenderer.playMusic(CoreData::getInstance().getMenuMusic());
    }

	Renderer::getInstance().loadConfig();
}

void MenuStateOptions::setActiveInputLable(GraphicLabel *newLable) {
	/*
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
		activeInputLabel->setEditModeEnabled(false);
	}
	activeInputLabel=newLable;
	if(activeInputLabel != NULL) {
		activeInputLabel->setEditModeEnabled(true);
	}
	*/

	MenuState::setActiveInputLabel(newLable,&activeInputLabel);
}

}}//end namespace
