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

#include "menu_state_options_graphics.h"

#include "renderer.h"
#include "game.h"
#include "program.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "config.h"
#include "menu_state_root.h"
#include "menu_state_options.h"
#include "util.h"
#include "menu_state_graphic_info.h"
#include "menu_state_keysetup.h"
#include "menu_state_options_graphics.h"
#include "menu_state_options_sound.h"
#include "menu_state_options_network.h"
#include "string_utils.h"
#include "metrics.h"
#include "leak_dumper.h"

using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class MenuStateOptions
// =====================================================
MenuStateOptionsGraphics::MenuStateOptionsGraphics(Program *program, MainMenu *mainMenu, ProgramState **parentUI):
	MenuState(program, mainMenu, "config")
{
	try {
		containerName = "Options";
		this->parentUI=parentUI;
		Lang &lang= Lang::getInstance();
		Config &config= Config::getInstance();
		this->console.setOnlyChatMessagesInStoredLines(false);
		screenModeChangedTimer= time(NULL); // just init
		//modeinfos=list<ModeInfo> ();
		::Shared::PlatformCommon::getFullscreenVideoModes(&modeInfos,!config.getBool("Windowed"));

		int leftLabelStart=50;
		int leftColumnStart=leftLabelStart+280;
		//int rightLabelStart=450;
		//int rightColumnStart=rightLabelStart+280;
		int buttonRowPos=50;
		int buttonStartPos=170;
		//int captionOffset=75;
		//int currentLabelStart=leftLabelStart;
		//int currentColumnStart=leftColumnStart;
		//int currentLine=700;
		int lineOffset=30;
		int tabButtonWidth=200;
		int tabButtonHeight=30;

		mainMessageBox.registerGraphicComponent(containerName,"mainMessageBox");
		mainMessageBox.init(lang.getString("Ok"));
		mainMessageBox.setEnabled(false);
		mainMessageBoxState=0;

		buttonAudioSection.registerGraphicComponent(containerName,"buttonAudioSection");
		buttonAudioSection.init(0, 720,tabButtonWidth,tabButtonHeight);
		buttonAudioSection.setFont(CoreData::getInstance().getMenuFontVeryBig());
		buttonAudioSection.setFont3D(CoreData::getInstance().getMenuFontVeryBig3D());
		buttonAudioSection.setText(lang.getString("Audio"));
		// Video Section
		buttonVideoSection.registerGraphicComponent(containerName,"labelVideoSection");
		buttonVideoSection.init(200, 700,tabButtonWidth,tabButtonHeight+20);
		buttonVideoSection.setFont(CoreData::getInstance().getMenuFontVeryBig());
		buttonVideoSection.setFont3D(CoreData::getInstance().getMenuFontVeryBig3D());
		buttonVideoSection.setText(lang.getString("Video"));
		//currentLine-=lineOffset;
		//MiscSection
		buttonMiscSection.registerGraphicComponent(containerName,"labelMiscSection");
		buttonMiscSection.init(400, 720,tabButtonWidth,tabButtonHeight);
		buttonMiscSection.setFont(CoreData::getInstance().getMenuFontVeryBig());
		buttonMiscSection.setFont3D(CoreData::getInstance().getMenuFontVeryBig3D());
		buttonMiscSection.setText(lang.getString("Misc"));
		//NetworkSettings
		buttonNetworkSettings.registerGraphicComponent(containerName,"labelNetworkSettingsSection");
		buttonNetworkSettings.init(600, 720,tabButtonWidth,tabButtonHeight);
		buttonNetworkSettings.setFont(CoreData::getInstance().getMenuFontVeryBig());
		buttonNetworkSettings.setFont3D(CoreData::getInstance().getMenuFontVeryBig3D());
		buttonNetworkSettings.setText(lang.getString("Network"));

		//KeyboardSetup
		buttonKeyboardSetup.registerGraphicComponent(containerName,"buttonKeyboardSetup");
		buttonKeyboardSetup.init(800, 720,tabButtonWidth,tabButtonHeight);
		buttonKeyboardSetup.setFont(CoreData::getInstance().getMenuFontVeryBig());
		buttonKeyboardSetup.setFont3D(CoreData::getInstance().getMenuFontVeryBig3D());
		buttonKeyboardSetup.setText(lang.getString("Keyboardsetup"));

		int currentLine=650; // reset line pos
		int currentLabelStart=leftLabelStart; // set to right side
		int currentColumnStart=leftColumnStart; // set to right side

		//resolution
		labelScreenModes.registerGraphicComponent(containerName,"labelScreenModes");
		labelScreenModes.init(currentLabelStart, currentLine);
		labelScreenModes.setText(lang.getString("Resolution"));

		listBoxScreenModes.registerGraphicComponent(containerName,"listBoxScreenModes");
		listBoxScreenModes.init(currentColumnStart, currentLine, 200);

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
		labelFullscreenWindowed.setText(lang.getString("Windowed"));
		checkBoxFullscreenWindowed.setValue(config.getBool("Windowed"));
		currentLine-=lineOffset;

		//gammaCorrection
		labelGammaCorrection.registerGraphicComponent(containerName,"labelGammaCorrection");
		labelGammaCorrection.init(currentLabelStart, currentLine);
		labelGammaCorrection.setText(lang.getString("GammaCorrection"));

		listBoxGammaCorrection.registerGraphicComponent(containerName,"listBoxGammaCorrection");
		listBoxGammaCorrection.init(currentColumnStart, currentLine, 200);
		for (float f=0.5;f<3.0f;f=f+0.1f) {
			listBoxGammaCorrection.pushBackItem(floatToStr(f));
		}
		float gammaValue=config.getFloat("GammaValue","1.0");
		if(gammaValue==0.0f) gammaValue=1.0f;
		listBoxGammaCorrection.setSelectedItem(floatToStr(gammaValue),false);

		currentLine-=lineOffset;

		//filter
		labelFilter.registerGraphicComponent(containerName,"labelFilter");
		labelFilter.init(currentLabelStart, currentLine);
		labelFilter.setText(lang.getString("Filter"));

		listBoxFilter.registerGraphicComponent(containerName,"listBoxFilter");
		listBoxFilter.init(currentColumnStart, currentLine, 200);
		listBoxFilter.pushBackItem("Bilinear");
		listBoxFilter.pushBackItem("Trilinear");
		listBoxFilter.setSelectedItem(config.getString("Filter"));
		currentLine-=lineOffset;

		//selectionType
		labelSelectionType.registerGraphicComponent(containerName,"labelSelectionType");
		labelSelectionType.init(currentLabelStart, currentLine);
		labelSelectionType.setText(lang.getString("SelectionType"));

		listBoxSelectionType.registerGraphicComponent(containerName,"listBoxSelectionType");
		listBoxSelectionType.init(currentColumnStart, currentLine, 200);
		listBoxSelectionType.pushBackItem("SelectBuffer (nvidia)");
		listBoxSelectionType.pushBackItem("ColorPicking (default)");
		listBoxSelectionType.pushBackItem("FrustumPicking (bad)");

		const string selectionType=toLower(config.getString("SelectionType",Config::colorPicking));
		if( selectionType==Config::colorPicking)
			listBoxSelectionType.setSelectedItemIndex(1);
		else if ( selectionType==Config::frustumPicking )
			listBoxSelectionType.setSelectedItemIndex(2);
		else
			listBoxSelectionType.setSelectedItemIndex(0);
		currentLine-=lineOffset;

		//shadows
		labelShadows.registerGraphicComponent(containerName,"labelShadows");
		labelShadows.init(currentLabelStart, currentLine);
		labelShadows.setText(lang.getString("Shadows"));

		listBoxShadows.registerGraphicComponent(containerName,"listBoxShadows");
		listBoxShadows.init(currentColumnStart, currentLine, 200);
		for(int i= 0; i<Renderer::sCount; ++i){
			listBoxShadows.pushBackItem(lang.getString(Renderer::shadowsToStr(static_cast<Renderer::Shadows>(i))));
		}
		string str= config.getString("Shadows");
		listBoxShadows.setSelectedItemIndex(clamp(Renderer::strToShadows(str), 0, Renderer::sCount-1));
		currentLine-=lineOffset;

		//shadows
		labelShadowTextureSize.registerGraphicComponent(containerName,"labelShadowTextureSize");
		labelShadowTextureSize.init(currentLabelStart, currentLine);
		labelShadowTextureSize.setText(lang.getString("ShadowTextureSize"));

		listBoxShadowTextureSize.registerGraphicComponent(containerName,"listBoxShadowTextureSize");
		listBoxShadowTextureSize.init(currentColumnStart, currentLine, 200);
		listBoxShadowTextureSize.pushBackItem("256");
		listBoxShadowTextureSize.pushBackItem("512");
		listBoxShadowTextureSize.pushBackItem("1024");
		listBoxShadowTextureSize.setSelectedItemIndex(1,false);
		listBoxShadowTextureSize.setSelectedItem(intToStr(config.getInt("ShadowTextureSize","512")),false);
		currentLine-=lineOffset;

		//shadows
		labelShadowIntensity.registerGraphicComponent(containerName,"labelShadowIntensity");
		labelShadowIntensity.init(currentLabelStart, currentLine);
		labelShadowIntensity.setText(lang.getString("ShadowIntensity"));

		listBoxShadowIntensity.registerGraphicComponent(containerName,"listBoxShadowIntensity");
		listBoxShadowIntensity.init(currentColumnStart, currentLine, 200);
		for (float f=0.5f;f<3.0f;f=f+0.1f) {
			listBoxShadowIntensity.pushBackItem(floatToStr(f));
		}
		float shadowIntensity=config.getFloat("ShadowIntensity","1.0");
		if(shadowIntensity<=0.0f) shadowIntensity=1.0f;
		listBoxShadowIntensity.setSelectedItem(floatToStr(shadowIntensity),false);

		currentLine-=lineOffset;

		//textures 3d
		labelTextures3D.registerGraphicComponent(containerName,"labelTextures3D");
		labelTextures3D.init(currentLabelStart, currentLine);

		checkBoxTextures3D.registerGraphicComponent(containerName,"checkBoxTextures3D");
		checkBoxTextures3D.init(currentColumnStart, currentLine);
		labelTextures3D.setText(lang.getString("Textures3D"));
		checkBoxTextures3D.setValue(config.getBool("Textures3D"));
		currentLine-=lineOffset;

		//lights
		labelLights.registerGraphicComponent(containerName,"labelLights");
		labelLights.init(currentLabelStart, currentLine);
		labelLights.setText(lang.getString("MaxLights"));

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
		labelUnitParticles.setText(lang.getString("ShowUnitParticles"));

		checkBoxUnitParticles.registerGraphicComponent(containerName,"checkBoxUnitParticles");
		checkBoxUnitParticles.init(currentColumnStart,currentLine);
		checkBoxUnitParticles.setValue(config.getBool("UnitParticles","true"));
		currentLine-=lineOffset;

		//tileset particles
		labelTilesetParticles.registerGraphicComponent(containerName,"labelTilesetParticles");
		labelTilesetParticles.init(currentLabelStart,currentLine);
		labelTilesetParticles.setText(lang.getString("ShowTilesetParticles"));

		checkBoxTilesetParticles.registerGraphicComponent(containerName,"checkBoxTilesetParticles");
		checkBoxTilesetParticles.init(currentColumnStart,currentLine);
		checkBoxTilesetParticles.setValue(config.getBool("TilesetParticles","true"));
		currentLine-=lineOffset;

		//animated tileset objects
		labelAnimatedTilesetObjects.registerGraphicComponent(containerName,"labelAnimatedTilesetObjects");
		labelAnimatedTilesetObjects.init(currentLabelStart,currentLine);
		labelAnimatedTilesetObjects.setText(lang.getString("AnimatedTilesetObjects"));

		listBoxAnimatedTilesetObjects.registerGraphicComponent(containerName,"listBoxAnimatedTilesetObjects");
		listBoxAnimatedTilesetObjects.init(currentColumnStart, currentLine, 80);
		listBoxAnimatedTilesetObjects.pushBackItem("0");
		listBoxAnimatedTilesetObjects.pushBackItem("10");
		listBoxAnimatedTilesetObjects.pushBackItem("25");
		listBoxAnimatedTilesetObjects.pushBackItem("50");
		listBoxAnimatedTilesetObjects.pushBackItem("100");
		listBoxAnimatedTilesetObjects.pushBackItem("300");
		listBoxAnimatedTilesetObjects.pushBackItem("500");
		listBoxAnimatedTilesetObjects.pushBackItem("∞");
		listBoxAnimatedTilesetObjects.setSelectedItem("∞",true);
		listBoxAnimatedTilesetObjects.setSelectedItem(config.getString("AnimatedTilesetObjects","-1"),false);
		currentLine-=lineOffset;

		//unit particles
		labelMapPreview.registerGraphicComponent(containerName,"labelMapPreview");
		labelMapPreview.init(currentLabelStart,currentLine);
		labelMapPreview.setText(lang.getString("ShowMapPreview"));

		checkBoxMapPreview.registerGraphicComponent(containerName,"checkBoxMapPreview");
		checkBoxMapPreview.init(currentColumnStart,currentLine);
		checkBoxMapPreview.setValue(config.getBool("MapPreview","true"));
		currentLine-=lineOffset;

		// Texture Compression flag
		labelEnableTextureCompression.registerGraphicComponent(containerName,"labelEnableTextureCompression");
		labelEnableTextureCompression.init(currentLabelStart ,currentLine);
		labelEnableTextureCompression.setText(lang.getString("EnableTextureCompression"));

		checkBoxEnableTextureCompression.registerGraphicComponent(containerName,"checkBoxEnableTextureCompression");
		checkBoxEnableTextureCompression.init(currentColumnStart ,currentLine );
		checkBoxEnableTextureCompression.setValue(config.getBool("EnableTextureCompression","false"));
		currentLine-=lineOffset;

		labelRainEffect.registerGraphicComponent(containerName,"labelRainEffect");
		labelRainEffect.init(currentLabelStart ,currentLine);
		labelRainEffect.setText(lang.getString("RainEffectMenuGame"));

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
		labelVideos.setText(lang.getString("EnableVideos"));

		checkBoxVideos.registerGraphicComponent(containerName,"checkBoxVideos");
		checkBoxVideos.init(currentColumnStart ,currentLine );
		checkBoxVideos.setValue(config.getBool("EnableVideos","true"));

		// end

		// external server port
		//currentLine-=lineOffset;

		// buttons
		buttonOk.registerGraphicComponent(containerName,"buttonOk");
		buttonOk.init(buttonStartPos, buttonRowPos, 100);
		buttonOk.setText(lang.getString("Save"));
		buttonReturn.setText(lang.getString("Return"));

		buttonReturn.registerGraphicComponent(containerName,"buttonAbort");
		buttonReturn.init(buttonStartPos+110, buttonRowPos, 100);
		buttonAutoConfig.setText(lang.getString("AutoConfig"));

		buttonAutoConfig.registerGraphicComponent(containerName,"buttonAutoConfig");
		buttonAutoConfig.init(buttonStartPos+250, buttonRowPos, 125);

		buttonVideoInfo.setText(lang.getString("VideoInfo"));
		buttonVideoInfo.registerGraphicComponent(containerName,"buttonVideoInfo");
		buttonVideoInfo.init(buttonStartPos+385, buttonRowPos, 125); // was 620

		GraphicComponent::applyAllCustomProperties(containerName);
	}
	catch(exception &e) {
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error loading options: %s\n",__FILE__,__FUNCTION__,__LINE__,e.what());
		throw megaglest_runtime_error(string("Error loading options msg: ") + e.what());
	}
}

