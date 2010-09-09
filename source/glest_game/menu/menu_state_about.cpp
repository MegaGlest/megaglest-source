// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 MartiC1o Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#include "menu_state_about.h"

#include "renderer.h"
#include "menu_state_root.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "config.h"
#include "menu_state_options.h"

#include "leak_dumper.h"

namespace Glest{ namespace Game{

// =====================================================
// 	class MenuStateAbout
// =====================================================

MenuStateAbout::MenuStateAbout(Program *program, MainMenu *mainMenu) :
		MenuState(program, mainMenu, "about") {

	containerName = "About";
	Lang &lang= Lang::getInstance();
	
	//init
	buttonReturn.registerGraphicComponent(containerName,"buttonReturn");
	buttonReturn.init(460, 100, 125);
	buttonReturn.setText(lang.get("Return"));
	
	for(int i= 0; i<aboutStringCount1; ++i){
		labelAbout1[i].registerGraphicComponent(containerName,"labelAbout1" + intToStr(i));
		labelAbout1[i].init(100, 650-i*20);
		labelAbout1[i].setText(getAboutString1(i));
	}

	for(int i= 0; i<aboutStringCount2; ++i){
		labelAbout2[i].registerGraphicComponent(containerName,"labelAbout2" + intToStr(i));
		labelAbout2[i].init(460, 650-i*20);
		labelAbout2[i].setText(getAboutString2(i));
	}
	
	for(int i= 0; i<teammateCount; ++i){
		labelTeammateName[i].registerGraphicComponent(containerName,"labelTeammateName" + intToStr(i));
		labelTeammateName[i].init(100+i*180, 500);
		labelTeammateRole[i].registerGraphicComponent(containerName,"labelTeammateRole" + intToStr(i));
		labelTeammateRole[i].init(100+i*180, 520);
		labelTeammateName[i].setText(getTeammateName(i));
		labelTeammateRole[i].setText(getTeammateRole(i));
	}

	labelTeammateName[5].init(100, 160);
	labelTeammateRole[5].init(100, 180);
	labelTeammateName[6].init(333, 160);
	labelTeammateRole[6].init(333, 180);
	labelTeammateName[7].init(566, 160);
	labelTeammateRole[7].init(566, 180);
	labelTeammateName[8].init(800, 160);
	labelTeammateRole[8].init(800, 180);

	GraphicComponent::applyAllCustomProperties(containerName);
}

void MenuStateAbout::mouseClick(int x, int y, MouseButton mouseButton){

	CoreData &coreData=  CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();

	if(buttonReturn.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundA());
		mainMenu->setState(new MenuStateRoot(program, mainMenu));
    }     

}

void MenuStateAbout::mouseMove(int x, int y, const MouseState *ms){
	buttonReturn.mouseMove(x, y);
}

void MenuStateAbout::render(){
	Renderer &renderer= Renderer::getInstance();

	renderer.renderButton(&buttonReturn);
	for(int i= 0; i<aboutStringCount1; ++i){
		renderer.renderLabel(&labelAbout1[i]);
	}
	for(int i= 0; i<aboutStringCount2; ++i){
		renderer.renderLabel(&labelAbout2[i]);
	}
	for(int i= 0; i<teammateCount; ++i){
		renderer.renderLabel(&labelTeammateName[i]);
		renderer.renderLabel(&labelTeammateRole[i]);
	}

	if(program != NULL) program->renderProgramMsgBox();

}

void MenuStateAbout::keyDown(char key) {
	Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));
	if(key == configKeys.getCharKey("SaveGUILayout")) {
		bool saved = GraphicComponent::saveAllCustomProperties(containerName);
		//Lang &lang= Lang::getInstance();
		//console.addLine(lang.get("GUILayoutSaved") + " [" + (saved ? lang.get("Yes") : lang.get("No"))+ "]");
	}
}

}}//end namespace
