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
#include "megaglest_cegui_manager.h"

#include "leak_dumper.h"

using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class MenuStateOptions
// =====================================================
MenuStateOptionsGraphics::MenuStateOptionsGraphics(Program *program, MainMenu *mainMenu, ProgramState **parentUI):
	MenuState(program, mainMenu, "config") {
	try {
		containerName 			= "OptionsVideo";
		this->parentUI 			= parentUI;
		mainMessageBoxState		= 0;
		screenModeChangedTimer 	= time(NULL);
		Config &config			= Config::getInstance();

		this->console.setOnlyChatMessagesInStoredLines(false);
		::Shared::PlatformCommon::getFullscreenVideoModes(&modeInfos,!config.getBool("Windowed"));

		setupCEGUIWidgets();

		GraphicComponent::applyAllCustomProperties(containerName);
	}
	catch(exception &e) {
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error loading options: %s\n",__FILE__,__FUNCTION__,__LINE__,e.what());
		throw megaglest_runtime_error(string("Error loading options msg: ") + e.what());
	}
}

void MenuStateOptionsGraphics::reloadUI() {
	console.resetFonts();
	GraphicComponent::reloadFontsForRegisterGraphicComponents(containerName);

	setupCEGUIWidgetsText();
}

void MenuStateOptionsGraphics::setupCEGUIWidgets() {

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
			"TabControl/__auto_TabPane__/Video/ButtonSave",
			cegui_manager.getEventButtonClicked(), this);

	cegui_manager.setControlEventCallback(containerName,
			"TabControl/__auto_TabPane__/Video/ButtonReturn",
			cegui_manager.getEventButtonClicked(), this);

	cegui_manager.setControlEventCallback(containerName,
			"TabControl/__auto_TabPane__/Video/ButtonAutoConfig",
			cegui_manager.getEventButtonClicked(), this);

	cegui_manager.setControlEventCallback(containerName,
			"TabControl/__auto_TabPane__/Video/SpinnerBrightness",
			cegui_manager.getEventSpinnerValueChanged(), this);

	cegui_manager.subscribeMessageBoxEventClicks(containerName, this);
	cegui_manager.subscribeMessageBoxEventClicks(containerName, this, "TabControl/__auto_TabPane__/Video/MsgBox");
}

