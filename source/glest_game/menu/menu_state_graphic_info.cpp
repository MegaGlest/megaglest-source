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

#include "menu_state_graphic_info.h"

#include "renderer.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "menu_state_options.h"
#include "config.h"
#include "opengl.h"
#include "leak_dumper.h"

namespace Glest{ namespace Game{

// =====================================================
// 	class MenuStateGraphicInfo
// =====================================================

MenuStateGraphicInfo::MenuStateGraphicInfo(Program *program, MainMenu *mainMenu):
	MenuState(program, mainMenu, "info")
{
	Lang &lang= Lang::getInstance();

	containerName = "GraphicInfo";
	buttonReturn.registerGraphicComponent(containerName,"buttonReturn");
	buttonReturn.init(100, 540, 125);

	buttonReturn.setText(lang.get("Return"));

	labelInfo.registerGraphicComponent(containerName,"labelInfo");
	labelInfo.init(100, 700);

	labelMoreInfo.registerGraphicComponent(containerName,"labelMoreInfo");
	labelMoreInfo.init(100, 520);
	labelMoreInfo.setFont(CoreData::getInstance().getDisplayFontSmall());
	labelMoreInfo.setFont3D(CoreData::getInstance().getDisplayFontSmall3D());

	labelInternalInfo.registerGraphicComponent(containerName,"labelInternalInfo");
	labelInternalInfo.init(600, 700);
	labelInternalInfo.setFont(CoreData::getInstance().getDisplayFontSmall());
	labelInternalInfo.setFont3D(CoreData::getInstance().getDisplayFontSmall3D());

	GraphicComponent::applyAllCustomProperties(containerName);

	Renderer &renderer= Renderer::getInstance();

	string glInfo= renderer.getGlInfo();
	string glMoreInfo= renderer.getGlMoreInfo();
	labelInfo.setText(glInfo);
	labelMoreInfo.setText(glMoreInfo);

	string strInternalInfo = "";
	strInternalInfo += "VBOSupported: " + boolToStr(getVBOSupported());
	if(getenv("MEGAGLEST_FONT") != NULL) {
		char *tryFont = getenv("MEGAGLEST_FONT");
		strInternalInfo += "\nMEGAGLEST_FONT: " + string(tryFont);
	}
	strInternalInfo += "\nforceLegacyFonts: " + boolToStr(Font::forceLegacyFonts);
	strInternalInfo += "\nrenderText3DEnabled: " + boolToStr(Renderer::renderText3DEnabled);
	strInternalInfo += "\nuseTextureCompression: " + boolToStr(Texture::useTextureCompression);
	strInternalInfo += "\nfontIsRightToLeft: " + boolToStr(Font::fontIsRightToLeft);
	strInternalInfo += "\nscaleFontValue: " + boolToStr(Font::scaleFontValue);
	strInternalInfo += "\nscaleFontValueCenterHFactor: " + boolToStr(Font::scaleFontValueCenterHFactor);
	strInternalInfo += "\nlangHeightText: " + Font::langHeightText;
	strInternalInfo += "\nAllowAltEnterFullscreenToggle: " + boolToStr(Window::getAllowAltEnterFullscreenToggle());
	strInternalInfo += "\nTryVSynch: " + boolToStr(Window::getTryVSynch());
	strInternalInfo += "\nVERBOSE_MODE_ENABLED: " + boolToStr(SystemFlags::VERBOSE_MODE_ENABLED);
	labelInternalInfo.setText(strInternalInfo);
}

void MenuStateGraphicInfo::reloadUI() {
	Lang &lang= Lang::getInstance();

	console.resetFonts();
	buttonReturn.setText(lang.get("Return"));

	labelMoreInfo.setFont(CoreData::getInstance().getDisplayFontSmall());
	labelMoreInfo.setFont3D(CoreData::getInstance().getDisplayFontSmall3D());

	labelInternalInfo.setFont(CoreData::getInstance().getDisplayFontSmall());
	labelInternalInfo.setFont3D(CoreData::getInstance().getDisplayFontSmall3D());

	Renderer &renderer= Renderer::getInstance();

	string glInfo= renderer.getGlInfo();
	string glMoreInfo= renderer.getGlMoreInfo();
	labelInfo.setText(glInfo);
	labelMoreInfo.setText(glMoreInfo);

	string strInternalInfo = "";
	strInternalInfo += "VBOSupported: " + boolToStr(getVBOSupported());
	if(getenv("MEGAGLEST_FONT") != NULL) {
		char *tryFont = getenv("MEGAGLEST_FONT");
		strInternalInfo += "\nMEGAGLEST_FONT: " + string(tryFont);
	}
	strInternalInfo += "\nforceLegacyFonts: " + boolToStr(Font::forceLegacyFonts);
	strInternalInfo += "\nrenderText3DEnabled: " + boolToStr(Renderer::renderText3DEnabled);
	strInternalInfo += "\nuseTextureCompression: " + boolToStr(Texture::useTextureCompression);
	strInternalInfo += "\nfontIsRightToLeft: " + boolToStr(Font::fontIsRightToLeft);
	strInternalInfo += "\nscaleFontValue: " + boolToStr(Font::scaleFontValue);
	strInternalInfo += "\nscaleFontValueCenterHFactor: " + boolToStr(Font::scaleFontValueCenterHFactor);
	strInternalInfo += "\nlangHeightText: " + Font::langHeightText;
	strInternalInfo += "\nAllowAltEnterFullscreenToggle: " + boolToStr(Window::getAllowAltEnterFullscreenToggle());
	strInternalInfo += "\nTryVSynch: " + boolToStr(Window::getTryVSynch());
	strInternalInfo += "\nVERBOSE_MODE_ENABLED: " + boolToStr(SystemFlags::VERBOSE_MODE_ENABLED);
	labelInternalInfo.setText(strInternalInfo);

	GraphicComponent::reloadFontsForRegisterGraphicComponents(containerName);
}

void MenuStateGraphicInfo::mouseClick(int x, int y, MouseButton mouseButton){
	CoreData &coreData= CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();

	if(buttonReturn.mouseClick(x,y)){
		soundRenderer.playFx(coreData.getClickSoundA());
		mainMenu->setState(new MenuStateOptions(program, mainMenu));
    }
}

void MenuStateGraphicInfo::mouseMove(int x, int y, const MouseState *ms){
	buttonReturn.mouseMove(x, y);
}

void MenuStateGraphicInfo::render(){

	Renderer &renderer= Renderer::getInstance();
	Lang &lang= Lang::getInstance();

	renderer.renderButton(&buttonReturn);
	renderer.renderLabel(&labelInfo);
	renderer.renderLabel(&labelInternalInfo);
	renderer.renderLabel(&labelMoreInfo);

	renderer.renderConsole(&console,false,true);
}

void MenuStateGraphicInfo::keyDown(SDL_KeyboardEvent key) {
	Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));
	//if(key == configKeys.getCharKey("SaveGUILayout")) {
	if(isKeyPressed(configKeys.getSDLKey("SaveGUILayout"),key) == true) {
		GraphicComponent::saveAllCustomProperties(containerName);
		//Lang &lang= Lang::getInstance();
		//console.addLine(lang.get("GUILayoutSaved") + " [" + (saved ? lang.get("Yes") : lang.get("No"))+ "]");
	}
}

}}//end namespace