void MenuStateOptionsGraphics::reloadUI() {
	Lang &lang= Lang::getInstance();

	console.resetFonts();
	mainMessageBox.init(lang.getString("Ok"));

	buttonAudioSection.setFont(CoreData::getInstance().getMenuFontVeryBig());
	buttonAudioSection.setFont3D(CoreData::getInstance().getMenuFontVeryBig3D());
	buttonAudioSection.setText(lang.getString("Audio"));

	buttonVideoSection.setFont(CoreData::getInstance().getMenuFontVeryBig());
	buttonVideoSection.setFont3D(CoreData::getInstance().getMenuFontVeryBig3D());
	buttonVideoSection.setText(lang.getString("Video"));

	buttonMiscSection.setFont(CoreData::getInstance().getMenuFontVeryBig());
	buttonMiscSection.setFont3D(CoreData::getInstance().getMenuFontVeryBig3D());
	buttonMiscSection.setText(lang.getString("Misc"));

	buttonNetworkSettings.setFont(CoreData::getInstance().getMenuFontVeryBig());
	buttonNetworkSettings.setFont3D(CoreData::getInstance().getMenuFontVeryBig3D());
	buttonNetworkSettings.setText(lang.getString("Network"));

	std::vector<string> listboxData;
	listboxData.push_back("None");
	listboxData.push_back("OpenAL");

	labelScreenModes.setText(lang.getString("Resolution"));

	labelFullscreenWindowed.setText(lang.getString("Windowed"));
	labelFilter.setText(lang.getString("Filter"));

	listboxData.clear();
	listboxData.push_back("Bilinear");
	listboxData.push_back("Trilinear");
	listBoxFilter.setItems(listboxData);

	listboxData.clear();
	for (float f=0.0;f<2.1f;f=f+0.1f) {
		listboxData.push_back(floatToStr(f));
	}
	listBoxGammaCorrection.setItems(listboxData);


	listboxData.clear();
	for (float f=0.5;f<3.0f;f=f+0.1f) {
		listboxData.push_back(floatToStr(f));
	}
	listBoxShadowIntensity.setItems(listboxData);


	labelShadows.setText(lang.getString("Shadows"));
	labelShadowTextureSize.setText(lang.getString("ShadowTextureSize"));

	labelShadowIntensity.setText(lang.getString("ShadowIntensity"));
	labelGammaCorrection.setText(lang.getString("GammaCorrection"));

	listboxData.clear();
	for(int i= 0; i<Renderer::sCount; ++i){
		listboxData.push_back(lang.getString(Renderer::shadowsToStr(static_cast<Renderer::Shadows>(i))));
	}
	listBoxShadows.setItems(listboxData);

	labelTextures3D.setText(lang.getString("Textures3D"));

	labelLights.setText(lang.getString("MaxLights"));

	labelUnitParticles.setText(lang.getString("ShowUnitParticles"));

	labelTilesetParticles.setText(lang.getString("ShowTilesetParticles"));
	labelAnimatedTilesetObjects.setText(lang.getString("AnimatedTilesetObjects"));

	labelMapPreview.setText(lang.getString("ShowMapPreview"));

	labelEnableTextureCompression.setText(lang.getString("EnableTextureCompression"));


	labelRainEffect.setText(lang.getString("RainEffectMenuGame"));

	labelVideos.setText(lang.getString("EnableVideos"));

	buttonOk.setText(lang.getString("Save"));
	buttonReturn.setText(lang.getString("Return"));

	buttonAutoConfig.setText(lang.getString("AutoConfig"));

	buttonVideoInfo.setText(lang.getString("VideoInfo"));

	labelSelectionType.setText(lang.getString("SelectionType"));

	GraphicComponent::reloadFontsForRegisterGraphicComponents(containerName);
}