void MenuStateOptionsGraphics::setupCEGUIWidgetsText() {

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

	cegui_manager.setSelectedTabPage("TabControl", "Video");

	if(cegui_manager.isChildControl(cegui_manager.getControl("TabControl/__auto_TabPane__/Video"),"MsgBox") == false) {
		cegui_manager.cloneMessageBoxControl("MsgBox", ctlVideo);
	}

	string currentResString = config.getString("ScreenWidth") + "x" +
							  config.getString("ScreenHeight") + "-" +
							  intToStr(config.getInt("ColorBits"));
	vector<string> videoModes;
	bool currentResolutionFound = false;
	for(vector<ModeInfo>::const_iterator it = modeInfos.begin(); it != modeInfos.end(); ++it) {
		if((*it).getString() == currentResString) {
			currentResolutionFound = true;
		}
		videoModes.push_back((*it).getString());
	}
	if(currentResolutionFound == false) {
		videoModes.push_back(currentResString);
	}

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlVideo,"LabelResolution"),lang.getString("Resolution","",false,true));
	cegui_manager.addItemsToComboBoxControl(
			cegui_manager.getChildControl(ctlVideo,"ComboBoxResolution"), videoModes,true);
	cegui_manager.setSelectedItemInComboBoxControl(
			cegui_manager.getChildControl(ctlVideo,"ComboBoxResolution"), currentResString,true);
	cegui_manager.setControlText(cegui_manager.getChildControl(ctlVideo,"ComboBoxResolution"),currentResString);


	cegui_manager.setControlText(cegui_manager.getChildControl(ctlVideo,"LabelWindowed"),lang.getString("Windowed","",false,true));
	cegui_manager.setCheckboxControlChecked(cegui_manager.getChildControl(ctlVideo,"CheckboxWindowed"),config.getBool("Windowed","true"));

	float gammaValue = config.getFloat("GammaValue","1.0");
	if(gammaValue == 0.0f) {
		gammaValue = 1.0f;
	}

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlVideo,"LabelBrightness"),lang.getString("GammaCorrection","",false,true));
	cegui_manager.setSpinnerControlValues(cegui_manager.getChildControl(ctlVideo,"SpinnerBrightness"),0.5,3.0,gammaValue,0.1);

	string selectedFilter = config.getString("Filter");
	vector<string> filterList;
	filterList.push_back("Bilinear");
	filterList.push_back("Trilinear");
	cegui_manager.setControlText(cegui_manager.getChildControl(ctlVideo,"LabelFilter"),lang.getString("Filter","",false,true));
	cegui_manager.addItemsToComboBoxControl(
			cegui_manager.getChildControl(ctlVideo,"ComboBoxFilter"), filterList,true);
	cegui_manager.setSelectedItemInComboBoxControl(
			cegui_manager.getChildControl(ctlVideo,"ComboBoxFilter"), selectedFilter,true);
	cegui_manager.setControlText(cegui_manager.getChildControl(ctlVideo,"ComboBoxFilter"),selectedFilter);

	filterList.clear();
	filterList.push_back("SelectBuffer (nvidia)");
	filterList.push_back("ColorPicking (default)");
	filterList.push_back("FrustumPicking (bad)");

	int selectedItemIndex = -1;
	const string selectionType = toLower(config.getString("SelectionType",Config::colorPicking));
	if(selectionType == Config::colorPicking) {
		selectedItemIndex = 1;
	}
	else if (selectionType == Config::frustumPicking ) {
		selectedItemIndex = 2;
	}
	else {
		selectedItemIndex = 0;
	}

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlVideo,"LabelUnitSelectionType"),lang.getString("SelectionType","",false,true));
	cegui_manager.addItemsToComboBoxControl(
			cegui_manager.getChildControl(ctlVideo,"ComboBoxUnitSelectionType"), filterList,true);
	cegui_manager.setSelectedItemInComboBoxControl(
			cegui_manager.getChildControl(ctlVideo,"ComboBoxUnitSelectionType"), selectedItemIndex);

	filterList.clear();
	for(int index = 0; index < Renderer::sCount; ++index) {
		filterList.push_back(lang.getString(Renderer::shadowsToStr(static_cast<Renderer::Shadows>(index))));
	}
	string selectedShadow = config.getString("Shadows");
	selectedItemIndex = clamp(Renderer::strToShadows(selectedShadow), 0, Renderer::sCount-1);

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlVideo,"LabelShadowType"),lang.getString("Shadows","",false,true));
	cegui_manager.addItemsToComboBoxControl(
			cegui_manager.getChildControl(ctlVideo,"ComboBoxShadowType"), filterList,true);
	cegui_manager.setSelectedItemInComboBoxControl(
			cegui_manager.getChildControl(ctlVideo,"ComboBoxShadowType"), selectedItemIndex);


	filterList.clear();
	filterList.push_back("256");
	filterList.push_back("512");
	filterList.push_back("1024");

	string selectedTexture = intToStr(config.getInt("ShadowTextureSize","512"));
	selectedItemIndex = clamp(Renderer::strToShadows(selectedShadow), 0, Renderer::sCount-1);

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlVideo,"LabelShadowTextureSize"),lang.getString("ShadowTextureSize","",false,true));
	cegui_manager.addItemsToComboBoxControl(
			cegui_manager.getChildControl(ctlVideo,"ComboBoxShadowTextureSize"), filterList,true);
	cegui_manager.setSelectedItemInComboBoxControl(
			cegui_manager.getChildControl(ctlVideo,"ComboBoxShadowTextureSize"), selectedItemIndex);

	float shadowIntensity = config.getFloat("ShadowIntensity","1.0");
	if(shadowIntensity <= 0.0f) {
		shadowIntensity = 1.0f;
	}

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlVideo,"LabelShadowIntensity"),lang.getString("ShadowIntensity","",false,true));
	cegui_manager.setSpinnerControlValues(cegui_manager.getChildControl(ctlVideo,"SpinnerShadowIntensity"),0.5,3.0,shadowIntensity,0.1);

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlVideo,"Label3dTextures"),lang.getString("Textures3D","",false,true));
	cegui_manager.setCheckboxControlChecked(cegui_manager.getChildControl(ctlVideo,"Checkbox3dTextures"),config.getBool("Textures3D","true"));

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlVideo,"LabelMaxLights"),lang.getString("MaxLights","",false,true));
	cegui_manager.setSpinnerControlValues(cegui_manager.getChildControl(ctlVideo,"SpinnerMaxLights"),1,8,config.getInt("MaxLights"),1);

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlVideo,"LabelUnitParticles"),lang.getString("ShowUnitParticles","",false,true));
	cegui_manager.setCheckboxControlChecked(cegui_manager.getChildControl(ctlVideo,"CheckboxUnitParticles"),config.getBool("UnitParticles","true"));

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlVideo,"LabelTilesetParticles"),lang.getString("ShowTilesetParticles","",false,true));
	cegui_manager.setCheckboxControlChecked(cegui_manager.getChildControl(ctlVideo,"CheckboxTilesetParticles"),config.getBool("TilesetParticles","true"));

	int selectedAnimatedObjects = config.getInt("AnimatedTilesetObjects","-1");
	cegui_manager.setControlText(cegui_manager.getChildControl(ctlVideo,"LabelAnimatedTilesetObjects"),lang.getString("AnimatedTilesetObjects","",false,true));
	cegui_manager.setSpinnerControlValues(cegui_manager.getChildControl(ctlVideo,"SpinnerAnimatedTilesetObjects"),0,10000,(selectedAnimatedObjects >= 0 ? selectedAnimatedObjects : 10000),10);

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlVideo,"LabelMapPreview"),lang.getString("ShowMapPreview","",false,true));
	cegui_manager.setCheckboxControlChecked(cegui_manager.getChildControl(ctlVideo,"CheckboxMapPreview"),config.getBool("MapPreview","true"));

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlVideo,"LabelTextureCompression"),lang.getString("EnableTextureCompression","",false,true));
	cegui_manager.setCheckboxControlChecked(cegui_manager.getChildControl(ctlVideo,"CheckboxTextureCompression"),config.getBool("EnableTextureCompression","false"));

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlVideo,"LabelRainEffect"),lang.getString("RainEffectMenuGame","",false,true));
	cegui_manager.setCheckboxControlChecked(cegui_manager.getChildControl(ctlVideo,"CheckboxRainEffectMenu"),config.getBool("RainEffectMenu","true"));
	cegui_manager.setCheckboxControlChecked(cegui_manager.getChildControl(ctlVideo,"CheckboxRainEffectGame"),config.getBool("RainEffect","true"));

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlVideo,"LabelVideoPlayback"),lang.getString("EnableVideos","",false,true));
	cegui_manager.setCheckboxControlChecked(cegui_manager.getChildControl(ctlVideo,"CheckboxVideoPlayback"),config.getBool("EnableVideos","true"));

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlVideo,"ButtonSave"),lang.getString("Save","",false,true));
	cegui_manager.setControlText(cegui_manager.getChildControl(ctlVideo,"ButtonReturn"),lang.getString("Return","",false,true));

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlVideo,"ButtonAutoConfig"),lang.getString("AutoConfig","",false,true));

	Renderer &renderer= Renderer::getInstance();
	string glInfo	  = renderer.getGlInfo();
	string glMoreInfo = renderer.getGlMoreInfo();
	string strInternalInfo = "VBOSupported: " + boolToStr(getVBOSupported());
	if(getenv("MEGAGLEST_FONT") != NULL) {
		char *tryFont = getenv("MEGAGLEST_FONT");
		strInternalInfo += "\nMEGAGLEST_FONT: " + string(tryFont);
	}
	strInternalInfo += "\nforceLegacyFonts: " + boolToStr(Font::forceLegacyFonts);
	strInternalInfo += "\nrenderText3DEnabled: " + boolToStr(Renderer::renderText3DEnabled);
	strInternalInfo += "\nuseTextureCompression: " + boolToStr(Texture::useTextureCompression);
	strInternalInfo += "\nfontIsRightToLeft: " + boolToStr(Font::fontIsRightToLeft);
	strInternalInfo += "\nscaleFontValue: " + floatToStr(Font::scaleFontValue);
	strInternalInfo += "\nscaleFontValueCenterHFactor: " + floatToStr(Font::scaleFontValueCenterHFactor);
	strInternalInfo += "\nlangHeightText: " + Font::langHeightText;
	strInternalInfo += "\nAllowAltEnterFullscreenToggle: " + boolToStr(Window::getAllowAltEnterFullscreenToggle());
	strInternalInfo += "\nTryVSynch: " + boolToStr(Window::getTryVSynch());
	strInternalInfo += "\nVERBOSE_MODE_ENABLED: " + boolToStr(SystemFlags::VERBOSE_MODE_ENABLED);

	cegui_manager.setControlText(cegui_manager.getChildControl(ctlVideo,"LabelVideoInfo"),lang.getString("VideoInfo","",false,true));
	cegui_manager.setControlText(cegui_manager.getChildControl(ctlVideo,"EditboxVideoInfo"),glInfo + "\n" + glMoreInfo + "\n" + strInternalInfo);
}

