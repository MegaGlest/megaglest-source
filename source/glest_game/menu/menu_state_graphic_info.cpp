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
#include "leak_dumper.h"

namespace Glest{ namespace Game{

// =====================================================
// 	class MenuStateGraphicInfo
// =====================================================

MenuStateGraphicInfo::MenuStateGraphicInfo(Program *program, MainMenu *mainMenu):
	MenuState(program, mainMenu, "info")
{
	containerName = "GraphicInfo";
	buttonReturn.registerGraphicComponent(containerName,"buttonReturn");
	buttonReturn.init(100, 540, 125);

	labelInfo.registerGraphicComponent(containerName,"labelInfo");
	labelInfo.init(100, 700);

	labelMoreInfo.registerGraphicComponent(containerName,"labelMoreInfo");
	labelMoreInfo.init(100, 520);
	labelMoreInfo.setFont(CoreData::getInstance().getDisplayFontSmall());

	GraphicComponent::applyAllCustomProperties(containerName);

	Renderer &renderer= Renderer::getInstance();
	glInfo= renderer.getGlInfo();
	glMoreInfo= renderer.getGlMoreInfo();
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

	buttonReturn.setText(lang.get("Return"));
	labelInfo.setText(glInfo);
	labelMoreInfo.setText(glMoreInfo);

	renderer.renderButton(&buttonReturn);
	renderer.renderLabel(&labelInfo);
	renderer.renderLabel(&labelMoreInfo);

	renderer.renderConsole(&console,false,true);
}

void MenuStateGraphicInfo::keyDown(char key) {
	Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));
	if(key == configKeys.getCharKey("SaveGUILayout")) {
		bool saved = GraphicComponent::saveAllCustomProperties(containerName);
		//Lang &lang= Lang::getInstance();
		//console.addLine(lang.get("GUILayoutSaved") + " [" + (saved ? lang.get("Yes") : lang.get("No"))+ "]");
	}
}

}}//end namespace