void MenuStateOptionsGraphics::showMessageBox(const string &text, const string &header, bool toggle){
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

void MenuStateOptionsGraphics::revertScreenMode(){
	Config &config= Config::getInstance();
	//!!!
	// Revert resolution or fullscreen
	checkBoxFullscreenWindowed.setValue(config.getBool("Windowed"));
	string currentResString = config.getString("ScreenWidth") + "x" +
							  config.getString("ScreenHeight") + "-" +
							  intToStr(config.getInt("ColorBits"));
	listBoxScreenModes.setSelectedItem(currentResString);


	changeVideoModeFullScreen(!config.getBool("Windowed"));
	WindowGl *window = this->program->getWindow();
	window->ChangeVideoMode(true,
					config.getInt("ScreenWidth"),
					config.getInt("ScreenHeight"),
					!config.getBool("Windowed"),
					config.getInt("ColorBits"),
				   config.getInt("DepthBits"),
				   config.getInt("StencilBits"),
				   config.getBool("HardwareAcceleration","false"),
				   config.getBool("FullScreenAntiAliasing","false"),
				   config.getFloat("GammaValue","0.0"));

	Metrics::reload();
	this->mainMenu->init();
}

void MenuStateOptionsGraphics::update(){
	if(mainMessageBox.getEnabled() && (mainMessageBoxState == 1)) {
		int waitTime=10;
		if(( time(NULL) - screenModeChangedTimer >waitTime)){
			mainMessageBoxState=0;
			mainMessageBox.setEnabled(false);

			Lang &lang= Lang::getInstance();
			mainMessageBox.init(lang.getString("Ok"));

			revertScreenMode();
		}
		else
		{
			Lang &lang= Lang::getInstance();
			int timeToShow=waitTime- time(NULL) + screenModeChangedTimer;
			// show timer in button
			mainMessageBox.getButton(0)->setText(lang.getString("Ok")+" ("+intToStr(timeToShow)+")");
		}
	}
}

void MenuStateOptionsGraphics::mouseClick(int x, int y, MouseButton mouseButton){

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
					mainMessageBox.init(lang.getString("Ok"));
					//mainMenu->setState(new MenuStateOptions(program, mainMenu));
				}
				else {
					mainMessageBox.setEnabled(false);

					Lang &lang= Lang::getInstance();
					mainMessageBox.init(lang.getString("Ok"));
				}
			}
			else {
				if(mainMessageBoxState == 1) {
					mainMessageBoxState=0;
					mainMessageBox.setEnabled(false);

					Lang &lang= Lang::getInstance();
					mainMessageBox.init(lang.getString("Ok"));

					revertScreenMode();
				}
			}
		}
	}
	else if(buttonOk.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundA());
		Lang &lang= Lang::getInstance();
		bool selectedFullscreenWindowed = checkBoxFullscreenWindowed.getValue();
		string currentResolution=config.getString("ScreenWidth")+"x"+config.getString("ScreenHeight")+"-"+intToStr(config.getInt("ColorBits"));
		string selectedResolution=listBoxScreenModes.getSelectedItem();
		bool currentFullscreenWindowed=config.getBool("Windowed");
		if(currentResolution != selectedResolution || currentFullscreenWindowed != selectedFullscreenWindowed){

			changeVideoModeFullScreen(!selectedFullscreenWindowed);
			const ModeInfo *selectedMode = NULL;
			for(vector<ModeInfo>::const_iterator it= modeInfos.begin(); it!=modeInfos.end(); ++it) {
				if((*it).getString() == selectedResolution) {
					//config.setInt("ScreenWidth",(*it).width);
					//config.setInt("ScreenHeight",(*it).height);
					//config.setInt("ColorBits",(*it).depth);
					selectedMode = &(*it);
				}
			}
			if(selectedMode == NULL) {
				throw megaglest_runtime_error("selectedMode == NULL");
			}
			WindowGl *window = this->program->getWindow();
			window->ChangeVideoMode(true,
					selectedMode->width,
					selectedMode->height,
						!selectedFullscreenWindowed,
							selectedMode->depth,
						   config.getInt("DepthBits"),
						   config.getInt("StencilBits"),
						   config.getBool("HardwareAcceleration","false"),
						   config.getBool("FullScreenAntiAliasing","false"),
						   strToFloat(listBoxGammaCorrection.getSelectedItem()));

			Metrics::reload(selectedMode->width,selectedMode->height);
			this->mainMenu->init();

			mainMessageBoxState=1;
			mainMessageBox.init(lang.getString("Ok"),lang.getString("Cancel"));
			screenModeChangedTimer= time(NULL);
			//showMessageBox(lang.getString("RestartNeeded"), lang.getString("ResolutionChanged"), false);
			showMessageBox(lang.getString("ResolutionChanged"), lang.getString("Notice"), false);
			//No saveConfig() here! this is done by the messageBox
			return;
		}
		saveConfig();
		return;
    }
	else if(buttonReturn.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundA());

		// reset the gamma to former value
		string currentGammaCorrection=config.getString("GammaValue","1.0");
		string selectedGammaCorrection=listBoxGammaCorrection.getSelectedItem();
		if(currentGammaCorrection!=selectedGammaCorrection){
			float gammaValue=strToFloat(currentGammaCorrection);
			if(gammaValue==0.0f) gammaValue=1.0f;
			if(gammaValue!=0.0){
				program->getWindow()->setGamma(gammaValue);
				SDL_SetGamma(gammaValue, gammaValue, gammaValue);
			}
		}
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
			mainMenu->setState(new MenuStateOptions(program, mainMenu,this->parentUI)); // open keyboard shortcuts setup screen
			return;
		}
	else if(buttonVideoSection.mouseClick(x, y)){ 
			soundRenderer.playFx(coreData.getClickSoundA());
			//mainMenu->setState(new MenuStateOptionsGraphics(program, mainMenu,this->parentUI)); // open keyboard shortcuts setup screen
			return;
		}
	else if(buttonAutoConfig.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundA());
		Renderer::getInstance().autoConfig();
		//saveConfig();
		mainMenu->setState(new MenuStateOptionsGraphics(program, mainMenu));
		return;
	}
	else if(buttonVideoInfo.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundA());
		mainMenu->setState(new MenuStateGraphicInfo(program, mainMenu));
		return;
	}
	else
	{
		listBoxSelectionType.mouseClick(x, y);
		listBoxShadows.mouseClick(x, y);
		listBoxAnimatedTilesetObjects.mouseClick(x, y);
		listBoxShadowTextureSize.mouseClick(x, y);
		listBoxShadowIntensity.mouseClick(x, y);
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
		listBoxScreenModes.mouseClick(x, y);
		checkBoxFullscreenWindowed.mouseClick(x, y);
        checkBoxEnableTextureCompression.mouseClick(x, y);
        checkBoxRainEffect.mouseClick(x,y);
        checkBoxRainEffectMenu.mouseClick(x,y);

		checkBoxVideos.mouseClick(x,y);
	}
}