void MenuStateOptionsGraphics::callDelayedCallbacks() {
	if(hasDelayedCallbacks() == true) {
		for(unsigned int index = 0; index < delayedCallbackList.size(); ++index) {
			DelayCallbackFunction pCB = delayedCallbackList[index];
			(this->*pCB)();
		}
	}
}

void MenuStateOptionsGraphics::delayedCallbackFunctionReturn() {
	CoreData &coreData				= CoreData::getInstance();
	SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

	soundRenderer.playFx(coreData.getClickSoundA());
	if(this->parentUI != NULL) {
		*this->parentUI = NULL;
		delete *this->parentUI;
	}

	mainMenu->setState(new MenuStateRoot(program, mainMenu));
}

void MenuStateOptionsGraphics::delayedCallbackFunctionSelectAudioTab() {
	CoreData &coreData				= CoreData::getInstance();
	SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

	soundRenderer.playFx(coreData.getClickSoundA());
	mainMenu->setState(new MenuStateOptionsSound(program, mainMenu,this->parentUI)); // open keyboard shortcuts setup screen
}

void MenuStateOptionsGraphics::delayedCallbackFunctionSelectKeyboardTab() {
	CoreData &coreData				= CoreData::getInstance();
	SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

	soundRenderer.playFx(coreData.getClickSoundA());
	mainMenu->setState(new MenuStateKeysetup(program, mainMenu,this->parentUI)); // open keyboard shortcuts setup screen
}