void MenuStateOptionsGraphics::mouseMove(int x, int y, const MouseState *ms){
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
	buttonAutoConfig.mouseMove(x, y);
	buttonVideoInfo.mouseMove(x, y);
	listBoxFilter.mouseMove(x, y);
	listBoxGammaCorrection.mouseMove(x, y);
	listBoxShadowIntensity.mouseMove(x, y);
	listBoxSelectionType.mouseMove(x, y);
	listBoxShadows.mouseMove(x, y);
	checkBoxTextures3D.mouseMove(x, y);
	checkBoxUnitParticles.mouseMove(x, y);
	checkBoxTilesetParticles.mouseMove(x, y);
	labelAnimatedTilesetObjects.mouseMove(x, y);
	listBoxAnimatedTilesetObjects.mouseMove(x, y);
	checkBoxTilesetParticles.mouseMove(x, y);
	checkBoxMapPreview.mouseMove(x, y);
	listBoxLights.mouseMove(x, y);
	listBoxScreenModes.mouseMove(x, y);
	checkBoxFullscreenWindowed.mouseMove(x, y);
	checkBoxEnableTextureCompression.mouseMove(x, y);

	checkBoxRainEffect.mouseMove(x, y);
	checkBoxRainEffectMenu.mouseMove(x, y);

	checkBoxVideos.mouseMove(x, y);
}

//bool MenuStateOptionsGraphics::isInSpecialKeyCaptureEvent() {
//	return (activeInputLabel != NULL);
//}
//
//void MenuStateOptionsGraphics::keyDown(SDL_KeyboardEvent key) {
//	if(activeInputLabel != NULL) {
//		keyDownEditLabel(key, &activeInputLabel);
//	}
//}

void MenuStateOptionsGraphics::keyPress(SDL_KeyboardEvent c) {
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
			//console.addLine(lang.getString("GUILayoutSaved") + " [" + (saved ? lang.getString("Yes") : lang.getString("No"))+ "]");
		}
//	}
}

void MenuStateOptionsGraphics::render(){
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
		renderer.renderButton(&buttonAutoConfig);
		renderer.renderButton(&buttonVideoInfo);
		renderer.renderListBox(&listBoxShadows);
		renderer.renderCheckBox(&checkBoxTextures3D);
		renderer.renderCheckBox(&checkBoxUnitParticles);
		renderer.renderCheckBox(&checkBoxTilesetParticles);
		renderer.renderCheckBox(&checkBoxMapPreview);
		renderer.renderListBox(&listBoxLights);
		renderer.renderListBox(&listBoxFilter);
		renderer.renderListBox(&listBoxGammaCorrection);
		renderer.renderListBox(&listBoxShadowIntensity);
		renderer.renderLabel(&labelShadows);
		renderer.renderLabel(&labelTextures3D);
		renderer.renderLabel(&labelUnitParticles);
		renderer.renderLabel(&labelTilesetParticles);
		renderer.renderListBox(&listBoxAnimatedTilesetObjects);
		renderer.renderLabel(&labelAnimatedTilesetObjects);
		renderer.renderLabel(&labelMapPreview);
		renderer.renderLabel(&labelLights);
		renderer.renderLabel(&labelFilter);
		renderer.renderLabel(&labelGammaCorrection);
		renderer.renderLabel(&labelShadowIntensity);
		renderer.renderLabel(&labelScreenModes);
		renderer.renderListBox(&listBoxScreenModes);
		renderer.renderLabel(&labelFullscreenWindowed);
		renderer.renderCheckBox(&checkBoxFullscreenWindowed);

		renderer.renderLabel(&labelEnableTextureCompression);
        renderer.renderCheckBox(&checkBoxEnableTextureCompression);

        renderer.renderLabel(&labelRainEffect);
        renderer.renderCheckBox(&checkBoxRainEffect);
        renderer.renderLabel(&labelRainEffectSeparator);
        renderer.renderCheckBox(&checkBoxRainEffectMenu);

		renderer.renderLabel(&labelShadowTextureSize);
		renderer.renderListBox(&listBoxShadowTextureSize);

		renderer.renderLabel(&labelSelectionType);
		renderer.renderListBox(&listBoxSelectionType);

        renderer.renderLabel(&labelVideos);
        renderer.renderCheckBox(&checkBoxVideos);
	}

	renderer.renderConsole(&console,false,true);
	if(program != NULL) program->renderProgramMsgBox();
}