void MenuStateOptionsGraphics::delayedCallbackFunctionSelectMiscTab() {
	CoreData &coreData				= CoreData::getInstance();
	SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

	soundRenderer.playFx(coreData.getClickSoundA());
	mainMenu->setState(new MenuStateOptions(program, mainMenu,this->parentUI)); // open keyboard shortcuts setup screen
}

void MenuStateOptionsGraphics::delayedCallbackFunctionSelectNetworkTab() {
	CoreData &coreData				= CoreData::getInstance();
	SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

	soundRenderer.playFx(coreData.getClickSoundA());
	mainMenu->setState(new MenuStateOptionsNetwork(program, mainMenu,this->parentUI)); // open keyboard shortcuts setup screen
}

bool MenuStateOptionsGraphics::EventCallback(CEGUI::Window *ctl, std::string name) {

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	if(name == cegui_manager.getEventTabControlSelectionChanged()) {

		if(cegui_manager.isSelectedTabPage("TabControl", "Audio") == true) {
			DelayCallbackFunction pCB = &MenuStateOptionsGraphics::delayedCallbackFunctionSelectAudioTab;
			delayedCallbackList.push_back(pCB);
			return true;
		}
		else if(cegui_manager.isSelectedTabPage("TabControl", "Keyboard") == true) {
			DelayCallbackFunction pCB = &MenuStateOptionsGraphics::delayedCallbackFunctionSelectKeyboardTab;
			delayedCallbackList.push_back(pCB);
			return true;
		}
		else if(cegui_manager.isSelectedTabPage("TabControl", "Misc") == true) {
			DelayCallbackFunction pCB = &MenuStateOptionsGraphics::delayedCallbackFunctionSelectMiscTab;
			delayedCallbackList.push_back(pCB);
			return true;
		}
		else if(cegui_manager.isSelectedTabPage("TabControl", "Network") == true) {
			DelayCallbackFunction pCB = &MenuStateOptionsGraphics::delayedCallbackFunctionSelectNetworkTab;
			delayedCallbackList.push_back(pCB);
			return true;
		}
	}
	else if(name == cegui_manager.getEventButtonClicked()) {

		//printf("Line: %d mainMessageBoxState = %d\n",__LINE__,mainMessageBoxState);

		if(cegui_manager.isControlMessageBoxOk(ctl,"TabControl/__auto_TabPane__/Video/MsgBox") == true) {

			CoreData &coreData				= CoreData::getInstance();
			SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

			soundRenderer.playFx(coreData.getClickSoundA());

			//printf("Line: %d mainMessageBoxState = %d\n",__LINE__,mainMessageBoxState);

			if(mainMessageBoxState == 1) {
				mainMessageBoxState = 0;
				saveConfig();

				//mainMenu->setState(new MenuStateRoot(program, mainMenu));
			}

			cegui_manager.hideMessageBox("TabControl/__auto_TabPane__/Video/MsgBox");
			return true;
		}
		else if(cegui_manager.isControlMessageBoxCancel(ctl,"TabControl/__auto_TabPane__/Video/MsgBox") == true) {

			CoreData &coreData				= CoreData::getInstance();
			SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

			soundRenderer.playFx(coreData.getClickSoundA());

			//printf("Line: %d mainMessageBoxState = %d\n",__LINE__,mainMessageBoxState);

			if(mainMessageBoxState == 1) {
				mainMessageBoxState = 0;
				revertScreenMode();
			}

			cegui_manager.hideMessageBox("TabControl/__auto_TabPane__/Video/MsgBox");
			return true;
		}
		else if(ctl == cegui_manager.getControl("TabControl/__auto_TabPane__/Video/ButtonSave")) {

			//printf("Line: %d mainMessageBoxState = %d\n",__LINE__,mainMessageBoxState);

			CoreData &coreData				= CoreData::getInstance();
			SoundRenderer &soundRenderer	= SoundRenderer::getInstance();
			Config &config 					= Config::getInstance();
			Lang &lang						= Lang::getInstance();

			soundRenderer.playFx(coreData.getClickSoundA());

			bool selectedFullscreenWindowed = cegui_manager.getCheckboxControlChecked(
					cegui_manager.getControl("TabControl/__auto_TabPane__/Video/CheckboxWindowed"));

			string currentResolution = config.getString("ScreenWidth") +
					                   "x" +
					                   config.getString("ScreenHeight") +
					                   "-" +
					                   intToStr(config.getInt("ColorBits"));

			string selectedResolution = cegui_manager.getSelectedItemFromComboBoxControl(
						cegui_manager.getControl("TabControl/__auto_TabPane__/Video/ComboBoxResolution"));

			bool currentFullscreenWindowed = config.getBool("Windowed");

			float selectedBrightness = (int)cegui_manager.getSpinnerControlValue(
					cegui_manager.getControl("TabControl/__auto_TabPane__/Video/SpinnerBrightness"));

			if(currentResolution != selectedResolution ||
				currentFullscreenWindowed != selectedFullscreenWindowed) {

				changeVideoModeFullScreen(!selectedFullscreenWindowed);

				const ModeInfo *selectedMode = NULL;
				for(vector<ModeInfo>::const_iterator it= modeInfos.begin(); it != modeInfos.end(); ++it) {
					if((*it).getString() == selectedResolution) {
						selectedMode = &(*it);
						break;
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
						selectedBrightness);

				Metrics::reload(selectedMode->width,selectedMode->height);
				this->mainMenu->init();

				mainMessageBoxState=1;
				screenModeChangedTimer = time(NULL);

				showMessageBox(lang.getString("ResolutionChanged"), lang.getString("Notice"), false);
				//No saveConfig() here! this is done by the messageBox
				return true;
			}
			saveConfig();

			return true;
		}
		else if(ctl == cegui_manager.getControl("TabControl/__auto_TabPane__/Video/ButtonReturn")) {

			//printf("Line: %d mainMessageBoxState = %d\n",__LINE__,mainMessageBoxState);

			DelayCallbackFunction pCB = &MenuStateOptionsGraphics::delayedCallbackFunctionReturn;
			delayedCallbackList.push_back(pCB);

			return true;
		}
		else if(ctl == cegui_manager.getControl("TabControl/__auto_TabPane__/Video/ButtonAutoConfig")) {

			//printf("Line: %d mainMessageBoxState = %d\n",__LINE__,mainMessageBoxState);

			CoreData &coreData				= CoreData::getInstance();
			SoundRenderer &soundRenderer	= SoundRenderer::getInstance();

			soundRenderer.playFx(coreData.getClickSoundA());
			Renderer::getInstance().autoConfig();
			//mainMenu->setState(new MenuStateOptionsGraphics(program, mainMenu));
			setupCEGUIWidgetsText();

			return true;

		}
		printf("Line: %d mainMessageBoxState = %d\n",__LINE__,mainMessageBoxState);
		cegui_manager.printDebugControlInfo(ctl);

	}
	else if(name == cegui_manager.getEventSpinnerValueChanged()) {
		if(ctl == cegui_manager.getControl("TabControl/__auto_TabPane__/Video/SpinnerBrightness")) {

			float selectedBrightness = (int)cegui_manager.getSpinnerControlValue(
					cegui_manager.getControl("TabControl/__auto_TabPane__/Video/SpinnerBrightness"));

			if(selectedBrightness != 0.0) {
				program->getWindow()->setGamma(selectedBrightness);
				SDL_SetGamma(selectedBrightness, selectedBrightness, selectedBrightness);
			}
			return true;
		}
	}

	return false;
}

void MenuStateOptionsGraphics::showMessageBox(const string &text, const string &header, bool toggle) {

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	if(cegui_manager.isMessageBoxShowing("TabControl/__auto_TabPane__/Video/MsgBox") == false) {
		MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
		Lang &lang= Lang::getInstance();
		cegui_manager.displayMessageBox(header, text, lang.getString("Yes","",false,true),lang.getString("No","",false,true),"TabControl/__auto_TabPane__/Video/MsgBox");
	}
	else {
		cegui_manager.hideMessageBox("TabControl/__auto_TabPane__/Video/MsgBox");
	}
}

void MenuStateOptionsGraphics::revertScreenMode() {
	Config &config= Config::getInstance();
	Lang &lang= Lang::getInstance();
	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();

	// Revert resolution or fullscreen
	cegui_manager.setControlText("TabControl/__auto_TabPane__/Video/LabelWindowed",lang.getString("Windowed","",false,true));

	string currentResString = config.getString("ScreenWidth") + "x" +
							  config.getString("ScreenHeight") + "-" +
							  intToStr(config.getInt("ColorBits"));

	cegui_manager.setSelectedItemInComboBoxControl(
			cegui_manager.getControl("TabControl/__auto_TabPane__/Video/ComboBoxResolution"), currentResString,true);
	cegui_manager.setControlText("TabControl/__auto_TabPane__/Video/ComboBoxResolution",currentResString);

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

void MenuStateOptionsGraphics::update() {
	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();
	if(cegui_manager.isMessageBoxShowing() == true && mainMessageBoxState == 1) {
		int waitTime = 10;
		if(( time(NULL) - screenModeChangedTimer > waitTime)) {
			mainMessageBoxState = 0;
			cegui_manager.hideMessageBox("TabControl/__auto_TabPane__/Video/MsgBox");

			revertScreenMode();
		}
		else {
			Lang &lang= Lang::getInstance();
			int timeToShow = waitTime - time(NULL) + screenModeChangedTimer;
			// show timer in button
			cegui_manager.setMessageBoxButtonText(lang.getString("Ok","",false,true) + " (" + intToStr(timeToShow) + ")", lang.getString("No","",false,true));
		}
	}
	MenuState::update();
}

void MenuStateOptionsGraphics::mouseClick(int x, int y, MouseButton mouseButton) { }

void MenuStateOptionsGraphics::mouseMove(int x, int y, const MouseState *ms) { }

void MenuStateOptionsGraphics::keyPress(SDL_KeyboardEvent c) { }

void MenuStateOptionsGraphics::render() {

	Renderer &renderer = Renderer::getInstance();

	renderer.renderConsole(&console,false,true);
	if(program != NULL) program->renderProgramMsgBox();
}

void MenuStateOptionsGraphics::saveConfig() {
	Config &config 	= Config::getInstance();
	Lang &lang		= Lang::getInstance();

	MegaGlest_CEGUIManager &cegui_manager = MegaGlest_CEGUIManager::getInstance();

	string selectedUnitPicker = cegui_manager.getSelectedItemFromComboBoxControl(
				cegui_manager.getControl("TabControl/__auto_TabPane__/Video/ComboBoxUnitSelectionType"));

	if(selectedUnitPicker == "SelectBuffer (nvidia)") {
		config.setString("SelectionType",Config::selectBufPicking);
	}
	else if(selectedUnitPicker == "ColorPicking (default)") {
		config.setString("SelectionType",Config::colorPicking);
	}
	else if (selectedUnitPicker == "FrustumPicking (bad)") {
		config.setString("SelectionType",Config::frustumPicking);
	}

	string selectedItem = cegui_manager.getSelectedItemFromComboBoxControl(
				cegui_manager.getControl("TabControl/__auto_TabPane__/Video/ComboBoxShadowType"));
	config.setString("Shadows", selectedItem);


	selectedItem = cegui_manager.getSelectedItemFromComboBoxControl(
				cegui_manager.getControl("TabControl/__auto_TabPane__/Video/ComboBoxShadowTextureSize"));
	config.setInt("ShadowTextureSize",strToInt(selectedItem) );

	config.setBool("Windowed", cegui_manager.getCheckboxControlChecked(
			cegui_manager.getControl("TabControl/__auto_TabPane__/Video/CheckboxWindowed")));

	selectedItem = cegui_manager.getSelectedItemFromComboBoxControl(
				cegui_manager.getControl("TabControl/__auto_TabPane__/Video/ComboBoxFilter"));
	config.setString("Filter", selectedItem);

	config.setFloat("GammaValue", cegui_manager.getSpinnerControlValue(
			cegui_manager.getControl("TabControl/__auto_TabPane__/Video/SpinnerBrightness")));

	config.setFloat("ShadowIntensity", cegui_manager.getSpinnerControlValue(
			cegui_manager.getControl("TabControl/__auto_TabPane__/Video/SpinnerShadowIntensity")));

	config.setBool("Textures3D", cegui_manager.getCheckboxControlChecked(
			cegui_manager.getControl("TabControl/__auto_TabPane__/Video/Checkbox3dTextures")));

	config.setBool("UnitParticles", cegui_manager.getCheckboxControlChecked(
			cegui_manager.getControl("TabControl/__auto_TabPane__/Video/CheckboxUnitParticles")));

	config.setBool("TilesetParticles", cegui_manager.getCheckboxControlChecked(
			cegui_manager.getControl("TabControl/__auto_TabPane__/Video/CheckboxTilesetParticles")));

	config.setBool("MapPreview", cegui_manager.getCheckboxControlChecked(
			cegui_manager.getControl("TabControl/__auto_TabPane__/Video/CheckboxMapPreview")));

	config.setInt("MaxLights", cegui_manager.getSpinnerControlValue(
			cegui_manager.getControl("TabControl/__auto_TabPane__/Video/SpinnerMaxLights")));

	config.setInt("AnimatedTilesetObjects", cegui_manager.getSpinnerControlValue(
			cegui_manager.getControl("TabControl/__auto_TabPane__/Video/SpinnerAnimatedTilesetObjects")));

    config.setBool("RainEffect", cegui_manager.getCheckboxControlChecked(
			cegui_manager.getControl("TabControl/__auto_TabPane__/Video/CheckboxRainEffectGame")));
    config.setBool("RainEffectMenu", cegui_manager.getCheckboxControlChecked(
			cegui_manager.getControl("TabControl/__auto_TabPane__/Video/CheckboxRainEffectMenu")));

    config.setBool("EnableTextureCompression", cegui_manager.getCheckboxControlChecked(
			cegui_manager.getControl("TabControl/__auto_TabPane__/Video/CheckboxTextureCompression")));

    config.setBool("EnableVideos", cegui_manager.getCheckboxControlChecked(
			cegui_manager.getControl("TabControl/__auto_TabPane__/Video/CheckboxVideoPlayback")));

	string currentResolution = config.getString("ScreenWidth") +
			                   "x" +
			                   config.getString("ScreenHeight");

	string selectedResolution = cegui_manager.getSelectedItemFromComboBoxControl(
				cegui_manager.getControl("TabControl/__auto_TabPane__/Video/ComboBoxResolution"));

	if(currentResolution != selectedResolution) {
		for(vector<ModeInfo>::const_iterator it = modeInfos.begin(); it != modeInfos.end(); ++it) {
			if((*it).getString() == selectedResolution) {
				config.setInt("ScreenWidth",(*it).width);
				config.setInt("ScreenHeight",(*it).height);
				config.setInt("ColorBits",(*it).depth);
				break;
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
	console.addLine(lang.getString("SettingsSaved"));
}

}}//end namespace