void MenuStateOptionsGraphics::saveConfig(){
	Config &config= Config::getInstance();
	Lang &lang= Lang::getInstance();
	//setActiveInputLable(NULL);

	int selectionTypeindex= listBoxSelectionType.getSelectedItemIndex();
	if(selectionTypeindex==0){
		config.setString("SelectionType",Config::selectBufPicking);
	}
	else if (selectionTypeindex==1){
		config.setString("SelectionType",Config::colorPicking);
	}
	else if (selectionTypeindex==2){
		config.setString("SelectionType",Config::frustumPicking);
	}

	int index= listBoxShadows.getSelectedItemIndex();
	config.setString("Shadows", Renderer::shadowsToStr(static_cast<Renderer::Shadows>(index)));

	string texSizeString= listBoxShadowTextureSize.getSelectedItem();
	config.setInt("ShadowTextureSize",strToInt(texSizeString) );

	config.setBool("Windowed", checkBoxFullscreenWindowed.getValue());
	config.setString("Filter", listBoxFilter.getSelectedItem());
	config.setFloat("GammaValue", strToFloat(listBoxGammaCorrection.getSelectedItem()));
	config.setFloat("ShadowIntensity", strToFloat(listBoxShadowIntensity.getSelectedItem()));
	config.setBool("Textures3D", checkBoxTextures3D.getValue());
	config.setBool("UnitParticles", (checkBoxUnitParticles.getValue()));
	config.setBool("TilesetParticles", (checkBoxTilesetParticles.getValue()));
	config.setBool("MapPreview", checkBoxMapPreview.getValue());
	config.setInt("MaxLights", listBoxLights.getSelectedItemIndex()+1);

	if (listBoxAnimatedTilesetObjects.getSelectedItem()=="∞") {
		config.setInt("AnimatedTilesetObjects", -1);
	} else {
		config.setInt("AnimatedTilesetObjects", atoi(listBoxAnimatedTilesetObjects.getSelectedItem().c_str()));
	}

    config.setBool("RainEffect", checkBoxRainEffect.getValue());
    config.setBool("RainEffectMenu", checkBoxRainEffectMenu.getValue());

    config.setBool("EnableTextureCompression", checkBoxEnableTextureCompression.getValue());

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
	console.addLine(lang.getString("SettingsSaved"));
}

//void MenuStateOptionsGraphics::setActiveInputLable(GraphicLabel *newLable) {
//	MenuState::setActiveInputLabel(newLable,&activeInputLabel);
//
//	if(newLable == &labelTransifexPwd) {
//		labelTransifexPwd.setIsPassword(false);
//	}
//	else {
//		labelTransifexPwd.setIsPassword(true);
//	}
//}

}}//end namespace
